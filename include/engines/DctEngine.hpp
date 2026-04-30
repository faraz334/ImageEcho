#pragma once
// ============================================================
// DctEngine.hpp  (fixed normalisation)
// ============================================================
// The DCT formula requires a normalisation factor of sqrt(2/N)
// on every 1D pass. Without it, the inverse DCT produces values
// that are way off — which is what caused the 87-243 delta bug.
//
// Fixed formula for each 1D DCT pass:
//   F(u) = alpha(u) * sqrt(2/N) * SUM[ f(x) * cos(...) ]
//
// And for each 1D inverse DCT pass:
//   f(x) = sqrt(2/N) * SUM[ alpha(u) * F(u) * cos(...) ]
//
// With N=8: sqrt(2/8) = 0.5  (that's the missing factor)
// ============================================================

#include "../IEchoEngine.hpp"
#include <cmath>
#include <array>
#include <algorithm>
#include <cstdint>

class DctEngine final : public IEchoEngine {
public:

    std::string name() const override {
        return "DCT Engine";
    }

    PerturbationReport apply(
        const ImageBuffer& original,
        ImageBuffer&       target,
        float              epsilon
    ) const override {

        const Dimensions& d = original.dims();

        float totalDelta    = 0.0f;
        float maxDelta      = 0.0f;
        int   pixelsChanged = 0;

        for (int channel = 0; channel < d.channels; ++channel) {
            for (int blockY = 0; blockY + BLOCK_SIZE <= d.height; blockY += BLOCK_SIZE) {
                for (int blockX = 0; blockX + BLOCK_SIZE <= d.width; blockX += BLOCK_SIZE) {
                    processBlock(
                        original, target,
                        blockX, blockY, channel,
                        epsilon,
                        totalDelta, maxDelta, pixelsChanged
                    );
                }
            }
        }

        PerturbationReport report;
        report.engineName    = name();
        report.epsilon       = epsilon;
        report.meanDelta     = pixelsChanged > 0
                             ? totalDelta / static_cast<float>(pixelsChanged)
                             : 0.0f;
        report.maxDelta      = maxDelta;
        report.pixelsAltered = pixelsChanged;
        return report;
    }

private:

    static constexpr int BLOCK_SIZE = 8;
    using Block = std::array<double, BLOCK_SIZE * BLOCK_SIZE>;

    void processBlock(
        const ImageBuffer& src,
        ImageBuffer&       dst,
        int blockX, int blockY, int channel,
        float epsilon,
        float& totalDelta, float& maxDelta, int& pixelsChanged
    ) const {

        // Step 1: Extract pixels, normalise to [0.0, 1.0]
        Block spatial{};
        for (int row = 0; row < BLOCK_SIZE; ++row)
            for (int col = 0; col < BLOCK_SIZE; ++col)
                spatial[row * BLOCK_SIZE + col] =
                    src.at(blockX + col, blockY + row, channel) / 255.0;

        // Step 2: Forward DCT
        Block freq = forwardDCT(spatial);

        // Step 3: Inject perturbation into HIGH-frequency coefficients
        // Only touch positions where (row + col) >= BLOCK_SIZE
        // These are the invisible high-frequency components
        const double eps = static_cast<double>(epsilon);
        for (int row = 0; row < BLOCK_SIZE; ++row)
            for (int col = 0; col < BLOCK_SIZE; ++col)
                if (row + col >= BLOCK_SIZE) {
                    double sign = ((row + col) % 2 == 0) ? 1.0 : -1.0;
                    freq[row * BLOCK_SIZE + col] += sign * eps;
                }

        // Step 4: Inverse DCT
        Block result = inverseDCT(freq);

        // Step 5: Write back to target, clamped to [0, 255]
        for (int row = 0; row < BLOCK_SIZE; ++row) {
            for (int col = 0; col < BLOCK_SIZE; ++col) {

                double perturbedPixel = result[row * BLOCK_SIZE + col] * 255.0;
                uint8_t clamped = static_cast<uint8_t>(
                    std::clamp(perturbedPixel, 0.0, 255.0)
                );

                uint8_t originalVal = src.at(blockX + col, blockY + row, channel);
                float delta = std::abs(
                    static_cast<float>(clamped) - static_cast<float>(originalVal)
                );

                totalDelta += delta;
                if (delta > maxDelta) maxDelta = delta;
                if (delta > 0.0f)    pixelsChanged++;

                dst.at(blockX + col, blockY + row, channel) = clamped;
            }
        }
    }

    // ── forwardDCT ───────────────────────────────────────────
    // THE FIX: multiply each output by sqrt(2.0 / BLOCK_SIZE)
    // For N=8 this equals 0.5 — without it values blow up.
    static Block forwardDCT(const Block& input) {
        // Normalisation factor per 1D pass
        const double norm = std::sqrt(2.0 / BLOCK_SIZE);  // = 0.5 for N=8

        Block temp{};
        Block output{};

        // Row-wise 1D DCT
        for (int row = 0; row < BLOCK_SIZE; ++row) {
            for (int u = 0; u < BLOCK_SIZE; ++u) {
                double sum = 0.0;
                for (int x = 0; x < BLOCK_SIZE; ++x)
                    sum += input[row * BLOCK_SIZE + x]
                         * std::cos(M_PI * u * (2.0*x + 1.0) / (2.0 * BLOCK_SIZE));
                temp[row * BLOCK_SIZE + u] = alphaFactor(u) * norm * sum;
            }
        }

        // Column-wise 1D DCT
        for (int col = 0; col < BLOCK_SIZE; ++col) {
            for (int v = 0; v < BLOCK_SIZE; ++v) {
                double sum = 0.0;
                for (int y = 0; y < BLOCK_SIZE; ++y)
                    sum += temp[y * BLOCK_SIZE + col]
                         * std::cos(M_PI * v * (2.0*y + 1.0) / (2.0 * BLOCK_SIZE));
                output[v * BLOCK_SIZE + col] = alphaFactor(v) * norm * sum;
            }
        }

        return output;
    }

    // ── inverseDCT ───────────────────────────────────────────
    // THE FIX: same sqrt(2/N) factor on each 1D inverse pass
    static Block inverseDCT(const Block& input) {
        const double norm = std::sqrt(2.0 / BLOCK_SIZE);  // = 0.5 for N=8

        Block temp{};
        Block output{};

        // Row-wise inverse 1D DCT
        for (int row = 0; row < BLOCK_SIZE; ++row) {
            for (int x = 0; x < BLOCK_SIZE; ++x) {
                double sum = 0.0;
                for (int u = 0; u < BLOCK_SIZE; ++u)
                    sum += alphaFactor(u)
                         * input[row * BLOCK_SIZE + u]
                         * std::cos(M_PI * u * (2.0*x + 1.0) / (2.0 * BLOCK_SIZE));
                temp[row * BLOCK_SIZE + x] = norm * sum;
            }
        }

        // Column-wise inverse 1D DCT
        for (int col = 0; col < BLOCK_SIZE; ++col) {
            for (int y = 0; y < BLOCK_SIZE; ++y) {
                double sum = 0.0;
                for (int v = 0; v < BLOCK_SIZE; ++v)
                    sum += alphaFactor(v)
                         * temp[v * BLOCK_SIZE + col]
                         * std::cos(M_PI * v * (2.0*y + 1.0) / (2.0 * BLOCK_SIZE));
                output[y * BLOCK_SIZE + col] = norm * sum;
            }
        }

        return output;
    }

    // alpha(0) = 1/sqrt(2), alpha(k>0) = 1
    static double alphaFactor(int k) {
        return (k == 0) ? (1.0 / std::sqrt(2.0)) : 1.0;
    }
};
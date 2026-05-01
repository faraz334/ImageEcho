#pragma once
// ============================================================
// PgdEngine.hpp
// ============================================================
// PURPOSE: Projected Gradient Descent (PGD) perturbation.
//
// PGD is the strongest attack in our collection.
// It was introduced by Madry et al. (2018) and is considered
// the "gold standard" of adversarial attacks.
//
// HOW IT DIFFERS FROM FGSM:
//   FGSM: one big step of size epsilon in the gradient direction
//   PGD:  many small steps, each of size (epsilon / numSteps)
//         After EVERY step, project back into the epsilon-ball
//
// WHAT IS "PROJECTING INTO THE EPSILON-BALL"?
//   After each small step, we check: has any pixel moved MORE
//   than epsilon from its original value?
//   If yes → clip it back to original ± epsilon.
//   This "projection" keeps the perturbation within budget.
//
//   ANALOGY: Imagine you're allowed to move at most 5 steps
//   from your starting point. PGD takes many tiny steps but
//   always checks "am I still within 5 steps?" after each one.
//   FGSM just takes one big 5-step jump.
//
// WHY IS PGD STRONGER?
//   Multiple steps explore the local neighbourhood more
//   thoroughly. FGSM might miss a better direction.
//   PGD finds the worst-case perturbation within the budget.
//
// PARAMETERS:
//   epsilon   — max total perturbation per pixel (L∞ budget)
//   numSteps  — how many iterations (default: 10)
//   stepSize  — how big each step is (default: epsilon/4)
//               smaller = more precise but slower
// ============================================================

#include "../IEchoEngine.hpp"
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <vector>

class PgdEngine final : public IEchoEngine {
public:

    // Constructor — lets you customise number of steps
    // Default: 10 steps (good balance of quality vs speed)
    explicit PgdEngine(int numSteps = 10)
        : numSteps_(numSteps)
    {}

    std::string name() const override {
        return "PGD Engine";
    }

    PerturbationReport apply(
        const ImageBuffer& original,
        ImageBuffer&       target,
        float              epsilon
    ) const override {

        const Dimensions& d = original.dims();
        const int totalBytes = d.byteCount();

        // Step size = epsilon / 4
        // Small enough for precision, large enough to converge quickly
        const double eps      = static_cast<double>(epsilon) * 255.0;
        const double stepSize = eps / 4.0;

        // ── Working buffer ────────────────────────────────────
        // We store pixel values as doubles during iteration
        // so we don't lose precision from repeated uint8 rounding.
        // At the end we convert back to uint8.
        std::vector<double> current(totalBytes);

        // Initialise current = original pixel values
        for (int i = 0; i < totalBytes; ++i) {
            int channel = i % d.channels;
            int pixelIdx = i / d.channels;
            int x = pixelIdx % d.width;
            int y = pixelIdx / d.width;
            current[i] = static_cast<double>(original.at(x, y, channel));
        }

        // ── PGD iteration loop ────────────────────────────────
        for (int step = 0; step < numSteps_; ++step) {

            // ── Step A: Compute gradient (Sobel approximation) ─
            // Same idea as FGSM — Sobel detects the direction
            // of maximum intensity change (approximates ML gradient)
            for (int c = 0; c < d.channels; ++c) {
                for (int y = 0; y < d.height; ++y) {
                    for (int x = 0; x < d.width; ++x) {

                        int idx = (y * d.width + x) * d.channels + c;

                        // Compute Sobel gradient from CURRENT (not original)
                        // This is what makes PGD iterative — each step
                        // builds on the previous perturbation
                        double gx = sobelX(current, d, x, y, c);
                        double gy = sobelY(current, d, x, y, c);
                        double sign = ((gx + gy) >= 0.0) ? 1.0 : -1.0;

                        // ── Step B: Take a small step ─────────
                        current[idx] += sign * stepSize;

                        // ── Step C: Project into epsilon-ball ─
                        // Clip to [original - eps, original + eps]
                        // This is the "projection" step.
                        // It ensures we never exceed our perturbation budget.
                        double originalVal = static_cast<double>(
                            original.at(x, y, c)
                        );
                        current[idx] = std::clamp(
                            current[idx],
                            originalVal - eps,   // lower bound
                            originalVal + eps    // upper bound
                        );

                        // Also clip to valid pixel range [0, 255]
                        current[idx] = std::clamp(current[idx], 0.0, 255.0);
                    }
                }
            }
        }

        // ── Write final result to target buffer ──────────────
        float totalDelta    = 0.0f;
        float maxDelta      = 0.0f;
        int   pixelsChanged = 0;

        for (int c = 0; c < d.channels; ++c) {
            for (int y = 0; y < d.height; ++y) {
                for (int x = 0; x < d.width; ++x) {

                    int idx = (y * d.width + x) * d.channels + c;
                    uint8_t clamped = static_cast<uint8_t>(
                        std::clamp(current[idx], 0.0, 255.0)
                    );

                    float delta = std::abs(
                        static_cast<float>(clamped) -
                        static_cast<float>(original.at(x, y, c))
                    );

                    totalDelta += delta;
                    if (delta > maxDelta) maxDelta = delta;
                    if (delta > 0.0f)    pixelsChanged++;

                    target.at(x, y, c) = clamped;
                }
            }
        }

        int total = d.totalPixels() * d.channels;

        PerturbationReport report;
        report.engineName    = name();
        report.epsilon       = epsilon;
        report.meanDelta     = totalDelta / static_cast<float>(total);
        report.maxDelta      = maxDelta;
        report.pixelsAltered = pixelsChanged;
        return report;
    }

private:

    int numSteps_;  // number of PGD iterations

    // ── sobelX() — horizontal gradient from a double buffer ──
    // Same as FgsmEngine but reads from std::vector<double>
    // instead of ImageBuffer (because during iteration we work
    // in double precision)
    double sobelX(
        const std::vector<double>& buf,
        const Dimensions& d,
        int x, int y, int c
    ) const {
        auto px = [&](int xi, int yi) -> double {
            int cx = std::clamp(xi, 0, d.width  - 1);
            int cy = std::clamp(yi, 0, d.height - 1);
            return buf[(cy * d.width + cx) * d.channels + c];
        };

        return -1.0*px(x-1,y-1) + 1.0*px(x+1,y-1)
               -2.0*px(x-1,y  ) + 2.0*px(x+1,y  )
               -1.0*px(x-1,y+1) + 1.0*px(x+1,y+1);
    }

    // ── sobelY() — vertical gradient from a double buffer ────
    double sobelY(
        const std::vector<double>& buf,
        const Dimensions& d,
        int x, int y, int c
    ) const {
        auto px = [&](int xi, int yi) -> double {
            int cx = std::clamp(xi, 0, d.width  - 1);
            int cy = std::clamp(yi, 0, d.height - 1);
            return buf[(cy * d.width + cx) * d.channels + c];
        };

        return -1.0*px(x-1,y-1) - 2.0*px(x,y-1) - 1.0*px(x+1,y-1)
               +1.0*px(x-1,y+1) + 2.0*px(x,y+1) + 1.0*px(x+1,y+1);
    }
};
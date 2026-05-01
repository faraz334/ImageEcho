#pragma once
// ============================================================
// FgsmEngine.hpp
// ============================================================
// PURPOSE: Fast Gradient Sign Method (FGSM) perturbation.
//
// BACKGROUND:
//   In real adversarial ML, FGSM computes:
//     perturbation = epsilon * sign( gradient of loss w.r.t. input )
//
//   The "gradient of loss w.r.t. input" tells us:
//   "which direction should each pixel move to maximally
//    increase the model's error?"
//
//   We don't have a real ML model here, so we APPROXIMATE
//   the gradient using a Sobel filter.
//
// WHAT IS A SOBEL FILTER?
//   A Sobel filter detects edges in an image by computing
//   how quickly pixel values change in X and Y directions.
//   CNNs use similar edge detectors in their first layers
//   to recognise shapes — so Sobel gradients are a good
//   approximation of what a CNN would respond to.
//
// SOBEL KERNELS (3x3 convolution):
//
//   Horizontal (Gx):     Vertical (Gy):
//   -1  0  +1           -1  -2  -1
//   -2  0  +2            0   0   0
//   -1  0  +1           +1  +2  +1
//
//   Combined gradient magnitude: G = sqrt(Gx^2 + Gy^2)
//   Gradient sign: sign(Gx + Gy) = +1 or -1
//
// PERTURBATION FORMULA:
//   perturbed_pixel = original_pixel + epsilon * sign(gradient)
//
// This is a single-step attack — stronger than LSB,
// weaker than PGD (which is multi-step).
// ============================================================

#include "../IEchoEngine.hpp"
#include <cmath>
#include <algorithm>
#include <cstdint>

class FgsmEngine final : public IEchoEngine {
public:

    std::string name() const override {
        return "FGSM Engine";
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

        // Process each channel independently
        for (int c = 0; c < d.channels; ++c) {
            for (int y = 0; y < d.height; ++y) {
                for (int x = 0; x < d.width; ++x) {

                    // ── Compute Sobel gradient at (x, y) ─────
                    // We convolve the 3x3 Sobel kernels around pixel (x,y).
                    // For edge pixels (x=0, y=0 etc.) we clamp coordinates
                    // to stay inside the image — this is called "clamp padding".
                    double gx = computeSobelX(original, x, y, c);
                    double gy = computeSobelY(original, x, y, c);

                    // Combined gradient: sum of both directions
                    double gradient = gx + gy;

                    // sign() returns +1.0 if gradient > 0, -1.0 otherwise
                    // This is the key FGSM step: we only care about DIRECTION
                    // not magnitude. We always perturb by exactly epsilon.
                    double sign = (gradient >= 0.0) ? 1.0 : -1.0;

                    // ── Apply perturbation ────────────────────
                    double original_val  = static_cast<double>(original.at(x, y, c));
                    double perturbed_val = original_val + sign * static_cast<double>(epsilon) * 255.0;

                    // Clamp to valid pixel range [0, 255]
                    uint8_t clamped = static_cast<uint8_t>(
                        std::clamp(perturbed_val, 0.0, 255.0)
                    );

                    // Track statistics
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

    // ── computeSobelX() ──────────────────────────────────────
    // Applies the horizontal Sobel kernel at position (x, y).
    // Detects vertical edges (left-right intensity changes).
    //
    // Kernel:  -1  0  +1
    //          -2  0  +2
    //          -1  0  +1
    double computeSobelX(const ImageBuffer& img, int x, int y, int c) const {
        const Dimensions& d = img.dims();

        // clampX / clampY keep coordinates inside the image bounds
        // so we don't read pixels outside the image
        auto clampX = [&](int v) { return std::clamp(v, 0, d.width  - 1); };
        auto clampY = [&](int v) { return std::clamp(v, 0, d.height - 1); };

        double gx =
            -1.0 * img.at(clampX(x-1), clampY(y-1), c) +
             1.0 * img.at(clampX(x+1), clampY(y-1), c) +
            -2.0 * img.at(clampX(x-1), clampY(y  ), c) +
             2.0 * img.at(clampX(x+1), clampY(y  ), c) +
            -1.0 * img.at(clampX(x-1), clampY(y+1), c) +
             1.0 * img.at(clampX(x+1), clampY(y+1), c);

        return gx;
    }

    // ── computeSobelY() ──────────────────────────────────────
    // Applies the vertical Sobel kernel at position (x, y).
    // Detects horizontal edges (top-bottom intensity changes).
    //
    // Kernel:  -1  -2  -1
    //           0   0   0
    //          +1  +2  +1
    double computeSobelY(const ImageBuffer& img, int x, int y, int c) const {
        const Dimensions& d = img.dims();

        auto clampX = [&](int v) { return std::clamp(v, 0, d.width  - 1); };
        auto clampY = [&](int v) { return std::clamp(v, 0, d.height - 1); };

        double gy =
            -1.0 * img.at(clampX(x-1), clampY(y-1), c) +
            -2.0 * img.at(clampX(x  ), clampY(y-1), c) +
            -1.0 * img.at(clampX(x+1), clampY(y-1), c) +
             1.0 * img.at(clampX(x-1), clampY(y+1), c) +
             2.0 * img.at(clampX(x  ), clampY(y+1), c) +
             1.0 * img.at(clampX(x+1), clampY(y+1), c);

        return gy;
    }
};
#pragma once
// ============================================================
// SsimAnalyzer.hpp
// ============================================================
// PURPOSE: Measure how "visually similar" two images are.
//
// WHY NOT JUST COMPARE PIXELS DIRECTLY?
//   Simple pixel difference (mean absolute error) doesn't match
//   how humans perceive images. A 10-unit change in a bright
//   area looks different than a 10-unit change in a dark area.
//   SSIM accounts for local structure, contrast, and luminance.
//
// SSIM FORMULA (simplified):
//   SSIM(x, y) = (luminance similarity)
//              × (contrast similarity)
//              × (structure similarity)
//
//   Each component is close to 1.0 when the images are similar.
//   The final SSIM score is their product.
//
// PSNR (Peak Signal-to-Noise Ratio):
//   Another metric, measured in decibels (dB).
//   Higher = more similar. > 40 dB = excellent quality.
//   Formula: PSNR = 10 * log10(255^2 / MSE)
//   where MSE = mean squared error between pixels.
//
// HOW TO READ THE RESULTS:
//   SSIM > 0.99  = virtually identical
//   SSIM > 0.95  = perceptually identical (our target)
//   SSIM > 0.90  = slight degradation, mostly invisible
//   SSIM < 0.90  = visible difference
//
//   PSNR > 40 dB = excellent (our target)
//   PSNR > 30 dB = acceptable
//   PSNR < 30 dB = noticeable degradation
// ============================================================

#include "ImageBuffer.hpp"
#include <cmath>    // std::sqrt, std::log10, std::abs
#include <cstdio>   // printf

class SsimAnalyzer {
public:

    // ── Result struct ────────────────────────────────────────
    // Holds all metrics from one comparison.
    struct Result {
        float ssim     = 0.0f;   // 0.0 to 1.0  (higher = more similar)
        float psnr     = 0.0f;   // in dB        (higher = more similar)
        float maxDelta = 0.0f;   // max pixel difference found anywhere

        // Is the perturbation considered "invisible"?
        bool isInvisible() const {
            return ssim >= 0.95f && psnr >= 40.0f;
        }

        // Print a formatted summary
        void printToConsole() const {
            printf("┌─────────────────────────────────────┐\n");
            printf("│        Image Quality Metrics         │\n");
            printf("├─────────────────────────────────────┤\n");
            printf("│ SSIM:      %-6.4f  (target: >0.95) │\n", ssim);
            printf("│ PSNR:      %-6.2f dB (target: >40) │\n", psnr);
            printf("│ Max delta: %-6.1f  (target: <10)   │\n", maxDelta);
            printf("├─────────────────────────────────────┤\n");
            if (isInvisible()) {
                printf("│ ✓ Perturbation is INVISIBLE         │\n");
            } else {
                printf("│ ✗ Perturbation may be visible        │\n");
            }
            printf("└─────────────────────────────────────┘\n");
        }
    };

    // ── compute() ───────────────────────────────────────────
    // Compares two images and returns SSIM + PSNR metrics.
    // Both images must have the same dimensions.
    //
    // Usage:
    //   auto result = SsimAnalyzer::compute(original, perturbed);
    //   std::cout << result.ssim;  // e.g. 0.9821
    //
    static Result compute(const ImageBuffer& imageA, const ImageBuffer& imageB) {

        const Dimensions& d = imageA.dims();
        const int N = d.totalPixels();   // total pixel count

        double ssimSum  = 0.0;   // we'll average SSIM across channels
        double mseSum   = 0.0;   // mean squared error (for PSNR)
        float  maxDelta = 0.0f;

        // ── Process each channel separately ──────────────────
        // We compute SSIM per channel (R, G, B) then average them.
        for (int c = 0; c < d.channels; ++c) {

            // ── Step 1: Compute means (mu) ────────────────────
            // Mean = average pixel value across the whole channel
            double muA = 0.0, muB = 0.0;
            for (int y = 0; y < d.height; ++y) {
                for (int x = 0; x < d.width; ++x) {
                    muA += imageA.at(x, y, c);
                    muB += imageB.at(x, y, c);
                }
            }
            muA /= N;
            muB /= N;

            // ── Step 2: Compute variances and covariance ──────
            // Variance = how spread out the values are
            // Covariance = how similarly the two images vary together
            double varA = 0.0, varB = 0.0, cov = 0.0;
            for (int y = 0; y < d.height; ++y) {
                for (int x = 0; x < d.width; ++x) {

                    double diffA = imageA.at(x, y, c) - muA;
                    double diffB = imageB.at(x, y, c) - muB;

                    varA += diffA * diffA;
                    varB += diffB * diffB;
                    cov  += diffA * diffB;

                    // Also accumulate MSE and max delta for PSNR
                    double pixelDiff = std::abs(
                        static_cast<double>(imageA.at(x, y, c)) -
                        static_cast<double>(imageB.at(x, y, c))
                    );
                    mseSum += pixelDiff * pixelDiff;
                    if (pixelDiff > maxDelta) maxDelta = static_cast<float>(pixelDiff);
                }
            }
            varA /= N;
            varB /= N;
            cov  /= N;

            // ── Step 3: Compute SSIM for this channel ─────────
            // C1 and C2 are small stability constants that prevent
            // division by zero when the variance is very small.
            // Standard values: C1=(0.01*L)^2, C2=(0.03*L)^2, L=255
            constexpr double C1 = (0.01 * 255.0) * (0.01 * 255.0);  // 6.5025
            constexpr double C2 = (0.03 * 255.0) * (0.03 * 255.0);  // 58.5225

            // SSIM formula:
            // (2*muA*muB + C1) * (2*cov + C2)
            // ─────────────────────────────────────────────────
            // (muA^2 + muB^2 + C1) * (varA + varB + C2)
            double numerator   = (2.0*muA*muB + C1) * (2.0*cov + C2);
            double denominator = (muA*muA + muB*muB + C1) * (varA + varB + C2);

            ssimSum += numerator / denominator;
        }

        // ── Average SSIM across all channels ─────────────────
        double ssimAvg = ssimSum / d.channels;

        // ── Compute PSNR from MSE ─────────────────────────────
        double mse = mseSum / (static_cast<double>(N) * d.channels);
        double psnr;
        if (mse < 1e-10) {
            psnr = 100.0;   // images are identical — set to high value
        } else {
            psnr = 10.0 * std::log10((255.0 * 255.0) / mse);
        }

        // ── Build and return result ───────────────────────────
        Result result;
        result.ssim     = static_cast<float>(ssimAvg);
        result.psnr     = static_cast<float>(psnr);
        result.maxDelta = maxDelta;
        return result;
    }
};
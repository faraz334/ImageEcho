#pragma once
// ============================================================
// EchoContext.hpp  (updated for Day 4)
// ============================================================
// NEW: runOptimal() — automatically finds the highest epsilon
//      that keeps SSIM above your target threshold.
//
// WHY DO WE NEED THIS?
//   Different images tolerate different levels of perturbation.
//   A noisy photo can handle more perturbation than a flat
//   solid-colour image before the SSIM drops below 0.95.
//
//   Instead of hardcoding epsilon = 8/255, runOptimal() uses
//   binary search to find the MAXIMUM epsilon that keeps
//   the image looking good (SSIM >= threshold).
//
// HOW BINARY SEARCH WORKS HERE:
//   We have a range [low=0, high=50/255].
//   We try the middle value (mid).
//   If SSIM >= threshold → mid is safe, try higher (lo = mid)
//   If SSIM <  threshold → mid is too much, try lower (hi = mid)
//   Repeat 16 times → converges to the optimal epsilon.
//
//   It's like guessing a number between 1-100:
//   "Is it 50?" Too high → "Is it 25?" etc.
// ============================================================

#include "IEchoEngine.hpp"
#include "SsimAnalyzer.hpp"
#include "ImageBuffer.hpp"
#include "PerturbationReport.hpp"
#include <memory>
#include <stdexcept>
#include <cstdio>   // printf

class EchoContext {
public:

    explicit EchoContext(std::unique_ptr<IEchoEngine> engine)
        : engine_(std::move(engine))
    {
        if (!engine_) {
            throw std::runtime_error("EchoContext: engine cannot be null");
        }
    }

    void setEngine(std::unique_ptr<IEchoEngine> newEngine) {
        if (!newEngine) {
            throw std::runtime_error("EchoContext: cannot set a null engine");
        }
        engine_ = std::move(newEngine);
    }

    // ── run() — apply with a fixed epsilon ──────────────────
    PerturbationReport run(
        const ImageBuffer& original,
        ImageBuffer&       target,
        float              epsilon = 8.0f / 255.0f
    ) const {
        return engine_->apply(original, target, epsilon);
    }

    // ── runOptimal() — auto-find the best epsilon ────────────
    // Finds the highest epsilon that keeps SSIM >= ssimTarget.
    // Returns the perturbation report for the optimal result.
    //
    // Parameters:
    //   original    — source image (never modified)
    //   target      — output image (written with optimal perturbation)
    //   ssimTarget  — minimum acceptable SSIM (default 0.95)
    //   maxEpsilon  — upper search bound (default 50/255 ≈ 0.196)
    //   verbose     — if true, prints each search step
    //
    PerturbationReport runOptimal(
        const ImageBuffer& original,
        ImageBuffer&       target,
        float              ssimTarget  = 0.95f,
        float              maxEpsilon  = 50.0f / 255.0f,
        bool               verbose     = false
    ) const {
        float lo       = 0.0f;
        float hi       = maxEpsilon;
        float bestEps  = 0.0f;

        if (verbose) {
            printf("  Searching for optimal epsilon (SSIM target: %.2f)...\n", ssimTarget);
        }

        // ── 16 iterations of binary search ───────────────────
        // 16 iterations gives us precision of maxEpsilon / 2^16
        // which is more than accurate enough.
        for (int iteration = 0; iteration < 16; ++iteration) {

            float mid = (lo + hi) / 2.0f;

            // Try this epsilon value on a temporary clone
            ImageBuffer temp = original.clone();
            engine_->apply(original, temp, mid);

            // Measure SSIM of the result
            SsimAnalyzer::Result metrics = SsimAnalyzer::compute(original, temp);

            if (verbose) {
                printf("    iter %2d: epsilon=%.5f  SSIM=%.4f  %s\n",
                    iteration + 1, mid, metrics.ssim,
                    metrics.ssim >= ssimTarget ? "✓ safe" : "✗ too much");
            }

            if (metrics.ssim >= ssimTarget) {
                // This epsilon is safe — try pushing higher
                bestEps = mid;
                lo = mid;
            } else {
                // Too much perturbation — pull back
                hi = mid;
            }
        }

        if (verbose) {
            printf("  Optimal epsilon found: %.5f\n\n", bestEps);
        }

        // ── Apply with the best found epsilon ─────────────────
        return engine_->apply(original, target, bestEps);
    }

    std::string activeEngineName() const {
        return engine_->name();
    }

private:
    std::unique_ptr<IEchoEngine> engine_;
};
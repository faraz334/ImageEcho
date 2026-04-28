
#pragma once
// ============================================================
// LsbEngine.hpp
// ============================================================
// PURPOSE: Implements the LSB (Least Significant Bit) perturbation.
//
// WHAT IS LSB?
//   Every pixel channel is a number from 0 to 255.
//   In binary, 255 looks like: 1 1 1 1 1 1 1 1
//                               ↑               ↑
//                         most important    least important
//                           (big change)     (tiny change)
//
//   Flipping the least significant bits changes the value by 1-3,
//   which is completely invisible to the human eye but still
//   alters the data that a machine learning model sees.
//
// EXAMPLE:
//   Original pixel red channel: 200  (binary: 11001000)
//   After LSB flip:             201  (binary: 11001001)
//   Difference: 1 — completely invisible!
//
// This is the simplest adversarial perturbation technique.
// It's not the strongest (that's DCT, coming Day 3) but it's
// a great starting point to understand the concept.
//
// 'final' means no class can inherit from LsbEngine.
// ============================================================

#include "../IEchoEngine.hpp"   // the interface we're implementing
#include <cmath>                // std::abs
#include <algorithm>            // std::clamp

class LsbEngine final : public IEchoEngine {
public:

    // ── name() ──────────────────────────────────────────────
    std::string name() const override {
        return "LSB Engine";
    }

    // ── apply() ─────────────────────────────────────────────
    // Flips the bottom 1-2 bits of every pixel channel.
    // The epsilon value controls HOW MANY bits we flip.
    //
    // epsilon < 0.02  → flip only the last 1 bit (change of 1)
    // epsilon >= 0.02 → flip the last 2 bits    (change of up to 3)
    PerturbationReport apply(
        const ImageBuffer& original,
        ImageBuffer&       target,
        float              epsilon
    ) const override {

        const Dimensions& d = original.dims();

        // How many bits to flip based on epsilon budget
        // epsilon of 8/255 ≈ 0.031, so default uses 2-bit mask
        int bitMask = (epsilon >= 2.0f / 255.0f) ? 0b00000011   // flip bottom 2 bits
                                                  : 0b00000001;  // flip bottom 1 bit

        // Tracking variables for the report
        float totalDelta   = 0.0f;
        float maxDelta     = 0.0f;
        int   pixelsChanged = 0;

        // ── Loop through every pixel, every channel ──────────
        // x = column (horizontal position)
        // y = row    (vertical position)
        // c = channel (0=Red, 1=Green, 2=Blue)
        for (int y = 0; y < d.height; ++y) {
            for (int x = 0; x < d.width; ++x) {
                for (int c = 0; c < d.channels; ++c) {

                    uint8_t originalValue = original.at(x, y, c);

                    // XOR flips the bits specified by bitMask.
                    // XOR with 1 flips that bit: 0→1, 1→0
                    // Example: 200 XOR 3 = 11001000 XOR 00000011 = 11001011 = 203
                    uint8_t perturbedValue = originalValue ^ static_cast<uint8_t>(bitMask);

                    // Calculate how much this pixel changed
                    float delta = std::abs(
                        static_cast<float>(perturbedValue) -
                        static_cast<float>(originalValue)
                    );

                    // Write the perturbed value into target
                    target.at(x, y, c) = perturbedValue;

                    // Update statistics
                    totalDelta += delta;
                    if (delta > maxDelta) maxDelta = delta;
                    if (delta > 0)        pixelsChanged++;
                }
            }
        }

        // ── Build and return the report ───────────────────────
        int totalValues = d.totalPixels() * d.channels;

        PerturbationReport report;
        report.engineName    = name();
        report.epsilon       = epsilon;
        report.meanDelta     = totalDelta / static_cast<float>(totalValues);
        report.maxDelta      = maxDelta;
        report.pixelsAltered = pixelsChanged;
        return report;
    }
};
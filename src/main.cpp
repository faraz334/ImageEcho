// ============================================================
// main.cpp  —  Day 3
// ============================================================
// Today we:
//   1. Test the DCT engine on a small block (verify math)
//   2. Run DCT on a real image
//   3. Compare LSB vs DCT side by side
//   4. Demonstrate strategy hot-swap (change engine at runtime)
// ============================================================

#include "ImageBuffer.hpp"
#include "ImageIO.hpp"
#include "EchoContext.hpp"
#include "engines/LsbEngine.hpp"
#include "engines/DctEngine.hpp"

#include <iostream>
#include <memory>   // std::make_unique
#include <cmath>    // std::abs

// ── Helper: print test result ────────────────────────────────
void check(const std::string& label, bool passed) {
    std::cout << (passed ? "  [PASS]  " : "  [FAIL]  ") << label << "\n";
}

// ── Test 1: DCT round-trip (forward then inverse = same image)
// This verifies our DCT math is correct.
// If we do forward DCT then inverse DCT WITHOUT injecting noise,
// we should get back exactly the same pixel values.
void test_dctRoundTrip() {
    std::cout << "\n--- Test: DCT round-trip accuracy ---\n";
    std::cout << "  (If forward+inverse DCT gives back the same image,\n";
    std::cout << "   our math is correct)\n";

    // Create a small image with known values
    ImageBuffer original(16, 16, 1);  // 16x16, grayscale

    // Fill with a gradient pattern (varied values to stress-test DCT)
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
            original.at(x, y, 0) = static_cast<uint8_t>((x * 16 + y * 8) % 256);

    ImageBuffer target = original.clone();

    // Run DCT with epsilon = 0 (no perturbation — pure round-trip test)
    EchoContext ctx(std::make_unique<DctEngine>());
    ctx.run(original, target, 0.0f);

    // Check that pixels came back close to original (within 2 units)
    // Small rounding error is normal with floating point math
    int badPixels = 0;
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            int diff = std::abs(
                static_cast<int>(original.at(x, y, 0)) -
                static_cast<int>(target.at(x, y, 0))
            );
            if (diff > 2) badPixels++;
        }
    }

    check("round-trip error is near zero", badPixels == 0);
    std::cout << "  Pixels with error > 2: " << badPixels << " / 256\n";
}

// ── Test 2: DCT actually changes pixels ──────────────────────
void test_dctChangesPixels() {
    std::cout << "\n--- Test: DCT changes pixels ---\n";

    ImageBuffer original(16, 16, 3);

    // Fill with value 128 (mid-grey)
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
            for (int c = 0; c < 3; ++c)
                original.at(x, y, c) = 128;

    ImageBuffer target = original.clone();

    EchoContext ctx(std::make_unique<DctEngine>());
    PerturbationReport report = ctx.run(original, target, 8.0f / 255.0f);

    check("pixels were changed",      report.pixelsAltered > 0);
    check("original still 128",       original.at(0, 0, 0) == 128);
    check("mean delta is small (<5)", report.meanDelta < 5.0f);

    std::cout << "  Pixels altered: " << report.pixelsAltered << "\n";
    std::cout << "  Mean delta:     " << report.meanDelta << "\n";
    std::cout << "  Max delta:      " << report.maxDelta  << "\n";
}

// ── Test 3: Run DCT on a real image ──────────────────────────
void test_dctRealImage() {
    std::cout << "\n--- Test: DCT on real image ---\n";

    try {
        ImageBuffer original = ImageIO::load("assets/test.png");
        ImageBuffer target   = original.clone();

        EchoContext ctx(std::make_unique<DctEngine>());
        PerturbationReport report = ctx.run(original, target, 8.0f / 255.0f);

        ImageIO::save(target, "assets/test_dct.png");

        check("output saved",              true);
        check("mean delta < 5 (invisible)", report.meanDelta < 5.0f);

        std::cout << "\n";
        report.printToConsole();

    } catch (const std::exception& e) {
        std::cerr << "  ERROR: " << e.what() << "\n";
    }
}

// ── Test 4: Compare LSB vs DCT side by side ──────────────────
void test_compareEngines() {
    std::cout << "\n--- Test: LSB vs DCT comparison ---\n";

    try {
        ImageBuffer original = ImageIO::load("assets/test.png");

        // ── Run LSB ──────────────────────────────────────────
        ImageBuffer lsbTarget = original.clone();
        EchoContext lsbCtx(std::make_unique<LsbEngine>());
        PerturbationReport lsbReport = lsbCtx.run(original, lsbTarget, 8.0f / 255.0f);

        // ── Run DCT ──────────────────────────────────────────
        ImageBuffer dctTarget = original.clone();
        EchoContext dctCtx(std::make_unique<DctEngine>());
        PerturbationReport dctReport = dctCtx.run(original, dctTarget, 8.0f / 255.0f);

        // ── Print comparison table ────────────────────────────
        std::cout << "\n";
        std::cout << "  Metric            LSB Engine    DCT Engine\n";
        std::cout << "  ─────────────────────────────────────────\n";
        printf("  Mean delta        %-14.4f%-14.4f\n",
            lsbReport.meanDelta, dctReport.meanDelta);
        printf("  Max delta         %-14.4f%-14.4f\n",
            lsbReport.maxDelta, dctReport.maxDelta);
        printf("  Pixels altered    %-14d%-14d\n",
            lsbReport.pixelsAltered, dctReport.pixelsAltered);
        std::cout << "  ─────────────────────────────────────────\n";
        std::cout << "  Lower mean delta = more invisible to humans\n";
        std::cout << "  DCT targets frequency domain = stronger ML confusion\n";

        // ── Demonstrate hot-swap ─────────────────────────────
        std::cout << "\n--- Demo: Strategy hot-swap ---\n";
        EchoContext ctx(std::make_unique<LsbEngine>());
        std::cout << "  Current engine: " << ctx.activeEngineName() << "\n";

        ctx.setEngine(std::make_unique<DctEngine>());  // swap at runtime!
        std::cout << "  After swap:     " << ctx.activeEngineName() << "\n";

        check("hot-swap works", ctx.activeEngineName() == "DCT Engine");

    } catch (const std::exception& e) {
        std::cerr << "  ERROR: " << e.what() << "\n";
    }
}

// ── main ─────────────────────────────────────────────────────
int main() {
    std::cout << "========================================\n";
    std::cout << "  ImageEcho - Day 3: DCT Engine\n";
    std::cout << "========================================\n";

    test_dctRoundTrip();
    test_dctChangesPixels();
    test_dctRealImage();
    test_compareEngines();

    std::cout << "\n========================================\n";
    std::cout << "  Day 3 complete!\n";
    std::cout << "========================================\n";

    return 0;
}
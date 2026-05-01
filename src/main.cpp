// ============================================================
// main.cpp  —  Day 4
// ============================================================
// Today we test:
//   1. SSIM returns 1.0 for identical images
//   2. SSIM returns near 0 for completely different images
//   3. SSIM correctly scores LSB and DCT perturbations
//   4. runOptimal() finds the best epsilon automatically
//   5. Final comparison table: LSB vs DCT with SSIM scores
// ============================================================

#include "ImageBuffer.hpp"
#include "ImageIO.hpp"
#include "EchoContext.hpp"
#include "SsimAnalyzer.hpp"
#include "engines/LsbEngine.hpp"
#include "engines/DctEngine.hpp"

#include <iostream>
#include <memory>
#include <cstdio>

// ── Helper ───────────────────────────────────────────────────
void check(const std::string& label, bool passed) {
    std::cout << (passed ? "  [PASS]  " : "  [FAIL]  ") << label << "\n";
}

// ── Test 1: Identical images → SSIM = 1.0 ────────────────────
void test_ssimIdentical() {
    std::cout << "\n--- Test: SSIM of identical images ---\n";
    std::cout << "  (Same image compared to itself should give SSIM = 1.0)\n";

    ImageBuffer img(100, 100, 3);
    // Fill with some values
    for (int y = 0; y < 100; ++y)
        for (int x = 0; x < 100; ++x) {
            img.at(x, y, 0) = static_cast<uint8_t>(x + y);
            img.at(x, y, 1) = static_cast<uint8_t>(x * 2 % 256);
            img.at(x, y, 2) = static_cast<uint8_t>(y * 3 % 256);
        }

    ImageBuffer copy = img.clone();
    auto result = SsimAnalyzer::compute(img, copy);

    printf("  SSIM: %.6f (expected: 1.0)\n", result.ssim);
    printf("  PSNR: %.2f dB (expected: 100)\n", result.psnr);

    check("SSIM == 1.0 for identical images", result.ssim >= 0.9999f);
    check("PSNR is very high for identical images", result.psnr >= 99.0f);
    check("max delta == 0 for identical images", result.maxDelta == 0.0f);
}

// ── Test 2: Inverted image → SSIM near 0 ─────────────────────
void test_ssimInverted() {
    std::cout << "\n--- Test: SSIM of inverted image ---\n";
    std::cout << "  (Image vs its negative/inverted version should give low SSIM)\n";

    ImageBuffer original(100, 100, 3);
    ImageBuffer inverted(100, 100, 3);

    for (int y = 0; y < 100; ++y)
        for (int x = 0; x < 100; ++x)
            for (int c = 0; c < 3; ++c) {
                uint8_t val = static_cast<uint8_t>((x + y * 2) % 256);
                original.at(x, y, c) = val;
                inverted.at(x, y, c) = static_cast<uint8_t>(255 - val);  // invert
            }

    auto result = SsimAnalyzer::compute(original, inverted);

    printf("  SSIM: %.6f (expected: near 0 or negative)\n", result.ssim);
    printf("  PSNR: %.2f dB (expected: low)\n", result.psnr);

    check("SSIM is low for inverted image", result.ssim < 0.5f);
    check("PSNR is low for inverted image", result.psnr < 20.0f);
}

// ── Test 3: SSIM scores for LSB and DCT ──────────────────────
void test_ssimScores() {
    std::cout << "\n--- Test: SSIM scores for LSB and DCT ---\n";

    try {
        ImageBuffer original = ImageIO::load("assets/test.png");

        // ── LSB ──────────────────────────────────────────────
        ImageBuffer lsbTarget = original.clone();
        EchoContext lsbCtx(std::make_unique<LsbEngine>());
        lsbCtx.run(original, lsbTarget, 8.0f / 255.0f);
        auto lsbMetrics = SsimAnalyzer::compute(original, lsbTarget);

        // ── DCT ──────────────────────────────────────────────
        ImageBuffer dctTarget = original.clone();
        EchoContext dctCtx(std::make_unique<DctEngine>());
        dctCtx.run(original, dctTarget, 8.0f / 255.0f);
        auto dctMetrics = SsimAnalyzer::compute(original, dctTarget);

        // ── Print results ─────────────────────────────────────
        std::cout << "\n  Engine       SSIM      PSNR      Max Delta  Invisible?\n";
        std::cout << "  ──────────────────────────────────────────────────────\n";
        printf("  LSB          %-10.4f%-10.2f%-11.1f%s\n",
            lsbMetrics.ssim, lsbMetrics.psnr, lsbMetrics.maxDelta,
            lsbMetrics.isInvisible() ? "YES" : "NO");
        printf("  DCT          %-10.4f%-10.2f%-11.1f%s\n",
            dctMetrics.ssim, dctMetrics.psnr, dctMetrics.maxDelta,
            dctMetrics.isInvisible() ? "YES" : "NO");

        check("LSB SSIM > 0.95 (invisible)", lsbMetrics.ssim > 0.95f);
        check("DCT SSIM > 0.95 (invisible)", dctMetrics.ssim > 0.95f);
        check("LSB PSNR > 40 dB",            lsbMetrics.psnr > 40.0f);
       check("DCT PSNR > 30 dB (stronger attack)", dctMetrics.psnr > 30.0f);

    } catch (const std::exception& e) {
        std::cerr << "  ERROR: " << e.what() << "\n";
    }
}

// ── Test 4: runOptimal() finds the best epsilon ───────────────
void test_optimalEpsilon() {
    std::cout << "\n--- Test: runOptimal() binary search ---\n";
    std::cout << "  (Finding highest epsilon that keeps SSIM >= 0.95)\n\n";

    try {
        ImageBuffer original = ImageIO::load("assets/test.png");

        // ── DCT optimal run ───────────────────────────────────
        ImageBuffer dctOptimal = original.clone();
        EchoContext dctCtx(std::make_unique<DctEngine>());

        // verbose=true so we can see the search in action
        PerturbationReport report = dctCtx.runOptimal(
            original, dctOptimal,
            0.95f,           // SSIM target
            50.0f / 255.0f,  // max epsilon to try
            true             // verbose: print each iteration
        );

        // Measure final SSIM
        auto finalMetrics = SsimAnalyzer::compute(original, dctOptimal);

        std::cout << "\n  Final result:\n";
        report.printToConsole();
        std::cout << "\n";
        finalMetrics.printToConsole();

        // Save the optimal result
        ImageIO::save(dctOptimal, "assets/test_dct_optimal.png");

        check("optimal SSIM >= 0.95", finalMetrics.ssim >= 0.95f);
        check("optimal result saved", true);

    } catch (const std::exception& e) {
        std::cerr << "  ERROR: " << e.what() << "\n";
    }
}

// ── main ─────────────────────────────────────────────────────
int main() {
    std::cout << "========================================\n";
    std::cout << "  ImageEcho - Day 4: SSIM Analyzer\n";
    std::cout << "========================================\n";

    test_ssimIdentical();
    test_ssimInverted();
    test_ssimScores();
    test_optimalEpsilon();

    std::cout << "\n========================================\n";
    std::cout << "  Day 4 complete!\n";
    std::cout << "========================================\n";

    return 0;
}
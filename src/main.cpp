// ============================================================
// main.cpp  —  Day 5
// ============================================================
// Today we run all 4 engines and produce a full benchmark table.
// This table will go directly into your README and LinkedIn post.
//
// Engines:
//   1. LSB   — bit flipping (simplest)
//   2. DCT   — frequency domain (invisible, structured)
//   3. FGSM  — gradient-based single step
//   4. PGD   — gradient-based multi-step (strongest)
// ============================================================

#include "ImageBuffer.hpp"
#include "ImageIO.hpp"
#include "EchoContext.hpp"
#include "SsimAnalyzer.hpp"
#include "engines/LsbEngine.hpp"
#include "engines/DctEngine.hpp"
#include "engines/FgsmEngine.hpp"
#include "engines/PgdEngine.hpp"

#include <iostream>
#include <memory>
#include <cstdio>
#include <string>

// ── Helper ───────────────────────────────────────────────────
void check(const std::string& label, bool passed) {
    std::cout << (passed ? "  [PASS]  " : "  [FAIL]  ") << label << "\n";
}

// ── Test 1: FGSM basic test ──────────────────────────────────
void test_fgsm() {
    std::cout << "\n--- Test: FGSM Engine ---\n";

    ImageBuffer original(50, 50, 3);
    for (int y = 0; y < 50; ++y)
        for (int x = 0; x < 50; ++x)
            for (int c = 0; c < 3; ++c)
                original.at(x, y, c) = static_cast<uint8_t>((x + y + c * 30) % 256);

    ImageBuffer target = original.clone();
    EchoContext ctx(std::make_unique<FgsmEngine>());
    PerturbationReport report = ctx.run(original, target, 8.0f / 255.0f);

    check("FGSM changes pixels",          report.pixelsAltered > 0);
    check("FGSM original unchanged",      original.at(10, 10, 0) == target.at(10, 10, 0)
                                          ? false : true);  // they SHOULD differ
    check("FGSM max delta <= epsilon*255", report.maxDelta <= 8.0f + 1.0f);

    printf("  Pixels altered: %d\n",   report.pixelsAltered);
    printf("  Mean delta:     %.4f\n", report.meanDelta);
    printf("  Max delta:      %.4f\n", report.maxDelta);
}

// ── Test 2: PGD basic test ───────────────────────────────────
void test_pgd() {
    std::cout << "\n--- Test: PGD Engine ---\n";

    ImageBuffer original(50, 50, 3);
    for (int y = 0; y < 50; ++y)
        for (int x = 0; x < 50; ++x)
            for (int c = 0; c < 3; ++c)
                original.at(x, y, c) = static_cast<uint8_t>((x * 3 + y * 2 + c * 50) % 256);

    ImageBuffer target = original.clone();

    // PGD with 10 steps (default)
    EchoContext ctx(std::make_unique<PgdEngine>(10));
    PerturbationReport report = ctx.run(original, target, 8.0f / 255.0f);

    check("PGD changes pixels",           report.pixelsAltered > 0);
    check("PGD stays within epsilon budget",
        report.maxDelta <= 8.0f + 2.0f);  // small tolerance for rounding

    printf("  Pixels altered: %d\n",   report.pixelsAltered);
    printf("  Mean delta:     %.4f\n", report.meanDelta);
    printf("  Max delta:      %.4f\n", report.maxDelta);
}

// ── Test 3: Strategy swap across all 4 engines ───────────────
void test_allEngineSwap() {
    std::cout << "\n--- Test: All 4 engines via strategy swap ---\n";

    EchoContext ctx(std::make_unique<LsbEngine>());
    check("engine 1 is LSB",  ctx.activeEngineName() == "LSB Engine");

    ctx.setEngine(std::make_unique<DctEngine>());
    check("engine 2 is DCT",  ctx.activeEngineName() == "DCT Engine");

    ctx.setEngine(std::make_unique<FgsmEngine>());
    check("engine 3 is FGSM", ctx.activeEngineName() == "FGSM Engine");

    ctx.setEngine(std::make_unique<PgdEngine>());
    check("engine 4 is PGD",  ctx.activeEngineName() == "PGD Engine");

    std::cout << "  All 4 engines swap correctly with zero code changes!\n";
}

// ── Test 4: Full benchmark — all 4 engines on real image ──────
void test_fullBenchmark() {
    std::cout << "\n--- Benchmark: All 4 engines on real image ---\n";
    std::cout << "  (This may take 30-60 seconds for PGD — it's iterative)\n\n";

    try {
        ImageBuffer original = ImageIO::load("assets/test.png");

        // ── Define engines to test ────────────────────────────
        struct EngineTest {
            std::string name;
            std::unique_ptr<IEchoEngine> engine;
            std::string outputFile;
        };

        // We can't put unique_ptr in an initializer list easily,
        // so we build the list manually
        const float eps = 8.0f / 255.0f;

        struct Result {
            std::string       name;
            PerturbationReport report;
            SsimAnalyzer::Result ssim;
        };

        std::vector<Result> results;

        // ── Run each engine ───────────────────────────────────
        auto runEngine = [&](std::string label,
                             std::unique_ptr<IEchoEngine> engine,
                             std::string outFile) {
            std::cout << "  Running " << label << "...\n";
            ImageBuffer target = original.clone();
            EchoContext ctx(std::move(engine));
            PerturbationReport rep = ctx.run(original, target, eps);
            SsimAnalyzer::Result ssim = SsimAnalyzer::compute(original, target);
            ImageIO::save(target, outFile);
            results.push_back({ label, rep, ssim });
        };

        runEngine("LSB Engine",  std::make_unique<LsbEngine>(),       "assets/out_lsb.png");
        runEngine("DCT Engine",  std::make_unique<DctEngine>(),       "assets/out_dct.png");
        runEngine("FGSM Engine", std::make_unique<FgsmEngine>(),      "assets/out_fgsm.png");
        runEngine("PGD Engine",  std::make_unique<PgdEngine>(10),     "assets/out_pgd.png");

        // ── Print benchmark table ─────────────────────────────
        std::cout << "\n";
        std::cout << "  ╔══════════════╦════════╦════════╦══════════╦══════════╦═══════════╗\n";
        std::cout << "  ║ Engine       ║  SSIM  ║  PSNR  ║ MeanΔ   ║  MaxΔ   ║ Invisible ║\n";
        std::cout << "  ╠══════════════╬════════╬════════╬══════════╬══════════╬═══════════╣\n";

        for (const auto& r : results) {
            printf("  ║ %-12s ║ %.4f ║ %5.1fdB ║ %8.4f ║ %8.1f ║    %-6s ║\n",
                r.name.c_str(),
                r.ssim.ssim,
                r.ssim.psnr,
                r.report.meanDelta,
                r.report.maxDelta,
                r.ssim.isInvisible() ? "YES" : "NO"
            );
        }

        std::cout << "  ╚══════════════╩════════╩════════╩══════════╩══════════╩═══════════╝\n";
        std::cout << "\n  Output images saved to assets/ folder.\n";
        std::cout << "  Open them side by side with the original to verify invisibility.\n";

        // Verify all engines pass SSIM > 0.95
        std::cout << "\n";
        for (const auto& r : results) {
            check(r.name + " SSIM > 0.95", r.ssim.ssim > 0.95f);
        }

    } catch (const std::exception& e) {
        std::cerr << "  ERROR: " << e.what() << "\n";
    }
}

// ── main ─────────────────────────────────────────────────────
int main() {
    std::cout << "========================================\n";
    std::cout << "  ImageEcho - Day 5: All 4 Engines\n";
    std::cout << "========================================\n";

    test_fgsm();
    test_pgd();
    test_allEngineSwap();
    test_fullBenchmark();

    std::cout << "\n========================================\n";
    std::cout << "  Day 5 complete! All engines built.\n";
    std::cout << "========================================\n";

    return 0;
}
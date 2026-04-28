// ============================================================
// main.cpp  —  Day 2
// ============================================================
// Today we test the Strategy Pattern:
//   1. Create an EchoContext with an LsbEngine
//   2. Load a real image
//   3. Run the engine — it perturbs the image
//   4. Save the result
//   5. Visually confirm the images look identical
// ============================================================

#include "ImageBuffer.hpp"
#include "ImageIO.hpp"
#include "EchoContext.hpp"
#include "engines/LsbEngine.hpp"

#include <iostream>
#include <memory>   // std::make_unique

// ── Helper: print test result ────────────────────────────────
void check(const std::string& label, bool passed) {
    std::cout << (passed ? "  [PASS]  " : "  [FAIL]  ") << label << "\n";
}

// ── Test 1: Strategy Pattern basics ─────────────────────────
void test_strategyPattern() {
    std::cout << "\n--- Test: Strategy Pattern ---\n";

    // Create a context and plug in the LSB engine
    // make_unique<LsbEngine>() creates a new LsbEngine on the heap
    // and wraps it in a unique_ptr (smart pointer)
    EchoContext ctx(std::make_unique<LsbEngine>());

    check("active engine is LSB", ctx.activeEngineName() == "LSB Engine");
    std::cout << "  Active engine: " << ctx.activeEngineName() << "\n";
}

// ── Test 2: LSB actually changes pixels ──────────────────────
void test_lsbChangesPixels() {
    std::cout << "\n--- Test: LSB changes pixels ---\n";

    // Create a small test image
    ImageBuffer original(10, 10, 3);

    // Fill with a known value so we can predict the result
    // Let's set all pixels to 200 (binary: 11001000)
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x)
            for (int c = 0; c < 3; ++c)
                original.at(x, y, c) = 200;

    // Clone original into target (engine writes into target)
    ImageBuffer target = original.clone();

    // Run the LSB engine
    EchoContext ctx(std::make_unique<LsbEngine>());
    PerturbationReport report = ctx.run(original, target, 8.0f / 255.0f);

    // 200 XOR 3 = 203 (flipping bottom 2 bits)
    // binary: 11001000 XOR 00000011 = 11001011 = 203
    check("pixel value changed",   target.at(0, 0, 0) != original.at(0, 0, 0));
    check("change is tiny (<=3)",  report.maxDelta <= 3.0f);
    check("all pixels altered",    report.pixelsAltered == 10 * 10 * 3);
    check("original unchanged",    original.at(0, 0, 0) == 200);

    std::cout << "  Original pixel value: " << (int)original.at(0, 0, 0) << "\n";
    std::cout << "  Perturbed pixel value: " << (int)target.at(0, 0, 0) << "\n";
    std::cout << "  Max delta: " << report.maxDelta << " (invisible if <=3)\n";
}

// ── Test 3: Run on a real image ──────────────────────────────
void test_realImage() {
    std::cout << "\n--- Test: Real image perturbation ---\n";

    const std::string inputPath  = "assets/test.png";
    const std::string outputPath = "assets/test_lsb.png";

    try {
        // Load original
        ImageBuffer original = ImageIO::load(inputPath);
        std::cout << "  Loaded: " << original.dims().width
                  << " x " << original.dims().height << " px\n";

        // Clone into target
        ImageBuffer target = original.clone();

        // Run LSB engine
        EchoContext ctx(std::make_unique<LsbEngine>());
        PerturbationReport report = ctx.run(original, target, 8.0f / 255.0f);

        // Save perturbed image
        ImageIO::save(target, outputPath);

        check("output saved successfully", true);
        check("max delta is invisible (<=3)", report.maxDelta <= 3.0f);

        std::cout << "\n";
        report.printToConsole();

        std::cout << "\n  Compare these two files visually — they should look IDENTICAL:\n";
        std::cout << "    Original:  " << inputPath  << "\n";
        std::cout << "    Perturbed: " << outputPath << "\n";

    } catch (const std::exception& e) {
        std::cerr << "  ERROR: " << e.what() << "\n";
    }
}

// ── main ─────────────────────────────────────────────────────
int main() {
    std::cout << "========================================\n";
    std::cout << "  ImageEcho — Day 2: Strategy Pattern\n";
    std::cout << "========================================\n";

    test_strategyPattern();
    test_lsbChangesPixels();
    test_realImage();

    std::cout << "\n========================================\n";
    std::cout << "  Day 2 complete!\n";
    std::cout << "========================================\n";

    return 0;
}
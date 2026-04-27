// ============================================================
// main.cpp  —  Day 1 test program
// ============================================================
// This file is our entry point. Right now it just tests that:
//   1. We can create an ImageBuffer
//   2. We can read and write pixel values
//   3. We can load a PNG from disk
//   4. We can save a PNG to disk
//   5. clone() makes an independent copy
//
// As the project grows, this file will become a proper CLI tool.
// For now it's just a sanity check that everything compiles and works.
// ============================================================

#include "ImageBuffer.hpp"
#include "ImageIO.hpp"

#include <iostream>  // std::cout, std::cerr
#include <cassert>   // assert() — crashes the program if a condition is false

// ── Helper: print a test result ──────────────────────────────
void printResult(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "  [PASS]  " << testName << "\n";
    } else {
        std::cout << "  [FAIL]  " << testName << "\n";
    }
}

// ── Test 1: Create a buffer and check its dimensions ─────────
void test_createBuffer() {
    std::cout << "\n--- Test: Create ImageBuffer ---\n";

    // Create a 100 x 80 RGB image (3 channels)
    ImageBuffer img(100, 80, 3);

    printResult("width  == 100", img.dims().width    == 100);
    printResult("height == 80",  img.dims().height   == 80);
    printResult("channels == 3", img.dims().channels == 3);
    printResult("totalPixels == 8000", img.dims().totalPixels() == 8000);
    printResult("byteCount == 24000",  img.dims().byteCount()   == 24000);
}

// ── Test 2: Write a pixel value and read it back ─────────────
void test_pixelReadWrite() {
    std::cout << "\n--- Test: Pixel read/write ---\n";

    ImageBuffer img(50, 50, 3);  // 50x50 RGB image

    // Set the pixel at position (10, 20) to bright red: R=255, G=0, B=0
    img.at(10, 20, 0) = 255;   // Red channel
    img.at(10, 20, 1) = 0;     // Green channel
    img.at(10, 20, 2) = 0;     // Blue channel

    // Set another pixel to pure green
    img.at(30, 10, 0) = 0;
    img.at(30, 10, 1) = 255;
    img.at(30, 10, 2) = 0;

    // Read them back and verify
    printResult("pixel (10,20) red   == 255", img.at(10, 20, 0) == 255);
    printResult("pixel (10,20) green == 0",   img.at(10, 20, 1) == 0);
    printResult("pixel (10,20) blue  == 0",   img.at(10, 20, 2) == 0);
    printResult("pixel (30,10) green == 255", img.at(30, 10, 1) == 255);
}

// ── Test 3: clone() makes an independent copy ────────────────
void test_clone() {
    std::cout << "\n--- Test: clone() independence ---\n";

    ImageBuffer original(20, 20, 3);
    original.at(5, 5, 0) = 100;  // set a pixel in the original

    // Clone it
    ImageBuffer copy = original.clone();

    // Verify the clone has the same pixel value
    printResult("clone has same pixel value", copy.at(5, 5, 0) == 100);

    // Now change the clone — original should NOT change
    copy.at(5, 5, 0) = 200;

    printResult("original unchanged after modifying clone",
        original.at(5, 5, 0) == 100);

    printResult("clone has new value",
        copy.at(5, 5, 0) == 200);
}

// ── Test 4: Load a real image and save a copy ─────────────────
void test_imageLoadSave() {
    std::cout << "\n--- Test: Load and save image ---\n";

    // IMPORTANT: put a real PNG file at assets/test.png before running this test!
    // You can use any PNG image (photo, screenshot, anything).
    // If the file doesn't exist, this test will report the error and skip.

    const std::string inputPath  = "assets/test.png";
    const std::string outputPath = "assets/test_copy.png";

    try {
        // Load the image
        ImageBuffer img = ImageIO::load(inputPath);

        std::cout << "  Loaded: " << img.dims().width << " x "
                  << img.dims().height << " px, "
                  << img.dims().channels << " channels\n";

        printResult("width  > 0",    img.dims().width    > 0);
        printResult("height > 0",    img.dims().height   > 0);
        printResult("channels > 0",  img.dims().channels > 0);

        // Save an exact copy
        ImageIO::save(img, outputPath);
        std::cout << "  Saved copy to: " << outputPath << "\n";
        printResult("save completed without exception", true);

    } catch (const std::exception& e) {
        // Something went wrong — print the error message
        std::cerr << "  WARNING: " << e.what() << "\n";
        std::cerr << "  Add a PNG image at 'assets/test.png' to test loading.\n";
    }
}

// ── main ─────────────────────────────────────────────────────
int main() {
    std::cout << "========================================\n";
    std::cout << "  ImageEcho — Day 1 Tests\n";
    std::cout << "========================================\n";

    test_createBuffer();
    test_pixelReadWrite();
    test_clone();
    test_imageLoadSave();

    std::cout << "\n========================================\n";
    std::cout << "  Done. Fix any [FAIL] before Day 2.\n";
    std::cout << "========================================\n";

    return 0;
}
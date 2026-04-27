#pragma once
// ============================================================
// ImageIO.hpp
// ============================================================
// PURPOSE: Load images from disk into an ImageBuffer,
//          and save an ImageBuffer back to disk as a PNG.
//
// We use a tiny free library called stb_image to do the
// heavy lifting of reading/writing PNG/JPG files.
// All we do is wrap it in our own clean interface.
// ============================================================

// These two lines activate the stb_image implementation.
// They must appear in exactly ONE .cpp/.hpp before including stb.
// (stb_image is "header-only" — the whole library is in one .h file,
//  but you only compile the implementation once using these defines.)
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"        // from third_party/
#include "stb_image_write.h"  // from third_party/
#include "ImageBuffer.hpp"

#include <string>
#include <stdexcept>

// ── ImageIO ──────────────────────────────────────────────────
// A class with only static functions — you never create an ImageIO object,
// you just call ImageIO::load(...) and ImageIO::save(...) directly.
// Think of it like a toolbox, not an object.
class ImageIO {
public:

    // ── load() ──────────────────────────────────────────────
    // Reads a PNG or JPG file from disk and returns an ImageBuffer.
    //
    // Usage:  ImageBuffer img = ImageIO::load("assets/photo.png");
    //
    static ImageBuffer load(const std::string& filePath) {
        int width, height, channelsInFile;

        // stbi_load does all the file reading work.
        // It returns a raw pointer to pixel data, or nullptr on failure.
        // The '0' at the end means "give me however many channels the file has".
        uint8_t* rawPixels = stbi_load(
            filePath.c_str(),   // file path as a C string
            &width,             // stb fills these in for us
            &height,
            &channelsInFile,
            0                   // 0 = keep original channels
        );

        if (rawPixels == nullptr) {
            throw std::runtime_error(
                "ImageIO::load — could not open file: " + filePath +
                "\nMake sure the file exists and the path is correct."
            );
        }

        // Create our ImageBuffer with the right size
        ImageBuffer buffer(width, height, channelsInFile);

        // Copy the raw pixel data from stb into our buffer
        // (stb owns its memory separately, so we need to copy)
        std::copy(
            rawPixels,
            rawPixels + buffer.dims().byteCount(),
            buffer.rawData()
        );

        // IMPORTANT: stb allocated its own memory, we must free it
        stbi_image_free(rawPixels);

        return buffer;  // move semantics kick in here — no copy made
    }

    // ── save() ──────────────────────────────────────────────
    // Saves an ImageBuffer to a PNG file on disk.
    //
    // Usage:  ImageIO::save(myBuffer, "output/result.png");
    //
    static void save(const ImageBuffer& buffer, const std::string& filePath) {
        const Dimensions& d = buffer.dims();

        // stbi_write_png writes the data to disk.
        // The last argument is "stride" — how many bytes per row.
        int result = stbi_write_png(
            filePath.c_str(),       // output path
            d.width,
            d.height,
            d.channels,
            buffer.rawData(),       // pointer to pixel data
            d.width * d.channels    // bytes per row (stride)
        );

        if (result == 0) {
            throw std::runtime_error(
                "ImageIO::save — could not save file: " + filePath +
                "\nMake sure the output folder exists."
            );
        }
    }
};
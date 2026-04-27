#pragma once
// ============================================================
// ImageBuffer.hpp
// ============================================================
// PURPOSE: This class owns the raw pixel data of an image.
//
// ANALOGY: Think of an image as a grid of tiny coloured squares (pixels).
//          Each pixel has up to 4 values: Red, Green, Blue, Alpha (opacity).
//          This class holds all those numbers in one big flat array.
//
// MEMORY LAYOUT EXAMPLE (a 3-pixel-wide, 2-pixel-tall RGB image):
//
//   [R,G,B, R,G,B, R,G,B,   <- row 0 (y=0)
//    R,G,B, R,G,B, R,G,B]   <- row 1 (y=1)
//
//   To get pixel at column x, row y, channel c:
//   index = (y * width + x) * channels + c
//
// ============================================================

#include <cstdint>      // uint8_t (an integer from 0-255, perfect for pixel values)
#include <memory>       // std::unique_ptr (smart pointer — automatically frees memory)
#include <stdexcept>    // std::invalid_argument (for throwing errors)
#include <algorithm>    // std::copy (for copying arrays)
#include <string>       // std::string

// ── Dimensions ──────────────────────────────────────────────
// A simple struct (like a plain class) that just holds three numbers.
// We use it to pass image size around without having separate variables.
struct Dimensions {
    int width    = 0;   // number of pixels across (horizontal)
    int height   = 0;   // number of pixels down   (vertical)
    int channels = 0;   // 1=grayscale, 3=RGB, 4=RGBA

    // Helper: total number of pixels
    int totalPixels() const { return width * height; }

    // Helper: total number of bytes in the array
    // (each pixel has `channels` bytes)
    int byteCount() const { return width * height * channels; }
};

// ── ImageBuffer ──────────────────────────────────────────────
class ImageBuffer {
public:

    // ── Constructor ─────────────────────────────────────────
    // Creates a blank image of a given size.
    // It allocates (reserves) memory automatically.
    ImageBuffer(int width, int height, int channels)
        : dims_{ width, height, channels }
    {
        // Guard against silly inputs
        if (width <= 0 || height <= 0 || channels <= 0) {
            throw std::invalid_argument("ImageBuffer: width, height and channels must all be > 0");
        }

        // unique_ptr<uint8_t[]> is a "smart array".
        // It works just like a normal array but automatically
        // frees the memory when this object is destroyed.
        // No manual delete[] needed — that's RAII (Resource Acquisition Is Initialisation).
        data_ = std::make_unique<uint8_t[]>(dims_.byteCount());
    }

    // ── clone() ─────────────────────────────────────────────
    // Makes a completely independent copy of this image.
    // Why not just copy with '='? Because we deleted that (see bottom).
    // We want copies to be EXPLICIT so nothing happens by accident.
    ImageBuffer clone() const {
        ImageBuffer copy(dims_.width, dims_.height, dims_.channels);
        // Copy every byte from this buffer into the new one
        std::copy(
            data_.get(),                          // source start
            data_.get() + dims_.byteCount(),      // source end
            copy.data_.get()                      // destination start
        );
        return copy;
    }

    // ── at() — read/write a single pixel channel ────────────
    // Returns a REFERENCE to the byte so you can both read and write it:
    //   uint8_t val = buffer.at(10, 5, 0);   // read red channel at (10,5)
    //   buffer.at(10, 5, 0) = 255;           // write red channel at (10,5)
    uint8_t& at(int x, int y, int channel) {
        return data_[pixelIndex(x, y, channel)];
    }

    // Const version — used when the ImageBuffer is marked const (read-only).
    // Returning a value (not a reference) because we can't modify a const object.
    uint8_t at(int x, int y, int channel) const {
        return data_[pixelIndex(x, y, channel)];
    }

    // ── Raw data access ─────────────────────────────────────
    // Sometimes we need a raw pointer to the start of the data,
    // for example when passing to stb_image_write.
    uint8_t*       rawData()       { return data_.get(); }
    const uint8_t* rawData() const { return data_.get(); }

    // ── dims() — get the image dimensions ───────────────────
    const Dimensions& dims() const { return dims_; }

    // ── Move semantics ──────────────────────────────────────
    // "Moving" transfers ownership of the memory without copying it.
    // This is how we return ImageBuffers from functions efficiently.
    // (Compiler uses this automatically when returning from functions)
    ImageBuffer(ImageBuffer&&)            = default;
    ImageBuffer& operator=(ImageBuffer&&) = default;

    // ── Disable accidental copies ───────────────────────────
    // If you want a copy, use clone() explicitly.
    // This prevents bugs where you accidentally copy a huge image.
    ImageBuffer(const ImageBuffer&)            = delete;
    ImageBuffer& operator=(const ImageBuffer&) = delete;

private:
    // ── pixelIndex() ────────────────────────────────────────
    // Converts (x, y, channel) coordinates into a flat array index.
    // This is the key formula for 2D pixel access.
    int pixelIndex(int x, int y, int channel) const {
        return (y * dims_.width + x) * dims_.channels + channel;
    }

    Dimensions                 dims_;   // width, height, channels
    std::unique_ptr<uint8_t[]> data_;   // the actual pixel bytes
};
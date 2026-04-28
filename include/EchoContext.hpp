#pragma once
// ============================================================
// EchoContext.hpp
// ============================================================
// PURPOSE: This is the "slot" that holds whichever engine
//          you want to use. It's the Strategy Pattern's
//          "Context" class.
//
// ANALOGY: Think of a universal remote control.
//          The remote (EchoContext) works with any TV brand (engine).
//          You can swap the TV without changing the remote.
//
// HOW TO USE:
//   // Create a context with the LSB engine
//   EchoContext ctx(std::make_unique<LsbEngine>());
//   ctx.run(original, target);
//
//   // Swap to DCT engine later — zero other code changes
//   ctx.setEngine(std::make_unique<DctEngine>());
//   ctx.run(original, target);
//
// std::unique_ptr means EchoContext OWNS the engine.
// When EchoContext is destroyed, the engine is automatically deleted.
// ============================================================

#include "IEchoEngine.hpp"
#include "ImageBuffer.hpp"
#include "PerturbationReport.hpp"
#include <memory>     // std::unique_ptr, std::make_unique
#include <stdexcept>  // std::runtime_error

class EchoContext {
public:

    // ── Constructor ─────────────────────────────────────────
    // Takes ownership of an engine via unique_ptr.
    // 'explicit' prevents accidental implicit conversions.
    explicit EchoContext(std::unique_ptr<IEchoEngine> engine)
        : engine_(std::move(engine))   // 'move' transfers ownership
    {
        if (!engine_) {
            throw std::runtime_error("EchoContext: engine cannot be null");
        }
    }

    // ── setEngine() — hot-swap the algorithm ────────────────
    // Replace the current engine with a different one at any time.
    // The old engine is automatically deleted (unique_ptr handles it).
    void setEngine(std::unique_ptr<IEchoEngine> newEngine) {
        if (!newEngine) {
            throw std::runtime_error("EchoContext: cannot set a null engine");
        }
        engine_ = std::move(newEngine);
    }

    // ── run() — apply the current engine to an image ────────
    // This is the main method you call to perturb an image.
    //
    // It automatically:
    //   1. Clones the original into target (so original is never touched)
    //   2. Runs the engine on the target
    //   3. Returns a report of what changed
    //
    // Usage:
    //   ImageBuffer original = ImageIO::load("photo.png");
    //   ImageBuffer target   = original.clone();  // or let run() do it
    //   auto report = ctx.run(original, target, 8.0f/255.0f);
    PerturbationReport run(
        const ImageBuffer& original,
        ImageBuffer&       target,
        float              epsilon = 8.0f / 255.0f   // default: 8 units max change
    ) const {
        return engine_->apply(original, target, epsilon);
    }

    // ── activeEngineName() ──────────────────────────────────
    // Returns the name of the currently loaded engine.
    std::string activeEngineName() const {
        return engine_->name();
    }

private:
    // unique_ptr owns the engine — automatically cleaned up
    // when EchoContext goes out of scope. No manual delete needed.
    std::unique_ptr<IEchoEngine> engine_;
};
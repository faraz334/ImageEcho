#pragma once
// ============================================================
// IEchoEngine.hpp
// ============================================================
// PURPOSE: This is the INTERFACE (contract) that every
//          perturbation engine must follow.
//
// ANALOGY: Think of this like a job description.
//          It says: "anyone who wants to be an EchoEngine
//          MUST have a method called apply() and a method
//          called name()." Every engine agrees to this contract.
//
// WHY AN INTERFACE?
//   Without it, EchoContext would need to know about
//   LsbEngine, DctEngine, FgsmEngine etc. specifically.
//   With it, EchoContext only knows about IEchoEngine —
//   it doesn't care WHICH engine is plugged in.
//   That's the Strategy Pattern.
//
// The 'I' prefix on IEchoEngine is a naming convention
// meaning "Interface".
// ============================================================

#include "ImageBuffer.hpp"
#include "PerturbationReport.hpp"
#include <string>

class IEchoEngine {
public:

    // ── Virtual destructor ───────────────────────────────────
    // IMPORTANT: Any class meant to be inherited from MUST have
    // a virtual destructor. Without it, C++ won't clean up
    // derived classes (LsbEngine, DctEngine etc.) properly.
    virtual ~IEchoEngine() = default;

    // ── apply() — THE core method ───────────────────────────
    // Every engine must implement this method.
    // It takes the original image, writes the perturbed version
    // into 'target', and returns a report of what changed.
    //
    // Parameters:
    //   original — the unmodified source image (read-only)
    //   target   — where the perturbed image is written
    //   epsilon  — max allowed change per pixel (0.0 to 1.0)
    //              e.g. 8.0/255.0 ≈ 0.031 means max 8 units change
    //
    // 'virtual' means subclasses can override this method.
    // '= 0' means this is PURE virtual — subclasses MUST implement it.
    // A class with any pure virtual method cannot be instantiated directly.
    virtual PerturbationReport apply(
        const ImageBuffer& original,   // read-only source
        ImageBuffer&       target,     // output (already cloned before passing in)
        float              epsilon     // perturbation budget
    ) const = 0;

    // ── name() — identifies the engine ──────────────────────
    // Returns a human-readable name like "LSB Engine".
    // Used in reports and console output.
    virtual std::string name() const = 0;
};
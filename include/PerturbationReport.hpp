#pragma once
// ============================================================
// PerturbationReport.hpp
// ============================================================
// PURPOSE: A simple container that holds the results after
//          a perturbation engine runs on an image.
//
// ANALOGY: Like a receipt after a transaction. The engine does
//          its work and hands you this report saying what happened.
//
// This is called a "value object" — it just holds data,
// no methods, no logic. Plain and simple.
// ============================================================

#include <string>

struct PerturbationReport {

    // Which engine produced this report? (e.g. "LSB Engine")
    std::string engineName = "Unknown";

    // The epsilon (ε) value used — how much perturbation was allowed.
    // Expressed as a fraction of 255. e.g. 0.03 means max 7.6 per pixel.
    float epsilon = 0.0f;

    // Average pixel change across the whole image.
    // Lower = more invisible. Good results are below 3.0.
    float meanDelta = 0.0f;

    // Maximum pixel change found anywhere in the image.
    // Should stay below 10 for invisible perturbations.
    float maxDelta = 0.0f;

    // How many pixels were actually changed.
    int pixelsAltered = 0;

    // ── printToConsole() ─────────────────────────────────────
    // Prints a neat summary to the terminal.
    void printToConsole() const {
        printf("┌─────────────────────────────────┐\n");
        printf("│     Perturbation Report         │\n");
        printf("├─────────────────────────────────┤\n");
        printf("│ Engine:          %-14s │\n", engineName.c_str());
        printf("│ Epsilon:         %-14.5f │\n", epsilon);
        printf("│ Mean delta:      %-14.4f │\n", meanDelta);
        printf("│ Max delta:       %-14.4f │\n", maxDelta);
        printf("│ Pixels altered:  %-14d │\n", pixelsAltered);
        printf("└─────────────────────────────────┘\n");
    }
};
#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>
using namespace std;

/**
 * ============================================================================
 * Module: Generator (Part 1)
 * Responsible Member: Member 1 (Domain & Encoding Lead)
 * Jira Ticket: ISE-1 (Subtasks 1.1 - 1.4)
 * Principles: Functional Cohesion, Open-Closed Principle (OCP)
 * ============================================================================
 */

// Encodes a 6-digit numeric string into a Code 39 barcode and saves it as a .bmp file.
// Returns true on success, or false if generation fails.
bool generateBarcodeBMP(const string& sixDigitCode, const string& outputFilename);

#endif // GENERATOR_H

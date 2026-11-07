#ifndef DECODER_H
#define DECODER_H

#include <string>
using namespace std;

/**
 * ============================================================================
 * Module: Decoder & Scanner (Part 2)
 * Responsible Member: Member 2 (Computer Vision & Decoder Lead)
 * Jira Ticket: ISE-2 & ISE-3 (Subtasks 2.1 - 2.4, 3.1 - 3.4)
 * Principles: Modularity, Fault Isolation, Loose Data Coupling
 * ============================================================================
 */

// Decodes a 6-digit number from a saved barcode BMP image file.
// Returns true on successful decode, or false with an explanation in errorMessage.
bool decodeBarcodeFromBMP(const string& imagePath, string& decodedCode, string& errorMessage);

// Scans a barcode live using a webcam / camera stream.
// Returns true when a 6-digit barcode is successfully detected and decoded.
bool scanBarcodeFromCamera(string& decodedCode, string& errorMessage);

#endif // DECODER_H

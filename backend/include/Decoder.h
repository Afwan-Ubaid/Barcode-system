#ifndef DECODER_H
#define DECODER_H

#include <string>

// Decodes a 6-digit number from a saved barcode BMP image file.
// Returns true on successful decode, or false with an explanation in errorMessage.
bool decodeBarcodeFromBMP(const std::string& imagePath, std::string& decodedCode, std::string& errorMessage);

// Decodes a 6-digit barcode directly from raw pixel memory (RGB24 or RGBA32).
// Robust against tilt, shadows, glare, phone screen backlights, and surrounding objects.
// Set fastMode = true during live 30 FPS camera video feed to maintain 60 FPS UI responsiveness.
bool decodeBarcodeFromMemory(const unsigned char* pixelData, int width, int height, bool isRGBA, std::string& decodedCode, std::string& errorMessage, bool fastMode = false);

// Scans a barcode live using a webcam / camera stream.
// Returns true when a 6-digit barcode is successfully detected and decoded.
bool scanBarcodeFromCamera(std::string& decodedCode, std::string& errorMessage);

#endif // DECODER_H

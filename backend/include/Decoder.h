#ifndef DECODER_H
#define DECODER_H

#include <string>

struct BarcodeBox {
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    bool detected = false;
};

// Decodes a 6-digit number from a saved barcode BMP image file.
// Returns true on successful decode, or false with an explanation in errorMessage.
bool decodeBarcodeFromBMP(const std::string& imagePath, std::string& decodedCode, std::string& errorMessage);

// Decodes a 6-digit barcode directly from raw pixel memory (RGB24 or RGBA32).
// Robust against tilt, shadows, glare, phone screen backlights, and surrounding objects.
// Set fastMode = true during live 30 FPS camera video feed to maintain 60 FPS UI responsiveness.
bool decodeBarcodeFromMemory(const unsigned char* pixelData, int width, int height, bool isRGBA, std::string& decodedCode, std::string& errorMessage, bool fastMode = false);

// Detects barcode location bounding box and decodes 6-digit payload from memory
bool detectBarcodeInFrame(const unsigned char* pixelData, int width, int height, bool isRGBA, std::string& decodedCode, BarcodeBox& outBox, bool fastMode = true);

// Scans a barcode live using a webcam / camera stream.
// Returns true when a 6-digit barcode is successfully detected and decoded.
bool scanBarcodeFromCamera(std::string& decodedCode, std::string& errorMessage);

#endif // DECODER_H

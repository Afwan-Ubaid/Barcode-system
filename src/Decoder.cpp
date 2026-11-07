#include "Decoder.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>
using namespace std;

#pragma pack(push, 1)
struct RawBMPHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

static char decodeCode39Pattern(const string& pattern) {
    if (pattern == "NNNWWNWNN") return '0';
    if (pattern == "WNNWNNNNW") return '1';
    if (pattern == "NNWWNNNNW") return '2';
    if (pattern == "WNWWNNNNN") return '3';
    if (pattern == "NNNWWNNNW") return '4';
    if (pattern == "WNNWWNNNN") return '5';
    if (pattern == "NNWWWNNNN") return '6';
    if (pattern == "NNNWNNWNW") return '7';
    if (pattern == "WNNWNNWNN") return '8';
    if (pattern == "NNWWNNWNN") return '9';
    if (pattern == "NWNNWNWNN") return '*';
    return '?';
}

bool decodeBarcodeFromBMP(const string& imagePath, string& decodedCode, string& errorMessage) {
    ifstream file(imagePath, ios::binary);
    if (!file) {
        errorMessage = "Error: File '" + imagePath + "' could not be opened.";
        return false;
    }

    RawBMPHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.bfType != 0x4D42) {
        errorMessage = "Error: File is not a valid BMP image.";
        return false;
    }

    if (header.biBitCount != 24) {
        errorMessage = "Error: Only 24-bit uncompressed BMP images are supported.";
        return false;
    }

    int width = header.biWidth;
    int height = abs(header.biHeight);
    if (width <= 0 || height <= 0) {
        errorMessage = "Error: Invalid image dimensions.";
        return false;
    }

    int rowStride = ((width * 3 + 3) / 4) * 4;
    int middleY = height / 2;

    // Seek to the middle scanline
    streamoff rowOffset = header.bfOffBits + static_cast<streamoff>(middleY) * rowStride;
    file.seekg(rowOffset, ios::beg);

    vector<uint8_t> rowData(rowStride);
    file.read(reinterpret_cast<char*>(rowData.data()), rowStride);
    file.close();

    // Convert middle row to binary black/white sequence (true = Black bar, false = White space)
    vector<bool> pixels(width);
    for (int x = 0; x < width; ++x) {
        uint8_t b = rowData[x * 3 + 0];
        uint8_t g = rowData[x * 3 + 1];
        uint8_t r = rowData[x * 3 + 2];
        int brightness = (static_cast<int>(r) + g + b) / 3;
        pixels[x] = (brightness < 128); // Dark pixel = Bar
    }

    // Skip left quiet zone (find first black pixel)
    int firstBar = -1;
    for (int x = 0; x < width; ++x) {
        if (pixels[x]) {
            firstBar = x;
            break;
        }
    }

    // Find last black pixel
    int lastBar = -1;
    for (int x = width - 1; x >= 0; --x) {
        if (pixels[x]) {
            lastBar = x;
            break;
        }
    }

    if (firstBar == -1 || lastBar == -1 || lastBar <= firstBar) {
        errorMessage = "Error: No barcode lines detected in the image.";
        return false;
    }

    // Group consecutive pixels into run lengths
    struct Run {
        bool isBar;
        int length;
    };
    vector<Run> runs;

    bool currentBar = pixels[firstBar];
    int currentLen = 0;
    for (int x = firstBar; x <= lastBar; ++x) {
        if (pixels[x] == currentBar) {
            currentLen++;
        } else {
            runs.push_back({currentBar, currentLen});
            currentBar = pixels[x];
            currentLen = 1;
        }
    }
    runs.push_back({currentBar, currentLen});

    // In Code 39 for 8 symbols (* + 6 digits + *), there are:
    // 8 symbols * 9 elements = 72 elements + 7 inter-character spaces = 79 runs total
    if (runs.size() < 79) {
        errorMessage = "Error: Barcode pattern is incomplete or damaged. Found " + 
                       to_string(runs.size()) + " elements (expected 79).";
        return false;
    }

    // Determine narrow vs wide threshold based on min and max run lengths
    int minRun = runs[0].length;
    int maxRun = runs[0].length;
    for (const auto& r : runs) {
        if (r.length < minRun) minRun = r.length;
        if (r.length > maxRun) maxRun = r.length;
    }

    double threshold = (minRun + maxRun) / 2.0;

    // Decode each character (9 runs per symbol + 1 run for inter-character space)
    string decodedText = "";
    size_t runIdx = 0;

    while (runIdx + 9 <= runs.size()) {
        string pattern = "";
        for (int i = 0; i < 9; ++i) {
            pattern += (runs[runIdx + i].length >= threshold) ? 'W' : 'N';
        }

        char ch = decodeCode39Pattern(pattern);
        if (ch == '?') {
            errorMessage = "Error: Unrecognized barcode symbol pattern at element " + to_string(runIdx);
            return false;
        }
        decodedText += ch;
        runIdx += 9;

        // Skip inter-character space if there is one
        if (runIdx < runs.size() && !runs[runIdx].isBar) {
            runIdx++;
        }
    }

    // Verify framing: * + 6 digits + *
    if (decodedText.length() != 8 || decodedText.front() != '*' || decodedText.back() != '*') {
        errorMessage = "Error: Invalid framing. Decoded string: " + decodedText;
        return false;
    }

    decodedCode = decodedText.substr(1, 6);
    errorMessage.clear();
    return true;
}

bool scanBarcodeFromCamera(string& decodedCode, string& errorMessage) {
    // Camera scanning interface (Subtask 3.1 - 3.4)
    // If OpenCV is installed in Visual Studio, plug in cv::VideoCapture here.
    // For standalone testing without requiring OpenCV dependencies:
    cout << "\n[CAMERA SCANNER]: Initializing webcam stream...\n";
    cout << "[CAMERA SCANNER]: Point your camera at a Code 39 barcode.\n";
    cout << "[CAMERA SCANNER]: Enter the image filename captured by camera to decode (or press Enter for 'barcode.bmp'): ";
    
    string capturedFile;
    getline(cin, capturedFile);
    if (capturedFile.empty()) {
        capturedFile = "barcode.bmp";
    }

    return decodeBarcodeFromBMP(capturedFile, decodedCode, errorMessage);
}

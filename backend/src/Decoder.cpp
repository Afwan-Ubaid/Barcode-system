#include "Decoder.h"
#include "CameraCapture.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

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

struct BarcodeRun {
    bool isBar;
    int len;
};

static bool parseCode39Runs(const vector<BarcodeRun>& runs, string& outDigits) {
    if (runs.size() < 79) return false;

    // Scan through all possible start positions
    for (size_t start = 0; start + 79 <= runs.size(); ++start) {
        if (!runs[start].isBar) continue;

        string decoded = "";
        size_t idx = start;
        bool valid = true;

        for (int c = 0; c < 8; ++c) {
            int minL = 99999, maxL = 0;
            for (int i = 0; i < 9; ++i) {
                int l = runs[idx + i].len;
                if (l < minL) minL = l;
                if (l > maxL) maxL = l;
            }

            if (maxL < minL * 1.35) { valid = false; break; }
            double t = (minL + maxL) / 2.0;

            string pat = "";
            int wideCount = 0;
            for (int i = 0; i < 9; ++i) {
                if (runs[idx + i].len >= t) {
                    pat += 'W';
                    wideCount++;
                } else {
                    pat += 'N';
                }
            }

            if (wideCount != 3) { valid = false; break; }
            char ch = decodeCode39Pattern(pat);
            if (ch == '?') { valid = false; break; }

            decoded += ch;
            idx += 9;
            if (c < 7) {
                if (idx < runs.size() && !runs[idx].isBar) {
                    idx++; // Skip inter-character space
                }
            }
        }

        if (valid && decoded.length() == 8 && decoded.front() == '*' && decoded.back() == '*') {
            bool allDigits = true;
            for (int i = 1; i <= 6; ++i) {
                if (decoded[i] < '0' || decoded[i] > '9') allDigits = false;
            }
            if (allDigits) {
                outDigits = decoded.substr(1, 6);
                return true;
            }
        }
    }
    return false;
}

bool decodeBarcodeFromMemory(const unsigned char* pixelData, int width, int height, bool isRGBA, string& decodedCode, string& errorMessage, bool fastMode) {
    if (!pixelData || width <= 10 || height <= 10) {
        errorMessage = "Error: Invalid image buffer dimensions.";
        return false;
    }

    int bpp = isRGBA ? 4 : 3;

    // Multi-angle and multi-scanline sweep
    // In fastMode (used during live video feed), evaluate 3 key angles at wider step for zero lag
    vector<float> angles;
    int yStep = 6;
    vector<int> deltas;

    if (fastMode) {
        angles = { 0.f, 3.5f, -3.5f };
        yStep = 10;
        deltas = { 12, 18 };
    } else {
        angles = { 0.f, 2.5f, -2.5f, 5.f, -5.f, 7.5f, -7.5f, 10.f, -10.f };
        yStep = 6;
        deltas = { 8, 14, 20 };
    }

    for (float deg : angles) {
        float rad = deg * 3.14159265f / 180.f;
        float tanA = std::tan(rad);

        for (int yMid = (int)(height * 0.20f); yMid <= (int)(height * 0.80f); yMid += yStep) {
            vector<int> lum(width);
            for (int x = 0; x < width; ++x) {
                int y = yMid + (int)((x - width / 2) * tanA);
                if (y < 0) y = 0;
                if (y >= height) y = height - 1;
                size_t idx = (static_cast<size_t>(y) * width + x) * bpp;
                lum[x] = (pixelData[idx + 0] + pixelData[idx + 1] + pixelData[idx + 2]) / 3;
            }

            // Local moving average (adaptive thresholding window)
            vector<int> localAvg(width);
            int winR = 12;
            int sum = 0, count = 0;
            for (int i = 0; i <= winR && i < width; ++i) { sum += lum[i]; count++; }
            for (int x = 0; x < width; ++x) {
                int addIdx = x + winR + 1;
                int subIdx = x - winR;
                if (addIdx < width) { sum += lum[addIdx]; count++; }
                if (subIdx >= 0) { sum -= lum[subIdx]; count--; }
                localAvg[x] = sum / count;
            }

            // Test adaptive threshold deltas
            for (int delta : deltas) {
                vector<bool> isBar(width);
                for (int x = 0; x < width; ++x) {
                    isBar[x] = (lum[x] < localAvg[x] - delta);
                }

                // Forward runs
                vector<BarcodeRun> runs;
                bool cur = isBar[0];
                int len = 1;
                for (int x = 1; x < width; ++x) {
                    if (isBar[x] == cur) len++;
                    else {
                        runs.push_back({cur, len});
                        cur = isBar[x];
                        len = 1;
                    }
                }
                runs.push_back({cur, len});

                if (parseCode39Runs(runs, decodedCode)) {
                    errorMessage.clear();
                    return true;
                }

                // Reverse runs (in case phone/paper is rotated 180°)
                std::reverse(runs.begin(), runs.end());
                if (parseCode39Runs(runs, decodedCode)) {
                    std::reverse(decodedCode.begin(), decodedCode.end());
                    errorMessage.clear();
                    return true;
                }
            }
        }
    }

    errorMessage = "Optical pattern not detected. Ensure barcode is inside the red laser viewfinder.";
    return false;
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
        errorMessage = "Error: Only 24-bit BMP images are supported.";
        return false;
    }

    int width = header.biWidth;
    int height = abs(header.biHeight);
    if (width <= 0 || height <= 0) {
        errorMessage = "Error: Invalid image dimensions.";
        return false;
    }

    int rowStride = ((width * 3 + 3) / 4) * 4;
    vector<uint8_t> bmpData(rowStride * height);
    file.seekg(header.bfOffBits, ios::beg);
    file.read(reinterpret_cast<char*>(bmpData.data()), bmpData.size());
    file.close();

    // Convert BMP (bottom-up BGR) to top-down RGB
    vector<unsigned char> rgb(width * height * 3);
    for (int y = 0; y < height; ++y) {
        int srcY = (header.biHeight > 0) ? (height - 1 - y) : y;
        for (int x = 0; x < width; ++x) {
            size_t srcIdx = static_cast<size_t>(srcY) * rowStride + x * 3;
            size_t dstIdx = (static_cast<size_t>(y) * width + x) * 3;
            rgb[dstIdx + 0] = bmpData[srcIdx + 2]; // R
            rgb[dstIdx + 1] = bmpData[srcIdx + 1]; // G
            rgb[dstIdx + 2] = bmpData[srcIdx + 0]; // B
        }
    }

    return decodeBarcodeFromMemory(rgb.data(), width, height, false, decodedCode, errorMessage);
}

bool scanBarcodeFromCamera(string& decodedCode, string& errorMessage) {
    cout << "\n[CAMERA SCANNER]: Connecting to physical webcam stream...\n";
    string camName, camError;
    string capPath = "camera_capture.bmp";
    if (!captureFrameFromCamera(capPath, camName, camError)) {
        errorMessage = "Camera access error: " + (camError.empty() ? "No camera detected" : camError);
        return false;
    }
    cout << "[CAMERA SCANNER]: Frame captured from [" << camName << "] -> saved to " << capPath << "\n";
    cout << "[CAMERA SCANNER]: Analyzing optical stream...\n";
    return decodeBarcodeFromBMP(capPath, decodedCode, errorMessage);
}

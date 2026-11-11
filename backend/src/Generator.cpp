#include "Generator.h"
#include <iostream>
#include <fstream>
#include <cstdint>
using namespace std;

static const char* getCode39Pattern(char ch) {
    switch (ch) {
        case '0': return "NNNWWNWNN";
        case '1': return "WNNWNNNNW";
        case '2': return "NNWWNNNNW";
        case '3': return "WNWWNNNNN";
        case '4': return "NNNWWNNNW";
        case '5': return "WNNWWNNNN";
        case '6': return "NNWWWNNNN";
        case '7': return "NNNWNNWNW";
        case '8': return "WNNWNNWNN";
        case '9': return "NNWWNNWNN";
        case '*': return "NWNNWNWNN"; // Start / Stop delimiter
        default:  return nullptr;
    }
}

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType{0x4D42};     // "BM"
    uint32_t bfSize{0};
    uint16_t bfReserved1{0};
    uint16_t bfReserved2{0};
    uint32_t bfOffBits{54};
};

struct BMPInfoHeader {
    uint32_t biSize{40};
    int32_t  biWidth{0};
    int32_t  biHeight{0};
    uint16_t biPlanes{1};
    uint16_t biBitCount{24};     // 24-bit RGB
    uint32_t biCompression{0};  // Uncompressed BI_RGB
    uint32_t biSizeImage{0};
    int32_t  biXPelsPerMeter{2835};
    int32_t  biYPelsPerMeter{2835};
    uint32_t biClrUsed{0};
    uint32_t biClrImportant{0};
};
#pragma pack(pop)

bool generateBarcodeBMP(const string& sixDigitCode, const string& outputFilename) {
    if (sixDigitCode.length() != 6) {
        return false;
    }

    // Standard Code 39 framing: * + 6 digits + *
    string fullCode = "*" + sixDigitCode + "*";

    const int NARROW_WIDTH = 2; // Pixels for narrow element
    const int WIDE_WIDTH   = 6; // Pixels for wide element (3x narrow)
    const int QUIET_ZONE   = 30; // White border pixels on both sides
    const int IMAGE_HEIGHT = 140;

    // Standard fixed buffer without std::vector
    const int MAX_WIDTH = 512;
    bool scanline[MAX_WIDTH] = { false };
    int imageWidth = 0;

    // Left Quiet Zone (White = false)
    for (int i = 0; i < QUIET_ZONE && imageWidth < MAX_WIDTH; ++i) {
        scanline[imageWidth++] = false;
    }

    // Encode each character
    for (size_t c = 0; c < fullCode.length(); ++c) {
        const char* pattern = getCode39Pattern(fullCode[c]);
        if (!pattern) return false;

        for (int i = 0; i < 9; ++i) {
            bool isBar = (i % 2 == 0); // Even index = Bar (Black), Odd index = Space (White)
            int width = (pattern[i] == 'W') ? WIDE_WIDTH : NARROW_WIDTH;

            for (int w = 0; w < width && imageWidth < MAX_WIDTH; ++w) {
                scanline[imageWidth++] = isBar;
            }
        }

        // Inter-character narrow white space (except after last character)
        if (c + 1 < fullCode.length()) {
            for (int w = 0; w < NARROW_WIDTH && imageWidth < MAX_WIDTH; ++w) {
                scanline[imageWidth++] = false;
            }
        }
    }

    // Right Quiet Zone (White = false)
    for (int i = 0; i < QUIET_ZONE && imageWidth < MAX_WIDTH; ++i) {
        scanline[imageWidth++] = false;
    }

    int rowStride = ((imageWidth * 3 + 3) / 4) * 4; // BMP rows must be 4-byte aligned
    uint32_t imageSize = rowStride * IMAGE_HEIGHT;

    BMPFileHeader fileHeader;
    fileHeader.bfSize = 54 + imageSize;

    BMPInfoHeader infoHeader;
    infoHeader.biWidth = imageWidth;
    infoHeader.biHeight = IMAGE_HEIGHT;
    infoHeader.biSizeImage = imageSize;

    ofstream outFile(outputFilename, ios::binary);
    if (!outFile) {
        cerr << "Error: Unable to open file for writing: " << outputFilename << "\n";
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    outFile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    // Pixel row buffer without std::vector (BGR format for standard Windows BMP)
    const int MAX_ROW_BYTES = 2048;
    uint8_t rowBuffer[MAX_ROW_BYTES];
    for (int i = 0; i < rowStride; ++i) {
        rowBuffer[i] = 255; // initialize with white
    }

    // Write rows bottom-up
    for (int y = 0; y < IMAGE_HEIGHT; ++y) {
        for (int x = 0; x < imageWidth; ++x) {
            uint8_t color = scanline[x] ? 0 : 255; // 0 = Black, 255 = White
            rowBuffer[x * 3 + 0] = color; // B
            rowBuffer[x * 3 + 1] = color; // G
            rowBuffer[x * 3 + 2] = color; // R
        }
        outFile.write(reinterpret_cast<const char*>(rowBuffer), rowStride);
    }

    outFile.close();
    return true;
}

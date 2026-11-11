#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>

// Encodes a 6-digit numeric string into a Code 39 barcode and saves it as a .bmp file.
// Returns true on success, or false if generation fails.
bool generateBarcodeBMP(const std::string& sixDigitCode, const std::string& outputFilename);

#endif // GENERATOR_H

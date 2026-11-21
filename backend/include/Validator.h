#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <string>

// Checks that the input string is strictly composed of 6 numeric digits (1-9 for first digit, 0-9 for others).
// Returns true if valid, or false with an explanation in errorMessage.
bool isValidSixDigitCode(const std::string& input, std::string& errorMessage);

#endif // VALIDATOR_H

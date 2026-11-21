#include "Validator.h"
using namespace std;

bool isValidSixDigitCode(const string& input, string& errorMessage) {
    if (input.empty()) {
        errorMessage = "Error: Input is empty. Please enter a 6-digit number.";
        return false;
    }

    if (input.length() < 6) {
        errorMessage = "Error: Input is too short. Exactly 6 digits are required.";
        return false;
    }

    if (input.length() > 6) {
        errorMessage = "Error: Input is too long. Exactly 6 digits are required.";
        return false;
    }

    for (size_t i = 0; i < input.length(); i++) {
        // Check if the character is outside '0' to '9'
        if (input[i] < '0' || input[i] > '9') {
            errorMessage = "Error: Invalid character '";
            errorMessage += input[i];
            errorMessage += "' at position " + to_string(i + 1) + ". Only digits (0-9) are allowed.";
            return false;
        }
    }

    if (input[0] == '0') {
        errorMessage = "Error: First digit cannot be zero. Must start with 1-9.";
        return false;
    }

    errorMessage.clear();
    return true;
}

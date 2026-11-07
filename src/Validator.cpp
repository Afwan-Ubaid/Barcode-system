#include "Validator.h"
#include <cctype>
using namespace std;

bool isValidSixDigitCode(const string& input, string& errorMessage) {
    if (input.empty()) {
        errorMessage = "Error: Input is empty. Please enter a 6-digit number.";
        return false;
    }

    if (input.length() < 6) {
        errorMessage = "Error: Input is too short (" + to_string(input.length()) + 
                       " digits). Exactly 6 digits are required.";
        return false;
    }

    if (input.length() > 6) {
        errorMessage = "Error: Input is too long (" + to_string(input.length()) + 
                       " digits). Exactly 6 digits are required.";
        return false;
    }

    for (size_t i = 0; i < input.length(); ++i) {
        if (!isdigit(static_cast<unsigned char>(input[i]))) {
            errorMessage = "Error: Invalid character '" + string(1, input[i]) + 
                           "' at position " + to_string(i + 1) + ". Only digits (0-9) are allowed.";
            return false;
        }
    }

    errorMessage.clear();
    return true;
}

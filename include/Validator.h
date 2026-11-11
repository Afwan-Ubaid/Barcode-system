#ifndef VALIDATOR_H
#define VALIDATOR_H
#include <string>
using namespace std;
// show  that the input string is strictly composed of 6 numeric digits 0 to 9.
// Returns true if valid, or false with an explanation in errorMessage.
bool isValidSixDigitCode(const string& input, string& errorMessage);

#endif // VALIDATOR_H

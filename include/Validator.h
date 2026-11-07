#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <string>
using namespace std;

/**
 * ============================================================================
 * Module: Validator
 * Responsible Member: Member 3 (UI/UX & Validation Lead)
 * Jira Ticket: ISE-4 (Subtask 4.1)
 * Principles: Single Responsibility, Data Coupling, Intention-revealing naming
 * ============================================================================
 */

// Validates that the input string is strictly composed of 6 numeric digits ('0'-'9').
// Returns true if valid, or false with an explanation in errorMessage.
bool isValidSixDigitCode(const string& input, string& errorMessage);

#endif // VALIDATOR_H

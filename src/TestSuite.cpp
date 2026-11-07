#include "TestSuite.h"
#include "Validator.h"
#include "Generator.h"
#include "Decoder.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
using namespace std;

struct TestCaseResult {
    string id;
    string purpose;
    string input;
    string expected;
    string actual;
    bool passed;
};

void runAllTests() {
    cout << "\n========================================================================================\n";
    cout << "               AUTOMATED TEST SUITE EXECUTION (TDD - LECTURE 16 FORMAT)                 \n";
    cout << "========================================================================================\n";

    vector<TestCaseResult> results;
    string err;
    string decoded;

    // TC-01: Valid 6 Digits
    {
        bool ok = isValidSixDigitCode("123456", err);
        results.push_back({
            "TC-01", "Valid 6-digit input", "\"123456\"", 
            "Valid (true)", ok ? "Valid (true)" : "Invalid", ok
        });
    }

    // TC-02: Length < 6
    {
        bool ok = isValidSixDigitCode("12345", err);
        results.push_back({
            "TC-02", "Input length < 6", "\"12345\"", 
            "Invalid (false)", !ok ? "Invalid (false)" : "Valid", !ok
        });
    }

    // TC-03: Length > 6
    {
        bool ok = isValidSixDigitCode("1234567", err);
        results.push_back({
            "TC-03", "Input length > 6", "\"1234567\"", 
            "Invalid (false)", !ok ? "Invalid (false)" : "Valid", !ok
        });
    }

    // TC-04: Non-digit characters
    {
        bool ok = isValidSixDigitCode("12A456", err);
        results.push_back({
            "TC-04", "Alphanumeric input", "\"12A456\"", 
            "Invalid (false)", !ok ? "Invalid (false)" : "Valid", !ok
        });
    }

    // TC-05: Empty input
    {
        bool ok = isValidSixDigitCode("", err);
        results.push_back({
            "TC-05", "Empty string input", "\"\"", 
            "Invalid (false)", !ok ? "Invalid (false)" : "Valid", !ok
        });
    }

    // TC-06: Barcode Generation (Part 1)
    {
        bool ok = generateBarcodeBMP("654321", "test_generated.bmp");
        results.push_back({
            "TC-06", "BMP Barcode Generation", "\"654321\"", 
            "File created (true)", ok ? "File created (true)" : "Failed", ok
        });
    }

    // TC-07: Barcode Decoding (Part 2)
    {
        bool ok = decodeBarcodeFromBMP("test_generated.bmp", decoded, err);
        bool match = ok && (decoded == "654321");
        results.push_back({
            "TC-07", "BMP Barcode Decoding", "test_generated.bmp", 
            "\"654321\"", ok ? ("\"" + decoded + "\"") : "Failed", match
        });
    }

    // TC-08: Non-existent file decode error handling
    {
        bool ok = decodeBarcodeFromBMP("ghost_file.bmp", decoded, err);
        results.push_back({
            "TC-08", "Missing file handling", "ghost_file.bmp", 
            "Error (false)", !ok ? "Error (false)" : "Passed", !ok
        });
    }

    // Print Results Table
    cout << left 
         << setw(8)  << "ID" 
         << setw(25) << "Purpose" 
         << setw(20) << "Input Data" 
         << setw(22) << "Expected Output" 
         << setw(22) << "Actual Output" 
         << "Result\n";
    cout << string(105, '-') << "\n";

    int passCount = 0;
    for (const auto& r : results) {
        if (r.passed) passCount++;
        cout << left 
             << setw(8)  << r.id 
             << setw(25) << r.purpose 
             << setw(20) << r.input 
             << setw(22) << r.expected 
             << setw(22) << r.actual 
             << (r.passed ? "[PASS]" : "[FAIL]") << "\n";
    }

    cout << string(105, '-') << "\n";
    cout << "TEST EXECUTION SUMMARY: " << passCount << " / " << results.size() 
         << " Passed (" << (passCount == static_cast<int>(results.size()) ? "100% ALL TESTS GREEN" : "FAILURES DETECTED") 
         << ")\n";
    cout << "========================================================================================\n\n";
}

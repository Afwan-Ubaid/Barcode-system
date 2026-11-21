#include "TestSuite.h"
#include "Validator.h"
#include "Generator.h"
#include "Decoder.h"
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

struct TestCaseResult {
    string id;
    string purpose;
    string input;
    string expected;
    string actual;
    bool   passed;
};

void runAllTests() {
    cout << "  AUTOMATED TEST SUITE EXECUTION \n";
    string err;
    string decoded;
    const int NUM_TESTS = 9;
    TestCaseResult results[NUM_TESTS];
    bool ok;

    // TC 01: Valid 6-digit input → should pass
    ok = isValidSixDigitCode("123456", err);
    results[0] = { "TC-01", "Valid 6-digit input", "123456",
                   "true", ok ? "true" : "false", ok };

    // TC-02: Starts with zero → should fail
    ok = isValidSixDigitCode("012345", err);
    results[1] = { "TC-02", "First digit is zero", "012345",
                   "false", !ok ? "false" : "true", !ok };

    // TC-03: Too short → should fail
    ok = isValidSixDigitCode("12345", err);
    results[2] = { "TC-03", "Length less than 6", "12345",
                   "false", !ok ? "false" : "true", !ok };

    // TC-04: Too long → should fail
    ok = isValidSixDigitCode("1234567", err);
    results[3] = { "TC-04", "Length more than 6", "1234567",
                   "false", !ok ? "false" : "true", !ok };

    // TC-05: Has a letter → should fail
    ok = isValidSixDigitCode("12A456", err);
    results[4] = { "TC-05", "Contains a letter", "12A456",
                   "false", !ok ? "false" : "true", !ok };

    // TC-06: Empty string → should fail
    ok = isValidSixDigitCode("", err);
    results[5] = { "TC-06", "Empty input", "(empty)",
                   "false", !ok ? "false" : "true", !ok };

    // TC-07: Generate a BMP barcode file
    ok = generateBarcodeBMP("654321", "test_generated.bmp");
    results[6] = { "TC-07", "Barcode generation", "654321",
                   "true", ok ? "true" : "false", ok };

    // TC-08: Decode the BMP file → must match "654321"
    ok = decodeBarcodeFromBMP("test_generated.bmp", decoded, err);
    bool match = ok && (decoded == "654321");
    results[7] = { "TC-08", "Barcode decoding", "test_generated.bmp",
                   "654321", decoded.empty() ? "empty" : decoded, match };

    // TC-09: Decode a file that does not exist → should fail safely
    ok = decodeBarcodeFromBMP("ghost_file.bmp", decoded, err);
    results[8] = { "TC-09", "Missing file test", "ghost_file.bmp",
                   "false", !ok ? "false" : "true", !ok };

    cout << left
        << setw(8) << "ID"
        << setw(26) << "Purpose"
        << setw(20) << "Input"
        << setw(12) << "Expected"
        << setw(12) << "Actual"
        << "Result\n";
    cout << string(100, '-') << "\n";

    int passed = 0;
    for (int i = 0; i < NUM_TESTS; i++) {
        if (results[i].passed) passed++;
        cout << left
            << setw(8) << results[i].id
            << setw(26) << results[i].purpose
            << setw(20) << results[i].input
            << setw(12) << results[i].expected
            << setw(12) << results[i].actual
            << (results[i].passed ? "[PASS]" : "[FAIL]") << "\n";
    }

    cout << string(100, '-') << "\n";
    cout << "SUMMARY: " << passed << " / " << NUM_TESTS << " tests passed\n";
    if (passed == NUM_TESTS)
        cout << ">>> ALL TESTS PASSED SUCCESSFULLY! <<<\n";
    else
        cout << ">>> SOME TESTS FAILED <<<\n";
}

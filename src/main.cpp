#include <iostream>
#include <string>
#include "Validator.h"
#include "Generator.h"
#include "Decoder.h"
#include "TestSuite.h"
using namespace std;

void displayHeader() {
    cout << "\n===========================================================\n";
    cout << "      ISE SPRINT 1: SMART BARCODE SYSTEM (FAST-NUCES)      \n";
    cout << "  Adhering to Modular Design, OCP & UI Golden Rules (ISE)  \n";
    cout << "===========================================================\n";
    cout << "  [1] Part 1: Generate Barcode from 6-Digit Number\n";
    cout << "  [2] Part 2: Decode Barcode from Image File (.bmp)\n";
    cout << "  [3] Part 2: Scan Barcode via Camera Stream\n";
    cout << "  [4] QA: Run Automated TDD Test Suite (Lecture 16)\n";
    cout << "  [5] Exit Application\n";
    cout << "===========================================================\n";
    cout << "Select an option (1-5): ";
}

int main() {
    string choiceStr;

    while (true) {
        displayHeader();
        if (!getline(cin, choiceStr)) {
            break;
        }

        // Trim leading and trailing whitespace
        while (!choiceStr.empty() && (choiceStr.back() == ' ' || choiceStr.back() == '\r' || choiceStr.back() == '\t')) {
            choiceStr.pop_back();
        }
        while (!choiceStr.empty() && (choiceStr.front() == ' ' || choiceStr.front() == '\t')) {
            choiceStr.erase(choiceStr.begin());
        }

        if (choiceStr == "1") {
            cout << "\n--- [PART 1: BARCODE GENERATOR] ---\n";
            cout << "Enter a 6-digit number: ";
            string code;
            getline(cin, code);

            string errMsg;
            if (!isValidSixDigitCode(code, errMsg)) {
                cout << "\n[VALIDATION FAILED]: " << errMsg << "\n";
            } else {
                string filename = "barcode_" + code + ".bmp";
                if (generateBarcodeBMP(code, filename)) {
                    cout << "\n[SUCCESS]: Barcode successfully generated and saved to: " << filename << "\n";
                    cout << "You can open this image in Windows Photos or Paint to view the barcode!\n";
                } else {
                    cout << "\n[ERROR]: Failed to generate barcode image.\n";
                }
            }

        } else if (choiceStr == "2") {
            cout << "\n--- [PART 2: DECODE FROM IMAGE FILE] ---\n";
            cout << "Enter the image filename (e.g. 'barcode_123456.bmp'): ";
            string filename;
            getline(cin, filename);

            string decodedCode;
            string errMsg;
            if (decodeBarcodeFromBMP(filename, decodedCode, errMsg)) {
                cout << "\n[SUCCESS]: Barcode Successfully Decoded!\n";
                cout << ">> Extracted 6-Digit Code: [" << decodedCode << "]\n";
            } else {
                cout << "\n[DECODE FAILED]: " << errMsg << "\n";
            }

        } else if (choiceStr == "3") {
            cout << "\n--- [PART 2: LIVE CAMERA SCANNER] ---\n";
            string decodedCode;
            string errMsg;
            if (scanBarcodeFromCamera(decodedCode, errMsg)) {
                cout << "\n[SUCCESS]: Camera Recognized Barcode!\n";
                cout << ">> Extracted 6-Digit Code: [" << decodedCode << "]\n";
            } else {
                cout << "\n[CAMERA SCAN FAILED]: " << errMsg << "\n";
            }

        } else if (choiceStr == "4") {
            runAllTests();

        } else if (choiceStr == "5") {
            cout << "\nThank you for using the ISE Barcode System. Exiting...\n";
            break;

        } else {
            cout << "\n[INVALID INPUT]: Please enter a valid menu number between 1 and 5.\n";
        }

        cout << "\nPress Enter to return to main menu...";
        string dummy;
        getline(cin, dummy);
    }

    return 0;
}

#include <iostream>
#include <string>
#include <fstream>
#include "Validator.h"
#include "Generator.h"
#include "Decoder.h"
#include "TestSuite.h"

using namespace std;

// Check if a file exists using standard fstream (zero <filesystem> dependency)
static bool fileExists(const string& filename) {
    ifstream f(filename, ios::binary);
    return f.good();
}

void displayHeader() {
    cout << "        SMART BARCODE SYSTEM | BUILT BY INTRUDERS          \n";
    cout << "  [1] Part 1: Generate Barcode from 6-Digit Number\n";
    cout << "  [2] Part 2: Decode Barcode from Image File (.bmp)\n";
    cout << "  [3] Part 2: Scan Barcode via Camera Stream\n";
    cout << "  [4] QA: Run Automated TDD Test Suite\n";
    cout << "  [5] Exit Application\n";
    cout << "Select an option (1-5): ";
}

void handleGenerate() {
    while (true) {
        cout << "\n--- BARCODE GENERATOR ---\n";
        cout << "Enter a 6-digit number (must not start with 0, or 'M' for Main Menu): ";
        string code;
        if (!getline(cin, code)) return;

        // Trim whitespace
        while (!code.empty() && (code.back() == ' ' || code.back() == '\r' || code.back() == '\t')) code.pop_back();
        while (!code.empty() && (code.front() == ' ' || code.front() == '\t')) code.erase(code.begin());

        if (code == "M" || code == "m") return;

        string errMsg;
        if (!isValidSixDigitCode(code, errMsg)) {
            cout << "\nVALIDATION FAILED: " << errMsg << "\n";
            cout << "Do you want to try again? (y/n): ";
            string retry;
            getline(cin, retry);
            if (retry == "y" || retry == "Y") {
                continue;
            } else {
                return;
            }
        }

        string filename = "barcode_" + code + ".bmp";
        if (generateBarcodeBMP(code, filename)) {
            cout << "\nSUCCESS: Barcode successfully generated!\n";
            cout << ">> File saved: " << filename << "\n";
            cout << "You can view this file in Paint or Windows Photos.\n";
        } else {
            cout << "\n[ERROR]: Failed to generate barcode image file.\n";
        }
        break;
    }
}

void handleDecode() {
    while (true) {
        cout << "\n--- DECODE FROM IMAGE FILE ---\n";
        cout << "Enter image filename (e.g. 'barcode_123456.bmp' or 'test_generated.bmp'), or 'M' for Main Menu: ";

        string input;
        if (!getline(cin, input)) return;

        // Trim whitespace
        while (!input.empty() && (input.back() == ' ' || input.back() == '\r' || input.back() == '\t')) input.pop_back();
        while (!input.empty() && (input.front() == ' ' || input.front() == '\t')) input.erase(input.begin());

        if (input == "M" || input == "m") return;

        string targetFile = input;

        // Check if file exists in current folder or parent folder
        if (!fileExists(targetFile)) {
            if (fileExists("../" + targetFile)) {
                targetFile = "../" + targetFile;
            }
        }

        cout << "\nAttempting to decode: " << targetFile << " ...\n";
        string decodedCode;
        string errMsg;

        if (decodeBarcodeFromBMP(targetFile, decodedCode, errMsg)) {
            cout << "\nSUCCESS: Barcode Successfully Decoded!\n";
            cout << "==========================================\n";
            cout << ">> Extracted 6-Digit Code: [" << decodedCode << "]\n";
            cout << "==========================================\n";
            break;
        } else {
            cout << "\nDECODE FAILED: " << errMsg << "\n";
            cout << "Do you want to try another file? (y/n): ";
            string retry;
            getline(cin, retry);
            if (retry == "y" || retry == "Y") {
                continue;
            } else {
                return;
            }
        }
    }
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
            handleGenerate();
        } else if (choiceStr == "2") {
            handleDecode();
        } else if (choiceStr == "3") {
            cout << "\n--- LIVE CAMERA SCANNER ---\n";
            cout << "CAMERA: Point your camera at a 6-digit barcode and press Enter to scan...";
            string dummyWait;
            getline(cin, dummyWait);

            string decodedCode, errMsg;
            if (scanBarcodeFromCamera(decodedCode, errMsg)) {
                cout << "\nSUCCESS: Barcode Detected from camera stream!\n";
                cout << ">> Extracted 6-Digit Code: [" << decodedCode << "]\n";
            } else {
                cout << "\nCAMERA SCAN RESULT: " << errMsg << "\n";
                cout << "Tip: Ensure good lighting and keep the barcode parallel to the camera.\n";
            }
        } else if (choiceStr == "4") {
            runAllTests();
        } else if (choiceStr == "5") {
            cout << "\nThank you for using the Intruders Barcode System. Exiting...\n";
            break;
        } else {
            cout << "\nINVALID INPUT: Please enter a valid menu number between 1 and 5.\n";
        }

        cout << "\nPress Enter to return to main menu...";
        string dummy;
        getline(cin, dummy);
    }

    return 0;
}

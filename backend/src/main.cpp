#include <iostream>
#include <string>
#include "CliHandlers.h"
#include "TestSuite.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: High Cohesion & Low Coupling
// Module: Console CLI Entry Point
// ============================================================================

int main(int argc, char** argv) {
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--test" || arg == "-t") {
            runAllTests();
            return 0;
        }
    }

    std::string choiceStr;

    while (true) {
        displayHeader();
        if (!std::getline(std::cin, choiceStr)) break;

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
            handleLiveCameraScan();
        } else if (choiceStr == "4") {
            handleViewProductSpecs();
        } else if (choiceStr == "5") {
            handleRecordOrderAndGenerateBill();
        } else if (choiceStr == "6") {
            handleInventoryAudit();
        } else if (choiceStr == "7") {
            handleSalesAudit();
        } else if (choiceStr == "8") {
            runAllTests();
        } else if (choiceStr == "9") {
            std::cout << "\nThank you for using the Intruders Barcode System. Exiting...\n";
            break;
        } else {
            std::cout << "\nINVALID INPUT: Please enter a valid menu number between 1 and 9.\n";
        }

        std::cout << "\nPress Enter to return to main menu...";
        std::string dummy;
        std::getline(std::cin, dummy);
    }

    return 0;
}

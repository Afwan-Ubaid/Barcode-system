#include "CliHandlers.h"
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <iomanip>
#include <ctime>
#include <algorithm>

#include "Validator.h"
#include "Generator.h"
#include "Decoder.h"
#include "TestSuite.h"
#include "ProductManager.h"

using namespace std;

static bool fileExists(const string& filename) {
    ifstream f(filename, ios::binary);
    return f.good();
}

static string getCurrentDateYYYYMMDD() {
    time_t now = time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d", &tmNow);
    return string(buf);
}

static string getCurrentTimeHHMMSS() {
    time_t now = time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", &tmNow);
    return string(buf);
}

static int getCurrentDayOfMonth() {
    time_t now = time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    return tmNow.tm_mday;
}

void displayHeader() {
    cout << "\n============================================================\n";
    cout << "        SMART BARCODE SYSTEM | BUILT BY INTRUDERS          \n";
    cout << "============================================================\n";
    cout << "  [1] Part 1: Generate Barcode from 6-Digit Number\n";
    cout << "  [2] Part 2: Decode Barcode from Image File (.bmp)\n";
    cout << "  [3] Part 2: Scan Barcode via Camera Stream\n";
    cout << "  [4] Specs: View Product Specifications (Product.txt)\n";
    cout << "  [5] POS: Record Order & Generate Bill (Camera/Barcode)\n";
    cout << "  [6] Inventory: View Stock Audit & Restock (Product.txt)\n";
    cout << "  [7] Sales: View Completed Orders & Bills Log\n";
    cout << "  [8] QA: Run Automated TDD Test Suite\n";
    cout << "  [9] Exit Application\n";
    cout << "============================================================\n";
    cout << "Select an option (1-9): ";
}

void handleGenerate() {
    while (true) {
        cout << "\n--- BARCODE GENERATOR ---\n";
        cout << "Enter a 6-digit number (must not start with 0, or 'M' for Main Menu): ";
        string code;
        if (!getline(cin, code)) return;

        while (!code.empty() && (code.back() == ' ' || code.back() == '\r' || code.back() == '\t')) code.pop_back();
        while (!code.empty() && (code.front() == ' ' || code.front() == '\t')) code.erase(code.begin());

        if (code == "M" || code == "m") return;

        string errMsg;
        if (!isValidSixDigitCode(code, errMsg)) {
            cout << "\nVALIDATION FAILED: " << errMsg << "\n";
            cout << "Do you want to try again? (y/n): ";
            string retry;
            getline(cin, retry);
            if (retry == "y" || retry == "Y") continue;
            return;
        }

        string filename = "barcodes/barcode_" + code + ".bmp";
        if (!generateBarcodeBMP(code, filename)) {
            filename = "barcode_" + code + ".bmp";
            generateBarcodeBMP(code, filename);
        }

        cout << "\nSUCCESS: Barcode successfully generated!\n";
        cout << ">> File saved: " << filename << "\n";
        break;
    }
}

void handleDecode() {
    while (true) {
        cout << "\n--- DECODE FROM IMAGE FILE ---\n";
        cout << "Enter image filename (e.g. 'barcodes/barcode_100001.bmp' or 'test_generated.bmp'), or 'M' for Menu: ";
        string input;
        if (!getline(cin, input)) return;

        while (!input.empty() && (input.back() == ' ' || input.back() == '\r' || input.back() == '\t')) input.pop_back();
        while (!input.empty() && (input.front() == ' ' || input.front() == '\t')) input.erase(input.begin());

        if (input == "M" || input == "m") return;

        string targetFile = input;
        if (!fileExists(targetFile)) {
            if (fileExists("../" + targetFile)) targetFile = "../" + targetFile;
        }

        cout << "\nAttempting to decode: " << targetFile << " ...\n";
        string decodedCode, errMsg;
        if (decodeBarcodeFromBMP(targetFile, decodedCode, errMsg)) {
            cout << "\nSUCCESS: Barcode Successfully Decoded!\n";
            cout << "==========================================\n";
            cout << ">> Extracted 6-Digit Code: [" << decodedCode << "]\n";
            cout << "==========================================\n";

            vector<ProductItem> products;
            if (loadProducts("Product.txt", products)) {
                const ProductItem* p = findProductByBarcode(products, decodedCode);
                if (p) {
                    cout << ">> Linked Product: " << p->name << " (" << p->acronym << ")\n";
                    cout << ">> Unit Price: Rs " << p->unitPrice << " | Tax: Rs " << p->tax << " | Available Qty: " << p->stock << "\n";
                }
            }
            break;
        } else {
            cout << "\nDECODE FAILED: " << errMsg << "\n";
            cout << "Do you want to try another file? (y/n): ";
            string retry;
            getline(cin, retry);
            if (retry == "y" || retry == "Y") continue;
            return;
        }
    }
}

void handleLiveCameraScan() {
    cout << "\n--- LIVE CAMERA SCANNER ---\n";
    cout << "CAMERA: Point your camera at a 6-digit barcode and press Enter to scan...";
    string dummyWait;
    getline(cin, dummyWait);

    string decodedCode, errMsg;
    if (scanBarcodeFromCamera(decodedCode, errMsg)) {
        cout << "\nSUCCESS: Barcode Detected from camera stream!\n";
        cout << ">> Extracted 6-Digit Code: [" << decodedCode << "]\n";
        vector<ProductItem> products;
        if (loadProducts("Product.txt", products)) {
            const ProductItem* p = findProductByBarcode(products, decodedCode);
            if (p) {
                cout << ">> Product: " << p->name << " | Unit Price: Rs " << p->unitPrice << " | Stock: " << p->stock << "\n";
            }
        }
    } else {
        cout << "\nCAMERA SCAN RESULT: " << errMsg << "\n";
    }
}

void handleViewProductSpecs() {
    vector<ProductItem> products;
    if (!loadProducts("Product.txt", products)) {
        cout << "\n[ERROR] Failed to load Product.txt file.\n";
        return;
    }
    ensureAllBarcodesExist(products);

    cout << "\n--- VIEW PRODUCT SPECIFICATIONS (Product.txt) ---\n";
    cout << "Options: Enter [1] to enter 6-digit barcode, [2] to scan via camera, or [3] to view all products: ";
    string sub;
    getline(cin, sub);

    string barcodeToLookup;
    if (sub == "2") {
        cout << "Point camera at barcode and press Enter...";
        string dummy;
        getline(cin, dummy);
        string err;
        if (!scanBarcodeFromCamera(barcodeToLookup, err)) {
            cout << "Camera scan failed: " << err << "\n";
            return;
        }
        cout << "Scanned Barcode: " << barcodeToLookup << "\n";
    } else if (sub == "3") {
        cout << "\n====================================================================================================\n";
        cout << left << setw(6) << "ID"
             << setw(10) << "Barcode"
             << setw(10) << "Acronym"
             << setw(20) << "Name"
             << setw(10) << "Unit"
             << setw(12) << "Price(PKR)"
             << setw(10) << "Tax(PKR)"
             << setw(10) << "Discount"
             << setw(8)  << "Stock"
             << "Barcode Path\n";
        cout << string(100, '-') << "\n";
        for (const auto& p : products) {
            cout << left << setw(6) << p.id
                 << setw(10) << p.barcode
                 << setw(10) << p.acronym
                 << setw(20) << p.name.substr(0, 18)
                 << setw(10) << p.unit
                 << setw(12) << p.unitPrice
                 << setw(10) << p.tax
                 << setw(10) << p.discount
                 << setw(8)  << p.stock
                 << p.barcodePath << "\n";
        }
        cout << string(100, '-') << "\n";
        return;
    } else {
        cout << "Enter 6-digit Barcode (e.g. 100001): ";
        getline(cin, barcodeToLookup);
        while (!barcodeToLookup.empty() && (barcodeToLookup.back() == ' ' || barcodeToLookup.back() == '\r')) barcodeToLookup.pop_back();
    }

    const ProductItem* p = findProductByBarcode(products, barcodeToLookup);
    if (!p) {
        cout << "\n[NOT FOUND] No product with barcode '" << barcodeToLookup << "' found in Product.txt\n";
        return;
    }

    cout << "\n============================================================\n";
    cout << "             PRODUCT SPECIFICATIONS (Product.txt)           \n";
    cout << "============================================================\n";
    cout << "  Product ID:               " << p->id << "\n";
    cout << "  Product Barcode:          " << p->barcode << "\n";
    cout << "  Product Acronym:          " << p->acronym << " (max 8 chars)\n";
    cout << "  Product Name:             " << p->name << "\n";
    cout << "  Measurement Unit:         " << p->unit << " (count/weight)\n";
    cout << "  Unit Price (without tax): Rs " << p->unitPrice << " PKR\n";
    cout << "  Tax on Product:           Rs " << p->tax << " PKR\n";
    cout << "  Discount on 1 Unit:       Rs " << p->discount << " PKR\n";
    cout << "  Available Quantity:       " << p->stock << "\n";
    cout << "  Barcode File & Path:      " << p->barcodePath << (fileExists(p->barcodePath) ? " [File Exists]" : " [Pending]") << "\n";
    cout << "============================================================\n";
}

void handleRecordOrderAndGenerateBill() {
    vector<ProductItem> products;
    if (!loadProducts("Product.txt", products)) {
        cout << "\n[ERROR] Failed to load Product.txt\n";
        return;
    }
    ensureAllBarcodesExist(products);

    cout << "\n============================================================\n";
    cout << "          POS: RECORD ORDER & GENERATE BILL                 \n";
    cout << "============================================================\n";

    struct CartEntry {
        ProductItem item;
        int qty;
    };
    vector<CartEntry> cart;

    while (true) {
        cout << "\nAdd item to bill: Enter 6-digit barcode, 'C' to scan with camera, or 'D' when Done: ";
        string input;
        if (!getline(cin, input)) break;
        while (!input.empty() && (input.back() == ' ' || input.back() == '\r')) input.pop_back();
        if (input == "D" || input == "d") break;

        string barcode = input;
        if (input == "C" || input == "c") {
            cout << "Point camera at barcode and press Enter...";
            string dummy;
            getline(cin, dummy);
            string err;
            if (!scanBarcodeFromCamera(barcode, err)) {
                cout << "Camera scan failed: " << err << "\n";
                continue;
            }
            cout << "Scanned Barcode: " << barcode << "\n";
        }

        const ProductItem* p = findProductByBarcode(products, barcode);
        if (!p) {
            cout << "Product with barcode '" << barcode << "' not found in Product.txt\n";
            continue;
        }

        cout << "Found: " << p->name << " (" << p->acronym << ") | Unit Price: Rs " << p->unitPrice << " | Stock: " << p->stock << "\n";
        cout << "Enter quantity to purchase (or press Enter for 1): ";
        string qtyStr;
        getline(cin, qtyStr);
        int reqQty = 1;
        if (!qtyStr.empty()) {
            try { reqQty = stoi(qtyStr); } catch (...) { reqQty = 1; }
        }

        int existingInCart = 0;
        int cartIdx = -1;
        for (int i = 0; i < (int)cart.size(); i++) {
            if (cart[i].item.barcode == p->barcode) {
                existingInCart = cart[i].qty;
                cartIdx = i;
                break;
            }
        }

        int totalRequired = existingInCart + reqQty;

        if (totalRequired > p->stock) {
            cout << "\n[REJECTED] Cannot add product! Required quantity (" << totalRequired
                 << ") exceeds available stock (" << p->stock << ")!\n";
            continue;
        }

        if (cartIdx >= 0) {
            cart[cartIdx].qty = totalRequired;
        } else {
            cart.push_back({*p, reqQty});
        }
        cout << "[SUCCESS] Added " << reqQty << "x " << p->name << " to order (Total in bill: " << totalRequired << ")\n";
    }

    if (cart.empty()) {
        cout << "\nOrder cancelled: no items in cart.\n";
        return;
    }

    cout << "\nEnter Customer Name (e.g. Ahmed_Khan, or press Enter for Walkin_Customer): ";
    string custName;
    getline(cin, custName);
    if (custName.empty()) custName = "Walkin_Customer";
    for (char& c : custName) if (c == ' ') c = '_';

    cout << "Enter Customer Contact (e.g. 03001234567, or press Enter for None): ";
    string custContact;
    getline(cin, custContact);
    if (custContact.empty()) custContact = "None";

    cout << "Select Payment Mode: [1] Cash, [2] Card, [3] Wallet (Default: 1): ";
    string pModeChoice;
    getline(cin, pModeChoice);
    string paymentMode = "cash";
    if (pModeChoice == "2") paymentMode = "card";
    else if (pModeChoice == "3") paymentMode = "wallet";

    cout << "Enter Merchant Name (e.g. CashCounter, ABL, JazzCash, EasyPaisa, HBL): ";
    string merchant;
    getline(cin, merchant);
    if (merchant.empty()) {
        if (paymentMode == "cash") merchant = "CashCounter";
        else if (paymentMode == "card") merchant = "ABL";
        else merchant = "JazzCash";
    }
    for (char& c : merchant) if (c == ' ') c = '_';

    cout << "Select Order Status: [1] paid, [2] in-progress, [3] on-hold (Default: 1): ";
    string statusChoice;
    getline(cin, statusChoice);
    string orderStatus = "paid";
    if (statusChoice == "2") orderStatus = "in-progress";
    else if (statusChoice == "3") orderStatus = "on-hold";

    int totalWithoutTax = 0;
    int totalTax = 0;
    int totalDiscount = 0;

    for (const auto& entry : cart) {
        totalWithoutTax += entry.item.unitPrice * entry.qty;
        totalTax += entry.item.tax * entry.qty;
        totalDiscount += entry.item.discount * entry.qty;
    }
    int totalPayable = totalWithoutTax + totalTax - totalDiscount;

    vector<OrderRecord> existingOrders;
    loadOrders("Order.txt", existingOrders);
    vector<BillRecord> existingBills;
    loadBills("Bill.txt", existingBills);

    int dayOfMonth = getCurrentDayOfMonth();
    string orderId = generateNextOrderId(dayOfMonth, (int)existingOrders.size());
    string billId = generateNextBillId((int)existingBills.size());
    string orderDate = getCurrentDateYYYYMMDD();
    string orderTime = getCurrentTimeHHMMSS();

    OrderRecord orderRec;
    orderRec.orderId = orderId;
    orderRec.orderDate = orderDate;
    orderRec.orderTime = orderTime;
    orderRec.customerName = custName;
    orderRec.customerContact = custContact;
    orderRec.orderStatus = orderStatus;
    orderRec.paymentAmount = totalPayable;
    orderRec.paymentMode = paymentMode;
    orderRec.paymentMerchant = merchant;
    appendOrder("Order.txt", orderRec);

    for (const auto& entry : cart) {
        OrderDetailRecord detail;
        detail.orderId = orderId;
        detail.productId = entry.item.id;
        detail.productBarcode = entry.item.barcode;
        detail.quantity = entry.qty;
        appendOrderDetail("OrderDetail.txt", detail);
    }

    for (const auto& entry : cart) {
        BillRecord bill;
        bill.billId = billId;
        bill.customerName = custName;
        bill.orderId = orderId;
        bill.productAcronym = entry.item.acronym;
        bill.unitPrice = entry.item.unitPrice;
        bill.discountPerUnit = entry.item.discount;
        bill.quantity = entry.qty;
        bill.totalWithoutTax = totalWithoutTax;
        bill.totalTax = totalTax;
        bill.totalDiscount = totalDiscount;
        bill.totalPayable = totalPayable;
        bill.paymentStatus = orderStatus;
        appendBill("Bill.txt", bill);
    }

    for (const auto& entry : cart) {
        for (auto& p : products) {
            if (p.barcode == entry.item.barcode) {
                p.stock = max(0, p.stock - entry.qty);
                break;
            }
        }
    }
    saveProducts("Product.txt", products);

    cout << "\n============================================================\n";
    cout << "                    SCANIT OFFICIAL BILL                    \n";
    cout << "============================================================\n";
    cout << "Bill ID:             " << billId << "\n";
    cout << "Order ID:            " << orderId << " (Date: " << orderDate << " " << orderTime << ")\n";
    cout << "Customer Name:       " << custName << " (" << custContact << ")\n";
    cout << "Payment Mode:        " << paymentMode << " via " << merchant << "\n";
    cout << "Order Status:        " << orderStatus << "\n";
    cout << "------------------------------------------------------------\n";
    cout << left << setw(10) << "Acronym"
         << setw(20) << "Name"
         << setw(10) << "Price"
         << setw(8)  << "Disc"
         << setw(6)  << "Qty"
         << "Line Total\n";
    cout << string(60, '-') << "\n";
    for (const auto& entry : cart) {
        int lineTotal = entry.item.unitPrice * entry.qty;
        cout << left << setw(10) << entry.item.acronym
             << setw(20) << entry.item.name.substr(0, 18)
             << setw(10) << ("Rs " + to_string(entry.item.unitPrice))
             << setw(8)  << ("-" + to_string(entry.item.discount))
             << setw(6)  << entry.qty
             << "Rs " << lineTotal << "\n";
    }
    cout << string(60, '-') << "\n";
    cout << "Total Bill (without tax):                  Rs " << totalWithoutTax << "\n";
    cout << "Total Tax:                                 Rs " << totalTax << "\n";
    cout << "Total Discount:                           -Rs " << totalDiscount << "\n";
    cout << "============================================================\n";
    cout << "TOTAL PAYABLE BILL:                        Rs " << totalPayable << " PKR\n";
    cout << "============================================================\n";
    cout << "[SAVED] Order record saved to Order.txt\n";
    cout << "[SAVED] Order details saved to OrderDetail.txt\n";
    cout << "[SAVED] Bill record saved to Bill.txt\n";
    cout << "[UPDATED] Inventory stock updated in Product.txt\n";
}

void handleInventoryAudit() {
    vector<ProductItem> products;
    if (!loadProducts("Product.txt", products)) {
        cout << "\n[ERROR] Failed to load Product.txt\n";
        return;
    }

    cout << "\n====================================================================================================\n";
    cout << "                          INVENTORY & REAL-TIME STOCK AUDIT (Product.txt)                           \n";
    cout << "====================================================================================================\n";
    cout << left << setw(6) << "ID"
         << setw(10) << "Barcode"
         << setw(10) << "Acronym"
         << setw(20) << "Product Name"
         << setw(10) << "Unit"
         << setw(12) << "Price(PKR)"
         << setw(12) << "Available"
         << "Stock Status\n";
    cout << string(90, '-') << "\n";
    for (const auto& p : products) {
        string status = (p.stock == 0 ? "[OUT OF STOCK]" : (p.stock < 15 ? "[LOW STOCK]" : "[HEALTHY]"));
        cout << left << setw(6) << p.id
             << setw(10) << p.barcode
             << setw(10) << p.acronym
             << setw(20) << p.name.substr(0, 18)
             << setw(10) << p.unit
             << setw(12) << p.unitPrice
             << setw(12) << p.stock
             << status << "\n";
    }
    cout << string(90, '-') << "\n";

    cout << "\nDo you want to restock an item? (Enter product ID or 'M' for Menu): ";
    string pId;
    getline(cin, pId);
    if (pId == "M" || pId == "m" || pId.empty()) return;

    for (auto& p : products) {
        if (p.id == pId) {
            cout << "Enter quantity to add to '" << p.name << "' (Current: " << p.stock << "): ";
            string addStr;
            getline(cin, addStr);
            try {
                int addQty = stoi(addStr);
                p.stock += addQty;
                saveProducts("Product.txt", products);
                cout << "[SUCCESS] Restocked " << p.name << ". New Available Quantity: " << p.stock << "\n";
            } catch (...) {
                cout << "Invalid quantity entered.\n";
            }
            return;
        }
    }
    cout << "Product ID not found.\n";
}

void handleSalesAudit() {
    vector<OrderRecord> orders;
    loadOrders("Order.txt", orders);
    vector<BillRecord> bills;
    loadBills("Bill.txt", bills);

    cout << "\n====================================================================================================\n";
    cout << "                            SALES & ORDERS AUDIT LOG (Order.txt & Bill.txt)                         \n";
    cout << "====================================================================================================\n";
    if (orders.empty()) {
        cout << "No orders recorded yet in Order.txt\n";
        return;
    }

    cout << left << setw(10) << "Order ID"
         << setw(12) << "Date"
         << setw(10) << "Time"
         << setw(18) << "Customer"
         << setw(14) << "Contact"
         << setw(10) << "Status"
         << setw(12) << "Amount"
         << "Mode / Merchant\n";
    cout << string(95, '-') << "\n";
    int grandTotal = 0;
    for (const auto& o : orders) {
        cout << left << setw(10) << o.orderId
             << setw(12) << o.orderDate
             << setw(10) << o.orderTime
             << setw(18) << o.customerName.substr(0, 16)
             << setw(14) << o.customerContact
             << setw(10) << o.orderStatus
             << setw(12) << ("Rs " + to_string(o.paymentAmount))
             << (o.paymentMode + " (" + o.paymentMerchant + ")") << "\n";
        if (o.orderStatus == "paid") grandTotal += o.paymentAmount;
    }
    cout << string(95, '-') << "\n";
    cout << "Total Orders: " << orders.size() << " | Total Paid Revenue: Rs " << grandTotal << " PKR\n";
}

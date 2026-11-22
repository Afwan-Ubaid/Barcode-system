#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <timeapi.h>
#ifdef None
#undef None
#endif
#pragma comment(lib, "winmm.lib")

#include "PosApp.h"
#include "PosNavigation.h"
#include "PosProductsView.h"
#include "PosReceiptView.h"
#include "PosReportsView.h"
#include "PosModalsView.h"

#include <cmath>
#include <ctime>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

PosApp::PosApp() {
}

PosApp::~PosApp() {
    mCameraStream.stop();
    timeEndPeriod(1);
}

std::string PosApp::getAssetPath(const std::string& relPath) const {
    return mAssetsDir + "/" + relPath;
}

const sf::Texture& PosApp::getTexture(const std::string& relPath) {
    auto it = mTextures.find(relPath);
    if (it != mTextures.end()) return it->second;

    sf::Texture& tx = mTextures[relPath];
    std::string fullPath = getAssetPath(relPath);
    if (tx.loadFromFile(fullPath) || tx.loadFromFile(relPath)) {
        tx.setSmooth(true);
        tx.generateMipmap();
    } else {
        // Fallback checks for barcodes (e.g. barcodes/100001.bmp -> barcodes/barcode_100001.bmp)
        std::string alt = relPath;
        size_t bPos = alt.find("barcodes/");
        if (bPos != std::string::npos && alt.find("barcode_") == std::string::npos) {
            alt.insert(bPos + 9, "barcode_");
            if (tx.loadFromFile(getAssetPath(alt)) || tx.loadFromFile(alt)) {
                tx.setSmooth(true);
                tx.generateMipmap();
            }
        }
    }
    return tx;
}

void PosApp::updateLayout(float winW, float winH) {
    if (winH <= 0.f) winH = 1.f;
    float aspect = winW / winH;
    mCurrentWidth = std::max(1280.f, mDesignHeight * aspect);
    mPanelX = mCurrentWidth - Layout::PANEL_W;
    mRpX = mPanelX + 17.f;
    mMainX = mPanelX - 22.f - Layout::MAIN_W;

    sf::View view(sf::FloatRect(0, 0, mCurrentWidth, mDesignHeight));
    view.setViewport({0.f, 0.f, 1.f, 1.f});
    mWindow.setView(view);
}

struct ReturnOrderDetailItem {
    std::string orderId;
    std::string productId;
    std::string productBarcode;
    int quantity = 0;
};

static bool parseOrdersFile(const std::string& filename, std::vector<OrderRecord>& orders) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        in.open("../" + filename);
        if (!in.is_open()) return false;
    }
    orders.clear();
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '#') {
            line.pop_back();
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) line.pop_back();
        }
        std::istringstream iss(line);
        OrderRecord ord;
        if (iss >> ord.orderId >> ord.orderDate >> ord.orderTime >> ord.customerName >> ord.customerContact >> ord.orderStatus >> ord.paymentAmount >> ord.paymentMode >> ord.paymentMerchant) {
            orders.push_back(ord);
        }
    }
    return true;
}

static bool parseOrderDetailsFile(const std::string& filename, std::vector<ReturnOrderDetailItem>& details) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        in.open("../" + filename);
        if (!in.is_open()) return false;
    }
    details.clear();
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '#') {
            line.pop_back();
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) line.pop_back();
        }
        std::istringstream iss(line);
        ReturnOrderDetailItem d;
        if (iss >> d.orderId >> d.productId >> d.productBarcode >> d.quantity) {
            details.push_back(d);
        }
    }
    return true;
}

static bool parseBillsFile(const std::string& filename, std::vector<BillRecord>& bills) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        in.open("../" + filename);
        if (!in.is_open()) return false;
    }
    bills.clear();
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '#') {
            line.pop_back();
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) line.pop_back();
        }
        std::istringstream iss(line);
        BillRecord b;
        if (iss >> b.billId >> b.customerName >> b.orderId >> b.productAcronym >> b.unitPrice >> b.discountPerUnit >> b.quantity >> b.totalWithoutTax >> b.totalTax >> b.totalDiscount >> b.totalPayable >> b.paymentStatus) {
            bills.push_back(b);
        }
    }
    return true;
}

static void updateOrderStatusInFile(const std::string& filename, const std::string& orderId, const std::string& newStatus) {
    std::string targetFile = filename;
    std::ifstream in(targetFile);
    if (!in.is_open()) {
        targetFile = "../" + filename;
        in.open(targetFile);
        if (!in.is_open()) return;
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        std::string raw = line;
        while (!raw.empty() && (raw.back() == '\r' || raw.back() == ' ' || raw.back() == '\t')) raw.pop_back();
        std::istringstream iss(raw);
        std::string oId, oDate, oTime, cName, cContact, status;
        if (iss >> oId >> oDate >> oTime >> cName >> cContact >> status) {
            if (oId == orderId && status == "on-hold") {
                size_t pos = raw.find(" " + status + " ");
                if (pos != std::string::npos) {
                    raw.replace(pos + 1, status.length(), newStatus);
                }
            }
        }
        lines.push_back(raw);
    }
    in.close();

    std::ofstream out(targetFile);
    if (out.is_open()) {
        for (const auto& l : lines) {
            out << l << "\n";
        }
        out.close();
    }
}

bool PosApp::init(bool screenshotMode, bool paidMode, const std::string& modalShot) {
    mScreenshotMode = screenshotMode;
    mModalShot = modalShot;
    timeBeginPeriod(1);

    // Locate assets folder
    std::vector<fs::path> candidates = {"assets", "../assets", "../../assets", "frontend/assets"};
    for (auto& p : candidates) {
        if (fs::exists(p / "fonts" / "Lora.ttf")) {
            mAssetsDir = fs::absolute(p).string();
            break;
        }
    }
    if (mAssetsDir.empty()) {
        fputs("Error: Assets folder not found.\n", stderr);
        return false;
    }

    // Load fonts
    bool fontsLoaded = mFontRegular.loadFromFile(getAssetPath("fonts/Inter-Regular.otf")) &
                       mFontMedium.loadFromFile(getAssetPath("fonts/Inter-Medium.otf")) &
                       mFontSemiBold.loadFromFile(getAssetPath("fonts/Inter-SemiBold.otf")) &
                       mFontBold.loadFromFile(getAssetPath("fonts/Inter-Bold.otf")) &
                       mFontSerif.loadFromFile(getAssetPath("fonts/Lora.ttf"));
    if (!fontsLoaded) {
        fputs("Error: Fonts failed to load.\n", stderr);
        return false;
    }

    // Set initial cash given from cart total and sort databases alphabetically
    mProductCatalog.sortAlphabetically();
    mInventoryManager.sortAlphabetically();
    mCartService.setCashGiven(mCartService.getTotal(mProductCatalog.getProducts()));

    // Setup Window with Context Antialiasing
    sf::ContextSettings cs;
    cs.antialiasingLevel = 8;
    mWindow.create(sf::VideoMode(1280, 720), "ScanIT - Smart Barcode Studio & POS", sf::Style::Default, cs);
    mWindow.setFramerateLimit(120);
    updateLayout((float)mWindow.getSize().x, (float)mWindow.getSize().y);

    // Set Application Window Icon
    sf::Image appIcon;
    std::vector<std::string> iconCandidates = {
        getAssetPath("brand/scanit-bust-card.png"),
        getAssetPath("scanit-bust-card.png"),
        getAssetPath("icon.png"),
        getAssetPath("icon_3d.png"),
        getAssetPath("brand/window-icon-512.png")
    };
    for (const auto& ip : iconCandidates) {
        if (appIcon.loadFromFile(ip)) {
            mWindow.setIcon(appIcon.getSize().x, appIcon.getSize().y, appIcon.getPixelsPtr());
            break;
        }
    }

    // Initialize Splash & Login Screens
    mSplashScreen.init(mAssetsDir, mFontRegular, mFontSemiBold, mFontBold, mFontSerif);
    mLoginScreen.init(mAssetsDir, mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    mLoginScreen.setAuthCallback([this](const std::string& u, const std::string& p, const std::string& r) {
        return mStaffManager.authenticateUser(u, p, r);
    });

    // Start background video stream (auto-detects mobile phone camera first, else laptop webcam)
    mCameraRunning = mCameraStream.start(-1);

    // Mode routing
    if (mScreenshotMode) {
        if (mModalShot == "splash") {
            mShowSplash = true;
            mShowLogin = false;
        } else if (mModalShot == "login") {
            mShowSplash = false;
            mShowLogin = true;
        } else {
            mShowSplash = false;
            mShowLogin = false;
            mCurrentUserName = "admin";
            mCurrentRole = "Admin";

            if (paidMode) {
                mCartService.setCashGiven(1500);
                std::string msg;
                mCartService.chargeSale(mProductCatalog.getProducts(), mCurrentUserName, msg);
                bool ok = false;
                mCartService.printReceipt(mProductCatalog.getProducts(), mCurrentUserName, ok);
                mReceiptModalOpen = false;
            }

            if (mModalShot == "receipt") {
                mCartService.setCashGiven(1500);
                std::string msg;
                mCartService.chargeSale(mProductCatalog.getProducts(), mCurrentUserName, msg);
                recordSaleToOutputFiles(mCartService.getCart(), mCurrentUserName, 0);
                mProductCatalog.saveToFile();
                bool ok = false;
                mCartService.printReceipt(mProductCatalog.getProducts(), mCurrentUserName, ok);
                mReceiptModalOpen = true;
            } else if (mModalShot == "enlarge") {
                mEnlargeCartModalOpen = true;
            } else if (mModalShot == "cash") {
                mCashModalOpen = true;
            } else if (mModalShot == "card") {
                mCardModalOpen = true;
            } else if (mModalShot == "products") {
                mActiveTab = 1;
            } else if (mModalShot == "inventory") {
                mActiveTab = 2;
            } else if (mModalShot == "sales") {
                mActiveTab = 3;
            } else if (mModalShot == "return_item") {
                mActiveTab = 6;
                lookupReturnItem("100001");
            } else if (mModalShot == "return") {
                mActiveTab = 6;
                mSelectedReturnProductIdx = -1;
                mReturnBarcode = "";
            } else if (mModalShot == "quickcheck") {
                mQuickCheckModalOpen = true;
                mQuickCheckBarcode = "100001";
                mQuickCheckProductIdx = mProductCatalog.findProductByBarcode("100001");
            } else if (mModalShot == "spec") {
                mSpecModalOpen = true;
                mSpecProductIdx = 0;
            } else if (mModalShot == "desc") {
                mActiveTab = 1;
                mDescItemName = "Whole Milk 1L";
                mDescItemBarcode = "100001";
                mDescItemCategory = "Dairy";
                mDescItemUnit = "1 L";
                mDescItemPrice = 240;
                mDescItemCost = 180;
                mDescItemStock = 45;
                mDescItemSupplier = "Nestle Pakistan";
                mDescItemDate = "2026-11-17";
                mItemDescModalOpen = true;
            } else if (mModalShot == "addproduct") {
                mActiveTab = 1;
                mAddProductModalOpen = true;
            } else if (mModalShot == "drafts") {
                mCartService.parkCurrentCart(mProductCatalog.getProducts(), "Customer #1");
                mDraftsModalOpen = true;
            } else if (mModalShot == "addinv") {
                mActiveTab = 2;
                mAddInventoryModalOpen = true;
            } else if (mModalShot == "logs" || mModalShot == "activity") {
                mActiveTab = 4;
            } else if (mModalShot == "settings") {
                mActiveTab = 5;
            } else if (mModalShot == "addstaff") {
                mActiveTab = 5;
                mAddStaffModalOpen = true;
            } else if (mModalShot == "resetpass") {
                mActiveTab = 5;
                mResetPassModalOpen = true;
                mResetStaffTarget = "cashier1";
            } else if (mModalShot == "cashier") {
                mCurrentUserName = "Muya Khan";
                mCurrentRole = "Employee";
            } else if (mModalShot == "cashier_settings") {
                mActiveTab = 5;
                mCurrentUserName = "Muya Khan";
                mCurrentRole = "Employee";
            } else if (mModalShot == "cashier_inventory") {
                mActiveTab = 2;
                mCurrentUserName = "Muya Khan";
                mCurrentRole = "Employee";
            }
        }
    } else {
        mShowSplash = true;
        mShowLogin = false;
    }

    // Sync existing on-hold orders from Order.txt into CartService drafts (ISE-96/97)
    std::vector<OrderRecord> initOrders;
    std::vector<ReturnOrderDetailItem> initDetails;
    if (parseOrdersFile("Order.txt", initOrders) && parseOrderDetailsFile("OrderDetail.txt", initDetails)) {
        for (const auto& ord : initOrders) {
            if (ord.orderStatus == "on-hold") {
                DraftCart d;
                d.draftId = "Draft #" + std::to_string(mCartService.getDraftsCount() + 1);
                d.customerLabel = "Order #" + ord.orderId;
                d.time = ord.orderTime;
                d.totalAmount = ord.paymentAmount;
                d.orderId = ord.orderId;
                for (const auto& det : initDetails) {
                    if (det.orderId == ord.orderId) {
                        int pIdx = mProductCatalog.findProductByBarcode(det.productBarcode);
                        if (pIdx >= 0) {
                            d.items.push_back({pIdx, det.quantity});
                        }
                    }
                }
                if (!d.items.empty()) {
                    mCartService.getDraftCarts().push_back(d);
                }
            }
        }
    }
    mSearchFocused = true;

    return true;
}

void PosApp::showToast(const std::string& message) {
    mToastMessage = message;
    mToastTimer = 2.2f;
}

void PosApp::onBarcodeScanned(const std::string& barcode) {
    if (mQuickCheckModalOpen) {
        mQuickCheckBarcode = barcode;
        mQuickCheckProductIdx = mProductCatalog.findProductByBarcode(barcode);
        mFlashTimer = 0.4f;
        if (mQuickCheckProductIdx >= 0) {
            showToast("Quick Check: " + mProductCatalog.getProducts()[mQuickCheckProductIdx].name);
        } else {
            showToast("Quick Check: Barcode " + barcode + " not found");
        }
        return;
    }
    if (mActiveTab == 6) { // Return tab
        lookupReturnItem(barcode);
        mFlashTimer = 0.4f;
        showToast("Scanned for return: " + barcode);
        return;
    }
    if (mAddInventoryModalOpen && (mNewItemActiveField == 6 || mScanningUniversalSupplier)) {
        mNewItemBarcode = barcode;
        mScanningUniversalSupplier = false;
        showToast("Scanned Universal Barcode: " + barcode);
        return;
    }

    int idx = mProductCatalog.findProductByBarcode(barcode);
    if (idx >= 0) {
        const auto& p = mProductCatalog.getProducts()[idx];
        int curQty = 0;
        for (const auto& item : mCartService.getCart()) {
            if (item.productId == idx) { curQty = item.quantity; break; }
        }
        if (curQty + 1 > p.stock) {
            showToast("Stock limit: only " + std::to_string(p.stock) + " available for " + p.name);
            return;
        }
        mCartService.addItem(idx, 1);
        mFlashTimer = 0.4f;
        mLastScannedInfo = p.name + " (" + formatRs(p.price) + ")";
        mLastScanTimer = 3.0f;
        showToast("Scanned: " + p.name + " (" + formatRs(p.price) + ")");
    } else {
        showToast("Barcode " + barcode + " not in product.txt catalog");
    }
}

void PosApp::lookupReturnItem(const std::string& barcode) {
    mReturnBarcode = barcode;
    mReturnSuccess = false;
    mReturnStatusMessage = "";
    mReturnQty = 1;
    mReturnOrderId = "";
    mReturnCustomer = "";
    mReturnDate = "";
    mReturnSalePrice = 0;
    mReturnSaleInfo = "";

    mSelectedReturnProductIdx = mProductCatalog.findProductByBarcode(barcode);
    if (mSelectedReturnProductIdx < 0) {
        const auto* inv = mInventoryManager.findByBarcode(barcode);
        if (inv) {
            ProductItem p;
            p.barcode = inv->barcode;
            p.name = inv->name;
            p.category = inv->category;
            p.unit = inv->unit;
            p.price = inv->retailPrice;
            p.unitPrice = inv->retailPrice;
            p.stock = inv->stock;
            mProductCatalog.addOrUpdateProduct(p);
            mSelectedReturnProductIdx = mProductCatalog.findProductByBarcode(barcode);
        }
    }

    if (mSelectedReturnProductIdx >= 0) {
        const auto& p = mProductCatalog.getProducts()[mSelectedReturnProductIdx];
        mReturnSalePrice = p.price;

        std::vector<OrderRecord> orders;
        std::vector<ReturnOrderDetailItem> details;
        parseOrdersFile("Order.txt", orders);
        parseOrderDetailsFile("OrderDetail.txt", details);

        bool matched = false;
        for (int i = (int)details.size() - 1; i >= 0; --i) {
            if (details[i].productBarcode == barcode) {
                for (const auto& ord : orders) {
                    if (ord.orderId == details[i].orderId) {
                        mReturnOrderId = ord.orderId;
                        mReturnDate = ord.orderDate + " " + ord.orderTime;
                        mReturnCustomer = ord.customerName;
                        for (char& c : mReturnCustomer) if (c == '_') c = ' ';
                        mReturnSaleInfo = "Verified Purchase in Order #" + ord.orderId;
                        matched = true;
                        break;
                    }
                }
                if (matched) break;
            }
        }

        if (!matched) {
            std::vector<BillRecord> bills;
            parseBillsFile("Bill.txt", bills);
            for (int i = (int)bills.size() - 1; i >= 0; --i) {
                if (bills[i].productAcronym == p.acronym || bills[i].unitPrice == p.price) {
                    mReturnOrderId = bills[i].orderId;
                    mReturnCustomer = bills[i].customerName;
                    for (char& c : mReturnCustomer) if (c == '_') c = ' ';
                    mReturnDate = "2026-11-17 02:15:47";
                    mReturnSalePrice = bills[i].unitPrice;
                    mReturnSaleInfo = "Verified Bill #" + bills[i].billId;
                    matched = true;
                    break;
                }
            }
        }

        if (!matched) {
            mReturnSaleInfo = "Catalog Price: Rs " + std::to_string(p.price);
            mReturnSalePrice = p.price;
        }
        showToast("Located Item: " + p.name);
    } else {
        showToast("Item barcode " + barcode + " not found in database.");
    }
}

void PosApp::processReturn() {
    if (mSelectedReturnProductIdx < 0 || mSelectedReturnProductIdx >= (int)mProductCatalog.getProducts().size()) {
        showToast("Please scan or select a valid item first.");
        return;
    }
    const auto& p = mProductCatalog.getProducts()[mSelectedReturnProductIdx];
    int refundTotal = ((mReturnSalePrice > 0) ? mReturnSalePrice : p.price) * mReturnQty;

    // 1. Restock in product.txt (store shelf inventory)
    mProductCatalog.addStock(mSelectedReturnProductIdx, mReturnQty);

    // 2. Log Activity
    mActivityLogger.logActivity("RETURN", mCurrentUserName,
        "Customer returned " + std::to_string(mReturnQty) + "x " + p.name + " (" + p.barcode +
        ") - Refunded Rs " + std::to_string(refundTotal) + " - Restocked in shop product.txt");

    mReturnSuccess = true;
    mReturnStatusMessage = "Returned " + std::to_string(mReturnQty) + "x " + p.name +
        " -> Rs " + std::to_string(refundTotal) + " refunded to customer. Stock restored.";
    showToast("Return Processed: Stock restored to shop shelves (product.txt)");
}

void PosApp::recordSaleToOutputFiles(const std::vector<CartLine>& cartItems, const std::string& cashierName, int paymentMethod) {
    std::vector<OrderRecord> orders;
    parseOrdersFile("Order.txt", orders);
    std::vector<BillRecord> bills;
    parseBillsFile("Bill.txt", bills);

    std::time_t now = std::time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    int dayOfMonth = tmNow.tm_mday;

    int nextOrderNum = (int)orders.size() + 1;
    char oBuf[32];
    sprintf_s(oBuf, "%06d%02d", nextOrderNum, dayOfMonth);
    std::string orderId = oBuf;

    int nextBillNum = (int)bills.size() + 1;
    char bBuf[32];
    sprintf_s(bBuf, "%09d", nextBillNum);
    std::string billId = bBuf;

    char dBuf[32], tBuf[32];
    std::strftime(dBuf, sizeof(dBuf), "%Y-%m-%d", &tmNow);
    std::strftime(tBuf, sizeof(tBuf), "%H:%M:%S", &tmNow);

    auto& products = mProductCatalog.getProducts();
    int totalWithoutTax = 0;
    int totalTax = 0; // Tax removed
    int totalDiscount = 0;
    for (const auto& item : cartItems) {
        if (item.productId >= 0 && item.productId < (int)products.size()) {
            const auto& p = products[item.productId];
            totalWithoutTax += p.price * item.quantity;
            totalDiscount += p.discount * item.quantity;
        }
    }
    int totalPayable = totalWithoutTax - totalDiscount;

    std::string custName = cashierName.empty() ? "Store_Customer" : cashierName;
    for (char& c : custName) if (c == ' ') c = '_';
    std::string custPhone = "03001234567";
    std::string pMode = (paymentMethod == 0 ? "cash" : "card");
    std::string pMerchant = (paymentMethod == 0 ? "CashCounter" : "BankPOS");

    // 1. Append to Order.txt (Output file 1)
    std::ofstream outOrd("Order.txt", std::ios::app);
    if (outOrd.is_open()) {
        outOrd << orderId << " " << dBuf << " " << tBuf << " "
               << custName << " " << custPhone << " paid "
               << totalPayable << " " << pMode << " " << pMerchant << " #\n";
        outOrd.close();
    }

    // 2. Append to OrderDetail.txt (Output file 2)
    std::ofstream outDet("OrderDetail.txt", std::ios::app);
    if (outDet.is_open()) {
        for (const auto& item : cartItems) {
            if (item.productId >= 0 && item.productId < (int)products.size()) {
                const auto& p = products[item.productId];
                outDet << orderId << " " << p.id << " " << p.barcode << " " << item.quantity << " #\n";
            }
        }
        outDet.close();
    }

    // 3. Append to Bill.txt (Output file 3)
    std::ofstream outBill("Bill.txt", std::ios::app);
    if (outBill.is_open()) {
        for (const auto& item : cartItems) {
            if (item.productId >= 0 && item.productId < (int)products.size()) {
                const auto& p = products[item.productId];
                outBill << billId << " " << custName << " " << orderId << " "
                        << (p.acronym.empty() ? p.name.substr(0, 8) : p.acronym) << " "
                        << p.price << " " << p.discount << " " << item.quantity << " "
                        << totalWithoutTax << " " << totalTax << " " << totalDiscount << " "
                        << totalPayable << " paid #\n";
            }
        }
        outBill.close();
    }
}

void PosApp::holdCurrentOrder() {
    if (mCartService.isEmpty()) {
        showToast("Cart is empty - nothing to hold");
        return;
    }

    std::vector<OrderRecord> orders;
    parseOrdersFile("Order.txt", orders);

    std::time_t now = std::time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    int dayOfMonth = tmNow.tm_mday;

    int nextOrderNum = (int)orders.size() + 1;
    char oBuf[32];
    sprintf_s(oBuf, "%06d%02d", nextOrderNum, dayOfMonth);
    std::string orderId = oBuf;

    char dBuf[32], tBuf[32];
    std::strftime(dBuf, sizeof(dBuf), "%Y-%m-%d", &tmNow);
    std::strftime(tBuf, sizeof(tBuf), "%H:%M:%S", &tmNow);

    auto& products = mProductCatalog.getProducts();
    int totalWithoutTax = 0;
    int totalDiscount = 0;
    for (const auto& item : mCartService.getCart()) {
        if (item.productId >= 0 && item.productId < (int)products.size()) {
            const auto& p = products[item.productId];
            totalWithoutTax += p.price * item.quantity;
            totalDiscount += p.discount * item.quantity;
        }
    }
    int totalPayable = totalWithoutTax - totalDiscount;

    std::string custName = mCurrentUserName.empty() ? "Store_Customer" : mCurrentUserName;
    for (char& c : custName) if (c == ' ') c = '_';
    std::string custPhone = "03001234567";

    // 1. Append to Order.txt (with status on-hold)
    std::ofstream outOrd("Order.txt", std::ios::app);
    if (!outOrd.is_open()) {
        outOrd.open("../Order.txt", std::ios::app);
    }
    if (outOrd.is_open()) {
        outOrd << orderId << " " << dBuf << " " << tBuf << " "
               << custName << " " << custPhone << " on-hold "
               << totalPayable << " cash CashCounter #\n";
        outOrd.close();
    }

    // 2. Append item lines to OrderDetail.txt
    std::ofstream outDet("OrderDetail.txt", std::ios::app);
    if (!outDet.is_open()) {
        outDet.open("../OrderDetail.txt", std::ios::app);
    }
    if (outDet.is_open()) {
        for (const auto& item : mCartService.getCart()) {
            if (item.productId >= 0 && item.productId < (int)products.size()) {
                const auto& p = products[item.productId];
                outDet << orderId << " " << p.id << " " << p.barcode << " " << item.quantity << " #\n";
            }
        }
        outDet.close();
    }

    // 3. Park in CartService drafts with orderId
    mCartService.parkCurrentCart(products, "Order #" + orderId, orderId);

    mReceiptScrollOffset = 0.f;
    mActivityLogger.logActivity("ORDER", mCurrentUserName, "Order #" + orderId + " placed ON-HOLD (" + formatRs(totalPayable) + ")");
    showToast("Order #" + orderId + " placed on hold (H / F4)");
}

void PosApp::resumeLatestOrder() {
    auto& drafts = mCartService.getDrafts();
    if (drafts.empty()) {
        // Fallback: check Order.txt for latest on-hold order
        std::vector<OrderRecord> orders;
        std::vector<ReturnOrderDetailItem> details;
        if (parseOrdersFile("Order.txt", orders) && parseOrderDetailsFile("OrderDetail.txt", details)) {
            for (int i = (int)orders.size() - 1; i >= 0; --i) {
                if (orders[i].orderStatus == "on-hold") {
                    DraftCart d;
                    d.draftId = "Draft #" + std::to_string(mCartService.getDraftsCount() + 1);
                    d.customerLabel = "Order #" + orders[i].orderId;
                    d.time = orders[i].orderTime;
                    d.totalAmount = orders[i].paymentAmount;
                    d.orderId = orders[i].orderId;
                    for (const auto& det : details) {
                        if (det.orderId == orders[i].orderId) {
                            int pIdx = mProductCatalog.findProductByBarcode(det.productBarcode);
                            if (pIdx >= 0) {
                                d.items.push_back({pIdx, det.quantity});
                            }
                        }
                    }
                    if (!d.items.empty()) {
                        mCartService.getDraftCarts().push_back(d);
                    }
                    break;
                }
            }
        }
    }

    if (mCartService.getDrafts().empty()) {
        showToast("No on-hold orders to resume");
        return;
    }

    // If active cart already has items, auto-park it first so the cashier does not lose current scan
    if (!mCartService.isEmpty()) {
        auto& products = mProductCatalog.getProducts();
        DraftCart activeDraft;
        activeDraft.draftId = "Draft #" + std::to_string(mCartService.getDraftsCount() + 1);
        activeDraft.customerLabel = "Active Cart (" + getCurrentTimeHHMMSS() + ")";
        activeDraft.time = getCurrentTimeHHMMSS();
        activeDraft.items = mCartService.getCart();
        activeDraft.totalAmount = mCartService.getTotal(products);
        mCartService.getDraftCarts().insert(mCartService.getDraftCarts().begin(), activeDraft);
    }

    // The latest on-hold order is the last draft
    int lastIdx = mCartService.getDraftsCount() - 1;
    DraftCart d = mCartService.getDrafts()[lastIdx];
    mCartService.deleteDraft(lastIdx);

    mCartService.getCart() = d.items;
    mReceiptScrollOffset = 0.f;

    if (!d.orderId.empty()) {
        updateOrderStatusInFile("Order.txt", d.orderId, "resumed");
        mActivityLogger.logActivity("ORDER", mCurrentUserName, "Resumed on-hold order #" + d.orderId);
        showToast("Resumed on-hold order #" + d.orderId + " (R / F5)");
    } else {
        mActivityLogger.logActivity("ORDER", mCurrentUserName, "Resumed draft sale (" + d.customerLabel + ")");
        showToast("Resumed draft: " + d.customerLabel + " (R / F5)");
    }
}

void PosApp::generateNewItemBarcode() {
    std::string newBc = mInventoryManager.generateUniqueBarcode();
    mNewItemBarcode = newBc;
    mNewItemGenerated = true;

    // Generate physical BMP file
    std::string outPath = "barcodes/barcode_" + newBc + ".bmp";
    generateBarcodeBMP(newBc, outPath);

    // Also mirror to assets
    std::string assetPath = "assets/barcodes/barcode_" + newBc + ".bmp";
    generateBarcodeBMP(newBc, assetPath);

    showToast("Generated Barcode: " + newBc);
}

void PosApp::saveNewInventoryItem() {
    if (mNewItemDesc.empty()) {
        showToast("Please enter item description / name");
        return;
    }
    if (mNewItemBarcode.empty()) {
        generateNewItemBarcode();
    }
    int retPrice = 100, costPrice = 80, qty = 20;
    try { retPrice = std::stoi(mNewItemPriceStr); } catch (...) {}
    try { costPrice = std::stoi(mNewItemCostStr); } catch (...) {}
    try { qty = std::stoi(mNewItemQtyStr); } catch (...) {}

    // Ensure barcode BMP exists
    std::string bmpPath = "barcodes/barcode_" + mNewItemBarcode + ".bmp";
    if (!std::filesystem::exists(bmpPath)) {
        generateBarcodeBMP(mNewItemBarcode, bmpPath);
    }

    InventoryItem invItem;
    invItem.barcode = mNewItemBarcode;
    invItem.name = mNewItemDesc;
    invItem.category = mNewItemCategory;
    invItem.unit = mNewItemUnit;
    invItem.retailPrice = retPrice;
    invItem.costPrice = costPrice;
    invItem.stock = qty;
    invItem.supplier = mNewItemSupplier;
    invItem.arrivalDate = mNewItemDateStr;
    invItem.barcodePath = bmpPath;

    mInventoryManager.addOrRestockItem(invItem, qty, costPrice, mNewItemSupplier, mNewItemDateStr);

    ProductItem p;
    p.barcode = mNewItemBarcode;
    p.name = mNewItemDesc;
    p.category = mNewItemCategory;
    p.unit = mNewItemUnit;
    p.price = retPrice;
    p.unitPrice = retPrice;
    p.stock = qty;
    p.barcodePath = bmpPath;
    mProductCatalog.addOrUpdateProduct(p);

    mActivityLogger.logActivity("INVENTORY", mCurrentUserName,
        "Bought " + std::to_string(qty) + " units of " + mNewItemDesc + " (" + mNewItemBarcode +
        ") from " + mNewItemSupplier + " at Rs " + std::to_string(costPrice) + "/unit on " + mNewItemDateStr);

    mAddInventoryModalOpen = false;
    showToast("Added " + mNewItemDesc + " to inventory.txt & product.txt!");
}

void PosApp::simulateScan(int productId) {
    const auto& products = mProductCatalog.getProducts();
    if (products.empty()) return;
    if (productId < 0) {
        productId = mScanSimIndex++ % (int)products.size();
    }
    onBarcodeScanned(products[productId].barcode);
}

void PosApp::handleScroll(float delta, sf::Vector2f m) {
    const auto& products = mProductCatalog.getProducts();
    const auto& cart = mCartService.getCart();

    if (mEnlargeCartModalOpen) {
        float maxScroll = std::max(0.f, (float)(cart.size() - 6) * 56.f);
        mEnlargeScrollOffset = std::clamp(mEnlargeScrollOffset - delta * 36.f, 0.f, maxScroll);
        return;
    }

    if (mAddProductModalOpen) {
        float maxScroll = std::max(0.f, (float)(mInventoryManager.getItems().size() - 6) * 42.f);
        mAddProductScrollOffset = std::clamp(mAddProductScrollOffset - delta * 30.f, 0.f, maxScroll);
        return;
    }

    if (mActiveTab == 1) {
        float maxScroll = std::max(0.f, (float)(products.size() - 8) * 46.f);
        mProductsScrollOffset = std::clamp(mProductsScrollOffset - delta * 30.f, 0.f, maxScroll);
        return;
    } else if (mActiveTab == 2) {
        if (mInventorySubTab == 1) {
            float maxScroll = std::max(0.f, (float)(mInventoryManager.getPurchases().size() - 9) * 38.f);
            mSupplierScrollOffset = std::clamp(mSupplierScrollOffset - delta * 30.f, 0.f, maxScroll);
        } else {
            float maxScroll = std::max(0.f, (float)(mInventoryManager.getItems().size() - 7) * 56.f);
            mInventoryScrollOffset = std::clamp(mInventoryScrollOffset - delta * 30.f, 0.f, maxScroll);
        }
        return;
    } else if (mActiveTab == 3) {
        float maxScroll = std::max(0.f, (float)(mCartService.getSalesHistory().size() - 6) * 54.f);
        mSalesScrollOffset = std::clamp(mSalesScrollOffset - delta * 30.f, 0.f, maxScroll);
        return;
    } else if (mActiveTab == 4) {
        float maxScroll = std::max(0.f, (float)(mActivityLogger.getLogs().size() - 10) * 34.f);
        mLogScrollOffset = std::clamp(mLogScrollOffset - delta * 30.f, 0.f, maxScroll);
        return;
    }

    if (pointInRect(m, mRpX, Layout::RP_Y, Layout::RP_W, Layout::RP_H)) {
        float maxScroll = std::max(0.f, (float)(cart.size() - 5) * Layout::ROW_H);
        mReceiptScrollOffset = std::clamp(mReceiptScrollOffset - delta * 32.f, 0.f, maxScroll);
    }
}

void PosApp::handleClick(sf::Vector2f m) {
    auto& products = mProductCatalog.getProducts();
    auto& cart = mCartService.getCart();

    // 1. Receipt Modal
    if (mReceiptModalOpen) {
        float rw = 440.f, rh = 600.f;
        float rx = (mCurrentWidth - rw) / 2.f;
        float ry = (mDesignHeight - (rh + 52.f)) / 2.f;
        float btnY = ry + rh + 12.f;

        // Close Button
        if (pointInRect(m, rx, btnY, 210.f, 38.f)) {
            mReceiptModalOpen = false;
            return;
        }
        // Start New Sale Button
        if (pointInRect(m, rx + 230.f, btnY, 210.f, 38.f)) {
            mReceiptModalOpen = false;
            mCartService.clear();
            showToast("Ready for new sale transaction");
            return;
        }
        // Click outside paper and buttons closes modal
        if (!pointInRect(m, rx - 12.f, ry - 12.f, rw + 24.f, rh + 68.f)) {
            mReceiptModalOpen = false;
        }
        return;
    }

    // Quick Check Overlay (ISE-56 Standalone Scanner Mode)
    if (mQuickCheckModalOpen) {
        float dw = 560.f, dh = 480.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        // Close button [X]
        if (pointInRect(m, dx + dw - 44, dy + 22, 28, 28)) {
            mQuickCheckModalOpen = false;
            return;
        }

        // Demo quick test pills
        const char* demoCodes[] = {"100001", "100002", "100004", "100006", "234567"};
        for (int i = 0; i < 5; ++i) {
            float cx = dx + 90 + i * 62;
            if (pointInRect(m, cx, dy + 122, 56, 20)) {
                mQuickCheckBarcode = demoCodes[i];
                mQuickCheckProductIdx = mProductCatalog.findProductByBarcode(mQuickCheckBarcode);
                if (mQuickCheckProductIdx >= 0) {
                    showToast("Quick Check: " + mProductCatalog.getProducts()[mQuickCheckProductIdx].name);
                }
                return;
            }
        }

        // Search / Barcode Input Box
        if (pointInRect(m, dx + 28, dy + 78, dw - 56.f, 38)) {
            mQuickCheckInputFocused = true;
            return;
        }

        // Bottom Action Buttons
        float btnY = dy + dh - 54.f;
        // Close (ESC)
        if (pointInRect(m, dx + 28, btnY, 180, 36)) {
            mQuickCheckModalOpen = false;
            return;
        }

        // + Add to Active Cart (safe non-intrusive addition to existing cart)
        if (mQuickCheckProductIdx >= 0 && pointInRect(m, dx + dw - 238, btnY, 210, 36)) {
            mCartService.addItem(mQuickCheckProductIdx, 1);
            showToast("Added " + mProductCatalog.getProducts()[mQuickCheckProductIdx].name + " to cart");
            mQuickCheckModalOpen = false;
            return;
        }

        // Click outside closes modal
        if (!pointInRect(m, dx, dy, dw, dh)) {
            mQuickCheckModalOpen = false;
        }
        return;
    }

    // Product Specifications Modal (ISE-57 Requirement 5 Card)
    if (mSpecModalOpen) {
        float dw = 620.f, dh = 480.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        // Close button [X]
        if (pointInRect(m, dx + dw - 44, dy + 22, 28, 28)) {
            mSpecModalOpen = false;
            return;
        }

        // Close Details button
        if (pointInRect(m, dx + dw / 2.f - 75, dy + dh - 52, 150, 36)) {
            mSpecModalOpen = false;
            return;
        }

        // Click outside closes modal
        if (!pointInRect(m, dx, dy, dw, dh)) {
            mSpecModalOpen = false;
        }
        return;
    }

    // Item Description Popup Modal
    if (mItemDescModalOpen) {
        float dw = 500.f, dh = 460.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 44, dy + 20, 28, 28)) {
            mItemDescModalOpen = false;
            return;
        }
        if (pointInRect(m, dx + dw / 2.f - 60, dy + dh - 48, 120, 34)) {
            mItemDescModalOpen = false;
            return;
        }
        if (!pointInRect(m, dx, dy, dw, dh)) {
            mItemDescModalOpen = false;
        }
        return;
    }

    // Held Drafts / Parked Carts Modal
    if (mDraftsModalOpen) {
        float dw = 620.f, dh = 460.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 40, dy + 20, 26, 26)) {
            mDraftsModalOpen = false;
            return;
        }
        if (pointInRect(m, dx + dw / 2.f - 60, dy + dh - 48, 120, 34)) {
            mDraftsModalOpen = false;
            return;
        }

        const auto& drafts = mCartService.getDrafts();
        float listY = dy + 85.f;
        for (size_t d = 0; d < drafts.size(); d++) {
            float ry = listY + d * 64.f;
            if (ry > dy + dh - 80.f) break;

            if (pointInRect(m, dx + dw - 200, ry + 12, 90, 32)) {
                std::string custLabel = (d < drafts.size()) ? drafts[d].customerLabel : "Customer";
                std::string oId = (d < drafts.size()) ? drafts[d].orderId : "";
                mCartService.resumeDraft((int)d);
                if (!oId.empty()) {
                    updateOrderStatusInFile("Order.txt", oId, "resumed");
                }
                mDraftsModalOpen = false;
                showToast("Resumed draft sale for " + custLabel);
                return;
            }
            if (pointInRect(m, dx + dw - 100, ry + 12, 70, 32)) {
                mCartService.deleteDraft((int)d);
                showToast("Draft sale discarded");
                return;
            }
        }

        if (!pointInRect(m, dx, dy, dw, dh)) {
            mDraftsModalOpen = false;
        }
        return;
    }

    // Add Product From Inventory Modal
    if (mAddProductModalOpen) {
        float dw = 780.f, dh = 560.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 42, dy + 20, 26, 26)) {
            mAddProductModalOpen = false;
            return;
        }

        float sBoxX = dx + 28.f, sBoxY = dy + 70.f, sBoxW = 340.f, sBoxH = 32.f;
        if (pointInRect(m, sBoxX, sBoxY, sBoxW, sBoxH)) {
            mAddProductSearchFocused = true;
            return;
        } else {
            mAddProductSearchFocused = false;
        }

        std::vector<int> visibleIndices = mInventoryManager.getFilteredItemIndices(mAddProductSearchQuery);
        float ty = dy + 112.f;
        float listY = ty + 32.f;

        for (size_t vi = 0; vi < visibleIndices.size(); vi++) {
            int idx = visibleIndices[vi];
            float ry = listY + vi * 42.f - mAddProductScrollOffset;
            if (ry < listY - 5.f || ry > listY + 260.f) continue;

            if (pointInRect(m, dx + 20, ry, dw - 40, 38)) {
                mSelectedInventoryForProduct = idx;
                return;
            }
        }

        float botY = dy + dh - 64.f;
        if (mSelectedInventoryForProduct >= 0 && mSelectedInventoryForProduct < (int)mInventoryManager.getItems().size()) {
            float qx = dx + 360;
            if (pointInRect(m, qx + 70, botY + 4, 26, 26)) {
                int q = 1;
                try { q = std::stoi(mTransferQtyStr); } catch (...) {}
                if (q > 1) q--;
                mTransferQtyStr = std::to_string(q);
                return;
            }
            if (pointInRect(m, qx + 146, botY + 4, 26, 26)) {
                int q = 1;
                try { q = std::stoi(mTransferQtyStr); } catch (...) {}
                q++;
                mTransferQtyStr = std::to_string(q);
                return;
            }

            if (pointInRect(m, dx + dw - 220, botY + 3, 200, 32)) {
                const auto& it = mInventoryManager.getItems()[mSelectedInventoryForProduct];
                int q = 10;
                try { q = std::stoi(mTransferQtyStr); } catch (...) {}
                if (q <= 0) q = 1;
                if (q > it.stock) q = it.stock;

                if (q <= 0) {
                    showToast("Error: 0 units available in inventory for " + it.name);
                    return;
                }

                mInventoryManager.updateItemStock(it.barcode, it.stock - q);

                int pIdx = mProductCatalog.findProductByBarcode(it.barcode);
                if (pIdx >= 0) {
                    mProductCatalog.addStock(pIdx, q);
                } else {
                    ProductItem newP;
                    newP.barcode = it.barcode;
                    newP.name = it.name;
                    newP.category = it.category;
                    newP.unit = it.unit;
                    newP.price = it.retailPrice;
                    newP.unitPrice = it.retailPrice;
                    newP.stock = q;
                    newP.barcodePath = it.barcodePath;
                    newP.iconKey = "apple";
                    mProductCatalog.addProduct(newP);
                }
                mProductCatalog.sortAlphabetically();

                mActivityLogger.logActivity("INVENTORY", mCurrentUserName,
                    "Stocked " + std::to_string(q) + " units of " + it.name + " into retail catalog");
                showToast("Stocked " + std::to_string(q) + " units of " + it.name + " into catalog!");
                mAddProductModalOpen = false;
                return;
            }
        }

        if (!pointInRect(m, dx, dy, dw, dh)) {
            mAddProductModalOpen = false;
        }
        return;
    }

    // 2. Enlarge Cart Modal (Fixed quantity buttons hit-testing)
    if (mEnlargeCartModalOpen) {
        float dw = 840.f, dh = 580.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 44, dy + 20, 28, 28)) {
            mEnlargeCartModalOpen = false;
            return;
        }

        float rowStartY = dy + 124.f;
        for (int i = 0; i < (int)cart.size(); i++) {
            float rowY = rowStartY + i * 56.f - mEnlargeScrollOffset;
            if (rowY < dy + 120.f || rowY > dy + 490.f) continue;

            if (pointInRect(m, dx + 510, rowY + 14, 24, 24)) {
                mCartService.setItemQuantity(i, cart[i].quantity - 1);
                return;
            }
            if (pointInRect(m, dx + 580, rowY + 14, 24, 24)) {
                mCartService.setItemQuantity(i, cart[i].quantity + 1);
                return;
            }
            if (pointInRect(m, dx + dw - 95, rowY + 14, 60, 24)) {
                std::string n = products[cart[i].productId].name;
                mCartService.deleteItem(i);
                showToast("Deleted " + n + " from receipt");
                return;
            }
        }

        if (pointInRect(m, dx + 30, dy + dh - 56, 220, 40)) {
            mEnlargeCartModalOpen = false;
            return;
        }

        if (pointInRect(m, dx + dw - 250, dy + dh - 56, 220, 40)) {
            mEnlargeCartModalOpen = false;
            if (mCartService.getPaymentMethod() == 0) mCashModalOpen = true;
            else mCardModalOpen = true;
            return;
        }

        if (!pointInRect(m, dx, dy, dw, dh)) {
            mEnlargeCartModalOpen = false;
        }
        return;
    }

    // 3. Cash Payment Modal
    if (mCashModalOpen) {
        float dw = 480.f, dh = 400.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        mCashInputFocused = pointInRect(m, dx + 150, dy + 144, 180, 34);

        if (pointInRect(m, dx + 340, dy + 144, 56, 34)) {
            mCartService.setCashGiven(mCartService.getTotal(products));
        }

        if (pointInRect(m, dx + 404, dy + 144, 48, 34)) {
            mCartService.setCashGiven(0);
        }

        const int chipVals[] = {100, 500, 1000, 5000};
        for (int i = 0; i < 4; i++) {
            float cx = dx + 30 + i * 105;
            float cy = dy + 195;
            if (pointInRect(m, cx, cy, 95, 32)) {
                mCartService.setCashGiven(mCartService.parseCashGiven() + chipVals[i]);
                return;
            }
        }

        if (pointInRect(m, dx + 30, dy + dh - 56, 250, 42)) {
            std::string msg;
            if (mCartService.chargeSale(products, mCurrentUserName, msg)) {
                recordSaleToOutputFiles(mCartService.getCart(), mCurrentUserName, 0);
                mProductCatalog.saveToFile();
                mActivityLogger.logActivity("SALE", mCurrentUserName, "Processed cash sale of " + formatRs(mCartService.getTotal(products)));
                showToast(msg);
                mCashModalOpen = false;
            } else {
                showToast(msg);
            }
            return;
        }

        if (pointInRect(m, dx + dw - 150, dy + dh - 56, 120, 42)) {
            mCashModalOpen = false;
            return;
        }

        if (!pointInRect(m, dx, dy, dw, dh)) {
            mCashModalOpen = false;
        }
        return;
    }

    // 4. Card Payment Modal
    if (mCardModalOpen) {
        float dw = 460.f, dh = 360.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + 30, dy + dh - 56, 250, 42)) {
            std::string msg;
            if (mCartService.chargeSale(products, mCurrentUserName, msg)) {
                recordSaleToOutputFiles(mCartService.getCart(), mCurrentUserName, 1);
                mProductCatalog.saveToFile();
                mActivityLogger.logActivity("SALE", mCurrentUserName, "Processed card sale of " + formatRs(mCartService.getTotal(products)));
                showToast(msg);
                mCardModalOpen = false;
            } else {
                showToast(msg);
            }
            return;
        }

        if (pointInRect(m, dx + dw - 150, dy + dh - 56, 120, 42)) {
            mCardModalOpen = false;
            return;
        }

        if (!pointInRect(m, dx, dy, dw, dh)) {
            mCardModalOpen = false;
        }
        return;
    }

    // 5. Quantity Stepper Modal
    if (mQtyModalOpen) {
        float dw = 320.f, dh = 240.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        auto clampStock = [&](int targetQ) -> int {
            if (mEditingCartIndex >= 0 && mEditingCartIndex < (int)cart.size()) {
                int pid = cart[mEditingCartIndex].productId;
                if (pid >= 0 && pid < (int)products.size()) {
                    int st = products[pid].stock;
                    if (targetQ > st) {
                        showToast("Stock limit: only " + std::to_string(st) + " available");
                        return st;
                    }
                }
            }
            return targetQ;
        };

        const int presets[] = {1, 2, 3, 5, 10};
        for (int i = 0; i < 5; i++) {
            float bx = dx + 20 + i * 56;
            float by = dy + 116;
            if (pointInRect(m, bx, by, 48, 28)) {
                int q = clampStock(presets[i]);
                mCartService.setItemQuantity(mEditingCartIndex, q);
                showToast("Quantity updated to " + std::to_string(q));
                mQtyModalOpen = false;
                return;
            }
        }

        if (pointInRect(m, dx + 20, dy + dh - 48, 130, 34)) {
            int q = 1;
            try { q = std::stoi(mQtyInputStr); } catch (...) {}
            q = clampStock(q);
            mCartService.setItemQuantity(mEditingCartIndex, q);
            showToast("Quantity updated to " + std::to_string(q));
            mQtyModalOpen = false;
            return;
        }

        if (pointInRect(m, dx + 170, dy + dh - 48, 130, 34)) {
            mQtyModalOpen = false;
            return;
        }

        if (!pointInRect(m, dx, dy, dw, dh)) {
            mQtyModalOpen = false;
        }
        return;
    }

    // 6. Add Staff Modal
    if (mAddStaffModalOpen) {
        float dw = 480.f, dh = 390.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + 30, dy + 104, dw - 60, 36)) {
            mNewStaffActiveField = 1;
            return;
        }
        if (pointInRect(m, dx + 30, dy + 171, dw - 60, 36)) {
            mNewStaffActiveField = 2;
            return;
        }
        if (pointInRect(m, dx + 30, dy + 236, 120, 32)) {
            mNewStaffRole = "Employee";
            return;
        }
        if (pointInRect(m, dx + 160, dy + 236, 120, 32)) {
            mNewStaffRole = "Admin";
            return;
        }
        if (pointInRect(m, dx + 30, dy + dh - 56, 220, 42)) {
            if (mNewStaffName.empty() || mNewStaffPass.empty()) {
                showToast("Error: Employee ID and Password cannot be empty");
            } else {
                if (mStaffManager.addStaffMember(mNewStaffName, mNewStaffPass, mNewStaffRole)) {
                    mActivityLogger.logActivity("STAFF", mCurrentUserName, "Added " + mNewStaffRole + " ID: " + mNewStaffName);
                    showToast("Staff account created for " + mNewStaffName);
                    mAddStaffModalOpen = false;
                } else {
                    showToast("Error: Staff ID '" + mNewStaffName + "' already exists");
                }
            }
            return;
        }
        if (pointInRect(m, dx + dw - 150, dy + dh - 56, 120, 42)) {
            mAddStaffModalOpen = false;
            return;
        }
        if (!pointInRect(m, dx, dy, dw, dh)) {
            mAddStaffModalOpen = false;
        }
        return;
    }

    // 7. Reset Password Modal
    if (mResetPassModalOpen) {
        float dw = 460.f, dh = 300.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + 30, dy + 115, dw - 60, 36)) {
            mResetPassFocused = true;
            return;
        }
        if (pointInRect(m, dx + 30, dy + dh - 56, 200, 42)) {
            if (mResetStaffNewPass.empty()) {
                showToast("Error: Password cannot be empty");
            } else {
                mStaffManager.resetStaffPassword(mResetStaffTarget, mResetStaffNewPass);
                mActivityLogger.logActivity("STAFF", mCurrentUserName, "Reset password for: " + mResetStaffTarget);
                showToast("Password updated for " + mResetStaffTarget);
                mResetPassModalOpen = false;
            }
            return;
        }
        if (pointInRect(m, dx + dw - 150, dy + dh - 56, 120, 42)) {
            mResetPassModalOpen = false;
            return;
        }
        if (!pointInRect(m, dx, dy, dw, dh)) {
            mResetPassModalOpen = false;
        }
        return;
    }

    // 8. Add Inventory & Supplier Modal
    if (mAddInventoryModalOpen) {
        float dw = 620.f, dh = 590.f;
        float dx = (mCurrentWidth - dw) / 2.f;
        float dy = (mDesignHeight - dh) / 2.f;

        // Vendor dropdown selection if open
        if (mVendorDropdownOpen) {
            std::vector<std::string> vendors = mInventoryManager.getUniqueSuppliers();
            float dropX = dx + 30;
            float dropY = dy + 68 + 50;
            float dropW = 320.f;
            float rowH = 26.f;
            for (size_t v = 0; v < vendors.size() && v < 6; v++) {
                float vy = dropY + 4.f + v * rowH;
                if (pointInRect(m, dropX, vy, dropW, rowH)) {
                    mNewItemSupplier = vendors[v];
                    mVendorDropdownOpen = false;
                    return;
                }
            }
        }

        // Dropdown chevron button
        if (pointInRect(m, dx + 30 + 320 - 28, dy + 68 + 16, 26, 32)) {
            mVendorDropdownOpen = !mVendorDropdownOpen;
            return;
        }

        // Supplier Name (Field 0)
        if (pointInRect(m, dx + 30, dy + 68 + 16, 290, 32)) {
            mNewItemActiveField = 0;
            mVendorDropdownOpen = false;
            return;
        }

        // Arrival Date (Field 5)
        if (pointInRect(m, dx + 370, dy + 68 + 16, 220, 32)) {
            mNewItemActiveField = 5;
            mVendorDropdownOpen = false;
            return;
        }

        // Description / Name (Field 1)
        if (pointInRect(m, dx + 30, dy + 125 + 16, 370, 32)) {
            mNewItemActiveField = 1;
            mVendorDropdownOpen = false;
            return;
        }

        // Unit (Field 7)
        if (pointInRect(m, dx + 420, dy + 125 + 16, 170, 32)) {
            mNewItemActiveField = 7;
            mVendorDropdownOpen = false;
            return;
        }

        // Expanded 8 Supermarket Category Pills
        const char* cats[] = {"Grocery", "Dairy", "Drinks", "Bakery", "Snacks", "Fruits & Veg", "Household", "Personal Care"};
        for (int c = 0; c < 8; c++) {
            float cx = dx + 30 + (c % 4) * 142;
            float cy = dy + 198 + (c / 4) * 26;
            if (pointInRect(m, cx, cy, 134, 22)) {
                mNewItemCategory = cats[c];
                mVendorDropdownOpen = false;
                return;
            }
        }

        // Retail Price (Field 2)
        if (pointInRect(m, dx + 30, dy + 256 + 16, 170, 32)) { mNewItemActiveField = 2; return; }
        // Cost Price (Field 3)
        if (pointInRect(m, dx + 225, dy + 256 + 16, 170, 32)) { mNewItemActiveField = 3; return; }
        // Quantity (Field 4)
        if (pointInRect(m, dx + 420, dy + 256 + 16, 170, 32)) { mNewItemActiveField = 4; return; }

        // Barcode Input Box (Field 6)
        if (pointInRect(m, dx + 30, dy + 332, 240, 34)) {
            mNewItemActiveField = 6;
            mScanningUniversalSupplier = false;
            return;
        }

        // Auto Barcode Button
        if (pointInRect(m, dx + 280, dy + 332, 140, 34)) {
            mScanningUniversalSupplier = false;
            generateNewItemBarcode();
            return;
        }

        // Universal Barcode Scanner Button
        if (pointInRect(m, dx + 430, dy + 332, 160, 34)) {
            mScanningUniversalSupplier = !mScanningUniversalSupplier;
            mNewItemActiveField = 6;
            if (mScanningUniversalSupplier) {
                showToast("Webcam / Gun scanner active: Hold barcode in front of camera!");
            }
            return;
        }

        // Save Button
        if (pointInRect(m, dx + 30, dy + dh - 52, 250, 38)) {
            saveNewInventoryItem();
            return;
        }

        // Cancel Button
        if (pointInRect(m, dx + dw - 150, dy + dh - 52, 120, 38)) {
            mAddInventoryModalOpen = false;
            return;
        }

        if (!pointInRect(m, dx, dy, dw, dh)) {
            mAddInventoryModalOpen = false;
        }
        return;
    }

    // Logout / Switch User profile button
    if (pointInRect(m, 4, 640, 64, 70)) {
        mActivityLogger.logActivity("LOGIN", mCurrentUserName, "Operator logged out of terminal session");
        mShowLogin = true;
        mLoginScreen.reset();
        showToast("Logged out to sign-in terminal");
        return;
    }

    // Navigation Sidebar
    bool isAdmin = (mCurrentRole == "Admin");
    std::vector<int> visibleTabs = isAdmin ? std::vector<int>{0, 1, 2, 3, 4, 5, 6} : std::vector<int>{0, 1, 3, 6};

    for (size_t slot = 0; slot < visibleTabs.size(); slot++) {
        float cy = Layout::NAV_Y[slot];
        if (pointInRect(m, 4, cy - 24, 60, 48)) {
            mActiveTab = visibleTabs[slot];
            return;
        }
    }

    if (mActiveTab != 0) {
        if (mActiveTab == 1) { // Products Tab
            // Search bar
            float searchW = 160.f;
            float searchX = mMainX + Layout::MAIN_W - 140.f - searchW - 10.f;
            if (pointInRect(m, searchX, 48.f, searchW, 30.f)) {
                mProductsSearchFocused = true;
                return;
            } else {
                mProductsSearchFocused = false;
            }

            // + Add Product (from Inventory)
            if (pointInRect(m, mMainX + Layout::MAIN_W - 140.f, 48.f, 140.f, 30.f)) {
                if (mCurrentRole != "Admin") {
                    showToast("Restricted: Admin authorization required to stock products");
                } else {
                    mAddProductModalOpen = true;
                    mAddProductSearchQuery = "";
                    mSelectedInventoryForProduct = -1;
                    mTransferQtyStr = "10";
                }
                return;
            }

            // Table rows: Desc and Del
            std::string qLower = mProductsSearchQuery;
            std::transform(qLower.begin(), qLower.end(), qLower.begin(), ::tolower);
            std::vector<int> visibleIndices;
            for (size_t i = 0; i < products.size(); i++) {
                if (qLower.empty()) {
                    visibleIndices.push_back((int)i);
                    continue;
                }
                std::string nLower = products[i].name;
                std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);
                std::string bLower = products[i].barcode;
                std::transform(bLower.begin(), bLower.end(), bLower.begin(), ::tolower);
                std::string cLower = products[i].category;
                std::transform(cLower.begin(), cLower.end(), cLower.begin(), ::tolower);
                if (nLower.find(qLower) != std::string::npos ||
                    bLower.find(qLower) != std::string::npos ||
                    cLower.find(qLower) != std::string::npos) {
                    visibleIndices.push_back((int)i);
                }
            }

            for (size_t vi = 0; vi < visibleIndices.size(); vi++) {
                int idx = visibleIndices[vi];
                float rowY = 92.f + 38.f + static_cast<float>(vi) * 44.f - mProductsScrollOffset;
                if (rowY < 92.f + 26.f || rowY > 92.f + 540.f) continue;

                // [ Desc ] button (ISE-57 Requirement 5 Product Specs)
                if (pointInRect(m, mMainX + 420.f, rowY - 6.f, 50.f, 22.f)) {
                    mSpecProductIdx = idx;
                    mSpecModalOpen = true;
                    return;
                }

                // [ Del ] button
                if (pointInRect(m, mMainX + 485.f, rowY - 6.f, 38.f, 22.f)) {
                    if (mCurrentRole != "Admin") {
                        showToast("Restricted: Admin authorization required to delete products");
                    } else {
                        mActivityLogger.logActivity("PRODUCT", mCurrentUserName, "Deleted product: " + products[idx].name);
                        showToast("Deleted " + products[idx].name);
                        mProductCatalog.deleteProduct(idx);
                    }
                    return;
                }
            }
            return;
        } else if (mActiveTab == 2) { // Inventory Tab
            float tabY = 140.f;
            // SubTab 0: Stock & Items (width 115)
            if (pointInRect(m, mMainX, tabY, 115.f, 26.f)) {
                mInventorySubTab = 0;
                return;
            }
            // SubTab 1: Supplier Purchase Ledger (width 120)
            if (pointInRect(m, mMainX + 122.f, tabY, 120.f, 26.f)) {
                mInventorySubTab = 1;
                return;
            }
            // Search Bar in Inventory Tab (width 140)
            if (pointInRect(m, mMainX + 250.f, tabY, 140.f, 26.f)) {
                mInventorySearchFocused = true;
                return;
            } else {
                mInventorySearchFocused = false;
            }
            // Add Supplier / Item Button (width 145)
            if (pointInRect(m, mMainX + Layout::MAIN_W - 145.f, tabY, 145.f, 26.f)) {
                if (mCurrentRole != "Admin") {
                    showToast("Restricted: Admin authorization required to add inventory");
                } else {
                    mAddInventoryModalOpen = true;
                    mNewItemDesc = "";
                    mNewItemBarcode = "";
                    mNewItemGenerated = false;
                    mNewItemActiveField = 1;
                    mVendorDropdownOpen = false;
                    mScanningUniversalSupplier = false;
                }
                return;
            }

            float ty = 174.f;
            float listStartY = (mCurrentRole != "Admin") ? (ty + 38.f) : (ty + 12.f);

            if (mInventorySubTab == 0) {
                std::vector<int> filteredIndices = mInventoryManager.getFilteredItemIndices(mInventorySearchQuery);
                const auto& invItems = mInventoryManager.getItems();

                for (size_t i = 0; i < filteredIndices.size(); i++) {
                    int idx = filteredIndices[i];
                    float rowY = listStartY + 12.f + static_cast<float>(i) * 54.f - mInventoryScrollOffset;
                    if (rowY < ty + 14.f || rowY > ty + 465.f) continue;

                    const auto& item = invItems[idx];

                    // [ Description ] button
                    if (pointInRect(m, mMainX + 250.f, rowY + 10.f, 68.f, 22.f)) {
                        mDescItemName = item.name;
                        mDescItemBarcode = item.barcode;
                        mDescItemCategory = item.category;
                        mDescItemUnit = item.unit;
                        mDescItemPrice = item.retailPrice;
                        mDescItemCost = item.costPrice;
                        mDescItemStock = item.stock;
                        mDescItemSupplier = item.supplier;
                        mDescItemDate = item.arrivalDate;
                        mItemDescModalOpen = true;
                        return;
                    }

                    if (mCurrentRole == "Admin") {
                        // +10 button
                        if (pointInRect(m, mMainX + 405.f, rowY + 10.f, 36.f, 22.f)) {
                            mInventoryManager.restockItem(item.barcode, 10);
                            int pIdx = mProductCatalog.findProductByBarcode(item.barcode);
                            if (pIdx >= 0) mProductCatalog.addStock(pIdx, 10);
                            mActivityLogger.logActivity("INVENTORY", mCurrentUserName, "Restocked +10 units of " + item.name);
                            showToast("+10 added to " + item.name);
                            return;
                        }
                        // +50 button
                        if (pointInRect(m, mMainX + 447.f, rowY + 10.f, 36.f, 22.f)) {
                            mInventoryManager.restockItem(item.barcode, 50);
                            int pIdx = mProductCatalog.findProductByBarcode(item.barcode);
                            if (pIdx >= 0) mProductCatalog.addStock(pIdx, 50);
                            mActivityLogger.logActivity("INVENTORY", mCurrentUserName, "Restocked +50 units of " + item.name);
                            showToast("+50 added to " + item.name);
                            return;
                        }
                        // Order button
                        if (pointInRect(m, mMainX + 489.f, rowY + 10.f, 42.f, 22.f)) {
                            mAddInventoryModalOpen = true;
                            mNewItemSupplier = item.supplier.empty() ? "Nestle Pakistan" : item.supplier;
                            mNewItemDesc = item.name;
                            mNewItemCategory = item.category;
                            mNewItemUnit = item.unit;
                            mNewItemPriceStr = std::to_string(item.retailPrice);
                            mNewItemCostStr = std::to_string(item.costPrice);
                            mNewItemBarcode = item.barcode;
                            mNewItemActiveField = 4;
                            mVendorDropdownOpen = false;
                            mScanningUniversalSupplier = false;
                            return;
                        }
                    }
                }
            } else {
                // Sub-tab 1: Supplier Ledger
                std::vector<int> filteredPurchases = mInventoryManager.getFilteredPurchaseIndices(mInventorySearchQuery);
                const auto& purchases = mInventoryManager.getPurchases();
                for (size_t i = 0; i < filteredPurchases.size(); i++) {
                    int idx = filteredPurchases[i];
                    float rowY = listStartY + 24.f + static_cast<float>(i) * 38.f - mSupplierScrollOffset;
                    if (rowY < ty + 30.f || rowY > ty + 465.f) continue;

                    // [ Desc ] button
                    if (pointInRect(m, mMainX + 288.f, rowY + 3.f, 50.f, 20.f)) {
                        const auto& p = purchases[idx];
                        mDescItemName = p.description;
                        mDescItemBarcode = p.barcode;
                        mDescItemCategory = "Procured Supply";
                        mDescItemUnit = std::to_string(p.quantity) + " units";
                        mDescItemPrice = p.totalCost;
                        mDescItemCost = p.unitCost;
                        mDescItemStock = p.quantity;
                        mDescItemSupplier = p.supplier;
                        mDescItemDate = p.date;
                        mItemDescModalOpen = true;
                        return;
                    }
                }
            }
            return;
        } else if (mActiveTab == 3) { // Sales Tab
            float filterY = 148.f;
            float fX = mMainX;
            for (int fi = 0; fi < 4; fi++) {
                float fw = (fi == 0) ? 120.f : 90.f;
                if (pointInRect(m, fX, filterY, fw, 24.f)) {
                    mSalesTimeFilter = fi;
                    return;
                }
                fX += fw + 8.f;
            }
            return;
        } else if (mActiveTab == 4) { // Activity Logs Tab
            const char* filters[] = {"ALL", "LOGIN", "SALE", "RETURN", "STAFF", "INVENTORY"};
            for (int i = 0; i < 6; i++) {
                if (pointInRect(m, mMainX + i * 86, 102, 78, 26)) {
                    mLogFilter = filters[i];
                    return;
                }
            }
        } else if (mActiveTab == 5) { // Settings Tab
            float ty = 105.f;
            // Quick Camera Device Switcher row in settings
            if (pointInRect(m, mMainX + 25, ty + 10, Layout::MAIN_W - 50, 80)) {
                switchCamera();
                return;
            }
            float staffY = ty + 175.f;
            if (pointInRect(m, mMainX + Layout::MAIN_W - 165, staffY + 12, 160, 28)) {
                if (mCurrentRole != "Admin") {
                    showToast("Restricted: Employee ID setup requires Administrator login");
                } else {
                    mAddStaffModalOpen = true;
                    mNewStaffName = "";
                    mNewStaffPass = "";
                    mNewStaffRole = "Employee";
                    mNewStaffActiveField = 1;
                }
                return;
            }
            if (mCurrentRole == "Admin") {
                float thY = staffY + 52.f;
                const auto& staffList = mStaffManager.getStaffList();
                for (size_t i = 0; i < staffList.size() && i < 4; i++) {
                    float sRowY = thY + 20.f + static_cast<float>(i) * 32.f;
                    if (pointInRect(m, mMainX + 355, sRowY, 74, 22)) {
                        mResetPassModalOpen = true;
                        mResetStaffTarget = staffList[i].name;
                        mResetStaffNewPass = "";
                        mResetPassFocused = true;
                        return;
                    }
                    if (staffList[i].name != mCurrentUserName && pointInRect(m, mMainX + 436, sRowY, 62, 22)) {
                        std::string remName = staffList[i].name;
                        mStaffManager.removeStaffMember(remName);
                        mActivityLogger.logActivity("STAFF", mCurrentUserName, "Removed staff account: " + remName);
                        showToast("Staff account removed: " + remName);
                        return;
                    }
                }
            }
            float utilY = ty + 372.f;
            if (pointInRect(m, mMainX + 25, utilY, 235, 34)) {
                mProductCatalog.loadProductsFile();
                showToast("product.txt reloaded from disk");
                return;
            }
            if (pointInRect(m, mMainX + 275, utilY, 235, 34)) {
                showToast("Barcode bitmaps synchronized");
                return;
            }
        } else if (mActiveTab == 6) { // Return Tab
            float ty = 100.f;
            float colW = (Layout::MAIN_W - 20.f) / 2.f;
            float inY = ty + 60.f + 150.f + 20.f;
            float paneW = colW - 40.f;

            // Barcode input box click
            if (pointInRect(m, mMainX + 20, inY + 18, paneW - 90, 36)) {
                mReturnBarcodeFocused = true;
                return;
            }
            // Look Up Button
            if (pointInRect(m, mMainX + 20 + paneW - 80, inY + 18, 80, 36)) {
                lookupReturnItem(mReturnBarcode);
                return;
            }
            // Demo test pills
            const char* demoCodes[] = {"100001", "100002", "100004", "100005"};
            float pillX = mMainX + 20;
            for (int p = 0; p < 4; p++) {
                if (pointInRect(m, pillX, inY + 95, 52, 24)) {
                    lookupReturnItem(demoCodes[p]);
                    return;
                }
                pillX += 58;
            }

            // Right Column actions
            float rX = mMainX + colW + 20.f;
            if (mSelectedReturnProductIdx >= 0) {
                float pY = ty + 60.f;
                float sY = pY + 90.f;
                float qY = sY + (mReturnOrderId.empty() ? 105.f : 130.f);

                // [-] Qty
                if (pointInRect(m, rX + 150, qY, 32, 32)) {
                    if (mReturnQty > 1) mReturnQty--;
                    return;
                }
                // [+] Qty
                if (pointInRect(m, rX + 218, qY, 32, 32)) {
                    mReturnQty++;
                    return;
                }
                // Confirm Return Button
                float rfY = qY + 45.f;
                float bY = rfY + 70.f;
                if (pointInRect(m, rX + 20, bY, paneW, 46)) {
                    processReturn();
                    return;
                }
            }
            return;
        }
        return;
    }

    // Quick Check Header Pill Button (Tab 0) - ISE-56
    if (mActiveTab == 0 && pointInRect(m, mMainX + 230.f, 56.f, 112.f, 25.f)) {
        mQuickCheckModalOpen = true;
        mQuickCheckBarcode.clear();
        mQuickCheckProductIdx = -1;
        mQuickCheckInputFocused = true;
        return;
    }

    // POS Tab Controls
    sf::FloatRect searchBox{mMainX, 82.f, 415.f, 39.f};
    sf::FloatRect addBtn{mMainX + 425.f, 82.f, 115.f, 39.f};
    sf::FloatRect viewportBox{mMainX, 133.f, Layout::MAIN_W, 160.f};

    // ISE-95: Auto-focus and maintain continuous gun scanner/keyboard focus on POS screen
    mSearchFocused = true;

    if (pointInRect(m, addBtn.left, addBtn.top, addBtn.width, addBtn.height)) {
        if (mCurrentRole != "Admin") {
            showToast("Restricted: Admin login required to add products");
            return;
        }
        mSearchFocused = true;
        showToast("Type product name or barcode, then Enter");
    }

    float camBadgeW = 215.f;
    float camBadgeX = viewportBox.left + viewportBox.width - camBadgeW - 14.f;
    float camBadgeY = viewportBox.top + 11.f;
    if (pointInRect(m, camBadgeX, camBadgeY, camBadgeW, 26.f)) {
        switchCamera();
        return;
    }

    if (pointInRect(m, viewportBox.left, viewportBox.top, viewportBox.width, viewportBox.height)) {
        simulateScan();
    }

    // Category selection pills (All 9 supermarket categories)
    float pillRight = mMainX + Layout::MAIN_W;
    for (int i = Layout::CATEGORY_COUNT - 1; i >= 0; i--) {
        float w = getTextWidth(mFontSemiBold, Layout::CATEGORIES[i], 8.5f) + 14.f;
        pillRight -= w;
        if (pointInRect(m, pillRight, 318.f, w, 22.f)) {
            mActiveCategory = Layout::CATEGORIES[i];
            break;
        }
        pillRight -= 4.f;
    }

    // Product cards click -> add to cart
    auto visible = mProductCatalog.getVisibleProducts(mSearchQuery, mActiveCategory);
    for (int k = 0; k < (int)visible.size() && k < 6; k++) {
        float cx = mMainX + (k % 3) * (Layout::CARD_W + Layout::CARD_GAP);
        float cy = Layout::CARD_Y + (k / 3) * (Layout::CARD_H + Layout::CARD_GAP);
        if (pointInRect(m, cx, cy, Layout::CARD_W, Layout::CARD_H)) {
            int pid = visible[k];
            mCartService.addItem(pid, 1);
            showToast("Added " + products[pid].name + " to receipt");
        }
    }

    // Drafts / Parked Carts Button (badge)
    if (pointInRect(m, mRpX + Layout::RP_W - 76.f, 12.f, 76.f, 22.f)) {
        mDraftsModalOpen = true;
        return;
    }

    // Hold Cart Button (ISE-96: 1-click order hold)
    if (pointInRect(m, mRpX, 38.f, 74.f, 22.f)) {
        holdCurrentOrder();
        return;
    }

    // Resume Cart Button (ISE-97: 1-click fast resume of latest held order)
    if (pointInRect(m, mRpX + 78.f, 38.f, 86.f, 22.f)) {
        resumeLatestOrder();
        return;
    }

    // Clear Cart Button (one-click wipe entire cart)
    if (pointInRect(m, mRpX + 168.f, 38.f, 60.f, 22.f)) {
        if (cart.empty()) {
            showToast("Cart is already empty");
        } else {
            mCartService.clear();
            mReceiptScrollOffset = 0.f;
            showToast("Whole cart cleared in one click");
        }
        return;
    }

    // Enlarge cart button
    if (pointInRect(m, mRpX + Layout::RP_W - 74.f, Layout::RP_Y + 7.f, 66.f, 22.f)) {
        mEnlargeCartModalOpen = true;
        mEnlargeScrollOffset = 0.f;
        return;
    }

    // Cart row item delete & stepper controls
    for (int i = 0; i < (int)cart.size(); i++) {
        float y0 = Layout::ROW_Y + i * Layout::ROW_H - mReceiptScrollOffset;
        if (y0 < Layout::ROW_Y - 5.f || y0 > Layout::RP_Y + Layout::RP_H - 20.f) continue;

        // 1. Delete button (Trashcan icon on far right)
        float delX = mRpX + 270.f;
        float delY = y0 + 14.f;
        if (pointInRect(m, delX, delY, 22.f, 22.f)) {
            std::string n = products[cart[i].productId].name;
            mCartService.deleteItem(i);
            showToast("Deleted " + n + " from receipt");
            return;
        }

        // 2. Info / Spec button or product name click
        if (pointInRect(m, mRpX + 38.f, y0 + 6.f, 96.f, 34.f)) {
            mSpecProductIdx = cart[i].productId;
            mSpecModalOpen = true;
            return;
        }

        // 3. Stepper: Minus, Qty modal, Plus
        if (pointInRect(m, mRpX + 144.f, y0 + 14.f, 16.f, 20.f)) {
            mCartService.setItemQuantity(i, cart[i].quantity - 1);
            return;
        }

        if (pointInRect(m, mRpX + 163.f, y0 + 14.f, 18.f, 20.f)) {
            mEditingCartIndex = i;
            mQtyModalOpen = true;
            mQtyInputStr = std::to_string(cart[i].quantity);
            return;
        }

        if (pointInRect(m, mRpX + 184.f, y0 + 14.f, 16.f, 20.f)) {
            int pid = cart[i].productId;
            if (pid >= 0 && pid < (int)products.size()) {
                if (cart[i].quantity + 1 > products[pid].stock) {
                    showToast("Stock limit: only " + std::to_string(products[pid].stock) + " available for " + products[pid].name);
                    return;
                }
            }
            mCartService.setItemQuantity(i, cart[i].quantity + 1);
            return;
        }
    }

    // Discount chip toggle
    float chipY = Layout::RP_Y + Layout::RP_H + 8.f;
    if (pointInRect(m, mRpX, chipY, 210.f, 22.f)) {
        mCartService.setDiscountApplied(!mCartService.isDiscountApplied());
        showToast(mCartService.isDiscountApplied() ? "Discount (- Rs 120) applied" : "Discount removed");
    }

    // Payment method buttons
    float btnPayY = chipY + 98.f;
    float payBtnW = (Layout::RP_W - 12.f) / 2.f;
    if (pointInRect(m, mRpX, btnPayY, payBtnW, 30.f)) {
        mCartService.setPaymentMethod(0);
        mCashModalOpen = true;
        mCashInputFocused = true;
    }
    if (pointInRect(m, mRpX + payBtnW + 12.f, btnPayY, payBtnW, 30.f)) {
        mCartService.setPaymentMethod(1);
        mCardModalOpen = true;
    }

    // Charge / Complete Payment
    float chargeY = btnPayY + 40.f;
    if (pointInRect(m, mRpX, chargeY, Layout::RP_W, 38.f)) {
        if (!mCartService.isPaymentDone() && !cart.empty()) {
            if (mCartService.getPaymentMethod() == 0) mCashModalOpen = true;
            else mCardModalOpen = true;
        }
    }

    // Print Receipt
    float printY = chargeY + 44.f;
    if (pointInRect(m, mRpX, printY, Layout::RP_W, 38.f)) {
        bool ok = false;
        mCartService.printReceipt(products, mCurrentUserName, ok);
        if (ok) {
            mReceiptModalOpen = true;
            showToast("Receipt printed and saved to receipt.txt!");
        } else {
            showToast("Payment must be completed before printing receipt!");
        }
    }

    // Start New Sale
    float newSaleY = printY + 44.f;
    if (mCartService.isPaymentDone() && pointInRect(m, mRpX, newSaleY, Layout::RP_W, 32.f)) {
        mCartService.clear();
        mReceiptScrollOffset = 0.f;
        showToast("Ready for new sale transaction");
    }
}

void PosApp::switchCamera() {
    int count = CameraStream::getAvailableCameraCount();
    if (count <= 1) {
        showToast("Single camera active: " + mCameraStream.getCameraName() + ". Connect phone via USB cable to switch!");
        return;
    }

    bool ok = mCameraStream.switchCamera();
    if (ok) {
        std::string camName = mCameraStream.getCameraName();
        bool isExt = mCameraStream.isActiveCameraExternal();
        if (isExt) {
            showToast("Switched to External/Mobile Scanner: " + camName);
        } else {
            showToast("Switched to Laptop Webcam: " + camName);
        }
    } else {
        showToast("Could not switch camera device.");
    }
}

void PosApp::handleKeyPress(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::F2) {
        simulateScan();
    } else if (key == sf::Keyboard::F3) {
        mQuickCheckModalOpen = !mQuickCheckModalOpen;
        if (mQuickCheckModalOpen) {
            mQuickCheckBarcode.clear();
            mQuickCheckProductIdx = -1;
            mQuickCheckInputFocused = true;
        }
    } else if (key == sf::Keyboard::F4) {
        // Fast Order Hold (ISE-96)
        holdCurrentOrder();
    } else if (key == sf::Keyboard::F5) {
        // Fast Order Resume (ISE-97)
        resumeLatestOrder();
    } else if (key == sf::Keyboard::H) {
        // Dedicated Hotkey H for Hold (ISE-96)
        if (mActiveTab == 0 && !mCashInputFocused && !mAddStaffModalOpen && !mResetPassModalOpen &&
            !mAddInventoryModalOpen && !mQuickCheckModalOpen && !mQtyModalOpen && mSearchQuery.empty()) {
            holdCurrentOrder();
            mSkipNextTextChar = true;
        }
    } else if (key == sf::Keyboard::R) {
        // Dedicated Hotkey R for Resume (ISE-97)
        if (mActiveTab == 0 && !mCashInputFocused && !mAddStaffModalOpen && !mResetPassModalOpen &&
            !mAddInventoryModalOpen && !mQuickCheckModalOpen && !mQtyModalOpen && mSearchQuery.empty()) {
            resumeLatestOrder();
            mSkipNextTextChar = true;
        }
    } else if (key == sf::Keyboard::C) {
        // Toggle camera when not editing text fields
        if (mActiveTab == 0 && !mCashInputFocused && !mAddStaffModalOpen && !mResetPassModalOpen &&
            !mAddInventoryModalOpen && !mQuickCheckModalOpen && !mQtyModalOpen && mSearchQuery.empty()) {
            switchCamera();
            mSkipNextTextChar = true;
        }
    } else if (key == sf::Keyboard::Escape) {
        mSearchQuery.clear();
        mSearchFocused = (mActiveTab == 0);
        mCashInputFocused = false;
        mProductsSearchFocused = false;
        mInventorySearchFocused = false;
        mAddProductSearchFocused = false;
        mReceiptModalOpen = false;
        mQtyModalOpen = false;
        mEnlargeCartModalOpen = false;
        mCashModalOpen = false;
        mCardModalOpen = false;
        mAddStaffModalOpen = false;
        mResetPassModalOpen = false;
        mItemDescModalOpen = false;
        mDraftsModalOpen = false;
        mAddProductModalOpen = false;
        mAddInventoryModalOpen = false;
        mVendorDropdownOpen = false;
        mQuickCheckModalOpen = false;
        mSpecModalOpen = false;
    }
}

void PosApp::handleTextInput(sf::Uint32 unicode) {
    if (mSkipNextTextChar) {
        mSkipNextTextChar = false;
        return;
    }
    if (mQuickCheckModalOpen) {
        if (unicode == 8) { // Backspace
            if (!mQuickCheckBarcode.empty()) {
                mQuickCheckBarcode.pop_back();
                if (mQuickCheckBarcode.size() == 6) {
                    mQuickCheckProductIdx = mProductCatalog.findProductByBarcode(mQuickCheckBarcode);
                } else {
                    mQuickCheckProductIdx = -1;
                }
            }
        } else if (unicode == 13) { // Enter
            if (!mQuickCheckBarcode.empty()) {
                mQuickCheckProductIdx = mProductCatalog.findProductByBarcode(mQuickCheckBarcode);
                if (mQuickCheckProductIdx >= 0) {
                    showToast("Found: " + mProductCatalog.getProducts()[mQuickCheckProductIdx].name);
                } else {
                    showToast("No product with barcode " + mQuickCheckBarcode);
                }
            }
        } else if (unicode >= '0' && unicode <= '9' && mQuickCheckBarcode.size() < 12) {
            mQuickCheckBarcode += (char)unicode;
            if (mQuickCheckBarcode.size() == 6) {
                mQuickCheckProductIdx = mProductCatalog.findProductByBarcode(mQuickCheckBarcode);
                if (mQuickCheckProductIdx >= 0) {
                    showToast("Found: " + mProductCatalog.getProducts()[mQuickCheckProductIdx].name);
                }
            }
        }
        return;
    }

    if (mAddStaffModalOpen) {
        if (unicode == 9) {
            mNewStaffActiveField = (mNewStaffActiveField == 1) ? 2 : 1;
        } else if (unicode == 8) {
            if (mNewStaffActiveField == 1 && !mNewStaffName.empty()) mNewStaffName.pop_back();
            else if (mNewStaffActiveField == 2 && !mNewStaffPass.empty()) mNewStaffPass.pop_back();
        } else if (unicode == 13) {
            if (mNewStaffName.empty() || mNewStaffPass.empty()) {
                showToast("Error: Employee ID and Password cannot be empty");
            } else {
                if (mStaffManager.addStaffMember(mNewStaffName, mNewStaffPass, mNewStaffRole)) {
                    mActivityLogger.logActivity("STAFF", mCurrentUserName, "Added " + mNewStaffRole + " ID: " + mNewStaffName);
                    showToast("Staff account created for " + mNewStaffName);
                    mAddStaffModalOpen = false;
                } else {
                    showToast("Error: Staff ID '" + mNewStaffName + "' already exists");
                }
            }
        } else if (unicode >= 32 && unicode < 127) {
            if (mNewStaffActiveField == 1 && mNewStaffName.size() < 24) mNewStaffName += (char)unicode;
            else if (mNewStaffActiveField == 2 && mNewStaffPass.size() < 24) mNewStaffPass += (char)unicode;
        }
        return;
    }

    if (mResetPassModalOpen) {
        if (unicode == 8) {
            if (!mResetStaffNewPass.empty()) mResetStaffNewPass.pop_back();
        } else if (unicode == 13) {
            if (mResetStaffNewPass.empty()) {
                showToast("Error: Password cannot be empty");
            } else {
                mStaffManager.resetStaffPassword(mResetStaffTarget, mResetStaffNewPass);
                mActivityLogger.logActivity("STAFF", mCurrentUserName, "Reset password for: " + mResetStaffTarget);
                showToast("Password updated for " + mResetStaffTarget);
                mResetPassModalOpen = false;
            }
        } else if (unicode >= 32 && unicode < 127 && mResetStaffNewPass.size() < 24) {
            mResetStaffNewPass += (char)unicode;
        }
        return;
    }

    if (mQtyModalOpen) {
        if (unicode == 8) {
            if (!mQtyInputStr.empty()) mQtyInputStr.pop_back();
        } else if (unicode >= '0' && unicode <= '9' && mQtyInputStr.size() < 4) {
            mQtyInputStr += (char)unicode;
        } else if (unicode == 13) {
            int q = 1;
            try { q = std::stoi(mQtyInputStr); } catch (...) {}
            auto& cart = mCartService.getCart();
            auto& products = mProductCatalog.getProducts();
            if (mEditingCartIndex >= 0 && mEditingCartIndex < (int)cart.size()) {
                int pid = cart[mEditingCartIndex].productId;
                if (pid >= 0 && pid < (int)products.size()) {
                    if (q > products[pid].stock) {
                        q = products[pid].stock;
                        showToast("Stock limit: capped to " + std::to_string(q) + " available");
                    }
                }
            }
            mCartService.setItemQuantity(mEditingCartIndex, q);
            mQtyModalOpen = false;
        }
        return;
    }

    if (mCashModalOpen && mCashInputFocused) {
        std::string currentStr = mCartService.getCashGivenStr();
        if (unicode == 8) {
            if (!currentStr.empty()) {
                currentStr.pop_back();
                mCartService.setCashGivenStr(currentStr);
            }
        } else if (unicode >= '0' && unicode <= '9' && currentStr.size() < 8) {
            currentStr += (char)unicode;
            mCartService.setCashGivenStr(currentStr);
        } else if (unicode == 13) {
            std::string msg;
            if (mCartService.chargeSale(mProductCatalog.getProducts(), mCurrentUserName, msg)) {
                recordSaleToOutputFiles(mCartService.getCart(), mCurrentUserName, 0);
                mProductCatalog.saveToFile();
                mActivityLogger.logActivity("SALE", mCurrentUserName, "Processed cash sale of " + formatRs(mCartService.getTotal(mProductCatalog.getProducts())));
                showToast(msg);
                mCashModalOpen = false;
            } else {
                showToast(msg);
            }
        }
        return;
    }

    if (mAddInventoryModalOpen) {
        auto getActiveStr = [&]() -> std::string* {
            switch (mNewItemActiveField) {
                case 0: return &mNewItemSupplier;
                case 1: return &mNewItemDesc;
                case 2: return &mNewItemPriceStr;
                case 3: return &mNewItemCostStr;
                case 4: return &mNewItemQtyStr;
                case 5: return &mNewItemDateStr;
                case 6: return &mNewItemBarcode;
                case 7: return &mNewItemUnit;
                default: return nullptr;
            }
        };
        std::string* targetStr = getActiveStr();
        if (unicode == 9) {
            mNewItemActiveField = (mNewItemActiveField + 1) % 8;
        } else if (unicode == 8) {
            if (targetStr && !targetStr->empty()) targetStr->pop_back();
        } else if (unicode == 13) {
            saveNewInventoryItem();
        } else if (unicode >= 32 && unicode < 127 && targetStr && targetStr->size() < 30) {
            *targetStr += (char)unicode;
        }
        return;
    }

    if (mActiveTab == 6 && mReturnBarcodeFocused) {
        if (unicode == 8) {
            if (!mReturnBarcode.empty()) mReturnBarcode.pop_back();
        } else if (unicode == 13) {
            lookupReturnItem(mReturnBarcode);
        } else if (unicode >= 32 && unicode < 127 && mReturnBarcode.size() < 20) {
            mReturnBarcode += (char)unicode;
        }
        return;
    }

    // Add Product From Inventory Search Box
    if (mAddProductModalOpen && mAddProductSearchFocused) {
        if (unicode == 8) {
            if (!mAddProductSearchQuery.empty()) mAddProductSearchQuery.pop_back();
        } else if (unicode == 13 || unicode == 27) {
            mAddProductSearchFocused = false;
        } else if (unicode >= 32 && unicode < 127 && mAddProductSearchQuery.size() < 30) {
            mAddProductSearchQuery += (char)unicode;
        }
        return;
    }

    // Products Tab Search Box
    if (mActiveTab == 1 && mProductsSearchFocused) {
        if (unicode == 8) {
            if (!mProductsSearchQuery.empty()) mProductsSearchQuery.pop_back();
        } else if (unicode == 13 || unicode == 27) {
            mProductsSearchFocused = false;
        } else if (unicode >= 32 && unicode < 127 && mProductsSearchQuery.size() < 30) {
            mProductsSearchQuery += (char)unicode;
        }
        return;
    }

    // Inventory Tab Search Box
    if (mActiveTab == 2 && mInventorySearchFocused) {
        if (unicode == 8) {
            if (!mInventorySearchQuery.empty()) mInventorySearchQuery.pop_back();
        } else if (unicode == 13 || unicode == 27) {
            mInventorySearchFocused = false;
        } else if (unicode >= 32 && unicode < 127 && mInventorySearchQuery.size() < 30) {
            mInventorySearchQuery += (char)unicode;
        }
        return;
    }

    // If any modal is open, ignore POS search input
    if (mReceiptModalOpen || mQtyModalOpen || mCashModalOpen || mCardModalOpen ||
        mAddStaffModalOpen || mResetPassModalOpen || mAddInventoryModalOpen ||
        mItemDescModalOpen || mAddProductModalOpen || mDraftsModalOpen ||
        mQuickCheckModalOpen || mSpecModalOpen) {
        return;
    }

    // ISE-95: On POS Tab (Tab 0), seamless gun scanner & keyboard input without needing to click search box
    if (mActiveTab != 0) return;

    if (unicode == 8) { // Backspace
        if (!mSearchQuery.empty()) mSearchQuery.pop_back();
    } else if (unicode == 13 || unicode == 10) { // Enter or LF from gun scanner
        std::string q = mSearchQuery;
        while (!q.empty() && (q.front() == ' ' || q.front() == '\t')) q.erase(q.begin());
        while (!q.empty() && (q.back() == ' ' || q.back() == '\t' || q.back() == '\r' || q.back() == '\n')) q.pop_back();

        if (!q.empty()) {
            std::string qLower = toLowerStr(q);
            int idx = mProductCatalog.findProductByBarcode(q);
            if (idx < 0) idx = mProductCatalog.findProductByBarcode(qLower);

            if (idx >= 0) {
                onBarcodeScanned(mProductCatalog.getProducts()[idx].barcode);
                mSearchQuery.clear();
                mSearchFocused = true;
                return;
            }
            auto vis = mProductCatalog.getVisibleProducts(q, mActiveCategory);
            if (!vis.empty()) {
                simulateScan(vis[0]);
                mSearchQuery.clear();
                mSearchFocused = true;
                return;
            }
            showToast("Product / Barcode '" + q + "' not found");
            mSearchQuery.clear();
            mSearchFocused = true;
        }
    } else if (unicode >= 32 && unicode < 127 && mSearchQuery.size() < 40) {
        mSearchQuery += (char)unicode;
        mSearchFocused = true;

        // Fast Auto-Scan: Code 39 in ScanIT uses 6 digits. If exact 6 digits match a product barcode,
        // add to cart immediately in 0 clicks even if scanner omits Enter!
        if (mSearchQuery.size() == 6) {
            bool allDigits = true;
            for (char c : mSearchQuery) {
                if (c < '0' || c > '9') { allDigits = false; break; }
            }
            if (allDigits) {
                int bIdx = mProductCatalog.findProductByBarcode(mSearchQuery);
                if (bIdx >= 0) {
                    onBarcodeScanned(mProductCatalog.getProducts()[bIdx].barcode);
                    mSearchQuery.clear();
                    mSearchFocused = true;
                    return;
                }
            }
        }
    }
}

void PosApp::handleEvents() {
    sf::Event e;
    while (mWindow.pollEvent(e)) {
        if (e.type == sf::Event::Closed) {
            mWindow.close();
            return;
        }

        if (mShowSplash) {
            if (e.type == sf::Event::MouseButtonPressed || e.type == sf::Event::KeyPressed) {
                mSplashScreen.skip();
                mShowSplash = false;
                mShowLogin = true;
            }
            continue;
        }

        if (mShowLogin) {
            sf::Vector2f m = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow));
            mLoginScreen.handleEvent(e, m);
            if (mLoginScreen.isLoggedIn()) {
                mCurrentUserName = mLoginScreen.getLoggedInUser();
                mCurrentRole = mLoginScreen.getLoggedInRole();
                mShowLogin = false;
                if (mCurrentRole != "Admin" && (mActiveTab == 2 || mActiveTab == 4 || mActiveTab == 5)) {
                    mActiveTab = 0;
                }
                showToast("Logged in as " + mCurrentUserName + " (" + mCurrentRole + ")");
                mActivityLogger.logActivity("LOGIN", mCurrentUserName, "Authenticated user session (" + mCurrentRole + ") via login terminal");
            }
            continue;
        }

        if (e.type == sf::Event::Resized) {
            updateLayout((float)e.size.width, (float)e.size.height);
        } else if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f m = mWindow.mapPixelToCoords({e.mouseButton.x, e.mouseButton.y});
            handleClick(m);
        } else if (e.type == sf::Event::MouseWheelScrolled) {
            sf::Vector2f m = mWindow.mapPixelToCoords({e.mouseWheelScroll.x, e.mouseWheelScroll.y});
            handleScroll(e.mouseWheelScroll.delta, m);
        } else if (e.type == sf::Event::KeyPressed) {
            handleKeyPress(e.key.code);
        } else if (e.type == sf::Event::TextEntered) {
            handleTextInput(e.text.unicode);
        }
    }
}

void PosApp::update(float dt) {
    if (mShowSplash) {
        mSplashScreen.update(dt);
        if (mSplashScreen.isFinished()) {
            mShowSplash = false;
            mShowLogin = true;
        }
        return;
    }

    if (mShowLogin) {
        mLoginScreen.update(dt);
        return;
    }

    if (mToastTimer > 0.f) mToastTimer -= dt;
    if (mFlashTimer > 0.f) mFlashTimer -= dt;
    if (mLastScanTimer > 0.f) mLastScanTimer -= dt;
    if (mScanCooldown > 0.f) mScanCooldown -= dt;
    if (mDetectThrottleTimer > 0.f) mDetectThrottleTimer -= dt;
    if (mBarcodeFoundTimer > 0.f) {
        mBarcodeFoundTimer -= dt;
        if (mBarcodeFoundTimer <= 0.f) {
            mCameraBarcodeFound = false;
        }
    }

    if (mCameraRunning) {
        if (mCameraStream.getLatestFrame(mProcessedCamFrame, mCamWidth, mCamHeight)) {
            if (mCamWidth > 0 && mCamHeight > 0) {
                if (mCameraTexture.getSize().x != (unsigned)mCamWidth || mCameraTexture.getSize().y != (unsigned)mCamHeight) {
                    mCameraTexture.create(mCamWidth, mCamHeight);
                }

                // Convert camera frame to Black & White (Grayscale) for high-contrast optical scanning
                if (mCamDisplayBuffer.size() != mProcessedCamFrame.size()) {
                    mCamDisplayBuffer.resize(mProcessedCamFrame.size());
                }
                const unsigned char* src = mProcessedCamFrame.data();
                unsigned char* dst = mCamDisplayBuffer.data();
                size_t total = mProcessedCamFrame.size();
                for (size_t i = 0; i + 3 < total; i += 4) {
                    unsigned char gray = static_cast<unsigned char>((src[i] * 77 + src[i + 1] * 150 + src[i + 2] * 29) >> 8);
                    dst[i]     = gray;
                    dst[i + 1] = gray;
                    dst[i + 2] = gray;
                    dst[i + 3] = src[i + 3];
                }
                mCameraTexture.update(mCamDisplayBuffer.data());

                if (mDetectThrottleTimer <= 0.f) {
                    mDetectThrottleTimer = 0.08f;
                    std::string detectedCode;
                    BarcodeBox box;
                    if (detectBarcodeInFrame(mProcessedCamFrame.data(), mCamWidth, mCamHeight, true, detectedCode, box, true)) {
                        mCameraBarcodeFound = true;
                        mBarcodeBox = box;
                        mDetectedBarcode = detectedCode;
                        mBarcodeFoundTimer = 0.45f;
                        if (mScanCooldown <= 0.f) {
                            onBarcodeScanned(detectedCode);
                            mScanCooldown = 1.6f;
                        }
                    }
                }
            }
        }
    }
}

void PosApp::render() {
    mWindow.clear(ThemeColor::Paper);

    if (mShowSplash) {
        mSplashScreen.render(mWindow, mCurrentWidth, mDesignHeight);
        mWindow.display();
        return;
    }

    if (mShowLogin) {
        mLoginScreen.render(mWindow, mCurrentWidth, mDesignHeight);
        mWindow.display();
        return;
    }

    // Draw Classical Watercolor Artwork Background (Natural 1:1 scale tree artwork on left with ambient continuous canvas)
    const sf::Texture& bg = getTexture("art/bg_art_hd.png");
    const sf::Texture& bgFb = getTexture("art/bg_art.png");
    const sf::Texture& useBg = (bg.getSize().x > 0) ? bg : bgFb;

    if (useBg.getSize().x > 0) {
        sf::Sprite bgSprite(useBg);
        float s = mDesignHeight / (float)useBg.getSize().y;
        bgSprite.setScale(s, s);
        bgSprite.setPosition(Layout::SIDE_W, 0.f);
        // Natural antique warm tint matching parchment
        bgSprite.setColor(sf::Color(228, 222, 212));
        mWindow.draw(bgSprite);
    }

    sf::Vector2f m = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow));
    auto getTx = [this](const std::string& rel) -> const sf::Texture& { return getTexture(rel); };

    // Navigation and Shell
    PosNavigation::drawSidebar(mWindow, m, mDesignHeight, mActiveTab, mCurrentRole, mCurrentUserName,
                               mFontRegular, mFontMedium, mFontSemiBold, getTx);

    if (mActiveTab == 0) {
        // POS Cash Register Mode
        PosNavigation::drawHeader(mWindow, m, mMainX, mCurrentRole, mCurrentUserName,
                                  mFontRegular, mFontMedium, mFontSemiBold, mFontSerif);
        PosNavigation::drawSearch(mWindow, m, mMainX, mSearchQuery, mSearchFocused, mFontRegular, mFontSemiBold, getTx);
        PosProductsView::drawViewport(mWindow, mMainX, mCameraTexture, mFlashTimer, mLastScanTimer, mLastScannedInfo,
                                      mCameraBarcodeFound, mBarcodeBox, mCamWidth, mCamHeight, mDetectedBarcode,
                                      mCameraStream.getCameraName(), mCameraStream.isActiveCameraExternal(),
                                      CameraStream::getAvailableCameraCount(),
                                      mFontSemiBold, mFontBold, getTx);
        auto visible = mProductCatalog.getVisibleProducts(mSearchQuery, mActiveCategory);
        PosProductsView::drawProducts(mWindow, m, mMainX, mProductCatalog.getProducts(), visible, mActiveCategory,
                                     mFontRegular, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mActiveTab == 1) {
        PosProductsView::drawProductsTab(mWindow, m, mMainX, mProductCatalog.getProducts(), mProductsScrollOffset,
                                         mCurrentRole, mProductCatalog.getProductFilePath(),
                                         mProductsSearchQuery, mProductsSearchFocused,
                                         mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 2) {
        PosProductsView::drawInventoryTab(mWindow, m, mMainX, mProductCatalog.getProducts(),
                                          mInventoryManager, mInventorySubTab, mInventoryScrollOffset,
                                          mCurrentRole, mInventorySearchQuery, mInventorySearchFocused,
                                          mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 3) {
        PosReportsView::drawSalesTab(mWindow, m, mMainX, mCartService.getSalesHistory(),
                                     mProductCatalog.getProducts(), mInventoryManager,
                                     mSalesTimeFilter, mSalesScrollOffset,
                                     mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 4) {
        PosReportsView::drawActivityLogsTab(mWindow, m, mMainX, mActivityLogger.getLogs(), mLogFilter, mLogScrollOffset,
                                            mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 5) {
        PosReportsView::drawSettingsTab(mWindow, m, mMainX, mStaffManager.getStaffList(), mCurrentRole, mCurrentUserName,
                                        mCameraRunning, mSoundEnabled,
                                        mCameraStream.getCameraName(), mCameraStream.isActiveCameraExternal(),
                                        CameraStream::getAvailableCameraCount(),
                                        mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 6) {
        const ProductItem* pPtr = (mSelectedReturnProductIdx >= 0 && mSelectedReturnProductIdx < (int)mProductCatalog.getProducts().size())
                                ? &mProductCatalog.getProducts()[mSelectedReturnProductIdx] : nullptr;
        PosReportsView::drawReturnTab(mWindow, m, mMainX, mReturnBarcode, mReturnBarcodeFocused,
                                      mReturnQty, pPtr, mReturnSaleInfo, mReturnSalePrice,
                                      mReturnOrderId, mReturnCustomer, mReturnDate,
                                      mReturnSuccess, mReturnStatusMessage, mCameraTexture,
                                      mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    }

    // Receipt Drawer
    const auto& prods = mProductCatalog.getProducts();
    PosReceiptView::drawReceiptDrawer(mWindow, m, mPanelX, mRpX, mDesignHeight,
                                     mCartService.getCart(), prods,
                                     mCartService.getSubtotal(prods),
                                     mCartService.getDiscount(),
                                     mCartService.getTotal(prods),
                                     mCartService.isDiscountApplied(),
                                     mCartService.getPaymentMethod(),
                                     mCartService.isPaymentDone(),
                                     mReceiptScrollOffset,
                                     (int)mCartService.getDrafts().size(),
                                     mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);

    // Modals
    if (mQuickCheckModalOpen) {
        const ProductItem* pPtr = (mQuickCheckProductIdx >= 0 && mQuickCheckProductIdx < (int)prods.size())
                                ? &prods[mQuickCheckProductIdx] : nullptr;
        PosModalsView::drawQuickCheckModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                          mQuickCheckBarcode, mQuickCheckInputFocused, pPtr,
                                          mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mSpecModalOpen && mSpecProductIdx >= 0 && mSpecProductIdx < (int)prods.size()) {
        PosModalsView::drawProductSpecModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                           prods[mSpecProductIdx],
                                           mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mReceiptModalOpen) {
        PosModalsView::drawReceiptModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                       mCartService, prods, mCurrentUserName,
                                       mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mItemDescModalOpen) {
        PosModalsView::drawItemDescriptionModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                               mDescItemName, mDescItemBarcode, mDescItemCategory, mDescItemUnit,
                                               mDescItemPrice, mDescItemCost, mDescItemStock,
                                               mDescItemSupplier, mDescItemDate,
                                               mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mDraftsModalOpen) {
        PosModalsView::drawHeldDraftsModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                          mCartService.getDrafts(), prods,
                                          mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mAddProductModalOpen) {
        std::vector<int> visible = mInventoryManager.getFilteredItemIndices(mAddProductSearchQuery);
        PosModalsView::drawAddProductFromInventoryModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                                       mInventoryManager.getItems(), visible,
                                                       mAddProductSearchQuery, mAddProductSearchFocused,
                                                       mSelectedInventoryForProduct, mTransferQtyStr, mAddProductScrollOffset,
                                                       mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mEnlargeCartModalOpen) {
        PosModalsView::drawEnlargeCartModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                           mCartService.getCart(), prods,
                                           mCartService.getSubtotal(prods),
                                           mCartService.getTotal(prods),
                                           mEnlargeScrollOffset,
                                           mFontRegular, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mCashModalOpen) {
        PosModalsView::drawCashModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                    mCartService.getTotal(prods),
                                    mCartService.parseCashGiven(),
                                    mCartService.getCashGivenStr(),
                                    mCashInputFocused,
                                    mFontRegular, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mCardModalOpen) {
        PosModalsView::drawCardModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                    mCartService.getTotal(prods),
                                    mFontRegular, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mQtyModalOpen && mEditingCartIndex >= 0 && mEditingCartIndex < (int)mCartService.getCart().size()) {
        const auto& item = mCartService.getCart()[mEditingCartIndex];
        if (item.productId >= 0 && item.productId < (int)prods.size()) {
            PosModalsView::drawQtyModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                       prods[item.productId], mQtyInputStr,
                                       mFontSemiBold, mFontBold, mFontSerif);
        }
    } else if (mAddStaffModalOpen) {
        PosModalsView::drawAddStaffModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                        mNewStaffName, mNewStaffPass, mNewStaffRole, mNewStaffActiveField,
                                        mFontRegular, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mResetPassModalOpen) {
        PosModalsView::drawResetPassModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                         mResetStaffTarget, mResetStaffNewPass, mResetPassFocused,
                                         mFontRegular, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mAddInventoryModalOpen) {
        PosModalsView::drawAddInventoryModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                            mNewItemSupplier, mNewItemDesc, mNewItemCategory, mNewItemUnit,
                                            mNewItemPriceStr, mNewItemCostStr, mNewItemQtyStr, mNewItemDateStr,
                                            mNewItemBarcode, mNewItemActiveField, mNewItemGenerated,
                                            mInventoryManager.getUniqueSuppliers(), mVendorDropdownOpen, mScanningUniversalSupplier,
                                            mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    }

    // Toast Notification
    if (mToastTimer > 0.f && !mToastMessage.empty()) {
        float alpha = std::min(1.f, mToastTimer / 0.3f);
        float tw = getTextWidth(mFontMedium, mToastMessage, 12) + 40.f;
        float tx = (mCurrentWidth - tw) / 2.f;
        float ty = 22.f;

        drawShadow(mWindow, tx, ty, tw, 36.f, 10, 4, 10.f, (int)(18 * alpha), 2);
        drawRounded(mWindow, tx, ty, tw, 36.f, 8, sf::Color(32, 28, 26, (sf::Uint8)(240 * alpha)));
        drawText(mWindow, mFontMedium, mToastMessage, 12, sf::Color(255, 255, 255, (sf::Uint8)(255 * alpha)),
                 tx + tw / 2.f, ty + 18.f, TextAlignment::Center);
    }

    mWindow.display();
}

void PosApp::run() {
    sf::Clock clock;
    while (mWindow.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;

        handleEvents();
        update(dt);
        render();

        if (mScreenshotMode) {
            sf::Texture scrTx;
            scrTx.create(mWindow.getSize().x, mWindow.getSize().y);
            scrTx.update(mWindow);
            sf::Image scrImg = scrTx.copyToImage();

            std::string outName = "screenshot.png";
            if (!mModalShot.empty()) {
                outName = "screenshot_" + mModalShot + ".png";
            }
            scrImg.saveToFile(outName);
            mWindow.close();
            return;
        }
    }
}

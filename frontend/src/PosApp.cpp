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
#include "PdfGenerator.h"

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

static bool parseBillsFile(const std::string& filename, std::vector<BillRecord>& bills);

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

static bool parseBillDetailsFile(const std::string& filename, std::vector<BillDetailRecord>& details) {
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
        BillDetailRecord d;
        if (iss >> d.billId >> d.productAcronym >> d.unitPrice >> d.quantity >> d.productDiscount) {
            d.discountOnProduct = d.productDiscount;
            details.push_back(d);
        }
    }
    return true;
}

void PosApp::reloadAuditData() {
    parseOrdersFile("Order.txt", mOrders);
    parseOrderDetailsFile("OrderDetail.txt", mOrderDetails);
    parseBillsFile("Bill.txt", mBills);
    parseBillDetailsFile("BillDetail.txt", mBillDetails);

    std::unordered_map<std::string, std::vector<CartLine>> orderLines;
    for (const auto& d : mOrderDetails) {
        int pIdx = mProductCatalog.findProductByBarcode(d.productBarcode);
        CartLine cl;
        cl.productId = (pIdx >= 0) ? pIdx : 0;
        cl.quantity = d.quantity;
        orderLines[d.orderId].push_back(cl);
    }

    auto& sales = mCartService.getSalesHistory();
    sales.clear();
    for (const auto& ord : mOrders) {
        SaleRecord sr;
        sr.orderId = ord.orderId;
        sr.timestamp = ord.orderDate + " " + ord.orderTime;
        sr.totalAmount = ord.paymentAmount;
        sr.paymentMethod = (ord.paymentMode == "card" ? "Card" : (ord.paymentMode == "wallet" ? "Wallet" : "Cash"));
        sr.cashier = ord.customerName.empty() ? "admin" : ord.customerName;
        for (char& c : sr.cashier) if (c == '_') c = ' ';
        if (orderLines.find(ord.orderId) != orderLines.end()) {
            sr.items = orderLines[ord.orderId];
            sr.itemCount = 0;
            for (const auto& line : sr.items) sr.itemCount += line.quantity;
        } else {
            sr.itemCount = 1;
        }
        sales.push_back(sr);
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

    // Sync ProductCatalog categoryIds from CategoryManager (PCategory.txt)
    syncProductCategoriesFromManager();

    // Set initial cash given from cart total and sort databases alphabetically
    mProductCatalog.sortAlphabetically();
    mInventoryManager.sortAlphabetically();
    mCartService.setCashGiven(computeCurrentBillSummary().totalPayable);

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
    loadReturnsFile("returns.txt");

    // Populate Sales History & Audit Records from Order.txt, OrderDetail.txt, Bill.txt
    reloadAuditData();

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
                BillSummary pSum = computeCurrentBillSummary();
                mCartService.setCashGiven(pSum.totalPayable + 100);
                std::string msg;
                mCartService.chargeSale(mProductCatalog.getProducts(), mCurrentUserName, msg, &pSum, "");
                bool ok = false;
                mCartService.printReceipt(mProductCatalog.getProducts(), mCurrentUserName, ok, &pSum);
                mReceiptModalOpen = false;
            }

            if (mModalShot == "receipt") {
                BillSummary rSum = computeCurrentBillSummary();
                mCartService.setCashGiven(rSum.totalPayable + 150);
                std::string msg;
                mCartService.chargeSale(mProductCatalog.getProducts(), mCurrentUserName, msg, &rSum, "");
                recordSaleToOutputFiles(mCartService.getCart(), mCurrentUserName, 0, "paid");
                bool ok = false;
                mCartService.printReceipt(mProductCatalog.getProducts(), mCurrentUserName, ok, &rSum);
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
            } else if (mModalShot == "return_slip") {
                mActiveTab = 6;
                lookupReturnItem("010001");
                processReturn();
            } else if (mModalShot == "return_item") {
                mActiveTab = 6;
                lookupReturnItem("010001");
            } else if (mModalShot == "return") {
                mActiveTab = 6;
                mSelectedReturnProductIdx = -1;
                mReturnBarcode = "";
            } else if (mModalShot == "discount") {
                mDiscountModalOpen = true;
            } else if (mModalShot == "quickcheck") {
                mQuickCheckModalOpen = true;
                mQuickCheckBarcode = "010001";
                mQuickCheckProductIdx = mProductCatalog.findProductByBarcode("010001");
            } else if (mModalShot == "spec") {
                mSpecModalOpen = true;
                mSpecProductIdx = 0;
            } else if (mModalShot == "desc") {
                mActiveTab = 1;
                mDescItemName = "Fresh Milk 1L";
                mDescItemBarcode = "010001";
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
            } else if (mModalShot == "categories") {
                mActiveTab = 5;
                mSettingsSubTab = 1;
            } else if (mModalShot == "seasons") {
                mActiveTab = 5;
                mSettingsSubTab = 2;
            } else if (mModalShot == "editproduct") {
                mActiveTab = 1;
                openEditProductModal(0);
            } else if (mModalShot == "editorder") {
                openEditOrderModal();
            } else if (mModalShot == "approval") {
                mApprovalTargetOrderId = mOrders.empty() ? "00000117" : mOrders.back().orderId;
                mApprovalTargetOrderDate = mOrders.empty() ? "2026-11-25" : mOrders.back().orderDate;
                mApprovalPassInput = "1234";
                mApprovalErrorMsg = "";
                mManagerApprovalModalOpen = true;
            } else if (mModalShot == "addcategory") {
                mActiveTab = 5;
                mSettingsSubTab = 1;
                openAddCategoryModal();
            } else if (mModalShot == "addseason") {
                mActiveTab = 5;
                mSettingsSubTab = 2;
                openAddSeasonModal();
            } else if (mModalShot == "addinv") {
                mActiveTab = 2;
                mAddInventoryModalOpen = true;
                mScanningUniversalSupplier = true;
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
            } else if (mModalShot == "sales_returns") {
                mActiveTab = 3;
                mSalesSubTab = 1;
            } else if (mModalShot == "report1") {
                mActiveTab = 3;
                mSalesSubTab = 0;
            } else if (mModalShot == "report2") {
                mActiveTab = 3;
                mSalesSubTab = 1;
            } else if (mModalShot == "report3") {
                mActiveTab = 3;
                mSalesSubTab = 2;
                mSalesTimeFilter = 1; // Weekly
            } else if (mModalShot == "report3_custom") {
                mActiveTab = 3;
                mSalesSubTab = 2;
                mSalesTimeFilter = 4; // Custom
                mCashCustomStartDate = "2026-11-17";
                mCashCustomEndDate = "2026-11-19";
            } else if (mModalShot == "report4") {
                mActiveTab = 3;
                mSalesSubTab = 3;
            } else if (mModalShot == "report5") {
                mActiveTab = 3;
                mSalesSubTab = 4;
            } else if (mModalShot == "report6") {
                mActiveTab = 3;
                mSalesSubTab = 5;
            } else if (mModalShot == "cashier_reports") {
                mActiveTab = 3;
                mCurrentUserName = "cashier1";
                mCurrentRole = "Employee";
            } else if (mModalShot == "sales_employee") {
                mActiveTab = 3;
                mCurrentUserName = "cashier1";
                mCurrentRole = "Employee";
            } else if (mModalShot == "saledetail") {
                mActiveTab = 3;
                mSelectedSaleIndex = 0;
                mSaleDetailModalOpen = true;
            } else if (mModalShot == "promo") {
                mActiveTab = 5;
                mPromoModalOpen = true;
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
        std::vector<std::string> tokens;
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);
        if (tokens.size() >= 12) {
            BillRecord b;
            b.billId = tokens[0];
            b.customerName = tokens[1];
            b.orderId = tokens[2];
            bool token3IsNum = !tokens[3].empty() && (std::isdigit((unsigned char)tokens[3][0]) || tokens[3][0] == '-');
            if (token3IsNum && tokens.size() >= 14) {
                try {
                    b.totalWithoutTax = std::stoi(tokens[3]);
                    b.totalTax = std::stoi(tokens[4]);
                    b.totalDiscount = std::stoi(tokens[5]);
                    b.totalPayable = std::stoi(tokens[6]);
                    b.amountReceived = std::stoi(tokens[7]);
                    b.balanceReturned = std::stoi(tokens[8]);
                    b.paymentStatus = tokens[9];
                    b.seasonalDiscount = std::stoi(tokens[10]);
                    b.specialDiscount = std::stoi(tokens[11]);
                    b.billDate = tokens[12];
                    b.billTime = tokens[13];
                } catch (...) {}
            } else {
                b.productAcronym = tokens[3];
                try {
                    b.unitPrice = std::stoi(tokens[4]);
                    b.discountPerUnit = std::stoi(tokens[5]);
                    b.quantity = std::stoi(tokens[6]);
                    b.totalWithoutTax = std::stoi(tokens[7]);
                    b.totalTax = std::stoi(tokens[8]);
                    b.totalDiscount = std::stoi(tokens[9]);
                    b.totalPayable = std::stoi(tokens[10]);
                    b.paymentStatus = tokens[11];
                } catch (...) {}
            }
            bills.push_back(b);
        }
    }
    return true;
}

void PosApp::lookupReturnItem(const std::string& inputQuery) {
    std::string query = trimStr(inputQuery);
    mReturnBarcode = query;
    mReturnSuccess = false;
    mReturnStatusMessage = "";
    mReturnQty = 1;
    mReturnMaxQty = 0;
    mReturnOrderId = "";
    mReturnCustomer = "";
    mReturnCustomerContact = "";
    mReturnDate = "";
    mReturnSalePrice = 0;
    mReturnSaleInfo = "";
    mReturnOrderItemPills.clear();

    std::vector<OrderRecord> orders;
    std::vector<ReturnOrderDetailItem> details;
    std::vector<BillDetailRecord> billDetails;
    parseOrdersFile("Order.txt", orders);
    parseOrderDetailsFile("OrderDetail.txt", details);
    parseBillDetailsFile("BillDetail.txt", billDetails);

    // 1. Check if query is an Order ID (or compound "orderId|barcode" from item pill click)
    std::string requestedOrderId;
    std::string requestedBarcode;
    size_t barPos = query.find('|');
    if (barPos != std::string::npos) {
        requestedOrderId = trimStr(query.substr(0, barPos));
        requestedBarcode = trimStr(query.substr(barPos + 1));
        mReturnBarcode = requestedBarcode;
    } else {
        requestedOrderId = query;
    }

    bool queryIsExactProductBarcode = (barPos == std::string::npos &&
                                       (mProductCatalog.findProductByBarcode(requestedOrderId) >= 0 ||
                                        mInventoryManager.findByBarcode(requestedOrderId) != nullptr));

    const OrderRecord* matchedOrder = nullptr;
    for (const auto& ord : orders) {
        if (ord.orderId == requestedOrderId || ("ORD-" + ord.orderId) == requestedOrderId) {
            matchedOrder = &ord;
            break;
        }
    }
    if (!matchedOrder && !queryIsExactProductBarcode && requestedOrderId.length() >= 6) {
        for (const auto& ord : orders) {
            if (ord.orderId.find(requestedOrderId) != std::string::npos) {
                matchedOrder = &ord;
                break;
            }
        }
    }

    std::string targetBarcode = "";
    if (matchedOrder != nullptr) {
        mReturnOrderId = matchedOrder->orderId;
        mReturnDate = matchedOrder->orderDate + " " + matchedOrder->orderTime;
        mReturnCustomer = matchedOrder->customerName;
        for (char& c : mReturnCustomer) if (c == '_') c = ' ';
        mReturnCustomerContact = matchedOrder->customerContact;

        // Collect all line items for this order so cashier can switch between items in multi-item orders
        for (const auto& d : details) {
            if (d.orderId == matchedOrder->orderId) {
                int pIdx = mProductCatalog.findProductByBarcode(d.productBarcode);
                std::string lbl = (pIdx >= 0) ? mProductCatalog.getProducts()[pIdx].name : d.productBarcode;
                bool dup = false;
                for (const auto& ep : mReturnOrderItemPills) {
                    if (ep.first == d.productBarcode) { dup = true; break; }
                }
                if (!dup) mReturnOrderItemPills.push_back({d.productBarcode, lbl});
                if (!requestedBarcode.empty() && d.productBarcode == requestedBarcode) {
                    targetBarcode = d.productBarcode;
                } else if (targetBarcode.empty()) {
                    targetBarcode = d.productBarcode;
                }
            }
        }
        if (targetBarcode.empty()) {
            std::vector<BillRecord> bills;
            parseBillsFile("Bill.txt", bills);
            for (const auto& b : bills) {
                if (b.orderId == matchedOrder->orderId) {
                    int pIdx = mProductCatalog.findProductByName(b.productAcronym);
                    if (pIdx >= 0) targetBarcode = mProductCatalog.getProducts()[pIdx].barcode;
                    break;
                }
            }
        }
    } else {
        targetBarcode = query;
    }

    // Lookup product by barcode, or by name if not found
    mSelectedReturnProductIdx = mProductCatalog.findProductByBarcode(targetBarcode);
    if (mSelectedReturnProductIdx < 0) {
        mSelectedReturnProductIdx = mProductCatalog.findProductByName(targetBarcode);
    }
    if (mSelectedReturnProductIdx < 0) {
        const auto* inv = mInventoryManager.findByBarcode(targetBarcode);
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
            mSelectedReturnProductIdx = mProductCatalog.findProductByBarcode(inv->barcode);
        }
    }

    if (mSelectedReturnProductIdx >= 0) {
        const auto& p = mProductCatalog.getProducts()[mSelectedReturnProductIdx];
        mReturnSalePrice = p.price;

        bool matched = false;
        int purchasedQty = 0;

        if (matchedOrder != nullptr) {
            matched = true;
            for (const auto& d : details) {
                if (d.orderId == matchedOrder->orderId && (d.productBarcode == p.barcode || d.productId == p.id)) {
                    purchasedQty += d.quantity;
                }
            }
            if (purchasedQty == 0) purchasedQty = 1;
            mReturnSaleInfo = "Verified Purchase in Order #" + matchedOrder->orderId;
        } else {
            const OrderRecord* bestOrder = nullptr;
            int bestPurchasedQty = 0;
            const OrderRecord* fallbackOrder = nullptr;
            int fallbackPurchasedQty = 0;

            for (int i = (int)orders.size() - 1; i >= 0; --i) {
                const auto& ord = orders[i];
                if (toLowerStr(ord.orderStatus) != "paid") continue;
                int qtyInOrd = 0;
                for (const auto& d : details) {
                    if (d.orderId == ord.orderId && (d.productBarcode == p.barcode || d.productId == p.id)) {
                        qtyInOrd += d.quantity;
                    }
                }
                if (qtyInOrd <= 0) continue;

                int prevRef = 0;
                for (const auto& ret : mReturnHistory) {
                    if ((ret.orderRef == ord.orderId || ret.orderRef.find(ord.orderId) != std::string::npos)
                        && ret.barcode == p.barcode) {
                        prevRef += ret.quantity;
                    }
                }
                if (!fallbackOrder) {
                    fallbackOrder = &ord;
                    fallbackPurchasedQty = qtyInOrd;
                }
                if (qtyInOrd - prevRef > 0) {
                    bestOrder = &ord;
                    bestPurchasedQty = qtyInOrd;
                    break;
                }
            }

            const OrderRecord* chosenOrd = bestOrder ? bestOrder : fallbackOrder;
            int chosenQty = bestOrder ? bestPurchasedQty : fallbackPurchasedQty;
            if (chosenOrd) {
                mReturnOrderId = chosenOrd->orderId;
                mReturnDate = chosenOrd->orderDate + " " + chosenOrd->orderTime;
                mReturnCustomer = chosenOrd->customerName;
                for (char& c : mReturnCustomer) if (c == '_') c = ' ';
                mReturnCustomerContact = chosenOrd->customerContact;
                purchasedQty = chosenQty;
                mReturnSaleInfo = "Verified Purchase in Order #" + chosenOrd->orderId;
                matched = true;

                for (const auto& d : details) {
                    if (d.orderId == chosenOrd->orderId) {
                        int pIdx = mProductCatalog.findProductByBarcode(d.productBarcode);
                        std::string lbl = (pIdx >= 0) ? mProductCatalog.getProducts()[pIdx].name : d.productBarcode;
                        bool dup = false;
                        for (const auto& ep : mReturnOrderItemPills) {
                            if (ep.first == d.productBarcode) { dup = true; break; }
                        }
                        if (!dup) mReturnOrderItemPills.push_back({d.productBarcode, lbl});
                    }
                }
            }
        }

        // Bill.txt & BillDetail.txt lookup for item-specific discounted sold price
        std::vector<BillRecord> bills;
        parseBillsFile("Bill.txt", bills);
        if (bills.empty()) parseBillsFile("frontend/Bill.txt", bills);

        for (int i = (int)bills.size() - 1; i >= 0; --i) {
            bool matchesOrder = (!mReturnOrderId.empty() && bills[i].orderId == mReturnOrderId);
            if (!matchesOrder) continue;

            if (!bills[i].productAcronym.empty()) {
                if (bills[i].productAcronym == p.acronym || bills[i].unitPrice == p.price) {
                    int discPerUnit = bills[i].discountPerUnit;
                    int effectivePrice = bills[i].unitPrice - discPerUnit;
                    if (effectivePrice > 0 && effectivePrice <= p.price) {
                        mReturnSalePrice = effectivePrice;
                    }
                    break;
                }
            } else {
                // 14-field Bill.txt format -> check BillDetail.txt for item line
                for (const auto& bd : billDetails) {
                    if (bd.billId == bills[i].billId && (bd.productAcronym == p.acronym || bd.unitPrice == p.price)) {
                        int discPerUnit = (bd.quantity > 0) ? (bd.productDiscount / bd.quantity) : 0;
                        int baseUnit = bd.unitPrice - discPerUnit;
                        int netBeforeSeason = std::max(1, bills[i].totalWithoutTax + bills[i].totalTax - bills[i].totalDiscount);
                        if (bills[i].totalPayable > 0 && bills[i].totalPayable < netBeforeSeason) {
                            baseUnit = (baseUnit * bills[i].totalPayable) / netBeforeSeason;
                        }
                        if (baseUnit > 0 && baseUnit <= p.price) {
                            mReturnSalePrice = baseUnit;
                        }
                        break;
                    }
                }
                break;
            }
        }

        // If not found in Bill.txt, check Order.txt total vs catalog total
        if (mReturnSalePrice == p.price && !mReturnOrderId.empty()) {
            int ordSubtotal = 0;
            for (const auto& d : details) {
                if (d.orderId == mReturnOrderId) {
                    int pIdx = mProductCatalog.findProductByBarcode(d.productBarcode);
                    int itemPrice = (pIdx >= 0) ? mProductCatalog.getProducts()[pIdx].price : p.price;
                    ordSubtotal += itemPrice * d.quantity;
                }
            }
            for (const auto& ord : orders) {
                if (ord.orderId == mReturnOrderId && ordSubtotal > 0 && ord.paymentAmount < ordSubtotal) {
                    int effectivePrice = (p.price * ord.paymentAmount) / ordSubtotal;
                    if (effectivePrice > 0 && effectivePrice < p.price) {
                        mReturnSalePrice = effectivePrice;
                    }
                    break;
                }
            }
        }

        // If not found in files, check current session sales history
        if (mReturnSalePrice == p.price && !mReturnOrderId.empty()) {
            for (const auto& s : mCartService.getSalesHistory()) {
                if (s.orderId == mReturnOrderId) {
                    int saleSubtotal = 0;
                    for (const auto& line : s.items) {
                        if (line.productId >= 0 && line.productId < (int)mProductCatalog.getProducts().size()) {
                            saleSubtotal += mProductCatalog.getProducts()[line.productId].price * line.quantity;
                        }
                    }
                    if (saleSubtotal > 0 && s.totalAmount < saleSubtotal) {
                        int effectivePrice = (p.price * s.totalAmount) / saleSubtotal;
                        if (effectivePrice > 0 && effectivePrice < p.price) {
                            mReturnSalePrice = effectivePrice;
                        }
                    }
                    break;
                }
            }
        }

        // If item has static discount in product catalog
        if (mReturnSalePrice == p.price && p.discount > 0) {
            mReturnSalePrice = std::max(1, p.price - p.discount);
        }

        if (!matched) {
            for (int i = (int)bills.size() - 1; i >= 0; --i) {
                if (bills[i].productAcronym == p.acronym || bills[i].unitPrice == p.price) {
                    mReturnOrderId = bills[i].orderId;
                    mReturnCustomer = bills[i].customerName;
                    for (char& c : mReturnCustomer) if (c == '_') c = ' ';
                    mReturnDate = "Verified Bill #" + bills[i].billId;
                    int discPerUnit = bills[i].discountPerUnit;
                    if (discPerUnit <= 0 && bills[i].totalDiscount > 0 && bills[i].totalWithoutTax > 0) {
                        discPerUnit = (bills[i].unitPrice * bills[i].totalDiscount) / bills[i].totalWithoutTax;
                    }
                    int effectivePrice = bills[i].unitPrice - discPerUnit;
                    mReturnSalePrice = (effectivePrice > 0) ? effectivePrice : bills[i].unitPrice;
                    purchasedQty = bills[i].quantity;
                    mReturnSaleInfo = "Verified Bill #" + bills[i].billId;
                    matched = true;
                    break;
                }
            }
        }

        // Check already refunded units for this order & barcode in returns.txt to prevent fraud
        int alreadyRefunded = 0;
        for (const auto& ret : mReturnHistory) {
            if ((ret.orderRef == mReturnOrderId || (!mReturnOrderId.empty() && ret.orderRef.find(mReturnOrderId) != std::string::npos))
                && ret.barcode == p.barcode) {
                alreadyRefunded += ret.quantity;
            }
        }

        if (matched) {
            mReturnMaxQty = std::max(0, purchasedQty - alreadyRefunded);
            mReturnQty = (mReturnMaxQty > 0) ? 1 : 0;
            if (mReturnMaxQty <= 0 && alreadyRefunded > 0) {
                mReturnSaleInfo = "All " + std::to_string(purchasedQty) + " units already refunded";
                mReturnSuccess = false;
                mReturnStatusMessage = "Ineligible: " + std::to_string(alreadyRefunded) + " units previously refunded.";
                showToast("All units of this item were already refunded.");
            } else {
                showToast("Located Item: " + p.name + " (" + std::to_string(mReturnMaxQty) + " eligible)");
            }
        } else {
            mReturnMaxQty = 0;
            mReturnQty = 0;
            mReturnSaleInfo = "No verified purchase record in database (Return Ineligible)";
            showToast("Warning: No purchase record found for this customer / item.");
        }
    } else {
        showToast("Item / Order '" + query + "' not found in database.");
    }
}

void PosApp::processReturn() {
    if (mSelectedReturnProductIdx < 0 || mSelectedReturnProductIdx >= (int)mProductCatalog.getProducts().size()) {
        showToast("Please scan or select a valid item first.");
        return;
    }
    if (mReturnMaxQty <= 0) {
        showToast("Cannot process return: 0 units eligible for return.");
        return;
    }
    if (mReturnQty <= 0) {
        showToast("Return quantity must be at least 1.");
        return;
    }
    if (mReturnQty > mReturnMaxQty) {
        showToast("Error: Return quantity (" + std::to_string(mReturnQty) + ") exceeds eligible units (" + std::to_string(mReturnMaxQty) + ")!");
        return;
    }

    const auto& p = mProductCatalog.getProducts()[mSelectedReturnProductIdx];
    int refundTotal = ((mReturnSalePrice > 0) ? mReturnSalePrice : p.price) * mReturnQty;
    int returnedCount = mReturnQty;

    // 1. Restock handling: if mReturnRestock is true, update BOTH product.txt AND inventory.txt
    if (mReturnRestock) {
        mProductCatalog.addStock(mSelectedReturnProductIdx, returnedCount);
        mInventoryManager.restockItem(p.barcode, returnedCount);
    }

    // 2. Reason & Refund method strings
    const char* reasonLabels[] = {"Changed Mind", "Defective / Damaged", "Wrong Item", "Expired"};
    mReturnReason = (mReturnReasonIdx >= 0 && mReturnReasonIdx < 4) ? reasonLabels[mReturnReasonIdx] : "Customer Changed Mind";

    const char* methodLabels[] = {"Cash", "Card / Original", "Store Credit"};
    std::string refMethod = (mReturnPaymentMethod >= 0 && mReturnPaymentMethod < 3) ? methodLabels[mReturnPaymentMethod] : "Cash";

    std::string conditionStr = mReturnRestock ? "Restocked" : "Damaged / Disposed";

    // 3. Activity Logger
    std::string logMsg = "Customer returned " + std::to_string(returnedCount) + "x " + p.name + " (" + p.barcode +
        ") - Refunded Rs " + std::to_string(refundTotal) + " via " + refMethod + " [" + mReturnReason + "] - " + conditionStr;
    mActivityLogger.logActivity("RETURN", mCurrentUserName, logMsg);

    // 4. Customer Return Audit Record (returns.txt)
    ReturnRecord retRec;
    int maxRetNum = 1000;
    for (const auto& rh : mReturnHistory) {
        if (rh.returnId.size() > 4 && rh.returnId.rfind("RET-", 0) == 0) {
            try { maxRetNum = std::max(maxRetNum, std::stoi(rh.returnId.substr(4))); } catch (...) {}
        }
    }
    char retIdBuf[32];
    sprintf_s(retIdBuf, "RET-%04d", maxRetNum + 1);
    retRec.returnId = retIdBuf;

    std::time_t now = std::time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    char timeBuf[64];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmNow);
    retRec.timestamp = timeBuf;
    retRec.cashier = mCurrentUserName.empty() ? "admin" : mCurrentUserName;
    retRec.barcode = p.barcode;
    retRec.itemName = p.name;
    retRec.quantity = returnedCount;
    retRec.refundAmount = refundTotal;
    retRec.orderRef = mReturnOrderId.empty() ? "DIRECT-RETURN" : mReturnOrderId;
    retRec.reason = mReturnReason;
    retRec.refundMethod = refMethod;
    retRec.customer = mReturnCustomer.empty() ? "Store Customer" : mReturnCustomer;
    retRec.condition = conditionStr;

    appendReturnRecord(retRec);

    // 5. Generate formatted Return Receipt Slip
    std::ostringstream slip;
    slip << "==================================================\n";
    slip << "               SCANIT SUPERMARKET                 \n";
    slip << "            CUSTOMER RETURN RECEIPT               \n";
    slip << "==================================================\n";
    slip << "Return ID:       " << retRec.returnId << "\n";
    slip << "Timestamp:       " << retRec.timestamp << "\n";
    slip << "Cashier:         " << retRec.cashier << "\n";
    slip << "Customer:        " << retRec.customer << "\n";
    slip << "Original Order:  #" << retRec.orderRef << "\n";
    slip << "--------------------------------------------------\n";
    slip << "Returned Item:   " << retRec.itemName << " (" << retRec.barcode << ")\n";
    slip << "Quantity:        " << retRec.quantity << "\n";
    slip << "Unit Sold Price: Rs " << mReturnSalePrice << "\n";
    slip << "TOTAL REFUND:    Rs " << retRec.refundAmount << "\n";
    slip << "Refund Method:   " << retRec.refundMethod << "\n";
    slip << "Return Reason:   " << retRec.reason << "\n";
    slip << "Item Status:     " << retRec.condition << "\n";
    slip << "==================================================\n";
    slip << "    * OFFICIAL RETURN PROOF & AUDIT RECORD *     \n";
    slip << "==================================================\n";
    mReturnSlipText = slip.str();

    std::ofstream rOut("return_receipt.txt");
    if (rOut.is_open()) {
        rOut << mReturnSlipText;
        rOut.close();
    }
    std::ofstream rOutF("frontend/return_receipt.txt");
    if (rOutF.is_open()) {
        rOutF << mReturnSlipText;
        rOutF.close();
    }

    mReturnMaxQty -= returnedCount;
    mReturnQty = (mReturnMaxQty > 0) ? 1 : 0;
    mReturnSuccess = true;
    mReturnStatusMessage = "Returned " + std::to_string(returnedCount) + "x " + p.name +
        " -> Rs " + std::to_string(refundTotal) + " (" + refMethod + ") refunded. " +
        (mReturnRestock ? "Stock restored." : "Item marked scrap.");
    mReturnSlipModalOpen = true;
    showToast("Return Processed! Receipt saved to return_receipt.txt");
}
void PosApp::syncProductCategoriesFromManager() {
    const auto& cats = mCategoryManager.getCategories();
    if (cats.empty()) return;
    bool changed = false;
    for (auto& p : mProductCatalog.getProducts()) {
        std::string catLower = toLowerStr(p.category);
        for (const auto& c : cats) {
            std::string cNameLower = toLowerStr(c.name);
            bool nameMatch = (cNameLower == catLower) ||
                             (cNameLower == "fruits" && catLower == "fruits & veg") ||
                             (cNameLower == "personal" && catLower == "personal care") ||
                             (!p.categoryId.empty() && c.categoryId == p.categoryId);
            if (nameMatch) {
                if (p.categoryId != c.categoryId) {
                    p.categoryId = c.categoryId;
                    changed = true;
                }
                break;
            }
        }
        if (p.categoryId.empty()) {
            p.categoryId = cats[0].categoryId;
            changed = true;
        }
    }
    if (changed) {
        mProductCatalog.saveToFile();
    }
}

BillSummary PosApp::computeCurrentBillSummary() const {
    std::vector<std::pair<std::string, std::string>> supplierMap;
    for (const auto& inv : mInventoryManager.getItems()) {
        supplierMap.push_back({inv.barcode, inv.supplier});
    }
    int seasonalPct = mSeasonManager.getActiveDiscountForDate(getCurrentDateYYYYMMDD());
    int specialPct = 0;
    try { specialPct = std::clamp(std::stoi(mSpecialDiscountInputStr), 0, 8); } catch (...) {}
    return mCartService.computeBillSummary(mProductCatalog.getProducts(), seasonalPct, specialPct, supplierMap);
}

void PosApp::finalizePayment(int paymentMethod) {
    auto& products = mProductCatalog.getProducts();
    if (mCartService.getCart().empty()) {
        showToast("Cannot charge: receipt is empty");
        return;
    }
    mCartService.setPaymentMethod(paymentMethod);
    BillSummary summary = computeCurrentBillSummary();
    std::string orderIdPreview = mIsEditingExistingOrder ? mCurrentEditingOrderId : "";

    std::string msg;
    if (mCartService.chargeSale(products, mCurrentUserName, msg, &summary, orderIdPreview)) {
        recordSaleToOutputFiles(mCartService.getCart(), mCurrentUserName, paymentMethod, "paid");
        if (!mLastSavedBillId.empty()) {
            for (const auto& b : mBills) {
                if (b.billId == mLastSavedBillId) {
                    mCartService.setLastOrderId(b.orderId);
                    break;
                }
            }
        }
        mActivityLogger.logActivity("SALE", mCurrentUserName,
            std::string(paymentMethod == 0 ? "Cash" : "Card") + " sale of " + formatRs(summary.totalPayable) +
            (mLastSavedBillId.empty() ? "" : (" (Bill #" + mLastSavedBillId + ")")));
        showToast(msg);
        mCashModalOpen = false;
        mCardModalOpen = false;
        mSpecialDiscountFocused = false;
    } else {
        showToast(msg);
    }
}

void PosApp::holdCurrentOrder() {
    auto& products = mProductCatalog.getProducts();
    if (mCartService.getCart().empty()) {
        showToast("Cart is empty - nothing to hold");
        return;
    }
    if (mIsEditingExistingOrder && mOriginalEditingOrderStatus == "paid") {
        showToast("Paid order cannot be downgraded to on-hold. Charge payment to save edits.");
        return;
    }
    std::vector<CartLine> snapshot = mCartService.getCart();
    recordSaleToOutputFiles(snapshot, mCurrentUserName, mCartService.getPaymentMethod(), "on-hold");
    std::string savedOrderId = mOrders.empty() ? "" : mOrders.back().orderId;
    if (mIsEditingExistingOrder && !mCurrentEditingOrderId.empty()) savedOrderId = mCurrentEditingOrderId;

    mCartService.parkCurrentCart(products, savedOrderId.empty()
        ? ("Customer #" + std::to_string(mCartService.getDrafts().size() + 1))
        : ("Order #" + savedOrderId));
    mIsEditingExistingOrder = false;
    mCurrentEditingOrderId = "";
    mOriginalEditingOrderItems.clear();
    mOriginalEditingOrderStatus = "";
    mReceiptScrollOffset = 0.f;
    mSpecialDiscountInputStr = "0";
    showToast("Order #" + savedOrderId + " held (saved on-hold)! Next customer ready.");
}

void PosApp::resumeLatestHeldOrder() {
    if (!mCartService.getDrafts().empty()) {
        const auto& dr = mCartService.getDrafts().back();
        std::string label = dr.customerLabel;
        int idx = (int)mCartService.getDrafts().size() - 1;
        mCartService.resumeDraft(idx);
        mReceiptScrollOffset = 0.f;
        if (label.rfind("Order #", 0) == 0 && label.size() > 7) {
            std::string oid = label.substr(7);
            reloadAuditData();
            mOriginalEditingOrderItems.clear();
            for (const auto& d : mOrderDetails) {
                if (d.orderId == oid) mOriginalEditingOrderItems.push_back(d);
            }
            mOriginalEditingOrderStatus = "on-hold";
            mIsEditingExistingOrder = true;
            mCurrentEditingOrderId = oid;
        }
        showToast("Resumed held cart (" + label + ")");
        return;
    }

    reloadAuditData();
    for (int i = (int)mOrders.size() - 1; i >= 0; --i) {
        if (mOrders[i].orderStatus == "on-hold") {
            tryOpenOrderForEditing(mOrders[i].orderId);
            return;
        }
    }
    showToast("No held orders available to resume");
}

void PosApp::recordSaleToOutputFiles(const std::vector<CartLine>& cartItems, const std::string& cashierName,
                                     int paymentMethod, const std::string& orderStatus) {
    std::vector<OrderRecord> orders;
    parseOrdersFile("Order.txt", orders);
    std::vector<ReturnOrderDetailItem> orderDetails;
    parseOrderDetailsFile("OrderDetail.txt", orderDetails);
    std::vector<BillRecord> bills;
    parseBillsFile("Bill.txt", bills);
    std::vector<BillDetailRecord> billDetails;
    parseBillDetailsFile("BillDetail.txt", billDetails);

    std::time_t now = std::time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    int dayOfMonth = tmNow.tm_mday;
    char dBuf[32], tBuf[32];
    std::strftime(dBuf, sizeof(dBuf), "%Y-%m-%d", &tmNow);
    std::strftime(tBuf, sizeof(tBuf), "%H:%M:%S", &tmNow);

    auto& products = mProductCatalog.getProducts();
    BillSummary summary = computeCurrentBillSummary();

    std::string orderId;
    std::string billId;
    bool updatingExisting = (mIsEditingExistingOrder && !mCurrentEditingOrderId.empty());

    // Build uniqueness sets
    std::unordered_map<std::string, bool> usedOrders;
    for (const auto& o : orders) usedOrders[o.orderId] = true;
    std::unordered_map<std::string, bool> usedBills;
    long long maxBillNum = 900000000LL;
    for (const auto& b : bills) {
        usedBills[b.billId] = true;
        try {
            long long v = std::stoll(b.billId);
            if (v > maxBillNum) maxBillNum = v;
        } catch (...) {}
    }

    if (updatingExisting) {
        orderId = mCurrentEditingOrderId;
        for (const auto& b : bills) {
            if (b.orderId == orderId && !b.billId.empty()) {
                billId = b.billId;
                break;
            }
        }
    }

    if (orderId.empty()) {
        int seq = (int)orders.size() + 1;
        do {
            char oBuf[32];
            sprintf_s(oBuf, "%06d%02d", seq++, dayOfMonth);
            orderId = oBuf;
        } while (usedOrders.count(orderId));
    }

    if (billId.empty()) {
        do {
            maxBillNum++;
            char bBuf[32];
            sprintf_s(bBuf, "%09lld", maxBillNum);
            billId = bBuf;
        } while (usedBills.count(billId));
    }

    // Update stock in BOTH Product.txt and inventory.txt (Req 5 + delta-safe on order edits)
    std::unordered_map<std::string, int> oldBarcodeQty;
    if (updatingExisting) {
        for (const auto& od : mOriginalEditingOrderItems) {
            oldBarcodeQty[od.productBarcode] += od.quantity;
        }
    }
    std::unordered_map<std::string, int> newBarcodeQty;
    for (const auto& item : cartItems) {
        if (item.productId >= 0 && item.productId < (int)products.size()) {
            newBarcodeQty[products[item.productId].barcode] += item.quantity;
        }
    }
    for (auto& p : products) {
        int oq = oldBarcodeQty.count(p.barcode) ? oldBarcodeQty[p.barcode] : 0;
        int nq = newBarcodeQty.count(p.barcode) ? newBarcodeQty[p.barcode] : 0;
        int delta = nq - oq;
        if (delta != 0) {
            p.stock = std::max(0, p.stock - delta);
            mInventoryManager.updateItemStock(p.barcode, p.stock);
        }
    }
    mProductCatalog.saveToFile();

    std::string custName = cashierName.empty() ? "Store_Customer" : cashierName;
    for (char& c : custName) if (c == ' ') c = '_';
    std::string custPhone = "03001234567";
    std::string pMode = (paymentMethod == 0 ? "cash" : "card");
    std::string pMerchant = (orderStatus == "on-hold" ? "OnHold" : (paymentMethod == 0 ? "CashCounter" : "BankPOS"));

    OrderRecord newOrd;
    newOrd.orderId = orderId;
    newOrd.orderDate = dBuf;
    newOrd.orderTime = tBuf;
    newOrd.customerName = custName;
    newOrd.customerContact = custPhone;
    newOrd.orderStatus = orderStatus;
    newOrd.paymentAmount = summary.totalPayable;
    newOrd.paymentMode = pMode;
    newOrd.paymentMerchant = pMerchant;

    if (updatingExisting) {
        bool replaced = false;
        for (auto& o : orders) {
            if (o.orderId == orderId) {
                o = newOrd;
                replaced = true;
                break;
            }
        }
        if (!replaced) orders.push_back(newOrd);

        std::vector<ReturnOrderDetailItem> keptDetails;
        for (const auto& d : orderDetails) {
            if (d.orderId != orderId) keptDetails.push_back(d);
        }
        for (const auto& item : cartItems) {
            if (item.productId >= 0 && item.productId < (int)products.size()) {
                ReturnOrderDetailItem nd;
                nd.orderId = orderId;
                nd.productId = products[item.productId].id;
                nd.productBarcode = products[item.productId].barcode;
                nd.quantity = item.quantity;
                keptDetails.push_back(nd);
            }
        }
        std::ofstream outOrd("Order.txt");
        if (outOrd.is_open()) {
            for (const auto& o : orders) {
                outOrd << o.orderId << " " << o.orderDate << " " << o.orderTime << " "
                       << o.customerName << " " << o.customerContact << " " << o.orderStatus << " "
                       << o.paymentAmount << " " << o.paymentMode << " " << o.paymentMerchant << " #\n";
            }
            outOrd.close();
        }
        std::ofstream outDet("OrderDetail.txt");
        if (outDet.is_open()) {
            for (const auto& d : keptDetails) {
                outDet << d.orderId << " " << d.productId << " " << d.productBarcode << " " << d.quantity << " #\n";
            }
            outDet.close();
        }
    } else {
        std::ofstream outOrd("Order.txt", std::ios::app);
        if (outOrd.is_open()) {
            outOrd << newOrd.orderId << " " << newOrd.orderDate << " " << newOrd.orderTime << " "
                   << newOrd.customerName << " " << newOrd.customerContact << " " << newOrd.orderStatus << " "
                   << newOrd.paymentAmount << " " << newOrd.paymentMode << " " << newOrd.paymentMerchant << " #\n";
            outOrd.close();
        }
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
    }

    int amountReceived = 0;
    int balanceReturned = 0;
    if (orderStatus == "paid") {
        amountReceived = (paymentMethod == 0) ? mCartService.parseCashGiven() : summary.totalPayable;
        if (amountReceived < summary.totalPayable) amountReceived = summary.totalPayable;
        balanceReturned = std::max(0, amountReceived - summary.totalPayable);
    }

    BillRecord bill;
    bill.billId = billId;
    bill.customerName = custName;
    bill.orderId = orderId;
    bill.totalWithoutTax = summary.subtotalWithoutTax;
    bill.totalTax = summary.totalTax;
    bill.totalDiscount = summary.totalApplicableDiscount;
    bill.totalPayable = summary.totalPayable;
    bill.amountReceived = amountReceived;
    bill.balanceReturned = balanceReturned;
    bill.paymentStatus = orderStatus;
    bill.seasonalDiscount = summary.seasonalDiscount;
    bill.specialDiscount = summary.specialDiscount;
    bill.billDate = dBuf;
    bill.billTime = tBuf;

    mLastSavedBillId = billId;

    if (updatingExisting) {
        bool bReplaced = false;
        for (auto& b : bills) {
            if (b.billId == billId || b.orderId == orderId) {
                b = bill;
                bReplaced = true;
                break;
            }
        }
        if (!bReplaced) bills.push_back(bill);

        std::vector<BillDetailRecord> keptBD;
        for (const auto& bd : billDetails) {
            if (bd.billId != billId) keptBD.push_back(bd);
        }
        for (const auto& item : cartItems) {
            if (item.productId >= 0 && item.productId < (int)products.size()) {
                const auto& p = products[item.productId];
                std::string acr = p.acronym.empty() ? p.name.substr(0, std::min<size_t>(8, p.name.size())) : p.acronym;
                for (char& ch : acr) if (ch == ' ') ch = '_';
                BillDetailRecord nd;
                nd.billId = billId;
                nd.productAcronym = acr;
                nd.unitPrice = p.price;
                nd.quantity = item.quantity;
                nd.productDiscount = p.discount * item.quantity;
                keptBD.push_back(nd);
            }
        }
        std::ofstream outBill("Bill.txt");
        if (outBill.is_open()) {
            for (const auto& b : bills) {
                if (!b.productAcronym.empty()) {
                    // Legacy 12-field line preserved safely
                    outBill << b.billId << " " << b.customerName << " " << b.orderId << " "
                            << b.productAcronym << " " << b.unitPrice << " " << b.discountPerUnit << " "
                            << b.quantity << " " << b.totalWithoutTax << " " << b.totalTax << " "
                            << b.totalDiscount << " " << b.totalPayable << " " << b.paymentStatus << " #\n";
                } else {
                    outBill << b.billId << " " << b.customerName << " " << b.orderId << " "
                            << b.totalWithoutTax << " " << b.totalTax << " " << b.totalDiscount << " "
                            << b.totalPayable << " " << b.amountReceived << " " << b.balanceReturned << " "
                            << b.paymentStatus << " " << b.seasonalDiscount << " " << b.specialDiscount << " "
                            << b.billDate << " " << b.billTime << " #\n";
                }
            }
            outBill.close();
        }
        std::ofstream outBD("BillDetail.txt");
        if (outBD.is_open()) {
            for (const auto& bd : keptBD) {
                outBD << bd.billId << " " << bd.productAcronym << " "
                      << bd.unitPrice << " " << bd.quantity << " " << bd.productDiscount << " #\n";
            }
            outBD.close();
        }
    } else {
        std::ofstream outBill("Bill.txt", std::ios::app);
        if (outBill.is_open()) {
            outBill << bill.billId << " " << bill.customerName << " " << bill.orderId << " "
                    << bill.totalWithoutTax << " " << bill.totalTax << " " << bill.totalDiscount << " "
                    << bill.totalPayable << " " << bill.amountReceived << " " << bill.balanceReturned << " "
                    << bill.paymentStatus << " " << bill.seasonalDiscount << " " << bill.specialDiscount << " "
                    << bill.billDate << " " << bill.billTime << " #\n";
            outBill.close();
        }
        std::ofstream outBillDet("BillDetail.txt", std::ios::app);
        if (outBillDet.is_open()) {
            for (const auto& item : cartItems) {
                if (item.productId >= 0 && item.productId < (int)products.size()) {
                    const auto& p = products[item.productId];
                    std::string acr = p.acronym.empty() ? p.name.substr(0, std::min<size_t>(8, p.name.size())) : p.acronym;
                    for (char& ch : acr) if (ch == ' ') ch = '_';
                    outBillDet << billId << " " << acr << " "
                               << p.price << " " << item.quantity << " "
                               << (p.discount * item.quantity) << " #\n";
                }
            }
            outBillDet.close();
        }
    }

    if (updatingExisting && orderStatus == "paid") {
        mIsEditingExistingOrder = false;
        mCurrentEditingOrderId = "";
        mOriginalEditingOrderItems.clear();
        mOriginalEditingOrderStatus = "";
        mSpecialDiscountInputStr = "0";
    }

    reloadAuditData();
}

bool PosApp::loadReturnsFile(const std::string& filename) {
    mReturnHistory.clear();
    std::vector<std::string> searchPaths = {filename, "frontend/" + filename, "../" + filename};
    std::string foundPath = "";
    for (const auto& p : searchPaths) {
        if (std::filesystem::exists(p)) {
            foundPath = p;
            break;
        }
    }

    if (foundPath.empty()) {
        saveReturnsFile(filename);
        return true;
    }

    std::ifstream in(foundPath);
    if (!in.is_open()) return false;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string part;
        std::vector<std::string> parts;
        while (std::getline(ss, part, '|')) {
            parts.push_back(part);
        }
        if (parts.size() >= 8) {
            ReturnRecord rec;
            rec.returnId = parts[0];
            rec.timestamp = parts[1];
            rec.cashier = parts[2];
            rec.barcode = parts[3];
            rec.itemName = parts[4];
            rec.quantity = std::atoi(parts[5].c_str());
            rec.refundAmount = std::atoi(parts[6].c_str());
            rec.orderRef = parts[7];
            if (parts.size() >= 9 && !parts[8].empty()) rec.reason = parts[8];
            if (parts.size() >= 10 && !parts[9].empty()) rec.refundMethod = parts[9];
            if (parts.size() >= 11 && !parts[10].empty()) rec.customer = parts[10];
            if (parts.size() >= 12 && !parts[11].empty()) rec.condition = parts[11];
            mReturnHistory.push_back(rec);
        }
    }
    in.close();
    return true;
}

bool PosApp::saveReturnsFile(const std::string& filename) {
    std::vector<std::string> writePaths = {filename, "frontend/" + filename};
    for (const auto& p : writePaths) {
        std::ofstream out(p);
        if (out.is_open()) {
            out << "# ReturnID|Timestamp|Cashier|Barcode|ItemName|Quantity|RefundAmount|OrderRef|Reason|RefundMethod|Customer|Condition\n";
            for (const auto& rec : mReturnHistory) {
                out << rec.returnId << "|" << rec.timestamp << "|" << rec.cashier << "|"
                    << rec.barcode << "|" << rec.itemName << "|" << rec.quantity << "|"
                    << rec.refundAmount << "|" << rec.orderRef << "|" << rec.reason << "|"
                    << rec.refundMethod << "|" << rec.customer << "|" << rec.condition << "\n";
            }
            out.close();
        }
    }
    return true;
}

bool PosApp::appendReturnRecord(const ReturnRecord& rec) {
    mReturnHistory.push_back(rec);
    std::vector<std::string> writePaths = {"returns.txt", "frontend/returns.txt"};
    for (const auto& p : writePaths) {
        std::ofstream out(p, std::ios::app);
        if (out.is_open()) {
            out << rec.returnId << "|" << rec.timestamp << "|" << rec.cashier << "|"
                << rec.barcode << "|" << rec.itemName << "|" << rec.quantity << "|"
                << rec.refundAmount << "|" << rec.orderRef << "|" << rec.reason << "|"
                << rec.refundMethod << "|" << rec.customer << "|" << rec.condition << "\n";
            out.close();
        }
    }
    return true;
}

void PosApp::generateNewItemBarcode() {
    std::string catId = "01";
    const auto* cat = mCategoryManager.findByName(mNewItemCategory);
    if (cat) catId = cat->categoryId;
    else if (!mCategoryManager.getCategories().empty()) catId = mCategoryManager.getCategories()[0].categoryId;

    std::vector<std::string> extraBarcodes;
    for (const auto& p : mProductCatalog.getProducts()) {
        extraBarcodes.push_back(p.barcode);
    }

    std::string newBc = mInventoryManager.generateUniqueBarcode(catId, extraBarcodes);
    mNewItemBarcode = newBc;
    mNewItemGenerated = true;

    // Generate physical BMP file
    std::string outPath = "barcodes/barcode_" + newBc + ".bmp";
    generateBarcodeBMP(newBc, outPath);

    // Also mirror to assets
    std::string assetPath = "assets/barcodes/barcode_" + newBc + ".bmp";
    generateBarcodeBMP(newBc, assetPath);

    showToast("Generated Barcode: " + newBc + " (Cat " + catId + ")");
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

    if (mNewItemAlsoAddToCatalog) {
        const auto* cat = mCategoryManager.findByName(mNewItemCategory);
        std::string catId = cat ? cat->categoryId : (mNewItemBarcode.size() >= 2 ? mNewItemBarcode.substr(0, 2) : "01");
        ProductItem prod;
        prod.barcode = mNewItemBarcode;
        prod.name = mNewItemDesc;
        prod.category = mNewItemCategory;
        prod.categoryId = catId;
        prod.unit = mNewItemUnit.empty() ? "1 pc" : mNewItemUnit;
        prod.price = retPrice;
        prod.unitPrice = retPrice;
        prod.tax = std::max(0, retPrice / 20);
        prod.discount = 0;
        prod.stock = qty;
        prod.barcodePath = bmpPath;
        prod.iconKey = mInventoryManager.findByBarcode(mNewItemBarcode) ? "rice" : "rice";
        std::string cLower = toLowerStr(mNewItemCategory);
        if (cLower.find("dairy") != std::string::npos) prod.iconKey = "milk";
        else if (cLower.find("bake") != std::string::npos || cLower.find("bread") != std::string::npos) prod.iconKey = "bread";
        else if (cLower.find("drink") != std::string::npos) prod.iconKey = "juice";
        else if (cLower.find("fruit") != std::string::npos) prod.iconKey = "apples";
        else if (cLower.find("veg") != std::string::npos) prod.iconKey = "rice";
        else if (cLower.find("snack") != std::string::npos) prod.iconKey = "olive";
        else if (cLower.find("house") != std::string::npos) prod.iconKey = "olive";
        else if (cLower.find("pers") != std::string::npos) prod.iconKey = "oil";

        int existingIdx = mProductCatalog.findProductByBarcode(mNewItemBarcode);
        if (existingIdx >= 0) {
            mProductCatalog.addStock(existingIdx, qty);
        } else {
            mProductCatalog.addOrUpdateProduct(prod);
        }
        mProductCatalog.saveToFile();
    }

    mActivityLogger.logActivity("INVENTORY", mCurrentUserName,
        "Added " + std::to_string(qty) + " units of " + mNewItemDesc + " (" + mNewItemBarcode +
        ") from " + mNewItemSupplier + (mNewItemAlsoAddToCatalog ? " into inventory.txt & Product.txt" : " into inventory.txt"));

    mAddInventoryModalOpen = false;
    showToast(mNewItemAlsoAddToCatalog
        ? ("Added " + mNewItemDesc + " to Inventory & Product.txt!")
        : ("Added " + mNewItemDesc + " to inventory.txt!"));
}

void PosApp::printBillPdf() {
    reloadAuditData();
    if (mBills.empty()) {
        showToast("No bills recorded yet to print");
        return;
    }
    const BillRecord* billToPrint = nullptr;
    if (!mLastSavedBillId.empty()) {
        for (const auto& b : mBills) {
            if (b.billId == mLastSavedBillId) {
                billToPrint = &b;
                break;
            }
        }
    }
    if (!billToPrint) {
        billToPrint = &mBills.back();
    }

    std::vector<BillDetailRecord> details;
    for (const auto& d : mBillDetails) {
        if (d.billId == billToPrint->billId) {
            details.push_back(d);
        }
    }

    std::filesystem::create_directories("receipts");
    std::string outPdf = "receipts/receipt_" + billToPrint->orderId + ".pdf";
    if (PdfGenerator::generateReceiptPdf(outPdf, *billToPrint, details, mCurrentUserName)) {
        showToast("Printed PDF: " + outPdf);
        PdfGenerator::openPdfInDefaultViewer(outPdf);
    } else {
        showToast("Failed to generate PDF bill");
    }
}

void PosApp::openEditProductModal(int productIdx) {
    const auto& prods = mProductCatalog.getProducts();
    if (productIdx < 0 || productIdx >= (int)prods.size()) return;
    mEditingProductIdx = productIdx;
    const auto& p = prods[productIdx];
    mEditProductName = p.name;
    mEditProductPriceStr = std::to_string(p.price);
    mEditProductUnit = p.unit;
    mEditProductStockStr = std::to_string(p.stock);
    mEditProductCategory = p.category;
    mEditProductCatId = p.categoryId.empty() ? "01" : p.categoryId;
    mEditProductDiscountStr = std::to_string(p.discount);
    mEditProductActiveField = 0;
    mEditProductModalOpen = true;
}

void PosApp::saveEditedProduct() {
    auto& prods = mProductCatalog.getProducts();
    if (mEditingProductIdx < 0 || mEditingProductIdx >= (int)prods.size()) return;

    ProductItem p = prods[mEditingProductIdx];
    p.name = trimStr(mEditProductName);
    if (p.name.empty()) p.name = prods[mEditingProductIdx].name;
    try { p.price = std::max(1, std::stoi(mEditProductPriceStr)); p.unitPrice = p.price; } catch (...) {}
    p.unit = mEditProductUnit.empty() ? prods[mEditingProductIdx].unit : mEditProductUnit;
    try { p.stock = std::max(0, std::stoi(mEditProductStockStr)); } catch (...) {}
    try { p.discount = std::max(0, std::stoi(mEditProductDiscountStr)); } catch (...) {}
    p.category = mEditProductCategory;
    p.categoryId = mEditProductCatId;

    mProductCatalog.updateProduct(mEditingProductIdx, p);
    mProductCatalog.saveToFile();
    mInventoryManager.updateItemFromProduct(p);

    mActivityLogger.logActivity("PRODUCT", mCurrentUserName, "Edited product: " + p.name + " (" + p.barcode + ")");
    mEditProductModalOpen = false;
    showToast("Product updated: " + p.name);
}

void PosApp::openEditOrderModal() {
    reloadAuditData();
    mEditOrderSearchId = "";
    mEditOrderStatusMsg = "";
    mEditOrderSearchFocused = true;
    mEditOrderScrollOffset = 0.f;
    mEditOrderModalOpen = true;
}

void PosApp::tryOpenOrderForEditing(const std::string& orderId) {
    reloadAuditData();
    const OrderRecord* targetOrder = nullptr;
    for (const auto& ord : mOrders) {
        if (ord.orderId == orderId) {
            targetOrder = &ord;
            break;
        }
    }

    if (!targetOrder) {
        mEditOrderStatusMsg = "Order #" + orderId + " not found!";
        showToast(mEditOrderStatusMsg);
        return;
    }

    // Check if Paid status (Requirement 3: Paid order editable only within 7 days and after Manager Approval)
    if (targetOrder->orderStatus == "paid") {
        if (targetOrder->orderDate.length() >= 10) {
            std::tm tOrder = {};
            tOrder.tm_isdst = -1;
            try {
                tOrder.tm_year = std::stoi(targetOrder->orderDate.substr(0, 4)) - 1900;
                tOrder.tm_mon  = std::stoi(targetOrder->orderDate.substr(5, 2)) - 1;
                tOrder.tm_mday = std::stoi(targetOrder->orderDate.substr(8, 2));
                if (targetOrder->orderTime.length() >= 8) {
                    tOrder.tm_hour = std::stoi(targetOrder->orderTime.substr(0, 2));
                    tOrder.tm_min  = std::stoi(targetOrder->orderTime.substr(3, 2));
                    tOrder.tm_sec  = std::stoi(targetOrder->orderTime.substr(6, 2));
                }
            } catch (...) {}
            std::time_t now = std::time(nullptr);
            std::time_t oTime = std::mktime(&tOrder);
            if (oTime != (std::time_t)-1) {
                double diffDays = std::difftime(now, oTime) / 86400.0;
                if (diffDays > 7.0) {
                    mEditOrderStatusMsg = "Order paid > 7 days ago (" + targetOrder->orderDate + "). Editing expired.";
                    showToast(mEditOrderStatusMsg);
                    return;
                }
            }
        }

        // If Cashier role, require manager authorization
        if (mCurrentRole != "Admin") {
            mApprovalTargetOrderId = targetOrder->orderId;
            mApprovalTargetOrderDate = targetOrder->orderDate;
            mApprovalPassInput = "";
            mApprovalErrorMsg = "";
            mEditOrderModalOpen = false;
            mManagerApprovalModalOpen = true;
            return;
        }
    }

    // Direct approval for Admin or on-hold/in-progress orders
    mApprovalTargetOrderId = targetOrder->orderId;
    approveAndLoadOrder();
}

void PosApp::approveAndLoadOrder() {
    reloadAuditData();
    std::string oId = mApprovalTargetOrderId;
    if (oId.empty()) return;

    // Track original items & status so saving adjusts inventory by exact net delta without double-deducting
    mOriginalEditingOrderItems.clear();
    mOriginalEditingOrderStatus = "paid";
    for (const auto& ord : mOrders) {
        if (ord.orderId == oId) {
            mOriginalEditingOrderStatus = ord.orderStatus;
            break;
        }
    }

    // Load order line items into cart
    mCartService.clear();

    for (const auto& d : mOrderDetails) {
        if (d.orderId == oId) {
            mOriginalEditingOrderItems.push_back(d);
            int pIdx = mProductCatalog.findProductByBarcode(d.productBarcode);
            if (pIdx >= 0) {
                mCartService.addItem(pIdx, d.quantity);
            }
        }
    }

    mIsEditingExistingOrder = true;
    mCurrentEditingOrderId = oId;
    mEditOrderModalOpen = false;
    mManagerApprovalModalOpen = false;
    mActiveTab = 0; // Return to Register view
    showToast("Editing " + mOriginalEditingOrderStatus + " order #" + oId + ". Make adjustments and charge/print.");
}

void PosApp::openAddCategoryModal() {
    mNewCatId = mCategoryManager.nextCategoryId();
    mNewCatName = "";
    mNewCatAcr = "";
    mNewCatActiveField = 1;
    mAddCategoryModalOpen = true;
}

void PosApp::saveNewCategory() {
    if (mNewCatName.empty()) {
        showToast("Please enter a category name");
        return;
    }
    if (mNewCatId.empty()) {
        mNewCatId = mCategoryManager.nextCategoryId();
    }
    if (mNewCatAcr.empty()) {
        mNewCatAcr = mNewCatName.substr(0, std::min((size_t)4, mNewCatName.length()));
    }

    ProductCategory cat;
    cat.categoryId = mNewCatId;
    cat.name = mNewCatName;
    cat.acronym = mNewCatAcr;

    if (mCategoryManager.addCategory(cat)) {
        syncProductCategoriesFromManager();
        mActivityLogger.logActivity("PRODUCT", mCurrentUserName, "Added category: " + cat.name + " (" + cat.categoryId + ")");
        showToast("Category " + cat.name + " (" + cat.categoryId + ") added to PCategory.txt!");
        mAddCategoryModalOpen = false;
    } else {
        showToast("Failed to save category");
    }
}

void PosApp::openAddSeasonModal() {
    mNewSeasonStart = "2026-11-23";
    mNewSeasonEnd = "2026-11-30";
    mNewSeasonPctStr = "10";
    mNewSeasonActiveField = 0;
    mAddSeasonModalOpen = true;
}

void PosApp::saveNewSeason() {
    int pct = 10;
    try { pct = std::stoi(mNewSeasonPctStr); } catch (...) {}
    if (pct <= 0 || pct > 80) {
        showToast("Invalid discount percentage (1-80%)");
        return;
    }

    Season s;
    s.startDate = mNewSeasonStart;
    s.endDate = mNewSeasonEnd;
    s.discountPercent = pct;
    s.isActive = true;

    if (mSeasonManager.addSeason(s)) {
        mActivityLogger.logActivity("PRODUCT", mCurrentUserName, "Added season: " + s.startDate + " to " + s.endDate + " (" + std::to_string(pct) + "%)");
        showToast("Seasonal discount added to Seasons.txt!");
        mAddSeasonModalOpen = false;
    } else {
        showToast("Failed to save seasonal discount");
    }
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

    if (mEditOrderModalOpen) {
        float maxScroll = std::max(0.f, (float)(mOrders.size() - 6) * 32.f);
        mEditOrderScrollOffset = std::clamp(mEditOrderScrollOffset - delta * 30.f, 0.f, maxScroll);
        return;
    }

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
        float maxScroll = std::max(0.f, (float)(mOrders.size() + 10) * 44.f);
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
        float rw = 440.f, rh = 618.f;
        float rx = (mCurrentWidth - rw) / 2.f;
        float ry = (mDesignHeight - (rh + 48.f)) / 2.f;
        float btnY = ry + rh + 10.f;

        // Close Button
        if (pointInRect(m, rx, btnY, 135.f, 38.f)) {
            mReceiptModalOpen = false;
            return;
        }
        // Print PDF Button (Requirement 7)
        if (pointInRect(m, rx + 145.f, btnY, 145.f, 38.f)) {
            printBillPdf();
            return;
        }
        // Start New Sale Button
        if (pointInRect(m, rx + 300.f, btnY, 140.f, 38.f)) {
            mReceiptModalOpen = false;
            mCartService.clear();
            mIsEditingExistingOrder = false;
            mCurrentEditingOrderId = "";
            mOriginalEditingOrderItems.clear();
            mOriginalEditingOrderStatus = "";
            mSpecialDiscountInputStr = "0";
            showToast("Ready for new sale transaction");
            return;
        }
        // Click outside paper and buttons closes modal
        if (!pointInRect(m, rx - 12.f, ry - 12.f, rw + 24.f, rh + 68.f)) {
            mReceiptModalOpen = false;
        }
        return;
    }

    // Edit Product Modal
    if (mEditProductModalOpen) {
        float dw = 540.f, dh = 500.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 42, dy + 18, 26, 26)) {
            mEditProductModalOpen = false;
            return;
        }

        if (pointInRect(m, dx + 165, dy + 72.f, 345.f, 32)) { mEditProductActiveField = 0; return; }
        else if (pointInRect(m, dx + 165, dy + 112.f, 180.f, 32)) { mEditProductActiveField = 1; return; }
        else if (pointInRect(m, dx + 165, dy + 152.f, 180.f, 32)) { mEditProductActiveField = 2; return; }
        else if (pointInRect(m, dx + 165, dy + 192.f, 180.f, 32)) { mEditProductActiveField = 3; return; }
        else if (pointInRect(m, dx + 165, dy + 232.f, 180.f, 32)) { mEditProductActiveField = 4; return; }

        // Category pills (up to 10 in 2 rows of 5)
        float pillY0 = dy + 312.f;
        float pw = 91.f;
        const auto& cats = mCategoryManager.getCategories();
        for (size_t ci = 0; ci < cats.size() && ci < 10; ci++) {
            float px = dx + 30.f + static_cast<float>(ci % 5) * (pw + 6.f);
            float pillY = pillY0 + static_cast<float>(ci / 5) * 28.f;
            if (pointInRect(m, px, pillY, pw, 22.f)) {
                mEditProductCatId = cats[ci].categoryId;
                mEditProductCategory = cats[ci].name;
                return;
            }
        }

        // Save
        float btnY = dy + dh - 56.f;
        if (pointInRect(m, dx + 30, btnY, 220, 40)) {
            saveEditedProduct();
            return;
        }
        // Cancel
        if (pointInRect(m, dx + dw - 150, btnY, 120, 40) || !pointInRect(m, dx, dy, dw, dh)) {
            mEditProductModalOpen = false;
            return;
        }
        return;
    }

    // Edit Order Modal
    if (mEditOrderModalOpen) {
        float dw = 680.f, dh = 520.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;
        float inY = dy + 68.f;

        if (pointInRect(m, dx + dw - 42, dy + 18, 26, 26)) {
            mEditOrderModalOpen = false;
            return;
        }

        if (pointInRect(m, dx + 95, inY, 260, 32)) {
            mEditOrderSearchFocused = true;
            return;
        } else {
            mEditOrderSearchFocused = false;
        }

        if (pointInRect(m, dx + 365, inY, 110, 32)) {
            tryOpenOrderForEditing(mEditOrderSearchId);
            return;
        }

        // Table rows (matching filter cleanly)
        float ty = inY + 72.f;
        int visIdx = 0;
        for (int i = (int)mOrders.size() - 1; i >= 0; i--) {
            const auto& o = mOrders[i];
            if (!mEditOrderSearchId.empty() && o.orderId.find(mEditOrderSearchId) == std::string::npos) continue;
            float rowY = ty + 36.f + static_cast<float>(visIdx) * 32.f - mEditOrderScrollOffset;
            visIdx++;
            if (rowY < ty + 30.f || rowY > ty + 245.f) continue;
            if (pointInRect(m, dx + dw - 105, rowY, 60, 20)) {
                tryOpenOrderForEditing(o.orderId);
                return;
            }
        }

        // Close
        float botY = dy + dh - 48.f;
        if (pointInRect(m, dx + dw - 150, botY, 120, 34) || !pointInRect(m, dx, dy, dw, dh)) {
            mEditOrderModalOpen = false;
            return;
        }
        return;
    }

    // Manager Approval Modal
    if (mManagerApprovalModalOpen) {
        float dw = 460.f, dh = 300.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;
        float btnY = dy + dh - 56.f;

        if (pointInRect(m, dx + dw - 42, dy + 18, 26, 26)) {
            mManagerApprovalModalOpen = false;
            return;
        }

        if (pointInRect(m, dx + 30, dy + 118, dw - 60, 36)) {
            return;
        }

        // Authorize button - check against ALL Admin accounts in staff.txt
        if (pointInRect(m, dx + 30, btnY, 220, 38)) {
            bool ok = false;
            for (const auto& s : mStaffManager.getStaffList()) {
                if (s.role == "Admin" && mStaffManager.authenticate(s.name, mApprovalPassInput, "Admin")) {
                    ok = true;
                    break;
                }
            }
            if (ok) {
                approveAndLoadOrder();
            } else {
                mApprovalErrorMsg = "Invalid Manager PIN or authorization credentials.";
            }
            return;
        }

        // Cancel
        if (pointInRect(m, dx + dw - 150, btnY, 120, 38) || !pointInRect(m, dx, dy, dw, dh)) {
            mManagerApprovalModalOpen = false;
            return;
        }
        return;
    }

    // Add Category Modal
    if (mAddCategoryModalOpen) {
        float dw = 460.f, dh = 320.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 42, dy + 18, 26, 26)) {
            mAddCategoryModalOpen = false;
            return;
        }

        if (pointInRect(m, dx + 180, dy + 80.f, 220, 30)) mNewCatActiveField = 0;
        else if (pointInRect(m, dx + 180, dy + 120.f, 220, 30)) mNewCatActiveField = 1;
        else if (pointInRect(m, dx + 180, dy + 160.f, 220, 30)) mNewCatActiveField = 2;

        float btnY = dy + dh - 52.f;
        if (pointInRect(m, dx + 30, btnY, 200, 36)) {
            saveNewCategory();
            return;
        }
        if (pointInRect(m, dx + dw - 140, btnY, 110, 36) || !pointInRect(m, dx, dy, dw, dh)) {
            mAddCategoryModalOpen = false;
            return;
        }
        return;
    }

    // Add Season Modal
    if (mAddSeasonModalOpen) {
        float dw = 460.f, dh = 320.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 42, dy + 18, 26, 26)) {
            mAddSeasonModalOpen = false;
            return;
        }

        if (pointInRect(m, dx + 190, dy + 80.f, 210, 30)) mNewSeasonActiveField = 0;
        else if (pointInRect(m, dx + 190, dy + 120.f, 210, 30)) mNewSeasonActiveField = 1;
        else if (pointInRect(m, dx + 190, dy + 160.f, 210, 30)) mNewSeasonActiveField = 2;

        float btnY = dy + dh - 52.f;
        if (pointInRect(m, dx + 30, btnY, 200, 36)) {
            saveNewSeason();
            return;
        }
        if (pointInRect(m, dx + dw - 140, btnY, 110, 36) || !pointInRect(m, dx, dy, dw, dh)) {
            mAddSeasonModalOpen = false;
            return;
        }
        return;
    }

    // Discount Modal (Student / Employee / Promo)
    if (mDiscountModalOpen) {
        float dw = 620.f, dh = 520.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        // Close button [X]
        if (pointInRect(m, dx + dw - 44, dy + 20, 28, 28)) {
            mDiscountModalOpen = false;
            return;
        }

        // 1. Student Discount Button
        float c1Y = dy + 74.f;
        float btn1X = dx + dw - 24 - 170.f;
        float btn1Y = c1Y + 20.f;
        if (pointInRect(m, btn1X, btn1Y, 170, 36)) {
            mCartService.applyStudentDiscount();
            mDiscountModalOpen = false;
            showToast("Student Discount (" + std::to_string(mCartService.getStudentDiscountRate()) + "%) applied!");
            return;
        }

        // 2. Employee Discount
        float c2Y = dy + 158.f;
        float fUserX = dx + 38.f;
        float fUserY = c2Y + 62.f;
        if (pointInRect(m, fUserX, fUserY, 160, 32)) {
            mDiscountActiveField = 1;
            return;
        }
        float fPassX = fUserX + 170.f;
        float fPassY = c2Y + 62.f;
        if (pointInRect(m, fPassX, fPassY, 150, 32)) {
            mDiscountActiveField = 2;
            return;
        }
        float btn2X = fPassX + 160.f;
        float btn2Y = c2Y + 62.f;
        if (pointInRect(m, btn2X, btn2Y, dw - 48 - (btn2X - (dx + 24)) - 14, 32)) {
            if (mStaffManager.authenticate(mEmployeeDiscountUser, mEmployeeDiscountPass, "Employee") ||
                mStaffManager.authenticate(mEmployeeDiscountUser, mEmployeeDiscountPass, "Admin")) {
                mCartService.applyEmployeeDiscount(mEmployeeDiscountUser);
                mDiscountModalOpen = false;
                showToast("Employee discount applied for " + mEmployeeDiscountUser + " (" + std::to_string(mCartService.getEmployeeDiscountRate()) + "%)");
            } else {
                mDiscountModalError = "Invalid employee credentials! Check username & PIN.";
                showToast("Error: Employee verification failed.");
            }
            return;
        }

        // 3. Promo Code
        float c3Y = dy + 288.f;
        float pInX = dx + 38.f;
        float pInY = c3Y + 58.f;
        if (pointInRect(m, pInX, pInY, 320, 34)) {
            mDiscountActiveField = 3;
            return;
        }
        float btn3X = pInX + 330.f;
        float btn3Y = pInY;
        if (pointInRect(m, btn3X, btn3Y, dw - 48 - (btn3X - (dx + 24)) - 14, 34)) {
            std::string msg;
            if (mCartService.applyPromoCode(mPromoCodeInput, msg)) {
                mDiscountModalOpen = false;
                showToast(msg);
            } else {
                mDiscountModalError = msg;
                showToast(msg);
            }
            return;
        }

        // Bottom buttons
        float botY = dy + dh - 48.f;
        if (mCartService.isDiscountApplied() && pointInRect(m, dx + 24, botY, 190, 34)) {
            mCartService.removeDiscount();
            mDiscountModalOpen = false;
            showToast("Discount removed");
            return;
        }
        if (pointInRect(m, dx + dw - 144, botY, 120, 34)) {
            mDiscountModalOpen = false;
            return;
        }

        if (!pointInRect(m, dx, dy, dw, dh)) {
            mDiscountModalOpen = false;
        }
        return;
    }

    if (mReturnSlipModalOpen) {
        float mw = 440.f, mh = 510.f;
        float mx = (mCurrentWidth - mw) / 2.f, my = (mDesignHeight - mh) / 2.f;
        float btnW = 120.f, btnH = 30.f;
        float btnX = mx + (mw - btnW) / 2.f;
        float btnY = my + mh - 42.f;
        if (pointInRect(m, btnX, btnY, btnW, btnH) || !pointInRect(m, mx, my, mw, mh)) {
            mReturnSlipModalOpen = false;
        }
        return;
    }

    // Quick Check Overlay (ISE-56 Standalone Scanner Mode)
    if (mQuickCheckModalOpen) {
        float dw = 840.f, dh = 500.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;
        float scanW = 320.f;
        float rightX = dx + 24.f + scanW + 24.f;
        float rightW = dw - (scanW + 48.f) - 24.f;

        // Close button [X]
        if (pointInRect(m, dx + dw - 44, dy + 20, 28, 28)) {
            mQuickCheckModalOpen = false;
            return;
        }

        // Demo quick test pills
        const char* demoCodes[] = {"010001", "020002", "060004", "090006", "090007"};
        float pillStartX = rightX + 62.f;
        float qPillW = (rightW - 62.f - 16.f) / 5.f;
        for (int i = 0; i < 5; ++i) {
            float cx = pillStartX + i * (qPillW + 4.f);
            if (pointInRect(m, cx, dy + 110, qPillW, 20)) {
                mQuickCheckBarcode = demoCodes[i];
                mQuickCheckProductIdx = mProductCatalog.findProductByBarcode(mQuickCheckBarcode);
                if (mQuickCheckProductIdx < 0) {
                    mQuickCheckProductIdx = mProductCatalog.findProductByName(mQuickCheckBarcode);
                }
                if (mQuickCheckProductIdx >= 0) {
                    showToast("Quick Check: " + mProductCatalog.getProducts()[mQuickCheckProductIdx].name);
                }
                return;
            }
        }

        // Search / Barcode Input Box
        if (pointInRect(m, rightX, dy + 68, rightW, 36)) {
            mQuickCheckInputFocused = true;
            return;
        }

        // Bottom Action Buttons
        float btnY = dy + dh - 54.f;
        // Close (ESC)
        if (pointInRect(m, rightX, btnY, 130, 36)) {
            mQuickCheckModalOpen = false;
            return;
        }

        // + Add to Active Cart (safe non-intrusive addition to existing cart)
        if (mQuickCheckProductIdx >= 0) {
            float addW = rightW - 145.f;
            if (pointInRect(m, rightX + 145.f, btnY, addW, 36)) {
                mCartService.addItem(mQuickCheckProductIdx, 1);
                showToast("Added " + mProductCatalog.getProducts()[mQuickCheckProductIdx].name + " to cart");
                mQuickCheckModalOpen = false;
                return;
            }
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
        float dw = 540.f, dh = 400.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 40, dy + 18, 26, 26)) {
            mItemDescModalOpen = false;
            return;
        }
        if (pointInRect(m, dx + dw / 2.f - 70, dy + dh - 48, 140, 34)) {
            mItemDescModalOpen = false;
            return;
        }
        if (!pointInRect(m, dx, dy, dw, dh)) {
            mItemDescModalOpen = false;
        }
        return;
    }

    // Sale Detail Audit Modal
    if (mSaleDetailModalOpen) {
        float dw = 620.f, dh = 520.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 40, dy + 16, 26, 26)) {
            mSaleDetailModalOpen = false;
            return;
        }
        if (pointInRect(m, dx + dw - 146, dy + dh - 46, 120, 32)) {
            mSaleDetailModalOpen = false;
            return;
        }
        if (!pointInRect(m, dx, dy, dw, dh)) {
            mSaleDetailModalOpen = false;
        }
        return;
    }

    // Promo Code Management Modal
    if (mPromoModalOpen) {
        float dw = 780.f, dh = 530.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        // Close [X] Button
        if (pointInRect(m, dx + dw - 40, dy + 16, 26, 26)) {
            mPromoModalOpen = false;
            return;
        }
        // Close Button
        if (pointInRect(m, dx + dw - 146, dy + dh - 44, 120, 32)) {
            mPromoModalOpen = false;
            return;
        }

        float colW = 340.f;
        float colH = 405.f;
        float colY = dy + 68.f;

        // Promo Code input
        float fCodeY = colY + 54.f;
        if (pointInRect(m, dx + 40, fCodeY + 14, colW - 32, 32)) {
            mNewPromoActiveField = 0;
            return;
        }

        // Discount Rate input
        float fRateY = fCodeY + 54.f;
        if (pointInRect(m, dx + 40, fRateY + 14, colW - 32, 32)) {
            mNewPromoActiveField = 1;
            return;
        }

        // Scope pills
        float fScopeY = fRateY + 54.f;
        float pillW = (colW - 36.f) / 3.f;
        float p1X = dx + 40.f;
        float p2X = p1X + pillW + 2.f;
        float p3X = p2X + pillW + 2.f;
        float pillY = fScopeY + 14.f;

        if (pointInRect(m, p1X, pillY, pillW, 26.f)) {
            mNewPromoTargetType = "ALL";
            mNewPromoTargetValue = "ALL";
            return;
        }
        if (pointInRect(m, p2X, pillY, pillW, 26.f)) {
            mNewPromoTargetType = "ITEM";
            if (mNewPromoTargetValue == "ALL") mNewPromoTargetValue.clear();
            mNewPromoActiveField = 2;
            return;
        }
        if (pointInRect(m, p3X, pillY, pillW, 26.f)) {
            mNewPromoTargetType = "SUPPLIER";
            if (mNewPromoTargetValue == "ALL") mNewPromoTargetValue.clear();
            mNewPromoActiveField = 2;
            return;
        }

        // Target value input
        float fTargY = pillY + 34.f;
        if (mNewPromoTargetType != "ALL" && pointInRect(m, dx + 40, fTargY + 14, colW - 32, 32)) {
            mNewPromoActiveField = 2;
            return;
        }

        // Save Promo button
        float fSaveBtnY = colY + colH - 46.f;
        if (pointInRect(m, dx + 40, fSaveBtnY, colW - 32, 34)) {
            if (mNewPromoCode.empty()) {
                showToast("Please enter a promo voucher code");
                return;
            }
            PromoRecord rec;
            rec.code = mNewPromoCode;
            int pct = 0, flat = 0;
            if (mNewPromoDiscountStr.find('%') != std::string::npos) {
                try { pct = std::stoi(mNewPromoDiscountStr); } catch (...) {}
            } else {
                try { flat = std::stoi(mNewPromoDiscountStr); } catch (...) {}
            }
            if (pct <= 0 && flat <= 0) pct = 10;
            rec.percent = pct;
            rec.flat = flat;
            rec.targetType = mNewPromoTargetType;
            rec.targetValue = (mNewPromoTargetType == "ALL") ? "ALL" : mNewPromoTargetValue;
            mCartService.addPromoCode(rec);
            mActivityLogger.logActivity("SETTINGS", mCurrentUserName, "Added promo voucher: " + rec.code);
            showToast("Promo voucher registered: " + rec.code);
            mNewPromoCode.clear();
            mNewPromoTargetValue.clear();
            return;
        }

        // Delete button in right column table
        float rColX = dx + 24 + colW + 14.f;
        float rColW = dw - (rColX - dx) - 24.f;
        float rHdrY = colY + 48.f;
        float rRowY = rHdrY + 22.f;
        const auto& pList = mCartService.getPromoCodes();
        for (size_t r = 0; r < pList.size() && r < 7; r++) {
            float delW = 48.f, delH = 22.f;
            float delX = rColX + rColW - 16.f - delW;
            float rowYPos = rRowY + static_cast<float>(r) * 32.f;
            if (pointInRect(m, delX, rowYPos, delW, delH)) {
                std::string codeToDel = pList[r].code;
                mCartService.deletePromoCode(codeToDel);
                mActivityLogger.logActivity("SETTINGS", mCurrentUserName, "Deleted promo voucher: " + codeToDel);
                showToast("Deleted promo voucher: " + codeToDel);
                return;
            }
        }

        if (!pointInRect(m, dx, dy, dw, dh)) {
            mPromoModalOpen = false;
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
                mCartService.resumeDraft((int)d);
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

        float sBoxX = dx + 28.f, sBoxY = dy + 70.f, sBoxW = 470.f, sBoxH = 32.f;
        if (pointInRect(m, sBoxX, sBoxY, sBoxW, sBoxH)) {
            mAddProductSearchFocused = true;
            return;
        } else {
            mAddProductSearchFocused = false;
        }

        // + New Product & Barcode shortcut (launches Add Inventory & Product creation modal)
        if (pointInRect(m, dx + dw - 242.f, sBoxY, 214.f, sBoxH)) {
            mAddProductModalOpen = false;
            mAddInventoryModalOpen = true;
            mNewItemAlsoAddToCatalog = true;
            mNewItemDesc = "";
            mNewItemBarcode = "";
            mNewItemGenerated = false;
            mNewItemActiveField = 1;
            mVendorDropdownOpen = false;
            mScanningUniversalSupplier = false;
            return;
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
                    const auto* cat = mCategoryManager.findByName(it.category);
                    ProductItem newP;
                    newP.barcode = it.barcode;
                    newP.name = it.name;
                    newP.category = it.category;
                    newP.categoryId = cat ? cat->categoryId : (it.barcode.size() >= 2 ? it.barcode.substr(0, 2) : "01");
                    newP.unit = it.unit;
                    newP.price = it.retailPrice;
                    newP.unitPrice = it.retailPrice;
                    newP.tax = std::max(0, it.retailPrice / 20);
                    newP.discount = 0;
                    newP.stock = q;
                    newP.barcodePath = it.barcodePath;
                    newP.iconKey = "rice";
                    std::string cLower = toLowerStr(it.category);
                    if (cLower.find("dairy") != std::string::npos) newP.iconKey = "milk";
                    else if (cLower.find("bake") != std::string::npos || cLower.find("bread") != std::string::npos) newP.iconKey = "bread";
                    else if (cLower.find("drink") != std::string::npos) newP.iconKey = "juice";
                    else if (cLower.find("fruit") != std::string::npos) newP.iconKey = "apples";
                    else if (cLower.find("pers") != std::string::npos) newP.iconKey = "oil";
                    else if (cLower.find("snack") != std::string::npos || cLower.find("house") != std::string::npos) newP.iconKey = "olive";
                    mProductCatalog.addProduct(newP);
                }
                mProductCatalog.saveToFile();
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

    // 2. Enlarge Cart Modal (1040x590 with Live Camera Scanner on Left Side)
    if (mEnlargeCartModalOpen) {
        float dw = 1040.f, dh = 590.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;
        float scanX = dx + 20.f, scanY = dy + 20.f, scanW = 286.f, scanH = dh - 40.f;
        float paneW = scanW - 24.f;
        float paneH = 200.f;
        float paneY = scanY + 44.f;
        float bcBoxY = paneY + paneH + 14.f;
        float cartX = scanX + scanW + 22.f;
        float cartW = dw - (scanW + 64.f);

        if (pointInRect(m, dx + dw - 44, dy + 20, 28, 28)) {
            mEnlargeCartModalOpen = false;
            return;
        }

        // Left side camera Switch (C) pill or Switch button
        if (pointInRect(m, scanX + scanW - 86.f, scanY + 11.f, 74.f, 22.f) ||
            pointInRect(m, scanX + 12.f, scanY + scanH - 86.f, paneW, 34.f)) {
            switchCamera();
            return;
        }

        // Barcode input box focus inside Enlarge Cart
        if (pointInRect(m, scanX + 12.f, bcBoxY, paneW, 34.f)) {
            mEnlargeScannerFocused = true;
            return;
        } else {
            mEnlargeScannerFocused = false;
        }

        // Add by Typed Barcode / Quick Check button inside left camera console
        if (pointInRect(m, scanX + 12.f, scanY + scanH - 44.f, paneW, 34.f)) {
            std::string q = trimStr(mEnlargeScannerQuery);
            if (!q.empty()) {
                int pIdx = mProductCatalog.findProductByBarcode(q);
                if (pIdx < 0) pIdx = mProductCatalog.findProductByName(q);
                if (pIdx >= 0) {
                    onBarcodeScanned(products[pIdx].barcode);
                    mEnlargeScannerQuery.clear();
                } else {
                    showToast("Barcode '" + q + "' not found");
                }
            } else {
                mEnlargeCartModalOpen = false;
                mQuickCheckModalOpen = true;
                mQuickCheckBarcode.clear();
                mQuickCheckProductIdx = -1;
                mQuickCheckInputFocused = true;
            }
            return;
        }

        // Right side Cart Items
        float rowStartY = dy + 120.f;
        for (int i = 0; i < (int)cart.size(); i++) {
            float rowY = rowStartY + i * 54.f - mEnlargeScrollOffset;
            if (rowY < rowStartY - 8.f || rowY > rowStartY + 324.f) continue;

            if (pointInRect(m, cartX + 410, rowY + 14, 24, 24)) {
                mCartService.setItemQuantity(i, cart[i].quantity - 1);
                return;
            }
            if (pointInRect(m, cartX + 478, rowY + 14, 24, 24)) {
                int pid = cart[i].productId;
                if (pid >= 0 && pid < (int)products.size() && cart[i].quantity + 1 > products[pid].stock) {
                    showToast("Stock limit: only " + std::to_string(products[pid].stock) + " available for " + products[pid].name);
                    return;
                }
                mCartService.setItemQuantity(i, cart[i].quantity + 1);
                return;
            }
            if (pointInRect(m, cartX + cartW - 68, rowY + 14, 58, 24)) {
                std::string n = products[cart[i].productId].name;
                mCartService.deleteItem(i);
                showToast("Deleted " + n + " from receipt");
                return;
            }
        }

        float botY = dy + dh - 64.f;

        // Back to Register button
        if (pointInRect(m, dx + dw - 406, botY - 8, 160, 40)) {
            mEnlargeCartModalOpen = false;
            return;
        }

        // Proceed to Pay button
        if (pointInRect(m, dx + dw - 234, botY - 8, 212, 40)) {
            mEnlargeCartModalOpen = false;
            if (mCartService.getPaymentMethod() == 0) {
                mCashModalOpen = true;
                mCashInputFocused = true;
                mSpecialDiscountFocused = false;
            } else {
                mCardModalOpen = true;
                mSpecialDiscountFocused = false;
            }
            return;
        }

        if (!pointInRect(m, dx, dy, dw, dh)) {
            mEnlargeCartModalOpen = false;
        }
        return;
    }

    // 3. Cash Payment Modal
    if (mCashModalOpen) {
        float dw = 500.f, dh = 468.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 42, dy + 18, 26, 26)) {
            mCashModalOpen = false;
            return;
        }

        // Special discount input focus (max 8%)
        if (pointInRect(m, dx + 225, dy + 140, 115, 28)) {
            mSpecialDiscountFocused = true;
            mCashInputFocused = false;
            return;
        }

        // Cash Tendered input focus
        if (pointInRect(m, dx + 160, dy + 180, 180, 32)) {
            mCashInputFocused = true;
            mSpecialDiscountFocused = false;
            return;
        }

        if (pointInRect(m, dx + 348, dy + 180, 56, 32)) {
            mCartService.setCashGiven(computeCurrentBillSummary().totalPayable);
            return;
        }

        if (pointInRect(m, dx + 412, dy + 180, 58, 32)) {
            mCartService.setCashGiven(0);
            return;
        }

        const int chipVals[] = {100, 500, 1000, 5000};
        for (int i = 0; i < 4; i++) {
            float cx = dx + 30 + i * 112.f;
            float cy = dy + 226.f;
            if (pointInRect(m, cx, cy, 102, 28)) {
                mCartService.setCashGiven(mCartService.parseCashGiven() + chipVals[i]);
                return;
            }
        }

        if (pointInRect(m, dx + 30, dy + dh - 56, 270, 42)) {
            finalizePayment(0);
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
        float dw = 480.f, dh = 415.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 42, dy + 18, 26, 26)) {
            mCardModalOpen = false;
            return;
        }

        if (pointInRect(m, dx + 220, dy + 158, 115, 28)) {
            mSpecialDiscountFocused = true;
            return;
        } else {
            mSpecialDiscountFocused = false;
        }

        if (pointInRect(m, dx + 30, dy + dh - 56, 260, 42)) {
            finalizePayment(1);
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
        float dw = 920.f, dh = 595.f;
        float dx = (mCurrentWidth - dw) / 2.f;
        float dy = (mDesignHeight - dh) / 2.f;
        float scanX = dx + 24.f;
        float scanY = dy + 24.f;
        float scanW = 280.f;
        float scanH = dh - 48.f;
        float formX = scanX + scanW + 24.f;
        float formW = dw - (scanW + 48.f) - 24.f;

        if (pointInRect(m, dx + dw - 42, dy + 18, 26, 26)) {
            mAddInventoryModalOpen = false;
            return;
        }

        // Camera Switch button on left scanner panel
        if (pointInRect(m, scanX + scanW - 86.f, scanY + 12.f, 74.f, 24.f)) {
            switchCamera();
            return;
        }

        // Left Scanner Panel Clicks (Quick Toggle Button or Viewfinder Tap)
        if (pointInRect(m, scanX + 12.f, scanY + scanH - 46.f, scanW - 24.f, 34.f) ||
            pointInRect(m, scanX + 12.f, scanY + 44.f, scanW - 24.f, 220.f)) {
            mScanningUniversalSupplier = !mScanningUniversalSupplier;
            mNewItemActiveField = 6;
            if (mScanningUniversalSupplier) {
                showToast("Webcam / Gun scanner active: Hold barcode in front of camera!");
            } else {
                showToast("Optical Scanner Paused");
            }
            return;
        }

        // Vendor dropdown selection if open
        if (mVendorDropdownOpen) {
            std::vector<std::string> vendors = mInventoryManager.getUniqueSuppliers();
            float dropX = formX;
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
        if (pointInRect(m, formX + 320 - 28, dy + 68 + 16, 26, 32)) {
            mVendorDropdownOpen = !mVendorDropdownOpen;
            return;
        }

        // Supplier Name (Field 0)
        if (pointInRect(m, formX, dy + 68 + 16, 290, 32)) {
            mNewItemActiveField = 0;
            mVendorDropdownOpen = false;
            return;
        }

        // Arrival Date (Field 5)
        if (pointInRect(m, formX + 338, dy + 68 + 16, formW - 338, 32)) {
            mNewItemActiveField = 5;
            mVendorDropdownOpen = false;
            return;
        }

        // Description / Name (Field 1)
        if (pointInRect(m, formX, dy + 125 + 16, 360, 32)) {
            mNewItemActiveField = 1;
            mVendorDropdownOpen = false;
            return;
        }

        // Unit (Field 7)
        if (pointInRect(m, formX + 378, dy + 125 + 16, formW - 378, 32)) {
            mNewItemActiveField = 7;
            mVendorDropdownOpen = false;
            return;
        }

        // Dynamic Category Pills from PCategory.txt (up to 10 in 2 rows of 5)
        const auto& cats = mCategoryManager.getCategories();
        float pillW = (formW - 32.f) / 5.f;
        size_t catCount = std::min<size_t>(10, cats.size());
        for (size_t c = 0; c < catCount; c++) {
            float cx = formX + (c % 5) * (pillW + 8.f);
            float cy = dy + 198 + (c / 5) * 26;
            if (pointInRect(m, cx, cy, pillW, 22)) {
                mNewItemCategory = cats[c].name;
                mVendorDropdownOpen = false;
                return;
            }
        }

        // Retail Price (Field 2)
        float col3W = (formW - 20.f) / 3.f;
        if (pointInRect(m, formX, dy + 256 + 16, col3W, 32)) { mNewItemActiveField = 2; return; }
        // Cost Price (Field 3)
        if (pointInRect(m, formX + col3W + 10.f, dy + 256 + 16, col3W, 32)) { mNewItemActiveField = 3; return; }
        // Quantity (Field 4)
        if (pointInRect(m, formX + (col3W + 10.f) * 2.f, dy + 256 + 16, col3W, 32)) { mNewItemActiveField = 4; return; }

        // Barcode Input Box (Field 6)
        float bcInputW = formW - 310.f;
        if (pointInRect(m, formX, dy + 330, bcInputW, 34)) {
            mNewItemActiveField = 6;
            return;
        }

        // Auto Barcode Button
        if (pointInRect(m, formX + bcInputW + 10.f, dy + 330, 135, 34)) {
            mScanningUniversalSupplier = false;
            generateNewItemBarcode();
            return;
        }

        // Universal Barcode Scanner Button
        float scanBtnW = formW - (bcInputW + 155.f);
        if (pointInRect(m, formX + bcInputW + 155.f, dy + 330, scanBtnW, 34)) {
            mScanningUniversalSupplier = !mScanningUniversalSupplier;
            mNewItemActiveField = 6;
            if (mScanningUniversalSupplier) {
                showToast("Webcam / Gun scanner active: Hold barcode in front of camera!");
            } else {
                showToast("Optical Scanner Paused");
            }
            return;
        }

        // Checkbox: Also list directly in Product.txt
        if (pointInRect(m, formX, dy + 474.f, 330, 22)) {
            mNewItemAlsoAddToCatalog = !mNewItemAlsoAddToCatalog;
            return;
        }

        // Save Button
        if (pointInRect(m, formX, dy + dh - 52, 260, 38)) {
            saveNewInventoryItem();
            return;
        }

        // Cancel Button
        if (pointInRect(m, formX + formW - 130, dy + dh - 52, 130, 38)) {
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

    // Navigation Sidebar (Strict ISE-105 Role Isolation: Tab 3 Reports restricted from Cashier)
    bool isAdmin = (mCurrentRole == "Admin");
    std::vector<int> visibleTabs = isAdmin ? std::vector<int>{0, 1, 2, 3, 4, 5, 6} : std::vector<int>{0, 1, 6};

    for (size_t slot = 0; slot < visibleTabs.size(); slot++) {
        float cy = Layout::NAV_Y[slot];
        if (pointInRect(m, 4, cy - 24, 60, 48)) {
            mActiveTab = visibleTabs[slot];
            if (mActiveTab == 3) {
                reloadAuditData();
                mSalesScrollOffset = 0.f;
                mCashCustomActiveField = 0;
            }
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
                float rowY = 92.f + 44.f + static_cast<float>(vi) * 44.f - mProductsScrollOffset;
                if (rowY < 92.f + 34.f || rowY + 24.f > 92.f + 562.f) continue;

                // [ Specs ] button (ISE-57 Requirement 5 Product Specs)
                float specBtnX = (mCurrentRole == "Admin") ? 406.f : 418.f;
                float specBtnW = (mCurrentRole == "Admin") ? 38.f : 46.f;
                if (pointInRect(m, mMainX + specBtnX, rowY - 6.f, specBtnW, 22.f)) {
                    mSpecProductIdx = idx;
                    mSpecModalOpen = true;
                    return;
                }

                // [ Edit ] button (Requirement 6: Manager can edit any product)
                if (pointInRect(m, mMainX + 448.f, rowY - 6.f, 38.f, 22.f)) {
                    if (mCurrentRole != "Admin") {
                        showToast("Restricted: Manager authorization required to edit products");
                    } else {
                        openEditProductModal(idx);
                    }
                    return;
                }

                // [ Del ] button
                if (pointInRect(m, mMainX + 490.f, rowY - 6.f, 38.f, 22.f)) {
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
                    mScanningUniversalSupplier = true;
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
                    if (rowY < listStartY + 6.f || rowY + 42.f > ty + 482.f) continue;

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
        } else if (mActiveTab == 3) { // Manager Reports Tab
            if (mCurrentRole == "Admin") {
                float pillW = 86.f;
                float pillY = 66.f;
                for (int ti = 0; ti < 6; ti++) {
                    float px = mMainX + ti * 91.f;
                    if (pointInRect(m, px, pillY, pillW, 26.f)) {
                        mSalesSubTab = ti;
                        mSalesScrollOffset = 0.f;
                        mCashCustomActiveField = 0;
                        return;
                    }
                }
            } else {
                return;
            }

            // Subtab 2: Cash Sales Audit controls
            if (mSalesSubTab == 2) {
                float filterY = 98.f;
                float fX = mMainX;
                for (int fi = 0; fi < 5; fi++) {
                    float fw = 98.f;
                    if (pointInRect(m, fX, filterY, fw, 22.f)) {
                        mSalesTimeFilter = fi;
                        mSalesScrollOffset = 0.f;
                        mCashCustomActiveField = 0;
                        return;
                    }
                    fX += fw + 8.f;
                }

                // If Custom Range is selected
                if (mSalesTimeFilter == 4) {
                    float customBoxY = 126.f;
                    float b1X = mMainX + 38.f;
                    float b2X = mMainX + 154.f;
                    float p1X = mMainX + 252.f;
                    float p2X = p1X + 90.f;
                    float p3X = p2X + 90.f;

                    if (pointInRect(m, b1X, customBoxY, 88.f, 22.f)) {
                        mCashCustomActiveField = 1;
                        return;
                    } else if (pointInRect(m, b2X, customBoxY, 88.f, 22.f)) {
                        mCashCustomActiveField = 2;
                        return;
                    } else if (pointInRect(m, p1X, customBoxY, 84.f, 22.f)) {
                        mCashCustomStartDate = "2026-11-17";
                        mCashCustomEndDate = "2026-11-19";
                        mCashCustomActiveField = 0;
                        mSalesScrollOffset = 0.f;
                        return;
                    } else if (pointInRect(m, p2X, customBoxY, 84.f, 22.f)) {
                        mCashCustomStartDate = "2026-01-01";
                        mCashCustomEndDate = "2026-12-31";
                        mCashCustomActiveField = 0;
                        mSalesScrollOffset = 0.f;
                        return;
                    } else if (pointInRect(m, p3X, customBoxY, 78.f, 22.f)) {
                        mCashCustomStartDate = "";
                        mCashCustomEndDate = "";
                        mCashCustomActiveField = 0;
                        mSalesScrollOffset = 0.f;
                        return;
                    } else {
                        mCashCustomActiveField = 0;
                    }
                }
            }
            return;
        } else if (mActiveTab == 4) { // Activity Logs Tab
            const char* filters[] = {"ALL", "LOGIN", "SALE", "RETURN", "STAFF", "INVENTORY", "ORDER"};
            for (int i = 0; i < 7; i++) {
                if (pointInRect(m, mMainX + i * 76.f, 95.f, 70.f, 26.f)) {
                    mLogFilter = filters[i];
                    mLogScrollOffset = 0.f;
                    return;
                }
            }
        } else if (mActiveTab == 5) { // Settings Tab
            // Sub-tab pills: [ 0. System & Staff ] [ 1. Product Categories ] [ 2. Seasonal Discounts ]
            if (mCurrentRole == "Admin") {
                float subW = (Layout::MAIN_W - 16.f) / 3.f;
                float subY = 56.f;
                for (int sti = 0; sti < 3; sti++) {
                    float px = mMainX + sti * (subW + 8.f);
                    if (pointInRect(m, px, subY, subW, 26.f)) {
                        mSettingsSubTab = sti;
                        return;
                    }
                }
            }

            // Subtab 1: Categories
            if (mSettingsSubTab == 1 && mCurrentRole == "Admin") {
                float ty = 88.f;
                if (pointInRect(m, mMainX + Layout::MAIN_W - 145, ty + 16, 125, 28)) {
                    openAddCategoryModal();
                    return;
                }
                float thY = ty + 62.f;
                const auto& cats = mCategoryManager.getCategories();
                for (size_t i = 0; i < cats.size(); i++) {
                    float rowY = thY + 28.f + static_cast<float>(i) * 34.f - mSettingsScrollOffset;
                    if (rowY < thY + 20.f || rowY + 24.f > ty + 560.f) continue;
                    if (pointInRect(m, mMainX + 455, rowY - 2, 56, 20)) {
                        if (cats.size() <= 1) {
                            showToast("At least one category must remain in PCategory.txt");
                            return;
                        }
                        std::string cid = cats[i].categoryId;
                        std::string cname = cats[i].name;
                        mCategoryManager.deleteCategory(cid);
                        syncProductCategoriesFromManager();
                        mActivityLogger.logActivity("PRODUCT", mCurrentUserName, "Removed category: " + cname + " (" + cid + ")");
                        showToast("Removed category " + cname);
                        return;
                    }
                }
                return;
            }

            // Subtab 2: Seasons
            if (mSettingsSubTab == 2 && mCurrentRole == "Admin") {
                float ty = 88.f;
                if (pointInRect(m, mMainX + Layout::MAIN_W - 145, ty + 16, 125, 28)) {
                    openAddSeasonModal();
                    return;
                }

                // Toggle & Remove buttons in table
                float thY = ty + 62.f;
                const auto& seasons = mSeasonManager.getSeasons();
                for (size_t i = 0; i < seasons.size(); i++) {
                    float rowY = thY + 28.f + static_cast<float>(i) * 36.f - mSettingsScrollOffset;
                    if (rowY < thY + 20.f || rowY + 24.f > ty + 560.f) continue;
                    if (pointInRect(m, mMainX + 380, rowY - 2, 64, 20)) {
                        mSeasonManager.toggleActive((int)i);
                        showToast(mSeasonManager.getSeasons()[i].isActive ? "Seasonal discount enabled" : "Seasonal discount disabled");
                        return;
                    }
                    if (pointInRect(m, mMainX + 452, rowY - 2, 58, 20)) {
                        mSeasonManager.deleteSeason((int)i);
                        mActivityLogger.logActivity("SETTINGS", mCurrentUserName, "Removed seasonal discount rule");
                        showToast("Seasonal rule removed from Seasons.txt");
                        return;
                    }
                }
                return;
            }

            float ty = 88.f;

            // 1. Active Scanner Hardware Button
            if (pointInRect(m, mMainX + Layout::MAIN_W - 145, ty + 10, 125, 24)) {
                if (mCameraRunning) {
                    mCameraStream.stop();
                    mCameraRunning = false;
                    showToast("Scanner hardware deactivated");
                } else {
                    mCameraRunning = mCameraStream.start(-1);
                    showToast(mCameraRunning ? "Scanner hardware activated" : "Camera failed to connect");
                }
                return;
            }

            // 2. Camera Device Switcher Button
            if (pointInRect(m, mMainX + Layout::MAIN_W - 145, ty + 46, 125, 24)) {
                switchCamera();
                return;
            }

            // 3. Audio Scan Feedback Button
            if (pointInRect(m, mMainX + Layout::MAIN_W - 145, ty + 118, 125, 24)) {
                mSoundEnabled = !mSoundEnabled;
                showToast(mSoundEnabled ? "Audio scan feedback enabled (beep on scan)" : "Audio scan feedback muted");
                return;
            }

            // 4. Staff & Operator Management (+ Add Employee)
            float staffY = ty + 160.f;
            if (pointInRect(m, mMainX + Layout::MAIN_W - 135, staffY + 8, 115, 24)) {
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

            // 5. Staff rows (Reset PIN & Remove)
            if (mCurrentRole == "Admin") {
                float thY = staffY + 42.f;
                const auto& staffList = mStaffManager.getStaffList();
                for (size_t i = 0; i < staffList.size() && i < 4; i++) {
                    float sRowY = thY + 18.f + static_cast<float>(i) * 28.f;
                    if (pointInRect(m, mMainX + 375, sRowY - 2, 65, 18)) {
                        mResetPassModalOpen = true;
                        mResetStaffTarget = staffList[i].name;
                        mResetStaffNewPass = "";
                        mResetPassFocused = true;
                        return;
                    }
                    if (staffList[i].name != mCurrentUserName && pointInRect(m, mMainX + 448, sRowY - 2, 50, 18)) {
                        std::string remName = staffList[i].name;
                        mStaffManager.removeStaffMember(remName);
                        mActivityLogger.logActivity("STAFF", mCurrentUserName, "Removed staff account: " + remName);
                        showToast("Staff account removed: " + remName);
                        return;
                    }
                }
            }

            // 6. Discount Configuration & Promo Management
            float discY = ty + 356.f;
            if (mCurrentRole == "Admin") {
                float r1Y = discY + 42.f;
                // Student Rate [-]
                if (pointInRect(m, mMainX + 185, r1Y, 26, 24)) {
                    mCartService.setStudentDiscountRate(mCartService.getStudentDiscountRate() - 5);
                    mActivityLogger.logActivity("SETTINGS", mCurrentUserName, "Updated student discount rate to " + std::to_string(mCartService.getStudentDiscountRate()) + "%");
                    showToast("Student discount set to " + std::to_string(mCartService.getStudentDiscountRate()) + "%");
                    return;
                }
                // Student Rate [+]
                if (pointInRect(m, mMainX + 250, r1Y, 26, 24)) {
                    mCartService.setStudentDiscountRate(mCartService.getStudentDiscountRate() + 5);
                    mActivityLogger.logActivity("SETTINGS", mCurrentUserName, "Updated student discount rate to " + std::to_string(mCartService.getStudentDiscountRate()) + "%");
                    showToast("Student discount set to " + std::to_string(mCartService.getStudentDiscountRate()) + "%");
                    return;
                }

                // Promo Management Button
                float promoBtnW = Layout::MAIN_W - 330.f;
                if (pointInRect(m, mMainX + 305, r1Y + 14, promoBtnW, 34)) {
                    mPromoModalOpen = true;
                    mNewPromoCode = "";
                    mNewPromoDiscountStr = "15%";
                    mNewPromoTargetType = "ALL";
                    mNewPromoTargetValue = "ALL";
                    mNewPromoActiveField = 0;
                    return;
                }

                float r2Y = discY + 76.f;
                // Employee Rate [-]
                if (pointInRect(m, mMainX + 185, r2Y, 26, 24)) {
                    mCartService.setEmployeeDiscountRate(mCartService.getEmployeeDiscountRate() - 5);
                    mActivityLogger.logActivity("SETTINGS", mCurrentUserName, "Updated employee discount rate to " + std::to_string(mCartService.getEmployeeDiscountRate()) + "%");
                    showToast("Employee discount set to " + std::to_string(mCartService.getEmployeeDiscountRate()) + "%");
                    return;
                }
                // Employee Rate [+]
                if (pointInRect(m, mMainX + 250, r2Y, 26, 24)) {
                    mCartService.setEmployeeDiscountRate(mCartService.getEmployeeDiscountRate() + 5);
                    mActivityLogger.logActivity("SETTINGS", mCurrentUserName, "Updated employee discount rate to " + std::to_string(mCartService.getEmployeeDiscountRate()) + "%");
                    showToast("Employee discount set to " + std::to_string(mCartService.getEmployeeDiscountRate()) + "%");
                    return;
                }
            }

            float utilY = discY + 122.f;
            if (pointInRect(m, mMainX + 25, utilY, 240, 30)) {
                mProductCatalog.loadProductsFile();
                mCategoryManager.loadCategories();
                syncProductCategoriesFromManager();
                mSeasonManager.loadSeasons();
                mCartService.loadDiscountSettings();
                mCartService.loadPromoCodes();
                loadReturnsFile();
                reloadAuditData();
                showToast("All databases & config files reloaded from disk");
                return;
            }
            if (pointInRect(m, mMainX + 280, utilY, 240, 30)) {
                showToast("Barcode bitmaps synchronized");
                return;
            }
            return;
        } else if (mActiveTab == 6) { // Return Tab
            float ty = 85.f;
            float colW = (Layout::MAIN_W - 20.f) / 2.f;
            float paneW = colW - 32.f;

            // Live Camera Switch (C) pill inside Return camera
            float camX = mMainX + 16.f;
            float camY = ty + 32.f;
            float camW = paneW;
            float camSwW = 110.f;
            float camSwX = camX + camW - camSwW - 8.f;
            if (pointInRect(m, camSwX, camY + 7.f, camSwW, 18.f)) {
                switchCamera();
                return;
            }

            float inY = ty + 154.f;
            float searchInputW = paneW - 60.f;

            // Barcode / Order input box click
            if (pointInRect(m, mMainX + 16, inY, searchInputW, 30.f)) {
                mReturnBarcodeFocused = true;
                return;
            } else {
                mReturnBarcodeFocused = false;
            }

            // Look Up / Find Button
            float findX = mMainX + 16 + searchInputW + 6.f;
            if (pointInRect(m, findX, inY, 54.f, 30.f)) {
                lookupReturnItem(mReturnBarcode);
                return;
            }

            // Quick Samples Pills
            float pillY = inY + 36.f;
            std::string latestPaidId = "10000117";
            for (int i = (int)mOrders.size() - 1; i >= 0; --i) {
                if (toLowerStr(mOrders[i].orderStatus) == "paid") {
                    latestPaidId = mOrders[i].orderId;
                    break;
                }
            }
            const char* demoCodes[] = {"010001", "030003", "090006", latestPaidId.c_str()};
            float pX = mMainX + 16;
            float pW = (paneW - 12.f) / 4.f;
            for (int p = 0; p < 4; p++) {
                if (pointInRect(m, pX, pillY + 13.f, pW, 20.f)) {
                    lookupReturnItem(demoCodes[p]);
                    return;
                }
                pX += pW + 4.f;
            }

            // Verified Past Orders list clicks
            float ordListY = pillY + 42.f;
            int maxOrdersToShow = 4;
            int shown = 0;
            for (int i = (int)mOrders.size() - 1; i >= 0 && shown < maxOrdersToShow; --i) {
                const auto& ord = mOrders[i];
                if (toLowerStr(ord.orderStatus) != "paid") continue;
                float rRowY = ordListY + 14.f + static_cast<float>(shown) * 48.f;
                shown++;

                if (pointInRect(m, mMainX + 16, rRowY, paneW, 43.f)) {
                    lookupReturnItem(ord.orderId);
                    return;
                }
            }

            // Right Column actions
            float rX = mMainX + colW + 20.f;
            if (mSelectedReturnProductIdx >= 0) {
                float pY = ty + 34.f;
                float vY = pY + 54.f;
                bool hasMultiItems = (mReturnOrderItemPills.size() > 1);
                float vH = hasMultiItems ? 78.f : 54.f;

                if (hasMultiItems && !mReturnOrderId.empty()) {
                    float pillCount = (float)std::min<size_t>(3, mReturnOrderItemPills.size());
                    float mpw = (paneW - 16.f - (pillCount - 1) * 4.f) / pillCount;
                    for (size_t pi = 0; pi < mReturnOrderItemPills.size() && pi < 3; ++pi) {
                        float mpx = rX + 24.f + pi * (mpw + 4.f);
                        float mpy = vY + 52.f;
                        if (pointInRect(m, mpx, mpy, mpw, 18.f)) {
                            lookupReturnItem(mReturnOrderId + "|" + mReturnOrderItemPills[pi].first);
                            return;
                        }
                    }
                }

                float qY = vY + vH + 8.f;

                // [-] Qty
                if (pointInRect(m, rX + 16 + paneW - 90.f, qY, 26.f, 24.f)) {
                    if (mReturnQty > 1) mReturnQty--;
                    return;
                }

                // [+] Qty
                if (pointInRect(m, rX + 16 + paneW - 26.f, qY, 26.f, 24.f)) {
                    if (mReturnMaxQty <= 0) {
                        showToast("Cannot return: 0 units eligible for return.");
                    } else if (mReturnQty < mReturnMaxQty) {
                        mReturnQty++;
                    } else {
                        showToast("Limit reached: Max eligible units is " + std::to_string(mReturnMaxQty));
                    }
                    return;
                }

                // Return Reason Pills
                float reasY = qY + 30.f;
                for (int ri = 0; ri < 4; ri++) {
                    float rbx = rX + 16 + (ri % 2) * (paneW / 2.f);
                    float rby = reasY + 16.f + (ri / 2) * 24.f;
                    float rbw = (paneW / 2.f) - 4.f;
                    if (pointInRect(m, rbx, rby, rbw, 20.f)) {
                        mReturnReasonIdx = ri;
                        mReturnRestock = !(ri == 1 || ri == 3);
                        return;
                    }
                }

                // Refund Method Pills
                float mthY = reasY + 66.f;
                for (int mi = 0; mi < 3; mi++) {
                    float mbw = (paneW - 8.f) / 3.f;
                    float mbx = rX + 16 + mi * (mbw + 4.f);
                    float mby = mthY + 16.f;
                    if (pointInRect(m, mbx, mby, mbw, 22.f)) {
                        mReturnPaymentMethod = mi;
                        return;
                    }
                }

                // Restock Checkbox
                float dispY = mthY + 44.f;
                if (pointInRect(m, rX + 16, dispY, paneW, 20.f)) {
                    mReturnRestock = !mReturnRestock;
                    return;
                }

                // PROCESS RETURN
                float refY = dispY + 26.f;
                float btnY = refY + 44.f;
                if (pointInRect(m, rX + 16, btnY, paneW, 34.f)) {
                    processReturn();
                    return;
                }

                // View Return Slip Button
                float slipY = btnY + 38.f;
                if (!mReturnSlipText.empty() && pointInRect(m, rX + 16, slipY, paneW, 24.f)) {
                    mReturnSlipModalOpen = true;
                    return;
                }
            } else {
                // "Load Sample Item (010001)" button in empty workflow guide
                float guideY = ty + 40.f;
                float splBtnY = guideY + 98.f;
                if (pointInRect(m, rX + 26, splBtnY, paneW - 20.f, 28.f)) {
                    lookupReturnItem("010001");
                    return;
                }
            }
            return;
        }
        return;
    }

    // Quick Check Header Pill Button (Tab 0) - aligned flush right
    if (mActiveTab == 0 && pointInRect(m, mMainX + Layout::MAIN_W - 125.f, 46.f, 125.f, 28.f)) {
        mQuickCheckModalOpen = true;
        mQuickCheckBarcode.clear();
        mQuickCheckProductIdx = -1;
        mQuickCheckInputFocused = true;
        return;
    }

    // POS Tab Controls
    sf::FloatRect searchBox{mMainX, 82.f, 410.f, 39.f};
    sf::FloatRect addBtn{mMainX + 420.f, 82.f, 125.f, 39.f};
    sf::FloatRect viewportBox{mMainX, 135.f, Layout::MAIN_W, 158.f};

    mSearchFocused = pointInRect(m, searchBox.left, searchBox.top, searchBox.width, searchBox.height);

    if (pointInRect(m, addBtn.left, addBtn.top, addBtn.width, addBtn.height)) {
        if (mCurrentRole != "Admin") {
            showToast("Restricted: Manager login required to add new products");
            return;
        }
        mAddInventoryModalOpen = true;
        mNewItemAlsoAddToCatalog = true;
        mNewItemDesc = "";
        mNewItemBarcode = "";
        mNewItemGenerated = false;
        mNewItemActiveField = 1;
        mVendorDropdownOpen = false;
        mScanningUniversalSupplier = false;
        return;
    }

    float camBadgeW = 215.f;
    float camBadgeX = viewportBox.left + viewportBox.width - camBadgeW - 14.f;
    float camBadgeY = viewportBox.top + 11.f;
    if (pointInRect(m, camBadgeX, camBadgeY, camBadgeW, 26.f)) {
        switchCamera();
        return;
    }

    if (pointInRect(m, viewportBox.left, viewportBox.top, viewportBox.width, viewportBox.height)) {
        mSearchFocused = false;
    }

    // Category selection pills from PCategory.txt (left-to-right matching PosProductsView::drawProducts)
    {
        std::vector<std::string> pillNames = {"All"};
        for (const auto& cat : mCategoryManager.getCategories()) {
            if (!cat.name.empty()) pillNames.push_back(cat.name);
        }
        float pillX = mMainX + 94.f;
        float pillY = 313.f;
        for (size_t i = 0; i < pillNames.size(); i++) {
            std::string label = pillNames[i];
            if (label == "Vegetables") label = "Veg";
            float w = getTextWidth(mFontSemiBold, label, 8.f) + 12.f;
            if (pillX + w > mMainX + Layout::MAIN_W + 2.f) break;
            if (pointInRect(m, pillX, pillY, w, 21.f)) {
                mActiveCategory = pillNames[i];
                break;
            }
            pillX += w + 3.f;
        }
    }

    // Product cards click -> add to cart (stock checked!)
    auto visible = mProductCatalog.getVisibleProducts(mSearchQuery, mActiveCategory);
    for (int k = 0; k < (int)visible.size() && k < 6; k++) {
        float cx = mMainX + (k % 3) * (Layout::CARD_W + Layout::CARD_GAP);
        float cy = Layout::CARD_Y + (k / 3) * (Layout::CARD_H + Layout::CARD_GAP);
        if (pointInRect(m, cx, cy, Layout::CARD_W, Layout::CARD_H)) {
            int pid = visible[k];
            int curQty = 0;
            for (const auto& cl : cart) if (cl.productId == pid) curQty = cl.quantity;
            if (curQty + 1 > products[pid].stock) {
                showToast("Stock limit: only " + std::to_string(products[pid].stock) + " available for " + products[pid].name);
            } else {
                mCartService.addItem(pid, 1);
                showToast("Added " + products[pid].name + " to receipt");
            }
        }
    }

    // Drafts / Parked Carts Button
    if (pointInRect(m, mRpX + Layout::RP_W - 76.f, 11.f, 76.f, 22.f)) {
        mDraftsModalOpen = true;
        return;
    }

    // Hold Cart Button (1-click persist on-hold to Order.txt & park draft)
    if (pointInRect(m, mRpX, 38.f, 68.f, 22.f)) {
        holdCurrentOrder();
        return;
    }

    // Clear Cart Button (one-click wipe entire cart)
    if (pointInRect(m, mRpX + 74.f, 38.f, 68.f, 22.f)) {
        if (cart.empty()) {
            showToast("Cart is already empty");
        } else {
            mCartService.clear();
            mIsEditingExistingOrder = false;
            mCurrentEditingOrderId = "";
            mOriginalEditingOrderItems.clear();
            mOriginalEditingOrderStatus = "";
            mSpecialDiscountInputStr = "0";
            mReceiptScrollOffset = 0.f;
            showToast("Whole cart cleared in one click");
        }
        return;
    }

    // Edit Order Button (width 72)
    if (pointInRect(m, mRpX + 148.f, 38.f, 72.f, 22.f)) {
        openEditOrderModal();
        return;
    }

    // Resume Held Order Button (1-click resume, width 76)
    if (pointInRect(m, mRpX + 226.f, 38.f, 76.f, 22.f)) {
        resumeLatestHeldOrder();
        return;
    }

    // Enlarge cart button
    if (pointInRect(m, mRpX + Layout::RP_W - 74.f, Layout::RP_Y + 7.f, 66.f, 22.f)) {
        mEnlargeCartModalOpen = true;
        mEnlargeScrollOffset = 0.f;
        mEnlargeScannerFocused = true;
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

    // Applied Discounts Chip (w=206) -> Opens Discount Modal
    float chipY = Layout::RP_Y + Layout::RP_H + 8.f;
    if (pointInRect(m, mRpX, chipY, 206.f, 22.f)) {
        mDiscountModalOpen = true;
        mDiscountModalError = "";
        mEmployeeDiscountUser = "";
        mEmployeeDiscountPass = "";
        mPromoCodeInput = "";
        mDiscountActiveField = 0;
        return;
    }

    // Payment method buttons
    float btnPayY = chipY + 98.f;
    float payBtnW = (Layout::RP_W - 12.f) / 2.f;
    if (pointInRect(m, mRpX, btnPayY, payBtnW, 30.f)) {
        mCartService.setPaymentMethod(0);
        mCashModalOpen = true;
        mCashInputFocused = true;
        mSpecialDiscountFocused = false;
    }
    if (pointInRect(m, mRpX + payBtnW + 12.f, btnPayY, payBtnW, 30.f)) {
        mCartService.setPaymentMethod(1);
        mCardModalOpen = true;
        mSpecialDiscountFocused = false;
    }

    // Charge / Complete Payment
    float chargeY = btnPayY + 40.f;
    if (pointInRect(m, mRpX, chargeY, Layout::RP_W, 38.f)) {
        if (!mCartService.isPaymentDone() && !cart.empty()) {
            if (mCartService.getPaymentMethod() == 0) {
                mCashModalOpen = true;
                mCashInputFocused = true;
                mSpecialDiscountFocused = false;
            } else {
                mCardModalOpen = true;
                mSpecialDiscountFocused = false;
            }
        }
    }

    // Print Receipt (w=192) & Print PDF (mRpX + 200, w=102)
    float printY = chargeY + 44.f;
    if (mCartService.isPaymentDone()) {
        if (pointInRect(m, mRpX, printY, 192.f, 38.f)) {
            bool ok = false;
            BillSummary sum = computeCurrentBillSummary();
            mCartService.printReceipt(products, mCurrentUserName, ok, &sum);
            if (ok) {
                mReceiptModalOpen = true;
                showToast("Receipt printed and saved to receipt.txt!");
            }
        } else if (pointInRect(m, mRpX + 200.f, printY, 102.f, 38.f)) {
            printBillPdf();
        }
    } else if (pointInRect(m, mRpX, printY, Layout::RP_W, 38.f)) {
        showToast("Payment must be completed before printing receipt!");
    }

    // Start New Sale
    float newSaleY = printY + 44.f;
    if (mCartService.isPaymentDone() && pointInRect(m, mRpX, newSaleY, Layout::RP_W, 32.f)) {
        mCartService.clear();
        mIsEditingExistingOrder = false;
        mCurrentEditingOrderId = "";
        mOriginalEditingOrderItems.clear();
        mOriginalEditingOrderStatus = "";
        mSpecialDiscountInputStr = "0";
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
    } else if (key == sf::Keyboard::C) {
        // Toggle camera when not editing text fields
        bool typingInReturn = (mActiveTab == 6 && mReturnBarcodeFocused);
        bool typingInEnlarge = (mEnlargeCartModalOpen && mEnlargeScannerFocused);
        if (!mSearchFocused && !mCashInputFocused && !mSpecialDiscountFocused && !typingInReturn && !typingInEnlarge &&
            !mAddStaffModalOpen && !mResetPassModalOpen && !mAddInventoryModalOpen && !mQuickCheckModalOpen &&
            !mDiscountModalOpen && !mPromoModalOpen && !mSaleDetailModalOpen && !mEditProductModalOpen &&
            !mEditOrderModalOpen && !mManagerApprovalModalOpen && !mAddCategoryModalOpen && !mAddSeasonModalOpen &&
            !mProductsSearchFocused && !mInventorySearchFocused && !mAddProductSearchFocused) {
            switchCamera();
        }
    } else if (key == sf::Keyboard::Escape) {
        mSearchQuery.clear();
        mSearchFocused = false;
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
        mDiscountModalOpen = false;
        mSaleDetailModalOpen = false;
        mPromoModalOpen = false;
        mEditProductModalOpen = false;
        mEditOrderModalOpen = false;
        mManagerApprovalModalOpen = false;
        mAddCategoryModalOpen = false;
        mAddSeasonModalOpen = false;
        mReturnSlipModalOpen = false;
    }
}

void PosApp::handleTextInput(sf::Uint32 unicode) {
    if (mDiscountModalOpen) {
        if (unicode == 27) { // ESC
            mDiscountModalOpen = false;
            return;
        }
        if (unicode == 9) { // TAB
            if (mDiscountActiveField == 1) mDiscountActiveField = 2;
            else if (mDiscountActiveField == 2) mDiscountActiveField = 3;
            else mDiscountActiveField = 1;
            return;
        }
        if (mDiscountActiveField == 1) { // Employee User
            if (unicode == 8) {
                if (!mEmployeeDiscountUser.empty()) mEmployeeDiscountUser.pop_back();
            } else if (unicode == 13) {
                mDiscountActiveField = 2; // move to password
            } else if (unicode >= 32 && unicode < 127 && mEmployeeDiscountUser.size() < 24) {
                mEmployeeDiscountUser += (char)unicode;
            }
            return;
        }
        if (mDiscountActiveField == 2) { // Employee PIN / Password
            if (unicode == 8) {
                if (!mEmployeeDiscountPass.empty()) mEmployeeDiscountPass.pop_back();
            } else if (unicode == 13) {
                if (mStaffManager.authenticate(mEmployeeDiscountUser, mEmployeeDiscountPass, "Employee") ||
                    mStaffManager.authenticate(mEmployeeDiscountUser, mEmployeeDiscountPass, "Admin")) {
                    mCartService.applyEmployeeDiscount(mEmployeeDiscountUser);
                    mDiscountModalOpen = false;
                    showToast("Employee discount applied for " + mEmployeeDiscountUser + " (" + std::to_string(mCartService.getEmployeeDiscountRate()) + "%)");
                } else {
                    mDiscountModalError = "Invalid employee credentials! Check username & PIN.";
                    showToast("Error: Employee verification failed.");
                }
            } else if (unicode >= 32 && unicode < 127 && mEmployeeDiscountPass.size() < 24) {
                mEmployeeDiscountPass += (char)unicode;
            }
            return;
        }
        if (mDiscountActiveField == 3) { // Promo Code
            if (unicode == 8) {
                if (!mPromoCodeInput.empty()) mPromoCodeInput.pop_back();
            } else if (unicode == 13) {
                std::string msg;
                if (mCartService.applyPromoCode(mPromoCodeInput, msg)) {
                    mDiscountModalOpen = false;
                    showToast(msg);
                } else {
                    mDiscountModalError = msg;
                    showToast(msg);
                }
            } else if (unicode >= 32 && unicode < 127 && mPromoCodeInput.size() < 20) {
                mPromoCodeInput += (char)std::toupper((char)unicode);
            }
            return;
        }
        return;
    }

    if (mSaleDetailModalOpen) {
        if (unicode == 27) { // ESC
            mSaleDetailModalOpen = false;
            return;
        }
        return;
    }

    if (mPromoModalOpen) {
        if (unicode == 27) { // ESC
            mPromoModalOpen = false;
            return;
        }
        if (unicode == 9) { // TAB
            if (mNewPromoActiveField == 0) mNewPromoActiveField = 1;
            else if (mNewPromoActiveField == 1) mNewPromoActiveField = (mNewPromoTargetType == "ALL") ? 0 : 2;
            else mNewPromoActiveField = 0;
            return;
        }
        if (mNewPromoActiveField == 0) { // Code
            if (unicode == 8) {
                if (!mNewPromoCode.empty()) mNewPromoCode.pop_back();
            } else if (unicode == 13) {
                mNewPromoActiveField = 1;
            } else if (unicode >= 32 && unicode < 127 && mNewPromoCode.size() < 20) {
                mNewPromoCode += (char)std::toupper((char)unicode);
            }
            return;
        }
        if (mNewPromoActiveField == 1) { // Discount
            if (unicode == 8) {
                if (!mNewPromoDiscountStr.empty()) mNewPromoDiscountStr.pop_back();
            } else if (unicode == 13) {
                if (mNewPromoTargetType != "ALL") mNewPromoActiveField = 2;
            } else if (unicode >= 32 && unicode < 127 && mNewPromoDiscountStr.size() < 10) {
                mNewPromoDiscountStr += (char)unicode;
            }
            return;
        }
        if (mNewPromoActiveField == 2) { // Target Value
            if (unicode == 8) {
                if (!mNewPromoTargetValue.empty()) mNewPromoTargetValue.pop_back();
            } else if (unicode >= 32 && unicode < 127 && mNewPromoTargetValue.size() < 30) {
                mNewPromoTargetValue += (char)unicode;
            }
            return;
        }
        return;
    }

    if (mQuickCheckModalOpen) {
        if (unicode == 8) { // Backspace
            if (!mQuickCheckBarcode.empty()) {
                mQuickCheckBarcode.pop_back();
                int idx = mProductCatalog.findProductByBarcode(mQuickCheckBarcode);
                if (idx < 0) idx = mProductCatalog.findProductByName(mQuickCheckBarcode);
                mQuickCheckProductIdx = idx;
            }
        } else if (unicode == 13) { // Enter
            if (!mQuickCheckBarcode.empty()) {
                int idx = mProductCatalog.findProductByBarcode(mQuickCheckBarcode);
                if (idx < 0) idx = mProductCatalog.findProductByName(mQuickCheckBarcode);
                mQuickCheckProductIdx = idx;
                if (mQuickCheckProductIdx >= 0) {
                    showToast("Found: " + mProductCatalog.getProducts()[mQuickCheckProductIdx].name);
                } else {
                    showToast("No product found matching '" + mQuickCheckBarcode + "'");
                }
            }
        } else if (unicode >= 32 && unicode < 127 && mQuickCheckBarcode.size() < 30) {
            mQuickCheckBarcode += (char)unicode;
            int idx = mProductCatalog.findProductByBarcode(mQuickCheckBarcode);
            if (idx < 0) idx = mProductCatalog.findProductByName(mQuickCheckBarcode);
            mQuickCheckProductIdx = idx;
            if (mQuickCheckProductIdx >= 0) {
                showToast("Found: " + mProductCatalog.getProducts()[mQuickCheckProductIdx].name);
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

    if (mEditProductModalOpen) {
        if (unicode == 27) { mEditProductModalOpen = false; return; }
        if (unicode == 9) { mEditProductActiveField = (mEditProductActiveField + 1) % 5; return; }
        std::string* targetStr = nullptr;
        if (mEditProductActiveField == 0) targetStr = &mEditProductName;
        else if (mEditProductActiveField == 1) targetStr = &mEditProductPriceStr;
        else if (mEditProductActiveField == 2) targetStr = &mEditProductUnit;
        else if (mEditProductActiveField == 3) targetStr = &mEditProductStockStr;
        else if (mEditProductActiveField == 4) targetStr = &mEditProductDiscountStr;

        if (unicode == 8) {
            if (targetStr && !targetStr->empty()) targetStr->pop_back();
        } else if (unicode == 13) {
            saveEditedProduct();
        } else if (unicode >= 32 && unicode < 127 && targetStr && targetStr->size() < 30) {
            *targetStr += (char)unicode;
        }
        return;
    }

    if (mEditOrderModalOpen) {
        if (unicode == 27) { mEditOrderModalOpen = false; return; }
        if (mEditOrderSearchFocused) {
            if (unicode == 8) {
                if (!mEditOrderSearchId.empty()) mEditOrderSearchId.pop_back();
            } else if (unicode == 13) {
                tryOpenOrderForEditing(mEditOrderSearchId);
            } else if (unicode >= 32 && unicode < 127 && mEditOrderSearchId.size() < 20) {
                mEditOrderSearchId += (char)unicode;
            }
        }
        return;
    }

    if (mManagerApprovalModalOpen) {
        if (unicode == 27) { mManagerApprovalModalOpen = false; return; }
        if (unicode == 8) {
            if (!mApprovalPassInput.empty()) mApprovalPassInput.pop_back();
        } else if (unicode == 13) {
            bool ok = false;
            for (const auto& s : mStaffManager.getStaffList()) {
                if (s.role == "Admin" && mStaffManager.authenticate(s.name, mApprovalPassInput, "Admin")) {
                    ok = true;
                    break;
                }
            }
            if (ok) {
                approveAndLoadOrder();
            } else {
                mApprovalErrorMsg = "Invalid Manager PIN or authorization credentials.";
            }
        } else if (unicode >= 32 && unicode < 127 && mApprovalPassInput.size() < 30) {
            mApprovalPassInput += (char)unicode;
        }
        return;
    }

    if (mAddCategoryModalOpen) {
        if (unicode == 27) { mAddCategoryModalOpen = false; return; }
        if (unicode == 9) { mNewCatActiveField = (mNewCatActiveField + 1) % 3; return; }
        std::string* targetStr = nullptr;
        if (mNewCatActiveField == 0) targetStr = &mNewCatId;
        else if (mNewCatActiveField == 1) targetStr = &mNewCatName;
        else if (mNewCatActiveField == 2) targetStr = &mNewCatAcr;

        if (unicode == 8) {
            if (targetStr && !targetStr->empty()) targetStr->pop_back();
        } else if (unicode == 13) {
            saveNewCategory();
        } else if (unicode >= 32 && unicode < 127 && targetStr && targetStr->size() < 24) {
            *targetStr += (char)unicode;
        }
        return;
    }

    if (mAddSeasonModalOpen) {
        if (unicode == 27) { mAddSeasonModalOpen = false; return; }
        if (unicode == 9) { mNewSeasonActiveField = (mNewSeasonActiveField + 1) % 3; return; }
        std::string* targetStr = nullptr;
        if (mNewSeasonActiveField == 0) targetStr = &mNewSeasonStart;
        else if (mNewSeasonActiveField == 1) targetStr = &mNewSeasonEnd;
        else if (mNewSeasonActiveField == 2) targetStr = &mNewSeasonPctStr;

        if (unicode == 8) {
            if (targetStr && !targetStr->empty()) targetStr->pop_back();
        } else if (unicode == 13) {
            saveNewSeason();
        } else if (unicode >= 32 && unicode < 127 && targetStr && targetStr->size() < 20) {
            *targetStr += (char)unicode;
        }
        return;
    }

    if ((mCashModalOpen || mCardModalOpen) && mSpecialDiscountFocused) {
        if (unicode == 8) {
            if (!mSpecialDiscountInputStr.empty()) mSpecialDiscountInputStr.pop_back();
        } else if (unicode >= '0' && unicode <= '9') {
            std::string temp = mSpecialDiscountInputStr + (char)unicode;
            try {
                int val = std::stoi(temp);
                if (val <= 8) mSpecialDiscountInputStr = temp;
                else showToast("Special discount cannot exceed 8% (Req 8)");
            } catch (...) {}
        } else if (unicode == 13 || unicode == 27) {
            mSpecialDiscountFocused = false;
        }
        return;
    }

    if (mEnlargeCartModalOpen && mEnlargeScannerFocused) {
        if (unicode == 8) {
            if (!mEnlargeScannerQuery.empty()) mEnlargeScannerQuery.pop_back();
        } else if (unicode == 13) {
            std::string q = trimStr(mEnlargeScannerQuery);
            int pIdx = mProductCatalog.findProductByBarcode(q);
            if (pIdx < 0) pIdx = mProductCatalog.findProductByName(q);
            if (pIdx >= 0) {
                onBarcodeScanned(mProductCatalog.getProducts()[pIdx].barcode);
                mEnlargeScannerQuery.clear();
            } else if (!q.empty()) {
                showToast("No product found matching '" + q + "'");
            }
        } else if (unicode >= 32 && unicode < 127 && mEnlargeScannerQuery.size() < 24) {
            mEnlargeScannerQuery += (char)unicode;
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
            finalizePayment(0);
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

    // Cash Sales Custom Date Range Input Box
    if (mActiveTab == 3 && mSalesSubTab == 2 && mCashCustomActiveField != 0) {
        if (unicode == 8) {
            if (mCashCustomActiveField == 1 && !mCashCustomStartDate.empty()) mCashCustomStartDate.pop_back();
            else if (mCashCustomActiveField == 2 && !mCashCustomEndDate.empty()) mCashCustomEndDate.pop_back();
        } else if (unicode == 13 || unicode == 27) {
            mCashCustomActiveField = 0;
        } else if ((unicode >= '0' && unicode <= '9') || unicode == '-') {
            if (mCashCustomActiveField == 1 && mCashCustomStartDate.length() < 10) mCashCustomStartDate += (char)unicode;
            else if (mCashCustomActiveField == 2 && mCashCustomEndDate.length() < 10) mCashCustomEndDate += (char)unicode;
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

    // Auto-focus POS barcode search on digit scan (0-click hardware USB scanner support - Req 10)
    if (!mSearchFocused) {
        if (mActiveTab == 0 && unicode >= '0' && unicode <= '9') {
            mSearchFocused = true;
            mSearchQuery += (char)unicode;
        }
        return;
    }

    if (unicode == 8) {
        if (!mSearchQuery.empty()) mSearchQuery.pop_back();
    } else if (unicode == 13) {
        std::string q = toLowerStr(trimStr(mSearchQuery));
        int idx = mProductCatalog.findProductByBarcode(q);
        if (idx < 0) {
            idx = mProductCatalog.findProductByName(q);
        }
        if (idx >= 0) {
            simulateScan(idx);
            mSearchQuery.clear();
            return;
        }
        auto vis = mProductCatalog.getVisibleProducts(mSearchQuery, "All");
        if (!vis.empty()) {
            simulateScan(vis[0]);
            mSearchQuery.clear();
            return;
        }
        showToast("No product found matching '" + mSearchQuery + "'");
    } else if (unicode >= 32 && unicode < 127 && mSearchQuery.size() < 40) {
        mSearchQuery += (char)unicode;
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
                                     mCategoryManager.getCategories(),
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
                                     mCurrentRole, mSalesSubTab, mReturnHistory,
                                     mSalesTimeFilter, mSalesScrollOffset,
                                     mOrders, mOrderDetails, mBills, mBillDetails,
                                     mCashCustomStartDate, mCashCustomEndDate, mCashCustomActiveField,
                                     mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 4) {
        PosReportsView::drawActivityLogsTab(mWindow, m, mMainX, mActivityLogger.getLogs(), mLogFilter, mLogScrollOffset,
                                            mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 5) {
        PosReportsView::drawSettingsTab(mWindow, m, mMainX, mStaffManager.getStaffList(), mCurrentRole, mCurrentUserName,
                                        mCameraRunning, mSoundEnabled,
                                        mCameraStream.getCameraName(), mCameraStream.isActiveCameraExternal(),
                                        CameraStream::getAvailableCameraCount(),
                                        mCartService.getStudentDiscountRate(), mCartService.getEmployeeDiscountRate(),
                                        mCartService.getPromoCodesCount(),
                                        mSettingsSubTab, mCategoryManager, mSeasonManager, mSettingsScrollOffset,
                                        mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 6) {
        const ProductItem* pPtr = (mSelectedReturnProductIdx >= 0 && mSelectedReturnProductIdx < (int)mProductCatalog.getProducts().size())
                                ? &mProductCatalog.getProducts()[mSelectedReturnProductIdx] : nullptr;
        PosReportsView::drawReturnTab(mWindow, m, mMainX,
                                      mCameraTexture,
                                      mCameraBarcodeFound, mBarcodeBox, mCamWidth, mCamHeight, mDetectedBarcode,
                                      mCameraStream.getCameraName(), mCameraStream.isActiveCameraExternal(),
                                      CameraStream::getAvailableCameraCount(),
                                      mReturnBarcode, mReturnBarcodeFocused,
                                      mReturnQty, mReturnMaxQty, pPtr, mReturnSaleInfo, mReturnSalePrice,
                                      mReturnOrderId, mReturnCustomer, mReturnDate,
                                      mReturnSuccess, mReturnStatusMessage,
                                      mReturnReasonIdx, mReturnPaymentMethod, mReturnRestock,
                                      mOrders, mReturnOrderItemPills, !mReturnSlipText.empty(),
                                      mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    }

    // Receipt Drawer (Single-source-of-truth BillSummary including Tax, Promo, Seasonal & Special)
    const auto& prods = mProductCatalog.getProducts();
    BillSummary billSummary = computeCurrentBillSummary();
    PosReceiptView::drawReceiptDrawer(mWindow, m, mPanelX, mRpX, mDesignHeight,
                                     mCartService.getCart(), prods,
                                     billSummary,
                                     mCartService.isDiscountApplied(),
                                     mCartService.getDiscountLabel(),
                                     mCartService.getPaymentMethod(),
                                     mCartService.isPaymentDone(),
                                     mReceiptScrollOffset,
                                     (int)mCartService.getDrafts().size(),
                                     mIsEditingExistingOrder,
                                     mCurrentEditingOrderId,
                                     mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);

    // Modals
    if (mDiscountModalOpen) {
        PosModalsView::drawDiscountModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                        mCartService,
                                        mCartService.getStudentDiscountRate(),
                                        mCartService.getEmployeeDiscountRate(),
                                        billSummary.seasonalPercent,
                                        mEmployeeDiscountUser, mEmployeeDiscountPass, mDiscountActiveField,
                                        mPromoCodeInput, mDiscountModalError,
                                        mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mQuickCheckModalOpen) {
        const ProductItem* pPtr = (mQuickCheckProductIdx >= 0 && mQuickCheckProductIdx < (int)prods.size())
                                ? &prods[mQuickCheckProductIdx] : nullptr;
        PosModalsView::drawQuickCheckModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                          mCameraTexture,
                                          mQuickCheckBarcode, mQuickCheckInputFocused, pPtr,
                                          mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mSpecModalOpen && mSpecProductIdx >= 0 && mSpecProductIdx < (int)prods.size()) {
        PosModalsView::drawProductSpecModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                           prods[mSpecProductIdx],
                                           mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mReceiptModalOpen) {
        PosModalsView::drawReceiptModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                       mCartService, billSummary, prods, mCurrentUserName,
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
                                           billSummary,
                                           mEnlargeScrollOffset,
                                           mCameraTexture,
                                           mEnlargeScannerQuery, mEnlargeScannerFocused,
                                           mLastScannedInfo, mCameraStream.getCameraName(),
                                           mFontRegular, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mCashModalOpen) {
        PosModalsView::drawCashModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                    billSummary,
                                    mCartService.parseCashGiven(),
                                    mCartService.getCashGivenStr(),
                                    mCashInputFocused,
                                    mSpecialDiscountInputStr, mSpecialDiscountFocused,
                                    mFontRegular, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mCardModalOpen) {
        PosModalsView::drawCardModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                    billSummary,
                                    mSpecialDiscountInputStr, mSpecialDiscountFocused,
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
                                            mCameraTexture,
                                            mNewItemSupplier, mNewItemDesc, mNewItemCategory,
                                            mCategoryManager.getCategories(),
                                            mNewItemUnit,
                                            mNewItemPriceStr, mNewItemCostStr, mNewItemQtyStr, mNewItemDateStr,
                                            mNewItemBarcode, mNewItemActiveField, mNewItemGenerated,
                                            mInventoryManager.getUniqueSuppliers(), mVendorDropdownOpen,
                                            mScanningUniversalSupplier, mNewItemAlsoAddToCatalog,
                                            mCameraStream.getCameraName(),
                                            mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mSaleDetailModalOpen) {
        const auto& sales = mCartService.getSalesHistory();
        if (mSelectedSaleIndex >= 0 && mSelectedSaleIndex < (int)sales.size()) {
            PosModalsView::drawSaleDetailModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                               sales[mSelectedSaleIndex], prods,
                                               mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);
        }
    } else if (mPromoModalOpen) {
        PosModalsView::drawPromoManagementModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                                mCartService.getPromoCodes(),
                                                mNewPromoCode, mNewPromoDiscountStr,
                                                mNewPromoTargetType, mNewPromoTargetValue,
                                                mNewPromoActiveField,
                                                mInventoryManager.getUniqueSuppliers(),
                                                mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mEditProductModalOpen) {
        PosModalsView::drawEditProductModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                           mEditProductName, mEditProductPriceStr,
                                           mEditProductUnit, mEditProductStockStr,
                                           mEditProductCategory, mEditProductCatId,
                                           mEditProductDiscountStr, mEditProductActiveField,
                                           mCategoryManager.getCategories(),
                                           mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mEditOrderModalOpen) {
        PosModalsView::drawEditOrderModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                         mOrders, mEditOrderSearchId, mEditOrderSearchFocused,
                                         mEditOrderStatusMsg, mEditOrderScrollOffset,
                                         mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mManagerApprovalModalOpen) {
        PosModalsView::drawManagerApprovalModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                               mApprovalTargetOrderId, mApprovalTargetOrderDate,
                                               mApprovalPassInput, mApprovalErrorMsg,
                                               mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mAddCategoryModalOpen) {
        PosModalsView::drawAddCategoryModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                           mNewCatId, mNewCatName, mNewCatAcr, mNewCatActiveField,
                                           mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mAddSeasonModalOpen) {
        PosModalsView::drawAddSeasonModal(mWindow, m, mCurrentWidth, mDesignHeight,
                                         mNewSeasonStart, mNewSeasonEnd, mNewSeasonPctStr, mNewSeasonActiveField,
                                         mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mReturnSlipModalOpen && !mReturnSlipText.empty()) {
        sf::RectangleShape overlay({mCurrentWidth, mDesignHeight});
        overlay.setFillColor(sf::Color(0, 0, 0, 140));
        mWindow.draw(overlay);

        float mw = 440.f;
        float mh = 510.f;
        float mx = (mCurrentWidth - mw) / 2.f;
        float my = (mDesignHeight - mh) / 2.f;

        drawShadow(mWindow, mx, my, mw, mh, 12, 6, 12.f, 25, 4.f);
        drawRounded(mWindow, mx, my, mw, mh, 12, ThemeColor::Paper, ThemeColor::Line, 1.2f);

        drawText(mWindow, mFontSerif, "Customer Return Slip Proof", 18, ThemeColor::Ink, mx + mw / 2.f, my + 24, TextAlignment::Center);

        float paperY = my + 44.f;
        float paperW = mw - 36.f;
        float paperH = 400.f;
        drawRounded(mWindow, mx + 18, paperY, paperW, paperH, 8, sf::Color(254, 252, 248), ThemeColor::Line, 1.f);

        std::istringstream iss(mReturnSlipText);
        std::string sLine;
        float lY = paperY + 14.f;
        while (std::getline(iss, sLine)) {
            if (sLine.empty()) continue;
            bool isHeader = (sLine.find("===") != std::string::npos || sLine.find("---") != std::string::npos);
            bool isTitle = (sLine.find("SCANIT") != std::string::npos || sLine.find("RECEIPT") != std::string::npos || sLine.find("OFFICIAL") != std::string::npos);
            bool isTotal = (sLine.find("TOTAL") != std::string::npos);

            if (isHeader) {
                sf::RectangleShape divLine({paperW - 24.f, 1.f});
                divLine.setPosition(mx + 30, lY + 6.f);
                divLine.setFillColor(ThemeColor::Line);
                mWindow.draw(divLine);
                lY += 13.f;
            } else if (isTitle) {
                drawText(mWindow, mFontBold, sLine, 9.5f, ThemeColor::Ink, mx + 18 + paperW / 2.f, lY, TextAlignment::Center);
                lY += 15.f;
            } else if (isTotal) {
                drawText(mWindow, mFontBold, sLine, 10.5f, sf::Color(180, 83, 9), mx + 30, lY);
                lY += 17.f;
            } else {
                drawText(mWindow, mFontMedium, sLine, 8.5f, ThemeColor::Ink, mx + 30, lY);
                lY += 15.f;
            }
        }

        // Close button
        float btnW = 120.f;
        float btnH = 30.f;
        float btnX = mx + (mw - btnW) / 2.f;
        float btnY = my + mh - 42.f;
        bool hovClose = pointInRect(m, btnX, btnY, btnW, btnH);
        drawRounded(mWindow, btnX, btnY, btnW, btnH, 6, hovClose ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(mWindow, mFontBold, "Close (ESC)", 9.5f, ThemeColor::White, btnX + btnW / 2.f, btnY + 10.f, TextAlignment::Center);
    }

    // Toast Notification
    if (mToastTimer > 0.f && !mToastMessage.empty()) {
        float alpha = std::min(1.f, mToastTimer / 0.3f);
        float tw = getTextWidth(mFontMedium, mToastMessage, 11) + 32.f;
        float tx = (mCurrentWidth - tw) / 2.f;
        float ty = 8.f;

        drawShadow(mWindow, tx, ty, tw, 28.f, 10, 4, 10.f, (int)(18 * alpha), 2);
        drawRounded(mWindow, tx, ty, tw, 28.f, 6, sf::Color(32, 28, 26, (sf::Uint8)(240 * alpha)));
        drawText(mWindow, mFontMedium, mToastMessage, 10.5f, sf::Color(255, 255, 255, (sf::Uint8)(255 * alpha)),
                 tx + tw / 2.f, ty + 14.f, TextAlignment::Center);
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

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
    if (tx.loadFromFile(fullPath)) {
        tx.setSmooth(true);
        tx.generateMipmap();
    }
    return tx;
}

void PosApp::updateLayout(float winW, float winH) {
    if (winH <= 0.f) winH = 1.f;
    float aspect = winW / winH;
    mCurrentWidth = std::max(1280.f, mDesignHeight * aspect);
    mPanelX = mCurrentWidth - Layout::PANEL_W;
    mRpX = mPanelX + 17.f;
    mMainX = mPanelX - 21.f - Layout::MAIN_W;

    sf::View view(sf::FloatRect(0, 0, mCurrentWidth, mDesignHeight));
    view.setViewport({0.f, 0.f, 1.f, 1.f});
    mWindow.setView(view);
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

    // Set initial cash given from cart total
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

    // Start background video stream
    mCameraRunning = mCameraStream.start(0);

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
                mCartService.printReceipt(mProductCatalog.getProducts(), "Muya Khan", ok);
                mReceiptModalOpen = false;
            }

            if (mModalShot == "receipt") {
                mCartService.setCashGiven(1500);
                std::string msg;
                mCartService.chargeSale(mProductCatalog.getProducts(), mCurrentUserName, msg);
                bool ok = false;
                mCartService.printReceipt(mProductCatalog.getProducts(), "Muya Khan", ok);
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

    return true;
}

void PosApp::showToast(const std::string& message) {
    mToastMessage = message;
    mToastTimer = 2.2f;
}

void PosApp::onBarcodeScanned(const std::string& barcode) {
    int idx = mProductCatalog.findProductByBarcode(barcode);
    if (idx >= 0) {
        const auto& p = mProductCatalog.getProducts()[idx];
        mCartService.addItem(idx, 1);
        mFlashTimer = 0.4f;
        mLastScannedInfo = p.name + " (" + formatRs(p.price) + ")";
        mLastScanTimer = 3.0f;
        showToast("Scanned: " + p.name + " (" + formatRs(p.price) + ")");
    } else {
        showToast("Barcode " + barcode + " not in product.txt catalog");
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

    if (mEnlargeCartModalOpen) {
        float maxScroll = std::max(0.f, (float)(cart.size() - 6) * 56.f);
        mEnlargeScrollOffset = std::clamp(mEnlargeScrollOffset - delta * 36.f, 0.f, maxScroll);
        return;
    }

    if (mActiveTab == 1) {
        float maxScroll = std::max(0.f, (float)(products.size() - 8) * 46.f);
        mProductsScrollOffset = std::clamp(mProductsScrollOffset - delta * 30.f, 0.f, maxScroll);
        return;
    } else if (mActiveTab == 2) {
        float maxScroll = std::max(0.f, (float)(products.size() - 7) * 54.f);
        mInventoryScrollOffset = std::clamp(mInventoryScrollOffset - delta * 30.f, 0.f, maxScroll);
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
        float dw = 620.f, dh = 580.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;
        if (pointInRect(m, dx + 40, dy + dh - 58, 240, 42)) {
            mReceiptModalOpen = false;
            return;
        }
        if (pointInRect(m, dx + dw - 280, dy + dh - 58, 240, 42)) {
            mReceiptModalOpen = false;
            mCartService.clear();
            showToast("Ready for new sale transaction");
            return;
        }
        if (!pointInRect(m, dx, dy, dw, dh)) {
            mReceiptModalOpen = false;
        }
        return;
    }

    // 2. Enlarge Cart Modal
    if (mEnlargeCartModalOpen) {
        float dw = 840.f, dh = 580.f;
        float dx = (mCurrentWidth - dw) / 2.f, dy = (mDesignHeight - dh) / 2.f;

        if (pointInRect(m, dx + dw - 44, dy + 20, 28, 28)) {
            mEnlargeCartModalOpen = false;
            return;
        }

        float tableY = dy + 110.f;
        for (int i = 0; i < (int)cart.size(); i++) {
            float rowY = tableY + i * 56.f - mEnlargeScrollOffset;
            if (rowY < tableY - 10.f || rowY > tableY + 330.f) continue;

            if (pointInRect(m, dx + 510, rowY + 12, 24, 24)) {
                mCartService.setItemQuantity(i, cart[i].quantity - 1);
                return;
            }
            if (pointInRect(m, dx + 580, rowY + 12, 24, 24)) {
                mCartService.setItemQuantity(i, cart[i].quantity + 1);
                return;
            }
            if (pointInRect(m, dx + dw - 95, rowY + 12, 60, 24)) {
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

        const int presets[] = {1, 2, 3, 5, 10};
        for (int i = 0; i < 5; i++) {
            float bx = dx + 20 + i * 56;
            float by = dy + 116;
            if (pointInRect(m, bx, by, 48, 28)) {
                mCartService.setItemQuantity(mEditingCartIndex, presets[i]);
                showToast("Quantity updated to " + std::to_string(presets[i]));
                mQtyModalOpen = false;
                return;
            }
        }

        if (pointInRect(m, dx + 20, dy + dh - 48, 130, 34)) {
            int q = 1;
            try { q = std::stoi(mQtyInputStr); } catch (...) {}
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

    // Logout / Switch User profile button
    if (pointInRect(m, 4, 640, 64, 70)) {
        mShowLogin = true;
        mLoginScreen.reset();
        showToast("Logged out to sign-in terminal");
        return;
    }

    // Navigation Sidebar
    bool isAdmin = (mCurrentRole == "Admin");
    std::vector<int> visibleTabs = isAdmin ? std::vector<int>{0, 1, 2, 3, 4, 5} : std::vector<int>{0, 1, 3};

    for (size_t slot = 0; slot < visibleTabs.size(); slot++) {
        float cy = Layout::NAV_Y[slot];
        if (pointInRect(m, 4, cy - 24, 60, 48)) {
            mActiveTab = visibleTabs[slot];
            return;
        }
    }

    if (mActiveTab != 0) {
        if (mActiveTab == 1) { // Products Tab
            if (pointInRect(m, mMainX + Layout::MAIN_W - 150, 50, 150, 34)) {
                if (mCurrentRole != "Admin") {
                    showToast("Restricted: Admin authorization required to add products to catalog");
                } else {
                    showToast("Edit product.txt to add or update items");
                }
                return;
            }
            for (size_t i = 0; i < products.size(); i++) {
                float rowY = 100.f + 46.f + static_cast<float>(i) * 46.f - mProductsScrollOffset;
                if (pointInRect(m, mMainX + 460, rowY - 8, 55, 22)) {
                    if (mCurrentRole != "Admin") {
                        showToast("Restricted: Admin authorization required to delete products");
                    } else {
                        mActivityLogger.logActivity("PRODUCT", mCurrentUserName, "Deleted product: " + products[i].name);
                        showToast("Deleted " + products[i].name);
                        mProductCatalog.deleteProduct((int)i);
                    }
                    return;
                }
            }
        } else if (mActiveTab == 2) { // Inventory Tab
            float listStartY = (mCurrentRole != "Admin") ? (155.f + 48.f) : (155.f + 20.f);
            for (size_t i = 0; i < products.size(); i++) {
                float rowY = listStartY + 15.f + static_cast<float>(i) * 54.f - mInventoryScrollOffset;
                if (pointInRect(m, mMainX + 375, rowY - 4, 42, 24)) {
                    if (mCurrentRole != "Admin") {
                        showToast("Restricted: Stock replenishment requires Admin authorization");
                    } else {
                        mProductCatalog.addStock((int)i, 10);
                        mActivityLogger.logActivity("INVENTORY", mCurrentUserName, "Restocked +10 units of " + products[i].name);
                        showToast("+10 added to " + products[i].name);
                    }
                    return;
                }
                if (pointInRect(m, mMainX + 425, rowY - 4, 42, 24)) {
                    if (mCurrentRole != "Admin") {
                        showToast("Restricted: Stock replenishment requires Admin authorization");
                    } else {
                        mProductCatalog.addStock((int)i, 50);
                        mActivityLogger.logActivity("INVENTORY", mCurrentUserName, "Restocked +50 units of " + products[i].name);
                        showToast("+50 added to " + products[i].name);
                    }
                    return;
                }
                if (pointInRect(m, mMainX + 475, rowY - 4, 55, 24)) {
                    if (mCurrentRole != "Admin") {
                        showToast("Restricted: Stock replenishment requires Admin authorization");
                    } else {
                        mProductCatalog.addStock((int)i, 25);
                        mActivityLogger.logActivity("INVENTORY", mCurrentUserName, "Restocked +25 units of " + products[i].name);
                        showToast("+25 added to " + products[i].name);
                    }
                    return;
                }
            }
        } else if (mActiveTab == 4) { // Activity Logs Tab
            const char* filters[] = {"ALL", "LOGIN", "SALE", "STAFF", "INVENTORY"};
            for (int i = 0; i < 5; i++) {
                if (pointInRect(m, mMainX + i * 92, 102, 82, 26)) {
                    mLogFilter = filters[i];
                    return;
                }
            }
        } else if (mActiveTab == 5) { // Settings Tab
            float ty = 105.f;
            float staffY = ty + 155.f;
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
                    if (pointInRect(m, mMainX + 360, sRowY, 78, 22)) {
                        mResetPassModalOpen = true;
                        mResetStaffTarget = staffList[i].name;
                        mResetStaffNewPass = "";
                        mResetPassFocused = true;
                        return;
                    }
                    if (staffList[i].name != mCurrentUserName && pointInRect(m, mMainX + 446, sRowY, 64, 22)) {
                        std::string remName = staffList[i].name;
                        mStaffManager.removeStaffMember(remName);
                        mActivityLogger.logActivity("STAFF", mCurrentUserName, "Removed staff account: " + remName);
                        showToast("Staff account removed: " + remName);
                        return;
                    }
                }
            }
            float utilY = ty + 380.f;
            if (pointInRect(m, mMainX + 25, utilY, 220, 34)) {
                mProductCatalog.loadProductsFile();
                showToast("product.txt reloaded from disk");
                return;
            }
            if (pointInRect(m, mMainX + 265, utilY, 245, 34)) {
                showToast("Barcode bitmaps synchronized");
                return;
            }
        }
        return;
    }

    // POS Tab Controls
    sf::FloatRect searchBox{mMainX, 82.f, 415.f, 39.f};
    sf::FloatRect addBtn{mMainX + 425.f, 82.f, 115.f, 39.f};
    sf::FloatRect viewportBox{mMainX, 133.f, Layout::MAIN_W, 160.f};

    mSearchFocused = pointInRect(m, searchBox.left, searchBox.top, searchBox.width, searchBox.height);

    if (pointInRect(m, addBtn.left, addBtn.top, addBtn.width, addBtn.height)) {
        if (mCurrentRole != "Admin") {
            showToast("Restricted: Admin login required to add products");
            return;
        }
        mSearchFocused = true;
        showToast("Type product name or barcode, then Enter");
    }

    if (pointInRect(m, viewportBox.left, viewportBox.top, viewportBox.width, viewportBox.height)) {
        simulateScan();
    }

    // Category selection pills
    float pillRight = mMainX + Layout::MAIN_W;
    for (int i = 4; i >= 0; i--) {
        float w = getTextWidth(mFontSemiBold, Layout::CATEGORIES[i], 10) + 20.f;
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

        float delX = mRpX + 7.f;
        float delY = y0 + 30.f;
        if (pointInRect(m, delX, delY, 26.f, 13.f)) {
            std::string n = products[cart[i].productId].name;
            mCartService.deleteItem(i);
            showToast("Deleted " + n + " from receipt");
            return;
        }

        if (pointInRect(m, mRpX + 126.f, y0 + 11.f, 16.f, 16.f)) {
            mCartService.setItemQuantity(i, cart[i].quantity - 1);
            return;
        }

        if (pointInRect(m, mRpX + 144.f, y0 + 11.f, 17.f, 16.f)) {
            mEditingCartIndex = i;
            mQtyModalOpen = true;
            mQtyInputStr = std::to_string(cart[i].quantity);
            return;
        }

        if (pointInRect(m, mRpX + 163.f, y0 + 11.f, 16.f, 16.f)) {
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
    if (pointInRect(m, mRpX, btnPayY, 110.f, 30.f)) {
        mCartService.setPaymentMethod(0);
        mCashModalOpen = true;
        mCashInputFocused = true;
    }
    if (pointInRect(m, mRpX + 122.f, btnPayY, 110.f, 30.f)) {
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
        mCartService.printReceipt(products, "Muya Khan", ok);
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

void PosApp::handleKeyPress(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::F2) {
        simulateScan();
    } else if (key == sf::Keyboard::Escape) {
        mSearchQuery.clear();
        mSearchFocused = false;
        mCashInputFocused = false;
        mReceiptModalOpen = false;
        mQtyModalOpen = false;
        mEnlargeCartModalOpen = false;
        mCashModalOpen = false;
        mCardModalOpen = false;
        mAddStaffModalOpen = false;
        mResetPassModalOpen = false;
    }
}

void PosApp::handleTextInput(sf::Uint32 unicode) {
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
                mActivityLogger.logActivity("SALE", mCurrentUserName, "Processed cash sale of " + formatRs(mCartService.getTotal(mProductCatalog.getProducts())));
                showToast(msg);
                mCashModalOpen = false;
            } else {
                showToast(msg);
            }
        }
        return;
    }

    if (!mSearchFocused) return;

    if (unicode == 8) {
        if (!mSearchQuery.empty()) mSearchQuery.pop_back();
    } else if (unicode == 13) {
        std::string q = toLowerStr(mSearchQuery);
        int idx = mProductCatalog.findProductByBarcode(q);
        if (idx >= 0) {
            simulateScan(idx);
            mSearchQuery.clear();
            return;
        }
        auto vis = mProductCatalog.getVisibleProducts(mSearchQuery, mActiveCategory);
        if (vis.size() == 1) {
            simulateScan(vis[0]);
            mSearchQuery.clear();
        }
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
                        if (mScanCooldown <= 0.f) {
                            onBarcodeScanned(detectedCode);
                            mScanCooldown = 1.6f;
                        }
                    } else {
                        mCameraBarcodeFound = false;
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

    // Draw Classical Watercolor Artwork Background
    const sf::Texture& bg = getTexture("art/bg_art_hd.png");
    if (bg.getSize().x > 0) {
        sf::Sprite bgSprite(bg);
        float s = mDesignHeight / (float)bg.getSize().y;
        bgSprite.setScale(s, s);
        bgSprite.setPosition(Layout::SIDE_W + 12.f, 0.f);
        mWindow.draw(bgSprite);
    } else {
        const sf::Texture& bgFallback = getTexture("art/bg_art.png");
        if (bgFallback.getSize().x > 0) {
            sf::Sprite bgSprite(bgFallback);
            float s = mDesignHeight / (float)bgFallback.getSize().y;
            bgSprite.setScale(s, s);
            bgSprite.setPosition(Layout::SIDE_W + 12.f, 0.f);
            mWindow.draw(bgSprite);
        }
    }

    sf::Vector2f m = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow));
    auto getTx = [this](const std::string& rel) -> const sf::Texture& { return getTexture(rel); };

    // Navigation and Shell
    PosNavigation::drawSidebar(mWindow, m, mDesignHeight, mActiveTab, mCurrentRole, mCurrentUserName,
                               mFontRegular, mFontMedium, mFontSemiBold, getTx);
    PosNavigation::drawHeader(mWindow, mMainX, mCurrentRole, mCurrentUserName,
                              mFontRegular, mFontMedium, mFontSemiBold, mFontSerif);

    if (mActiveTab == 0) {
        // POS Cash Register Mode
        PosNavigation::drawSearch(mWindow, m, mMainX, mSearchQuery, mSearchFocused, mFontRegular, mFontSemiBold, getTx);
        PosProductsView::drawViewport(mWindow, mMainX, mCameraTexture, mFlashTimer, mLastScanTimer, mLastScannedInfo, mFontSemiBold, getTx);
        auto visible = mProductCatalog.getVisibleProducts(mSearchQuery, mActiveCategory);
        PosProductsView::drawProducts(mWindow, m, mMainX, mProductCatalog.getProducts(), visible, mActiveCategory,
                                     mFontRegular, mFontSemiBold, mFontBold, mFontSerif, getTx);
    } else if (mActiveTab == 1) {
        PosProductsView::drawProductsTab(mWindow, m, mMainX, mProductCatalog.getProducts(), mProductsScrollOffset,
                                         mCurrentRole, mProductCatalog.getProductFilePath(),
                                         mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 2) {
        PosProductsView::drawInventoryTab(mWindow, m, mMainX, mProductCatalog.getProducts(), mInventoryScrollOffset,
                                          mCurrentRole, mFontRegular, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 3) {
        PosReportsView::drawSalesTab(mWindow, m, mMainX, mCartService.getSalesHistory(), mSalesScrollOffset,
                                     mFontRegular, mFontBold, mFontSerif);
    } else if (mActiveTab == 4) {
        PosReportsView::drawActivityLogsTab(mWindow, m, mMainX, mActivityLogger.getLogs(), mLogFilter, mLogScrollOffset,
                                            mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
    } else if (mActiveTab == 5) {
        PosReportsView::drawSettingsTab(mWindow, m, mMainX, mStaffManager.getStaffList(), mCurrentRole, mCurrentUserName,
                                        mCameraRunning, mSoundEnabled,
                                        mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif);
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
                                     mFontRegular, mFontMedium, mFontSemiBold, mFontBold, mFontSerif, getTx);

    // Modals
    if (mReceiptModalOpen) {
        PosModalsView::drawReceiptModal(mWindow, m, mCurrentWidth, mDesignHeight, mCartService.getLastReceiptText(),
                                       mFontRegular, mFontSemiBold, mFontSerif, getTx);
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

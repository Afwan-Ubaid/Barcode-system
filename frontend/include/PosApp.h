#ifndef POS_APP_H
#define POS_APP_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

#include "PosModels.h"
#include "UIUtils.h"
#include "StaffManager.h"
#include "ActivityLogger.h"
#include "ProductCatalog.h"
#include "InventoryManager.h"
#include "CartService.h"
#include "SplashScreen.h"
#include "LoginScreen.h"
#include "../../backend/include/CameraCapture.h"
#include "../../backend/include/Decoder.h"
#include "../../backend/include/Generator.h"
#include "../../backend/include/Validator.h"
#include "../../backend/include/PaymentStrategy.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: High Cohesion & Loose Coupling
// Module: POS Application Master Controller & View Orchestrator
// ============================================================================

class PosApp {
public:
    PosApp();
    ~PosApp();

    bool init(bool screenshotMode = false, bool paidMode = false, const std::string& modalShot = "");
    void run();

    const sf::Texture& getTexture(const std::string& relPath);
    std::string getAssetPath(const std::string& relPath) const;

private:
    void handleEvents();
    void update(float dt);
    void render();

    // Event & User Interaction Handlers
    void handleClick(sf::Vector2f m);
    void handleScroll(float delta, sf::Vector2f m);
    void handleKeyPress(sf::Keyboard::Key key);
    void handleTextInput(sf::Uint32 unicode);
    void simulateScan(int productId = -1);
    void onBarcodeScanned(const std::string& barcode);
    void lookupReturnItem(const std::string& barcode);
    void processReturn();
    void recordSaleToOutputFiles(const std::vector<CartLine>& cartItems, const std::string& cashierName, int paymentMethod);
    void generateNewItemBarcode();
    void saveNewInventoryItem();

    void showToast(const std::string& message);
    void switchCamera();
    void updateLayout(float winW, float winH);

    // Domain Managers & Services (Separation of Concerns)
    StaffManager mStaffManager;
    ActivityLogger mActivityLogger;
    ProductCatalog mProductCatalog;
    InventoryManager mInventoryManager;
    CartService mCartService;

    // Window & Display Layout
    sf::RenderWindow mWindow;
    float mDesignWidth = 1280.f;
    float mDesignHeight = 720.f;
    float mCurrentWidth = 1280.f;
    float mMainX = 448.f;
    float mPanelX = 1014.f;
    float mRpX = 1031.f;

    // Fonts & Texture Cache
    sf::Font mFontRegular;
    sf::Font mFontMedium;
    sf::Font mFontSemiBold;
    sf::Font mFontBold;
    sf::Font mFontSerif;
    std::map<std::string, sf::Texture> mTextures;
    std::string mAssetsDir;

    // Authentication & Splash Sequences
    SplashScreen mSplashScreen;
    LoginScreen mLoginScreen;
    bool mShowSplash = true;
    bool mShowLogin = false;
    std::string mCurrentRole = "Admin";
    std::string mCurrentUserName = "admin";

    // Application Navigation & Operational State
    int mActiveTab = 0;
    std::string mSearchQuery;
    std::string mActiveCategory = "All";
    std::string mToastMessage;
    float mToastTimer = 0.f;
    float mFlashTimer = 0.f;
    float mScanCooldown = 0.f;
    float mDetectThrottleTimer = 0.f;
    bool mSearchFocused = false;
    bool mCashInputFocused = false;
    int mScanSimIndex = 0;
    bool mScreenshotMode = false;
    std::string mModalShot = "";
    std::string mLastScannedInfo;
    float mLastScanTimer = 0.f;
    bool mSoundEnabled = true;

    // Search Fields in Products & Inventory Tabs
    std::string mProductsSearchQuery = "";
    bool mProductsSearchFocused = false;
    std::string mInventorySearchQuery = "";
    bool mInventorySearchFocused = false;
    int mSalesTimeFilter = 0; // 0 = All, 1 = Today, 2 = This Week, 3 = This Month

    // Scrolling Offsets
    float mReceiptScrollOffset = 0.f;
    float mEnlargeScrollOffset = 0.f;
    float mProductsScrollOffset = 0.f;
    float mInventoryScrollOffset = 0.f;
    float mSalesScrollOffset = 0.f;
    float mLogScrollOffset = 0.f;
    float mAddProductScrollOffset = 0.f;

    // Modals Visibility
    bool mReceiptModalOpen = false;
    bool mQtyModalOpen = false;
    bool mEnlargeCartModalOpen = false;
    bool mCashModalOpen = false;
    bool mCardModalOpen = false;
    bool mAddStaffModalOpen = false;
    bool mResetPassModalOpen = false;
    bool mAddInventoryModalOpen = false;
    bool mItemDescModalOpen = false;
    bool mAddProductModalOpen = false;
    bool mDraftsModalOpen = false;

    // Inventory View Sub-Tabs & Supplier Ledger
    int mInventorySubTab = 0; // 0 = Stock & Items, 1 = Supplier Ledger
    float mSupplierScrollOffset = 0.f;

    // Add Inventory & Supplier Modal Input States
    std::string mNewItemSupplier = "Nestle Pakistan";
    std::string mNewItemDesc = "";
    std::string mNewItemCategory = "Grocery";
    std::string mNewItemUnit = "1 L";
    std::string mNewItemPriceStr = "320";
    std::string mNewItemCostStr = "250";
    std::string mNewItemQtyStr = "30";
    std::string mNewItemDateStr = "2026-11-17";
    std::string mNewItemBarcode = "";
    int mNewItemActiveField = 1;
    bool mNewItemGenerated = false;
    bool mVendorDropdownOpen = false;
    bool mScanningUniversalSupplier = false;

    // Item Description Modal Details
    std::string mDescItemName = "";
    std::string mDescItemBarcode = "";
    std::string mDescItemCategory = "";
    std::string mDescItemUnit = "";
    int mDescItemPrice = 0;
    int mDescItemCost = 0;
    int mDescItemStock = 0;
    std::string mDescItemSupplier = "";
    std::string mDescItemDate = "";

    // Quick Check Overlay & Product Spec Modal (ISE-55, ISE-56, ISE-57)
    bool mQuickCheckModalOpen = false;
    std::string mQuickCheckBarcode = "";
    int mQuickCheckProductIdx = -1;
    bool mQuickCheckInputFocused = true;
    bool mSpecModalOpen = false;
    int mSpecProductIdx = -1;

    // Add Product From Inventory Modal States
    std::string mAddProductSearchQuery = "";
    bool mAddProductSearchFocused = false;
    int mSelectedInventoryForProduct = -1;
    std::string mTransferQtyStr = "10";

    // Return Tab State
    std::string mReturnBarcode = "";
    bool mReturnBarcodeFocused = true;
    int mReturnQty = 1;
    int mSelectedReturnProductIdx = -1;
    std::string mReturnSaleInfo = "";
    int mReturnSalePrice = 0;
    std::string mReturnOrderId = "";
    std::string mReturnCustomer = "";
    std::string mReturnDate = "";
    std::string mReturnStatusMessage = "";
    bool mReturnSuccess = false;

    // Modal Input States
    int mEditingCartIndex = -1;
    std::string mQtyInputStr = "";

    std::string mNewStaffName = "";
    std::string mNewStaffPass = "";
    std::string mNewStaffRole = "Employee";
    int mNewStaffActiveField = 1;

    std::string mResetStaffTarget = "";
    std::string mResetStaffNewPass = "";
    bool mResetPassFocused = true;

    std::string mLogFilter = "ALL";

    // Camera Hardware Streaming & Frame Processing
    CameraStream mCameraStream;
    sf::Texture mCameraTexture;
    std::vector<unsigned char> mProcessedCamFrame;
    std::vector<unsigned char> mCamDisplayBuffer;
    int mCamWidth = 0;
    int mCamHeight = 0;
    bool mCameraRunning = false;
    bool mCameraBarcodeFound = false;
    float mBarcodeFoundTimer = 0.f;
    std::string mDetectedBarcode = "";
    BarcodeBox mBarcodeBox;
};

#endif // POS_APP_H

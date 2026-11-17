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
#include "CartService.h"
#include "SplashScreen.h"
#include "LoginScreen.h"
#include "../../backend/include/CameraCapture.h"
#include "../../backend/include/Decoder.h"
#include "../../backend/include/Generator.h"
#include "../../backend/include/Validator.h"

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

    void showToast(const std::string& message);
    void updateLayout(float winW, float winH);

    // Domain Managers & Services (Separation of Concerns)
    StaffManager mStaffManager;
    ActivityLogger mActivityLogger;
    ProductCatalog mProductCatalog;
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

    // Scrolling Offsets
    float mReceiptScrollOffset = 0.f;
    float mEnlargeScrollOffset = 0.f;
    float mProductsScrollOffset = 0.f;
    float mInventoryScrollOffset = 0.f;
    float mSalesScrollOffset = 0.f;
    float mLogScrollOffset = 0.f;

    // Modals Visibility
    bool mReceiptModalOpen = false;
    bool mQtyModalOpen = false;
    bool mEnlargeCartModalOpen = false;
    bool mCashModalOpen = false;
    bool mCardModalOpen = false;
    bool mAddStaffModalOpen = false;
    bool mResetPassModalOpen = false;

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
    BarcodeBox mBarcodeBox;
};

#endif // POS_APP_H

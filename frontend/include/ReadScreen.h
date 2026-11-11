#ifndef READSCREEN_H
#define READSCREEN_H
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "UITheme.h"
#include "CameraCapture.h"
class ReadScreen {
public:
    using ScanCallback = std::function<void(const std::string& code, const std::string& source)>;

    ReadScreen(sf::RenderWindow& window, sf::Font& font);
    ~ReadScreen();

    AppScreen handleEvent(const sf::Event& event);
    void update(float dt);
    void draw();

    std::string getLastDecodedCode() const { return mDecodedCode; }
    void stopCamera();
    void setOnScanCallback(ScanCallback cb) { mOnScanCallback = cb; }

private:
    sf::RenderWindow& mWindow;
    sf::Font&         mFont;

    // Header
    sf::Text           mTitle;
    sf::Text           mSubtitle;
    sf::Text           mBackBtn;

    // Action Buttons
    sf::RectangleShape mUploadBtn;
    sf::Text           mUploadBtnText;
    sf::RectangleShape mCameraBtn;
    sf::Text           mCameraBtnText;

    // Optical Viewport & Laser
    sf::RectangleShape mViewportCard;
    sf::RectangleShape mLaserLine;
    float              mLaserY;
    float              mLaserSpeed;

    // Real-Time Camera Stream
    CameraStream       mCameraStream;
    sf::Texture        mCameraTexture;
    bool               mCameraActive;
    float              mScanCooldown;

    // Preview
    sf::Texture        mPreviewTexture;
    sf::Sprite         mPreviewSprite;
    bool               mHasPreview;
    sf::Text           mTargetGuide;

    // Result Card
    sf::RectangleShape mResultCard;
    sf::Text           mResultLabel;
    sf::Text           mResultDigits;
    sf::Text           mStatusText;

    // Bottom action buttons
    sf::RectangleShape mDecodeBtn;
    sf::Text           mDecodeBtnText;
    sf::RectangleShape mClearBtn;
    sf::Text           mClearBtnText;

    std::string        mLoadedPath;
    std::string        mDecodedCode;
    std::string        mStatusMsg;
    bool               mStatusSuccess;

    ScanCallback       mOnScanCallback;

    void openFileDialog();
    void toggleCameraStream();
    void decodeCurrent();
};

#endif // READSCREEN_H

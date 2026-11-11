#include <windows.h>
#include <commdlg.h>
#include <algorithm>
#include <filesystem>
#include "../include/ReadScreen.h"
#include "../include/UITheme.h"
#include "../include/ui_assets.hpp"
#include "Decoder.h"
#include "CameraCapture.h"

namespace fs = std::filesystem;

static void fitSpriteToViewport(sf::Sprite& sprite, unsigned imgW, unsigned imgH) {
    if (imgW == 0 || imgH == 0) return;
    const float vpW = 440.f;
    const float vpH = 230.f;
    const float vpX = 50.f;
    const float vpY = 200.f;

    float scaleX = vpW / (float)imgW;
    float scaleY = vpH / (float)imgH;
    float scale = std::min(scaleX, scaleY);

    sprite.setScale(scale, scale);
    float renderedW = (float)imgW * scale;
    float renderedH = (float)imgH * scale;
    float posX = vpX + (vpW - renderedW) * 0.5f;
    float posY = vpY + (vpH - renderedH) * 0.5f;
    sprite.setPosition(posX, posY);
}

ReadScreen::ReadScreen(sf::RenderWindow& window, sf::Font& font)
    : mWindow(window), mFont(font),
      mLaserY(210.f), mLaserSpeed(150.0f),
      mCameraActive(false), mScanCooldown(0.f),
      mHasPreview(false), mStatusSuccess(false)
{
    Assets& A = Assets::instance();

    // Back button
    mBackBtn.setFont(A.fontSemiBold());
    mBackBtn.setString("<  Back to Dashboard");
    mBackBtn.setCharacterSize(14);
    mBackBtn.setFillColor(Theme::Cyan);
    mBackBtn.setPosition(40.f, 25.f);

    // Header Title
    mTitle.setFont(A.fontBold());
    mTitle.setString("Optical Barcode Scanner & Camera Stream");
    mTitle.setCharacterSize(24);
    mTitle.setFillColor(Theme::TextWhite);
    mTitle.setPosition(40.f, 55.f);

    mSubtitle.setFont(A.fontRegular());
    mSubtitle.setString("Upload a .BMP image file or align your barcode inside the live optical laser viewport");
    mSubtitle.setCharacterSize(13);
    mSubtitle.setFillColor(Theme::TextMuted);
    mSubtitle.setPosition(40.f, 85.f);

    // Upload button
    mUploadBtn.setSize(sf::Vector2f(210.f, 44.f));
    mUploadBtn.setPosition(40.f, 125.f);
    mUploadBtn.setFillColor(Theme::Purple);

    mUploadBtnText.setFont(A.fontSemiBold());
    mUploadBtnText.setString("Upload BMP Image");
    mUploadBtnText.setCharacterSize(14);
    mUploadBtnText.setFillColor(Theme::TextWhite);
    mUploadBtnText.setPosition(65.f, 137.f);

    // Camera Stream toggle button
    mCameraBtn.setSize(sf::Vector2f(230.f, 44.f));
    mCameraBtn.setPosition(270.f, 125.f);
    mCameraBtn.setFillColor(sf::Color(35, 18, 75));
    mCameraBtn.setOutlineColor(Theme::PanelBorder);
    mCameraBtn.setOutlineThickness(1.2f);

    mCameraBtnText.setFont(A.fontSemiBold());
    mCameraBtnText.setString("[o] Start Live Camera");
    mCameraBtnText.setCharacterSize(14);
    mCameraBtnText.setFillColor(Theme::Cyan);
    mCameraBtnText.setPosition(290.f, 137.f);

    // Optical Viewport Card
    mViewportCard.setSize(sf::Vector2f(460.f, 250.f));
    mViewportCard.setPosition(40.f, 190.f);
    mViewportCard.setFillColor(sf::Color(12, 6, 24, 240));
    mViewportCard.setOutlineColor(Theme::PanelBorder);
    mViewportCard.setOutlineThickness(1.5f);

    // Target guide text
    mTargetGuide.setFont(A.fontRegular());
    mTargetGuide.setString("Live optical sensor standby.\nClick 'Start Live Camera' or 'Upload BMP Image'");
    mTargetGuide.setCharacterSize(13);
    mTargetGuide.setFillColor(Theme::TextMuted);
    mTargetGuide.setPosition(80.f, 290.f);

    // Laser Line (animated across viewport)
    mLaserLine.setSize(sf::Vector2f(440.f, 2.5f));
    mLaserLine.setPosition(50.f, mLaserY);
    mLaserLine.setFillColor(Theme::Red);

    // Right Card: Decoded Number
    mResultCard.setSize(sf::Vector2f(440.f, 250.f));
    mResultCard.setPosition(520.f, 190.f);
    mResultCard.setFillColor(sf::Color(24, 16, 46, 220));
    mResultCard.setOutlineColor(Theme::PanelBorder);
    mResultCard.setOutlineThickness(1.2f);

    mResultLabel.setFont(A.fontSemiBold());
    mResultLabel.setString("DECODED 6-DIGIT CODE");
    mResultLabel.setCharacterSize(12);
    mResultLabel.setFillColor(Theme::TextMuted);
    mResultLabel.setPosition(544.f, 215.f);

    mResultDigits.setFont(A.fontBold());
    mResultDigits.setString("------");
    mResultDigits.setCharacterSize(44);
    mResultDigits.setFillColor(Theme::Cyan);
    mResultDigits.setPosition(544.f, 260.f);

    mStatusText.setFont(A.fontRegular());
    mStatusText.setCharacterSize(12);
    mStatusText.setFillColor(Theme::TextMuted);
    mStatusText.setPosition(544.f, 340.f);

    // Bottom Action Buttons
    mDecodeBtn.setSize(sf::Vector2f(210.f, 44.f));
    mDecodeBtn.setPosition(40.f, 465.f);
    mDecodeBtn.setFillColor(Theme::Purple);

    mDecodeBtnText.setFont(A.fontSemiBold());
    mDecodeBtnText.setString("Execute Optical Decode");
    mDecodeBtnText.setCharacterSize(14);
    mDecodeBtnText.setFillColor(Theme::TextWhite);
    mDecodeBtnText.setPosition(65.f, 477.f);

    mClearBtn.setSize(sf::Vector2f(140.f, 44.f));
    mClearBtn.setPosition(270.f, 465.f);
    mClearBtn.setFillColor(sf::Color(35, 18, 55));
    mClearBtn.setOutlineColor(Theme::PanelBorder);
    mClearBtn.setOutlineThickness(1.2f);

    mClearBtnText.setFont(A.fontSemiBold());
    mClearBtnText.setString("Clear");
    mClearBtnText.setCharacterSize(14);
    mClearBtnText.setFillColor(Theme::TextMuted);
    mClearBtnText.setPosition(315.f, 477.f);
}

ReadScreen::~ReadScreen() {
    stopCamera();
}

void ReadScreen::stopCamera() {
    if (mCameraActive) {
        mCameraStream.stop();
        mCameraActive = false;
        mCameraBtnText.setString("[o] Start Live Camera");
        mCameraBtn.setFillColor(sf::Color(35, 18, 75));
    }
}

void ReadScreen::toggleCameraStream() {
    if (mCameraActive) {
        stopCamera();
        mStatusMsg = "Camera stream paused.";
        mStatusSuccess = true;
    } else {
        mLoadedPath.clear();
        mHasPreview = false;
        mPreviewTexture = sf::Texture();
        mCameraActive = mCameraStream.start(0);
        if (mCameraActive) {
            mCameraBtnText.setString("[#] Stop Live Camera");
            mCameraBtn.setFillColor(sf::Color(76, 29, 149));
            mStatusMsg = "Continuous live camera stream active (" + mCameraStream.getCameraName() + "). Point barcode inside laser line!";
            mStatusSuccess = true;
        } else {
            mStatusMsg = "[CAMERA ERROR]: Could not open webcam hardware.";
            mStatusSuccess = false;
        }
    }
}

void ReadScreen::update(float dt) {
    // Animate red scanner laser
    mLaserY += mLaserSpeed * dt;
    if (mLaserY > 415.f) {
        mLaserY = 415.f;
        mLaserSpeed = -std::abs(mLaserSpeed);
    } else if (mLaserY < 205.f) {
        mLaserY = 205.f;
        mLaserSpeed = std::abs(mLaserSpeed);
    }
    mLaserLine.setPosition(50.f, mLaserY);

    // Continuous Real-Time Camera Stream Update
    if (mCameraActive) {
        std::vector<unsigned char> rgba;
        int w = 0, h = 0;
        if (mCameraStream.getLatestFrame(rgba, w, h) && !rgba.empty()) {
            if (mCameraTexture.getSize().x != (unsigned)w || mCameraTexture.getSize().y != (unsigned)h) {
                mCameraTexture.create(w, h);
            }
            mPreviewSprite.setTexture(mCameraTexture, true);
            fitSpriteToViewport(mPreviewSprite, w, h);
            mCameraTexture.update(rgba.data());
            mHasPreview = true;

            // Real-Time Barcode Recognition with fastMode (zero UI lag)
            mScanCooldown -= dt;
            if (mScanCooldown <= 0.f && mDecodedCode.empty()) {
                mScanCooldown = 0.10f;
                std::string detectedCode, err;
                if (decodeBarcodeFromMemory(rgba.data(), w, h, true, detectedCode, err, true)) {
                    mDecodedCode = detectedCode;
                    mResultDigits.setString(detectedCode);
                    mResultDigits.setFillColor(Theme::Green);
                    mStatusMsg = "[OPTICAL SUCCESS]: Decoded " + detectedCode + " live from " + mCameraStream.getCameraName() + "!";
                    mStatusSuccess = true;
                    if (mOnScanCallback) {
                        mOnScanCallback(detectedCode, "Camera");
                    }
                }
            }
        }
    }
}

void ReadScreen::openFileDialog() {
    stopCamera();

    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = mWindow.getSystemHandle();
    ofn.lpstrFile   = szFile;
    ofn.nMaxFile    = sizeof(szFile);
    ofn.lpstrFilter = "BMP Barcode Images (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        mLoadedPath = szFile;
        if (mPreviewTexture.loadFromFile(mLoadedPath)) {
            mPreviewSprite.setTexture(mPreviewTexture, true);
            sf::Vector2u sz = mPreviewTexture.getSize();
            fitSpriteToViewport(mPreviewSprite, sz.x, sz.y);
            mHasPreview = true;
        }
        mStatusMsg = "File loaded. Laser calibrated. Ready to decode.";
        mStatusSuccess = true;
        mResultDigits.setString("------");
        mResultDigits.setFillColor(Theme::Cyan);
        mDecodedCode.clear();

        decodeCurrent();
    }
}

void ReadScreen::decodeCurrent() {
    if (!mLoadedPath.empty()) {
        std::string err;
        if (decodeBarcodeFromBMP(mLoadedPath, mDecodedCode, err)) {
            mResultDigits.setString(mDecodedCode);
            mResultDigits.setFillColor(Theme::Green);
            mStatusMsg = "OPTICAL SUCCESS: Decoded value: " + mDecodedCode;
            mStatusSuccess = true;
            if (mOnScanCallback) {
                mOnScanCallback(mDecodedCode, "File Upload");
            }
        } else {
            mResultDigits.setString("FAILED");
            mResultDigits.setFillColor(Theme::Red);
            mStatusMsg = err;
            mStatusSuccess = false;
        }
    } else if (mCameraActive) {
        // Immediate decode from active camera frame
        std::vector<unsigned char> rgba;
        int w = 0, h = 0;
        if (mCameraStream.getLatestFrame(rgba, w, h) && !rgba.empty()) {
            std::string err;
            if (decodeBarcodeFromMemory(rgba.data(), w, h, true, mDecodedCode, err)) {
                mResultDigits.setString(mDecodedCode);
                mResultDigits.setFillColor(Theme::Green);
                mStatusMsg = "OPTICAL SUCCESS: Decoded value: " + mDecodedCode;
                mStatusSuccess = true;
                if (mOnScanCallback) {
                    mOnScanCallback(mDecodedCode, "Camera");
                }
            } else {
                mResultDigits.setString("FAILED");
                mResultDigits.setFillColor(Theme::Red);
                mStatusMsg = err;
                mStatusSuccess = false;
            }
        }
    } else {
        mStatusMsg = "Please upload an image or start camera stream first.";
        mStatusSuccess = false;
    }
}

AppScreen ReadScreen::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mp((float)event.mouseButton.x, (float)event.mouseButton.y);

        if (mBackBtn.getGlobalBounds().contains(mp)) {
            stopCamera();
            return AppScreen::Home;
        }
        if (mUploadBtn.getGlobalBounds().contains(mp)) {
            openFileDialog();
        }
        if (mCameraBtn.getGlobalBounds().contains(mp)) {
            toggleCameraStream();
        }
        if (mDecodeBtn.getGlobalBounds().contains(mp)) {
            decodeCurrent();
        }
        if (mClearBtn.getGlobalBounds().contains(mp)) {
            mDecodedCode.clear();
            mLoadedPath.clear();
            mPreviewTexture = sf::Texture();
            mResultDigits.setString("------");
            mResultDigits.setFillColor(Theme::Cyan);
            mStatusMsg = "Cleared. Viewport reset.";
            mStatusSuccess = true;
            if (!mCameraActive) {
                mHasPreview = false;
            }
        }
    }
    return AppScreen::Read;
}

void ReadScreen::draw() {
    Assets& A = Assets::instance();

    // Background
    sf::Texture& bg = A.tex("ui/background.png");
    if (bg.getSize().x > 2) {
        sf::Sprite bgSp(bg);
        mWindow.draw(bgSp);
    }

    // Header & Navigation
    mWindow.draw(mBackBtn);
    mWindow.draw(mTitle);
    mWindow.draw(mSubtitle);

    // Action Buttons
    mWindow.draw(mUploadBtn);
    mWindow.draw(mUploadBtnText);
    mWindow.draw(mCameraBtn);
    mWindow.draw(mCameraBtnText);

    // Viewport Card
    mWindow.draw(mViewportCard);
    if (mHasPreview) {
        mWindow.draw(mPreviewSprite);
    } else {
        mWindow.draw(mTargetGuide);
    }

    // Viewfinder Corners
    sf::Texture& corners = A.tex("ui/scan-corners.png");
    if (corners.getSize().x > 2) {
        sf::Sprite cSp(corners);
        cSp.setPosition(48.f, 198.f);
        mWindow.draw(cSp);
    }

    // Animated Red Laser Scanning Line
    if (mHasPreview || mCameraActive) {
        mWindow.draw(mLaserLine);
    }

    // Result Card
    mWindow.draw(mResultCard);
    mWindow.draw(mResultLabel);
    mWindow.draw(mResultDigits);

    // Status Message
    mStatusText.setString(mStatusMsg);
    mStatusText.setFillColor(mStatusSuccess ? Theme::Green : Theme::Red);
    mWindow.draw(mStatusText);

    // Bottom Action Controls
    mWindow.draw(mDecodeBtn);
    mWindow.draw(mDecodeBtnText);
    mWindow.draw(mClearBtn);
    mWindow.draw(mClearBtnText);

    // Active path / device indicator
    std::string targetInfo = mCameraActive ? ("Live Stream: " + mCameraStream.getCameraName()) : (mLoadedPath.empty() ? "" : ("Target: " + fs::path(mLoadedPath).filename().string()));
    if (!targetInfo.empty()) {
        sf::Text pathInfo(targetInfo, A.fontRegular(), 11);
        pathInfo.setFillColor(Theme::TextMuted);
        pathInfo.setPosition(40.f, 520.f);
        mWindow.draw(pathInfo);
    }
}

#include "../include/GenerateScreen.h"
#include "../include/UITheme.h"
#include "../include/ui_assets.hpp"
#include "Generator.h"
#include "Validator.h"
#include <cmath>

GenerateScreen::GenerateScreen(sf::RenderWindow& window, sf::Font& font)
    : mWindow(window), mFont(font),
      mInputFocused(true), mCursorBlink(0.f), mCursorVisible(true),
      mHasBarcode(false), mStatusOk(false)
{
    // Back button
    mBackBtn.setFont(Assets::instance().fontSemiBold());
    mBackBtn.setString("<  Back to Dashboard");
    mBackBtn.setCharacterSize(14);
    mBackBtn.setFillColor(Theme::Cyan);
    mBackBtn.setStyle(sf::Text::Bold);
    mBackBtn.setPosition(40.f, 25.f);

    // Title
    mInputLabel.setFont(Assets::instance().fontSemiBold());
    mInputLabel.setString("ENTER 6-DIGIT CODE (FIRST DIGIT CANNOT BE ZERO):");
    mInputLabel.setCharacterSize(12);
    mInputLabel.setFillColor(Theme::TextMuted);
    mInputLabel.setPosition(40.f, 85.f);

    // Input box
    mInputBox.setSize(sf::Vector2f(420.f, 48.f));
    mInputBox.setPosition(40.f, 110.f);
    mInputBox.setFillColor(sf::Color(20, 10, 40, 230));
    mInputBox.setOutlineColor(Theme::Purple);
    mInputBox.setOutlineThickness(1.5f);

    mInputText.setFont(Assets::instance().fontMono());
    mInputText.setCharacterSize(22);
    mInputText.setFillColor(Theme::TextWhite);
    mInputText.setStyle(sf::Text::Bold);
    mInputText.setPosition(55.f, 120.f);

    // Hint
    mInputHint.setFont(Assets::instance().fontRegular());
    mInputHint.setString("Rule: Exactly 6 numeric digits (1-9 for 1st digit, 0-9 for others).");
    mInputHint.setCharacterSize(12);
    mInputHint.setFillColor(Theme::TextMuted);
    mInputHint.setPosition(40.f, 168.f);

    // Generate button
    mGenerateBtn.setSize(sf::Vector2f(200.f, 44.f));
    mGenerateBtn.setPosition(40.f, 198.f);
    mGenerateBtn.setFillColor(Theme::Purple);

    mGenerateBtnText.setFont(Assets::instance().fontSemiBold());
    mGenerateBtnText.setString("Generate Barcode");
    mGenerateBtnText.setCharacterSize(14);
    mGenerateBtnText.setFillColor(Theme::TextWhite);
    mGenerateBtnText.setPosition(70.f, 210.f);

    // Save button
    mSaveBtn.setSize(sf::Vector2f(200.f, 44.f));
    mSaveBtn.setPosition(260.f, 198.f);
    mSaveBtn.setFillColor(Theme::Green);

    mSaveBtnText.setFont(Assets::instance().fontSemiBold());
    mSaveBtnText.setString("Save as .BMP");
    mSaveBtnText.setCharacterSize(14);
    mSaveBtnText.setFillColor(Theme::TextWhite);
    mSaveBtnText.setPosition(305.f, 210.f);

    // Status text
    mStatusText.setFont(Assets::instance().fontMedium());
    mStatusText.setCharacterSize(13);
    mStatusText.setPosition(40.f, 255.f);

    // Barcode background canvas
    mBarcodeBackground.setSize(sf::Vector2f(944.f, 260.f));
    mBarcodeBackground.setPosition(40.f, 290.f);
    mBarcodeBackground.setFillColor(sf::Color(255, 255, 255));
    mBarcodeBackground.setOutlineColor(Theme::PanelBorder);
    mBarcodeBackground.setOutlineThickness(2.f);

    // Digits label
    mDigitsLabel.setFont(Assets::instance().fontMono());
    mDigitsLabel.setCharacterSize(24);
    mDigitsLabel.setFillColor(sf::Color(10, 10, 20));
    mDigitsLabel.setStyle(sf::Text::Bold);
    mDigitsLabel.setPosition(440.f, 495.f);
}

void GenerateScreen::buildBarcodePreview(const std::string& code) {
    struct PatternHelper {
        static const char* get(char ch) {
            switch (ch) {
                case '0': return "NNNWWNWNN";
                case '1': return "WNNWNNNNW";
                case '2': return "NNWWNNNNW";
                case '3': return "WNWWNNNNN";
                case '4': return "NNNWWNNNW";
                case '5': return "WNNWWNNNN";
                case '6': return "NNWWWNNNN";
                case '7': return "NNNWNNWNW";
                case '8': return "WNNWNNWNN";
                case '9': return "NNWWNNWNN";
                case '*': return "NWNNWNWNN";
                default:  return nullptr;
            }
        }
    };

    mBars.clear();
    std::string full = "*" + code + "*";
    const int N = 4;
    const int W = 11;
    const int H = 170;
    const int QUIET = 50;

    float x = 40.f + QUIET;
    float yTop = 310.f;

    for (size_t ci = 0; ci < full.size(); ++ci) {
        const char* pat = PatternHelper::get(full[ci]);
        if (!pat) continue;
        for (int i = 0; i < 9; ++i) {
            int w = (pat[i] == 'W') ? W : N;
            bool isBar = (i % 2 == 0);
            if (isBar) {
                sf::RectangleShape bar(sf::Vector2f((float)w, (float)H));
                bar.setPosition(x, yTop);
                bar.setFillColor(sf::Color(10, 10, 20));
                mBars.push_back(bar);
            }
            x += w;
        }
        if (ci + 1 < full.size()) x += N;
    }

    mDigitsLabel.setString("* " + code + " *");
    mHasBarcode = true;
}

void GenerateScreen::generateAndSave() {
    std::string err;
    if (!isValidSixDigitCode(mInputStr, err)) {
        mStatusMsg = "[VALIDATION FAILED]: " + err;
        mStatusOk = false;
        mHasBarcode = false;
        return;
    }

    buildBarcodePreview(mInputStr);
    mLastCode = mInputStr;

    mSavedFilename = "barcode_" + mInputStr + ".bmp";
    if (generateBarcodeBMP(mInputStr, mSavedFilename)) {
        mStatusMsg = "[SUCCESS] Barcode saved to disk: " + mSavedFilename;
        mStatusOk = true;
        if (mOnGenerateCallback) {
            mOnGenerateCallback(mInputStr, "Generator");
        }
    } else {
        mStatusMsg = "[ERROR] Could not generate barcode image.";
        mStatusOk = false;
    }
}

AppScreen GenerateScreen::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::BackSpace && !mInputStr.empty()) {
            mInputStr.pop_back();
            mHasBarcode = false;
            mStatusMsg.clear();
        }
        if (event.key.code == sf::Keyboard::Return) {
            generateAndSave();
        }
    }

    if (event.type == sf::Event::TextEntered) {
        char c = (char)event.text.unicode;
        if (c >= '0' && c <= '9' && mInputStr.size() < 6) {
            mInputStr += c;
            mHasBarcode = false;
            mStatusMsg.clear();
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mp((float)event.mouseButton.x, (float)event.mouseButton.y);

        if (mBackBtn.getGlobalBounds().contains(mp)) {
            return AppScreen::Home;
        }
        if (mGenerateBtn.getGlobalBounds().contains(mp)) {
            generateAndSave();
        }
        if (mSaveBtn.getGlobalBounds().contains(mp) && mHasBarcode) {
            generateBarcodeBMP(mInputStr, "barcode_" + mInputStr + ".bmp");
            mStatusMsg = "[SUCCESS] Re-saved: barcode_" + mInputStr + ".bmp";
            mStatusOk = true;
        }
    }

    return AppScreen::Generate;
}

void GenerateScreen::draw() {
    Assets& A = Assets::instance();

    sf::Texture& bg = A.tex("ui/background.png");
    if (bg.getSize().x > 2) {
        sf::Sprite bgSp(bg);
        mWindow.draw(bgSp);
    } else {
        mWindow.clear(Theme::Background);
    }

    A.glow(mWindow, { 200.f, 150.f }, 220.f, Theme::PurpleSoft);

    // Header
    mWindow.draw(mBackBtn);

    sf::Text hdr("Generate Barcode Studio", A.fontBold(), 24);
    hdr.setFillColor(Theme::TextWhite);
    hdr.setPosition(40.f, 50.f);
    mWindow.draw(hdr);

    // Input Section
    mWindow.draw(mInputLabel);
    mWindow.draw(mInputBox);

    mCursorBlink += 0.016f;
    if (mCursorBlink >= 0.5f) { mCursorBlink = 0.f; mCursorVisible = !mCursorVisible; }
    std::string display = mInputStr;
    if (mCursorVisible) display += "|";
    mInputText.setString(display);
    mWindow.draw(mInputText);

    mWindow.draw(mInputHint);
    mWindow.draw(mGenerateBtn);
    mWindow.draw(mGenerateBtnText);
    mWindow.draw(mSaveBtn);
    mWindow.draw(mSaveBtnText);

    if (!mStatusMsg.empty()) {
        mStatusText.setString(mStatusMsg);
        mStatusText.setFillColor(mStatusOk ? Theme::Green : Theme::Red);
        mWindow.draw(mStatusText);
    }

    // Barcode Display Area
    mWindow.draw(mBarcodeBackground);
    if (mHasBarcode) {
        for (auto& b : mBars) mWindow.draw(b);
        mWindow.draw(mDigitsLabel);
    } else {
        sf::Text ph("Barcode canvas will appear here upon typing 6 digits", A.fontMedium(), 15);
        ph.setFillColor(sf::Color(120, 120, 140));
        ph.setPosition(Theme::centerX(ph, 40.f, 944.f), 410.f);
        mWindow.draw(ph);
    }
}

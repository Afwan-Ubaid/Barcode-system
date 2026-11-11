#define _CRT_SECURE_NO_WARNINGS
#include "../include/HomeScreen.h"
#include "../include/ui_assets.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

static std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* t = std::localtime(&now);
    std::ostringstream ss;
    if (t) {
        ss << std::setfill('0') << std::setw(2) << t->tm_hour << ":"
           << std::setfill('0') << std::setw(2) << t->tm_min << ":"
           << std::setfill('0') << std::setw(2) << t->tm_sec;
    } else {
        ss << "Just now";
    }
    return ss.str();
}

HomeScreen::HomeScreen(sf::RenderWindow& window, sf::Font& font)
    : mWindow(window), mFont(font),
      mHoverGen(false), mHoverScan(false),
      mHoverNavHome(false), mHoverNavGen(false), mHoverNavScan(false), mHoverNavSettings(false)
{
    // Start with empty records 
    mRecords.clear();

    // Clickable hitboxes
    mCardGenerate.setSize(sf::Vector2f(466.f, 260.f));
    mCardGenerate.setPosition(32.f, 104.f);

    mCardScan.setSize(sf::Vector2f(466.f, 260.f));
    mCardScan.setPosition(526.f, 104.f);

    // Dock Hitboxes
    mNavHome = sf::FloatRect(365.f, 605.f, 60.f, 44.f);
    mNavGen  = sf::FloatRect(443.f, 605.f, 60.f, 44.f);
    mNavScan = sf::FloatRect(521.f, 605.f, 60.f, 44.f);
    mNavSettings = sf::FloatRect(599.f, 605.f, 60.f, 44.f);
}

void HomeScreen::addRecentScan(const std::string& code, const std::string& type) {
    if (code.empty()) return;
    ScanRecord rec = { code, type, getCurrentTimestamp(), "SUCCESS" };
    mRecords.insert(mRecords.begin(), rec);
    if (mRecords.size() > 5) mRecords.pop_back();
}

AppScreen HomeScreen::handleEvent(const sf::Event& event) {
    sf::Vector2f mouse((float)event.mouseMove.x, (float)event.mouseMove.y);
    if (event.type == sf::Event::MouseButtonPressed) {
        mouse = sf::Vector2f((float)event.mouseButton.x, (float)event.mouseButton.y);
    }

    mHoverGen  = mCardGenerate.getGlobalBounds().contains(mouse);
    mHoverScan = mCardScan.getGlobalBounds().contains(mouse);

    // Dock icon hover bounds
    mHoverNavHome = mNavHome.contains(mouse);
    mHoverNavGen  = mNavGen.contains(mouse);
    mHoverNavScan = mNavScan.contains(mouse);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (mHoverGen || mHoverNavGen) {
            return AppScreen::Generate;
        }
        if (mHoverScan || mHoverNavScan) {
            return AppScreen::Read;
        }
    }

    return AppScreen::Home;
}

void HomeScreen::draw() {
    Assets& A = Assets::instance();

    // 1. Background texture
    sf::Texture& bg = A.tex("ui/background.png");
    if (bg.getSize().x > 2) {
        sf::Sprite bgSprite(bg);
        mWindow.draw(bgSprite);
    } else {
        mWindow.clear(Theme::Background);
    }

    // 2. Additive Ambient Glow Orbs
    A.glow(mWindow, { 150.f, 100.f }, 240.f, sf::Color(139, 92, 246, 70));
    A.glow(mWindow, { 880.f, 420.f }, 220.f, sf::Color(6, 182, 212, 50));

    // 3. Header Panel
    A.panel(mWindow, "header", 32.f, 16.f, 40);

    // App Logo Mark & Title (No v2.4 PRO)
    sf::Texture& logoTex = A.tex("brand/logo-mark.png");
    if (logoTex.getSize().x > 2) {
        sf::Sprite logo(logoTex);
        logo.setScale(40.f / (float)logoTex.getSize().x, 40.f / (float)logoTex.getSize().y);
        logo.setPosition(52.f, 32.f);
        mWindow.draw(logo);
    }

    sf::Text title("ScanIT", A.fontBold(), 24);
    title.setFillColor(Theme::TextWhite);
    title.setPosition(102.f, 26.f);
    mWindow.draw(title);

    sf::Text subHeader("Your smart barcode companion & high-speed encoding studio", A.fontRegular(), 13);
    subHeader.setFillColor(Theme::TextMuted);
    subHeader.setPosition(102.f, 56.f);
    mWindow.draw(subHeader);


    // 4. Hero Card 1: Generate Barcode
    A.panel(mWindow, "hero", 32.f, 104.f, 40, mHoverGen ? sf::Color(255, 255, 255, 240) : sf::Color::White);

    sf::Text genTitle("GENERATE BARCODE", A.fontSemiBold(), 13);
    genTitle.setFillColor(Theme::PurpleSoft);
    genTitle.setPosition(58.f, 128.f);
    mWindow.draw(genTitle);

    sf::Text genDesc("Create scannable barcodes from 6-digit\nproduct or inventory identifiers.", A.fontRegular(), 12);
    genDesc.setFillColor(Theme::TextMuted);
    genDesc.setPosition(58.f, 150.f);
    mWindow.draw(genDesc);

    // Preview panel inside card (250x84, pad 12 at (58, 192))
    A.panel(mWindow, "preview", 58.f, 192.f, 12);

    // Dynamic Barcode preview: if we have generated records, show latest code, else show placeholder icon
    std::string latestCode = mRecords.empty() ? "" : mRecords.front().code;
    A.icon(mWindow, "barcode", { 183.f, 234.f }, 110.f, Theme::TextWhite);

    // Input field (Clean, without CODE 39 box)
    A.panel(mWindow, "input", 58.f, 290.f, 12);
    std::string inputDisplay = latestCode.empty() ? "Enter 6 digits..." : (latestCode + " [Saved]");
    sf::Text sampleVal(inputDisplay, A.fontMono(), 13);
    sampleVal.setFillColor(latestCode.empty() ? Theme::TextMuted : Theme::Cyan);
    sampleVal.setPosition(74.f, 299.f);
    mWindow.draw(sampleVal);

    sf::Text genLink("Launch Generator Studio  ->", A.fontSemiBold(), 12);
    genLink.setFillColor(mHoverGen ? Theme::TextWhite : Theme::Cyan);
    genLink.setPosition(58.f, 338.f);
    mWindow.draw(genLink);

    // 5. Hero Card 2: Optical Scanner
    A.panel(mWindow, "hero", 526.f, 104.f, 40, mHoverScan ? sf::Color(255, 255, 255, 240) : sf::Color::White);

    sf::Text scanTitle("OPTICAL SCANNER", A.fontSemiBold(), 13);
    scanTitle.setFillColor(Theme::Cyan);
    scanTitle.setPosition(552.f, 128.f);
    mWindow.draw(scanTitle);

    sf::Text scanDesc("Use your webcam or upload a .BMP image to detect\nand decode barcode data in real-time.", A.fontRegular(), 12);
    scanDesc.setFillColor(Theme::TextMuted);
    scanDesc.setPosition(552.f, 150.f);
    mWindow.draw(scanDesc);

    // Camera Viewport
    A.panel(mWindow, "camera", 552.f, 192.f, 12);

    sf::Texture& corners = A.tex("ui/scan-corners.png");
    if (corners.getSize().x > 2) {
        sf::Sprite cSp(corners);
        cSp.setPosition(556.f, 196.f);
        mWindow.draw(cSp);
    }

    sf::Text camText("LIVE OPTICAL VIEWPORT", A.fontSemiBold(), 11);
    camText.setFillColor(Theme::TextMuted);
    camText.setPosition(572.f, 204.f);
    mWindow.draw(camText);

    // Laser line preview
    sf::RectangleShape laserLine({ 380.f, 2.f });
    laserLine.setPosition(557.f, 254.f);
    laserLine.setFillColor(Theme::Red);
    mWindow.draw(laserLine);

    sf::Text scanLink("Open Camera & Scanner  ->", A.fontSemiBold(), 12);
    scanLink.setFillColor(mHoverScan ? Theme::TextWhite : Theme::Cyan);
    scanLink.setPosition(552.f, 338.f);
    mWindow.draw(scanLink);

    // 6. Activity Card
    A.panel(mWindow, "activity", 32.f, 384.f, 40);

    sf::Text actTitle("RECENT SCANS & ACTIVITY LOG", A.fontSemiBold(), 13);
    actTitle.setFillColor(Theme::TextWhite);
    actTitle.setPosition(58.f, 402.f);
    mWindow.draw(actTitle);

    // Column Headers
    sf::Text col1("BARCODE DATA", A.fontSemiBold(), 11);
    col1.setFillColor(Theme::TextMuted);
    col1.setPosition(58.f, 432.f);
    mWindow.draw(col1);

    sf::Text col2("SOURCE", A.fontSemiBold(), 11);
    col2.setFillColor(Theme::TextMuted);
    col2.setPosition(360.f, 432.f);
    mWindow.draw(col2);

    sf::Text col3("TIMESTAMP", A.fontSemiBold(), 11);
    col3.setFillColor(Theme::TextMuted);
    col3.setPosition(600.f, 432.f);
    mWindow.draw(col3);

    sf::Text col4("STATUS", A.fontSemiBold(), 11);
    col4.setFillColor(Theme::TextMuted);
    col4.setPosition(815.f, 432.f);
    mWindow.draw(col4);

    // Dynamic Data Rows (Empty by default until you generate or scan!)
    if (mRecords.empty()) {
        sf::Text emptyMsg("No scans or barcodes generated yet. Click 'Generate Barcode' or 'Optical Scanner' above.", A.fontRegular(), 12);
        emptyMsg.setFillColor(Theme::TextMuted);
        emptyMsg.setPosition(58.f, 470.f);
        mWindow.draw(emptyMsg);
    } else {
        float rowY = 456.f;
        for (size_t i = 0; i < mRecords.size() && i < 4; ++i) {
            sf::Text dVal(mRecords[i].code, A.fontMono(), 12);
            dVal.setFillColor(Theme::Cyan);
            dVal.setPosition(58.f, rowY);
            mWindow.draw(dVal);

            sf::Text dSrc(mRecords[i].source, A.fontRegular(), 12);
            dSrc.setFillColor(Theme::TextMuted);
            dSrc.setPosition(360.f, rowY);
            mWindow.draw(dSrc);

            sf::Text dTime(mRecords[i].time, A.fontMono(), 11);
            dTime.setFillColor(Theme::TextMuted);
            dTime.setPosition(600.f, rowY);
            mWindow.draw(dTime);

            // Status Pill Container (pill-ok)
            A.panel(mWindow, "pill-ok", 805.f, rowY - 3.f, 6);

            // Crisp green indicator dot
            sf::CircleShape statusDot(3.5f);
            statusDot.setFillColor(Theme::Green);
            statusDot.setPosition(817.f, rowY + 6.f);
            mWindow.draw(statusDot);

            // SUCCESS label inside status pill
            sf::Text okText("SUCCESS", A.fontSemiBold(), 10);
            okText.setFillColor(Theme::Green);
            okText.setPosition(828.f, rowY + 2.f);
            mWindow.draw(okText);

            rowY += 27.f;
        }
    }

    // 7. Floating Bottom Dock
    A.panel(mWindow, "dock", 312.f, 598.f, 40);

    // Active pill behind Home
    A.panel(mWindow, "tab-active", 353.f, 605.f, 14);

    // Crisp Vector Icons at x = 395, 473, 551, 629
    A.icon(mWindow, "home",     { 395.f, 627.f }, 22.f, Theme::TextWhite);
    A.icon(mWindow, "barcode",  { 473.f, 627.f }, 22.f, mHoverNavGen ? Theme::TextWhite : Theme::PurpleSoft);
    A.icon(mWindow, "scan",     { 551.f, 627.f }, 22.f, mHoverNavScan ? Theme::TextWhite : Theme::Cyan);
    A.icon(mWindow, "settings", { 629.f, 627.f }, 22.f, Theme::TextMuted);
}

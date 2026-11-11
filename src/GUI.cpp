#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

#include "GUI.h"
#include "Validator.h"
#include "Generator.h"
#include "Decoder.h"

#include <iostream>
#include <cctype>
#include <cmath>
#include <algorithm>
using namespace std;

const float WIN_W = 1100.f;
const float WIN_H = 720.f;

// palette
static const sf::Color BG_TOP(24, 30, 46);
static const sf::Color BG_BOTTOM(12, 15, 26);
static const sf::Color ACCENT(0, 180, 255);
static const sf::Color ACCENT2(0, 220, 140);
static const sf::Color DANGER(220, 70, 70);
static const sf::Color TEXT_MAIN(240, 245, 255);
static const sf::Color TEXT_DIM(150, 165, 195);
static const sf::Color PANEL(35, 45, 65);

struct Rect {
    float x, y, w, h;
    bool contains(float mx, float my) const {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    }
};

// HOME
static const Rect BTN_GENERATE{ 400.f, 300.f, 300.f, 62.f };
static const Rect BTN_DECODE{ 400.f, 380.f, 300.f, 62.f };
static const Rect BTN_CAMERA{ 400.f, 460.f, 300.f, 62.f };
static const Rect BTN_ABOUT{ 400.f, 540.f, 300.f, 62.f };

// GENERATE
static const Rect BTN_GEN_BACK{ 30.f, 30.f, 130.f, 45.f };
static const Rect BTN_GEN_SUBMIT{ 400.f, 500.f, 300.f, 55.f };

// DECODE
static const Rect BTN_DEC_BACK{ 30.f, 30.f, 130.f, 45.f };
static const Rect BTN_DEC_BROWSE{ 830.f, 220.f, 150.f, 60.f };  // NEW
static const Rect BTN_DEC_SUBMIT{ 400.f, 500.f, 300.f, 55.f };

// CAMERA
static const Rect BTN_CAM_BACK{ 30.f, 30.f, 130.f, 45.f };
static const Rect BTN_CAM_START{ 400.f, 600.f, 300.f, 55.f };

// ABOUT
static const Rect BTN_ABOUT_BACK{ 30.f, 30.f, 130.f, 45.f };

// ------------------------------------------------------------------

BarcodeApp::BarcodeApp()
    : window(sf::VideoMode((unsigned)WIN_W, (unsigned)WIN_H),
        "ISE Barcode System  -  Increment 1",
        sf::Style::Titlebar | sf::Style::Close)
{
    window.setFramerateLimit(60);

    sf::Image icon;
    if (icon.loadFromFile("app.png")) {
        window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    }

    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        cerr << "Warning: arial.ttf not loaded\n";
    }
}

void BarcodeApp::drawBackground() {
    sf::VertexArray grad(sf::Quads, 4);
    grad[0].position = { 0.f, 0.f };       grad[0].color = BG_TOP;
    grad[1].position = { WIN_W, 0.f };     grad[1].color = BG_TOP;
    grad[2].position = { WIN_W, WIN_H };   grad[2].color = BG_BOTTOM;
    grad[3].position = { 0.f, WIN_H };     grad[3].color = BG_BOTTOM;
    window.draw(grad);

    sf::CircleShape glow(320.f);
    glow.setFillColor(sf::Color(0, 120, 200, 25));
    glow.setPosition(-80.f + 20.f * sin(clockSeconds * 0.5f), -120.f);
    window.draw(glow);
}

void BarcodeApp::drawButton(const string& label, float x, float y,
    float w, float h, sf::Color fill,
    sf::Color outline)
{
    sf::RectangleShape shadow({ w, h });
    shadow.setPosition(x + 4.f, y + 5.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 90));
    window.draw(shadow);

    sf::RectangleShape box({ w, h });
    box.setPosition(x, y);
    box.setFillColor(fill);
    box.setOutlineThickness(2.f);
    box.setOutlineColor(outline);
    window.draw(box);

    sf::Text t(label, font, 24);
    t.setFillColor(TEXT_MAIN);
    t.setStyle(sf::Text::Bold);
    sf::FloatRect b = t.getLocalBounds();
    t.setPosition(x + (w - b.width) / 2.f - b.left,
        y + (h - b.height) / 2.f - b.top - 4.f);
    window.draw(t);
}

void BarcodeApp::drawStatus() {
    if (statusMessage.empty()) return;
    sf::RectangleShape bar({ WIN_W - 120.f, 44.f });
    bar.setPosition(60.f, 640.f);
    bar.setFillColor(statusIsError ? sf::Color(80, 20, 20, 160)
        : sf::Color(20, 70, 50, 160));
    bar.setOutlineThickness(1.f);
    bar.setOutlineColor(statusIsError ? DANGER : ACCENT2);
    window.draw(bar);

    sf::Text st(statusMessage, font, 18);
    st.setFillColor(statusIsError ? sf::Color(255, 150, 150)
        : sf::Color(160, 240, 200));
    st.setPosition(76.f, 652.f);
    window.draw(st);
}

// ---------------- HOME ----------------
void BarcodeApp::drawHome() {
    sf::Text title("ISE SMART BARCODE SYSTEM", font, 48);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(TEXT_MAIN);
    sf::FloatRect tb = title.getLocalBounds();
    title.setPosition((WIN_W - tb.width) / 2.f - tb.left, 110.f);
    window.draw(title);

    sf::RectangleShape line({ 340.f, 4.f });
    line.setFillColor(ACCENT);
    line.setPosition((WIN_W - 340.f) / 2.f, 178.f);
    window.draw(line);

    sf::Text sub("Increment 1  |  Fast-NUCES  |  Sprint 1 Demo", font, 20);
    sub.setFillColor(TEXT_DIM);
    sf::FloatRect sb = sub.getLocalBounds();
    sub.setPosition((WIN_W - sb.width) / 2.f - sb.left, 210.f);
    window.draw(sub);

    drawButton("1.  Generate Barcode", BTN_GENERATE.x, BTN_GENERATE.y,
        BTN_GENERATE.w, BTN_GENERATE.h, sf::Color(25, 90, 160), ACCENT);
    drawButton("2.  Decode Image", BTN_DECODE.x, BTN_DECODE.y,
        BTN_DECODE.w, BTN_DECODE.h, sf::Color(20, 120, 90), ACCENT2);
    drawButton("3.  Scan with Camera", BTN_CAMERA.x, BTN_CAMERA.y,
        BTN_CAMERA.w, BTN_CAMERA.h, sf::Color(90, 60, 160),
        sf::Color(160, 120, 255));
    drawButton("4.  About", BTN_ABOUT.x, BTN_ABOUT.y,
        BTN_ABOUT.w, BTN_ABOUT.h, sf::Color(50, 55, 70),
        sf::Color(150, 165, 195));

    sf::Text footer("Afwan  |  Ahmed  |  Abdul Wadood  |  Muhammad Raheel",
        font, 14);
    footer.setFillColor(TEXT_DIM);
    footer.setPosition(20.f, WIN_H - 28.f);
    window.draw(footer);
}

void BarcodeApp::handleHomeClick(float mx, float my) {
    if (BTN_GENERATE.contains(mx, my)) {
        currentScreen = Screen::GENERATE;
        inputBuffer.clear(); statusMessage.clear(); hasBarcode = false;
    }
    else if (BTN_DECODE.contains(mx, my)) {
        currentScreen = Screen::DECODE;
        decodePathBuffer.clear(); decodedCode.clear(); statusMessage.clear();
    }
    else if (BTN_CAMERA.contains(mx, my)) {
        currentScreen = Screen::CAMERA;
        cameraTimer = 0.f; cameraDone = false; cameraStatus.clear();
        decodedCode.clear(); statusMessage.clear();
    }
    else if (BTN_ABOUT.contains(mx, my)) {
        currentScreen = Screen::ABOUT;
    }
}

// ---------------- GENERATE ----------------
void BarcodeApp::drawGenerate() {
    sf::Text title("Generate Code 39 Barcode", font, 34);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(TEXT_MAIN);
    title.setPosition(60.f, 95.f);
    window.draw(title);

    drawButton("< Back", BTN_GEN_BACK.x, BTN_GEN_BACK.y,
        BTN_GEN_BACK.w, BTN_GEN_BACK.h, sf::Color(60, 60, 70));

    sf::RectangleShape panel({ 500.f, 70.f });
    panel.setPosition(300.f, 210.f);
    panel.setFillColor(PANEL);
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(ACCENT);
    window.draw(panel);

    sf::Text typed(inputBuffer.empty() ? "______" : inputBuffer,
        font, 34);
    typed.setStyle(sf::Text::Bold);
    typed.setFillColor(inputBuffer.empty() ? TEXT_DIM : sf::Color(255, 255, 140));
    typed.setLetterSpacing(3.f);
    typed.setPosition(315.f, 222.f);
    window.draw(typed);

    if ((int)inputBuffer.size() < 6) {
        sf::RectangleShape caret({ 2.f, 34.f });
        caret.setPosition(315.f + typed.getLocalBounds().width + 8.f, 225.f);
        caret.setFillColor(sf::Color(255, 255, 100));
        window.draw(caret);
    }

    sf::Text hint("Type 6 digits  (first digit 1-9)", font, 16);
    hint.setFillColor(TEXT_DIM);
    hint.setPosition(300.f, 288.f);
    window.draw(hint);

    drawButton("GENERATE & DISPLAY",
        BTN_GEN_SUBMIT.x, BTN_GEN_SUBMIT.y,
        BTN_GEN_SUBMIT.w, BTN_GEN_SUBMIT.h,
        sf::Color(25, 120, 200), ACCENT);

    if (hasBarcode && barcodeTexture.getSize().x > 0) {
        sf::RectangleShape frame({ 860.f, 210.f });
        frame.setPosition(120.f, 355.f);
        frame.setFillColor(sf::Color(250, 250, 250));
        frame.setOutlineThickness(3.f);
        frame.setOutlineColor(ACCENT);
        window.draw(frame);

        float maxW = 820.f, maxH = 180.f;
        sf::Vector2u sz = barcodeTexture.getSize();
        float scale = min(maxW / sz.x, maxH / sz.y);
        barcodeSprite.setTexture(barcodeTexture);
        barcodeSprite.setScale(scale, scale);
        barcodeSprite.setPosition((WIN_W - sz.x * scale) / 2.f, 370.f);
        window.draw(barcodeSprite);
    }

    drawStatus();
}

void BarcodeApp::handleGenerateClick(float mx, float my) {
    if (BTN_GEN_BACK.contains(mx, my)) { currentScreen = Screen::HOME; return; }
    if (BTN_GEN_SUBMIT.contains(mx, my)) {
        string err;
        if (!isValidSixDigitCode(inputBuffer, err)) {
            statusMessage = err; statusIsError = true; return;
        }
        if (inputBuffer[0] == '0') {
            statusMessage = "Error: the number must not start with 0.";
            statusIsError = true; return;
        }
        lastGeneratedFile = "barcode_" + inputBuffer + ".bmp";
        if (generateBarcodeBMP(inputBuffer, lastGeneratedFile)) {
            if (barcodeTexture.loadFromFile(lastGeneratedFile)) hasBarcode = true;
            statusMessage = "Saved: " + lastGeneratedFile;
            statusIsError = false;
        }
        else {
            statusMessage = "Could not write BMP file.";
            statusIsError = true;
        }
    }
}

// ---------------- DECODE ----------------
void BarcodeApp::drawDecode() {
    sf::Text title("Decode Barcode Image", font, 34);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(TEXT_MAIN);
    title.setPosition(60.f, 95.f);
    window.draw(title);

    drawButton("< Back", BTN_DEC_BACK.x, BTN_DEC_BACK.y,
        BTN_DEC_BACK.w, BTN_DEC_BACK.h, sf::Color(60, 60, 70));

    sf::Text lbl("BMP file path:", font, 18);
    lbl.setFillColor(TEXT_DIM);
    lbl.setPosition(60.f, 185.f);
    window.draw(lbl);

    sf::RectangleShape box({ 760.f, 60.f });
    box.setPosition(60.f, 220.f);
    box.setFillColor(PANEL);
    box.setOutlineThickness(2.f);
    box.setOutlineColor(ACCENT2);
    window.draw(box);

    sf::Text typed(decodePathBuffer.empty() ? "Click Browse to pick a .bmp file"
        : decodePathBuffer,
        font, 22);
    typed.setFillColor(decodePathBuffer.empty() ? TEXT_DIM : TEXT_MAIN);
    typed.setPosition(76.f, 232.f);
    window.draw(typed);

    drawButton("Browse...",
        BTN_DEC_BROWSE.x, BTN_DEC_BROWSE.y,
        BTN_DEC_BROWSE.w, BTN_DEC_BROWSE.h,
        sf::Color(90, 90, 110), ACCENT2);

    drawButton("DECODE", BTN_DEC_SUBMIT.x, BTN_DEC_SUBMIT.y,
        BTN_DEC_SUBMIT.w, BTN_DEC_SUBMIT.h,
        sf::Color(20, 150, 110), ACCENT2);

    if (!decodedCode.empty()) {
        sf::RectangleShape resultBox({ 620.f, 200.f });
        resultBox.setPosition((WIN_W - 620.f) / 2.f, 340.f);
        resultBox.setFillColor(sf::Color(20, 40, 30));
        resultBox.setOutlineThickness(3.f);
        resultBox.setOutlineColor(ACCENT2);
        window.draw(resultBox);

        sf::Text cap("EXTRACTED 6-DIGIT CODE", font, 18);
        cap.setFillColor(TEXT_DIM);
        sf::FloatRect cb = cap.getLocalBounds();
        cap.setPosition((WIN_W - cb.width) / 2.f, 360.f);
        window.draw(cap);

        sf::Text big(decodedCode, font, 110);
        big.setStyle(sf::Text::Bold);
        big.setFillColor(sf::Color(120, 240, 160));
        sf::FloatRect bb = big.getLocalBounds();
        big.setPosition((WIN_W - bb.width) / 2.f - bb.left, 390.f);
        window.draw(big);
    }

    drawStatus();
}

void BarcodeApp::handleDecodeClick(float mx, float my) {
    if (BTN_DEC_BACK.contains(mx, my)) { currentScreen = Screen::HOME; return; }
    if (BTN_DEC_BROWSE.contains(mx, my)) { openFileDialogForBmp(); return; }
    if (BTN_DEC_SUBMIT.contains(mx, my)) {
        string err, out;
        if (decodeBarcodeFromBMP(decodePathBuffer, out, err)) {
            decodedCode = out; statusMessage = "Success"; statusIsError = false;
        }
        else {
            decodedCode.clear(); statusMessage = err; statusIsError = true;
        }
    }
}

// Native Windows file dialog — no external library needed
void BarcodeApp::openFileDialogForBmp() {
    OPENFILENAMEA ofn = {};
    char fileBuf[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = window.getSystemHandle();
    ofn.lpstrFilter =
        "BMP Image (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select a barcode BMP image";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        decodePathBuffer = fileBuf;
        statusMessage.clear();
        statusIsError = false;
    }
}

// ---------------- CAMERA ----------------
void BarcodeApp::drawCamera() {
    sf::Text title("Live Camera Scanner", font, 34);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(TEXT_MAIN);
    title.setPosition(60.f, 95.f);
    window.draw(title);

    drawButton("< Back", BTN_CAM_BACK.x, BTN_CAM_BACK.y,
        BTN_CAM_BACK.w, BTN_CAM_BACK.h, sf::Color(60, 60, 70));

    sf::RectangleShape view({ 640.f, 360.f });
    view.setPosition((WIN_W - 640.f) / 2.f, 170.f);
    view.setFillColor(sf::Color(10, 12, 20));
    view.setOutlineThickness(3.f);
    view.setOutlineColor(sf::Color(160, 120, 255));
    window.draw(view);

    float scanY = 170.f + fmod(clockSeconds * 90.f, 360.f);
    sf::RectangleShape scanLine({ 640.f, 3.f });
    scanLine.setPosition((WIN_W - 640.f) / 2.f, scanY);
    scanLine.setFillColor(sf::Color(120, 255, 180, 220));
    window.draw(scanLine);

    auto bracket = [&](float x, float y, float dx, float dy) {
        sf::RectangleShape h({ 40.f, 4.f }); h.setPosition(x, y);
        sf::RectangleShape v({ 4.f, 40.f }); v.setPosition(x, y);
        h.setFillColor(ACCENT2); v.setFillColor(ACCENT2);
        if (dx < 0) h.move(-40.f, 0.f);
        if (dy < 0) v.move(0.f, -40.f);
        window.draw(h); window.draw(v);
    };
    float vx = (WIN_W - 640.f) / 2.f, vy = 170.f;
    bracket(vx + 20.f, vy + 20.f, 1.f, 1.f);
    bracket(vx + 620.f, vy + 20.f, -1.f, 1.f);
    bracket(vx + 20.f, vy + 340.f, 1.f, -1.f);
    bracket(vx + 620.f, vy + 340.f, -1.f, -1.f);

    sf::Text msg;
    msg.setFont(font);
    msg.setCharacterSize(18);
    msg.setFillColor(TEXT_DIM);
    msg.setPosition(vx + 20.f, vy + 380.f);

    if (!cameraDone) {
        msg.setString("Place a Code 39 barcode inside the frame.  "
            "Click SCAN to capture.");
        window.draw(msg);
    }
    else {
        msg.setString("Capture complete.");
        window.draw(msg);

        if (!decodedCode.empty()) {
            sf::Text big(decodedCode, font, 90);
            big.setStyle(sf::Text::Bold);
            big.setFillColor(sf::Color(120, 240, 160));
            sf::FloatRect bb = big.getLocalBounds();
            big.setPosition((WIN_W - bb.width) / 2.f - bb.left, 500.f);
            window.draw(big);
        }
    }

    drawButton(cameraDone ? "SCAN AGAIN" : "SCAN",
        BTN_CAM_START.x, BTN_CAM_START.y,
        BTN_CAM_START.w, BTN_CAM_START.h,
        sf::Color(90, 60, 160), sf::Color(160, 120, 255));

    drawStatus();
}

void BarcodeApp::handleCameraClick(float mx, float my) {
    if (BTN_CAM_BACK.contains(mx, my)) { currentScreen = Screen::HOME; return; }

    if (BTN_CAM_START.contains(mx, my)) {
        string fileName = lastGeneratedFile.empty()
            ? "barcode.bmp"
            : lastGeneratedFile;
        string err, out;
        if (decodeBarcodeFromBMP(fileName, out, err)) {
            decodedCode = out;
            cameraStatus = "Decoded from " + fileName;
            statusMessage = cameraStatus;
            statusIsError = false;
            cameraDone = true;
        }
        else {
            decodedCode.clear();
            statusMessage = "Camera: " + err +
                "  (Generate a barcode first to test the camera path.)";
            statusIsError = true;
            cameraDone = true;
        }
    }
}

// ---------------- ABOUT ----------------
void BarcodeApp::drawAbout() {
    sf::Text title("About", font, 34);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(TEXT_MAIN);
    title.setPosition(60.f, 95.f);
    window.draw(title);

    drawButton("< Back", BTN_ABOUT_BACK.x, BTN_ABOUT_BACK.y,
        BTN_ABOUT_BACK.w, BTN_ABOUT_BACK.h, sf::Color(60, 60, 70));

    string lines[] = {
        "ISE Smart Barcode System - Increment 1",
        "Fast-NUCES  |  Software Engineering  |  Sprint 1",
        "",
        "Team members:",
        "  - Afwan",
        "  - Ahmed",
        "  - Abdul Wadood",
        "  - Muhammad Raheel",
        "",
        "This desktop application encodes and decodes",
        "Code 39 barcodes for six-digit product codes.",
        "",
        "Camera path in Increment 1 uses the last generated BMP",
        "as the captured frame.  OpenCV integration is scheduled",
        "for Increment 2.",
    };
    float y = 180.f;
    for (const auto& l : lines) {
        sf::Text t(l, font, 18);
        t.setFillColor(TEXT_DIM);
        t.setPosition(80.f, y);
        window.draw(t);
        y += 26.f;
    }
}

void BarcodeApp::handleAboutClick(float mx, float my) {
    if (BTN_ABOUT_BACK.contains(mx, my)) currentScreen = Screen::HOME;
}

// ---------------- INPUT ----------------
void BarcodeApp::handleTextEntered(sf::Uint32 unicode) {
    if (unicode == 8) { handleBackspace(); return; }
    if (unicode < 32 || unicode > 126) return;
    char c = (char)unicode;

    if (currentScreen == Screen::GENERATE) {
        if (isdigit((unsigned char)c) && inputBuffer.size() < 6)
            inputBuffer.push_back(c);
    }
    else if (currentScreen == Screen::DECODE) {
        if (decodePathBuffer.size() < 200) decodePathBuffer.push_back(c);
    }
}

void BarcodeApp::handleBackspace() {
    if (currentScreen == Screen::GENERATE && !inputBuffer.empty())
        inputBuffer.pop_back();
    else if (currentScreen == Screen::DECODE && !decodePathBuffer.empty())
        decodePathBuffer.pop_back();
}

// ---------------- MAIN LOOP ----------------
void BarcodeApp::run() {
    sf::Clock frameClock;
    while (window.isOpen()) {
        clockSeconds += frameClock.restart().asSeconds();

        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            if (e.type == sf::Event::KeyPressed &&
                e.key.code == sf::Keyboard::Escape) window.close();
            if (e.type == sf::Event::TextEntered)
                handleTextEntered(e.text.unicode);
            if (e.type == sf::Event::MouseButtonPressed &&
                e.mouseButton.button == sf::Mouse::Left)
            {
                float mx = (float)e.mouseButton.x;
                float my = (float)e.mouseButton.y;
                switch (currentScreen) {
                case Screen::HOME:     handleHomeClick(mx, my); break;
                case Screen::GENERATE: handleGenerateClick(mx, my); break;
                case Screen::DECODE:   handleDecodeClick(mx, my); break;
                case Screen::CAMERA:   handleCameraClick(mx, my); break;
                case Screen::ABOUT:    handleAboutClick(mx, my); break;
                }
            }
        }

        window.clear(BG_BOTTOM);
        drawBackground();

        switch (currentScreen) {
        case Screen::HOME:     drawHome();     break;
        case Screen::GENERATE: drawGenerate(); break;
        case Screen::DECODE:   drawDecode();   break;
        case Screen::CAMERA:   drawCamera();   break;
        case Screen::ABOUT:    drawAbout();    break;
        }

        window.display();
    }
}
#include "PosProductsView.h"
#include "UIUtils.h"
#include <cmath>
#include <ctime>
#include <algorithm>

void PosProductsView::drawViewport(sf::RenderTarget& target, float mainX,
                                   const sf::Texture& cameraTexture,
                                   float flashTimer, float lastScanTimer, const std::string& lastScannedInfo,
                                   bool barcodeFound, const BarcodeBox& barcodeBox,
                                   int camWidth, int camHeight, const std::string& detectedBarcode,
                                   const std::string& cameraName, bool isExternalCam, int cameraCount,
                                   const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                   std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::FloatRect r{mainX, 135.f, Layout::MAIN_W, 160.f};
    drawShadow(target, r.left, r.top, r.width, r.height, 14, 5, 8, 10, 3);

    // Sleek camera viewport container background
    drawRounded(target, r.left, r.top, r.width, r.height, 12, sf::Color(16, 16, 18));

    // Full camera pane: Entire box dedicated to camera stream!
    float paneX = r.left + 4.f;
    float paneY = r.top + 4.f;
    float paneW = r.width - 8.f;
    float paneH = r.height - 8.f;

    float cropX = 0.f, cropY = 0.f, cropW = 1.f, cropH = 1.f;
    float scaleX = 1.f, scaleY = 1.f;

    if (cameraTexture.getSize().x > 0) {
        sf::Sprite camSprite(cameraTexture);
        float srcW = (float)cameraTexture.getSize().x;
        float srcH = (float)cameraTexture.getSize().y;
        float targetAspect = paneW / paneH;
        cropH = srcW / targetAspect;
        cropW = srcW;
        if (cropH <= srcH) {
            cropY = (srcH - cropH) / 2.f;
            cropX = 0.f;
            camSprite.setTextureRect(sf::IntRect(0, (int)cropY, (int)srcW, (int)cropH));
        } else {
            cropW = srcH * targetAspect;
            cropX = (srcW - cropW) / 2.f;
            cropY = 0.f;
            cropH = srcH;
            camSprite.setTextureRect(sf::IntRect((int)cropX, 0, (int)cropW, (int)srcH));
        }
        scaleX = paneW / camSprite.getTextureRect().width;
        scaleY = paneH / camSprite.getTextureRect().height;
        camSprite.setPosition(paneX, paneY);
        camSprite.setScale(scaleX, scaleY);
        target.draw(camSprite);
    } else {
        drawRounded(target, paneX, paneY, paneW, paneH, 8, sf::Color(22, 22, 26));
        drawSprite(target, getTexture("icons/scan.png"), paneX + paneW / 2.f - 24.f, paneY + paneH / 2.f - 30.f, 48.f, 48.f, sf::Color(200, 200, 200, 160));
        drawText(target, fontSemiBold, "Webcam Optical Scanner Active", 12.f, sf::Color(230, 230, 230), paneX + paneW / 2.f, paneY + paneH / 2.f + 24.f, TextAlignment::Center);
    }

    // 4 Corner Optical Reticles
    float retSize = 20.f;
    float retThick = 2.5f;
    float retPad = 12.f;
    sf::Color retColor(255, 255, 255, 200);
    // Top-Left
    sf::RectangleShape cTL1({retSize, retThick}); cTL1.setPosition(r.left + retPad, r.top + retPad); cTL1.setFillColor(retColor); target.draw(cTL1);
    sf::RectangleShape cTL2({retThick, retSize}); cTL2.setPosition(r.left + retPad, r.top + retPad); cTL2.setFillColor(retColor); target.draw(cTL2);
    // Top-Right
    sf::RectangleShape cTR1({retSize, retThick}); cTR1.setPosition(r.left + r.width - retPad - retSize, r.top + retPad); cTR1.setFillColor(retColor); target.draw(cTR1);
    sf::RectangleShape cTR2({retThick, retSize}); cTR2.setPosition(r.left + r.width - retPad - retThick, r.top + retPad); cTR2.setFillColor(retColor); target.draw(cTR2);
    // Bottom-Left
    sf::RectangleShape cBL1({retSize, retThick}); cBL1.setPosition(r.left + retPad, r.top + r.height - retPad - retThick); cBL1.setFillColor(retColor); target.draw(cBL1);
    sf::RectangleShape cBL2({retThick, retSize}); cBL2.setPosition(r.left + retPad, r.top + r.height - retPad - retSize); cBL2.setFillColor(retColor); target.draw(cBL2);
    // Bottom-Right
    sf::RectangleShape cBR1({retSize, retThick}); cBR1.setPosition(r.left + r.width - retPad - retSize, r.top + r.height - retPad - retThick); cBR1.setFillColor(retColor); target.draw(cBR1);
    sf::RectangleShape cBR2({retThick, retSize}); cBR2.setPosition(r.left + r.width - retPad - retThick, r.top + r.height - retPad - retSize); cBR2.setFillColor(retColor); target.draw(cBR2);

    // Live continuous status badge at top-left
    float badgeW = barcodeFound ? 165.f : 142.f;
    drawRounded(target, r.left + 14.f, r.top + 12.f, badgeW, 20.f, 5.f, sf::Color(15, 15, 18, 210), sf::Color(255, 255, 255, 45), 1.f);
    sf::CircleShape liveDot(3.5f, 16);
    liveDot.setOrigin(3.5f, 3.5f);
    liveDot.setPosition(r.left + 24.f, r.top + 22.f);
    liveDot.setFillColor(barcodeFound ? sf::Color(34, 197, 94) : ThemeColor::Green);
    target.draw(liveDot);
    std::string badgeLabel = barcodeFound ? ("OPTICAL LOCK: " + detectedBarcode) : "CONTINUOUS SCANNER";
    drawText(target, fontSemiBold, badgeLabel, 7.8f, sf::Color(245, 245, 245), r.left + 33.f, r.top + 22.f);

    // Active Camera Source & Quick Switch Pill at top-right
    float camBadgeW = 215.f;
    float camBadgeX = r.left + r.width - camBadgeW - 14.f;
    float camBadgeY = r.top + 11.f;
    sf::Color badgeBg = isExternalCam ? sf::Color(15, 75, 40, 220) : sf::Color(28, 36, 52, 220);
    sf::Color badgeBorder = isExternalCam ? sf::Color(34, 197, 94, 200) : sf::Color(148, 163, 184, 130);
    drawRounded(target, camBadgeX, camBadgeY, camBadgeW, 22.f, 6.f, badgeBg, badgeBorder, 1.2f);

    std::string camPrefix = isExternalCam ? "[Phone] " : "[Laptop] ";
    std::string cleanName = cameraName;
    if (cleanName.length() > 14) cleanName = cleanName.substr(0, 12) + "..";
    std::string pillText = camPrefix + cleanName + (cameraCount > 1 ? " | Switch (C)" : "");
    drawText(target, fontSemiBold, pillText, 8.f, isExternalCam ? sf::Color(220, 252, 231) : sf::Color(241, 245, 249),
             camBadgeX + camBadgeW / 2.f, camBadgeY + 11.f, TextAlignment::Center);

    // Outer border
    drawRounded(target, r.left, r.top, r.width, r.height, 12, ThemeColor::None, sf::Color(140, 130, 120, 140), 1.2f);

    // Real-Time Bounding Box for Detected Barcode (ISE-28 Requirement)
    if (barcodeFound && barcodeBox.detected && camWidth > 0 && camHeight > 0) {
        float bx1 = paneX + (barcodeBox.x1 - cropX) * scaleX;
        float by1 = paneY + (barcodeBox.y1 - cropY) * scaleY;
        float bx2 = paneX + (barcodeBox.x2 - cropX) * scaleX;
        float by2 = paneY + (barcodeBox.y2 - cropY) * scaleY;

        bx1 = std::clamp(bx1, paneX + 2.f, paneX + paneW - 20.f);
        bx2 = std::clamp(bx2, bx1 + 25.f, paneX + paneW - 2.f);
        by1 = std::clamp(by1, paneY + 2.f, paneY + paneH - 20.f);
        by2 = std::clamp(by2, by1 + 25.f, paneY + paneH - 2.f);

        float bw = bx2 - bx1;
        float bh = by2 - by1;

        // Glowing semi-transparent green box fill
        drawRounded(target, bx1, by1, bw, bh, 6.f, sf::Color(34, 197, 94, 45), sf::Color(34, 197, 94, 235), 2.2f);

        // 4 High-Tech Corner Reticles
        float cLen = std::min(14.f, std::min(bw, bh) / 3.f);
        float cThick = 3.f;
        sf::Color neonGreen(34, 197, 94, 255);
        // Top-Left corner
        sf::RectangleShape bTL1({cLen, cThick}); bTL1.setPosition(bx1, by1); bTL1.setFillColor(neonGreen); target.draw(bTL1);
        sf::RectangleShape bTL2({cThick, cLen}); bTL2.setPosition(bx1, by1); bTL2.setFillColor(neonGreen); target.draw(bTL2);
        // Top-Right corner
        sf::RectangleShape bTR1({cLen, cThick}); bTR1.setPosition(bx2 - cLen, by1); bTR1.setFillColor(neonGreen); target.draw(bTR1);
        sf::RectangleShape bTR2({cThick, cLen}); bTR2.setPosition(bx2 - cThick, by1); bTR2.setFillColor(neonGreen); target.draw(bTR2);
        // Bottom-Left corner
        sf::RectangleShape bBL1({cLen, cThick}); bBL1.setPosition(bx1, by2 - cThick); bBL1.setFillColor(neonGreen); target.draw(bBL1);
        sf::RectangleShape bBL2({cThick, cLen}); bBL2.setPosition(bx1, by2 - cLen); bBL2.setFillColor(neonGreen); target.draw(bBL2);
        // Bottom-Right corner
        sf::RectangleShape bBR1({cLen, cThick}); bBR1.setPosition(bx2 - cLen, by2 - cThick); bBR1.setFillColor(neonGreen); target.draw(bBR1);
        sf::RectangleShape bBR2({cThick, cLen}); bBR2.setPosition(bx2 - cThick, by2 - cLen); bBR2.setFillColor(neonGreen); target.draw(bBR2);

        // Detection tag badge above bounding box
        float tagW = 150.f;
        float tagH = 18.f;
        float tagY = std::max(paneY + 4.f, by1 - tagH - 4.f);
        float tagX = std::clamp(bx1, paneX + 4.f, paneX + paneW - tagW - 4.f);
        drawRounded(target, tagX, tagY, tagW, tagH, 4.f, sf::Color(16, 80, 40, 240), sf::Color(34, 197, 94, 220), 1.f);
        drawText(target, fontBold, "[ ✔ CODE: " + (detectedBarcode.empty() ? "DETECTED" : detectedBarcode) + " ]", 8.5f, sf::Color(240, 255, 240), tagX + tagW / 2.f, tagY + 9.f, TextAlignment::Center);
    }

    // Animated Moving Red Laser Scan Line sweeping up and down across the ENTIRE camera pane
    float pulse = 0.75f + 0.25f * std::sin((float)clock() / CLOCKS_PER_SEC * 6.f);
    float sweep = 0.5f + 0.5f * std::sin((float)clock() / CLOCKS_PER_SEC * 2.2f);
    float minY = paneY + 12.f;
    float maxY = paneY + paneH - 12.f;
    float ly = minY + sweep * (maxY - minY);

    for (int i = 4; i > 0; i--) {
        sf::RectangleShape glow({paneW, i * 3.f});
        glow.setPosition(paneX, ly - i * 1.5f);
        glow.setFillColor(sf::Color(226, 60, 50, (sf::Uint8)(16 * pulse)));
        target.draw(glow);
    }
    sf::RectangleShape laser({paneW, 2.0f});
    laser.setPosition(paneX, ly - 1.0f);
    laser.setFillColor(sf::Color(255, 45, 40, (sf::Uint8)(240 * pulse)));
    target.draw(laser);

    // Feedback banner when scanned
    if (lastScanTimer > 0.f && !lastScannedInfo.empty()) {
        float alpha = std::min(1.f, lastScanTimer) * 230;
        drawRounded(target, r.left + 15, r.top + r.height - 34, r.width - 30, 24, 6,
                    sf::Color(30, 80, 50, (sf::Uint8)alpha));
        drawSprite(target, getTexture("icons/check.png"), r.left + 22, r.top + r.height - 29, 14, 14,
                    sf::Color(180, 255, 180, (sf::Uint8)alpha));
        drawText(target, fontSemiBold, "Scanned: " + lastScannedInfo + " -> Added to receipt", 9.5f,
                 sf::Color(255, 255, 255, (sf::Uint8)alpha), r.left + 42, r.top + r.height - 22);
    }

    if (flashTimer > 0.f) {
        drawRounded(target, r.left, r.top, r.width, r.height, 12,
                    sf::Color(255, 255, 255, (sf::Uint8)(flashTimer / 0.4f * 130)));
    }
}

void PosProductsView::drawProducts(sf::RenderTarget& target, sf::Vector2f m, float mainX,
                                  const std::vector<ProductItem>& products,
                                  const std::vector<int>& visible,
                                  const std::string& activeCategory,
                                  const std::vector<ProductCategory>& categories,
                                  const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                  const sf::Font& fontBold, const sf::Font& fontSerif,
                                  std::function<const sf::Texture&(const std::string&)> getTexture) {
    drawText(target, fontSerif, "Products", 18, ThemeColor::Ink, mainX, 322);

    // Category filter pills from PCategory.txt (always starting with "All" left-to-right so none are dropped)
    std::vector<std::string> pillNames = {"All"};
    for (const auto& cat : categories) {
        if (!cat.name.empty()) pillNames.push_back(cat.name);
    }

    float pillX = mainX + 94.f;
    float pillY = 313.f;
    for (size_t i = 0; i < pillNames.size(); i++) {
        std::string label = pillNames[i];
        if (label == "Vegetables") label = "Veg";
        float w = getTextWidth(fontSemiBold, label, 8.f) + 12.f;
        if (pillX + w > mainX + Layout::MAIN_W + 2.f) break;

        bool selected = (activeCategory == pillNames[i]);
        bool hovered = pointInRect(m, pillX, pillY, w, 21.f);

        if (selected) {
            drawRounded(target, pillX, pillY, w, 21.f, 10.5f, ThemeColor::Dark);
        } else if (hovered) {
            drawRounded(target, pillX, pillY, w, 21.f, 10.5f, sf::Color(232, 224, 207));
        }
        drawText(target, fontSemiBold, label, 8.f, selected ? ThemeColor::White : ThemeColor::Mute,
                 pillX + w / 2.f, pillY + 10.5f, TextAlignment::Center, 0.f);

        pillX += w + 3.f;
    }

    if (visible.empty()) {
        drawText(target, fontRegular, "No products found matching search", 13, ThemeColor::Mute,
                 mainX + Layout::MAIN_W / 2.f, 440, TextAlignment::Center);
    }

    for (int k = 0; k < (int)visible.size() && k < 6; k++) {
        const auto& p = products[visible[k]];
        float cx = mainX + (k % 3) * (Layout::CARD_W + Layout::CARD_GAP);
        float cy = Layout::CARD_Y + (k / 3) * (Layout::CARD_H + Layout::CARD_GAP);
        bool hovered = pointInRect(m, cx, cy, Layout::CARD_W, Layout::CARD_H);
        float lift = hovered ? -2.f : 0.f;

        drawShadow(target, cx, cy + lift, Layout::CARD_W, Layout::CARD_H, 10, 4, hovered ? 10.f : 5.f, hovered ? 14 : 8, 2);
        drawRounded(target, cx, cy + lift, Layout::CARD_W, Layout::CARD_H, 10, ThemeColor::Paper,
                    hovered ? ThemeColor::Ink : ThemeColor::Edge, hovered ? 1.4f : 1.f);

        drawRounded(target, cx + 8, cy + lift + 8, Layout::CARD_W - 16, 72, 6, p.cardTint);
        drawSprite(target, getTexture("products/" + p.iconKey + ".png"), cx + Layout::CARD_W / 2.f - 26, cy + lift + 18, 52, 52);

        if (p.discount > 0) {
            drawRounded(target, cx + Layout::CARD_W - 58, cy + lift + 12, 46, 16, 4, sf::Color(254, 226, 226));
            drawText(target, fontBold, "-Rs " + std::to_string(p.discount), 7.5f, ThemeColor::Red, cx + Layout::CARD_W - 35, cy + lift + 20, TextAlignment::Center);
        }

        std::string dispName = p.name;
        if (dispName.length() > 16) dispName = dispName.substr(0, 14) + "..";
        drawText(target, fontSemiBold, dispName, 11.5f, ThemeColor::Ink, cx + 10, cy + lift + 94);
        drawText(target, fontRegular, p.unit, 9, ThemeColor::Mute, cx + 10, cy + lift + 112);
        drawText(target, fontBold, formatRs(p.price), 11, ThemeColor::Ink, cx + Layout::CARD_W - 10, cy + lift + 103, TextAlignment::Right);
    }
}

void PosProductsView::drawProductsTab(sf::RenderTarget& target, sf::Vector2f /*m*/, float mainX,
                                     const std::vector<ProductItem>& products,
                                     float scrollOffset, const std::string& currentRole,
                                     const std::string& productFilePath,
                                     const std::string& searchQuery, bool searchFocused,
                                     const sf::Font& fontRegular, const sf::Font& fontMedium,
                                     const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                     const sf::Font& fontSerif) {
    drawText(target, fontSerif, "Product Catalog", 21, ThemeColor::Ink, mainX, 60);

    // Search bar
    float searchW = 160.f;
    float searchX = mainX + Layout::MAIN_W - 140.f - searchW - 10.f;
    drawRounded(target, searchX, 48, searchW, 30, 6, ThemeColor::Paper, searchFocused ? ThemeColor::Blue : ThemeColor::Line, 1.2f);
    std::string sDisplay = searchQuery.empty() ? (searchFocused ? "|" : "Search products...") : (searchQuery + (searchFocused ? "|" : ""));
    drawText(target, fontRegular, sDisplay, 9.f, searchQuery.empty() ? ThemeColor::Mute : ThemeColor::Ink, searchX + 10, 63);

    // + Add Product (stocks from Inventory or creates new product)
    if (currentRole == "Admin") {
        drawRounded(target, mainX + Layout::MAIN_W - 140, 48, 140, 30, 6, ThemeColor::Blue);
        drawText(target, fontSemiBold, "+ Add Product", 9.5f, ThemeColor::White, mainX + Layout::MAIN_W - 70, 63, TextAlignment::Center);
    } else {
        drawRounded(target, mainX + Layout::MAIN_W - 140, 48, 140, 30, 6, sf::Color(230, 222, 210), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, "[Manager] + Add", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 70, 63, TextAlignment::Center);
    }

    float ty = 92.f;
    drawRounded(target, mainX, ty, Layout::MAIN_W, 570, 10, ThemeColor::Paper, ThemeColor::Line, 1);

    // Table Header
    drawText(target, fontSemiBold, "Barcode", 9.5f, ThemeColor::Mute, mainX + 15, ty + 16);
    drawText(target, fontSemiBold, "Item Name", 9.5f, ThemeColor::Mute, mainX + 85, ty + 16);
    drawText(target, fontSemiBold, "Category", 9.5f, ThemeColor::Mute, mainX + 215, ty + 16);
    drawText(target, fontSemiBold, "Price", 9.5f, ThemeColor::Mute, mainX + 305, ty + 16);
    drawText(target, fontSemiBold, "Stock", 9.5f, ThemeColor::Mute, mainX + 365, ty + 16);
    drawText(target, fontSemiBold, "Actions", 9.5f, ThemeColor::Mute, mainX + 465, ty + 16, TextAlignment::Center);

    sf::RectangleShape hdrLine({Layout::MAIN_W - 30, 1});
    hdrLine.setPosition(mainX + 15, ty + 28);
    hdrLine.setFillColor(ThemeColor::Line);
    target.draw(hdrLine);

    // Filter indices by search query (case-insensitive)
    std::string qLower = searchQuery;
    std::transform(qLower.begin(), qLower.end(), qLower.begin(), ::tolower);

    std::vector<int> visibleIndices;
    for (size_t i = 0; i < products.size(); i++) {
        if (qLower.empty()) {
            visibleIndices.push_back(static_cast<int>(i));
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
            visibleIndices.push_back(static_cast<int>(i));
        }
    }

    for (size_t vi = 0; vi < visibleIndices.size(); vi++) {
        int idx = visibleIndices[vi];
        float rowY = ty + 44.f + static_cast<float>(vi) * 44.f - scrollOffset;
        if (rowY < ty + 34.f || rowY + 24.f > ty + 562.f) continue;

        const auto& p = products[idx];
        drawText(target, fontMedium, p.barcode, 9.5f, ThemeColor::Ink, mainX + 15, rowY + 5);
        std::string dName = p.name;
        if (dName.length() > 17) dName = dName.substr(0, 15) + "..";
        drawText(target, fontBold, dName, 10.f, ThemeColor::Ink, mainX + 85, rowY + 5);
        std::string dCat = p.category;
        if (dCat.length() > 13) dCat = dCat.substr(0, 11) + "..";
        drawText(target, fontRegular, dCat, 9.f, ThemeColor::Mute, mainX + 215, rowY + 5);
        drawText(target, fontBold, formatRs(p.price), 10.f, ThemeColor::Ink, mainX + 305, rowY + 5);

        bool low = p.stock < 15;
        drawText(target, fontSemiBold, std::to_string(p.stock), 10.f,
                 p.stock == 0 ? ThemeColor::Red : (low ? ThemeColor::Amber : ThemeColor::Green),
                 mainX + 365, rowY + 5);

        if (currentRole == "Admin") {
            drawRounded(target, mainX + 406, rowY - 6, 38, 22, 4, sf::Color(240, 245, 250), ThemeColor::Blue, 1.f);
            drawText(target, fontSemiBold, "Specs", 8.f, ThemeColor::Blue, mainX + 425, rowY + 5, TextAlignment::Center);

            drawRounded(target, mainX + 448, rowY - 6, 38, 22, 4, sf::Color(220, 235, 250), sf::Color(37, 99, 235), 1.f);
            drawText(target, fontSemiBold, "Edit", 8.f, sf::Color(29, 78, 216), mainX + 467, rowY + 5, TextAlignment::Center);

            drawRounded(target, mainX + 490, rowY - 6, 38, 22, 4, sf::Color(254, 226, 226), sf::Color(220, 80, 70), 1.f);
            drawText(target, fontSemiBold, "Del", 8.f, ThemeColor::Red, mainX + 509, rowY + 5, TextAlignment::Center);
        } else {
            drawRounded(target, mainX + 418, rowY - 6, 46, 22, 4, sf::Color(240, 245, 250), ThemeColor::Blue, 1.f);
            drawText(target, fontSemiBold, "Specs", 8.5f, ThemeColor::Blue, mainX + 441, rowY + 5, TextAlignment::Center);

            drawRounded(target, mainX + 470, rowY - 6, 52, 22, 4, sf::Color(235, 230, 220));
            drawText(target, fontRegular, "Locked", 8.f, ThemeColor::Mute, mainX + 496, rowY + 5, TextAlignment::Center);
        }

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, rowY + 22);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);
    }
}

void PosProductsView::drawInventoryTab(sf::RenderTarget& target, sf::Vector2f /*m*/, float mainX,
                                       const std::vector<ProductItem>& products,
                                       const InventoryManager& inventoryMgr,
                                       int inventorySubTab, float scrollOffset,
                                       const std::string& currentRole,
                                       const std::string& searchQuery, bool searchFocused,
                                       const sf::Font& fontRegular, const sf::Font& fontMedium,
                                       const sf::Font& fontSemiBold,
                                       const sf::Font& fontBold, const sf::Font& fontSerif) {
    drawText(target, fontSerif, "Inventory & Supplier Procurement", 22, ThemeColor::Ink, mainX, 46);

    const auto& invItems = inventoryMgr.getItems();
    const auto& purchases = inventoryMgr.getPurchases();

    int healthy = 0, lowStock = 0, outOfStock = 0;
    std::vector<std::string> uniqueSuppliers = inventoryMgr.getUniqueSuppliers();
    for (const auto& it : invItems) {
        if (it.stock == 0) outOfStock++;
        else if (it.stock < 15) lowStock++;
        else healthy++;
    }

    auto drawKpi = [&](const std::string& label, int count, sf::Color col, float x) {
        drawRounded(target, x, 88, 125, 44, 8, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontRegular, label, 8.5f, ThemeColor::Mute, x + 10, 98);
        drawText(target, fontBold, std::to_string(count), 14, col, x + 10, 116);
    };

    drawKpi("Total Items", (int)invItems.size(), ThemeColor::Ink, mainX);
    drawKpi("Healthy Stock", healthy, ThemeColor::Green, mainX + 138);
    drawKpi("Low Stock (<15)", lowStock, ThemeColor::Amber, mainX + 276);
    drawKpi("Active Suppliers", (int)uniqueSuppliers.size(), ThemeColor::Blue, mainX + 414);

    // Sub-tab switcher, Search bar, and Add Supplier button
    float tabY = 140.f;
    bool subTab0 = (inventorySubTab == 0);
    bool subTab1 = (inventorySubTab == 1);

    // Pill 1: Stock & Items
    drawRounded(target, mainX, tabY, 115, 26, 6, subTab0 ? ThemeColor::Dark : sf::Color(235, 227, 212), ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "Stock (" + std::to_string(invItems.size()) + ")", 9.f, subTab0 ? ThemeColor::White : ThemeColor::Ink, mainX + 57, tabY + 12, TextAlignment::Center);

    // Pill 2: Supplier Purchase Ledger
    drawRounded(target, mainX + 122, tabY, 120, 26, 6, subTab1 ? ThemeColor::Dark : sf::Color(235, 227, 212), ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "Ledger (" + std::to_string(purchases.size()) + ")", 9.f, subTab1 ? ThemeColor::White : ThemeColor::Ink, mainX + 182, tabY + 12, TextAlignment::Center);

    // Search bar
    float invSearchW = 140.f;
    float invSearchX = mainX + 250.f;
    drawRounded(target, invSearchX, tabY, invSearchW, 26, 6, ThemeColor::Paper, searchFocused ? ThemeColor::Blue : ThemeColor::Line, 1.2f);
    std::string invSDisplay = searchQuery.empty() ? (searchFocused ? "|" : "Search...") : (searchQuery + (searchFocused ? "|" : ""));
    drawText(target, fontRegular, invSDisplay, 8.5f, searchQuery.empty() ? ThemeColor::Mute : ThemeColor::Ink, invSearchX + 8, tabY + 13);

    // Top Right: + Add Supplier / Purchase Item (Admin only)
    if (currentRole == "Admin") {
        drawRounded(target, mainX + Layout::MAIN_W - 145, tabY, 145, 26, 6, ThemeColor::Blue);
        drawText(target, fontSemiBold, "+ Add Supplier Item", 9.f, ThemeColor::White, mainX + Layout::MAIN_W - 72, tabY + 13, TextAlignment::Center);
    } else {
        drawRounded(target, mainX + Layout::MAIN_W - 145, tabY, 145, 26, 6, sf::Color(230, 222, 210), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, "[Manager] + Add", 8.5f, ThemeColor::Mute, mainX + Layout::MAIN_W - 72, tabY + 13, TextAlignment::Center);
    }

    float ty = 174.f;
    drawRounded(target, mainX, ty, Layout::MAIN_W, 490, 10, ThemeColor::Paper, ThemeColor::Line, 1);

    if (currentRole != "Admin") {
        drawRounded(target, mainX + 15, ty + 8, Layout::MAIN_W - 30, 24, 6, sf::Color(254, 243, 199), sf::Color(217, 119, 6), 1.f);
        drawText(target, fontSemiBold, "[Employee View] Supplier costs & restock permissions are read-only. Editing requires Manager credentials.", 8.f, sf::Color(146, 64, 14), mainX + 25, ty + 20);
    }

    float listStartY = (currentRole != "Admin") ? (ty + 38.f) : (ty + 12.f);

    if (inventorySubTab == 0) {
        // -------------------------------------------------------------
        // SUB-TAB 0: CURRENT STOCK & ITEMS
        // -------------------------------------------------------------
        std::vector<int> filteredIndices = inventoryMgr.getFilteredItemIndices(searchQuery);

        for (size_t i = 0; i < filteredIndices.size(); i++) {
            int idx = filteredIndices[i];
            float rowY = listStartY + 12.f + static_cast<float>(i) * 54.f - scrollOffset;
            if (rowY < listStartY + 6.f || rowY + 42.f > ty + 482.f) continue;

            const auto& item = invItems[idx];
            std::string dName = item.name;
            if (dName.length() > 24) dName = dName.substr(0, 22) + "..";
            drawText(target, fontBold, dName, 11.5f, ThemeColor::Ink, mainX + 15, rowY + 4);

            // Clean line truncated safely before [Description] button at mainX + 250
            std::string suppInfo = item.supplier.empty() ? "Direct Inbound" : item.supplier;
            std::string infoLine = item.barcode + " | " + item.category + " (" + item.unit + ") | " + suppInfo;
            if (infoLine.length() > 36) infoLine = infoLine.substr(0, 34) + "..";
            drawText(target, fontRegular, infoLine, 8.5f, ThemeColor::Mute, mainX + 15, rowY + 19);

            std::string priceLine = "Retail: Rs " + std::to_string(item.retailPrice);
            if (currentRole == "Admin" && item.costPrice > 0) {
                priceLine += " | Cost: Rs " + std::to_string(item.costPrice);
            }
            drawText(target, fontSemiBold, priceLine, 8.5f, sf::Color(100, 90, 80), mainX + 15, rowY + 33);

            // [ Description ] popup button
            drawRounded(target, mainX + 250, rowY + 10, 68, 22, 4, sf::Color(240, 245, 250), ThemeColor::Blue, 1.f);
            drawText(target, fontSemiBold, "Description", 8.f, ThemeColor::Blue, mainX + 284, rowY + 21, TextAlignment::Center);

            // Stock badge
            bool low = item.stock < 15;
            drawRounded(target, mainX + 325, rowY + 10, 72, 22, 5,
                        item.stock == 0 ? sf::Color(254, 226, 226) : (low ? sf::Color(254, 243, 199) : sf::Color(220, 252, 231)));
            drawText(target, fontSemiBold, std::to_string(item.stock) + (item.stock == 0 ? " Empty" : (low ? " Low" : " In Stk")), 8.f,
                     item.stock == 0 ? ThemeColor::Red : (low ? ThemeColor::Amber : sf::Color(22, 101, 52)),
                     mainX + 361, rowY + 21, TextAlignment::Center);

            if (currentRole == "Admin") {
                drawRounded(target, mainX + 405, rowY + 10, 36, 22, 4, sf::Color(230, 238, 246), ThemeColor::Blue, 1);
                drawText(target, fontSemiBold, "+10", 8.5f, ThemeColor::Ink, mainX + 423, rowY + 21, TextAlignment::Center);

                drawRounded(target, mainX + 447, rowY + 10, 36, 22, 4, sf::Color(230, 238, 246), ThemeColor::Blue, 1);
                drawText(target, fontSemiBold, "+50", 8.5f, ThemeColor::Ink, mainX + 465, rowY + 21, TextAlignment::Center);

                drawRounded(target, mainX + 489, rowY + 10, 42, 22, 4, ThemeColor::Blue);
                drawText(target, fontSemiBold, "Order", 8.f, ThemeColor::White, mainX + 510, rowY + 21, TextAlignment::Center);
            } else {
                drawRounded(target, mainX + 405, rowY + 10, 126, 22, 4, sf::Color(240, 235, 225));
                drawText(target, fontRegular, "[Restock Locked]", 8.f, ThemeColor::Mute, mainX + 468, rowY + 21, TextAlignment::Center);
            }

            sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
            sep.setPosition(mainX + 15, rowY + 44);
            sep.setFillColor(ThemeColor::Line);
            target.draw(sep);
        }
    } else {
        // -------------------------------------------------------------
        // SUB-TAB 1: SUPPLIER PROCUREMENT LEDGER
        // -------------------------------------------------------------
        drawText(target, fontSemiBold, "Date & Supplier", 9.f, ThemeColor::Mute, mainX + 15, listStartY + 4);
        drawText(target, fontSemiBold, "Barcode / Item Purchased", 9.f, ThemeColor::Mute, mainX + 150, listStartY + 4);
        drawText(target, fontSemiBold, "Detail", 9.f, ThemeColor::Mute, mainX + 300, listStartY + 4);
        drawText(target, fontSemiBold, "Qty", 9.f, ThemeColor::Mute, mainX + 360, listStartY + 4);
        drawText(target, fontSemiBold, "Cost", 9.f, ThemeColor::Mute, mainX + 415, listStartY + 4);
        drawText(target, fontSemiBold, "Total", 9.f, ThemeColor::Mute, mainX + 475, listStartY + 4);

        sf::RectangleShape hSep({Layout::MAIN_W - 30, 1.5f});
        hSep.setPosition(mainX + 15, listStartY + 16);
        hSep.setFillColor(ThemeColor::Line);
        target.draw(hSep);

        std::vector<int> filteredPurchases = inventoryMgr.getFilteredPurchaseIndices(searchQuery);

        for (size_t i = 0; i < filteredPurchases.size(); i++) {
            int idx = filteredPurchases[i];
            float rowY = listStartY + 26.f + static_cast<float>(i) * 38.f - scrollOffset;
            if (rowY < listStartY + 20.f || rowY + 28.f > ty + 482.f) continue;

            const auto& p = purchases[idx];
            drawText(target, fontBold, p.supplier, 9.5f, ThemeColor::Ink, mainX + 15, rowY + 4);
            drawText(target, fontRegular, p.date, 8.f, ThemeColor::Mute, mainX + 15, rowY + 18);

            drawText(target, fontBold, p.barcode, 9.5f, ThemeColor::Ink, mainX + 150, rowY + 4);
            drawText(target, fontRegular, p.description.substr(0, 18), 8.f, ThemeColor::Mute, mainX + 150, rowY + 18);

            // [ Desc ] button for Ledger
            drawRounded(target, mainX + 288, rowY + 3, 50, 20, 4, sf::Color(240, 245, 250), ThemeColor::Blue, 1.f);
            drawText(target, fontSemiBold, "Desc", 8.f, ThemeColor::Blue, mainX + 313, rowY + 13, TextAlignment::Center);

            drawText(target, fontSemiBold, std::to_string(p.quantity) + " pcs", 9.5f, ThemeColor::Ink, mainX + 360, rowY + 10);
            drawText(target, fontRegular, "Rs " + std::to_string(p.unitCost), 9.5f, ThemeColor::Ink, mainX + 415, rowY + 10);
            drawText(target, fontBold, "Rs " + std::to_string(p.totalCost), 9.5f, ThemeColor::Blue, mainX + 475, rowY + 10);

            sf::RectangleShape rSep({Layout::MAIN_W - 30, 1});
            rSep.setPosition(mainX + 15, rowY + 30);
            rSep.setFillColor(ThemeColor::Line);
            target.draw(rSep);
        }
    }
}

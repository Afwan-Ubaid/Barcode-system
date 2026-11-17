#include "PosProductsView.h"
#include "UIUtils.h"
#include <cmath>
#include <ctime>
#include <algorithm>

void PosProductsView::drawViewport(sf::RenderTarget& target, float mainX,
                                   const sf::Texture& cameraTexture,
                                   float flashTimer, float lastScanTimer, const std::string& lastScannedInfo,
                                   const sf::Font& fontSemiBold,
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

    if (cameraTexture.getSize().x > 0) {
        sf::Sprite camSprite(cameraTexture);
        float srcW = (float)cameraTexture.getSize().x;
        float srcH = (float)cameraTexture.getSize().y;
        float targetAspect = paneW / paneH;
        float cropH = srcW / targetAspect;
        if (cropH <= srcH) {
            camSprite.setTextureRect(sf::IntRect(0, (int)((srcH - cropH) / 2.f), (int)srcW, (int)cropH));
        } else {
            float cropW = srcH * targetAspect;
            camSprite.setTextureRect(sf::IntRect((int)((srcW - cropW) / 2.f), 0, (int)cropW, (int)srcH));
        }
        camSprite.setPosition(paneX, paneY);
        camSprite.setScale(paneW / camSprite.getTextureRect().width, paneH / camSprite.getTextureRect().height);
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
    drawRounded(target, r.left + 14.f, r.top + 12.f, 142.f, 20.f, 5.f, sf::Color(15, 15, 18, 210), sf::Color(255, 255, 255, 45), 1.f);
    sf::CircleShape liveDot(3.5f, 16);
    liveDot.setOrigin(3.5f, 3.5f);
    liveDot.setPosition(r.left + 24.f, r.top + 22.f);
    liveDot.setFillColor(ThemeColor::Green);
    target.draw(liveDot);
    drawText(target, fontSemiBold, "CONTINUOUS SCANNER", 7.8f, sf::Color(245, 245, 245), r.left + 33.f, r.top + 22.f);

    // Outer border
    drawRounded(target, r.left, r.top, r.width, r.height, 12, ThemeColor::None, sf::Color(140, 130, 120, 140), 1.2f);

    // Animated Moving Red Laser Scan Line sweeping up and down across the ENTIRE camera pane
    float pulse = 0.75f + 0.25f * std::sin((float)clock() / CLOCKS_PER_SEC * 6.f);
    float sweep = 0.5f + 0.5f * std::sin((float)clock() / CLOCKS_PER_SEC * 2.2f); // oscillates smoothly between 0.0 and 1.0
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
                                  const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                  const sf::Font& fontBold, const sf::Font& fontSerif,
                                  std::function<const sf::Texture&(const std::string&)> getTexture) {
    drawText(target, fontSerif, "Products", 18, ThemeColor::Ink, mainX, 328);
    drawText(target, fontRegular, "(product.txt catalog)", 9, ThemeColor::Mute, mainX + 80, 332);

    // Category filter pills
    float pillRight = mainX + Layout::MAIN_W;
    for (int i = 4; i >= 0; i--) {
        float w = getTextWidth(fontSemiBold, Layout::CATEGORIES[i], 10) + 20.f;
        pillRight -= w;
        bool selected = (activeCategory == Layout::CATEGORIES[i]);
        bool hovered = pointInRect(m, pillRight, 318.f, w, 22.f);

        if (selected) {
            drawRounded(target, pillRight, 318.f, w, 22.f, 11, ThemeColor::Dark);
        } else if (hovered) {
            drawRounded(target, pillRight, 318.f, w, 22.f, 11, sf::Color(232, 224, 207));
        }
        drawText(target, fontSemiBold, Layout::CATEGORIES[i], 10, selected ? ThemeColor::White : ThemeColor::Mute,
                 pillRight + w / 2.f, 329.f, TextAlignment::Center, 0.f);

        pillRight -= 4.f;
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

        drawRounded(target, cx + 8, cy + lift + 8, Layout::CARD_W - 16, 68, 6, p.cardTint);
        drawSprite(target, getTexture("products/" + p.iconKey + ".png"), cx + Layout::CARD_W / 2.f - 26, cy + lift + 16, 52, 52);

        drawText(target, fontSemiBold, p.name, 12, ThemeColor::Ink, cx + 10, cy + lift + 90);
        drawText(target, fontRegular, p.unit, 9, ThemeColor::Mute, cx + 10, cy + lift + 106);
        drawText(target, fontBold, formatRs(p.price), 11, ThemeColor::Ink, cx + Layout::CARD_W - 10, cy + lift + 99, TextAlignment::Right);
    }
}

void PosProductsView::drawProductsTab(sf::RenderTarget& target, sf::Vector2f /*m*/, float mainX,
                                     const std::vector<ProductItem>& products,
                                     float scrollOffset, const std::string& currentRole,
                                     const std::string& productFilePath,
                                     const sf::Font& fontRegular, const sf::Font& fontMedium,
                                     const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                     const sf::Font& fontSerif) {
    drawText(target, fontSerif, "Product Catalog Database", 24, ThemeColor::Ink, mainX, 50);
    drawText(target, fontRegular, "Live product database linked with barcode scanner (" + productFilePath + ")", 10.5f, ThemeColor::Mute, mainX, 74);

    if (currentRole == "Admin") {
        drawRounded(target, mainX + Layout::MAIN_W - 130, 50, 130, 34, 6, ThemeColor::Blue);
        drawText(target, fontSemiBold, "+ Add Product", 11, ThemeColor::White, mainX + Layout::MAIN_W - 65, 67, TextAlignment::Center);
    } else {
        drawRounded(target, mainX + Layout::MAIN_W - 150, 50, 150, 34, 6, sf::Color(230, 222, 210), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, "[Admin Only] + Add", 9.5f, ThemeColor::Mute, mainX + Layout::MAIN_W - 75, 67, TextAlignment::Center);
    }

    float ty = 100.f;
    drawRounded(target, mainX, ty, Layout::MAIN_W, 560, 10, ThemeColor::Paper, ThemeColor::Line, 1);

    // Table Header
    drawText(target, fontSemiBold, "Barcode", 10, ThemeColor::Mute, mainX + 20, ty + 18);
    drawText(target, fontSemiBold, "Item Name", 10, ThemeColor::Mute, mainX + 110, ty + 18);
    drawText(target, fontSemiBold, "Category", 10, ThemeColor::Mute, mainX + 240, ty + 18);
    drawText(target, fontSemiBold, "Price", 10, ThemeColor::Mute, mainX + 340, ty + 18);
    drawText(target, fontSemiBold, "Stock", 10, ThemeColor::Mute, mainX + 410, ty + 18);
    drawText(target, fontSemiBold, "Actions", 10, ThemeColor::Mute, mainX + 475, ty + 18);

    sf::RectangleShape hdrLine({Layout::MAIN_W - 40, 1});
    hdrLine.setPosition(mainX + 20, ty + 30);
    hdrLine.setFillColor(ThemeColor::Line);
    target.draw(hdrLine);

    for (size_t i = 0; i < products.size(); i++) {
        float rowY = ty + 46.f + static_cast<float>(i) * 46.f - scrollOffset;
        if (rowY < ty + 30.f || rowY > ty + 530.f) continue;

        const auto& p = products[i];
        drawText(target, fontMedium, p.barcode, 10.5f, ThemeColor::Ink, mainX + 20, rowY);
        drawText(target, fontBold, p.name, 11.f, ThemeColor::Ink, mainX + 110, rowY);
        drawText(target, fontRegular, p.category, 10.f, ThemeColor::Mute, mainX + 240, rowY);
        drawText(target, fontBold, formatRs(p.price), 11.f, ThemeColor::Ink, mainX + 340, rowY);

        bool low = p.stock < 15;
        drawText(target, fontSemiBold, std::to_string(p.stock), 10.5f,
                 p.stock == 0 ? ThemeColor::Red : (low ? ThemeColor::Amber : ThemeColor::Green),
                 mainX + 410, rowY);

        if (currentRole == "Admin") {
            drawRounded(target, mainX + 470, rowY - 8, 36, 22, 4, sf::Color(254, 226, 226), sf::Color(220, 80, 70), 1.f);
            drawText(target, fontSemiBold, "Del", 9.f, ThemeColor::Red, mainX + 488, rowY + 3, TextAlignment::Center);
        } else {
            drawRounded(target, mainX + 460, rowY - 8, 52, 22, 4, sf::Color(235, 230, 220));
            drawText(target, fontRegular, "View Only", 8.5f, ThemeColor::Mute, mainX + 486, rowY + 3, TextAlignment::Center);
        }

        sf::RectangleShape sep({Layout::MAIN_W - 40, 1});
        sep.setPosition(mainX + 20, rowY + 18);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);
    }
}

void PosProductsView::drawInventoryTab(sf::RenderTarget& target, sf::Vector2f /*m*/, float mainX,
                                      const std::vector<ProductItem>& products,
                                      float scrollOffset, const std::string& currentRole,
                                      const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                      const sf::Font& fontBold, const sf::Font& fontSerif) {
    drawText(target, fontSerif, "Inventory & Stock Audit", 24, ThemeColor::Ink, mainX, 50);
    drawText(target, fontRegular, "Real-time stock level audit linked with continuous barcode scanner", 10.5f, ThemeColor::Mute, mainX, 74);

    int healthy = 0, lowStock = 0, outOfStock = 0;
    for (const auto& p : products) {
        if (p.stock == 0) outOfStock++;
        else if (p.stock < 15) lowStock++;
        else healthy++;
    }

    auto drawKpi = [&](const std::string& label, int count, sf::Color col, float x) {
        drawRounded(target, x, 96, 125, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontRegular, label, 9.5f, ThemeColor::Mute, x + 12, 110);
        drawText(target, fontBold, std::to_string(count), 16, col, x + 12, 130);
    };

    drawKpi("Total Items", (int)products.size(), ThemeColor::Ink, mainX);
    drawKpi("Healthy Stock", healthy, ThemeColor::Green, mainX + 138);
    drawKpi("Low Stock (<15)", lowStock, ThemeColor::Amber, mainX + 276);
    drawKpi("Out of Stock", outOfStock, ThemeColor::Red, mainX + 414);

    float ty = 155.f;
    drawRounded(target, mainX, ty, Layout::MAIN_W, 510, 10, ThemeColor::Paper, ThemeColor::Line, 1);

    if (currentRole != "Admin") {
        drawRounded(target, mainX + 15, ty + 12, Layout::MAIN_W - 30, 28, 6, sf::Color(254, 243, 199), sf::Color(217, 119, 6), 1.f);
        drawText(target, fontSemiBold, "[Employee View] Stock levels are read-only. Adjusting inventory requires Admin credentials.", 9.5f, sf::Color(146, 64, 14), mainX + 25, ty + 26);
    }

    float listStartY = (currentRole != "Admin") ? (ty + 48.f) : (ty + 20.f);

    for (size_t i = 0; i < products.size(); i++) {
        float rowY = listStartY + 15.f + static_cast<float>(i) * 54.f - scrollOffset;
        if (rowY < ty + 20.f || rowY > ty + 480.f) continue;

        const auto& p = products[i];
        drawText(target, fontBold, p.name, 12.f, ThemeColor::Ink, mainX + 20, rowY);
        drawText(target, fontRegular, "SKU: " + p.barcode + " | " + p.unit + " | " + p.category, 9.5f, ThemeColor::Mute, mainX + 20, rowY + 16);

        bool low = p.stock < 15;
        drawRounded(target, mainX + 270, rowY - 4, 95, 24, 5,
                    p.stock == 0 ? sf::Color(254, 226, 226) : (low ? sf::Color(254, 243, 199) : sf::Color(220, 252, 231)));
        drawText(target, fontSemiBold, std::to_string(p.stock) + (p.stock == 0 ? " Empty" : (low ? " Low" : " In Stock")), 9.f,
                 p.stock == 0 ? ThemeColor::Red : (low ? ThemeColor::Amber : sf::Color(22, 101, 52)),
                 mainX + 317, rowY + 8, TextAlignment::Center);

        if (currentRole == "Admin") {
            drawRounded(target, mainX + 375, rowY - 4, 42, 24, 4, sf::Color(230, 238, 246), ThemeColor::Blue, 1);
            drawText(target, fontSemiBold, "+10", 9.5f, ThemeColor::Ink, mainX + 396, rowY + 8, TextAlignment::Center);

            drawRounded(target, mainX + 425, rowY - 4, 42, 24, 4, sf::Color(230, 238, 246), ThemeColor::Blue, 1);
            drawText(target, fontSemiBold, "+50", 9.5f, ThemeColor::Ink, mainX + 446, rowY + 8, TextAlignment::Center);

            drawRounded(target, mainX + 475, rowY - 4, 55, 24, 4, ThemeColor::Blue);
            drawText(target, fontSemiBold, "Custom", 9.f, ThemeColor::White, mainX + 502, rowY + 8, TextAlignment::Center);
        } else {
            drawRounded(target, mainX + 385, rowY - 4, 140, 24, 4, sf::Color(240, 235, 225));
            drawText(target, fontRegular, "[Restock Locked]", 8.5f, ThemeColor::Mute, mainX + 455, rowY + 8, TextAlignment::Center);
        }

        sf::RectangleShape sep({Layout::MAIN_W - 40, 1});
        sep.setPosition(mainX + 20, rowY + 28);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);
    }
}

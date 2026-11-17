#include "PosModalsView.h"
#include "UIUtils.h"
#include <sstream>
#include <ctime>
#include <cmath>

void PosModalsView::drawReceiptModal(sf::RenderTarget& target, sf::Vector2f m,
                                     float currentWidth, float designHeight,
                                     const std::string& receiptText,
                                     const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                     const sf::Font& fontSerif,
                                     std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(dim);

    float dw = 620.f, dh = 580.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 18, 10, 18, 22, 6);
    drawRounded(target, dx, dy, dw, dh, 16, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    drawSprite(target, getTexture("icons/check.png"), dx + dw / 2.f - 24, dy + 20, 48, 48, ThemeColor::Green);
    drawText(target, fontSerif, "Receipt Printed!", 24, ThemeColor::Ink, dx + dw / 2.f, dy + 82, TextAlignment::Center);
    drawText(target, fontRegular, "Receipt transaction saved to receipt.txt", 11, ThemeColor::Mute, dx + dw / 2.f, dy + 104, TextAlignment::Center);

    float boxW = dw - 60.f;
    float boxH = 370.f;
    drawRounded(target, dx + 30, dy + 124, boxW, boxH, 8, sf::Color(255, 255, 255), ThemeColor::Line, 1);

    std::stringstream ss(receiptText);
    std::string line;
    float ly = dy + 146;
    int lineCount = 0;
    while (std::getline(ss, line) && lineCount < 18) {
        drawText(target, fontRegular, line, 11.5f, ThemeColor::Ink, dx + 50, ly);
        ly += 19.5f;
        lineCount++;
    }

    bool hovClose = pointInRect(m, dx + 40, dy + dh - 58, 240, 42);
    drawRounded(target, dx + 40, dy + dh - 58, 240, 42, 8,
                hovClose ? sf::Color(230, 220, 206) : sf::Color(238, 230, 216), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Close", 12.5f, ThemeColor::Ink, dx + 160, dy + dh - 37, TextAlignment::Center);

    bool hovNew = pointInRect(m, dx + dw - 280, dy + dh - 58, 240, 42);
    drawRounded(target, dx + dw - 280, dy + dh - 58, 240, 42, 8,
                hovNew ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "Start New Sale", 12.5f, ThemeColor::White, dx + dw - 160, dy + dh - 37, TextAlignment::Center);
}

void PosModalsView::drawEnlargeCartModal(sf::RenderTarget& target, sf::Vector2f m,
                                        float currentWidth, float designHeight,
                                        const std::vector<CartLine>& cart,
                                        const std::vector<ProductItem>& products,
                                        int subtotal, int total, float scrollOffset,
                                        const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                        const sf::Font& fontBold, const sf::Font& fontSerif,
                                        std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(dim);

    float dw = 840.f, dh = 580.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 18, 10, 18, 22, 6);
    drawRounded(target, dx, dy, dw, dh, 16, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    drawText(target, fontSerif, "Full Cart Inspection", 24, ThemeColor::Ink, dx + 30, dy + 42);
    drawText(target, fontRegular, "Cashier Operator - Total " + std::to_string(cart.size()) + " unique items in current order",
             11, ThemeColor::Mute, dx + 30, dy + 68);

    bool hovClose = pointInRect(m, dx + dw - 44, dy + 20, 28, 28);
    drawRounded(target, dx + dw - 44, dy + 20, 28, 28, 6,
                hovClose ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 13, ThemeColor::Ink, dx + dw - 30, dy + 34, TextAlignment::Center);

    float tableY = dy + 88.f;
    drawRounded(target, dx + 25, tableY, dw - 50, 32, 6, sf::Color(238, 230, 214));
    drawText(target, fontSemiBold, "PRODUCT ITEM", 10.5f, ThemeColor::Mute, dx + 45, tableY + 16);
    drawText(target, fontSemiBold, "CATEGORY", 10.5f, ThemeColor::Mute, dx + 280, tableY + 16);
    drawText(target, fontSemiBold, "UNIT PRICE", 10.5f, ThemeColor::Mute, dx + 400, tableY + 16);
    drawText(target, fontSemiBold, "QUANTITY", 10.5f, ThemeColor::Mute, dx + 520, tableY + 16);
    drawText(target, fontSemiBold, "LINE TOTAL", 10.5f, ThemeColor::Mute, dx + 650, tableY + 16);
    drawText(target, fontSemiBold, "ACTION", 10.5f, ThemeColor::Mute, dx + dw - 65, tableY + 16, TextAlignment::Center);

    float rowStartY = tableY + 36.f;
    for (int i = 0; i < (int)cart.size(); i++) {
        const auto& item = cart[i];
        if (item.productId < 0 || item.productId >= (int)products.size()) continue;
        const auto& p = products[item.productId];
        float rowY = rowStartY + i * 56.f - scrollOffset;

        if (rowY < rowStartY - 10.f || rowY > rowStartY + 330.f) continue;

        drawRounded(target, dx + 36, rowY + 6, 40, 40, 6, p.cardTint);
        drawSprite(target, getTexture("products/" + p.iconKey + ".png"), dx + 42, rowY + 12, 28, 28);

        drawText(target, fontSemiBold, p.name, 12.5f, ThemeColor::Ink, dx + 86, rowY + 18);
        drawText(target, fontRegular, "SKU: " + p.barcode, 9.5f, ThemeColor::Mute, dx + 86, rowY + 34);

        drawText(target, fontRegular, p.category, 11, ThemeColor::Ink, dx + 280, rowY + 26);
        drawText(target, fontRegular, formatRs(p.price) + " / " + p.unit, 11, ThemeColor::Ink, dx + 400, rowY + 26);

        bool hovMinus = pointInRect(m, dx + 510, rowY + 14, 24, 24);
        drawRounded(target, dx + 510, rowY + 14, 24, 24, 4,
                    hovMinus ? sf::Color(230, 220, 206) : ThemeColor::Paper, ThemeColor::Edge, 1);
        drawText(target, fontBold, "-", 12, ThemeColor::Ink, dx + 522, rowY + 26, TextAlignment::Center);

        drawRounded(target, dx + 538, rowY + 14, 38, 24, 4, ThemeColor::Paper, ThemeColor::Edge, 1);
        drawText(target, fontBold, std::to_string(item.quantity), 11.5f, ThemeColor::Ink, dx + 557, rowY + 26, TextAlignment::Center);

        bool hovPlus = pointInRect(m, dx + 580, rowY + 14, 24, 24);
        drawRounded(target, dx + 580, rowY + 14, 24, 24, 4,
                    hovPlus ? sf::Color(230, 220, 206) : ThemeColor::Paper, ThemeColor::Edge, 1);
        drawText(target, fontBold, "+", 12, ThemeColor::Ink, dx + 592, rowY + 26, TextAlignment::Center);

        drawText(target, fontBold, formatRs(p.price * item.quantity), 12.5f, ThemeColor::Ink, dx + 650, rowY + 26);

        bool hovDel = pointInRect(m, dx + dw - 95, rowY + 14, 60, 24);
        drawRounded(target, dx + dw - 95, rowY + 14, 60, 24, 4,
                    hovDel ? sf::Color(254, 226, 226) : sf::Color(246, 240, 234),
                    hovDel ? ThemeColor::Red : ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, "Delete", 9.5f,
                 hovDel ? ThemeColor::Red : ThemeColor::Mute, dx + dw - 65, rowY + 26, TextAlignment::Center);

        sf::RectangleShape sep({dw - 60.f, 1});
        sep.setPosition(dx + 30, rowY + 54);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);
    }

    float botY = dy + dh - 60.f;
    drawText(target, fontSemiBold, "Subtotal: " + formatRs(subtotal), 12, ThemeColor::Ink, dx + 30, botY - 14);
    drawText(target, fontBold, "Grand Total: " + formatRs(total), 15, ThemeColor::Ink, dx + 30, botY + 10);

    bool hovBack = pointInRect(m, dx + dw - 440, botY - 8, 180, 40);
    drawRounded(target, dx + dw - 440, botY - 8, 180, 40, 7,
                hovBack ? sf::Color(230, 220, 206) : sf::Color(238, 230, 216), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Back to Register", 11.5f, ThemeColor::Ink, dx + dw - 350, botY + 12, TextAlignment::Center);

    bool hovPay = pointInRect(m, dx + dw - 240, botY - 8, 210, 40);
    drawRounded(target, dx + dw - 240, botY - 8, 210, 40, 7,
                hovPay ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "Proceed to Pay (" + formatRs(total) + ")", 12.f, ThemeColor::White, dx + dw - 135, botY + 12, TextAlignment::Center);
}

void PosModalsView::drawCashModal(sf::RenderTarget& target, sf::Vector2f m,
                                 float currentWidth, float designHeight,
                                 int total, int cashGiven, const std::string& cashGivenStr,
                                 bool inputFocused,
                                 const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                 const sf::Font& fontBold, const sf::Font& fontSerif,
                                 std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(dim);

    float dw = 480.f, dh = 400.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 18, 10, 18, 22, 6);
    drawRounded(target, dx, dy, dw, dh, 16, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    drawSprite(target, getTexture("icons/cash.png"), dx + 30, dy + 28, 26, 26, ThemeColor::Ink);
    drawText(target, fontSerif, "Cash Payment & Change", 22, ThemeColor::Ink, dx + 65, dy + 38);
    drawText(target, fontRegular, "Enter amount received from customer", 10.5f, ThemeColor::Mute, dx + 65, dy + 62);

    drawRounded(target, dx + 30, dy + 82, dw - 60, 48, 8, sf::Color(238, 230, 214));
    drawText(target, fontSemiBold, "Total Amount Due:", 12, ThemeColor::Ink, dx + 45, dy + 106);
    drawText(target, fontBold, formatRs(total), 16, ThemeColor::Ink, dx + dw - 45, dy + 106, TextAlignment::Right);

    drawText(target, fontSemiBold, "Cash Tendered:", 11, ThemeColor::Ink, dx + 30, dy + 160);
    drawRounded(target, dx + 150, dy + 144, 180, 34, 6,
                inputFocused ? sf::Color(255, 255, 255) : sf::Color(246, 241, 233),
                inputFocused ? ThemeColor::Ink : ThemeColor::Edge, 1.5f);

    std::string dispCash = cashGivenStr.empty() ? (cashGiven > 0 ? std::to_string(cashGiven) : "0") : cashGivenStr;
    bool showCaret = inputFocused && (std::fmod((float)clock() / CLOCKS_PER_SEC, 1.f) < 0.5f);
    drawText(target, fontBold, "Rs " + dispCash + (showCaret ? "|" : ""), 13, ThemeColor::Ink, dx + 165, dy + 161);

    bool hovExact = pointInRect(m, dx + 340, dy + 144, 56, 34);
    drawRounded(target, dx + 340, dy + 144, 56, 34, 6,
                hovExact ? sf::Color(228, 219, 203) : sf::Color(238, 230, 216), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Exact", 10.5f, ThemeColor::Ink, dx + 368, dy + 161, TextAlignment::Center);

    bool hovClr = pointInRect(m, dx + 404, dy + 144, 48, 34);
    drawRounded(target, dx + 404, dy + 144, 48, 34, 6,
                hovClr ? sf::Color(228, 219, 203) : sf::Color(238, 230, 216), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Clr", 10.5f, ThemeColor::Ink, dx + 428, dy + 161, TextAlignment::Center);

    const int chipVals[] = {100, 500, 1000, 5000};
    const char* chipLbls[] = {"+100", "+500", "+1,000", "+5,000"};
    for (int i = 0; i < 4; i++) {
        float cx = dx + 30 + i * 105;
        float cy = dy + 195;
        bool hov = pointInRect(m, cx, cy, 95, 32);
        drawRounded(target, cx, cy, 95, 32, 5,
                    hov ? sf::Color(230, 220, 204) : sf::Color(242, 234, 220), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, chipLbls[i], 10, ThemeColor::Ink, cx + 47.5f, cy + 16, TextAlignment::Center);
    }

    int change = cashGiven - total;
    float resY = dy + 245.f;

    if (cashGiven >= total) {
        drawRounded(target, dx + 30, resY, dw - 60, 52, 8, sf::Color(220, 252, 231), sf::Color(110, 168, 110), 1.2f);
        drawText(target, fontSemiBold, "Give Back Change:", 13, sf::Color(22, 101, 52), dx + 45, resY + 26);
        drawText(target, fontBold, formatRs(change), 16, sf::Color(22, 101, 52), dx + dw - 45, resY + 26, TextAlignment::Right);
    } else {
        drawRounded(target, dx + 30, resY, dw - 60, 52, 8, sf::Color(254, 226, 226), ThemeColor::Red, 1.2f);
        drawText(target, fontSemiBold, "Remaining Cash Due:", 13, ThemeColor::Red, dx + 45, resY + 26);
        drawText(target, fontBold, "Need " + formatRs(total - cashGiven), 15, ThemeColor::Red, dx + dw - 45, resY + 26, TextAlignment::Right);
    }

    bool canConfirm = (cashGiven >= total && total > 0);
    bool hovConfirm = canConfirm && pointInRect(m, dx + 30, dy + dh - 56, 250, 42);
    drawRounded(target, dx + 30, dy + dh - 56, 250, 42, 8,
                !canConfirm ? ThemeColor::BlueDisabled : (hovConfirm ? sf::Color(35, 75, 50) : sf::Color(44, 95, 63)));
    drawText(target, fontSemiBold, "Confirm Cash Payment", 12.f, ThemeColor::White, dx + 155, dy + dh - 35, TextAlignment::Center);

    bool hovCancel = pointInRect(m, dx + dw - 150, dy + dh - 56, 120, 42);
    drawRounded(target, dx + dw - 150, dy + dh - 56, 120, 42, 8,
                hovCancel ? sf::Color(230, 220, 206) : sf::Color(238, 230, 216), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Cancel", 11.5f, ThemeColor::Ink, dx + dw - 90, dy + dh - 35, TextAlignment::Center);
}

void PosModalsView::drawCardModal(sf::RenderTarget& target, sf::Vector2f m,
                                 float currentWidth, float designHeight, int total,
                                 const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                 const sf::Font& fontBold, const sf::Font& fontSerif,
                                 std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(dim);

    float dw = 460.f, dh = 360.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 18, 10, 18, 22, 6);
    drawRounded(target, dx, dy, dw, dh, 16, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    drawSprite(target, getTexture("icons/card.png"), dx + 30, dy + 28, 28, 28, ThemeColor::Ink);
    drawText(target, fontSerif, "Card POS Terminal Ready", 22, ThemeColor::Ink, dx + 68, dy + 38);
    drawText(target, fontRegular, "Tap, Insert Chip, or Swipe Customer Card", 10.5f, ThemeColor::Mute, dx + 68, dy + 62);

    drawRounded(target, dx + 30, dy + 90, dw - 60, 68, 8, sf::Color(238, 230, 214));
    drawText(target, fontSemiBold, "Amount to Charge:", 12, ThemeColor::Ink, dx + 45, dy + 114);
    drawText(target, fontBold, formatRs(total), 18, ThemeColor::Ink, dx + dw - 45, dy + 114, TextAlignment::Right);
    drawText(target, fontRegular, "Terminal: POS-9842 | Contactless NFC / EMV Chip Enabled", 9.5f, ThemeColor::Mute, dx + 45, dy + 138);

    drawRounded(target, dx + 30, dy + 172, dw - 60, 56, 8, sf::Color(246, 241, 233), ThemeColor::Edge, 1);
    drawSprite(target, getTexture("icons/scan.png"), dx + 45, dy + 188, 24, 24, ThemeColor::Green);
    drawText(target, fontSemiBold, "Waiting for customer card action...", 11.5f, ThemeColor::Ink, dx + 78, dy + 194);
    drawText(target, fontRegular, "Customer may tap device or insert chip now", 9.5f, ThemeColor::Mute, dx + 78, dy + 210);

    bool canConfirm = (total > 0);
    bool hovConfirm = canConfirm && pointInRect(m, dx + 30, dy + dh - 56, 250, 42);
    drawRounded(target, dx + 30, dy + dh - 56, 250, 42, 8,
                !canConfirm ? ThemeColor::BlueDisabled : (hovConfirm ? sf::Color(35, 75, 50) : sf::Color(44, 95, 63)));
    drawText(target, fontSemiBold, "Complete Card Payment", 12.f, ThemeColor::White, dx + 155, dy + dh - 35, TextAlignment::Center);

    bool hovCancel = pointInRect(m, dx + dw - 150, dy + dh - 56, 120, 42);
    drawRounded(target, dx + dw - 150, dy + dh - 56, 120, 42, 8,
                hovCancel ? sf::Color(230, 220, 206) : sf::Color(238, 230, 216), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Cancel", 11.5f, ThemeColor::Ink, dx + dw - 90, dy + dh - 35, TextAlignment::Center);
}

void PosModalsView::drawQtyModal(sf::RenderTarget& target, sf::Vector2f m,
                                float currentWidth, float designHeight,
                                const ProductItem& product, const std::string& qtyInputStr,
                                const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                const sf::Font& fontSerif) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 110));
    target.draw(dim);

    float dw = 320.f, dh = 240.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 14, 8, 14, 18, 5);
    drawRounded(target, dx, dy, dw, dh, 14, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    drawText(target, fontSerif, "Change Quantity", 18, ThemeColor::Ink, dx + dw / 2.f, dy + 32, TextAlignment::Center);
    drawText(target, fontSemiBold, product.name + " (" + formatRs(product.price) + ")", 11, ThemeColor::Mute, dx + dw / 2.f, dy + 56, TextAlignment::Center);

    drawRounded(target, dx + 85, dy + 72, 150, 30, 6, sf::Color(255, 255, 255), ThemeColor::Ink, 1.5f);
    drawText(target, fontBold, qtyInputStr.empty() ? "1" : qtyInputStr, 14, ThemeColor::Ink, dx + dw / 2.f, dy + 87, TextAlignment::Center);

    const int presets[] = {1, 2, 3, 5, 10};
    for (int i = 0; i < 5; i++) {
        float bx = dx + 20 + i * 56;
        float by = dy + 116;
        bool hov = pointInRect(m, bx, by, 48, 28);
        drawRounded(target, bx, by, 48, 28, 5,
                    hov ? sf::Color(228, 219, 203) : sf::Color(240, 232, 218), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, std::to_string(presets[i]), 11, ThemeColor::Ink, bx + 24, by + 14, TextAlignment::Center);
    }

    bool hovSet = pointInRect(m, dx + 20, dy + dh - 48, 130, 34);
    drawRounded(target, dx + 20, dy + dh - 48, 130, 34, 6,
                hovSet ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "Set Quantity", 11, ThemeColor::White, dx + 85, dy + dh - 31, TextAlignment::Center);

    bool hovCancel = pointInRect(m, dx + 170, dy + dh - 48, 130, 34);
    drawRounded(target, dx + 170, dy + dh - 48, 130, 34, 6,
                hovCancel ? sf::Color(230, 220, 206) : sf::Color(238, 230, 216), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Cancel", 11, ThemeColor::Ink, dx + 235, dy + dh - 31, TextAlignment::Center);
}

void PosModalsView::drawAddStaffModal(sf::RenderTarget& target, sf::Vector2f /*m*/,
                                     float currentWidth, float designHeight,
                                     const std::string& newStaffName,
                                     const std::string& newStaffPass,
                                     const std::string& newStaffRole,
                                     int activeField,
                                     const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                     const sf::Font& fontBold, const sf::Font& fontSerif) {
    sf::RectangleShape bg({currentWidth, designHeight});
    bg.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(bg);

    float dw = 480.f, dh = 390.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 14, 8, 14, 18, 5);
    drawRounded(target, dx, dy, dw, dh, 14, ThemeColor::Paper, ThemeColor::Line, 1.5f);
    drawText(target, fontSerif, "Register New Staff Member", 20, ThemeColor::Ink, dx + 30, dy + 35);
    drawText(target, fontRegular, "Setup employee ID and credentials saved to staff.txt", 10.5f, ThemeColor::Mute, dx + 30, dy + 55);

    drawText(target, fontSemiBold, "Employee ID / Username:", 10.5f, ThemeColor::Ink, dx + 30, dy + 88);
    drawRounded(target, dx + 30, dy + 104, dw - 60, 36, 6, ThemeColor::White,
                (activeField == 1) ? ThemeColor::Blue : ThemeColor::Line, 1.2f);
    std::string nameDisp = newStaffName.empty() && activeField != 1 ? "e.g. cashier2" : newStaffName;
    drawText(target, fontRegular, nameDisp, 11, newStaffName.empty() ? ThemeColor::Mute : ThemeColor::Ink, dx + 42, dy + 122);

    drawText(target, fontSemiBold, "Security Password / PIN:", 10.5f, ThemeColor::Ink, dx + 30, dy + 155);
    drawRounded(target, dx + 30, dy + 171, dw - 60, 36, 6, ThemeColor::White,
                (activeField == 2) ? ThemeColor::Blue : ThemeColor::Line, 1.2f);
    std::string passDisp = newStaffPass.empty() && activeField != 2 ? "e.g. 1234" : std::string(newStaffPass.size(), '*');
    drawText(target, fontRegular, passDisp, 11, newStaffPass.empty() ? ThemeColor::Mute : ThemeColor::Ink, dx + 42, dy + 189);

    drawText(target, fontSemiBold, "Access Role:", 10.5f, ThemeColor::Ink, dx + 30, dy + 220);
    bool isEmp = (newStaffRole == "Employee");
    drawRounded(target, dx + 30, dy + 236, 120, 32, 6,
                isEmp ? ThemeColor::Dark : sf::Color(238, 230, 214),
                isEmp ? ThemeColor::Dark : ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Employee", 10, isEmp ? ThemeColor::White : ThemeColor::Ink, dx + 90, dy + 252, TextAlignment::Center);

    bool isAdm = (newStaffRole == "Admin");
    drawRounded(target, dx + 160, dy + 236, 120, 32, 6,
                isAdm ? ThemeColor::Dark : sf::Color(238, 230, 214),
                isAdm ? ThemeColor::Dark : ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Admin", 10, isAdm ? ThemeColor::White : ThemeColor::Ink, dx + 220, dy + 252, TextAlignment::Center);

    drawRounded(target, dx + 30, dy + dh - 56, 220, 42, 6, ThemeColor::Blue);
    drawText(target, fontBold, "Create Staff Account", 11.5f, ThemeColor::White, dx + 140, dy + dh - 35, TextAlignment::Center);

    drawRounded(target, dx + dw - 150, dy + dh - 56, 120, 42, 6, sf::Color(230, 220, 206), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Cancel", 11, ThemeColor::Ink, dx + dw - 90, dy + dh - 35, TextAlignment::Center);
}

void PosModalsView::drawResetPassModal(sf::RenderTarget& target, sf::Vector2f /*m*/,
                                      float currentWidth, float designHeight,
                                      const std::string& targetUser,
                                      const std::string& newPass,
                                      bool inputFocused,
                                      const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                      const sf::Font& fontBold, const sf::Font& fontSerif) {
    sf::RectangleShape bg({currentWidth, designHeight});
    bg.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(bg);

    float dw = 460.f, dh = 300.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 14, 8, 14, 18, 5);
    drawRounded(target, dx, dy, dw, dh, 14, ThemeColor::Paper, ThemeColor::Line, 1.5f);
    drawText(target, fontSerif, "Reset Password", 20, ThemeColor::Ink, dx + 30, dy + 35);
    drawText(target, fontRegular, "Reset security credentials for operator '" + targetUser + "'", 10.5f, ThemeColor::Mute, dx + 30, dy + 55);

    drawText(target, fontSemiBold, "New Password / PIN:", 10.5f, ThemeColor::Ink, dx + 30, dy + 95);
    drawRounded(target, dx + 30, dy + 115, dw - 60, 36, 6, ThemeColor::White,
                inputFocused ? ThemeColor::Blue : ThemeColor::Line, 1.2f);
    std::string passDisp = newPass.empty() ? "Enter new password" : std::string(newPass.size(), '*');
    drawText(target, fontRegular, passDisp, 11, newPass.empty() ? ThemeColor::Mute : ThemeColor::Ink, dx + 42, dy + 133);

    drawRounded(target, dx + 30, dy + dh - 56, 200, 42, 6, ThemeColor::Blue);
    drawText(target, fontBold, "Save New Password", 11.5f, ThemeColor::White, dx + 130, dy + dh - 35, TextAlignment::Center);

    drawRounded(target, dx + dw - 150, dy + dh - 56, 120, 42, 6, sf::Color(230, 220, 206), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Cancel", 11, ThemeColor::Ink, dx + dw - 90, dy + dh - 35, TextAlignment::Center);
}

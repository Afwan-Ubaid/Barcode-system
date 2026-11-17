#include "PosModalsView.h"
#include "UIUtils.h"
#include <sstream>
#include <ctime>
#include <cmath>

void PosModalsView::drawReceiptModal(sf::RenderTarget& target, sf::Vector2f m,
                                     float currentWidth, float designHeight,
                                     const CartService& cartService,
                                     const std::vector<ProductItem>& products,
                                     const std::string& cashierName,
                                     const sf::Font& fontRegular, const sf::Font& fontMedium,
                                     const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                     const sf::Font& fontSerif,
                                     std::function<const sf::Texture&(const std::string&)> getTexture) {
    // 1. Darkened Backdrop
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 165));
    target.draw(dim);

    // 2. Paper Geometry
    float rw = 440.f;
    float rh = 600.f;
    float rx = (currentWidth - rw) / 2.f;
    float ry = (designHeight - (rh + 52.f)) / 2.f;

    // 3. Deep Soft Paper Shadow
    drawShadow(target, rx, ry, rw, rh, 26, 10, 24, 90, 5);

    // 4. Pure Crisp White Thermal Paper
    sf::RectangleShape paper({rw, rh});
    paper.setPosition(rx, ry);
    paper.setFillColor(sf::Color(255, 255, 255)); // Pure White Paper
    paper.setOutlineThickness(1.f);
    paper.setOutlineColor(sf::Color(226, 222, 216));
    target.draw(paper);

    // 5. Perforated Serrated Paper Teeth at Bottom Edge
    float toothW = 8.f;
    int teethCount = (int)(rw / toothW);
    for (int i = 0; i < teethCount; ++i) {
        sf::ConvexShape tooth(3);
        tooth.setPoint(0, sf::Vector2f(rx + i * toothW, ry + rh));
        tooth.setPoint(1, sf::Vector2f(rx + (i + 0.5f) * toothW, ry + rh + 4.f));
        tooth.setPoint(2, sf::Vector2f(rx + (i + 1) * toothW, ry + rh));
        tooth.setFillColor(sf::Color(255, 255, 255));
        target.draw(tooth);
    }

    // 6. Thermal Paper Header & Branding
    drawRounded(target, rx + rw / 2.f - 75.f, ry + 12.f, 150.f, 20.f, 10.f,
                sf::Color(238, 250, 238), sf::Color(165, 218, 165), 1.f);
    drawText(target, fontSemiBold, "OFFICIAL SALES RECEIPT", 8.5f, sf::Color(40, 140, 60), rx + rw / 2.f, ry + 22.f, TextAlignment::Center);

    drawText(target, fontSerif, "SCANIT SUPERMARKET", 17.f, sf::Color(32, 28, 26), rx + rw / 2.f, ry + 46.f, TextAlignment::Center);
    drawText(target, fontMedium, "Smart Point of Sale & Retail Express", 9.5f, sf::Color(105, 95, 88), rx + rw / 2.f, ry + 64.f, TextAlignment::Center);
    drawText(target, fontRegular, "Main Boulevard, Gulberg III, Lahore, Pakistan", 8.5f, sf::Color(120, 112, 105), rx + rw / 2.f, ry + 79.f, TextAlignment::Center);
    drawText(target, fontRegular, "UAN: +92 (42) 3578-9000  *  NTN: 8492019-3", 8.5f, sf::Color(120, 112, 105), rx + rw / 2.f, ry + 93.f, TextAlignment::Center);
    drawText(target, fontRegular, "Terminal: REG-04  *  Store ID: PK-LHR-01", 8.5f, sf::Color(120, 112, 105), rx + rw / 2.f, ry + 107.f, TextAlignment::Center);

    // Dashed divider line
    for (float cx = rx + 20.f; cx < rx + rw - 20.f; cx += 8.f) {
        sf::RectangleShape dash({5.f, 1.f});
        dash.setPosition(cx, ry + 120.f);
        dash.setFillColor(sf::Color(195, 190, 185));
        target.draw(dash);
    }

    // 7. Live Dynamic Metadata
    std::string dateStr = cartService.getLastDateStr();
    if (dateStr.empty()) {
        dateStr = getCurrentDateYYYYMMDD() + " " + getCurrentTimeHHMMSS();
    }
    std::string orderId = cartService.getLastOrderId();
    if (orderId.empty()) {
        orderId = "ORD-" + std::to_string(1001 + cartService.getSalesHistory().size());
    }
    std::string effectiveCashier = cashierName.empty() ? cartService.getLastCashier() : cashierName;
    if (effectiveCashier.empty()) effectiveCashier = "System Admin";

    drawText(target, fontMedium, "DATE:    " + dateStr, 9.f, sf::Color(45, 40, 35), rx + 22.f, ry + 132.f);
    drawText(target, fontSemiBold, "ORDER: #" + orderId, 9.5f, sf::Color(35, 30, 25), rx + rw - 22.f, ry + 132.f, TextAlignment::Right);

    drawText(target, fontMedium, "CASHIER: " + effectiveCashier, 9.f, sf::Color(45, 40, 35), rx + 22.f, ry + 148.f);
    drawText(target, fontMedium, "STATUS:  PAID IN FULL", 9.f, sf::Color(40, 130, 60), rx + rw - 22.f, ry + 148.f, TextAlignment::Right);

    // Dashed divider line
    for (float cx = rx + 20.f; cx < rx + rw - 20.f; cx += 8.f) {
        sf::RectangleShape dash({5.f, 1.f});
        dash.setPosition(cx, ry + 160.f);
        dash.setFillColor(sf::Color(195, 190, 185));
        target.draw(dash);
    }

    // 8. Itemized Column Header
    drawText(target, fontSemiBold, "ITEM DESCRIPTION", 9.f, sf::Color(75, 70, 65), rx + 22.f, ry + 172.f);
    drawText(target, fontSemiBold, "QTY", 9.f, sf::Color(75, 70, 65), rx + 225.f, ry + 172.f, TextAlignment::Center);
    drawText(target, fontSemiBold, "PRICE", 9.f, sf::Color(75, 70, 65), rx + 315.f, ry + 172.f, TextAlignment::Right);
    drawText(target, fontSemiBold, "TOTAL", 9.f, sf::Color(75, 70, 65), rx + rw - 22.f, ry + 172.f, TextAlignment::Right);

    sf::RectangleShape colSep({rw - 44.f, 1.f});
    colSep.setPosition(rx + 22.f, ry + 182.f);
    colSep.setFillColor(sf::Color(215, 210, 205));
    target.draw(colSep);

    // 9. Render Purchased Cart Items
    float itemY = ry + 190.f;
    const auto& cart = cartService.getCart();
    int maxShow = 6;
    int shown = 0;
    for (size_t i = 0; i < cart.size(); ++i) {
        if (shown >= maxShow) break;
        const auto& line = cart[i];
        if (line.productId < 0 || line.productId >= (int)products.size()) continue;
        const auto& p = products[line.productId];

        std::string name = p.name;
        if (name.length() > 22) name = name.substr(0, 20) + "..";

        drawText(target, fontRegular, name, 9.5f, sf::Color(35, 30, 25), rx + 22.f, itemY);
        drawText(target, fontRegular, "x" + std::to_string(line.quantity), 9.5f, sf::Color(35, 30, 25), rx + 225.f, itemY, TextAlignment::Center);
        drawText(target, fontRegular, "Rs " + std::to_string(p.price), 9.5f, sf::Color(35, 30, 25), rx + 315.f, itemY, TextAlignment::Right);
        drawText(target, fontMedium, "Rs " + std::to_string(p.price * line.quantity), 9.5f, sf::Color(35, 30, 25), rx + rw - 22.f, itemY, TextAlignment::Right);

        itemY += 17.f;
        shown++;
    }
    if ((int)cart.size() > maxShow) {
        drawText(target, fontRegular, "... and " + std::to_string(cart.size() - maxShow) + " more item(s)", 8.5f, sf::Color(120, 115, 110), rx + 22.f, itemY);
        itemY += 15.f;
    }

    // Dashed divider line
    for (float cx = rx + 20.f; cx < rx + rw - 20.f; cx += 8.f) {
        sf::RectangleShape dash({5.f, 1.f});
        dash.setPosition(cx, itemY + 4.f);
        dash.setFillColor(sf::Color(195, 190, 185));
        target.draw(dash);
    }
    itemY += 14.f;

    // 10. Financial Summary
    drawText(target, fontRegular, "Subtotal", 9.5f, sf::Color(80, 75, 70), rx + 22.f, itemY);
    drawText(target, fontMedium, formatRs(cartService.getSubtotal(products)), 9.5f, sf::Color(35, 30, 25), rx + rw - 22.f, itemY, TextAlignment::Right);
    itemY += 16.f;

    if (cartService.getDiscount() > 0) {
        drawText(target, fontRegular, "Store Promo Discount (Applied)", 9.5f, sf::Color(170, 70, 50), rx + 22.f, itemY);
        drawText(target, fontMedium, "- " + formatRs(cartService.getDiscount()), 9.5f, sf::Color(170, 70, 50), rx + rw - 22.f, itemY, TextAlignment::Right);
        itemY += 16.f;
    }

    // Double Horizontal Divider
    sf::RectangleShape d1({rw - 44.f, 1.5f});
    d1.setPosition(rx + 22.f, itemY);
    d1.setFillColor(sf::Color(40, 35, 30));
    target.draw(d1);
    itemY += 3.f;

    sf::RectangleShape d2({rw - 44.f, 1.f});
    d2.setPosition(rx + 22.f, itemY);
    d2.setFillColor(sf::Color(40, 35, 30));
    target.draw(d2);
    itemY += 7.f;

    // Grand Total
    drawText(target, fontBold, "TOTAL AMOUNT:", 13.f, sf::Color(25, 20, 15), rx + 22.f, itemY);
    drawText(target, fontBold, formatRs(cartService.getTotal(products)), 14.5f, sf::Color(20, 18, 15), rx + rw - 22.f, itemY, TextAlignment::Right);
    itemY += 21.f;

    sf::RectangleShape d3({rw - 44.f, 1.f});
    d3.setPosition(rx + 22.f, itemY);
    d3.setFillColor(sf::Color(200, 195, 190));
    target.draw(d3);
    itemY += 7.f;

    // 11. Payment Mode & Tendered Details
    std::string payModeStr = (cartService.getPaymentMethod() == 0 ? "CASH" : "CARD (VISA/MASTERCARD)");
    drawText(target, fontRegular, "Payment Mode", 9.f, sf::Color(85, 80, 75), rx + 22.f, itemY);
    drawText(target, fontSemiBold, payModeStr, 9.f, sf::Color(35, 30, 25), rx + rw - 22.f, itemY, TextAlignment::Right);
    itemY += 15.f;

    if (cartService.getPaymentMethod() == 0) {
        drawText(target, fontRegular, "Cash Tendered", 9.f, sf::Color(85, 80, 75), rx + 22.f, itemY);
        drawText(target, fontMedium, formatRs(cartService.parseCashGiven()), 9.f, sf::Color(35, 30, 25), rx + rw - 22.f, itemY, TextAlignment::Right);
        itemY += 15.f;

        drawText(target, fontRegular, "Change Returned", 9.f, sf::Color(85, 80, 75), rx + 22.f, itemY);
        drawText(target, fontBold, formatRs(cartService.getChangeAmount(products)), 9.5f, sf::Color(35, 120, 50), rx + rw - 22.f, itemY, TextAlignment::Right);
        itemY += 16.f;
    } else {
        drawText(target, fontRegular, "POS Auth Code", 9.f, sf::Color(85, 80, 75), rx + 22.f, itemY);
        drawText(target, fontMedium, "AUTH #984321 - APPROVED", 9.f, sf::Color(35, 120, 50), rx + rw - 22.f, itemY, TextAlignment::Right);
        itemY += 16.f;
    }

    // Dashed divider line
    for (float cx = rx + 20.f; cx < rx + rw - 20.f; cx += 8.f) {
        sf::RectangleShape dash({5.f, 1.f});
        dash.setPosition(cx, itemY + 2.f);
        dash.setFillColor(sf::Color(195, 190, 185));
        target.draw(dash);
    }
    itemY += 12.f;

    // 12. Authentic Barcode Graphics on White Paper
    float bcX = rx + rw / 2.f - 110.f;
    float bcH = 26.f;
    std::string codeSeed = orderId + "2026";
    for (size_t ci = 0; ci < codeSeed.length(); ++ci) {
        unsigned char ch = (unsigned char)codeSeed[ci];
        int pat = (ch * 19 + 37);
        for (int b = 0; b < 5; ++b) {
            bool isBlack = ((pat >> b) & 1) == 0;
            float bWidth = ((pat >> (b + 1)) & 1) ? 2.5f : 1.2f;
            if (isBlack) {
                sf::RectangleShape bar({bWidth, bcH});
                bar.setPosition(bcX, itemY);
                bar.setFillColor(sf::Color(30, 25, 20));
                target.draw(bar);
            }
            bcX += bWidth + 1.2f;
        }
    }
    itemY += bcH + 5.f;
    drawText(target, fontMedium, "* " + orderId + " *", 8.5f, sf::Color(70, 65, 60), rx + rw / 2.f, itemY, TextAlignment::Center);
    itemY += 14.f;

    // 13. Store Thank You Note
    drawText(target, fontSemiBold, "*** THANK YOU FOR SHOPPING AT SCANIT ***", 9.f, sf::Color(35, 30, 25), rx + rw / 2.f, itemY, TextAlignment::Center);
    itemY += 13.f;
    drawText(target, fontRegular, "Items returnable within 7 days with original receipt", 8.f, sf::Color(115, 110, 105), rx + rw / 2.f, itemY, TextAlignment::Center);
    itemY += 12.f;
    drawText(target, fontRegular, "Customer Care: info@scanit-pos.com", 7.5f, sf::Color(135, 130, 125), rx + rw / 2.f, itemY, TextAlignment::Center);

    // 14. Action Buttons Below the Floating White Paper Slip
    float btnY = ry + rh + 12.f;

    // Close Button
    bool hovClose = pointInRect(m, rx, btnY, 210.f, 38.f);
    drawShadow(target, rx, btnY, 210.f, 38.f, 8, 4, 8, 30, 2);
    drawRounded(target, rx, btnY, 210.f, 38.f, 8,
                hovClose ? sf::Color(245, 238, 228) : sf::Color(255, 255, 255), ThemeColor::Edge, 1.2f);
    drawText(target, fontSemiBold, "Close Slip", 11.5f, ThemeColor::Ink, rx + 105.f, btnY + 19.f, TextAlignment::Center);

    // Start New Sale Button
    bool hovNew = pointInRect(m, rx + 230.f, btnY, 210.f, 38.f);
    drawShadow(target, rx + 230.f, btnY, 210.f, 38.f, 8, 4, 8, 30, 2);
    drawRounded(target, rx + 230.f, btnY, 210.f, 38.f, 8,
                hovNew ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "+  Start New Sale", 11.5f, ThemeColor::White, rx + 230.f + 105.f, btnY + 19.f, TextAlignment::Center);
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
        drawText(target, fontRegular, p.barcode, 9.5f, ThemeColor::Mute, dx + 86, rowY + 34);

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

void PosModalsView::drawAddInventoryModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                          float currentWidth, float designHeight,
                                          const std::string& supplier,
                                          const std::string& desc,
                                          const std::string& category,
                                          const std::string& unit,
                                          const std::string& retailPriceStr,
                                          const std::string& costPriceStr,
                                          const std::string& qtyStr,
                                          const std::string& dateStr,
                                          const std::string& barcodeStr,
                                          int activeField,
                                          bool barcodeGenerated,
                                          const std::vector<std::string>& previousVendors,
                                          bool vendorDropdownOpen,
                                          bool scanningUniversal,
                                          const sf::Font& fontRegular, const sf::Font& fontMedium,
                                          const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                          const sf::Font& fontSerif,
                                          std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape ov({currentWidth, designHeight});
    ov.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(ov);

    float dw = 620.f, dh = 590.f;
    float dx = (currentWidth - dw) / 2.f;
    float dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 16, 8, 20, 30, 5);
    drawRounded(target, dx, dy, dw, dh, 12, ThemeColor::Paper, ThemeColor::Edge, 1.2f);

    // Header
    drawText(target, fontSerif, "Add Supplier & Inventory Item", 20, ThemeColor::Ink, dx + 30, dy + 26);
    drawText(target, fontRegular, "Log supplier procurement batch, purchase price, scan or auto-generate barcode", 10.f, ThemeColor::Mute, dx + 30, dy + 46);

    auto drawFieldBox = [&](const std::string& label, const std::string& val, float x, float y, float w, int fieldIdx, const std::string& ph) {
        drawText(target, fontSemiBold, label, 9.5f, ThemeColor::Ink, x, y);
        bool focused = (activeField == fieldIdx);
        drawRounded(target, x, y + 16.f, w, 32.f, 6, ThemeColor::White, focused ? ThemeColor::Blue : ThemeColor::Line, focused ? 1.5f : 1.f);
        std::string disp = val.empty() ? ph : val;
        drawText(target, fontRegular, disp, 10.f, val.empty() ? ThemeColor::Mute : ThemeColor::Ink, x + 10.f, y + 32.f);
    };

    // Row 1: Supplier Name (Field 0) with Dropdown chevron & Arrival Date (Field 5)
    drawFieldBox("Supplier / Vendor Name:", supplier, dx + 30, dy + 68, 320, 0, "Select or enter vendor");
    // Dropdown chevron button
    bool hovDrop = pointInRect(mouse, dx + 30 + 320 - 28, dy + 68 + 16, 26, 32);
    drawRounded(target, dx + 30 + 320 - 28, dy + 68 + 16, 26, 32, 4, hovDrop ? sf::Color(230, 222, 210) : ThemeColor::None);
    drawText(target, fontBold, vendorDropdownOpen ? "^" : "v", 9.f, ThemeColor::Ink, dx + 30 + 320 - 15, dy + 68 + 32, TextAlignment::Center);

    drawFieldBox("Arrival Date:", dateStr, dx + 370, dy + 68, 220, 5, "YYYY-MM-DD");

    // Row 2: Description / Item Name (Field 1) & Unit (Field 7)
    drawFieldBox("Item Description / Name:", desc, dx + 30, dy + 125, 370, 1, "e.g. Almond Milk 1L");
    drawFieldBox("Unit:", unit, dx + 420, dy + 125, 170, 7, "e.g. 1 L, 1 kg, 1 pc");

    // Row 3: Expanded Supermarket Category Pills (2 rows of 4)
    drawText(target, fontSemiBold, "Department / Category:", 9.5f, ThemeColor::Ink, dx + 30, dy + 180);
    const char* cats[] = {"Grocery", "Dairy", "Drinks", "Bakery", "Snacks", "Fruits & Veg", "Household", "Personal Care"};
    for (int c = 0; c < 8; c++) {
        float cx = dx + 30 + (c % 4) * 142;
        float cy = dy + 198 + (c / 4) * 26;
        bool sel = (category == cats[c]);
        bool hov = pointInRect(mouse, cx, cy, 134, 22);
        drawRounded(target, cx, cy, 134, 22, 11, sel ? ThemeColor::Dark : (hov ? sf::Color(232, 224, 208) : sf::Color(242, 235, 222)), ThemeColor::Line, 1);
        drawText(target, fontSemiBold, cats[c], 8.5f, sel ? ThemeColor::White : ThemeColor::Ink, cx + 67, cy + 11, TextAlignment::Center);
    }

    // Row 4: Pricing & Quantity: Retail Price (Field 2), Cost Price (Field 3), Quantity (Field 4)
    drawFieldBox("Selling Price (PKR):", retailPriceStr, dx + 30, dy + 256, 170, 2, "Retail Price");
    drawFieldBox("Supplier Cost (PKR):", costPriceStr, dx + 225, dy + 256, 170, 3, "Cost Price");
    drawFieldBox("Purchase Qty:", qtyStr, dx + 420, dy + 256, 170, 4, "Units bought");

    // Row 5: Barcode, Generate & Live Scanner Options
    drawText(target, fontSemiBold, "Barcode (Universal / Code 39 Numeric):", 9.5f, ThemeColor::Ink, dx + 30, dy + 316);
    bool bcFocused = (activeField == 6);
    drawRounded(target, dx + 30, dy + 332, 240, 34, 6, ThemeColor::White, bcFocused ? ThemeColor::Blue : ThemeColor::Line, bcFocused ? 1.5f : 1.f);
    std::string bcDisp = barcodeStr.empty() ? (scanningUniversal ? "Ready to scan..." : "Enter or scan barcode") : barcodeStr;
    drawText(target, fontBold, bcDisp, 11.f, barcodeStr.empty() ? ThemeColor::Mute : ThemeColor::Ink, dx + 40, dy + 349);

    // Button 1: + Auto Barcode
    bool hovGen = pointInRect(mouse, dx + 280, dy + 332, 140, 34);
    drawRounded(target, dx + 280, dy + 332, 140, 34, 6, hovGen ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "+ Auto Barcode", 10.f, ThemeColor::White, dx + 350, dy + 349, TextAlignment::Center);

    // Button 2: Scan Barcode Option (Universal Manufacturer Barcode)
    bool hovScan = pointInRect(mouse, dx + 430, dy + 332, 160, 34);
    drawRounded(target, dx + 430, dy + 332, 160, 34, 6,
                scanningUniversal ? sf::Color(34, 197, 94) : (hovScan ? sf::Color(30, 70, 110) : sf::Color(40, 90, 140)));
    drawText(target, fontSemiBold, scanningUniversal ? "[OK] Scanner Active" : "[*] Scan Barcode", 10.f, ThemeColor::White, dx + 510, dy + 349, TextAlignment::Center);

    // Visual Barcode Preview Container
    drawRounded(target, dx + 30, dy + 376, dw - 60, 112, 8, sf::Color(245, 242, 235), ThemeColor::Line, 1.2f);
    if (!barcodeStr.empty()) {
        drawText(target, fontBold, barcodeStr, 12.f, ThemeColor::Ink, dx + 46, dy + 392);
        drawText(target, fontRegular, "Universal / Code 39 Barcode", 8.5f, ThemeColor::Mute, dx + 46, dy + 407);

        std::string bcPath = "barcodes/barcode_" + barcodeStr + ".bmp";
        drawSprite(target, getTexture(bcPath), dx + 46, dy + 420, 240, 52);

        int qVal = 0, cVal = 0;
        try { qVal = std::stoi(qtyStr); } catch (...) {}
        try { cVal = std::stoi(costPriceStr); } catch (...) {}
        int invoiceTot = qVal * cVal;

        drawText(target, fontSemiBold, "Supplier Invoice Summary:", 9.5f, ThemeColor::Ink, dx + 320, dy + 398);
        drawText(target, fontRegular, "Bought: " + std::to_string(qVal) + " units  *  Rs " + std::to_string(cVal) + "/unit", 9.f, ThemeColor::Mute, dx + 320, dy + 418);
        drawText(target, fontBold, "Total Procurement: Rs " + std::to_string(invoiceTot), 10.5f, ThemeColor::Blue, dx + 320, dy + 440);
    } else {
        std::string guide = scanningUniversal
            ? "Optical webcam scanner or barcode gun is listening! Hold item in front of camera or scan SKU."
            : "Click [Scan Barcode] to read universal barcodes from packaging, or [+ Auto Barcode] to generate a unique code.";
        drawText(target, fontRegular, guide, 9.5f, ThemeColor::Mute, dx + 46, dy + 424);
    }

    // Action Buttons
    bool hovSave = pointInRect(mouse, dx + 30, dy + dh - 52, 250, 38);
    drawRounded(target, dx + 30, dy + dh - 52, 250, 38, 6, hovSave ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontBold, "Save to Inventory & Catalog", 11.f, ThemeColor::White, dx + 155, dy + dh - 33, TextAlignment::Center);

    bool hovCancel = pointInRect(mouse, dx + dw - 150, dy + dh - 52, 120, 38);
    drawRounded(target, dx + dw - 150, dy + dh - 52, 120, 38, 6, hovCancel ? sf::Color(228, 218, 204) : sf::Color(235, 226, 212), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Cancel", 11, ThemeColor::Ink, dx + dw - 90, dy + dh - 33, TextAlignment::Center);

    // Floating Vendor Dropdown Menu (Drawn on top if open)
    if (vendorDropdownOpen && !previousVendors.empty()) {
        float dropX = dx + 30;
        float dropY = dy + 68 + 50;
        float dropW = 320.f;
        float rowH = 26.f;
        float dropH = std::min(160.f, (float)previousVendors.size() * rowH + 8.f);

        drawShadow(target, dropX, dropY, dropW, dropH, 12, 6, 12, 22, 4);
        drawRounded(target, dropX, dropY, dropW, dropH, 6, ThemeColor::White, ThemeColor::Line, 1.2f);

        for (size_t v = 0; v < previousVendors.size() && v < 6; v++) {
            float vy = dropY + 4.f + v * rowH;
            bool hovV = pointInRect(mouse, dropX + 2, vy, dropW - 4, rowH);
            if (hovV) {
                drawRounded(target, dropX + 2, vy, dropW - 4, rowH, 4, sf::Color(238, 244, 252));
            }
            drawText(target, fontRegular, previousVendors[v], 10.f, ThemeColor::Ink, dropX + 12, vy + 13);
        }
    }
}

void PosModalsView::drawProductSpecModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                         float currentWidth, float designHeight,
                                         const ProductItem& p,
                                         const sf::Font& fontRegular, const sf::Font& fontMedium,
                                         const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                         const sf::Font& fontSerif,
                                         std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    target.draw(dim);

    float dw = 620.f, dh = 480.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 20, 10, 20, 35, 6);
    drawRounded(target, dx, dy, dw, dh, 16, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    // Header Badge & Title
    drawRounded(target, dx + 28, dy + 22, 110, 20, 10, ThemeColor::Dark);
    drawText(target, fontSemiBold, "SPECIFICATION", 8.5f, ThemeColor::White, dx + 83, dy + 32, TextAlignment::Center);

    drawText(target, fontSerif, "Product Specifications", 20.f, ThemeColor::Ink, dx + 28, dy + 56);
    drawText(target, fontRegular, "Official Product.txt specifications and verified barcode image", 10.f, ThemeColor::Mute, dx + 28, dy + 76);

    // Close button [X]
    bool hovX = pointInRect(mouse, dx + dw - 44, dy + 22, 28, 28);
    drawRounded(target, dx + dw - 44, dy + 22, 28, 28, 6, hovX ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 13, ThemeColor::Ink, dx + dw - 30, dy + 36, TextAlignment::Center);

    // Left Column: Barcode BMP Card & Quick Glance
    float lX = dx + 28.f;
    float lY = dy + 104.f;
    float lW = 240.f;
    float lH = 300.f;

    drawRounded(target, lX, lY, lW, lH, 10, sf::Color(255, 255, 255), ThemeColor::Line, 1.f);

    // Actual BMP Barcode Image
    std::string bcImg = p.barcodePath.empty() ? ("barcodes/barcode_" + p.barcode + ".bmp") : p.barcodePath;
    drawText(target, fontSemiBold, "BARCODE GRAPHIC (BMP)", 8.5f, ThemeColor::Mute, lX + lW / 2.f, lY + 18, TextAlignment::Center);
    drawRounded(target, lX + 16, lY + 30, lW - 32, 68, 6, sf::Color(252, 250, 246), ThemeColor::Line, 1.f);
    drawSprite(target, getTexture(bcImg), lX + 22, lY + 36, lW - 44, 56);

    drawText(target, fontBold, "* " + p.barcode + " *", 11.f, ThemeColor::Ink, lX + lW / 2.f, lY + 112, TextAlignment::Center);

    // Thumbnail & Name
    drawRounded(target, lX + 20, lY + 134, 38, 38, 8, p.cardTint);
    drawSprite(target, getTexture("products/" + p.iconKey + ".png"), lX + 25, lY + 139, 28, 28);
    drawText(target, fontBold, p.name, 12.f, ThemeColor::Ink, lX + 68, lY + 144);
    drawText(target, fontRegular, p.category + " (" + p.unit + ")", 9.f, ThemeColor::Mute, lX + 68, lY + 160);

    // Quick Status Pill
    bool inStock = p.stock > 0;
    sf::Color stCol = p.stock > 15 ? ThemeColor::Green : (p.stock > 0 ? ThemeColor::Amber : ThemeColor::Red);
    drawRounded(target, lX + 20, lY + 186, lW - 40, 30, 6, sf::Color(stCol.r, stCol.g, stCol.b, 25));
    drawText(target, fontSemiBold, inStock ? ("In Stock: " + std::to_string(p.stock) + " available") : "Out of Stock", 9.5f, stCol, lX + lW / 2.f, lY + 201, TextAlignment::Center);

    // Price Display
    drawText(target, fontRegular, "Unit Selling Price", 9.f, ThemeColor::Mute, lX + lW / 2.f, lY + 236, TextAlignment::Center);
    drawText(target, fontBold, formatRs(p.price), 18.f, ThemeColor::Ink, lX + lW / 2.f, lY + 260, TextAlignment::Center);

    // Right Column: Complete Requirement 5 Formatted Fields
    float rX = dx + 288.f;
    float rY = dy + 104.f;
    float rW = dw - 316.f;
    float rH = 300.f;

    drawRounded(target, rX, rY, rW, rH, 10, sf::Color(255, 255, 255), ThemeColor::Line, 1.f);

    auto drawRow = [&](const std::string& label, const std::string& val, float cy, bool highlight = false) {
        drawText(target, fontMedium, label, 9.5f, ThemeColor::Mute, rX + 16, cy);
        drawText(target, highlight ? fontBold : fontSemiBold, val, 10.f, highlight ? ThemeColor::Amber : ThemeColor::Ink, rX + rW - 16, cy, TextAlignment::Right);
        sf::RectangleShape sep({rW - 32, 1});
        sep.setPosition(rX + 16, cy + 13);
        sep.setFillColor(sf::Color(240, 236, 230));
        target.draw(sep);
    };

    float ry0 = rY + 18;
    drawRow("Product ID", p.id, ry0);
    drawRow("Universal Barcode", p.barcode, ry0 + 26);
    drawRow("Product Acronym", p.acronym, ry0 + 52, true);
    drawRow("Official Name", p.name, ry0 + 78);
    drawRow("Category", p.category, ry0 + 104);
    drawRow("Measurement Unit", p.unit, ry0 + 130);
    drawRow("Unit Price (PKR)", "Rs " + std::to_string(p.price), ry0 + 156);
    drawRow("Unit Discount", "Rs " + std::to_string(p.discount), ry0 + 182);
    drawRow("Available Quantity", std::to_string(p.stock) + " " + p.unit, ry0 + 208);
    drawRow("Barcode File Path", bcImg, ry0 + 234);

    // Bottom Action Button
    bool hovClose = pointInRect(mouse, dx + dw / 2.f - 75, dy + dh - 52, 150, 36);
    drawRounded(target, dx + dw / 2.f - 75, dy + dh - 52, 150, 36, 8,
                hovClose ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "Close Details", 11.5f, ThemeColor::White, dx + dw / 2.f, dy + dh - 34, TextAlignment::Center);
}

void PosModalsView::drawQuickCheckModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                        float currentWidth, float designHeight,
                                        const std::string& barcodeQuery,
                                        bool inputFocused,
                                        const ProductItem* foundProduct,
                                        const sf::Font& fontRegular, const sf::Font& fontMedium,
                                        const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                        const sf::Font& fontSerif,
                                        std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    target.draw(dim);

    float dw = 560.f, dh = 480.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 20, 10, 20, 35, 6);
    drawRounded(target, dx, dy, dw, dh, 16, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    // Header ("QUICK CHECK" as requested!)
    drawText(target, fontSerif, "QUICK CHECK", 20.f, ThemeColor::Ink, dx + 28, dy + 36);
    drawText(target, fontRegular, "Scan barcode with camera or enter code to inspect price & stock", 9.5f, ThemeColor::Mute, dx + 28, dy + 56);

    // Close button [X]
    bool hovX = pointInRect(mouse, dx + dw - 44, dy + 22, 28, 28);
    drawRounded(target, dx + dw - 44, dy + 22, 28, 28, 6, hovX ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 13, ThemeColor::Ink, dx + dw - 30, dy + 36, TextAlignment::Center);

    // Barcode Search / Scan Input Box
    float inW = dw - 56.f;
    drawRounded(target, dx + 28, dy + 78, inW, 38, 8, sf::Color(255, 255, 255), inputFocused ? ThemeColor::Ink : ThemeColor::Line, inputFocused ? 1.5f : 1.f);
    drawSprite(target, getTexture("icons/scan.png"), dx + 40, dy + 88, 18, 18, ThemeColor::Mute);

    std::string dispCode = barcodeQuery.empty() ? "Aim barcode at camera or type 6-digit code..." : barcodeQuery;
    drawText(target, barcodeQuery.empty() ? fontRegular : fontBold, dispCode, 11.f,
             barcodeQuery.empty() ? ThemeColor::Mute : ThemeColor::Ink, dx + 68, dy + 97);

    // Quick Test Demo Pills
    const char* demoCodes[] = {"100001", "100002", "100004", "100006", "234567"};
    drawText(target, fontMedium, "Quick Test:", 8.5f, ThemeColor::Mute, dx + 28, dy + 130);
    for (int i = 0; i < 5; ++i) {
        float cx = dx + 90 + i * 62;
        bool hovChip = pointInRect(mouse, cx, dy + 122, 56, 20);
        drawRounded(target, cx, dy + 122, 56, 20, 5, hovChip ? sf::Color(230, 220, 206) : sf::Color(245, 240, 230), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, demoCodes[i], 8.5f, ThemeColor::Ink, cx + 28, dy + 132, TextAlignment::Center);
    }

    // Results Box
    float resY = dy + 152.f;
    float resH = 250.f;
    drawRounded(target, dx + 28, resY, inW, resH, 10, sf::Color(255, 255, 255), ThemeColor::Line, 1.f);

    if (foundProduct) {
        const auto& p = *foundProduct;

        // Top Row: Thumbnail, Name, Acronym, Category
        drawRounded(target, dx + 44, resY + 16, 44, 44, 8, p.cardTint);
        drawSprite(target, getTexture("products/" + p.iconKey + ".png"), dx + 50, resY + 22, 32, 32);

        drawText(target, fontBold, p.name, 15.f, ThemeColor::Ink, dx + 100, resY + 22);

        // Acronym tag
        drawRounded(target, dx + 100, resY + 40, 70, 18, 4, sf::Color(245, 235, 220));
        drawText(target, fontBold, p.acronym, 8.5f, ThemeColor::Amber, dx + 135, resY + 49, TextAlignment::Center);
        drawText(target, fontRegular, p.category + " • Unit: " + p.unit, 9.f, ThemeColor::Mute, dx + 180, resY + 49);

        // Big Price Display
        drawRounded(target, dx + dw - 180, resY + 16, 136, 46, 8, sf::Color(246, 250, 246), sf::Color(180, 220, 180), 1.f);
        drawText(target, fontRegular, "RETAIL PRICE", 8.f, ThemeColor::Green, dx + dw - 112, resY + 27, TextAlignment::Center);
        drawText(target, fontBold, formatRs(p.price), 15.f, ThemeColor::Ink, dx + dw - 112, resY + 46, TextAlignment::Center);

        // Divider
        sf::RectangleShape sep({inW - 32, 1});
        sep.setPosition(dx + 44, resY + 74);
        sep.setFillColor(sf::Color(235, 230, 224));
        target.draw(sep);

        // Specs Grid
        float sy = resY + 86;
        drawText(target, fontMedium, "Product ID:", 9.f, ThemeColor::Mute, dx + 44, sy);
        drawText(target, fontSemiBold, p.id, 9.5f, ThemeColor::Ink, dx + 130, sy);

        drawText(target, fontMedium, "Barcode:", 9.f, ThemeColor::Mute, dx + 44, sy + 22);
        drawText(target, fontSemiBold, p.barcode, 9.5f, ThemeColor::Ink, dx + 130, sy + 22);

        drawText(target, fontMedium, "Stock on Hand:", 9.f, ThemeColor::Mute, dx + 44, sy + 44);
        bool inStk = p.stock > 0;
        sf::Color stkCol = p.stock > 15 ? ThemeColor::Green : (p.stock > 0 ? ThemeColor::Amber : ThemeColor::Red);
        drawText(target, fontBold, std::to_string(p.stock) + " " + p.unit + " (" + (inStk ? "Available" : "Empty") + ")", 9.5f, stkCol, dx + 130, sy + 44);

        drawText(target, fontMedium, "Category:", 9.f, ThemeColor::Mute, dx + 260, sy);
        drawText(target, fontSemiBold, p.category, 9.5f, ThemeColor::Ink, dx + 340, sy);

        drawText(target, fontMedium, "Discount:", 9.f, ThemeColor::Mute, dx + 260, sy + 22);
        drawText(target, fontSemiBold, p.discount > 0 ? ("Rs " + std::to_string(p.discount)) : "None (0)", 9.5f, p.discount > 0 ? ThemeColor::Green : ThemeColor::Mute, dx + 340, sy + 22);

        drawText(target, fontMedium, "Status:", 9.f, ThemeColor::Mute, dx + 260, sy + 44);
        drawText(target, fontSemiBold, inStk ? "Active Catalog" : "Out of Stock", 9.5f, inStk ? ThemeColor::Green : ThemeColor::Red, dx + 340, sy + 44);

        // BMP Barcode Image Preview
        std::string bcImg = p.barcodePath.empty() ? ("barcodes/barcode_" + p.barcode + ".bmp") : p.barcodePath;
        drawRounded(target, dx + 44, resY + 162, inW - 32, 74, 6, sf::Color(252, 250, 246), ThemeColor::Line, 1.f);
        drawSprite(target, getTexture(bcImg), dx + 52, resY + 168, inW - 48, 48);
        drawText(target, fontSemiBold, "* " + p.barcode + " *", 9.f, ThemeColor::Mute, dx + dw / 2.f, resY + 224, TextAlignment::Center);

    } else {
        // Empty State / Scanner Ready
        drawSprite(target, getTexture("icons/scan.png"), dx + dw / 2.f - 24, resY + 60, 48, 48, ThemeColor::Mute);
        drawText(target, fontSemiBold, "Ready for Barcode Scan", 13.f, ThemeColor::Ink, dx + dw / 2.f, resY + 125, TextAlignment::Center);
        drawText(target, fontRegular, "Aim product barcode at camera or click one of the quick test pills above", 10.f, ThemeColor::Mute, dx + dw / 2.f, resY + 145, TextAlignment::Center);
    }

    // Action Buttons
    float btnY = dy + dh - 54.f;

    // Close Button
    bool hovClose = pointInRect(mouse, dx + 28, btnY, 180, 36);
    drawRounded(target, dx + 28, btnY, 180, 36, 8,
                hovClose ? sf::Color(230, 220, 206) : sf::Color(240, 235, 226), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Close (ESC)", 11.f, ThemeColor::Ink, dx + 118, btnY + 18, TextAlignment::Center);

    // Add to Active Cart Button (only if product found)
    if (foundProduct) {
        bool hovAdd = pointInRect(mouse, dx + dw - 238, btnY, 210, 36);
        drawRounded(target, dx + dw - 238, btnY, 210, 36, 8,
                    hovAdd ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontSemiBold, "+ Add to Active Cart", 11.f, ThemeColor::White, dx + dw - 133, btnY + 18, TextAlignment::Center);
    }
}

void PosModalsView::drawItemDescriptionModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                             float currentWidth, float designHeight,
                                             const std::string& name, const std::string& barcode,
                                             const std::string& category, const std::string& unit,
                                             int price, int cost, int stock,
                                             const std::string& supplier, const std::string& date,
                                             const sf::Font& fontRegular, const sf::Font& fontMedium,
                                             const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                             const sf::Font& fontSerif,
                                             std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 130));
    target.draw(dim);

    float dw = 500.f, dh = 360.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 16, 8, 16, 24, 6);
    drawRounded(target, dx, dy, dw, dh, 14, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    // Header
    drawText(target, fontSerif, "Item Description & Supplier Ledger", 18, ThemeColor::Ink, dx + 28, dy + 32);
    drawText(target, fontRegular, "Detailed audit specifications for registered item", 9.5f, ThemeColor::Mute, dx + 28, dy + 52);

    // Close X
    bool hovX = pointInRect(mouse, dx + dw - 40, dy + 18, 24, 24);
    drawRounded(target, dx + dw - 40, dy + 18, 24, 24, 5, hovX ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 12, ThemeColor::Ink, dx + dw - 28, dy + 30, TextAlignment::Center);

    // Item Name and Category pill
    drawText(target, fontBold, name, 16.f, ThemeColor::Ink, dx + 28, dy + 82);
    drawRounded(target, dx + 28, dy + 104, 80, 20, 10, ThemeColor::Dark);
    drawText(target, fontSemiBold, category, 8.5f, ThemeColor::White, dx + 68, dy + 114, TextAlignment::Center);
    drawText(target, fontMedium, "Unit: " + unit, 9.5f, ThemeColor::Mute, dx + 120, dy + 114);

    // Barcode number & Bitmap
    drawRounded(target, dx + 28, dy + 138, dw - 56, 120, 8, sf::Color(245, 241, 233), ThemeColor::Line, 1.f);
    drawText(target, fontBold, "Universal Barcode: " + barcode, 11.f, ThemeColor::Ink, dx + 44, dy + 154);

    std::string bmpPath = "barcodes/barcode_" + barcode + ".bmp";
    drawSprite(target, getTexture(bmpPath), dx + 44, dy + 172, 220, 48);

    // Details Grid
    float gy = dy + 154;
    float gx = dx + 290;
    drawText(target, fontRegular, "Retail Price:", 9.f, ThemeColor::Mute, gx, gy);
    drawText(target, fontBold, formatRs(price), 10.5f, ThemeColor::Ink, gx + 90, gy);

    drawText(target, fontRegular, "Supplier Cost:", 9.f, ThemeColor::Mute, gx, gy + 20);
    drawText(target, fontBold, formatRs(cost), 10.5f, ThemeColor::Blue, gx + 90, gy + 20);

    drawText(target, fontRegular, "Stock on Hand:", 9.f, ThemeColor::Mute, gx, gy + 40);
    drawText(target, fontBold, std::to_string(stock) + " units", 10.5f, stock < 15 ? ThemeColor::Amber : ThemeColor::Green, gx + 90, gy + 40);

    drawText(target, fontRegular, "Vendor:", 9.f, ThemeColor::Mute, gx, gy + 60);
    drawText(target, fontSemiBold, supplier.empty() ? "Inbound" : supplier, 9.5f, ThemeColor::Ink, gx + 90, gy + 60);

    drawText(target, fontRegular, "Arrival Date:", 9.f, ThemeColor::Mute, gx, gy + 80);
    drawText(target, fontRegular, date.empty() ? "2026-11-17" : date, 9.f, ThemeColor::Mute, gx + 90, gy + 80);

    // Close Button
    bool hovClose = pointInRect(mouse, dx + dw / 2.f - 60, dy + dh - 48, 120, 34);
    drawRounded(target, dx + dw / 2.f - 60, dy + dh - 48, 120, 34, 6, hovClose ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "Close", 11.f, ThemeColor::White, dx + dw / 2.f, dy + dh - 31, TextAlignment::Center);
}

void PosModalsView::drawAddProductFromInventoryModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                                    float currentWidth, float designHeight,
                                                    const std::vector<InventoryItem>& invItems,
                                                    const std::vector<int>& visibleIndices,
                                                    const std::string& searchQuery,
                                                    bool searchFocused,
                                                    int selectedItemIdx,
                                                    const std::string& transferQtyStr,
                                                    float scrollOffset,
                                                    const sf::Font& fontRegular, const sf::Font& fontMedium,
                                                    const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                                    const sf::Font& fontSerif,
                                                    std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(dim);

    float dw = 780.f, dh = 560.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 18, 10, 18, 26, 6);
    drawRounded(target, dx, dy, dw, dh, 14, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    // Header
    drawText(target, fontSerif, "Stock Products from Warehouse Inventory", 21, ThemeColor::Ink, dx + 28, dy + 32);
    drawText(target, fontRegular, "Select items received in inventory to add or replenish stock in active retail catalog", 10.f, ThemeColor::Mute, dx + 28, dy + 52);

    // Close X
    bool hovX = pointInRect(mouse, dx + dw - 42, dy + 20, 26, 26);
    drawRounded(target, dx + dw - 42, dy + 20, 26, 26, 5, hovX ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 12, ThemeColor::Ink, dx + dw - 29, dy + 33, TextAlignment::Center);

    // Search Box
    float sBoxX = dx + 28.f, sBoxY = dy + 70.f, sBoxW = 340.f, sBoxH = 32.f;
    drawRounded(target, sBoxX, sBoxY, sBoxW, sBoxH, 6, ThemeColor::White, searchFocused ? ThemeColor::Blue : ThemeColor::Line, searchFocused ? 1.5f : 1.f);
    std::string sDisp = searchQuery.empty() ? "Search inventory by name, barcode, vendor..." : searchQuery;
    drawText(target, fontRegular, sDisp, 9.5f, searchQuery.empty() ? ThemeColor::Mute : ThemeColor::Ink, sBoxX + 10, sBoxY + 16);

    // Table Header
    float ty = dy + 112.f;
    drawRounded(target, dx + 20, ty, dw - 40, 28, 5, sf::Color(238, 230, 214));
    drawText(target, fontSemiBold, "ITEM NAME", 9.5f, ThemeColor::Mute, dx + 35, ty + 14);
    drawText(target, fontSemiBold, "BARCODE", 9.5f, ThemeColor::Mute, dx + 230, ty + 14);
    drawText(target, fontSemiBold, "CATEGORY", 9.5f, ThemeColor::Mute, dx + 340, ty + 14);
    drawText(target, fontSemiBold, "RETAIL PRICE", 9.5f, ThemeColor::Mute, dx + 440, ty + 14);
    drawText(target, fontSemiBold, "INVENTORY STOCK", 9.5f, ThemeColor::Mute, dx + 560, ty + 14);
    drawText(target, fontSemiBold, "SELECT", 9.5f, ThemeColor::Mute, dx + dw - 65, ty + 14, TextAlignment::Center);

    // List of Inventory Items
    float listY = ty + 32.f;
    for (size_t vi = 0; vi < visibleIndices.size(); vi++) {
        int idx = visibleIndices[vi];
        if (idx < 0 || idx >= (int)invItems.size()) continue;
        const auto& it = invItems[idx];
        float ry = listY + vi * 42.f - scrollOffset;
        if (ry < listY - 5.f || ry > listY + 260.f) continue;

        bool isSel = (selectedItemIdx == idx);
        bool hovRow = pointInRect(mouse, dx + 20, ry, dw - 40, 38);

        sf::Color rowBg = isSel ? sf::Color(220, 236, 252) : (hovRow ? sf::Color(245, 238, 226) : ThemeColor::None);
        drawRounded(target, dx + 20, ry, dw - 40, 38, 5, rowBg, isSel ? ThemeColor::Blue : ThemeColor::None, isSel ? 1.2f : 0.f);

        drawText(target, fontBold, it.name, 11.f, ThemeColor::Ink, dx + 35, ry + 16);
        drawText(target, fontRegular, it.barcode, 10.f, ThemeColor::Ink, dx + 230, ry + 16);
        drawText(target, fontRegular, it.category + " (" + it.unit + ")", 9.5f, ThemeColor::Mute, dx + 340, ry + 16);
        drawText(target, fontBold, formatRs(it.retailPrice), 10.5f, ThemeColor::Ink, dx + 440, ry + 16);

        drawText(target, fontSemiBold, std::to_string(it.stock) + " available", 10.5f,
                 it.stock == 0 ? ThemeColor::Red : (it.stock < 15 ? ThemeColor::Amber : ThemeColor::Green),
                 dx + 560, ry + 16);

        bool hovBtn = pointInRect(mouse, dx + dw - 95, ry + 7, 60, 24);
        drawRounded(target, dx + dw - 95, ry + 7, 60, 24, 4,
                    isSel ? ThemeColor::Dark : (hovBtn ? ThemeColor::BlueHover : ThemeColor::Blue));
        drawText(target, fontSemiBold, isSel ? "Selected" : "Pick", 9.5f, ThemeColor::White, dx + dw - 65, ry + 19, TextAlignment::Center);

        sf::RectangleShape sep({dw - 40.f, 1});
        sep.setPosition(dx + 20, ry + 40);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);
    }

    // Bottom Action Bar: Quantity Selector & Add Button
    float botY = dy + dh - 64.f;
    drawRounded(target, dx + 20, botY - 10, dw - 40, 58, 8, sf::Color(242, 236, 224), ThemeColor::Edge, 1);

    if (selectedItemIdx >= 0 && selectedItemIdx < (int)invItems.size()) {
        const auto& sel = invItems[selectedItemIdx];
        drawText(target, fontBold, "Selected: " + sel.name + " (" + sel.barcode + ")", 11.5f, ThemeColor::Ink, dx + 35, botY + 8);
        drawText(target, fontRegular, "Avail in inventory: " + std::to_string(sel.stock) + " units", 9.5f, ThemeColor::Mute, dx + 35, botY + 24);

        // Quantity Stepper
        float qx = dx + 360;
        drawText(target, fontSemiBold, "Stock Qty:", 10.f, ThemeColor::Ink, qx, botY + 16);

        bool hovM = pointInRect(mouse, qx + 70, botY + 4, 26, 26);
        drawRounded(target, qx + 70, botY + 4, 26, 26, 4, hovM ? sf::Color(228, 218, 204) : ThemeColor::White, ThemeColor::Edge, 1);
        drawText(target, fontBold, "-", 12, ThemeColor::Ink, qx + 83, botY + 17, TextAlignment::Center);

        drawRounded(target, qx + 100, botY + 4, 42, 26, 4, ThemeColor::White, ThemeColor::Line, 1);
        drawText(target, fontBold, transferQtyStr.empty() ? "1" : transferQtyStr, 11.f, ThemeColor::Ink, qx + 121, botY + 17, TextAlignment::Center);

        bool hovP = pointInRect(mouse, qx + 146, botY + 4, 26, 26);
        drawRounded(target, qx + 146, botY + 4, 26, 26, 4, hovP ? sf::Color(228, 218, 204) : ThemeColor::White, ThemeColor::Edge, 1);
        drawText(target, fontBold, "+", 12, ThemeColor::Ink, qx + 159, botY + 17, TextAlignment::Center);

        // Add Button
        bool hovAdd = pointInRect(mouse, dx + dw - 220, botY + 3, 200, 32);
        drawRounded(target, dx + dw - 220, botY + 3, 200, 32, 6, hovAdd ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontBold, "Stock in Store Catalog", 10.5f, ThemeColor::White, dx + dw - 120, botY + 19, TextAlignment::Center);
    } else {
        drawText(target, fontRegular, "Click [Pick] on an inventory item above to select it, then choose quantity to stock in store catalog.", 10.5f, ThemeColor::Mute, dx + 35, botY + 16);
    }
}

void PosModalsView::drawHeldDraftsModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                       float currentWidth, float designHeight,
                                       const std::vector<DraftCart>& drafts,
                                       const std::vector<ProductItem>& products,
                                       const sf::Font& fontRegular, const sf::Font& fontMedium,
                                       const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                       const sf::Font& fontSerif) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(dim);

    float dw = 620.f, dh = 460.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 18, 10, 18, 26, 6);
    drawRounded(target, dx, dy, dw, dh, 14, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    // Header
    drawText(target, fontSerif, "Parked / Held Customer Carts", 20, ThemeColor::Ink, dx + 28, dy + 32);
    drawText(target, fontRegular, "Draft orders parked by cashier. Resume customer transaction when ready.", 10.f, ThemeColor::Mute, dx + 28, dy + 52);

    // Close X
    bool hovX = pointInRect(mouse, dx + dw - 40, dy + 20, 26, 26);
    drawRounded(target, dx + dw - 40, dy + 20, 26, 26, 5, hovX ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 12, ThemeColor::Ink, dx + dw - 27, dy + 33, TextAlignment::Center);

    float listY = dy + 85.f;
    if (drafts.empty()) {
        drawText(target, fontRegular, "No customer carts currently held on draft.", 12, ThemeColor::Mute,
                 dx + dw / 2.f, listY + 100, TextAlignment::Center);
        drawText(target, fontRegular, "Click 'Hold Cart' on the register to park a customer's items and serve the next buyer.", 10, ThemeColor::Mute,
                 dx + dw / 2.f, listY + 125, TextAlignment::Center);
    } else {
        for (size_t d = 0; d < drafts.size(); d++) {
            const auto& dr = drafts[d];
            float ry = listY + d * 64.f;
            if (ry > dy + dh - 80.f) break;

            drawRounded(target, dx + 24, ry, dw - 48, 56, 8, sf::Color(245, 239, 228), ThemeColor::Line, 1.f);

            drawText(target, fontBold, dr.draftId + ": " + dr.customerLabel, 12.f, ThemeColor::Ink, dx + 38, ry + 16);
            drawText(target, fontRegular, "Held at: " + dr.time + " | " + std::to_string(dr.items.size()) + " unique items", 9.5f, ThemeColor::Mute, dx + 38, ry + 34);

            drawText(target, fontBold, formatRs(dr.totalAmount), 13.f, ThemeColor::Blue, dx + dw - 220, ry + 25, TextAlignment::Right);

            // Resume Button
            bool hovResume = pointInRect(mouse, dx + dw - 200, ry + 12, 90, 32);
            drawRounded(target, dx + dw - 200, ry + 12, 90, 32, 5, hovResume ? ThemeColor::BlueHover : ThemeColor::Blue);
            drawText(target, fontBold, "Resume", 10.5f, ThemeColor::White, dx + dw - 155, ry + 28, TextAlignment::Center);

            // Discard Button
            bool hovDisc = pointInRect(mouse, dx + dw - 100, ry + 12, 70, 32);
            drawRounded(target, dx + dw - 100, ry + 12, 70, 32, 5,
                        hovDisc ? sf::Color(254, 226, 226) : sf::Color(244, 236, 226),
                        hovDisc ? ThemeColor::Red : ThemeColor::Edge, 1);
            drawText(target, fontSemiBold, "Discard", 10.f, hovDisc ? ThemeColor::Red : ThemeColor::Mute, dx + dw - 65, ry + 28, TextAlignment::Center);
        }
    }

    // Close button
    bool hovC = pointInRect(mouse, dx + dw / 2.f - 60, dy + dh - 48, 120, 34);
    drawRounded(target, dx + dw / 2.f - 60, dy + dh - 48, 120, 34, 6, hovC ? sf::Color(228, 218, 204) : sf::Color(235, 226, 212), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Close", 11.f, ThemeColor::Ink, dx + dw / 2.f, dy + dh - 31, TextAlignment::Center);
}

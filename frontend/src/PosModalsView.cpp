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
                                          const sf::Texture& cameraTexture,
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

    float dw = 920.f, dh = 595.f;
    float dx = (currentWidth - dw) / 2.f;
    float dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 18, 8, 20, 30, 5);
    drawRounded(target, dx, dy, dw, dh, 12, ThemeColor::Paper, ThemeColor::Edge, 1.2f);

    // ─────────────────────────────────────────────────────────────
    // LEFT SIDE: Live Scanner Viewport Screen (Requested Feature)
    // ─────────────────────────────────────────────────────────────
    float scanX = dx + 24.f;
    float scanY = dy + 24.f;
    float scanW = 280.f;
    float scanH = dh - 48.f;

    drawRounded(target, scanX, scanY, scanW, scanH, 10, sf::Color(18, 18, 22), sf::Color(45, 45, 55), 1.2f);

    // Live continuous status badge at top
    sf::Color badgeBg = scanningUniversal ? sf::Color(15, 45, 25) : sf::Color(32, 32, 38);
    sf::Color badgeBorder = scanningUniversal ? sf::Color(34, 197, 94) : sf::Color(90, 90, 100);
    drawRounded(target, scanX + 12.f, scanY + 12.f, scanW - 24.f, 24.f, 5, badgeBg, badgeBorder, 1.f);
    sf::CircleShape liveDot(3.5f, 16);
    liveDot.setOrigin(3.5f, 3.5f);
    liveDot.setPosition(scanX + 24.f, scanY + 24.f);
    liveDot.setFillColor(scanningUniversal ? sf::Color(34, 197, 94) : sf::Color(180, 180, 190));
    target.draw(liveDot);
    std::string badgeTxt = scanningUniversal ? "LIVE SCANNER ACTIVE" : "SCANNER STANDBY";
    drawText(target, fontSemiBold, badgeTxt, 8.5f, scanningUniversal ? sf::Color(220, 252, 231) : sf::Color(200, 200, 210), scanX + 34.f, scanY + 24.f);

    // Camera Viewfinder Box
    float paneX = scanX + 12.f;
    float paneY = scanY + 44.f;
    float paneW = scanW - 24.f;
    float paneH = 220.f;

    drawRounded(target, paneX, paneY, paneW, paneH, 8, sf::Color(10, 10, 14));

    if (cameraTexture.getSize().x > 0) {
        sf::Sprite camSprite(cameraTexture);
        float srcW = (float)cameraTexture.getSize().x;
        float srcH = (float)cameraTexture.getSize().y;
        float targetAspect = paneW / paneH;
        float cropH = srcW / targetAspect;
        float cropW = srcW;
        if (cropH <= srcH) {
            float cropY = (srcH - cropH) / 2.f;
            camSprite.setTextureRect(sf::IntRect(0, (int)cropY, (int)srcW, (int)cropH));
        } else {
            cropW = srcH * targetAspect;
            float cropX = (srcW - cropW) / 2.f;
            camSprite.setTextureRect(sf::IntRect((int)cropX, 0, (int)cropW, (int)srcH));
        }
        camSprite.setPosition(paneX, paneY);
        camSprite.setScale(paneW / camSprite.getTextureRect().width, paneH / camSprite.getTextureRect().height);
        target.draw(camSprite);
    } else {
        drawSprite(target, getTexture("icons/scan.png"), paneX + paneW / 2.f - 24, paneY + paneH / 2.f - 26, 48, 48, sf::Color(180, 180, 190, 180));
        drawText(target, fontSemiBold, "Camera Stream Active", 11.f, sf::Color(220, 220, 220), paneX + paneW / 2.f, paneY + paneH / 2.f + 24, TextAlignment::Center);
    }

    // 4 Corner Optical Reticles
    float retSize = 16.f, retThick = 2.5f, retPad = 8.f;
    sf::Color retColor = scanningUniversal ? sf::Color(34, 197, 94, 230) : sf::Color(255, 255, 255, 160);
    // Top-Left
    sf::RectangleShape cTL1({retSize, retThick}); cTL1.setPosition(paneX + retPad, paneY + retPad); cTL1.setFillColor(retColor); target.draw(cTL1);
    sf::RectangleShape cTL2({retThick, retSize}); cTL2.setPosition(paneX + retPad, paneY + retPad); cTL2.setFillColor(retColor); target.draw(cTL2);
    // Top-Right
    sf::RectangleShape cTR1({retSize, retThick}); cTR1.setPosition(paneX + paneW - retPad - retSize, paneY + retPad); cTR1.setFillColor(retColor); target.draw(cTR1);
    sf::RectangleShape cTR2({retThick, retSize}); cTR2.setPosition(paneX + paneW - retPad - retThick, paneY + retPad); cTR2.setFillColor(retColor); target.draw(cTR2);
    // Bottom-Left
    sf::RectangleShape cBL1({retSize, retThick}); cBL1.setPosition(paneX + retPad, paneY + paneH - retPad - retThick); cBL1.setFillColor(retColor); target.draw(cBL1);
    sf::RectangleShape cBL2({retThick, retSize}); cBL2.setPosition(paneX + retPad, paneY + paneH - retPad - retSize); cBL2.setFillColor(retColor); target.draw(cBL2);
    // Bottom-Right
    sf::RectangleShape cBR1({retSize, retThick}); cBR1.setPosition(paneX + paneW - retPad - retSize, paneY + paneH - retPad - retThick); cBR1.setFillColor(retColor); target.draw(cBR1);
    sf::RectangleShape cBR2({retThick, retSize}); cBR2.setPosition(paneX + paneW - retPad - retThick, paneY + paneH - retPad - retSize); cBR2.setFillColor(retColor); target.draw(cBR2);

    // Laser Sweep Line
    if (scanningUniversal) {
        float sweep = 0.5f + 0.5f * std::sin((float)clock() / CLOCKS_PER_SEC * 3.5f);
        float ly = paneY + 8.f + sweep * (paneH - 16.f);
        sf::RectangleShape laser({paneW, 2.5f});
        laser.setPosition(paneX, ly);
        laser.setFillColor(sf::Color(240, 50, 40, 230));
        target.draw(laser);
    }

    // Viewfinder Instructions / Status Box below
    float infoY = paneY + paneH + 12.f;
    drawRounded(target, scanX + 12.f, infoY, scanW - 24.f, 96.f, 6, sf::Color(26, 26, 32), sf::Color(50, 50, 60), 1.f);
    if (!barcodeStr.empty()) {
        drawText(target, fontMedium, "DETECTED / SET SKU:", 8.f, sf::Color(140, 150, 165), scanX + 22.f, infoY + 12.f);
        drawText(target, fontBold, barcodeStr, 13.f, sf::Color(34, 197, 94), scanX + 22.f, infoY + 30.f);
        drawText(target, fontRegular, "Barcode mapped to inventory item", 8.5f, sf::Color(160, 160, 170), scanX + 22.f, infoY + 52.f);
        drawText(target, fontSemiBold, "[OK] Verified", 8.5f, sf::Color(52, 211, 153), scanX + 22.f, infoY + 74.f);
    } else {
        drawText(target, fontSemiBold, scanningUniversal ? "Ready to Scan" : "Scanner Idle", 10.f, scanningUniversal ? sf::Color(240, 240, 240) : sf::Color(180, 180, 190), scanX + 22.f, infoY + 14.f);
        if (scanningUniversal) {
            drawText(target, fontRegular, "Hold packaging barcode to lens.", 8.5f, sf::Color(150, 150, 160), scanX + 22.f, infoY + 34.f);
            drawText(target, fontRegular, "Detected SKU auto-fills form.", 8.5f, sf::Color(150, 150, 160), scanX + 22.f, infoY + 50.f);
        } else {
            drawText(target, fontRegular, "Scanner standby. Click [Activate Scanner]", 8.5f, sf::Color(150, 150, 160), scanX + 22.f, infoY + 34.f);
            drawText(target, fontRegular, "or tap camera screen to scan.", 8.5f, sf::Color(150, 150, 160), scanX + 22.f, infoY + 50.f);
        }
    }

    // Device switch tip
    drawText(target, fontRegular, "Tip: Press 'C' key to switch between cameras", 8.f, sf::Color(130, 130, 140), scanX + scanW / 2.f, infoY + 114.f, TextAlignment::Center);

    // Quick Scanner Toggle Button on the panel
    bool hovPanelBtn = pointInRect(mouse, scanX + 12.f, scanY + scanH - 46.f, scanW - 24.f, 34.f);
    sf::Color panelBtnCol = scanningUniversal
        ? (hovPanelBtn ? sf::Color(22, 163, 74) : sf::Color(34, 197, 94))
        : (hovPanelBtn ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawRounded(target, scanX + 12.f, scanY + scanH - 46.f, scanW - 24.f, 34.f, 6, panelBtnCol);
    std::string panelBtnLabel = scanningUniversal ? "[||] Pause Optical Scanner" : "[>] Activate Camera Scanner";
    drawText(target, fontSemiBold, panelBtnLabel, 9.5f, ThemeColor::White, scanX + scanW / 2.f, scanY + scanH - 29.f, TextAlignment::Center);

    // ─────────────────────────────────────────────────────────────
    // RIGHT SIDE: Inventory Item & Supplier Batch Form
    // ─────────────────────────────────────────────────────────────
    float formX = scanX + scanW + 24.f;
    float formW = dw - (scanW + 48.f) - 24.f;

    // Header
    drawText(target, fontSerif, "Add Supplier & Inventory Item", 20, ThemeColor::Ink, formX, dy + 24);
    drawText(target, fontRegular, "Log supplier procurement batch, purchase price, scan or auto-generate barcode", 9.5f, ThemeColor::Mute, formX, dy + 44);

    auto drawFieldBox = [&](const std::string& label, const std::string& val, float x, float y, float w, int fieldIdx, const std::string& ph) {
        drawText(target, fontSemiBold, label, 9.5f, ThemeColor::Ink, x, y);
        bool focused = (activeField == fieldIdx);
        drawRounded(target, x, y + 16.f, w, 32.f, 6, ThemeColor::White, focused ? ThemeColor::Blue : ThemeColor::Line, focused ? 1.5f : 1.f);
        std::string disp = val.empty() ? ph : val;
        drawText(target, fontRegular, disp, 10.f, val.empty() ? ThemeColor::Mute : ThemeColor::Ink, x + 10.f, y + 32.f);
    };

    // Row 1: Supplier Name (Field 0) with Dropdown chevron & Arrival Date (Field 5)
    drawFieldBox("Supplier / Vendor Name:", supplier, formX, dy + 68, 320, 0, "Select or enter vendor");
    // Dropdown chevron button
    bool hovDrop = pointInRect(mouse, formX + 320 - 28, dy + 68 + 16, 26, 32);
    drawRounded(target, formX + 320 - 28, dy + 68 + 16, 26, 32, 4, hovDrop ? sf::Color(230, 222, 210) : ThemeColor::None);
    drawText(target, fontBold, vendorDropdownOpen ? "^" : "v", 9.f, ThemeColor::Ink, formX + 320 - 15, dy + 68 + 32, TextAlignment::Center);

    drawFieldBox("Arrival Date:", dateStr, formX + 338, dy + 68, formW - 338, 5, "YYYY-MM-DD");

    // Row 2: Description / Item Name (Field 1) & Unit (Field 7)
    drawFieldBox("Item Description / Name:", desc, formX, dy + 125, 360, 1, "e.g. Almond Milk 1L");
    drawFieldBox("Unit:", unit, formX + 378, dy + 125, formW - 378, 7, "e.g. 1 L, 1 kg, 1 pc");

    // Row 3: Supermarket Category Pills (2 rows of 4)
    drawText(target, fontSemiBold, "Department / Category:", 9.5f, ThemeColor::Ink, formX, dy + 180);
    const char* cats[] = {"Grocery", "Dairy", "Drinks", "Bakery", "Snacks", "Fruits & Veg", "Household", "Personal Care"};
    float pillW = (formW - 24.f) / 4.f;
    for (int c = 0; c < 8; c++) {
        float cx = formX + (c % 4) * (pillW + 8.f);
        float cy = dy + 198 + (c / 4) * 26;
        bool sel = (category == cats[c]);
        bool hov = pointInRect(mouse, cx, cy, pillW, 22);
        drawRounded(target, cx, cy, pillW, 22, 11, sel ? ThemeColor::Dark : (hov ? sf::Color(232, 224, 208) : sf::Color(242, 235, 222)), ThemeColor::Line, 1);
        drawText(target, fontSemiBold, cats[c], 8.5f, sel ? ThemeColor::White : ThemeColor::Ink, cx + pillW / 2.f, cy + 11, TextAlignment::Center);
    }

    // Row 4: Pricing & Quantity: Retail Price (Field 2), Cost Price (Field 3), Quantity (Field 4)
    float col3W = (formW - 20.f) / 3.f;
    drawFieldBox("Selling Price (PKR):", retailPriceStr, formX, dy + 256, col3W, 2, "Retail Price");
    drawFieldBox("Supplier Cost (PKR):", costPriceStr, formX + col3W + 10.f, dy + 256, col3W, 3, "Cost Price");
    drawFieldBox("Purchase Qty:", qtyStr, formX + (col3W + 10.f) * 2.f, dy + 256, col3W, 4, "Units bought");

    // Row 5: Barcode, Generate & Live Scanner Options
    drawText(target, fontSemiBold, "Barcode (Universal / Code 39 Numeric):", 9.5f, ThemeColor::Ink, formX, dy + 316);
    bool bcFocused = (activeField == 6);
    float bcInputW = formW - 310.f;
    drawRounded(target, formX, dy + 332, bcInputW, 34, 6, ThemeColor::White, bcFocused ? ThemeColor::Blue : ThemeColor::Line, bcFocused ? 1.5f : 1.f);
    std::string bcDisp = barcodeStr.empty() ? (scanningUniversal ? "Aim at camera on left..." : "Enter or scan barcode") : barcodeStr;
    drawText(target, fontBold, bcDisp, 11.f, barcodeStr.empty() ? ThemeColor::Mute : ThemeColor::Ink, formX + 10, dy + 349);

    // Button 1: + Auto Barcode
    bool hovGen = pointInRect(mouse, formX + bcInputW + 10.f, dy + 332, 135, 34);
    drawRounded(target, formX + bcInputW + 10.f, dy + 332, 135, 34, 6, hovGen ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "+ Auto Barcode", 9.5f, ThemeColor::White, formX + bcInputW + 10.f + 67.f, dy + 349, TextAlignment::Center);

    // Button 2: Scan Barcode Option (Universal Manufacturer Barcode)
    float scanBtnW = formW - (bcInputW + 155.f);
    bool hovScan = pointInRect(mouse, formX + bcInputW + 155.f, dy + 332, scanBtnW, 34);
    drawRounded(target, formX + bcInputW + 155.f, dy + 332, scanBtnW, 34, 6,
                scanningUniversal ? sf::Color(34, 197, 94) : (hovScan ? sf::Color(30, 70, 110) : sf::Color(40, 90, 140)));
    drawText(target, fontSemiBold, scanningUniversal ? "[OK] Scanning..." : "[*] Scan Barcode", 9.5f, ThemeColor::White, formX + bcInputW + 155.f + scanBtnW / 2.f, dy + 349, TextAlignment::Center);

    // Visual Barcode Preview Container
    drawRounded(target, formX, dy + 376, formW, 112, 8, sf::Color(245, 242, 235), ThemeColor::Line, 1.2f);
    if (!barcodeStr.empty()) {
        drawText(target, fontBold, barcodeStr, 12.f, ThemeColor::Ink, formX + 16, dy + 392);
        drawText(target, fontRegular, "Universal / Code 39 Barcode", 8.5f, ThemeColor::Mute, formX + 16, dy + 407);

        std::string bcPath = "barcodes/barcode_" + barcodeStr + ".bmp";
        drawSprite(target, getTexture(bcPath), formX + 16, dy + 420, 220, 52);

        int qVal = 0, cVal = 0;
        try { qVal = std::stoi(qtyStr); } catch (...) {}
        try { cVal = std::stoi(costPriceStr); } catch (...) {}
        int invoiceTot = qVal * cVal;

        drawText(target, fontSemiBold, "Supplier Invoice Summary:", 9.5f, ThemeColor::Ink, formX + 260, dy + 398);
        drawText(target, fontRegular, "Bought: " + std::to_string(qVal) + " units  *  Rs " + std::to_string(cVal) + "/unit", 9.f, ThemeColor::Mute, formX + 260, dy + 418);
        drawText(target, fontBold, "Total Procurement: Rs " + std::to_string(invoiceTot), 10.5f, ThemeColor::Blue, formX + 260, dy + 440);
    } else {
        std::string guide = scanningUniversal
            ? "Optical webcam scanner is active on the left! Hold item in front of camera or scan SKU."
            : "Use the optical scanner on the left, click [Scan Barcode], or [+ Auto Barcode] to generate a code.";
        drawText(target, fontRegular, guide, 9.5f, ThemeColor::Mute, formX + 16, dy + 424);
    }

    // Action Buttons
    bool hovSave = pointInRect(mouse, formX, dy + dh - 52, 260, 38);
    drawRounded(target, formX, dy + dh - 52, 260, 38, 6, hovSave ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontBold, "Save to Inventory & Catalog", 11.f, ThemeColor::White, formX + 130, dy + dh - 33, TextAlignment::Center);

    bool hovCancel = pointInRect(mouse, formX + formW - 130, dy + dh - 52, 130, 38);
    drawRounded(target, formX + formW - 130, dy + dh - 52, 130, 38, 6, hovCancel ? sf::Color(228, 218, 204) : sf::Color(235, 226, 212), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Cancel", 11, ThemeColor::Ink, formX + formW - 65, dy + dh - 33, TextAlignment::Center);

    // Floating Vendor Dropdown Menu (Drawn on top if open)
    if (vendorDropdownOpen && !previousVendors.empty()) {
        float dropX = formX;
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
                                        const sf::Texture& cameraTexture,
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

    float dw = 840.f, dh = 500.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 20, 10, 20, 35, 6);
    drawRounded(target, dx, dy, dw, dh, 16, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    // ─────────────────────────────────────────────────────────────
    // LEFT SIDE: Live Scanner Viewport Screen (Requested Feature)
    // ─────────────────────────────────────────────────────────────
    float scanX = dx + 24.f;
    float scanY = dy + 24.f;
    float scanW = 320.f;
    float scanH = dh - 48.f;

    drawRounded(target, scanX, scanY, scanW, scanH, 12, sf::Color(18, 18, 22), sf::Color(45, 45, 55), 1.2f);

    // Top Header Badge
    drawRounded(target, scanX + 12.f, scanY + 12.f, scanW - 24.f, 24.f, 5, sf::Color(15, 45, 25), sf::Color(34, 197, 94), 1.f);
    sf::CircleShape liveDot(3.5f, 16);
    liveDot.setOrigin(3.5f, 3.5f);
    liveDot.setPosition(scanX + 24.f, scanY + 24.f);
    liveDot.setFillColor(sf::Color(34, 197, 94));
    target.draw(liveDot);
    drawText(target, fontSemiBold, "OPTICAL BARCODE SCANNER", 8.5f, sf::Color(220, 252, 231), scanX + 34.f, scanY + 24.f);

    // Camera Viewfinder Box
    float paneX = scanX + 12.f;
    float paneY = scanY + 44.f;
    float paneW = scanW - 24.f;
    float paneH = 240.f;

    drawRounded(target, paneX, paneY, paneW, paneH, 8, sf::Color(10, 10, 14));

    if (cameraTexture.getSize().x > 0) {
        sf::Sprite camSprite(cameraTexture);
        float srcW = (float)cameraTexture.getSize().x;
        float srcH = (float)cameraTexture.getSize().y;
        float targetAspect = paneW / paneH;
        float cropH = srcW / targetAspect;
        float cropW = srcW;
        if (cropH <= srcH) {
            float cropY = (srcH - cropH) / 2.f;
            camSprite.setTextureRect(sf::IntRect(0, (int)cropY, (int)srcW, (int)cropH));
        } else {
            cropW = srcH * targetAspect;
            float cropX = (srcW - cropW) / 2.f;
            camSprite.setTextureRect(sf::IntRect((int)cropX, 0, (int)cropW, (int)srcH));
        }
        camSprite.setPosition(paneX, paneY);
        camSprite.setScale(paneW / camSprite.getTextureRect().width, paneH / camSprite.getTextureRect().height);
        target.draw(camSprite);
    } else {
        drawSprite(target, getTexture("icons/scan.png"), paneX + paneW / 2.f - 24, paneY + paneH / 2.f - 26, 48, 48, sf::Color(180, 180, 190, 180));
        drawText(target, fontSemiBold, "Optical Scanner Active", 11.f, sf::Color(220, 220, 220), paneX + paneW / 2.f, paneY + paneH / 2.f + 24, TextAlignment::Center);
    }

    // 4 Corner Optical Reticles
    float retSize = 18.f, retThick = 2.5f, retPad = 10.f;
    sf::Color retColor(34, 197, 94, 230);
    // Top-Left
    sf::RectangleShape cTL1({retSize, retThick}); cTL1.setPosition(paneX + retPad, paneY + retPad); cTL1.setFillColor(retColor); target.draw(cTL1);
    sf::RectangleShape cTL2({retThick, retSize}); cTL2.setPosition(paneX + retPad, paneY + retPad); cTL2.setFillColor(retColor); target.draw(cTL2);
    // Top-Right
    sf::RectangleShape cTR1({retSize, retThick}); cTR1.setPosition(paneX + paneW - retPad - retSize, paneY + retPad); cTR1.setFillColor(retColor); target.draw(cTR1);
    sf::RectangleShape cTR2({retThick, retSize}); cTR2.setPosition(paneX + paneW - retPad - retThick, paneY + retPad); cTR2.setFillColor(retColor); target.draw(cTR2);
    // Bottom-Left
    sf::RectangleShape cBL1({retSize, retThick}); cBL1.setPosition(paneX + retPad, paneY + paneH - retPad - retThick); cBL1.setFillColor(retColor); target.draw(cBL1);
    sf::RectangleShape cBL2({retThick, retSize}); cBL2.setPosition(paneX + retPad, paneY + paneH - retPad - retSize); cBL2.setFillColor(retColor); target.draw(cBL2);
    // Bottom-Right
    sf::RectangleShape cBR1({retSize, retThick}); cBR1.setPosition(paneX + paneW - retPad - retSize, paneY + paneH - retPad - retThick); cBR1.setFillColor(retColor); target.draw(cBR1);
    sf::RectangleShape cBR2({retThick, retSize}); cBR2.setPosition(paneX + paneW - retPad - retThick, paneY + paneH - retPad - retSize); cBR2.setFillColor(retColor); target.draw(cBR2);

    // Laser Sweep Line
    float sweep = 0.5f + 0.5f * std::sin((float)clock() / CLOCKS_PER_SEC * 3.5f);
    float ly = paneY + 8.f + sweep * (paneH - 16.f);
    sf::RectangleShape laser({paneW, 2.5f});
    laser.setPosition(paneX, ly);
    laser.setFillColor(sf::Color(240, 50, 40, 230));
    target.draw(laser);

    // Info & Instruction Box below viewfinder
    float infoY = paneY + paneH + 12.f;
    drawRounded(target, scanX + 12.f, infoY, scanW - 24.f, 96.f, 6, sf::Color(26, 26, 32), sf::Color(50, 50, 60), 1.f);
    if (foundProduct) {
        drawText(target, fontMedium, "MATCHED PRODUCT:", 8.f, sf::Color(140, 150, 165), scanX + 22.f, infoY + 12.f);
        drawText(target, fontBold, foundProduct->name, 11.5f, sf::Color(34, 197, 94), scanX + 22.f, infoY + 30.f);
        drawText(target, fontRegular, "Barcode: " + foundProduct->barcode + " | " + foundProduct->category, 8.5f, sf::Color(180, 180, 190), scanX + 22.f, infoY + 52.f);
        drawText(target, fontSemiBold, "[OK] Price: Rs " + std::to_string(foundProduct->price), 9.5f, sf::Color(52, 211, 153), scanX + 22.f, infoY + 74.f);
    } else {
        drawText(target, fontSemiBold, "Continuous Scanner Ready", 10.5f, sf::Color(240, 240, 240), scanX + 22.f, infoY + 14.f);
        drawText(target, fontRegular, "Aim any product barcode at the camera or use quick test buttons on the right.", 8.5f, sf::Color(150, 150, 160), scanX + 22.f, infoY + 34.f);
        drawText(target, fontMedium, "Autofills price, stock, & specs", 8.5f, sf::Color(96, 165, 250), scanX + 22.f, infoY + 68.f);
    }

    // Camera Switch Hint
    drawText(target, fontRegular, "Press 'C' key to switch between camera devices", 8.f, sf::Color(130, 130, 140), scanX + scanW / 2.f, scanY + scanH - 18.f, TextAlignment::Center);

    // ─────────────────────────────────────────────────────────────
    // RIGHT SIDE: Quick Check Search, Demo Pills & Details
    // ─────────────────────────────────────────────────────────────
    float rightX = scanX + scanW + 24.f;
    float rightW = dw - (scanW + 48.f) - 24.f;

    // Header
    drawText(target, fontSerif, "QUICK CHECK", 20.f, ThemeColor::Ink, rightX, dy + 32);
    drawText(target, fontRegular, "Search by barcode or product name to inspect price & stock", 9.5f, ThemeColor::Mute, rightX, dy + 52);

    // Close button [X]
    bool hovX = pointInRect(mouse, dx + dw - 44, dy + 20, 28, 28);
    drawRounded(target, dx + dw - 44, dy + 20, 28, 28, 6, hovX ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 13, ThemeColor::Ink, dx + dw - 30, dy + 34, TextAlignment::Center);

    // Barcode Search / Scan Input Box
    drawRounded(target, rightX, dy + 68, rightW, 36, 8, sf::Color(255, 255, 255), inputFocused ? ThemeColor::Ink : ThemeColor::Line, inputFocused ? 1.5f : 1.f);
    drawSprite(target, getTexture("icons/scan.png"), rightX + 12, dy + 77, 18, 18, ThemeColor::Mute);

    std::string dispCode = barcodeQuery.empty() ? "Aim barcode at camera or type name / 6-digit code..." : barcodeQuery;
    drawText(target, barcodeQuery.empty() ? fontRegular : fontBold, dispCode, 10.5f,
             barcodeQuery.empty() ? ThemeColor::Mute : ThemeColor::Ink, rightX + 38, dy + 86);

    // Quick Test Demo Pills
    const char* demoCodes[] = {"100001", "100002", "100004", "100006", "234567"};
    drawText(target, fontMedium, "Quick Test:", 8.5f, ThemeColor::Mute, rightX, dy + 118);
    float pillStartX = rightX + 62.f;
    float qPillW = (rightW - 62.f - 16.f) / 5.f;
    for (int i = 0; i < 5; ++i) {
        float cx = pillStartX + i * (qPillW + 4.f);
        bool hovChip = pointInRect(mouse, cx, dy + 110, qPillW, 20);
        drawRounded(target, cx, dy + 110, qPillW, 20, 5, hovChip ? sf::Color(230, 220, 206) : sf::Color(245, 240, 230), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, demoCodes[i], 8.5f, ThemeColor::Ink, cx + qPillW / 2.f, dy + 120, TextAlignment::Center);
    }

    // Results Box
    float resY = dy + 138.f;
    float resH = 290.f;
    drawRounded(target, rightX, resY, rightW, resH, 10, sf::Color(255, 255, 255), ThemeColor::Line, 1.f);

    if (foundProduct) {
        const auto& p = *foundProduct;

        // Top Row: Thumbnail, Name, Acronym, Category
        drawRounded(target, rightX + 14, resY + 14, 42, 42, 8, p.cardTint);
        drawSprite(target, getTexture("products/" + p.iconKey + ".png"), rightX + 19, resY + 19, 32, 32);

        drawText(target, fontBold, p.name, 14.f, ThemeColor::Ink, rightX + 66, resY + 18);

        // Acronym tag
        drawRounded(target, rightX + 66, resY + 36, 60, 18, 4, sf::Color(245, 235, 220));
        drawText(target, fontBold, p.acronym, 8.5f, ThemeColor::Amber, rightX + 96, resY + 45, TextAlignment::Center);
        drawText(target, fontRegular, p.category + "  *  Unit: " + p.unit, 8.5f, ThemeColor::Mute, rightX + 134, resY + 45);

        // Retail Price Display
        float priceBoxW = 120.f;
        drawRounded(target, rightX + rightW - priceBoxW - 14.f, resY + 12, priceBoxW, 46, 8, sf::Color(246, 250, 246), sf::Color(180, 220, 180), 1.f);
        drawText(target, fontRegular, "RETAIL PRICE", 7.5f, ThemeColor::Green, rightX + rightW - priceBoxW / 2.f - 14.f, resY + 22, TextAlignment::Center);
        drawText(target, fontBold, formatRs(p.price), 14.f, ThemeColor::Ink, rightX + rightW - priceBoxW / 2.f - 14.f, resY + 40, TextAlignment::Center);

        // Divider
        sf::RectangleShape sep({rightW - 28.f, 1});
        sep.setPosition(rightX + 14, resY + 66);
        sep.setFillColor(sf::Color(235, 230, 224));
        target.draw(sep);

        // Specs Grid
        float sy = resY + 76;
        float halfW = (rightW - 28.f) / 2.f;

        drawText(target, fontMedium, "Product ID:", 8.5f, ThemeColor::Mute, rightX + 14, sy);
        drawText(target, fontSemiBold, p.id, 9.f, ThemeColor::Ink, rightX + 90, sy);

        drawText(target, fontMedium, "Barcode:", 8.5f, ThemeColor::Mute, rightX + 14, sy + 20);
        drawText(target, fontSemiBold, p.barcode, 9.f, ThemeColor::Ink, rightX + 90, sy + 20);

        drawText(target, fontMedium, "Stock on Hand:", 8.5f, ThemeColor::Mute, rightX + 14, sy + 40);
        bool inStk = p.stock > 0;
        sf::Color stkCol = p.stock > 15 ? ThemeColor::Green : (p.stock > 0 ? ThemeColor::Amber : ThemeColor::Red);
        drawText(target, fontBold, std::to_string(p.stock) + " " + p.unit + " (" + (inStk ? "Available" : "Empty") + ")", 9.f, stkCol, rightX + 90, sy + 40);

        drawText(target, fontMedium, "Category:", 8.5f, ThemeColor::Mute, rightX + 14 + halfW, sy);
        drawText(target, fontSemiBold, p.category, 9.f, ThemeColor::Ink, rightX + 80 + halfW, sy);

        drawText(target, fontMedium, "Discount:", 8.5f, ThemeColor::Mute, rightX + 14 + halfW, sy + 20);
        drawText(target, fontSemiBold, p.discount > 0 ? ("Rs " + std::to_string(p.discount)) : "None (0)", 9.f, p.discount > 0 ? ThemeColor::Green : ThemeColor::Mute, rightX + 80 + halfW, sy + 20);

        drawText(target, fontMedium, "Catalog Status:", 8.5f, ThemeColor::Mute, rightX + 14 + halfW, sy + 40);
        drawText(target, fontSemiBold, inStk ? "Active Catalog" : "Out of Stock", 9.f, inStk ? ThemeColor::Green : ThemeColor::Red, rightX + 80 + halfW, sy + 40);

        // BMP Barcode Image Preview
        std::string bcImg = p.barcodePath.empty() ? ("barcodes/barcode_" + p.barcode + ".bmp") : p.barcodePath;
        drawRounded(target, rightX + 14, resY + 144, rightW - 28, 62, 6, sf::Color(252, 250, 246), ThemeColor::Line, 1.f);
        drawSprite(target, getTexture(bcImg), rightX + 22, resY + 149, rightW - 44, 40);
        drawText(target, fontSemiBold, "* " + p.barcode + " *", 8.5f, ThemeColor::Mute, rightX + rightW / 2.f, resY + 196, TextAlignment::Center);

    } else {
        // Empty State / Scanner Ready
        drawSprite(target, getTexture("icons/scan.png"), rightX + rightW / 2.f - 22, resY + 60, 44, 44, ThemeColor::Mute);
        drawText(target, fontSemiBold, "Ready for Barcode Scan", 12.f, ThemeColor::Ink, rightX + rightW / 2.f, resY + 120, TextAlignment::Center);
        drawText(target, fontRegular, "Aim product barcode at camera or search by name / SKU", 9.5f, ThemeColor::Mute, rightX + rightW / 2.f, resY + 140, TextAlignment::Center);
    }

    // Action Buttons
    float btnY = dy + dh - 54.f;

    // Close Button
    bool hovClose = pointInRect(mouse, rightX, btnY, 130, 36);
    drawRounded(target, rightX, btnY, 130, 36, 8,
                hovClose ? sf::Color(230, 220, 206) : sf::Color(240, 235, 226), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Close (ESC)", 10.5f, ThemeColor::Ink, rightX + 65, btnY + 18, TextAlignment::Center);

    // Add to Active Cart Button (only if product found)
    if (foundProduct) {
        float addW = rightW - 145.f;
        bool hovAdd = pointInRect(mouse, rightX + 145.f, btnY, addW, 36);
        drawRounded(target, rightX + 145.f, btnY, addW, 36, 8,
                    hovAdd ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontSemiBold, "+ Add to Active Cart", 10.5f, ThemeColor::White, rightX + 145.f + addW / 2.f, btnY + 18, TextAlignment::Center);
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

    float dw = 540.f, dh = 400.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 16, 8, 16, 24, 6);
    drawRounded(target, dx, dy, dw, dh, 14, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    // Header
    drawText(target, fontSerif, "Item Description & Supplier Ledger", 18, ThemeColor::Ink, dx + 28, dy + 32);
    drawText(target, fontRegular, "Detailed audit specifications for registered item", 9.5f, ThemeColor::Mute, dx + 28, dy + 52);

    // Close X
    bool hovX = pointInRect(mouse, dx + dw - 40, dy + 18, 26, 26);
    drawRounded(target, dx + dw - 40, dy + 18, 26, 26, 5, hovX ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 12, ThemeColor::Ink, dx + dw - 27, dy + 30, TextAlignment::Center);

    // Item Name and Category pill
    drawText(target, fontBold, name, 16.f, ThemeColor::Ink, dx + 28, dy + 82);
    drawRounded(target, dx + 28, dy + 104, 80, 20, 10, ThemeColor::Dark);
    drawText(target, fontSemiBold, category, 8.5f, ThemeColor::White, dx + 68, dy + 114, TextAlignment::Center);
    drawText(target, fontMedium, "Unit: " + unit, 9.5f, ThemeColor::Mute, dx + 120, dy + 114);

    // Barcode number & Bitmap
    drawRounded(target, dx + 28, dy + 138, dw - 56, 150, 8, sf::Color(245, 241, 233), ThemeColor::Line, 1.f);
    drawText(target, fontBold, "Universal Barcode: " + barcode, 11.f, ThemeColor::Ink, dx + 44, dy + 154);

    std::string bmpPath = "barcodes/barcode_" + barcode + ".bmp";
    drawSprite(target, getTexture(bmpPath), dx + 44, dy + 176, 220, 52);

    // Details Grid
    float gy = dy + 154;
    float gx = dx + 290;
    drawText(target, fontRegular, "Retail Price:", 9.f, ThemeColor::Mute, gx, gy);
    drawText(target, fontBold, formatRs(price), 10.5f, ThemeColor::Ink, gx + 95, gy);

    drawText(target, fontRegular, "Supplier Cost:", 9.f, ThemeColor::Mute, gx, gy + 22);
    drawText(target, fontBold, formatRs(cost), 10.5f, ThemeColor::Blue, gx + 95, gy + 22);

    drawText(target, fontRegular, "Stock on Hand:", 9.f, ThemeColor::Mute, gx, gy + 44);
    drawText(target, fontBold, std::to_string(stock) + " units", 10.5f, stock < 15 ? ThemeColor::Amber : ThemeColor::Green, gx + 95, gy + 44);

    drawText(target, fontRegular, "Vendor:", 9.f, ThemeColor::Mute, gx, gy + 66);
    drawText(target, fontSemiBold, supplier.empty() ? "Inbound" : supplier, 9.5f, ThemeColor::Ink, gx + 95, gy + 66);

    drawText(target, fontRegular, "Arrival Date:", 9.f, ThemeColor::Mute, gx, gy + 88);
    drawText(target, fontRegular, date.empty() ? "2026-11-17" : date, 9.f, ThemeColor::Mute, gx + 95, gy + 88);

    // Close Button
    bool hovClose = pointInRect(mouse, dx + dw / 2.f - 70, dy + dh - 48, 140, 34);
    drawRounded(target, dx + dw / 2.f - 70, dy + dh - 48, 140, 34, 6, hovClose ? ThemeColor::BlueHover : ThemeColor::Blue);
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

void PosModalsView::drawDiscountModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                     float currentWidth, float designHeight,
                                     const CartService& cartService,
                                     int studentRate, int employeeRate,
                                     const std::string& empUser, const std::string& empPass, int activeField,
                                     const std::string& promoInput,
                                     const std::string& errorMessage,
                                     const sf::Font& fontRegular, const sf::Font& fontMedium,
                                     const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                     const sf::Font& fontSerif,
                                     std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 160));
    target.draw(dim);

    float dw = 620.f, dh = 520.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    drawShadow(target, dx, dy, dw, dh, 22, 10, 20, 40, 6);
    drawRounded(target, dx, dy, dw, dh, 16, ThemeColor::Paper, ThemeColor::Line, 1.2f);

    // Header
    drawText(target, fontSerif, "APPLY DISCOUNT", 20.f, ThemeColor::Ink, dx + 28, dy + 32);
    drawText(target, fontRegular, "Choose an eligible discount: Student, Employee, or Promo voucher", 9.5f, ThemeColor::Mute, dx + 28, dy + 52);

    // Close button [X]
    bool hovX = pointInRect(mouse, dx + dw - 44, dy + 20, 28, 28);
    drawRounded(target, dx + dw - 44, dy + 20, 28, 28, 6, hovX ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 13, ThemeColor::Ink, dx + dw - 30, dy + 34, TextAlignment::Center);

    // ─────────────────────────────────────────────────────────────
    // 1. STUDENT DISCOUNT CARD
    // ─────────────────────────────────────────────────────────────
    float c1Y = dy + 74.f;
    bool isStudentActive = (cartService.getDiscountCategory() == DiscountCategory::Student);
    drawRounded(target, dx + 24, c1Y, dw - 48, 76, 10,
                isStudentActive ? sf::Color(230, 245, 235) : sf::Color(245, 240, 230),
                isStudentActive ? ThemeColor::Green : ThemeColor::Line, 1.2f);

    drawSprite(target, getTexture("icons/tag.png"), dx + 38, c1Y + 16, 20, 20, isStudentActive ? ThemeColor::Green : ThemeColor::Ink);
    drawText(target, fontBold, "Student Discount (" + std::to_string(studentRate) + "% OFF)", 12.f, ThemeColor::Ink, dx + 66, c1Y + 22);
    drawText(target, fontRegular, "Fixed student policy configured in Admin Settings. Applied with 1-click.", 9.f, ThemeColor::Mute, dx + 66, c1Y + 44);

    float btn1X = dx + dw - 24 - 170.f;
    float btn1Y = c1Y + 20.f;
    bool hovB1 = pointInRect(mouse, btn1X, btn1Y, 170, 36);
    drawRounded(target, btn1X, btn1Y, 170, 36, 6,
                isStudentActive ? sf::Color(34, 150, 80) : (hovB1 ? ThemeColor::BlueHover : ThemeColor::Blue));
    drawText(target, fontBold, isStudentActive ? "Active (Applied)" : "Apply Student (" + std::to_string(studentRate) + "%)",
             9.5f, ThemeColor::White, btn1X + 85, btn1Y + 18, TextAlignment::Center);

    // ─────────────────────────────────────────────────────────────
    // 2. EMPLOYEE DISCOUNT CARD
    // ─────────────────────────────────────────────────────────────
    float c2Y = dy + 158.f;
    bool isEmpActive = (cartService.getDiscountCategory() == DiscountCategory::Employee);
    drawRounded(target, dx + 24, c2Y, dw - 48, 122, 10,
                isEmpActive ? sf::Color(230, 245, 235) : sf::Color(245, 240, 230),
                isEmpActive ? ThemeColor::Green : ThemeColor::Line, 1.2f);

    drawSprite(target, getTexture("icons/clipboard.png"), dx + 38, c2Y + 14, 20, 20, isEmpActive ? ThemeColor::Green : ThemeColor::Ink);
    drawText(target, fontBold, "Employee Discount (" + std::to_string(employeeRate) + "% OFF)", 12.f, ThemeColor::Ink, dx + 66, c2Y + 20);
    drawText(target, fontRegular, "Requires staff username & security PIN authentication from staff.txt", 9.f, ThemeColor::Mute, dx + 66, c2Y + 38);

    // Username Box
    float fUserX = dx + 38.f;
    float fUserY = c2Y + 62.f;
    drawText(target, fontMedium, "Staff Username:", 8.5f, ThemeColor::Mute, fUserX, fUserY - 10);
    drawRounded(target, fUserX, fUserY, 160, 32, 6, sf::Color(255, 255, 255),
                (activeField == 1) ? ThemeColor::Blue : ThemeColor::Line, (activeField == 1) ? 1.5f : 1.f);
    std::string dispUser = empUser.empty() ? (activeField == 1 ? "|" : "Enter username") : (empUser + (activeField == 1 ? "|" : ""));
    drawText(target, fontRegular, dispUser, 9.5f, empUser.empty() ? ThemeColor::Mute : ThemeColor::Ink, fUserX + 10, fUserY + 16);

    // Password Box
    float fPassX = fUserX + 170.f;
    float fPassY = c2Y + 62.f;
    drawText(target, fontMedium, "Security PIN / Password:", 8.5f, ThemeColor::Mute, fPassX, fPassY - 10);
    drawRounded(target, fPassX, fPassY, 150, 32, 6, sf::Color(255, 255, 255),
                (activeField == 2) ? ThemeColor::Blue : ThemeColor::Line, (activeField == 2) ? 1.5f : 1.f);
    std::string maskedPass = std::string(empPass.size(), '*');
    std::string dispPass = maskedPass.empty() ? (activeField == 2 ? "|" : "Enter PIN") : (maskedPass + (activeField == 2 ? "|" : ""));
    drawText(target, fontRegular, dispPass, 9.5f, maskedPass.empty() ? ThemeColor::Mute : ThemeColor::Ink, fPassX + 10, fPassY + 16);

    // Verify & Apply Button
    float btn2X = fPassX + 160.f;
    float btn2Y = c2Y + 62.f;
    bool hovB2 = pointInRect(mouse, btn2X, btn2Y, dw - 48 - (btn2X - (dx + 24)) - 14, 32);
    drawRounded(target, btn2X, btn2Y, dw - 48 - (btn2X - (dx + 24)) - 14, 32, 6,
                isEmpActive ? sf::Color(34, 150, 80) : (hovB2 ? ThemeColor::BlueHover : ThemeColor::Blue));
    drawText(target, fontBold, isEmpActive ? "Active (Verified)" : "Verify & Apply",
             9.5f, ThemeColor::White, btn2X + (dw - 48 - (btn2X - (dx + 24)) - 14) / 2.f, btn2Y + 16, TextAlignment::Center);

    // ─────────────────────────────────────────────────────────────
    // 3. PROMO CODE CARD
    // ─────────────────────────────────────────────────────────────
    float c3Y = dy + 288.f;
    bool isPromoActive = (cartService.getDiscountCategory() == DiscountCategory::Promo);
    drawRounded(target, dx + 24, c3Y, dw - 48, 108, 10,
                isPromoActive ? sf::Color(230, 245, 235) : sf::Color(245, 240, 230),
                isPromoActive ? ThemeColor::Green : ThemeColor::Line, 1.2f);

    drawSprite(target, getTexture("icons/tag.png"), dx + 38, c3Y + 14, 20, 20, isPromoActive ? ThemeColor::Green : ThemeColor::Ink);
    drawText(target, fontBold, "Promotional Voucher Code", 12.f, ThemeColor::Ink, dx + 66, c3Y + 20);
    drawText(target, fontRegular, "Enter promo code to validate against promo.txt database", 9.f, ThemeColor::Mute, dx + 66, c3Y + 38);

    // Code Input Box
    float pInX = dx + 38.f;
    float pInY = c3Y + 58.f;
    drawRounded(target, pInX, pInY, 320, 34, 6, sf::Color(255, 255, 255),
                (activeField == 3) ? ThemeColor::Blue : ThemeColor::Line, (activeField == 3) ? 1.5f : 1.f);
    std::string dispCode = promoInput.empty() ? (activeField == 3 ? "|" : "Enter Promo Code (e.g. WELCOME10, SAVE50)...")
                                              : (promoInput + (activeField == 3 ? "|" : ""));
    drawText(target, fontRegular, dispCode, 9.5f, promoInput.empty() ? ThemeColor::Mute : ThemeColor::Ink, pInX + 12, pInY + 17);

    // Apply Promo Button
    float btn3X = pInX + 330.f;
    float btn3Y = pInY;
    bool hovB3 = pointInRect(mouse, btn3X, btn3Y, dw - 48 - (btn3X - (dx + 24)) - 14, 34);
    drawRounded(target, btn3X, btn3Y, dw - 48 - (btn3X - (dx + 24)) - 14, 34, 6,
                isPromoActive ? sf::Color(34, 150, 80) : (hovB3 ? ThemeColor::BlueHover : ThemeColor::Blue));
    drawText(target, fontBold, isPromoActive ? ("Active: " + cartService.getAppliedPromoCode()) : "Apply Code",
             9.5f, ThemeColor::White, btn3X + (dw - 48 - (btn3X - (dx + 24)) - 14) / 2.f, btn3Y + 17, TextAlignment::Center);

    // Error / Status Banner
    float errY = dy + 406.f;
    if (!errorMessage.empty()) {
        drawRounded(target, dx + 24, errY, dw - 48, 28, 6, sf::Color(254, 226, 226), sf::Color(220, 80, 70), 1.f);
        drawText(target, fontBold, errorMessage, 9.5f, ThemeColor::Red, dx + dw / 2.f, errY + 14, TextAlignment::Center);
    } else if (cartService.isDiscountApplied()) {
        drawRounded(target, dx + 24, errY, dw - 48, 28, 6, sf::Color(220, 252, 231), sf::Color(22, 101, 52), 1.f);
        drawText(target, fontBold, "Current Active: " + cartService.getDiscountLabel(), 9.5f, sf::Color(22, 101, 52), dx + dw / 2.f, errY + 14, TextAlignment::Center);
    }

    // Bottom Action Buttons
    float botY = dy + dh - 48.f;
    if (cartService.isDiscountApplied()) {
        bool hovRem = pointInRect(mouse, dx + 24, botY, 190, 34);
        drawRounded(target, dx + 24, botY, 190, 34, 6,
                    hovRem ? sf::Color(254, 226, 226) : sf::Color(245, 238, 230), ThemeColor::Red, 1.f);
        drawText(target, fontSemiBold, "Remove Applied Discount", 9.5f, ThemeColor::Red, dx + 119, botY + 17, TextAlignment::Center);
    }

    bool hovClose = pointInRect(mouse, dx + dw - 144, botY, 120, 34);
    drawRounded(target, dx + dw - 144, botY, 120, 34, 6,
                hovClose ? sf::Color(228, 218, 204) : sf::Color(235, 226, 212), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Close", 10.5f, ThemeColor::Ink, dx + dw - 84, botY + 17, TextAlignment::Center);
}

// ─────────────────────────────────────────────────────────────────────────────
// SALE AUDIT DETAIL MODAL (Inspection of Items & Cashier on Duty)
// ─────────────────────────────────────────────────────────────────────────────
void PosModalsView::drawSaleDetailModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                        float currentWidth, float designHeight,
                                        const SaleRecord& sale,
                                        const std::vector<ProductItem>& products,
                                        const sf::Font& fontRegular, const sf::Font& fontMedium,
                                        const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                        const sf::Font& fontSerif,
                                        std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(dim);

    float dw = 620.f, dh = 520.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    // Card background
    drawRounded(target, dx, dy, dw, dh, 12, ThemeColor::Panel, ThemeColor::Line, 1.5f);

    // Close [X] Button
    bool hovX = pointInRect(mouse, dx + dw - 40, dy + 16, 26, 26);
    drawRounded(target, dx + dw - 40, dy + 16, 26, 26, 5, hovX ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 12, ThemeColor::Ink, dx + dw - 27, dy + 28, TextAlignment::Center);

    // Modal Header
    drawText(target, fontSerif, "Sale Transaction Audit Details", 18.f, ThemeColor::Ink, dx + 26, dy + 22);
    drawText(target, fontRegular, "Order Reference: " + sale.orderId + "  |  Recorded: " + sale.timestamp,
             9.5f, ThemeColor::Mute, dx + 26, dy + 44);

    // Info Badges Row
    float infoY = dy + 68.f;
    float cardW = (dw - 64.f) / 3.f;

    // Badge 1: Cashier on Duty
    drawRounded(target, dx + 26, infoY, cardW, 46, 6, ThemeColor::Paper, ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "Cashier Present:", 8.5f, ThemeColor::Mute, dx + 36, infoY + 8);
    std::string cashierDisp = sale.cashier.empty() ? "admin" : sale.cashier;
    drawText(target, fontBold, cashierDisp, 11.5f, sf::Color(30, 95, 150), dx + 36, infoY + 23);

    // Badge 2: Payment Method
    float b2X = dx + 26 + cardW + 6.f;
    drawRounded(target, b2X, infoY, cardW, 46, 6, ThemeColor::Paper, ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "Payment Method:", 8.5f, ThemeColor::Mute, b2X + 10, infoY + 8);
    drawText(target, fontBold, sale.paymentMethod.empty() ? "Cash" : sale.paymentMethod, 11.5f, ThemeColor::Ink, b2X + 10, infoY + 23);

    // Badge 3: Total Paid
    float b3X = b2X + cardW + 6.f;
    drawRounded(target, b3X, infoY, cardW, 46, 6, ThemeColor::Paper, ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "Total Amount:", 8.5f, ThemeColor::Mute, b3X + 10, infoY + 8);
    drawText(target, fontBold, formatRs(sale.totalAmount), 12.f, ThemeColor::Green, b3X + 10, infoY + 23);

    // Purchased Items Section
    float tblY = infoY + 56.f;
    float tblH = 285.f;
    drawRounded(target, dx + 26, tblY, dw - 52, tblH, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);

    // Table Header
    drawText(target, fontSemiBold, "#", 8.5f, ThemeColor::Mute, dx + 40, tblY + 12);
    drawText(target, fontSemiBold, "Purchased Item & Barcode", 8.5f, ThemeColor::Mute, dx + 70, tblY + 12);
    drawText(target, fontSemiBold, "Unit Price", 8.5f, ThemeColor::Mute, dx + 340, tblY + 12);
    drawText(target, fontSemiBold, "Qty", 8.5f, ThemeColor::Mute, dx + 440, tblY + 12);
    drawText(target, fontSemiBold, "Subtotal", 8.5f, ThemeColor::Mute, dx + dw - 46, tblY + 12, TextAlignment::Right);

    sf::RectangleShape tblSep({dw - 72.f, 1.f});
    tblSep.setPosition(dx + 36, tblY + 24.f);
    tblSep.setFillColor(ThemeColor::Line);
    target.draw(tblSep);

    // Table Rows
    float rowY = tblY + 34.f;
    if (sale.items.empty()) {
        drawText(target, fontRegular, "No individual line item details stored for this legacy order.",
                 10.f, ThemeColor::Mute, dx + dw / 2.f, tblY + tblH / 2.f, TextAlignment::Center);
    } else {
        int idx = 1;
        for (const auto& line : sale.items) {
            if (rowY > tblY + tblH - 32.f) break;

            // Match product
            std::string itemName = "Product #" + std::to_string(line.productId);
            std::string barcode = "N/A";
            int unitPrice = 0;

            for (const auto& p : products) {
                if (p.id == std::to_string(line.productId)) {
                    itemName = p.name;
                    barcode = p.barcode;
                    unitPrice = p.price;
                    break;
                }
            }

            int lineSubtotal = unitPrice * line.quantity;

            drawText(target, fontRegular, std::to_string(idx), 9.f, ThemeColor::Mute, dx + 40, rowY);
            drawText(target, fontBold, itemName, 10.f, ThemeColor::Ink, dx + 70, rowY - 1);
            drawText(target, fontRegular, "Barcode: " + barcode, 8.f, ThemeColor::Mute, dx + 70, rowY + 13);
            drawText(target, fontMedium, formatRs(unitPrice), 9.5f, ThemeColor::Ink, dx + 340, rowY + 5);
            drawText(target, fontBold, std::to_string(line.quantity) + "x", 10.f, ThemeColor::Ink, dx + 440, rowY + 5);
            drawText(target, fontBold, formatRs(lineSubtotal), 10.5f, ThemeColor::Ink, dx + dw - 46, rowY + 5, TextAlignment::Right);

            sf::RectangleShape rSep({dw - 72.f, 1.f});
            rSep.setPosition(dx + 36, rowY + 28.f);
            rSep.setFillColor(sf::Color(240, 235, 226));
            target.draw(rSep);

            rowY += 34.f;
            idx++;
        }
    }

    // Cashier & Register Verification Footer Note
    float botY = dy + dh - 46.f;
    drawText(target, fontRegular, "Receipt verified by cashier: " + cashierDisp + "  |  Stored in Order.txt & Bill.txt",
             8.5f, ThemeColor::Mute, dx + 30, botY + 8);

    // Close Button
    bool hovClose = pointInRect(mouse, dx + dw - 146, botY, 120, 32);
    drawRounded(target, dx + dw - 146, botY, 120, 32, 6,
                hovClose ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "Close", 10.5f, ThemeColor::White, dx + dw - 86, botY + 15, TextAlignment::Center);
}

// ─────────────────────────────────────────────────────────────────────────────
// PROMO MANAGEMENT MODAL (Create Promo, Target by Item / Supplier)
// ─────────────────────────────────────────────────────────────────────────────
void PosModalsView::drawPromoManagementModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                             float currentWidth, float designHeight,
                                             const std::vector<PromoRecord>& promoList,
                                             const std::string& newCode,
                                             const std::string& newDiscount,
                                             const std::string& newTargetType,
                                             const std::string& newTargetValue,
                                             int activeField,
                                             const std::vector<std::string>& supplierList,
                                             const sf::Font& fontRegular, const sf::Font& fontMedium,
                                             const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                             const sf::Font& fontSerif) {
    sf::RectangleShape dim({currentWidth, designHeight});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(dim);

    float dw = 780.f, dh = 530.f;
    float dx = (currentWidth - dw) / 2.f, dy = (designHeight - dh) / 2.f;

    // Main Card background
    drawRounded(target, dx, dy, dw, dh, 12, ThemeColor::Panel, ThemeColor::Line, 1.5f);

    // Close [X] Button
    bool hovX = pointInRect(mouse, dx + dw - 40, dy + 16, 26, 26);
    drawRounded(target, dx + dw - 40, dy + 16, 26, 26, 5, hovX ? sf::Color(230, 220, 206) : ThemeColor::None);
    drawText(target, fontBold, "X", 12, ThemeColor::Ink, dx + dw - 27, dy + 28, TextAlignment::Center);

    // Modal Header
    drawText(target, fontSerif, "Promotional Codes & Campaign Management", 19.f, ThemeColor::Ink, dx + 26, dy + 22);
    drawText(target, fontRegular, "Create promo vouchers with storewide, item-specific, or supplier-specific discount rules (promo.txt)",
             9.5f, ThemeColor::Mute, dx + 26, dy + 44);

    // ─────────────────────────────────────────────────────────────
    // LEFT COLUMN: Create / Add New Promo Code
    // ─────────────────────────────────────────────────────────────
    float colW = 340.f;
    float colH = 405.f;
    float colY = dy + 68.f;

    drawRounded(target, dx + 24, colY, colW, colH, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
    drawText(target, fontBold, "Create New Promo Voucher", 12.f, ThemeColor::Ink, dx + 40, colY + 16);
    drawText(target, fontRegular, "Enter code details and target discount rule", 8.5f, ThemeColor::Mute, dx + 40, colY + 31);

    // Input 1: Promo Code
    float fCodeY = colY + 54.f;
    drawText(target, fontSemiBold, "Promo Code (Uppercase):", 8.5f, ThemeColor::Ink, dx + 40, fCodeY);
    drawRounded(target, dx + 40, fCodeY + 14, colW - 32, 32, 6, sf::Color(255, 255, 255),
                (activeField == 0) ? ThemeColor::Blue : ThemeColor::Line, (activeField == 0) ? 1.5f : 1.f);
    std::string dispCode = newCode.empty() ? (activeField == 0 ? "|" : "e.g. WELCOME15, SAVE50")
                                           : (newCode + (activeField == 0 ? "|" : ""));
    drawText(target, fontRegular, dispCode, 9.5f, newCode.empty() ? ThemeColor::Mute : ThemeColor::Ink, dx + 50, fCodeY + 29);

    // Input 2: Discount Rate
    float fRateY = fCodeY + 54.f;
    drawText(target, fontSemiBold, "Discount Value (% or Flat Rs):", 8.5f, ThemeColor::Ink, dx + 40, fRateY);
    drawRounded(target, dx + 40, fRateY + 14, colW - 32, 32, 6, sf::Color(255, 255, 255),
                (activeField == 1) ? ThemeColor::Blue : ThemeColor::Line, (activeField == 1) ? 1.5f : 1.f);
    std::string dispRate = newDiscount.empty() ? (activeField == 1 ? "|" : "e.g. 15% or 50")
                                               : (newDiscount + (activeField == 1 ? "|" : ""));
    drawText(target, fontRegular, dispRate, 9.5f, newDiscount.empty() ? ThemeColor::Mute : ThemeColor::Ink, dx + 50, fRateY + 29);

    // Scope Selection Pills: [ All Items ] [ Specific Item ] [ Supplier Brand ]
    float fScopeY = fRateY + 54.f;
    drawText(target, fontSemiBold, "Target Rule / Scope:", 8.5f, ThemeColor::Ink, dx + 40, fScopeY);

    float pillW = (colW - 36.f) / 3.f;
    float p1X = dx + 40.f;
    float p2X = p1X + pillW + 2.f;
    float p3X = p2X + pillW + 2.f;
    float pillY = fScopeY + 14.f;

    // Scope Pill 1: ALL
    bool isAll = (newTargetType == "ALL");
    bool hovP1 = pointInRect(mouse, p1X, pillY, pillW, 26.f);
    drawRounded(target, p1X, pillY, pillW, 26.f, 5,
                isAll ? ThemeColor::Dark : (hovP1 ? sf::Color(228, 220, 206) : sf::Color(238, 230, 216)), ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "All Items", 8.f, isAll ? ThemeColor::White : ThemeColor::Ink, p1X + pillW / 2.f, pillY + 13.f, TextAlignment::Center);

    // Scope Pill 2: ITEM
    bool isItem = (newTargetType == "ITEM");
    bool hovP2 = pointInRect(mouse, p2X, pillY, pillW, 26.f);
    drawRounded(target, p2X, pillY, pillW, 26.f, 5,
                isItem ? ThemeColor::Blue : (hovP2 ? sf::Color(228, 220, 206) : sf::Color(238, 230, 216)), ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "By Item", 8.f, isItem ? ThemeColor::White : ThemeColor::Ink, p2X + pillW / 2.f, pillY + 13.f, TextAlignment::Center);

    // Scope Pill 3: SUPPLIER
    bool isSupp = (newTargetType == "SUPPLIER");
    bool hovP3 = pointInRect(mouse, p3X, pillY, pillW, 26.f);
    drawRounded(target, p3X, pillY, pillW, 26.f, 5,
                isSupp ? sf::Color(210, 105, 30) : (hovP3 ? sf::Color(228, 220, 206) : sf::Color(238, 230, 216)), ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "By Supplier", 8.f, isSupp ? ThemeColor::White : ThemeColor::Ink, p3X + pillW / 2.f, pillY + 13.f, TextAlignment::Center);

    // Target Value Input (Only visible / relevant if scope != ALL)
    float fTargY = pillY + 34.f;
    std::string targLabel = isItem ? "Target Product Barcode:" : (isSupp ? "Target Supplier Brand:" : "Scope Note:");
    drawText(target, fontSemiBold, targLabel, 8.5f, ThemeColor::Ink, dx + 40, fTargY);

    if (isAll) {
        drawRounded(target, dx + 40, fTargY + 14, colW - 32, 32, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontRegular, "Universal voucher: applies to all store items", 8.5f, ThemeColor::Mute, dx + 50, fTargY + 29);
    } else {
        drawRounded(target, dx + 40, fTargY + 14, colW - 32, 32, 6, sf::Color(255, 255, 255),
                    (activeField == 2) ? ThemeColor::Blue : ThemeColor::Line, (activeField == 2) ? 1.5f : 1.f);
        std::string placeholder = isItem ? "Enter SKU / Barcode (e.g. 100001)" : "Enter vendor (e.g. Nestle Pakistan)";
        std::string dispVal = newTargetValue.empty() ? (activeField == 2 ? "|" : placeholder)
                                                     : (newTargetValue + (activeField == 2 ? "|" : ""));
        drawText(target, fontRegular, dispVal, 9.f, newTargetValue.empty() ? ThemeColor::Mute : ThemeColor::Ink, dx + 50, fTargY + 29);
    }

    // Save Promo Button
    float fSaveBtnY = colY + colH - 46.f;
    bool hovSave = pointInRect(mouse, dx + 40, fSaveBtnY, colW - 32, 34);
    drawRounded(target, dx + 40, fSaveBtnY, colW - 32, 34, 6, hovSave ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontBold, "+ Save & Register Promo Code", 9.5f, ThemeColor::White, dx + 40 + (colW - 32) / 2.f, fSaveBtnY + 17, TextAlignment::Center);

    // ─────────────────────────────────────────────────────────────
    // RIGHT COLUMN: Active Promo Codes Database Table
    // ─────────────────────────────────────────────────────────────
    float rColX = dx + 24 + colW + 14.f;
    float rColW = dw - (rColX - dx) - 24.f;

    drawRounded(target, rColX, colY, rColW, colH, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
    drawText(target, fontBold, "Active Promo Codes (" + std::to_string(promoList.size()) + ")", 12.f, ThemeColor::Ink, rColX + 16, colY + 16);
    drawText(target, fontRegular, "Loaded live from promo.txt database file", 8.5f, ThemeColor::Mute, rColX + 16, colY + 31);

    // Table Header
    float rHdrY = colY + 48.f;
    drawText(target, fontSemiBold, "Promo Code", 8.5f, ThemeColor::Mute, rColX + 16, rHdrY);
    drawText(target, fontSemiBold, "Discount", 8.5f, ThemeColor::Mute, rColX + 115, rHdrY);
    drawText(target, fontSemiBold, "Scope & Target", 8.5f, ThemeColor::Mute, rColX + 185, rHdrY);
    drawText(target, fontSemiBold, "Action", 8.5f, ThemeColor::Mute, rColX + rColW - 16, rHdrY, TextAlignment::Right);

    sf::RectangleShape rSepLine({rColW - 32.f, 1.f});
    rSepLine.setPosition(rColX + 16, rHdrY + 14.f);
    rSepLine.setFillColor(ThemeColor::Line);
    target.draw(rSepLine);

    // Promo list rows
    float rRowY = rHdrY + 22.f;
    if (promoList.empty()) {
        drawText(target, fontRegular, "No promo codes active in promo.txt.\nUse the form on the left to add your first voucher.",
                 9.5f, ThemeColor::Mute, rColX + rColW / 2.f, colY + colH / 2.f, TextAlignment::Center);
    } else {
        int rCount = 0;
        for (const auto& pr : promoList) {
            if (rCount >= 7) break; // Display up to 7 items

            // Code
            drawText(target, fontBold, pr.code, 10.f, ThemeColor::Ink, rColX + 16, rRowY + 3);

            // Discount
            std::string discText = (pr.percent > 0) ? (std::to_string(pr.percent) + "% OFF") : ("Rs " + std::to_string(pr.flat) + " OFF");
            drawText(target, fontSemiBold, discText, 9.f, ThemeColor::Green, rColX + 115, rRowY + 3);

            // Scope & Target
            std::string scopeDisp = pr.targetType;
            if (pr.targetType == "ITEM") {
                scopeDisp = "SKU: " + pr.targetValue;
            } else if (pr.targetType == "SUPPLIER") {
                scopeDisp = "Vendor: " + pr.targetValue;
            } else {
                scopeDisp = "All Items";
            }
            if (scopeDisp.length() > 18) scopeDisp = scopeDisp.substr(0, 16) + "..";
            drawText(target, fontRegular, scopeDisp, 8.5f, ThemeColor::Mute, rColX + 185, rRowY + 4);

            // Delete Button
            float delW = 48.f;
            float delH = 22.f;
            float delX = rColX + rColW - 16.f - delW;
            bool hovDel = pointInRect(mouse, delX, rRowY, delW, delH);
            drawRounded(target, delX, rRowY, delW, delH, 4,
                        hovDel ? sf::Color(254, 226, 226) : sf::Color(245, 238, 230), ThemeColor::Red, 1.f);
            drawText(target, fontSemiBold, "Delete", 8.f, ThemeColor::Red, delX + delW / 2.f, rRowY + 11.f, TextAlignment::Center);

            sf::RectangleShape rowSep({rColW - 32.f, 1.f});
            rowSep.setPosition(rColX + 16, rRowY + 28.f);
            rowSep.setFillColor(sf::Color(240, 235, 226));
            target.draw(rowSep);

            rRowY += 32.f;
            rCount++;
        }
    }

    // Bottom Modal Action Bar
    float botY = dy + dh - 44.f;
    drawText(target, fontRegular, "Vouchers are permanently saved to promo.txt upon saving or deletion.",
             8.5f, ThemeColor::Mute, dx + 26, botY + 8);

    bool hovClose = pointInRect(mouse, dx + dw - 146, botY, 120, 32);
    drawRounded(target, dx + dw - 146, botY, 120, 32, 6,
                hovClose ? sf::Color(228, 218, 204) : sf::Color(235, 226, 212), ThemeColor::Edge, 1);
    drawText(target, fontSemiBold, "Close", 10.5f, ThemeColor::Ink, dx + dw - 86, botY + 15, TextAlignment::Center);
}

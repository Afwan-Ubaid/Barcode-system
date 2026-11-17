#include "PosReceiptView.h"
#include "UIUtils.h"
#include <algorithm>

void PosReceiptView::drawReceiptDrawer(sf::RenderTarget& target, sf::Vector2f m,
                                      float panelX, float rpX, float designHeight,
                                      const std::vector<CartLine>& cart,
                                      const std::vector<ProductItem>& products,
                                      int subtotal, int discount, int total,
                                      bool discountApplied, int paymentMethod, bool paymentDone,
                                      float receiptScrollOffset,
                                      const sf::Font& fontRegular, const sf::Font& fontMedium,
                                      const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                      const sf::Font& fontSerif,
                                      std::function<const sf::Texture&(const std::string&)> getTexture) {
    // Right panel background
    sf::RectangleShape pbg({Layout::PANEL_W, designHeight});
    pbg.setPosition(panelX, 0);
    pbg.setFillColor(ThemeColor::Panel);
    target.draw(pbg);

    sf::RectangleShape div({1.f, designHeight});
    div.setPosition(panelX, 0);
    div.setFillColor(ThemeColor::Line);
    target.draw(div);

    // Header "Current Sale"
    drawText(target, fontSerif, "Current Sale", 22, ThemeColor::Ink, rpX, 36);

    // Receipt Paper with authentic zigzag perforated bottom edge
    drawShadow(target, rpX, Layout::RP_Y, Layout::RP_W, Layout::RP_H, 6, 5, 7, 10, 3);
    drawRounded(target, rpX, Layout::RP_Y, Layout::RP_W, Layout::RP_H, 6, ThemeColor::Paper, sf::Color(228, 218, 200), 1);

    // Zigzag serrated edge
    for (float x = rpX; x < rpX + Layout::RP_W - 1; x += 10) {
        sf::ConvexShape tri(3);
        float w = std::min(10.f, rpX + Layout::RP_W - x);
        tri.setPoint(0, {x, Layout::RP_Y + Layout::RP_H - 2});
        tri.setPoint(1, {x + w, Layout::RP_Y + Layout::RP_H - 2});
        tri.setPoint(2, {x + w / 2, Layout::RP_Y + Layout::RP_H + 6});
        tri.setFillColor(ThemeColor::Paper);
        target.draw(tri);
    }

    // Receipt Drawer title & Enlarge Cart Button
    drawText(target, fontSerif, "Receipt drawer", 13.5f, ThemeColor::Ink, rpX + 12.f, Layout::RP_Y + 18.f);

    float enlW = 66.f, enlH = 22.f;
    float enlX = rpX + Layout::RP_W - enlW - 8.f;
    float enlY = Layout::RP_Y + 7.f;
    bool hovEnlarge = pointInRect(m, enlX, enlY, enlW, enlH);
    drawRounded(target, enlX, enlY, enlW, enlH, 4,
                hovEnlarge ? sf::Color(230, 220, 204) : sf::Color(242, 234, 220), ThemeColor::Edge, 1);
    drawSprite(target, getTexture("icons/scan.png"), enlX + 7.f, enlY + 5.f, 12, 12, ThemeColor::Ink);
    drawText(target, fontSemiBold, "Enlarge", 8.5f, ThemeColor::Ink, enlX + 38.f, enlY + 11.f, TextAlignment::Center);

    // Column table header
    sf::RectangleShape hline1({Layout::RP_W - 16, 1});
    hline1.setPosition(rpX + 8, Layout::RP_Y + 34.f);
    hline1.setFillColor(ThemeColor::Line);
    target.draw(hline1);

    drawText(target, fontMedium, "Purchases", 9, ThemeColor::Mute, rpX + 10, Layout::RP_Y + 45.f);
    drawText(target, fontMedium, "QTY", 9, ThemeColor::Mute, rpX + 152, Layout::RP_Y + 45.f, TextAlignment::Center);
    drawText(target, fontMedium, "PRICE", 9, ThemeColor::Mute, rpX + Layout::RP_W - 10, Layout::RP_Y + 45.f, TextAlignment::Right);

    sf::RectangleShape hline2({Layout::RP_W - 16, 1});
    hline2.setPosition(rpX + 8, Layout::RP_Y + 56.f);
    hline2.setFillColor(ThemeColor::Line);
    target.draw(hline2);

    if (cart.empty()) {
        drawText(target, fontRegular, "Receipt empty - scan barcode", 11, ThemeColor::Mute, rpX + Layout::RP_W / 2.f, 220, TextAlignment::Center);
    }

    float listH = Layout::RP_H - 74.f;
    float maxScroll = std::max(0.f, (float)(cart.size() - 5) * Layout::ROW_H);
    float clampedOffset = std::min(receiptScrollOffset, maxScroll);

    for (int i = 0; i < (int)cart.size(); i++) {
        const auto& item = cart[i];
        if (item.productId < 0 || item.productId >= (int)products.size()) continue;
        const auto& p = products[item.productId];
        float y0 = Layout::ROW_Y + i * Layout::ROW_H - clampedOffset;

        if (y0 < Layout::ROW_Y - 5.f || y0 > Layout::RP_Y + Layout::RP_H - 24.f) continue;

        // 1. Product thumbnail
        drawRounded(target, rpX + 8, y0 + 4, 24, 24, 5, p.cardTint);
        drawSprite(target, getTexture("products/" + p.iconKey + ".png"), rpX + 11, y0 + 7, 18, 18);

        // 2. Delete button below product icon
        float delX = rpX + 7.f;
        float delY = y0 + 30.f;
        float delW = 26.f;
        float delH = 13.f;
        bool hovDel = pointInRect(m, delX, delY, delW, delH);
        drawRounded(target, delX, delY, delW, delH, 3,
                    hovDel ? sf::Color(254, 226, 226) : sf::Color(246, 240, 234),
                    hovDel ? ThemeColor::Red : ThemeColor::Edge, 1);
        drawSprite(target, getTexture("icons/trash.png"), delX + 2, delY + 2.f, 9, 9,
                   hovDel ? ThemeColor::Red : ThemeColor::Mute);
        drawText(target, fontSemiBold, "Del", 7.5f,
                 hovDel ? ThemeColor::Red : ThemeColor::Mute, delX + 12, delY + 6.5f);

        // 3. Name & Unit
        std::string shortName = p.name;
        if (shortName.length() > 14) shortName = shortName.substr(0, 12) + "..";
        drawText(target, fontSemiBold, shortName, 10.5f, ThemeColor::Ink, rpX + 38, y0 + 9);
        drawText(target, fontRegular, p.unit + " | " + formatRs(p.price), 8, ThemeColor::Mute, rpX + 38, y0 + 22);

        // 4. Quantity Stepper
        bool hovMinus = pointInRect(m, rpX + 126, y0 + 11, 16, 16);
        drawRounded(target, rpX + 126, y0 + 11, 16, 16, 4,
                    hovMinus ? sf::Color(236, 228, 212) : ThemeColor::Paper, ThemeColor::Edge, 1);
        sf::RectangleShape mline({6, 1.2f});
        mline.setPosition(rpX + 131.f, y0 + 18.4f);
        mline.setFillColor(ThemeColor::Ink);
        target.draw(mline);

        bool hovQty = pointInRect(m, rpX + 144, y0 + 11, 17, 16);
        drawRounded(target, rpX + 144, y0 + 11, 17, 16, 4,
                    hovQty ? sf::Color(236, 228, 212) : ThemeColor::Paper,
                    hovQty ? ThemeColor::Ink : ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, std::to_string(item.quantity), 9.5f, ThemeColor::Ink, rpX + 152.5f, y0 + 19, TextAlignment::Center);

        bool hovPlus = pointInRect(m, rpX + 163, y0 + 11, 16, 16);
        drawRounded(target, rpX + 163, y0 + 11, 16, 16, 4,
                    hovPlus ? sf::Color(236, 228, 212) : ThemeColor::Paper, ThemeColor::Edge, 1);
        sf::RectangleShape pline1({6, 1.2f});
        pline1.setPosition(rpX + 168.f, y0 + 18.4f);
        pline1.setFillColor(ThemeColor::Ink);
        target.draw(pline1);
        sf::RectangleShape pline2({1.2f, 6});
        pline2.setPosition(rpX + 170.4f, y0 + 16.f);
        pline2.setFillColor(ThemeColor::Ink);
        target.draw(pline2);

        // 5. Line item total price
        drawText(target, fontBold, formatRs(p.price * item.quantity), 10.5f, ThemeColor::Ink, rpX + Layout::RP_W - 8, y0 + 19, TextAlignment::Right);

        // Row separator
        sf::RectangleShape rline({Layout::RP_W - 18, 1});
        rline.setPosition(rpX + 8, y0 + Layout::ROW_H - 1);
        rline.setFillColor(sf::Color(235, 227, 211));
        target.draw(rline);
    }

    // Scrollbar track & thumb
    if (cart.size() > 5) {
        float trackX = rpX + Layout::RP_W - 6.f;
        float trackY = Layout::ROW_Y + 4.f;
        float trackH = listH - 16.f;
        drawRounded(target, trackX, trackY, 4.f, trackH, 2.f, sf::Color(228, 220, 206));

        float thumbRatio = 5.f / (float)cart.size();
        float thumbH = std::max(24.f, trackH * thumbRatio);
        float thumbY = trackY + (clampedOffset / maxScroll) * (trackH - thumbH);
        drawRounded(target, trackX - 0.5f, thumbY, 5.f, thumbH, 2.5f, ThemeColor::Mute);
    }

    // Applied Discounts Chip
    float chipY = Layout::RP_Y + Layout::RP_H + 8.f;
    bool hovChip = pointInRect(m, rpX, chipY, 210.f, 22.f);
    drawRounded(target, rpX, chipY, 210.f, 22.f, 6,
                hovChip ? sf::Color(236, 227, 208) : sf::Color(238, 230, 213), ThemeColor::Edge, 1);
    drawSprite(target, getTexture("icons/tag.png"), rpX + 8, chipY + 5, 14, 14, ThemeColor::Ink);
    std::string chipText = discountApplied ? "Applied Discounts (- Rs 120)" : "Apply discount (- Rs 120)";
    drawText(target, fontMedium, chipText, 9, ThemeColor::Ink, rpX + 28, chipY + 12);

    // Totals Section
    auto drawSummaryRow = [&](const char* label, const std::string& val, float y) {
        drawText(target, fontRegular, label, 10, ThemeColor::Mute, rpX, y);
        drawText(target, fontMedium, val, 10, ThemeColor::Ink, rpX + Layout::RP_W, y, TextAlignment::Right);
    };

    drawSummaryRow("Subtotal", formatRs(subtotal), chipY + 32.f);
    drawSummaryRow("Discount", discount ? formatRs(-discount) : "Rs 0", chipY + 48.f);

    sf::RectangleShape tline({Layout::RP_W, 1});
    tline.setPosition(rpX, chipY + 62.f);
    tline.setFillColor(ThemeColor::Line);
    target.draw(tline);

    drawText(target, fontSemiBold, "TOTAL:", 14, ThemeColor::Ink, rpX, chipY + 76.f);
    drawText(target, fontBold, formatRs(total), 16, ThemeColor::Ink, rpX + Layout::RP_W, chipY + 76.f, TextAlignment::Right);

    // Cash and Card Selection Buttons
    float btnPayY = chipY + 98.f;
    bool selCash = (paymentMethod == 0);
    bool hovCash = pointInRect(m, rpX, btnPayY, 110.f, 30.f);
    drawRounded(target, rpX, btnPayY, 110.f, 30.f, 6,
                hovCash ? sf::Color(255, 253, 248) : ThemeColor::Paper,
                selCash ? ThemeColor::Ink : ThemeColor::Edge, selCash ? 1.5f : 1.f);
    drawSprite(target, getTexture("icons/cash.png"), rpX + 22, btnPayY + 8, 14, 14, ThemeColor::Ink);
    drawText(target, fontSemiBold, "Cash", 11, ThemeColor::Ink, rpX + 44, btnPayY + 15);

    bool selCard = (paymentMethod == 1);
    bool hovCard = pointInRect(m, rpX + 122.f, btnPayY, 110.f, 30.f);
    drawRounded(target, rpX + 122.f, btnPayY, 110.f, 30.f, 6,
                hovCard ? sf::Color(255, 253, 248) : ThemeColor::Paper,
                selCard ? ThemeColor::Ink : ThemeColor::Edge, selCard ? 1.5f : 1.f);
    drawSprite(target, getTexture("icons/card.png"), rpX + 144, btnPayY + 8, 14, 14, ThemeColor::Ink);
    drawText(target, fontSemiBold, "Card", 11, ThemeColor::Ink, rpX + 166, btnPayY + 15);

    // Charge / Pay Button
    float chargeY = btnPayY + 40.f;
    bool canCharge = !cart.empty();
    bool hovCharge = canCharge && pointInRect(m, rpX, chargeY, Layout::RP_W, 38.f);

    if (paymentDone) {
        drawRounded(target, rpX, chargeY, Layout::RP_W, 38.f, 7,
                    sf::Color(220, 252, 231), sf::Color(110, 168, 110), 1.2f);
        drawSprite(target, getTexture("icons/check.png"), rpX + 12, chargeY + 11, 16, 16, sf::Color(22, 101, 52));
        drawText(target, fontSemiBold, "Paid (" + std::string(paymentMethod == 0 ? "Cash" : "Card") + ")",
                 10.5f, sf::Color(22, 101, 52), rpX + 34, chargeY + 19);
        drawText(target, fontBold, formatRs(total), 11, sf::Color(22, 101, 52),
                 rpX + Layout::RP_W - 12, chargeY + 19, TextAlignment::Right);
    } else {
        sf::Color chargeColor = !canCharge ? ThemeColor::BlueDisabled : (hovCharge ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawRounded(target, rpX, chargeY, Layout::RP_W, 38.f, 7, chargeColor);
        std::string chargeLbl = (paymentMethod == 0 ? "Pay Cash: " : "Charge Card: ") + formatRs(total);
        drawText(target, fontSemiBold, chargeLbl, 12.f, ThemeColor::White, rpX + Layout::RP_W / 2.f, chargeY + 19, TextAlignment::Center);
    }

    // Print Receipt Button
    float printY = chargeY + 44.f;
    bool hovPrint = paymentDone && pointInRect(m, rpX, printY, Layout::RP_W, 38.f);

    if (paymentDone) {
        drawRounded(target, rpX, printY, Layout::RP_W, 38.f, 7,
                    hovPrint ? sf::Color(35, 75, 50) : sf::Color(44, 95, 63));
        drawSprite(target, getTexture("icons/print.png"), rpX + 70, printY + 10, 18, 18, ThemeColor::White);
        drawText(target, fontSemiBold, "Print Receipt", 12.5f, ThemeColor::White, rpX + 98, printY + 19);
    } else {
        drawRounded(target, rpX, printY, Layout::RP_W, 38.f, 7,
                    sf::Color(232, 226, 214), ThemeColor::Edge, 1);
        drawSprite(target, getTexture("icons/print.png"), rpX + 50, printY + 10, 18, 18, ThemeColor::Mute);
        drawText(target, fontMedium, "Print Receipt (Pay First)", 11, ThemeColor::Mute, rpX + 78, printY + 19);
    }

    // New Sale Reset Button
    if (paymentDone) {
        float newSaleY = printY + 44.f;
        bool hovNew = pointInRect(m, rpX, newSaleY, Layout::RP_W, 32.f);
        drawRounded(target, rpX, newSaleY, Layout::RP_W, 32.f, 6,
                    hovNew ? sf::Color(255, 253, 248) : ThemeColor::Paper, ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, "+ Start New Sale", 11, ThemeColor::Ink,
                 rpX + Layout::RP_W / 2.f, newSaleY + 16, TextAlignment::Center);
    }
}

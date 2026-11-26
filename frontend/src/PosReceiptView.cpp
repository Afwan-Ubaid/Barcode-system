#include "PosReceiptView.h"
#include "UIUtils.h"
#include <algorithm>

void PosReceiptView::drawReceiptDrawer(sf::RenderTarget& target, sf::Vector2f m,
                                      float panelX, float rpX, float designHeight,
                                      const std::vector<CartLine>& cart,
                                      const std::vector<ProductItem>& products,
                                      const BillSummary& summary,
                                      bool discountApplied, const std::string& discountLabel,
                                      int paymentMethod, bool paymentDone,
                                      float receiptScrollOffset,
                                      int draftsCount,
                                      bool isEditingExistingOrder,
                                      const std::string& editingOrderId,
                                      const std::string& customerName,
                                      int customerPoints,
                                      bool redeemPoints,
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
    drawText(target, fontSerif, "Current Sale", 18, ThemeColor::Ink, rpX, 22);

    // Drafts / Parked Carts Button (Badge if drafts exist)
    float draftW = 76.f, draftH = 22.f;
    float draftX = rpX + Layout::RP_W - draftW;
    float draftY = 11.f;
    bool hovDraft = pointInRect(m, draftX, draftY, draftW, draftH);
    sf::Color draftBg = draftsCount > 0 ? (hovDraft ? sf::Color(245, 158, 11) : sf::Color(251, 191, 36))
                                       : (hovDraft ? sf::Color(230, 222, 210) : sf::Color(242, 236, 226));
    drawRounded(target, draftX, draftY, draftW, draftH, 4, draftBg, ThemeColor::Edge, 1.f);
    std::string draftLabel = "Drafts (" + std::to_string(draftsCount) + ")";
    drawText(target, fontSemiBold, draftLabel, 8.5f, draftsCount > 0 ? ThemeColor::Ink : ThemeColor::Mute,
             draftX + draftW / 2.f, draftY + 11.f, TextAlignment::Center);

    // 4 Action Buttons spanning exact Layout::RP_W = 302.f (68 + 6 + 68 + 6 + 72 + 6 + 76 = 302)
    float actionY = 38.f;
    float actionH = 22.f;

    // 1. Hold Cart (1-click hold; dimmed when order is already paid)
    float holdX = rpX, holdW = 68.f;
    bool hovHold = !paymentDone && pointInRect(m, holdX, actionY, holdW, actionH);
    drawRounded(target, holdX, actionY, holdW, actionH, 4,
                paymentDone ? sf::Color(242, 238, 232) : (hovHold ? sf::Color(254, 243, 199) : sf::Color(245, 240, 230)),
                hovHold ? ThemeColor::Amber : ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "Hold Cart", 8.5f,
             paymentDone ? sf::Color(180, 175, 165) : (hovHold ? sf::Color(180, 83, 9) : ThemeColor::Ink),
             holdX + holdW / 2.f, actionY + 11.f, TextAlignment::Center);

    // 2. Clear Cart (1-click clear)
    float clrX = rpX + 74.f, clrW = 68.f;
    bool hovClr = pointInRect(m, clrX, actionY, clrW, actionH);
    drawRounded(target, clrX, actionY, clrW, actionH, 4,
                hovClr ? sf::Color(254, 226, 226) : sf::Color(245, 240, 230),
                hovClr ? ThemeColor::Red : ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "Clear Cart", 8.5f,
             hovClr ? ThemeColor::Red : ThemeColor::Ink,
             clrX + clrW / 2.f, actionY + 11.f, TextAlignment::Center);

    // 3. Edit Order (Open & Edit existing order)
    float editX = rpX + 148.f, editW = 72.f;
    bool hovEdit = pointInRect(m, editX, actionY, editW, actionH);
    drawRounded(target, editX, actionY, editW, actionH, 4,
                hovEdit ? sf::Color(220, 235, 250) : sf::Color(245, 240, 230),
                hovEdit ? ThemeColor::Blue : ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, isEditingExistingOrder ? "Editing" : "Edit Order", 8.5f,
             hovEdit ? ThemeColor::Blue : (isEditingExistingOrder ? sf::Color(180, 50, 40) : ThemeColor::Ink),
             editX + editW / 2.f, actionY + 11.f, TextAlignment::Center);

    // 4. Resume Held Order (1-click resume for Requirement 10)
    float resX = rpX + 226.f, resW = 76.f;
    bool hasDrafts = (draftsCount > 0);
    bool hovRes = hasDrafts && pointInRect(m, resX, actionY, resW, actionH);
    sf::Color resBg = hasDrafts
        ? (hovRes ? sf::Color(205, 230, 212) : sf::Color(226, 244, 230))
        : sf::Color(242, 237, 228);
    drawRounded(target, resX, actionY, resW, actionH, 4, resBg,
                hasDrafts ? ThemeColor::Green : ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "Resume", 8.5f,
             hasDrafts ? sf::Color(22, 101, 52) : ThemeColor::Mute,
             resX + resW / 2.f, actionY + 11.f, TextAlignment::Center);

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

    // Receipt Drawer title (or Editing badge cleanly inside header) & Enlarge Cart Button
    if (isEditingExistingOrder) {
        drawRounded(target, rpX + 8.f, Layout::RP_Y + 7.f, 204.f, 22.f, 4,
                    sf::Color(254, 243, 199), sf::Color(217, 119, 6), 1.f);
        drawText(target, fontBold, "Editing Order #" + editingOrderId, 9.f,
                 sf::Color(180, 83, 9), rpX + 16.f, Layout::RP_Y + 18.f);
    } else {
        drawText(target, fontSerif, "Receipt drawer", 13.5f, ThemeColor::Ink, rpX + 12.f, Layout::RP_Y + 18.f);
    }

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
    drawText(target, fontMedium, "QTY", 9, ThemeColor::Mute, rpX + 172.f, Layout::RP_Y + 45.f, TextAlignment::Center);
    drawText(target, fontMedium, "PRICE", 9, ThemeColor::Mute, rpX + 258.f, Layout::RP_Y + 45.f, TextAlignment::Right);

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

        // 1. Item Thumbnail Icon (26x26)
        float iconX = rpX + 8.f;
        float iconY = y0 + 11.f;
        drawRounded(target, iconX, iconY, 26.f, 26.f, 5.f, sf::Color(246, 240, 230), ThemeColor::Line, 1.f);
        drawSprite(target, getTexture("products/" + p.iconKey + ".png"), iconX + 3.f, iconY + 3.f, 20.f, 20.f);

        // 2. Product Name & Unit Price
        float textX = rpX + 38.f;
        std::string shortName = p.name;
        if (shortName.length() > 14) shortName = shortName.substr(0, 12) + "..";
        drawText(target, fontSemiBold, shortName, 10.5f, ThemeColor::Ink, textX, y0 + 14.f);

        std::string priceUnit = p.unit + " @ Rs " + std::to_string(p.price);
        if (priceUnit.length() > 16) priceUnit = "Rs " + std::to_string(p.price);
        drawText(target, fontRegular, priceUnit, 8.5f, ThemeColor::Mute, textX, y0 + 31.f);

        // 3. Info/spec icon badge [i] clamped strictly before minus button (rpX + 144)
        float unitW = getTextWidth(fontRegular, priceUnit, 8.5f);
        float infoX = std::min(rpX + 126.f, textX + unitW + 5.f);
        float infoY = y0 + 24.f;
        bool hovInfo = pointInRect(m, infoX, infoY, 14.f, 14.f);
        drawRounded(target, infoX, infoY, 14.f, 14.f, 7.f,
                    hovInfo ? ThemeColor::Blue : sf::Color(242, 236, 226),
                    hovInfo ? ThemeColor::Blue : ThemeColor::Edge, 1.f);
        drawText(target, fontSemiBold, "i", 8.f,
                 hovInfo ? ThemeColor::White : ThemeColor::Mute, infoX + 7.f, infoY + 7.f, TextAlignment::Center);

        // 4. Quantity Stepper (dimmed & locked when paymentDone)
        sf::Color stepBg = paymentDone ? sf::Color(242, 238, 232) : ThemeColor::Paper;
        sf::Color stepText = paymentDone ? sf::Color(180, 175, 165) : ThemeColor::Ink;
        float minusX = rpX + 144.f;
        bool hovMinus = !paymentDone && pointInRect(m, minusX, y0 + 15.f, 16.f, 18.f);
        drawRounded(target, minusX, y0 + 15.f, 16.f, 18.f, 4,
                    hovMinus ? sf::Color(236, 228, 212) : stepBg, ThemeColor::Edge, 1);
        drawText(target, fontBold, "-", 11.f, stepText, minusX + 8.f, y0 + 24.f, TextAlignment::Center);

        float qtyX = rpX + 163.f;
        drawRounded(target, qtyX, y0 + 15.f, 18.f, 18.f, 4,
                    stepBg, ThemeColor::Edge, 1);
        drawText(target, fontBold, std::to_string(item.quantity), 9.5f, stepText, qtyX + 9.f, y0 + 24.f, TextAlignment::Center);

        float plusX = rpX + 184.f;
        bool hovPlus = !paymentDone && pointInRect(m, plusX, y0 + 15.f, 16.f, 18.f);
        drawRounded(target, plusX, y0 + 15.f, 16.f, 18.f, 4,
                    hovPlus ? sf::Color(236, 228, 212) : stepBg, ThemeColor::Edge, 1);
        drawText(target, fontBold, "+", 11.f, stepText, plusX + 8.f, y0 + 24.f, TextAlignment::Center);

        // 5. Line item total price (Right aligned in price column)
        drawText(target, fontBold, formatRs(p.price * item.quantity), 10.5f, ThemeColor::Ink, rpX + 258.f, y0 + 24.f, TextAlignment::Right);

        // 6. Delete button on far right (Trashcan icon button, dimmed when paymentDone)
        float delX = rpX + 270.f;
        float delY = y0 + 14.f;
        float delW = 22.f;
        float delH = 22.f;
        bool hovDel = !paymentDone && pointInRect(m, delX, delY, delW, delH);
        drawRounded(target, delX, delY, delW, delH, 4,
                    paymentDone ? sf::Color(242, 238, 232) : (hovDel ? sf::Color(254, 226, 226) : sf::Color(246, 240, 232)),
                    hovDel ? ThemeColor::Red : ThemeColor::Line, 1);
        drawSprite(target, getTexture("icons/trash.png"), delX + 5.f, delY + 4.5f, 12, 13,
                   paymentDone ? sf::Color(185, 180, 170) : (hovDel ? ThemeColor::Red : ThemeColor::Mute));

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

    // Option A: Customer Loyalty Chip (above Apply discount)
    float custY = Layout::RP_Y + Layout::RP_H + 6.f;
    bool hasCust = !customerName.empty() && customerName != "Walk-in Customer";
    bool hovCust = !paymentDone && pointInRect(m, rpX, custY, Layout::RP_W, 22.f);
    sf::Color custBg = paymentDone ? sf::Color(242, 238, 232)
                                   : (hasCust ? sf::Color(232, 245, 236) : (hovCust ? sf::Color(236, 227, 208) : sf::Color(238, 230, 213)));
    drawRounded(target, rpX, custY, Layout::RP_W, 22.f, 6, custBg,
                hasCust && !paymentDone ? ThemeColor::Green : ThemeColor::Edge, 1.f);
    drawSprite(target, getTexture("icons/user.png"), rpX + 7, custY + 4, 14, 14,
               paymentDone ? ThemeColor::Mute : (hasCust ? ThemeColor::Green : ThemeColor::Ink));
    std::string custLabel = hasCust
        ? (customerName + " (" + std::to_string(customerPoints) + " pts" + (redeemPoints ? ", Redeeming)" : ")"))
        : "Walk-in Customer (Click or F10 for Loyalty)";
    if (custLabel.length() > 42) custLabel = custLabel.substr(0, 40) + "..";
    drawText(target, fontMedium, custLabel, 8.3f,
             paymentDone ? ThemeColor::Mute : (hasCust ? sf::Color(22, 101, 52) : ThemeColor::Ink),
             rpX + 26, custY + 11.f);

    // Applied Discounts Chip (206w) + Active Seasonal Badge (90w) spanning exact 302px
    float chipY = custY + 26.f;
    bool hovChip = !paymentDone && pointInRect(m, rpX, chipY, 206.f, 22.f);
    drawRounded(target, rpX, chipY, 206.f, 22.f, 6,
                paymentDone ? sf::Color(242, 238, 232) : (hovChip ? sf::Color(236, 227, 208) : sf::Color(238, 230, 213)),
                ThemeColor::Edge, 1);
    drawSprite(target, getTexture("icons/tag.png"), rpX + 7, chipY + 4, 14, 14, paymentDone ? ThemeColor::Mute : ThemeColor::Ink);
    std::string chipText = discountApplied ? ("Disc: " + discountLabel) : "Apply discount (Click)...";
    if (chipText.length() > 26) chipText = chipText.substr(0, 24) + "..";
    drawText(target, fontMedium, chipText, 8.5f, paymentDone ? ThemeColor::Mute : ThemeColor::Ink, rpX + 26, chipY + 11.f);

    bool hasSeason = (summary.seasonalPercent > 0);
    drawRounded(target, rpX + 212.f, chipY, 90.f, 22.f, 6,
                hasSeason ? sf::Color(220, 252, 231) : sf::Color(238, 230, 213),
                hasSeason ? ThemeColor::Green : ThemeColor::Edge, 1.f);
    std::string seaTxt = hasSeason ? ("Season -" + std::to_string(summary.seasonalPercent) + "%") : "No Season";
    drawText(target, fontSemiBold, seaTxt, 8.f,
             hasSeason ? sf::Color(22, 101, 52) : ThemeColor::Mute,
             rpX + 212.f + 45.f, chipY + 11.f, TextAlignment::Center);

    // Totals Section
    auto drawSummaryRow = [&](const std::string& label, const std::string& val, float y, sf::Color valCol = ThemeColor::Ink) {
        drawText(target, fontRegular, label, 9.2f, ThemeColor::Mute, rpX, y);
        drawText(target, fontMedium, val, 9.2f, valCol, rpX + Layout::RP_W, y, TextAlignment::Right);
    };

    int totalDisc = summary.totalApplicableDiscount + summary.seasonalDiscount + summary.specialDiscount + summary.loyaltyDiscount;
    drawSummaryRow("Subtotal (No Tax)", formatRs(summary.subtotalWithoutTax), chipY + 26.f);
    drawSummaryRow("Tax", formatRs(summary.totalTax), chipY + 38.f);
    std::string discLbl = summary.loyaltyDiscount > 0
        ? "Discounts (incl. Loyalty)"
        : (summary.seasonalDiscount > 0
            ? ("Discounts (incl. " + std::to_string(summary.seasonalPercent) + "% Season)")
            : "Discounts");
    drawSummaryRow(discLbl, totalDisc > 0 ? formatRs(-totalDisc) : "Rs 0", chipY + 50.f,
                   totalDisc > 0 ? sf::Color(185, 28, 28) : ThemeColor::Ink);

    sf::RectangleShape tline({Layout::RP_W, 1});
    tline.setPosition(rpX, chipY + 60.f);
    tline.setFillColor(ThemeColor::Line);
    target.draw(tline);

    drawText(target, fontSemiBold, "TOTAL:", 13.5f, ThemeColor::Ink, rpX, chipY + 72.f);
    drawText(target, fontBold, formatRs(summary.totalPayable), 15.5f, ThemeColor::Ink, rpX + Layout::RP_W, chipY + 72.f, TextAlignment::Right);

    // Cash, Card, and Split Selection Buttons (3 * 96 + 2 * 7 = 302px; dimmed when paymentDone)
    float btnPayY = chipY + 89.f;
    float payBtnW = 96.f;
    float payGap = 7.f;
    sf::Color payDimBg(242, 238, 232);
    sf::Color payDimFg(175, 170, 160);

    bool selCash = (paymentMethod == 0);
    bool hovCash = !paymentDone && pointInRect(m, rpX, btnPayY, payBtnW, 28.f);
    drawRounded(target, rpX, btnPayY, payBtnW, 28.f, 6,
                paymentDone ? payDimBg : (hovCash ? sf::Color(255, 253, 248) : ThemeColor::Paper),
                paymentDone ? ThemeColor::Line : (selCash ? ThemeColor::Ink : ThemeColor::Edge),
                (!paymentDone && selCash) ? 1.5f : 1.f);
    drawSprite(target, getTexture("icons/cash.png"), rpX + 16.f, btnPayY + 7.f, 14, 14, paymentDone ? payDimFg : ThemeColor::Ink);
    drawText(target, fontSemiBold, "Cash", 10.f, paymentDone ? payDimFg : ThemeColor::Ink, rpX + 36.f, btnPayY + 14.f);

    bool selCard = (paymentMethod == 1);
    float cardX = rpX + payBtnW + payGap;
    bool hovCard = !paymentDone && pointInRect(m, cardX, btnPayY, payBtnW, 28.f);
    drawRounded(target, cardX, btnPayY, payBtnW, 28.f, 6,
                paymentDone ? payDimBg : (hovCard ? sf::Color(255, 253, 248) : ThemeColor::Paper),
                paymentDone ? ThemeColor::Line : (selCard ? ThemeColor::Ink : ThemeColor::Edge),
                (!paymentDone && selCard) ? 1.5f : 1.f);
    drawSprite(target, getTexture("icons/card.png"), cardX + 16.f, btnPayY + 7.f, 14, 14, paymentDone ? payDimFg : ThemeColor::Ink);
    drawText(target, fontSemiBold, "Card", 10.f, paymentDone ? payDimFg : ThemeColor::Ink, cardX + 36.f, btnPayY + 14.f);

    bool selSplit = (paymentMethod == 2);
    float splitX = cardX + payBtnW + payGap;
    bool hovSplit = !paymentDone && pointInRect(m, splitX, btnPayY, payBtnW, 28.f);
    drawRounded(target, splitX, btnPayY, payBtnW, 28.f, 6,
                paymentDone ? payDimBg : (hovSplit ? sf::Color(255, 253, 248) : ThemeColor::Paper),
                paymentDone ? ThemeColor::Line : (selSplit ? ThemeColor::Ink : ThemeColor::Edge),
                (!paymentDone && selSplit) ? 1.5f : 1.f);
    drawSprite(target, getTexture("icons/card.png"), splitX + 14.f, btnPayY + 7.f, 14, 14, paymentDone ? payDimFg : ThemeColor::Ink);
    drawText(target, fontSemiBold, "Split", 10.f, paymentDone ? payDimFg : ThemeColor::Ink, splitX + 34.f, btnPayY + 14.f);

    // Charge / Pay Button
    float chargeY = btnPayY + 34.f;
    bool canCharge = !cart.empty();
    bool hovCharge = !paymentDone && canCharge && pointInRect(m, rpX, chargeY, Layout::RP_W, 36.f);

    if (paymentDone) {
        drawRounded(target, rpX, chargeY, Layout::RP_W, 36.f, 7,
                    sf::Color(220, 252, 231), sf::Color(110, 168, 110), 1.2f);
        drawSprite(target, getTexture("icons/check.png"), rpX + 12, chargeY + 10, 16, 16, sf::Color(22, 101, 52));
        std::string paidMethodStr = (paymentMethod == 0 ? "Cash" : (paymentMethod == 1 ? "Card" : "Split"));
        drawText(target, fontSemiBold, "Paid (" + paidMethodStr + ") - Locked",
                 10.f, sf::Color(22, 101, 52), rpX + 34, chargeY + 18);
        drawText(target, fontBold, formatRs(summary.totalPayable), 11, sf::Color(22, 101, 52),
                 rpX + Layout::RP_W - 12, chargeY + 18, TextAlignment::Right);
    } else {
        sf::Color chargeColor = !canCharge ? ThemeColor::BlueDisabled : (hovCharge ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawRounded(target, rpX, chargeY, Layout::RP_W, 36.f, 7, chargeColor);
        std::string chargeLbl = (paymentMethod == 0 ? "Pay Cash: " : (paymentMethod == 1 ? "Charge Card: " : "Split Pay: ")) + formatRs(summary.totalPayable);
        drawText(target, fontSemiBold, chargeLbl, 11.5f, ThemeColor::White, rpX + Layout::RP_W / 2.f, chargeY + 18, TextAlignment::Center);
    }

    // Print Receipt & Print PDF Buttons
    float printY = chargeY + 41.f;
    if (paymentDone) {
        float recW = 192.f;
        float pdfW = 102.f;
        bool hovPrint = pointInRect(m, rpX, printY, recW, 36.f);
        drawRounded(target, rpX, printY, recW, 36.f, 7,
                    hovPrint ? sf::Color(35, 75, 50) : sf::Color(44, 95, 63));
        drawSprite(target, getTexture("icons/print.png"), rpX + 20, printY + 9, 18, 18, ThemeColor::White);
        drawText(target, fontSemiBold, "Print Receipt", 11.5f, ThemeColor::White, rpX + 46, printY + 18);

        bool hovPdf = pointInRect(m, rpX + 200.f, printY, pdfW, 36.f);
        drawRounded(target, rpX + 200.f, printY, pdfW, 36.f, 7,
                    hovPdf ? sf::Color(29, 78, 216) : sf::Color(37, 99, 235));
        drawText(target, fontBold, "Print PDF", 11.f, ThemeColor::White, rpX + 200.f + pdfW / 2.f, printY + 18, TextAlignment::Center);
    } else {
        drawRounded(target, rpX, printY, Layout::RP_W, 36.f, 7,
                    sf::Color(232, 226, 214), ThemeColor::Edge, 1);
        drawSprite(target, getTexture("icons/print.png"), rpX + 50, printY + 9, 18, 18, ThemeColor::Mute);
        drawText(target, fontMedium, "Print Receipt (Pay First)", 10.5f, ThemeColor::Mute, rpX + 78, printY + 18);
    }

    // New Sale Reset Button
    if (paymentDone) {
        float newSaleY = printY + 41.f;
        bool hovNew = pointInRect(m, rpX, newSaleY, Layout::RP_W, 30.f);
        drawRounded(target, rpX, newSaleY, Layout::RP_W, 30.f, 6,
                    hovNew ? sf::Color(255, 253, 248) : ThemeColor::Paper, ThemeColor::Ink, 1.2f);
        drawText(target, fontSemiBold, "+ Start New Sale", 10.5f, ThemeColor::Ink,
                 rpX + Layout::RP_W / 2.f, newSaleY + 15, TextAlignment::Center);
    }
}

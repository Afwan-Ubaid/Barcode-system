#include "PosReportsView.h"
#include "UIUtils.h"

void PosReportsView::drawSalesTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                  const std::vector<SaleRecord>& salesHistory,
                                  const std::vector<ProductItem>& products,
                                  const InventoryManager& inventoryMgr,
                                  const std::string& currentRole,
                                  int salesSubTab,
                                  const std::vector<ReturnRecord>& returnHistory,
                                  int salesTimeFilter, float scrollOffset,
                                  const sf::Font& fontRegular, const sf::Font& fontMedium,
                                  const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                  const sf::Font& fontSerif) {
    bool isAdmin = (currentRole == "Admin");

    // Header Title and Subtab Switcher
    drawText(target, fontSerif, salesSubTab == 0 ? "Sales & Orders Performance Audit" : "Customer Returns & Refund Audit", 18, ThemeColor::Ink, mainX, 48);
    std::string subDesc = salesSubTab == 0
        ? (isAdmin ? "Real-time transactions, revenue analytics, and gross profit breakdown" : "Real-time store transactions, orders count, and payment ledger")
        : "Official audit trail of returned merchandise, cashier on duty, and cash refunded";
    drawText(target, fontRegular, subDesc, 9.f, ThemeColor::Mute, mainX, 70);

    // SubTab Pills: [ Sales Orders ] and [ Returns Audit ]
    float subPillW = 120.f;
    float subPillY = 48.f;
    float sub1X = mainX + Layout::MAIN_W - subPillW * 2.f - 8.f;
    float sub2X = sub1X + subPillW + 8.f;

    bool hovSub0 = pointInRect(mouse, sub1X, subPillY, subPillW, 26.f);
    drawRounded(target, sub1X, subPillY, subPillW, 26.f, 6,
                salesSubTab == 0 ? ThemeColor::Dark : (hovSub0 ? sf::Color(228, 220, 206) : sf::Color(238, 230, 216)), ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "Sales Orders", 9.f, salesSubTab == 0 ? ThemeColor::White : ThemeColor::Ink, sub1X + subPillW / 2.f, subPillY + 13.f, TextAlignment::Center);

    bool hovSub1 = pointInRect(mouse, sub2X, subPillY, subPillW, 26.f);
    drawRounded(target, sub2X, subPillY, subPillW, 26.f, 6,
                salesSubTab == 1 ? ThemeColor::Dark : (hovSub1 ? sf::Color(228, 220, 206) : sf::Color(238, 230, 216)), ThemeColor::Line, 1.f);
    drawText(target, fontSemiBold, "Returns Audit (" + std::to_string(returnHistory.size()) + ")", 9.f, salesSubTab == 1 ? ThemeColor::White : ThemeColor::Ink, sub2X + subPillW / 2.f, subPillY + 13.f, TextAlignment::Center);

    // =========================================================================
    // SUBTAB 0: SALES ORDERS AUDIT
    // =========================================================================
    if (salesSubTab == 0) {
        // 1. Calculate Profit and Metrics (Profit only calculated for Admin)
        auto calcSaleProfit = [&](const SaleRecord& s) -> int {
            if (!isAdmin) return 0;
            int profit = 0;
            if (!s.items.empty()) {
                for (const auto& cl : s.items) {
                    if (cl.productId >= 0 && cl.productId < (int)products.size()) {
                        const auto& p = products[cl.productId];
                        int cost = 0;
                        for (const auto& inv : inventoryMgr.getItems()) {
                            if (inv.barcode == p.barcode) {
                                cost = inv.costPrice;
                                break;
                            }
                        }
                        if (cost <= 0) cost = static_cast<int>(p.price * 0.70f);
                        profit += std::max(0, p.price - cost) * cl.quantity;
                    }
                }
            }
            if (profit <= 0 && s.totalAmount > 0) {
                profit = static_cast<int>(s.totalAmount * 0.30f);
            }
            return profit;
        };

        // Calculate Today, Week, Month totals
        std::time_t now = std::time(nullptr);
        std::tm* ltm = std::localtime(&now);
        char todayBuf[32];
        std::strftime(todayBuf, sizeof(todayBuf), "%Y-%m-%d", ltm);
        std::string todayStr = todayBuf;

        int daySales = 0, dayProfit = 0, dayCount = 0;
        int weekSales = 0, weekProfit = 0, weekCount = 0;
        int monthSales = 0, monthProfit = 0, monthCount = 0;

        for (const auto& s : salesHistory) {
            int pft = calcSaleProfit(s);
            bool isToday = (s.timestamp.find(todayStr) != std::string::npos || s.timestamp.length() < 10);
            if (isToday) {
                daySales += s.totalAmount;
                dayProfit += pft;
                dayCount++;
            }
            weekSales += s.totalAmount;
            weekProfit += pft;
            weekCount++;

            monthSales += s.totalAmount;
            monthProfit += pft;
            monthCount++;
        }

        // Top KPI Cards for Day / Week / Month
        float cardW = (Layout::MAIN_W - 20.f) / 3.f;
        float cardY = 88.f;

        // Card 1: Today (Day)
        drawRounded(target, mainX, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Today's Sales (Day)", 8.5f, ThemeColor::Mute, mainX + 12, cardY + 10);
        drawText(target, fontBold, formatRs(daySales), 13, ThemeColor::Ink, mainX + 12, cardY + 26);
        if (isAdmin) {
            drawText(target, fontSemiBold, "Profit: " + formatRs(dayProfit), 8.5f, dayProfit > 0 ? ThemeColor::Green : ThemeColor::Mute, mainX + 12, cardY + 41);
        } else {
            drawText(target, fontSemiBold, "Orders: " + std::to_string(dayCount) + " transactions", 8.5f, ThemeColor::Blue, mainX + 12, cardY + 41);
        }

        // Card 2: This Week (Week)
        float card2X = mainX + cardW + 10.f;
        drawRounded(target, card2X, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "This Week (7 Days)", 8.5f, ThemeColor::Mute, card2X + 12, cardY + 10);
        drawText(target, fontBold, formatRs(weekSales), 13, ThemeColor::Ink, card2X + 12, cardY + 26);
        if (isAdmin) {
            drawText(target, fontSemiBold, "Profit: " + formatRs(weekProfit), 8.5f, weekProfit > 0 ? ThemeColor::Green : ThemeColor::Mute, card2X + 12, cardY + 41);
        } else {
            drawText(target, fontSemiBold, "Orders: " + std::to_string(weekCount) + " transactions", 8.5f, ThemeColor::Blue, card2X + 12, cardY + 41);
        }

        // Card 3: This Month (Month)
        float card3X = card2X + cardW + 10.f;
        drawRounded(target, card3X, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "This Month (30 Days)", 8.5f, ThemeColor::Mute, card3X + 12, cardY + 10);
        drawText(target, fontBold, formatRs(monthSales), 13, ThemeColor::Ink, card3X + 12, cardY + 26);
        if (isAdmin) {
            drawText(target, fontSemiBold, "Profit: " + formatRs(monthProfit), 8.5f, monthProfit > 0 ? ThemeColor::Green : ThemeColor::Mute, card3X + 12, cardY + 41);
        } else {
            drawText(target, fontSemiBold, "Orders: " + std::to_string(monthCount) + " transactions", 8.5f, ThemeColor::Blue, card3X + 12, cardY + 41);
        }

        // Time Filter switcher pills
        float filterY = 148.f;
        const char* filters[] = {"All Transactions", "Today Only", "This Week", "This Month"};
        float fX = mainX;
        for (int fi = 0; fi < 4; fi++) {
            bool active = (salesTimeFilter == fi);
            float fw = (fi == 0) ? 120.f : 90.f;
            drawRounded(target, fX, filterY, fw, 24, 6,
                        active ? ThemeColor::Dark : sf::Color(235, 227, 212), ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, filters[fi], 8.5f,
                     active ? ThemeColor::White : ThemeColor::Ink, fX + fw / 2.f, filterY + 11, TextAlignment::Center);
            fX += fw + 8.f;
        }

        float ty = 180.f;
        drawRounded(target, mainX, ty, Layout::MAIN_W, 480, 10, ThemeColor::Paper, ThemeColor::Line, 1);

        if (salesHistory.empty()) {
            drawText(target, fontRegular, "No transactions recorded yet in this register.", 12, ThemeColor::Mute,
                     mainX + Layout::MAIN_W / 2.f, ty + 160, TextAlignment::Center);
            return;
        }

        // Table Header
        drawText(target, fontSemiBold, "Order & Payment", 9.f, ThemeColor::Mute, mainX + 20, ty + 14);
        drawText(target, fontSemiBold, "Timestamp & Items", 9.f, ThemeColor::Mute, mainX + 185, ty + 14);
        if (isAdmin) {
            drawText(target, fontSemiBold, "Profit Earned", 9.f, ThemeColor::Mute, mainX + 340, ty + 14);
        } else {
            drawText(target, fontSemiBold, "Cashier Present", 9.f, ThemeColor::Mute, mainX + 340, ty + 14);
        }
        drawText(target, fontSemiBold, "Total Amount", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 20, ty + 14, TextAlignment::Right);

        sf::RectangleShape hdrLine({Layout::MAIN_W - 40, 1});
        hdrLine.setPosition(mainX + 20, ty + 26);
        hdrLine.setFillColor(ThemeColor::Line);
        target.draw(hdrLine);

        int rowIdx = 0;
        for (int i = (int)salesHistory.size() - 1; i >= 0; i--) {
            const auto& s = salesHistory[i];
            if (salesTimeFilter == 1 && s.timestamp.find(todayStr) == std::string::npos && s.timestamp.length() >= 10) continue;

            float rowY = ty + 38.f + static_cast<float>(rowIdx) * 44.f - scrollOffset;
            rowIdx++;
            if (rowY < ty + 24 || rowY > ty + 460) continue;

            int pft = calcSaleProfit(s);

            std::string ordLabel = s.orderId.empty() ? ("Sale #" + std::to_string(i + 1)) : s.orderId;
            drawText(target, fontBold, ordLabel + " (" + s.paymentMethod + ")", 10.5f, ThemeColor::Ink, mainX + 20, rowY);
            drawText(target, fontRegular, "Cashier: " + (s.cashier.empty() ? "Staff" : s.cashier), 8.5f, ThemeColor::Mute, mainX + 20, rowY + 15);

            drawText(target, fontMedium, s.timestamp, 9.5f, ThemeColor::Ink, mainX + 185, rowY);
            drawText(target, fontRegular, std::to_string(s.itemCount) + " items purchased", 8.5f, ThemeColor::Mute, mainX + 185, rowY + 15);

            // Profit badge (ONLY shown for Admin)
            if (isAdmin) {
                drawRounded(target, mainX + 335, rowY - 2, 85, 20, 4, sf::Color(220, 252, 231));
                drawText(target, fontBold, "+" + formatRs(pft) + " Profit", 8.5f, ThemeColor::Green, mainX + 377, rowY + 6, TextAlignment::Center);
            } else {
                drawText(target, fontSemiBold, s.cashier.empty() ? "Staff" : s.cashier, 9.5f, sf::Color(40, 110, 160), mainX + 340, rowY + 6);
            }

            // Description [ i ] button for each sale
            float btnIX = mainX + Layout::MAIN_W - 130.f;
            bool hovI = pointInRect(mouse, btnIX, rowY - 2.f, 26.f, 24.f);
            drawRounded(target, btnIX, rowY - 2.f, 26.f, 24.f, 5,
                        hovI ? ThemeColor::Blue : sf::Color(230, 238, 248), ThemeColor::Blue, 1.f);
            drawText(target, fontBold, "i", 11.f, hovI ? ThemeColor::White : ThemeColor::Blue, btnIX + 13.f, rowY + 10.f, TextAlignment::Center);

            // Total Amount
            drawText(target, fontBold, formatRs(s.totalAmount), 12, ThemeColor::Ink, mainX + Layout::MAIN_W - 20, rowY + 6, TextAlignment::Right);

            sf::RectangleShape sep({Layout::MAIN_W - 40, 1});
            sep.setPosition(mainX + 20, rowY + 28);
            sep.setFillColor(ThemeColor::Line);
            target.draw(sep);
        }
    }
    // =========================================================================
    // SUBTAB 1: CUSTOMER RETURNS AUDIT (Requested Feature)
    // =========================================================================
    else {
        // Calculate Returns KPI
        int totalReturns = (int)returnHistory.size();
        int totalUnitsRestocked = 0;
        int totalRefundsPKR = 0;
        for (const auto& r : returnHistory) {
            totalUnitsRestocked += r.quantity;
            totalRefundsPKR += r.refundAmount;
        }

        float cardW = (Layout::MAIN_W - 20.f) / 3.f;
        float cardY = 88.f;

        // Card 1: Total Return Events
        drawRounded(target, mainX, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Total Return Events", 8.5f, ThemeColor::Mute, mainX + 12, cardY + 10);
        drawText(target, fontBold, std::to_string(totalReturns) + " returns", 13, ThemeColor::Ink, mainX + 12, cardY + 26);
        drawText(target, fontSemiBold, "Processed & Verified", 8.5f, ThemeColor::Green, mainX + 12, cardY + 41);

        // Card 2: Restocked Units
        float card2X = mainX + cardW + 10.f;
        drawRounded(target, card2X, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Units Restocked to Shelf", 8.5f, ThemeColor::Mute, card2X + 12, cardY + 10);
        drawText(target, fontBold, std::to_string(totalUnitsRestocked) + " pcs", 13, sf::Color(30, 95, 150), card2X + 12, cardY + 26);
        drawText(target, fontSemiBold, "Restored in product.txt", 8.5f, ThemeColor::Mute, card2X + 12, cardY + 41);

        // Card 3: Total Refunds Issued
        float card3X = card2X + cardW + 10.f;
        drawRounded(target, card3X, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Total Refunds Issued", 8.5f, ThemeColor::Mute, card3X + 12, cardY + 10);
        drawText(target, fontBold, formatRs(totalRefundsPKR), 13, sf::Color(220, 50, 40), card3X + 12, cardY + 26);
        drawText(target, fontSemiBold, "Cash reimbursed to customers", 8.5f, ThemeColor::Mute, card3X + 12, cardY + 41);

        float ty = 155.f;
        drawRounded(target, mainX, ty, Layout::MAIN_W, 505, 10, ThemeColor::Paper, ThemeColor::Line, 1);

        if (returnHistory.empty()) {
            drawText(target, fontRegular, "No customer returns recorded yet. Process returns from the Return tab.", 12, ThemeColor::Mute,
                     mainX + Layout::MAIN_W / 2.f, ty + 160, TextAlignment::Center);
            return;
        }

        // Table Header
        drawText(target, fontSemiBold, "Return ID & Ref", 9.f, ThemeColor::Mute, mainX + 20, ty + 14);
        drawText(target, fontSemiBold, "Timestamp & Cashier Present", 9.f, ThemeColor::Mute, mainX + 175, ty + 14);
        drawText(target, fontSemiBold, "Restocked Item", 9.f, ThemeColor::Mute, mainX + 340, ty + 14);
        drawText(target, fontSemiBold, "Refund Amount", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 20, ty + 14, TextAlignment::Right);

        sf::RectangleShape hdrLine({Layout::MAIN_W - 40, 1});
        hdrLine.setPosition(mainX + 20, ty + 26);
        hdrLine.setFillColor(ThemeColor::Line);
        target.draw(hdrLine);

        int rIdx = 0;
        for (int i = (int)returnHistory.size() - 1; i >= 0; i--) {
            const auto& ret = returnHistory[i];
            float rowY = ty + 38.f + static_cast<float>(rIdx) * 44.f - scrollOffset;
            rIdx++;
            if (rowY < ty + 24 || rowY > ty + 485) continue;

            // Return ID & Ref Order
            drawText(target, fontBold, ret.returnId, 10.5f, ThemeColor::Ink, mainX + 20, rowY);
            drawText(target, fontRegular, "Order Ref: " + ret.orderRef, 8.5f, ThemeColor::Mute, mainX + 20, rowY + 15);

            // Timestamp & Cashier Present
            drawText(target, fontMedium, ret.timestamp, 9.5f, ThemeColor::Ink, mainX + 175, rowY);
            drawText(target, fontBold, "Cashier: " + ret.cashier, 8.5f, sf::Color(30, 95, 150), mainX + 175, rowY + 15);

            // Restocked Item
            std::string itemDisp = ret.itemName;
            if (itemDisp.length() > 20) itemDisp = itemDisp.substr(0, 18) + "..";
            drawText(target, fontBold, itemDisp + " (" + std::to_string(ret.quantity) + "x)", 10.f, ThemeColor::Ink, mainX + 340, rowY);
            drawText(target, fontRegular, "SKU: " + ret.barcode, 8.5f, ThemeColor::Mute, mainX + 340, rowY + 15);

            // Refund Badge
            drawText(target, fontBold, "-" + formatRs(ret.refundAmount), 12, sf::Color(220, 50, 40), mainX + Layout::MAIN_W - 20, rowY + 6, TextAlignment::Right);

            sf::RectangleShape sep({Layout::MAIN_W - 40, 1});
            sep.setPosition(mainX + 20, rowY + 28);
            sep.setFillColor(ThemeColor::Line);
            target.draw(sep);
        }
    }
}

void PosReportsView::drawActivityLogsTab(sf::RenderTarget& target, sf::Vector2f /*m*/, float mainX,
                                        const std::vector<ActivityRecord>& logs,
                                        const std::string& logFilter, float scrollOffset,
                                        const sf::Font& fontRegular, const sf::Font& fontMedium,
                                        const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                        const sf::Font& fontSerif) {
    drawText(target, fontSerif, "Activity & Security Audit Logs", 24, ThemeColor::Ink, mainX, 50);

    const char* filters[] = {"ALL", "LOGIN", "SALE", "RETURN", "STAFF", "INVENTORY"};
    for (int i = 0; i < 6; i++) {
        bool active = (logFilter == filters[i]);
        drawRounded(target, mainX + i * 86, 95, 78, 26, 6,
                    active ? ThemeColor::Dark : sf::Color(238, 230, 214),
                    active ? ThemeColor::Dark : ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, filters[i], 9.f,
                 active ? ThemeColor::White : ThemeColor::Ink, mainX + i * 86 + 39, 108, TextAlignment::Center);
    }

    float ty = 135.f;
    drawRounded(target, mainX, ty, Layout::MAIN_W, 525, 10, ThemeColor::Paper, ThemeColor::Line, 1);

    // Header row
    drawText(target, fontSemiBold, "Timestamp", 10, ThemeColor::Mute, mainX + 20, ty + 16);
    drawText(target, fontSemiBold, "Category", 10, ThemeColor::Mute, mainX + 148, ty + 16);
    drawText(target, fontSemiBold, "Operator", 10, ThemeColor::Mute, mainX + 220, ty + 16);
    drawText(target, fontSemiBold, "Audit Action Details", 10, ThemeColor::Mute, mainX + 300, ty + 16);

    sf::RectangleShape line({Layout::MAIN_W - 40, 1});
    line.setPosition(mainX + 20, ty + 28);
    line.setFillColor(ThemeColor::Line);
    target.draw(line);

    if (logs.empty()) {
        drawText(target, fontRegular, "No activity recorded yet in activity_log.txt.", 13, ThemeColor::Mute,
                 mainX + Layout::MAIN_W / 2.f, ty + 200, TextAlignment::Center);
        return;
    }

    int rowIdx = 0;
    for (int i = (int)logs.size() - 1; i >= 0; i--) {
        const auto& log = logs[i];
        if (logFilter != "ALL" && log.category != logFilter) continue;

        float rowY = ty + 42 + rowIdx * 34 - scrollOffset;
        rowIdx++;
        if (rowY < ty + 25 || rowY > ty + 495) continue;

        drawText(target, fontRegular, log.timestamp, 9.f, ThemeColor::Mute, mainX + 20, rowY);

        sf::Color catColor = ThemeColor::Ink;
        if (log.category == "LOGIN") catColor = ThemeColor::Blue;
        else if (log.category == "SALE") catColor = ThemeColor::Green;
        else if (log.category == "RETURN") catColor = sf::Color(220, 90, 40);
        else if (log.category == "STAFF") catColor = sf::Color(130, 60, 160);
        else if (log.category == "INVENTORY" || log.category == "PRODUCT") catColor = sf::Color(190, 90, 40);

        drawRounded(target, mainX + 132, rowY - 8, 70, 20, 4, sf::Color(catColor.r, catColor.g, catColor.b, 35));
        drawText(target, fontBold, log.category, 8.5f, catColor, mainX + 167, rowY + 2, TextAlignment::Center);

        drawText(target, fontMedium, log.user, 9.5f, ThemeColor::Ink, mainX + 220, rowY);

        std::string safeDetails = log.details;
        if (safeDetails.length() > 34) {
            safeDetails = safeDetails.substr(0, 31) + "...";
        }
        drawText(target, fontRegular, safeDetails, 9.5f, ThemeColor::Ink, mainX + 300, rowY);

        sf::RectangleShape sep({Layout::MAIN_W - 40, 1});
        sep.setPosition(mainX + 20, rowY + 16);
        sep.setFillColor(sf::Color(230, 222, 210));
        target.draw(sep);
    }
}

void PosReportsView::drawSettingsTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                    const std::vector<StaffMember>& staffList,
                                    const std::string& currentRole, const std::string& currentUserName,
                                    bool cameraRunning, bool soundEnabled,
                                    const std::string& cameraName, bool isExternalCam, int cameraCount,
                                    int studentDiscount, int employeeDiscount, int promoCount,
                                    const sf::Font& fontRegular, const sf::Font& fontMedium,
                                    const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                    const sf::Font& fontSerif) {
    drawText(target, fontSerif, "System & Staff Settings", 24, ThemeColor::Ink, mainX, 50);
    drawText(target, fontRegular, "Cashier operator profiles, employee ID setup, password configuration, and camera streamer", 10.5f, ThemeColor::Mute, mainX, 74);

    float ty = 105.f;
    drawRounded(target, mainX, ty, Layout::MAIN_W, 560, 10, ThemeColor::Paper, ThemeColor::Line, 1);

    auto drawSetting = [&](const std::string& title, const std::string& desc, const std::string& val, float y, sf::Color valCol = ThemeColor::Ink, bool isClickable = false) {
        drawText(target, fontSemiBold, title, 12, ThemeColor::Ink, mainX + 25, y);
        drawText(target, fontRegular, desc, 9.5f, ThemeColor::Mute, mainX + 25, y + 15);
        bool hov = isClickable && pointInRect(mouse, mainX + Layout::MAIN_W - 145, y - 4, 125, 26);
        drawRounded(target, mainX + Layout::MAIN_W - 145, y - 4, 125, 26, 6,
                    hov ? sf::Color(228, 220, 204) : sf::Color(238, 230, 213), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, val, 9.5f, valCol, mainX + Layout::MAIN_W - 82, y + 8, TextAlignment::Center);
    };

    std::string camStatus = cameraRunning ? (isExternalCam ? "Active (Mobile)" : "Active (Laptop)") : "Offline";
    sf::Color camStatusCol = cameraRunning ? (isExternalCam ? sf::Color(34, 197, 94) : ThemeColor::Green) : ThemeColor::Red;
    drawSetting("Active Scanner Hardware", cameraName + " (Media Foundation 30 FPS)", camStatus, ty + 16, camStatusCol, true);
    drawSetting("Camera Device Switcher", (cameraCount > 1) ? "Switch between phone & laptop camera" : "Plug mobile phone via USB cable to enable", (cameraCount > 1) ? "Click / Press 'C'" : "1 Camera Ready", ty + 56, (cameraCount > 1) ? ThemeColor::Blue : ThemeColor::Mute, (cameraCount > 1));
    drawSetting("Logged-in Operator", "Active role & session user", currentUserName + " (" + currentRole + ")", ty + 96, (currentRole == "Admin") ? sf::Color(180, 70, 50) : sf::Color(40, 110, 160));
    drawSetting("Audio Scan Feedback", "Beep sound upon barcode read", soundEnabled ? "Enabled" : "Muted", ty + 136, soundEnabled ? sf::Color(34, 197, 94) : sf::Color(140, 140, 140), true);

    // ── Staff & Operator Management (staff.txt) ──
    float staffY = ty + 175.f;
    sf::RectangleShape staffSep({Layout::MAIN_W - 50, 1});
    staffSep.setPosition(mainX + 25, staffY);
    staffSep.setFillColor(ThemeColor::Line);
    target.draw(staffSep);

    if (currentRole == "Admin") {
        drawText(target, fontSemiBold, "Staff & Operator Management (staff.txt)", 13, ThemeColor::Ink, mainX + 25, staffY + 15);
        drawText(target, fontRegular, "Manage cashier accounts, role credentials, and PINs", 9.5f, ThemeColor::Mute, mainX + 25, staffY + 31);

        // "+ Add Employee" button
        bool hovAddStaff = pointInRect(mouse, mainX + Layout::MAIN_W - 145, staffY + 12, 120, 28);
        drawRounded(target, mainX + Layout::MAIN_W - 145, staffY + 12, 120, 28, 6, hovAddStaff ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontBold, "+ Add Employee", 9.5f, ThemeColor::White, mainX + Layout::MAIN_W - 85, staffY + 25, TextAlignment::Center);

        // Staff table header
        float thY = staffY + 52.f;
        drawText(target, fontSemiBold, "Employee ID", 9.5f, ThemeColor::Mute, mainX + 30, thY);
        drawText(target, fontSemiBold, "Role", 9.5f, ThemeColor::Mute, mainX + 160, thY);
        drawText(target, fontSemiBold, "Security PIN", 9.5f, ThemeColor::Mute, mainX + 270, thY);
        drawText(target, fontSemiBold, "Actions", 9.5f, ThemeColor::Mute, mainX + 400, thY);

        for (size_t i = 0; i < staffList.size() && i < 4; i++) {
            const auto& s = staffList[i];
            float sRowY = thY + 20.f + static_cast<float>(i) * 32.f;

            drawText(target, fontBold, s.name, 10.5f, ThemeColor::Ink, mainX + 30, sRowY + 6);

            bool isAdm = (s.role == "Admin");
            drawRounded(target, mainX + 155, sRowY, 68, 20, 4, isAdm ? sf::Color(254, 226, 226) : sf::Color(220, 235, 248));
            drawText(target, fontSemiBold, s.role, 8.5f, isAdm ? sf::Color(180, 50, 40) : sf::Color(30, 95, 150), mainX + 189, sRowY + 10, TextAlignment::Center);

            std::string maskedPass = std::string(s.password.size(), '*');
            if (maskedPass.empty()) maskedPass = "••••";
            drawText(target, fontRegular, maskedPass, 11, ThemeColor::Mute, mainX + 270, sRowY + 6);

            // Reset PIN Button
            bool hovReset = pointInRect(mouse, mainX + 355, sRowY, 74, 22);
            drawRounded(target, mainX + 355, sRowY, 74, 22, 4, hovReset ? ThemeColor::Blue : sf::Color(230, 238, 248), ThemeColor::Blue, 1.f);
            drawText(target, fontSemiBold, "Reset PIN", 8.5f, hovReset ? ThemeColor::White : ThemeColor::BlueHover, mainX + 392, sRowY + 10, TextAlignment::Center);

            // Remove Button
            if (s.name != currentUserName) {
                bool hovRem = pointInRect(mouse, mainX + 436, sRowY, 62, 22);
                drawRounded(target, mainX + 436, sRowY, 62, 22, 4, hovRem ? ThemeColor::Red : sf::Color(254, 226, 226), sf::Color(220, 80, 70), 1.f);
                drawText(target, fontSemiBold, "Remove", 8.5f, hovRem ? ThemeColor::White : ThemeColor::Red, mainX + 467, sRowY + 10, TextAlignment::Center);
            }
        }
    } else {
        drawText(target, fontSemiBold, "[Restricted] Staff Administration", 13, ThemeColor::Ink, mainX + 25, staffY + 18);
        drawText(target, fontRegular, "You are logged in as Employee (Standard Cashier).\nRegistering new Employee IDs, assigning passwords, and managing accounts are restricted to Administrators.", 10.f, ThemeColor::Mute, mainX + 25, staffY + 40);

        drawRounded(target, mainX + Layout::MAIN_W - 165, staffY + 18, 140, 28, 6, sf::Color(235, 228, 218), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, "[Admin Only] Add Staff", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 95, staffY + 31, TextAlignment::Center);

        float prevY = staffY + 80.f;
        drawRounded(target, mainX + 25, prevY, Layout::MAIN_W - 50, 48, 6, sf::Color(245, 240, 232), ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Active Staff ID: " + currentUserName, 11, ThemeColor::Ink, mainX + 40, prevY + 16);
        drawText(target, fontRegular, "Role: Employee Cashier | Authorized for Sales & Optical Scanning", 9.5f, ThemeColor::Mute, mainX + 40, prevY + 32);
    }

    // ── Discount Settings (Admin Settings & promo.txt) ──
    float discY = ty + 360.f;
    sf::RectangleShape discSep({Layout::MAIN_W - 50, 1});
    discSep.setPosition(mainX + 25, discY);
    discSep.setFillColor(ThemeColor::Line);
    target.draw(discSep);

    drawText(target, fontSemiBold, "Discount Configuration (Admin Settings)", 12.f, ThemeColor::Ink, mainX + 25, discY + 12);
    drawText(target, fontRegular, "Store discount policies and promotional vouchers with supplier/item targeting", 9.f, ThemeColor::Mute, mainX + 25, discY + 27);

    if (currentRole == "Admin") {
        // Student discount row
        float r1Y = discY + 45.f;
        drawText(target, fontSemiBold, "Student Discount Rate:", 10.f, ThemeColor::Ink, mainX + 25, r1Y + 3);
        drawText(target, fontRegular, "Fixed rate, 1-click in checkout", 8.5f, ThemeColor::Mute, mainX + 25, r1Y + 17);

        // [-] button
        bool hovSMinus = pointInRect(mouse, mainX + 185, r1Y, 26, 24);
        drawRounded(target, mainX + 185, r1Y, 26, 24, 4, hovSMinus ? sf::Color(228, 220, 204) : sf::Color(238, 230, 213), ThemeColor::Edge, 1);
        drawText(target, fontBold, "-", 12, ThemeColor::Ink, mainX + 198, r1Y + 11, TextAlignment::Center);

        // Rate display
        drawText(target, fontBold, std::to_string(studentDiscount) + "%", 11.5f, ThemeColor::Blue, mainX + 230, r1Y + 11, TextAlignment::Center);

        // [+] button
        bool hovSPlus = pointInRect(mouse, mainX + 250, r1Y, 26, 24);
        drawRounded(target, mainX + 250, r1Y, 26, 24, 4, hovSPlus ? sf::Color(228, 220, 204) : sf::Color(238, 230, 213), ThemeColor::Edge, 1);
        drawText(target, fontBold, "+", 12, ThemeColor::Ink, mainX + 263, r1Y + 11, TextAlignment::Center);

        // Employee discount row
        float r2Y = discY + 78.f;
        drawText(target, fontSemiBold, "Employee Discount Rate:", 10.f, ThemeColor::Ink, mainX + 25, r2Y + 3);
        drawText(target, fontRegular, "Requires staff username & PIN", 8.5f, ThemeColor::Mute, mainX + 25, r2Y + 17);

        // [-] button
        bool hovEMinus = pointInRect(mouse, mainX + 185, r2Y, 26, 24);
        drawRounded(target, mainX + 185, r2Y, 26, 24, 4, hovEMinus ? sf::Color(228, 220, 204) : sf::Color(238, 230, 213), ThemeColor::Edge, 1);
        drawText(target, fontBold, "-", 12, ThemeColor::Ink, mainX + 198, r2Y + 11, TextAlignment::Center);

        // Rate display
        drawText(target, fontBold, std::to_string(employeeDiscount) + "%", 11.5f, ThemeColor::Blue, mainX + 230, r2Y + 11, TextAlignment::Center);

        // [+] button
        bool hovEPlus = pointInRect(mouse, mainX + 250, r2Y, 26, 24);
        drawRounded(target, mainX + 250, r2Y, 26, 24, 4, hovEPlus ? sf::Color(228, 220, 204) : sf::Color(238, 230, 213), ThemeColor::Edge, 1);
        drawText(target, fontBold, "+", 12, ThemeColor::Ink, mainX + 263, r2Y + 11, TextAlignment::Center);

        // Promo database info badge on right side
        drawRounded(target, mainX + 295, r1Y, Layout::MAIN_W - 320, 62, 6, sf::Color(242, 237, 228), ThemeColor::Edge, 1);
        drawText(target, fontBold, "Promo Codes (promo.txt):", 9.f, ThemeColor::Ink, mainX + 305, r1Y + 10);
        drawText(target, fontRegular, std::to_string(promoCount) + " vouchers | Target by Item / Supplier", 8.f, ThemeColor::Mute, mainX + 305, r1Y + 23);

        // [+ Manage Promo Codes] Button
        float promoBtnW = Layout::MAIN_W - 340.f;
        bool hovPromo = pointInRect(mouse, mainX + 305, r1Y + 36, promoBtnW, 20);
        drawRounded(target, mainX + 305, r1Y + 36, promoBtnW, 20, 4, hovPromo ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontSemiBold, "+ Add / Manage Promo Codes", 8.f, ThemeColor::White, mainX + 305 + promoBtnW / 2.f, r1Y + 44, TextAlignment::Center);
    } else {
        float r1Y = discY + 45.f;
        drawText(target, fontRegular, "[Admin Only] Policies: Student " + std::to_string(studentDiscount) + "% | Employee " + std::to_string(employeeDiscount) + "% | " + std::to_string(promoCount) + " promo codes in promo.txt", 9.5f, ThemeColor::Mute, mainX + 25, r1Y + 10);
    }

    // System Utilities
    float utilY = discY + 115.f;
    bool hovRel = pointInRect(mouse, mainX + 25, utilY, 235, 30);
    drawRounded(target, mainX + 25, utilY, 235, 30, 6, hovRel ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "Reload product.txt Database", 9.f, ThemeColor::White, mainX + 142, utilY + 14, TextAlignment::Center);

    bool hovGen = pointInRect(mouse, mainX + 275, utilY, 235, 30);
    drawRounded(target, mainX + 275, utilY, 235, 30, 6, hovGen ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontSemiBold, "Regenerate Barcode Bitmaps", 9.f, ThemeColor::White, mainX + 392, utilY + 14, TextAlignment::Center);

    float compY = utilY + 38.f;
    drawRounded(target, mainX + 25, compY, Layout::MAIN_W - 50, 42, 8, sf::Color(242, 237, 228), ThemeColor::Edge, 1);
    drawText(target, fontBold, "System Compliance & Credentials Storage:", 9.f, ThemeColor::Ink, mainX + 38, compY + 8);
    drawText(target, fontRegular, "Credentials in staff.txt | Discounts in discount_settings.txt & promo.txt | Logged in activity_log.txt", 8.5f, ThemeColor::Mute, mainX + 38, compY + 23);
}

void PosReportsView::drawReturnTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                   const std::string& returnBarcode, bool returnBarcodeFocused,
                                   int returnQty, int returnMaxQty, const ProductItem* foundProduct,
                                   const std::string& saleInfo, int salePrice,
                                   const std::string& saleOrderId, const std::string& saleCustomer,
                                   const std::string& saleDate, bool returnSuccess,
                                   const std::string& statusMessage, const sf::Texture& cameraTexture,
                                   const sf::Font& fontRegular, const sf::Font& fontMedium,
                                   const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                   const sf::Font& fontSerif,
                                   std::function<const sf::Texture&(const std::string&)> getTexture) {
    drawText(target, fontSerif, "Customer Returns & Restock", 24, ThemeColor::Ink, mainX, 48);

    float ty = 100.f;
    float colW = (Layout::MAIN_W - 20.f) / 2.f;

    // -------------------------------------------------------------
    // LEFT COLUMN: Optical Scanner & Item Lookup
    // -------------------------------------------------------------
    drawRounded(target, mainX, ty, colW, 560, 10, ThemeColor::Paper, ThemeColor::Line, 1.2f);
    drawText(target, fontBold, "Optical Barcode Scanner & Search", 13, ThemeColor::Ink, mainX + 20, ty + 20);

    // Camera viewfinder pane
    float paneX = mainX + 20.f;
    float paneY = ty + 60.f;
    float paneW = colW - 40.f;
    float paneH = 150.f;

    drawRounded(target, paneX, paneY, paneW, paneH, 8, sf::Color(16, 16, 18));
    if (cameraTexture.getSize().x > 0) {
        sf::Sprite camSprite(cameraTexture);
        camSprite.setPosition(paneX, paneY);
        camSprite.setScale(paneW / cameraTexture.getSize().x, paneH / cameraTexture.getSize().y);
        target.draw(camSprite);
    } else {
        drawSprite(target, getTexture("icons/scan.png"), paneX + paneW / 2.f - 20, paneY + paneH / 2.f - 24, 40, 40, sf::Color(200, 200, 200, 160));
        drawText(target, fontSemiBold, "Camera Scanner Ready", 11.f, sf::Color(220, 220, 220), paneX + paneW / 2.f, paneY + paneH / 2.f + 20, TextAlignment::Center);
    }

    // Viewfinder laser sweep
    float sweep = 0.5f + 0.5f * std::sin((float)clock() / CLOCKS_PER_SEC * 3.f);
    float ly = paneY + 10.f + sweep * (paneH - 20.f);
    sf::RectangleShape laser({paneW, 2.f});
    laser.setPosition(paneX, ly);
    laser.setFillColor(sf::Color(240, 50, 40, 220));
    target.draw(laser);

    // Live continuous badge
    drawRounded(target, paneX + 10.f, paneY + 10.f, 130.f, 18.f, 4.f, sf::Color(15, 15, 18, 200));
    sf::CircleShape dot(3.f, 16);
    dot.setOrigin(3.f, 3.f);
    dot.setPosition(paneX + 18.f, paneY + 19.f);
    dot.setFillColor(ThemeColor::Green);
    target.draw(dot);
    drawText(target, fontSemiBold, "AUTO SCANNER ACTIVE", 7.5f, sf::Color(240, 240, 240), paneX + 26.f, paneY + 19.f);

    // Manual Barcode Input
    float inY = paneY + paneH + 20.f;
    drawText(target, fontSemiBold, "Item Barcode / SKU:", 10.5f, ThemeColor::Ink, mainX + 20, inY);
    drawRounded(target, mainX + 20, inY + 18, paneW - 90, 36, 6, ThemeColor::White, returnBarcodeFocused ? ThemeColor::Blue : ThemeColor::Line, returnBarcodeFocused ? 1.5f : 1.f);
    std::string dispBc = returnBarcode.empty() ? "Enter 6-digit barcode..." : returnBarcode;
    drawText(target, fontBold, dispBc, 11.f, returnBarcode.empty() ? ThemeColor::Mute : ThemeColor::Ink, mainX + 32, inY + 36);

    // Lookup Button
    drawRounded(target, mainX + 20 + paneW - 80, inY + 18, 80, 36, 6, ThemeColor::Blue);
    drawText(target, fontSemiBold, "Look Up", 10.5f, ThemeColor::White, mainX + 20 + paneW - 40, inY + 36, TextAlignment::Center);

    // Quick Test Barcode Pills
    drawText(target, fontSemiBold, "Quick Demo Test Items:", 9.5f, ThemeColor::Mute, mainX + 20, inY + 75);
    const char* demoCodes[] = {"100001", "100002", "100004", "100005"};
    const char* demoNames[] = {"Milk", "Bread", "Apples", "Juice"};
    float pillX = mainX + 20;
    for (int p = 0; p < 4; p++) {
        drawRounded(target, pillX, inY + 95, 52, 24, 5, sf::Color(235, 227, 212), ThemeColor::Line, 1);
        drawText(target, fontSemiBold, demoNames[p], 8.5f, ThemeColor::Ink, pillX + 26, inY + 107, TextAlignment::Center);
        pillX += 58;
    }

    drawRounded(target, mainX + 20, inY + 140, paneW, 110, 8, sf::Color(245, 240, 230), ThemeColor::Edge, 1);
    drawText(target, fontBold, "Return Policy & Verification:", 9.5f, ThemeColor::Ink, mainX + 32, inY + 155);
    drawText(target, fontRegular, "- Queries Order.txt & Bill.txt records\n- Restores stock back to inventory\n- Logs refund to activity audit trail", 8.5f, ThemeColor::Mute, mainX + 32, inY + 175);

    // -------------------------------------------------------------
    // RIGHT COLUMN: Sale Verification & Return Confirmation
    // -------------------------------------------------------------
    float rX = mainX + colW + 20.f;
    drawRounded(target, rX, ty, colW, 560, 10, ThemeColor::Paper, ThemeColor::Line, 1.2f);
    drawText(target, fontBold, "Sale Origin & Refund Calculation", 13, ThemeColor::Ink, rX + 20, ty + 20);

    if (foundProduct != nullptr) {
        // Product Info Card
        float pY = ty + 60.f;
        drawRounded(target, rX + 20, pY, paneW, 82, 8, sf::Color(243, 238, 228), ThemeColor::Line, 1);
        drawText(target, fontBold, foundProduct->name, 12.5f, ThemeColor::Ink, rX + 32, pY + 15);
        drawText(target, fontRegular, "Barcode: " + foundProduct->barcode + " | " + foundProduct->category, 9.f, ThemeColor::Mute, rX + 32, pY + 32);
        drawText(target, fontBold, "Retail Price: Rs " + std::to_string(foundProduct->price), 10.f, ThemeColor::Ink, rX + 32, pY + 49);
        drawText(target, fontSemiBold, "Stock in Store: " + std::to_string(foundProduct->stock) + " units", 9.5f, sf::Color(22, 101, 52), rX + 32, pY + 66);

        // Sale Origin Verification Box
        float sY = pY + 92.f;
        if (!saleOrderId.empty() && returnMaxQty > 0) {
            drawRounded(target, rX + 20, sY, paneW, 132, 8, sf::Color(220, 252, 231), sf::Color(22, 101, 52), 1.2f);
            drawText(target, fontBold, "[VERIFIED SALE ORIGIN LOCATED]", 10.f, sf::Color(22, 101, 52), rX + 32, sY + 16);
            drawText(target, fontRegular, "Order ID: " + saleOrderId, 9.5f, ThemeColor::Ink, rX + 32, sY + 34);
            drawText(target, fontRegular, "Sale Date: " + saleDate, 9.5f, ThemeColor::Ink, rX + 32, sY + 52);
            drawText(target, fontRegular, "Purchaser: " + saleCustomer, 9.5f, ThemeColor::Ink, rX + 32, sY + 70);
            drawText(target, fontBold, "Purchased Quantity: " + std::to_string(returnMaxQty) + " units (Max Eligible)", 10.f, sf::Color(22, 101, 52), rX + 32, sY + 89);
            std::string soldPriceLabel = "Original Sold Price: Rs " + std::to_string(salePrice);
            if (foundProduct != nullptr && salePrice < foundProduct->price) {
                soldPriceLabel += " (Discounted from Rs " + std::to_string(foundProduct->price) + ")";
            }
            drawText(target, fontBold, soldPriceLabel, 10.f, (foundProduct != nullptr && salePrice < foundProduct->price) ? ThemeColor::Green : ThemeColor::Ink, rX + 32, sY + 108);
        } else {
            drawRounded(target, rX + 20, sY, paneW, 90, 8, sf::Color(254, 243, 199), sf::Color(217, 119, 6), 1.f);
            drawText(target, fontBold, "[!] Purchase Record Not Found", 9.5f, sf::Color(146, 64, 14), rX + 32, sY + 16);
            drawText(target, fontRegular, "No sale record for this customer / item found in Order.txt or Bill.txt.", 9.f, ThemeColor::Mute, rX + 32, sY + 36);
            drawText(target, fontBold, "Return Limit: 0 units eligible", 10.f, ThemeColor::Red, rX + 32, sY + 56);
        }

        // Return Quantity Selector
        float qY = sY + ((!saleOrderId.empty() && returnMaxQty > 0) ? 142.f : 100.f);
        drawText(target, fontSemiBold, "Return Qty:", 10.f, ThemeColor::Ink, rX + 20, qY + 8);

        // [-] Button
        drawRounded(target, rX + 105, qY, 28, 28, 5, sf::Color(230, 222, 210), ThemeColor::Edge, 1);
        drawText(target, fontBold, "-", 13, ThemeColor::Ink, rX + 119, qY + 12, TextAlignment::Center);

        // Qty Display
        drawText(target, fontBold, std::to_string(returnQty), 12.5f, ThemeColor::Ink, rX + 147, qY + 12, TextAlignment::Center);

        // [+] Button
        drawRounded(target, rX + 162, qY, 28, 28, 5, sf::Color(230, 222, 210), ThemeColor::Edge, 1);
        drawText(target, fontBold, "+", 13, ThemeColor::Ink, rX + 176, qY + 12, TextAlignment::Center);

        // Max badge
        drawText(target, fontSemiBold, "Max: " + std::to_string(returnMaxQty), 9.f,
                 (returnQty >= returnMaxQty) ? ThemeColor::Red : ThemeColor::Mute, rX + 198, qY + 12);

        // Refund Total Box
        int effPrice = (salePrice > 0) ? salePrice : foundProduct->price;
        int totalRefund = effPrice * returnQty;

        float rfY = qY + 45.f;
        drawRounded(target, rX + 20, rfY, paneW, 55, 8, sf::Color(235, 242, 250), ThemeColor::Blue, 1.2f);
        drawText(target, fontRegular, "Total Refund Amount to Customer:", 9.5f, ThemeColor::Mute, rX + 32, rfY + 12);
        drawText(target, fontBold, "Rs " + std::to_string(totalRefund), 18.f, ThemeColor::Blue, rX + 32, rfY + 34);
        if (foundProduct != nullptr && effPrice < foundProduct->price) {
            drawText(target, fontSemiBold, "(Refund at discounted price: Rs " + std::to_string(effPrice) + " / unit)", 8.5f, ThemeColor::Green, rX + 160, rfY + 38);
        }

        // Confirm Return Button
        float bY = rfY + 70.f;
        drawRounded(target, rX + 20, bY, paneW, 46, 8, ThemeColor::Blue);
        drawText(target, fontBold, "Confirm Return & Restock Store Shelf", 10.5f, ThemeColor::White, rX + 20 + paneW / 2.f, bY + 23, TextAlignment::Center);

        if (returnSuccess) {
            drawRounded(target, rX + 20, bY + 56, paneW, 36, 6, sf::Color(220, 252, 231), sf::Color(22, 101, 52), 1.f);
            drawText(target, fontBold, statusMessage, 9.5f, sf::Color(22, 101, 52), rX + 20 + paneW / 2.f, bY + 74, TextAlignment::Center);
        }
    } else {
        float eY = ty + 120.f;
        drawRounded(target, rX + 20, eY, paneW, 120, 8, sf::Color(245, 242, 235), ThemeColor::Line, 1);
        drawSprite(target, getTexture("icons/scan.png"), rX + paneW / 2.f + 4, eY + 22, 32, 32, sf::Color(160, 150, 140));
        drawText(target, fontBold, "No Item Selected", 12.f, ThemeColor::Ink, rX + 20 + paneW / 2.f, eY + 68, TextAlignment::Center);
        drawText(target, fontRegular, "Scan or enter barcode to verify", 9.5f, ThemeColor::Mute, rX + 20 + paneW / 2.f, eY + 88, TextAlignment::Center);

        if (returnSuccess && !statusMessage.empty()) {
            drawRounded(target, rX + 20, eY + 145, paneW, 44, 6, sf::Color(220, 252, 231), sf::Color(22, 101, 52), 1.2f);
            drawText(target, fontBold, statusMessage, 9.5f, sf::Color(22, 101, 52), rX + 20 + paneW / 2.f, eY + 167, TextAlignment::Center);
        }
    }
}

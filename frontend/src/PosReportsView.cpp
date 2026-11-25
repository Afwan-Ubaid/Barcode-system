#include "PosReportsView.h"
#include "UIUtils.h"
#include <ctime>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>

// Helper to check calendar-day window (0 = today, < 7 = weekly, < 30 = monthly, < 365 = yearly)
static int getDaysAgo(const std::string& dateStr) {
    if (dateStr.length() < 10) return -1;
    std::tm tOrder = {};
    try {
        tOrder.tm_year = std::stoi(dateStr.substr(0, 4)) - 1900;
        tOrder.tm_mon  = std::stoi(dateStr.substr(5, 2)) - 1;
        tOrder.tm_mday = std::stoi(dateStr.substr(8, 2));
        tOrder.tm_isdst = -1;
    } catch (...) { return -1; }

    std::time_t now = std::time(nullptr);
    std::tm tToday = {};
    localtime_s(&tToday, &now);
    tToday.tm_hour = 0; tToday.tm_min = 0; tToday.tm_sec = 0; tToday.tm_isdst = -1;

    std::time_t todayMid = std::mktime(&tToday);
    std::time_t oTime = std::mktime(&tOrder);
    if (oTime == (std::time_t)-1 || todayMid == (std::time_t)-1) return -1;

    return static_cast<int>(std::round(std::difftime(todayMid, oTime) / 86400.0));
}

static bool isWithinDays(const std::string& dateStr, int maxDays) {
    int diff = getDaysAgo(dateStr);
    if (maxDays <= 1) return (diff == 0);
    return (diff >= 0 && diff < maxDays);
}

static std::string cleanDisplayStr(std::string s) {
    for (char& c : s) if (c == '_') c = ' ';
    return trimStr(s);
}

void PosReportsView::drawSalesTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                  const std::vector<SaleRecord>& salesHistory,
                                  const std::vector<ProductItem>& products,
                                  const InventoryManager& inventoryMgr,
                                  const std::string& currentRole,
                                  int salesSubTab,
                                  const std::vector<ReturnRecord>& returnHistory,
                                  int salesTimeFilter, float scrollOffset,
                                  const std::vector<OrderRecord>& orders,
                                  const std::vector<OrderDetailItem>& orderDetails,
                                  const std::vector<BillRecord>& bills,
                                  const std::vector<BillDetailRecord>& billDetails,
                                  const std::string& customStartDate,
                                  const std::string& customEndDate,
                                  int customActiveField,
                                  const sf::Font& fontRegular, const sf::Font& fontMedium,
                                  const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                  const sf::Font& fontSerif) {
    // =========================================================================
    // ISE-105: ROLE ISOLATION - STRICTLY RESTRICT TAB TO STORE MANAGER
    // =========================================================================
    if (currentRole != "Admin") {
        drawRounded(target, mainX, 40.f, Layout::MAIN_W, 620.f, 12.f, ThemeColor::Paper, ThemeColor::Line, 1.f);
        float cx = mainX + Layout::MAIN_W / 2.f;
        float cy = 320.f;

        drawRounded(target, cx - 36, cy - 50, 72, 72, 36, sf::Color(254, 226, 226));
        drawText(target, fontBold, "!", 38.f, sf::Color(220, 38, 38), cx, cy - 14, TextAlignment::Center);

        drawText(target, fontSerif, "Access Restricted - Store Manager Exclusive", 18.f, ThemeColor::Ink, cx, cy + 42, TextAlignment::Center);
        drawText(target, fontRegular, "The Reports and Financial Audit Ledger is strictly restricted to Store Managers and Administrators.", 10.f, ThemeColor::Mute, cx, cy + 70, TextAlignment::Center);
        drawText(target, fontSemiBold, "Cashier accounts do not have permission to view sales, audits, or operational reports.", 10.f, sf::Color(220, 38, 38), cx, cy + 90, TextAlignment::Center);
        return;
    }

    // Header Title
    std::string title = "Store Reports & Operational Audit";
    if (salesSubTab == 0) title = "Report 1: Incomplete Orders Audit";
    else if (salesSubTab == 1) title = "Report 2: Completed Orders Ledger";
    else if (salesSubTab == 2) title = "Report 3: Cash Sales Audit";
    else if (salesSubTab == 3) title = "Report 4: All Sales & Discounts Summary";
    else if (salesSubTab == 4) title = "Report 5: Top 10 Most Sold Items";
    else if (salesSubTab == 5) title = "Report 6: Top 10 Overstocked Items (Hold Purchase)";
    else if (salesSubTab == 6) title = "Customer Returns & Refund Audit";

    drawText(target, fontSerif, title, 16.5f, ThemeColor::Ink, mainX + 2.f, 38);

    // Tab switcher pills (6 Distinct Manager Reports)
    const char* tabLabels[] = {"1. Incomplete", "2. Completed", "3. Cash Audit", "4. All Sales", "5. Top Sold", "6. Overstock"};
    float pillW = 86.f;
    float pillY = 66.f;
    for (int ti = 0; ti < 6; ti++) {
        float px = mainX + ti * 91.f;
        bool active = (salesSubTab == ti);
        bool hov = pointInRect(mouse, px, pillY, pillW, 26.f);
        drawRounded(target, px, pillY, pillW, 26.f, 5,
                    active ? ThemeColor::Dark : (hov ? sf::Color(228, 220, 206) : sf::Color(238, 230, 216)),
                    ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, tabLabels[ti], 8.f,
                 active ? ThemeColor::White : ThemeColor::Ink,
                 px + pillW / 2.f, pillY + 13.f, TextAlignment::Center);
    }

    float ty = 158.f;
    float cardY = 98.f;
    float cardW = (Layout::MAIN_W - 20.f) / 3.f;

    // =========================================================================
    // REPORT 1: INCOMPLETE ORDERS REPORT (Unpaid / On-Hold / Pending)
    // =========================================================================
    if (salesSubTab == 0) {
        int incompCount = 0;
        int incompTotal = 0;
        std::vector<OrderRecord> incompOrders;
        for (const auto& o : orders) {
            std::string st = toLowerStr(o.orderStatus);
            if (st != "paid" && st != "archived") {
                incompOrders.push_back(o);
                incompCount++;
                incompTotal += o.paymentAmount;
            }
        }

        // Summary KPI Cards
        drawRounded(target, mainX, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Incomplete Orders", 8.5f, ThemeColor::Mute, mainX + 12, cardY + 10);
        drawText(target, fontBold, std::to_string(incompCount) + " Unsettled", 12.f, sf::Color(217, 119, 6), mainX + 12, cardY + 26);
        drawText(target, fontRegular, "Not paid or archived", 7.5f, ThemeColor::Mute, mainX + 12, cardY + 41);

        float card2X = mainX + cardW + 10.f;
        drawRounded(target, card2X, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Total Incomplete Amount", 8.5f, ThemeColor::Mute, card2X + 12, cardY + 10);
        drawText(target, fontBold, formatRs(incompTotal), 12.f, sf::Color(220, 70, 60), card2X + 12, cardY + 26);
        drawText(target, fontRegular, "Pending receivables sum", 7.5f, ThemeColor::Mute, card2X + 12, cardY + 41);

        float card3X = card2X + cardW + 10.f;
        drawRounded(target, card3X, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Audit Status", 8.5f, ThemeColor::Mute, card3X + 12, cardY + 10);
        drawText(target, fontBold, incompCount > 0 ? "Fulfill or Cancel" : "Register Clean", 11.5f, incompCount > 0 ? sf::Color(217, 119, 6) : sf::Color(35, 120, 50), card3X + 12, cardY + 26);
        drawText(target, fontRegular, "Audit trail reconciliation", 7.5f, ThemeColor::Mute, card3X + 12, cardY + 41);

        // Tabular View
        drawRounded(target, mainX, ty, Layout::MAIN_W, 502, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Order ID", 9.f, ThemeColor::Mute, mainX + 15, ty + 14);
        drawText(target, fontSemiBold, "Date & Time", 9.f, ThemeColor::Mute, mainX + 85, ty + 14);
        drawText(target, fontSemiBold, "Customer Name", 9.f, ThemeColor::Mute, mainX + 195, ty + 14);
        drawText(target, fontSemiBold, "Contact", 9.f, ThemeColor::Mute, mainX + 285, ty + 14);
        drawText(target, fontSemiBold, "Status", 9.f, ThemeColor::Mute, mainX + 375, ty + 14);
        drawText(target, fontSemiBold, "Pending Amount", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 15, ty + 14, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 26);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        if (incompOrders.empty()) {
            drawText(target, fontRegular, "All orders are settled. No incomplete orders found.", 11.5f, ThemeColor::Mute,
                     mainX + 15, ty + 48, TextAlignment::Left);
        } else {
            for (size_t i = 0; i < incompOrders.size(); i++) {
                float rowY = ty + 38.f + static_cast<float>(i) * 36.f - scrollOffset;
                if (rowY < ty + 30.f || rowY + 22.f > ty + 458.f) continue;
                const auto& o = incompOrders[i];

                drawText(target, fontBold, o.orderId, 9.f, ThemeColor::Ink, mainX + 15, rowY + 6);
                drawText(target, fontRegular, o.orderDate + " " + o.orderTime, 8.5f, ThemeColor::Mute, mainX + 85, rowY + 6);
                drawText(target, fontMedium, cleanDisplayStr(o.customerName), 9.f, ThemeColor::Ink, mainX + 195, rowY + 6);
                drawText(target, fontRegular, o.customerContact.empty() ? "N/A" : o.customerContact, 8.5f, ThemeColor::Mute, mainX + 285, rowY + 6);

                drawRounded(target, mainX + 365, rowY - 3, 56, 18, 4, sf::Color(254, 243, 199));
                drawText(target, fontBold, o.orderStatus, 8.f, sf::Color(180, 83, 9), mainX + 393, rowY + 6, TextAlignment::Center);

                drawText(target, fontBold, formatRs(o.paymentAmount), 10.5f, sf::Color(220, 70, 60), mainX + Layout::MAIN_W - 15, rowY + 6, TextAlignment::Right);

                sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
                rLine.setPosition(mainX + 15, rowY + 22);
                rLine.setFillColor(ThemeColor::Line);
                target.draw(rLine);
            }
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 462.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "GRAND TOTAL SUM (INCOMPLETE ORDERS)", 10.f, ThemeColor::Ink, mainX + 15, ty + 481.f);
        drawText(target, fontSemiBold, std::to_string(incompCount) + " Unsettled Orders", 9.5f, ThemeColor::Mute, mainX + 300, ty + 481.f);
        drawText(target, fontBold, formatRs(incompTotal), 12.f, sf::Color(220, 70, 60), mainX + Layout::MAIN_W - 15, ty + 481.f, TextAlignment::Right);
    }
    // =========================================================================
    // REPORT 2: COMPLETED ORDERS REPORT
    // =========================================================================
    else if (salesSubTab == 1) {
        int compCount = 0;
        int compTotal = 0;
        std::vector<OrderRecord> completedOrders;
        for (const auto& o : orders) {
            if (toLowerStr(o.orderStatus) == "paid") {
                completedOrders.push_back(o);
                compCount++;
                compTotal += o.paymentAmount;
            }
        }

        // Top KPI Cards
        drawRounded(target, mainX, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Completed Orders", 8.5f, ThemeColor::Mute, mainX + 12, cardY + 10);
        drawText(target, fontBold, std::to_string(compCount) + " Orders", 12.f, sf::Color(35, 120, 50), mainX + 12, cardY + 26);
        drawText(target, fontRegular, "Status: PAID in Order.txt", 7.5f, ThemeColor::Mute, mainX + 12, cardY + 41);

        float card2X = mainX + cardW + 10.f;
        drawRounded(target, card2X, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Total Settled Revenue", 8.5f, ThemeColor::Mute, card2X + 12, cardY + 10);
        drawText(target, fontBold, formatRs(compTotal), 12.f, ThemeColor::Ink, card2X + 12, cardY + 26);
        drawText(target, fontRegular, "Fully paid & reconciled", 7.5f, ThemeColor::Mute, card2X + 12, cardY + 41);

        float card3X = card2X + cardW + 10.f;
        drawRounded(target, card3X, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Average Order Value", 8.5f, ThemeColor::Mute, card3X + 12, cardY + 10);
        int aov = compCount > 0 ? (compTotal / compCount) : 0;
        drawText(target, fontBold, formatRs(aov), 12.f, sf::Color(40, 110, 160), card3X + 12, cardY + 26);
        drawText(target, fontRegular, "Per completed basket", 7.5f, ThemeColor::Mute, card3X + 12, cardY + 41);

        // Tabular View
        drawRounded(target, mainX, ty, Layout::MAIN_W, 502, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Order ID", 9.f, ThemeColor::Mute, mainX + 15, ty + 14);
        drawText(target, fontSemiBold, "Date & Time", 9.f, ThemeColor::Mute, mainX + 85, ty + 14);
        drawText(target, fontSemiBold, "Customer Name", 9.f, ThemeColor::Mute, mainX + 195, ty + 14);
        drawText(target, fontSemiBold, "Payment Mode", 9.f, ThemeColor::Mute, mainX + 285, ty + 14);
        drawText(target, fontSemiBold, "Status", 9.f, ThemeColor::Mute, mainX + 400, ty + 14);
        drawText(target, fontSemiBold, "Settled Total", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 15, ty + 14, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 26);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        for (size_t i = 0; i < completedOrders.size(); i++) {
            float rowY = ty + 38.f + static_cast<float>(i) * 36.f - scrollOffset;
            if (rowY < ty + 30.f || rowY + 22.f > ty + 458.f) continue;
            const auto& o = completedOrders[completedOrders.size() - 1 - i];

            drawText(target, fontBold, o.orderId, 9.f, ThemeColor::Ink, mainX + 15, rowY + 6);
            drawText(target, fontRegular, o.orderDate + " " + o.orderTime, 8.5f, ThemeColor::Mute, mainX + 85, rowY + 6);
            std::string cDisp = cleanDisplayStr(o.customerName);
            if (cDisp.length() > 14) cDisp = cDisp.substr(0, 12) + "..";
            drawText(target, fontMedium, cDisp, 9.f, ThemeColor::Ink, mainX + 195, rowY + 6);
            std::string mDisp = o.paymentMode + " (" + cleanDisplayStr(o.paymentMerchant) + ")";
            if (mDisp.length() > 18) mDisp = mDisp.substr(0, 16) + "..";
            drawText(target, fontRegular, mDisp, 8.5f, ThemeColor::Mute, mainX + 285, rowY + 6);

            drawRounded(target, mainX + 395, rowY - 3, 45, 18, 4, sf::Color(220, 252, 231));
            drawText(target, fontBold, "PAID", 8.f, sf::Color(35, 120, 50), mainX + 417, rowY + 6, TextAlignment::Center);

            drawText(target, fontBold, formatRs(o.paymentAmount), 10.5f, ThemeColor::Ink, mainX + Layout::MAIN_W - 15, rowY + 6, TextAlignment::Right);

            sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
            rLine.setPosition(mainX + 15, rowY + 22);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 462.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "GRAND TOTAL SETTLED REVENUE", 10.f, ThemeColor::Ink, mainX + 15, ty + 481.f);
        drawText(target, fontSemiBold, std::to_string(compCount) + " Paid Orders", 9.5f, ThemeColor::Mute, mainX + 270, ty + 481.f);
        drawText(target, fontBold, formatRs(compTotal), 12.f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 15, ty + 481.f, TextAlignment::Right);
    }
    // =========================================================================
    // REPORT 3: CASH SALES AUDIT (Daily, Weekly, Monthly, Yearly, Custom)
    // =========================================================================
    else if (salesSubTab == 2) {
        // Time filter selector
        float fY = 98.f;
        const char* timeLabels[] = {"Daily (Today)", "Weekly (7d)", "Monthly (30d)", "Yearly (365d)", "Custom Range"};
        float fx = mainX;
        for (int fi = 0; fi < 5; fi++) {
            bool active = (salesTimeFilter == fi);
            float fw = 102.f;
            bool hov = pointInRect(mouse, fx, fY, fw, 22.f);
            drawRounded(target, fx, fY, fw, 22.f, 4,
                        active ? ThemeColor::Blue : (hov ? sf::Color(230, 222, 206) : sf::Color(240, 233, 220)),
                        ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, timeLabels[fi], 8.f,
                     active ? ThemeColor::White : ThemeColor::Ink,
                     fx + fw / 2.f, fY + 11.f, TextAlignment::Center);
            fx += fw + 8.75f;
        }

        std::string todayStr = getCurrentDateYYYYMMDD();
        std::vector<OrderRecord> cashOrders;
        int cashTotal = 0;
        for (const auto& o : orders) {
            std::string modeLower = toLowerStr(o.paymentMode);
            if (toLowerStr(o.orderStatus) == "paid" && modeLower.find("cash") != std::string::npos) {
                if (salesTimeFilter == 0) {
                    if (o.orderDate != todayStr) continue;
                } else if (salesTimeFilter == 1) {
                    if (!isWithinDays(o.orderDate, 7)) continue;
                } else if (salesTimeFilter == 2) {
                    if (!isWithinDays(o.orderDate, 30)) continue;
                } else if (salesTimeFilter == 3) {
                    if (!isWithinDays(o.orderDate, 365)) continue;
                } else if (salesTimeFilter == 4) {
                    if (!customStartDate.empty() && o.orderDate < customStartDate) continue;
                    if (!customEndDate.empty() && o.orderDate > customEndDate) continue;
                }

                cashOrders.push_back(o);
                cashTotal += o.paymentAmount;
            }
        }

        float kpiY = 126.f;
        float actualTy = 158.f;
        float tableHeight = 502.f;

        // If Custom Range is selected, draw date input boxes inside a clean Paper bar
        if (salesTimeFilter == 4) {
            float customBoxY = 126.f;
            drawRounded(target, mainX, customBoxY - 2.f, Layout::MAIN_W, 28.f, 6, ThemeColor::Paper, ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, "From:", 8.5f, ThemeColor::Mute, mainX + 8.f, customBoxY + 12.f, TextAlignment::Left);
            float b1X = mainX + 42.f;
            drawRounded(target, b1X, customBoxY + 1.f, 84.f, 22.f, 4,
                        customActiveField == 1 ? sf::Color(254, 249, 195) : sf::Color(248, 244, 236),
                        customActiveField == 1 ? ThemeColor::Blue : ThemeColor::Edge, 1.f);
            std::string dispStart = (customStartDate.empty() && customActiveField != 1) ? "YYYY-MM-DD" : (customStartDate + (customActiveField == 1 ? "|" : ""));
            drawText(target, fontMedium, dispStart, 8.f,
                     customStartDate.empty() ? ThemeColor::Mute : ThemeColor::Ink, b1X + 42.f, customBoxY + 12.f, TextAlignment::Center);

            drawText(target, fontSemiBold, "To:", 8.5f, ThemeColor::Mute, mainX + 132.f, customBoxY + 12.f, TextAlignment::Left);
            float b2X = mainX + 152.f;
            drawRounded(target, b2X, customBoxY + 1.f, 84.f, 22.f, 4,
                        customActiveField == 2 ? sf::Color(254, 249, 195) : sf::Color(248, 244, 236),
                        customActiveField == 2 ? ThemeColor::Blue : ThemeColor::Edge, 1.f);
            std::string dispEnd = (customEndDate.empty() && customActiveField != 2) ? "YYYY-MM-DD" : (customEndDate + (customActiveField == 2 ? "|" : ""));
            drawText(target, fontMedium, dispEnd, 8.f,
                     customEndDate.empty() ? ThemeColor::Mute : ThemeColor::Ink, b2X + 42.f, customBoxY + 12.f, TextAlignment::Center);

            float p1X = mainX + 246.f;
            bool hovP1 = pointInRect(mouse, p1X, customBoxY + 1.f, 92.f, 22.f);
            drawRounded(target, p1X, customBoxY + 1.f, 92.f, 22.f, 4, hovP1 ? sf::Color(220, 230, 245) : sf::Color(235, 242, 250), ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, "Nov 17 - 25", 8.f, ThemeColor::Ink, p1X + 46.f, customBoxY + 12.f, TextAlignment::Center);

            float p2X = p1X + 98.f;
            bool hovP2 = pointInRect(mouse, p2X, customBoxY + 1.f, 96.f, 22.f);
            drawRounded(target, p2X, customBoxY + 1.f, 96.f, 22.f, 4, hovP2 ? sf::Color(220, 230, 245) : sf::Color(235, 242, 250), ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, "Full Year 2026", 8.f, ThemeColor::Ink, p2X + 48.f, customBoxY + 12.f, TextAlignment::Center);

            float p3X = p2X + 102.f;
            bool hovP3 = pointInRect(mouse, p3X, customBoxY + 1.f, 90.f, 22.f);
            drawRounded(target, p3X, customBoxY + 1.f, 90.f, 22.f, 4, hovP3 ? sf::Color(240, 220, 220) : sf::Color(250, 235, 235), ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, "All Dates", 8.f, sf::Color(180, 50, 50), p3X + 45.f, customBoxY + 12.f, TextAlignment::Center);

            kpiY = 158.f;
            actualTy = 190.f;
            tableHeight = 470.f;
        }

        // Summary KPI bar
        drawRounded(target, mainX, kpiY, Layout::MAIN_W, 26, 6, sf::Color(240, 248, 240), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "Cash Sales Volume: " + formatRs(cashTotal), 10.f, sf::Color(35, 120, 50), mainX + 12, kpiY + 13);
        std::string filterDesc = (salesTimeFilter == 0) ? ("Today " + todayStr) : timeLabels[salesTimeFilter % 5];
        if (salesTimeFilter == 4) {
            filterDesc = (customStartDate.empty() ? "start" : customStartDate) + " to " + (customEndDate.empty() ? "now" : customEndDate);
        }
        drawText(target, fontSemiBold, "Filtered: " + std::to_string(cashOrders.size()) + " txns (" + filterDesc + ")", 8.5f, ThemeColor::Mute, mainX + Layout::MAIN_W - 12, kpiY + 13, TextAlignment::Right);

        drawRounded(target, mainX, actualTy, Layout::MAIN_W, tableHeight, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Order ID", 9.f, ThemeColor::Mute, mainX + 15, actualTy + 14);
        drawText(target, fontSemiBold, "Date", 9.f, ThemeColor::Mute, mainX + 90, actualTy + 14);
        drawText(target, fontSemiBold, "Time", 9.f, ThemeColor::Mute, mainX + 165, actualTy + 14);
        drawText(target, fontSemiBold, "Customer", 9.f, ThemeColor::Mute, mainX + 235, actualTy + 14);
        drawText(target, fontSemiBold, "Terminal", 9.f, ThemeColor::Mute, mainX + 335, actualTy + 14);
        drawText(target, fontSemiBold, "Cash Settled", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 15, actualTy + 14, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, actualTy + 26);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        if (cashOrders.empty()) {
            drawText(target, fontRegular, "No cash sales found for selected period (" + filterDesc + ").", 11.f, ThemeColor::Mute,
                     mainX + 15, actualTy + 46);
        }

        for (size_t i = 0; i < cashOrders.size(); i++) {
            float rowY = actualTy + 38.f + static_cast<float>(i) * 36.f - scrollOffset;
            if (rowY < actualTy + 30.f || rowY + 22.f > actualTy + tableHeight - 44.f) continue;
            const auto& o = cashOrders[cashOrders.size() - 1 - i];

            drawText(target, fontBold, o.orderId, 9.f, ThemeColor::Ink, mainX + 15, rowY + 6);
            drawText(target, fontRegular, o.orderDate, 8.5f, ThemeColor::Mute, mainX + 90, rowY + 6);
            drawText(target, fontRegular, o.orderTime, 8.5f, ThemeColor::Mute, mainX + 165, rowY + 6);
            drawText(target, fontMedium, cleanDisplayStr(o.customerName), 9.f, ThemeColor::Ink, mainX + 235, rowY + 6);
            drawText(target, fontRegular, cleanDisplayStr(o.paymentMerchant), 8.5f, ThemeColor::Mute, mainX + 335, rowY + 6);
            drawText(target, fontBold, formatRs(o.paymentAmount), 10.5f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 15, rowY + 6, TextAlignment::Right);

            sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
            rLine.setPosition(mainX + 15, rowY + 22);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }

        // Pinned Grand Total Footer
        float footerY = actualTy + tableHeight - 40.f;
        drawRounded(target, mainX + 2, footerY, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "GRAND TOTAL CASH SALES", 10.f, ThemeColor::Ink, mainX + 15, footerY + 19.f);
        drawText(target, fontSemiBold, std::to_string(cashOrders.size()) + " Cash Orders Settled", 9.5f, ThemeColor::Mute, mainX + 240, footerY + 19.f);
        drawText(target, fontBold, formatRs(cashTotal), 12.f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 15, footerY + 19.f, TextAlignment::Right);
    }
    // =========================================================================
    // REPORT 4: ALL SALES & DISCOUNTS AUDIT REPORT
    // =========================================================================
    else if (salesSubTab == 3) {
        int grossSum = 0, taxSum = 0, prodDiscSum = 0, seasonDiscSum = 0, netSum = 0;
        for (const auto& b : bills) {
            grossSum += b.totalWithoutTax;
            taxSum += b.totalTax;
            prodDiscSum += b.totalDiscount;
            seasonDiscSum += (b.seasonalDiscount + b.specialDiscount);
            netSum += b.totalPayable;
        }

        // Summary KPI Cards
        float card4W = (Layout::MAIN_W - 30.f) / 4.f;
        drawRounded(target, mainX, cardY, card4W, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Gross Amount", 8.f, ThemeColor::Mute, mainX + 10, cardY + 10);
        drawText(target, fontBold, formatRs(grossSum), 11.5f, ThemeColor::Ink, mainX + 10, cardY + 26);
        drawText(target, fontRegular, "Tax: " + formatRs(taxSum), 7.5f, ThemeColor::Mute, mainX + 10, cardY + 41);

        float c2X = mainX + card4W + 10.f;
        drawRounded(target, c2X, cardY, card4W, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Product Disc", 8.f, ThemeColor::Mute, c2X + 10, cardY + 10);
        drawText(target, fontBold, formatRs(prodDiscSum), 11.5f, sf::Color(220, 70, 60), c2X + 10, cardY + 26);
        drawText(target, fontRegular, "Product specific", 7.5f, ThemeColor::Mute, c2X + 10, cardY + 41);

        float c3X = c2X + card4W + 10.f;
        drawRounded(target, c3X, cardY, card4W, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Seasonal & Spec", 8.f, ThemeColor::Mute, c3X + 10, cardY + 10);
        drawText(target, fontBold, formatRs(seasonDiscSum), 11.5f, sf::Color(220, 70, 60), c3X + 10, cardY + 26);
        drawText(target, fontRegular, "Campaign & special", 7.5f, ThemeColor::Mute, c3X + 10, cardY + 41);

        float c4X = c3X + card4W + 10.f;
        drawRounded(target, c4X, cardY, card4W, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Net Bill Revenue", 8.f, ThemeColor::Mute, c4X + 10, cardY + 10);
        drawText(target, fontBold, formatRs(netSum), 11.5f, sf::Color(35, 120, 50), c4X + 10, cardY + 26);
        drawText(target, fontRegular, "Final collected sum", 7.5f, ThemeColor::Mute, c4X + 10, cardY + 41);

        // Tabular View
        drawRounded(target, mainX, ty, Layout::MAIN_W, 502, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Bill ID", 9.f, ThemeColor::Mute, mainX + 12, ty + 14);
        drawText(target, fontSemiBold, "Order Ref", 9.f, ThemeColor::Mute, mainX + 78, ty + 14);
        drawText(target, fontSemiBold, "Customer", 9.f, ThemeColor::Mute, mainX + 145, ty + 14);
        drawText(target, fontSemiBold, "Gross", 9.f, ThemeColor::Mute, mainX + 235, ty + 14);
        drawText(target, fontSemiBold, "Prod Disc", 9.f, ThemeColor::Mute, mainX + 310, ty + 14);
        drawText(target, fontSemiBold, "Season/Spec", 9.f, ThemeColor::Mute, mainX + 385, ty + 14);
        drawText(target, fontSemiBold, "Net Bill", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 12, ty + 14, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 26);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        for (size_t i = 0; i < bills.size(); i++) {
            float rowY = ty + 38.f + static_cast<float>(i) * 36.f - scrollOffset;
            if (rowY < ty + 30.f || rowY + 22.f > ty + 458.f) continue;
            const auto& b = bills[bills.size() - 1 - i];

            drawText(target, fontBold, b.billId, 8.5f, ThemeColor::Ink, mainX + 12, rowY + 6);
            drawText(target, fontRegular, b.orderId, 8.f, ThemeColor::Mute, mainX + 78, rowY + 6);
            std::string bCust = cleanDisplayStr(b.customerName);
            if (bCust.length() > 13) bCust = bCust.substr(0, 11) + "..";
            drawText(target, fontMedium, bCust, 8.5f, ThemeColor::Ink, mainX + 145, rowY + 6);
            drawText(target, fontBold, formatRs(b.totalWithoutTax), 8.5f, ThemeColor::Ink, mainX + 235, rowY + 6);

            drawText(target, fontMedium, (b.totalDiscount > 0 ? ("-" + formatRs(b.totalDiscount)) : "Rs 0"), 8.5f,
                     b.totalDiscount > 0 ? sf::Color(220, 70, 60) : ThemeColor::Mute, mainX + 310, rowY + 6);

            int sDisc = b.seasonalDiscount + b.specialDiscount;
            drawText(target, fontMedium, (sDisc > 0 ? ("-" + formatRs(sDisc)) : "Rs 0"), 8.5f,
                     sDisc > 0 ? sf::Color(220, 70, 60) : ThemeColor::Mute, mainX + 385, rowY + 6);

            drawText(target, fontBold, formatRs(b.totalPayable), 10.f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 12, rowY + 6, TextAlignment::Right);

            sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
            rLine.setPosition(mainX + 15, rowY + 22);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 462.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "GRAND TOTALS", 9.5f, ThemeColor::Ink, mainX + 15, ty + 481.f);
        drawText(target, fontBold, "Gross: " + formatRs(grossSum) + "  |  Disc: -" + formatRs(prodDiscSum + seasonDiscSum), 8.5f, ThemeColor::Ink, mainX + 115, ty + 481.f);
        drawText(target, fontBold, "Net: " + formatRs(netSum), 11.5f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 15, ty + 481.f, TextAlignment::Right);
    }
    // =========================================================================
    // REPORT 5: TOP 10 MOST SOLD ITEMS (Without double counting)
    // =========================================================================
    else if (salesSubTab == 4) {
        std::set<std::string> paidOrderIds;
        for (const auto& o : orders) {
            if (toLowerStr(o.orderStatus) == "paid") paidOrderIds.insert(o.orderId);
        }

        std::map<std::string, int> unitsSoldByBarcode;
        std::set<std::string> ordersWithDetails;
        for (const auto& od : orderDetails) {
            if (paidOrderIds.empty() || paidOrderIds.count(od.orderId)) {
                unitsSoldByBarcode[od.productBarcode] += od.quantity;
                ordersWithDetails.insert(od.orderId);
            }
        }
        // Fallback to BillDetail only for paid bills whose orderId lacked OrderDetail lines
        std::map<std::string, int> unitsSoldByAcronym;
        for (const auto& b : bills) {
            if (toLowerStr(b.paymentStatus) == "paid" && !ordersWithDetails.count(b.orderId)) {
                for (const auto& d : billDetails) {
                    if (d.billId == b.billId) {
                        unitsSoldByAcronym[d.productAcronym] += d.quantity;
                    }
                }
            }
        }

        struct SoldRankItem {
            std::string code;
            std::string name;
            std::string category;
            int price = 0;
            int unitsSold = 0;
            int stock = 0;
        };

        std::vector<SoldRankItem> ranked;
        for (const auto& p : products) {
            SoldRankItem item;
            item.code = p.acronym.empty() ? p.barcode : p.acronym;
            item.name = p.name;
            item.category = p.category;
            item.price = p.price;
            item.stock = p.stock;
            item.unitsSold = unitsSoldByBarcode[p.barcode] + unitsSoldByAcronym[item.code];
            ranked.push_back(item);
        }

        std::sort(ranked.begin(), ranked.end(), [](const SoldRankItem& a, const SoldRankItem& b) {
            return a.unitsSold > b.unitsSold;
        });

        size_t limit = std::min<size_t>(10, ranked.size());
        int top10Units = 0;
        int top10Revenue = 0;
        for (size_t i = 0; i < limit; i++) {
            top10Units += ranked[i].unitsSold;
            top10Revenue += (ranked[i].unitsSold * ranked[i].price);
        }

        drawRounded(target, mainX, cardY, Layout::MAIN_W, 52, 8, sf::Color(240, 245, 255), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "Top 10 High-Velocity Products (Descending by Sold Qty)", 10.5f, sf::Color(29, 78, 216), mainX + 15, cardY + 16);
        drawText(target, fontRegular, "Fast-moving inventory with remaining stock levels for proactive replenishment", 8.5f, ThemeColor::Mute, mainX + 15, cardY + 35);

        drawRounded(target, mainX, ty, Layout::MAIN_W, 502, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Rank", 9.f, ThemeColor::Mute, mainX + 15, ty + 14);
        drawText(target, fontSemiBold, "Code", 9.f, ThemeColor::Mute, mainX + 55, ty + 14);
        drawText(target, fontSemiBold, "Product Name", 9.f, ThemeColor::Mute, mainX + 115, ty + 14);
        drawText(target, fontSemiBold, "Category", 9.f, ThemeColor::Mute, mainX + 245, ty + 14);
        drawText(target, fontSemiBold, "Qty Sold", 9.f, ThemeColor::Mute, mainX + 335, ty + 14);
        drawText(target, fontSemiBold, "In Stock", 9.f, ThemeColor::Mute, mainX + 400, ty + 14);
        drawText(target, fontSemiBold, "Revenue", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 15, ty + 14, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 26);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        for (size_t i = 0; i < limit; i++) {
            float rowY = ty + 38.f + static_cast<float>(i) * 38.f;
            const auto& r = ranked[i];

            std::string rankStr = "#" + std::to_string(i + 1);
            drawText(target, fontBold, rankStr, 10.f, (i < 3) ? sf::Color(217, 119, 6) : ThemeColor::Ink, mainX + 15, rowY + 6);
            drawText(target, fontMedium, r.code, 9.f, ThemeColor::Ink, mainX + 55, rowY + 6);
            std::string rName = r.name;
            if (rName.length() > 17) rName = rName.substr(0, 15) + "..";
            drawText(target, fontBold, rName, 9.5f, ThemeColor::Ink, mainX + 115, rowY + 6);
            std::string rCat = r.category;
            if (rCat.length() > 12) rCat = rCat.substr(0, 10) + "..";
            drawText(target, fontRegular, rCat, 8.5f, ThemeColor::Mute, mainX + 245, rowY + 6);

            drawRounded(target, mainX + 330, rowY - 3, 40, 18, 4, sf::Color(220, 235, 250));
            drawText(target, fontBold, std::to_string(r.unitsSold), 9.f, sf::Color(29, 78, 216), mainX + 350, rowY + 6, TextAlignment::Center);

            bool low = r.stock < 15;
            drawText(target, fontBold, std::to_string(r.stock) + " in stock", 9.f,
                     r.stock == 0 ? ThemeColor::Red : (low ? ThemeColor::Amber : ThemeColor::Green),
                     mainX + 400, rowY + 6);

            int itemRev = r.unitsSold * r.price;
            drawText(target, fontBold, formatRs(itemRev), 10.5f, ThemeColor::Ink, mainX + Layout::MAIN_W - 15, rowY + 6, TextAlignment::Right);

            sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
            rLine.setPosition(mainX + 15, rowY + 24);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 462.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "TOP 10 AGGREGATE TOTALS", 9.5f, ThemeColor::Ink, mainX + 15, ty + 481.f);
        drawText(target, fontBold, "Units Sold: " + std::to_string(top10Units) + " units", 10.f, sf::Color(29, 78, 216), mainX + 220, ty + 481.f);
        drawText(target, fontBold, "Revenue: " + formatRs(top10Revenue), 11.5f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 15, ty + 481.f, TextAlignment::Right);
    }
    // =========================================================================
    // REPORT 6: TOP 10 OVERSTOCKED ITEMS (Top 10 by Remaining Stock)
    // =========================================================================
    else if (salesSubTab == 5) {
        auto sortedProds = products;
        std::sort(sortedProds.begin(), sortedProds.end(), [](const ProductItem& a, const ProductItem& b) {
            return a.stock > b.stock;
        });

        size_t limit = std::min<size_t>(10, sortedProds.size());
        int totalSurplusUnits = 0;
        int totalCapitalTied = 0;
        for (size_t i = 0; i < limit; i++) {
            totalSurplusUnits += sortedProds[i].stock;
            totalCapitalTied += (sortedProds[i].stock * sortedProds[i].price);
        }

        drawRounded(target, mainX, cardY, Layout::MAIN_W, 52, 8, sf::Color(254, 243, 199), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "Top 10 High Remaining Stock (Hold New Purchases)", 10.5f, sf::Color(180, 83, 9), mainX + 15, cardY + 16);
        drawText(target, fontRegular, "Abundant remaining stock detected; avoid initiating purchase orders for these SKUs", 8.5f, ThemeColor::Mute, mainX + 15, cardY + 35);

        drawRounded(target, mainX, ty, Layout::MAIN_W, 502, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Rank", 9.f, ThemeColor::Mute, mainX + 15, ty + 14);
        drawText(target, fontSemiBold, "Barcode", 9.f, ThemeColor::Mute, mainX + 50, ty + 14);
        drawText(target, fontSemiBold, "Product Name", 9.f, ThemeColor::Mute, mainX + 110, ty + 14);
        drawText(target, fontSemiBold, "Category", 9.f, ThemeColor::Mute, mainX + 225, ty + 14);
        drawText(target, fontSemiBold, "Unit Price", 9.f, ThemeColor::Mute, mainX + 300, ty + 14);
        drawText(target, fontSemiBold, "Remaining", 9.f, ThemeColor::Mute, mainX + 365, ty + 14);
        drawText(target, fontSemiBold, "Directive", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 61, ty + 14, TextAlignment::Center);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 26);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        for (size_t i = 0; i < limit; i++) {
            float rowY = ty + 38.f + static_cast<float>(i) * 38.f;
            const auto& p = sortedProds[i];

            std::string rankStr = "#" + std::to_string(i + 1);
            drawText(target, fontBold, rankStr, 10.f, ThemeColor::Ink, mainX + 15, rowY + 7);
            drawText(target, fontMedium, p.barcode, 9.f, ThemeColor::Ink, mainX + 50, rowY + 7);
            std::string pName = p.name;
            if (pName.length() > 16) pName = pName.substr(0, 14) + "..";
            drawText(target, fontBold, pName, 9.5f, ThemeColor::Ink, mainX + 110, rowY + 7);
            std::string pCat = p.category;
            if (pCat.length() > 11) pCat = pCat.substr(0, 9) + "..";
            drawText(target, fontRegular, pCat, 8.5f, ThemeColor::Mute, mainX + 225, rowY + 7);
            drawText(target, fontRegular, formatRs(p.price), 9.f, ThemeColor::Mute, mainX + 300, rowY + 7);

            drawText(target, fontBold, std::to_string(p.stock) + " units", 9.5f, sf::Color(35, 120, 50), mainX + 365, rowY + 7);

            // Directive Badge
            drawRounded(target, mainX + Layout::MAIN_W - 110, rowY - 3, 98, 20, 4, sf::Color(254, 226, 226));
            drawText(target, fontBold, "DO NOT PURCHASE", 7.5f, sf::Color(185, 28, 28), mainX + Layout::MAIN_W - 61, rowY + 7, TextAlignment::Center);

            sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
            rLine.setPosition(mainX + 15, rowY + 24);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 462.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "OVERSTOCK SURPLUS SUMMARY", 9.5f, ThemeColor::Ink, mainX + 15, ty + 481.f);
        drawText(target, fontBold, "Top 10 Stock: " + std::to_string(totalSurplusUnits) + " units", 10.f, sf::Color(180, 83, 9), mainX + 220, ty + 481.f);
        drawText(target, fontBold, "Tied-Up Capital: " + formatRs(totalCapitalTied), 11.5f, sf::Color(220, 70, 60), mainX + Layout::MAIN_W - 15, ty + 481.f, TextAlignment::Right);
    }
    // =========================================================================
    // REPORT 7: CUSTOMER RETURNS AUDIT
    // =========================================================================
    else {
        int totalReturns = (int)returnHistory.size();
        int totalRefundsPKR = 0;
        int totalUnitsRestocked = 0;
        for (const auto& r : returnHistory) {
            totalUnitsRestocked += r.quantity;
            totalRefundsPKR += r.refundAmount;
        }

        drawRounded(target, mainX, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Merchandise Returns", 8.5f, ThemeColor::Mute, mainX + 12, cardY + 10);
        drawText(target, fontBold, std::to_string(totalReturns) + " Claims", 13, sf::Color(220, 90, 40), mainX + 12, cardY + 26);
        drawText(target, fontRegular, "Processed & restocked", 8.f, ThemeColor::Mute, mainX + 12, cardY + 41);

        float card2X = mainX + cardW + 10.f;
        drawRounded(target, card2X, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Total Cash Refunded", 8.5f, ThemeColor::Mute, card2X + 12, cardY + 10);
        drawText(target, fontBold, formatRs(totalRefundsPKR), 13, sf::Color(220, 70, 60), card2X + 12, cardY + 26);
        drawText(target, fontRegular, "Returned to shoppers", 8.f, ThemeColor::Mute, card2X + 12, cardY + 41);

        float card3X = card2X + cardW + 10.f;
        drawRounded(target, card3X, cardY, cardW, 52, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Units Restocked", 8.5f, ThemeColor::Mute, card3X + 12, cardY + 10);
        drawText(target, fontBold, std::to_string(totalUnitsRestocked) + " Units", 13, sf::Color(35, 120, 50), card3X + 12, cardY + 26);
        drawText(target, fontRegular, "Returned to shelves", 8.f, ThemeColor::Mute, card3X + 12, cardY + 41);

        drawRounded(target, mainX, ty, Layout::MAIN_W, 515, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Return Ref", 9.f, ThemeColor::Mute, mainX + 15, ty + 12);
        drawText(target, fontSemiBold, "Timestamp", 9.f, ThemeColor::Mute, mainX + 90, ty + 12);
        drawText(target, fontSemiBold, "Item & Barcode", 9.f, ThemeColor::Mute, mainX + 200, ty + 12);
        drawText(target, fontSemiBold, "Quantity", 9.f, ThemeColor::Mute, mainX + 355, ty + 12);
        drawText(target, fontSemiBold, "Cashier", 9.f, ThemeColor::Mute, mainX + 425, ty + 12);
        drawText(target, fontSemiBold, "Refund Amount", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 15, ty + 12, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 24);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        if (returnHistory.empty()) {
            drawText(target, fontRegular, "No customer returns recorded yet in returns.txt", 12, ThemeColor::Mute,
                     mainX + Layout::MAIN_W / 2.f, ty + 140, TextAlignment::Center);
        } else {
            for (size_t i = 0; i < returnHistory.size(); i++) {
                float rowY = ty + 36.f + static_cast<float>(i) * 36.f - scrollOffset;
                if (rowY < ty + 20.f || rowY > ty + 465.f) continue;
                const auto& r = returnHistory[returnHistory.size() - 1 - i];

                drawText(target, fontBold, r.returnId, 9.f, ThemeColor::Ink, mainX + 15, rowY);
                drawText(target, fontRegular, r.timestamp, 8.5f, ThemeColor::Mute, mainX + 90, rowY);
                drawText(target, fontBold, r.itemName + " (" + r.barcode + ")", 9.5f, ThemeColor::Ink, mainX + 200, rowY);
                drawText(target, fontMedium, std::to_string(r.quantity) + " unit(s)", 9.f, ThemeColor::Mute, mainX + 355, rowY);
                drawText(target, fontRegular, r.cashier, 9.f, ThemeColor::Ink, mainX + 425, rowY);
                drawText(target, fontBold, formatRs(r.refundAmount), 10.5f, sf::Color(220, 70, 60), mainX + Layout::MAIN_W - 15, rowY, TextAlignment::Right);

                sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
                rLine.setPosition(mainX + 15, rowY + 22);
                rLine.setFillColor(ThemeColor::Line);
                target.draw(rLine);
            }
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 475.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "TOTAL REFUNDS PAID", 10.f, ThemeColor::Ink, mainX + 15, ty + 488.f);
        drawText(target, fontSemiBold, std::to_string(totalReturns) + " Claims Processed", 9.5f, ThemeColor::Mute, mainX + 240, ty + 488.f);
        drawText(target, fontBold, formatRs(totalRefundsPKR), 12.f, sf::Color(220, 70, 60), mainX + Layout::MAIN_W - 15, ty + 488.f, TextAlignment::Right);
    }
}

void PosReportsView::drawActivityLogsTab(sf::RenderTarget& target, sf::Vector2f /*mouse*/, float mainX,
                                        const std::vector<ActivityRecord>& logs,
                                        const std::string& logFilter, float scrollOffset,
                                        const sf::Font& fontRegular, const sf::Font& fontMedium,
                                        const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                        const sf::Font& fontSerif) {
    drawText(target, fontSerif, "Activity & Security Audit Logs", 24, ThemeColor::Ink, mainX, 50);

    const char* filters[] = {"ALL", "LOGIN", "SALE", "RETURN", "STAFF", "INVENTORY", "ORDER"};
    for (int i = 0; i < 7; i++) {
        bool active = (logFilter == filters[i]);
        drawRounded(target, mainX + i * 76, 95, 70, 26, 6,
                    active ? ThemeColor::Dark : sf::Color(238, 230, 214),
                    active ? ThemeColor::Dark : ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, filters[i], 9.f,
                 active ? ThemeColor::White : ThemeColor::Ink, mainX + i * 76 + 35, 108, TextAlignment::Center);
    }

    float ty = 135.f;
    drawRounded(target, mainX, ty, Layout::MAIN_W, 525, 10, ThemeColor::Paper, ThemeColor::Line, 1);

    drawText(target, fontSemiBold, "Timestamp", 9.5f, ThemeColor::Mute, mainX + 20, ty + 15);
    drawText(target, fontSemiBold, "Category", 9.5f, ThemeColor::Mute, mainX + 140, ty + 15);
    drawText(target, fontSemiBold, "User / Role", 9.5f, ThemeColor::Mute, mainX + 225, ty + 15);
    drawText(target, fontSemiBold, "Action Details (activity_log.txt)", 9.5f, ThemeColor::Mute, mainX + 315, ty + 15);

    sf::RectangleShape hdrLine({Layout::MAIN_W - 40, 1});
    hdrLine.setPosition(mainX + 20, ty + 28);
    hdrLine.setFillColor(ThemeColor::Line);
    target.draw(hdrLine);

    if (logs.empty()) {
        drawText(target, fontRegular, "No activity records found.", 12, ThemeColor::Mute,
                 mainX + Layout::MAIN_W / 2.f, ty + 160, TextAlignment::Center);
        return;
    }

    int visibleCount = 0;
    for (int i = (int)logs.size() - 1; i >= 0; i--) {
        const auto& log = logs[i];
        if (logFilter != "ALL" && log.category != logFilter) continue;

        float rowY = ty + 42.f + static_cast<float>(visibleCount) * 32.f - scrollOffset;
        visibleCount++;
        if (rowY < ty + 26 || rowY > ty + 505) continue;

        drawText(target, fontRegular, log.timestamp, 9.f, ThemeColor::Mute, mainX + 20, rowY + 3);

        sf::Color catColor = ThemeColor::Ink;
        if (log.category == "LOGIN") catColor = ThemeColor::Blue;
        else if (log.category == "SALE") catColor = ThemeColor::Green;
        else if (log.category == "RETURN") catColor = sf::Color(220, 90, 40);
        else if (log.category == "STAFF") catColor = sf::Color(130, 60, 160);
        else if (log.category == "INVENTORY" || log.category == "PRODUCT") catColor = sf::Color(190, 90, 40);
        else if (log.category == "ORDER") catColor = sf::Color(20, 120, 180);

        drawRounded(target, mainX + 132, rowY - 6, 68, 18, 4, sf::Color(catColor.r, catColor.g, catColor.b, 35));
        drawText(target, fontBold, log.category, 8.f, catColor, mainX + 166, rowY + 3, TextAlignment::Center);

        drawText(target, fontMedium, log.user, 9.5f, ThemeColor::Ink, mainX + 225, rowY + 3);

        std::string safeDetails = log.details;
        if (safeDetails.length() > 50) {
            safeDetails = safeDetails.substr(0, 47) + "...";
        }
        drawText(target, fontRegular, safeDetails, 9.f, ThemeColor::Ink, mainX + 315, rowY + 3);

        sf::RectangleShape sep({Layout::MAIN_W - 40, 1});
        sep.setPosition(mainX + 20, rowY + 22);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);
    }
}

void PosReportsView::drawSettingsTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                    const std::vector<StaffMember>& staffList,
                                    const std::string& currentRole, const std::string& currentUserName,
                                    bool cameraRunning, bool soundEnabled,
                                    const std::string& cameraName, bool isExternalCam, int cameraCount,
                                    int studentDiscount, int employeeDiscount, int promoCount,
                                    int settingsSubTab,
                                    const CategoryManager& categoryMgr,
                                    const SeasonManager& seasonMgr,
                                    float scrollOffset,
                                    const sf::Font& fontRegular, const sf::Font& fontMedium,
                                    const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                    const sf::Font& fontSerif) {
    drawText(target, fontSerif, "System & Staff Settings", 18, ThemeColor::Ink, mainX, 36);

    // Sub-tab pills: [ 1. System & Staff ] [ 2. Product Categories ] [ 3. Seasonal Discounts ]
    if (currentRole == "Admin") {
        const char* sTabs[] = {"Staff & System", "Categories (PCategory.txt)", "Seasons (Seasons.txt)"};
        float subW = (Layout::MAIN_W - 16.f) / 3.f;
        float subY = 56.f;
        for (int sti = 0; sti < 3; sti++) {
            float px = mainX + sti * (subW + 8.f);
            bool active = (settingsSubTab == sti);
            bool hov = pointInRect(mouse, px, subY, subW, 26.f);
            drawRounded(target, px, subY, subW, 26.f, 5,
                        active ? ThemeColor::Dark : (hov ? sf::Color(228, 220, 206) : sf::Color(238, 230, 216)),
                        ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, sTabs[sti], 8.5f,
                     active ? ThemeColor::White : ThemeColor::Ink,
                     px + subW / 2.f, subY + 13.f, TextAlignment::Center);
        }
    }

    float ty = 88.f;

    // =========================================================================
    // SETTINGS SUBTAB 0: SYSTEM & STAFF (Original view)
    // =========================================================================
    if (settingsSubTab == 0) {
        drawRounded(target, mainX, ty, Layout::MAIN_W, 575, 10, ThemeColor::Paper, ThemeColor::Line, 1);

        auto drawSetting = [&](const std::string& title, const std::string& desc, const std::string& val, float y, sf::Color valCol = ThemeColor::Ink, bool isClickable = false) {
            drawText(target, fontSemiBold, title, 11, ThemeColor::Ink, mainX + 25, y);
            drawText(target, fontRegular, desc, 9.f, ThemeColor::Mute, mainX + 25, y + 14);
            bool hov = isClickable && pointInRect(mouse, mainX + Layout::MAIN_W - 145, y - 4, 125, 24);
            drawRounded(target, mainX + Layout::MAIN_W - 145, y - 4, 125, 24, 5,
                        hov ? sf::Color(228, 220, 204) : sf::Color(238, 230, 213), ThemeColor::Edge, 1);
            drawText(target, fontSemiBold, val, 9.f, valCol, mainX + Layout::MAIN_W - 82, y + 7, TextAlignment::Center);
        };

        std::string camStatus = cameraRunning ? (isExternalCam ? "Active (Mobile)" : "Active (Laptop)") : "Offline";
        sf::Color camStatusCol = cameraRunning ? sf::Color(34, 197, 94) : ThemeColor::Red;
        drawSetting("Active Scanner Hardware", cameraName + " (Media Foundation 30 FPS)", camStatus, ty + 14, camStatusCol, true);
        drawSetting("Camera Switcher", (cameraCount > 1) ? "Switch between phone & laptop camera" : "Plug USB camera to enable", (cameraCount > 1) ? "Click / Press 'C'" : "1 Camera Ready", ty + 50, (cameraCount > 1) ? ThemeColor::Blue : ThemeColor::Mute, (cameraCount > 1));
        drawSetting("Logged-in Operator", "Active role & session user", currentUserName + " (" + currentRole + ")", ty + 86, (currentRole == "Admin") ? sf::Color(180, 70, 50) : sf::Color(40, 110, 160));
        drawSetting("Audio Feedback", "Beep sound upon barcode read", soundEnabled ? "Enabled" : "Muted", ty + 122, soundEnabled ? sf::Color(34, 197, 94) : sf::Color(140, 140, 140), true);

        // Staff table
        float staffY = ty + 160.f;
        sf::RectangleShape staffSep({Layout::MAIN_W - 50, 1});
        staffSep.setPosition(mainX + 25, staffY);
        staffSep.setFillColor(ThemeColor::Line);
        target.draw(staffSep);

        if (currentRole == "Admin") {
            drawText(target, fontSemiBold, "Staff & Cashier Operator Accounts (staff.txt)", 11.5f, ThemeColor::Ink, mainX + 25, staffY + 18);

            // "+ Add Employee" button
            bool hovAddStaff = pointInRect(mouse, mainX + Layout::MAIN_W - 135, staffY + 8, 115, 24);
            drawRounded(target, mainX + Layout::MAIN_W - 135, staffY + 8, 115, 24, 5, hovAddStaff ? ThemeColor::BlueHover : ThemeColor::Blue);
            drawText(target, fontBold, "+ Add Employee", 9.f, ThemeColor::White, mainX + Layout::MAIN_W - 77, staffY + 20, TextAlignment::Center);

            float thY = staffY + 42.f;
            drawText(target, fontSemiBold, "Employee ID", 9.f, ThemeColor::Mute, mainX + 30, thY);
            drawText(target, fontSemiBold, "Role", 9.f, ThemeColor::Mute, mainX + 160, thY);
            drawText(target, fontSemiBold, "Security PIN", 9.f, ThemeColor::Mute, mainX + 270, thY);
            drawText(target, fontSemiBold, "Actions", 9.f, ThemeColor::Mute, mainX + 390, thY);

            for (size_t i = 0; i < staffList.size() && i < 4; i++) {
                const auto& s = staffList[i];
                float sRowY = thY + 18.f + static_cast<float>(i) * 28.f;

                drawText(target, fontBold, s.name, 10.f, ThemeColor::Ink, mainX + 30, sRowY + 7);

                bool isAdm = (s.role == "Admin");
                drawRounded(target, mainX + 155, sRowY - 2, 65, 18, 4, isAdm ? sf::Color(254, 226, 226) : sf::Color(220, 235, 248));
                drawText(target, fontSemiBold, s.role, 8.f, isAdm ? sf::Color(180, 50, 40) : sf::Color(30, 95, 150), mainX + 187, sRowY + 7, TextAlignment::Center);

                std::string maskedPass = std::string(s.password.size(), '*');
                if (maskedPass.empty()) maskedPass = "****";
                drawText(target, fontRegular, maskedPass, 10, ThemeColor::Mute, mainX + 270, sRowY + 7);

                // Reset PIN
                drawRounded(target, mainX + 375, sRowY - 2, 65, 18, 4, sf::Color(230, 238, 248), ThemeColor::Blue, 1.f);
                drawText(target, fontSemiBold, "Reset PIN", 8.f, ThemeColor::Blue, mainX + 407, sRowY + 7, TextAlignment::Center);

                if (s.name != currentUserName) {
                    drawRounded(target, mainX + 448, sRowY - 2, 50, 18, 4, sf::Color(254, 226, 226), sf::Color(220, 80, 70), 1.f);
                    drawText(target, fontSemiBold, "Remove", 8.f, ThemeColor::Red, mainX + 473, sRowY + 7, TextAlignment::Center);
                }
            }

            // Discount & Promotional Configuration Section
            float discY = ty + 356.f;
            sf::RectangleShape dSep({Layout::MAIN_W - 50, 1});
            dSep.setPosition(mainX + 25, discY);
            dSep.setFillColor(ThemeColor::Line);
            target.draw(dSep);

            drawText(target, fontSemiBold, "Discount Rates & Promo Vouchers", 12, ThemeColor::Ink, mainX + 25, discY + 18);

            float r1Y = discY + 42.f;
            drawText(target, fontMedium, "Student Discount Rate:", 9.5f, ThemeColor::Mute, mainX + 25, r1Y + 12);
            bool hovSMinus = pointInRect(mouse, mainX + 185, r1Y, 26, 24);
            drawRounded(target, mainX + 185, r1Y, 26, 24, 4, hovSMinus ? sf::Color(228, 220, 206) : ThemeColor::Panel, ThemeColor::Edge, 1);
            drawText(target, fontBold, "-", 11, ThemeColor::Ink, mainX + 198, r1Y + 12, TextAlignment::Center);
            drawText(target, fontBold, std::to_string(studentDiscount) + "%", 10.5f, ThemeColor::Ink, mainX + 230, r1Y + 12, TextAlignment::Center);
            bool hovSPlus = pointInRect(mouse, mainX + 250, r1Y, 26, 24);
            drawRounded(target, mainX + 250, r1Y, 26, 24, 4, hovSPlus ? sf::Color(228, 220, 206) : ThemeColor::Panel, ThemeColor::Edge, 1);
            drawText(target, fontBold, "+", 11, ThemeColor::Ink, mainX + 263, r1Y + 12, TextAlignment::Center);

            float r2Y = discY + 76.f;
            drawText(target, fontMedium, "Employee Discount Rate:", 9.5f, ThemeColor::Mute, mainX + 25, r2Y + 12);
            bool hovEMinus = pointInRect(mouse, mainX + 185, r2Y, 26, 24);
            drawRounded(target, mainX + 185, r2Y, 26, 24, 4, hovEMinus ? sf::Color(228, 220, 206) : ThemeColor::Panel, ThemeColor::Edge, 1);
            drawText(target, fontBold, "-", 11, ThemeColor::Ink, mainX + 198, r2Y + 12, TextAlignment::Center);
            drawText(target, fontBold, std::to_string(employeeDiscount) + "%", 10.5f, ThemeColor::Ink, mainX + 230, r2Y + 12, TextAlignment::Center);
            bool hovEPlus = pointInRect(mouse, mainX + 250, r2Y, 26, 24);
            drawRounded(target, mainX + 250, r2Y, 26, 24, 4, hovEPlus ? sf::Color(228, 220, 206) : ThemeColor::Panel, ThemeColor::Edge, 1);
            drawText(target, fontBold, "+", 11, ThemeColor::Ink, mainX + 263, r2Y + 12, TextAlignment::Center);

            // Manage Promo Codes Button
            float promoBtnW = Layout::MAIN_W - 330.f;
            bool hovPromo = pointInRect(mouse, mainX + 305, r1Y + 14, promoBtnW, 34);
            drawRounded(target, mainX + 305, r1Y + 14, promoBtnW, 34, 6, hovPromo ? ThemeColor::BlueHover : ThemeColor::Blue);
            drawText(target, fontSemiBold, "Manage Promo Codes (" + std::to_string(promoCount) + ")", 9.5f, ThemeColor::White,
                     mainX + 305 + promoBtnW / 2.f, r1Y + 31, TextAlignment::Center);

            // Database Utilities Row
            float utilY = discY + 122.f;
            bool hovRel = pointInRect(mouse, mainX + 25, utilY, 240, 30);
            drawRounded(target, mainX + 25, utilY, 240, 30, 6, hovRel ? sf::Color(232, 224, 210) : ThemeColor::Panel, ThemeColor::Edge, 1);
            drawText(target, fontSemiBold, "Reload Databases from Disk", 9.5f, ThemeColor::Ink, mainX + 145, utilY + 15, TextAlignment::Center);

            bool hovSync = pointInRect(mouse, mainX + 280, utilY, 240, 30);
            drawRounded(target, mainX + 280, utilY, 240, 30, 6, hovSync ? sf::Color(232, 224, 210) : ThemeColor::Panel, ThemeColor::Edge, 1);
            drawText(target, fontSemiBold, "Sync Barcode Bitmaps", 9.5f, ThemeColor::Ink, mainX + 400, utilY + 15, TextAlignment::Center);
        }
    }
    // =========================================================================
    // SETTINGS SUBTAB 1: PRODUCT CATEGORIES (Requirement 1 & PCategory.txt)
    // =========================================================================
    else if (settingsSubTab == 1 && currentRole == "Admin") {
        drawRounded(target, mainX, ty, Layout::MAIN_W, 575, 10, ThemeColor::Paper, ThemeColor::Line, 1);

        drawText(target, fontSemiBold, "Product Category Directory (PCategory.txt)", 12.5f, ThemeColor::Ink, mainX + 25, ty + 20);
        drawText(target, fontRegular, "First 2 digits of generated barcodes encode Category ID", 9.f, ThemeColor::Mute, mainX + 25, ty + 38);

        // "+ Add Category" button
        bool hovAddCat = pointInRect(mouse, mainX + Layout::MAIN_W - 145, ty + 16, 125, 28);
        drawRounded(target, mainX + Layout::MAIN_W - 145, ty + 16, 125, 28, 6, hovAddCat ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontBold, "+ Add Category", 9.5f, ThemeColor::White, mainX + Layout::MAIN_W - 82, ty + 30, TextAlignment::Center);

        float thY = ty + 62.f;
        drawText(target, fontSemiBold, "Cat ID", 9.5f, ThemeColor::Mute, mainX + 30, thY);
        drawText(target, fontSemiBold, "Category Name", 9.5f, ThemeColor::Mute, mainX + 100, thY);
        drawText(target, fontSemiBold, "Acronym", 9.5f, ThemeColor::Mute, mainX + 235, thY);
        drawText(target, fontSemiBold, "Barcode Prefix", 9.5f, ThemeColor::Mute, mainX + 325, thY);
        drawText(target, fontSemiBold, "Action", 9.5f, ThemeColor::Mute, mainX + 465, thY);

        sf::RectangleShape cSep({Layout::MAIN_W - 50, 1});
        cSep.setPosition(mainX + 25, thY + 16);
        cSep.setFillColor(ThemeColor::Line);
        target.draw(cSep);

        const auto& cats = categoryMgr.getCategories();
        for (size_t i = 0; i < cats.size(); i++) {
            float rowY = thY + 28.f + static_cast<float>(i) * 34.f - scrollOffset;
            if (rowY < thY + 20.f || rowY + 24.f > ty + 560.f) continue;
            const auto& c = cats[i];

            // ID badge
            drawRounded(target, mainX + 30, rowY - 2, 42, 20, 4, sf::Color(220, 235, 250));
            drawText(target, fontBold, c.categoryId, 9.5f, sf::Color(29, 78, 216), mainX + 51, rowY + 8, TextAlignment::Center);

            drawText(target, fontBold, c.name, 10.f, ThemeColor::Ink, mainX + 100, rowY + 8);
            drawText(target, fontMedium, c.acronym, 9.5f, ThemeColor::Mute, mainX + 235, rowY + 8);
            drawText(target, fontRegular, c.categoryId + "XXXX (6d)", 9.f, sf::Color(35, 120, 50), mainX + 325, rowY + 8);

            // Delete category button
            bool hovDelC = pointInRect(mouse, mainX + 455, rowY - 2, 56, 20);
            drawRounded(target, mainX + 455, rowY - 2, 56, 20, 4,
                        hovDelC ? sf::Color(254, 226, 226) : sf::Color(245, 238, 230), ThemeColor::Red, 1.f);
            drawText(target, fontSemiBold, "Remove", 8.f, ThemeColor::Red, mainX + 483, rowY + 8, TextAlignment::Center);

            sf::RectangleShape rLine({Layout::MAIN_W - 50, 1});
            rLine.setPosition(mainX + 25, rowY + 24);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }
    }
    // =========================================================================
    // SETTINGS SUBTAB 2: SEASONAL DISCOUNTS (Requirements 13 & 14 & Seasons.txt)
    // =========================================================================
    else if (settingsSubTab == 2 && currentRole == "Admin") {
        drawRounded(target, mainX, ty, Layout::MAIN_W, 575, 10, ThemeColor::Paper, ThemeColor::Line, 1);

        drawText(target, fontSemiBold, "Seasonal Discount Calendars (Seasons.txt)", 12.5f, ThemeColor::Ink, mainX + 25, ty + 20);
        drawText(target, fontRegular, "Active date windows auto-apply seasonal discount at checkout", 9.f, ThemeColor::Mute, mainX + 25, ty + 38);

        // "+ Add Season" button
        bool hovAddSeason = pointInRect(mouse, mainX + Layout::MAIN_W - 145, ty + 16, 125, 28);
        drawRounded(target, mainX + Layout::MAIN_W - 145, ty + 16, 125, 28, 6, hovAddSeason ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontBold, "+ Add Season", 9.5f, ThemeColor::White, mainX + Layout::MAIN_W - 82, ty + 30, TextAlignment::Center);

        float thY = ty + 62.f;
        drawText(target, fontSemiBold, "Start Date", 9.5f, ThemeColor::Mute, mainX + 25, thY);
        drawText(target, fontSemiBold, "End Date", 9.5f, ThemeColor::Mute, mainX + 125, thY);
        drawText(target, fontSemiBold, "Disc %", 9.5f, ThemeColor::Mute, mainX + 225, thY);
        drawText(target, fontSemiBold, "Status", 9.5f, ThemeColor::Mute, mainX + 295, thY);
        drawText(target, fontSemiBold, "Actions", 9.5f, ThemeColor::Mute, mainX + 410, thY);

        sf::RectangleShape sSep({Layout::MAIN_W - 50, 1});
        sSep.setPosition(mainX + 25, thY + 16);
        sSep.setFillColor(ThemeColor::Line);
        target.draw(sSep);

        const auto& seasons = seasonMgr.getSeasons();
        for (size_t i = 0; i < seasons.size(); i++) {
            float rowY = thY + 28.f + static_cast<float>(i) * 36.f - scrollOffset;
            if (rowY < thY + 20.f || rowY + 24.f > ty + 560.f) continue;
            const auto& s = seasons[i];

            drawText(target, fontMedium, s.startDate, 9.5f, ThemeColor::Ink, mainX + 25, rowY + 8);
            drawText(target, fontMedium, s.endDate, 9.5f, ThemeColor::Ink, mainX + 125, rowY + 8);
            drawText(target, fontBold, std::to_string(s.discountPercent) + " %", 10.f, sf::Color(220, 70, 60), mainX + 225, rowY + 8);

            // Status Badge
            bool active = s.isActive;
            drawRounded(target, mainX + 290, rowY - 2, 68, 20, 4, active ? sf::Color(220, 252, 231) : sf::Color(243, 244, 246));
            drawText(target, fontBold, active ? "ACTIVE" : "INACTIVE", 8.f, active ? sf::Color(35, 120, 50) : sf::Color(107, 114, 128), mainX + 324, rowY + 8, TextAlignment::Center);

            // Toggle Button
            bool hovTog = pointInRect(mouse, mainX + 380, rowY - 2, 64, 20);
            drawRounded(target, mainX + 380, rowY - 2, 64, 20, 4,
                        hovTog ? (active ? sf::Color(254, 226, 226) : sf::Color(220, 252, 231)) : sf::Color(235, 230, 220),
                        ThemeColor::Edge, 1.f);
            drawText(target, fontSemiBold, active ? "Disable" : "Enable", 8.f,
                     active ? ThemeColor::Red : sf::Color(35, 120, 50), mainX + 412, rowY + 8, TextAlignment::Center);

            // Delete Button
            bool hovDelS = pointInRect(mouse, mainX + 452, rowY - 2, 58, 20);
            drawRounded(target, mainX + 452, rowY - 2, 58, 20, 4,
                        hovDelS ? sf::Color(254, 226, 226) : sf::Color(245, 238, 230), ThemeColor::Red, 1.f);
            drawText(target, fontSemiBold, "Remove", 8.f, ThemeColor::Red, mainX + 481, rowY + 8, TextAlignment::Center);

            sf::RectangleShape rLine({Layout::MAIN_W - 50, 1});
            rLine.setPosition(mainX + 25, rowY + 24);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }
    }
}

void PosReportsView::drawReturnTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                   const sf::Texture& cameraTexture,
                                   bool barcodeFound, const BarcodeBox& barcodeBox,
                                   int camWidth, int camHeight, const std::string& detectedBarcode,
                                   const std::string& cameraName, bool isExternalCam, int cameraCount,
                                   const std::string& returnBarcode, bool returnBarcodeFocused,
                                   int returnQty, int returnMaxQty, const ProductItem* foundProduct,
                                   const std::string& saleInfo, int salePrice,
                                   const std::string& saleOrderId, const std::string& saleCustomer,
                                   const std::string& saleDate, bool returnSuccess,
                                   const std::string& statusMessage,
                                   int returnReasonIdx, int returnPaymentMethod, bool returnRestock,
                                   const std::vector<OrderRecord>& recentOrders,
                                   const std::vector<std::pair<std::string, std::string>>& orderItemPills,
                                   bool hasReturnSlip,
                                   const sf::Font& fontRegular, const sf::Font& fontMedium,
                                   const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                   const sf::Font& fontSerif,
                                   std::function<const sf::Texture&(const std::string&)> getTexture) {
    drawText(target, fontSerif, "Merchandise Return Terminal", 22, ThemeColor::Ink, mainX, 50);

    float ty = 85.f;
    float colW = (Layout::MAIN_W - 20.f) / 2.f;
    float paneW = colW - 32.f;

    // LEFT COLUMN: Live Camera Scanner, Quick Orders & Lookup
    drawRounded(target, mainX, ty, colW, 575, 10, ThemeColor::Paper, ThemeColor::Line, 1.2f);
    drawText(target, fontBold, "Scanner & Order Lookup", 12.f, ThemeColor::Ink, mainX + 16, ty + 18);

    // Live Camera Viewfinder inside Return Tab
    float camX = mainX + 16.f;
    float camY = ty + 32.f;
    float camW = paneW;
    float camH = 114.f;
    drawRounded(target, camX, camY, camW, camH, 8, sf::Color(16, 16, 20), sf::Color(55, 55, 65), 1.f);

    float paneX = camX + 3.f, paneY = camY + 3.f, cPaneW = camW - 6.f, cPaneH = camH - 6.f;
    float cropX = 0.f, cropY = 0.f, scaleX = 1.f, scaleY = 1.f;
    if (cameraTexture.getSize().x > 0) {
        sf::Sprite camSprite(cameraTexture);
        float srcW = (float)cameraTexture.getSize().x;
        float srcH = (float)cameraTexture.getSize().y;
        float targetAspect = cPaneW / cPaneH;
        float cropH = srcW / targetAspect;
        float cropW = srcW;
        if (cropH <= srcH) {
            cropY = (srcH - cropH) / 2.f;
            camSprite.setTextureRect(sf::IntRect(0, (int)cropY, (int)srcW, (int)cropH));
        } else {
            cropW = srcH * targetAspect;
            cropX = (srcW - cropW) / 2.f;
            camSprite.setTextureRect(sf::IntRect((int)cropX, 0, (int)cropW, (int)srcH));
        }
        scaleX = cPaneW / camSprite.getTextureRect().width;
        scaleY = cPaneH / camSprite.getTextureRect().height;
        camSprite.setPosition(paneX, paneY);
        camSprite.setScale(scaleX, scaleY);
        target.draw(camSprite);
    } else {
        drawRounded(target, paneX, paneY, cPaneW, cPaneH, 6, sf::Color(22, 22, 28));
        drawSprite(target, getTexture("icons/scan.png"), paneX + cPaneW / 2.f - 16.f, paneY + cPaneH / 2.f - 22.f, 32.f, 32.f, sf::Color(200, 200, 200, 160));
        drawText(target, fontSemiBold, "Return Camera Scanner Active", 9.f, sf::Color(230, 230, 230), paneX + cPaneW / 2.f, paneY + cPaneH / 2.f + 18.f, TextAlignment::Center);
    }

    // Reticle corners & red laser line
    float retSize = 14.f, retThick = 2.f, retPad = 7.f;
    sf::Color retColor(34, 197, 94, 210);
    sf::RectangleShape cTL1({retSize, retThick}); cTL1.setPosition(paneX + retPad, paneY + retPad); cTL1.setFillColor(retColor); target.draw(cTL1);
    sf::RectangleShape cTL2({retThick, retSize}); cTL2.setPosition(paneX + retPad, paneY + retPad); cTL2.setFillColor(retColor); target.draw(cTL2);
    sf::RectangleShape cTR1({retSize, retThick}); cTR1.setPosition(paneX + cPaneW - retPad - retSize, paneY + retPad); cTR1.setFillColor(retColor); target.draw(cTR1);
    sf::RectangleShape cTR2({retThick, retSize}); cTR2.setPosition(paneX + cPaneW - retPad - retThick, paneY + retPad); cTR2.setFillColor(retColor); target.draw(cTR2);
    sf::RectangleShape cBL1({retSize, retThick}); cBL1.setPosition(paneX + retPad, paneY + cPaneH - retPad - retThick); cBL1.setFillColor(retColor); target.draw(cBL1);
    sf::RectangleShape cBL2({retThick, retSize}); cBL2.setPosition(paneX + retPad, paneY + cPaneH - retPad - retSize); cBL2.setFillColor(retColor); target.draw(cBL2);
    sf::RectangleShape cBR1({retSize, retThick}); cBR1.setPosition(paneX + cPaneW - retPad - retSize, paneY + cPaneH - retPad - retThick); cBR1.setFillColor(retColor); target.draw(cBR1);
    sf::RectangleShape cBR2({retThick, retSize}); cBR2.setPosition(paneX + cPaneW - retPad - retThick, paneY + cPaneH - retPad - retSize); cBR2.setFillColor(retColor); target.draw(cBR2);

    float sweep = 0.5f + 0.5f * std::sin((float)clock() / CLOCKS_PER_SEC * 2.4f);
    float ly = paneY + 8.f + sweep * (cPaneH - 16.f);
    sf::RectangleShape laser({cPaneW, 1.8f});
    laser.setPosition(paneX, ly);
    laser.setFillColor(sf::Color(255, 50, 45, 220));
    target.draw(laser);

    if (barcodeFound && barcodeBox.detected && camWidth > 0 && camHeight > 0) {
        float bx1 = std::clamp(paneX + (barcodeBox.x1 - cropX) * scaleX, paneX + 2.f, paneX + cPaneW - 18.f);
        float by1 = std::clamp(paneY + (barcodeBox.y1 - cropY) * scaleY, paneY + 2.f, paneY + cPaneH - 18.f);
        float bx2 = std::clamp(paneX + (barcodeBox.x2 - cropX) * scaleX, bx1 + 16.f, paneX + cPaneW - 2.f);
        float by2 = std::clamp(paneY + (barcodeBox.y2 - cropY) * scaleY, by1 + 16.f, paneY + cPaneH - 2.f);
        drawRounded(target, bx1, by1, bx2 - bx1, by2 - by1, 4.f, sf::Color(34, 197, 94, 45), sf::Color(34, 197, 94, 235), 2.f);
    }

    // Status pill & Camera switcher pill inside return camera
    drawRounded(target, camX + 8.f, camY + 7.f, 96.f, 18.f, 4.f, sf::Color(15, 15, 20, 210), sf::Color(255, 255, 255, 45), 1.f);
    drawText(target, fontSemiBold, barcodeFound ? ("LOCK: " + detectedBarcode) : "RETURN SCANNER", 7.f, sf::Color(220, 252, 231), camX + 56.f, camY + 16.f, TextAlignment::Center);

    float camSwW = 110.f;
    float camSwX = camX + camW - camSwW - 8.f;
    drawRounded(target, camSwX, camY + 7.f, camSwW, 18.f, 4.f,
                isExternalCam ? sf::Color(15, 75, 40, 220) : sf::Color(28, 36, 52, 220),
                isExternalCam ? sf::Color(34, 197, 94, 200) : sf::Color(148, 163, 184, 130), 1.f);
    std::string cLbl = (isExternalCam ? "[Phone]" : "[Cam]") + std::string(cameraCount > 1 ? " Switch (C)" : " Ready");
    drawText(target, fontSemiBold, cLbl, 7.f, sf::Color(241, 245, 249), camSwX + camSwW / 2.f, camY + 16.f, TextAlignment::Center);

    // Search Input + Find Button
    float inY = ty + 154.f;
    float searchInputW = paneW - 60.f;
    drawRounded(target, mainX + 16, inY, searchInputW, 30, 6, ThemeColor::Paper,
                returnBarcodeFocused ? ThemeColor::Blue : ThemeColor::Line, 1.5f);
    std::string bDisplay = returnBarcode.empty() ? (returnBarcodeFocused ? "|" : "Barcode / Order ID...")
                                                : (returnBarcode + (returnBarcodeFocused ? "|" : ""));
    drawText(target, fontMedium, bDisplay, 9.f, returnBarcode.empty() ? ThemeColor::Mute : ThemeColor::Ink, mainX + 24, inY + 15);

    float findX = mainX + 16 + searchInputW + 6.f;
    bool hovFind = pointInRect(mouse, findX, inY, 54.f, 30.f);
    drawRounded(target, findX, inY, 54.f, 30.f, 6, hovFind ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontBold, "Find", 9.5f, ThemeColor::White, findX + 27.f, inY + 15, TextAlignment::Center);

    // Quick Samples Pills (using real latest paid order ID for 4th pill)
    float pillY = inY + 36.f;
    drawText(target, fontSemiBold, "Quick Samples:", 8.f, ThemeColor::Mute, mainX + 16, pillY + 4);
    std::string latestPaidId = "10000117";
    for (int i = (int)recentOrders.size() - 1; i >= 0; --i) {
        if (toLowerStr(recentOrders[i].orderStatus) == "paid") {
            latestPaidId = recentOrders[i].orderId;
            break;
        }
    }
    const char* demoCodes[] = {"010001", "030003", "090006", latestPaidId.c_str()};
    float pX = mainX + 16;
    float pW = (paneW - 12.f) / 4.f;
    for (int p = 0; p < 4; p++) {
        bool hovP = pointInRect(mouse, pX, pillY + 13.f, pW, 20.f);
        drawRounded(target, pX, pillY + 13.f, pW, 20.f, 4,
                    hovP ? sf::Color(228, 220, 206) : sf::Color(242, 236, 224), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, demoCodes[p], 7.5f, ThemeColor::Ink, pX + pW / 2.f, pillY + 23.f, TextAlignment::Center);
        pX += pW + 4.f;
    }

    // Verified Sales Orders Ledger (Clickable to initiate return)
    float ordListY = pillY + 42.f;
    drawText(target, fontBold, "Verified Paid Orders (Click to Return):", 9.f, ThemeColor::Ink, mainX + 16, ordListY);

    int maxOrdersToShow = 4;
    int shown = 0;
    for (int i = (int)recentOrders.size() - 1; i >= 0 && shown < maxOrdersToShow; --i) {
        const auto& ord = recentOrders[i];
        if (toLowerStr(ord.orderStatus) != "paid") continue;

        float rRowY = ordListY + 14.f + static_cast<float>(shown) * 48.f;
        shown++;

        bool isSelected = (!saleOrderId.empty() && ord.orderId == saleOrderId);
        bool hovRow = pointInRect(mouse, mainX + 16, rRowY, paneW, 43.f);

        drawRounded(target, mainX + 16, rRowY, paneW, 43.f, 6,
                    isSelected ? sf::Color(230, 240, 252) : (hovRow ? sf::Color(246, 241, 232) : ThemeColor::Panel),
                    isSelected ? ThemeColor::Blue : ThemeColor::Edge, isSelected ? 1.5f : 1.f);

        drawText(target, fontBold, "#" + ord.orderId, 9.5f, isSelected ? ThemeColor::Blue : ThemeColor::Ink, mainX + 24, rRowY + 13);
        drawText(target, fontBold, "Rs " + std::to_string(ord.paymentAmount), 9.5f, sf::Color(35, 120, 50), mainX + 16 + paneW - 10, rRowY + 13, TextAlignment::Right);

        std::string cName = cleanDisplayStr(ord.customerName);
        if (cName.length() > 14) cName = cName.substr(0, 12) + "..";
        drawText(target, fontRegular, cName + " | " + ord.orderDate + " (" + ord.paymentMode + ")", 8.f, ThemeColor::Mute, mainX + 24, rRowY + 30);
    }

    // Bottom Return Policy Guidelines Box
    float polY = ty + 478.f;
    drawRounded(target, mainX + 16, polY, paneW, 84.f, 8, sf::Color(246, 242, 234), ThemeColor::Edge, 1);
    drawText(target, fontBold, "Store Return Policy:", 9.f, ThemeColor::Ink, mainX + 26, polY + 15);
    drawText(target, fontRegular, "- 7-day verified return policy with receipt\n- Deducts already refunded units from limits\n- Restocks product & warehouse inventory",
             8.f, ThemeColor::Mute, mainX + 26, polY + 32);

    // RIGHT COLUMN: Sale Verification, Reasons, Methods & Refund Processing
    float rX = mainX + colW + 20.f;
    drawRounded(target, rX, ty, colW, 575, 10, ThemeColor::Paper, ThemeColor::Line, 1.2f);
    drawText(target, fontBold, "Return & Refund Processing", 12.f, ThemeColor::Ink, rX + 16, ty + 18);

    if (foundProduct != nullptr) {
        // 1. Found Product Card
        float pY = ty + 34.f;
        drawRounded(target, rX + 16, pY, paneW, 48.f, 6, ThemeColor::Panel, ThemeColor::Edge, 1);
        drawText(target, fontBold, foundProduct->name, 11.f, ThemeColor::Ink, rX + 24, pY + 12);
        drawText(target, fontRegular, foundProduct->barcode + " | " + foundProduct->category + " | Stk: " + std::to_string(foundProduct->stock),
                 8.f, ThemeColor::Mute, rX + 24, pY + 27);
        drawText(target, fontSemiBold, "Catalog Unit Price: Rs " + std::to_string(foundProduct->price), 8.5f, sf::Color(35, 120, 50), rX + 24, pY + 40);

        // 2. Verified Purchase Card (with optional multi-item order pills)
        float vY = pY + 54.f;
        bool hasMultiItems = (orderItemPills.size() > 1);
        float vH = hasMultiItems ? 78.f : 54.f;
        drawRounded(target, rX + 16, vY, paneW, vH, 6, sf::Color(243, 247, 243), sf::Color(187, 222, 190), 1);
        drawText(target, fontSemiBold, "Verified Purchase Record:", 8.5f, sf::Color(22, 101, 52), rX + 24, vY + 12);
        std::string vCust = saleCustomer.empty() ? "Customer" : saleCustomer;
        if (vCust.length() > 18) vCust = vCust.substr(0, 16) + "..";
        drawText(target, fontRegular, "Order: " + (saleOrderId.empty() ? "Store Sale" : saleOrderId) + " | " + vCust,
                 8.f, ThemeColor::Ink, rX + 24, vY + 26);
        drawText(target, fontBold, "Refund Unit: Rs " + std::to_string(salePrice) + " (Eligible: " + std::to_string(returnMaxQty) + ")",
                 8.5f, returnMaxQty > 0 ? sf::Color(22, 101, 52) : ThemeColor::Red, rX + 24, vY + 41);

        if (hasMultiItems) {
            float pillCount = (float)std::min<size_t>(3, orderItemPills.size());
            float mpw = (paneW - 16.f - (pillCount - 1) * 4.f) / pillCount;
            for (size_t pi = 0; pi < orderItemPills.size() && pi < 3; ++pi) {
                float mpx = rX + 24.f + pi * (mpw + 4.f);
                float mpy = vY + 52.f;
                bool activeItem = (orderItemPills[pi].first == foundProduct->barcode);
                drawRounded(target, mpx, mpy, mpw, 18.f, 4,
                            activeItem ? ThemeColor::Blue : ThemeColor::Paper,
                            activeItem ? ThemeColor::Blue : ThemeColor::Edge, 1.f);
                std::string lbl = orderItemPills[pi].second;
                if (lbl.length() > 15) lbl = lbl.substr(0, 13) + "..";
                drawText(target, fontSemiBold, lbl, 7.5f, activeItem ? ThemeColor::White : ThemeColor::Ink,
                         mpx + mpw / 2.f, mpy + 9.f, TextAlignment::Center);
            }
        }

        // 3. Return Quantity Stepper
        float qY = vY + vH + 8.f;
        drawText(target, fontBold, "Quantity to Return:", 9.5f, ThemeColor::Ink, rX + 16, qY + 12);

        bool hovMinus = pointInRect(mouse, rX + 16 + paneW - 90.f, qY, 26.f, 24.f);
        drawRounded(target, rX + 16 + paneW - 90.f, qY, 26.f, 24.f, 4,
                    hovMinus ? sf::Color(228, 220, 206) : sf::Color(240, 235, 224), ThemeColor::Edge, 1);
        drawText(target, fontBold, "-", 12.f, ThemeColor::Ink, rX + 16 + paneW - 77.f, qY + 12, TextAlignment::Center);

        drawRounded(target, rX + 16 + paneW - 60.f, qY, 30.f, 24.f, 4, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontBold, std::to_string(returnQty), 11.f, ThemeColor::Ink, rX + 16 + paneW - 45.f, qY + 12, TextAlignment::Center);

        bool hovPlus = pointInRect(mouse, rX + 16 + paneW - 26.f, qY, 26.f, 24.f);
        drawRounded(target, rX + 16 + paneW - 26.f, qY, 26.f, 24.f, 4,
                    hovPlus ? sf::Color(228, 220, 206) : sf::Color(240, 235, 224), ThemeColor::Edge, 1);
        drawText(target, fontBold, "+", 12.f, ThemeColor::Ink, rX + 16 + paneW - 13.f, qY + 12, TextAlignment::Center);

        // 4. Return Reason Selection
        float reasY = qY + 30.f;
        drawText(target, fontBold, "Return Reason:", 9.f, ThemeColor::Ink, rX + 16, reasY + 4);
        const char* reasons[] = {"Changed Mind", "Defective / Damaged", "Wrong Item", "Expired"};
        for (int ri = 0; ri < 4; ri++) {
            float rbx = rX + 16 + (ri % 2) * (paneW / 2.f);
            float rby = reasY + 16.f + (ri / 2) * 24.f;
            float rbw = (paneW / 2.f) - 4.f;
            bool active = (returnReasonIdx == ri);
            bool hov = pointInRect(mouse, rbx, rby, rbw, 20.f);

            drawRounded(target, rbx, rby, rbw, 20.f, 4,
                        active ? ThemeColor::Dark : (hov ? sf::Color(230, 222, 208) : ThemeColor::Paper),
                        active ? ThemeColor::Dark : ThemeColor::Edge, 1);
            drawText(target, fontSemiBold, reasons[ri], 7.5f,
                     active ? ThemeColor::White : ThemeColor::Ink, rbx + rbw / 2.f, rby + 10.f, TextAlignment::Center);
        }

        // 5. Refund Method Selection
        float mthY = reasY + 66.f;
        drawText(target, fontBold, "Refund Method:", 9.f, ThemeColor::Ink, rX + 16, mthY + 4);
        const char* methods[] = {"Cash", "Card / Original", "Store Credit"};
        for (int mi = 0; mi < 3; mi++) {
            float mbw = (paneW - 8.f) / 3.f;
            float mbx = rX + 16 + mi * (mbw + 4.f);
            float mby = mthY + 16.f;
            bool active = (returnPaymentMethod == mi);
            bool hov = pointInRect(mouse, mbx, mby, mbw, 22.f);

            drawRounded(target, mbx, mby, mbw, 22.f, 4,
                        active ? sf::Color(35, 120, 50) : (hov ? sf::Color(230, 222, 208) : ThemeColor::Paper),
                        active ? sf::Color(35, 120, 50) : ThemeColor::Edge, 1);
            drawText(target, fontSemiBold, methods[mi], 7.5f,
                     active ? ThemeColor::White : ThemeColor::Ink, mbx + mbw / 2.f, mby + 11.f, TextAlignment::Center);
        }

        // 6. Restock Checkbox / Toggle
        float dispY = mthY + 44.f;
        drawRounded(target, rX + 16, dispY + 2.f, 16.f, 16.f, 3,
                    returnRestock ? sf::Color(35, 120, 50) : ThemeColor::Paper, ThemeColor::Edge, 1);
        if (returnRestock) {
            drawSprite(target, getTexture("icons/check.png"), rX + 18.f, dispY + 4.f, 12.f, 12.f, ThemeColor::White);
        }
        drawText(target, fontSemiBold, "Restock item back to shelves & inventory", 8.5f,
                 returnRestock ? ThemeColor::Ink : ThemeColor::Mute, rX + 38.f, dispY + 10.f);

        // 7. Total Refund Amount Banner
        int refundTotal = ((salePrice > 0) ? salePrice : foundProduct->price) * returnQty;
        float refY = dispY + 26.f;
        drawRounded(target, rX + 16, refY, paneW, 38.f, 6, sf::Color(254, 243, 199), sf::Color(245, 158, 11), 1);
        drawText(target, fontSemiBold, "Total Refund Payable:", 9.5f, sf::Color(180, 83, 9), rX + 26, refY + 19);
        drawText(target, fontBold, "Rs " + std::to_string(refundTotal), 13.f, sf::Color(180, 83, 9), rX + 16 + paneW - 12, refY + 19, TextAlignment::Right);

        // 8. Main Action Button: PROCESS RETURN
        float btnY = refY + 44.f;
        bool hovProc = pointInRect(mouse, rX + 16, btnY, paneW, 34.f);
        bool canProcess = (returnMaxQty > 0 && returnQty > 0);
        drawRounded(target, rX + 16, btnY, paneW, 34.f, 6,
                    !canProcess ? sf::Color(200, 190, 180) : (hovProc ? sf::Color(185, 28, 28) : ThemeColor::Red));
        drawText(target, fontBold, "PROCESS RETURN & REFUND", 10.f, ThemeColor::White, rX + 16 + paneW / 2.f, btnY + 17, TextAlignment::Center);

        // 9. View Return Slip Button
        if (hasReturnSlip) {
            float slipY = btnY + 38.f;
            bool hovSlip = pointInRect(mouse, rX + 16, slipY, paneW, 24.f);
            drawRounded(target, rX + 16, slipY, paneW, 24.f, 4,
                        hovSlip ? ThemeColor::BlueHover : sf::Color(230, 240, 250), ThemeColor::Blue, 1.f);
            drawText(target, fontSemiBold, "Print / View Return Slip (Proof)", 8.f,
                     hovSlip ? ThemeColor::White : ThemeColor::Blue, rX + 16 + paneW / 2.f, slipY + 12.f, TextAlignment::Center);
        }
    } else {
        // Clean top-aligned workflow steps (no floating middle placeholder card)
        float guideY = ty + 40.f;
        drawRounded(target, rX + 16, guideY, paneW, 138.f, 8, sf::Color(248, 244, 236), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, "Return Workflow Steps", 10.5f, ThemeColor::Ink, rX + 26, guideY + 18.f);
        drawText(target, fontRegular, "1. Scan product barcode via live camera\n2. Or enter Barcode / Order ID & click Find\n3. Or click any Verified Paid Order on left\n4. Verify qty, reason, & process refund",
                 8.5f, ThemeColor::Mute, rX + 26, guideY + 42.f);

        float splBtnY = guideY + 98.f;
        bool hovSpl = pointInRect(mouse, rX + 26, splBtnY, paneW - 20.f, 28.f);
        drawRounded(target, rX + 26, splBtnY, paneW - 20.f, 28.f, 5,
                    hovSpl ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontBold, "Load Sample Item (010001)", 9.f, ThemeColor::White, rX + 16 + paneW / 2.f, splBtnY + 14.f, TextAlignment::Center);
    }

    // Status Message Box at bottom (safely clipped so it never overflows box)
    if (!statusMessage.empty()) {
        float stY = ty + 532.f;
        drawRounded(target, rX + 16, stY, paneW, 30.f, 6,
                    returnSuccess ? sf::Color(220, 252, 231) : sf::Color(254, 226, 226),
                    returnSuccess ? sf::Color(134, 239, 172) : sf::Color(252, 165, 165), 1.f);
        std::string stDisp = statusMessage;
        if (stDisp.length() > 42) stDisp = stDisp.substr(0, 39) + "...";
        drawText(target, fontSemiBold, stDisp, 8.f,
                 returnSuccess ? sf::Color(22, 101, 52) : sf::Color(185, 28, 28),
                 rX + 16 + paneW / 2.f, stY + 15.f, TextAlignment::Center);
    }
}

#include "PosReportsView.h"
#include "UIUtils.h"
#include <ctime>
#include <algorithm>
#include <map>

// Helper to check date window (e.g. within N days)
static bool isWithinDays(const std::string& dateStr, int maxDays) {
    if (dateStr.length() < 10) return false;
    std::tm tOrder = {};
    try {
        tOrder.tm_year = std::stoi(dateStr.substr(0, 4)) - 1900;
        tOrder.tm_mon  = std::stoi(dateStr.substr(5, 2)) - 1;
        tOrder.tm_mday = std::stoi(dateStr.substr(8, 2));
    } catch (...) { return false; }

    std::time_t now = std::time(nullptr);
    std::time_t oTime = std::mktime(&tOrder);
    if (oTime == (std::time_t)-1) return false;

    double diffSec = std::difftime(now, oTime);
    double diffDays = diffSec / 86400.0;
    return (diffDays >= 0.0 && diffDays <= static_cast<double>(maxDays));
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
    float cardY = 100.f;
    float cardW = (Layout::MAIN_W - 20.f) / 3.f;

    // =========================================================================
    // ISE-99: REPORT 1: INCOMPLETE ORDERS REPORT (Unpaid / On-Hold / Pending)
    // =========================================================================
    if (salesSubTab == 0) {
        int incompCount = 0;
        int incompTotal = 0;
        std::vector<OrderRecord> incompOrders;
        for (const auto& o : orders) {
            if (o.orderStatus != "paid" && o.orderStatus != "archived") {
                incompOrders.push_back(o);
                incompCount++;
                incompTotal += o.paymentAmount;
            }
        }

        // Summary KPI Cards
        drawRounded(target, mainX, cardY, cardW, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Incomplete Orders", 8.5f, ThemeColor::Mute, mainX + 12, cardY + 9);
        drawText(target, fontBold, std::to_string(incompCount) + " Unsettled", 12.5f, sf::Color(217, 119, 6), mainX + 12, cardY + 24);
        drawText(target, fontRegular, "Not paid or archived", 7.5f, ThemeColor::Mute, mainX + 12, cardY + 38);

        float card2X = mainX + cardW + 10.f;
        drawRounded(target, card2X, cardY, cardW, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Total Incomplete Amount", 8.5f, ThemeColor::Mute, card2X + 12, cardY + 9);
        drawText(target, fontBold, formatRs(incompTotal), 12.5f, sf::Color(220, 70, 60), card2X + 12, cardY + 24);
        drawText(target, fontRegular, "Pending receivables sum", 7.5f, ThemeColor::Mute, card2X + 12, cardY + 38);

        float card3X = card2X + cardW + 10.f;
        drawRounded(target, card3X, cardY, cardW, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Audit Status", 8.5f, ThemeColor::Mute, card3X + 12, cardY + 9);
        drawText(target, fontBold, incompCount > 0 ? "Fulfill or Cancel" : "Register Clean", 11.5f, incompCount > 0 ? sf::Color(217, 119, 6) : sf::Color(35, 120, 50), card3X + 12, cardY + 24);
        drawText(target, fontRegular, "Audit trail reconciliation", 7.5f, ThemeColor::Mute, card3X + 12, cardY + 38);

        // Tabular View
        drawRounded(target, mainX, ty, Layout::MAIN_W, 502, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Order ID", 9.f, ThemeColor::Mute, mainX + 15, ty + 12);
        drawText(target, fontSemiBold, "Date & Time", 9.f, ThemeColor::Mute, mainX + 85, ty + 12);
        drawText(target, fontSemiBold, "Customer Name", 9.f, ThemeColor::Mute, mainX + 195, ty + 12);
        drawText(target, fontSemiBold, "Contact", 9.f, ThemeColor::Mute, mainX + 285, ty + 12);
        drawText(target, fontSemiBold, "Status", 9.f, ThemeColor::Mute, mainX + 375, ty + 12);
        drawText(target, fontSemiBold, "Pending Amount", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 15, ty + 12, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 24);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        if (incompOrders.empty()) {
            drawText(target, fontRegular, "All orders are fully paid! No incomplete orders found.", 12, ThemeColor::Mute,
                     mainX + Layout::MAIN_W / 2.f, ty + 140, TextAlignment::Center);
        } else {
            for (size_t i = 0; i < incompOrders.size(); i++) {
                float rowY = ty + 36.f + static_cast<float>(i) * 36.f - scrollOffset;
                if (rowY < ty + 20.f || rowY > ty + 455.f) continue;
                const auto& o = incompOrders[i];

                drawText(target, fontBold, o.orderId, 9.f, ThemeColor::Ink, mainX + 15, rowY);
                drawText(target, fontRegular, o.orderDate + " " + o.orderTime, 8.5f, ThemeColor::Mute, mainX + 85, rowY);
                drawText(target, fontMedium, o.customerName, 9.f, ThemeColor::Ink, mainX + 195, rowY);
                drawText(target, fontRegular, o.customerContact.empty() ? "N/A" : o.customerContact, 8.5f, ThemeColor::Mute, mainX + 285, rowY);

                drawRounded(target, mainX + 365, rowY - 2, 54, 18, 4, sf::Color(254, 243, 199));
                drawText(target, fontBold, o.orderStatus, 8.f, sf::Color(180, 83, 9), mainX + 392, rowY + 3, TextAlignment::Center);

                drawText(target, fontBold, formatRs(o.paymentAmount), 10.5f, sf::Color(220, 70, 60), mainX + Layout::MAIN_W - 15, rowY, TextAlignment::Right);

                sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
                rLine.setPosition(mainX + 15, rowY + 22);
                rLine.setFillColor(ThemeColor::Line);
                target.draw(rLine);
            }
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 462.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "GRAND TOTAL SUM (INCOMPLETE ORDERS)", 10.f, ThemeColor::Ink, mainX + 15, ty + 475.f);
        drawText(target, fontSemiBold, std::to_string(incompCount) + " Unsettled Orders", 9.5f, ThemeColor::Mute, mainX + 300, ty + 475.f);
        drawText(target, fontBold, formatRs(incompTotal), 12.f, sf::Color(220, 70, 60), mainX + Layout::MAIN_W - 15, ty + 475.f, TextAlignment::Right);
    }
    // =========================================================================
    // ISE-100: REPORT 2: COMPLETED ORDERS REPORT
    // =========================================================================
    else if (salesSubTab == 1) {
        int compCount = 0;
        int compTotal = 0;
        std::vector<OrderRecord> completedOrders;
        for (const auto& o : orders) {
            if (o.orderStatus == "paid") {
                completedOrders.push_back(o);
                compCount++;
                compTotal += o.paymentAmount;
            }
        }

        // Top KPI Cards
        drawRounded(target, mainX, cardY, cardW, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Completed Orders", 8.5f, ThemeColor::Mute, mainX + 12, cardY + 9);
        drawText(target, fontBold, std::to_string(compCount) + " Orders", 12.5f, sf::Color(35, 120, 50), mainX + 12, cardY + 24);
        drawText(target, fontRegular, "Status: PAID in Order.txt", 7.5f, ThemeColor::Mute, mainX + 12, cardY + 38);

        float card2X = mainX + cardW + 10.f;
        drawRounded(target, card2X, cardY, cardW, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Total Settled Revenue", 8.5f, ThemeColor::Mute, card2X + 12, cardY + 9);
        drawText(target, fontBold, formatRs(compTotal), 12.5f, ThemeColor::Ink, card2X + 12, cardY + 24);
        drawText(target, fontRegular, "Fully paid & reconciled", 7.5f, ThemeColor::Mute, card2X + 12, cardY + 38);

        float card3X = card2X + cardW + 10.f;
        drawRounded(target, card3X, cardY, cardW, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Average Order Value", 8.5f, ThemeColor::Mute, card3X + 12, cardY + 9);
        int aov = compCount > 0 ? (compTotal / compCount) : 0;
        drawText(target, fontBold, formatRs(aov), 12.5f, sf::Color(40, 110, 160), card3X + 12, cardY + 24);
        drawText(target, fontRegular, "Per completed basket", 7.5f, ThemeColor::Mute, card3X + 12, cardY + 38);

        // Tabular View
        drawRounded(target, mainX, ty, Layout::MAIN_W, 502, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Order ID", 9.f, ThemeColor::Mute, mainX + 15, ty + 12);
        drawText(target, fontSemiBold, "Date & Time", 9.f, ThemeColor::Mute, mainX + 85, ty + 12);
        drawText(target, fontSemiBold, "Customer Name", 9.f, ThemeColor::Mute, mainX + 195, ty + 12);
        drawText(target, fontSemiBold, "Payment Mode", 9.f, ThemeColor::Mute, mainX + 285, ty + 12);
        drawText(target, fontSemiBold, "Status", 9.f, ThemeColor::Mute, mainX + 400, ty + 12);
        drawText(target, fontSemiBold, "Settled Total", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 15, ty + 12, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 24);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        for (size_t i = 0; i < completedOrders.size(); i++) {
            float rowY = ty + 36.f + static_cast<float>(i) * 36.f - scrollOffset;
            if (rowY < ty + 20.f || rowY > ty + 455.f) continue;
            const auto& o = completedOrders[completedOrders.size() - 1 - i];

            drawText(target, fontBold, o.orderId, 9.f, ThemeColor::Ink, mainX + 15, rowY);
            drawText(target, fontRegular, o.orderDate + " " + o.orderTime, 8.5f, ThemeColor::Mute, mainX + 85, rowY);
            drawText(target, fontMedium, o.customerName, 9.f, ThemeColor::Ink, mainX + 195, rowY);
            drawText(target, fontRegular, o.paymentMode + " (" + o.paymentMerchant + ")", 8.5f, ThemeColor::Mute, mainX + 285, rowY);

            drawRounded(target, mainX + 395, rowY - 2, 45, 18, 4, sf::Color(220, 252, 231));
            drawText(target, fontBold, "PAID", 8.f, sf::Color(35, 120, 50), mainX + 417, rowY + 3, TextAlignment::Center);

            drawText(target, fontBold, formatRs(o.paymentAmount), 10.5f, ThemeColor::Ink, mainX + Layout::MAIN_W - 15, rowY, TextAlignment::Right);

            sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
            rLine.setPosition(mainX + 15, rowY + 22);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 462.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "GRAND TOTAL SETTLED REVENUE", 10.f, ThemeColor::Ink, mainX + 15, ty + 475.f);
        drawText(target, fontSemiBold, std::to_string(compCount) + " Paid Orders", 9.5f, ThemeColor::Mute, mainX + 270, ty + 475.f);
        drawText(target, fontBold, formatRs(compTotal), 12.f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 15, ty + 475.f, TextAlignment::Right);
    }
    // =========================================================================
    // ISE-101: REPORT 3: CASH SALES AUDIT (Daily, Weekly, Monthly, Yearly, Custom)
    // =========================================================================
    else if (salesSubTab == 2) {
        // Time filter selector
        float fY = 98.f;
        const char* timeLabels[] = {"Daily (Today)", "Weekly (7d)", "Monthly (30d)", "Yearly (365d)", "Custom Range"};
        float fx = mainX;
        for (int fi = 0; fi < 5; fi++) {
            bool active = (salesTimeFilter == fi);
            float fw = 98.f;
            bool hov = pointInRect(mouse, fx, fY, fw, 22.f);
            drawRounded(target, fx, fY, fw, 22.f, 4,
                        active ? ThemeColor::Blue : (hov ? sf::Color(230, 222, 206) : sf::Color(240, 233, 220)),
                        ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, timeLabels[fi], 8.f,
                     active ? ThemeColor::White : ThemeColor::Ink,
                     fx + fw / 2.f, fY + 11.f, TextAlignment::Center);
            fx += fw + 8.f;
        }

        // Find latest date in orders for daily fallback
        std::string latestOrderDate = "";
        for (const auto& o : orders) {
            if (o.orderDate > latestOrderDate) latestOrderDate = o.orderDate;
        }

        std::vector<OrderRecord> cashOrders;
        int cashTotal = 0;
        for (const auto& o : orders) {
            std::string modeLower = o.paymentMode;
            std::transform(modeLower.begin(), modeLower.end(), modeLower.begin(), ::tolower);
            if (o.orderStatus == "paid" && modeLower.find("cash") != std::string::npos) {
                if (salesTimeFilter == 0) {
                    if (!isWithinDays(o.orderDate, 1) && o.orderDate != latestOrderDate) continue;
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

        // If Custom Range is selected, draw date input boxes and presets
        if (salesTimeFilter == 4) {
            float customBoxY = 126.f;
            drawText(target, fontSemiBold, "From:", 9.f, ThemeColor::Mute, mainX, customBoxY + 11.f, TextAlignment::Left);
            float b1X = mainX + 38.f;
            drawRounded(target, b1X, customBoxY, 88.f, 22.f, 4,
                        customActiveField == 1 ? sf::Color(254, 249, 195) : ThemeColor::Paper,
                        customActiveField == 1 ? ThemeColor::Blue : ThemeColor::Line, 1.f);
            drawText(target, fontMedium, customStartDate.empty() ? "YYYY-MM-DD" : customStartDate, 8.5f,
                     customStartDate.empty() ? ThemeColor::Mute : ThemeColor::Ink, b1X + 44.f, customBoxY + 11.f, TextAlignment::Center);

            drawText(target, fontSemiBold, "To:", 9.f, ThemeColor::Mute, mainX + 132.f, customBoxY + 11.f, TextAlignment::Left);
            float b2X = mainX + 154.f;
            drawRounded(target, b2X, customBoxY, 88.f, 22.f, 4,
                        customActiveField == 2 ? sf::Color(254, 249, 195) : ThemeColor::Paper,
                        customActiveField == 2 ? ThemeColor::Blue : ThemeColor::Line, 1.f);
            drawText(target, fontMedium, customEndDate.empty() ? "YYYY-MM-DD" : customEndDate, 8.5f,
                     customEndDate.empty() ? ThemeColor::Mute : ThemeColor::Ink, b2X + 44.f, customBoxY + 11.f, TextAlignment::Center);

            float p1X = mainX + 252.f;
            bool hovP1 = pointInRect(mouse, p1X, customBoxY, 84.f, 22.f);
            drawRounded(target, p1X, customBoxY, 84.f, 22.f, 4, hovP1 ? sf::Color(220, 230, 245) : sf::Color(235, 242, 250), ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, "Nov 17 - 19", 8.f, ThemeColor::Ink, p1X + 42.f, customBoxY + 11.f, TextAlignment::Center);

            float p2X = p1X + 90.f;
            bool hovP2 = pointInRect(mouse, p2X, customBoxY, 84.f, 22.f);
            drawRounded(target, p2X, customBoxY, 84.f, 22.f, 4, hovP2 ? sf::Color(220, 230, 245) : sf::Color(235, 242, 250), ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, "Full Year 2026", 8.f, ThemeColor::Ink, p2X + 42.f, customBoxY + 11.f, TextAlignment::Center);

            float p3X = p2X + 90.f;
            bool hovP3 = pointInRect(mouse, p3X, customBoxY, 78.f, 22.f);
            drawRounded(target, p3X, customBoxY, 78.f, 22.f, 4, hovP3 ? sf::Color(240, 220, 220) : sf::Color(250, 235, 235), ThemeColor::Line, 1.f);
            drawText(target, fontSemiBold, "All Dates", 8.f, sf::Color(180, 50, 50), p3X + 39.f, customBoxY + 11.f, TextAlignment::Center);

            kpiY = 154.f;
            actualTy = 186.f;
            tableHeight = 474.f;
        }

        // Summary KPI bar
        drawRounded(target, mainX, kpiY, Layout::MAIN_W, 26, 6, sf::Color(240, 248, 240), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "Cash Sales Volume: " + formatRs(cashTotal), 10.f, sf::Color(35, 120, 50), mainX + 12, kpiY + 7);
        std::string filterDesc = timeLabels[salesTimeFilter % 5];
        if (salesTimeFilter == 4) {
            filterDesc = (customStartDate.empty() ? "start" : customStartDate) + " to " + (customEndDate.empty() ? "now" : customEndDate);
        }
        drawText(target, fontSemiBold, "Filtered: " + std::to_string(cashOrders.size()) + " transactions (" + filterDesc + ")", 8.5f, ThemeColor::Mute, mainX + 240, kpiY + 7);

        drawRounded(target, mainX, actualTy, Layout::MAIN_W, tableHeight, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Order ID", 9.f, ThemeColor::Mute, mainX + 15, actualTy + 12);
        drawText(target, fontSemiBold, "Date", 9.f, ThemeColor::Mute, mainX + 90, actualTy + 12);
        drawText(target, fontSemiBold, "Time", 9.f, ThemeColor::Mute, mainX + 165, actualTy + 12);
        drawText(target, fontSemiBold, "Customer", 9.f, ThemeColor::Mute, mainX + 235, actualTy + 12);
        drawText(target, fontSemiBold, "Terminal", 9.f, ThemeColor::Mute, mainX + 335, actualTy + 12);
        drawText(target, fontSemiBold, "Cash Settled", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 15, actualTy + 12, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, actualTy + 24);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        for (size_t i = 0; i < cashOrders.size(); i++) {
            float rowY = actualTy + 36.f + static_cast<float>(i) * 36.f - scrollOffset;
            if (rowY < actualTy + 20.f || rowY > actualTy + tableHeight - 48.f) continue;
            const auto& o = cashOrders[cashOrders.size() - 1 - i];

            drawText(target, fontBold, o.orderId, 9.f, ThemeColor::Ink, mainX + 15, rowY);
            drawText(target, fontRegular, o.orderDate, 8.5f, ThemeColor::Mute, mainX + 90, rowY);
            drawText(target, fontRegular, o.orderTime, 8.5f, ThemeColor::Mute, mainX + 165, rowY);
            drawText(target, fontMedium, o.customerName, 9.f, ThemeColor::Ink, mainX + 235, rowY);
            drawText(target, fontRegular, o.paymentMerchant, 8.5f, ThemeColor::Mute, mainX + 340, rowY);
            drawText(target, fontBold, formatRs(o.paymentAmount), 10.5f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 15, rowY, TextAlignment::Right);

            sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
            rLine.setPosition(mainX + 15, rowY + 22);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }

        // Pinned Grand Total Footer
        float footerY = actualTy + tableHeight - 40.f;
        drawRounded(target, mainX + 2, footerY, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "GRAND TOTAL CASH SALES", 10.f, ThemeColor::Ink, mainX + 15, footerY + 13.f);
        drawText(target, fontSemiBold, std::to_string(cashOrders.size()) + " Cash Orders Settled", 9.5f, ThemeColor::Mute, mainX + 240, footerY + 13.f);
        drawText(target, fontBold, formatRs(cashTotal), 12.f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 15, footerY + 13.f, TextAlignment::Right);
    }
    // =========================================================================
    // ISE-102: REPORT 4: ALL SALES & DISCOUNTS AUDIT REPORT
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
        drawRounded(target, mainX, cardY, card4W, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Gross Amount", 8.f, ThemeColor::Mute, mainX + 10, cardY + 9);
        drawText(target, fontBold, formatRs(grossSum), 11.5f, ThemeColor::Ink, mainX + 10, cardY + 24);
        drawText(target, fontRegular, "Pre-discount value", 7.5f, ThemeColor::Mute, mainX + 10, cardY + 38);

        float c2X = mainX + card4W + 10.f;
        drawRounded(target, c2X, cardY, card4W, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Individual Disc", 8.f, ThemeColor::Mute, c2X + 10, cardY + 9);
        drawText(target, fontBold, formatRs(prodDiscSum), 11.5f, sf::Color(220, 70, 60), c2X + 10, cardY + 24);
        drawText(target, fontRegular, "Product specific", 7.5f, ThemeColor::Mute, c2X + 10, cardY + 38);

        float c3X = c2X + card4W + 10.f;
        drawRounded(target, c3X, cardY, card4W, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Seasonal Disc", 8.f, ThemeColor::Mute, c3X + 10, cardY + 9);
        drawText(target, fontBold, formatRs(seasonDiscSum), 11.5f, sf::Color(220, 70, 60), c3X + 10, cardY + 24);
        drawText(target, fontRegular, "Campaign & special", 7.5f, ThemeColor::Mute, c3X + 10, cardY + 38);

        float c4X = c3X + card4W + 10.f;
        drawRounded(target, c4X, cardY, card4W, 48, 8, ThemeColor::Paper, ThemeColor::Line, 1.f);
        drawText(target, fontSemiBold, "Net Bill Revenue", 8.f, ThemeColor::Mute, c4X + 10, cardY + 9);
        drawText(target, fontBold, formatRs(netSum), 11.5f, sf::Color(35, 120, 50), c4X + 10, cardY + 24);
        drawText(target, fontRegular, "Final collected sum", 7.5f, ThemeColor::Mute, c4X + 10, cardY + 38);

        // Tabular View
        drawRounded(target, mainX, ty, Layout::MAIN_W, 502, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Bill ID", 9.f, ThemeColor::Mute, mainX + 12, ty + 12);
        drawText(target, fontSemiBold, "Order Ref", 9.f, ThemeColor::Mute, mainX + 78, ty + 12);
        drawText(target, fontSemiBold, "Customer", 9.f, ThemeColor::Mute, mainX + 145, ty + 12);
        drawText(target, fontSemiBold, "Gross Amount", 9.f, ThemeColor::Mute, mainX + 235, ty + 12);
        drawText(target, fontSemiBold, "Indiv Disc", 9.f, ThemeColor::Mute, mainX + 315, ty + 12);
        drawText(target, fontSemiBold, "Season Disc", 9.f, ThemeColor::Mute, mainX + 385, ty + 12);
        drawText(target, fontSemiBold, "Net Bill", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 12, ty + 12, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 24);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        for (size_t i = 0; i < bills.size(); i++) {
            float rowY = ty + 36.f + static_cast<float>(i) * 36.f - scrollOffset;
            if (rowY < ty + 20.f || rowY > ty + 455.f) continue;
            const auto& b = bills[bills.size() - 1 - i];

            drawText(target, fontBold, b.billId, 8.5f, ThemeColor::Ink, mainX + 12, rowY);
            drawText(target, fontRegular, b.orderId, 8.f, ThemeColor::Mute, mainX + 78, rowY);
            drawText(target, fontMedium, b.customerName, 8.5f, ThemeColor::Ink, mainX + 145, rowY);
            drawText(target, fontBold, formatRs(b.totalWithoutTax), 8.5f, ThemeColor::Ink, mainX + 235, rowY);

            drawText(target, fontMedium, (b.totalDiscount > 0 ? ("-" + formatRs(b.totalDiscount)) : "Rs 0"), 8.5f,
                     b.totalDiscount > 0 ? sf::Color(220, 70, 60) : ThemeColor::Mute, mainX + 315, rowY);

            int sDisc = b.seasonalDiscount + b.specialDiscount;
            drawText(target, fontMedium, (sDisc > 0 ? ("-" + formatRs(sDisc)) : "Rs 0"), 8.5f,
                     sDisc > 0 ? sf::Color(220, 70, 60) : ThemeColor::Mute, mainX + 385, rowY);

            drawText(target, fontBold, formatRs(b.totalPayable), 10.f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 12, rowY, TextAlignment::Right);

            sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
            rLine.setPosition(mainX + 15, rowY + 22);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 462.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "GRAND TOTALS", 9.5f, ThemeColor::Ink, mainX + 15, ty + 475.f);
        drawText(target, fontBold, "Gross: " + formatRs(grossSum) + "  |  Indiv Disc: -" + formatRs(prodDiscSum) + "  |  Season: -" + formatRs(seasonDiscSum), 8.5f, ThemeColor::Ink, mainX + 105, ty + 475.f);
        drawText(target, fontBold, "Net: " + formatRs(netSum), 11.5f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 15, ty + 475.f, TextAlignment::Right);
    }
    // =========================================================================
    // ISE-103: REPORT 5: TOP 10 MOST SOLD ITEMS
    // =========================================================================
    else if (salesSubTab == 4) {
        // Aggregate units sold from billDetails and orderDetails
        std::map<std::string, int> unitsSoldMap;
        for (const auto& d : billDetails) {
            unitsSoldMap[d.productAcronym] += d.quantity;
        }
        for (const auto& od : orderDetails) {
            unitsSoldMap[od.productBarcode] += od.quantity;
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
            const auto* invItem = inventoryMgr.findByBarcode(p.barcode);
            item.stock = invItem ? invItem->stock : p.stock;
            item.unitsSold = unitsSoldMap[item.code];
            if (item.unitsSold == 0 && unitsSoldMap.count(p.barcode)) {
                item.unitsSold = unitsSoldMap[p.barcode];
            }
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

        drawRounded(target, mainX, cardY, Layout::MAIN_W, 48, 8, sf::Color(240, 245, 255), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "Top 10 High-Velocity Products (Ranked Descending by Quantity Sold)", 10.5f, sf::Color(29, 78, 216), mainX + 15, cardY + 14);
        drawText(target, fontRegular, "Essential fast-moving inventory; ensure proactive replenishment to prevent stockouts", 8.5f, ThemeColor::Mute, mainX + 15, cardY + 30);

        drawRounded(target, mainX, ty, Layout::MAIN_W, 502, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Rank", 9.f, ThemeColor::Mute, mainX + 15, ty + 12);
        drawText(target, fontSemiBold, "Code", 9.f, ThemeColor::Mute, mainX + 55, ty + 12);
        drawText(target, fontSemiBold, "Product Name", 9.f, ThemeColor::Mute, mainX + 115, ty + 12);
        drawText(target, fontSemiBold, "Category", 9.f, ThemeColor::Mute, mainX + 245, ty + 12);
        drawText(target, fontSemiBold, "Qty Sold", 9.f, ThemeColor::Mute, mainX + 335, ty + 12);
        drawText(target, fontSemiBold, "In Stock", 9.f, ThemeColor::Mute, mainX + 400, ty + 12);
        drawText(target, fontSemiBold, "Revenue", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 15, ty + 12, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 24);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        for (size_t i = 0; i < limit; i++) {
            float rowY = ty + 36.f + static_cast<float>(i) * 38.f;
            const auto& r = ranked[i];

            std::string rankStr = "#" + std::to_string(i + 1);
            drawText(target, fontBold, rankStr, 10.f, (i < 3) ? sf::Color(217, 119, 6) : ThemeColor::Ink, mainX + 15, rowY);
            drawText(target, fontMedium, r.code, 9.f, ThemeColor::Ink, mainX + 55, rowY);
            drawText(target, fontBold, r.name, 9.5f, ThemeColor::Ink, mainX + 115, rowY);
            drawText(target, fontRegular, r.category, 8.5f, ThemeColor::Mute, mainX + 245, rowY);

            drawRounded(target, mainX + 330, rowY - 2, 40, 18, 4, sf::Color(220, 235, 250));
            drawText(target, fontBold, std::to_string(r.unitsSold), 9.f, sf::Color(29, 78, 216), mainX + 350, rowY + 3, TextAlignment::Center);

            bool low = r.stock < 15;
            drawText(target, fontBold, std::to_string(r.stock) + " in stock", 9.f,
                     r.stock == 0 ? ThemeColor::Red : (low ? ThemeColor::Amber : ThemeColor::Green),
                     mainX + 400, rowY);

            int itemRev = r.unitsSold * r.price;
            drawText(target, fontBold, formatRs(itemRev), 10.5f, ThemeColor::Ink, mainX + Layout::MAIN_W - 15, rowY, TextAlignment::Right);

            sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
            rLine.setPosition(mainX + 15, rowY + 24);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 462.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "TOP 10 AGGREGATE TOTALS", 9.5f, ThemeColor::Ink, mainX + 15, ty + 475.f);
        drawText(target, fontBold, "Units Sold: " + std::to_string(top10Units) + " units", 10.f, sf::Color(29, 78, 216), mainX + 220, ty + 475.f);
        drawText(target, fontBold, "Revenue: " + formatRs(top10Revenue), 11.5f, sf::Color(35, 120, 50), mainX + Layout::MAIN_W - 15, ty + 475.f, TextAlignment::Right);
    }
    // =========================================================================
    // ISE-104: REPORT 6: TOP 10 OVERSTOCKED ITEMS (Top 10 by Remaining Stock)
    // =========================================================================
    else if (salesSubTab == 5) {
        auto invItems = inventoryMgr.getItems();
        std::sort(invItems.begin(), invItems.end(), [](const InventoryItem& a, const InventoryItem& b) {
            return a.stock > b.stock;
        });

        size_t limit = std::min<size_t>(10, invItems.size());
        int totalSurplusUnits = 0;
        int totalCapitalTied = 0;
        for (size_t i = 0; i < limit; i++) {
            totalSurplusUnits += invItems[i].stock;
            totalCapitalTied += (invItems[i].stock * invItems[i].retailPrice);
        }

        drawRounded(target, mainX, cardY, Layout::MAIN_W, 48, 8, sf::Color(254, 243, 199), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "Top 10 High Remaining Stock Items (Procurement Hold: Avoid Unnecessary Purchase Orders)", 10.5f, sf::Color(180, 83, 9), mainX + 15, cardY + 14);
        drawText(target, fontRegular, "Abundant on-hand stock detected; freeze purchase orders for these SKUs to prevent capital lockup", 8.5f, ThemeColor::Mute, mainX + 15, cardY + 30);

        drawRounded(target, mainX, ty, Layout::MAIN_W, 502, 10, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontSemiBold, "Rank", 9.f, ThemeColor::Mute, mainX + 15, ty + 12);
        drawText(target, fontSemiBold, "Barcode", 9.f, ThemeColor::Mute, mainX + 50, ty + 12);
        drawText(target, fontSemiBold, "Product Name", 9.f, ThemeColor::Mute, mainX + 110, ty + 12);
        drawText(target, fontSemiBold, "Category", 9.f, ThemeColor::Mute, mainX + 225, ty + 12);
        drawText(target, fontSemiBold, "Unit Price", 9.f, ThemeColor::Mute, mainX + 300, ty + 12);
        drawText(target, fontSemiBold, "Remaining Stock", 9.f, ThemeColor::Mute, mainX + 360, ty + 12);
        drawText(target, fontSemiBold, "Directive", 9.f, ThemeColor::Mute, mainX + Layout::MAIN_W - 61, ty + 12, TextAlignment::Center);

        sf::RectangleShape sep({Layout::MAIN_W - 30, 1});
        sep.setPosition(mainX + 15, ty + 24);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);

        for (size_t i = 0; i < limit; i++) {
            float rowY = ty + 36.f + static_cast<float>(i) * 38.f;
            const auto& p = invItems[i];

            std::string rankStr = "#" + std::to_string(i + 1);
            drawText(target, fontBold, rankStr, 10.f, ThemeColor::Ink, mainX + 15, rowY);
            drawText(target, fontMedium, p.barcode, 9.f, ThemeColor::Ink, mainX + 50, rowY);
            drawText(target, fontBold, p.name, 9.5f, ThemeColor::Ink, mainX + 110, rowY);
            drawText(target, fontRegular, p.category, 8.5f, ThemeColor::Mute, mainX + 225, rowY);
            drawText(target, fontRegular, formatRs(p.retailPrice), 9.f, ThemeColor::Mute, mainX + 300, rowY);

            drawText(target, fontBold, std::to_string(p.stock) + " units", 9.5f, sf::Color(35, 120, 50), mainX + 360, rowY);

            // Directive Badge
            drawRounded(target, mainX + Layout::MAIN_W - 110, rowY - 2, 98, 20, 4, sf::Color(254, 226, 226));
            drawText(target, fontBold, "DO NOT PURCHASE", 7.5f, sf::Color(185, 28, 28), mainX + Layout::MAIN_W - 61, rowY + 4, TextAlignment::Center);

            sf::RectangleShape rLine({Layout::MAIN_W - 30, 1});
            rLine.setPosition(mainX + 15, rowY + 24);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }

        // Pinned Grand Total Footer
        drawRounded(target, mainX + 2, ty + 462.f, Layout::MAIN_W - 4.f, 38.f, 6, sf::Color(245, 240, 230), ThemeColor::Line, 1.f);
        drawText(target, fontBold, "OVERSTOCK SURPLUS SUMMARY", 9.5f, ThemeColor::Ink, mainX + 15, ty + 475.f);
        drawText(target, fontBold, "Top 10 Stock: " + std::to_string(totalSurplusUnits) + " units", 10.f, sf::Color(180, 83, 9), mainX + 220, ty + 475.f);
        drawText(target, fontBold, "Tied-Up Capital: " + formatRs(totalCapitalTied), 11.5f, sf::Color(220, 70, 60), mainX + Layout::MAIN_W - 15, ty + 475.f, TextAlignment::Right);
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

        drawText(target, fontRegular, log.timestamp, 9.f, ThemeColor::Mute, mainX + 20, rowY);

        sf::Color catColor = ThemeColor::Ink;
        if (log.category == "LOGIN") catColor = ThemeColor::Blue;
        else if (log.category == "SALE") catColor = ThemeColor::Green;
        else if (log.category == "RETURN") catColor = sf::Color(220, 90, 40);
        else if (log.category == "STAFF") catColor = sf::Color(130, 60, 160);
        else if (log.category == "INVENTORY" || log.category == "PRODUCT") catColor = sf::Color(190, 90, 40);
        else if (log.category == "ORDER") catColor = sf::Color(20, 120, 180);

        drawRounded(target, mainX + 132, rowY - 6, 68, 18, 4, sf::Color(catColor.r, catColor.g, catColor.b, 35));
        drawText(target, fontBold, log.category, 8.f, catColor, mainX + 166, rowY + 3, TextAlignment::Center);

        drawText(target, fontMedium, log.user, 9.5f, ThemeColor::Ink, mainX + 225, rowY);

        std::string safeDetails = log.details;
        if (safeDetails.length() > 55) {
            safeDetails = safeDetails.substr(0, 52) + "...";
        }
        drawText(target, fontRegular, safeDetails, 9.f, ThemeColor::Ink, mainX + 315, rowY);

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
            drawText(target, fontSemiBold, "Staff & Cashier Operator Accounts (staff.txt)", 12, ThemeColor::Ink, mainX + 25, staffY + 12);

            // "+ Add Employee" button
            bool hovAddStaff = pointInRect(mouse, mainX + Layout::MAIN_W - 135, staffY + 8, 115, 26);
            drawRounded(target, mainX + Layout::MAIN_W - 135, staffY + 8, 115, 26, 5, hovAddStaff ? ThemeColor::BlueHover : ThemeColor::Blue);
            drawText(target, fontBold, "+ Add Employee", 9.f, ThemeColor::White, mainX + Layout::MAIN_W - 77, staffY + 20, TextAlignment::Center);

            float thY = staffY + 44.f;
            drawText(target, fontSemiBold, "Employee ID", 9.f, ThemeColor::Mute, mainX + 30, thY);
            drawText(target, fontSemiBold, "Role", 9.f, ThemeColor::Mute, mainX + 160, thY);
            drawText(target, fontSemiBold, "Security PIN", 9.f, ThemeColor::Mute, mainX + 270, thY);
            drawText(target, fontSemiBold, "Actions", 9.f, ThemeColor::Mute, mainX + 390, thY);

            for (size_t i = 0; i < staffList.size() && i < 5; i++) {
                const auto& s = staffList[i];
                float sRowY = thY + 18.f + static_cast<float>(i) * 30.f;

                drawText(target, fontBold, s.name, 10.f, ThemeColor::Ink, mainX + 30, sRowY + 4);

                bool isAdm = (s.role == "Admin");
                drawRounded(target, mainX + 155, sRowY - 2, 65, 18, 4, isAdm ? sf::Color(254, 226, 226) : sf::Color(220, 235, 248));
                drawText(target, fontSemiBold, s.role, 8.f, isAdm ? sf::Color(180, 50, 40) : sf::Color(30, 95, 150), mainX + 187, sRowY + 7, TextAlignment::Center);

                std::string maskedPass = std::string(s.password.size(), '*');
                if (maskedPass.empty()) maskedPass = "****";
                drawText(target, fontRegular, maskedPass, 10, ThemeColor::Mute, mainX + 270, sRowY + 4);

                // Reset PIN
                drawRounded(target, mainX + 375, sRowY - 2, 65, 18, 4, sf::Color(230, 238, 248), ThemeColor::Blue, 1.f);
                drawText(target, fontSemiBold, "Reset PIN", 8.f, ThemeColor::Blue, mainX + 407, sRowY + 7, TextAlignment::Center);

                if (s.name != currentUserName) {
                    drawRounded(target, mainX + 448, sRowY - 2, 50, 18, 4, sf::Color(254, 226, 226), sf::Color(220, 80, 70), 1.f);
                    drawText(target, fontSemiBold, "Remove", 8.f, ThemeColor::Red, mainX + 473, sRowY + 7, TextAlignment::Center);
                }
            }
        }
    }
    // =========================================================================
    // SETTINGS SUBTAB 1: PRODUCT CATEGORIES (Requirement 1 & PCategory.txt)
    // =========================================================================
    else if (settingsSubTab == 1 && currentRole == "Admin") {
        drawRounded(target, mainX, ty, Layout::MAIN_W, 575, 10, ThemeColor::Paper, ThemeColor::Line, 1);

        drawText(target, fontSemiBold, "Product Category Directory (PCategory.txt)", 13, ThemeColor::Ink, mainX + 25, ty + 18);
        drawText(target, fontRegular, "First 2 digits of generated barcodes correspond to Category ID (Requirement 1)", 9.5f, ThemeColor::Mute, mainX + 25, ty + 34);

        // "+ Add Category" button
        bool hovAddCat = pointInRect(mouse, mainX + Layout::MAIN_W - 145, ty + 16, 125, 28);
        drawRounded(target, mainX + Layout::MAIN_W - 145, ty + 16, 125, 28, 6, hovAddCat ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontBold, "+ Add Category", 9.5f, ThemeColor::White, mainX + Layout::MAIN_W - 82, ty + 29, TextAlignment::Center);

        float thY = ty + 60.f;
        drawText(target, fontSemiBold, "Category ID", 9.5f, ThemeColor::Mute, mainX + 30, thY);
        drawText(target, fontSemiBold, "Category Name", 9.5f, ThemeColor::Mute, mainX + 140, thY);
        drawText(target, fontSemiBold, "Acronym", 9.5f, ThemeColor::Mute, mainX + 270, thY);
        drawText(target, fontSemiBold, "Barcode Prefix Rule", 9.5f, ThemeColor::Mute, mainX + 380, thY);

        sf::RectangleShape cSep({Layout::MAIN_W - 50, 1});
        cSep.setPosition(mainX + 25, thY + 16);
        cSep.setFillColor(ThemeColor::Line);
        target.draw(cSep);

        const auto& cats = categoryMgr.getCategories();
        for (size_t i = 0; i < cats.size(); i++) {
            float rowY = thY + 28.f + static_cast<float>(i) * 34.f - scrollOffset;
            if (rowY < thY + 16.f || rowY > ty + 550.f) continue;
            const auto& c = cats[i];

            // ID badge
            drawRounded(target, mainX + 30, rowY - 2, 45, 20, 4, sf::Color(220, 235, 250));
            drawText(target, fontBold, c.categoryId, 9.5f, sf::Color(29, 78, 216), mainX + 52, rowY + 7, TextAlignment::Center);

            drawText(target, fontBold, c.name, 10.5f, ThemeColor::Ink, mainX + 140, rowY + 3);
            drawText(target, fontMedium, c.acronym, 10.f, ThemeColor::Mute, mainX + 270, rowY + 3);

            drawText(target, fontRegular, c.categoryId + "XXXX (6 digits)", 9.5f, sf::Color(35, 120, 50), mainX + 380, rowY + 3);

            sf::RectangleShape rLine({Layout::MAIN_W - 50, 1});
            rLine.setPosition(mainX + 25, rowY + 22);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }
    }
    // =========================================================================
    // SETTINGS SUBTAB 2: SEASONAL DISCOUNTS (Requirements 13 & 14 & Seasons.txt)
    // =========================================================================
    else if (settingsSubTab == 2 && currentRole == "Admin") {
        drawRounded(target, mainX, ty, Layout::MAIN_W, 575, 10, ThemeColor::Paper, ThemeColor::Line, 1);

        drawText(target, fontSemiBold, "Seasonal Discount Calendars (Seasons.txt)", 13, ThemeColor::Ink, mainX + 25, ty + 18);
        drawText(target, fontRegular, "Dates active will automatically deduct percentage discount during bill checkout (Req 13, 14, 15)", 9.5f, ThemeColor::Mute, mainX + 25, ty + 34);

        // "+ Add Season" button
        bool hovAddSeason = pointInRect(mouse, mainX + Layout::MAIN_W - 145, ty + 16, 125, 28);
        drawRounded(target, mainX + Layout::MAIN_W - 145, ty + 16, 125, 28, 6, hovAddSeason ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontBold, "+ Add Season", 9.5f, ThemeColor::White, mainX + Layout::MAIN_W - 82, ty + 29, TextAlignment::Center);

        float thY = ty + 60.f;
        drawText(target, fontSemiBold, "Start Date", 9.5f, ThemeColor::Mute, mainX + 30, thY);
        drawText(target, fontSemiBold, "End Date", 9.5f, ThemeColor::Mute, mainX + 140, thY);
        drawText(target, fontSemiBold, "Discount %", 9.5f, ThemeColor::Mute, mainX + 255, thY);
        drawText(target, fontSemiBold, "Current Status", 9.5f, ThemeColor::Mute, mainX + 355, thY);
        drawText(target, fontSemiBold, "Action (Toggle)", 9.5f, ThemeColor::Mute, mainX + 465, thY);

        sf::RectangleShape sSep({Layout::MAIN_W - 50, 1});
        sSep.setPosition(mainX + 25, thY + 16);
        sSep.setFillColor(ThemeColor::Line);
        target.draw(sSep);

        const auto& seasons = seasonMgr.getSeasons();
        for (size_t i = 0; i < seasons.size(); i++) {
            float rowY = thY + 28.f + static_cast<float>(i) * 36.f - scrollOffset;
            if (rowY < thY + 16.f || rowY > ty + 550.f) continue;
            const auto& s = seasons[i];

            drawText(target, fontMedium, s.startDate, 10.f, ThemeColor::Ink, mainX + 30, rowY + 3);
            drawText(target, fontMedium, s.endDate, 10.f, ThemeColor::Ink, mainX + 140, rowY + 3);

            drawText(target, fontBold, std::to_string(s.discountPercent) + " %", 10.5f, sf::Color(220, 70, 60), mainX + 255, rowY + 3);

            // Status Badge
            bool active = s.isActive;
            drawRounded(target, mainX + 350, rowY - 2, 70, 20, 4, active ? sf::Color(220, 252, 231) : sf::Color(243, 244, 246));
            drawText(target, fontBold, active ? "ACTIVE" : "INACTIVE", 8.5f, active ? sf::Color(35, 120, 50) : sf::Color(107, 114, 128), mainX + 385, rowY + 7, TextAlignment::Center);

            // Toggle Button
            bool hovTog = pointInRect(mouse, mainX + 460, rowY - 2, 60, 20);
            drawRounded(target, mainX + 460, rowY - 2, 60, 20, 4,
                        hovTog ? (active ? sf::Color(254, 226, 226) : sf::Color(220, 252, 231)) : sf::Color(235, 230, 220),
                        ThemeColor::Edge, 1.f);
            drawText(target, fontSemiBold, active ? "Disable" : "Enable", 8.5f,
                     active ? ThemeColor::Red : sf::Color(35, 120, 50), mainX + 490, rowY + 7, TextAlignment::Center);

            sf::RectangleShape rLine({Layout::MAIN_W - 50, 1});
            rLine.setPosition(mainX + 25, rowY + 24);
            rLine.setFillColor(ThemeColor::Line);
            target.draw(rLine);
        }
    }
}

void PosReportsView::drawReturnTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                   const std::string& returnBarcode, bool returnBarcodeFocused,
                                   int returnQty, int returnMaxQty, const ProductItem* foundProduct,
                                   const std::string& saleInfo, int salePrice,
                                   const std::string& saleOrderId, const std::string& saleCustomer,
                                   const std::string& saleDate, bool returnSuccess,
                                   const std::string& statusMessage,
                                   int returnReasonIdx, int returnPaymentMethod, bool returnRestock,
                                   const std::vector<OrderRecord>& recentOrders,
                                   bool hasReturnSlip,
                                   const sf::Font& fontRegular, const sf::Font& fontMedium,
                                   const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                   const sf::Font& fontSerif,
                                   std::function<const sf::Texture&(const std::string&)> getTexture) {
    drawText(target, fontSerif, "Merchandise Return Terminal", 22, ThemeColor::Ink, mainX, 44);

    float ty = 85.f;
    float colW = (Layout::MAIN_W - 20.f) / 2.f;
    float paneW = colW - 32.f;

    // LEFT COLUMN: Scanner, Quick Orders & Lookup
    drawRounded(target, mainX, ty, colW, 575, 10, ThemeColor::Paper, ThemeColor::Line, 1.2f);
    drawText(target, fontBold, "Item & Order Lookup", 12.5f, ThemeColor::Ink, mainX + 16, ty + 16);

    float inY = ty + 38.f;
    float searchInputW = paneW - 68.f;
    drawRounded(target, mainX + 16, inY, searchInputW, 32, 6, ThemeColor::Paper,
                returnBarcodeFocused ? ThemeColor::Blue : ThemeColor::Line, 1.5f);
    std::string bDisplay = returnBarcode.empty() ? (returnBarcodeFocused ? "|" : "Enter Barcode / Order ID...")
                                                : (returnBarcode + (returnBarcodeFocused ? "|" : ""));
    drawText(target, fontMedium, bDisplay, 9.5f, returnBarcode.empty() ? ThemeColor::Mute : ThemeColor::Ink, mainX + 24, inY + 10);

    // Look Up Button
    float findX = mainX + 16 + searchInputW + 6.f;
    bool hovFind = pointInRect(mouse, findX, inY, 62.f, 32.f);
    drawRounded(target, findX, inY, 62.f, 32.f, 6, hovFind ? ThemeColor::BlueHover : ThemeColor::Blue);
    drawText(target, fontBold, "Find", 9.5f, ThemeColor::White, findX + 31.f, inY + 10, TextAlignment::Center);

    // Quick Test / Demo Pills
    float pillY = inY + 38.f;
    drawText(target, fontSemiBold, "Quick Samples:", 8.5f, ThemeColor::Mute, mainX + 16, pillY);
    const char* demoCodes[] = {"100001", "100003", "100006", "ORD-1001"};
    float pX = mainX + 16;
    for (int p = 0; p < 4; p++) {
        float pW = 54.f;
        bool hovP = pointInRect(mouse, pX, pillY + 12.f, pW, 20.f);
        drawRounded(target, pX, pillY + 12.f, pW, 20.f, 4,
                    hovP ? sf::Color(228, 220, 206) : sf::Color(242, 236, 224), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, demoCodes[p], 7.5f, ThemeColor::Ink, pX + pW / 2.f, pillY + 18.f, TextAlignment::Center);
        pX += pW + 5.f;
    }

    // Verified Sales Orders Ledger (Clickable to initiate return)
    float ordListY = pillY + 42.f;
    drawText(target, fontBold, "Verified Past Orders (Click to Return):", 9.5f, ThemeColor::Ink, mainX + 16, ordListY);

    int maxOrdersToShow = 4;
    int shown = 0;
    for (int i = (int)recentOrders.size() - 1; i >= 0 && shown < maxOrdersToShow; --i) {
        const auto& ord = recentOrders[i];
        if (ord.orderStatus != "PAID" && ord.orderStatus != "paid") continue;

        float rRowY = ordListY + 18.f + static_cast<float>(shown) * 58.f;
        shown++;

        bool isSelected = (!saleOrderId.empty() && ord.orderId == saleOrderId);
        bool hovRow = pointInRect(mouse, mainX + 16, rRowY, paneW, 52.f);

        drawRounded(target, mainX + 16, rRowY, paneW, 52.f, 6,
                    isSelected ? sf::Color(230, 240, 252) : (hovRow ? sf::Color(246, 241, 232) : ThemeColor::Panel),
                    isSelected ? ThemeColor::Blue : ThemeColor::Edge, isSelected ? 1.5f : 1.f);

        drawText(target, fontBold, "#" + ord.orderId, 10.f, isSelected ? ThemeColor::Blue : ThemeColor::Ink, mainX + 24, rRowY + 8);
        drawText(target, fontBold, "Rs " + std::to_string(ord.paymentAmount), 10.f, sf::Color(35, 120, 50), mainX + 16 + paneW - 10, rRowY + 8, TextAlignment::Right);

        std::string cName = ord.customerName;
        for (char& c : cName) if (c == '_') c = ' ';
        if (cName.length() > 16) cName = cName.substr(0, 14) + "..";
        drawText(target, fontRegular, cName + " | " + ord.orderDate, 8.5f, ThemeColor::Mute, mainX + 24, rRowY + 24);
        drawText(target, fontRegular, "Mode: " + ord.paymentMode, 8.f, ThemeColor::Mute, mainX + 24, rRowY + 38);
    }

    // Bottom Return Policy Guidelines Box
    float polY = ty + 465.f;
    drawRounded(target, mainX + 16, polY, paneW, 95.f, 8, sf::Color(246, 242, 234), ThemeColor::Edge, 1);
    drawText(target, fontBold, "Store Return Policy:", 9.5f, ThemeColor::Ink, mainX + 26, polY + 12);
    drawText(target, fontRegular, "- 7-day verified return policy with receipt\n- Deducts already refunded units from limits\n- Restocks both product and warehouse inventory\n- Full audit trail in returns.txt & activity_log.txt",
             8.f, ThemeColor::Mute, mainX + 26, polY + 28);

    // RIGHT COLUMN: Sale Verification, Reasons, Methods & Refund Processing
    float rX = mainX + colW + 20.f;
    drawRounded(target, rX, ty, colW, 575, 10, ThemeColor::Paper, ThemeColor::Line, 1.2f);
    drawText(target, fontBold, "Return & Refund Processing", 12.5f, ThemeColor::Ink, rX + 16, ty + 16);

    if (foundProduct != nullptr) {
        // 1. Found Product Card
        float pY = ty + 40.f;
        drawRounded(target, rX + 16, pY, paneW, 52.f, 6, ThemeColor::Panel, ThemeColor::Edge, 1);
        drawText(target, fontBold, foundProduct->name, 11.5f, ThemeColor::Ink, rX + 24, pY + 8);
        drawText(target, fontRegular, foundProduct->barcode + " | " + foundProduct->category + " | Shelf Stock: " + std::to_string(foundProduct->stock),
                 8.5f, ThemeColor::Mute, rX + 24, pY + 24);
        drawText(target, fontSemiBold, "Unit Price: Rs " + std::to_string(foundProduct->price), 9.f, sf::Color(35, 120, 50), rX + 24, pY + 38);

        // 2. Verified Purchase Card
        float vY = pY + 58.f;
        drawRounded(target, rX + 16, vY, paneW, 58.f, 6, sf::Color(243, 247, 243), sf::Color(187, 222, 190), 1);
        drawText(target, fontSemiBold, "Verified Purchase Record:", 9.f, sf::Color(22, 101, 52), rX + 24, vY + 8);
        drawText(target, fontRegular, "Order: " + (saleOrderId.empty() ? "Store Sale" : saleOrderId) + " | " + (saleCustomer.empty() ? "Customer" : saleCustomer),
                 8.5f, ThemeColor::Ink, rX + 24, vY + 24);
        drawText(target, fontBold, "Sold Unit Price: Rs " + std::to_string(salePrice) + " (Eligible: " + std::to_string(returnMaxQty) + " units)",
                 9.f, returnMaxQty > 0 ? sf::Color(22, 101, 52) : ThemeColor::Red, rX + 24, vY + 40);

        // 3. Return Quantity Stepper
        float qY = vY + 66.f;
        drawText(target, fontBold, "Quantity to Return:", 9.5f, ThemeColor::Ink, rX + 16, qY + 6);

        // [-] Button
        bool hovMinus = pointInRect(mouse, rX + 16 + paneW - 90.f, qY, 26.f, 24.f);
        drawRounded(target, rX + 16 + paneW - 90.f, qY, 26.f, 24.f, 4,
                    hovMinus ? sf::Color(228, 220, 206) : sf::Color(240, 235, 224), ThemeColor::Edge, 1);
        drawText(target, fontBold, "-", 12.f, ThemeColor::Ink, rX + 16 + paneW - 77.f, qY + 4, TextAlignment::Center);

        // Qty Display
        drawRounded(target, rX + 16 + paneW - 60.f, qY, 30.f, 24.f, 4, ThemeColor::Paper, ThemeColor::Line, 1);
        drawText(target, fontBold, std::to_string(returnQty), 11.f, ThemeColor::Ink, rX + 16 + paneW - 45.f, qY + 5, TextAlignment::Center);

        // [+] Button
        bool hovPlus = pointInRect(mouse, rX + 16 + paneW - 26.f, qY, 26.f, 24.f);
        drawRounded(target, rX + 16 + paneW - 26.f, qY, 26.f, 24.f, 4,
                    hovPlus ? sf::Color(228, 220, 206) : sf::Color(240, 235, 224), ThemeColor::Edge, 1);
        drawText(target, fontBold, "+", 12.f, ThemeColor::Ink, rX + 16 + paneW - 13.f, qY + 4, TextAlignment::Center);

        // 4. Return Reason Selection
        float reasY = qY + 32.f;
        drawText(target, fontBold, "Return Reason:", 9.5f, ThemeColor::Ink, rX + 16, reasY);
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
                     active ? ThemeColor::White : ThemeColor::Ink, rbx + rbw / 2.f, rby + 5.f, TextAlignment::Center);
        }

        // 5. Refund Method Selection
        float mthY = reasY + 68.f;
        drawText(target, fontBold, "Refund Method:", 9.5f, ThemeColor::Ink, rX + 16, mthY);
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
                     active ? ThemeColor::White : ThemeColor::Ink, mbx + mbw / 2.f, mby + 6.f, TextAlignment::Center);
        }

        // 6. Restock Checkbox / Toggle
        float dispY = mthY + 46.f;
        bool hovBox = pointInRect(mouse, rX + 16, dispY, paneW, 20.f);
        drawRounded(target, rX + 16, dispY + 1.f, 16.f, 16.f, 3,
                    returnRestock ? sf::Color(35, 120, 50) : ThemeColor::Paper, ThemeColor::Edge, 1);
        if (returnRestock) {
            drawText(target, fontBold, "v", 9.f, ThemeColor::White, rX + 24.f, dispY + 2.f, TextAlignment::Center);
        }
        drawText(target, fontSemiBold, "Restock item back to shelves & inventory", 8.5f,
                 returnRestock ? ThemeColor::Ink : ThemeColor::Mute, rX + 38.f, dispY + 3.f);

        // 7. Total Refund Amount Banner
        int refundTotal = ((salePrice > 0) ? salePrice : foundProduct->price) * returnQty;
        float refY = dispY + 28.f;
        drawRounded(target, rX + 16, refY, paneW, 40.f, 6, sf::Color(254, 243, 199), sf::Color(245, 158, 11), 1);
        drawText(target, fontSemiBold, "Total Refund Payable:", 9.5f, sf::Color(180, 83, 9), rX + 26, refY + 13);
        drawText(target, fontBold, "Rs " + std::to_string(refundTotal), 13.5f, sf::Color(180, 83, 9), rX + 16 + paneW - 12, refY + 11, TextAlignment::Right);

        // 8. Main Action Button: PROCESS RETURN
        float btnY = refY + 48.f;
        bool hovProc = pointInRect(mouse, rX + 16, btnY, paneW, 36.f);
        bool canProcess = (returnMaxQty > 0 && returnQty > 0);
        drawRounded(target, rX + 16, btnY, paneW, 36.f, 6,
                    !canProcess ? sf::Color(200, 190, 180) : (hovProc ? sf::Color(185, 28, 28) : ThemeColor::Red));
        drawText(target, fontBold, "PROCESS RETURN & REFUND", 10.f, ThemeColor::White, rX + 16 + paneW / 2.f, btnY + 12, TextAlignment::Center);

        // 9. View Return Slip Button
        if (hasReturnSlip) {
            float slipY = btnY + 42.f;
            bool hovSlip = pointInRect(mouse, rX + 16, slipY, paneW, 26.f);
            drawRounded(target, rX + 16, slipY, paneW, 26.f, 4,
                        hovSlip ? ThemeColor::BlueHover : sf::Color(230, 240, 250), ThemeColor::Blue, 1.f);
            drawText(target, fontSemiBold, "Print / View Return Slip (Proof)", 8.5f,
                     hovSlip ? ThemeColor::White : ThemeColor::Blue, rX + 16 + paneW / 2.f, slipY + 7.f, TextAlignment::Center);
        }
    } else {
        // Elegant placeholder when no item selected
        float empY = ty + 150.f;
        drawRounded(target, rX + 24, empY, paneW - 16.f, 130.f, 8, sf::Color(248, 244, 236), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, "No Product Selected", 11.f, ThemeColor::Ink, rX + colW / 2.f, empY + 25.f, TextAlignment::Center);
        drawText(target, fontRegular, "Enter barcode, click a sample pill,\nor choose an order from the list on the left\nto inspect and process a return.",
                 8.5f, ThemeColor::Mute, rX + colW / 2.f, empY + 48.f, TextAlignment::Center);

        float splBtnY = empY + 88.f;
        float splW = 140.f;
        float splX = rX + (colW - splW) / 2.f;
        bool hovSpl = pointInRect(mouse, splX, splBtnY, splW, 26.f);
        drawRounded(target, splX, splBtnY, splW, 26.f, 4,
                    hovSpl ? ThemeColor::BlueHover : ThemeColor::Blue);
        drawText(target, fontBold, "Load Sample: 100001", 8.5f, ThemeColor::White, splX + splW / 2.f, splBtnY + 8.f, TextAlignment::Center);
    }

    // Status Message Box at bottom
    if (!statusMessage.empty()) {
        float stY = ty + 526.f;
        drawRounded(target, rX + 16, stY, paneW, 34.f, 6,
                    returnSuccess ? sf::Color(220, 252, 231) : sf::Color(254, 226, 226),
                    returnSuccess ? sf::Color(134, 239, 172) : sf::Color(252, 165, 165), 1.f);
        drawText(target, fontSemiBold, statusMessage, 8.5f,
                 returnSuccess ? sf::Color(22, 101, 52) : sf::Color(185, 28, 28),
                 rX + 16 + paneW / 2.f, stY + 11.f, TextAlignment::Center);
    }
}

#include "PosReportsView.h"
#include "UIUtils.h"

void PosReportsView::drawSalesTab(sf::RenderTarget& target, sf::Vector2f /*m*/, float mainX,
                                  const std::vector<SaleRecord>& salesHistory, float scrollOffset,
                                  const sf::Font& fontRegular, const sf::Font& fontBold,
                                  const sf::Font& fontSerif) {
    drawText(target, fontSerif, "Sales & Orders Audit Log", 24, ThemeColor::Ink, mainX, 50);
    drawText(target, fontRegular, "Completed register transactions recorded live during this shift", 10.5f, ThemeColor::Mute, mainX, 74);

    float ty = 100.f;
    drawRounded(target, mainX, ty, Layout::MAIN_W, 560, 10, ThemeColor::Paper, ThemeColor::Line, 1);

    if (salesHistory.empty()) {
        drawText(target, fontRegular, "No transactions recorded yet in this shift.", 13, ThemeColor::Mute,
                 mainX + Layout::MAIN_W / 2.f, ty + 200, TextAlignment::Center);
        return;
    }

    for (size_t i = 0; i < salesHistory.size() && i < 10; i++) {
        const auto& s = salesHistory[salesHistory.size() - 1 - i];
        float rowY = ty + 35.f + static_cast<float>(i) * 52.f - scrollOffset;
        if (rowY < ty + 20 || rowY > ty + 530) continue;

        drawText(target, fontBold, "Sale #" + std::to_string(salesHistory.size() - i) + " (" + s.paymentMethod + ")",
                 12, ThemeColor::Ink, mainX + 25, rowY);
        drawText(target, fontRegular, "Time: " + s.timestamp + " | " + std::to_string(s.itemCount) + " items",
                 10, ThemeColor::Mute, mainX + 25, rowY + 16);
        drawText(target, fontBold, formatRs(s.totalAmount), 13, ThemeColor::Ink, mainX + Layout::MAIN_W - 35, rowY + 8, TextAlignment::Right);

        sf::RectangleShape sep({Layout::MAIN_W - 40, 1});
        sep.setPosition(mainX + 20, rowY + 28);
        sep.setFillColor(ThemeColor::Line);
        target.draw(sep);
    }
}

void PosReportsView::drawActivityLogsTab(sf::RenderTarget& target, sf::Vector2f /*m*/, float mainX,
                                        const std::vector<ActivityRecord>& logs,
                                        const std::string& logFilter, float scrollOffset,
                                        const sf::Font& fontRegular, const sf::Font& fontMedium,
                                        const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                        const sf::Font& fontSerif) {
    drawText(target, fontSerif, "Activity & Security Audit Logs", 24, ThemeColor::Ink, mainX, 50);
    drawText(target, fontRegular, "Live audit trail of user logins, sales transactions, inventory restocks, and staff credential setup", 10.5f, ThemeColor::Mute, mainX, 74);

    const char* filters[] = {"ALL", "LOGIN", "SALE", "STAFF", "INVENTORY"};
    for (int i = 0; i < 5; i++) {
        bool active = (logFilter == filters[i]);
        drawRounded(target, mainX + i * 92, 102, 82, 26, 6,
                    active ? ThemeColor::Dark : sf::Color(238, 230, 214),
                    active ? ThemeColor::Dark : ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, filters[i], 9.5f,
                 active ? ThemeColor::White : ThemeColor::Ink, mainX + i * 92 + 41, 115, TextAlignment::Center);
    }

    float ty = 140.f;
    drawRounded(target, mainX, ty, Layout::MAIN_W, 520, 10, ThemeColor::Paper, ThemeColor::Line, 1);

    // Header row
    drawText(target, fontSemiBold, "Timestamp", 10, ThemeColor::Mute, mainX + 20, ty + 16);
    drawText(target, fontSemiBold, "Category", 10, ThemeColor::Mute, mainX + 160, ty + 16);
    drawText(target, fontSemiBold, "Operator", 10, ThemeColor::Mute, mainX + 245, ty + 16);
    drawText(target, fontSemiBold, "Audit Action Details", 10, ThemeColor::Mute, mainX + 340, ty + 16);

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
        if (rowY < ty + 25 || rowY > ty + 490) continue;

        drawText(target, fontRegular, log.timestamp, 9.5f, ThemeColor::Mute, mainX + 20, rowY);

        sf::Color catColor = ThemeColor::Ink;
        if (log.category == "LOGIN") catColor = ThemeColor::Blue;
        else if (log.category == "SALE") catColor = ThemeColor::Green;
        else if (log.category == "STAFF") catColor = sf::Color(130, 60, 160);
        else if (log.category == "INVENTORY" || log.category == "PRODUCT") catColor = sf::Color(190, 90, 40);

        drawRounded(target, mainX + 155, rowY - 8, 75, 20, 4, sf::Color(catColor.r, catColor.g, catColor.b, 35));
        drawText(target, fontBold, log.category, 8.5f, catColor, mainX + 192, rowY + 2, TextAlignment::Center);

        drawText(target, fontMedium, log.user, 10, ThemeColor::Ink, mainX + 245, rowY);
        drawText(target, fontRegular, log.details, 9.5f, ThemeColor::Ink, mainX + 340, rowY);

        sf::RectangleShape sep({Layout::MAIN_W - 40, 1});
        sep.setPosition(mainX + 20, rowY + 16);
        sep.setFillColor(sf::Color(230, 222, 210));
        target.draw(sep);
    }
}

void PosReportsView::drawSettingsTab(sf::RenderTarget& target, sf::Vector2f /*m*/, float mainX,
                                    const std::vector<StaffMember>& staffList,
                                    const std::string& currentRole, const std::string& currentUserName,
                                    bool cameraRunning, bool soundEnabled,
                                    const sf::Font& fontRegular, const sf::Font& fontMedium,
                                    const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                    const sf::Font& fontSerif) {
    drawText(target, fontSerif, "System & Staff Settings", 24, ThemeColor::Ink, mainX, 50);
    drawText(target, fontRegular, "Cashier operator profiles, employee ID setup, password configuration, and camera streamer", 10.5f, ThemeColor::Mute, mainX, 74);

    float ty = 105.f;
    drawRounded(target, mainX, ty, Layout::MAIN_W, 560, 10, ThemeColor::Paper, ThemeColor::Line, 1);

    auto drawSetting = [&](const std::string& title, const std::string& desc, const std::string& val, float y, sf::Color valCol = ThemeColor::Ink) {
        drawText(target, fontSemiBold, title, 12, ThemeColor::Ink, mainX + 25, y);
        drawText(target, fontRegular, desc, 9.5f, ThemeColor::Mute, mainX + 25, y + 15);
        drawRounded(target, mainX + Layout::MAIN_W - 145, y - 4, 125, 26, 6, sf::Color(238, 230, 213), ThemeColor::Edge, 1);
        drawText(target, fontSemiBold, val, 9.5f, valCol, mainX + Layout::MAIN_W - 82, y + 8, TextAlignment::Center);
    };

    drawSetting("Webcam Optical Scanner", "Media Foundation 30 FPS Streamer", cameraRunning ? "Active" : "Offline", ty + 20, cameraRunning ? ThemeColor::Green : ThemeColor::Red);
    drawSetting("Logged-in Operator", "Active role & session user", currentUserName + " (" + currentRole + ")", ty + 65, (currentRole == "Admin") ? sf::Color(180, 70, 50) : sf::Color(40, 110, 160));
    drawSetting("Audio Scan Feedback", "Beep sound upon barcode read", soundEnabled ? "Enabled" : "Muted", ty + 110);

    // ── Staff & Operator Management (staff.txt) ──
    float staffY = ty + 155.f;
    sf::RectangleShape staffSep({Layout::MAIN_W - 50, 1});
    staffSep.setPosition(mainX + 25, staffY);
    staffSep.setFillColor(ThemeColor::Line);
    target.draw(staffSep);

    if (currentRole == "Admin") {
        drawText(target, fontSemiBold, "Staff & Operator Management (staff.txt)", 13, ThemeColor::Ink, mainX + 25, staffY + 15);
        drawText(target, fontRegular, "Configure employee IDs, setup accounts, assign roles, and reset staff passwords", 9.5f, ThemeColor::Mute, mainX + 25, staffY + 31);

        // "+ Add Employee" button
        drawRounded(target, mainX + Layout::MAIN_W - 145, staffY + 12, 120, 28, 6, ThemeColor::Blue);
        drawText(target, fontBold, "+ Add Employee", 9.5f, ThemeColor::White, mainX + Layout::MAIN_W - 85, staffY + 25, TextAlignment::Center);

        // Staff table header
        float thY = staffY + 52.f;
        drawText(target, fontSemiBold, "Employee ID", 9.5f, ThemeColor::Mute, mainX + 30, thY);
        drawText(target, fontSemiBold, "Role", 9.5f, ThemeColor::Mute, mainX + 160, thY);
        drawText(target, fontSemiBold, "Security PIN", 9.5f, ThemeColor::Mute, mainX + 270, thY);
        drawText(target, fontSemiBold, "Actions", 9.5f, ThemeColor::Mute, mainX + 410, thY);

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
            drawRounded(target, mainX + 360, sRowY, 78, 22, 4, sf::Color(230, 238, 248), ThemeColor::Blue, 1.f);
            drawText(target, fontSemiBold, "Reset PIN", 8.5f, ThemeColor::BlueHover, mainX + 399, sRowY + 10, TextAlignment::Center);

            // Remove Button
            if (s.name != currentUserName) {
                drawRounded(target, mainX + 446, sRowY, 64, 22, 4, sf::Color(254, 226, 226), sf::Color(220, 80, 70), 1.f);
                drawText(target, fontSemiBold, "Remove", 8.5f, ThemeColor::Red, mainX + 478, sRowY + 10, TextAlignment::Center);
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

    // System Utilities
    float utilY = ty + 380.f;
    drawRounded(target, mainX + 25, utilY, 220, 34, 6, ThemeColor::Blue);
    drawText(target, fontSemiBold, "Reload product.txt Database", 10.f, ThemeColor::White, mainX + 135, utilY + 16, TextAlignment::Center);

    drawRounded(target, mainX + 265, utilY, 245, 34, 6, ThemeColor::Blue);
    drawText(target, fontSemiBold, "Regenerate Barcode Bitmaps", 10.f, ThemeColor::White, mainX + 387, utilY + 16, TextAlignment::Center);

    drawRounded(target, mainX + 25, ty + 440, Layout::MAIN_W - 50, 90, 8, sf::Color(240, 235, 225), ThemeColor::Edge, 1);
    drawText(target, fontBold, "System Compliance & Credentials Storage:", 10.5f, ThemeColor::Ink, mainX + 38, ty + 458);
    drawText(target, fontRegular, "- Staff credentials and roles stored in staff.txt\n- Security events, logins, and sales logged in activity_log.txt\n- Optical Barcode scanner with live real-time continuous Media Foundation streaming", 9.5f, ThemeColor::Mute, mainX + 38, ty + 480);
}

void PosReportsView::drawReturnTab(sf::RenderTarget&, sf::Vector2f) {
}

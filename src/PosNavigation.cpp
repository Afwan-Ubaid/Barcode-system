#include "PosNavigation.h"
#include "UIUtils.h"
#include <ctime>
#include <cmath>

void PosNavigation::drawSidebar(sf::RenderTarget& target, sf::Vector2f m, float designHeight,
                                int activeTab, const std::string& currentRole, const std::string& currentUserName,
                                const sf::Font& fontRegular, const sf::Font& fontMedium, const sf::Font& fontSemiBold,
                                std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::RectangleShape bar({Layout::SIDE_W, designHeight});
    bar.setPosition(0, 0);
    bar.setFillColor(ThemeColor::Sidebar);
    target.draw(bar);

    sf::RectangleShape div({1.f, designHeight});
    div.setPosition(Layout::SIDE_W - 1.f, 0);
    div.setFillColor(ThemeColor::Line);
    target.draw(div);

    // ScanIT Logo
    drawSprite(target, getTexture("icons/scan.png"), 22, 22, 24, 24, ThemeColor::Ink);
    drawText(target, fontSemiBold, "ScanIT", 11, ThemeColor::Ink, 34, 58, TextAlignment::Center, 0.4f);

    // Navigation Buttons (Admin: POS, Products, Inventory, Reports, Activity, Settings, Return; Employee: POS, Products, Return - ISE-105: Reports restricted from Cashier)
    bool isAdmin = (currentRole == "Admin");
    std::vector<int> visibleTabs = isAdmin ? std::vector<int>{0, 1, 2, 3, 4, 5, 6} : std::vector<int>{0, 1, 6};

    for (size_t slot = 0; slot < visibleTabs.size(); slot++) {
        int i = visibleTabs[slot];
        float cy = Layout::NAV_Y[slot];
        bool selected = (activeTab == i);
        bool hovered = pointInRect(m, 6, cy - 24, 60, 48);

        if (selected) {
            drawRounded(target, 6, cy - 24, 60, 48, 12, sf::Color(228, 219, 200), sf::Color(214, 203, 182), 1.2f);
        } else if (hovered) {
            drawRounded(target, 6, cy - 24, 60, 48, 12, sf::Color(233, 225, 208));
        }

        drawSprite(target, getTexture("ui/pebble_" + std::string(Layout::NAV_ITEMS[i][1]) + ".png"), 18, cy - 22, 36, 36);
        drawText(target, selected ? fontSemiBold : fontMedium, Layout::NAV_ITEMS[i][0], 8.5f,
                 selected ? ThemeColor::Ink : ThemeColor::Mute, 36.f, cy + 17, TextAlignment::Center, 0.2f);
    }

    // Role-Differentiated User Profile at bottom
    sf::CircleShape avatar(13, 32);
    avatar.setOrigin(13, 13);
    avatar.setPosition(34, 664);
    if (currentRole == "Admin") {
        avatar.setFillColor(sf::Color(155, 60, 50)); // Crimson/Burgundy badge for Admin
    } else {
        avatar.setFillColor(sf::Color(179, 150, 118)); // Warm tan for Cashier / Employee
    }
    target.draw(avatar);

    std::string initials = "MK";
    if (currentRole == "Admin") {
        initials = "AD";
    } else if (!currentUserName.empty()) {
        initials = std::string(1, (char)std::toupper((unsigned char)currentUserName[0]));
        if (currentUserName.length() > 1) {
            initials += (char)std::toupper((unsigned char)currentUserName[1]);
        }
    }
    drawText(target, fontSemiBold, initials, 9, ThemeColor::White, 34, 664, TextAlignment::Center, 0.3f);

    std::string displayName = (currentRole == "Admin" ? "Administrator" : currentUserName);
    if (displayName.length() > 10) displayName = displayName.substr(0, 9) + "..";
    drawText(target, fontSemiBold, displayName, 9, ThemeColor::Ink, 34, 688, TextAlignment::Center, 0.1f);

    std::string roleSubtitle = (currentRole == "Admin" ? "System Admin" : "Cashier, 904");
    drawText(target, fontRegular, roleSubtitle, 8, ThemeColor::Mute, 34, 700, TextAlignment::Center, 0.1f);
}

void PosNavigation::drawHeader(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                              const std::string& currentRole, const std::string& currentUserName,
                              const sf::Font& fontRegular, const sf::Font& fontMedium,
                              const sf::Font& fontSemiBold, const sf::Font& fontSerif) {
    if (currentRole == "Admin") {
        drawText(target, fontSemiBold, "SALES COUNTER / 04 [ADMINISTRATOR]", 9, sf::Color(170, 70, 50), mainX, 36, TextAlignment::Left, 1.6f);
    } else {
        drawText(target, fontSemiBold, "SALES COUNTER / 04 [CASHIER: " + currentUserName + "]", 9, ThemeColor::Mute, mainX, 36, TextAlignment::Left, 1.6f);
    }
    drawText(target, fontSerif, "Point of Sale", 28, ThemeColor::Ink, mainX, 62, TextAlignment::Left, 0.f);

    // Quick Check Pill Button (ISE-56 Standalone Inspection Overlay)
    float qcW = 112.f;
    float qcH = 25.f;
    float qcX = mainX + 230.f;
    float qcY = 56.f;
    bool hovQc = pointInRect(mouse, qcX, qcY, qcW, qcH);
    drawRounded(target, qcX, qcY, qcW, qcH, 6,
                hovQc ? ThemeColor::BlueHover : sf::Color(245, 238, 226),
                hovQc ? ThemeColor::BlueHover : ThemeColor::Edge, 1.f);
    drawText(target, fontSemiBold, "QUICK CHECK", 9.f,
             hovQc ? ThemeColor::White : ThemeColor::Ink, qcX + qcW / 2.f, qcY + 12.5f, TextAlignment::Center);
}

void PosNavigation::drawSearch(sf::RenderTarget& target, sf::Vector2f m, float mainX,
                              const std::string& searchQuery, bool searchFocused,
                              const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                              std::function<const sf::Texture&(const std::string&)> getTexture) {
    sf::FloatRect searchBox{mainX, 82.f, 415.f, 39.f};
    sf::FloatRect addBtn{mainX + 425.f, 82.f, 115.f, 39.f};

    drawShadow(target, searchBox.left, searchBox.top, searchBox.width, searchBox.height, 10, 4, 6, 8, 2);
    drawRounded(target, searchBox.left, searchBox.top, searchBox.width, searchBox.height, 10,
                ThemeColor::Paper, searchFocused ? ThemeColor::Ink : ThemeColor::Edge, searchFocused ? 1.6f : 1.f);

    drawSprite(target, getTexture("icons/scan.png"), searchBox.left + 14, searchBox.top + 11, 17, 17, ThemeColor::Mute);

    if (searchQuery.empty()) {
        drawText(target, fontRegular, "Scan barcode or search products...", 12, ThemeColor::Mute, searchBox.left + 42, searchBox.top + 19.5f);
    } else {
        bool showCaret = searchFocused && (std::fmod((float)clock() / CLOCKS_PER_SEC, 1.f) < 0.5f);
        drawText(target, fontRegular, searchQuery + (showCaret ? "|" : ""), 12, ThemeColor::Ink, searchBox.left + 42, searchBox.top + 19.5f);
    }

    drawRounded(target, searchBox.left + searchBox.width - 32, searchBox.top + 10, 20, 19, 4,
                ThemeColor::None, ThemeColor::Edge, 1);
    drawSprite(target, getTexture("icons/kbd.png"), searchBox.left + searchBox.width - 30, searchBox.top + 11, 16, 16, ThemeColor::Mute);

    bool hovAdd = pointInRect(m, addBtn.left, addBtn.top, addBtn.width, addBtn.height);
    drawRounded(target, addBtn.left, addBtn.top, addBtn.width, addBtn.height, 10,
                hovAdd ? sf::Color(255, 253, 248) : ThemeColor::Paper, hovAdd ? ThemeColor::Ink : ThemeColor::Edge, 1);
    drawSprite(target, getTexture("icons/plus.png"), addBtn.left + 14, addBtn.top + 11, 17, 17, ThemeColor::Ink);
    drawText(target, fontSemiBold, "Add product", 12, ThemeColor::Ink, addBtn.left + 36, addBtn.top + 19.5f);
}

#include "UIUtils.h"
#include <cmath>
#include <ctime>
#include <algorithm>

namespace Layout {
    const char* CATEGORIES[] = {
        "All", "Grocery", "Drinks", "Bakery", "Dairy", "Snacks", "Fruits & Veg", "Household", "Personal Care"
    };
    const char* NAV_ITEMS[][2] = {
        {"POS", "pos"},
        {"Products", "products"},
        {"Inventory", "inventory"},
        {"Sales", "sales"},
        {"Activity", "activity"},
        {"Settings", "settings"},
        {"Return", "return"}
    };
    const float NAV_Y[] = {88.f, 138.f, 188.f, 238.f, 288.f, 338.f, 388.f};
}

sf::ConvexShape roundedRect(float x, float y, float w, float h, float r) {
    r = std::min(r, std::min(w, h) / 2.f);
    const int seg = 8;
    std::vector<sf::Vector2f> pts;
    auto arc = [&](float cx, float cy, float startAngle) {
        for (int i = 0; i <= seg; i++) {
            float rad = (startAngle + 90.f * i / seg) * 3.14159265f / 180.f;
            pts.push_back({cx + std::cos(rad) * r, cy + std::sin(rad) * r});
        }
    };
    arc(x + w - r, y + r, -90);
    arc(x + w - r, y + h - r, 0);
    arc(x + r, y + h - r, 90);
    arc(x + r, y + r, 180);

    sf::ConvexShape s(pts.size());
    for (size_t i = 0; i < pts.size(); i++) s.setPoint(i, pts[i]);
    return s;
}

void drawRounded(sf::RenderTarget& t, float x, float y, float w, float h, float r,
                 sf::Color fill, sf::Color outline, float thickness) {
    sf::ConvexShape s = roundedRect(x, y, w, h, r);
    s.setFillColor(fill);
    if (thickness > 0) {
        s.setOutlineColor(outline);
        s.setOutlineThickness(-thickness);
    }
    t.draw(s);
}

void drawShadow(sf::RenderTarget& t, float x, float y, float w, float h, float r,
                int layers, float spread, int alpha, float dy) {
    for (int i = layers; i > 0; i--) {
        float g = spread * i / layers;
        drawRounded(t, x - g, y - g + dy, w + 2 * g, h + 2 * g, r + g, sf::Color(70, 50, 25, alpha));
    }
}

float getTextWidth(const sf::Font& f, const std::string& s, float size, float letterSpacing) {
    unsigned int uSize = static_cast<unsigned int>(std::round(size));
    sf::Text t(s, f, uSize);
    t.setLetterSpacing(letterSpacing);
    return t.getLocalBounds().width;
}

void drawText(sf::RenderTarget& t, const sf::Font& f, const std::string& s, float size,
              sf::Color c, float x, float cy, TextAlignment a, float letterSpacing) {
    unsigned int uSize = static_cast<unsigned int>(std::round(size));
    sf::Text text(s, f, uSize);
    text.setFillColor(c);
    text.setLetterSpacing(letterSpacing);
    sf::FloatRect b = text.getLocalBounds();
    float ox = b.left + (a == TextAlignment::Left ? 0.f : (a == TextAlignment::Center ? b.width / 2.f : b.width));
    text.setOrigin(std::round(ox), 0);
    text.setPosition(std::round(x), std::round(cy - size * 0.62f));
    t.draw(text);
}

void drawSprite(sf::RenderTarget& t, const sf::Texture& tx, float x, float y, float w, float h,
                sf::Color tint) {
    sf::Vector2u sz = tx.getSize();
    if (!sz.x || !sz.y) return;
    sf::Sprite s(tx);
    s.setScale(w / sz.x, h / sz.y);
    s.setPosition(x, y);
    s.setColor(tint);
    t.draw(s);
}

bool pointInRect(sf::Vector2f pt, float x, float y, float w, float h) {
    return pt.x >= x && pt.x <= x + w && pt.y >= y && pt.y <= y + h;
}

std::string toLowerStr(std::string str) {
    for (auto& ch : str) ch = (char)std::tolower((unsigned char)ch);
    return str;
}

std::string trimStr(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

std::string formatRs(int amount) {
    std::string s = std::to_string(std::abs(amount));
    for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert(i, ",");
    return std::string(amount < 0 ? "- " : "") + "Rs " + s;
}

std::string getCurrentDateYYYYMMDD() {
    std::time_t now = std::time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tmNow);
    return std::string(buf);
}

std::string getCurrentTimeHHMMSS() {
    std::time_t now = std::time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmNow);
    return std::string(buf);
}

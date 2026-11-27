#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Separation of Concerns (SoC) & High Cohesion
// Module: UI Drawing Primitives, Layout Constants, and Geometric Utilities
// ============================================================================

namespace Layout {
    constexpr float SIDE_W = 72.f;
    constexpr float MAIN_W = 545.f;
    constexpr float PANEL_W = 336.f;

    constexpr float CARD_W = 175.f;
    constexpr float CARD_H = 135.f;
    constexpr float CARD_GAP = 10.f;
    constexpr float CARD_Y = 347.f;

    constexpr float RP_W = 302.f;
    constexpr float RP_Y = 66.f;
    constexpr float RP_H = 378.f;
    constexpr float ROW_Y = 128.f;
    constexpr float ROW_H = 50.f;

    constexpr int CATEGORY_COUNT = 9;
    extern const char* CATEGORIES[CATEGORY_COUNT];
    extern const char* NAV_ITEMS[7][2];
    extern const float NAV_Y[7];
}

enum class TextAlignment {
    Left,
    Center,
    Right
};

// Shape & Drawing Helpers
sf::ConvexShape roundedRect(float x, float y, float w, float h, float r);

void drawRounded(sf::RenderTarget& t, float x, float y, float w, float h, float r,
                 sf::Color fill, sf::Color outline = ThemeColor::None, float thickness = 0.f);

void drawShadow(sf::RenderTarget& t, float x, float y, float w, float h, float r,
                int layers = 5, float spread = 8.f, int alpha = 10, float dy = 3.f);

float getTextWidth(const sf::Font& f, const std::string& s, float size, float letterSpacing = 1.f);

void drawText(sf::RenderTarget& t, const sf::Font& f, const std::string& s, float size,
              sf::Color c, float x, float cy, TextAlignment a = TextAlignment::Left, float letterSpacing = 1.f);

void drawSprite(sf::RenderTarget& t, const sf::Texture& tx, float x, float y, float w, float h,
                sf::Color tint = ThemeColor::White);

bool pointInRect(sf::Vector2f pt, float x, float y, float w, float h);

// String & Formatting Utilities
std::string toLowerStr(std::string str);
std::string trimStr(const std::string& s);
std::string formatRs(int amount);

// Date & Time Formatting
std::string getCurrentDateYYYYMMDD();
std::string getCurrentTimeHHMMSS();

// Data File Path Resolution (data/ folder)
std::string resolveDataFilePath(const std::string& filename);

#endif // UI_UTILS_H

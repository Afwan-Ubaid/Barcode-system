#ifndef UITHEME_H
#define UITHEME_H
#include <SFML/Graphics.hpp>
#include <string>
enum class AppScreen { Splash, Home, Generate, Read };

namespace Theme {
    const unsigned int WIN_W = 1024;
    const unsigned int WIN_H = 680;

    // Palette
    const sf::Color Background     = sf::Color(14, 7, 28);       // #0E071C Deep obsidian
    const sf::Color Panel          = sf::Color(24, 16, 46, 190);  // translucent dark glass
    const sf::Color PanelBorder    = sf::Color(139, 92, 246, 90);
    const sf::Color Purple         = sf::Color(139, 92, 246);    // #8B5CF6
    const sf::Color PurpleSoft     = sf::Color(168, 85, 247);    // #A855F7
    const sf::Color DeepPurple     = sf::Color(59, 7, 100);      // #3B0764
    const sf::Color Cyan           = sf::Color(6, 182, 212);     // #06B6D4
    const sf::Color TextWhite      = sf::Color(248, 250, 252);   // #F8FAFC
    const sf::Color TextMuted      = sf::Color(148, 163, 184);   // #94A3B8
    const sf::Color Green          = sf::Color(34, 197, 94);     // #22C55E
    const sf::Color Red            = sf::Color(239, 68, 68);     // #EF4444

    inline float centerX(const sf::Text& text, float areaX, float areaW) {
        return areaX + (areaW - text.getLocalBounds().width) * 0.5f;
    }
}

#endif // UITHEME_H

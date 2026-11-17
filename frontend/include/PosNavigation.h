#ifndef POS_NAVIGATION_H
#define POS_NAVIGATION_H

#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Separation of Concerns (SoC)
// Module: Header, Sidebar Navigation, Search Input & Category Bar View
// ============================================================================

class PosNavigation {
public:
    static void drawSidebar(sf::RenderTarget& target, sf::Vector2f mouse, float designHeight,
                            int activeTab, const std::string& currentRole, const std::string& currentUserName,
                            const sf::Font& fontRegular, const sf::Font& fontMedium, const sf::Font& fontSemiBold,
                            std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawHeader(sf::RenderTarget& target, float mainX,
                           const std::string& currentRole, const std::string& currentUserName,
                           const sf::Font& fontRegular, const sf::Font& fontMedium,
                           const sf::Font& fontSemiBold, const sf::Font& fontSerif);

    static void drawSearch(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                           const std::string& searchQuery, bool searchFocused,
                           const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                           std::function<const sf::Texture&(const std::string&)> getTexture);
};

#endif // POS_NAVIGATION_H

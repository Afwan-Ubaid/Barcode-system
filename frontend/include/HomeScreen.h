#ifndef HOMESCREEN_H
#define HOMESCREEN_H
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "UITheme.h"

class HomeScreen {
public:
    HomeScreen(sf::RenderWindow& window, sf::Font& font);
    void addRecentScan(const std::string& code, const std::string& type);
    AppScreen handleEvent(const sf::Event& event);
    void draw();

private:
    sf::RenderWindow& mWindow;
    sf::Font&         mFont;

    // Action Cards Click Bounds
    sf::RectangleShape mCardGenerate;
    sf::RectangleShape mCardScan;
    bool               mHoverGen;
    bool               mHoverScan;

    // Dynamic Activity Log
    struct ScanRecord {
        std::string code;
        std::string source;
        std::string time;
        std::string format;
    };
    std::vector<ScanRecord> mRecords;

    // Dock Hitboxes
    sf::FloatRect mNavHome;
    sf::FloatRect mNavGen;
    sf::FloatRect mNavScan;
    sf::FloatRect mNavSettings;

    bool mHoverNavHome;
    bool mHoverNavGen;
    bool mHoverNavScan;
    bool mHoverNavSettings;
};

#endif // HOMESCREEN_H

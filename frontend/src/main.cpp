
#include <SFML/Graphics.hpp>
#include "../include/UITheme.h"
#include "../include/ui_assets.hpp"
#include "../include/splash.hpp"
#include "../include/HomeScreen.h"
#include "../include/GenerateScreen.h"
#include "../include/ReadScreen.h"

int main() {
    // 1. Run Cinematic Intruders Splash Screen 
    runSplash();

    // 2. Create Main Window with 8x Antialiasing
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    sf::RenderWindow window(
        sf::VideoMode(Theme::WIN_W, Theme::WIN_H),
        "ScanIT - Barcode Studio | Built by Intruders",
        sf::Style::Close | sf::Style::Titlebar,
        settings
    );
    window.setFramerateLimit(60);

    // Center window on screen
    const sf::VideoMode desk = sf::VideoMode::getDesktopMode();
    window.setPosition({ int(desk.width - Theme::WIN_W) / 2, int(desk.height - Theme::WIN_H) / 2 });

    // Initialize Asset Manager (Inter & JetBrains Mono fonts, pre-rendered glass panels)
    Assets::instance().init();

    // Set Window Icon from new brand assets
    sf::Image iconImg;
    std::vector<std::string> iconPaths = {
        "assets/brand/window-icon-v2.png",
        "frontend/assets/brand/window-icon-v2.png",
        "assets/brand/window-icon.png",
        "frontend/assets/brand/window-icon.png"
    };
    for (const auto& ip : iconPaths) {
        if (iconImg.loadFromFile(ip)) {
            window.setIcon(iconImg.getSize().x, iconImg.getSize().y, iconImg.getPixelsPtr());
            break;
        }
    }

    // Initialize Application Screens
    HomeScreen     home(window, Assets::instance().fontRegular());
    GenerateScreen generate(window, Assets::instance().fontRegular());
    ReadScreen     read(window, Assets::instance().fontRegular());

    // Connect real-time activity log callbacks (immediate timeline updates)
    generate.setOnGenerateCallback([&home](const std::string& code, const std::string& source) {
        home.addRecentScan(code, source);
    });
    read.setOnScanCallback([&home](const std::string& code, const std::string& source) {
        home.addRecentScan(code, source);
    });

    AppScreen currentScreen = AppScreen::Home;
    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.016f;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (currentScreen == AppScreen::Home) {
                AppScreen next = home.handleEvent(event);
                if (next != AppScreen::Home) currentScreen = next;
            } else if (currentScreen == AppScreen::Generate) {
                AppScreen next = generate.handleEvent(event);
                if (next == AppScreen::Home) {
                    currentScreen = AppScreen::Home;
                }
            } else if (currentScreen == AppScreen::Read) {
                AppScreen next = read.handleEvent(event);
                if (next == AppScreen::Home) {
                    currentScreen = AppScreen::Home;
                }
            }
        }

        // Screen updates (laser animation in scanner)
        if (currentScreen == AppScreen::Read) {
            read.update(dt);
        }

        window.clear(Theme::Background);
        switch (currentScreen) {
            case AppScreen::Home:     home.draw();     break;
            case AppScreen::Generate: generate.draw(); break;
            case AppScreen::Read:     read.draw();     break;
            default: break;
        }
        window.display();
    }

    return 0;
}

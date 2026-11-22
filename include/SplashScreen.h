#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include <SFML/Graphics.hpp>
#include <string>

class SplashScreen {
public:
    SplashScreen();
    bool init(const std::string& assetsDir, const sf::Font& fontRegular, const sf::Font& fontSemiBold, const sf::Font& fontBold, const sf::Font& fontSerif);
    void update(float dt);
    void render(sf::RenderTarget& target, float designWidth, float designHeight);
    bool isFinished() const;
    void skip();
    float getProgress() const;

private:
    float mTimer = 0.f;
    float mDuration = 2.4f;
    bool mFinished = false;

    const sf::Font* mFontRegular = nullptr;
    const sf::Font* mFontSemiBold = nullptr;
    const sf::Font* mFontBold = nullptr;
    const sf::Font* mFontSerif = nullptr;

    sf::Texture mBgTexture;
    sf::Texture mEmblemTexture;
    bool mHasBg = false;
    bool mHasEmblem = false;

    std::string mBrandingTarget = "BUILT BY INTRUDERS";
};

#endif // SPLASH_SCREEN_H

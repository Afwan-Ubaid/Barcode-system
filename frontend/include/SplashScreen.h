#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H
#include <SFML/Graphics.hpp>
#include <string>
class SplashScreen {
public:
    SplashScreen(sf::RenderWindow& window, sf::Font& font);
    bool load();
    bool update(float deltaTime);
    void handleEvent(const sf::Event& event);
    void draw();

    bool isFinished() const { return mFinished; }

private:
    sf::RenderWindow&  mWindow;
    sf::Font&          mFont;

    sf::Texture        mIconTexture;
    sf::Sprite         mIconSprite;
    bool               mIconLoaded;

    // Glowing Neon Rings behind 3D icon
    sf::CircleShape    mRingOuter;
    sf::CircleShape    mRingInner;
    sf::CircleShape    mCoreGlow;

    // Texts
    sf::Text           mTitle;
    sf::Text           mVersionBadge;
    sf::RectangleShape mBadgeBox;
    sf::Text           mSubtitle;
    sf::Text           mBuiltBy;
    sf::Text           mLoadingText;
    sf::Text           mPercentText;
    sf::Text           mSkipHint;

    // Loading bar
    sf::RectangleShape mBarBG;
    sf::RectangleShape mBarFill;

    float mProgress;
    float mElapsed;
    float mTotalDuration;
    float mFloatOffset;
    float mFloatTime;
    bool  mFinished;
};

#endif // SPLASHSCREEN_H

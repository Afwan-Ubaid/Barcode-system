#include "../include/SplashScreen.h"
#include "../include/UITheme.h"
#include "../include/ui_assets.hpp"
#include <cmath>

SplashScreen::SplashScreen(sf::RenderWindow& window, sf::Font& font)
    : mWindow(window), mFont(font),
      mIconLoaded(false), mProgress(0.f), mElapsed(0.f),
      mTotalDuration(4.5f), mFloatOffset(0.f), mFloatTime(0.f),
      mFinished(false)
{
    // Neon Ring Backdrops
    mRingOuter.setRadius(105.f);
    mRingOuter.setPointCount(60);
    mRingOuter.setFillColor(sf::Color(35, 12, 70, 90));
    mRingOuter.setOutlineColor(sf::Color(145, 75, 255, 140));
    mRingOuter.setOutlineThickness(2.f);

    mRingInner.setRadius(85.f);
    mRingInner.setPointCount(60);
    mRingInner.setFillColor(sf::Color(55, 18, 105, 120));
    mRingInner.setOutlineColor(Theme::Cyan);
    mRingInner.setOutlineThickness(1.5f);

    mCoreGlow.setRadius(65.f);
    mCoreGlow.setFillColor(sf::Color(80, 30, 160, 140));

    sf::Font& fBold = Assets::instance().fontBold();
    sf::Font& fSemi = Assets::instance().fontSemiBold();
    sf::Font& fReg  = Assets::instance().fontRegular();

    // Title
    mTitle.setFont(fBold);
    mTitle.setString("ScanIT");
    mTitle.setCharacterSize(56);
    mTitle.setFillColor(Theme::TextWhite);
    mTitle.setStyle(sf::Text::Bold);

    // Version Badge
    mBadgeBox.setSize(sf::Vector2f(90.f, 28.f));
    mBadgeBox.setFillColor(sf::Color(Theme::Purple.r, Theme::Purple.g, Theme::Purple.b, 45));
    mBadgeBox.setOutlineColor(Theme::Purple);
    mBadgeBox.setOutlineThickness(1.2f);

    mVersionBadge.setFont(fSemi);
    mVersionBadge.setString("v2.4 PRO");
    mVersionBadge.setCharacterSize(13);
    mVersionBadge.setFillColor(Theme::Cyan);
    mVersionBadge.setStyle(sf::Text::Bold);

    // Subtitle
    mSubtitle.setFont(fReg);
    mSubtitle.setString("3D Optical Barcode Studio & Neural Vision Engine");
    mSubtitle.setCharacterSize(15);
    mSubtitle.setFillColor(Theme::TextWhite);

    // Built by Intruders
    mBuiltBy.setFont(fSemi);
    mBuiltBy.setString("──  Built by Intruders  ──");
    mBuiltBy.setCharacterSize(15);
    mBuiltBy.setFillColor(Theme::PurpleSoft);
    mBuiltBy.setStyle(sf::Text::Bold);

    // Loading status text
    mLoadingText.setFont(fReg);
    mLoadingText.setString("Initializing Optical Core Engine...");
    mLoadingText.setCharacterSize(13);
    mLoadingText.setFillColor(Theme::TextMuted);

    mPercentText.setFont(fBold);
    mPercentText.setString("0%");
    mPercentText.setCharacterSize(13);
    mPercentText.setFillColor(Theme::Cyan);
    mPercentText.setStyle(sf::Text::Bold);

    // Loading Bar
    mBarBG.setSize(sf::Vector2f(380.f, 10.f));
    mBarBG.setPosition((Theme::WIN_W - 380.f) / 2.f, 530.f);
    mBarBG.setFillColor(sf::Color(25, 12, 45));
    mBarBG.setOutlineColor(Theme::PanelBorder);
    mBarBG.setOutlineThickness(1.5f);

    mBarFill.setSize(sf::Vector2f(0.f, 10.f));
    mBarFill.setPosition((Theme::WIN_W - 380.f) / 2.f, 530.f);
    mBarFill.setFillColor(Theme::Purple);

    // Skip hint
    mSkipHint.setFont(fReg);
    mSkipHint.setString("Press SPACE or Click anywhere to Launch ->");
    mSkipHint.setCharacterSize(12);
    mSkipHint.setFillColor(sf::Color(140, 120, 180, 180));
}

bool SplashScreen::load() {
    std::vector<std::string> searchPaths = {
        "brand/app-icon-256.png",
        "brand/logo-mark.png",
        "icon_3d.png",
        "icon.png"
    };

    for (const auto& path : searchPaths) {
        sf::Texture& t = Assets::instance().tex(path);
        if (t.getSize().x > 2) {
            mIconLoaded = true;
            mIconSprite.setTexture(t, true);
            auto sz = t.getSize();
            float scale = 170.f / (float)std::max(sz.x, sz.y);
            mIconSprite.setScale(scale, scale);
            break;
        }
    }
    return true;
}

void SplashScreen::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed ||
       (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) ||
       (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return)) {
        mFinished = true;
    }
}

bool SplashScreen::update(float deltaTime) {
    if (mFinished) return true;

    mElapsed += deltaTime;
    mFloatTime += deltaTime;

    mFloatOffset = std::sin(mFloatTime * 2.5f) * 10.f;

    float iconW = mIconLoaded ? mIconSprite.getGlobalBounds().width : 160.f;
    float iconX = (Theme::WIN_W - iconW) / 2.f;
    float iconY = 110.f + mFloatOffset;

    if (mIconLoaded) {
        mIconSprite.setPosition(iconX, iconY);
    }

    mRingOuter.setPosition((Theme::WIN_W - 210.f) / 2.f, 90.f + mFloatOffset);
    mRingInner.setPosition((Theme::WIN_W - 170.f) / 2.f, 110.f + mFloatOffset);
    mCoreGlow.setPosition((Theme::WIN_W - 130.f) / 2.f, 130.f + mFloatOffset);

    float pulse = (std::sin(mFloatTime * 4.f) + 1.f) * 0.5f;
    mRingOuter.setOutlineColor(sf::Color(145, 75, 255, (sf::Uint8)(120 + pulse * 100)));

    mProgress = std::min(1.f, mElapsed / mTotalDuration);
    mBarFill.setSize(sf::Vector2f(380.f * mProgress, 10.f));

    int pct = static_cast<int>(mProgress * 100.f);
    mPercentText.setString(std::to_string(pct) + "%");

    if (mProgress < 0.25f) {
        mLoadingText.setString("Loading High-Res Glass Panels & Lucide Icons...");
    } else if (mProgress < 0.55f) {
        mLoadingText.setString("Calibrating Symbology Matrix Engine...");
    } else if (mProgress < 0.85f) {
        mLoadingText.setString("Activating Optical Decoder & Laptop Camera Bus...");
    } else {
        mLoadingText.setString("Engine Ready! Launching...");
    }

    if (mProgress >= 1.f) {
        mFinished = true;
    }

    return mFinished;
}

void SplashScreen::draw() {
    // Background
    sf::Texture& bgTex = Assets::instance().tex("ui/background.png");
    if (bgTex.getSize().x > 2) {
        sf::Sprite bg(bgTex);
        mWindow.draw(bg);
    } else {
        mWindow.clear(Theme::Background);
    }

    // Additive glow behind 3D icon
    Assets::instance().glow(mWindow, { (float)Theme::WIN_W * 0.5f, 190.f + mFloatOffset }, 190.f, Theme::PurpleSoft);

    // Glowing Rings
    mWindow.draw(mRingOuter);
    mWindow.draw(mRingInner);
    mWindow.draw(mCoreGlow);

    // 3D Floating Icon
    if (mIconLoaded) {
        mWindow.draw(mIconSprite);
    }

    // Title & Badge
    float titleW = mTitle.getLocalBounds().width;
    float startX = (Theme::WIN_W - (titleW + 105.f)) / 2.f;

    mTitle.setPosition(startX, 310.f);
    mWindow.draw(mTitle);

    mBadgeBox.setPosition(startX + titleW + 12.f, 332.f);
    mWindow.draw(mBadgeBox);

    mVersionBadge.setPosition(startX + titleW + 20.f, 337.f);
    mWindow.draw(mVersionBadge);

    // Subtitle
    float subX = Theme::centerX(mSubtitle, 0, Theme::WIN_W);
    mSubtitle.setPosition(subX, 385.f);
    mWindow.draw(mSubtitle);

    // Built by Intruders
    float bx = Theme::centerX(mBuiltBy, 0, Theme::WIN_W);
    mBuiltBy.setPosition(bx, 420.f);
    mWindow.draw(mBuiltBy);

    // Loading Bar
    mWindow.draw(mBarBG);
    mWindow.draw(mBarFill);

    // Loading text & percent
    float lx = (Theme::WIN_W - 380.f) / 2.f;
    mLoadingText.setPosition(lx, 548.f);
    mWindow.draw(mLoadingText);

    float px = (Theme::WIN_W + 380.f) / 2.f - mPercentText.getLocalBounds().width;
    mPercentText.setPosition(px, 548.f);
    mWindow.draw(mPercentText);

    // Skip Hint
    float hx = Theme::centerX(mSkipHint, 0, Theme::WIN_W);
    mSkipHint.setPosition(hx, 620.f);
    mWindow.draw(mSkipHint);
}

#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "LoginTheme.h"

enum class LoginRole {
    Admin,
    Employee
};

enum class LoginResult {
    None,
    SuccessCashier,
    SuccessAdmin,
    ScannerLoginRequested
};

class LoginScreen {
public:
    LoginScreen();

    // Standard SFML 2.5.1 initialization by window size
    bool init(const sf::Vector2u& windowSize);

    // ScanIT PosApp initialization with asset directory and loaded fonts
    bool init(const std::string& assetsDir,
              const sf::Font& fontRegular,
              const sf::Font& fontMedium,
              const sf::Font& fontSemiBold,
              const sf::Font& fontBold,
              const sf::Font& fontSerif);

    // Event handling
    LoginResult handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    LoginResult handleEvent(const sf::Event& event, const sf::Vector2f& mousePos);

    void update(float dt);
    void draw(sf::RenderTarget& target) const;
    void render(sf::RenderTarget& target, float designWidth, float designHeight);

    void reset();
    void resetFields();

    bool isLoggedIn() const { return mIsLoggedIn; }
    std::string getLoggedInUser() const { return mLoggedInUser.empty() ? userIdInput : mLoggedInUser; }
    std::string getLoggedInRole() const { return (currentRole == LoginRole::Admin) ? "Admin" : "Employee"; }
    LoginRole getRole() const { return currentRole; }
    const std::string& getEnteredUser() const { return userIdInput; }

    void setAuthCallback(std::function<bool(const std::string& user, const std::string& pass, const std::string& role)> callback) {
        mAuthCallback = callback;
    }

private:
    sf::Vector2u winSize{1280, 720};
    float elapsed = 0.f;

    sf::Texture bgTexture;
    sf::Sprite bgSprite;

    sf::Font serifFont;
    sf::Font sansFont;

    // Font pointers (from PosApp or local)
    const sf::Font* mFontRegular = nullptr;
    const sf::Font* mFontMedium = nullptr;
    const sf::Font* mFontSemiBold = nullptr;
    const sf::Font* mFontBold = nullptr;
    const sf::Font* mFontSerif = nullptr;

    // Liquid-glass login card & form geometry
    sf::FloatRect cardRect;
    sf::FloatRect roleAdminRect;
    sf::FloatRect roleEmployeeRect;
    sf::FloatRect userFieldRect;
    sf::FloatRect passFieldRect;
    sf::FloatRect eyeBtnRect;
    sf::FloatRect loginBtnRect;
    sf::FloatRect resetBtnRect;

    // Active state
    LoginRole currentRole = LoginRole::Admin;
    int activeField = 0; // 0 = User ID, 1 = Password, -1 = None
    bool showPassword = false;

    std::string userIdInput;
    std::string passwordInput;
    std::string statusMessage;
    float statusTimer = 0.f;

    bool mIsLoggedIn = false;
    std::string mLoggedInUser;
    std::function<bool(const std::string&, const std::string&, const std::string&)> mAuthCallback;

    // Animation variables
    float rolePillX = 0.f;
    float rolePillTargetX = 0.f;
    float buttonScale = 1.0f;
    bool buttonPressed = false;
    float cursorTimer = 0.f;
    bool cursorVisible = true;

    // Mouse interaction states
    sf::Vector2f currentMousePos{0.f, 0.f};
    bool hoveredAdminRole = false;
    bool hoveredEmployeeRole = false;
    bool hoveredLoginBtn = false;
    bool hoveredResetBtn = false;
    bool hoveredEyeBtn = false;

    void updateLayout(float w, float h);
    LoginResult attemptLogin();

    sf::ConvexShape makeRoundedBox(sf::Vector2f pos, sf::Vector2f size, float radius,
                                   sf::Color fill, sf::Color outline, float thickness = 1.5f) const;
};

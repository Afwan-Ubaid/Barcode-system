#include "LoginScreen.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

LoginScreen::LoginScreen() {
    reset();
}

bool LoginScreen::init(const sf::Vector2u& windowSize) {
    winSize = windowSize;

    // Load background texture
    std::vector<std::string> bgCandidates = {
        "assets/login/bg_login_athelia.png",
        "../assets/login/bg_login_athelia.png",
        "../../assets/login/bg_login_athelia.png",
        "frontend/assets/login/bg_login_athelia.png"
    };

    bool bgLoaded = false;
    for (const auto& path : bgCandidates) {
        if (fs::exists(path) && bgTexture.loadFromFile(path)) {
            bgTexture.setSmooth(true);
            bgLoaded = true;
            break;
        }
    }
    if (!bgLoaded) return false;

    // Fonts fallback
    std::vector<std::string> serifCandidates = {
        "assets/fonts/Lora.ttf",
        "../assets/fonts/Lora.ttf",
        "assets/fonts/PlayfairDisplay-SemiBold.ttf"
    };
    for (const auto& path : serifCandidates) {
        if (fs::exists(path) && serifFont.loadFromFile(path)) {
            break;
        }
    }

    std::vector<std::string> sansCandidates = {
        "assets/fonts/Inter-Regular.ttf",
        "assets/fonts/Inter-Regular.otf",
        "../assets/fonts/Inter-Regular.ttf"
    };
    for (const auto& path : sansCandidates) {
        if (fs::exists(path) && sansFont.loadFromFile(path)) {
            break;
        }
    }

    updateLayout(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    return true;
}

bool LoginScreen::init(const std::string& assetsDir,
                       const sf::Font& fontRegular,
                       const sf::Font& fontMedium,
                       const sf::Font& fontSemiBold,
                       const sf::Font& fontBold,
                       const sf::Font& fontSerif) {
    mFontRegular = &fontRegular;
    mFontMedium = &fontMedium;
    mFontSemiBold = &fontSemiBold;
    mFontBold = &fontBold;
    mFontSerif = &fontSerif;

    std::string bgPath = assetsDir + "/login/bg_login_athelia.png";
    if (fs::exists(bgPath)) {
        if (bgTexture.loadFromFile(bgPath)) {
            bgTexture.setSmooth(true);
        }
    } else {
        init(winSize);
    }

    updateLayout(static_cast<float>(winSize.x), static_cast<float>(winSize.y));
    reset();
    return true;
}

void LoginScreen::updateLayout(float w, float h) {
    winSize = {static_cast<unsigned>(w), static_cast<unsigned>(h)};

    if (bgTexture.getSize().x > 0 && bgTexture.getSize().y > 0) {
        bgSprite.setTexture(bgTexture);
        bgSprite.setScale(
            w / static_cast<float>(bgTexture.getSize().x),
            h / static_cast<float>(bgTexture.getSize().y)
        );
        // Keep the artwork completely natural, bright, and clean at full 100% resolution
        bgSprite.setColor(LoginTheme::ArtworkTint);
    }

    // Login Card Geometry
    const float cardW = std::min(450.f, w * 0.34f);
    const float cardH = 460.f;
    const float cardX = w * 0.57f;
    const float cardY = (h - cardH) * 0.5f + 30.f;
    cardRect = {cardX, cardY, cardW, cardH};

    const float padX = 32.f;
    const float contentX = cardX + padX;
    const float contentW = cardW - (padX * 2.f);

    float curY = cardY + 70.f;

    // Role selector track & buttons
    const float roleH = 38.f;
    const float halfW = contentW * 0.5f;
    roleAdminRect = {contentX, curY, halfW, roleH};
    roleEmployeeRect = {contentX + halfW, curY, halfW, roleH};
    rolePillTargetX = (currentRole == LoginRole::Admin) ? roleAdminRect.left : roleEmployeeRect.left;
    if (rolePillX == 0.f) rolePillX = rolePillTargetX;

    curY += roleH + 24.f;

    // User ID field
    const float inputH = 42.f;
    userFieldRect = {contentX, curY + 18.f, contentW, inputH};
    curY += inputH + 34.f;

    // Password field & eye button
    passFieldRect = {contentX, curY + 18.f, contentW, inputH};
    const float eyeW = 42.f;
    eyeBtnRect = {passFieldRect.left + passFieldRect.width - eyeW - 4.f, passFieldRect.top + 4.f, eyeW, inputH - 8.f};
    curY += inputH + 36.f;

    // Action buttons row (SIGN IN & RESET)
    const float btnH = 44.f;
    const float resetW = 96.f;
    const float loginW = contentW - resetW - 12.f;
    loginBtnRect = {contentX, curY, loginW, btnH};
    resetBtnRect = {contentX + loginW + 12.f, curY, resetW, btnH};
}

LoginResult LoginScreen::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    return handleEvent(event, mp);
}

LoginResult LoginScreen::handleEvent(const sf::Event& event, const sf::Vector2f& mp) {
    currentMousePos = mp;

    // Hover detection
    hoveredAdminRole = roleAdminRect.contains(mp);
    hoveredEmployeeRole = roleEmployeeRect.contains(mp);
    hoveredLoginBtn = loginBtnRect.contains(mp);
    hoveredResetBtn = resetBtnRect.contains(mp);
    hoveredEyeBtn = eyeBtnRect.contains(mp);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        // Role selection
        if (hoveredAdminRole) {
            currentRole = LoginRole::Admin;
            rolePillTargetX = roleAdminRect.left;
            statusMessage.clear();
            return LoginResult::None;
        }
        if (hoveredEmployeeRole) {
            currentRole = LoginRole::Employee;
            rolePillTargetX = roleEmployeeRect.left;
            statusMessage.clear();
            return LoginResult::None;
        }

        // Eye toggle button
        if (hoveredEyeBtn) {
            showPassword = !showPassword;
            return LoginResult::None;
        }

        // Input field selection
        if (userFieldRect.contains(mp)) {
            activeField = 0;
            cursorVisible = true;
            cursorTimer = 0.f;
            return LoginResult::None;
        }
        if (passFieldRect.contains(mp)) {
            activeField = 1;
            cursorVisible = true;
            cursorTimer = 0.f;
            return LoginResult::None;
        }

        // Action buttons
        if (hoveredResetBtn) {
            resetFields();
            return LoginResult::None;
        }
        if (hoveredLoginBtn) {
            buttonPressed = true;
            buttonScale = 0.94f;
            return attemptLogin();
        }

        // Click outside fields
        activeField = -1;
    } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        buttonPressed = false;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Tab) {
            activeField = (activeField == 0) ? 1 : 0;
            cursorVisible = true;
            cursorTimer = 0.f;
        } else if (event.key.code == sf::Keyboard::Return) {
            buttonPressed = true;
            buttonScale = 0.94f;
            return attemptLogin();
        } else if (event.key.code == sf::Keyboard::Escape) {
            activeField = -1;
        }
    }

    if (event.type == sf::Event::TextEntered) {
        uint32_t uni = event.text.unicode;
        if (uni == 8) { // Backspace
            if (activeField == 0 && !userIdInput.empty()) userIdInput.pop_back();
            else if (activeField == 1 && !passwordInput.empty()) passwordInput.pop_back();
        } else if (uni >= 32 && uni < 127) {
            char c = static_cast<char>(uni);
            if (activeField == 0 && userIdInput.size() < 28) userIdInput.push_back(c);
            else if (activeField == 1 && passwordInput.size() < 28) passwordInput.push_back(c);
        }
    }

    return LoginResult::None;
}

LoginResult LoginScreen::attemptLogin() {
    std::string roleStr = (currentRole == LoginRole::Admin) ? "Admin" : "Employee";

    std::string u = userIdInput;
    size_t first = u.find_first_not_of(" \t\r\n");
    if (first != std::string::npos) {
        size_t last = u.find_last_not_of(" \t\r\n");
        u = u.substr(first, last - first + 1);
    } else {
        u.clear();
    }

    if (u.empty()) {
        statusMessage = "Please enter your operator name or user ID";
        statusTimer = 3.5f;
        activeField = 0;
        return LoginResult::None;
    }

    if (passwordInput.empty()) {
        statusMessage = "Please enter your password";
        statusTimer = 3.5f;
        activeField = 1;
        return LoginResult::None;
    }

    bool authenticated = false;
    if (mAuthCallback) {
        authenticated = mAuthCallback(u, passwordInput, roleStr);
    } else {
        if (currentRole == LoginRole::Admin) {
            authenticated = (u == "admin" && (passwordInput == "admin" || passwordInput == "admin123"));
        } else {
            authenticated = (u == "cashier1" && (passwordInput == "1234" || passwordInput == "cashier1"));
        }
    }

    if (authenticated) {
        mIsLoggedIn = true;
        mLoggedInUser = u;
        statusMessage = "Authenticated successfully!";
        statusTimer = 2.0f;
        return (currentRole == LoginRole::Admin) ? LoginResult::SuccessAdmin : LoginResult::SuccessCashier;
    } else {
        statusMessage = "Invalid credentials for " + roleStr + " tier!";
        statusTimer = 4.0f;
        return LoginResult::None;
    }
}

void LoginScreen::update(float dt) {
    elapsed += dt;

    // Smooth sliding animation for role pill
    rolePillTargetX = (currentRole == LoginRole::Admin) ? roleAdminRect.left : roleEmployeeRect.left;
    rolePillX += (rolePillTargetX - rolePillX) * std::min(1.0f, dt * 14.f);

    // Button spring bounce
    if (!buttonPressed && buttonScale < 1.0f) {
        buttonScale = std::min(1.0f, buttonScale + dt * 4.5f);
    }

    // Cursor blink
    cursorTimer += dt;
    if (cursorTimer >= 0.5f) {
        cursorTimer = 0.f;
        cursorVisible = !cursorVisible;
    }

    // Status message countdown
    if (statusTimer > 0.f) {
        statusTimer -= dt;
        if (statusTimer <= 0.f) {
            statusMessage.clear();
        }
    }
}

void LoginScreen::render(sf::RenderTarget& target, float designWidth, float designHeight) {
    if (static_cast<unsigned>(designWidth) != winSize.x || static_cast<unsigned>(designHeight) != winSize.y) {
        updateLayout(designWidth, designHeight);
    }
    draw(target);
}

void LoginScreen::draw(sf::RenderTarget& target) const {
    const sf::Font& fontS = (mFontSerif ? *mFontSerif : serifFont);
    const sf::Font& fontR = (mFontRegular ? *mFontRegular : sansFont);
    const sf::Font& fontM = (mFontMedium ? *mFontMedium : (mFontRegular ? *mFontRegular : sansFont));
    const sf::Font& fontSB = (mFontSemiBold ? *mFontSemiBold : (mFontBold ? *mFontBold : sansFont));

    // ── 1. CLEAN ORIGINAL WATERCOLOR CANVAS (Bright, Natural & High Resolution) ──
    sf::RectangleShape parchmentBg({static_cast<float>(winSize.x), static_cast<float>(winSize.y)});
    parchmentBg.setFillColor(LoginTheme::ParchmentCanvas);
    target.draw(parchmentBg);

    // Draw background artwork at 100% natural color (Tree, Bunnies, Athena, Hermes, Temples)
    target.draw(bgSprite);

    // ── 2. LOGIN SIDE ANIMATION (Behind the dark card only) ──
    const float cardX = cardRect.left;
    const float cardY = cardRect.top;
    const float cardW = cardRect.width;
    const float cardH = cardRect.height;
    const float cx = cardX + cardW * 0.5f;
    const float cy = cardY + cardH * 0.5f;

    // Atmospheric Warm Bronze and Sage breathing glow orbs behind the card
    float orb1X = std::sin(elapsed * 0.45f) * 35.f;
    float orb1Y = std::cos(elapsed * 0.35f) * 25.f;
    sf::CircleShape orb1(150.f);
    orb1.setOrigin(150.f, 150.f);
    orb1.setPosition(cx - 40.f + orb1X, cy - 50.f + orb1Y);
    orb1.setFillColor(sf::Color(165, 120, 78, 22));
    target.draw(orb1);

    float orb2X = std::cos(elapsed * 0.40f) * -40.f;
    float orb2Y = std::sin(elapsed * 0.50f) * 30.f;
    sf::CircleShape orb2(170.f);
    orb2.setOrigin(170.f, 170.f);
    orb2.setPosition(cx + 50.f + orb2X, cy + 60.f + orb2Y);
    orb2.setFillColor(sf::Color(126, 154, 118, 26));
    target.draw(orb2);

    // ── 3. HEADER BRANDING ABOVE LOGIN CARD ──
    sf::Text brand("ScanIT", fontS, 52);
    brand.setFillColor(LoginTheme::InkBrandTitle);
    brand.setPosition(cardX + (cardW - brand.getLocalBounds().width) * 0.5f, cardY - 122.f);
    target.draw(brand);

    sf::Text sub("Smart Barcode Studio & POS", fontM, 16);
    sub.setFillColor(LoginTheme::InkBrandSubtitle);
    sub.setPosition(cardX + (cardW - sub.getLocalBounds().width) * 0.5f, cardY - 60.f);
    target.draw(sub);

    sf::Text badge("SALES COUNTER | OPERATOR ACCESS CONTROL", fontSB, 10);
    badge.setFillColor(LoginTheme::InkBrandCaption);
    badge.setLetterSpacing(1.5f);
    badge.setPosition(cardX + (cardW - badge.getLocalBounds().width) * 0.5f, cardY - 32.f);
    target.draw(badge);

    // ── 4. DARK EYE-PROTECTED FROSTED GLASS LOGIN CARD ──
    // Soft drop shadow behind the card
    target.draw(makeRoundedBox({cardX + 6.f, cardY + 12.f}, {cardW, cardH}, 22.f,
                               LoginTheme::GlassCardShadow, sf::Color::Transparent, 0.f));

    // Breathing rim border
    float rimAlpha = 110.f + 35.f * std::sin(elapsed * 1.8f);
    sf::Color specularRim(195, 175, 145, static_cast<sf::Uint8>(rimAlpha));
    target.draw(makeRoundedBox({cardX, cardY}, {cardW, cardH}, 22.f,
                               LoginTheme::GlassCardFill, specularRim, 1.8f));

    // Card Title (Ivory inside dark glass)
    std::string titleStr = (currentRole == LoginRole::Admin) ? "Manager Login" : "Cashier Login";
    sf::Text title(titleStr, fontS, 24);
    title.setFillColor(LoginTheme::CardTitle);
    title.setPosition(cardX + 32.f, cardY + 24.f);
    target.draw(title);

    // ── 5. ANIMATED ROLE SELECTOR (ADMIN vs EMPLOYEE) ──
    const float roleH = roleAdminRect.height;
    const float halfW = roleAdminRect.width;

    // Recessed background track
    target.draw(makeRoundedBox({roleAdminRect.left, roleAdminRect.top}, {halfW * 2.f, roleH},
                               roleH * 0.5f, LoginTheme::RoleTrackBg, LoginTheme::RoleTrackBorder, 1.0f));

    // Smooth sliding active pill indicator
    sf::Color activePillColor = (currentRole == LoginRole::Admin) ? LoginTheme::RolePillAdmin : LoginTheme::RolePillEmployee;
    target.draw(makeRoundedBox({rolePillX + 2.f, roleAdminRect.top + 2.f}, {halfW - 4.f, roleH - 4.f},
                               (roleH - 4.f) * 0.5f, activePillColor, sf::Color(255, 255, 255, 50), 1.0f));

    // Manager label
    sf::Text adminLabel("Manager", fontSB, 12);
    adminLabel.setFillColor((currentRole == LoginRole::Admin) ? LoginTheme::RoleTextActive : LoginTheme::RoleTextIdle);
    sf::FloatRect ab = adminLabel.getLocalBounds();
    adminLabel.setOrigin(std::round(ab.left + ab.width * 0.5f), std::round(ab.top + ab.height * 0.5f));
    adminLabel.setPosition(roleAdminRect.left + halfW * 0.5f, roleAdminRect.top + roleH * 0.5f);
    target.draw(adminLabel);

    // Cashier label
    sf::Text empLabel("Cashier", fontSB, 12);
    empLabel.setFillColor((currentRole == LoginRole::Employee) ? LoginTheme::RoleTextActive : LoginTheme::RoleTextIdle);
    sf::FloatRect eb = empLabel.getLocalBounds();
    empLabel.setOrigin(std::round(eb.left + eb.width * 0.5f), std::round(eb.top + eb.height * 0.5f));
    empLabel.setPosition(roleEmployeeRect.left + halfW * 0.5f, roleEmployeeRect.top + roleH * 0.5f);
    target.draw(empLabel);

    // ── 6. INPUT FIELD 1: USER ID / OPERATOR NAME ──
    sf::Text lblUser("Operator Name / User ID", fontM, 12);
    lblUser.setFillColor(LoginTheme::CardLabel);
    lblUser.setPosition(userFieldRect.left, userFieldRect.top - 20.f);
    target.draw(lblUser);

    bool userFocused = (activeField == 0);
    sf::Color userBorder = userFocused ? LoginTheme::InputBorderFocus : LoginTheme::InputBorderIdle;
    sf::Color userFill = userFocused ? LoginTheme::InputFieldFocus : LoginTheme::InputFieldBg;
    target.draw(makeRoundedBox({userFieldRect.left, userFieldRect.top}, {userFieldRect.width, userFieldRect.height},
                               10.f, userFill, userBorder, userFocused ? 1.8f : 1.0f));

    if (userIdInput.empty() && !userFocused) {
        std::string hintStr = (currentRole == LoginRole::Admin) ? "e.g. admin" : "e.g. cashier1";
        sf::Text hint(hintStr, fontR, 13);
        hint.setFillColor(LoginTheme::InputPlaceholder);
        hint.setPosition(userFieldRect.left + 14.f, userFieldRect.top + 10.f);
        target.draw(hint);
    } else {
        std::string shownUser = userIdInput;
        if (userFocused && cursorVisible) shownUser += "|";
        sf::Text txtUser(shownUser, fontR, 14);
        txtUser.setFillColor(LoginTheme::InputText);
        txtUser.setPosition(userFieldRect.left + 14.f, userFieldRect.top + 10.f);
        target.draw(txtUser);
    }

    // ── 7. INPUT FIELD 2: PASSWORD ──
    sf::Text lblPass("Security Password", fontM, 12);
    lblPass.setFillColor(LoginTheme::CardLabel);
    lblPass.setPosition(passFieldRect.left, passFieldRect.top - 20.f);
    target.draw(lblPass);

    bool passFocused = (activeField == 1);
    sf::Color passBorder = passFocused ? LoginTheme::InputBorderFocus : LoginTheme::InputBorderIdle;
    sf::Color passFill = passFocused ? LoginTheme::InputFieldFocus : LoginTheme::InputFieldBg;
    target.draw(makeRoundedBox({passFieldRect.left, passFieldRect.top}, {passFieldRect.width, passFieldRect.height},
                               10.f, passFill, passBorder, passFocused ? 1.8f : 1.0f));

    // Eye button [ SHOW / HIDE ]
    sf::Color eyeFill = hoveredEyeBtn ? sf::Color(65, 55, 48, 200) : sf::Color(40, 34, 30, 180);
    target.draw(makeRoundedBox({eyeBtnRect.left, eyeBtnRect.top}, {eyeBtnRect.width, eyeBtnRect.height},
                               6.f, eyeFill, sf::Color(80, 70, 60), 1.0f));
    sf::Text eyeText(showPassword ? "HIDE" : "SHOW", fontSB, 9);
    eyeText.setFillColor(showPassword ? LoginTheme::RolePillEmployee : LoginTheme::CardLabel);
    sf::FloatRect etb = eyeText.getLocalBounds();
    eyeText.setOrigin(std::round(etb.left + etb.width * 0.5f), std::round(etb.top + etb.height * 0.5f));
    eyeText.setPosition(eyeBtnRect.left + eyeBtnRect.width * 0.5f, eyeBtnRect.top + eyeBtnRect.height * 0.5f);
    target.draw(eyeText);

    if (passwordInput.empty() && !passFocused) {
        sf::Text hint("Enter security password", fontR, 13);
        hint.setFillColor(LoginTheme::InputPlaceholder);
        hint.setPosition(passFieldRect.left + 14.f, passFieldRect.top + 10.f);
        target.draw(hint);
    } else {
        std::string shownPass = showPassword ? passwordInput : std::string(passwordInput.size(), '*');
        if (passFocused && cursorVisible) shownPass += "|";
        sf::Text txtPass(shownPass, fontR, 14);
        txtPass.setFillColor(LoginTheme::InputText);
        txtPass.setPosition(passFieldRect.left + 14.f, passFieldRect.top + 10.f);
        target.draw(txtPass);
    }

    // ── 8. ACTION BUTTONS (SIGN IN & RESET) ──
    // SIGN IN button with spring scale on click
    float scW = loginBtnRect.width * buttonScale;
    float scH = loginBtnRect.height * buttonScale;
    float scX = loginBtnRect.left + (loginBtnRect.width - scW) * 0.5f;
    float scY = loginBtnRect.top + (loginBtnRect.height - scH) * 0.5f;

    sf::Color loginFill = hoveredLoginBtn ? LoginTheme::LoginBtnHover : LoginTheme::LoginBtnFill;
    target.draw(makeRoundedBox({scX, scY}, {scW, scH}, 11.f, loginFill, sf::Color(255, 255, 255, 60), 1.2f));

    std::string btnTextStr = (currentRole == LoginRole::Admin) ? "SIGN IN AS MANAGER" : "SIGN IN AS CASHIER";
    sf::Text btnText(btnTextStr, fontSB, 12);
    btnText.setFillColor(sf::Color(18, 15, 13));
    btnText.setLetterSpacing(1.1f);
    sf::FloatRect bb = btnText.getLocalBounds();
    btnText.setOrigin(std::round(bb.left + bb.width * 0.5f), std::round(bb.top + bb.height * 0.5f));
    btnText.setPosition(loginBtnRect.left + loginBtnRect.width * 0.5f, loginBtnRect.top + loginBtnRect.height * 0.5f);
    target.draw(btnText);

    // RESET button
    sf::Color resetFill = hoveredResetBtn ? LoginTheme::ResetBtnHover : LoginTheme::ResetBtnFill;
    target.draw(makeRoundedBox({resetBtnRect.left, resetBtnRect.top}, {resetBtnRect.width, resetBtnRect.height},
                               11.f, resetFill, sf::Color(85, 75, 65), 1.0f));

    sf::Text resetText("RESET", fontSB, 12);
    resetText.setFillColor(LoginTheme::CardLabel);
    sf::FloatRect rb = resetText.getLocalBounds();
    resetText.setOrigin(std::round(rb.left + rb.width * 0.5f), std::round(rb.top + rb.height * 0.5f));
    resetText.setPosition(resetBtnRect.left + resetBtnRect.width * 0.5f, resetBtnRect.top + resetBtnRect.height * 0.5f);
    target.draw(resetText);

    // ── 9. STATUS / ERROR MESSAGE NOTIFICATION ──
    if (!statusMessage.empty()) {
        bool isSuccess = (statusMessage.find("successfully") != std::string::npos ||
                          statusMessage.find("Authenticated") != std::string::npos);
        sf::Text st(statusMessage, fontM, 11);
        st.setFillColor(isSuccess ? LoginTheme::SageVerified : LoginTheme::TerracottaAlert);
        sf::FloatRect sb = st.getLocalBounds();
        st.setOrigin(std::round(sb.left + sb.width * 0.5f), 0.f);
        st.setPosition(cardX + cardW * 0.5f, loginBtnRect.top + loginBtnRect.height + 12.f);
        target.draw(st);
    } else {
        // Default credentials hint
        std::string credHint = (currentRole == LoginRole::Admin) ? "Default: admin / admin123" : "Default: cashier1 / 1234";
        sf::Text hint(credHint, fontR, 10);
        hint.setFillColor(LoginTheme::InputPlaceholder);
        sf::FloatRect hb = hint.getLocalBounds();
        hint.setOrigin(std::round(hb.left + hb.width * 0.5f), 0.f);
        hint.setPosition(cardX + cardW * 0.5f, loginBtnRect.top + loginBtnRect.height + 14.f);
        target.draw(hint);
    }
}

sf::ConvexShape LoginScreen::makeRoundedBox(sf::Vector2f pos, sf::Vector2f size, float radius,
                                            sf::Color fill, sf::Color outline, float thickness) const {
    radius = std::min(radius, std::min(size.x, size.y) * 0.5f);
    const int pts = 8;
    sf::ConvexShape shape(pts * 4);
    const float pi = 3.14159265f;
    sf::Vector2f c[4] = {
        {pos.x + size.x - radius, pos.y + radius},
        {pos.x + size.x - radius, pos.y + size.y - radius},
        {pos.x + radius,          pos.y + size.y - radius},
        {pos.x + radius,          pos.y + radius}
    };
    int idx = 0;
    for (int i = 0; i < 4; ++i) {
        float a0 = (i - 1) * (pi * 0.5f);
        for (int j = 0; j < pts; ++j) {
            float a = a0 + (pi * 0.5f) * (static_cast<float>(j) / (pts - 1));
            shape.setPoint(idx++, {c[i].x + std::cos(a) * radius, c[i].y + std::sin(a) * radius});
        }
    }
    shape.setFillColor(fill);
    shape.setOutlineColor(outline);
    shape.setOutlineThickness(thickness);
    return shape;
}

void LoginScreen::reset() {
    userIdInput.clear();
    passwordInput.clear();
    statusMessage.clear();
    statusTimer = 0.f;
    activeField = 0;
    currentRole = LoginRole::Admin;
    showPassword = false;
    mIsLoggedIn = false;
    mLoggedInUser.clear();
    buttonScale = 1.0f;
    buttonPressed = false;
}

void LoginScreen::resetFields() {
    userIdInput.clear();
    passwordInput.clear();
    statusMessage.clear();
    statusTimer = 0.f;
    activeField = 0;
}

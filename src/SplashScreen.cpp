#include "SplashScreen.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

static sf::ConvexShape createRoundedRectShape(float x, float y, float w, float h, float r) {
    r = std::min(r, std::min(w, h) / 2.f);
    const int seg = 8;
    std::vector<sf::Vector2f> pts;
    pts.reserve(seg * 4);
    auto arc = [&](float cx, float cy, float startAngle) {
        for (int i = 0; i <= seg; i++) {
            float rad = (startAngle + 90.f * i / seg) * 3.14159265f / 180.f;
            pts.push_back({cx + std::cos(rad) * r, cy + std::sin(rad) * r});
        }
    };
    arc(x + w - r, y + r, -90);
    arc(x + w - r, y + h - r, 0);
    arc(x + r, y + h - r, 90);
    arc(x + r, y + r, 180);

    sf::ConvexShape s(pts.size());
    for (size_t i = 0; i < pts.size(); i++) s.setPoint(i, pts[i]);
    return s;
}

SplashScreen::SplashScreen() {
    mTimer = 0.f;
    mDuration = 2.4f;
    mFinished = false;
}

bool SplashScreen::init(const std::string& assetsDir, const sf::Font& fontRegular,
                         const sf::Font& fontSemiBold, const sf::Font& fontBold, const sf::Font& fontSerif) {
    mFontRegular = &fontRegular;
    mFontSemiBold = &fontSemiBold;
    mFontBold = &fontBold;
    mFontSerif = &fontSerif;

    // Load Athelia watercolor background artwork for seamless blending
    std::vector<std::string> bgCandidates = {
        assetsDir + "/login/bg_login_athelia.png",
        assetsDir + "/art/bg_art_hd.png",
        assetsDir + "/art/bg_art.png",
        "assets/login/bg_login_athelia.png",
        "assets/art/bg_art.png"
    };
    mHasBg = false;
    for (const auto& path : bgCandidates) {
        if (fs::exists(path) && mBgTexture.loadFromFile(path)) {
            mBgTexture.setSmooth(true);
            mHasBg = true;
            break;
        }
    }

    // Load Classical Greek Bust Emblem Card
    std::vector<std::string> emblemCandidates = {
        assetsDir + "/brand/scanit-bust-card.png",
        assetsDir + "/icon.png",
        assetsDir + "/brand/app-icon-512.png",
        "assets/brand/scanit-bust-card.png",
        "assets/icon.png"
    };
    mHasEmblem = false;
    for (const auto& path : emblemCandidates) {
        if (fs::exists(path) && mEmblemTexture.loadFromFile(path)) {
            mEmblemTexture.setSmooth(true);
            mHasEmblem = true;
            break;
        }
    }

    mTimer = 0.f;
    mFinished = false;
    return true;
}

void SplashScreen::update(float dt) {
    if (mFinished) return;
    mTimer += dt;
    if (mTimer >= mDuration) {
        mFinished = true;
    }
}

float SplashScreen::getProgress() const {
    return std::min(1.f, mTimer / (mDuration * 0.90f));
}

bool SplashScreen::isFinished() const {
    return mFinished;
}

void SplashScreen::skip() {
    mFinished = true;
}

void SplashScreen::render(sf::RenderTarget& target, float designWidth, float designHeight) {
    float cx = designWidth / 2.f;
    float cy = designHeight / 2.f - 14.f;
    float t = mTimer;
    float progress = getProgress();

    // ── 1. WARM CLASSICAL PARCHMENT CANVAS (#F3EFE6) ──
    sf::RectangleShape parchment({designWidth, designHeight});
    parchment.setFillColor(sf::Color(243, 239, 230)); // #F3EFE6 exact match with Login Screen
    target.draw(parchment);

    // Subtle ambient watercolor background artwork tint
    if (mHasBg) {
        sf::Sprite bgSprite(mBgTexture);
        sf::Vector2u texSize = mBgTexture.getSize();
        float scaleX = designWidth / static_cast<float>(texSize.x);
        float scaleY = designHeight / static_cast<float>(texSize.y);
        float s = std::max(scaleX, scaleY);
        bgSprite.setScale(s, s);
        bgSprite.setPosition(0, 0);
        bgSprite.setColor(sf::Color(255, 255, 255, 75)); // Soft watercolor background presence
        target.draw(bgSprite);
    }

    // ── 2. NEOCLASSICAL TEMPLE PILLARS (Subtle architectural etched linework) ──
    auto drawPillar = [&](float px) {
        sf::Color pillarCol(125, 104, 82, 35);
        sf::Color capCol(125, 104, 82, 55);

        // Capital header
        sf::RectangleShape capTop({28.f, 4.f});
        capTop.setOrigin(14.f, 2.f);
        capTop.setPosition(px, 50.f);
        capTop.setFillColor(capCol);
        target.draw(capTop);

        sf::RectangleShape capMid({22.f, 4.f});
        capMid.setOrigin(11.f, 2.f);
        capMid.setPosition(px, 55.f);
        capMid.setFillColor(capCol);
        target.draw(capMid);

        // Fluted column shafts
        float colTop = 58.f;
        float colBot = designHeight - 80.f;
        float colH = colBot - colTop;

        for (float off : {-7.f, 0.f, 7.f}) {
            sf::RectangleShape flute({1.5f, colH});
            flute.setPosition(px + off - 0.75f, colTop);
            flute.setFillColor(pillarCol);
            target.draw(flute);
        }

        // Base plinth
        sf::RectangleShape baseMid({22.f, 4.f});
        baseMid.setOrigin(11.f, 2.f);
        baseMid.setPosition(px, colBot + 2.f);
        baseMid.setFillColor(capCol);
        target.draw(baseMid);

        sf::RectangleShape baseBot({28.f, 5.f});
        baseBot.setOrigin(14.f, 2.5f);
        baseBot.setPosition(px, colBot + 7.f);
        baseBot.setFillColor(capCol);
        target.draw(baseBot);
    };

    drawPillar(75.f);
    drawPillar(125.f);
    drawPillar(designWidth - 125.f);
    drawPillar(designWidth - 75.f);

    // ── 3. ATMOSPHERIC GOLDEN GLOW & DRIFTING DUST ──
    float breath = 1.0f + 0.035f * std::sin(t * 2.8f);

    // Warm antique bronze radial glow behind cameo
    sf::CircleShape glowOrb(190.f * breath);
    glowOrb.setOrigin(190.f * breath, 190.f * breath);
    glowOrb.setPosition(cx, cy);
    glowOrb.setFillColor(sf::Color(197, 155, 76, 26)); // Antique gold aura
    target.draw(glowOrb);

    sf::CircleShape coreGlow(120.f * breath);
    coreGlow.setOrigin(120.f * breath, 120.f * breath);
    coreGlow.setPosition(cx, cy);
    coreGlow.setFillColor(sf::Color(220, 180, 110, 32));
    target.draw(coreGlow);

    // Floating Golden Dust Particles (40 procedural particles)
    for (int i = 0; i < 40; i++) {
        float px = std::fmod((float)(i * 127 + 60) + std::sin(t * 0.8f + i * 1.5f) * 25.f, designWidth);
        float py = std::fmod((float)(i * 89 + 40) - t * 30.f + designHeight * 2.f, designHeight);
        float pAlpha = 0.35f + 0.55f * std::abs(std::sin(t * 2.2f + i * 2.1f));
        float pSize = (i % 6 == 0) ? 2.2f : 1.3f;

        sf::CircleShape part(pSize);
        part.setPosition(px, py);
        part.setFillColor(sf::Color(197, 155, 76, static_cast<sf::Uint8>(pAlpha * 160)));
        target.draw(part);
    }

    // Drifting Botanical Olive Leaves (12 leaves)
    for (int i = 0; i < 12; i++) {
        float lx = cx - 240.f + (i * 40.f) + std::sin(t * 1.1f + i * 0.9f) * 35.f;
        float ly = std::fmod(120.f + i * 35.f + t * 22.f, designHeight);
        float lRot = t * 45.f + i * 30.f;

        sf::ConvexShape leaf(4);
        leaf.setPoint(0, {0.f, -5.f});
        leaf.setPoint(1, {3.5f, 0.f});
        leaf.setPoint(2, {0.f, 5.f});
        leaf.setPoint(3, {-3.5f, 0.f});
        leaf.setPosition(lx, ly);
        leaf.setRotation(lRot);
        leaf.setFillColor(sf::Color(118, 148, 108, 65)); // Athelia sage tint
        target.draw(leaf);
    }

    // ── 4. CONCENTRIC 3D ROTATING GYRO / CELESTIAL RINGS ──
    const float ringRadii[3] = {165.f, 135.f, 110.f};
    const sf::Color ringCols[3] = {
        sf::Color(197, 155, 76, 175),  // Antique Gold
        sf::Color(212, 139, 56, 185),  // Warm Amber
        sf::Color(197, 155, 76, 200)   // Core Gold
    };

    for (int rIdx = 0; rIdx < 3; rIdx++) {
        float baseR = ringRadii[rIdx];
        float rotDir = (rIdx % 2 == 0) ? 1.f : -1.f;
        float rotSpeed = 0.55f + rIdx * 0.28f;
        float curRot = t * rotSpeed * rotDir;

        // 3D perspective tilt
        float tiltY = 0.38f + 0.28f * std::abs(std::sin(curRot * 0.5f));
        const int numPts = 64;
        sf::VertexArray ringLine(sf::PrimitiveType::LineStrip);

        for (int p = 0; p <= numPts; p++) {
            float angle = p * 2.f * 3.14159265f / numPts + curRot;
            float rx = std::cos(angle) * baseR;
            float ry = std::sin(angle) * baseR * tiltY;

            // Dashed segments (skip 1/4th)
            if ((p / 4) % 2 == 0) {
                ringLine.append(sf::Vertex(sf::Vector2f(cx + rx, cy + ry), ringCols[rIdx]));
            } else {
                if (ringLine.getVertexCount() > 0) {
                    target.draw(ringLine);
                    ringLine.clear();
                }
            }
        }
        if (ringLine.getVertexCount() > 0) target.draw(ringLine);

        // Golden Celestial Orb Node orbiting on the ring
        float orbAngle = curRot * 2.2f;
        float ox = cx + std::cos(orbAngle) * baseR;
        float oy = cy + std::sin(orbAngle) * baseR * tiltY;

        sf::CircleShape nodeGlow(5.5f);
        nodeGlow.setOrigin(5.5f, 5.5f);
        nodeGlow.setPosition(ox, oy);
        nodeGlow.setFillColor(sf::Color(240, 192, 88, 120));
        target.draw(nodeGlow);

        sf::CircleShape node(2.5f);
        node.setOrigin(2.5f, 2.5f);
        node.setPosition(ox, oy);
        node.setFillColor(sf::Color(255, 240, 200, 240));
        target.draw(node);
    }

    // ── 5. CENTRAL CAMEO MEDALLION PLAQUE (Portrait Brooch Frame) ──
    const float plaqueW = 126.f;
    const float plaqueH = 160.f;
    const float plaqueR = 24.f;

    // Drop shadow
    sf::ConvexShape shadow = createRoundedRectShape(cx - plaqueW / 2.f + 3.f, cy - plaqueH / 2.f + 5.f, plaqueW, plaqueH, plaqueR);
    shadow.setFillColor(sf::Color(40, 32, 24, 45));
    target.draw(shadow);

    // Outer gold border rim
    sf::ConvexShape outerRim = createRoundedRectShape(cx - plaqueW / 2.f, cy - plaqueH / 2.f, plaqueW, plaqueH, plaqueR);
    outerRim.setFillColor(sf::Color(252, 249, 244));
    outerRim.setOutlineThickness(3.5f);
    outerRim.setOutlineColor(sf::Color(197, 155, 76)); // Radiant Antique Gold
    target.draw(outerRim);

    // Inner hairline frame
    sf::ConvexShape innerRim = createRoundedRectShape(cx - (plaqueW - 8.f) / 2.f, cy - (plaqueH - 8.f) / 2.f, plaqueW - 8.f, plaqueH - 8.f, plaqueR - 4.f);
    innerRim.setFillColor(sf::Color::Transparent);
    innerRim.setOutlineThickness(1.2f);
    innerRim.setOutlineColor(sf::Color(215, 185, 130, 160));
    target.draw(innerRim);

    // Laurel Wreath framing the left and right flanks of the plaque
    const int wreathLeaves = 10;
    for (int side : {-1, 1}) {
        for (int i = 0; i < wreathLeaves; i++) {
            float frac = (i + 0.5f) / static_cast<float>(wreathLeaves);
            float ly = (cy - plaqueH / 2.f + 16.f) + frac * (plaqueH - 32.f);
            float curveOff = std::sin(frac * 3.14159265f) * 12.f;
            float lx = cx + side * (plaqueW / 2.f + 10.f + curveOff);
            float rotDeg = (side == -1) ? (140.f + frac * 80.f) : (-140.f - frac * 80.f);

            sf::ConvexShape leaf(4);
            leaf.setPoint(0, {0.f, -4.f});
            leaf.setPoint(1, {3.f, 0.f});
            leaf.setPoint(2, {0.f, 4.f});
            leaf.setPoint(3, {-3.f, 0.f});
            leaf.setPosition(lx, ly);
            leaf.setRotation(rotDeg + std::sin(t * 2.f + i) * 3.f);
            leaf.setFillColor(sf::Color(197, 155, 76, 220)); // Antique Gold leaves
            target.draw(leaf);
        }
    }

    // Central Greek Bust Card Sprite
    if (mHasEmblem) {
        sf::Sprite emblemSprite(mEmblemTexture);
        sf::Vector2u eSize = mEmblemTexture.getSize();
        float targetW = 104.f;
        float scale = targetW / static_cast<float>(eSize.x);
        emblemSprite.setScale(scale, scale);
        emblemSprite.setOrigin(eSize.x / 2.f, eSize.y / 2.f);
        emblemSprite.setPosition(cx, cy);
        target.draw(emblemSprite);
    }

    // ── 6. LIVE OPTICAL BARCODE LASER SCANLINE ──
    float sweepOffset = std::fmod(t * 70.f, plaqueH * 0.85f);
    float laserY = (cy - plaqueH * 0.42f) + sweepOffset;
    float laserHalfW = (plaqueW * 0.45f);

    // Soft diffuse laser aura
    sf::RectangleShape laserGlow({laserHalfW * 2.f, 6.f});
    laserGlow.setOrigin(laserHalfW, 3.f);
    laserGlow.setPosition(cx, laserY);
    laserGlow.setFillColor(sf::Color(230, 120, 40, 55));
    target.draw(laserGlow);

    // Core bright amber/coral laser beam
    sf::RectangleShape laserBeam({laserHalfW * 2.f, 2.0f});
    laserBeam.setOrigin(laserHalfW, 1.0f);
    laserBeam.setPosition(cx, laserY);
    laserBeam.setFillColor(sf::Color(220, 90, 30, 225));
    target.draw(laserBeam);

    // ── 7. TOP BRANDING HUD (Roman Serif Typography) ──
    const sf::Font& fontS = (mFontSerif ? *mFontSerif : (mFontBold ? *mFontBold : *mFontRegular));
    const sf::Font& fontM = (mFontSemiBold ? *mFontSemiBold : (mFontBold ? *mFontBold : *mFontRegular));
    const sf::Font& fontR = (mFontRegular ? *mFontRegular : *mFontSemiBold);

    // "S C A N I T" in classical Roman serif
    sf::Text brandTitle("S C A N I T", fontS, 36);
    brandTitle.setFillColor(sf::Color(38, 34, 30)); // Deep classical charcoal ink
    sf::FloatRect brandBounds = brandTitle.getLocalBounds();
    brandTitle.setPosition(cx - brandBounds.width / 2.f, 32.f);
    target.draw(brandTitle);

    // Subtitle "ATHENA OPTICAL ENGINE - REGISTER 04"
    sf::Text brandSub("ATHENA OPTICAL ENGINE   -   REGISTER 04", fontM, 11);
    brandSub.setFillColor(sf::Color(115, 100, 85)); // Warm stone slate
    sf::FloatRect subBounds = brandSub.getLocalBounds();
    brandSub.setPosition(cx - subBounds.width / 2.f, 78.f);
    target.draw(brandSub);

    // Top-Right Percentage Counter
    int pct = static_cast<int>(progress * 100.f);
    sf::Text pctText(std::to_string(pct) + "%", fontM, 20);
    pctText.setFillColor(sf::Color(38, 34, 30));
    sf::FloatRect pctBounds = pctText.getLocalBounds();
    pctText.setPosition(designWidth - 55.f - pctBounds.width, 36.f);
    target.draw(pctText);

    // Top-Left Operational Tag
    sf::Text intLab("SCANIT // ATHENA CORE", fontM, 11);
    intLab.setFillColor(sf::Color(135, 120, 105));
    intLab.setPosition(55.f, 38.f);
    target.draw(intLab);

    // ── 8. BOTTOM PROGRESS BAR & STATUS HUD ──
    float bottomY = designHeight - 85.f;

    // Status message cycling based on progress
    std::string statusMsg = "CALIBRATING OPTICAL ENGINE...";
    if (progress >= 0.70f) {
        statusMsg = "ATHENA POS SYSTEM READY";
    } else if (progress >= 0.35f) {
        statusMsg = "SYNCHRONIZING PRODUCT CATALOG...";
    }

    sf::Text statusText(statusMsg, fontM, 11);
    statusText.setFillColor(sf::Color(115, 100, 85));
    statusText.setPosition(cx - 160.f, bottomY - 24.f);
    target.draw(statusText);

    sf::Text fpsText("60 FPS  |  VERIFIED", fontM, 11);
    fpsText.setFillColor(sf::Color(135, 120, 105));
    sf::FloatRect fpsBounds = fpsText.getLocalBounds();
    fpsText.setPosition(cx + 160.f - fpsBounds.width, bottomY - 24.f);
    target.draw(fpsText);

    // Progress Track (Width 320, Height 4, rounded)
    const float barW = 320.f;
    const float barH = 5.f;
    sf::ConvexShape barBg = createRoundedRectShape(cx - barW / 2.f, bottomY, barW, barH, 2.5f);
    barBg.setFillColor(sf::Color(215, 205, 192)); // Soft parchment recess
    target.draw(barBg);

    // Progress Fill (Glowing Antique Gold #C59B4C)
    float curFillW = std::max(4.f, barW * progress);
    sf::ConvexShape barFill = createRoundedRectShape(cx - barW / 2.f, bottomY, curFillW, barH, 2.5f);
    barFill.setFillColor(sf::Color(197, 155, 76)); // Radiant Antique Gold
    target.draw(barFill);

    // Progress leading bright pip
    if (progress > 0.02f && progress < 0.99f) {
        sf::CircleShape pip(3.5f);
        pip.setOrigin(3.5f, 3.5f);
        pip.setPosition(cx - barW / 2.f + curFillW, bottomY + barH / 2.f);
        pip.setFillColor(sf::Color(245, 200, 100));
        target.draw(pip);
    }

    // Bottom prompt
    sf::Text promptText("Press Space or Click to enter ScanIT POS", fontR, 10);
    promptText.setFillColor(sf::Color(140, 125, 110, 190));
    sf::FloatRect pBounds = promptText.getLocalBounds();
    promptText.setPosition(cx - pBounds.width / 2.f, bottomY + 20.f);
    target.draw(promptText);
}

#pragma once
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <filesystem>

namespace splash_detail
{
    inline float clamp01(float v) { return std::max(0.f, std::min(1.f, v)); }
    inline float easeOutCubic(float t) { t = 1.f - clamp01(t); return 1.f - t * t * t; }
    inline float easeOutBack(float t)
    {
        t = clamp01(t); const float c1 = 1.70158f, c3 = c1 + 1.f;
        return 1.f + c3 * std::pow(t - 1.f, 3.f) + c1 * std::pow(t - 1.f, 2.f);
    }
    inline sf::Uint8 a8(float v) { return static_cast<sf::Uint8>(255.f * clamp01(v)); }

    inline sf::Texture loadTexWithFallback(const std::string& relPath)
    {
        sf::Texture t;
        std::vector<std::string> prefixes = { "assets/", "frontend/assets/", "bin/Debug/assets/", "../frontend/assets/", "../assets/" };
        for (const auto& pre : prefixes) {
            if (t.loadFromFile(pre + relPath)) {
                t.setSmooth(true);
                return t;
            }
        }
        t.create(2, 2);
        return t;
    }

    inline bool loadFontWithFallback(sf::Font& f, const std::string& relPath)
    {
        std::vector<std::string> prefixes = { "assets/fonts/", "frontend/assets/fonts/", "bin/Debug/assets/fonts/", "../frontend/assets/fonts/" };
        for (const auto& pre : prefixes) {
            if (f.loadFromFile(pre + relPath)) {
                return true;
            }
        }
        return f.loadFromFile("C:/Windows/Fonts/segoeui.ttf") || f.loadFromFile("C:/Windows/Fonts/arial.ttf");
    }
}

inline void runSplash()
{
    using namespace splash_detail;
    constexpr unsigned W = 640, H = 400;
    constexpr float TOTAL = 3.6f;          // seconds
    const std::string kText = "BUILD BY INTRUDERS";

    sf::ContextSettings cs; cs.antialiasingLevel = 8;
    sf::RenderWindow win(sf::VideoMode(W, H), "ScanIT", sf::Style::None, cs);
    win.setFramerateLimit(60);
    const sf::VideoMode desk = sf::VideoMode::getDesktopMode();
    win.setPosition({ int(desk.width - W) / 2, int(desk.height - H) / 2 });

    sf::Texture bgT = loadTexWithFallback("splash/splash-bg.png");
    sf::Texture emT = loadTexWithFallback("splash/emblem-512.png");
    sf::Texture glT = loadTexWithFallback("ui/glow.png");
    sf::Texture lsT = loadTexWithFallback("ui/laser.png");

    sf::Font fBold, fSemi, fMono;
    loadFontWithFallback(fBold, "Inter-Bold.ttf");
    loadFontWithFallback(fSemi, "Inter-SemiBold.ttf");
    loadFontWithFallback(fMono, "JetBrainsMono-Regular.ttf");

    sf::Sprite bg(bgT), em(emT), glow(glT), laser(lsT);
    auto emSz = emT.getSize();
    em.setOrigin((float)emSz.x * 0.5f, (float)emSz.y * 0.5f);
    auto glSz = glT.getSize();
    glow.setOrigin((float)glSz.x * 0.5f, (float)glSz.y * 0.5f);

    const sf::Vector2f emC((float)W * 0.5f, 128.f);       // emblem center
    const float emSize = 150.f;                    // on-screen emblem size

    // "ScanIT" title
    sf::Text title("ScanIT", fBold, 36);
    title.setFillColor(sf::Color(248, 250, 252));
    { auto b = title.getLocalBounds(); title.setOrigin(b.left + b.width / 2.f, 0.f); title.setPosition(W / 2.f, 224.f); }

    // "BUILD BY INTRUDERS": per-letter, tracked-out, typed in one at a time
    const unsigned fs = 13; const float tracking = 6.f;
    std::vector<sf::Text> letters;
    {
        float total = 0.f;
        for (char c : kText) total += fSemi.getGlyph(c, fs, false).advance + tracking;
        total -= tracking;
        float x = (W - total) / 2.f;
        for (char c : kText)
        {
            sf::Text t(std::string(1, c), fSemi, fs);
            t.setPosition(std::floor(x), 272.f);
            letters.push_back(t);
            x += fSemi.getGlyph(c, fs, false).advance + tracking;
        }
    }

    // thin progress bar
    const float barW = 220.f, barX = (W - barW) / 2.f, barY = 336.f;
    sf::RectangleShape barBg({ barW, 3.f }); barBg.setPosition(barX, barY); barBg.setFillColor(sf::Color(255, 255, 255, 22));
    sf::RectangleShape bar({ 0.f, 3.f });    bar.setPosition(barX, barY);
    sf::RectangleShape barHead({ 18.f, 3.f }); barHead.setFillColor(sf::Color(103, 232, 249));

    sf::Clock clk;
    while (win.isOpen())
    {
        sf::Event e;
        while (win.pollEvent(e))
            if (e.type == sf::Event::Closed || e.type == sf::Event::KeyPressed ||
                e.type == sf::Event::MouseButtonPressed) {
                win.close();
                return;     // click / key = skip
            }

        const float t = clk.getElapsedTime().asSeconds();
        if (t >= TOTAL) break;

        const float fadeIn  = easeOutCubic(t / 0.45f);
        const float fadeOut = 1.f - clamp01((t - (TOTAL - 0.45f)) / 0.45f);
        const float master  = fadeIn * fadeOut;

        win.clear(sf::Color(8, 5, 20));

        if (bgT.getSize().x > 2) {
            bg.setColor(sf::Color(255, 255, 255, a8(master)));
            win.draw(bg);
        }

        // emblem: pop-in with overshoot, then gentle float
        const float p = easeOutBack((t - 0.15f) / 0.75f);
        const float sc = (emSize / (float)std::max(1u, emSz.x)) * (0.6f + 0.4f * p);
        const float bob = std::sin(t * 2.2f) * 2.5f;
        em.setScale(sc, sc);
        em.setPosition(emC.x, emC.y + bob);
        em.setColor(sf::Color(255, 255, 255, a8(clamp01((t - 0.1f) / 0.5f) * master)));

        // pulsing glow behind it (additive)
        if (glSz.x > 2) {
            const float pulse = 0.65f + 0.35f * std::sin(t * 3.0f);
            const float gs = (emSize * 2.6f) / (float)glSz.x;
            glow.setScale(gs, gs); glow.setPosition(emC.x, emC.y + bob);
            glow.setColor(sf::Color(139, 92, 246, a8(0.55f * pulse * master)));
            win.draw(glow, sf::BlendAdd);
            glow.setScale(gs * 0.6f, gs * 0.6f);
            glow.setColor(sf::Color(6, 182, 212, a8(0.18f * pulse * master)));
            win.draw(glow, sf::BlendAdd);
        }
        win.draw(em);

        // scan beam sweeping down over the emblem (0.9s - 1.9s)
        const float sweep = clamp01((t - 0.9f) / 1.0f);
        if (sweep > 0.f && sweep < 1.f && lsT.getSize().x > 2)
        {
            const float yy = emC.y - emSize * 0.38f + sweep * emSize * 0.76f + bob;
            auto lsSz = lsT.getSize();
            laser.setOrigin((float)lsSz.x * 0.5f, (float)lsSz.y * 0.5f);
            laser.setScale((emSize * 0.9f) / (float)lsSz.x, 1.4f);
            laser.setPosition(emC.x, yy);
            const float edge = std::sin(sweep * 3.14159f);
            laser.setColor(sf::Color(255, 255, 255, a8(edge * master)));
            win.draw(laser, sf::BlendAdd);
        }

        // title
        const float tIn = easeOutCubic((t - 0.7f) / 0.6f);
        title.setFillColor(sf::Color(248, 250, 252, a8(tIn * master)));
        title.setPosition(W / 2.f, 224.f + (1.f - tIn) * 10.f);
        win.draw(title);

        // BUILD BY INTRUDERS - letters type in from 1.2s, 55ms apart, each slides up + fades
        for (std::size_t i = 0; i < letters.size(); ++i)
        {
            const float lt = clamp01((t - (1.2f + i * 0.055f)) / 0.25f);
            if (lt <= 0.f) continue;
            const float e2 = easeOutCubic(lt);
            sf::Text l = letters[i];
            const float k = static_cast<float>(i) / (float)(letters.size() - 1);
            l.setFillColor(sf::Color(sf::Uint8(103 + (168 - 103) * k), sf::Uint8(232 + (85 - 232) * k),
                                     sf::Uint8(249 + (247 - 249) * k), a8(e2 * master)));
            l.move(0.f, (1.f - e2) * 8.f);
            win.draw(l);
        }

        // progress bar
        const float prog = clamp01((t - 0.5f) / (TOTAL - 1.2f));
        const float pw = barW * easeOutCubic(prog);
        barBg.setFillColor(sf::Color(255, 255, 255, a8(0.09f * master)));
        win.draw(barBg);
        bar.setSize({ pw, 3.f });
        bar.setFillColor(sf::Color(139, 92, 246, a8(master)));
        win.draw(bar);
        barHead.setPosition(barX + std::max(0.f, pw - 18.f), barY);
        barHead.setSize({ std::min(18.f, pw), 3.f });
        barHead.setFillColor(sf::Color(103, 232, 249, a8(master)));
        win.draw(barHead);

        win.display();
    }
    win.close();
}

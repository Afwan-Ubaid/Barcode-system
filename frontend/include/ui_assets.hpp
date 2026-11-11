#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
class Assets {
public:
    static Assets& instance() {
        static Assets s_instance;
        return s_instance;
    }

    bool init() {
        // Load fonts
        std::vector<std::string> fontPrefixes = { "assets/fonts/", "frontend/assets/fonts/", "bin/Debug/assets/fonts/", "../frontend/assets/fonts/" };
        
        bool interLoaded = false;
        for (const auto& pre : fontPrefixes) {
            if (m_fontInterBold.loadFromFile(pre + "Inter-Bold.ttf") &&
                m_fontInterSemiBold.loadFromFile(pre + "Inter-SemiBold.ttf") &&
                m_fontInterMedium.loadFromFile(pre + "Inter-Medium.ttf") &&
                m_fontInterRegular.loadFromFile(pre + "Inter-Regular.ttf") &&
                m_fontMono.loadFromFile(pre + "JetBrainsMono-SemiBold.ttf")) {
                interLoaded = true;
                break;
            }
        }
        if (!interLoaded) {
            // Fallback system font
            m_fontInterRegular.loadFromFile("C:/Windows/Fonts/segoeui.ttf");
            m_fontInterBold = m_fontInterRegular;
            m_fontInterSemiBold = m_fontInterRegular;
            m_fontInterMedium = m_fontInterRegular;
            m_fontMono.loadFromFile("C:/Windows/Fonts/consola.ttf");
        }
        return true;
    }

    sf::Font& fontBold() { return m_fontInterBold; }
    sf::Font& fontSemiBold() { return m_fontInterSemiBold; }
    sf::Font& fontMedium() { return m_fontInterMedium; }
    sf::Font& fontRegular() { return m_fontInterRegular; }
    sf::Font& fontMono() { return m_fontMono; }

    sf::Texture& tex(const std::string& relPath) {
        auto it = m_textures.find(relPath);
        if (it != m_textures.end()) return it->second;

        std::vector<std::string> prefixes = { "assets/", "frontend/assets/", "bin/Debug/assets/", "../frontend/assets/", "../assets/" };
        sf::Texture t;
        for (const auto& pre : prefixes) {
            if (t.loadFromFile(pre + relPath)) {
                t.setSmooth(true);
                it = m_textures.emplace(relPath, std::move(t)).first;
                return it->second;
            }
        }
        // Fallback dummy 1x1 texture
        t.create(2, 2);
        it = m_textures.emplace(relPath, std::move(t)).first;
        return it->second;
    }

    // Panels include a transparent shadow/glow margin ("pad").
    // Draw at (x - pad, y - pad) so the visible card lands exactly at (x, y).
    void panel(sf::RenderTarget& rt, const std::string& name, float x, float y, int pad = 40,
               sf::Color tint = sf::Color::White) {
        sf::Texture& t = tex("panels/" + name + ".png");
        if (t.getSize().x > 2) {
            sf::Sprite s(t);
            s.setPosition(x - (float)pad, y - (float)pad);
            s.setColor(tint);
            rt.draw(s);
        }
    }

    // Crisp @2x icon drawn centered
    void icon(sf::RenderTarget& rt, const std::string& name, sf::Vector2f center, float px, sf::Color c) {
        sf::Texture& t = tex("icons/" + name + "@2x.png");
        if (t.getSize().x <= 2) {
            t = tex("icons/" + name + ".png");
        }
        if (t.getSize().x > 2) {
            sf::Sprite s(t);
            auto sz = t.getSize();
            s.setOrigin((float)sz.x * 0.5f, (float)sz.y * 0.5f);
            s.setScale(px / (float)sz.x, px / (float)sz.y);
            s.setPosition(center);
            s.setColor(c);
            rt.draw(s);
        }
    }

    // Additive glow orb
    void glow(sf::RenderTarget& rt, sf::Vector2f center, float radius, sf::Color c) {
        sf::Texture& t = tex("ui/glow.png");
        if (t.getSize().x > 2) {
            sf::Sprite s(t);
            auto sz = t.getSize();
            s.setOrigin((float)sz.x * 0.5f, (float)sz.y * 0.5f);
            s.setScale((radius * 2.f) / (float)sz.x, (radius * 2.f) / (float)sz.y);
            s.setPosition(center);
            s.setColor(c);
            rt.draw(s, sf::BlendAdd);
        }
    }

private:
    Assets() = default;
    std::map<std::string, sf::Texture> m_textures;
    sf::Font m_fontInterBold;
    sf::Font m_fontInterSemiBold;
    sf::Font m_fontInterMedium;
    sf::Font m_fontInterRegular;
    sf::Font m_fontMono;
};

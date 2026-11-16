#ifndef SCREEN_FACTORY_H
#define SCREEN_FACTORY_H

#include "UITheme.h"
#include <SFML/Graphics.hpp>
#include <memory>
using namespace std;

class BaseScreen {
public:
    virtual ~BaseScreen() = default;
    virtual AppScreen handleEvent(const sf::Event& event) = 0;
    virtual void update(float dt) = 0;
    virtual void draw() = 0;
};

class ScreenFactory {
public:
    static unique_ptr<BaseScreen> createScreen(AppScreen type, sf::RenderWindow& window, sf::Font& font);
};

#endif
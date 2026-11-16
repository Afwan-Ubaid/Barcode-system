#include "../include/ScreenFactory.h"
#include "../include/HomeScreen.h"
#include "../include/GenerateScreen.h"
#include "../include/ReadScreen.h"
#include "../include/OrderScreen.h"

class HomeAdapter : public BaseScreen {
    HomeScreen screen;
public:
    HomeAdapter(sf::RenderWindow& w, sf::Font& f) : screen(w, f) {}
    AppScreen handleEvent(const sf::Event& e) override { return screen.handleEvent(e); }
    void update(float dt) override {}
    void draw() override { screen.draw(); }
};

class GenerateAdapter : public BaseScreen {
    GenerateScreen screen;
public:
    GenerateAdapter(sf::RenderWindow& w, sf::Font& f) : screen(w, f) {}
    AppScreen handleEvent(const sf::Event& e) override { return screen.handleEvent(e); }
    void update(float dt) override {}
    void draw() override { screen.draw(); }
};

class ReadAdapter : public BaseScreen {
    ReadScreen screen;
public:
    ReadAdapter(sf::RenderWindow& w, sf::Font& f) : screen(w, f) {}
    AppScreen handleEvent(const sf::Event& e) override { return screen.handleEvent(e); }
    void update(float dt) override { screen.update(dt); }
    void draw() override { screen.draw(); }
};

unique_ptr<BaseScreen> ScreenFactory::createScreen(AppScreen type, sf::RenderWindow& window, sf::Font& font) {
    switch (type) {
        case AppScreen::Home: return make_unique<HomeAdapter>(window, font);
        case AppScreen::Generate: return make_unique<GenerateAdapter>(window, font);
        case AppScreen::Read: return make_unique<ReadAdapter>(window, font);
        case AppScreen::Order: return make_unique<OrderScreen>(window, font);
        default: return make_unique<HomeAdapter>(window, font);
    }
}
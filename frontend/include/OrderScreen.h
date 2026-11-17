#ifndef ORDER_SCREEN_H
#define ORDER_SCREEN_H

#include "ScreenFactory.h"
#include "CameraCapture.h"
#include "Decoder.h"
#include "POSManager.h"
#include <vector>
using namespace std;

class OrderScreen : public BaseScreen {
private:
    sf::RenderWindow& window;
    sf::Font& font;
    CameraStream camera;
    Order currentOrder;
    string statusMsg;

    // NEW: Cooldown to prevent scanning the same barcode 60 times a second
    float scanCooldown;
    string lastScannedCode;

    sf::RectangleShape cartPanel;
    sf::RectangleShape billPanel;
    sf::Texture cameraTexture;
    sf::Sprite cameraSprite;

public:
    OrderScreen(sf::RenderWindow& win, sf::Font& f);
    ~OrderScreen();

    AppScreen handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void draw() override;
};

#endif
#ifndef GUI_H
#define GUI_H

#include <SFML/Graphics.hpp>
#include <string>
using namespace std;

class BarcodeApp {
public:
    BarcodeApp();
    void run();

private:
    sf::RenderWindow window;
    sf::Font font;

    enum class Screen { HOME, GENERATE, DECODE, CAMERA, ABOUT };
    Screen currentScreen = Screen::HOME;

    // Generate screen
    string inputBuffer;
    string lastGeneratedFile;
    sf::Texture barcodeTexture;
    sf::Sprite  barcodeSprite;
    bool        hasBarcode = false;

    // Decode screen
    string decodePathBuffer;
    string decodedCode;

    // Camera screen
    float  cameraTimer = 0.f;
    bool   cameraDone = false;
    string cameraStatus;

    // Shared status
    string statusMessage;
    bool   statusIsError = false;

    // Animation timer
    float clockSeconds = 0.f;

    void drawHome();
    void drawGenerate();
    void drawDecode();
    void drawCamera();
    void drawAbout();
    void drawButton(const string& label, float x, float y,
        float w, float h, sf::Color fill,
        sf::Color outline = sf::Color::White);
    void drawBackground();
    void drawStatus();

    void handleHomeClick(float mx, float my);
    void handleGenerateClick(float mx, float my);
    void handleDecodeClick(float mx, float my);
    void handleCameraClick(float mx, float my);
    void handleAboutClick(float mx, float my);
    void handleTextEntered(sf::Uint32 unicode);
    void handleBackspace();

    // NEW: opens the native Windows file picker and fills decodePathBuffer
    void openFileDialogForBmp();
};

#endif
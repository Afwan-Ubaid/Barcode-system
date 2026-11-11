#ifndef GENERATESCREEN_H
#define GENERATESCREEN_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "UITheme.h"

class GenerateScreen {
public:
    using GenerateCallback = std::function<void(const std::string& code, const std::string& source)>;

    GenerateScreen(sf::RenderWindow& window, sf::Font& font);
    AppScreen handleEvent(const sf::Event& event);
    void draw();

    // Returns the generated code (if any) so HomeScreen can log it
    std::string getLastGeneratedCode() const { return mLastCode; }
    void setOnGenerateCallback(GenerateCallback cb) { mOnGenerateCallback = cb; }

private:
    sf::RenderWindow& mWindow;
    sf::Font&         mFont;

    // Input field
    std::string        mInputStr;
    sf::RectangleShape mInputBox;
    sf::Text           mInputText;
    sf::Text           mInputLabel;
    sf::Text           mInputHint;
    bool               mInputFocused;
    float              mCursorBlink;
    bool               mCursorVisible;

    // Status message
    std::string mStatusMsg;
    bool        mStatusOk;
    sf::Text    mStatusText;

    // Barcode preview (drawn with sf::RectangleShape bars)
    bool                            mHasBarcode;
    std::vector<sf::RectangleShape> mBars;
    sf::RectangleShape              mBarcodeBackground;
    sf::Text                        mDigitsLabel;

    // Buttons
    sf::RectangleShape mGenerateBtn;
    sf::RectangleShape mSaveBtn;
    sf::Text           mGenerateBtnText;
    sf::Text           mSaveBtnText;
    sf::Text           mBackBtn;

    // State tracking
    std::string mLastCode;
    std::string mSavedFilename;

    GenerateCallback mOnGenerateCallback;

    void generateAndSave();
    void buildBarcodePreview(const std::string& code);
};

#endif // GENERATESCREEN_H

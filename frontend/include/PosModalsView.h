#ifndef POS_MODALS_VIEW_H
#define POS_MODALS_VIEW_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Separation of Concerns (SoC)
// Module: Popup Modals (Receipt, Enlarge Cart, Cash, Card, Qty, Staff Management)
// ============================================================================

class PosModalsView {
public:
    static void drawReceiptModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                 float currentWidth, float designHeight,
                                 const std::string& receiptText,
                                 const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                 const sf::Font& fontSerif,
                                 std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawEnlargeCartModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                     float currentWidth, float designHeight,
                                     const std::vector<CartLine>& cart,
                                     const std::vector<ProductItem>& products,
                                     int subtotal, int total, float scrollOffset,
                                     const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                     const sf::Font& fontBold, const sf::Font& fontSerif,
                                     std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawCashModal(sf::RenderTarget& target, sf::Vector2f mouse,
                              float currentWidth, float designHeight,
                              int total, int cashGiven, const std::string& cashGivenStr,
                              bool inputFocused,
                              const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                              const sf::Font& fontBold, const sf::Font& fontSerif,
                              std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawCardModal(sf::RenderTarget& target, sf::Vector2f mouse,
                              float currentWidth, float designHeight, int total,
                              const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                              const sf::Font& fontBold, const sf::Font& fontSerif,
                              std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawQtyModal(sf::RenderTarget& target, sf::Vector2f mouse,
                             float currentWidth, float designHeight,
                             const ProductItem& product, const std::string& qtyInputStr,
                             const sf::Font& fontSemiBold, const sf::Font& fontBold,
                             const sf::Font& fontSerif);

    static void drawAddStaffModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                  float currentWidth, float designHeight,
                                  const std::string& newStaffName,
                                  const std::string& newStaffPass,
                                  const std::string& newStaffRole,
                                  int activeField,
                                  const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                  const sf::Font& fontBold, const sf::Font& fontSerif);

    static void drawResetPassModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                   float currentWidth, float designHeight,
                                   const std::string& targetUser,
                                   const std::string& newPass,
                                   bool inputFocused,
                                   const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                   const sf::Font& fontBold, const sf::Font& fontSerif);

    static void drawWalletModal(sf::RenderTarget& target, sf::Vector2f mouse) {}
    static void drawDescModal(sf::RenderTarget& target, sf::Vector2f mouse) {}
    static void drawAddProductModal(sf::RenderTarget& target, sf::Vector2f mouse) {}
    static void drawRestockModal(sf::RenderTarget& target, sf::Vector2f mouse) {}
};

#endif // POS_MODALS_VIEW_H

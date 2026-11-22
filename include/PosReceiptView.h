#ifndef POS_RECEIPT_VIEW_H
#define POS_RECEIPT_VIEW_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Separation of Concerns (SoC)
// Module: Right-Panel Receipt Drawer, Perforated Paper & Line Items View
// ============================================================================

class PosReceiptView {
public:
    static void drawReceiptDrawer(sf::RenderTarget& target, sf::Vector2f mouse,
                                 float panelX, float rpX, float designHeight,
                                 const std::vector<CartLine>& cart,
                                 const std::vector<ProductItem>& products,
                                 int subtotal, int discount, int total,
                                 bool discountApplied, int paymentMethod, bool paymentDone,
                                 float receiptScrollOffset,
                                 int draftsCount,
                                 const sf::Font& fontRegular, const sf::Font& fontMedium,
                                 const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                 const sf::Font& fontSerif,
                                 std::function<const sf::Texture&(const std::string&)> getTexture);
};

#endif // POS_RECEIPT_VIEW_H

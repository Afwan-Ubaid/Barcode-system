#ifndef POS_PRODUCTS_VIEW_H
#define POS_PRODUCTS_VIEW_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "PosModels.h"
#include "InventoryManager.h"
#include "../../backend/include/Decoder.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Separation of Concerns (SoC)
// Module: Product Card Grid, Live Optical Viewport, Catalog Table & Inventory Views
// ============================================================================

class PosProductsView {
public:
    static void drawViewport(sf::RenderTarget& target, float mainX,
                             const sf::Texture& cameraTexture,
                             float flashTimer, float lastScanTimer, const std::string& lastScannedInfo,
                             bool barcodeFound, const BarcodeBox& barcodeBox,
                             int camWidth, int camHeight, const std::string& detectedBarcode,
                             const sf::Font& fontSemiBold, const sf::Font& fontBold,
                             std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawProducts(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                             const std::vector<ProductItem>& products,
                             const std::vector<int>& visibleIndices,
                             const std::string& activeCategory,
                             const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                             const sf::Font& fontBold, const sf::Font& fontSerif,
                             std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawProductsTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                const std::vector<ProductItem>& products,
                                float scrollOffset, const std::string& currentRole,
                                const std::string& productFilePath,
                                const sf::Font& fontRegular, const sf::Font& fontMedium,
                                const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                const sf::Font& fontSerif);

    static void drawInventoryTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                 const std::vector<ProductItem>& products,
                                 const InventoryManager& inventoryMgr,
                                 int inventorySubTab, float scrollOffset,
                                 const std::string& currentRole,
                                 const sf::Font& fontRegular, const sf::Font& fontMedium,
                                 const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                 const sf::Font& fontSerif);
};

#endif // POS_PRODUCTS_VIEW_H

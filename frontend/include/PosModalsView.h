#ifndef POS_MODALS_VIEW_H
#define POS_MODALS_VIEW_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "PosModels.h"
#include "CartService.h"
#include "InventoryManager.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Separation of Concerns (SoC)
// Module: Popup Modals (Receipt, Enlarge Cart, Cash, Card, Qty, Staff Management)
// ============================================================================

class PosModalsView {
public:
    static void drawReceiptModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                 float currentWidth, float designHeight,
                                 const CartService& cartService,
                                 const BillSummary& summary,
                                 const std::vector<ProductItem>& products,
                                 const std::string& cashierName,
                                 const sf::Font& fontRegular, const sf::Font& fontMedium,
                                 const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                 const sf::Font& fontSerif,
                                 std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawEnlargeCartModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                     float currentWidth, float designHeight,
                                     const std::vector<CartLine>& cart,
                                     const std::vector<ProductItem>& products,
                                     const BillSummary& summary, float scrollOffset,
                                     bool paymentDone,
                                     const sf::Texture& cameraTexture,
                                     const std::string& barcodeQuery,
                                     bool scannerInputFocused,
                                     const std::string& lastScannedInfo,
                                     const std::string& activeCameraName,
                                     const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                                     const sf::Font& fontBold, const sf::Font& fontSerif,
                                     std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawCashModal(sf::RenderTarget& target, sf::Vector2f mouse,
                              float currentWidth, float designHeight,
                              const BillSummary& summary, int cashGiven, const std::string& cashGivenStr,
                              bool inputFocused,
                              const std::string& specialDiscountStr, bool specialDiscountFocused,
                              const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                              const sf::Font& fontBold, const sf::Font& fontSerif,
                              std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawCardModal(sf::RenderTarget& target, sf::Vector2f mouse,
                              float currentWidth, float designHeight, const BillSummary& summary,
                              const std::string& specialDiscountStr, bool specialDiscountFocused,
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

    static void drawAddInventoryModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                      float currentWidth, float designHeight,
                                      const sf::Texture& cameraTexture,
                                      const std::string& supplier,
                                      const std::string& desc,
                                      const std::string& category,
                                      const std::vector<ProductCategory>& categories,
                                      const std::string& unit,
                                      const std::string& retailPriceStr,
                                      const std::string& costPriceStr,
                                      const std::string& qtyStr,
                                      const std::string& dateStr,
                                      const std::string& expiryDateStr,
                                      const std::string& barcodeStr,
                                      int activeField,
                                      bool barcodeGenerated,
                                      const std::vector<std::string>& previousVendors,
                                      bool vendorDropdownOpen,
                                      bool scanningUniversal,
                                      bool alsoAddToCatalog,
                                      const std::string& activeCameraName,
                                      const sf::Font& fontRegular, const sf::Font& fontMedium,
                                      const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                      const sf::Font& fontSerif,
                                      std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawProductSpecModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                     float currentWidth, float designHeight,
                                     const ProductItem& product,
                                     const sf::Font& fontRegular, const sf::Font& fontMedium,
                                     const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                     const sf::Font& fontSerif,
                                     std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawQuickCheckModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                    float currentWidth, float designHeight,
                                    const sf::Texture& cameraTexture,
                                    const std::string& barcodeQuery,
                                    bool inputFocused,
                                    const ProductItem* foundProduct,
                                    const sf::Font& fontRegular, const sf::Font& fontMedium,
                                    const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                    const sf::Font& fontSerif,
                                    std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawItemDescriptionModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                         float currentWidth, float designHeight,
                                         const std::string& name, const std::string& barcode,
                                         const std::string& category, const std::string& unit,
                                         int price, int cost, int stock,
                                         const std::string& supplier, const std::string& date,
                                         const sf::Font& fontRegular, const sf::Font& fontMedium,
                                         const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                         const sf::Font& fontSerif,
                                         std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawAddProductFromInventoryModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                                float currentWidth, float designHeight,
                                                const std::vector<InventoryItem>& invItems,
                                                const std::vector<int>& visibleIndices,
                                                const std::string& searchQuery,
                                                bool searchFocused,
                                                int selectedItemIdx,
                                                const std::string& transferQtyStr,
                                                float scrollOffset,
                                                const sf::Font& fontRegular, const sf::Font& fontMedium,
                                                const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                                const sf::Font& fontSerif,
                                                std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawHeldDraftsModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                   float currentWidth, float designHeight,
                                   const std::vector<DraftCart>& drafts,
                                   const std::vector<ProductItem>& products,
                                   const sf::Font& fontRegular, const sf::Font& fontMedium,
                                   const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                   const sf::Font& fontSerif);

    static void drawDiscountModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                  float currentWidth, float designHeight,
                                  const CartService& cartService,
                                  int studentRate, int employeeRate, int activeSeasonalPct,
                                  const std::string& empUser, const std::string& empPass, int activeField,
                                  const std::string& promoInput,
                                  const std::string& errorMessage,
                                  const sf::Font& fontRegular, const sf::Font& fontMedium,
                                  const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                  const sf::Font& fontSerif,
                                  std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawSaleDetailModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                    float currentWidth, float designHeight,
                                    const SaleRecord& sale,
                                    const std::vector<ProductItem>& products,
                                    const sf::Font& fontRegular, const sf::Font& fontMedium,
                                    const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                    const sf::Font& fontSerif,
                                    std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawPromoManagementModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                         float currentWidth, float designHeight,
                                         const std::vector<PromoRecord>& promoList,
                                         const std::string& newCode,
                                         const std::string& newDiscount,
                                         const std::string& newTargetType,
                                         const std::string& newTargetValue,
                                         int activeField,
                                         const std::vector<std::string>& supplierList,
                                         const sf::Font& fontRegular, const sf::Font& fontMedium,
                                         const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                         const sf::Font& fontSerif);

    static void drawEditProductModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                     float currentWidth, float designHeight,
                                     const std::string& name, const std::string& priceStr,
                                     const std::string& unit, const std::string& stockStr,
                                     const std::string& category, const std::string& catId,
                                     const std::string& discountStr, int activeField,
                                     const std::vector<ProductCategory>& categories,
                                     const sf::Font& fontRegular, const sf::Font& fontMedium,
                                     const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                     const sf::Font& fontSerif);

    static void drawEditOrderModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                   float currentWidth, float designHeight,
                                   const std::vector<OrderRecord>& orders,
                                   const std::string& searchOrderId, bool searchFocused,
                                   const std::string& statusMessage,
                                   float scrollOffset,
                                   const sf::Font& fontRegular, const sf::Font& fontMedium,
                                   const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                   const sf::Font& fontSerif);

    static void drawManagerApprovalModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                         float currentWidth, float designHeight,
                                         const std::string& orderId, const std::string& orderDate,
                                         const std::string& passInput, const std::string& errorMessage,
                                         const sf::Font& fontRegular, const sf::Font& fontMedium,
                                         const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                         const sf::Font& fontSerif);

    static void drawAddCategoryModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                     float currentWidth, float designHeight,
                                     const std::string& catId, const std::string& catName,
                                     const std::string& catAcr, int activeField,
                                     const sf::Font& fontRegular, const sf::Font& fontMedium,
                                     const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                     const sf::Font& fontSerif);

    static void drawAddSeasonModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                   float currentWidth, float designHeight,
                                   const std::string& startDate, const std::string& endDate,
                                   const std::string& pctStr, int activeField,
                                   const sf::Font& fontRegular, const sf::Font& fontMedium,
                                   const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                   const sf::Font& fontSerif);

    static void drawSplitModal(sf::RenderTarget& target, sf::Vector2f mouse,
                               float currentWidth, float designHeight,
                               const BillSummary& summary, const std::string& splitCashStr,
                               bool cashInputFocused,
                               const std::string& specialDiscountStr, bool specialDiscountFocused,
                               const sf::Font& fontRegular, const sf::Font& fontSemiBold,
                               const sf::Font& fontBold, const sf::Font& fontSerif,
                               std::function<const sf::Texture&(const std::string&)> getTexture);

    static void drawCustomerModal(sf::RenderTarget& target, sf::Vector2f mouse,
                                  float currentWidth, float designHeight,
                                  const std::string& phoneInput, const std::string& nameInput,
                                  int activeField,
                                  const CustomerRecord* foundCustomer,
                                  bool redeemPoints,
                                  const std::vector<OrderRecord>& customerOrders,
                                  const std::string& statusMsg,
                                  const sf::Font& fontRegular, const sf::Font& fontMedium,
                                  const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                  const sf::Font& fontSerif);

    static void drawWalletModal(sf::RenderTarget& target, sf::Vector2f mouse) {}
    static void drawDescModal(sf::RenderTarget& target, sf::Vector2f mouse) {}
    static void drawAddProductModal(sf::RenderTarget& target, sf::Vector2f mouse) {}
    static void drawRestockModal(sf::RenderTarget& target, sf::Vector2f mouse) {}
};

#endif // POS_MODALS_VIEW_H

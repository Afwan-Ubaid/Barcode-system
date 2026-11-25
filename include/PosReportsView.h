#ifndef POS_REPORTS_VIEW_H
#define POS_REPORTS_VIEW_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "PosModels.h"
#include "InventoryManager.h"
#include "CategoryManager.h"
#include "SeasonManager.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Separation of Concerns (SoC)
// Module: Sales History Log, Security Activity Audit, Settings & Staff Access Views
// Tabular Manager Reports (Requirement 11) & Category/Season Settings (1 & 14)
// ============================================================================

class PosReportsView {
public:
    static void drawSalesTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                             const std::vector<SaleRecord>& salesHistory,
                             const std::vector<ProductItem>& products,
                             const InventoryManager& inventoryMgr,
                             const std::string& currentRole,
                             int salesSubTab,
                             const std::vector<ReturnRecord>& returnHistory,
                             int salesTimeFilter, float scrollOffset,
                             const std::vector<OrderRecord>& orders,
                             const std::vector<OrderDetailItem>& orderDetails,
                             const std::vector<BillRecord>& bills,
                             const std::vector<BillDetailRecord>& billDetails,
                             const std::string& customStartDate,
                             const std::string& customEndDate,
                             int customActiveField,
                             const sf::Font& fontRegular, const sf::Font& fontMedium,
                             const sf::Font& fontSemiBold, const sf::Font& fontBold,
                             const sf::Font& fontSerif);

    static void drawActivityLogsTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                    const std::vector<ActivityRecord>& logs,
                                    const std::string& logFilter, float scrollOffset,
                                    const sf::Font& fontRegular, const sf::Font& fontMedium,
                                    const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                    const sf::Font& fontSerif);

    static void drawSettingsTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                                const std::vector<StaffMember>& staffList,
                                const std::string& currentRole, const std::string& currentUserName,
                                bool cameraRunning, bool soundEnabled,
                                const std::string& cameraName, bool isExternalCam, int cameraCount,
                                int studentDiscount, int employeeDiscount, int promoCount,
                                int settingsSubTab,
                                const CategoryManager& categoryMgr,
                                const SeasonManager& seasonMgr,
                                float scrollOffset,
                                const sf::Font& fontRegular, const sf::Font& fontMedium,
                                const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                const sf::Font& fontSerif);

    static void drawReturnTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                              const std::string& returnBarcode, bool returnBarcodeFocused,
                              int returnQty, int returnMaxQty, const ProductItem* foundProduct,
                              const std::string& saleInfo, int salePrice,
                              const std::string& saleOrderId, const std::string& saleCustomer,
                              const std::string& saleDate, bool returnSuccess,
                              const std::string& statusMessage,
                              int returnReasonIdx, int returnPaymentMethod, bool returnRestock,
                              const std::vector<OrderRecord>& recentOrders,
                              bool hasReturnSlip,
                              const sf::Font& fontRegular, const sf::Font& fontMedium,
                              const sf::Font& fontSemiBold, const sf::Font& fontBold,
                              const sf::Font& fontSerif,
                              std::function<const sf::Texture&(const std::string&)> getTexture);
};

#endif // POS_REPORTS_VIEW_H

#ifndef POS_REPORTS_VIEW_H
#define POS_REPORTS_VIEW_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Separation of Concerns (SoC)
// Module: Sales History Log, Security Activity Audit, Settings & Staff Access Views
// ============================================================================

class PosReportsView {
public:
    static void drawSalesTab(sf::RenderTarget& target, sf::Vector2f mouse, float mainX,
                             const std::vector<SaleRecord>& salesHistory, float scrollOffset,
                             const sf::Font& fontRegular, const sf::Font& fontBold,
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
                                const sf::Font& fontRegular, const sf::Font& fontMedium,
                                const sf::Font& fontSemiBold, const sf::Font& fontBold,
                                const sf::Font& fontSerif);

    static void drawReturnTab(sf::RenderTarget& target, sf::Vector2f mouse);
};

#endif // POS_REPORTS_VIEW_H

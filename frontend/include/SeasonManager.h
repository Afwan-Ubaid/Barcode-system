#ifndef SEASON_MANAGER_H
#define SEASON_MANAGER_H

#include <string>
#include <vector>
#include "PosModels.h"

// ============================================================================
// D3 Req 13, 14, 15: Seasons.txt + seasonal discount calculation
// SRP: loads/saves seasons and answers "what discount applies on date X".
// ============================================================================
class SeasonManager {
public:
    SeasonManager();

    bool loadSeasons(const std::string& path = "Seasons.txt");
    bool saveSeasons(const std::string& path = "Seasons.txt") const;

    const std::vector<Season>& getSeasons() const { return mSeasons; }

    bool addSeason(const Season& s);
    bool toggleActive(int index);

    // Returns the discount percent that applies on todayYYYYMMDD,
    // or 0 if no active season covers that date.
    int getActiveDiscountForDate(const std::string& todayYYYYMMDD) const;

private:
    std::vector<Season> mSeasons;
};

#endif // SEASON_MANAGER_H
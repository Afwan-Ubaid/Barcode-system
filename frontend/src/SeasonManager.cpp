#include "SeasonManager.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

SeasonManager::SeasonManager() {
    loadSeasons();
    if (mSeasons.empty()) {
        mSeasons = {
            {"2026-11-01", "2026-11-30", 10, true},
            {"2026-12-20", "2027-01-05", 15, true}
        };
        saveSeasons();
    }
}

bool SeasonManager::loadSeasons(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    mSeasons.clear();
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.empty()) continue;
        if (line.back() == '#') {
            line.pop_back();
            while (!line.empty() && line.back() == ' ') line.pop_back();
        }
        std::istringstream iss(line);
        std::string start, end, pctStr, activeStr;
        if (iss >> start >> end >> pctStr >> activeStr) {
            Season s;
            s.startDate = start;
            s.endDate = end;
            try { s.discountPercent = std::stoi(pctStr); } catch (...) { s.discountPercent = 0; }
            s.isActive = (activeStr == "true" || activeStr == "1");
            mSeasons.push_back(s);
        }
    }
    return true;
}

bool SeasonManager::saveSeasons(const std::string& path) const {
    std::vector<std::string> targets = {path};
    if (path == "Seasons.txt" && fs::exists("frontend")) {
        targets.push_back("frontend/Seasons.txt");
    }
    bool ok = false;
    for (const auto& p : targets) {
        std::ofstream out(p);
        if (out.is_open()) {
            for (const auto& s : mSeasons) {
                out << s.startDate << " "
                    << s.endDate << " "
                    << s.discountPercent << " "
                    << (s.isActive ? "true" : "false") << " #\n";
            }
            ok = true;
        }
    }
    return ok;
}

bool SeasonManager::addSeason(const Season& s) {
    mSeasons.push_back(s);
    saveSeasons();
    return true;
}

bool SeasonManager::toggleActive(int index) {
    if (index < 0 || index >= (int)mSeasons.size()) return false;
    mSeasons[index].isActive = !mSeasons[index].isActive;
    saveSeasons();
    return true;
}

bool SeasonManager::deleteSeason(int index) {
    if (index < 0 || index >= (int)mSeasons.size()) return false;
    mSeasons.erase(mSeasons.begin() + index);
    saveSeasons();
    return true;
}

int SeasonManager::getActiveDiscountForDate(const std::string& today) const {
    int best = 0;
    for (const auto& s : mSeasons) {
        if (!s.isActive) continue;
        // ISO date YYYY-MM-DD: lexicographic == chronological
        if (today >= s.startDate && today <= s.endDate) {
            best = std::max(best, s.discountPercent);
        }
    }
    return best;
}

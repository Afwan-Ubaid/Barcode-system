#include "SeasonManager.h"
#include <fstream>
#include <sstream>

SeasonManager::SeasonManager() {
    loadSeasons();
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
    std::ofstream out(path);
    if (!out.is_open()) return false;
    for (const auto& s : mSeasons) {
        out << s.startDate << " "
            << s.endDate << " "
            << s.discountPercent << " "
            << (s.isActive ? "true" : "false") << " #\n";
    }
    return true;
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

int SeasonManager::getActiveDiscountForDate(const std::string& today) const {
    for (const auto& s : mSeasons) {
        if (!s.isActive) continue;
        // ISO date YYYY-MM-DD: lexicographic == chronological
        if (today >= s.startDate && today <= s.endDate) {
            return s.discountPercent;
        }
    }
    return 0;
}
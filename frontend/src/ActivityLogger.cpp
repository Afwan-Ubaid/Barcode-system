#include "ActivityLogger.h"
#include "UIUtils.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

ActivityLogger::ActivityLogger() {
    loadActivityLogs();
}

bool ActivityLogger::loadActivityLogs(const std::string& customPath) {
    if (!customPath.empty() && fs::exists(customPath)) {
        mActivityLogFilePath = customPath;
    } else {
        std::string foundPath = resolveDataFilePath("activity_log.txt");
        if (!fs::exists(foundPath)) {
            std::ofstream out(foundPath);
            if (out.is_open()) {
                out << "# ScanIT Activity Log & Audit Trail\n"
                    << "# Format: Timestamp|Category|User|Details\n"
                    << "2026-11-17 01:00:00|LOGIN|admin|Administrator session authenticated\n"
                    << "2026-11-17 01:27:10|SALE|admin|Processed sale #1001 of Rs 1,090 (Cash)\n";
                out.close();
            }
        }
        mActivityLogFilePath = foundPath;
    }

    std::ifstream in(mActivityLogFilePath);
    if (!in.is_open()) return false;

    mActivityLogs.clear();
    std::string line;
    while (std::getline(in, line)) {
        line = trimStr(line);
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string ts, cat, usr, det;
        std::getline(ss, ts, '|');
        std::getline(ss, cat, '|');
        std::getline(ss, usr, '|');
        std::getline(ss, det, '|');
        ts = trimStr(ts);
        cat = trimStr(cat);
        usr = trimStr(usr);
        det = trimStr(det);
        if (!ts.empty() && !cat.empty()) {
            mActivityLogs.push_back({ts, cat, usr.empty() ? "system" : usr, det});
        }
    }
    return true;
}

void ActivityLogger::logActivity(const std::string& category, const std::string& user, const std::string& details) {
    std::string ts = getCurrentDateYYYYMMDD() + " " + getCurrentTimeHHMMSS();
    std::string usr = user.empty() ? "admin" : user;
    mActivityLogs.push_back({ts, category, usr, details});

    std::ofstream out(mActivityLogFilePath.empty() ? "activity_log.txt" : mActivityLogFilePath, std::ios::app);
    if (out.is_open()) {
        out << ts << "|" << category << "|" << usr << "|" << details << "\n";
    }
}

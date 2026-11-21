#ifndef ACTIVITY_LOGGER_H
#define ACTIVITY_LOGGER_H

#include <string>
#include <vector>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Single Responsibility Principle (SRP)
// Module: System Activity Logging, Audit Trails & Persistence
// ============================================================================

class ActivityLogger {
public:
    ActivityLogger();

    bool loadActivityLogs(const std::string& customPath = "");
    void logActivity(const std::string& category, const std::string& user, const std::string& details);

    const std::vector<ActivityRecord>& getLogs() const { return mActivityLogs; }
    std::vector<ActivityRecord>& getLogs() { return mActivityLogs; }
    const std::string& getLogFilePath() const { return mActivityLogFilePath; }

private:
    std::vector<ActivityRecord> mActivityLogs;
    std::string mActivityLogFilePath;
};

#endif // ACTIVITY_LOGGER_H

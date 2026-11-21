#ifndef STAFF_MANAGER_H
#define STAFF_MANAGER_H

#include <string>
#include <vector>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Single Responsibility Principle (SRP)
// Module: Staff Authentication, Role Access Control, and Persistence
// ============================================================================

class StaffManager {
public:
    StaffManager();

    bool loadStaffFile(const std::string& customPath = "");
    bool saveStaffFile();

    bool authenticateUser(const std::string& username, const std::string& password, const std::string& role);
    bool authenticate(const std::string& username, const std::string& password, const std::string& role = "Employee") {
        return authenticateUser(username, password, role);
    }
    bool addStaffMember(const std::string& username, const std::string& password, const std::string& role);
    bool removeStaffMember(const std::string& username);
    bool resetStaffPassword(const std::string& username, const std::string& newPassword);

    const std::vector<StaffMember>& getStaffList() const { return mStaffList; }
    std::vector<StaffMember>& getStaffList() { return mStaffList; }
    const std::string& getStaffFilePath() const { return mStaffFilePath; }

private:
    std::vector<StaffMember> mStaffList;
    std::string mStaffFilePath;
};

#endif // STAFF_MANAGER_H

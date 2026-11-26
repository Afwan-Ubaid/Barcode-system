#pragma once

#include "PosModels.h"
#include <string>
#include <vector>

class CustomerManager {
public:
    bool loadCustomers(const std::string& filePath = "customers.txt");
    bool saveCustomers(const std::string& filePath = "") const;

    const CustomerRecord* findByPhone(const std::string& phone) const;
    CustomerRecord* findByPhoneMutable(const std::string& phone);
    std::vector<CustomerRecord> search(const std::string& query) const;
    const std::vector<CustomerRecord>& getAllCustomers() const { return mCustomers; }

    // Creates or updates customer name/phone. Returns pointer to stored record.
    const CustomerRecord* upsertCustomer(const std::string& phone, const std::string& name);

    // Records a completed sale: deducts redeemedPoints, adds earnedPoints (totalPayable / 100), increments visits & totalSpent.
    bool recordPurchase(const std::string& phone, int totalPayable, int redeemedPoints, const std::string& dateStr);

    std::string getFilePath() const { return mFilePath; }

private:
    std::vector<CustomerRecord> mCustomers;
    std::string mFilePath = "customers.txt";

    static std::string normalizePhone(const std::string& phone);
};

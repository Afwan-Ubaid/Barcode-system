#include "CustomerManager.h"
#include "UIUtils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

std::string CustomerManager::normalizePhone(const std::string& phone) {
    std::string out;
    for (unsigned char c : phone) {
        if (std::isdigit(c) || c == '+') out.push_back(static_cast<char>(c));
    }
    return out;
}

static std::string sanitizeCustomerField(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c == '|' || c == '\n' || c == '\r' || c == '#') c = ' ';
    }
    return out;
}

bool CustomerManager::loadCustomers(const std::string& filePath) {
    mFilePath = resolveDataFilePath(filePath);
    mCustomers.clear();

    std::ifstream file(mFilePath);
    if (!file.is_open()) {
        // Seed a couple of sample loyalty customers or start empty cleanly
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line.rfind("phone|", 0) == 0 || line[0] == ';') continue;

        std::stringstream ss(line);
        std::string phone, name, ptsStr, spentStr, visitsStr, lastVisit;
        if (!std::getline(ss, phone, '|')) continue;
        if (!std::getline(ss, name, '|')) continue;
        if (!std::getline(ss, ptsStr, '|')) continue;
        if (!std::getline(ss, spentStr, '|')) continue;
        if (!std::getline(ss, visitsStr, '|')) continue;
        std::getline(ss, lastVisit);

        if (!lastVisit.empty() && lastVisit.back() == '#') lastVisit.pop_back();
        while (!lastVisit.empty() && (lastVisit.back() == ' ' || lastVisit.back() == '\r')) lastVisit.pop_back();

        CustomerRecord rec;
        rec.phone = normalizePhone(phone);
        rec.name = name;
        try {
            rec.points = std::stoi(ptsStr);
            rec.totalSpent = std::stoi(spentStr);
            rec.visits = std::stoi(visitsStr);
        } catch (...) {
            continue;
        }
        rec.lastVisit = lastVisit;
        if (!rec.phone.empty()) mCustomers.push_back(rec);
    }
    return true;
}

bool CustomerManager::saveCustomers(const std::string& filePath) const {
    std::string target = filePath.empty() ? mFilePath : filePath;
    if (target.empty()) target = "customers.txt";

    std::ofstream out(target, std::ios::trunc);
    if (!out.is_open()) return false;

    out << "phone|name|points|totalSpent|visits|lastVisit\n";
    for (const auto& c : mCustomers) {
        out << c.phone << "|"
            << sanitizeCustomerField(c.name) << "|"
            << c.points << "|"
            << c.totalSpent << "|"
            << c.visits << "|"
            << sanitizeCustomerField(c.lastVisit) << "\n";
    }
    return true;
}

const CustomerRecord* CustomerManager::findByPhone(const std::string& phone) const {
    std::string norm = normalizePhone(phone);
    if (norm.empty()) return nullptr;
    for (const auto& c : mCustomers) {
        if (c.phone == norm) return &c;
    }
    return nullptr;
}

CustomerRecord* CustomerManager::findByPhoneMutable(const std::string& phone) {
    std::string norm = normalizePhone(phone);
    if (norm.empty()) return nullptr;
    for (auto& c : mCustomers) {
        if (c.phone == norm) return &c;
    }
    return nullptr;
}

std::vector<CustomerRecord> CustomerManager::search(const std::string& query) const {
    if (query.empty()) return mCustomers;
    std::string qLower = query;
    std::transform(qLower.begin(), qLower.end(), qLower.begin(), ::tolower);

    std::vector<CustomerRecord> out;
    for (const auto& c : mCustomers) {
        std::string nLower = c.name;
        std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);
        if (nLower.find(qLower) != std::string::npos || c.phone.find(qLower) != std::string::npos) {
            out.push_back(c);
        }
    }
    return out;
}

const CustomerRecord* CustomerManager::upsertCustomer(const std::string& phone, const std::string& name) {
    std::string norm = normalizePhone(phone);
    if (norm.empty()) return nullptr;

    if (CustomerRecord* existing = findByPhoneMutable(norm)) {
        if (!name.empty()) existing->name = sanitizeCustomerField(name);
        saveCustomers();
        return existing;
    }

    CustomerRecord rec;
    rec.phone = norm;
    rec.name = name.empty() ? ("Customer " + norm.substr(norm.size() > 4 ? norm.size() - 4 : 0)) : sanitizeCustomerField(name);
    rec.points = 0;
    rec.totalSpent = 0;
    rec.visits = 0;
    rec.lastVisit = "";
    mCustomers.push_back(rec);
    saveCustomers();
    return &mCustomers.back();
}

bool CustomerManager::recordPurchase(const std::string& phone, int totalPayable, int redeemedPoints, const std::string& dateStr) {
    CustomerRecord* c = findByPhoneMutable(phone);
    if (!c) return false;

    int redeem = std::max(0, std::min(redeemedPoints, c->points));
    c->points -= redeem;
    int earned = std::max(0, totalPayable / 100);
    c->points += earned;
    c->totalSpent += std::max(0, totalPayable);
    c->visits += 1;
    c->lastVisit = dateStr;
    return saveCustomers();
}

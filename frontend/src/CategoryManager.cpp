#include "CategoryManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

CategoryManager::CategoryManager() {
    if (!loadCategories("PCategory.txt")) {
        loadCategories("pCategory.txt");
    }
    if (mCategories.empty()) {
        mCategories = {
            {"01", "Dairy",      "DAIRY"},
            {"02", "Bakery",     "BAKE"},
            {"03", "Drinks",     "DRINK"},
            {"04", "Snacks",     "SNCK"},
            {"05", "Household",  "HOUS"},
            {"06", "Fruits",     "FRUIT"},
            {"07", "Vegetables", "VEG"},
            {"08", "Personal",   "PERS"},
            {"09", "Grocery",    "GROC"}
        };
        saveCategories("PCategory.txt");
        saveCategories("pCategory.txt");
    }
}

bool CategoryManager::loadCategories(const std::string& path) {
    std::string targetPath = path;
    if (!fs::exists(targetPath)) {
        if (targetPath == "PCategory.txt" && fs::exists("pCategory.txt")) {
            targetPath = "pCategory.txt";
        } else if (targetPath == "pCategory.txt" && fs::exists("PCategory.txt")) {
            targetPath = "PCategory.txt";
        } else {
            return false;
        }
    }

    std::ifstream in(targetPath);
    if (!in.is_open()) return false;

    mCategories.clear();
    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.empty()) continue;
        if (line.back() == '#') {
            line.pop_back();
            while (!line.empty() && line.back() == ' ') line.pop_back();
        }
        std::istringstream iss(line);
        ProductCategory cat;
        if (iss >> cat.categoryId >> cat.name >> cat.acronym) {
            for (char& ch : cat.name) if (ch == '_') ch = ' ';
            for (char& ch : cat.acronym) if (ch == '_') ch = ' ';
            mCategories.push_back(cat);
        }
    }
    return !mCategories.empty();
}

bool CategoryManager::saveCategories(const std::string& path) const {
    std::vector<std::string> paths = {path};
    if (path == "PCategory.txt") paths.push_back("pCategory.txt");
    else if (path == "pCategory.txt") paths.push_back("PCategory.txt");
    if (fs::exists("frontend")) {
        paths.push_back("frontend/pCategory.txt");
    }

    bool success = false;
    for (const auto& p : paths) {
        std::ofstream out(p);
        if (out.is_open()) {
            for (const auto& c : mCategories) {
                std::string sName = c.name;
                std::string sAcr = c.acronym;
                for (char& ch : sName) if (ch == ' ') ch = '_';
                for (char& ch : sAcr) if (ch == ' ') ch = '_';
                out << c.categoryId << " " << sName << " " << sAcr << " #\n";
            }
            out.close();
            success = true;
        }
    }
    return success;
}

bool CategoryManager::addCategory(const ProductCategory& cat) {
    if (findById(cat.categoryId)) return false;
    mCategories.push_back(cat);
    saveCategories();
    return true;
}

bool CategoryManager::updateCategory(const ProductCategory& cat) {
    for (auto& c : mCategories) {
        if (c.categoryId == cat.categoryId) {
            c = cat;
            saveCategories();
            return true;
        }
    }
    return false;
}

bool CategoryManager::deleteCategory(const std::string& id) {
    auto it = std::remove_if(mCategories.begin(), mCategories.end(), [&](const ProductCategory& c) {
        return c.categoryId == id;
    });
    if (it != mCategories.end()) {
        mCategories.erase(it, mCategories.end());
        saveCategories();
        return true;
    }
    return false;
}

const ProductCategory* CategoryManager::findById(const std::string& id) const {
    for (const auto& c : mCategories) {
        if (c.categoryId == id) return &c;
    }
    return nullptr;
}

const ProductCategory* CategoryManager::findByName(const std::string& name) const {
    for (const auto& c : mCategories) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

std::string CategoryManager::nextCategoryId() const {
    int maxId = 0;
    for (const auto& c : mCategories) {
        try { maxId = std::max(maxId, std::stoi(c.categoryId)); } catch (...) {}
    }
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d", maxId + 1);
    return std::string(buf);
}

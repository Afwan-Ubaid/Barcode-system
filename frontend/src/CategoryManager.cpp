#include "CategoryManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>

CategoryManager::CategoryManager() {
    loadCategories();
    if (mCategories.empty()) {
        // Seed defaults so the demo isn't empty on first run
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
        saveCategories();
    }
}

bool CategoryManager::loadCategories(const std::string& path) {
    std::ifstream in(path);
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
            mCategories.push_back(cat);
        }
    }
    return true;
}

bool CategoryManager::saveCategories(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) return false;
    for (const auto& c : mCategories) {
        out << c.categoryId << " " << c.name << " " << c.acronym << " #\n";
    }
    return true;
}

bool CategoryManager::addCategory(const ProductCategory& cat) {
    if (findById(cat.categoryId)) return false;
    mCategories.push_back(cat);
    saveCategories();
    return true;
}

const ProductCategory* CategoryManager::findById(const std::string& id) const {
    for (const auto& c : mCategories) {
        if (c.categoryId == id) return &c;
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
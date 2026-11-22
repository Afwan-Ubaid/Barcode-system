#include "ProductCatalog.h"
#include "UIUtils.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

ProductCatalog::ProductCatalog() {
    loadProductsFile();
}

bool ProductCatalog::saveDefaultProductFile(const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << "# ScanIT Point of Sale - Product Catalog Database\n"
        << "# Format: Barcode,Name,Price,Unit,Category,IconKey,Stock\n"
        << "100001,Fresh Milk,320,1 L,Grocery,milk,24\n"
        << "100002,Sourdough Bread,180,1 loaf,Grocery,bread,15\n"
        << "100003,Atelier coffee,180,250 g,Drinks,coffee,32\n"
        << "100004,Red apples,240,1 kg,Grocery,apple,40\n"
        << "100005,Orange Juice,290,1 L,Drinks,juice,18\n"
        << "100006,Basmati rice,540,1 kg,Grocery,rice,50\n"
        << "100007,Pistachio Crunch,350,150 g,Snacks,coffee,20\n"
        << "100008,Lavender Deterg.,480,1.5 L,Household,milk,12\n"
        << "234567,Cereal Box,420,500 g,Grocery,bread,25\n"
        << "567890,Green Tea,260,100 g,Drinks,juice,30\n"
        << "6001001,Fresh Milk,320,1 L,Grocery,milk,24\n"
        << "6001002,Sourdough Bread,180,1 loaf,Grocery,bread,15\n"
        << "6001003,Atelier coffee,180,250 g,Drinks,coffee,32\n"
        << "6001004,Red apples,240,1 kg,Grocery,apple,40\n"
        << "6001005,Orange Juice,290,1 L,Drinks,juice,18\n"
        << "6001006,Basmati rice,540,1 kg,Grocery,rice,50\n"
        << "6001007,Pistachio Crunch,350,150 g,Snacks,coffee,20\n"
        << "6001008,Lavender Deterg.,480,1.5 L,Household,milk,12\n";
    out.close();
    return true;
}

bool ProductCatalog::loadProductsFile(const std::string& customPath) {
    if (!customPath.empty() && fs::exists(customPath)) {
        mProductFilePath = customPath;
    } else {
        std::vector<std::string> candidates = {
            "product.txt",
            "frontend/product.txt",
            "assets/product.txt",
            "../product.txt",
            "../../product.txt"
        };

        std::string foundPath;
        for (const auto& c : candidates) {
            if (fs::exists(c)) {
                foundPath = c;
                break;
            }
        }

        if (foundPath.empty()) {
            saveDefaultProductFile("product.txt");
            foundPath = "product.txt";
        }
        mProductFilePath = foundPath;
    }

    std::ifstream in(mProductFilePath);
    if (!in.is_open()) return false;

    mProducts.clear();
    const sf::Color tints[] = {
        sf::Color(240, 230, 222),
        sf::Color(241, 236, 214),
        sf::Color(232, 236, 230),
        sf::Color(242, 228, 222),
        sf::Color(243, 236, 218),
        sf::Color(232, 236, 238),
        sf::Color(240, 236, 220),
        sf::Color(230, 234, 240)
    };

    std::string line;
    int idCounter = 0;
    while (std::getline(in, line)) {
        line = trimStr(line);
        if (line.empty() || line[0] == '#') continue;

        if (line.find(',') != std::string::npos) {
            std::stringstream ss(line);
            std::string barcode, name, priceStr, unit, category, iconKey, stockStr;

            std::getline(ss, barcode, ',');
            std::getline(ss, name, ',');
            std::getline(ss, priceStr, ',');
            std::getline(ss, unit, ',');
            std::getline(ss, category, ',');
            std::getline(ss, iconKey, ',');
            std::getline(ss, stockStr, ',');

            barcode = trimStr(barcode);
            name = trimStr(name);
            priceStr = trimStr(priceStr);
            unit = trimStr(unit);
            category = trimStr(category);
            iconKey = trimStr(iconKey);
            stockStr = trimStr(stockStr);

            if (barcode.empty() || name.empty()) continue;

            int price = 100;
            try { price = std::stoi(priceStr); } catch (...) {}
            int stock = 20;
            try { stock = std::stoi(stockStr); } catch (...) {}

            if (iconKey.empty()) iconKey = "milk";
            if (unit.empty()) unit = "1 pc";
            if (category.empty()) category = "Grocery";

            sf::Color tint = tints[idCounter % 8];
            mProducts.push_back({idCounter, name, unit, category, iconKey, barcode, price, stock, tint});
            idCounter++;
        } else {
            // Space-delimited project standard format:
            // <id> <barcode> <acronym> <name> <unit> <unitPrice> <tax> <discount> <stock> <barcodePath> #
            std::string clean = line;
            if (!clean.empty() && clean.back() == '#') clean.pop_back();
            std::istringstream iss(clean);
            std::vector<std::string> tokens;
            std::string tok;
            while (iss >> tok) tokens.push_back(tok);
            if (tokens.size() < 9) continue;

            std::string pId = tokens[0];
            std::string barcode = tokens[1];
            std::string acronym = tokens[2];
            std::string rawName = tokens[3];
            for (char& c : rawName) if (c == '_') c = ' ';
            std::string unit = tokens[4];
            int price = 100, stock = 20;
            try { price = std::stoi(tokens[5]); } catch (...) {}
            try { stock = std::stoi(tokens[8]); } catch (...) {}

            std::string category = "Grocery";
            std::string iconKey = "milk";
            std::string lower = toLowerStr(rawName);
            if (lower.find("coffee") != std::string::npos) { category = "Drinks"; iconKey = "coffee"; }
            else if (lower.find("tea") != std::string::npos || lower.find("juice") != std::string::npos) { category = "Drinks"; iconKey = "juice"; }
            else if (lower.find("bread") != std::string::npos) { category = "Bakery"; iconKey = "bread"; }
            else if (lower.find("milk") != std::string::npos) { category = "Dairy"; iconKey = "milk"; }
            else if (lower.find("apple") != std::string::npos || lower.find("fruit") != std::string::npos || lower.find("veg") != std::string::npos) { category = "Fruits & Veg"; iconKey = "apple"; }
            else if (lower.find("rice") != std::string::npos || lower.find("cereal") != std::string::npos) { category = "Grocery"; iconKey = "rice"; }
            else if (lower.find("snack") != std::string::npos || lower.find("crunch") != std::string::npos) { category = "Snacks"; iconKey = "coffee"; }
            else if (lower.find("deterg") != std::string::npos || lower.find("clean") != std::string::npos) { category = "Household"; iconKey = "milk"; }
            else if (lower.find("soap") != std::string::npos || lower.find("shampoo") != std::string::npos || lower.find("care") != std::string::npos) { category = "Personal Care"; iconKey = "milk"; }

            sf::Color tint = tints[idCounter % 8];
            mProducts.push_back({idCounter, rawName, unit, category, iconKey, barcode, price, stock, tint});
            idCounter++;
        }
    }
    in.close();

    if (mProducts.empty()) {
        mProducts = {
            {0, "Atelier coffee",  "250 g",  "Drinks",    "coffee", "100003", 180, 32, sf::Color(232, 236, 230)},
            {1, "Basmati rice",    "1 kg",   "Grocery",   "rice",   "100006", 540, 50, sf::Color(232, 236, 238)},
            {2, "Fresh Milk",      "1 L",    "Grocery",   "milk",   "100001", 320, 24, sf::Color(240, 230, 222)},
            {3, "Lavender Deterg.","1.5 L",  "Household", "milk",   "100008", 480, 12, sf::Color(230, 234, 240)},
            {4, "Orange Juice",    "1 L",    "Drinks",    "juice",  "100005", 290, 18, sf::Color(243, 236, 218)},
            {5, "Pistachio Crunch","150 g",  "Snacks",    "coffee", "100007", 350, 20, sf::Color(240, 236, 220)},
            {6, "Red apples",      "1 kg",   "Grocery",   "apple",  "100004", 240, 40, sf::Color(242, 228, 222)},
            {7, "Sourdough Bread", "1 loaf", "Grocery",   "bread",  "100002", 180, 15, sf::Color(241, 236, 214)}
        };
    }
    sortAlphabetically();
    return true;
}

void ProductCatalog::sortAlphabetically() {
    std::sort(mProducts.begin(), mProducts.end(), [](const ProductItem& a, const ProductItem& b) {
        return toLowerStr(a.name) < toLowerStr(b.name);
    });
    for (size_t i = 0; i < mProducts.size(); ++i) {
        mProducts[i].id = std::to_string(101 + (int)i);
    }
}

std::vector<int> ProductCatalog::getVisibleProducts(const std::string& searchQuery, const std::string& activeCategory) const {
    std::vector<int> result;
    std::string q = toLowerStr(searchQuery);
    for (int i = 0; i < (int)mProducts.size(); i++) {
        const auto& p = mProducts[i];
        if (activeCategory != "All" && p.category != activeCategory) continue;
        if (!q.empty()) {
            std::string nameLower = toLowerStr(p.name);
            if (nameLower.find(q) == std::string::npos && p.barcode.find(q) == std::string::npos) {
                continue;
            }
        }
        result.push_back(i);
    }
    return result;
}

int ProductCatalog::findProductByBarcode(const std::string& barcode) const {
    for (int i = 0; i < (int)mProducts.size(); i++) {
        if (mProducts[i].barcode == barcode) return i;
    }
    return -1;
}

void ProductCatalog::deleteProduct(int index) {
    if (index >= 0 && index < (int)mProducts.size()) {
        mProducts.erase(mProducts.begin() + index);
    }
}

void ProductCatalog::addStock(int index, int amount) {
    if (index >= 0 && index < (int)mProducts.size()) {
        mProducts[index].stock += amount;
        saveProductsFile();
    }
}

void ProductCatalog::deductStock(int index, int amount) {
    if (index >= 0 && index < (int)mProducts.size()) {
        mProducts[index].stock = std::max(0, mProducts[index].stock - amount);
        saveProductsFile();
    }
}

void ProductCatalog::addOrUpdateProduct(const ProductItem& item) {
    int idx = findProductByBarcode(item.barcode);
    if (idx >= 0) {
        mProducts[idx].stock += item.stock;
        mProducts[idx].price = item.price;
        mProducts[idx].unitPrice = item.price;
        mProducts[idx].name = item.name;
        mProducts[idx].category = item.category;
        mProducts[idx].unit = item.unit;
    } else {
        ProductItem newItem = item;
        newItem.id = std::to_string(100 + (int)mProducts.size() + 1);
        mProducts.push_back(newItem);
    }
    sortAlphabetically();
    saveProductsFile();
}

bool ProductCatalog::saveProductsFile(const std::string& path) {
    std::string targetPath = path.empty() ? mProductFilePath : path;
    if (targetPath.empty()) targetPath = "product.txt";

    std::ofstream out(targetPath);
    if (!out.is_open()) return false;

    for (size_t i = 0; i < mProducts.size(); ++i) {
        const auto& p = mProducts[i];
        std::string sName = p.name;
        for (char& c : sName) if (c == ' ') c = '_';

        std::string acr = p.acronym.empty() ? p.name.substr(0, std::min<size_t>(8, p.name.length())) : p.acronym;
        for (char& c : acr) if (c == ' ') c = '_';
        if (acr.length() > 8) acr = acr.substr(0, 8);

        std::string bcPath = p.barcodePath.empty() ? ("barcodes/barcode_" + p.barcode + ".bmp") : p.barcodePath;

        out << (101 + (int)i) << " "
            << p.barcode << " "
            << acr << " "
            << sName << " "
            << (p.unit.empty() ? "count" : p.unit) << " "
            << p.price << " "
            << (p.price / 20) << " "  // tax ~5%
            << 0 << " "              // discount
            << p.stock << " "
            << bcPath << " #\n";
    }
    out.close();

    // Mirror to secondary paths
    std::vector<std::string> mirrors = {"product.txt", "frontend/product.txt"};
    for (const auto& m : mirrors) {
        if (m != targetPath) {
            std::ofstream mOut(m);
            if (mOut.is_open()) {
                for (size_t i = 0; i < mProducts.size(); ++i) {
                    const auto& p = mProducts[i];
                    std::string sName = p.name;
                    for (char& c : sName) if (c == ' ') c = '_';
                    std::string acr = p.acronym.empty() ? p.name.substr(0, std::min<size_t>(8, p.name.length())) : p.acronym;
                    for (char& c : acr) if (c == ' ') c = '_';
                    if (acr.length() > 8) acr = acr.substr(0, 8);
                    std::string bcPath = p.barcodePath.empty() ? ("barcodes/barcode_" + p.barcode + ".bmp") : p.barcodePath;

                    mOut << (101 + (int)i) << " "
                         << p.barcode << " "
                         << acr << " "
                         << sName << " "
                         << (p.unit.empty() ? "count" : p.unit) << " "
                         << p.price << " "
                         << (p.price / 20) << " "
                         << 0 << " "
                         << p.stock << " "
                         << bcPath << " #\n";
                }
                mOut.close();
            }
        }
    }
    return true;
}

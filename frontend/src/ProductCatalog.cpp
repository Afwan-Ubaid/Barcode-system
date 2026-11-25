#include "ProductCatalog.h"
#include "UIUtils.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

ProductCatalog::ProductCatalog() {
    loadProductsFile();
}

// ============================================================================
// Default seed file — CSV format, D3: categoryId as the 8th column
// ============================================================================
bool ProductCatalog::saveDefaultProductFile(const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << "# ScanIT Point of Sale - Product Catalog Database\n"
        << "# Format: Barcode,Name,Price,Unit,Category,IconKey,Stock,CategoryId\n"
        << "010001,Fresh Milk,320,1 L,Dairy,milk,24,01\n"
        << "020002,Sourdough Bread,180,1 loaf,Bakery,bread,15,02\n"
        << "030003,Atelier coffee,180,250 g,Drinks,coffee,32,03\n"
        << "060004,Red apples,240,1 kg,Fruits,apple,40,06\n"
        << "030005,Orange Juice,290,1 L,Drinks,juice,18,03\n"
        << "090006,Basmati rice,540,1 kg,Grocery,rice,50,09\n"
        << "040007,Pistachio Crunch,350,150 g,Snacks,coffee,20,04\n"
        << "050008,Lavender Deterg.,480,1.5 L,Household,milk,12,05\n"
        << "090007,Cereal Box,420,500 g,Grocery,bread,25,09\n"
        << "030006,Green Tea,260,100 g,Drinks,juice,30,03\n";
    out.close();
    return true;
}

// ============================================================================
// loadProductsFile — accepts BOTH CSV and space-delimited formats.
// D3: categoryId is read from CSV column 8, or from the last token
//      in the space-delimited format. Defaults to "01" if absent.
// ============================================================================
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

        // Strip trailing '#' end-of-line marker (project convention) before parsing
        if (!line.empty() && line.back() == '#') {
            line.pop_back();
            line = trimStr(line);
        }
        if (line.empty()) continue;

        // -------------------------------------------------------------------
        // CSV branch:  Barcode,Name,Price,Unit,Category,IconKey,Stock,CategoryId
        // -------------------------------------------------------------------
        if (line.find(',') != std::string::npos) {
            std::stringstream ss(line);
            std::string barcode, name, priceStr, unit, category, iconKey, stockStr, catIdCsv;

            std::getline(ss, barcode, ',');
            std::getline(ss, name, ',');
            std::getline(ss, priceStr, ',');
            std::getline(ss, unit, ',');
            std::getline(ss, category, ',');
            std::getline(ss, iconKey, ',');
            std::getline(ss, stockStr, ',');
            std::getline(ss, catIdCsv, ',');

            barcode  = trimStr(barcode);
            name     = trimStr(name);
            priceStr = trimStr(priceStr);
            unit     = trimStr(unit);
            category = trimStr(category);
            iconKey  = trimStr(iconKey);
            stockStr = trimStr(stockStr);
            catIdCsv = trimStr(catIdCsv);

            if (barcode.empty() || name.empty()) continue;

            int price = 100;
            try { price = std::stoi(priceStr); } catch (...) {}
            int stock = 20;
            try { stock = std::stoi(stockStr); } catch (...) {}

            if (iconKey.empty()) iconKey = "milk";
            if (unit.empty()) unit = "1 pc";
            if (category.empty()) category = "Grocery";

            // D3 Req 2: validate categoryId is 2 digits, else default "01"
            std::string categoryId = "01";
            if (catIdCsv.size() == 2 && catIdCsv[0] >= '0' && catIdCsv[0] <= '9'
                                    && catIdCsv[1] >= '0' && catIdCsv[1] <= '9') {
                categoryId = catIdCsv;
            }

            sf::Color tint = tints[idCounter % 8];
            ProductItem pi(idCounter, name, unit, category, iconKey, barcode, price, stock, tint);
            pi.categoryId = categoryId;
            mProducts.push_back(pi);
            idCounter++;
        }
        // -------------------------------------------------------------------
        // Space-delimited branch (D2/D3 format):
        //   id barcode acronym name unit unitPrice tax discount stock
        //   barcodePath categoryId
        // -------------------------------------------------------------------
        else {
            std::istringstream iss(line);
            std::vector<std::string> tokens;
            std::string tok;
            while (iss >> tok) tokens.push_back(tok);
            if (tokens.size() < 9) continue;

            std::string pId     = tokens[0];
            std::string barcode = tokens[1];
            std::string acronym = tokens[2];

            // Minimum 11 tokens for D3: id barcode acronym name unit price tax discount stock barcodePath categoryId
            bool hasCategoryId = (tokens.size() >= 11);

            // barcodePath index depends on whether categoryId is present
            size_t barcodePathIdx = hasCategoryId ? tokens.size() - 2 : tokens.size() - 1;
            // Tokens before barcodePath: [barcodePathIdx-5]=unit, -4=price, -3=tax, -2=disc, -1=stock
            size_t nameEndIdx     = (barcodePathIdx >= 5) ? (barcodePathIdx - 5) : 4;

            if (nameEndIdx <= 3) nameEndIdx = 4;

            std::string rawName;
            for (size_t k = 3; k < nameEndIdx && k < tokens.size(); k++) {
                if (!rawName.empty()) rawName += " ";
                rawName += tokens[k];
            }
            if (rawName.empty() && tokens.size() > 3) rawName = tokens[3];
            for (char& c : rawName) if (c == '_') c = ' ';

            std::string unit = "count";
            int price = 100, stock = 20, tax = 0, discount = 0;

            if (barcodePathIdx >= 5) {
                unit     = tokens[barcodePathIdx - 5];
                for (char& c : unit) if (c == '_') c = ' ';
                try { price    = std::stoi(tokens[barcodePathIdx - 4]); } catch (...) {}
                try { tax      = std::stoi(tokens[barcodePathIdx - 3]); } catch (...) {}
                try { discount = std::stoi(tokens[barcodePathIdx - 2]); } catch (...) {}
                try { stock    = std::stoi(tokens[barcodePathIdx - 1]); } catch (...) {}
            }

            std::string barcodePath = tokens[barcodePathIdx];

            // D3 Req 2: categoryId = last token (validated), else default "01"
            std::string categoryId = "01";
            if (hasCategoryId) {
                std::string cid = tokens.back();
                if (cid.size() == 2 && cid[0] >= '0' && cid[0] <= '9'
                                    && cid[1] >= '0' && cid[1] <= '9') {
                    categoryId = cid;
                }
            }

            // Heuristic category/icon inference from the name (preserves D2 behavior)
            std::string category = "Grocery";
            std::string iconKey  = "milk";
            std::string lower    = toLowerStr(rawName);
            if (lower.find("coffee") != std::string::npos) {
                category = "Drinks"; iconKey = "coffee";
            } else if (lower.find("tea") != std::string::npos ||
                       lower.find("juice") != std::string::npos) {
                category = "Drinks"; iconKey = "juice";
            } else if (lower.find("bread") != std::string::npos) {
                category = "Bakery"; iconKey = "bread";
            } else if (lower.find("milk") != std::string::npos) {
                category = "Dairy"; iconKey = "milk";
            } else if (lower.find("apple") != std::string::npos ||
                       lower.find("fruit") != std::string::npos ||
                       lower.find("veg")   != std::string::npos) {
                category = "Fruits & Veg"; iconKey = "apple";
            } else if (lower.find("rice") != std::string::npos ||
                       lower.find("cereal") != std::string::npos) {
                category = "Grocery"; iconKey = "rice";
            } else if (lower.find("snack") != std::string::npos ||
                       lower.find("crunch") != std::string::npos) {
                category = "Snacks"; iconKey = "coffee";
            } else if (lower.find("deterg") != std::string::npos ||
                       lower.find("clean") != std::string::npos) {
                category = "Household"; iconKey = "milk";
            } else if (lower.find("soap") != std::string::npos ||
                       lower.find("shampoo") != std::string::npos ||
                       lower.find("care") != std::string::npos) {
                category = "Personal Care"; iconKey = "milk";
            }

            sf::Color tint = tints[idCounter % 8];
            ProductItem pi(idCounter, rawName, unit, category, iconKey, barcode, price, stock, tint);
            pi.categoryId = categoryId;
            pi.tax        = tax;
            pi.discount   = discount;
            if (!barcodePath.empty()) pi.barcodePath = barcodePath;
            mProducts.push_back(pi);
            idCounter++;
        }
    }
    in.close();

    // Fallback defaults if the file was empty / unreadable
    if (mProducts.empty()) {
        mProducts = {
            {0, "Atelier coffee",   "250 g",  "Drinks",    "coffee", "030003", 180, 32, sf::Color(232, 236, 230)},
            {1, "Basmati rice",     "1 kg",   "Grocery",   "rice",   "090006", 540, 50, sf::Color(232, 236, 238)},
            {2, "Fresh Milk",       "1 L",    "Dairy",     "milk",   "010001", 320, 24, sf::Color(240, 230, 222)},
            {3, "Lavender Deterg.", "1.5 L",  "Household", "milk",   "050008", 480, 12, sf::Color(230, 234, 240)},
            {4, "Orange Juice",     "1 L",    "Drinks",    "juice",  "030005", 290, 18, sf::Color(243, 236, 218)},
            {5, "Pistachio Crunch", "150 g",  "Snacks",    "coffee", "040007", 350, 20, sf::Color(240, 236, 220)},
            {6, "Red apples",       "1 kg",   "Fruits",    "apple",  "060004", 240, 40, sf::Color(242, 228, 222)},
            {7, "Sourdough Bread",  "1 loaf", "Bakery",    "bread",  "020002", 180, 15, sf::Color(241, 236, 214)}
        };
        for (auto& p : mProducts) p.categoryId = p.barcode.substr(0, 2);
    }

    sortAlphabetically();
    return true;
}

// ============================================================================
// sortAlphabetically — also renumbers product IDs (101, 102, ...) to match
// the on-disk convention. categoryId is preserved per-item.
// ============================================================================
void ProductCatalog::sortAlphabetically() {
    std::sort(mProducts.begin(), mProducts.end(),
              [](const ProductItem& a, const ProductItem& b) {
                  return toLowerStr(a.name) < toLowerStr(b.name);
              });
    for (size_t i = 0; i < mProducts.size(); ++i) {
        mProducts[i].id = std::to_string(101 + (int)i);
    }
}

// ============================================================================
// getVisibleProducts — search + category filter (unchanged from D2)
// ============================================================================
std::vector<int> ProductCatalog::getVisibleProducts(const std::string& searchQuery,
                                                    const std::string& activeCategory) const {
    std::vector<int> result;
    std::string q = toLowerStr(trimStr(searchQuery));
    std::string actLower = toLowerStr(trimStr(activeCategory));

    auto categoryMatches = [&](const std::string& prodCat) -> bool {
        if (actLower.empty() || actLower == "all") return true;
        std::string pc = toLowerStr(trimStr(prodCat));
        if (pc == actLower) return true;
        if ((actLower == "fruits" && pc.find("fruit") != std::string::npos) ||
            (actLower == "vegetables" && pc.find("veg") != std::string::npos) ||
            (actLower == "personal" && pc.find("personal") != std::string::npos) ||
            (actLower == "fruits & veg" && (pc == "fruits" || pc == "vegetables")) ||
            (actLower == "personal care" && pc == "personal")) {
            return true;
        }
        return false;
    };

    for (int i = 0; i < (int)mProducts.size(); i++) {
        const auto& p = mProducts[i];
        if (q.empty() && !categoryMatches(p.category)) {
            continue;
        }
        if (!q.empty()) {
            std::string nameLower = toLowerStr(p.name);
            std::string bcLower   = toLowerStr(p.barcode);
            std::string acrLower  = toLowerStr(p.acronym);
            std::string catLower  = toLowerStr(p.category);

            std::string nameUnderscore = nameLower;
            for (char& c : nameUnderscore) if (c == ' ') c = '_';

            std::string qUnderscore = q;
            for (char& c : qUnderscore) if (c == ' ') c = '_';

            if (nameLower.find(q) == std::string::npos &&
                nameUnderscore.find(qUnderscore) == std::string::npos &&
                bcLower.find(q) == std::string::npos &&
                acrLower.find(q) == std::string::npos &&
                catLower.find(q) == std::string::npos) {
                continue;
            }
        }
        result.push_back(i);
    }
    return result;
}

// ============================================================================
// Lookups
// ============================================================================
int ProductCatalog::findProductByBarcode(const std::string& barcode) const {
    std::string bTrim = trimStr(barcode);
    for (int i = 0; i < (int)mProducts.size(); i++) {
        if (mProducts[i].barcode == bTrim) return i;
    }
    return -1;
}

int ProductCatalog::findProductByName(const std::string& name) const {
    std::string q = toLowerStr(trimStr(name));
    if (q.empty()) return -1;

    std::string qUnderscore = q;
    for (char& c : qUnderscore) if (c == ' ') c = '_';

    // 1. Exact match
    for (int i = 0; i < (int)mProducts.size(); i++) {
        std::string nLower = toLowerStr(mProducts[i].name);
        std::string nUnderscore = nLower;
        for (char& c : nUnderscore) if (c == ' ') c = '_';
        if (nLower == q || nUnderscore == qUnderscore) return i;
    }

    // 2. Substring match
    for (int i = 0; i < (int)mProducts.size(); i++) {
        std::string nLower = toLowerStr(mProducts[i].name);
        std::string nUnderscore = nLower;
        for (char& c : nUnderscore) if (c == ' ') c = '_';
        if (nLower.find(q) != std::string::npos ||
            nUnderscore.find(qUnderscore) != std::string::npos) {
            return i;
        }
    }
    return -1;
}

// ============================================================================
// Mutations
// ============================================================================
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
        mProducts[idx].stock     += item.stock;
        mProducts[idx].price      = item.price;
        mProducts[idx].unitPrice  = item.price;
        mProducts[idx].tax        = item.tax;
        mProducts[idx].discount   = item.discount;
        mProducts[idx].name       = item.name;
        mProducts[idx].category   = item.category;
        mProducts[idx].unit       = item.unit;
        if (!item.acronym.empty()) mProducts[idx].acronym = item.acronym;
        if (!item.barcodePath.empty()) mProducts[idx].barcodePath = item.barcodePath;
        if (!item.categoryId.empty()) mProducts[idx].categoryId = item.categoryId;
    } else {
        ProductItem newItem = item;
        newItem.id = std::to_string(100 + (int)mProducts.size() + 1);
        mProducts.push_back(newItem);
    }
    sortAlphabetically();
    saveProductsFile();
}

void ProductCatalog::updateProduct(int index, const ProductItem& item) {
    if (index < 0 || index >= (int)mProducts.size()) return;
    mProducts[index].name       = item.name;
    mProducts[index].price      = item.price;
    mProducts[index].unitPrice  = item.price;
    mProducts[index].tax        = item.tax;
    mProducts[index].discount   = std::max(0, item.discount);
    mProducts[index].stock      = std::max(0, item.stock);
    mProducts[index].unit       = item.unit;
    mProducts[index].category   = item.category;
    if (!item.categoryId.empty()) mProducts[index].categoryId = item.categoryId;
    if (!item.acronym.empty()) mProducts[index].acronym = item.acronym;
    if (!item.barcodePath.empty()) mProducts[index].barcodePath = item.barcodePath;
    sortAlphabetically();
    saveProductsFile();
}

// ============================================================================
// saveProductsFile — D3: writes 11 fields per line
//   id barcode acronym name unit price tax discount stock barcodePath categoryId #
// Also mirrors to product.txt / frontend/product.txt if they exist.
// ============================================================================
bool ProductCatalog::saveProductsFile(const std::string& path) {
    std::string targetPath = path.empty() ? mProductFilePath : path;
    if (targetPath.empty()) targetPath = "product.txt";

    auto writeAll = [&](std::ostream& os) {
        for (size_t i = 0; i < mProducts.size(); ++i) {
            const auto& p = mProducts[i];

            std::string sName = p.name;
            for (char& c : sName) if (c == ' ') c = '_';

            std::string sUnit = p.unit.empty() ? "count" : p.unit;
            for (char& c : sUnit) if (c == ' ') c = '_';

            std::string acr = p.acronym.empty()
                            ? p.name.substr(0, std::min<size_t>(8, p.name.length()))
                            : p.acronym;
            for (char& c : acr) if (c == ' ') c = '_';
            if (acr.length() > 8) acr = acr.substr(0, 8);

            std::string bcPath = p.barcodePath.empty()
                               ? ("barcodes/barcode_" + p.barcode + ".bmp")
                               : p.barcodePath;

            int taxVal = p.tax;
            os << (101 + (int)i) << " "
               << p.barcode << " "
               << acr << " "
               << sName << " "
               << sUnit << " "
               << p.price << " "
               << taxVal << " "
               << std::max(0, p.discount) << " "
               << std::max(0, p.stock) << " "
               << bcPath << " "
               << (p.categoryId.empty() ? "01" : p.categoryId) << " #\n";
        }
    };

    std::ofstream out(targetPath);
    if (!out.is_open()) return false;
    writeAll(out);
    out.close();

    // Mirror writes
    std::vector<std::string> mirrors = {"product.txt", "frontend/product.txt"};
    for (const auto& m : mirrors) {
        if (m != targetPath && fs::exists(fs::path(m).parent_path().empty() ? fs::path(".") : fs::path(m).parent_path())) {
            std::ofstream mOut(m);
            if (mOut.is_open()) {
                writeAll(mOut);
                mOut.close();
            }
        }
    }
    return true;
}

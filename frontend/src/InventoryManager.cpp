#include "InventoryManager.h"
#include "UIUtils.h"
#include "../../backend/include/Generator.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <ctime>

namespace fs = std::filesystem;

InventoryManager::InventoryManager() {
    loadInventory();
}

bool InventoryManager::createDefaultInventoryFile(const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << "# ==============================================================================\n"
        << "# ScanIT Inventory & Supplier Procurement Database\n"
        << "# ==============================================================================\n"
        << "# [INVENTORY_ITEMS]\n"
        << "# Format: Barcode,Description,Category,Unit,RetailPrice,CostPrice,Stock,Supplier,ArrivalDate,BarcodePath\n"
        << "010001,Fresh Milk,Dairy,1 L,320,250,24,Nestle Pakistan,2026-11-15,barcodes/barcode_010001.bmp\n"
        << "020002,Sourdough Bread,Bakery,1 loaf,180,130,15,Artisan Bakes,2026-11-16,barcodes/barcode_020002.bmp\n"
        << "030003,Atelier coffee,Drinks,250 g,180,120,32,Espresso Labs,2026-11-14,barcodes/barcode_030003.bmp\n"
        << "060004,Red apples,Fruits,1 kg,240,170,40,Fresh Orchards,2026-11-17,barcodes/barcode_060004.bmp\n"
        << "030005,Orange Juice,Drinks,1 L,290,210,18,Citrus Farms,2026-11-15,barcodes/barcode_030005.bmp\n"
        << "090006,Basmati rice,Grocery,1 kg,540,420,50,Punjab Grains,2026-11-12,barcodes/barcode_090006.bmp\n"
        << "040007,Pistachio Crunch,Snacks,150 g,350,260,20,NutriDelight,2026-11-16,barcodes/barcode_040007.bmp\n"
        << "050008,Lavender Deterg.,Household,1.5 L,480,360,12,CleanHome Co,2026-11-13,barcodes/barcode_050008.bmp\n"
        << "090007,Cereal Box,Grocery,500 g,420,310,25,Morning Foods,2026-11-10,barcodes/barcode_090007.bmp\n"
        << "030006,Green Tea,Drinks,100 g,260,190,30,Himalayan Herbs,2026-11-11,barcodes/barcode_030006.bmp\n"
        << "\n"
        << "# [SUPPLIER_PURCHASE_HISTORY]\n"
        << "# Format: Date | Supplier | Barcode | Description | QtyPurchased | UnitCostPrice | TotalCost\n"
        << "2026-11-15 | Nestle Pakistan | 010001 | Fresh Milk | 50 | 250 | 12500\n"
        << "2026-11-16 | Artisan Bakes | 020002 | Sourdough Bread | 30 | 130 | 3900\n"
        << "2026-11-14 | Espresso Labs | 030003 | Atelier coffee | 40 | 120 | 4800\n"
        << "2026-11-17 | Fresh Orchards | 060004 | Red apples | 60 | 170 | 10200\n"
        << "2026-11-15 | Citrus Farms | 030005 | Orange Juice | 30 | 210 | 6300\n"
        << "2026-11-12 | Punjab Grains | 090006 | Basmati rice | 80 | 420 | 33600\n"
        << "2026-11-16 | NutriDelight | 040007 | Pistachio Crunch | 25 | 260 | 6500\n"
        << "2026-11-13 | CleanHome Co | 050008 | Lavender Deterg. | 20 | 360 | 7200\n"
        << "2026-11-10 | Morning Foods | 090007 | Cereal Box | 35 | 310 | 10850\n"
        << "2026-11-11 | Himalayan Herbs | 030006 | Green Tea | 40 | 190 | 7600\n";
    out.close();
    return true;
}

bool InventoryManager::loadInventory(const std::string& customPath) {
    if (!customPath.empty() && fs::exists(customPath)) {
        mFilePath = customPath;
    } else {
        std::string found = resolveDataFilePath("inventory.txt");
        if (!fs::exists(found)) {
            createDefaultInventoryFile(found);
        }
        mFilePath = found;
    }

    std::ifstream in(mFilePath);
    if (!in.is_open()) return false;

    mItems.clear();
    mPurchases.clear();

    std::string line;
    enum Section { None, Items, Purchases } currentSection = None;

    while (std::getline(in, line)) {
        line = trimStr(line);
        if (line.empty()) continue;

        if (line.find("[INVENTORY_ITEMS]") != std::string::npos) {
            currentSection = Items;
            continue;
        } else if (line.find("[SUPPLIER_PURCHASE_HISTORY]") != std::string::npos) {
            currentSection = Purchases;
            continue;
        }

        if (line[0] == '#') continue;

        if (currentSection == Items) {
            std::stringstream ss(line);
            std::string bc, desc, cat, unit, retP, costP, stk, supp, arrDate, bcPath, expDate;

            std::getline(ss, bc, ',');
            std::getline(ss, desc, ',');
            std::getline(ss, cat, ',');
            std::getline(ss, unit, ',');
            std::getline(ss, retP, ',');
            std::getline(ss, costP, ',');
            std::getline(ss, stk, ',');
            std::getline(ss, supp, ',');
            std::getline(ss, arrDate, ',');
            std::getline(ss, bcPath, ',');
            std::getline(ss, expDate, ',');

            bc = trimStr(bc);
            desc = trimStr(desc);
            if (bc.empty() || desc.empty()) continue;

            InventoryItem item;
            item.barcode = bc;
            item.name = desc;
            item.category = trimStr(cat);
            item.unit = trimStr(unit);
            try { item.retailPrice = std::stoi(trimStr(retP)); } catch (...) { item.retailPrice = 0; }
            try { item.costPrice = std::stoi(trimStr(costP)); } catch (...) { item.costPrice = 0; }
            try { item.stock = std::stoi(trimStr(stk)); } catch (...) { item.stock = 0; }
            item.supplier = trimStr(supp);
            item.arrivalDate = trimStr(arrDate);
            item.barcodePath = trimStr(bcPath);
            if (item.barcodePath.empty()) {
                item.barcodePath = "barcodes/barcode_" + bc + ".bmp";
            }
            item.expiryDate = trimStr(expDate);

            mItems.push_back(item);
        } else if (currentSection == Purchases) {
            std::stringstream ss(line);
            std::string d, supp, bc, desc, qStr, cStr, totStr;

            std::getline(ss, d, '|');
            std::getline(ss, supp, '|');
            std::getline(ss, bc, '|');
            std::getline(ss, desc, '|');
            std::getline(ss, qStr, '|');
            std::getline(ss, cStr, '|');
            std::getline(ss, totStr, '|');

            d = trimStr(d);
            supp = trimStr(supp);
            bc = trimStr(bc);
            if (d.empty() || supp.empty() || bc.empty()) continue;

            SupplierPurchaseRecord rec;
            rec.date = d;
            rec.supplier = supp;
            rec.barcode = bc;
            rec.description = trimStr(desc);
            try { rec.quantity = std::stoi(trimStr(qStr)); } catch (...) { rec.quantity = 0; }
            try { rec.unitCost = std::stoi(trimStr(cStr)); } catch (...) { rec.unitCost = 0; }
            try { rec.totalCost = std::stoi(trimStr(totStr)); } catch (...) { rec.totalCost = rec.quantity * rec.unitCost; }

            mPurchases.push_back(rec);
        }
    }
    in.close();
    sortAlphabetically();
    return true;
}

bool InventoryManager::saveInventory(const std::string& customPath) {
    std::string path = customPath.empty() ? mFilePath : customPath;
    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << "# ==============================================================================\n"
        << "# ScanIT Inventory & Supplier Procurement Database\n"
        << "# ==============================================================================\n"
        << "# [INVENTORY_ITEMS]\n"
        << "# Format: Barcode,Description,Category,Unit,RetailPrice,CostPrice,Stock,Supplier,ArrivalDate,BarcodePath,ExpiryDate\n";

    for (const auto& it : mItems) {
        out << it.barcode << ","
            << it.name << ","
            << it.category << ","
            << it.unit << ","
            << it.retailPrice << ","
            << it.costPrice << ","
            << it.stock << ","
            << it.supplier << ","
            << it.arrivalDate << ","
            << it.barcodePath << ","
            << it.expiryDate << "\n";
    }

    out << "\n# [SUPPLIER_PURCHASE_HISTORY]\n"
        << "# Format: Date | Supplier | Barcode | Description | QtyPurchased | UnitCostPrice | TotalCost\n";

    for (const auto& p : mPurchases) {
        out << p.date << " | "
            << p.supplier << " | "
            << p.barcode << " | "
            << p.description << " | "
            << p.quantity << " | "
            << p.unitCost << " | "
            << p.totalCost << "\n";
    }

    out.close();

    // Mirror to secondary path if present
    std::string mirror = (path == "inventory.txt") ? "frontend/inventory.txt" : "inventory.txt";
    if (fs::exists(fs::path(mirror).parent_path())) {
        std::ofstream mOut(mirror);
        if (mOut.is_open()) {
            mOut << "# ==============================================================================\n"
                 << "# ScanIT Inventory & Supplier Procurement Database\n"
                 << "# ==============================================================================\n"
                 << "# [INVENTORY_ITEMS]\n";
            for (const auto& it : mItems) {
                mOut << it.barcode << "," << it.name << "," << it.category << "," << it.unit << ","
                     << it.retailPrice << "," << it.costPrice << "," << it.stock << ","
                     << it.supplier << "," << it.arrivalDate << "," << it.barcodePath << ","
                     << it.expiryDate << "\n";
            }
            mOut << "\n# [SUPPLIER_PURCHASE_HISTORY]\n";
            for (const auto& p : mPurchases) {
                mOut << p.date << " | " << p.supplier << " | " << p.barcode << " | "
                     << p.description << " | " << p.quantity << " | " << p.unitCost << " | "
                     << p.totalCost << "\n";
            }
            mOut.close();
        }
    }

    return true;
}

bool InventoryManager::addOrRestockItem(const InventoryItem& item, int purchaseQty, int unitCost,
                                        const std::string& supplier, const std::string& date) {
    int idx = findIndexByBarcode(item.barcode);
    if (idx >= 0) {
        mItems[idx].stock += purchaseQty;
        mItems[idx].retailPrice = item.retailPrice;
        mItems[idx].costPrice = unitCost;
        mItems[idx].supplier = supplier;
        mItems[idx].arrivalDate = date;
        if (!item.expiryDate.empty()) mItems[idx].expiryDate = item.expiryDate;
    } else {
        InventoryItem newItem = item;
        newItem.stock = purchaseQty;
        newItem.costPrice = unitCost;
        newItem.supplier = supplier;
        newItem.arrivalDate = date;
        mItems.push_back(newItem);
    }

    SupplierPurchaseRecord rec;
    rec.date = date;
    rec.supplier = supplier;
    rec.barcode = item.barcode;
    rec.description = item.name;
    rec.quantity = purchaseQty;
    rec.unitCost = unitCost;
    rec.totalCost = purchaseQty * unitCost;
    mPurchases.insert(mPurchases.begin(), rec); // Most recent first

    sortAlphabetically();
    return saveInventory();
}

bool InventoryManager::restockItem(const std::string& barcode, int qty) {
    int idx = findIndexByBarcode(barcode);
    if (idx >= 0) {
        mItems[idx].stock += qty;
        return saveInventory();
    }
    return false;
}

bool InventoryManager::deductStock(const std::string& barcode, int qty) {
    int idx = findIndexByBarcode(barcode);
    if (idx >= 0) {
        mItems[idx].stock = std::max(0, mItems[idx].stock - qty);
        return saveInventory();
    }
    return false;
}

bool InventoryManager::updateItemStock(const std::string& barcode, int newStock) {
    int idx = findIndexByBarcode(barcode);
    if (idx >= 0) {
        mItems[idx].stock = std::max(0, newStock);
        return saveInventory();
    }
    return false;
}

bool InventoryManager::updateItemFromProduct(const ProductItem& p) {
    int idx = findIndexByBarcode(p.barcode);
    if (idx >= 0) {
        mItems[idx].name = p.name;
        mItems[idx].category = p.category;
        mItems[idx].unit = p.unit;
        mItems[idx].retailPrice = p.price;
        mItems[idx].stock = std::max(0, p.stock);
        sortAlphabetically();
        return saveInventory();
    }
    return false;
}

std::string InventoryManager::generateUniqueBarcode(const std::string& categoryId,
                                                    const std::vector<std::string>& extraUsedBarcodes) const {
    // Requirement 1: The first two digits of the barcode shall represent the product category.
    std::string catPrefix = categoryId;
    if (catPrefix.length() < 2) catPrefix = "0" + catPrefix;
    if (catPrefix.length() > 2) catPrefix = catPrefix.substr(0, 2);
    if (catPrefix.empty()) catPrefix = "01";

    // 6-digit barcode: 2 digits category + 4 digits sequence (0001 to 9999)
    int seq = 1;
    while (seq <= 9999) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%s%04d", catPrefix.c_str(), seq);
        std::string candStr = buf;
        bool used = false;
        for (const auto& it : mItems) {
            if (it.barcode == candStr) {
                used = true;
                break;
            }
        }
        if (!used) {
            for (const auto& eb : extraUsedBarcodes) {
                if (eb == candStr) {
                    used = true;
                    break;
                }
            }
        }
        if (!used) {
            return candStr;
        }
        seq++;
    }
    return catPrefix + "9999";
}

const InventoryItem* InventoryManager::findByBarcode(const std::string& barcode) const {
    for (const auto& it : mItems) {
        if (it.barcode == barcode) return &it;
    }
    return nullptr;
}

int InventoryManager::findIndexByBarcode(const std::string& barcode) const {
    for (size_t i = 0; i < mItems.size(); ++i) {
        if (mItems[i].barcode == barcode) return static_cast<int>(i);
    }
    return -1;
}

void InventoryManager::sortAlphabetically() {
    std::sort(mItems.begin(), mItems.end(), [](const InventoryItem& a, const InventoryItem& b) {
        return toLowerStr(a.name) < toLowerStr(b.name);
    });
}

std::vector<std::string> InventoryManager::getUniqueSuppliers() const {
    std::vector<std::string> result;
    for (const auto& it : mItems) {
        if (!it.supplier.empty() && std::find(result.begin(), result.end(), it.supplier) == result.end()) {
            result.push_back(it.supplier);
        }
    }
    for (const auto& p : mPurchases) {
        if (!p.supplier.empty() && std::find(result.begin(), result.end(), p.supplier) == result.end()) {
            result.push_back(p.supplier);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<int> InventoryManager::getFilteredItemIndices(const std::string& query) const {
    std::vector<int> res;
    std::string q = toLowerStr(trimStr(query));
    for (size_t i = 0; i < mItems.size(); ++i) {
        if (q.empty()) {
            res.push_back((int)i);
        } else {
            const auto& it = mItems[i];
            if (toLowerStr(it.name).find(q) != std::string::npos ||
                it.barcode.find(q) != std::string::npos ||
                toLowerStr(it.category).find(q) != std::string::npos ||
                toLowerStr(it.supplier).find(q) != std::string::npos) {
                res.push_back((int)i);
            }
        }
    }
    return res;
}

std::vector<int> InventoryManager::getFilteredPurchaseIndices(const std::string& query) const {
    std::vector<int> res;
    std::string q = toLowerStr(trimStr(query));
    for (size_t i = 0; i < mPurchases.size(); ++i) {
        if (q.empty()) {
            res.push_back((int)i);
        } else {
            const auto& p = mPurchases[i];
            if (toLowerStr(p.supplier).find(q) != std::string::npos ||
                toLowerStr(p.description).find(q) != std::string::npos ||
                p.barcode.find(q) != std::string::npos ||
                p.date.find(q) != std::string::npos) {
                res.push_back((int)i);
            }
        }
    }
    return res;
}

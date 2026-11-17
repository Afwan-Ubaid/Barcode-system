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
        << "100001,Fresh Milk,Grocery,1 L,320,250,24,Nestle Pakistan,2026-11-15,barcodes/barcode_100001.bmp\n"
        << "100002,Sourdough Bread,Grocery,1 loaf,180,130,15,Artisan Bakes,2026-11-16,barcodes/barcode_100002.bmp\n"
        << "100003,Atelier coffee,Drinks,250 g,180,120,32,Espresso Labs,2026-11-14,barcodes/barcode_100003.bmp\n"
        << "100004,Red apples,Grocery,1 kg,240,170,40,Fresh Orchards,2026-11-17,barcodes/barcode_100004.bmp\n"
        << "100005,Orange Juice,Drinks,1 L,290,210,18,Citrus Farms,2026-11-15,barcodes/barcode_100005.bmp\n"
        << "100006,Basmati rice,Grocery,1 kg,540,420,50,Punjab Grains,2026-11-12,barcodes/barcode_100006.bmp\n"
        << "100007,Pistachio Crunch,Snacks,150 g,350,260,20,NutriDelight,2026-11-16,barcodes/barcode_100007.bmp\n"
        << "100008,Lavender Deterg.,Household,1.5 L,480,360,12,CleanHome Co,2026-11-13,barcodes/barcode_100008.bmp\n"
        << "234567,Cereal Box,Grocery,500 g,420,310,25,Morning Foods,2026-11-10,barcodes/barcode_234567.bmp\n"
        << "567890,Green Tea,Drinks,100 g,260,190,30,Himalayan Herbs,2026-11-11,barcodes/barcode_567890.bmp\n"
        << "\n"
        << "# [SUPPLIER_PURCHASE_HISTORY]\n"
        << "# Format: Date | Supplier | Barcode | Description | QtyPurchased | UnitCostPrice | TotalCost\n"
        << "2026-11-15 | Nestle Pakistan | 100001 | Fresh Milk | 50 | 250 | 12500\n"
        << "2026-11-16 | Artisan Bakes | 100002 | Sourdough Bread | 30 | 130 | 3900\n"
        << "2026-11-14 | Espresso Labs | 100003 | Atelier coffee | 40 | 120 | 4800\n"
        << "2026-11-17 | Fresh Orchards | 100004 | Red apples | 60 | 170 | 10200\n"
        << "2026-11-15 | Citrus Farms | 100005 | Orange Juice | 30 | 210 | 6300\n"
        << "2026-11-12 | Punjab Grains | 100006 | Basmati rice | 80 | 420 | 33600\n"
        << "2026-11-16 | NutriDelight | 100007 | Pistachio Crunch | 25 | 260 | 6500\n"
        << "2026-11-13 | CleanHome Co | 100008 | Lavender Deterg. | 20 | 360 | 7200\n"
        << "2026-11-10 | Morning Foods | 234567 | Cereal Box | 35 | 310 | 10850\n"
        << "2026-11-11 | Himalayan Herbs | 567890 | Green Tea | 40 | 190 | 7600\n";
    out.close();
    return true;
}

bool InventoryManager::loadInventory(const std::string& customPath) {
    if (!customPath.empty() && fs::exists(customPath)) {
        mFilePath = customPath;
    } else {
        std::vector<std::string> candidates = {
            "inventory.txt",
            "frontend/inventory.txt",
            "../inventory.txt",
            "../../inventory.txt"
        };

        std::string found;
        for (const auto& c : candidates) {
            if (fs::exists(c)) {
                found = c;
                break;
            }
        }
        if (found.empty()) {
            createDefaultInventoryFile("inventory.txt");
            found = "inventory.txt";
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
            std::string bc, desc, cat, unit, retP, costP, stk, supp, arrDate, bcPath;

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
        << "# Format: Barcode,Description,Category,Unit,RetailPrice,CostPrice,Stock,Supplier,ArrivalDate,BarcodePath\n";

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
            << it.barcodePath << "\n";
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
                     << it.supplier << "," << it.arrivalDate << "," << it.barcodePath << "\n";
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

std::string InventoryManager::generateUniqueBarcode() const {
    // Generate sequential 6-digit numeric barcode starting at 100011
    int candidate = 100001;
    while (true) {
        std::string candStr = std::to_string(candidate);
        bool used = false;
        for (const auto& it : mItems) {
            if (it.barcode == candStr) {
                used = true;
                break;
            }
        }
        if (!used) {
            return candStr;
        }
        candidate++;
    }
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

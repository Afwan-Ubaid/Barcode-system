#ifndef INVENTORY_MANAGER_H
#define INVENTORY_MANAGER_H

#include <string>
#include <vector>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Single Responsibility Principle (SRP)
// Module: Inventory Management, Supplier Procurement Ledger & Barcode Auto-Generation
// ============================================================================

struct InventoryItem {
    std::string barcode;       // 6-digit Code 39 barcode
    std::string name;          // Product description / item name
    std::string category = "Grocery";
    std::string unit = "1 pc";
    int retailPrice = 0;       // Selling price at POS (PKR)
    int costPrice = 0;         // Purchase price from supplier (PKR)
    int stock = 0;             // Available quantity
    std::string supplier;      // Supplier / Vendor name
    std::string arrivalDate;   // Date item was received (YYYY-MM-DD)
    std::string barcodePath;   // Path to generated/existing BMP barcode
};

struct SupplierPurchaseRecord {
    std::string date;          // Transaction date (YYYY-MM-DD)
    std::string supplier;      // Supplier name
    std::string barcode;       // Barcode of item
    std::string description;   // Description of item
    int quantity = 0;          // Units purchased
    int unitCost = 0;          // Cost per unit (PKR)
    int totalCost = 0;         // Total invoice cost (PKR)
};

class InventoryManager {
public:
    InventoryManager();

    bool loadInventory(const std::string& customPath = "");
    bool saveInventory(const std::string& customPath = "");

    const std::vector<InventoryItem>& getItems() const { return mItems; }
    std::vector<InventoryItem>& getItems() { return mItems; }

    const std::vector<SupplierPurchaseRecord>& getPurchases() const { return mPurchases; }
    std::vector<SupplierPurchaseRecord>& getPurchases() { return mPurchases; }

    const std::string& getFilePath() const { return mFilePath; }

    // Adds a new inventory item or restocks an existing one, logging the supplier purchase
    bool addOrRestockItem(const InventoryItem& item, int purchaseQty, int unitCost,
                          const std::string& supplier, const std::string& date);

    // Restocks an existing item (e.g. from customer return or quick restock)
    bool restockItem(const std::string& barcode, int qty);

    // Decrements stock upon sale
    bool deductStock(const std::string& barcode, int qty);
    bool updateItemStock(const std::string& barcode, int newStock);
    bool updateItemFromProduct(const ProductItem& p);

    // Generates a unique, unused 6-digit barcode where first 2 digits are Category ID (Requirement 1)
    std::string generateUniqueBarcode(const std::string& categoryId = "01",
                                      const std::vector<std::string>& extraUsedBarcodes = {}) const;

    const InventoryItem* findByBarcode(const std::string& barcode) const;
    int findIndexByBarcode(const std::string& barcode) const;

    void sortAlphabetically();
    std::vector<std::string> getUniqueSuppliers() const;
    std::vector<int> getFilteredItemIndices(const std::string& query) const;
    std::vector<int> getFilteredPurchaseIndices(const std::string& query) const;

private:
    bool createDefaultInventoryFile(const std::string& path);

    std::vector<InventoryItem> mItems;
    std::vector<SupplierPurchaseRecord> mPurchases;
    std::string mFilePath = "inventory.txt";
};

#endif // INVENTORY_MANAGER_H

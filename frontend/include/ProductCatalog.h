#ifndef PRODUCT_CATALOG_H
#define PRODUCT_CATALOG_H

#include <string>
#include <vector>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Single Responsibility Principle (SRP)
// Module: Product Catalog, Stock Inventory Tracking & Persistence
// ============================================================================

class ProductCatalog {
public:
    ProductCatalog();

    bool loadProductsFile(const std::string& customPath = "");
    bool saveDefaultProductFile(const std::string& path);

    const std::vector<ProductItem>& getProducts() const { return mProducts; }
    std::vector<ProductItem>& getProducts() { return mProducts; }
    const std::string& getProductFilePath() const { return mProductFilePath; }

    std::vector<int> getVisibleProducts(const std::string& searchQuery, const std::string& activeCategory) const;
    int findProductByBarcode(const std::string& barcode) const;

    void deleteProduct(int index);
    void addStock(int index, int amount);

private:
    std::vector<ProductItem> mProducts;
    std::string mProductFilePath;
};

#endif // PRODUCT_CATALOG_H

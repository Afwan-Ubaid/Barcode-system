#ifndef CATEGORY_MANAGER_H
#define CATEGORY_MANAGER_H

#include <string>
#include <vector>
#include "PosModels.h"

// ============================================================================
// D3 Req 1: PCategory.txt persistence
// Handles loading, saving, and querying categories.
// ============================================================================
class CategoryManager {
public:
    CategoryManager();

    bool loadCategories(const std::string& path = "PCategory.txt");
    bool saveCategories(const std::string& path = "PCategory.txt") const;

    const std::vector<ProductCategory>& getCategories() const { return mCategories; }
    std::vector<ProductCategory>& getCategories() { return mCategories; }

    bool addCategory(const ProductCategory& cat);
    bool updateCategory(const ProductCategory& cat);
    bool deleteCategory(const std::string& id);
    const ProductCategory* findById(const std::string& id) const;
    const ProductCategory* findByName(const std::string& name) const;
    std::string nextCategoryId() const;  // "01" -> "02"

private:
    std::vector<ProductCategory> mCategories;
};

#endif // CATEGORY_MANAGER_H

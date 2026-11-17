#include "ProductManager.h"
#include "Generator.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstdio>
#include <iomanip>

namespace fs = std::filesystem;

static std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

static std::vector<std::string> splitSpaces(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

static std::string sanitizeName(std::string name) {
    // Replace spaces with underscores for space-separated file storage
    for (char& c : name) {
        if (c == ' ') c = '_';
    }
    return name;
}

static std::string desanitizeName(std::string name) {
    // Replace underscores with spaces for human display
    for (char& c : name) {
        if (c == '_') c = ' ';
    }
    return name;
}

std::string findDataFilePath(const std::string& filename) {
    std::vector<std::string> candidates = {
        filename,
        "frontend/" + filename,
        "backend/" + filename,
        "assets/" + filename,
        "../" + filename,
        "../../" + filename
    };

    for (const auto& c : candidates) {
        if (fs::exists(c)) {
            return c;
        }
    }
    return filename;
}

static void determineCategoryAndIcon(ProductItem& p) {
    std::string lowerName = p.name;
    for (char& c : lowerName) c = (char)std::tolower((unsigned char)c);

    if (lowerName.find("milk") != std::string::npos) {
        p.category = "Grocery";
        p.iconKey = "milk";
    } else if (lowerName.find("bread") != std::string::npos || lowerName.find("sourdough") != std::string::npos) {
        p.category = "Grocery";
        p.iconKey = "bread";
    } else if (lowerName.find("coffee") != std::string::npos) {
        p.category = "Drinks";
        p.iconKey = "coffee";
    } else if (lowerName.find("tea") != std::string::npos) {
        p.category = "Drinks";
        p.iconKey = "juice";
    } else if (lowerName.find("apple") != std::string::npos || lowerName.find("fruit") != std::string::npos) {
        p.category = "Grocery";
        p.iconKey = "apple";
    } else if (lowerName.find("juice") != std::string::npos) {
        p.category = "Drinks";
        p.iconKey = "juice";
    } else if (lowerName.find("rice") != std::string::npos) {
        p.category = "Grocery";
        p.iconKey = "rice";
    } else if (lowerName.find("crunch") != std::string::npos || lowerName.find("snack") != std::string::npos || lowerName.find("pistachio") != std::string::npos) {
        p.category = "Snacks";
        p.iconKey = "coffee";
    } else if (lowerName.find("cereal") != std::string::npos) {
        p.category = "Grocery";
        p.iconKey = "bread";
    } else if (lowerName.find("deterg") != std::string::npos || lowerName.find("clean") != std::string::npos || lowerName.find("soap") != std::string::npos) {
        p.category = "Household";
        p.iconKey = "milk";
    } else {
        p.category = (p.unit == "weight" ? "Grocery" : "Household");
        p.iconKey = "milk";
    }
}

bool loadProducts(const std::string& path, std::vector<ProductItem>& products) {
    std::string actualPath = findDataFilePath(path);
    std::ifstream in(actualPath);
    if (!in.is_open()) return false;

    products.clear();
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '/') continue;

        // Strip end-of-line marker # if present
        if (!line.empty() && line.back() == '#') {
            line.pop_back();
            line = trim(line);
        }

        auto tokens = splitSpaces(line);
        if (tokens.size() < 10) continue;

        ProductItem item;
        item.id = tokens[0];
        item.barcode = tokens[1];
        item.acronym = tokens[2];
        if (item.acronym.length() > 8) {
            item.acronym = item.acronym.substr(0, 8);
        }

        item.barcodePath = tokens.back();
        try { item.stock = std::stoi(tokens[tokens.size() - 2]); } catch (...) { item.stock = 0; }
        try { item.discount = std::stoi(tokens[tokens.size() - 3]); } catch (...) { item.discount = 0; }
        try { item.tax = std::stoi(tokens[tokens.size() - 4]); } catch (...) { item.tax = 0; }
        try { item.unitPrice = std::stoi(tokens[tokens.size() - 5]); } catch (...) { item.unitPrice = 0; }
        item.unit = tokens[tokens.size() - 6];

        // Join tokens between acronym and unit as the product name
        std::string name;
        for (size_t i = 3; i < tokens.size() - 6; i++) {
            if (!name.empty()) name += " ";
            name += tokens[i];
        }
        item.name = desanitizeName(name);

        determineCategoryAndIcon(item);
        products.push_back(item);
    }
    in.close();
    return !products.empty();
}

bool saveProducts(const std::string& path, const std::vector<ProductItem>& products) {
    std::string actualPath = findDataFilePath(path);
    std::ofstream out(actualPath);
    if (!out.is_open()) return false;

    for (const auto& p : products) {
        std::string acronym = p.acronym;
        if (acronym.length() > 8) acronym = acronym.substr(0, 8);
        std::string sName = sanitizeName(p.name);

        out << p.id << " "
            << p.barcode << " "
            << acronym << " "
            << sName << " "
            << p.unit << " "
            << p.unitPrice << " "
            << p.tax << " "
            << p.discount << " "
            << p.stock << " "
            << p.barcodePath << " #\n";
    }
    out.close();

    // Also mirror to secondary locations if they exist
    std::vector<std::string> mirrors = {"Product.txt", "product.txt", "frontend/Product.txt", "frontend/product.txt"};
    for (const auto& m : mirrors) {
        if (m != actualPath && fs::exists(m)) {
            std::ofstream mOut(m);
            if (mOut.is_open()) {
                for (const auto& p : products) {
                    std::string acronym = p.acronym.substr(0, std::min<size_t>(8, p.acronym.length()));
                    mOut << p.id << " "
                         << p.barcode << " "
                         << acronym << " "
                         << sanitizeName(p.name) << " "
                         << p.unit << " "
                         << p.unitPrice << " "
                         << p.tax << " "
                         << p.discount << " "
                         << p.stock << " "
                         << p.barcodePath << " #\n";
                }
                mOut.close();
            }
        }
    }
    return true;
}

bool loadOrders(const std::string& path, std::vector<OrderRecord>& orders) {
    std::string actualPath = findDataFilePath(path);
    std::ifstream in(actualPath);
    if (!in.is_open()) return false;

    orders.clear();
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (!line.empty() && line.back() == '#') {
            line.pop_back();
            line = trim(line);
        }

        auto tokens = splitSpaces(line);
        if (tokens.size() < 9) continue;

        OrderRecord rec;
        rec.orderId = tokens[0];
        rec.orderDate = tokens[1];
        rec.orderTime = tokens[2];
        rec.customerName = desanitizeName(tokens[3]);
        rec.customerContact = tokens[4];
        rec.orderStatus = tokens[5];
        try { rec.paymentAmount = std::stoi(tokens[6]); } catch (...) { rec.paymentAmount = 0; }
        rec.paymentMode = tokens[7];
        rec.paymentMerchant = tokens[8];

        orders.push_back(rec);
    }
    in.close();
    return true;
}

bool appendOrder(const std::string& path, const OrderRecord& order) {
    std::string actualPath = findDataFilePath(path);
    std::ofstream out(actualPath, std::ios::app);
    if (!out.is_open()) return false;

    out << order.orderId << " "
        << order.orderDate << " "
        << order.orderTime << " "
        << sanitizeName(order.customerName) << " "
        << (order.customerContact.empty() ? "None" : order.customerContact) << " "
        << order.orderStatus << " "
        << order.paymentAmount << " "
        << order.paymentMode << " "
        << order.paymentMerchant << " #\n";
    out.close();
    return true;
}

bool loadOrderDetails(const std::string& path, std::vector<OrderDetailRecord>& details) {
    std::string actualPath = findDataFilePath(path);
    std::ifstream in(actualPath);
    if (!in.is_open()) return false;

    details.clear();
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (!line.empty() && line.back() == '#') {
            line.pop_back();
            line = trim(line);
        }

        auto tokens = splitSpaces(line);
        if (tokens.size() < 4) continue;

        OrderDetailRecord d;
        d.orderId = tokens[0];
        d.productId = tokens[1];
        d.productBarcode = tokens[2];
        try { d.quantity = std::stoi(tokens[3]); } catch (...) { d.quantity = 0; }

        details.push_back(d);
    }
    in.close();
    return true;
}

bool appendOrderDetail(const std::string& path, const OrderDetailRecord& detail) {
    std::string actualPath = findDataFilePath(path);
    std::ofstream out(actualPath, std::ios::app);
    if (!out.is_open()) return false;

    out << detail.orderId << " "
        << detail.productId << " "
        << detail.productBarcode << " "
        << detail.quantity << " #\n";
    out.close();
    return true;
}

bool loadBills(const std::string& path, std::vector<BillRecord>& bills) {
    std::string actualPath = findDataFilePath(path);
    std::ifstream in(actualPath);
    if (!in.is_open()) return false;

    bills.clear();
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (!line.empty() && line.back() == '#') {
            line.pop_back();
            line = trim(line);
        }

        auto tokens = splitSpaces(line);
        if (tokens.size() < 12) continue;

        BillRecord b;
        b.billId = tokens[0];
        b.customerName = desanitizeName(tokens[1]);
        b.orderId = tokens[2];
        b.productAcronym = tokens[3];
        try { b.unitPrice = std::stoi(tokens[4]); } catch (...) { b.unitPrice = 0; }
        try { b.discountPerUnit = std::stoi(tokens[5]); } catch (...) { b.discountPerUnit = 0; }
        try { b.quantity = std::stoi(tokens[6]); } catch (...) { b.quantity = 0; }
        try { b.totalWithoutTax = std::stoi(tokens[7]); } catch (...) { b.totalWithoutTax = 0; }
        try { b.totalTax = std::stoi(tokens[8]); } catch (...) { b.totalTax = 0; }
        try { b.totalDiscount = std::stoi(tokens[9]); } catch (...) { b.totalDiscount = 0; }
        try { b.totalPayable = std::stoi(tokens[10]); } catch (...) { b.totalPayable = 0; }
        b.paymentStatus = tokens[11];

        bills.push_back(b);
    }
    in.close();
    return true;
}

bool appendBill(const std::string& path, const BillRecord& bill) {
    std::string actualPath = findDataFilePath(path);
    std::ofstream out(actualPath, std::ios::app);
    if (!out.is_open()) return false;

    out << bill.billId << " "
        << sanitizeName(bill.customerName) << " "
        << bill.orderId << " "
        << bill.productAcronym << " "
        << bill.unitPrice << " "
        << bill.discountPerUnit << " "
        << bill.quantity << " "
        << bill.totalWithoutTax << " "
        << bill.totalTax << " "
        << bill.totalDiscount << " "
        << bill.totalPayable << " "
        << bill.paymentStatus << " #\n";
    out.close();
    return true;
}

std::string generateNextOrderId(int dayOfMonth, int existingCount) {
    // 8-digit order ID where last 2 digits indicate date of order
    int prefix = 100000 + ((existingCount + 1) % 900000);
    int day = std::clamp(dayOfMonth, 1, 31);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%06d%02d", prefix, day);
    return std::string(buf);
}

std::string generateNextBillId(int existingCount) {
    // 9-digit bill ID
    long long base = 900000000LL + ((existingCount + 1) % 100000000LL);
    char buf[20];
    std::snprintf(buf, sizeof(buf), "%09lld", base);
    return std::string(buf);
}

const ProductItem* findProductByBarcode(const std::vector<ProductItem>& products, const std::string& barcode) {
    for (const auto& p : products) {
        if (p.barcode == barcode) return &p;
    }
    return nullptr;
}

const ProductItem* findProductById(const std::vector<ProductItem>& products, const std::string& id) {
    for (const auto& p : products) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

void ensureAllBarcodesExist(const std::vector<ProductItem>& products) {
    if (!fs::exists("barcodes")) {
        fs::create_directories("barcodes");
    }
    for (const auto& p : products) {
        if (p.barcode.length() == 6 && !fs::exists(p.barcodePath)) {
            generateBarcodeBMP(p.barcode, p.barcodePath);
        }
    }
}

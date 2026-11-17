#ifndef POS_MODELS_H
#define POS_MODELS_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <algorithm>

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Single Responsibility Principle (SRP)
// Module: Domain Models & Global Aesthetic Theme Tokens
// ============================================================================

namespace ThemeColor {
    const sf::Color Ink(62, 53, 47);
    const sf::Color Mute(140, 128, 116);
    const sf::Color Line(224, 214, 196);
    const sf::Color Edge(208, 198, 180);
    const sf::Color Paper(251, 248, 241);
    const sf::Color Panel(243, 236, 222);
    const sf::Color Sidebar(240, 233, 218);
    const sf::Color White(255, 255, 255);
    const sf::Color Dark(47, 42, 38);
    const sf::Color Blue(135, 164, 190);
    const sf::Color BlueHover(119, 148, 174);
    const sf::Color BlueDisabled(190, 206, 222);
    const sf::Color Green(110, 168, 110);
    const sf::Color Red(226, 70, 60);
    const sf::Color Amber(217, 119, 6);
    const sf::Color None(0, 0, 0, 0);
}

// ----------------------------------------------------------------------------
// Product Item Model
// ----------------------------------------------------------------------------
struct ProductItem {
    std::string id = "0";
    std::string barcode = "100001";
    std::string acronym = "MILK";
    std::string name = "Fresh Milk";
    std::string unit = "1 L";
    int price = 320;
    int unitPrice = 320;
    int tax = 0;
    int discount = 0;
    int stock = 24;
    std::string barcodePath = "barcodes/100001.bmp";
    std::string category = "Grocery";
    std::string iconKey = "milk";
    sf::Color cardTint = sf::Color(240, 230, 222);

    ProductItem() = default;

    ProductItem(int idNum, const std::string& n, const std::string& u, const std::string& cat,
                const std::string& icon, const std::string& bc, int p, int st, sf::Color tint)
        : id(std::to_string(idNum)), barcode(bc), acronym(n.substr(0, std::min<size_t>(4, n.length()))),
          name(n), unit(u), price(p), unitPrice(p), tax(0), discount(0), stock(st),
          barcodePath("barcodes/" + bc + ".bmp"), category(cat), iconKey(icon), cardTint(tint) {}
};

// ----------------------------------------------------------------------------
// Cart Line Item Model
// ----------------------------------------------------------------------------
struct CartLine {
    int productId = 0;
    int quantity = 1;
};

// ----------------------------------------------------------------------------
// Sale Record Model (Completed Transactions)
// ----------------------------------------------------------------------------
struct SaleRecord {
    std::string timestamp;
    int itemCount = 0;
    int totalAmount = 0;
    std::string paymentMethod;
    int cashGiven = 0;
    int changeGiven = 0;
    std::vector<CartLine> items;
};

// ----------------------------------------------------------------------------
// Order Record Model (Audit / Requirements 8 & 9)
// ----------------------------------------------------------------------------
struct OrderRecord {
    std::string orderId;
    std::string orderDate;
    std::string orderTime;
    std::string customerName;
    std::string customerContact;
    std::string orderStatus;
    int paymentAmount = 0;
    std::string paymentMode;
    std::string paymentMerchant;
};

// ----------------------------------------------------------------------------
// Bill Record Model (Itemized Receipt Line / Requirement 10)
// ----------------------------------------------------------------------------
struct BillRecord {
    std::string billId;
    std::string customerName;
    std::string orderId;
    std::string productAcronym;
    int unitPrice = 0;
    int discountPerUnit = 0;
    int quantity = 0;
    int totalWithoutTax = 0;
    int totalTax = 0;
    int totalDiscount = 0;
    int totalPayable = 0;
    std::string paymentStatus;
};

// ----------------------------------------------------------------------------
// Staff Member Model (Access Control & Roles)
// ----------------------------------------------------------------------------
struct StaffMember {
    std::string name;
    std::string password;
    std::string role; // "Admin" or "Employee"
    std::string createdDate;
};

// ----------------------------------------------------------------------------
// Activity Record Model (Audit Trail)
// ----------------------------------------------------------------------------
struct ActivityRecord {
    std::string timestamp;
    std::string category; // "LOGIN", "RETURN", "STAFF", "ORDER", "SALE", "PRODUCT", "INVENTORY"
    std::string user;
    std::string details;
};

// ----------------------------------------------------------------------------
// Navigation Tabs
// ----------------------------------------------------------------------------
enum class NavTab {
    POS = 0,
    Products = 1,
    Inventory = 2,
    Sales = 3,
    ActivityLogs = 4,
    Settings = 5,
    Return = 6
};

#endif // POS_MODELS_H

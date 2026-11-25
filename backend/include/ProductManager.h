#ifndef PRODUCT_MANAGER_H
#define PRODUCT_MANAGER_H

#include <string>
#include <vector>

// Product Specification matching Requirement 4
struct ProductItem {
    std::string id;            // product id
    std::string barcode;       // product barcode (6-digit)
    std::string acronym;       // max 8 characters
    std::string name;          // product name
    std::string unit;          // measurement unit: "count" or "weight"
    int price = 0;             // price
    int unitPrice = 0;         // unit price in PKR (without tax)
    int tax = 0;               // tax on product (amount in PKR)
    int discount = 0;          // discount on one unit of product (in PKR)
    int stock = 0;             // available quantity
    std::string barcodePath;   // barcode file name and path
    std::string categoryId = "01"; // 2-digit category ID

    // UI categorization helpers
    std::string category = "Grocery";
    std::string iconKey = "milk";
};

// Order Record matching Requirement 8
struct OrderRecord {
    std::string orderId;         // 8 digit Order Id, last two digits indicate date of order
    std::string orderDate;       // order date (YYYY-MM-DD)
    std::string orderTime;       // order time (HH:MM:SS)
    std::string customerName;    // customer name (without spaces, e.g. Ahmed_Khan)
    std::string customerContact; // customer contact (optional)
    std::string orderStatus;     // in-progress, on-hold, paid, archived
    int paymentAmount = 0;       // payment amount in PKR
    std::string paymentMode;     // cash, card, wallet
    std::string paymentMerchant; // ABL, JazzCash, EasyPaisa, CashCounter, etc.
};

// Order Detail Record matching Requirement 9
struct OrderDetailRecord {
    std::string orderId;        // order id from Order.txt
    std::string productId;      // product id
    std::string productBarcode; // product barcode
    int quantity = 0;           // quantity
};

// Bill Detail Record matching Requirement 9
struct BillDetailRecord {
    std::string billId;          // 9 digit bill id
    std::string productAcronym;  // product acronym
    int unitPrice = 0;           // product unit price without tax
    int quantity = 0;            // product quantity
    int discountOnProduct = 0;   // discount on bought product
};

// Bill Record matching Requirement 8 (14 fields)
struct BillRecord {
    std::string billId;          // 9 digit id unique
    std::string customerName;    // customer name
    std::string orderId;         // Order Id from Order.txt file
    std::string productAcronym;  // product acronym (optional legacy)
    int unitPrice = 0;           // unit price (optional legacy)
    int discountPerUnit = 0;     // discount per unit (optional legacy)
    int quantity = 0;            // product quantity (optional legacy)
    int totalWithoutTax = 0;     // total bill without tax
    int totalTax = 0;            // total tax
    int totalDiscount = 0;       // total applicable discount
    int totalPayable = 0;        // total payable bill
    int amountReceived = 0;      // amount received
    int balanceReturned = 0;     // balance returned
    std::string paymentStatus;   // bill payment status (order status from Order.txt)
    int seasonalDiscount = 0;    // seasonal discount
    int specialDiscount = 0;     // special discount (<= 8%)
    std::string billDate;        // bill date
    std::string billTime;        // bill time
};

// Manager Functions for Loading, Saving & Validating Database Files
std::string findDataFilePath(const std::string& filename);

bool loadProducts(const std::string& path, std::vector<ProductItem>& products);
bool saveProducts(const std::string& path, const std::vector<ProductItem>& products);

bool loadOrders(const std::string& path, std::vector<OrderRecord>& orders);
bool appendOrder(const std::string& path, const OrderRecord& order);

bool loadOrderDetails(const std::string& path, std::vector<OrderDetailRecord>& details);
bool appendOrderDetail(const std::string& path, const OrderDetailRecord& detail);

bool loadBills(const std::string& path, std::vector<BillRecord>& bills);
bool appendBill(const std::string& path, const BillRecord& bill);

std::string generateNextOrderId(int dayOfMonth, int existingCount);
std::string generateNextBillId(int existingCount);

const ProductItem* findProductByBarcode(const std::vector<ProductItem>& products, const std::string& barcode);
const ProductItem* findProductById(const std::vector<ProductItem>& products, const std::string& id);

void ensureAllBarcodesExist(const std::vector<ProductItem>& products);
bool loadBillDetails(const std::string& path, std::vector<BillDetailRecord>& details);
bool appendBillDetail(const std::string& path, const BillDetailRecord& detail);
#endif // PRODUCT_MANAGER_H

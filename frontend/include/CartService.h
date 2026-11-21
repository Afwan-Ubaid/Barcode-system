#ifndef CART_SERVICE_H
#define CART_SERVICE_H

#include <string>
#include <vector>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Single Responsibility Principle (SRP)
// Module: Cart Management, Financial Calculations, Payment & Receipt Service
// ============================================================================

enum class DiscountCategory {
    None,
    Student,
    Employee,
    Promo
};

struct PromoRecord {
    std::string code;
    int percent = 0;
    int flat = 0;
    std::string targetType = "ALL";     // "ALL", "ITEM", "SUPPLIER"
    std::string targetValue = "ALL";    // "ALL", barcode / item name, or supplier name
    std::string description = "";
};

class CartService {
public:
    CartService();

    // Cart Line Manipulation
    void addItem(int productId, int qty = 1);
    void deleteItem(int index);
    void setItemQuantity(int index, int qty);
    void clear();

    const std::vector<CartLine>& getCart() const { return mCart; }
    std::vector<CartLine>& getCart() { return mCart; }
    bool isEmpty() const { return mCart.empty(); }

    // Pricing & Tax Calculations
    int getSubtotal(const std::vector<ProductItem>& products) const;
    int getDiscount(const std::vector<ProductItem>& products) const;
    int getDiscount(const std::vector<ProductItem>& products,
                    const std::vector<std::pair<std::string, std::string>>& barcodeSupplierMap) const;
    int getDiscount() const;
    int getTax() const;
    int getTotal(const std::vector<ProductItem>& products) const;
    int parseCashGiven() const;
    int getChangeAmount(const std::vector<ProductItem>& products) const;

    // Financial State Accessors
    void setDiscountApplied(bool applied);
    bool isDiscountApplied() const;
    void applyStudentDiscount();
    void applyEmployeeDiscount(const std::string& username);
    bool applyPromoCode(const std::string& code, std::string& outMessage);
    void removeDiscount();
    std::string getDiscountLabel() const;

    DiscountCategory getDiscountCategory() const { return mDiscountCategory; }
    int getStudentDiscountRate() const { return mStudentDiscountPercent; }
    int getEmployeeDiscountRate() const { return mEmployeeDiscountPercent; }
    void setStudentDiscountRate(int rate);
    void setEmployeeDiscountRate(int rate);
    int getPromoCodesCount() const;
    bool loadDiscountSettings(const std::string& customPath = "");
    bool saveDiscountSettings(const std::string& customPath = "");
    bool loadPromoCodes(const std::string& customPath = "");
    bool savePromoCodes(const std::string& customPath = "");
    bool addPromoCode(const PromoRecord& rec);
    bool deletePromoCode(const std::string& code);
    const std::vector<PromoRecord>& getPromoCodes() const { return mPromoDatabase; }
    std::vector<PromoRecord>& getPromoCodes() { return mPromoDatabase; }
    const std::string& getAppliedPromoCode() const { return mAppliedPromoCode; }
    const std::string& getAppliedPromoTargetType() const { return mAppliedPromoTargetType; }
    const std::string& getAppliedPromoTargetValue() const { return mAppliedPromoTargetValue; }
    const std::string& getAppliedEmployeeUser() const { return mAppliedEmployeeUser; }

    void setPaymentMethod(int method) { mPaymentMethod = method; }
    int getPaymentMethod() const { return mPaymentMethod; }

    void setPaymentDone(bool done) { mPaymentDone = done; }
    bool isPaymentDone() const { return mPaymentDone; }

    void setCashGiven(int amount);
    int getCashGiven() const { return mCashGiven; }

    void setCashGivenStr(const std::string& str);
    const std::string& getCashGivenStr() const { return mCashGivenStr; }

    // Transactions & Output
    bool chargeSale(std::vector<ProductItem>& products, const std::string& userName, std::string& outMessage);
    std::string printReceipt(const std::vector<ProductItem>& products, const std::string& cashierName, bool& outSuccess);

    // Park & Retrieve Draft Orders
    bool parkCurrentCart(const std::vector<ProductItem>& products, const std::string& label = "");
    bool parkCurrentCart(const std::string& label = "") { return parkCurrentCart({}, label); }
    bool resumeDraft(int index);
    void deleteDraft(int index);
    const std::vector<DraftCart>& getDraftCarts() const { return mDraftCarts; }
    const std::vector<DraftCart>& getDrafts() const { return mDraftCarts; }
    int getDraftsCount() const { return (int)mDraftCarts.size(); }

    const std::string& getLastReceiptText() const { return mLastReceiptText; }
    const std::string& getLastOrderId() const { return mLastOrderId; }
    const std::string& getLastCashier() const { return mLastCashier; }
    const std::string& getLastDateStr() const { return mLastDateStr; }
    const std::vector<SaleRecord>& getSalesHistory() const { return mSalesHistory; }
    std::vector<SaleRecord>& getSalesHistory() { return mSalesHistory; }

private:
    std::vector<CartLine> mCart;
    std::vector<DraftCart> mDraftCarts;
    std::vector<SaleRecord> mSalesHistory;
    DiscountCategory mDiscountCategory = DiscountCategory::None;
    int mStudentDiscountPercent = 10;
    int mEmployeeDiscountPercent = 20;
    int mPromoDiscountPercent = 0;
    int mPromoDiscountFlat = 0;
    std::string mAppliedPromoCode = "";
    std::string mAppliedPromoTargetType = "ALL";
    std::string mAppliedPromoTargetValue = "ALL";
    std::string mAppliedEmployeeUser = "";
    std::vector<PromoRecord> mPromoDatabase;
    mutable int mLastCalculatedDiscount = 0;
    int mPaymentMethod = 0; // 0=Cash, 1=Card, 2=Wallet
    bool mPaymentDone = false;
    int mCashGiven = 0;
    std::string mCashGivenStr = "";
    std::string mLastReceiptText;
    std::string mLastOrderId = "ORD-1001";
    std::string mLastCashier = "System Admin";
    std::string mLastDateStr = "";
};

#endif // CART_SERVICE_H

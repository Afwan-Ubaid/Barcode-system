#ifndef CART_SERVICE_H
#define CART_SERVICE_H

#include <string>
#include <vector>
#include "PosModels.h"

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Single Responsibility Principle (SRP)
// Module: Cart Management, Financial Calculations, Payment & Receipt Service
// ============================================================================

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
    int getDiscount() const;
    int getTax() const;
    int getTotal(const std::vector<ProductItem>& products) const;
    int parseCashGiven() const;
    int getChangeAmount(const std::vector<ProductItem>& products) const;

    // Financial State Accessors
    void setDiscountApplied(bool applied) { mDiscountApplied = applied; mPaymentDone = false; }
    bool isDiscountApplied() const { return mDiscountApplied; }

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
    bool mDiscountApplied = true;
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

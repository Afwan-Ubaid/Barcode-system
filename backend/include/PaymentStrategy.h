#ifndef PAYMENT_STRATEGY_H
#define PAYMENT_STRATEGY_H

#include <string>
#include <memory>
#include <algorithm>

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Open-Closed Principle (OCP)
// Module: Extensible Payment Method Strategy Interface (ISE-43)
// ============================================================================

struct PaymentResult {
    bool success = false;
    std::string transactionId;
    std::string message;
    int amountCharged = 0;
    int changeGiven = 0;
};

class IPaymentMethod {
public:
    virtual ~IPaymentMethod() = default;
    virtual std::string getMethodName() const = 0;
    virtual std::string getMerchantName() const = 0;
    virtual PaymentResult processPayment(int totalAmount, int tenderAmount = 0) = 0;
};

// 1. Concrete Cash Payment Strategy
class CashPayment : public IPaymentMethod {
private:
    std::string mMerchant;
public:
    explicit CashPayment(const std::string& merchant = "CashCounter") : mMerchant(merchant) {}

    std::string getMethodName() const override { return "cash"; }
    std::string getMerchantName() const override { return mMerchant; }

    PaymentResult processPayment(int totalAmount, int tenderAmount = 0) override {
        PaymentResult res;
        if (tenderAmount < totalAmount) {
            res.success = false;
            res.message = "Cash given is less than payable bill";
            res.changeGiven = 0;
            return res;
        }
        res.success = true;
        res.amountCharged = totalAmount;
        res.changeGiven = std::max(0, tenderAmount - totalAmount);
        res.transactionId = "CASH-PAID";
        res.message = "Cash transaction completed successfully";
        return res;
    }
};

// 2. Concrete Card Payment Strategy
class CardPayment : public IPaymentMethod {
private:
    std::string mMerchant;
public:
    explicit CardPayment(const std::string& merchant = "BankPOS") : mMerchant(merchant) {}

    std::string getMethodName() const override { return "card"; }
    std::string getMerchantName() const override { return mMerchant; }

    PaymentResult processPayment(int totalAmount, int /*tenderAmount*/ = 0) override {
        PaymentResult res;
        res.success = true;
        res.amountCharged = totalAmount;
        res.changeGiven = 0;
        res.transactionId = "AUTH-CARD-984321";
        res.message = "Card approved via " + mMerchant;
        return res;
    }
};

// 3. Concrete Mobile Wallet Payment Strategy (ABL, JazzCash, EasyPaisa)
class MobileWalletPayment : public IPaymentMethod {
private:
    std::string mWalletProvider;
public:
    explicit MobileWalletPayment(const std::string& provider = "JazzCash") : mWalletProvider(provider) {}

    std::string getMethodName() const override { return "wallet"; }
    std::string getMerchantName() const override { return mWalletProvider; }

    PaymentResult processPayment(int totalAmount, int /*tenderAmount*/ = 0) override {
        PaymentResult res;
        res.success = true;
        res.amountCharged = totalAmount;
        res.changeGiven = 0;
        res.transactionId = "TXN-" + mWalletProvider + "-554411";
        res.message = "Wallet transfer approved via " + mWalletProvider;
        return res;
    }
};

// Factory to create strategies dynamically adhering to OCP
class PaymentMethodFactory {
public:
    static std::unique_ptr<IPaymentMethod> create(int methodType, const std::string& merchant = "") {
        switch (methodType) {
            case 0: return std::make_unique<CashPayment>(merchant.empty() ? "CashCounter" : merchant);
            case 1: return std::make_unique<CardPayment>(merchant.empty() ? "BankPOS" : merchant);
            case 2: return std::make_unique<MobileWalletPayment>(merchant.empty() ? "JazzCash" : merchant);
            default: return std::make_unique<CashPayment>();
        }
    }
};

#endif // PAYMENT_STRATEGY_H

#include "CartService.h"
#include "UIUtils.h"
#include <ctime>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <algorithm>

CartService::CartService() {
    // Default initial demonstration cart
    mCart = {{0, 1}, {1, 2}, {3, 1}, {4, 1}};
}

void CartService::addItem(int productId, int qty) {
    for (auto& line : mCart) {
        if (line.productId == productId) {
            line.quantity += qty;
            mPaymentDone = false;
            return;
        }
    }
    mCart.push_back({productId, qty});
    mPaymentDone = false;
}

void CartService::deleteItem(int index) {
    if (index >= 0 && index < (int)mCart.size()) {
        mCart.erase(mCart.begin() + index);
        mPaymentDone = false;
    }
}

void CartService::setItemQuantity(int index, int qty) {
    if (index >= 0 && index < (int)mCart.size()) {
        if (qty <= 0) {
            deleteItem(index);
        } else {
            mCart[index].quantity = qty;
            mPaymentDone = false;
        }
    }
}

void CartService::clear() {
    mCart.clear();
    mPaymentDone = false;
    mCashGiven = 0;
    mCashGivenStr = "";
}

int CartService::getSubtotal(const std::vector<ProductItem>& products) const {
    int sub = 0;
    for (const auto& line : mCart) {
        if (line.productId >= 0 && line.productId < (int)products.size()) {
            sub += products[line.productId].price * line.quantity;
        }
    }
    return sub;
}

int CartService::getDiscount() const {
    return (mDiscountApplied && !mCart.empty()) ? 120 : 0;
}

int CartService::getTax() const {
    return 0; // Tax free
}

int CartService::getTotal(const std::vector<ProductItem>& products) const {
    return std::max(0, getSubtotal(products) - getDiscount());
}

int CartService::parseCashGiven() const {
    if (!mCashGivenStr.empty()) {
        try {
            return std::max(0, std::stoi(mCashGivenStr));
        } catch (...) {}
    }
    return mCashGiven;
}

int CartService::getChangeAmount(const std::vector<ProductItem>& products) const {
    int given = parseCashGiven();
    return given - getTotal(products);
}

void CartService::setCashGiven(int amount) {
    mCashGiven = amount;
    mCashGivenStr = std::to_string(amount);
}

void CartService::setCashGivenStr(const std::string& str) {
    mCashGivenStr = str;
    mCashGiven = parseCashGiven();
}

bool CartService::chargeSale(std::vector<ProductItem>& products, const std::string& userName, std::string& outMessage) {
    if (mCart.empty()) {
        outMessage = "Cannot charge: receipt is empty";
        return false;
    }

    int total = getTotal(products);
    int given = parseCashGiven();
    if (mPaymentMethod == 0 && given < total) {
        outMessage = "Cash given (" + formatRs(given) + ") is less than total (" + formatRs(total) + ")!";
        return false;
    }

    SaleRecord rec;
    std::time_t now = std::time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmNow);
    char fullDateBuf[64];
    std::strftime(fullDateBuf, sizeof(fullDateBuf), "%Y-%m-%d %H:%M:%S", &tmNow);

    mLastDateStr = fullDateBuf;
    mLastOrderId = "ORD-" + std::to_string(1001 + mSalesHistory.size());
    mLastCashier = userName.empty() ? "System Admin" : userName;

    rec.timestamp = buf;
    rec.itemCount = (int)mCart.size();
    rec.totalAmount = total;
    rec.paymentMethod = (mPaymentMethod == 0 ? "Cash" : "Card");
    rec.cashGiven = (mPaymentMethod == 0 ? given : total);
    rec.changeGiven = (mPaymentMethod == 0 ? std::max(0, given - total) : 0);
    rec.items = mCart;
    rec.orderId = mLastOrderId;
    rec.cashier = mLastCashier;
    mSalesHistory.push_back(rec);

    // Decrement stock levels
    for (const auto& line : mCart) {
        if (line.productId >= 0 && line.productId < (int)products.size()) {
            products[line.productId].stock = std::max(0, products[line.productId].stock - line.quantity);
        }
    }

    mPaymentDone = true;
    outMessage = "Payment Successful! Paid " + formatRs(total) + (mPaymentMethod == 0 ? " in Cash" : " by Card") + " - Print is ready";
    return true;
}

std::string CartService::printReceipt(const std::vector<ProductItem>& products, const std::string& cashierName, bool& outSuccess) {
    if (!mPaymentDone) {
        outSuccess = false;
        return "";
    }

    if (!cashierName.empty()) {
        mLastCashier = cashierName;
    }

    std::time_t now = std::time(nullptr);
    tm tmNow;
    localtime_s(&tmNow, &now);
    char dateBuf[64];
    std::strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M:%S", &tmNow);
    mLastDateStr = dateBuf;

    if (mLastOrderId.empty() || mLastOrderId == "ORD-1001") {
        mLastOrderId = "ORD-" + std::to_string(1000 + mSalesHistory.size());
    }

    std::stringstream ss;
    ss << "========================================================\n";
    ss << "             SCANIT SUPERMARKET & SMART POS             \n";
    ss << "          Smart Point of Sale & Retail Express          \n";
    ss << "        Main Boulevard, Gulberg III, Lahore, PK         \n";
    ss << "          Tel: 042-35789000  *  NTN: 8492019-3          \n";
    ss << "         Terminal: REG-04  *  Store: PK-LHR-01          \n";
    ss << "========================================================\n";
    ss << "                 SALES RECEIPT / INVOICE                \n";
    ss << "--------------------------------------------------------\n";
    ss << "Date:     " << mLastDateStr << "\n";
    ss << "Order ID: #" << mLastOrderId << "\n";
    ss << "Cashier:  " << mLastCashier << "\n";
    ss << "Terminal: POS Counter 04\n";
    ss << "Status:   COMPLETED / PAID IN FULL\n";
    ss << "--------------------------------------------------------\n";
    ss << "ITEM NAME                  QTY    UNIT PRICE     TOTAL  \n";
    ss << "--------------------------------------------------------\n";

    for (const auto& line : mCart) {
        if (line.productId >= 0 && line.productId < (int)products.size()) {
            const auto& p = products[line.productId];
            char lineBuf[128];
            std::snprintf(lineBuf, sizeof(lineBuf), "%-26s x%-2d    Rs %-8d  Rs %d\n",
                          p.name.substr(0, 26).c_str(), line.quantity, p.price, p.price * line.quantity);
            ss << lineBuf;
        }
    }

    ss << "--------------------------------------------------------\n";
    ss << "Subtotal:                                  " << formatRs(getSubtotal(products)) << "\n";
    if (getDiscount() > 0) {
        ss << "Discount (Store Promo):                   " << formatRs(-getDiscount()) << "\n";
    }
    ss << "========================================================\n";
    ss << "TOTAL AMOUNT:                              " << formatRs(getTotal(products)) << "\n";
    ss << "========================================================\n";
    ss << "Payment Method:                            " << (mPaymentMethod == 0 ? "CASH" : "CARD (VISA/MC)") << "\n";

    if (mPaymentMethod == 0) {
        ss << "Cash Tendered:                             " << formatRs(parseCashGiven()) << "\n";
        ss << "Change Returned:                           " << formatRs(getChangeAmount(products)) << "\n";
    } else {
        ss << "Card POS Approval:                         AUTH #984321 - APPROVED\n";
    }

    ss << "--------------------------------------------------------\n";
    ss << "                 * " << mLastOrderId << " *\n";
    ss << "--------------------------------------------------------\n";
    ss << "       THANK YOU FOR SHOPPING WITH SCANIT STORE!        \n";
    ss << "    Exchange / Return valid for 7 days with this slip   \n";
    ss << "         Customer Helpline: info@scanit-pos.com         \n";
    ss << "========================================================\n";

    mLastReceiptText = ss.str();

    std::ofstream out("receipt.txt");
    if (out.is_open()) {
        out << mLastReceiptText;
        out.close();
    }

    outSuccess = true;
    return mLastReceiptText;
}

bool CartService::parkCurrentCart(const std::vector<ProductItem>& products, const std::string& label, const std::string& orderId) {
    if (mCart.empty()) return false;
    DraftCart d;
    d.draftId = "Draft #" + std::to_string(mDraftCarts.size() + 1);
    d.customerLabel = label.empty() ? ("Customer " + std::to_string(mDraftCarts.size() + 1)) : label;
    d.time = getCurrentTimeHHMMSS();
    d.items = mCart;
    d.totalAmount = getTotal(products);
    d.orderId = orderId;
    mDraftCarts.push_back(d);
    clear();
    return true;
}

bool CartService::resumeDraft(int index) {
    if (index >= 0 && index < (int)mDraftCarts.size()) {
        mCart = mDraftCarts[index].items;
        mDraftCarts.erase(mDraftCarts.begin() + index);
        mPaymentDone = false;
        return true;
    }
    return false;
}

void CartService::deleteDraft(int index) {
    if (index >= 0 && index < (int)mDraftCarts.size()) {
        mDraftCarts.erase(mDraftCarts.begin() + index);
    }
}

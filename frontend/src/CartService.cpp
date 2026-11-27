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
    loadDiscountSettings();
    loadPromoCodes();
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

int CartService::getDiscount(const std::vector<ProductItem>& products) const {
    return getDiscount(products, {});
}

int CartService::getDiscount(const std::vector<ProductItem>& products,
                             const std::vector<std::pair<std::string, std::string>>& barcodeSupplierMap) const {
    if (mDiscountCategory == DiscountCategory::None || mCart.empty()) {
        mLastCalculatedDiscount = 0;
        return 0;
    }
    int sub = getSubtotal(products);
    if (sub <= 0) {
        mLastCalculatedDiscount = 0;
        return 0;
    }
    int disc = 0;
    if (mDiscountCategory == DiscountCategory::Student) {
        disc = (sub * mStudentDiscountPercent) / 100;
    } else if (mDiscountCategory == DiscountCategory::Employee) {
        disc = (sub * mEmployeeDiscountPercent) / 100;
    } else if (mDiscountCategory == DiscountCategory::Promo) {
        int eligibleSubtotal = sub;
        if (mAppliedPromoTargetType == "ITEM" && !mAppliedPromoTargetValue.empty() && mAppliedPromoTargetValue != "ALL") {
            eligibleSubtotal = 0;
            for (const auto& line : mCart) {
                if (line.productId >= 0 && line.productId < (int)products.size()) {
                    const auto& p = products[line.productId];
                    if (p.barcode == mAppliedPromoTargetValue || p.name == mAppliedPromoTargetValue) {
                        eligibleSubtotal += p.price * line.quantity;
                    }
                }
            }
        } else if (mAppliedPromoTargetType == "SUPPLIER" && !mAppliedPromoTargetValue.empty() && mAppliedPromoTargetValue != "ALL") {
            eligibleSubtotal = 0;
            std::string targetSuppLower = mAppliedPromoTargetValue;
            for (char& c : targetSuppLower) c = (char)std::tolower(c);

            for (const auto& line : mCart) {
                if (line.productId >= 0 && line.productId < (int)products.size()) {
                    const auto& p = products[line.productId];
                    std::string supp = "";
                    for (const auto& entry : barcodeSupplierMap) {
                        if (entry.first == p.barcode) {
                            supp = entry.second;
                            break;
                        }
                    }
                    std::string suppLower = supp;
                    for (char& c : suppLower) c = (char)std::tolower(c);
                    if (!suppLower.empty() && suppLower.find(targetSuppLower) != std::string::npos) {
                        eligibleSubtotal += p.price * line.quantity;
                    }
                }
            }
        }

        if (mPromoDiscountPercent > 0) {
            disc = (eligibleSubtotal * mPromoDiscountPercent) / 100;
        } else if (mPromoDiscountFlat > 0) {
            disc = std::min(eligibleSubtotal, mPromoDiscountFlat);
        }
    }
    mLastCalculatedDiscount = disc;
    return disc;
}

int CartService::getDiscount() const {
    return mLastCalculatedDiscount;
}

int CartService::getTax(const std::vector<ProductItem>& products) const {
    int t = 0;
    for (const auto& line : mCart) {
        if (line.productId >= 0 && line.productId < (int)products.size()) {
            t += products[line.productId].tax * line.quantity;
        }
    }
    return t;
}

int CartService::getTax() const {
    return 0;
}

int CartService::getTotal(const std::vector<ProductItem>& products) const {
    return std::max(0, getSubtotal(products) - getDiscount(products));
}

BillSummary CartService::computeBillSummary(const std::vector<ProductItem>& products,
                                            int seasonalPercent,
                                            int specialPercent,
                                            const std::vector<std::pair<std::string, std::string>>& barcodeSupplierMap,
                                            int loyaltyPointsToRedeem) const {
    BillSummary s;
    for (const auto& line : mCart) {
        if (line.productId >= 0 && line.productId < (int)products.size()) {
            const auto& p = products[line.productId];
            s.subtotalWithoutTax += p.price * line.quantity;
            s.totalTax           += p.tax * line.quantity;
            s.productDiscount    += p.discount * line.quantity;
        }
    }
    s.promoDiscount = getDiscount(products, barcodeSupplierMap);
    s.totalApplicableDiscount = std::min(s.subtotalWithoutTax, s.productDiscount + s.promoDiscount);

    int afterProdDisc = std::max(0, s.subtotalWithoutTax - s.totalApplicableDiscount);
    s.seasonalPercent = std::clamp(seasonalPercent, 0, 100);
    s.seasonalDiscount = (afterProdDisc * s.seasonalPercent) / 100;

    int afterSeasonal = std::max(0, afterProdDisc - s.seasonalDiscount);
    s.specialPercent = std::clamp(specialPercent, 0, 8);
    s.specialDiscount = (afterSeasonal * s.specialPercent) / 100;

    int afterSpecial = std::max(0, s.subtotalWithoutTax + s.totalTax
                                      - s.totalApplicableDiscount
                                      - s.seasonalDiscount
                                      - s.specialDiscount);
    s.loyaltyDiscount = std::clamp(loyaltyPointsToRedeem, 0, afterSpecial);
    s.totalPayable = std::max(0, afterSpecial - s.loyaltyDiscount);
    return s;
}

void CartService::setDiscountApplied(bool applied) {
    if (!applied) {
        removeDiscount();
    } else {
        if (mDiscountCategory == DiscountCategory::None) {
            applyStudentDiscount();
        }
    }
}

bool CartService::isDiscountApplied() const {
    return mDiscountCategory != DiscountCategory::None;
}

void CartService::applyStudentDiscount() {
    mDiscountCategory = DiscountCategory::Student;
    mPaymentDone = false;
}

void CartService::applyEmployeeDiscount(const std::string& username) {
    mDiscountCategory = DiscountCategory::Employee;
    mAppliedEmployeeUser = username;
    mPaymentDone = false;
}

void CartService::removeDiscount() {
    mDiscountCategory = DiscountCategory::None;
    mAppliedPromoCode.clear();
    mAppliedPromoTargetType = "ALL";
    mAppliedPromoTargetValue = "ALL";
    mAppliedEmployeeUser.clear();
    mPromoDiscountPercent = 0;
    mPromoDiscountFlat = 0;
    mLastCalculatedDiscount = 0;
    mPaymentDone = false;
}

std::string CartService::getDiscountLabel() const {
    switch (mDiscountCategory) {
        case DiscountCategory::Student:
            return "Student (" + std::to_string(mStudentDiscountPercent) + "%)";
        case DiscountCategory::Employee:
            return "Employee (" + std::to_string(mEmployeeDiscountPercent) + "%" +
                   (mAppliedEmployeeUser.empty() ? "" : (" - " + mAppliedEmployeeUser)) + ")";
        case DiscountCategory::Promo: {
            std::string tInfo = "";
            if (mAppliedPromoTargetType == "SUPPLIER" && !mAppliedPromoTargetValue.empty() && mAppliedPromoTargetValue != "ALL") {
                tInfo = " [" + mAppliedPromoTargetValue + "]";
            } else if (mAppliedPromoTargetType == "ITEM" && !mAppliedPromoTargetValue.empty() && mAppliedPromoTargetValue != "ALL") {
                tInfo = " [Item " + mAppliedPromoTargetValue + "]";
            }
            if (mPromoDiscountPercent > 0) {
                return "Promo: " + mAppliedPromoCode + " (" + std::to_string(mPromoDiscountPercent) + "%" + tInfo + ")";
            } else {
                return "Promo: " + mAppliedPromoCode + " (-Rs " + std::to_string(mPromoDiscountFlat) + tInfo + ")";
            }
        }
        case DiscountCategory::None:
        default:
            return "None";
    }
}

bool CartService::applyPromoCode(const std::string& code, std::string& outMessage) {
    loadPromoCodes();
    std::string q = trimStr(code);
    if (q.empty()) {
        outMessage = "Please enter a promo code.";
        return false;
    }
    std::string qUpper = q;
    for (char& c : qUpper) c = (char)std::toupper(c);

    for (const auto& pr : mPromoDatabase) {
        std::string cUpper = pr.code;
        for (char& c : cUpper) c = (char)std::toupper(c);
        if (cUpper == qUpper) {
            mDiscountCategory = DiscountCategory::Promo;
            mAppliedPromoCode = pr.code;
            mPromoDiscountPercent = pr.percent;
            mPromoDiscountFlat = pr.flat;
            mAppliedPromoTargetType = pr.targetType.empty() ? "ALL" : pr.targetType;
            mAppliedPromoTargetValue = pr.targetValue.empty() ? "ALL" : pr.targetValue;
            mPaymentDone = false;

            std::string ruleNote = "";
            if (mAppliedPromoTargetType == "SUPPLIER" && mAppliedPromoTargetValue != "ALL") {
                ruleNote = " for supplier " + mAppliedPromoTargetValue;
            } else if (mAppliedPromoTargetType == "ITEM" && mAppliedPromoTargetValue != "ALL") {
                ruleNote = " for item " + mAppliedPromoTargetValue;
            }

            if (pr.percent > 0) {
                outMessage = "Applied promo " + pr.code + " (" + std::to_string(pr.percent) + "% off" + ruleNote + ")";
            } else {
                outMessage = "Applied promo " + pr.code + " (Rs " + std::to_string(pr.flat) + " off" + ruleNote + ")";
            }
            return true;
        }
    }
    outMessage = "Invalid promo code: " + code;
    return false;
}

bool CartService::loadDiscountSettings(const std::string& customPath) {
    std::vector<std::string> paths = {customPath, resolveDataFilePath("discount_settings.txt"), "discount_settings.txt", "../discount_settings.txt"};
    for (const auto& p : paths) {
        if (p.empty()) continue;
        std::ifstream in(p);
        if (in.is_open()) {
            std::string line;
            while (std::getline(in, line)) {
                line = trimStr(line);
                if (line.empty() || line[0] == '#') continue;
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = trimStr(line.substr(0, eq));
                    std::string val = trimStr(line.substr(eq + 1));
                    if (key == "student_discount") {
                        try { mStudentDiscountPercent = std::stoi(val); } catch (...) {}
                    } else if (key == "employee_discount") {
                        try { mEmployeeDiscountPercent = std::stoi(val); } catch (...) {}
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool CartService::saveDiscountSettings(const std::string& customPath) {
    std::vector<std::string> targets = {customPath.empty() ? resolveDataFilePath("discount_settings.txt") : customPath};
    for (const auto& t : targets) {
        std::ofstream out(t);
        if (out.is_open()) {
            out << "# ScanIT Admin Discount Configuration\n"
                << "student_discount=" << mStudentDiscountPercent << "\n"
                << "employee_discount=" << mEmployeeDiscountPercent << "\n";
            out.close();
        }
    }
    return true;
}

void CartService::setStudentDiscountRate(int rate) {
    mStudentDiscountPercent = std::clamp(rate, 0, 90);
    saveDiscountSettings();
}

void CartService::setEmployeeDiscountRate(int rate) {
    mEmployeeDiscountPercent = std::clamp(rate, 0, 90);
    saveDiscountSettings();
}

bool CartService::loadPromoCodes(const std::string& customPath) {
    mPromoDatabase.clear();
    std::vector<std::string> paths = {customPath, resolveDataFilePath("promo.txt"), "promo.txt", "../promo.txt"};
    for (const auto& p : paths) {
        if (p.empty()) continue;
        std::ifstream in(p);
        if (in.is_open()) {
            std::string line;
            while (std::getline(in, line)) {
                line = trimStr(line);
                if (line.empty() || line[0] == '#') continue;
                std::istringstream iss(line);
                std::string code, val, tType, tVal;
                if (iss >> code >> val) {
                    PromoRecord rec;
                    rec.code = code;
                    if (!val.empty() && val.back() == '%') {
                        try { rec.percent = std::stoi(val.substr(0, val.length() - 1)); } catch (...) {}
                    } else {
                        try { rec.flat = std::stoi(val); } catch (...) {}
                    }
                    if (iss >> tType >> tVal) {
                        rec.targetType = tType;
                        for (char& c : tVal) if (c == '_') c = ' ';
                        rec.targetValue = tVal;
                    } else {
                        rec.targetType = "ALL";
                        rec.targetValue = "ALL";
                    }
                    mPromoDatabase.push_back(rec);
                }
            }
            return true;
        }
    }
    return false;
}

bool CartService::savePromoCodes(const std::string& customPath) {
    std::vector<std::string> targets = {customPath.empty() ? resolveDataFilePath("promo.txt") : customPath};
    for (const auto& t : targets) {
        std::ofstream out(t);
        if (out.is_open()) {
            out << "# ScanIT Promotional Discount Database\n"
                << "# Format: <PROMO_CODE> <DISCOUNT_VALUE> [TARGET_TYPE] [TARGET_VALUE]\n"
                << "# Target Types: ALL ALL | ITEM <barcode/name> | SUPPLIER <supplier_name>\n";
            for (const auto& pr : mPromoDatabase) {
                std::string v = (pr.percent > 0) ? (std::to_string(pr.percent) + "%") : std::to_string(pr.flat);
                std::string tType = pr.targetType.empty() ? "ALL" : pr.targetType;
                std::string tVal = pr.targetValue.empty() ? "ALL" : pr.targetValue;
                for (char& c : tVal) if (c == ' ') c = '_';
                out << pr.code << " " << v << " " << tType << " " << tVal << "\n";
            }
        }
    }
    return true;
}

bool CartService::addPromoCode(const PromoRecord& rec) {
    for (auto& pr : mPromoDatabase) {
        if (pr.code == rec.code) {
            pr = rec;
            savePromoCodes();
            return true;
        }
    }
    mPromoDatabase.push_back(rec);
    savePromoCodes();
    return true;
}

bool CartService::deletePromoCode(const std::string& code) {
    for (auto it = mPromoDatabase.begin(); it != mPromoDatabase.end(); ++it) {
        if (it->code == code) {
            mPromoDatabase.erase(it);
            savePromoCodes();
            return true;
        }
    }
    return false;
}

int CartService::getPromoCodesCount() const {
    return (int)mPromoDatabase.size();
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

bool CartService::chargeSale(std::vector<ProductItem>& products, const std::string& userName, std::string& outMessage,
                             const BillSummary* summaryOverride, const std::string& orderIdOverride) {
    if (mCart.empty()) {
        outMessage = "Cannot charge: receipt is empty";
        return false;
    }

    BillSummary summary = summaryOverride ? *summaryOverride : computeBillSummary(products, 0, 0);
    mLastBillSummary = summary;

    int total = summary.totalPayable;
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
    mLastOrderId = !orderIdOverride.empty() ? orderIdOverride : ("ORD-" + std::to_string(1001 + mSalesHistory.size()));
    mLastCashier = userName.empty() ? "System Admin" : userName;

    rec.timestamp = buf;
    rec.itemCount = (int)mCart.size();
    rec.totalAmount = total;
    rec.paymentMethod = (mPaymentMethod == 0 ? "Cash" : (mPaymentMethod == 1 ? "Card" : "Split"));
    if (mPaymentMethod == 0) {
        rec.cashGiven = given;
        rec.changeGiven = std::max(0, given - total);
    } else if (mPaymentMethod == 2) {
        rec.cashGiven = summary.splitCashAmount;
        rec.changeGiven = 0;
    } else {
        rec.cashGiven = total;
        rec.changeGiven = 0;
    }
    rec.items = mCart;
    rec.orderId = mLastOrderId;
    rec.cashier = mLastCashier;
    mSalesHistory.push_back(rec);

    // Note: stock deduction is managed cleanly by PosApp (supporting edits without double-deduct)
    if (!summaryOverride) {
        for (const auto& line : mCart) {
            if (line.productId >= 0 && line.productId < (int)products.size()) {
                products[line.productId].stock = std::max(0, products[line.productId].stock - line.quantity);
            }
        }
    }

    mPaymentDone = true;
    if (mPaymentMethod == 2) {
        outMessage = "Payment Successful! Paid " + formatRs(total) + " (Split: Cash " +
                     formatRs(summary.splitCashAmount) + " + Card " + formatRs(summary.splitCardAmount) + ") - Print is ready";
    } else {
        outMessage = "Payment Successful! Paid " + formatRs(total) + (mPaymentMethod == 0 ? " in Cash" : " by Card") + " - Print is ready";
    }
    return true;
}

std::string CartService::printReceipt(const std::vector<ProductItem>& products, const std::string& cashierName, bool& outSuccess,
                                      const BillSummary* summaryOverride) {
    if (!mPaymentDone) {
        outSuccess = false;
        return "";
    }

    if (!cashierName.empty()) {
        mLastCashier = cashierName;
    }

    BillSummary summary = summaryOverride ? *summaryOverride : mLastBillSummary;
    if (summary.subtotalWithoutTax == 0 && !mCart.empty()) {
        summary = computeBillSummary(products, 0, 0);
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
    ss << "Subtotal (No Tax):                         " << formatRs(summary.subtotalWithoutTax) << "\n";
    if (summary.totalTax > 0) {
        ss << "Sales Tax:                                 " << formatRs(summary.totalTax) << "\n";
    }
    if (summary.totalApplicableDiscount > 0) {
        ss << "Product / Promo Discount:                  " << formatRs(-summary.totalApplicableDiscount) << "\n";
    }
    if (summary.seasonalDiscount > 0) {
        ss << "Seasonal Discount (" << summary.seasonalPercent << "%):                   " << formatRs(-summary.seasonalDiscount) << "\n";
    }
    if (summary.specialDiscount > 0) {
        ss << "Special Discount (" << summary.specialPercent << "%):                    " << formatRs(-summary.specialDiscount) << "\n";
    }
    if (summary.loyaltyDiscount > 0) {
        ss << "Loyalty Points Redeemed:                   " << formatRs(-summary.loyaltyDiscount) << "\n";
    }
    ss << "========================================================\n";
    ss << "TOTAL PAYABLE AMOUNT:                      " << formatRs(summary.totalPayable) << "\n";
    ss << "========================================================\n";
    std::string payLabel = (mPaymentMethod == 0 ? "CASH" : (mPaymentMethod == 1 ? "CARD (VISA/MC)" : "SPLIT (CASH + CARD)"));
    ss << "Payment Method:                            " << payLabel << "\n";

    if (mPaymentMethod == 0) {
        int given = parseCashGiven();
        ss << "Cash Tendered:                             " << formatRs(given) << "\n";
        ss << "Change Returned:                           " << formatRs(std::max(0, given - summary.totalPayable)) << "\n";
    } else if (mPaymentMethod == 2) {
        ss << "Cash Portion:                              " << formatRs(summary.splitCashAmount) << "\n";
        ss << "Card Portion (AUTH #984321):               " << formatRs(summary.splitCardAmount) << "\n";
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

    std::ofstream out(resolveDataFilePath("receipt.txt"));
    if (out.is_open()) {
        out << mLastReceiptText;
        out.close();
    }

    outSuccess = true;
    return mLastReceiptText;
}

bool CartService::parkCurrentCart(const std::vector<ProductItem>& products, const std::string& label) {
    if (mCart.empty()) return false;
    DraftCart d;
    d.draftId = "Draft #" + std::to_string(mDraftCarts.size() + 1);
    d.customerLabel = label.empty() ? ("Customer " + std::to_string(mDraftCarts.size() + 1)) : label;
    d.time = getCurrentTimeHHMMSS();
    d.items = mCart;
    d.totalAmount = getTotal(products);
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

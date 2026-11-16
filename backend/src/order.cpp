#include "../include/Order.h"

// You can add helper methods here if you want, 
// or define constructors. But since it's a struct, 
// we usually leave it empty. 

// To prove to your Scrum Master that you have a .cpp for it,
// here is a namespace function that calculates the bill totals cleanly:

namespace OrderUtils {
    double getSubtotal(const Order& order) {
        double total = 0;
        for (const auto& d : order.details) {
            total += (d.unitPrice * d.quantity);
        }
        return total;
    }

    double getTotalTax(const Order& order) {
        double total = 0;
        for (const auto& d : order.details) {
            total += (d.unitTax * d.quantity);
        }
        return total;
    }

    double getTotalDiscount(const Order& order) {
        double total = 0;
        for (const auto& d : order.details) {
            total += (d.unitDiscount * d.quantity);
        }
        return total;
    }
}
#include "../include/FileHandler.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
using namespace std;

vector<Product> FileHandler::loadProducts(const string& filename) {
    vector<Product> products;
    ifstream file(filename);
    if (!file.is_open()) return products;

    string line;
    while (getline(file, line, '#')) {
        if (line.empty() || line.find_first_not_of(" \n\r\t") == string::npos) continue;
        
        istringstream iss(line);
        Product p;
        // Read exact format: id barcode acronym name unit price tax discount qty path
        iss >> p.id >> p.barcode >> p.acronym >> p.name >> p.unit 
            >> p.price >> p.tax >> p.discount >> p.quantity >> p.barcodePath;
            
        products.push_back(p);
    }
    return products;
}

void FileHandler::saveOrder(const Order& order) {
    ofstream file("Order.txt", ios::app);
    if (file.is_open()) {
        file << order.orderId << " " << order.date << " " << order.time << " "
             << order.customerName << " " << order.customerContact << " "
             << order.status << " " << fixed << setprecision(2) << order.paymentAmount << " "
             << order.paymentMode << " " << order.paymentMerchant << " #\n";
    }
}

void FileHandler::saveOrderDetails(const Order& order) {
    ofstream file("OrderDetail.txt", ios::app);
    if (file.is_open()) {
        for (const auto& detail : order.details) {
            file << order.orderId << " " << detail.productId << " " 
                 << detail.barcode << " " << detail.quantity << " #\n";
        }
    }
}

void FileHandler::saveBill(const Order& order, const string& billId, const string& productAcronym) {
    ofstream file("Bill.txt", ios::app);
    if (file.is_open()) {
        double totalWithoutTax = 0, totalTax = 0, totalDiscount = 0;
        for (const auto& d : order.details) {
            totalWithoutTax += (d.unitPrice * d.quantity);
            totalTax += (d.unitTax * d.quantity);
            totalDiscount += (d.unitDiscount * d.quantity);
        }
        double payable = totalWithoutTax + totalTax - totalDiscount;

        file << billId << " " << order.customerName << " " << order.orderId << " "
             << productAcronym << " " << fixed << setprecision(2) << totalWithoutTax << " "
             << totalDiscount << " " << order.details.size() << " "
             << totalWithoutTax << " " << totalTax << " " << totalDiscount << " "
             << payable << " " << order.status << " #\n";
    }
}
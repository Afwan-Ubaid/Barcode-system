#include "PdfGenerator.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;

static std::string escapePdf(const std::string& str) {
    std::string out;
    for (char c : str) {
        if (c == '(' || c == ')' || c == '\\') {
            out += '\\';
        }
        out += c;
    }
    return out;
}

bool PdfGenerator::generateReceiptPdf(const std::string& outputPath,
                                     const BillRecord& bill,
                                     const std::vector<BillDetailRecord>& details,
                                     const std::string& cashierName) {
    fs::path p(outputPath);
    if (p.has_parent_path() && !fs::exists(p.parent_path())) {
        fs::create_directories(p.parent_path());
    }

    std::ostringstream stream;

    // Helper lambda to draw text
    auto drawText = [&](const std::string& text, float x, float y, const std::string& font, float size, float r = 0.f, float g = 0.f, float b = 0.f) {
        stream << r << " " << g << " " << b << " rg\n";
        stream << "BT /" << font << " " << size << " Tf " << x << " " << y << " Td (" << escapePdf(text) << ") Tj ET\n";
    };

    // Helper lambda to draw line
    auto drawLine = [&](float x1, float y1, float x2, float y2, float gray = 0.7f, float width = 1.f) {
        stream << gray << " G " << width << " w\n";
        stream << x1 << " " << y1 << " m " << x2 << " " << y2 << " l S\n";
    };

    // Helper lambda to draw rectangle
    auto drawRect = [&](float x, float y, float w, float h, float r, float g, float b, bool fill = true) {
        if (fill) {
            stream << r << " " << g << " " << b << " rg " << x << " " << y << " " << w << " " << h << " re f\n";
        } else {
            stream << r << " " << g << " " << b << " RG 1 w " << x << " " << y << " " << w << " " << h << " re S\n";
        }
    };

    // Header Background
    drawRect(40, 740, 515, 65, 0.94f, 0.93f, 0.89f);
    drawText("ScanIT SUPERMARKET", 55, 782, "F2", 18, 0.15f, 0.15f, 0.15f);
    drawText("Official Retail Sales Invoice & Payment Receipt", 55, 764, "F1", 10, 0.45f, 0.42f, 0.38f);
    drawText("ORIGINAL COPY", 450, 775, "F2", 10, 0.2f, 0.5f, 0.25f);

    // Meta Box
    drawRect(40, 650, 515, 75, 0.98f, 0.97f, 0.95f);
    drawRect(40, 650, 515, 75, 0.85f, 0.82f, 0.78f, false);

    drawText("Bill ID:", 55, 705, "F2", 9, 0.3f, 0.3f, 0.3f);
    drawText(bill.billId, 115, 705, "F1", 9, 0.1f, 0.1f, 0.1f);

    drawText("Order ID:", 55, 685, "F2", 9, 0.3f, 0.3f, 0.3f);
    drawText(bill.orderId, 115, 685, "F1", 9, 0.1f, 0.1f, 0.1f);

    drawText("Customer:", 55, 665, "F2", 9, 0.3f, 0.3f, 0.3f);
    drawText(bill.customerName.empty() ? "Walk-in Customer" : bill.customerName, 115, 665, "F1", 9, 0.1f, 0.1f, 0.1f);

    drawText("Date:", 330, 705, "F2", 9, 0.3f, 0.3f, 0.3f);
    drawText(bill.billDate, 385, 705, "F1", 9, 0.1f, 0.1f, 0.1f);

    drawText("Time:", 330, 685, "F2", 9, 0.3f, 0.3f, 0.3f);
    drawText(bill.billTime, 385, 685, "F1", 9, 0.1f, 0.1f, 0.1f);

    drawText("Cashier:", 330, 665, "F2", 9, 0.3f, 0.3f, 0.3f);
    drawText(cashierName.empty() ? "Admin" : cashierName, 385, 665, "F1", 9, 0.1f, 0.1f, 0.1f);

    // Items Table Header
    float tableY = 620;
    drawRect(40, tableY - 5, 515, 22, 0.2f, 0.25f, 0.3f);
    drawText("NO", 50, tableY + 2, "F2", 9, 1.f, 1.f, 1.f);
    drawText("PRODUCT / ACRONYM", 85, tableY + 2, "F2", 9, 1.f, 1.f, 1.f);
    drawText("PRICE (PKR)", 270, tableY + 2, "F2", 9, 1.f, 1.f, 1.f);
    drawText("QTY", 360, tableY + 2, "F2", 9, 1.f, 1.f, 1.f);
    drawText("DISC", 415, tableY + 2, "F2", 9, 1.f, 1.f, 1.f);
    drawText("TOTAL (PKR)", 480, tableY + 2, "F2", 9, 1.f, 1.f, 1.f);

    float curY = tableY - 24;
    int lineNo = 1;
    for (const auto& d : details) {
        if (curY < 260) break; // page boundary guard above totals box
        if (lineNo % 2 == 0) {
            drawRect(40, curY - 4, 515, 18, 0.96f, 0.96f, 0.96f);
        }

        int discAmt = (d.discountOnProduct > 0) ? d.discountOnProduct : d.productDiscount;
        int lineTotal = (d.unitPrice * d.quantity) - discAmt;
        drawText(std::to_string(lineNo), 50, curY, "F1", 9, 0.3f, 0.3f, 0.3f);
        drawText(d.productAcronym, 85, curY, "F2", 9, 0.1f, 0.1f, 0.1f);
        drawText(std::to_string(d.unitPrice), 270, curY, "F1", 9, 0.2f, 0.2f, 0.2f);
        drawText(std::to_string(d.quantity), 365, curY, "F1", 9, 0.2f, 0.2f, 0.2f);
        drawText(std::to_string(discAmt), 420, curY, "F1", 9, 0.2f, 0.2f, 0.2f);
        drawText(std::to_string(lineTotal), 485, curY, "F2", 9, 0.1f, 0.1f, 0.1f);

        drawLine(40, curY - 5, 555, curY - 5, 0.9f, 0.5f);
        curY -= 20;
        lineNo++;
    }

    // Totals Section
    float sumY = curY - 10;
    if (sumY > 230) sumY = 230; // pin near bottom for consistent layout

    drawRect(300, sumY - 145, 255, 150, 0.98f, 0.98f, 0.98f);
    drawRect(300, sumY - 145, 255, 150, 0.8f, 0.8f, 0.8f, false);

    drawText("Subtotal (Without Tax):", 310, sumY - 10, "F1", 9, 0.3f, 0.3f, 0.3f);
    drawText("Rs " + std::to_string(bill.totalWithoutTax), 470, sumY - 10, "F1", 9, 0.1f, 0.1f, 0.1f);

    drawText("Applicable Product Discount:", 310, sumY - 26, "F1", 9, 0.3f, 0.3f, 0.3f);
    drawText("- Rs " + std::to_string(bill.totalDiscount), 470, sumY - 26, "F1", 9, 0.7f, 0.2f, 0.2f);

    drawText("Seasonal Discount (Seasons.txt):", 310, sumY - 42, "F1", 9, 0.3f, 0.3f, 0.3f);
    drawText("- Rs " + std::to_string(bill.seasonalDiscount), 470, sumY - 42, "F1", 9, 0.7f, 0.2f, 0.2f);

    drawText("Special Discount (<= 8%):", 310, sumY - 58, "F1", 9, 0.3f, 0.3f, 0.3f);
    drawText("- Rs " + std::to_string(bill.specialDiscount), 470, sumY - 58, "F1", 9, 0.7f, 0.2f, 0.2f);

    drawText("Sales Tax (GST):", 310, sumY - 74, "F1", 9, 0.3f, 0.3f, 0.3f);
    drawText("Rs " + std::to_string(bill.totalTax), 470, sumY - 74, "F1", 9, 0.1f, 0.1f, 0.1f);

    drawLine(310, sumY - 82, 545, sumY - 82, 0.7f, 1.f);

    drawText("TOTAL PAYABLE:", 310, sumY - 98, "F2", 11, 0.1f, 0.1f, 0.1f);
    drawText("Rs " + std::to_string(bill.totalPayable), 465, sumY - 98, "F2", 11, 0.15f, 0.45f, 0.2f);

    drawText("Amount Tendered / Received:", 310, sumY - 114, "F1", 9, 0.3f, 0.3f, 0.3f);
    drawText("Rs " + std::to_string(bill.amountReceived), 470, sumY - 114, "F1", 9, 0.1f, 0.1f, 0.1f);

    drawText("Change / Balance Returned:", 310, sumY - 130, "F1", 9, 0.3f, 0.3f, 0.3f);
    drawText("Rs " + std::to_string(bill.balanceReturned), 470, sumY - 130, "F1", 9, 0.1f, 0.1f, 0.1f);

    // Payment Status Stamp
    drawRect(55, sumY - 90, 180, 50, 0.92f, 0.98f, 0.92f);
    drawRect(55, sumY - 90, 180, 50, 0.2f, 0.6f, 0.2f, false);
    drawText("STATUS: " + (bill.paymentStatus.empty() ? "PAID" : bill.paymentStatus), 70, sumY - 60, "F2", 11, 0.15f, 0.5f, 0.2f);
    drawText("Payment Verified & Logged", 70, sumY - 78, "F1", 8.5f, 0.35f, 0.45f, 0.35f);

    // Footer notice
    drawLine(40, 60, 555, 60, 0.8f, 1.f);
    drawText("Exchange / return valid for 7 days with this slip & original packing. Paid orders editable only upon Manager Approval.", 55, 45, "F1", 7.5f, 0.45f, 0.45f, 0.45f);
    drawText("ScanIT Point of Sale System - Software Engineering Project", 55, 32, "F1", 7.5f, 0.55f, 0.55f, 0.55f);

    std::string streamData = stream.str();

    // Assemble PDF Objects
    std::vector<std::string> objects;

    // 1: Catalog
    objects.push_back("<< /Type /Catalog /Pages 2 0 R >>");

    // 2: Pages
    objects.push_back("<< /Type /Pages /Kids [3 0 R] /Count 1 >>");

    // 3: Page (A4 = 595 x 842 points)
    objects.push_back("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Resources << /Font << /F1 4 0 R /F2 5 0 R >> >> /Contents 6 0 R >>");

    // 4: Regular Font (Helvetica)
    objects.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>");

    // 5: Bold Font (Helvetica-Bold)
    objects.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>");

    // 6: Content Stream
    std::ostringstream obj6;
    obj6 << "<< /Length " << streamData.length() << " >>\nstream\n" << streamData << "\nendstream";
    objects.push_back(obj6.str());

    // Write file with xref table
    std::ofstream pdf(outputPath, std::ios::binary);
    if (!pdf.is_open()) return false;

    pdf << "%PDF-1.4\n";
    std::vector<long long> xrefOffsets;
    xrefOffsets.push_back(0); // 0th object is always free

    for (size_t i = 0; i < objects.size(); i++) {
        xrefOffsets.push_back((long long)pdf.tellp());
        pdf << (i + 1) << " 0 obj\n" << objects[i] << "\nendobj\n";
    }

    long long xrefStart = (long long)pdf.tellp();
    pdf << "xref\n";
    pdf << "0 " << (objects.size() + 1) << "\n";
    pdf << "0000000000 65535 f \n";
    for (size_t i = 1; i <= objects.size(); i++) {
        pdf << std::setw(10) << std::setfill('0') << xrefOffsets[i] << " 00000 n \n";
    }

    pdf << "trailer\n";
    pdf << "<< /Size " << (objects.size() + 1) << " /Root 1 0 R >>\n";
    pdf << "startxref\n" << xrefStart << "\n%%EOF\n";
    pdf.close();

    return true;
}

bool PdfGenerator::generateReportPdf(const std::string& outputPath,
                                     const std::string& title,
                                     const std::string& subtitle,
                                     const std::vector<std::string>& headers,
                                     const std::vector<std::vector<std::string>>& rows,
                                     const std::string& footerSummary) {
    fs::path p(outputPath);
    if (p.has_parent_path() && !fs::exists(p.parent_path())) {
        fs::create_directories(p.parent_path());
    }

    std::ostringstream stream;
    auto drawText = [&](const std::string& text, float x, float y, const std::string& font, float size, float r = 0.f, float g = 0.f, float b = 0.f) {
        stream << r << " " << g << " " << b << " rg\n";
        stream << "BT /" << font << " " << size << " Tf " << x << " " << y << " Td (" << escapePdf(text) << ") Tj ET\n";
    };
    auto drawLine = [&](float x1, float y1, float x2, float y2, float gray = 0.7f, float width = 1.f) {
        stream << gray << " G " << width << " w\n";
        stream << x1 << " " << y1 << " m " << x2 << " " << y2 << " l S\n";
    };
    auto drawRect = [&](float x, float y, float w, float h, float r, float g, float b, bool fill = true) {
        if (fill) {
            stream << r << " " << g << " " << b << " rg " << x << " " << y << " " << w << " " << h << " re f\n";
        } else {
            stream << r << " " << g << " " << b << " RG 1 w " << x << " " << y << " " << w << " " << h << " re S\n";
        }
    };

    // Header banner
    drawRect(36, 748, 523, 56, 0.94f, 0.93f, 0.89f);
    drawText(title, 50, 782, "F2", 15, 0.14f, 0.14f, 0.14f);
    drawText(subtitle, 50, 762, "F1", 9, 0.40f, 0.38f, 0.35f);

    // Table header
    float tableX = 36.f;
    float tableW = 523.f;
    float tableY = 718.f;
    drawRect(tableX, tableY - 5, tableW, 20, 0.18f, 0.24f, 0.22f);

    size_t colCount = headers.empty() ? 1 : headers.size();
    float colW = tableW / static_cast<float>(colCount);
    size_t maxChars = colCount <= 4 ? 22 : (colCount <= 6 ? 15 : 11);

    for (size_t c = 0; c < headers.size(); ++c) {
        std::string h = headers[c].substr(0, maxChars);
        drawText(h, tableX + 6.f + c * colW, tableY + 2, "F2", 8.f, 1.f, 1.f, 1.f);
    }

    float curY = tableY - 22.f;
    int maxRows = 34;
    int drawn = 0;
    for (size_t r = 0; r < rows.size() && drawn < maxRows; ++r, ++drawn) {
        if (r % 2 == 1) {
            drawRect(tableX, curY - 4, tableW, 17, 0.96f, 0.96f, 0.96f);
        }
        for (size_t c = 0; c < rows[r].size() && c < colCount; ++c) {
            std::string cell = rows[r][c];
            if (cell.size() > maxChars) cell = cell.substr(0, maxChars - 2) + "..";
            drawText(cell, tableX + 6.f + c * colW, curY, "F1", 8.f, 0.15f, 0.15f, 0.15f);
        }
        drawLine(tableX, curY - 4, tableX + tableW, curY - 4, 0.90f, 0.5f);
        curY -= 17.5f;
    }

    if ((int)rows.size() > maxRows) {
        drawText("... (" + std::to_string(rows.size() - maxRows) + " more rows in CSV export)", tableX + 6.f, curY - 4, "F1", 8.f, 0.45f, 0.45f, 0.45f);
    }

    if (!footerSummary.empty()) {
        drawRect(tableX, 54, tableW, 24, 0.92f, 0.96f, 0.93f);
        drawRect(tableX, 54, tableW, 24, 0.25f, 0.55f, 0.35f, false);
        drawText(footerSummary, tableX + 10.f, 62, "F2", 9.f, 0.12f, 0.38f, 0.18f);
    }

    drawLine(36, 44, 559, 44, 0.8f, 1.f);
    drawText("ScanIT Supermarket POS - Generated Report PDF", 36, 30, "F1", 7.5f, 0.50f, 0.50f, 0.50f);

    std::string streamData = stream.str();
    std::vector<std::string> objects;
    objects.push_back("<< /Type /Catalog /Pages 2 0 R >>");
    objects.push_back("<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
    objects.push_back("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Resources << /Font << /F1 4 0 R /F2 5 0 R >> >> /Contents 6 0 R >>");
    objects.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>");
    objects.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>");
    std::ostringstream obj6;
    obj6 << "<< /Length " << streamData.length() << " >>\nstream\n" << streamData << "\nendstream";
    objects.push_back(obj6.str());

    std::ofstream pdf(outputPath, std::ios::binary);
    if (!pdf.is_open()) return false;
    pdf << "%PDF-1.4\n";
    std::vector<long long> xrefOffsets(1, 0);
    for (size_t i = 0; i < objects.size(); i++) {
        xrefOffsets.push_back((long long)pdf.tellp());
        pdf << (i + 1) << " 0 obj\n" << objects[i] << "\nendobj\n";
    }
    long long xrefStart = (long long)pdf.tellp();
    pdf << "xref\n0 " << (objects.size() + 1) << "\n0000000000 65535 f \n";
    for (size_t i = 1; i <= objects.size(); i++) {
        pdf << std::setw(10) << std::setfill('0') << xrefOffsets[i] << " 00000 n \n";
    }
    pdf << "trailer\n<< /Size " << (objects.size() + 1) << " /Root 1 0 R >>\nstartxref\n" << xrefStart << "\n%%EOF\n";
    return true;
}

bool PdfGenerator::openPdfInDefaultViewer(const std::string& filePath) {
    if (!fs::exists(filePath)) return false;
    HINSTANCE res = ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return ((INT_PTR)res > 32);
}

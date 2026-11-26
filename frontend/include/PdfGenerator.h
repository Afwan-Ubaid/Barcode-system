#ifndef PDF_GENERATOR_H
#define PDF_GENERATOR_H

#include <string>
#include <vector>
#include "PosModels.h"

// ============================================================================
// Native Self-Contained Standard PDF 1.4 Generator (Requirement 7)
// Creates authentic PDF bills without external third-party dependencies.
// ============================================================================
class PdfGenerator {
public:
    static bool generateReceiptPdf(const std::string& outputPath,
                                   const BillRecord& bill,
                                   const std::vector<BillDetailRecord>& details,
                                   const std::string& cashierName = "Cashier");

    static bool generateReportPdf(const std::string& outputPath,
                                  const std::string& title,
                                  const std::string& subtitle,
                                  const std::vector<std::string>& headers,
                                  const std::vector<std::vector<std::string>>& rows,
                                  const std::string& footerSummary = "");

    static bool openPdfInDefaultViewer(const std::string& filePath);
};

#endif // PDF_GENERATOR_H

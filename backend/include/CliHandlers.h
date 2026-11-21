#ifndef CLI_HANDLERS_H
#define CLI_HANDLERS_H

#include <string>

// ============================================================================
// ISE SOFTWARE ENGINEERING PRINCIPLE: Separation of Concerns (SoC)
// Module: Backend Interactive Command Line Handlers & Dispatcher
// ============================================================================

void displayHeader();
void handleGenerate();
void handleDecode();
void handleLiveCameraScan();
void handleViewProductSpecs();
void handleRecordOrderAndGenerateBill();
void handleInventoryAudit();
void handleSalesAudit();

#endif // CLI_HANDLERS_H

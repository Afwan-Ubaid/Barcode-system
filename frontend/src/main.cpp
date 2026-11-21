#include "PosApp.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>
#include <cstring>

static void enableHighDpi() {
    typedef BOOL (WINAPI *SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32) {
        SetProcessDpiAwarenessContextProc setDpiContext = 
            (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (setDpiContext) {
            setDpiContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }
    }
    SetProcessDPIAware();
}

static void parseArgs(const char* cmd, bool& shot, bool& shotPaid, std::string& modal) {
    if (!cmd) return;
    if (strstr(cmd, "--screenshot-discount")) { shot = true; modal = "discount"; }
    else if (strstr(cmd, "--screenshot-quickcheck")) { shot = true; modal = "quickcheck"; }
    else if (strstr(cmd, "--screenshot-spec")) { shot = true; modal = "spec"; }
    else if (strstr(cmd, "--screenshot-desc")) { shot = true; modal = "desc"; }
    else if (strstr(cmd, "--screenshot-addproduct")) { shot = true; modal = "addproduct"; }
    else if (strstr(cmd, "--screenshot-drafts")) { shot = true; modal = "drafts"; }
    else if (strstr(cmd, "--screenshot-addinv")) { shot = true; modal = "addinv"; }
    else if (strstr(cmd, "--screenshot-splash")) { shot = true; modal = "splash"; }
    else if (strstr(cmd, "--screenshot-login")) { shot = true; modal = "login"; }
    else if (strstr(cmd, "--screenshot-products")) { shot = true; modal = "products"; }
    else if (strstr(cmd, "--screenshot-cashier-inventory")) { shot = true; modal = "cashier_inventory"; }
    else if (strstr(cmd, "--screenshot-inventory")) { shot = true; modal = "inventory"; }
    else if (strstr(cmd, "--screenshot-sales-returns")) { shot = true; modal = "sales_returns"; }
    else if (strstr(cmd, "--screenshot-sales-employee")) { shot = true; modal = "sales_employee"; }
    else if (strstr(cmd, "--screenshot-saledetail")) { shot = true; modal = "saledetail"; }
    else if (strstr(cmd, "--screenshot-promo")) { shot = true; modal = "promo"; }
    else if (strstr(cmd, "--screenshot-sales")) { shot = true; modal = "sales"; }
    else if (strstr(cmd, "--screenshot-cashier-settings")) { shot = true; modal = "cashier_settings"; }
    else if (strstr(cmd, "--screenshot-settings")) { shot = true; modal = "settings"; }
    else if (strstr(cmd, "--screenshot-logs")) { shot = true; modal = "logs"; }
    else if (strstr(cmd, "--screenshot-activity")) { shot = true; modal = "logs"; }
    else if (strstr(cmd, "--screenshot-addstaff")) { shot = true; modal = "addstaff"; }
    else if (strstr(cmd, "--screenshot-resetpass")) { shot = true; modal = "resetpass"; }
    else if (strstr(cmd, "--screenshot-return-item")) { shot = true; modal = "return_item"; }
    else if (strstr(cmd, "--screenshot-return")) { shot = true; modal = "return"; }
    else if (strstr(cmd, "--screenshot-add")) { shot = true; modal = "add"; }
    else if (strstr(cmd, "--screenshot-restock")) { shot = true; modal = "restock"; }
    else if (strstr(cmd, "--screenshot-receipt")) { shot = true; modal = "receipt"; }
    else if (strstr(cmd, "--screenshot-enlarge")) { shot = true; modal = "enlarge"; }
    else if (strstr(cmd, "--screenshot-cashier")) { shot = true; modal = "cashier"; }
    else if (strstr(cmd, "--screenshot-cash")) { shot = true; modal = "cash"; }
    else if (strstr(cmd, "--screenshot-card")) { shot = true; modal = "card"; }
    else if (strstr(cmd, "--screenshot-paid")) { shot = true; shotPaid = true; }
    else if (strstr(cmd, "--screenshot")) { shot = true; }
}

static void setWorkingDirToExe() {
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH)) {
        char* p = strrchr(path, '\\');
        if (p) {
            *p = '\0';
            SetCurrentDirectoryA(path);
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    setWorkingDirToExe();
    enableHighDpi();
    const char* cmd = GetCommandLineA();
    bool shot = false;
    bool shotPaid = false;
    std::string modal = "";
    parseArgs(cmd, shot, shotPaid, modal);

    PosApp app;
    if (!app.init(shot, shotPaid, modal)) {
        return 1;
    }
    app.run();
    return 0;
}

int main(int argc, char** argv) {
    setWorkingDirToExe();
    enableHighDpi();
    const char* cmd = GetCommandLineA();
    bool shot = false;
    bool shotPaid = false;
    std::string modal = "";
    parseArgs(cmd, shot, shotPaid, modal);
    PosApp app;
    if (!app.init(shot, shotPaid, modal)) {
        return 1;
    }
    app.run();
    return 0;
}

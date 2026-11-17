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

static void parseArgs(const char* cmd, bool& shot, bool& shotPaid, std::string& modal) {
    if (!cmd) return;
    if (strstr(cmd, "--screenshot-desc")) { shot = true; modal = "desc"; }
    else if (strstr(cmd, "--screenshot-splash")) { shot = true; modal = "splash"; }
    else if (strstr(cmd, "--screenshot-login")) { shot = true; modal = "login"; }
    else if (strstr(cmd, "--screenshot-products")) { shot = true; modal = "products"; }
    else if (strstr(cmd, "--screenshot-cashier-inventory")) { shot = true; modal = "cashier_inventory"; }
    else if (strstr(cmd, "--screenshot-inventory")) { shot = true; modal = "inventory"; }
    else if (strstr(cmd, "--screenshot-sales")) { shot = true; modal = "sales"; }
    else if (strstr(cmd, "--screenshot-cashier-settings")) { shot = true; modal = "cashier_settings"; }
    else if (strstr(cmd, "--screenshot-settings")) { shot = true; modal = "settings"; }
    else if (strstr(cmd, "--screenshot-logs")) { shot = true; modal = "logs"; }
    else if (strstr(cmd, "--screenshot-activity")) { shot = true; modal = "logs"; }
    else if (strstr(cmd, "--screenshot-addstaff")) { shot = true; modal = "addstaff"; }
    else if (strstr(cmd, "--screenshot-resetpass")) { shot = true; modal = "resetpass"; }
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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
    bool shot = false;
    bool shotPaid = false;
    std::string modal = "";
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--screenshot-desc") { shot = true; modal = "desc"; }
        else if (arg == "--screenshot-splash") { shot = true; modal = "splash"; }
        else if (arg == "--screenshot-login") { shot = true; modal = "login"; }
        else if (arg == "--screenshot-products") { shot = true; modal = "products"; }
        else if (arg == "--screenshot-cashier-inventory") { shot = true; modal = "cashier_inventory"; }
        else if (arg == "--screenshot-inventory") { shot = true; modal = "inventory"; }
        else if (arg == "--screenshot-sales") { shot = true; modal = "sales"; }
        else if (arg == "--screenshot-cashier-settings") { shot = true; modal = "cashier_settings"; }
        else if (arg == "--screenshot-settings") { shot = true; modal = "settings"; }
        else if (arg == "--screenshot-logs" || arg == "--screenshot-activity") { shot = true; modal = "logs"; }
        else if (arg == "--screenshot-addstaff") { shot = true; modal = "addstaff"; }
        else if (arg == "--screenshot-resetpass") { shot = true; modal = "resetpass"; }
        else if (arg == "--screenshot-return") { shot = true; modal = "return"; }
        else if (arg == "--screenshot-add") { shot = true; modal = "add"; }
        else if (arg == "--screenshot-restock") { shot = true; modal = "restock"; }
        else if (arg == "--screenshot-receipt") { shot = true; modal = "receipt"; }
        else if (arg == "--screenshot-enlarge") { shot = true; modal = "enlarge"; }
        else if (arg == "--screenshot-cashier") { shot = true; modal = "cashier"; }
        else if (arg == "--screenshot-cash") { shot = true; modal = "cash"; }
        else if (arg == "--screenshot-card") { shot = true; modal = "card"; }
        else if (arg == "--screenshot-paid") { shot = true; shotPaid = true; }
        else if (arg == "--screenshot") { shot = true; }
    }
    PosApp app;
    if (!app.init(shot, shotPaid, modal)) {
        return 1;
    }
    app.run();
    return 0;
}

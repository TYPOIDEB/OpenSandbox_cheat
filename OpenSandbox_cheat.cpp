#include <windows.h>
#include <commctrl.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <stdexcept>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")

// ==================== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ====================
DWORD g_gamePid = 0; // Инфа в оперативке (PID игры)
HWND g_hwndReg, g_hwndEditRegPid, g_hwndRegStatus;
HWND g_hwndMain, g_hwndMainStatus, g_hwndEditCmd;

// ==================== ОТПРАВКА В КОНСОЛЬ ====================
bool SendToConsole(const std::wstring& command) {
    if (g_gamePid == 0) return false;

    FreeConsole();
    if (!AttachConsole(g_gamePid)) return false;

    HANDLE hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hConsoleInput == INVALID_HANDLE_VALUE || hConsoleInput == NULL) {
        FreeConsole();
        return false;
    }

    std::vector<INPUT_RECORD> records;
    for (wchar_t c : command) {
        INPUT_RECORD rec = { 0 };
        rec.EventType = KEY_EVENT;
        rec.Event.KeyEvent.bKeyDown = TRUE;
        rec.Event.KeyEvent.wVirtualKeyCode = VkKeyScanW(c) & 0xFF;
        rec.Event.KeyEvent.uChar.UnicodeChar = c;
        records.push_back(rec);

        rec.Event.KeyEvent.bKeyDown = FALSE;
        records.push_back(rec);
    }

    INPUT_RECORD enter = { 0 };
    enter.EventType = KEY_EVENT;
    enter.Event.KeyEvent.bKeyDown = TRUE;
    enter.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
    enter.Event.KeyEvent.uChar.UnicodeChar = L'\r';
    records.push_back(enter);

    enter.Event.KeyEvent.bKeyDown = FALSE;
    records.push_back(enter);

    DWORD written = 0;
    WriteConsoleInputW(hConsoleInput, records.data(), (DWORD)records.size(), &written);

    FreeConsole();
    return true;
}

// ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================
BOOL CALLBACK SetFontProc(HWND hwnd, LPARAM lParam) {
    SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
    return TRUE;
}

void CenterWindow(HWND hwnd, int width, int height) {
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd, 0, (screenW - width) / 2, (screenH - height) / 2, width, height, SWP_NOZORDER);
}

// ==================== ОКНО 2: МЕНЮ ЧИТОВ ====================
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        std::wstring title = L"Подключено к PID: " + std::to_wstring(g_gamePid);
        CreateWindowW(L"STATIC", title.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 10, 400, 25, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"BUTTON", L"🚀 СУПЕР-ИНЖЕКТ (Cheats + Give All + God)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 40, 460, 40, hwnd, (HMENU)10, NULL, NULL);

        CreateWindowW(L"BUTTON", L"God Mode", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 90, 145, 35, hwnd, (HMENU)11, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Noclip", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 175, 90, 145, 35, hwnd, (HMENU)12, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Give All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, 90, 150, 35, hwnd, (HMENU)13, NULL, NULL);

        CreateWindowW(L"BUTTON", L"3rd Person", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 135, 145, 35, hwnd, (HMENU)14, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Quad Damage", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 175, 135, 145, 35, hwnd, (HMENU)15, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Low Gravity", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, 135, 150, 35, hwnd, (HMENU)16, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Slow Motion", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 180, 145, 35, hwnd, (HMENU)17, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Fast Motion", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 175, 180, 145, 35, hwnd, (HMENU)18, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Say 'Я тут'", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, 180, 150, 35, hwnd, (HMENU)19, NULL, NULL);

        CreateWindowW(L"STATIC", L"Своя команда:", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 235, 100, 25, hwnd, NULL, NULL, NULL);
        g_hwndEditCmd = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 130, 233, 220, 25, hwnd, NULL, NULL, NULL);
        SendMessageW(g_hwndEditCmd, EM_SETCUEBANNER, 0, (LPARAM)L"timescale 1.0, cg_fov 120...");

        CreateWindowW(L"BUTTON", L"Отправить", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 360, 232, 120, 26, hwnd, (HMENU)30, NULL, NULL);

        g_hwndMainStatus = CreateWindowW(L"STATIC", L"Готов к работе", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN, 20, 275, 460, 25, hwnd, NULL, NULL, NULL);

        EnumChildWindows(hwnd, SetFontProc, (LPARAM)hFont);
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case 10:
            SendToConsole(L"sv_cheats 1"); Sleep(100);
            SendToConsole(L"give all"); Sleep(100);
            SendToConsole(L"god"); Sleep(100);
            SendToConsole(L"say Инжект завершён! Я неуязвим.");
            SetWindowTextW(g_hwndMainStatus, L"УСПЕХ: Выдано оружие, включен God Mode.");
            break;
        case 11: SendToConsole(L"god"); SetWindowTextW(g_hwndMainStatus, L"Включен God Mode"); break;
        case 12: SendToConsole(L"noclip"); SetWindowTextW(g_hwndMainStatus, L"Включен Noclip"); break;
        case 13: SendToConsole(L"give all"); SetWindowTextW(g_hwndMainStatus, L"Выданы все предметы"); break;
        case 14: SendToConsole(L"cg_thirdperson 1"); SetWindowTextW(g_hwndMainStatus, L"Вид от 3-го лица"); break;
        case 15: SendToConsole(L"give quad damage"); SetWindowTextW(g_hwndMainStatus, L"Выдан Quad Damage"); break;
        case 16: SendToConsole(L"g_gravity 100"); SetWindowTextW(g_hwndMainStatus, L"Гравитация снижена"); break;
        case 17: SendToConsole(L"timescale 0.5"); SetWindowTextW(g_hwndMainStatus, L"Замедление времени"); break;
        case 18: SendToConsole(L"timescale 2.0"); SetWindowTextW(g_hwndMainStatus, L"Ускорение времени"); break;
        case 19: SendToConsole(L"say Я тут читер!"); SetWindowTextW(g_hwndMainStatus, L"Сообщение отправлено"); break;
        case 30: {
            wchar_t buf[256];
            GetWindowTextW(g_hwndEditCmd, buf, 256);
            if (wcslen(buf) > 0) {
                if (SendToConsole(buf)) SetWindowTextW(g_hwndMainStatus, L"Команда отправлена");
                else SetWindowTextW(g_hwndMainStatus, L"Ошибка отправки");
            }
            break;
        }
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ==================== ОКНО 1: РЕГИСТРАЦИЯ ====================
LRESULT CALLBACK RegWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        CreateWindowW(L"STATIC", L"ДОБРО ПОЖАЛОВАТЬ В ИНЖЕКТОР", WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 15, 360, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Введите PID процесса sandbox.x64.exe:", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 50, 360, 20, hwnd, NULL, NULL, NULL);

        g_hwndEditRegPid = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 20, 80, 150, 25, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Найти автоматически", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 180, 79, 200, 26, hwnd, (HMENU)1, NULL, NULL);

        CreateWindowW(L"BUTTON", L"ПОДКЛЮЧИТЬСЯ К ИГРЕ", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 130, 360, 40, hwnd, (HMENU)2, NULL, NULL);

        g_hwndRegStatus = CreateWindowW(L"STATIC", L"Статус: Ожидание", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN, 20, 190, 360, 25, hwnd, NULL, NULL, NULL);

        EnumChildWindows(hwnd, SetFontProc, (LPARAM)hFont);
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        if (wmId == 1) { // Авто-поиск
            std::vector<DWORD> pids;
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
            if (Process32FirstW(snap, &pe)) {
                do {
                    if (_wcsicmp(pe.szExeFile, L"sandbox.x64.exe") == 0) pids.push_back(pe.th32ProcessID);
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);

            if (pids.empty()) SetWindowTextW(g_hwndRegStatus, L"Статус: Игра не найдена!");
            else {
                DWORD validPid = 0;
                for (DWORD pid : pids) {
                    FreeConsole();
                    if (AttachConsole(pid)) { FreeConsole(); validPid = pid; break; }
                }
                if (validPid != 0) {
                    SetWindowTextW(g_hwndEditRegPid, std::to_wstring(validPid).c_str());
                    SetWindowTextW(g_hwndRegStatus, L"Статус: Игра найдена! Жмите Подключиться.");
                }
                else {
                    SetWindowTextW(g_hwndEditRegPid, std::to_wstring(pids[0]).c_str());
                    SetWindowTextW(g_hwndRegStatus, L"Статус: Взят первый процесс.");
                }
            }
        }
        else if (wmId == 2) { // Подключение и переход ко 2-му окну
            wchar_t buf[32];
            GetWindowTextW(g_hwndEditRegPid, buf, 32);
            if (wcslen(buf) == 0) {
                SetWindowTextW(g_hwndRegStatus, L"Ошибка: Введите PID!");
                break;
            }

            try {
                DWORD pid = std::stoul(buf);
                FreeConsole();
                if (AttachConsole(pid)) {
                    FreeConsole();
                    g_gamePid = pid; // 1. СОХРАНЯЕМ PID В ОПЕРАТИВКУ

                    // 2. СОЗДАЕМ И ОТКРЫВАЕМ ВТОРОЕ ОКНО
                    g_hwndMain = CreateWindowExW(WS_EX_TOPMOST, L"MainClass", L"Open Sandbox - Cheat Menu",
                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                        0, 0, 500, 350, NULL, NULL, GetModuleHandle(NULL), NULL);

                    CenterWindow(g_hwndMain, 500, 350);
                    ShowWindow(g_hwndMain, SW_SHOW);

                    // 3. ТЕПЕРЬ ЗАКРЫВАЕМ ПЕРВОЕ ОКНО (РЕГИСТРАЦИЮ)
                    DestroyWindow(hwnd);
                }
                else {
                    SetWindowTextW(g_hwndRegStatus, L"Ошибка: Неверный PID или нет консоли!");
                }
            }
            catch (...) {
                SetWindowTextW(g_hwndRegStatus, L"Ошибка ввода!");
            }
        }
        break;
    }
    case WM_DESTROY:
        if (g_gamePid == 0) PostQuitMessage(0); // Выходим, только если не подключились
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ==================== ГЛАВНАЯ ФУНКЦИЯ ====================
int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Класс окна 1
    WNDCLASSEXW wcReg = { 0 };
    wcReg.cbSize = sizeof(WNDCLASSEXW);
    wcReg.lpfnWndProc = RegWndProc;
    wcReg.hInstance = hInstance;
    wcReg.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcReg.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcReg.lpszClassName = L"RegClass";
    RegisterClassExW(&wcReg);

    // Класс окна 2
    WNDCLASSEXW wcMain = { 0 };
    wcMain.cbSize = sizeof(WNDCLASSEXW);
    wcMain.lpfnWndProc = MainWndProc;
    wcMain.hInstance = hInstance;
    wcMain.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcMain.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcMain.lpszClassName = L"MainClass";
    RegisterClassExW(&wcMain);

    // Создаем Окно 1 (Регистрация)
    g_hwndReg = CreateWindowExW(WS_EX_TOPMOST, L"RegClass", L"Авторизация в чите",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        0, 0, 400, 260, NULL, NULL, hInstance, NULL);

    CenterWindow(g_hwndReg, 400, 260);
    ShowWindow(g_hwndReg, SW_SHOW);

    ShowWindow(GetConsoleWindow(), SW_HIDE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

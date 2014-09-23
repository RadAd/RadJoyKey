#include "stdafx.h"
#include "RadJoyKey.h"

#include "JoyUtils.h"

#include "resource.h"
#include <shellapi.h>
#include <shlwapi.h>
#include <psapi.h>
#include "WinUtils.h"

#define MAX_LOADSTRING 100

HINSTANCE g_hInst;								// current instance
TCHAR g_szTitle[MAX_LOADSTRING];				// The title bar text

namespace
{
    // Global Variables:
    TCHAR g_szWindowClass[] = L"RADJOYKEY";			// the main window class name

    JoystickConfigT& g_joyconfigs = JoystickConfigT();
    AppConfigT g_appconfigs;
};

const AppConfig& GetAppConfig(const AppConfigT& appconfigs, HWND hWnd)
{
    TCHAR str[1024] = { 0 };
    GetModuleFileNameEx(hWnd, str, ARRAYSIZE(str));

    for (AppConfigT::const_iterator it = appconfigs.begin() + 1; it != appconfigs.end(); ++it)
    {
        if (PathMatchSpec(str, it->app.c_str()))
            return *it;
    }

    return appconfigs.front();
}

AppConfigT::iterator FindAppConfig(AppConfigT& appconfigs, const std::wstring& app)
{
    for (AppConfigT::iterator it = appconfigs.begin(); it != appconfigs.end(); ++it)
    {
        if (it->app == app)
            return it;
    }

    return appconfigs.end();
}

AppConfigT::const_iterator FindAppConfig(const AppConfigT& appconfigs, const std::wstring& app)
{
    for (AppConfigT::const_iterator it = appconfigs.begin(); it != appconfigs.end(); ++it)
    {
        if (it->app == app)
            return it;
    }

    return appconfigs.end();
}

JoystickConfigT::const_iterator FindJoystickConfig(const JoystickConfigT& configs, const std::wstring& name)
{
    for (JoystickConfigT::const_iterator it = configs.begin() + 1; it != configs.end(); ++it)
    {
        if (it->name == name)
            return it;
    }

    return configs.end();
}

const JoystickConfig& GetJoystickConfig(HWND hFGWnd)
{
    const AppConfig& appconfig = GetAppConfig(g_appconfigs, hFGWnd);
    JoystickConfigT::const_iterator it = FindJoystickConfig(g_joyconfigs, appconfig.config);
    if (it != g_joyconfigs.end())
        return *it;
    else
        return g_joyconfigs.front();   // return default
}

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	AboutDlg(HWND, UINT, WPARAM, LPARAM);

void SendKey(WORD wScan, bool bDown)
{
    INPUT ip = { INPUT_KEYBOARD };
    if (wScan & KF_VKEY)
    {
        switch (wScan & 0xFF)
        {
        case VK_LBUTTON:
            ip.type = INPUT_MOUSE;
            ip.mi.dwFlags = bDown ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
            break;

        case VK_RBUTTON:
            ip.type = INPUT_MOUSE;
            ip.mi.dwFlags = bDown ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
            break;

        case VK_MBUTTON:
            ip.type = INPUT_MOUSE;
            ip.mi.dwFlags = bDown ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
            break;

        case VK_XBUTTON1:
            ip.type = INPUT_MOUSE;
            ip.mi.dwFlags = bDown ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
            ip.mi.mouseData = XBUTTON1;
            break;

        case VK_XBUTTON2:
            ip.type = INPUT_MOUSE;
            ip.mi.dwFlags = bDown ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
            ip.mi.mouseData = XBUTTON2;
            break;

        default:
            ip.ki.wVk = wScan & 0xFF;
            ip.ki.wScan = MapVirtualKey(ip.ki.wVk & 0xFF, MAPVK_VK_TO_VSC);
            ip.ki.dwFlags = 0;
            if (!bDown)
                ip.ki.dwFlags |= KEYEVENTF_KEYUP;
            //ip.ki.dwFlags |= KEYEVENTF_SCANCODE;
            break;
        }
    }
    else
    {
        ip.ki.wScan = wScan;
        ip.ki.wVk = MapVirtualKey(ip.ki.wScan & 0xFF, MAPVK_VSC_TO_VK);
        ip.ki.dwFlags = 0;
        if (!bDown)
            ip.ki.dwFlags |= KEYEVENTF_KEYUP;
        if (wScan & KF_EXTENDED)
            ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        //ip.ki.dwFlags |= KEYEVENTF_SCANCODE;
    }

#if _DEBUG
    {
        TCHAR str[1024];
        _stprintf_s(str, _T("SendKey scan: %X vk: %X %s\n"), ip.ki.wScan, ip.ki.wVk, bDown ? _T("Down") : _T("Up"));
        OutputDebugString(str);
    }
#endif

    SendInput(1, &ip, sizeof(INPUT));
}

void SendMouse(LONG dx, LONG dy)
{
    INPUT ip = { INPUT_MOUSE };
    ip.mi.dx = dx;
    ip.mi.dy = dy;
    ip.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &ip, sizeof(INPUT));
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RADJOYKEY));

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    Save(g_joyconfigs);
    Save(g_appconfigs);

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= WndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= hInstance;
    wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RADJOYKEY));
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_RADJOYKEY);
    wcex.lpszClassName	= g_szWindowClass;
    //wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcex.hIconSm		= wcex.hIcon;

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    g_hInst = hInstance; // Store instance handle in our global variable

    LoadString(hInstance, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    InitCommonControls();
    initJoyCaps();

    HWND hWnd = CreateWindow(g_szWindowClass, g_szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        MessageBox(NULL, _T("Unable to create window."), g_szTitle, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    //ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    g_joyconfigs.push_back(JoystickConfig());
    g_joyconfigs.back().name = _T("Default");

    g_appconfigs.push_back(AppConfig());
    g_appconfigs.back().app = _T("*");
    g_appconfigs.back().config = g_joyconfigs.front().name;

    Load(g_joyconfigs);
    Load(g_appconfigs);

    if (g_joyconfigs.size() <= 1)
    {
        g_joyconfigs.push_back(JoystickConfig());
        {
            g_joyconfigs.back().name = _T("WASD and Mouse");
            JoyConfig& joyconfig = g_joyconfigs.back().joy[JOYSTICKID1];
            joyconfig.mouse = MAKEWORD(0, 1);
            joyconfig.button[JOY_BUTTON_BASE | 1]            = MapVirtualKey(VK_ESCAPE, MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_BUTTON_BASE | 2]            = MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_BUTTON_BASE | 4]            = MapVirtualKey(VK_CONTROL, MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_BUTTON_BASE | 5]            = MapVirtualKey(VK_MENU, MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_BUTTON_BASE | 6]            = KF_VKEY | VK_LBUTTON;
            joyconfig.button[JOY_BUTTON_BASE | 7]            = KF_VKEY | VK_RBUTTON;
            joyconfig.button[JOY_POV_BASE | JOY_POVLEFT]     = MapVirtualKey('A', MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_POV_BASE | JOY_POVRIGHT]    = MapVirtualKey('D', MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_POV_BASE | JOY_POVFORWARD]  = MapVirtualKey('W', MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_POV_BASE | JOY_POVBACKWARD] = MapVirtualKey('S', MAPVK_VK_TO_VSC);
            g_joyconfigs.back().joy[JOYSTICKID2] = joyconfig;
        }

        g_joyconfigs.push_back(JoystickConfig());
        {
            g_joyconfigs.back().name = _T("Arrows and Mouse");
            JoyConfig& joyconfig = g_joyconfigs.back().joy[JOYSTICKID1];
            joyconfig.mouse = MAKEWORD(0, 1);
            joyconfig.button[JOY_BUTTON_BASE | 1]            = MapVirtualKey(VK_ESCAPE, MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_BUTTON_BASE | 2]            = MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_BUTTON_BASE | 4]            = MapVirtualKey(VK_CONTROL, MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_BUTTON_BASE | 5]            = MapVirtualKey(VK_MENU, MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_BUTTON_BASE | 6]            = KF_VKEY | VK_LBUTTON;
            joyconfig.button[JOY_BUTTON_BASE | 7]            = KF_VKEY | VK_RBUTTON;
            joyconfig.button[JOY_POV_BASE | JOY_POVLEFT]     = KF_EXTENDED | MapVirtualKey(VK_LEFT,  MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_POV_BASE | JOY_POVRIGHT]    = KF_EXTENDED | MapVirtualKey(VK_RIGHT, MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_POV_BASE | JOY_POVFORWARD]  = KF_EXTENDED | MapVirtualKey(VK_UP,    MAPVK_VK_TO_VSC);
            joyconfig.button[JOY_POV_BASE | JOY_POVBACKWARD] = KF_EXTENDED | MapVirtualKey(VK_DOWN,  MAPVK_VK_TO_VSC);
            g_joyconfigs.back().joy[JOYSTICKID2] = joyconfig;
        }
    }

    return TRUE;
}

WORD GetKey(UINT joystick, DWORD button, const JoystickConfig& config)
{
    JoystickConfig::joymapT::const_iterator itJoy = config.joy.find(joystick);
    if (itJoy != config.joy.end())
    {
        JoyConfig::buttonmapT::const_iterator itButton = itJoy->second.button.find(button);
        if (itButton != itJoy->second.button.end())
            return itButton->second;
    }
    return 0;
}

#define WM_TRAY (WM_USER + 30)
#define ID_TRAY 100

int getAxis(const JOYINFOEX& info, const JOYCAPS& caps, int axis)
{
    int pos = getAxisPos(info, axis);
    WORD min = getAxisMin(caps, axis);
    WORD max = getAxisMax(caps, axis);
    WORD mid = (max - min) / 2;
    return (pos - mid);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
            nid.hWnd = hWnd;
            nid.uID = ID_TRAY;
            nid.uVersion = NOTIFYICON_VERSION;
            nid.uCallbackMessage = WM_TRAY;
            nid.hIcon = (HICON) GetClassLongPtr(hWnd, GCLP_HICONSM);
            GetWindowText(hWnd, nid.szTip, ARRAYSIZE(nid.szTip));
            nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
            Shell_NotifyIcon(NIM_ADD, &nid);

            for (UINT j = 0; j < joyGetNumDevs(); ++j)
            {
                if (isJoyValid(j))
                {
                    const JOYCAPS& caps = g_joy_caps[j];
                    SetTimer(hWnd, JOY_TIMER + j, caps.wPeriodMin, NULL);
                }
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_ABOUT:
            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutDlg);
            break;

        case IDM_SELECT:
            DoJoySelectDlg(hWnd, g_joyconfigs, g_appconfigs, NULL);
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_CONTEXTMENU:
        {
            HMENU hMenu = GetSubMenu(GetMenu(hWnd), 0);
            SetMenuDefaultItem(hMenu, IDM_SELECT, FALSE);
            TrackPopupMenu(hMenu, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
            PostMessage(hWnd, WM_NULL, 0, 0);
        }
        break;

    case WM_TRAY:
        switch (lParam)
        {
        case WM_RBUTTONUP:
            if (IsWindowEnabled(hWnd))
            {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hWnd);
                SendMessage(hWnd, WM_CONTEXTMENU, (WPARAM) hWnd, MAKELPARAM(pt.x, pt.y));
            }
            else
            {
                SetForegroundWindow(hWnd);
            }
            break;

        case WM_LBUTTONUP:
            if (!IsWindowEnabled(hWnd))
            {
                SetForegroundWindow(hWnd);
            }
            break;

        case WM_LBUTTONDBLCLK:
            if (IsWindowEnabled(hWnd))
            {
                SendMessage(hWnd, WM_COMMAND, IDM_SELECT, 0);
            }
            else
            {
                SetForegroundWindow(hWnd);
            }
            break;
        }
        break;

    case JOY_BUTTON_UP:
        {
            UINT joystick = static_cast<UINT>(wParam);
            DWORD button = static_cast<DWORD>(lParam);
            HWND hFGWnd = GetForegroundWindow();
            const JoystickConfig& config = GetJoystickConfig(hFGWnd);
            WORD key = GetKey(joystick, button, config);

            if (key != 0)
                SendKey(key, false);
        }
        break;

    case JOY_BUTTON_DOWN:
        {
            UINT joystick = static_cast<DWORD>(wParam);
            DWORD button = static_cast<DWORD>(lParam);
            HWND hFGWnd = GetForegroundWindow();
            const JoystickConfig& config = GetJoystickConfig(hFGWnd);
            WORD key = GetKey(joystick, button, config);

            if (key != 0)
                SendKey(key, true);

            const JOYINFOEX& info = g_joy_info[joystick];
            if (info.dwButtons == (JOY_BUTTON11 | JOY_BUTTON12))
            {
                SetForegroundWindow(hWnd);
                DoJoySelectDlg(hWnd, g_joyconfigs, g_appconfigs, hFGWnd);
                SetForegroundWindow(hFGWnd);
            }
        }
        break;

    case WM_TIMER:
        if (IsWindowEnabled(hWnd))
        {
            UINT joystick = static_cast<UINT>(wParam) - JOY_TIMER;
            processJoyToMsg(joystick, hWnd);

            HWND hFGWnd = GetForegroundWindow();
            const JoystickConfig& config = GetJoystickConfig(hFGWnd);
            const JoyConfig* joyconfig = config.GetJoyConfig(joystick);

            if (joyconfig != nullptr && joyconfig->mouse != 0)
            {
                JOYCAPS caps;
                MMRESULT r = joyGetDevCaps(joystick, &caps, sizeof(JOYCAPS));
                const JOYINFOEX& info = g_joy_info[joystick];

                int xaxis = LOBYTE(joyconfig->mouse);
                int yaxis = HIBYTE(joyconfig->mouse);

                int sensitivity = 2048;
                int x = getAxis(info, caps, xaxis) / sensitivity;
                int y = getAxis(info, caps, yaxis) / sensitivity;

                if (GetKeyState(VK_CONTROL))
                {   // Slow down
                    x /= 2;
                    y /= 2;
                }

                if (x != 0 || y != 0)
                    SendMouse(x, y);
            }
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code here...
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        {
            NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
            nid.hWnd = hWnd;
            nid.uID = ID_TRAY;
            nid.uVersion = NOTIFYICON_VERSION_4;
            Shell_NotifyIcon(NIM_DELETE, &nid);

            PostQuitMessage(0);
        }
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK AboutDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


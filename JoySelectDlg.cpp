#include "stdafx.h"

#include "JoyUtils.h"
#include "RadJoyKey.h"

#include "resource.h"
#include <psapi.h>
#include "WinUtils.h"

namespace
{

JoystickConfigT* g_joyconfigs = nullptr;
AppConfigT g_appconfigs;
HWND g_hWndCurrent = NULL;
int previous_selection = -1;

INT_PTR CALLBACK JoySelectDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    HWND hCBApplication = GetDlgItem(hDlg, IDC_APPLICATION);
    HWND hLBSelect = GetDlgItem(hDlg, IDC_MAPSELECT);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HICON hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_RADJOYKEY));
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

            {   // init hCBApplication
                ComboBox_ResetContent(hCBApplication);
                for (AppConfigT::iterator it = g_appconfigs.begin(); it != g_appconfigs.end(); ++it)
                {
                    ComboBox_AddString(hCBApplication, it->app.c_str());
                }
            }

            {   // init hLBSelect
                for (JoystickConfigT::const_iterator it = g_joyconfigs->begin(); it != g_joyconfigs->end(); ++it)
                {
                    ListBox_AddString(hLBSelect, it->name.c_str());
                }
            }

            {
                const AppConfig& apptive = GetAppConfig(g_appconfigs, g_hWndCurrent);
                ComboBox_SelectString(hCBApplication, -1, apptive.app.c_str());
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_APPLICATION, CBN_SELCHANGE), 0);
            }

            for (UINT j = 0; j < joyGetNumDevs(); ++j)
            {
                if (isJoyValid(j))
                {
                    const JOYCAPS& caps = g_joy_caps[j];
                    SetTimer(hDlg, JOY_TIMER + j, caps.wPeriodMin, NULL);
                }
            }

            SetFocus(hLBSelect);
        }
        return (INT_PTR)FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_APPLICATION:
            {
                switch (HIWORD(wParam))
                {
                case CBN_SELENDOK:
                    {
                        if (previous_selection >= 0)
                        {
                            AppConfig& appconfig = g_appconfigs[previous_selection];

                            int index = ListBox_GetCurSel(hLBSelect);
                            if (index >= 0)
                            {
                                TCHAR str[1024];
                                ListBox_GetText(hLBSelect, index, str);
                                appconfig.config = str;
                            }
                        }
                    }
                    break;

                case CBN_SELCHANGE:
                    {
                        int selected = ComboBox_GetCurSel(hCBApplication);
                        AppConfig& appconfig = g_appconfigs[selected];

                        if (ListBox_SelectString(hLBSelect, -1, appconfig.config.c_str()) < 0)
                            ListBox_SetCurSel(hLBSelect, 0);
                        SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_MAPSELECT, LBN_SELCHANGE), 0);

                        previous_selection = selected;
                        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_APP), selected > 0);
                        EnableWindow(GetDlgItem(hDlg, IDC_DELETE_APP), selected > 0);
                    }
                    break;
                }
            }
            break;

        case IDC_MAPSELECT:
            {
                switch (HIWORD(wParam))
                {
                case LBN_SELCHANGE:
                    {
                        int selected = ListBox_GetCurSel(hLBSelect);
                        EnableWindow(GetDlgItem(hDlg, IDC_DELETE), selected > 0);
                        EnableWindow(GetDlgItem(hDlg, IDC_EDIT), selected > 0);
                    }
                    break;

                case LBN_DBLCLK:
                    SendMessage(hDlg, WM_COMMAND, IDC_EDIT, 0);
                    break;
                }
            }
            break;

        case IDC_ADD_APP:
            {
                AppConfig appconfig;
                int selected = ComboBox_GetCurSel(hCBApplication);
                TCHAR str[1024] = _T("*\\");
                if (g_hWndCurrent != NULL)
                {
                    DWORD offset = static_cast<DWORD>(_tcslen(str));
                    if (GetModuleBaseName(g_hWndCurrent, str + offset, ARRAYSIZE(str) - offset) > 0)
                        appconfig.app = str;
                }
                else if (selected >= 0)
                {
                    appconfig = g_appconfigs[selected];
                }

                if (DoApplicationDlg(hDlg, appconfig))
                {
                    AppConfigT::iterator it = FindAppConfig(g_appconfigs, appconfig.app);
                    if (it != g_appconfigs.end())
                    {
                        ComboBox_SelectString(hCBApplication, -1, it->app.c_str());
                        SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_APPLICATION, CBN_SELCHANGE), 0);
                    }
                    else
                    {
                        g_appconfigs.push_back(appconfig);
                        int selected = ComboBox_AddString(hCBApplication, appconfig.app.c_str());
                        ComboBox_SetCurSel(hCBApplication, selected);
                        SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_APPLICATION, CBN_SELCHANGE), 0);
                    }
                }
            }
            break;

        case IDC_EDIT_APP:
            {
                int selected = ComboBox_GetCurSel(hCBApplication);
                if (selected > 0)
                {
                    AppConfig& appconfig = g_appconfigs[selected];
                    if (DoApplicationDlg(hDlg, appconfig))
                    {
                        ComboBox_DeleteString(hCBApplication, selected);
                        ComboBox_InsertString(hCBApplication, selected, appconfig.app.c_str());
                        ComboBox_SetCurSel(hCBApplication, selected);
                    }
                }
            }
            break;

        case IDC_DELETE_APP:
            {
                int selected = ComboBox_GetCurSel(hCBApplication);
                if (selected > 0)
                {
                    g_appconfigs.erase(g_appconfigs.begin() + selected);
                    ComboBox_DeleteString(hCBApplication, selected);

                    ComboBox_SetCurSel(hCBApplication, 0);
                    SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_APPLICATION, CBN_SELCHANGE), 0);
                }
            }
            break;

        case IDC_ADD:
            {
                JoystickConfig joyconfig;
                if (DoJoyConfigDlg(hDlg, joyconfig))
                {
                    // TODO Check if name already exists
                    g_joyconfigs->push_back(joyconfig);
                    int selected = ListBox_AddString(hLBSelect, joyconfig.name.c_str());
                    ListBox_SetCurSel(hLBSelect, selected);
                    SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_MAPSELECT, LBN_SELCHANGE), 0);
                }
            }
            break;

        case IDC_DELETE:
            {
                int selected = ListBox_GetCurSel(hLBSelect);
                if (selected > 0)
                {
                    g_joyconfigs->erase(g_joyconfigs->begin() + selected);
                    ListBox_DeleteString(hLBSelect, selected);
                    if (selected >= ListBox_GetCount(hLBSelect))
                        selected = ListBox_GetCount(hLBSelect) - 1;
                    ListBox_SetCurSel(hLBSelect, selected);
                    SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_MAPSELECT, LBN_SELCHANGE), 0);
                }
            }
            break;

        case IDC_EDIT:
            {
                int selected = ListBox_GetCurSel(hLBSelect);
                if (selected > 0)
                {
                    JoystickConfig& joyconfig = (*g_joyconfigs)[selected];
                    if (DoJoyConfigDlg(hDlg, joyconfig))
                    {
                        // TODO Check if name already exists
                        ListBox_DeleteString(hLBSelect, selected);
                        ListBox_InsertString(hLBSelect, selected, joyconfig.name.c_str());
                        ListBox_SetCurSel(hLBSelect, selected);
                    }
                }
            }
            break;

        case IDOK:
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_APPLICATION, CBN_SELENDOK), 0);
            // fallthrough
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    case JOY_BUTTON_UP:
    case JOY_BUTTON_DOWN:
        {
            UINT keymsg = uMsg == JOY_BUTTON_DOWN ? WM_KEYDOWN : WM_KEYUP;
            switch (lParam)
            {
            case JOY_POV_BASE | JOY_POVFORWARD:
                SendMessage(hLBSelect, keymsg, VK_UP, 0);
                break;

            case JOY_POV_BASE | JOY_POVBACKWARD:
                SendMessage(hLBSelect, keymsg, VK_DOWN, 0);
                break;

            case JOY_BUTTON_BASE | 0:
                if (uMsg == JOY_BUTTON_UP)
                    SendMessage(hDlg, WM_COMMAND, IDC_EDIT, 0);
                break;

            case JOY_BUTTON_BASE | 1:
                if (uMsg == JOY_BUTTON_UP)
                    SendMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
                break;

            case JOY_BUTTON_BASE | 2:
                if (uMsg == JOY_BUTTON_UP)
                    SendMessage(hDlg, WM_COMMAND, IDOK, 0);
                break;
            }
        }
        break;

    case WM_TIMER:
        if (IsWindowEnabled(hDlg))
        {
            UINT joystick = static_cast<UINT>(wParam) - JOY_TIMER;
            processJoyToMsg(joystick, hDlg);
        }
        break;
    }
    return (INT_PTR)FALSE;
}

};

void DoJoySelectDlg(HWND hWnd, JoystickConfigT& joyconfigs, AppConfigT& appconfigs, HWND hWndCurrent)
{
    g_joyconfigs = &joyconfigs;
    g_appconfigs = appconfigs;
    g_hWndCurrent = hWndCurrent;

    if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_JOY_SELECT), hWnd, JoySelectDlg) == IDOK)
    {
        appconfigs = g_appconfigs;
    }
}

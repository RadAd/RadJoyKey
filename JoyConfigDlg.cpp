#include "stdafx.h"

#include "JoyUtils.h"
#include "WinUtils.h"
#include "RadJoyKey.h"

#include "resource.h"

int GetKeyNameTextEx(LONG lParam, LPTSTR lptString, int cchSize)
{
    WORD h = HIWORD(lParam);
    if (h & KF_VKEY)
    {
        switch (h & 0xFF)
        {
        case VK_LBUTTON:
            {
                _tcscpy_s(lptString, cchSize, _T("Left Mouse Button"));
                return (int) _tcslen(lptString);
            }

        case VK_RBUTTON:
            {
                _tcscpy_s(lptString, cchSize, _T("Right Mouse Button"));
                return (int) _tcslen(lptString);
            }

        case VK_MBUTTON:
            {
                _tcscpy_s(lptString, cchSize, _T("Middle Mouse Button"));
                return (int) _tcslen(lptString);
            }

        case VK_XBUTTON1:
            {
                _tcscpy_s(lptString, cchSize, _T("X1 Mouse Button"));
                return (int) _tcslen(lptString);
            }

        case VK_XBUTTON2:
            {
                _tcscpy_s(lptString, cchSize, _T("X2 Mouse Button"));
                return (int) _tcslen(lptString);
            }

        default:
            {
                LONG scan = MapVirtualKey(lParam, MAPVK_VK_TO_VSC);
                if (scan == 0)
                {
                    _tcscpy_s(lptString, cchSize, _T("???"));
                    return (int) _tcslen(lptString);
                }
                else
                    return GetKeyNameText(scan, lptString, cchSize);
            }
        }
    }
    else
        return GetKeyNameText(lParam, lptString, cchSize);
}

namespace
{

JoystickConfig g_joyconfig;

void SetKeyMapping(HWND hLVKeyMapping, int i, WORD scan, int j)
{
    TCHAR str[1024] = { 0 };
    //_stprintf_s(str, _T("Key %c"), wParam);
    GetKeyNameTextEx(MAKELPARAM(0, scan), str, ARRAYSIZE(str));
    ListView_SetItemText(hLVKeyMapping, i, 1, str);

    DWORD joybutton = static_cast<DWORD>(ListView_GetItemParam(hLVKeyMapping, i));

    if (scan != 0)
        g_joyconfig.joy[j].button[joybutton] = scan;
    else
        g_joyconfig.joy[j].button.erase(joybutton);
}

LRESULT CALLBACK LVWndProc(HWND hLVKeyMapping, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        {
            NMLVKEYDOWN nmlvkd = { 0 };
            nmlvkd.hdr.hwndFrom = hLVKeyMapping;
            nmlvkd.hdr.idFrom = GetWindowLong(hLVKeyMapping, GWL_ID);
            nmlvkd.hdr.code = LVN_KEYDOWN;
            nmlvkd.wVKey = (WORD) wParam;
            nmlvkd.flags = HIWORD(lParam);
            SendMessage(GetParent(hLVKeyMapping), WM_NOTIFY, nmlvkd.hdr.idFrom, (LPARAM) &nmlvkd);
        }
        return 0;

    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;

    default:
        return DefSubclassProc(hLVKeyMapping, uMsg, wParam, lParam);
    };
}

void InitLVKeyMappingJoyButtonsProc(LPTSTR pStr, DWORD dwButton, void* pData)
{
    HWND hLVKeyMapping = static_cast<HWND>(pData);
    ListView_AppendItem(hLVKeyMapping, pStr, dwButton);
}

INT_PTR CALLBACK JoyConfigDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    HWND hETName       = GetDlgItem(hDlg, IDC_NAME);
    HWND hCBJoystick   = GetDlgItem(hDlg, IDC_JOYSTICK);
    HWND hCBMouse      = GetDlgItem(hDlg, IDC_MOUSE);
    HWND hLVKeyMapping = GetDlgItem(hDlg, IDC_KEYMAPPING);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SetWindowSubclass(hLVKeyMapping, LVWndProc, 0, 0);

            Edit_SetText(hETName, g_joyconfig.name.c_str());

            {   // init hCBJoystick
                for (UINT i = 0; i < joyGetNumDevs(); ++i)
                {
                    if (isJoyValid(i))
                    {
                        TCHAR str[1024];
                        _stprintf_s(str, _T("Joystick %d"), i + 1);
                        ComboBox_AddString(hCBJoystick, str);
                    }
                }
                if (ComboBox_GetCount(hCBJoystick) == 0)
                {
                    MessageBox(hDlg, _T("No valid joysticks found."), g_szTitle, MB_OK | MB_ICONERROR);
                }
                else
                {
                    ComboBox_SetCurSel(hCBJoystick, JOYSTICKID1);
                }
            }

            {   // init hLVKeyMapping
                DWORD exstyle = ListView_GetExtendedListViewStyle(hLVKeyMapping);
                exstyle |= LVS_EX_FULLROWSELECT;
                ListView_SetExtendedListViewStyle(hLVKeyMapping, exstyle);

                RECT rect;
                GetClientRect(hLVKeyMapping, &rect);
                int cx = (rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL)) / 2;

                ListView_AppendColum(hLVKeyMapping, _T("Joystick Button"), cx);
                ListView_AppendColum(hLVKeyMapping, _T("Keyboard Button"), cx);
            }

            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_JOYSTICK, CBN_SELCHANGE), 0);

            for (UINT j = 0; j < joyGetNumDevs(); ++j)
            {
                if (isJoyValid(j))
                {
                    const JOYCAPS& caps = g_joy_caps[j];
                    SetTimer(hDlg, JOY_TIMER + j, caps.wPeriodMin, NULL);
                }
            }
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_JOYSTICK:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int joystick = ComboBox_GetCurSel(hCBJoystick);

                ComboBox_ResetContent(hCBMouse);
                ListView_DeleteAllItems(hLVKeyMapping);

                int index = ComboBox_AddString(hCBMouse, _T("None"));
                ComboBox_SetItemData(hCBMouse, index, 0);
                ComboBox_SetCurSel(hCBMouse, 0);

                if (isJoyValid(joystick))
                {
                    const JoyConfig* joyconfig = g_joyconfig.GetJoyConfig(joystick);
                    const JOYCAPS& caps = g_joy_caps[joystick];

                    {   // fill hCBMouse
                        EnableWindow(hCBMouse, TRUE);

                        TCHAR str[1024];
                        for (UINT i = 0; i < (caps.wNumAxes - 1); i += 2)
                        {
                            _stprintf_s(str, _T("Axis %s %s"), getAxisName(i), getAxisName(i+1));
                            int index = ComboBox_AddString(hCBMouse, str);
                            ComboBox_SetItemData(hCBMouse, index, MAKEWORD(i, i+1));

                            _stprintf_s(str, _T("Axis %s %s"), getAxisName(i+1), getAxisName(i));
                            /*int*/ index = ComboBox_AddString(hCBMouse, str);
                            ComboBox_SetItemData(hCBMouse, index, MAKEWORD(i+1, i));
                        }

                        if (joyconfig != nullptr)
                        {
                            int index = ComboBox_FindItemParam(hCBMouse, -1, joyconfig->mouse);
                            ComboBox_SetCurSel(hCBMouse, index);
                        }
                    }

                    {   // fill hLVKeyMapping
                        EnumJoyButtons(&caps, InitLVKeyMappingJoyButtonsProc, hLVKeyMapping);

                        for (int i = 0; i < ListView_GetItemCount(hLVKeyMapping); ++i)
                        {
                            TCHAR str[1024] = { 0 };

                            if (joyconfig != nullptr)
                            {
                                DWORD joybutton = static_cast<DWORD>(ListView_GetItemParam(hLVKeyMapping, i));
                                JoyConfig::buttonmapT::const_iterator it = joyconfig->button.find(joybutton);

                                if (it != g_joyconfig.joy[joystick].button.end())
                                {
                                    //_stprintf_s(str, _T("Key %c"), wParam);
                                    GetKeyNameTextEx(MAKELPARAM(0, it->second), str, ARRAYSIZE(str));
                                }
                            }

                            ListView_SetItemText(hLVKeyMapping, i, 1, str);
                        }
                    }
                }
                else
                {
                    EnableWindow(hCBMouse, FALSE);
                }
            }
            break;

        case IDC_SPECIAL:
            {
                int joystick = ComboBox_GetCurSel(hCBJoystick);
                int focused = ListView_GetNextItem(hLVKeyMapping, -1, LVNI_FOCUSED);
                if (joystick >= 0 && focused >= 0)
                {
                    HMENU hMenu = CreatePopupMenu();
                    WORD special[] = {
                        KF_VKEY | VK_LBUTTON,
                        KF_VKEY | VK_RBUTTON,
                        KF_VKEY | VK_MBUTTON,
                        KF_VKEY | VK_XBUTTON1,
                        KF_VKEY | VK_XBUTTON2,
                        0 };

                    for (int i = 0; special[i] != 0; ++i)
                    {
                        TCHAR str[1024] = _T("???");
                        GetKeyNameTextEx(MAKELPARAM(0, special[i]), str, ARRAYSIZE(str));
                        AppendMenu(hMenu, MF_ENABLED | MF_STRING, i + 1, str);
                    }
                    RECT rect;
                    GetWindowRect((HWND) lParam, &rect);
                    int i = TrackPopupMenu(hMenu, TPM_RETURNCMD, rect.left, rect.bottom, 0, hDlg, nullptr);
                    if (i > 0)
                        SetKeyMapping(hLVKeyMapping, focused, special[i - 1], joystick);
                }
            }
            break;

        case IDC_CLEAR:
            {
                int joystick = ComboBox_GetCurSel(hCBJoystick);
                int focused = ListView_GetNextItem(hLVKeyMapping, -1, LVNI_FOCUSED);
                if (joystick >= 0 && focused >= 0)
                    SetKeyMapping(hLVKeyMapping, focused, 0, joystick);
            }
            break;

        case IDC_CLEAR_ALL:
            {
                int joystick = ComboBox_GetCurSel(hCBJoystick);
                for (int i = 0; i < ListView_GetItemCount(hLVKeyMapping); ++i)
                {
                    SetKeyMapping(hLVKeyMapping, i, 0, joystick);
                }
            }
            break;

        case IDOK:
            {
                TCHAR str[1024];
                Edit_GetText(hETName, str, ARRAYSIZE(str));
                g_joyconfig.name = str;
                if (g_joyconfig.name.empty())
                {
                    MessageBox(hDlg, _T("The name cannot be empty."), g_szTitle, MB_OK | MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }

                int joystick = ComboBox_GetCurSel(hCBJoystick);
                if (isJoyValid(joystick))
                {
                    int i = ComboBox_GetCurSel(hCBMouse);
                    g_joyconfig.joy[joystick].mouse = (WORD) ComboBox_GetItemData(hCBMouse, i);
                }
            }
            // fallthrough
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            switch (pnmh->idFrom)
            {
            case IDC_KEYMAPPING:
                switch (pnmh->code)
                {
                case LVN_KEYDOWN:
                    {
                        LPNMLVKEYDOWN pnmlvkd = (LPNMLVKEYDOWN) pnmh;
                        int joystick = ComboBox_GetCurSel(hCBJoystick);
                        int focused = ListView_GetNextItem(hLVKeyMapping, -1, LVNI_FOCUSED);
                        if (joystick >= 0 && focused >= 0)
                            //SetKeyMapping(hLVKeyMapping, focused, MapVirtualKey(pnmlvkd->wVKey, MAPVK_VK_TO_VSC), joystick);
                            SetKeyMapping(hLVKeyMapping, focused, pnmlvkd->flags, joystick);
                        return (INT_PTR)TRUE;
                    }
                    break;
                }
                break;
            }
        }
        break;

    case JOY_BUTTON_DOWN:
        {
            int joystick = ComboBox_GetCurSel(hCBJoystick);
            if (wParam != joystick)
            {
                ComboBox_SetCurSel(hCBJoystick, wParam);
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_JOYSTICK, CBN_SELCHANGE), reinterpret_cast<LPARAM>(hCBJoystick));
            }

            if (wParam == joystick)
            {
                SetFocus(hLVKeyMapping);

                LVFINDINFO findinfo = { 0 };
                findinfo.flags = LVFI_PARAM;
                findinfo.lParam = lParam;
                int i = ListView_FindItem(hLVKeyMapping, -1, &findinfo);

                if (i >= 0)
                {
                    ListView_SetItemState(hLVKeyMapping, i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                    ListView_EnsureVisible(hLVKeyMapping, i, FALSE);
                }
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

    case WM_DESTROY:
        RemoveWindowSubclass(hLVKeyMapping, LVWndProc, 0);
        break;
    }
    return (INT_PTR)FALSE;
}

};

bool DoJoyConfigDlg(HWND hWnd, JoystickConfig& joyconfig)
{
    g_joyconfig = joyconfig;
    if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_JOY_CONFIG), hWnd, JoyConfigDlg) == IDOK)
    {
        joyconfig = g_joyconfig;
        return true;
    }
    else
        return false;
}

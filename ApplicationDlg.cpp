#include "stdafx.h"

#include "RadJoyKey.h"

#include "resource.h"

namespace
{

AppConfig g_appconfig;

INT_PTR CALLBACK ApplicationDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hExecutable  = GetDlgItem(hDlg, IDC_EXECUTABLE);
    HWND hWindowTitle = GetDlgItem(hDlg, IDC_WINDOW_TITLE);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            Edit_SetText(hExecutable, g_appconfig.app.c_str());
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                TCHAR str[1024];
                Edit_GetText(hExecutable, str, ARRAYSIZE(str));
                g_appconfig.app = str;
                if (g_appconfig.app.empty())
                {
                    MessageBox(hDlg, _T("The executable cannot be empty."), g_szTitle, MB_OK | MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
            }
            // fallthrough
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        };
    };

    return (INT_PTR)FALSE;
}

};

bool DoApplicationDlg(HWND hWnd, AppConfig& appconfig)
{
    g_appconfig = appconfig;
    if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_APPLICATION), hWnd, ApplicationDlg) == IDOK)
    {
        appconfig = g_appconfig;
        return true;
    }
    else
        return false;
}

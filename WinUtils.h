#pragma once

inline int ListView_AppendColum(HWND hWnd, LPCTSTR pszText, int cx)
{
    LV_COLUMN column = { 0 };
    column.mask = LVCF_TEXT | LVCF_WIDTH;
    column.pszText = const_cast<LPTSTR>(pszText);
    column.cx = cx;
    return ListView_InsertColumn(hWnd, INT_MAX, &column);
}

inline int ListView_AppendItem(HWND hWnd, LPCTSTR pszText, LPARAM lParam)
{
    LV_ITEM item = { 0 };
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = INT_MAX;
    item.pszText = const_cast<LPTSTR>(pszText);
    item.lParam = lParam;
    return ListView_InsertItem(hWnd, &item);
}

inline LPARAM ListView_GetItemParam(HWND hWnd, int i)
{
    LV_ITEM item = { 0 };
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = i;
    if (ListView_GetItem(hWnd, &item))
        return item.lParam;
    else
        return -1;
}

inline BOOL ListView_SetItemParam(HWND hWnd, int i, LPARAM lParam)
{
    LV_ITEM item = { 0 };
    item.mask = LVIF_PARAM;
    item.iItem = i;
    item.lParam = lParam;
    return ListView_GetItem(hWnd, &item);
}

// Like ComboBox_FindItemData
inline int ComboBox_FindItemParam(HWND hWnd, int i, LPARAM lParam)
{
    int j = i;

    do
    {
        ++j;
        if (j >= ComboBox_GetCount(hWnd))
        {
            if (i == -1)
                break;
            else
                j = 0;
        }
        if (ComboBox_GetItemData(hWnd, j) == lParam)
            return j;
    }
    while (j != i);

    return CB_ERR;
}

#ifdef _PSAPI_H_    // #include <psapi.h>

inline DWORD GetModuleBaseName(HWND hWnd, LPWSTR lpBaseName, DWORD nSize)
{
    HINSTANCE hInst = NULL;//GetWindowInstance(hWnd);
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    DWORD ret = GetModuleBaseName(hProcess, hInst, lpBaseName, nSize);
    CloseHandle(hProcess);
    return ret;
}

inline DWORD GetModuleFileNameEx(HWND hWnd, LPWSTR lpFileName, DWORD nSize)
{
    HINSTANCE hInst = NULL;//GetWindowInstance(hWnd);
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    DWORD ret = GetModuleFileNameEx(hProcess, hInst, lpFileName, nSize);
    CloseHandle(hProcess);
    return ret;
}

#endif

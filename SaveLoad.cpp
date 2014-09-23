#include "stdafx.h"
#include "RadJoyKey.h"

#include "JoyUtils.h"

HKEY RegCreateAppKey()
{
    HKEY key = NULL;
    RegCreateKey(HKEY_CURRENT_USER, _T("Software\\RadSoft\\RadJoyKey"), &key);
    return key;
}

void Save(HKEY key, const TCHAR* str, const JoyConfig::buttonmapT& button, DWORD b)
{
    JoyConfig::buttonmapT::const_iterator it = button.find(b);
    if (it != button.end())
    {
        DWORD data = it->second;
        RegSetValueEx(key, str, 0, REG_DWORD, (const BYTE*) &data, sizeof(data));
    }
    else
    {
        RegDeleteValue(key, str);
    }
}

void Load(HKEY key, const TCHAR* str, JoyConfig::buttonmapT& button, DWORD b)
{
    DWORD data = 0;
    DWORD type = 0;
    DWORD size = sizeof(data);
    if (RegQueryValueEx(key, str, 0, &type, (BYTE*) &data, &size) == ERROR_SUCCESS && type == REG_DWORD)
    {
        button[b] = static_cast<WORD>(data);
    }
    else
    {
        JoyConfig::buttonmapT::const_iterator it = button.find(b);
        if (it != button.end())
            button.erase(it);
    }
}

struct SaveJoyButtonsData
{
    HKEY key;
    const JoyConfig::buttonmapT* button;
};

struct LoadJoyButtonsData
{
    HKEY key;
    JoyConfig::buttonmapT* button;
};

void SaveJoyButtonsProc(LPTSTR pStr, DWORD dwButton, void* pData)
{
    const SaveJoyButtonsData* pSaveJoyButtonsData = static_cast<const SaveJoyButtonsData*>(pData);
    Save(pSaveJoyButtonsData->key, pStr, *pSaveJoyButtonsData->button, dwButton);
}

void LoadJoyButtonsProc(LPTSTR pStr, DWORD dwButton, void* pData)
{
    const LoadJoyButtonsData* pLoadJoyButtonsData = static_cast<const LoadJoyButtonsData*>(pData);
    Load(pLoadJoyButtonsData->key, pStr, *pLoadJoyButtonsData->button, dwButton);
}

void Save(HKEY key, const JoyConfig& joyconfig)
{
    DWORD data = joyconfig.mouse;
    RegSetValueEx(key, _T("mouse"), 0, REG_DWORD, (const BYTE*) &data, sizeof(data));

    SaveJoyButtonsData dSaveJoyButtonsData;
    dSaveJoyButtonsData.key = key;
    dSaveJoyButtonsData.button = &joyconfig.button;
    EnumJoyButtons(SaveJoyButtonsProc, &dSaveJoyButtonsData);
}

void Load(HKEY key, JoyConfig& joyconfig)
{
    DWORD data = 0;
    DWORD type = 0;
    DWORD size = sizeof(data);
    if (RegQueryValueEx(key, _T("mouse"), 0, &type, (BYTE*) &data, &size) == ERROR_SUCCESS && type == REG_DWORD)
    {
        joyconfig.mouse = static_cast<WORD>(data);
    }
    else
    {
        joyconfig.mouse = 0;
    }

    LoadJoyButtonsData dLoadJoyButtonsData;
    dLoadJoyButtonsData.key = key;
    dLoadJoyButtonsData.button = &joyconfig.button;
    EnumJoyButtons(LoadJoyButtonsProc, &dLoadJoyButtonsData);
}

void Save(HKEY configkey, const JoystickConfig& config)
{
    LONG err = 0;
    HKEY thisconfigkey = NULL;
    err = RegCreateKey(configkey, config.name.c_str(), &thisconfigkey);

    TCHAR str[1024];

    for (JoystickConfig::joymapT::const_iterator itJoy = config.joy.begin(); itJoy != config.joy.end(); ++itJoy)
    {
        HKEY joykey = NULL;
        _stprintf_s(str, _T("Joy %d"), itJoy->first + 1);
        err = RegCreateKey(thisconfigkey, str, &joykey);

        Save(joykey, itJoy->second);

        err = RegCloseKey(joykey);
    }

    DWORD dwIndex = 0;
    while (RegEnumKey(thisconfigkey, dwIndex, str, ARRAYSIZE(str)) == ERROR_SUCCESS)
    {
        UINT j = 0;
        _stscanf_s(str, _T("Joy %d"), &j);
        if (j > 0 && config.joy.find(j - 1) == config.joy.end())
        {
            err = RegDeleteTree(thisconfigkey, str);
            if (FAILED(err))
            {
                MessageBox(NULL, _T("Error deleting registry."), g_szTitle, MB_OK | MB_ICONERROR);
                ++dwIndex;
            }
        }
        else
        {
            ++dwIndex;
        }
    }

    err = RegCloseKey(thisconfigkey);
}

void Load(HKEY configkey, JoystickConfig& config)
{
    TCHAR str[1024];
    DWORD dwIndex = 0;
    while (RegEnumKey(configkey, dwIndex, str, ARRAYSIZE(str)) == ERROR_SUCCESS)
    {
        UINT j = 0;
        _stscanf_s(str, _T("Joy %d"), &j);
        if (j > 0)
        {
            HKEY joykey = NULL;
            RegOpenKey(configkey, str, &joykey);

            Load(joykey, config.joy[j - 1]);

            RegCloseKey(joykey);
        }

        ++dwIndex;
    }
}

void Save(const JoystickConfigT& configs)
{
    LONG err = 0;
    HKEY appkey = RegCreateAppKey();
    HKEY configkey = NULL;
    err = RegCreateKey(appkey, _T("Config"), &configkey);

    for (JoystickConfigT::const_iterator it = configs.begin() + 1; it != configs.end(); ++it)
    {
        Save(configkey, *it);
    }

    TCHAR str[1024];
    DWORD dwIndex = 0;
    while (RegEnumKey(configkey, dwIndex, str, ARRAYSIZE(str)) == ERROR_SUCCESS)
    {
        if (FindJoystickConfig(configs, str) == configs.end())
        {
            err = RegDeleteTree(configkey, str);
            if (FAILED(err))
            {
                MessageBox(NULL, _T("Error deleting registry."), g_szTitle, MB_OK | MB_ICONERROR);
                ++dwIndex;
            }
        }
        else
        {
            ++dwIndex;
        }
    }

    err = RegCloseKey(configkey);
    err = RegCloseKey(appkey);
}

void Load(JoystickConfigT& configs)
{
    HKEY appkey = RegCreateAppKey();
    HKEY configkey = NULL;
    RegOpenKey(appkey, _T("Config"), &configkey);

    TCHAR str[1024];
    DWORD dwIndex = 0;
    while (RegEnumKey(configkey, dwIndex, str, ARRAYSIZE(str)) == ERROR_SUCCESS)
    {
        HKEY thisconfigkey = NULL;
        RegOpenKey(configkey, str, &thisconfigkey);

        configs.push_back(JoystickConfig());
        configs.back().name = str;

        Load(thisconfigkey, configs.back());

        RegCloseKey(thisconfigkey);

        ++dwIndex;
    }

    RegCloseKey(configkey);
    RegCloseKey(appkey);
}

void Save(HKEY configkey, const AppConfig& appconfig)
{
    RegSetValueEx(configkey, appconfig.app.c_str(), 0, REG_SZ, (const BYTE*) appconfig.config.c_str(), ((DWORD) appconfig.config.length() + 1) * sizeof(std::wstring::value_type));
}

void Save(const AppConfigT& appconfigs)
{
    LONG err = 0;
    HKEY appkey = RegCreateAppKey();
    HKEY configkey = NULL;
    err = RegCreateKey(appkey, _T("App"), &configkey);

    for (AppConfigT::const_iterator it = appconfigs.begin(); it != appconfigs.end(); ++it)
    {
        Save(configkey, *it);
    }

    TCHAR str[1024];
    DWORD strlength = ARRAYSIZE(str);
    DWORD dwIndex = 0;
    DWORD type;
    TCHAR value[1024];
    DWORD valuelength = ARRAYSIZE(value) * sizeof(TCHAR);
    while (RegEnumValue(configkey, dwIndex, str, &strlength, 0, &type, (BYTE*) &value, &valuelength) == ERROR_SUCCESS)
    {
        if (type == REG_SZ && FindAppConfig(appconfigs, str) == appconfigs.end())
        {
            err = RegDeleteValue(configkey, str);
            if (FAILED(err))
            {
                MessageBox(NULL, _T("Error deleting registry."), g_szTitle, MB_OK | MB_ICONERROR);
                ++dwIndex;
            }
        }
        else
        {
            ++dwIndex;
        }

        strlength = ARRAYSIZE(str);
        valuelength = ARRAYSIZE(value) * sizeof(TCHAR);
    }

    err = RegCloseKey(configkey);
    err = RegCloseKey(appkey);
}

static AppConfig& GetAppConfig(AppConfigT& appconfigs, const std::wstring& app)
{
    AppConfigT::iterator it = FindAppConfig(appconfigs, app);

    if (it == appconfigs.end())
    {
        appconfigs.push_back(AppConfig());
        appconfigs.back().app = app;
        return appconfigs.back();
    }
    else
        return *it;
}


void Load(AppConfigT& appconfigs)
{
    HKEY appkey = RegCreateAppKey();
    HKEY configkey = NULL;
    RegOpenKey(appkey, _T("App"), &configkey);

    TCHAR str[1024];
    DWORD strlength = ARRAYSIZE(str);
    DWORD dwIndex = 0;
    DWORD type;
    TCHAR value[1024];
    DWORD valuelength = ARRAYSIZE(value) * sizeof(TCHAR);
    while (RegEnumValue(configkey, dwIndex, str, &strlength, 0, &type, (BYTE*) &value, &valuelength) == ERROR_SUCCESS)
    {
        AppConfig& appconfig = GetAppConfig(appconfigs, str);
        appconfig.config = value;

        ++dwIndex;
        strlength = ARRAYSIZE(str);
        valuelength = ARRAYSIZE(value) * sizeof(TCHAR);
    }

    RegCloseKey(configkey);
    RegCloseKey(appkey);
}

#pragma once

#include <string>
#include <map>
#include <vector>

#define JOY_TIMER 1
#define KF_VKEY       0x0400

extern HINSTANCE g_hInst;
extern TCHAR g_szTitle[];

struct JoyConfig
{
    JoyConfig()
        : mouse(0)
    {
    }

    WORD mouse;
    typedef std::map<DWORD, WORD> buttonmapT;
    buttonmapT button;  // Map joystick button to keyboard scan code
};

struct JoystickConfig
{
    std::wstring name;

    typedef std::map<UINT, JoyConfig> joymapT;
    joymapT joy;

    inline const JoyConfig* GetJoyConfig(UINT joystick) const
    {
        JoystickConfig::joymapT::const_iterator itJoy = joy.find(joystick);
        if (itJoy != joy.end())
        {
            return &itJoy->second;
        }
        return nullptr;
    }
};

typedef std::vector<JoystickConfig> JoystickConfigT;

struct AppConfig
{
    std::wstring app;
    std::wstring config;
};

typedef std::vector<AppConfig> AppConfigT;

const AppConfig& GetAppConfig(const AppConfigT& appconfigs, HWND hWnd);
AppConfigT::iterator FindAppConfig(AppConfigT& appconfigs, const std::wstring& app);
AppConfigT::const_iterator FindAppConfig(const AppConfigT& appconfigs, const std::wstring& app);
JoystickConfigT::const_iterator FindJoystickConfig(const JoystickConfigT& joyconfigs, const std::wstring& name);

void DoJoySelectDlg(HWND hWnd, JoystickConfigT& joyconfigs, AppConfigT& appconfigs, HWND hWndCurrent);   // See JoySelectDlg.cpp
bool DoJoyConfigDlg(HWND hWnd, JoystickConfig& joyconfig);  // See JoyConfigDlg.cpp
bool DoApplicationDlg(HWND hWnd, AppConfig& appconfig);     // See ApplicationDlg.cpp

void Save(const JoystickConfigT& joyconfigs);   // See SaveLoad.cpp
void Load(JoystickConfigT& joyconfigs);   // See SaveLoad.cpp

void Save(const AppConfigT& appconfigs);   // See SaveLoad.cpp
void Load(AppConfigT& appconfigs);   // See SaveLoad.cpp

#pragma once

#define JOY_MAX_AXIS 6

extern JOYCAPS* g_joy_caps;
extern JOYINFOEX* g_joy_info;

inline bool isJoyValid(UINT j)
{
    return j < joyGetNumDevs() && g_joy_caps[j].wMid != g_joy_caps[j].wPid;
}

inline bool isButtonDown(DWORD dwButtons, int b)
{
    int f = 1 << b;
    return (dwButtons & f) == f;
}

inline DWORD getAxisPos(const JOYINFOEX& info, UINT axis)
{
    switch (axis)
    {
    case 0:   return info.dwXpos;
    case 1:   return info.dwYpos;
    case 2:   return info.dwZpos;
    case 3:   return info.dwRpos;
    case 4:   return info.dwUpos;
    case 5:   return info.dwVpos;
    default:  return 0;
    }
}

inline WORD getAxisMin(const JOYCAPS& caps, UINT axis)
{
    switch (axis)
    {
    case 0:   return caps.wXmin;
    case 1:   return caps.wYmin;
    case 2:   return caps.wZmin;
    case 3:   return caps.wRmin;
    case 4:   return caps.wUmin;
    case 5:   return caps.wVmin;
    default:  return 0;
    }
}

inline WORD getAxisMax(const JOYCAPS& caps, UINT axis)
{
    switch (axis)
    {
    case 0:   return caps.wXmax;
    case 1:   return caps.wYmax;
    case 2:   return caps.wZmax;
    case 3:   return caps.wRmax;
    case 4:   return caps.wUmax;
    case 5:   return caps.wVmax;
    default:  return 0;
    }
}

const TCHAR* getJoyErr(MMRESULT r);
const TCHAR* getAxisName(UINT axis);

#define JOY_MSG             (WM_USER + 156)
#define JOY_BUTTON_UP       (JOY_MSG + 1)
#define JOY_BUTTON_DOWN     (JOY_MSG + 2)

#define JOY_BUTTON_BASE     0x40
#define JOY_POV_BASE        0x100
#define JOY_AXIS_POS_BASE   0x80
#define JOY_AXIS_NEG_BASE   0xC0

void initJoyCaps();
void processJoyToMsg(UINT j, HWND hWnd);

typedef void EnumJoyButtonsProc(LPTSTR pStr, DWORD dwButton, void* pData);

void EnumJoyButtons(const JOYCAPS* caps, EnumJoyButtonsProc pProc, void* pData);
void EnumJoyButtons(EnumJoyButtonsProc pProc, void* pData);

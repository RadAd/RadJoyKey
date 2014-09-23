#include "stdafx.h"
#include "JoyUtils.h"

JOYCAPS* g_joy_caps = nullptr;
JOYINFOEX* g_joy_info = nullptr;

JOYINFOEX g_joy_init =  { sizeof(JOYINFOEX), JOY_RETURNALL, 32767, 32767, 32767, 32767, 32767, 32767, 0, 0, JOY_POVCENTERED, 0, 0 };

template<class E>
E axisSelect(UINT vValue, UINT vMin, UINT vMax, UINT vTolerance, E rLow, E rMid, E rHigh)
{
    UINT vMid = (vMax + vMin) / 2;
    if (vValue < (vMid - vTolerance))
        return rLow;
    else if (vValue > (vMid + vTolerance))
        return rHigh;
    else
        return rMid;
}

const TCHAR* getJoyErr(MMRESULT r)
{
    switch (r)
    {
    case JOYERR_NOERROR:      return L"";
    case JOYERR_PARMS:        return L"bad parameters";
    case JOYERR_NOCANDO:      return L"request not completed";
    case JOYERR_UNPLUGGED:    return L"joystick is unplugged";
    default:                  return L"Unknown";
    }
}

const TCHAR* getAxisName(UINT axis)
{
    switch (axis)
    {
    case 0:   return L"X";
    case 1:   return L"Y";
    case 2:   return L"Z";
    case 3:   return L"R";
    case 4:   return L"U";
    case 5:   return L"V";
    default:  return nullptr;
    }
}

bool isPovDown(DWORD dwPOV, DWORD b)
{
    if (dwPOV == JOY_POVCENTERED)
        return false;

    DWORD min = dwPOV - 6750;
    DWORD max = dwPOV + 6750;

    if (min > 36000)
    {
        min += 36000;
        max += 36000;
        b += 36000;
    }

    return (b >= min && b <= max);
}

void initJoyCaps()
{
    UINT num = joyGetNumDevs();
    g_joy_caps = static_cast<JOYCAPS*>(malloc(sizeof(JOYCAPS) * num));
    g_joy_info = static_cast<JOYINFOEX*>(malloc(sizeof(JOYINFOEX) * num));

    for (UINT j = 0; j < num; ++j)
    {
        MMRESULT r = joyGetDevCaps(j, g_joy_caps + j, sizeof(JOYCAPS));
        if (r != JOYERR_NOERROR)
            g_joy_caps[j].wMid = g_joy_caps[j].wPid = r;

        g_joy_info[j] = g_joy_init;
    }
}

void processJoyToMsg(UINT j, HWND hWnd)
{
    if (!isJoyValid(j))
        return;

    const JOYCAPS& caps = g_joy_caps[j];
    JOYINFOEX& thisInfo = g_joy_info[j];
    JOYINFOEX lastInfo = thisInfo;
    MMRESULT r = joyGetPosEx(j, &thisInfo);

    if (r == JOYERR_NOERROR) 
    {
        if (lastInfo.dwButtons != thisInfo.dwButtons)
        {
            for (UINT b = 0; b < caps.wNumButtons; ++b)
            {
                if (isButtonDown(lastInfo.dwButtons, b) != isButtonDown(thisInfo.dwButtons, b))
                {
                    DWORD bmsg = JOY_BUTTON_BASE | b;
                    if (isButtonDown(thisInfo.dwButtons, b))
                        SendMessage(hWnd, JOY_BUTTON_DOWN, j, bmsg);
                    else
                        SendMessage(hWnd, JOY_BUTTON_UP, j, bmsg);
                }
            }
        }

        if (caps.wCaps & JOYCAPS_HASPOV)
        {
            int povButtons[4] = { JOY_POVFORWARD, JOY_POVRIGHT, JOY_POVBACKWARD, JOY_POVLEFT };

            for (int i = 0; i < 4; ++i)
            {
                if (isPovDown(lastInfo.dwPOV, povButtons[i]) != isPovDown(thisInfo.dwPOV, povButtons[i]))
                {
                    DWORD bmsg = JOY_POV_BASE | povButtons[i];
                    if (isPovDown(thisInfo.dwPOV, povButtons[i]))
                        SendMessage(hWnd, JOY_BUTTON_DOWN, j, bmsg);
                    else
                        SendMessage(hWnd, JOY_BUTTON_UP, j, bmsg);
                }
            }
        }

        for (UINT a = 0; a < caps.wNumAxes; ++a)
        {
            DWORD lastPos = getAxisPos(lastInfo, a);
            DWORD thisPos = getAxisPos(thisInfo, a);
            WORD min = getAxisMin(caps, a);
            WORD max = getAxisMax(caps, a);
            WORD mid = (min + max)/2;
            DWORD tolerance = (max - mid) * 10 / 100;

            int lastButton = axisSelect(lastPos, min, max, tolerance, -1, 0, 1);
            int thisButton = axisSelect(thisPos, min, max, tolerance, -1, 0, 1);

            if (lastButton != thisButton)
            {
                if (lastButton == -1)
                {
                    SendMessage(hWnd, JOY_BUTTON_UP, j, JOY_AXIS_NEG_BASE | a);
                }
                else if (lastButton == 1)
                {
                    SendMessage(hWnd, JOY_BUTTON_UP, j, JOY_AXIS_POS_BASE | a);
                }

                if (thisButton == -1)
                {
                    SendMessage(hWnd, JOY_BUTTON_DOWN, j, JOY_AXIS_NEG_BASE | a);
                }
                else if (thisButton == 1)
                {
                    SendMessage(hWnd, JOY_BUTTON_DOWN, j, JOY_AXIS_POS_BASE | a);
                }
            }
        }

        lastInfo = thisInfo;
    }
}

void EnumJoyButtons(const JOYCAPS* caps, EnumJoyButtonsProc pProc, void* pData)
{
    if (caps != nullptr)
    {
        TCHAR str[1024];
        for (UINT i = 0; i < caps->wNumAxes; ++i)
        {
            _stprintf_s(str, _T("Analog %s +"), getAxisName(i));
            pProc(str, JOY_AXIS_POS_BASE | i, pData);

            _stprintf_s(str, _T("Analog %s -"), getAxisName(i));
            pProc(str, JOY_AXIS_NEG_BASE | i, pData);
        }

        if (caps->wCaps & JOYCAPS_HASPOV)
        {
            pProc(_T("Pov Left"),  JOY_POV_BASE | JOY_POVLEFT, pData);
            pProc(_T("Pov Right"), JOY_POV_BASE | JOY_POVRIGHT, pData);
            pProc(_T("Pov Up"),    JOY_POV_BASE | JOY_POVFORWARD, pData);
            pProc(_T("Pov Down"),  JOY_POV_BASE | JOY_POVBACKWARD, pData);
        }

        for (UINT i = 0; i < caps->wNumButtons; ++i)
        {
            _stprintf_s(str, _T("Button %d"), i + 1);
            pProc(str,  JOY_BUTTON_BASE + i, pData);
        }
    }
}

void EnumJoyButtons(EnumJoyButtonsProc pProc, void* pData)
{
    {
        TCHAR str[1024];
        for (UINT i = 0; i < JOY_MAX_AXIS; ++i)
        {
            _stprintf_s(str, _T("Analog %s +"), getAxisName(i));
            pProc(str, JOY_AXIS_POS_BASE | i, pData);

            _stprintf_s(str, _T("Analog %s -"), getAxisName(i));
            pProc(str, JOY_AXIS_NEG_BASE | i, pData);
        }

        {
            pProc(_T("Pov Left"),  JOY_POV_BASE | JOY_POVLEFT, pData);
            pProc(_T("Pov Right"), JOY_POV_BASE | JOY_POVRIGHT, pData);
            pProc(_T("Pov Up"),    JOY_POV_BASE | JOY_POVFORWARD, pData);
            pProc(_T("Pov Down"),  JOY_POV_BASE | JOY_POVBACKWARD, pData);
        }

        for (UINT i = 0; i < 32; ++i)
        {
            _stprintf_s(str, _T("Button %d"), i + 1);
            pProc(str,  JOY_BUTTON_BASE + i, pData);
        }
    }
}

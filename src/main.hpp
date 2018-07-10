// Free Shooter
// Copyright (c) 2009, 2010, 2018 Henry++

#ifndef __MAIN_H__
#define __MAIN_H__

#include <windows.h>
#include <commctrl.h>
#include "resource.hpp"
#include "app.hpp"

#define WM_TRAYICON WM_APP + 1
#define UID 1337

#define LANG_MENU 4
#define SETTINGS_MENU 6
#define FILENAME_MENU 4
#define FORMAT_MENU 5

#define WND_SLEEP 150
#define JPEG_QUALITY 100

#define PEN_COLOR RGB(0,0,0)
#define PEN_COLOR_BK RGB(255,255,255)

#define FILE_FORMAT_DATE L"yyyy-MM-dd"
#define FILE_FORMAT_TIME L"HH-mm-ss"

#define HOTKEY_ID_FULLSCREEN 1
#define HOTKEY_ID_WINDOW 2
#define HOTKEY_ID_REGION 3

#define HOTKEY_FULLSCREEN MAKEWORD (VK_SNAPSHOT, 0)
#define HOTKEY_WINDOW MAKEWORD (VK_SNAPSHOT, HOTKEYF_CONTROL)
#define HOTKEY_REGION MAKEWORD (VK_SNAPSHOT, HOTKEYF_ALT)

#define DUMMY_CLASS_DLG L"DummyDlg"
#define REGION_CLASS_DLG L"RegionDlg"

#define DEFAULT_DIRECTORY L"%userprofile%\\Desktop"

// libs
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")

struct STATIC_DATA
{
	WCHAR default_folder[MAX_PATH] = {0};

	HWND hdummy = nullptr;
	HWND hregion = nullptr;
	HWND hactive = nullptr;

	HANDLE hregion_mutex = nullptr;
};

struct IMAGE_FORMAT
{
	WCHAR ext[16] = {0};

	CLSID clsid = {0};
};

enum EnumScreenshot
{
	ScreenshotFullscreen,
	ScreenshotWindow,
	ScreenshotRegion,
};

enum EnumImageName
{
	NameIndex,
	NameDate,
};

enum EnumImageFormat
{
	FormatBitmap,
	FormatJpeg,
	FormatPng,
	FormatGif,
	FormatTiff,
};

typedef HRESULT (WINAPI *DWMGWA) (HWND, DWORD, PVOID, DWORD); // DwmGetWindowAttribute
typedef HRESULT (WINAPI *DWMSWA) (HWND, DWORD, LPCVOID, DWORD); // DwmSetWindowAttribute

#endif // __MAIN_H__

// Free Shooter
// Copyright (c) 2009-2021 Henry++

#pragma once

#include "routine.h"

#include "resource.h"
#include "app.h"

#define UID 1337

#define LANG_MENU 6
#define SETTINGS_MENU 6
#define FILENAME_MENU 0
#define FORMAT_MENU 1
#define DELAY_MENU 4

#define WND_SLEEP 150
#define JPEG_QUALITY 95
#define BLEND 1.5
#define START_IDX 1

#define REGION_PEN_COLOR RGB(255,0,0)
#define REGION_PEN_DRAW_COLOR RGB(0,255,0)
#define REGION_COLOR_BK RGB(0,0,0)
#define REGION_BLEND 90

#define FILE_FORMAT_NAME_PREFIX L"sshot-"
#define FILE_FORMAT_NAME_FORMAT L"%s%03d"
#define FILE_FORMAT_DATE_FORMAT_1 L"yyyy-MM-dd"
#define FILE_FORMAT_DATE_FORMAT_2 L"HH-mm-ss"

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
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "winmm.lib")

typedef struct _STATIC_DATA
{
	PR_ARRAY formats;
	PR_STRING default_folder;

	HWND hregion;
	HWND hregion_tran;

	HANDLE hregion_mutex;
} STATIC_DATA, *PSTATIC_DATA;

typedef struct _IMAGE_FORMAT
{
	WCHAR ext[8];
	GUID guid;
} IMAGE_FORMAT, *PIMAGE_FORMAT;

typedef struct _ENUM_INFO
{
	HWND hroot;
	PRECT rect;

	BOOLEAN is_menu;
} ENUM_INFO, *PENUM_INFO;

typedef enum _ENUM_TYPE_SCREENSHOT
{
	ScreenshotFullscreen,
	ScreenshotWindow,
	ScreenshotRegion,
} ENUM_TYPE_SCREENSHOT, *PENUM_TYPE_SCREENSHOT;

typedef enum _ENUM_IMAGE_NAME
{
	NameIndex,
	NameDate,
} ENUM_IMAGE_NAME, *PENUM_IMAGE_NAME;


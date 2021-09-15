// Free Shooter
// Copyright (c) 2009-2021 Henry++

#pragma once

#include "routine.h"

#include "resource.h"
#include "app.h"

#define LANG_MENU 6
#define SETTINGS_MENU 7
#define FORMAT_MENU 0
#define FILENAME_MENU 1
#define DELAY_MENU 2

#define WND_SLEEP 75
#define JPEG_QUALITY 95
#define BLEND 1.5
#define START_IDX 1

#define REGION_PEN_COLOR RGB (255,0,0)
#define REGION_PEN_DRAW_COLOR RGB (0,255,0)
#define REGION_COLOR_BK RGB (0,0,0)
#define REGION_BLEND 90

#define FILE_FORMAT_NAME_PREFIX L"sshot-"
#define FILE_FORMAT_NAME_FORMAT L"%s%03d"
#define FILE_FORMAT_DATE_FORMAT_1 L"yyyy-MM-dd"
#define FILE_FORMAT_DATE_FORMAT_2 L"HH-mm-ss"

#define HOTKEY_ID_FULLSCREEN 1
#define HOTKEY_ID_MONITOR 2
#define HOTKEY_ID_WINDOW 3
#define HOTKEY_ID_REGION 4

#define HOTKEY_FULLSCREEN MAKEWORD (VK_SNAPSHOT, HOTKEYF_CONTROL | HOTKEYF_ALT)
#define HOTKEY_MONITOR MAKEWORD (VK_SNAPSHOT, 0)
#define HOTKEY_WINDOW MAKEWORD (VK_SNAPSHOT, HOTKEYF_CONTROL)
#define HOTKEY_REGION MAKEWORD (VK_SNAPSHOT, HOTKEYF_ALT)

#define DUMMY_CLASS_DLG L"DummyDlg"
#define REGION_CLASS_DLG L"RegionDlg"
#define TIMER_CLASS_DLG L"TimerDlg"

#define DEFAULT_DIRECTORY L"%userprofile%\\Desktop"

// libs
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "winmm.lib")

DEFINE_GUID (GUID_TrayIcon, 0xabb007cc, 0xd7aa, 0x4ebc, 0xa0, 0x59, 0xb5, 0xd, 0x72, 0x57, 0x4e, 0xfc);

typedef struct _STATIC_DATA
{
	PTP_TIMER htimer;

	PR_ARRAY formats;

	HWND hdummy;

	HWND hregion;
	HWND hregion_tran;

	HANDLE hregion_mutex;
} STATIC_DATA, *PSTATIC_DATA;

typedef struct _SHOT_INFO
{
	union
	{
		R_RECTANGLE rectangle;

		struct
		{
			LONG left;
			LONG top;
			LONG width;
			LONG height;
		};
	};

	struct
	{
		volatile LONG64 timer_value;
	};
} SHOT_INFO, *PSHOT_INFO;

typedef struct _IMAGE_FORMAT
{
	WCHAR ext[8];
	GUID guid;
} IMAGE_FORMAT, *PIMAGE_FORMAT;

typedef enum _ENUM_TYPE_SCREENSHOT
{
	SHOT_FULLSCREEN,
	SHOT_MONITOR,
	SHOT_WINDOW,
	SHOT_REGION,
} ENUM_TYPE_SCREENSHOT, *PENUM_TYPE_SCREENSHOT;

typedef enum _ENUM_IMAGE_NAME
{
	NAME_INDEX,
	NAME_DATE,
} ENUM_IMAGE_NAME, *PENUM_IMAGE_NAME;

// fix DWMNCRENDERINGPOLICY definition
typedef enum _DWMNCRENDERINGPOLICY
{
	NCRP_USEWINDOWSTYLE, // Enable/disable non-client rendering based on window style
	NCRP_DISABLED,       // Disabled non-client rendering; window style is ignored
	NCRP_ENABLED,        // Enabled non-client rendering; window style is ignored
	NCRP_LAST
} DWMNCRENDERINGPOLICY;

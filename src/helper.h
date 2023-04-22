// Free Shooter
// Copyright (c) 2009-2023 Henry++

#pragma once

VOID dump_rect_info (
	_In_ LPCRECT rect,
	_In_ BOOLEAN is_calc
);

VOID dump_wnd_info (
	_In_ HWND hwnd
);

VOID _app_playsound ();

LONG _app_getimageformat_id ();

PIMAGE_FORMAT _app_getimageformat_data ();

ENUM_IMAGE_NAME _app_getimagename_id ();

ENUM_TYPE_SCREENSHOT _app_getmode_id ();

_Success_ (return != -1)
LONG _app_getdelay_id ();

PR_STRING _app_getdirectory ();

VOID _app_getmonitorrect (
	_Out_ PRECT rect
);

VOID _app_switchaeroonwnd (
	_In_ HWND hwnd,
	_In_ BOOLEAN is_disable
);

BOOLEAN _app_getwindowrect (
	_In_ HWND hwnd,
	_Out_ PRECT rect
);

BOOLEAN _app_isnormalwindow (
	_In_ HWND hwnd
);

LONG _app_getwindowshadowsize (
	_In_ HWND hwnd
);

_Ret_maybenull_
PR_STRING _app_uniquefilename (
	_In_ LPCWSTR directory,
	_In_ ENUM_IMAGE_NAME name_type
);

VOID _app_proceedscreenshot (
	_In_ PSHOT_INFO shot_info
);

VOID _app_savescreenshot (
	_In_ PSHOT_INFO shot_info
);

VOID _app_screenshot (
	_In_opt_ HWND hwnd,
	_In_ ENUM_TYPE_SCREENSHOT mode
);

// Free Shooter
// Copyright (c) 2009-2023 Henry++

#include "global.h"

VOID dump_rect_info (
	_In_ LPCRECT rect,
	_In_ BOOLEAN is_calc
)
{
	_r_debug (
		L"left: %d, top: %d, width: %d, height: %d",
		rect->left,
		rect->top,
		is_calc ? _r_calc_rectwidth (rect) : rect->right,
		is_calc ? _r_calc_rectheight (rect) : rect->bottom
	);
}

VOID dump_wnd_info (
	_In_ HWND hwnd
)
{
	WCHAR title[128];
	WCHAR class_name[128];
	RECT rect = {0};

	if (!GetWindowText (hwnd, title, RTL_NUMBER_OF (title)))
		_r_str_copy (title, RTL_NUMBER_OF (title), L"n/a");

	if (!GetClassName (hwnd, class_name, RTL_NUMBER_OF (class_name)))
		_r_str_copy (class_name, RTL_NUMBER_OF (class_name), L"n/a");

	_app_getwindowrect (hwnd, &rect);

	_r_debug (
		L"0x%08p | % 20s | % 20s | left: %d, top: %d, width: %d, height: %d",
		hwnd,
		title,
		class_name,
		rect.left,
		rect.top,
		_r_calc_rectwidth (&rect),
		_r_calc_rectheight (&rect)
	);
}

VOID _app_playsound ()
{
	if (!_r_config_getboolean (L"IsPlaySound", TRUE))
		return;

	PlaySound (MAKEINTRESOURCE (IDW_MAIN), _r_sys_getimagebase (), SND_ASYNC | SND_NODEFAULT | SND_NOWAIT | SND_FILENAME | SND_SENTRY | SND_RESOURCE);
}

LONG _app_getimageformat_id ()
{
	LONG format_id;

	format_id = _r_config_getlong (L"ImageFormat", 2);

	return _r_calc_clamp (format_id, 0, (LONG)_r_obj_getarraysize (config.formats) - 1);
}

PIMAGE_FORMAT _app_getimageformat_data ()
{
	PIMAGE_FORMAT format_data;

	format_data = _r_obj_getarrayitem (config.formats, _app_getimageformat_id ());

	return format_data;
}

ENUM_IMAGE_NAME _app_getimagename_id ()
{
	LONG name_id;

	name_id = _r_config_getlong (L"FilenameType", NAME_INDEX);

	return _r_calc_clamp (name_id, NAME_INDEX, NAME_DATE);
}

ENUM_TYPE_SCREENSHOT _app_getmode_id ()
{
	LONG mode_id;

	mode_id = _r_config_getlong (L"Mode", SHOT_MONITOR);

	return _r_calc_clamp (mode_id, SHOT_FULLSCREEN, SHOT_REGION);
}

_Success_ (return != -1)
LONG _app_getdelay_id ()
{
	LONG delay_idx;

	delay_idx = _r_config_getlong (L"Delay", 0);

	if (delay_idx <= 0)
		return -1;

	return _r_calc_clamp (delay_idx - 1, 0, RTL_NUMBER_OF (timer_array));
}

PR_STRING _app_getdirectory ()
{
	static R_INITONCE init_once = PR_INITONCE_INIT;
	static PR_STRING default_folder = NULL;

	PR_STRING string;
	HRESULT status;

	if (_r_initonce_begin (&init_once))
	{
		status = _r_path_getknownfolder (&FOLDERID_Desktop, NULL, &string);

		if (FAILED (status))
			status = _r_path_getknownfolder (&FOLDERID_Pictures, NULL, &string);

		if (FAILED (status))
			string = _r_obj_createstring (DEFAULT_DIRECTORY);

		default_folder = string;

		_r_initonce_end (&init_once);
	}

	return _r_config_getstringexpand (L"Folder", _r_obj_getstring (default_folder));
}

BOOL CALLBACK enum_monitor_proc (
	_In_ HMONITOR hmonitor,
	_In_ HDC hdc,
	_In_ LPRECT rect,
	_In_ LPARAM lparam
)
{
	LPRECT new_rect;

	new_rect = (LPRECT)lparam;

	if (IsRectEmpty (new_rect))
		CopyRect (new_rect, rect);

	UnionRect (new_rect, new_rect, rect);

	return TRUE;
}

VOID _app_getmonitorrect (
	_Out_ PRECT rect
)
{
	SetRectEmpty (rect);

	EnumDisplayMonitors (NULL, NULL, &enum_monitor_proc, (LPARAM)rect);
}

VOID _app_switchaeroonwnd (
	_In_ HWND hwnd,
	_In_ BOOLEAN is_disable
)
{
	DwmSetWindowAttribute (hwnd, DWMWA_NCRENDERING_POLICY, &(DWMNCRENDERINGPOLICY){is_disable ? NCRP_DISABLED : NCRP_ENABLED}, sizeof (DWMNCRENDERINGPOLICY));
}

BOOLEAN _app_getwindowrect (
	_In_ HWND hwnd,
	_Out_ PRECT rect
)
{
	HRESULT hr;

	hr = DwmGetWindowAttribute (hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, rect, sizeof (RECT));

	if (hr == S_OK)
		return TRUE;

	return !!GetWindowRect (hwnd, rect); // fallback
}

BOOLEAN _app_isnormalwindow (
	_In_ HWND hwnd
)
{
	if (!_r_wnd_isvisible (hwnd))
		return FALSE;

	return !_r_wnd_ismenu (hwnd) && !_r_wnd_isdesktop (hwnd) && (hwnd != GetShellWindow ());
}

LONG _app_getwindowshadowsize (
	_In_ HWND hwnd
)
{
	RECT rect;
	RECT rect_dwm;
	LONG size;
	HRESULT hr;

	if (_r_wnd_ismaximized (hwnd))
		return 0;

	if (!GetWindowRect (hwnd, &rect))
		return 0;

	hr = DwmGetWindowAttribute (hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect_dwm, sizeof (rect_dwm));

	if (hr != S_OK)
		return 0;

	size = max (rect_dwm.left, rect.left) - min (rect_dwm.left, rect.left);

	return _r_calc_clamp (size, 0, 32);
}

_Ret_maybenull_
PR_STRING _app_uniquefilename (
	_In_ LPCWSTR directory,
	_In_ ENUM_IMAGE_NAME name_type
)
{
	WCHAR date_format[MAX_PATH];
	WCHAR time_format[MAX_PATH];
	SYSTEMTIME st;
	PR_STRING name_prefix;
	PR_STRING string;
	PIMAGE_FORMAT format;

	format = _app_getimageformat_data ();

	name_prefix = _r_config_getstring (L"FilenamePrefix", FILE_FORMAT_NAME_PREFIX);
	string = NULL;

	if (name_type == NAME_DATE)
	{
		GetLocalTime (&st);

		if (GetDateFormat (LOCALE_SYSTEM_DEFAULT, 0, &st, FILE_FORMAT_DATE_FORMAT_1, date_format, RTL_NUMBER_OF (date_format)) &&
			GetTimeFormat (LOCALE_SYSTEM_DEFAULT, 0, &st, FILE_FORMAT_DATE_FORMAT_2, time_format, RTL_NUMBER_OF (time_format)))
		{
			_r_obj_movereference (&string, _r_format_string (
				L"%s\\%s%s-%s.%s",
				directory,
				_r_obj_getstringordefault (name_prefix, FILE_FORMAT_NAME_PREFIX),
				date_format,
				time_format,
				format->ext)
			);

			if (_r_fs_exists (string->buffer))
				_r_obj_clearreference (&string);
		}

		//if (PathYetAnotherMakeUniqueName (result, path_format, NULL, short_path_format))
		//	return result;
	}

	if (!string)
	{
		for (USHORT i = START_IDX; i < USHRT_MAX; i++)
		{
			_r_obj_movereference (&string, _r_format_string (
				L"%s\\" FILE_FORMAT_NAME_FORMAT L".%s",
				directory,
				_r_obj_getstringordefault (name_prefix, FILE_FORMAT_NAME_PREFIX),
				i,
				format->ext)
			);

			if (!_r_fs_exists (string->buffer))
				break;
		}

		//_r_str_printf (path_format, RTL_NUMBER_OF (path_format), L"%s\\%s.%s", directory, name_prefix, fext);
		//_r_str_printf (short_path_format, RTL_NUMBER_OF (short_path_format), L"%s.%s", name_prefix, fext);

		//if (PathYetAnotherMakeUniqueName (result, path_format, NULL, short_path_format))
		//	return result;
	}

	if (name_prefix)
		_r_obj_dereference (name_prefix);

	return string;
}

VOID _app_proceedscreenshot (
	_In_ PSHOT_INFO shot_info
)
{
	CURSORINFO cursor_info;
	ICONINFO icon_info;
	HDC hdc;
	HDC hcapture;
	HBITMAP hbitmap;
	HGDIOBJ old_bitmap;
	R_RECTANGLE prev_rect = {0};
	HWND my_hwnd;
	LONG dpi_value;
	BOOLEAN is_hideme;
	BOOLEAN is_windowdisplayed;

	hcapture = NULL;

	my_hwnd = _r_app_gethwnd ();

	is_hideme = _r_config_getboolean (L"IsHideMe", TRUE);
	is_windowdisplayed = _r_wnd_isvisible (my_hwnd);

	if (is_hideme)
	{
		if (is_windowdisplayed)
		{
			_r_wnd_getposition (my_hwnd, &prev_rect);

			dpi_value = _r_dc_getwindowdpi (my_hwnd);

			SetWindowPos (
				my_hwnd,
				NULL,
				-_r_dc_getsystemmetrics (SM_CXVIRTUALSCREEN, dpi_value),
				-_r_dc_getsystemmetrics (SM_CYVIRTUALSCREEN, dpi_value),
				0,
				0,
				SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW | SWP_FRAMECHANGED | SWP_NOOWNERZORDER
			);
		}

		_r_tray_toggle (my_hwnd, &GUID_TrayIcon, FALSE);
	}

	if (shot_info->hcapture)
	{
		hdc = shot_info->hcapture;
	}
	else
	{
		hdc = GetDC (NULL);

		if (!hdc)
		{
			_r_log (LOG_LEVEL_WARNING, 0, L"GetDC", GetLastError (), NULL);

			goto CleanupExit;
		}
	}

	hcapture = CreateCompatibleDC (hdc);

	if (!hcapture)
	{
		_r_log (LOG_LEVEL_WARNING, 0, L"CreateCompatibleDC", GetLastError (), NULL);

		goto CleanupExit;
	}

	hbitmap = _app_image_createbitmap (hdc, shot_info->width, shot_info->height, NULL);

	if (hbitmap)
	{
		old_bitmap = SelectObject (hcapture, hbitmap);

		BitBlt (hcapture, 0, 0, shot_info->width, shot_info->height, hdc, shot_info->left, shot_info->top, SRCCOPY);

		if (_r_config_getboolean (L"IsIncludeMouseCursor", FALSE))
		{
			RtlZeroMemory (&cursor_info, sizeof (cursor_info));

			cursor_info.cbSize = sizeof (cursor_info);

			if (GetCursorInfo (&cursor_info))
			{
				// check cursor is showing
				if (cursor_info.flags & CURSOR_SHOWING)
				{
					if (cursor_info.hCursor)
					{
						if (GetIconInfo (cursor_info.hCursor, &icon_info))
						{
							DrawIconEx (
								hcapture,
								cursor_info.ptScreenPos.x - icon_info.xHotspot - shot_info->left,
								cursor_info.ptScreenPos.y - icon_info.yHotspot - shot_info->top,
								cursor_info.hCursor,
								0,
								0,
								0,
								NULL,
								DI_NORMAL
							);

							if (icon_info.hbmColor)
								DeleteObject (icon_info.hbmColor);

							if (icon_info.hbmMask)
								DeleteObject (icon_info.hbmMask);
						}
					}
				}
			}
		}

		_app_image_savebitmaptofile (hbitmap, shot_info->width, shot_info->height);

		SelectObject (hcapture, old_bitmap);

		DeleteObject (hbitmap);
	}

CleanupExit:

	if (hcapture)
		DeleteDC (hcapture);

	if (shot_info->hcapture)
	{
		DeleteDC (shot_info->hcapture);
		shot_info->hcapture = NULL;
	}
	else if (hdc)
	{
		ReleaseDC (NULL, hdc);
	}

	if (is_hideme)
	{
		if (is_windowdisplayed)
		{
			SetWindowPos (
				my_hwnd,
				NULL,
				prev_rect.left,
				prev_rect.top,
				prev_rect.width,
				prev_rect.height,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOOWNERZORDER
			);
		}

		_r_tray_toggle (my_hwnd, &GUID_TrayIcon, TRUE);
		_r_tray_setinfo (my_hwnd, &GUID_TrayIcon, NULL, _r_app_getname ());
	}

	SetEvent (config.hshot_evt);
}

VOID _app_savescreenshot (
	_In_ PSHOT_INFO shot_info
)
{
	LONG delay_id;

	delay_id = _app_getdelay_id ();

	if (delay_id == -1)
	{
		_app_proceedscreenshot (shot_info);
	}
	else
	{
		SAFE_DELETE_DC (shot_info->hcapture);

		_app_createtimer (delay_id, shot_info);
	}
}

VOID _app_screenshot (
	_In_opt_ HWND hwnd,
	_In_ ENUM_TYPE_SCREENSHOT mode
)
{
	PSHOT_INFO shot_info;
	MONITORINFO monitor_info;
	RECT rect;
	RECT window_rect;
	POINT pt;
	HMONITOR hmonitor;
	HWND hdummy;
	LONG shadow_size;
	BOOLEAN is_maximized;
	BOOLEAN is_menu;
	BOOLEAN is_includewindowshadow;
	BOOLEAN is_clearbackground;
	BOOLEAN is_disableaeroonwnd;

	if (!GetCursorPos (&pt))
		pt.x = pt.y = 0;

	shot_info = _r_obj_allocate (sizeof (SHOT_INFO), NULL);

	switch (mode)
	{
		case SHOT_FULLSCREEN:
		{
			_app_getmonitorrect (&rect);

			_r_wnd_setrectangle (&shot_info->rectangle, rect.left, rect.top, rect.right, rect.bottom);

			_app_savescreenshot (shot_info);

			break;
		}

		case SHOT_MONITOR:
		{
			RtlZeroMemory (&monitor_info, sizeof (monitor_info));

			monitor_info.cbSize = sizeof (monitor_info);

			hmonitor = MonitorFromPoint (pt, MONITOR_DEFAULTTONEAREST);

			if (GetMonitorInfo (hmonitor, &monitor_info))
			{
				_r_wnd_recttorectangle (&shot_info->rectangle, &monitor_info.rcMonitor);

				_app_savescreenshot (shot_info);
			}

			break;
		}

		case SHOT_WINDOW:
		{
			if (!hwnd)
				break;

			if (!_app_isnormalwindow (hwnd))
				break;

			is_maximized = _r_wnd_ismaximized (hwnd);
			is_menu = _r_wnd_ismenu (hwnd);
			is_includewindowshadow = !is_maximized && _r_config_getboolean (L"IsIncludeWindowShadow", TRUE);
			is_clearbackground = !is_maximized && _r_config_getboolean (L"IsClearBackground", TRUE);
			is_disableaeroonwnd = !is_menu && _r_config_getboolean (L"IsDisableAeroOnWnd", FALSE);

			if (is_includewindowshadow)
				shadow_size = _app_getwindowshadowsize (hwnd);

			if (is_disableaeroonwnd)
				_app_switchaeroonwnd (hwnd, TRUE);

			if (_app_getwindowrect (hwnd, &window_rect))
			{
				// increment all overlapped windows size
				if (!is_maximized)
					_r_wnd_calculateoverlappedrect (hwnd, &window_rect);

				// increment shadow size
				if (is_includewindowshadow)
				{
					if (shadow_size)
						InflateRect (&window_rect, shadow_size, shadow_size);
				}

				hdummy = is_clearbackground ? _app_showdummy (NULL, hwnd, &window_rect) : NULL;

				_r_sys_sleep (WND_SLEEP);

				_r_wnd_recttorectangle (&shot_info->rectangle, &window_rect);

				_app_savescreenshot (shot_info);

				if (hdummy)
					_app_showdummy (hdummy, NULL, NULL);
			}

			if (is_disableaeroonwnd)
				_app_switchaeroonwnd (hwnd, FALSE);

			break;
		}

		case SHOT_REGION:
		{
			_app_createregion ();

			break;
		}
	}

	_r_obj_dereference (shot_info);
}

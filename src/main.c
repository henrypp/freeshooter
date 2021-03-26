// Free Shooter
// Copyright (c) 2009-2021 Henry++

#include "routine.h"

#include <mmsystem.h>
#include <shlobj.h>
#include <wincodec.h>

#include "main.h"
#include "rapp.h"

#include "resource.h"

STATIC_DATA config;

BOOLEAN _app_getwindowrect (_In_ HWND hwnd, _Out_ PRECT rect);
INT_PTR CALLBACK SettingsProc (_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wparam, _In_ LPARAM lparam);

//VOID dump_wnd_info (_In_ HWND hwnd)
//{
//	WCHAR title[100] = {0};
//	WCHAR class_name[100] = {0};
//	RECT rect = {0};
//
//	GetWindowText (hwnd, title, RTL_NUMBER_OF (title));
//	GetClassName (hwnd, class_name, RTL_NUMBER_OF (class_name));
//
//	_app_getwindowrect (hwnd, &rect);
//
//	if (!title[0])
//		_r_str_copy (title, RTL_NUMBER_OF (title), L"n/a");
//
//	if (!class_name[0])
//		_r_str_copy (class_name, RTL_NUMBER_OF (class_name), L"n/a");
//
//	_r_debug_v (L"0x%08x | % 20s | % 20s | left: %.4d, top: %.4d, width: %.4d, height: %.4d",
//				hwnd,
//				title,
//				class_name,
//				rect.left,
//				rect.top,
//				_r_calc_rectwidth (&rect),
//				_r_calc_rectheight (&rect)
//	);
//}

FORCEINLINE INT _app_getimageformat_id ()
{
	assert (!_r_obj_isarrayempty (config.formats));

	return _r_calc_clamp (_r_config_getinteger (L"ImageFormat", FormatPng), 0, (INT)(INT_PTR)_r_obj_getarraysize (config.formats) - 1);
}

FORCEINLINE PIMAGE_FORMAT _app_getimageformat_data ()
{
	return _r_obj_getarrayitem (config.formats, _app_getimageformat_id ());
}

PR_STRING _app_getdirectory ()
{
	PR_STRING directory = _r_str_expandenvironmentstring (_r_config_getstring (L"Folder", _r_obj_getstring (config.default_folder)));

	if (_r_obj_isstringempty (directory) || !_r_fs_exists (directory->buffer))
		_r_obj_movereference (&directory, _r_str_expandenvironmentstring (_r_obj_getstring (config.default_folder)));

	return directory;
}

PR_STRING _app_uniquefilename (_In_ LPCWSTR directory, _In_ ENUM_IMAGE_NAME name_type)
{
	PIMAGE_FORMAT format = _app_getimageformat_data ();

	if (!format)
		return NULL;

	PR_STRING string = NULL;
	LPCWSTR name_prefix = _r_config_getstring (L"FilenamePrefix", FILE_FORMAT_NAME_PREFIX);

	if (name_type == NameDate)
	{
		WCHAR date_format[MAX_PATH];
		WCHAR time_format[MAX_PATH];

		SYSTEMTIME st;
		GetLocalTime (&st);

		if (
			GetDateFormat (LOCALE_SYSTEM_DEFAULT, 0, &st, FILE_FORMAT_DATE_FORMAT_1, date_format, RTL_NUMBER_OF (date_format)) &&
			GetTimeFormat (LOCALE_SYSTEM_DEFAULT, 0, &st, FILE_FORMAT_DATE_FORMAT_2, time_format, RTL_NUMBER_OF (time_format))
			)
		{
			_r_obj_movereference (&string, _r_format_string (L"%s\\%s%s-%s.%s", directory, name_prefix, date_format, time_format, format->ext));

			if (!_r_fs_exists (string->buffer))
				return string;
		}

		//if (PathYetAnotherMakeUniqueName (result, path_format, NULL, short_path_format))
		//	return result;
	}
	else
	{
		for (USHORT i = START_IDX; i < USHRT_MAX; i++)
		{
			_r_obj_movereference (&string, _r_format_string (L"%s\\" FILE_FORMAT_NAME_FORMAT L".%s", directory, name_prefix, i, format->ext));

			if (!_r_fs_exists (string->buffer))
				return string;
		}

		//_r_str_printf (path_format, RTL_NUMBER_OF (path_format), L"%s\\%s.%s", directory, name_prefix, fext);
		//_r_str_printf (short_path_format, RTL_NUMBER_OF (short_path_format), L"%s.%s", name_prefix, fext);

		//if (PathYetAnotherMakeUniqueName (result, path_format, NULL, short_path_format))
		//	return result;
	}

	return NULL;
}

FORCEINLINE BOOLEAN _wic_setoptions (_In_ LPCGUID guid, _Inout_ IPropertyBag2 *property_bag)
{
	PROPBAG2 options[5];
	VARIANT values[5];
	INT options_count = 0;

	memset (options, 0, sizeof (options));
	memset (values, 0, sizeof (values)); // VariantInit

	if (memcmp (guid, &GUID_ContainerFormatBmp, sizeof (GUID)) == 0)
	{
		options[0].pstrName = L"EnableV5Header32bppBGRA";

		values[0].vt = VT_BOOL;
		values[0].boolVal = VARIANT_TRUE;

		options_count = 1;
	}
	else if (memcmp (guid, &GUID_ContainerFormatJpeg, sizeof (GUID)) == 0)
	{
		options[0].pstrName = L"ImageQuality";

		values[0].vt = VT_R4;
		values[0].fltVal = 1.0f;

		options[1].pstrName = L"SuppressApp0";

		values[1].vt = VT_BOOL;
		values[1].boolVal = VARIANT_TRUE;

		options_count = 2;
	}
	else if (memcmp (guid, &GUID_ContainerFormatWmp, sizeof (GUID)) == 0)
	{
		options[0].pstrName = L"UseCodecOptions";

		values[0].vt = VT_BOOL;
		values[0].boolVal = VARIANT_TRUE;

		options[1].pstrName = L"ImageQuality";

		values[1].vt = VT_R4;
		values[1].fltVal = 1.0f;

		options[2].pstrName = L"ProgressiveMode";

		values[2].vt = VT_BOOL;
		values[2].boolVal = VARIANT_TRUE;

		options[3].pstrName = L"InterleavedAlpha";

		values[3].vt = VT_BOOL;
		values[3].boolVal = VARIANT_TRUE;

		options_count = 4;
	}
	else if (memcmp (guid, &GUID_ContainerFormatPng, sizeof (GUID)) == 0)
	{
		options[0].pstrName = L"InterlaceOption";

		values[0].vt = VT_BOOL;
		values[0].boolVal = VARIANT_TRUE;

		options[1].pstrName = L"FilterOption";

		values[1].vt = VT_UI1;
		values[1].bVal = WICPngFilterAdaptive;

		options_count = 2;
	}
	else if (memcmp (guid, &GUID_ContainerFormatTiff, sizeof (GUID)) == 0)
	{
		options[0].pstrName = L"CompressionQuality";

		values[0].vt = VT_R4;
		values[0].fltVal = 1.0f;

		options[1].pstrName = L"TiffCompressionMethod";

		values[1].vt = VT_UI1;
		values[1].bVal = WICTiffCompressionZIP;

		options_count = 2;
	}

	if (options_count)
	{
		HRESULT hr = IPropertyBag2_Write (property_bag, options_count, options, values);

		if (FAILED (hr))
		{
			_r_log (Warning, 0, L"IPropertyBag2_Write", hr, NULL);
		}
		else
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN _wic_savehbitmap (HBITMAP hbitmap, LPCWSTR filepath)
{
	PIMAGE_FORMAT format = _app_getimageformat_data ();

	if (!format)
		return FALSE;

	HRESULT hr = 0;

	BOOLEAN is_success = FALSE;

	IWICStream* wicStream = NULL;
	IWICBitmap* wicBitmap = NULL;
	IWICBitmapEncoder* wicEncoder = NULL;
	IWICBitmapFrameEncode* wicFrame = NULL;
	IWICImagingFactory2* wicFactory = NULL;
	IPropertyBag2 *pPropertybag = NULL;

	BITMAP bitmap = {0};

	if (FAILED (hr = CoCreateInstance (&CLSID_WICImagingFactory2, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory2, &wicFactory)))
	{
		if (FAILED (hr = CoCreateInstance (&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &wicFactory)))
			goto CleanupExit;
	}

	if (FAILED (hr = IWICImagingFactory_CreateBitmapFromHBITMAP (wicFactory, hbitmap, NULL, WICBitmapIgnoreAlpha, &wicBitmap)))
		goto CleanupExit;

	if (FAILED (hr = IWICImagingFactory_CreateStream (wicFactory, &wicStream)))
		goto CleanupExit;

	if (FAILED (hr = IWICStream_InitializeFromFilename (wicStream, filepath, GENERIC_WRITE)))
		goto CleanupExit;

	if (FAILED (hr = IWICImagingFactory_CreateEncoder (wicFactory, &format->guid, NULL, &wicEncoder)))
		goto CleanupExit;

	if (FAILED (hr = IWICBitmapEncoder_Initialize (wicEncoder, (IStream*)wicStream, WICBitmapEncoderNoCache)))
		goto CleanupExit;

	if (!GetObject (hbitmap, sizeof (bitmap), &bitmap))
		goto CleanupExit;

	if (FAILED (hr = IWICBitmapEncoder_CreateNewFrame (wicEncoder, &wicFrame, &pPropertybag)))
		goto CleanupExit;

	_wic_setoptions (&format->guid, pPropertybag);

	if (FAILED (hr = IWICBitmapFrameEncode_Initialize (wicFrame, pPropertybag)))
		goto CleanupExit;

	if (FAILED (hr = IWICBitmapFrameEncode_SetSize (wicFrame, bitmap.bmWidth, bitmap.bmHeight)))
	{
		GUID pixel_format = GUID_WICPixelFormat24bppBGR;

		if (FAILED (hr = IWICBitmapFrameEncode_SetPixelFormat (wicFrame, &pixel_format)))
			goto CleanupExit;
	}

	if (FAILED (hr = IWICBitmapFrameEncode_WriteSource (wicFrame, (IWICBitmapSource*)wicBitmap, NULL)))
		goto CleanupExit;

	if (FAILED (hr = IWICBitmapFrameEncode_Commit (wicFrame)))
		goto CleanupExit;

	if (FAILED (hr = IWICBitmapEncoder_Commit (wicEncoder)))
		goto CleanupExit;

	is_success = TRUE;

CleanupExit:

	if (FAILED (hr))
	{
		_r_show_errormessage (_r_app_gethwnd (), NULL, hr, NULL, NULL);
	}

	if (pPropertybag)
		IPropertyBag2_Release (pPropertybag);

	if (wicBitmap)
		IWICBitmap_Release (wicBitmap);

	if (wicFrame)
		IWICBitmapFrameEncode_Release (wicFrame);

	if (wicStream)
		IWICStream_Release (wicStream);

	if (wicEncoder)
		IWICBitmapDecoder_Release (wicEncoder);

	if (wicFactory)
		IWICImagingFactory_Release (wicFactory);

	return is_success;
}

HBITMAP _app_createbitmap (_In_opt_ HDC hdc, _In_ LONG width, _In_ LONG height)
{
	BITMAPINFO bmi = {0};

	bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32; // four 8-bit components
	bmi.bmiHeader.biSizeImage = (width * height) * 4; // rgba

	return CreateDIBSection (hdc, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
}

VOID _app_dofinishjob (_In_ HBITMAP hbitmap, _In_ INT width, _In_ INT height)
{
	PR_STRING path_string;
	PR_STRING unique_path_string;

	if (_r_config_getboolean (L"CopyToClipboard", FALSE))
	{
		if (OpenClipboard (_r_app_gethwnd ()))
		{
			EmptyClipboard ();

			HBITMAP hbitmap_copy = CopyImage (hbitmap, IMAGE_BITMAP, width, height, 0);

			if (hbitmap_copy)
				SetClipboardData (CF_BITMAP, hbitmap_copy);

			CloseClipboard ();
		}
	}

	path_string = _app_getdirectory ();

	if (path_string)
	{
		unique_path_string = _app_uniquefilename (path_string->buffer, _r_config_getinteger (L"FilenameType", NameIndex));

		if (unique_path_string)
			_r_obj_movereference (&path_string, unique_path_string);

		_wic_savehbitmap (hbitmap, path_string->buffer);

		_r_obj_dereference (path_string);
	}
}

VOID _app_screenshot (_In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height, _In_ BOOLEAN is_cursor)
{
	HDC hdc;
	HDC hcapture = NULL;

	hdc = GetDC (NULL);

	if (!hdc)
	{
		_r_log (Warning, 0, L"GetDC", GetLastError (), NULL);

		goto CleanupExit;
	}

	hcapture = CreateCompatibleDC (hdc);

	if (!hcapture)
	{
		_r_log (Warning, 0, L"CreateCompatibleDC", GetLastError (), NULL);

		goto CleanupExit;
	}

	HBITMAP hbitmap = _app_createbitmap (hdc, width, height);

	if (hbitmap)
	{
		HGDIOBJ old_bitmap = SelectObject (hcapture, hbitmap);
		BitBlt (hcapture, 0, 0, width, height, hdc, x, y, SRCCOPY);

		if (is_cursor)
		{
			CURSORINFO cursor_info = {0};
			cursor_info.cbSize = sizeof (cursor_info);

			if (GetCursorInfo (&cursor_info))
			{
				if (cursor_info.hCursor)
				{
					HICON hicon = CopyIcon (cursor_info.hCursor);

					if (hicon)
					{
						ICONINFO icon_info;

						if (GetIconInfo (hicon, &icon_info))
							DrawIcon (hcapture, cursor_info.ptScreenPos.x - icon_info.xHotspot - x, cursor_info.ptScreenPos.y - icon_info.yHotspot - y, hicon);

						DestroyIcon (hicon);
					}
				}
			}
		}

		_app_dofinishjob (hbitmap, width, height);

		SelectObject (hcapture, old_bitmap);

		DeleteObject (hbitmap);
	}

CleanupExit:

	if (hcapture)
		DeleteDC (hcapture);

	if (hdc)
		ReleaseDC (NULL, hdc);
}

VOID _app_switchaeroonwnd (_In_ HWND hwnd, _In_ BOOLEAN is_disable)
{
	HMODULE hlib = GetModuleHandle (L"dwmapi.dll");

	if (hlib)
	{
		typedef HRESULT (WINAPI *DWMSWA)(HWND hwnd, ULONG dwAttribute, LPCVOID pvAttribute, ULONG cbAttribute);
		const DWMSWA _DwmSetWindowAttribute = (DWMSWA)GetProcAddress (hlib, "DwmSetWindowAttribute"); // vista+

		if (_DwmSetWindowAttribute)
		{
			INT policy = is_disable ? DWMNCRP_DISABLED : DWMNCRP_ENABLED;

			_DwmSetWindowAttribute (hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof (policy));
		}
	}
}

BOOLEAN _app_getwindowrect (_In_ HWND hwnd, _Out_ PRECT rect)
{
	if (_r_sys_isosversiongreaterorequal (WINDOWS_VISTA))
	{
		HMODULE hlib = GetModuleHandle (L"dwmapi.dll");

		if (hlib)
		{
			typedef HRESULT (WINAPI *DWMGWA)(HWND hwnd, ULONG dwAttribute, PVOID pvAttribute, ULONG cbAttribute);
			const DWMGWA _DwmGetWindowAttribute = (DWMGWA)GetProcAddress (hlib, "DwmGetWindowAttribute"); // vista+

			if (_DwmGetWindowAttribute)
			{
				if (SUCCEEDED (_DwmGetWindowAttribute (hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, rect, sizeof (RECT))))
					return TRUE;
			}
		}
	}

	return !!GetWindowRect (hwnd, rect); // fallback
}

BOOLEAN _app_isnormalwindow (_In_ HWND hwnd)
{
	return hwnd && IsWindowVisible (hwnd) && !IsIconic (hwnd) && !_r_wnd_ismenu (hwnd) && !_r_wnd_isdesktop (hwnd) && (hwnd != GetShellWindow ());
}

INT _app_getwindowzorder (_In_ HWND hwnd)
{
	INT z = 0;

	for (HWND h = hwnd; h != NULL; h = GetWindow (h, GW_HWNDPREV)) z++;

	return z;
}

BOOLEAN _app_iswndoverlapped (_In_ HWND hwnd, _In_ PRECT rect)
{
	RECT window_rect;
	POINT pt;

	if (!_app_getwindowrect (hwnd, &window_rect))
		return TRUE;

	pt.x = window_rect.left;
	pt.y = window_rect.top;

	if (PtInRect (rect, pt))
		return TRUE;

	pt.x = window_rect.left;
	pt.y = window_rect.bottom;

	if (PtInRect (rect, pt))
		return TRUE;

	pt.x = window_rect.right;
	pt.y = window_rect.top;

	if (PtInRect (rect, pt))
		return TRUE;

	pt.x = window_rect.right;
	pt.y = window_rect.bottom;

	if (PtInRect (rect, pt))
		return TRUE;

	return FALSE;
}

BOOL CALLBACK CalculateOverlappedRect (_In_ HWND hwnd, _In_ LPARAM lparam)
{
	PENUM_INFO enum_info = (PENUM_INFO)lparam;
	RECT window_rect;

	if (hwnd == enum_info->hroot)
		return TRUE;

	if (enum_info->is_menu)
	{
		if (!_r_wnd_ismenu (hwnd))
			return TRUE;

		if (_app_getwindowzorder (hwnd) > _app_getwindowzorder (enum_info->hroot))
			enum_info->hroot = hwnd;
	}
	else
	{
		if (
			!_app_isnormalwindow (hwnd) ||
			!_app_iswndoverlapped (hwnd, enum_info->rect) ||
			(_app_getwindowzorder (hwnd) > _app_getwindowzorder (enum_info->hroot))
			)
		{
			return TRUE;
		}
	}

	if (!_app_getwindowrect (hwnd, &window_rect))
		return TRUE;

	if (window_rect.left < enum_info->rect->left)
		enum_info->rect->left -= (enum_info->rect->left - window_rect.left);

	if (window_rect.top < enum_info->rect->top)
		enum_info->rect->top -= (enum_info->rect->top - window_rect.top);

	if (window_rect.bottom > enum_info->rect->bottom)
		enum_info->rect->bottom += (window_rect.bottom - enum_info->rect->bottom);

	if (window_rect.right > enum_info->rect->right)
		enum_info->rect->right += (window_rect.right - enum_info->rect->right);

	return TRUE;
}

BOOL CALLBACK FindTopWindow (_In_ HWND hwnd, _In_ LPARAM lparam)
{
	HWND* lpresult = (HWND*)lparam;

	if (
		!(_app_isnormalwindow (hwnd)) || // exclude shell windows
		!(_r_wnd_getstyle (hwnd) & WS_SYSMENU) // exclude controls
		)
	{
		return TRUE;
	}

	*lpresult = hwnd;

	return FALSE;
}

INT _app_getshadowsize (_In_ HWND hwnd)
{
	if (IsZoomed (hwnd))
		return 0;

	// Determine the border size by asking windows to calculate the window rect
	// required for a client rect with a width and height of 0. This method will
	// work before the window is fully initialized and when the window is minimized.
	RECT rect = {0};
	AdjustWindowRectEx (&rect, WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, FALSE, WS_EX_WINDOWEDGE);

	return _r_calc_rectwidth (&rect);
}

VOID _app_playsound ()
{
	if (_r_config_getboolean (L"IsPlaySound", TRUE))
		PlaySound (MAKEINTRESOURCE (IDW_MAIN), _r_sys_getimagebase (), SND_RESOURCE | SND_ASYNC);
}

VOID _app_takeshot (_In_opt_ HWND hwnd, _In_ ENUM_TYPE_SCREENSHOT mode)
{
	BOOLEAN result = FALSE;

	HWND my_window = _r_app_gethwnd ();

	BOOLEAN is_includecursor = _r_config_getboolean (L"IsIncludeMouseCursor", FALSE);
	BOOLEAN is_hideme = _r_config_getboolean (L"IsHideMe", TRUE);
	BOOLEAN is_windowdisplayed = IsWindowVisible (my_window) && !IsIconic (my_window);

	RECT prev_rect = {0};
	RECT window_rect;

	if (is_hideme)
	{
		if (is_windowdisplayed)
		{
			GetWindowRect (my_window, &prev_rect);
			SetWindowPos (my_window, NULL, -_r_dc_getsystemmetrics (my_window, SM_CXVIRTUALSCREEN), -_r_dc_getsystemmetrics (my_window, SM_CYVIRTUALSCREEN), 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOOWNERZORDER);
		}

		_r_tray_toggle (my_window, UID, FALSE);
	}

	if (mode == ScreenshotFullscreen)
	{
		POINT pt = {0};
		GetCursorPos (&pt);

		HMONITOR hmonitor = MonitorFromPoint (pt, MONITOR_DEFAULTTONEAREST);

		MONITORINFO monitor_info = {0};
		monitor_info.cbSize = sizeof (monitor_info);

		if (GetMonitorInfo (hmonitor, &monitor_info))
		{
			PRECT lprc = &monitor_info.rcMonitor;

			_app_playsound ();
			_app_screenshot (lprc->left, lprc->top, _r_calc_rectwidth (lprc), _r_calc_rectheight (lprc), is_includecursor);
		}
	}
	else if (mode == ScreenshotWindow)
	{
		if (!hwnd || !_app_isnormalwindow (hwnd))
		{
			POINT pt = {0};
			GetCursorPos (&pt);

			hwnd = GetAncestor (WindowFromPoint (pt), GA_ROOT);
		}

		if (_app_isnormalwindow (hwnd))
		{
			HWND hdummy = NULL;

			BOOLEAN is_menu = _r_wnd_ismenu (hwnd);
			BOOLEAN is_includewindowshadow = _r_config_getboolean (L"IsIncludeWindowShadow", TRUE);
			BOOLEAN is_clearbackground = _r_config_getboolean (L"IsClearBackground", TRUE);
			BOOLEAN is_disableaeroonwnd = !is_menu && _r_config_getboolean (L"IsDisableAeroOnWnd", FALSE);

			if (is_disableaeroonwnd)
				_app_switchaeroonwnd (hwnd, TRUE);


			if (_app_getwindowrect (hwnd, &window_rect))
			{
				// calculate window rectangle and all overlapped windows
				if (!IsZoomed (hwnd))
				{
					ENUM_INFO enum_info = {0};

					enum_info.hroot = hwnd;
					enum_info.is_menu = is_menu;
					enum_info.rect = &window_rect;

					EnumWindows (&CalculateOverlappedRect, (LPARAM)&enum_info);

					if (is_menu)
						hwnd = enum_info.hroot;
				}

				// calculate shadow padding
				if (is_includewindowshadow)
				{
					INT shadow_size = _app_getshadowsize (hwnd);

					if (shadow_size > 0)
					{
						window_rect.left -= shadow_size;
						window_rect.right += shadow_size;

						window_rect.top -= shadow_size;
						window_rect.bottom += shadow_size;
					}
				}

				if (is_clearbackground)
				{
					hdummy = CreateWindowEx (0, DUMMY_CLASS_DLG, APP_NAME, WS_POPUP | WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, _r_sys_getimagebase (), NULL);

					if (hdummy)
					{
						if (!SetWindowPos (hdummy, hwnd, window_rect.left, window_rect.top, _r_calc_rectwidth (&window_rect), _r_calc_rectheight (&window_rect), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_FRAMECHANGED))
						{
							// fucked uipi-fix
							SetWindowPos (hdummy, HWND_BOTTOM, window_rect.left, window_rect.top, _r_calc_rectwidth (&window_rect), _r_calc_rectheight (&window_rect), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_FRAMECHANGED);
						}
					}
				}

				_r_sleep (WND_SLEEP);

				_app_playsound ();
				_app_screenshot (window_rect.left, window_rect.top, _r_calc_rectwidth (&window_rect), _r_calc_rectheight (&window_rect), is_includecursor);
			}
			if (is_disableaeroonwnd)
				_app_switchaeroonwnd (hwnd, FALSE);

			if (is_clearbackground)
			{
				if (hdummy)
					DestroyWindow (hdummy);
			}
		}
	}
	else if (mode == ScreenshotRegion)
	{
		if (WaitForSingleObjectEx (config.hregion_mutex, 0, FALSE) == WAIT_OBJECT_0)
		{
			config.hregion = CreateWindowEx (WS_EX_TOPMOST, REGION_CLASS_DLG, APP_NAME, WS_POPUP | WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, my_window, NULL, _r_sys_getimagebase (), NULL);

			if (config.hregion)
			{
				WaitForSingleObjectEx (config.hregion, INFINITE, FALSE);
			}
			else
			{
				SetEvent (config.hregion_mutex);
			}
		}
	}

	if (is_hideme)
	{
		if (is_windowdisplayed)
			SetWindowPos (my_window, NULL, prev_rect.left, prev_rect.top, _r_calc_rectwidth (&prev_rect), _r_calc_rectheight (&prev_rect), SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);

		_r_tray_toggle (my_window, UID, TRUE);
		_r_tray_setinfo (my_window, UID, NULL, APP_NAME);
	}
}

_Success_ (return)
BOOLEAN _app_key2string (_Out_writes_ (length) _Always_ (_Post_z_) LPWSTR buffer, _In_ SIZE_T length, _In_ UINT key)
{
	WCHAR key_name[64];
	UINT vk_code = LOBYTE (key);
	UINT modifiers = HIBYTE (key);

	*buffer = UNICODE_NULL;

	if ((modifiers & HOTKEYF_CONTROL) != 0)
		_r_str_append (buffer, length, L"Ctrl+");

	if ((modifiers & HOTKEYF_ALT) != 0)
		_r_str_append (buffer, length, L"Alt+");

	if ((modifiers & HOTKEYF_SHIFT) != 0)
		_r_str_append (buffer, length, L"Shift+");

	if (vk_code == VK_SNAPSHOT)
	{
		_r_str_copy (key_name, RTL_NUMBER_OF (key_name), L"Prnt scrn");
	}
	else
	{
		UINT scan_code = MapVirtualKey (vk_code, MAPVK_VK_TO_VSC);
		GetKeyNameText ((scan_code << 16), key_name, RTL_NUMBER_OF (key_name));
	}

	_r_str_append (buffer, length, key_name);

	return TRUE;
}

BOOLEAN _app_hotkeyinit (_In_ HWND hwnd, _In_opt_ HWND hwnd_hotkey)
{
	UINT hk_fullscreen = _r_config_getuinteger (L"HotkeyFullscreen", HOTKEY_FULLSCREEN);
	UINT hk_window = _r_config_getuinteger (L"HotkeyWindow", HOTKEY_WINDOW);
	UINT hk_region = _r_config_getuinteger (L"HotkeyRegion", HOTKEY_REGION);

	BOOLEAN is_nofullscreen = FALSE;
	BOOLEAN is_nowindow = FALSE;
	BOOLEAN is_noregion = FALSE;

	UnregisterHotKey (hwnd, HOTKEY_ID_FULLSCREEN);
	UnregisterHotKey (hwnd, HOTKEY_ID_WINDOW);
	UnregisterHotKey (hwnd, HOTKEY_ID_REGION);

	if (_r_config_getboolean (L"HotkeyFullscreenEnabled", TRUE))
	{
		if (hk_fullscreen)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_FULLSCREEN, (HIBYTE (hk_fullscreen) & 2) | ((HIBYTE (hk_fullscreen) & 4) >> 2) | ((HIBYTE (hk_fullscreen) & 1) << 2), LOBYTE (hk_fullscreen)))
				is_nofullscreen = TRUE;
		}
	}

	if (_r_config_getboolean (L"HotkeyWindowEnabled", TRUE))
	{
		if (hk_window)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_WINDOW, (HIBYTE (hk_window) & 2) | ((HIBYTE (hk_window) & 4) >> 2) | ((HIBYTE (hk_window) & 1) << 2), LOBYTE (hk_window)))
				is_nowindow = TRUE;
		}
	}

	if (_r_config_getboolean (L"HotkeyRegionEnabled", TRUE))
	{
		if (hk_region)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_REGION, (HIBYTE (hk_region) & 2) | ((HIBYTE (hk_region) & 4) >> 2) | ((HIBYTE (hk_region) & 1) << 2), LOBYTE (hk_region)))
				is_noregion = TRUE;
		}
	}

	// show warning
	if (is_nofullscreen || is_nowindow || is_noregion)
	{
		WCHAR buffer[256] = {0};
		WCHAR key_string[128];

		if (is_nofullscreen)
		{
			_app_key2string (key_string, RTL_NUMBER_OF (key_string), hk_fullscreen);
			_r_str_appendformat (buffer, RTL_NUMBER_OF (buffer), L"%s [%s]\r\n", _r_locale_getstring (IDS_MODE_FULLSCREEN), key_string);
		}

		if (is_nowindow)
		{
			_app_key2string (key_string, RTL_NUMBER_OF (key_string), hk_window);
			_r_str_appendformat (buffer, RTL_NUMBER_OF (buffer), L"%s [%s]\r\n", _r_locale_getstring (IDS_MODE_WINDOW), key_string);
		}

		if (is_noregion)
		{
			_app_key2string (key_string, RTL_NUMBER_OF (key_string), hk_region);
			_r_str_appendformat (buffer, RTL_NUMBER_OF (buffer), L"%s [%s]\r\n", _r_locale_getstring (IDS_MODE_REGION), key_string);
		}

		_r_str_trim (buffer, L"\r\n");

		if (_r_show_message (hwnd_hotkey ? hwnd_hotkey : hwnd, MB_YESNO | MB_ICONWARNING | MB_TOPMOST, NULL, _r_locale_getstring (IDS_WARNING_HOTKEYS), buffer) == IDYES)
		{
			if (!hwnd_hotkey)
				_r_settings_createwindow (hwnd, &SettingsProc, 0);
		}
		else
		{
			return TRUE;
		}
	}

	return !(is_nofullscreen || is_nowindow || is_noregion);
}

LRESULT CALLBACK RegionProc (_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wparam, _In_ LPARAM lparam)
{
	static BOOLEAN is_drawing = FALSE;

	static HDC hcapture = NULL, hcapture_mask = NULL;
	static HBITMAP hbitmap = NULL, hbitmap_mask = NULL;
	static HPEN hpen = NULL, hpen_draw = NULL;
	static POINT pt_start = {0}, pt_end = {0};

	switch (msg)
	{
		case WM_CREATE:
		{
			INT width = _r_dc_getsystemmetrics (hwnd, SM_CXVIRTUALSCREEN);
			INT height = _r_dc_getsystemmetrics (hwnd, SM_CYVIRTUALSCREEN);

			ResetEvent (config.hregion_mutex);

			SetWindowPos (hwnd, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOOWNERZORDER);

			HDC hdc = GetDC (NULL);

			if (hdc)
			{
				INT pen_size = _r_dc_getsystemmetrics (hwnd, SM_CXBORDER) * 2;

				hpen = CreatePen (PS_SOLID, pen_size, REGION_PEN_COLOR);
				hpen_draw = CreatePen (PS_SOLID, pen_size, REGION_PEN_DRAW_COLOR);

				hcapture = CreateCompatibleDC (hdc);
				hcapture_mask = CreateCompatibleDC (hdc);

				hbitmap = _app_createbitmap (hdc, width, height);
				hbitmap_mask = _app_createbitmap (hdc, width, height);

				if (hbitmap && hcapture)
				{
					SelectObject (hcapture, hbitmap);
					BitBlt (hcapture, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
				}

				if (hbitmap_mask && hcapture_mask)
				{
					SelectObject (hcapture_mask, hbitmap_mask);
					BitBlt (hcapture_mask, 0, 0, width, height, hcapture, 0, 0, SRCCOPY);

					ULONG quadrate_size = width * height;
					ULONG image_bytes = quadrate_size * sizeof (COLORREF);

					COLORREF* bmp_buffer = _r_mem_allocatezero (image_bytes);

					if (GetBitmapBits (hbitmap_mask, image_bytes, bmp_buffer))
					{
						COLORREF clr;

						for (ULONG i = 0; i < quadrate_size; i++)
						{
							clr = bmp_buffer[i];

							bmp_buffer[i] = RGB (
								GetRValue (clr) / BLEND,
								GetGValue (clr) / BLEND,
								GetBValue (clr) / BLEND
							);
						}

						SetBitmapBits (hbitmap_mask, image_bytes, bmp_buffer);
					}

					_r_mem_free (bmp_buffer);
				}

				ReleaseDC (NULL, hdc);
			}

			ShowWindow (hwnd, SW_SHOWNA);

			break;
		}

		case WM_DESTROY:
		{
			SetEvent (config.hregion_mutex);

			is_drawing = FALSE;

			SAFE_DELETE_OBJECT (hpen);
			SAFE_DELETE_OBJECT (hpen_draw);
			SAFE_DELETE_OBJECT (hbitmap);
			SAFE_DELETE_OBJECT (hbitmap_mask);

			SAFE_DELETE_DC (hcapture);
			SAFE_DELETE_DC (hcapture_mask);

			return TRUE;
		}

		case WM_DPICHANGED:
		case WM_DISPLAYCHANGE:
		{
			SetWindowPos (hwnd, NULL, 0, 0, _r_dc_getsystemmetrics (hwnd, SM_CXVIRTUALSCREEN), _r_dc_getsystemmetrics (hwnd, SM_CYVIRTUALSCREEN), SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
			break;
		}

		case WM_LBUTTONDOWN:
		{
			if (!is_drawing)
			{
				is_drawing = TRUE;

				pt_start.x = LOWORD (lparam);
				pt_start.y = HIWORD (lparam);

				InvalidateRect (hwnd, NULL, TRUE);
			}
			else
			{
				is_drawing = FALSE;

				pt_end.x = LOWORD (lparam);
				pt_end.y = HIWORD (lparam);

				INT x = min (pt_start.x, pt_end.x);
				INT y = min (pt_start.y, pt_end.y);
				INT width = max (pt_start.x, pt_end.x) - x;
				INT height = max (pt_start.y, pt_end.y) - y;

				if (width && height)
				{
					// save region to a file
					HDC hdc = GetDC (NULL);

					if (hdc)
					{
						HDC hcapture_finish = CreateCompatibleDC (hdc);

						if (hcapture_finish)
						{
							HBITMAP hbitmap_finish = _app_createbitmap (hdc, width, height);

							if (hbitmap_finish)
							{
								SelectObject (hcapture_finish, hbitmap_finish);
								BitBlt (hcapture_finish, 0, 0, width, height, hcapture, x, y, SRCCOPY);

								_app_playsound ();
								_app_dofinishjob (hbitmap_finish, width, height);

								SAFE_DELETE_OBJECT (hbitmap_finish);
							}

							SAFE_DELETE_DC (hcapture_finish);
						}

						ReleaseDC (NULL, hdc);
					}

					DestroyWindow (hwnd);
				}
			}

			return TRUE;
		}

		case WM_MBUTTONDOWN:
		{
			if (is_drawing)
			{
				is_drawing = FALSE;
				pt_start.x = pt_start.y = pt_end.x = pt_end.y = 0;

				InvalidateRect (hwnd, NULL, TRUE);
			}
			else
			{
				DestroyWindow (hwnd);
			}

			return TRUE;
		}

		case WM_MOUSEMOVE:
		{
			InvalidateRect (hwnd, NULL, TRUE);
			return TRUE;
		}

		case WM_ERASEBKGND:
		{
			HDC hdc = (HDC)wparam;
			HDC hdc_buffered;

			RECT rect;
			POINT pt;

			if (!GetCursorPos (&pt))
				break;

			if (!GetClientRect (hwnd, &rect))
				break;

			BP_PAINTPARAMS bpp = {0};

			bpp.cbSize = sizeof (bpp);
			bpp.dwFlags = BPPF_NOCLIP;

			HPAINTBUFFER hdpaint = BeginBufferedPaint (hdc, &rect, BPBF_TOPDOWNDIB, &bpp, &hdc_buffered);

			if (hdpaint)
			{
				BitBlt (hdc_buffered, 0, 0, rect.right, rect.bottom, hcapture_mask, 0, 0, SRCCOPY);

				HGDIOBJ old_pen = SelectObject (hdc_buffered, is_drawing ? hpen_draw : hpen);
				HGDIOBJ old_brush = SelectObject (hdc_buffered, GetStockObject (NULL_BRUSH));

				if (is_drawing)
				{
					// draw region rectangle
					RECT rect_target;
					SetRect (&rect_target, min (pt_start.x, pt.x), min (pt_start.y, pt.y), max (pt_start.x, pt.x), max (pt_start.y, pt.y));

					BitBlt (hdc_buffered, rect_target.left, rect_target.top, _r_calc_rectwidth (&rect_target), _r_calc_rectheight (&rect_target), hcapture, rect_target.left, rect_target.top, SRCCOPY);
					Rectangle (hdc_buffered, pt_start.x, pt_start.y, pt.x, pt.y);
				}
				else
				{
					// draw cursor crosshair
					MoveToEx (hdc_buffered, pt.x, 0, NULL);
					LineTo (hdc_buffered, pt.x, rect.bottom);

					MoveToEx (hdc_buffered, 0, pt.y, NULL);
					LineTo (hdc_buffered, rect.right, pt.y);
				}

				SelectObject (hdc_buffered, old_brush);
				SelectObject (hdc_buffered, old_pen);

				EndBufferedPaint (hdpaint, TRUE);
			}

			return TRUE;
		}

		case WM_KEYDOWN:
		{
			switch (wparam)
			{
				case VK_ESCAPE:
				{
					if (is_drawing)
					{
						is_drawing = FALSE;
						pt_start.x = pt_start.y = pt_end.x = pt_end.y = 0;

						InvalidateRect (hwnd, NULL, TRUE);
					}
					else
					{
						DestroyWindow (hwnd);
					}

					return TRUE;
				}
			}

			break;
		}
	}

	return DefWindowProc (hwnd, msg, wparam, lparam);
}

VOID generate_keys_array (_Out_writes_ (count) PCHAR keys, _In_ SIZE_T count)
{
	UINT predefined_keys[] = {
		VK_SNAPSHOT,
		VK_BACK,
		VK_TAB,
		VK_RETURN,
		VK_SPACE,
		VK_DELETE,
	};

	SIZE_T idx = 0;

	for (CHAR i = 0; i < RTL_NUMBER_OF (predefined_keys); i++)
	{
		if (count <= idx)
			goto CleanupExit;

		keys[idx++] = MAKEWORD (predefined_keys[i], 0);
	}

	for (CHAR i = 'A'; i <= 'Z'; i++)
	{
		if (count <= idx)
			goto CleanupExit;

		keys[idx++] = MAKEWORD (i, 0);
	}

	for (CHAR i = '0'; i <= '9'; i++)
	{
		if (count <= idx)
			goto CleanupExit;

		keys[idx++] = MAKEWORD (i, 0);
	}

	for (CHAR i = VK_F1; i <= VK_F12; i++)
	{
		if (count <= idx)
			goto CleanupExit;

		keys[idx++] = MAKEWORD (i, 0);
	}

CleanupExit:

	keys[idx] = ANSI_NULL;
}

INT_PTR CALLBACK SettingsProc (_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wparam, _In_ LPARAM lparam)
{
	switch (msg)
	{
		case RM_INITIALIZE:
		{
			INT dialog_id = (INT)wparam;

			switch (dialog_id)
			{
				case IDD_SETTINGS_HOTKEYS:
				{
					// show config
					UINT fullscreen_code = _r_config_getuinteger (L"HotkeyFullscreen", HOTKEY_FULLSCREEN);
					UINT window_code = _r_config_getuinteger (L"HotkeyWindow", HOTKEY_WINDOW);
					UINT region_code = _r_config_getuinteger (L"HotkeyRegion", HOTKEY_REGION);

					BOOLEAN fullscreen_allowed = _r_config_getboolean (L"HotkeyFullscreenEnabled", TRUE);
					BOOLEAN window_allowed = _r_config_getboolean (L"HotkeyWindowEnabled", TRUE);
					BOOLEAN region_allowed = _r_config_getboolean (L"HotkeyRegionEnabled", TRUE);

					CheckDlgButton (hwnd, IDC_FULLSCREEN_SHIFT, ((HIBYTE (fullscreen_code) & HOTKEYF_SHIFT) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_FULLSCREEN_CTRL, ((HIBYTE (fullscreen_code) & HOTKEYF_CONTROL) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_FULLSCREEN_ALT, ((HIBYTE (fullscreen_code) & HOTKEYF_ALT) != 0) ? BST_CHECKED : BST_UNCHECKED);

					CheckDlgButton (hwnd, IDC_WINDOW_SHIFT, ((HIBYTE (window_code) & HOTKEYF_SHIFT) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_WINDOW_CTRL, ((HIBYTE (window_code) & HOTKEYF_CONTROL) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_WINDOW_ALT, ((HIBYTE (window_code) & HOTKEYF_ALT) != 0) ? BST_CHECKED : BST_UNCHECKED);

					CheckDlgButton (hwnd, IDC_REGION_SHIFT, ((HIBYTE (region_code) & HOTKEYF_SHIFT) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_REGION_CTRL, ((HIBYTE (region_code) & HOTKEYF_CONTROL) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_REGION_ALT, ((HIBYTE (region_code) & HOTKEYF_ALT) != 0) ? BST_CHECKED : BST_UNCHECKED);

					LPCWSTR disabled_string = _r_locale_getstring (IDS_DISABLE);

					_r_combobox_insertitem (hwnd, IDC_FULLSCREEN_CB, 0, disabled_string);
					_r_combobox_insertitem (hwnd, IDC_WINDOW_CB, 0, disabled_string);
					_r_combobox_insertitem (hwnd, IDC_REGION_CB, 0, disabled_string);

					CHAR keys[64];
					WCHAR key_string[64];
					UINT key_code;

					generate_keys_array (keys, RTL_NUMBER_OF (keys));

					for (UINT i = 0, idx = 0; i < RTL_NUMBER_OF (keys); i++)
					{
						key_code = keys[i];

						if (!key_code)
							break;

						idx += 1;

						_app_key2string (key_string, RTL_NUMBER_OF (key_string), MAKEWORD (key_code, 0));

						_r_combobox_insertitem (hwnd, IDC_FULLSCREEN_CB, idx, key_string);
						_r_combobox_insertitem (hwnd, IDC_WINDOW_CB, idx, key_string);
						_r_combobox_insertitem (hwnd, IDC_REGION_CB, idx, key_string);

						_r_combobox_setitemparam (hwnd, IDC_FULLSCREEN_CB, idx, (LPARAM)key_code);
						_r_combobox_setitemparam (hwnd, IDC_WINDOW_CB, idx, (LPARAM)key_code);
						_r_combobox_setitemparam (hwnd, IDC_REGION_CB, idx, (LPARAM)key_code);

						if (fullscreen_allowed && LOBYTE (fullscreen_code) == key_code)
							_r_combobox_setcurrentitem (hwnd, IDC_FULLSCREEN_CB, idx);

						if (window_allowed && LOBYTE (window_code) == key_code)
							_r_combobox_setcurrentitem (hwnd, IDC_WINDOW_CB, idx);

						if (region_allowed && LOBYTE (region_code) == key_code)
							_r_combobox_setcurrentitem (hwnd, IDC_REGION_CB, idx);
					}

					if (_r_combobox_getcurrentitem (hwnd, IDC_FULLSCREEN_CB) == CB_ERR)
						_r_combobox_setcurrentitem (hwnd, IDC_FULLSCREEN_CB, 0);

					if (_r_combobox_getcurrentitem (hwnd, IDC_WINDOW_CB) == CB_ERR)
						_r_combobox_setcurrentitem (hwnd, IDC_WINDOW_CB, 0);

					if (_r_combobox_getcurrentitem (hwnd, IDC_REGION_CB) == CB_ERR)
						_r_combobox_setcurrentitem (hwnd, IDC_REGION_CB, 0);

					PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_FULLSCREEN_CB, CBN_SELCHANGE), 0);
					PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_WINDOW_CB, CBN_SELCHANGE), 0);
					PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_REGION_CB, CBN_SELCHANGE), 0);

					break;
				}
			}

			break;
		}

		case RM_LOCALIZE:
		{
			_r_ctrl_settextformat (hwnd, IDC_TITLE_FULLSCREEN, L"%s:", _r_locale_getstring (IDS_MODE_FULLSCREEN));
			_r_ctrl_settextformat (hwnd, IDC_TITLE_WINDOW, L"%s:", _r_locale_getstring (IDS_MODE_WINDOW));
			_r_ctrl_settextformat (hwnd, IDC_TITLE_REGION, L"%s:", _r_locale_getstring (IDS_MODE_REGION));

			break;
		}

		case WM_COMMAND:
		{
			INT ctrl_id = LOWORD (wparam);
			INT notify_code = HIWORD (wparam);

			if (notify_code == CBN_SELCHANGE && (ctrl_id == IDC_FULLSCREEN_CB || ctrl_id == IDC_WINDOW_CB || ctrl_id == IDC_REGION_CB))
			{
				BOOLEAN is_disable = _r_combobox_getcurrentitem (hwnd, ctrl_id) > 0;

				_r_ctrl_enable (hwnd, ctrl_id - 3, is_disable); // shift
				_r_ctrl_enable (hwnd, ctrl_id - 2, is_disable); // ctrl
				_r_ctrl_enable (hwnd, ctrl_id - 1, is_disable); // alt

				break;
			}

			//case IDOK: // process Enter key
			//case IDC_SAVE:
			//{
			//	INT fullscreen_idx = _r_combobox_getcurrentitem (hwnd, IDC_FULLSCREEN_CB);
			//	INT window_idx = _r_combobox_getcurrentitem (hwnd, IDC_WINDOW_CB);
			//	INT region_idx = _r_combobox_getcurrentitem (hwnd, IDC_REGION_CB);

			//	_r_config_setboolean (L"HotkeyFullscreenEnabled", fullscreen_idx > 0);
			//	_r_config_setboolean (L"HotkeyWindowEnabled", window_idx > 0);
			//	_r_config_setboolean (L"HotkeyRegionEnabled", region_idx > 0);

			//	if (fullscreen_idx > 0)
			//	{
			//		ULONG modifiers = 0;

			//		if (IsDlgButtonChecked (hwnd, IDC_FULLSCREEN_SHIFT) == BST_CHECKED)
			//			modifiers |= HOTKEYF_SHIFT;

			//		if (IsDlgButtonChecked (hwnd, IDC_FULLSCREEN_CTRL) == BST_CHECKED)
			//			modifiers |= HOTKEYF_CONTROL;

			//		if (IsDlgButtonChecked (hwnd, IDC_FULLSCREEN_ALT) == BST_CHECKED)
			//			modifiers |= HOTKEYF_ALT;

			//		_r_config_setulong (L"HotkeyFullscreen", (ULONG)MAKEWORD (_r_combobox_getitemparam (hwnd, IDC_FULLSCREEN_CB, fullscreen_idx), modifiers));
			//	}

			//	if (window_idx > 0)
			//	{
			//		ULONG modifiers = 0;

			//		if (IsDlgButtonChecked (hwnd, IDC_WINDOW_SHIFT) == BST_CHECKED)
			//			modifiers |= HOTKEYF_SHIFT;

			//		if (IsDlgButtonChecked (hwnd, IDC_WINDOW_CTRL) == BST_CHECKED)
			//			modifiers |= HOTKEYF_CONTROL;

			//		if (IsDlgButtonChecked (hwnd, IDC_WINDOW_ALT) == BST_CHECKED)
			//			modifiers |= HOTKEYF_ALT;

			//		_r_config_setulong (L"HotkeyWindow", (ULONG)MAKEWORD (_r_combobox_getitemparam (hwnd, IDC_WINDOW_CB, window_idx), modifiers));
			//	}

			//	if (region_idx > 0)
			//	{
			//		ULONG modifiers = 0;

			//		if (IsDlgButtonChecked (hwnd, IDC_REGION_SHIFT) == BST_CHECKED)
			//			modifiers |= HOTKEYF_SHIFT;

			//		if (IsDlgButtonChecked (hwnd, IDC_REGION_CTRL) == BST_CHECKED)
			//			modifiers |= HOTKEYF_CONTROL;

			//		if (IsDlgButtonChecked (hwnd, IDC_REGION_ALT) == BST_CHECKED)
			//			modifiers |= HOTKEYF_ALT;

			//		_r_config_setulong (L"HotkeyRegion", (ULONG)MAKEWORD (_r_combobox_getitemparam (hwnd, IDC_REGION_CB, region_idx), modifiers));
			//	}

			//	if (!_app_hotkeyinit (_r_app_gethwnd (), hwnd))
			//		break;

			//	break;
			//}
		}
	}

	return FALSE;
}

VOID _app_initdropdownmenu (_In_ HMENU hmenu, _In_ BOOLEAN is_button)
{
	PIMAGE_FORMAT format;
	HMENU hsubmenu;

	_r_menu_setitemtext (hmenu, IDM_COPYTOCLIPBOARD_CHK, FALSE, _r_locale_getstring (IDS_COPYTOCLIPBOARD_CHK));
	_r_menu_setitemtext (hmenu, IDM_PLAYSOUNDS_CHK, FALSE, _r_locale_getstring (IDS_PLAYSOUNDS_CHK));
	_r_menu_setitemtext (hmenu, IDM_INCLUDEMOUSECURSOR_CHK, FALSE, _r_locale_getstring (IDS_INCLUDEMOUSECURSOR_CHK));
	_r_menu_setitemtext (hmenu, IDM_HIDEME_CHK, FALSE, _r_locale_getstring (IDS_HIDEME_CHK));
	_r_menu_setitemtext (hmenu, IDM_INCLUDEMOUSECURSOR_CHK, FALSE, _r_locale_getstring (IDS_INCLUDEMOUSECURSOR_CHK));
	_r_menu_setitemtext (hmenu, IDM_INCLUDEWINDOWSHADOW_CHK, FALSE, _r_locale_getstring (IDS_INCLUDEWINDOWSHADOW_CHK));
	_r_menu_setitemtext (hmenu, IDM_CLEARBACKGROUND_CHK, FALSE, _r_locale_getstring (IDS_CLEARBACKGROUND_CHK));
	_r_menu_setitemtext (hmenu, IDM_DISABLEAEROONWND_CHK, FALSE, _r_locale_getstring (IDS_DISABLEAEROONWND_CHK));
	_r_menu_setitemtext (hmenu, FILENAME_MENU, TRUE, _r_locale_getstring (IDS_FILENAME));
	_r_menu_setitemtext (hmenu, FORMAT_MENU, TRUE, _r_locale_getstring (IDS_IMAGEFORMAT));
	_r_menu_setitemtextformat (hmenu, IDM_HOTKEYS, FALSE, L"%s%s", _r_locale_getstring (IDS_HOTKEYS), is_button ? L"...\tF3" : L"...");

	format = _app_getimageformat_data ();

	_r_menu_setitemtextformat (hmenu, IDM_FILENAME_INDEX, FALSE, FILE_FORMAT_NAME_FORMAT L".%s", _r_config_getstring (L"FilenamePrefix", FILE_FORMAT_NAME_PREFIX), START_IDX, format->ext);
	_r_menu_setitemtextformat (hmenu, IDM_FILENAME_DATE, FALSE, L"%s" FILE_FORMAT_DATE_FORMAT_1 L"-" FILE_FORMAT_DATE_FORMAT_2 L".%s", _r_config_getstring (L"FilenamePrefix", FILE_FORMAT_NAME_PREFIX), format->ext);

	// initialize formats
	UINT formats_count = (UINT)_r_obj_getarraysize (config.formats);
	hsubmenu = GetSubMenu (hmenu, FORMAT_MENU);

	if (hsubmenu)
	{
		_r_menu_clearitems (hsubmenu);

		for (UINT i = 0; i < formats_count; i++)
		{
			format = _r_obj_getarrayitem (config.formats, i);

			if (format)
				AppendMenu (hsubmenu, MF_BYPOSITION, IDX_FORMATS + i, format->ext);
		}
	}

	CheckMenuItem (hmenu, IDM_COPYTOCLIPBOARD_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"CopyToClipboard", FALSE) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_INCLUDEMOUSECURSOR_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsIncludeMouseCursor", FALSE) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_PLAYSOUNDS_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsPlaySound", TRUE) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_HIDEME_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsHideMe", TRUE) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_INCLUDEWINDOWSHADOW_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsIncludeWindowShadow", TRUE) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_CLEARBACKGROUND_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsClearBackground", TRUE) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_DISABLEAEROONWND_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsDisableAeroOnWnd", FALSE) ? MF_CHECKED : MF_UNCHECKED));

	CheckMenuRadioItem (hmenu, IDM_FILENAME_INDEX, IDM_FILENAME_DATE, IDM_FILENAME_INDEX + _r_config_getuinteger (L"FilenameType", NameIndex), MF_BYCOMMAND);
	CheckMenuRadioItem (hmenu, IDX_FORMATS, IDX_FORMATS + formats_count, IDX_FORMATS + _app_getimageformat_id (), MF_BYCOMMAND);
}

VOID _app_initialize ()
{
	static R_INITONCE init_once = PR_INITONCE_INIT;

	if (_r_initonce_begin (&init_once))
	{
		WNDCLASSEX wcex = {0};

		wcex.cbSize = sizeof (wcex);
		wcex.style = CS_VREDRAW | CS_HREDRAW;
		wcex.hInstance = _r_sys_getimagebase ();

		// register dummy class
		wcex.lpszClassName = DUMMY_CLASS_DLG;
		wcex.lpfnWndProc = &DefWindowProc;
		wcex.hbrBackground = GetSysColorBrush (COLOR_WINDOW);
		wcex.hCursor = LoadCursor (NULL, IDC_ARROW);

		RegisterClassEx (&wcex);

		// register region class
		wcex.lpszClassName = REGION_CLASS_DLG;
		wcex.lpfnWndProc = &RegionProc;
		wcex.hCursor = LoadCursor (_r_sys_getimagebase (), MAKEINTRESOURCE (IDI_MAIN));

		RegisterClassEx (&wcex);

		// create region event
		config.hregion_mutex = CreateEvent (NULL, TRUE, TRUE, NULL);

		// set default folder
		config.default_folder = _r_path_getknownfolder (CSIDL_DESKTOPDIRECTORY, NULL);

		if (!config.default_folder)
			config.default_folder = _r_path_getknownfolder (CSIDL_MYPICTURES, NULL);

		if (!config.default_folder)
			config.default_folder = _r_obj_createstring (DEFAULT_DIRECTORY);

		PR_STRING path_string = _app_getdirectory ();
		PR_STRING expanded_string = _r_str_unexpandenvironmentstring (path_string->buffer);

		if (expanded_string)
			_r_obj_movereference (&path_string, expanded_string);

		if (path_string)
		{
			_r_config_setstring (L"Folder", path_string->buffer);

			_r_obj_dereference (path_string);
		}

		// initialize formats
		LPCWSTR szext[] = {
			L"bmp",
			L"jpeg",
			L"jxr",
			L"png",
			L"tiff",
		};

		LPCGUID guids[] = {
			&GUID_ContainerFormatBmp,
			&GUID_ContainerFormatJpeg,
			&GUID_ContainerFormatWmp,
			&GUID_ContainerFormatPng,
			&GUID_ContainerFormatTiff,
		};

		C_ASSERT (RTL_NUMBER_OF (szext) == RTL_NUMBER_OF (guids));

		config.formats = _r_obj_createarray (sizeof (IMAGE_FORMAT), NULL);

		IMAGE_FORMAT image_format;

		for (SIZE_T i = 0; i < RTL_NUMBER_OF (szext); i++)
		{
			_r_str_copy (image_format.ext, RTL_NUMBER_OF (image_format.ext), szext[i]);
			memcpy (&image_format.guid, guids[i], sizeof (image_format.guid));

			_r_obj_addarrayitem (config.formats, &image_format);
		}

		_r_initonce_end (&init_once);
	}
}

INT_PTR CALLBACK DlgProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			// set edit control configuration
			SHAutoComplete (GetDlgItem (hwnd, IDC_FOLDER), SHACF_FILESYS_ONLY | SHACF_FILESYS_DIRS | SHACF_AUTOSUGGEST_FORCE_ON | SHACF_USETAB);

			_r_settings_addpage (IDD_SETTINGS_HOTKEYS, IDS_HOTKEYS);

			_app_initialize ();

			_app_hotkeyinit (hwnd, NULL);

			break;
		}

		case WM_NCCREATE:
		{
			_r_wnd_enablenonclientscaling (hwnd);
			break;
		}

		case RM_INITIALIZE:
		{
			_r_tray_create (hwnd, UID, WM_TRAYICON, _r_app_getsharedimage (_r_sys_getimagebase (), IDI_MAIN, _r_dc_getsystemmetrics (hwnd, SM_CXSMICON)), APP_NAME, FALSE);

			PR_STRING path_string = _app_getdirectory ();

			if (path_string)
			{
				_r_ctrl_settext (hwnd, IDC_FOLDER, path_string->buffer);

				_r_obj_dereference (path_string);
			}

			CheckDlgButton (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, _r_config_getboolean (L"IsIncludeMouseCursor", FALSE) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_INCLUDEWINDOWSHADOW_CHK, _r_config_getboolean (L"IsIncludeWindowShadow", TRUE) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_CLEARBACKGROUND_CHK, _r_config_getboolean (L"IsClearBackground", TRUE) ? BST_CHECKED : BST_UNCHECKED);

			CheckRadioButton (hwnd, IDC_MODE_FULLSCREEN, IDC_MODE_REGION, IDC_MODE_FULLSCREEN + _r_config_getinteger (L"Mode", 0));

			// configure menu
			HMENU hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				CheckMenuItem (hmenu, IDM_ALWAYSONTOP_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"AlwaysOnTop", APP_ALWAYSONTOP) ? MF_CHECKED : MF_UNCHECKED));
				CheckMenuItem (hmenu, IDM_CLASSICUI_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"ClassicUI", APP_CLASSICUI) ? MF_CHECKED : MF_UNCHECKED));

				CheckMenuItem (hmenu, IDM_STARTMINIMIZED_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsStartMinimized", FALSE) ? MF_CHECKED : MF_UNCHECKED));

				CheckMenuItem (hmenu, IDM_LOADONSTARTUP_CHK, MF_BYCOMMAND | (_r_autorun_isenabled () ? MF_CHECKED : MF_UNCHECKED));

				CheckMenuItem (hmenu, IDM_CHECKUPDATES_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"CheckUpdates", TRUE) ? MF_CHECKED : MF_UNCHECKED));
			}

			break;
		}

		case RM_TASKBARCREATED:
		{
			_r_tray_destroy (hwnd, UID);
			_r_tray_create (hwnd, UID, WM_TRAYICON, _r_app_getsharedimage (_r_sys_getimagebase (), IDI_MAIN, _r_dc_getsystemmetrics (hwnd, SM_CXSMICON)), APP_NAME, FALSE);

			break;
		}

		case RM_LOCALIZE:
		{
			// configure button
			_r_ctrl_settextformat (hwnd, IDC_TITLE_FOLDER, L"%s:", _r_locale_getstring (IDS_FOLDER));
			_r_ctrl_settextformat (hwnd, IDC_TITLE_SETTINGS, L"%s:", _r_locale_getstring (IDS_QUICKSETTINGS));
			_r_ctrl_settextformat (hwnd, IDC_TITLE_MODE, L"%s:", _r_locale_getstring (IDS_MODE));

			_r_ctrl_settext (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, _r_locale_getstring (IDS_INCLUDEMOUSECURSOR_CHK));
			_r_ctrl_settext (hwnd, IDC_INCLUDEWINDOWSHADOW_CHK, _r_locale_getstring (IDS_INCLUDEWINDOWSHADOW_CHK));
			_r_ctrl_settext (hwnd, IDC_CLEARBACKGROUND_CHK, _r_locale_getstring (IDS_CLEARBACKGROUND_CHK));

			_r_ctrl_settext (hwnd, IDC_MODE_FULLSCREEN, _r_locale_getstring (IDS_MODE_FULLSCREEN));
			_r_ctrl_settext (hwnd, IDC_MODE_WINDOW, _r_locale_getstring (IDS_MODE_WINDOW));
			_r_ctrl_settext (hwnd, IDC_MODE_REGION, _r_locale_getstring (IDS_MODE_REGION));

			_r_ctrl_settext (hwnd, IDC_SETTINGS, _r_locale_getstring (IDS_SETTINGS));
			_r_ctrl_settext (hwnd, IDC_SCREENSHOT, _r_locale_getstring (IDS_SCREENSHOT));
			_r_ctrl_settext (hwnd, IDC_EXIT, _r_locale_getstring (IDS_EXIT));

			LONG_PTR style = _r_app_isclassicui () ? WS_EX_STATICEDGE : 0;

			_r_wnd_addstyle (hwnd, IDC_BROWSE_BTN, style, WS_EX_STATICEDGE, GWL_EXSTYLE);
			_r_wnd_addstyle (hwnd, IDC_SETTINGS, style, WS_EX_STATICEDGE, GWL_EXSTYLE);
			_r_wnd_addstyle (hwnd, IDC_SCREENSHOT, style, WS_EX_STATICEDGE, GWL_EXSTYLE);
			_r_wnd_addstyle (hwnd, IDC_EXIT, style, WS_EX_STATICEDGE, GWL_EXSTYLE);

			WCHAR mode_fullscreen[128];
			WCHAR mode_window[128];
			WCHAR mode_region[128];
			WCHAR key_string[128];

			_r_str_copy (mode_fullscreen, RTL_NUMBER_OF (mode_fullscreen), _r_locale_getstring (IDS_MODE_FULLSCREEN));
			_r_str_copy (mode_window, RTL_NUMBER_OF (mode_window), _r_locale_getstring (IDS_MODE_WINDOW));
			_r_str_copy (mode_region, RTL_NUMBER_OF (mode_region), _r_locale_getstring (IDS_MODE_REGION));

			if (_r_config_getboolean (L"HotkeyFullscreenEnabled", TRUE))
			{
				_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getuinteger (L"HotkeyFullscreen", HOTKEY_FULLSCREEN));
				_r_str_appendformat (mode_fullscreen, RTL_NUMBER_OF (mode_fullscreen), L"\t%s", key_string);
			}

			if (_r_config_getboolean (L"HotkeyWindowEnabled", TRUE))
			{
				_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getuinteger (L"HotkeyWindow", HOTKEY_WINDOW));
				_r_str_appendformat (mode_window, RTL_NUMBER_OF (mode_window), L"\t%s", key_string);
			}

			if (_r_config_getboolean (L"HotkeyRegionEnabled", TRUE))
			{
				_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getuinteger (L"HotkeyRegion", HOTKEY_REGION));
				_r_str_appendformat (mode_region, RTL_NUMBER_OF (mode_region), L"\t%s", key_string);
			}

			// localize menu
			HMENU hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				_r_menu_setitemtext (hmenu, 0, TRUE, _r_locale_getstring (IDS_FILE));
				_r_menu_setitemtext (hmenu, 1, TRUE, _r_locale_getstring (IDS_VIEW));
				_r_menu_setitemtextformat (GetSubMenu (hmenu, 1), LANG_MENU, TRUE, L"%s (Language)", _r_locale_getstring (IDS_LANGUAGE));
				_r_menu_setitemtext (hmenu, 2, TRUE, _r_locale_getstring (IDS_HELP));

				_r_menu_setitemtextformat (hmenu, IDM_EXPLORE, FALSE, L"%s...\tCtrl+E", _r_locale_getstring (IDS_EXPLORE));
				_r_menu_setitemtextformat (hmenu, IDM_TAKE_FULLSCREEN, FALSE, L"%s: %s", _r_locale_getstring (IDS_SCREENSHOT), mode_fullscreen);
				_r_menu_setitemtextformat (hmenu, IDM_TAKE_WINDOW, FALSE, L"%s: %s", _r_locale_getstring (IDS_SCREENSHOT), mode_window);
				_r_menu_setitemtextformat (hmenu, IDM_TAKE_REGION, FALSE, L"%s: %s", _r_locale_getstring (IDS_SCREENSHOT), mode_region);
				_r_menu_setitemtext (hmenu, IDM_EXIT, FALSE, _r_locale_getstring (IDS_EXIT));
				_r_menu_setitemtext (hmenu, IDM_ALWAYSONTOP_CHK, FALSE, _r_locale_getstring (IDS_ALWAYSONTOP_CHK));
				_r_menu_setitemtextformat (hmenu, IDM_CLASSICUI_CHK, FALSE, L"%s*", _r_locale_getstring (IDS_CLASSICUI_CHK));
				_r_menu_setitemtext (hmenu, IDM_STARTMINIMIZED_CHK, FALSE, _r_locale_getstring (IDS_STARTMINIMIZED_CHK));
				_r_menu_setitemtext (hmenu, IDM_LOADONSTARTUP_CHK, FALSE, _r_locale_getstring (IDS_LOADONSTARTUP_CHK));
				_r_menu_setitemtext (hmenu, IDM_CHECKUPDATES_CHK, FALSE, _r_locale_getstring (IDS_CHECKUPDATES_CHK));
				_r_menu_setitemtext (hmenu, IDM_WEBSITE, FALSE, _r_locale_getstring (IDS_WEBSITE));
				_r_menu_setitemtext (hmenu, IDM_CHECKUPDATES, FALSE, _r_locale_getstring (IDS_CHECKUPDATES));
				_r_menu_setitemtextformat (hmenu, IDM_ABOUT, FALSE, L"%s\tF1", _r_locale_getstring (IDS_ABOUT));

				_r_locale_enum ((HWND)GetSubMenu (hmenu, 1), LANG_MENU, IDX_LANGUAGE); // enum localizations
			}

			break;
		}

		case WM_DPICHANGED:
		{
			_r_tray_setinfo (hwnd, UID, _r_app_getsharedimage (_r_sys_getimagebase (), IDI_MAIN, _r_dc_getsystemmetrics (hwnd, SM_CXSMICON)), NULL);
			break;
		}

		case RM_UNINITIALIZE:
		{
			_r_tray_destroy (hwnd, UID);
			break;
		}

		case WM_DESTROY:
		{
			SAFE_DELETE_HANDLE (config.hregion_mutex);

			UnregisterClass (DUMMY_CLASS_DLG, _r_sys_getimagebase ());
			UnregisterClass (REGION_CLASS_DLG, _r_sys_getimagebase ());

			_r_tray_destroy (hwnd, UID);

			PostQuitMessage (0);

			break;
		}

		case WM_HOTKEY:
		{
			if (wparam == HOTKEY_ID_FULLSCREEN)
			{
				_app_takeshot (NULL, ScreenshotFullscreen);
			}
			else if (wparam == HOTKEY_ID_WINDOW)
			{
				_app_takeshot (NULL, ScreenshotWindow);
			}
			else if (wparam == HOTKEY_ID_REGION)
			{
				_app_takeshot (NULL, ScreenshotRegion);
			}

			break;
		}

		case WM_TRAYICON:
		{
			switch (LOWORD (lparam))
			{
				case WM_LBUTTONUP:
				{
					_r_wnd_toggle (hwnd, FALSE);
					break;
				}

				case WM_MBUTTONUP:
				{
					PostMessage (hwnd, WM_COMMAND, IDM_EXPLORE, 0);
					break;
				}

				case WM_CONTEXTMENU:
				{
					SetForegroundWindow (hwnd); // don't touch

					HMENU hmenu = LoadMenu (NULL, MAKEINTRESOURCE (IDM_TRAY));

					if (hmenu)
					{
						HMENU hsubmenu = GetSubMenu (hmenu, 0);
						HMENU hmenu_settings = NULL;

						WCHAR mode_fullscreen[128];
						WCHAR mode_window[128];
						WCHAR mode_region[128];
						WCHAR key_string[128];

						_r_str_copy (mode_fullscreen, RTL_NUMBER_OF (mode_fullscreen), _r_locale_getstring (IDS_MODE_FULLSCREEN));
						_r_str_copy (mode_window, RTL_NUMBER_OF (mode_window), _r_locale_getstring (IDS_MODE_WINDOW));
						_r_str_copy (mode_region, RTL_NUMBER_OF (mode_region), _r_locale_getstring (IDS_MODE_REGION));

						if (_r_config_getboolean (L"HotkeyFullscreenEnabled", TRUE))
						{
							_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getuinteger (L"HotkeyFullscreen", HOTKEY_FULLSCREEN));
							_r_str_appendformat (mode_fullscreen, RTL_NUMBER_OF (mode_fullscreen), L"\t%s", key_string);
						}

						if (_r_config_getboolean (L"HotkeyWindowEnabled", TRUE))
						{
							_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getuinteger (L"HotkeyWindow", HOTKEY_WINDOW));
							_r_str_appendformat (mode_window, RTL_NUMBER_OF (mode_window), L"\t%s", key_string);
						}

						if (_r_config_getboolean (L"HotkeyRegionEnabled", TRUE))
						{
							_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getuinteger (L"HotkeyRegion", HOTKEY_REGION));
							_r_str_appendformat (mode_region, RTL_NUMBER_OF (mode_region), L"\t%s", key_string);
						}

						// localize
						_r_menu_setitemtext (hsubmenu, IDM_TRAY_SHOW, FALSE, _r_locale_getstring (IDS_TRAY_SHOW));
						_r_menu_setitemtextformat (hsubmenu, IDM_TRAY_TAKE_FULLSCREEN, FALSE, L"%s: %s", _r_locale_getstring (IDS_SCREENSHOT), mode_fullscreen);
						_r_menu_setitemtextformat (hsubmenu, IDM_TRAY_TAKE_WINDOW, FALSE, L"%s: %s", _r_locale_getstring (IDS_SCREENSHOT), mode_window);
						_r_menu_setitemtextformat (hsubmenu, IDM_TRAY_TAKE_REGION, FALSE, L"%s: %s", _r_locale_getstring (IDS_SCREENSHOT), mode_region);
						_r_menu_setitemtext (hsubmenu, SETTINGS_MENU, TRUE, _r_locale_getstring (IDS_SETTINGS));
						_r_menu_setitemtext (hsubmenu, IDM_TRAY_WEBSITE, FALSE, _r_locale_getstring (IDS_WEBSITE));
						_r_menu_setitemtext (hsubmenu, IDM_TRAY_ABOUT, FALSE, _r_locale_getstring (IDS_ABOUT));
						_r_menu_setitemtext (hsubmenu, IDM_TRAY_EXIT, FALSE, _r_locale_getstring (IDS_EXIT));

						// initialize settings submenu
						hmenu_settings = LoadMenu (NULL, MAKEINTRESOURCE (IDM_SETTINGS));

						if (hmenu_settings)
						{
							HMENU hsubmenu_settings = GetSubMenu (hmenu_settings, 0);

							_app_initdropdownmenu (hsubmenu_settings, false);

							MENUITEMINFO mii = {0};

							mii.cbSize = sizeof (mii);
							mii.fMask = MIIM_SUBMENU;
							mii.hSubMenu = hsubmenu_settings;

							SetMenuItemInfo (hsubmenu, SETTINGS_MENU, TRUE, &mii);
						}

						INT command_id = _r_menu_popup (hsubmenu, hwnd, NULL, FALSE);

						DestroyMenu (hmenu);

						if (hmenu_settings)
							DestroyMenu (hmenu_settings);

						if (command_id)
							PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (command_id, 0), 0);
					}

					break;
				}
			}

			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR nmlp = (LPNMHDR)lparam;

			switch (nmlp->code)
			{
				case BCN_DROPDOWN:
				{
					LPNMBCDROPDOWN lpdropdown = (LPNMBCDROPDOWN)lparam;

					if (lpdropdown->hdr.idFrom == IDC_SETTINGS)
					{
						SendMessage (hwnd, WM_COMMAND, IDC_SETTINGS, 0);

						SetWindowLongPtr (hwnd, DWLP_MSGRESULT, TRUE);
						return TRUE;
					}

					break;
				}
			}

			return FALSE;
		}

		case WM_COMMAND:
		{
			INT ctrl_id = LOWORD (wparam);
			INT notify_code = HIWORD (wparam);

			if (notify_code == 0 && ctrl_id >= IDX_LANGUAGE && ctrl_id <= IDX_LANGUAGE + (INT)_r_locale_getcount ())
			{
				_r_locale_applyfrommenu (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), ctrl_id);
				return FALSE;
			}

			else if ((ctrl_id >= IDX_FORMATS && ctrl_id <= IDX_FORMATS + (INT)_r_obj_getarraysize (config.formats)))
			{
				ULONG idx = (ctrl_id - IDX_FORMATS);

				_r_config_setulong (L"ImageFormat", idx);

				return FALSE;
			}
			else if (ctrl_id == IDC_FOLDER && notify_code == EN_KILLFOCUS)
			{
				PR_STRING path_string = _r_ctrl_gettext (hwnd, ctrl_id);

				if (path_string)
				{
					PR_STRING expanded_string = _r_str_unexpandenvironmentstring (path_string->buffer);

					if (expanded_string)
						_r_obj_movereference (&path_string, expanded_string);

					_r_config_setstring (L"Folder", path_string->buffer);

					_r_obj_dereference (path_string);
				}

				return FALSE;
			}

			switch (ctrl_id)
			{
				case IDC_EXIT:
				case IDM_EXIT:
				case IDM_TRAY_EXIT:
				{
					DestroyWindow (hwnd);
					break;
				}

				case IDCANCEL: // process Esc key
				case IDM_TRAY_SHOW:
				{
					_r_wnd_toggle (hwnd, FALSE);
					break;
				}

				case IDC_SETTINGS:
				{
					RECT rect = {0};
					GetWindowRect (GetDlgItem (hwnd, IDC_SETTINGS), &rect);

					HMENU hmenu = LoadMenu (NULL, MAKEINTRESOURCE (IDM_SETTINGS));

					if (hmenu)
					{
						HMENU hsubmenu = GetSubMenu (hmenu, 0);

						if (hsubmenu)
						{
							_app_initdropdownmenu (hsubmenu, TRUE);

							_r_menu_popup (hsubmenu, hwnd, (PPOINT)&rect, TRUE);
						}

						DestroyMenu (hmenu);
					}

					break;
				}

				case IDM_WEBSITE:
				case IDM_TRAY_WEBSITE:
				{
					ShellExecute (hwnd, NULL, _r_app_getwebsite_url (), NULL, NULL, SW_SHOWDEFAULT);
					break;
				}

				case IDM_CHECKUPDATES:
				{
					_r_update_check (hwnd);
					break;
				}

				case IDM_ABOUT:
				case IDM_TRAY_ABOUT:
				{
					_r_show_aboutmessage (hwnd);
					break;
				}

				case IDM_EXPLORE:
				{
					PR_STRING path_string = _app_getdirectory ();

					if (path_string)
					{
						ShellExecute (hwnd, NULL, path_string->buffer, NULL, NULL, SW_SHOWDEFAULT);
						_r_obj_dereference (path_string);
					}

					break;
				}

				case IDM_ALWAYSONTOP_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"AlwaysOnTop", APP_ALWAYSONTOP);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_config_setboolean (L"AlwaysOnTop", new_val);

					_r_wnd_top (hwnd, new_val);

					break;
				}

				case IDM_CLASSICUI_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"ClassicUI", APP_CLASSICUI);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_config_setboolean (L"ClassicUI", new_val);

					_r_app_restart (hwnd);

					break;
				}

				case IDM_CHECKUPDATES_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"CheckUpdates", TRUE);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_config_setboolean (L"CheckUpdates", new_val);

					break;
				}

				case IDM_STARTMINIMIZED_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"IsStartMinimized", FALSE);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_config_setboolean (L"IsStartMinimized", new_val);

					break;
				}

				case IDM_LOADONSTARTUP_CHK:
				{
					BOOLEAN new_val = !_r_autorun_isenabled ();

					_r_autorun_enable (hwnd, new_val);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (_r_autorun_isenabled () ? MF_CHECKED : MF_UNCHECKED));

					break;
				}

				case IDC_BROWSE_BTN:
				{
					BROWSEINFO browse_info = {0};

					browse_info.hwndOwner = hwnd;
					browse_info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_VALIDATE;

					LPITEMIDLIST pidl = SHBrowseForFolder (&browse_info);

					if (pidl)
					{
						WCHAR buffer[MAX_PATH];

						if (SHGetPathFromIDList (pidl, buffer))
						{
							PR_STRING expanded_string = _r_str_unexpandenvironmentstring (buffer);

							if (expanded_string)
							{
								_r_config_setstring (L"Folder", expanded_string->buffer);

								_r_obj_dereference (expanded_string);
							}
							else
							{
								_r_config_setstring (L"Folder", buffer);
							}

							_r_ctrl_settext (hwnd, IDC_FOLDER, buffer);
						}

						CoTaskMemFree (pidl);
					}

					break;
				}

				case IDM_HIDEME_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"IsHideMe", TRUE);

					_r_config_setboolean (L"IsHideMe", new_val);

					break;
				}

				case IDM_PLAYSOUNDS_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"IsPlaySound", TRUE);

					_r_config_setboolean (L"IsPlaySound", new_val);

					break;
				}

				case IDM_HOTKEYS:
				{
					_r_settings_createwindow (hwnd, &SettingsProc, 0);
					break;
				}

				case IDM_COPYTOCLIPBOARD_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"CopyToClipboard", FALSE);
					_r_config_setboolean (L"CopyToClipboard", new_val);

					break;
				}

				case IDM_DISABLEAEROONWND_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"IsDisableAeroOnWnd", FALSE);
					_r_config_setboolean (L"IsDisableAeroOnWnd", new_val);

					break;
				}

				case IDM_INCLUDEMOUSECURSOR_CHK:
				case IDC_INCLUDEMOUSECURSOR_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"IsIncludeMouseCursor", FALSE);

					_r_config_setboolean (L"IsIncludeMouseCursor", new_val);
					CheckDlgButton (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, new_val ? BST_CHECKED : BST_UNCHECKED);

					break;
				}

				case IDM_CLEARBACKGROUND_CHK:
				case IDC_CLEARBACKGROUND_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"IsClearBackground", TRUE);

					_r_config_setboolean (L"IsClearBackground", new_val);
					CheckDlgButton (hwnd, IDC_CLEARBACKGROUND_CHK, new_val ? BST_CHECKED : BST_UNCHECKED);

					break;
				}

				case IDM_INCLUDEWINDOWSHADOW_CHK:
				case IDC_INCLUDEWINDOWSHADOW_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"IsIncludeWindowShadow", TRUE);

					_r_config_setboolean (L"IsIncludeWindowShadow", new_val);
					CheckDlgButton (hwnd, IDC_INCLUDEWINDOWSHADOW_CHK, new_val ? BST_CHECKED : BST_UNCHECKED);

					break;
				}

				case IDC_MODE_FULLSCREEN:
				case IDC_MODE_WINDOW:
				case IDC_MODE_REGION:
				{
					ENUM_TYPE_SCREENSHOT mode;

					if (ctrl_id == IDC_MODE_WINDOW)
						mode = ScreenshotWindow;

					else if (ctrl_id == IDC_MODE_REGION)
						mode = ScreenshotRegion;

					else
						mode = ScreenshotFullscreen;

					_r_config_setinteger (L"Mode", mode);

					break;
				}

				case IDM_FILENAME_INDEX:
				case IDM_FILENAME_DATE:
				{
					ENUM_IMAGE_NAME val;

					if (ctrl_id == IDM_FILENAME_DATE)
						val = NameDate;

					else
						val = NameIndex;

					_r_config_setinteger (L"FilenameType", val);

					break;
				}

				case IDM_TAKE_FULLSCREEN:
				case IDM_TRAY_TAKE_FULLSCREEN:
				case IDM_TAKE_WINDOW:
				case IDM_TRAY_TAKE_WINDOW:
				case IDM_TAKE_REGION:
				case IDM_TRAY_TAKE_REGION:
				case IDC_SCREENSHOT:
				{
					ENUM_TYPE_SCREENSHOT mode;
					HWND hwindow = NULL;

					if (
						ctrl_id == IDM_TRAY_TAKE_WINDOW ||
						ctrl_id == IDM_TAKE_WINDOW
						)
					{
						mode = ScreenshotWindow;
					}
					else if (
						ctrl_id == IDM_TRAY_TAKE_REGION ||
						ctrl_id == IDM_TAKE_REGION
						)
					{
						mode = ScreenshotRegion;
					}
					else if (
						ctrl_id == IDM_TAKE_FULLSCREEN ||
						ctrl_id == IDM_TRAY_TAKE_FULLSCREEN
						)
					{
						mode = ScreenshotFullscreen;
					}
					else
					{
						mode = (ENUM_TYPE_SCREENSHOT)_r_config_getinteger (L"Mode", ScreenshotFullscreen);
					}

					if (mode == ScreenshotWindow)
					{
						EnumWindows (&FindTopWindow, (LPARAM)&hwindow);

						if (hwindow)
							_r_wnd_toggle (hwindow, TRUE);
					}

					_app_takeshot (hwindow, mode);

					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

INT APIENTRY wWinMain (_In_ HINSTANCE hinst, _In_opt_ HINSTANCE prev_hinst, _In_ LPWSTR cmdline, _In_ INT show_cmd)
{
	MSG msg;

	RtlSecureZeroMemory (&config, sizeof (config));

	if (_r_app_initialize ())
	{
		if (_r_app_createwindow (IDD_MAIN, IDI_MAIN, &DlgProc))
		{
			HACCEL haccel = LoadAccelerators (_r_sys_getimagebase (), MAKEINTRESOURCE (IDA_MAIN));

			if (haccel)
			{
				while (GetMessage (&msg, NULL, 0, 0) > 0)
				{
					HWND hwnd = GetActiveWindow ();

					if (!TranslateAccelerator (hwnd, haccel, &msg) && !IsDialogMessage (hwnd, &msg))
					{
						TranslateMessage (&msg);
						DispatchMessage (&msg);
					}
				}

				DestroyAcceleratorTable (haccel);
			}
		}
	}

	return ERROR_SUCCESS;
}

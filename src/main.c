// Free Shooter
// Copyright (c) 2009-2021 Henry++

#include "routine.h"

#include <mmsystem.h>
#include <shlobj.h>
#include <wincodec.h>

#include "main.h"
#include "rapp.h"

#include "resource.h"

STATIC_DATA config = {0};

LONG64 timer_array[] =
{
	1,
	2,
	3,
	5,
	7,
	9,
};

BOOLEAN _app_getwindowrect (_In_ HWND hwnd, _Out_ PRECT rect);
INT_PTR CALLBACK SettingsProc (_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wparam, _In_ LPARAM lparam);

VOID dump_rect_info (_In_ PRECT rect)
{
	_r_debug_v (L"left: %d, top: %d, width: %d, height: %d",
				rect->left,
				rect->top,
				_r_calc_rectwidth (rect),
				_r_calc_rectheight (rect)
	);
}

VOID dump_wnd_info (_In_ HWND hwnd)
{
	WCHAR title[128];
	WCHAR class_name[128];
	RECT rect = {0};

	if (!GetWindowText (hwnd, title, RTL_NUMBER_OF (title)))
		_r_str_copy (title, RTL_NUMBER_OF (title), L"n/a");

	if (!GetClassName (hwnd, class_name, RTL_NUMBER_OF (class_name)))
		_r_str_copy (class_name, RTL_NUMBER_OF (class_name), L"n/a");

	_app_getwindowrect (hwnd, &rect);

	_r_debug_v (L"0x%08p | % 20s | % 20s | left: %d, top: %d, width: %d, height: %d",
				hwnd,
				title,
				class_name,
				rect.left,
				rect.top,
				_r_calc_rectwidth (&rect),
				_r_calc_rectheight (&rect)
	);
}

FORCEINLINE VOID _app_playsound ()
{
	if (_r_config_getboolean (L"IsPlaySound", TRUE))
	{
		PlaySound (MAKEINTRESOURCE (IDW_MAIN), _r_sys_getimagebase (), SND_ASYNC | SND_NODEFAULT | SND_NOWAIT | SND_FILENAME | SND_SENTRY | SND_RESOURCE);
	}
}

LONG _app_getimageformat_id ()
{
	LONG format_id;

	format_id = _r_config_getlong (L"ImageFormat", 2);

	return _r_calc_clamp32 (format_id, 0, (LONG)_r_obj_getarraysize (config.formats) - 1);
}

FORCEINLINE PIMAGE_FORMAT _app_getimageformat_data ()
{
	return _r_obj_getarrayitem (config.formats, _app_getimageformat_id ());
}

ENUM_IMAGE_NAME _app_getimagename_id ()
{
	LONG name_id;

	name_id = _r_config_getlong (L"FilenameType", NAME_INDEX);

	return _r_calc_clamp32 (name_id, NAME_INDEX, NAME_DATE);
}

ENUM_TYPE_SCREENSHOT _app_getmode_id ()
{
	LONG mode_id;

	mode_id = _r_config_getlong (L"Mode", SHOT_MONITOR);

	return _r_calc_clamp32 (mode_id, SHOT_FULLSCREEN, SHOT_REGION);
}

LONG _app_getdelay_id ()
{
	LONG delay_idx;

	delay_idx = _r_config_getlong (L"Delay", 0);

	if (delay_idx <= 0)
		return -1;

	return _r_calc_clamp32 (delay_idx - 1, 0, RTL_NUMBER_OF (timer_array));
}

PR_STRING _app_getdirectory ()
{
	static R_INITONCE init_once = PR_INITONCE_INIT;
	static PR_STRING default_folder = NULL;

	if (_r_initonce_begin (&init_once))
	{
		PR_STRING string;

		string = _r_path_getknownfolder (CSIDL_DESKTOPDIRECTORY, NULL);

		if (!string)
			string = _r_path_getknownfolder (CSIDL_MYPICTURES, NULL);

		if (!string)
			string = _r_obj_createstring (DEFAULT_DIRECTORY);

		default_folder = string;

		_r_initonce_end (&init_once);
	}

	return _r_config_getstringexpand (L"Folder", _r_obj_getstring (default_folder));
}

FORCEINLINE VOID _app_switchaeroonwnd (_In_ HWND hwnd, _In_ BOOLEAN is_disable)
{
	DwmSetWindowAttribute (hwnd, DWMWA_NCRENDERING_POLICY, &(DWMNCRENDERINGPOLICY){is_disable ? NCRP_DISABLED : NCRP_ENABLED}, sizeof (DWMNCRENDERINGPOLICY));
}

BOOLEAN _app_getwindowrect (_In_ HWND hwnd, _Out_ PRECT rect)
{
	if (DwmGetWindowAttribute (hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, rect, sizeof (RECT)) == S_OK)
		return TRUE;

	return !!GetWindowRect (hwnd, rect); // fallback
}

BOOLEAN _app_isnormalwindow (_In_ HWND hwnd)
{
	return hwnd && _r_wnd_isvisible (hwnd) && !_r_wnd_ismenu (hwnd) && !_r_wnd_isdesktop (hwnd) && (hwnd != GetShellWindow ());
}

LONG _app_getwindowshadowsize (_In_ HWND hwnd)
{
	RECT rect;
	RECT rect_dwm;

	if (_r_wnd_ismaximized (hwnd))
		return 0;

	if (!GetWindowRect (hwnd, &rect))
		return 0;

	if (DwmGetWindowAttribute (hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect_dwm, sizeof (rect_dwm)) != S_OK)
		return 0;

	return _r_calc_clamp32 (rect_dwm.left - rect.left, 0, 20);
}

_Ret_maybenull_
PR_STRING _app_uniquefilename (_In_ LPCWSTR directory, _In_ ENUM_IMAGE_NAME name_type)
{
	PIMAGE_FORMAT format;
	PR_STRING name_prefix;
	PR_STRING string;

	format = _app_getimageformat_data ();

	name_prefix = _r_config_getstring (L"FilenamePrefix", FILE_FORMAT_NAME_PREFIX);
	string = NULL;

	if (name_type == NAME_DATE)
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
			_r_obj_movereference (&string, _r_format_string (L"%s\\%s%s-%s.%s", directory, _r_obj_getstringordefault (name_prefix, FILE_FORMAT_NAME_PREFIX), date_format, time_format, format->ext));

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
			_r_obj_movereference (&string, _r_format_string (L"%s\\" FILE_FORMAT_NAME_FORMAT L".%s", directory, _r_obj_getstringordefault (name_prefix, FILE_FORMAT_NAME_PREFIX), i, format->ext));

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

VOID _wic_setoptions (_In_ LPCGUID guid, _Inout_ IPropertyBag2 *property_bag)
{
	PROPBAG2 options[5];
	VARIANT values[5];
	HRESULT hr;
	INT options_count = 0;

	RtlZeroMemory (options, sizeof (options));
	RtlZeroMemory (values, sizeof (values)); // VariantInit

	if (IsEqualGUID (guid, &GUID_ContainerFormatBmp))
	{
		options[0].pstrName = L"EnableV5Header32bppBGRA";

		values[0].vt = VT_BOOL;
		values[0].boolVal = VARIANT_TRUE;

		options_count = 1;
	}
	else if (IsEqualGUID (guid, &GUID_ContainerFormatJpeg))
	{
		options[0].pstrName = L"ImageQuality";

		values[0].vt = VT_R4;
		values[0].fltVal = 1.0f;

		options[1].pstrName = L"SuppressApp0";

		values[1].vt = VT_BOOL;
		values[1].boolVal = VARIANT_TRUE;

		options_count = 2;
	}
	else if (IsEqualGUID (guid, &GUID_ContainerFormatWmp))
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
	else if (IsEqualGUID (guid, &GUID_ContainerFormatPng))
	{
		options[0].pstrName = L"InterlaceOption";

		values[0].vt = VT_BOOL;
		values[0].boolVal = VARIANT_TRUE;

		options[1].pstrName = L"FilterOption";

		values[1].vt = VT_UI1;
		values[1].bVal = WICPngFilterAdaptive;

		options_count = 2;
	}
	else if (IsEqualGUID (guid, &GUID_ContainerFormatTiff))
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
		hr = IPropertyBag2_Write (property_bag, options_count, options, values);

		if (hr != S_OK)
		{
			_r_log (LOG_LEVEL_WARNING, 0, L"IPropertyBag2_Write", hr, NULL);
		}
	}
}

BOOLEAN _wic_savehbitmap (_In_ HBITMAP hbitmap, _In_ LPCWSTR filepath)
{
	PIMAGE_FORMAT format;
	IWICStream *wicStream = NULL;
	IWICBitmap *wicBitmap = NULL;
	IWICBitmapEncoder *wicEncoder = NULL;
	IWICBitmapFrameEncode *wicFrame = NULL;
	IWICImagingFactory2 *wicFactory = NULL;
	IPropertyBag2 *pPropertybag = NULL;
	BITMAP bitmap = {0};
	HRESULT hr;
	BOOLEAN is_success;

	format = _app_getimageformat_data ();

	is_success = FALSE;

	hr = CoCreateInstance (&CLSID_WICImagingFactory2, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory2, &wicFactory);

	if (FAILED (hr))
	{
		hr = CoCreateInstance (&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &wicFactory);

		if (FAILED (hr))
			goto CleanupExit;
	}

	hr = IWICImagingFactory_CreateBitmapFromHBITMAP (wicFactory, hbitmap, NULL, WICBitmapIgnoreAlpha, &wicBitmap);

	if (hr != S_OK)
		goto CleanupExit;

	hr = IWICImagingFactory_CreateStream (wicFactory, &wicStream);

	if (hr != S_OK)
		goto CleanupExit;

	hr = IWICStream_InitializeFromFilename (wicStream, filepath, GENERIC_WRITE);

	if (hr != S_OK)
		goto CleanupExit;

	hr = IWICImagingFactory_CreateEncoder (wicFactory, &format->guid, NULL, &wicEncoder);

	if (hr != S_OK)
		goto CleanupExit;

	hr = IWICBitmapEncoder_Initialize (wicEncoder, (IStream *)wicStream, WICBitmapEncoderNoCache);

	if (hr != S_OK)
		goto CleanupExit;

	if (!GetObject (hbitmap, sizeof (bitmap), &bitmap))
		goto CleanupExit;

	hr = IWICBitmapEncoder_CreateNewFrame (wicEncoder, &wicFrame, &pPropertybag);

	if (hr != S_OK)
		goto CleanupExit;

	_wic_setoptions (&format->guid, pPropertybag);

	hr = IWICBitmapFrameEncode_Initialize (wicFrame, pPropertybag);

	if (hr != S_OK)
		goto CleanupExit;

	hr = IWICBitmapFrameEncode_SetSize (wicFrame, bitmap.bmWidth, bitmap.bmHeight);

	if (hr != S_OK)
	{
		GUID pixel_format = GUID_WICPixelFormat24bppBGR;

		hr = IWICBitmapFrameEncode_SetPixelFormat (wicFrame, &pixel_format);

		if (hr != S_OK)
			goto CleanupExit;
	}

	hr = IWICBitmapFrameEncode_WriteSource (wicFrame, (IWICBitmapSource *)wicBitmap, NULL);

	if (hr != S_OK)
		goto CleanupExit;

	hr = IWICBitmapFrameEncode_Commit (wicFrame);

	if (hr != S_OK)
		goto CleanupExit;

	hr = IWICBitmapEncoder_Commit (wicEncoder);

	if (hr != S_OK)
		goto CleanupExit;

	is_success = TRUE;

CleanupExit:

	if (hr != S_OK)
	{
		_r_log (LOG_LEVEL_ERROR, 0, L"_wic_savehbitmap", hr, NULL);

		_r_show_errormessage (_r_app_gethwnd (), NULL, hr, NULL);
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
	PVOID pbits = NULL;

	bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32; // four 8-bit components
	bmi.bmiHeader.biSizeImage = (width * height) * 4; // rgba

	return CreateDIBSection (hdc, &bmi, DIB_RGB_COLORS, &pbits, NULL, 0);
}

VOID _app_savehbitmaptofile (_In_ HBITMAP hbitmap, _In_ INT width, _In_ INT height)
{
	PR_STRING path_string;
	PR_STRING unique_path_string;
	HBITMAP hbitmap_copy;

	_app_playsound ();

	if (_r_config_getboolean (L"CopyToClipboard", FALSE))
	{
		if (OpenClipboard (_r_app_gethwnd ()))
		{
			EmptyClipboard ();

			hbitmap_copy = CopyImage (hbitmap, IMAGE_BITMAP, width, height, 0);

			if (hbitmap_copy)
				SetClipboardData (CF_BITMAP, hbitmap_copy);

			CloseClipboard ();
		}
	}

	path_string = _app_getdirectory ();

	unique_path_string = _app_uniquefilename (path_string->buffer, _app_getimagename_id ());

	if (unique_path_string)
		_r_obj_movereference (&path_string, unique_path_string);

	_wic_savehbitmap (hbitmap, path_string->buffer);

	_r_obj_dereference (path_string);
}

VOID _app_proceedscreenshot (_In_ PSHOT_INFO shot_info)
{
	HDC hdc;
	HDC hcapture;
	HBITMAP hbitmap;
	HGDIOBJ old_bitmap;

	R_RECTANGLE prev_rect = {0};
	HWND my_hwnd;
	BOOLEAN is_hideme;
	BOOLEAN is_windowdisplayed;

	hdc = GetDC (NULL);
	hcapture = NULL;

	my_hwnd = _r_app_gethwnd ();

	is_hideme = _r_config_getboolean (L"IsHideMe", TRUE);
	is_windowdisplayed = _r_wnd_isvisible (my_hwnd);

	if (is_hideme)
	{
		if (is_windowdisplayed)
		{
			LONG dpi_value;

			_r_wnd_getposition (my_hwnd, &prev_rect);

			dpi_value = _r_dc_getwindowdpi (my_hwnd);

			SetWindowPos (my_hwnd, NULL, -_r_dc_getsystemmetrics (SM_CXVIRTUALSCREEN, dpi_value), -_r_dc_getsystemmetrics (SM_CYVIRTUALSCREEN, dpi_value), 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW | SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
		}

		_r_tray_toggle (my_hwnd, &GUID_TrayIcon, FALSE);
	}

	if (!hdc)
	{
		_r_log (LOG_LEVEL_WARNING, 0, L"GetDC", GetLastError (), NULL);

		goto CleanupExit;
	}

	hcapture = CreateCompatibleDC (hdc);

	if (!hcapture)
	{
		_r_log (LOG_LEVEL_WARNING, 0, L"CreateCompatibleDC", GetLastError (), NULL);

		goto CleanupExit;
	}

	hbitmap = _app_createbitmap (hdc, shot_info->width, shot_info->height);

	if (hbitmap)
	{
		old_bitmap = SelectObject (hcapture, hbitmap);
		BitBlt (hcapture, 0, 0, shot_info->width, shot_info->height, hdc, shot_info->left, shot_info->top, SRCCOPY);

		if (_r_config_getboolean (L"IsIncludeMouseCursor", FALSE))
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
							DrawIcon (hcapture, cursor_info.ptScreenPos.x - icon_info.xHotspot - shot_info->left, cursor_info.ptScreenPos.y - icon_info.yHotspot - shot_info->top, hicon);

						DestroyIcon (hicon);
					}
				}
			}
		}

		_app_savehbitmaptofile (hbitmap, shot_info->width, shot_info->height);

		SelectObject (hcapture, old_bitmap);

		DeleteObject (hbitmap);
	}

CleanupExit:

	if (hcapture)
		DeleteDC (hcapture);

	if (hdc)
		ReleaseDC (NULL, hdc);

	if (is_hideme)
	{
		if (is_windowdisplayed)
			SetWindowPos (my_hwnd, NULL, prev_rect.left, prev_rect.top, prev_rect.width, prev_rect.height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);

		_r_tray_toggle (my_hwnd, &GUID_TrayIcon, TRUE);
		_r_tray_setinfo (my_hwnd, &GUID_TrayIcon, NULL, _r_app_getname ());
	}
}

VOID _app_savescreenshot (_In_ PSHOT_INFO shot_info)
{
	PSHOT_INFO context;
	INT delay_id;

	delay_id = _app_getdelay_id ();

	if (delay_id == -1)
	{
		_app_proceedscreenshot (shot_info);
	}
	else
	{
		context = _r_mem_allocateandcopy (shot_info, sizeof (SHOT_INFO));

		context->timer_value = timer_array[delay_id];
		//context->timstamp = 0;

		if (!CreateWindowEx (WS_EX_TOPMOST | WS_EX_LAYERED, TIMER_CLASS_DLG, NULL, WS_POPUP | WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, _r_app_gethwnd (), NULL, _r_sys_getimagebase (), context))
			_r_mem_free (context);
	}
}

VOID _app_screenshot (_In_ HWND hwnd, _In_ ENUM_TYPE_SCREENSHOT mode)
{
	SHOT_INFO shot_info = {0};
	RECT window_rect;
	POINT pt = {0};

	GetCursorPos (&pt);

	if (mode == SHOT_FULLSCREEN)
	{
		LONG dpi_value;

		dpi_value = _r_dc_getwindowdpi (hwnd);

		_r_wnd_setrectangle (
			&shot_info.rectangle,
			0,
			0,
			_r_dc_getsystemmetrics (SM_CXVIRTUALSCREEN, dpi_value),
			_r_dc_getsystemmetrics (SM_CYVIRTUALSCREEN, dpi_value)
		);

		_app_savescreenshot (&shot_info);
	}
	else if (mode == SHOT_MONITOR)
	{
		HMONITOR hmonitor = MonitorFromPoint (pt, MONITOR_DEFAULTTONEAREST);

		MONITORINFO monitor_info = {0};
		monitor_info.cbSize = sizeof (monitor_info);

		GetMonitorInfo (hmonitor, &monitor_info);

		_r_wnd_recttorectangle (&shot_info.rectangle, &monitor_info.rcMonitor);

		_app_savescreenshot (&shot_info);
	}
	else if (mode == SHOT_WINDOW)
	{
		if (!_app_isnormalwindow (hwnd))
			return;

		LONG shadow_size;
		BOOLEAN is_menu = _r_wnd_ismenu (hwnd);
		BOOLEAN is_includewindowshadow = _r_config_getboolean (L"IsIncludeWindowShadow", TRUE);
		BOOLEAN is_clearbackground = _r_config_getboolean (L"IsClearBackground", TRUE);
		BOOLEAN is_disableaeroonwnd = !is_menu && _r_config_getboolean (L"IsDisableAeroOnWnd", FALSE);

		if (is_includewindowshadow)
			shadow_size = _app_getwindowshadowsize (hwnd);

		if (is_disableaeroonwnd)
			_app_switchaeroonwnd (hwnd, TRUE);

		if (_app_getwindowrect (hwnd, &window_rect))
		{
			// increment all overlapped windows size
			if (!_r_wnd_ismaximized (hwnd))
			{
				_r_wnd_calculateoverlappedrect (hwnd, &window_rect);
			}

			// increment shadow size
			if (is_includewindowshadow)
			{
				if (shadow_size)
					InflateRect (&window_rect, shadow_size, shadow_size);
			}

			if (is_clearbackground)
			{
				if (config.hdummy)
				{
					if (!SetWindowPos (config.hdummy, hwnd, window_rect.left, window_rect.top, _r_calc_rectwidth (&window_rect), _r_calc_rectheight (&window_rect), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_FRAMECHANGED))
					{
						// fucked uipi-fix
						SetWindowPos (config.hdummy, HWND_BOTTOM, window_rect.left, window_rect.top, _r_calc_rectwidth (&window_rect), _r_calc_rectheight (&window_rect), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_FRAMECHANGED);
					}
				}
			}

			_r_sys_sleep (WND_SLEEP);

			_r_wnd_recttorectangle (&shot_info.rectangle, &window_rect);

			_app_savescreenshot (&shot_info);
		}

		if (is_disableaeroonwnd)
			_app_switchaeroonwnd (hwnd, FALSE);

		if (is_clearbackground)
		{
			if (config.hdummy)
				SetWindowPos (config.hdummy, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE);
		}
	}
	else if (mode == SHOT_REGION)
	{
		if (WaitForSingleObjectEx (config.hregion_mutex, 0, FALSE) == WAIT_OBJECT_0)
		{
			config.hregion = CreateWindowEx (WS_EX_TOPMOST, REGION_CLASS_DLG, _r_app_getname (), WS_POPUP | WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, _r_app_gethwnd (), NULL, _r_sys_getimagebase (), NULL);

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
}

VOID _app_key2string (_Out_writes_ (length) LPWSTR buffer, _In_ SIZE_T length, _In_ UINT key)
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
}

BOOLEAN _app_hotkeyinit (_In_ HWND hwnd, _In_opt_ HWND hwnd_hotkey)
{
	UINT hk_fullscreen;
	UINT hk_monitor;
	UINT hk_window;
	UINT hk_region;

	BOOLEAN is_nofullscreen;
	BOOLEAN is_nomonitor;
	BOOLEAN is_nowindow;
	BOOLEAN is_noregion;

	UnregisterHotKey (hwnd, HOTKEY_ID_FULLSCREEN);
	UnregisterHotKey (hwnd, HOTKEY_ID_MONITOR);
	UnregisterHotKey (hwnd, HOTKEY_ID_WINDOW);
	UnregisterHotKey (hwnd, HOTKEY_ID_REGION);

	hk_fullscreen = _r_config_getlong (L"HotkeyFullscreen2", HOTKEY_FULLSCREEN);
	hk_monitor = _r_config_getlong (L"HotkeyMonitor", HOTKEY_MONITOR);
	hk_window = _r_config_getlong (L"HotkeyWindow", HOTKEY_WINDOW);
	hk_region = _r_config_getlong (L"HotkeyRegion", HOTKEY_REGION);

	is_nofullscreen = FALSE;
	is_nomonitor = FALSE;
	is_nowindow = FALSE;
	is_noregion = FALSE;

	if (_r_config_getboolean (L"HotkeyFullscreenEnabled", FALSE))
	{
		if (hk_fullscreen)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_FULLSCREEN, (HIBYTE (hk_fullscreen) & 2) | ((HIBYTE (hk_fullscreen) & 4) >> 2) | ((HIBYTE (hk_fullscreen) & 1) << 2), LOBYTE (hk_fullscreen)))
				is_nofullscreen = TRUE;
		}
	}

	if (_r_config_getboolean (L"HotkeyMonitorEnabled", TRUE))
	{
		if (hk_monitor)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_MONITOR, (HIBYTE (hk_monitor) & 2) | ((HIBYTE (hk_monitor) & 4) >> 2) | ((HIBYTE (hk_monitor) & 1) << 2), LOBYTE (hk_monitor)))
				is_nomonitor = TRUE;
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
	if (is_nofullscreen || is_nomonitor || is_nowindow || is_noregion)
	{
		WCHAR buffer[256] = {0};
		WCHAR key_string[128];

		if (is_nofullscreen)
		{
			_app_key2string (key_string, RTL_NUMBER_OF (key_string), hk_fullscreen);
			_r_str_appendformat (buffer, RTL_NUMBER_OF (buffer), L"%s [%s]\r\n", _r_locale_getstring (IDS_MODE_FULLSCREEN), key_string);
		}

		if (is_nomonitor)
		{
			_app_key2string (key_string, RTL_NUMBER_OF (key_string), hk_monitor);
			_r_str_appendformat (buffer, RTL_NUMBER_OF (buffer), L"%s [%s]\r\n", _r_locale_getstring (IDS_MODE_MONITOR), key_string);
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

		if (_r_show_message (hwnd_hotkey ? hwnd_hotkey : hwnd, MB_YESNO | MB_ICONWARNING, NULL, _r_locale_getstring (IDS_WARNING_HOTKEYS), buffer) == IDYES)
		{
			if (!hwnd_hotkey)
				_r_settings_createwindow (hwnd, &SettingsProc, IDD_SETTINGS_HOTKEYS);

			return FALSE;
		}
	}

	return TRUE;
}

LRESULT CALLBACK RegionProc (_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wparam, _In_ LPARAM lparam)
{
	static BOOLEAN is_drawing = FALSE;

	static HDC hcapture = NULL;
	static HDC hcapture_mask = NULL;
	static HBITMAP hbitmap = NULL;
	static HBITMAP hbitmap_mask = NULL;
	static HPEN hpen = NULL;
	static HPEN hpen_draw = NULL;
	static POINT pt_start = {0};
	static POINT pt_end = {0};

	switch (msg)
	{
		case WM_CREATE:
		{
			HDC hdc;
			LONG dpi_value;
			LONG width;
			LONG height;
			INT pen_size;

			dpi_value = _r_dc_getwindowdpi (hwnd);

			width = _r_dc_getsystemmetrics (SM_CXVIRTUALSCREEN, dpi_value);
			height = _r_dc_getsystemmetrics (SM_CYVIRTUALSCREEN, dpi_value);

			ResetEvent (config.hregion_mutex);

			SetWindowPos (
				hwnd,
				NULL,
				0,
				0,
				width,
				height,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOOWNERZORDER
			);

			hdc = GetDC (NULL);

			if (hdc)
			{
				pen_size = _r_dc_getsystemmetrics (SM_CXBORDER, dpi_value) * 2;

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
					PULONG bmp_buffer;
					ULONG quadrate_size;
					ULONG image_bytes;

					SelectObject (hcapture_mask, hbitmap_mask);
					BitBlt (hcapture_mask, 0, 0, width, height, hcapture, 0, 0, SRCCOPY);

					quadrate_size = width * height;
					image_bytes = quadrate_size * sizeof (ULONG);

					bmp_buffer = _r_mem_allocatezero (image_bytes);

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

			ShowWindow (hwnd, SW_SHOW);

			break;
		}

		case WM_DESTROY:
		{
			is_drawing = FALSE;

			SAFE_DELETE_OBJECT (hpen);
			SAFE_DELETE_OBJECT (hpen_draw);
			SAFE_DELETE_OBJECT (hbitmap);
			SAFE_DELETE_OBJECT (hbitmap_mask);

			SAFE_DELETE_DC (hcapture);
			SAFE_DELETE_DC (hcapture_mask);

			SetEvent (config.hregion_mutex);

			return TRUE;
		}

		case WM_DPICHANGED:
		case WM_DISPLAYCHANGE:
		{
			LONG dpi_value;
			LONG width;
			LONG height;

			dpi_value = _r_dc_getwindowdpi (hwnd);

			width = _r_dc_getsystemmetrics (SM_CXVIRTUALSCREEN, dpi_value);
			height = _r_dc_getsystemmetrics (SM_CYVIRTUALSCREEN, dpi_value);

			SetWindowPos (
				hwnd,
				NULL,
				0,
				0,
				width,
				height,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOOWNERZORDER
			);

			break;
		}

		case WM_LBUTTONDOWN:
		{
			PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDM_REGION_START, 0), lparam);
			return TRUE;
		}

		case WM_MBUTTONDOWN:
		{
			PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDM_REGION_CANCEL, 0), 0);
			return TRUE;
		}

		case WM_RBUTTONDOWN:
		{
			POINT pt;
			HMENU hmenu;
			HMENU hsubmenu;
			INT command_id;

			if (!GetCursorPos (&pt))
				break;

			hmenu = LoadMenu (NULL, MAKEINTRESOURCE (IDM_REGION));

			if (hmenu)
			{
				hsubmenu = GetSubMenu (hmenu, 0);

				if (hsubmenu)
				{
					_r_menu_setitemtext (hsubmenu, IDM_REGION_START, FALSE, L"Start selection...");
					_r_menu_setitemtext (hsubmenu, IDM_REGION_CANCEL, FALSE, L"Close\tEsc");

					_r_menu_setitemtext (hsubmenu, 3, TRUE, L"Dismiss menu");

					command_id = _r_menu_popup (hsubmenu, hwnd, &pt, FALSE);

					if (command_id)
						PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (command_id, 0), MAKELPARAM (pt.x, pt.y));
				}

				DestroyMenu (hmenu);
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
			RECT rect;
			POINT pt;

			HDC hdc;
			HDC hdc_buffered;

			HPAINTBUFFER hdpaint;

			HGDIOBJ old_pen;
			HGDIOBJ old_brush;

			if (!GetCursorPos (&pt))
				break;

			if (!GetClientRect (hwnd, &rect))
				break;

			BP_PAINTPARAMS bpp = {0};

			bpp.cbSize = sizeof (bpp);
			bpp.dwFlags = BPPF_NOCLIP;

			hdc = (HDC)wparam;
			hdpaint = BeginBufferedPaint (hdc, &rect, BPBF_TOPDOWNDIB, &bpp, &hdc_buffered);

			if (hdpaint)
			{
				BitBlt (hdc_buffered, 0, 0, rect.right, rect.bottom, hcapture_mask, 0, 0, SRCCOPY);

				old_pen = SelectObject (hdc_buffered, is_drawing ? hpen_draw : hpen);
				old_brush = SelectObject (hdc_buffered, GetStockObject (NULL_BRUSH));

				if (is_drawing)
				{
					// draw region rectangle
					SetRect (&rect, min (pt_start.x, pt.x), min (pt_start.y, pt.y), max (pt_start.x, pt.x), max (pt_start.y, pt.y));

					BitBlt (hdc_buffered, rect.left, rect.top, _r_calc_rectwidth (&rect), _r_calc_rectheight (&rect), hcapture, rect.left, rect.top, SRCCOPY);
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
			if (wparam == VK_ESCAPE)
			{
				PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDM_REGION_CANCEL, 0), 0);
			}

			break;
		}

		case WM_COMMAND:
		{
			INT ctrl_id = LOWORD (wparam);
			//INT notify_code = HIWORD (wparam);

			switch (ctrl_id)
			{
				case IDM_REGION_START:
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

										_app_savehbitmaptofile (hbitmap_finish, width, height);

										SAFE_DELETE_OBJECT (hbitmap_finish);
									}

									SAFE_DELETE_DC (hcapture_finish);
								}

								ReleaseDC (NULL, hdc);
							}

							DestroyWindow (hwnd);
						}
					}

					break;
				}

				case IDM_REGION_CANCEL:
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

					break;
				}
			}

			break;
		}
	}

	return DefWindowProc (hwnd, msg, wparam, lparam);
}

FORCEINLINE COLORREF get_accent_clr ()
{
	return RGB (255, 0, 0);
	/*
	COLORREF clr;
	BOOL is_opaque;

	if (FAILED (DwmGetColorizationColor (&clr, &is_opaque)))
		return RGB (255, 0, 0);

	//clr = RGB (GetRValue (clr), GetGValue (clr), GetBValue (clr));

	return
		((clr && 0x00ff0000) >> 16) //red
		||
		((clr && 0x0000ff00)) //green
		||
		((clr && 0x000000ff) << 16);
		*/
}

LRESULT CALLBACK TimerProc (_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wparam, _In_ LPARAM lparam)
{
	static BOOLEAN is_drawing = FALSE;

	static PSHOT_INFO context = NULL;
	static HFONT hfont = NULL;
	//static RECT rect = {0};

	switch (msg)
	{
		case WM_CREATE:
		{
			MONITORINFO monitor_info;
			LOGFONT logfont = {0};
			RECT rect;
			POINT pt = {0};
			LPCREATESTRUCT cs;
			HMONITOR hmonitor;
			LONG dpi_value;
			cs = (LPCREATESTRUCT)lparam;

			context = cs->lpCreateParams;

			dpi_value = _r_dc_getwindowdpi (hwnd);

			// initialize timer font
			logfont.lfWeight = FW_SEMIBOLD;
			logfont.lfHeight = _r_dc_fontsizetoheight (74, dpi_value); // size
			_r_str_copy (logfont.lfFaceName, LF_FACESIZE, L"Tahoma"); // face name

			hfont = CreateFontIndirect (&logfont);

			// get monitor size
			GetCursorPos (&pt);

			hmonitor = MonitorFromPoint (pt, MONITOR_DEFAULTTONEAREST);

			RtlZeroMemory (&monitor_info, sizeof (monitor_info));

			monitor_info.cbSize = sizeof (monitor_info);

			if (GetMonitorInfo (hmonitor, &monitor_info))
			{
				INT width = 300;
				INT height = 200;
				PRECT rectang = &monitor_info.rcMonitor;

				SetRect (&rect, rectang->left + rectang->right - width, rectang->top + rectang->bottom - height, width, height);
			}
			else
			{
				SetRect (&rect, 20, 20, 400, 400);
			}

			SetWindowPos (hwnd, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOOWNERZORDER);

			SetLayeredWindowAttributes (hwnd, 0, 0, LWA_COLORKEY);

			SetTimer (hwnd, 5431, 1000, NULL);

			ShowWindow (hwnd, SW_SHOW);

			break;
		}

		case WM_DESTROY:
		{
			SAFE_DELETE_OBJECT (hfont);

			_r_mem_free (context);

			break;
		}

		case WM_TIMER:
		{
			InvalidateRect (hwnd, NULL, TRUE);

			LONG64 seconds = InterlockedDecrement64 (&context->timer_value);

			if (!seconds)
			{
				KillTimer (hwnd, wparam);
				ShowWindow (hwnd, SW_HIDE);

				_app_proceedscreenshot (context);

				DestroyWindow (hwnd);
			}

			return TRUE;
		}

		case WM_ERASEBKGND:
		{
			BP_PAINTPARAMS bpp;

			HDC hdc;
			HDC hdc_buffered;
			HPAINTBUFFER hdpaint;

			RECT rect;
			INT length;
			WCHAR text[8];

			if (!_r_wnd_isvisible (hwnd))
				break;

			if (!GetClientRect (hwnd, &rect))
				break;

			RtlZeroMemory (&bpp, sizeof (bpp));

			bpp.cbSize = sizeof (bpp);
			bpp.dwFlags = BPPF_NOCLIP;

			hdc = (HDC)wparam;

			hdpaint = BeginBufferedPaint (hdc, &rect, BPBF_DIB, &bpp, &hdc_buffered);

			if (hdpaint)
			{
				BitBlt (hdc_buffered, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCERASE);

				SetBkMode (hdc_buffered, TRANSPARENT);
				SetTextColor (hdc_buffered, get_accent_clr ());
				SelectObject (hdc_buffered, hfont);

				_r_str_fromlong64 (text, RTL_NUMBER_OF (text), context->timer_value);
				length = (INT)(INT_PTR)_r_str_getlength (text);

				DrawTextEx (hdc_buffered, text, length, &rect, DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX, NULL);

				EndBufferedPaint (hdpaint, TRUE);
			}

			return TRUE;
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

	SIZE_T index = 0;

	for (CHAR i = 0; i < RTL_NUMBER_OF (predefined_keys); i++)
	{
		if (count <= index)
			goto CleanupExit;

		keys[index++] = MAKEWORD (predefined_keys[i], 0);
	}

	for (CHAR i = 'A'; i <= 'Z'; i++)
	{
		if (count <= index)
			goto CleanupExit;

		keys[index++] = MAKEWORD (i, 0);
	}

	for (CHAR i = '0'; i <= '9'; i++)
	{
		if (count <= index)
			goto CleanupExit;

		keys[index++] = MAKEWORD (i, 0);
	}

	for (CHAR i = VK_F1; i <= VK_F12; i++)
	{
		if (count <= index)
			goto CleanupExit;

		keys[index++] = MAKEWORD (i, 0);
	}

CleanupExit:

	keys[index] = ANSI_NULL;
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
					UINT fullscreen_code;
					UINT monitor_code;
					UINT window_code;
					UINT region_code;

					BOOLEAN fullscreen_allowed;
					BOOLEAN monitor_allowed;
					BOOLEAN window_allowed;
					BOOLEAN region_allowed;

					fullscreen_code = _r_config_getlong (L"HotkeyFullscreen2", HOTKEY_FULLSCREEN);
					monitor_code = _r_config_getlong (L"HotkeyMonitor", HOTKEY_MONITOR);
					window_code = _r_config_getlong (L"HotkeyWindow", HOTKEY_WINDOW);
					region_code = _r_config_getlong (L"HotkeyRegion", HOTKEY_REGION);

					fullscreen_allowed = _r_config_getboolean (L"HotkeyFullscreenEnabled", FALSE);
					monitor_allowed = _r_config_getboolean (L"HotkeyMonitorEnabled", TRUE);
					window_allowed = _r_config_getboolean (L"HotkeyWindowEnabled", TRUE);
					region_allowed = _r_config_getboolean (L"HotkeyRegionEnabled", TRUE);

					CheckDlgButton (hwnd, IDC_FULLSCREEN_SHIFT, ((HIBYTE (fullscreen_code) & HOTKEYF_SHIFT) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_FULLSCREEN_CTRL, ((HIBYTE (fullscreen_code) & HOTKEYF_CONTROL) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_FULLSCREEN_ALT, ((HIBYTE (fullscreen_code) & HOTKEYF_ALT) != 0) ? BST_CHECKED : BST_UNCHECKED);

					CheckDlgButton (hwnd, IDC_MONITOR_SHIFT, ((HIBYTE (monitor_code) & HOTKEYF_SHIFT) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_MONITOR_CTRL, ((HIBYTE (monitor_code) & HOTKEYF_CONTROL) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_MONITOR_ALT, ((HIBYTE (monitor_code) & HOTKEYF_ALT) != 0) ? BST_CHECKED : BST_UNCHECKED);

					CheckDlgButton (hwnd, IDC_WINDOW_SHIFT, ((HIBYTE (window_code) & HOTKEYF_SHIFT) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_WINDOW_CTRL, ((HIBYTE (window_code) & HOTKEYF_CONTROL) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_WINDOW_ALT, ((HIBYTE (window_code) & HOTKEYF_ALT) != 0) ? BST_CHECKED : BST_UNCHECKED);

					CheckDlgButton (hwnd, IDC_REGION_SHIFT, ((HIBYTE (region_code) & HOTKEYF_SHIFT) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_REGION_CTRL, ((HIBYTE (region_code) & HOTKEYF_CONTROL) != 0) ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton (hwnd, IDC_REGION_ALT, ((HIBYTE (region_code) & HOTKEYF_ALT) != 0) ? BST_CHECKED : BST_UNCHECKED);

					LPCWSTR disabled_string = _r_locale_getstring (IDS_DISABLE);

					_r_combobox_insertitem (hwnd, IDC_FULLSCREEN_CB, 0, disabled_string);
					_r_combobox_insertitem (hwnd, IDC_MONITOR_CB, 0, disabled_string);
					_r_combobox_insertitem (hwnd, IDC_WINDOW_CB, 0, disabled_string);
					_r_combobox_insertitem (hwnd, IDC_REGION_CB, 0, disabled_string);

					CHAR keys[64];
					WCHAR key_string[64];
					UINT key_code;

					generate_keys_array (keys, RTL_NUMBER_OF (keys));

					for (UINT i = 0, index = 0; i < RTL_NUMBER_OF (keys); i++)
					{
						key_code = keys[i];

						if (!key_code)
							break;

						index += 1;

						_app_key2string (key_string, RTL_NUMBER_OF (key_string), MAKEWORD (key_code, 0));

						_r_combobox_insertitem (hwnd, IDC_FULLSCREEN_CB, index, key_string);
						_r_combobox_insertitem (hwnd, IDC_MONITOR_CB, index, key_string);
						_r_combobox_insertitem (hwnd, IDC_WINDOW_CB, index, key_string);
						_r_combobox_insertitem (hwnd, IDC_REGION_CB, index, key_string);

						_r_combobox_setitemparam (hwnd, IDC_FULLSCREEN_CB, index, (LPARAM)key_code);
						_r_combobox_setitemparam (hwnd, IDC_MONITOR_CB, index, (LPARAM)key_code);
						_r_combobox_setitemparam (hwnd, IDC_WINDOW_CB, index, (LPARAM)key_code);
						_r_combobox_setitemparam (hwnd, IDC_REGION_CB, index, (LPARAM)key_code);

						if (fullscreen_allowed && LOBYTE (fullscreen_code) == key_code)
							_r_combobox_setcurrentitem (hwnd, IDC_FULLSCREEN_CB, index);

						if (monitor_allowed && LOBYTE (monitor_code) == key_code)
							_r_combobox_setcurrentitem (hwnd, IDC_MONITOR_CB, index);

						if (window_allowed && LOBYTE (window_code) == key_code)
							_r_combobox_setcurrentitem (hwnd, IDC_WINDOW_CB, index);

						if (region_allowed && LOBYTE (region_code) == key_code)
							_r_combobox_setcurrentitem (hwnd, IDC_REGION_CB, index);
					}

					if (_r_combobox_getcurrentitem (hwnd, IDC_FULLSCREEN_CB) == CB_ERR)
						_r_combobox_setcurrentitem (hwnd, IDC_FULLSCREEN_CB, 0);

					if (_r_combobox_getcurrentitem (hwnd, IDC_MONITOR_CB) == CB_ERR)
						_r_combobox_setcurrentitem (hwnd, IDC_MONITOR_CB, 0);

					if (_r_combobox_getcurrentitem (hwnd, IDC_WINDOW_CB) == CB_ERR)
						_r_combobox_setcurrentitem (hwnd, IDC_WINDOW_CB, 0);

					if (_r_combobox_getcurrentitem (hwnd, IDC_REGION_CB) == CB_ERR)
						_r_combobox_setcurrentitem (hwnd, IDC_REGION_CB, 0);

					PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_FULLSCREEN_CB, CBN_SELCHANGE), 0);
					PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_MONITOR_CB, CBN_SELCHANGE), 0);
					PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_WINDOW_CB, CBN_SELCHANGE), 0);
					PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_REGION_CB, CBN_SELCHANGE), 0);

					break;
				}
			}

			break;
		}

		case RM_LOCALIZE:
		{
			_r_ctrl_setstringformat (hwnd, IDC_TITLE_FULLSCREEN, L"%s:", _r_locale_getstring (IDS_MODE_FULLSCREEN));
			_r_ctrl_setstringformat (hwnd, IDC_TITLE_MONITOR, L"%s:", _r_locale_getstring (IDS_MODE_MONITOR));
			_r_ctrl_setstringformat (hwnd, IDC_TITLE_WINDOW, L"%s:", _r_locale_getstring (IDS_MODE_WINDOW));
			_r_ctrl_setstringformat (hwnd, IDC_TITLE_REGION, L"%s:", _r_locale_getstring (IDS_MODE_REGION));

			break;
		}

		case RM_CONFIG_SAVE:
		{
			INT dialog_id = (INT)wparam;

			switch (dialog_id)
			{
				case IDD_SETTINGS_HOTKEYS:
				{
					ULONG modifiers;

					INT fullscreen_idx;
					INT monitor_idx;
					INT window_idx;
					INT region_idx;

					fullscreen_idx = _r_combobox_getcurrentitem (hwnd, IDC_FULLSCREEN_CB);
					monitor_idx = _r_combobox_getcurrentitem (hwnd, IDC_MONITOR_CB);
					window_idx = _r_combobox_getcurrentitem (hwnd, IDC_WINDOW_CB);
					region_idx = _r_combobox_getcurrentitem (hwnd, IDC_REGION_CB);

					_r_config_setboolean (L"HotkeyFullscreenEnabled", fullscreen_idx > 0);
					_r_config_setboolean (L"HotkeyMonitorEnabled", monitor_idx > 0);
					_r_config_setboolean (L"HotkeyWindowEnabled", window_idx > 0);
					_r_config_setboolean (L"HotkeyRegionEnabled", region_idx > 0);

					// save fullscreen hotkey
					if (fullscreen_idx > 0)
					{
						modifiers = 0;

						if (IsDlgButtonChecked (hwnd, IDC_FULLSCREEN_SHIFT) == BST_CHECKED)
							modifiers |= HOTKEYF_SHIFT;

						if (IsDlgButtonChecked (hwnd, IDC_FULLSCREEN_CTRL) == BST_CHECKED)
							modifiers |= HOTKEYF_CONTROL;

						if (IsDlgButtonChecked (hwnd, IDC_FULLSCREEN_ALT) == BST_CHECKED)
							modifiers |= HOTKEYF_ALT;

						_r_config_setulong (L"HotkeyFullscreen2", (ULONG)MAKEWORD (_r_combobox_getitemparam (hwnd, IDC_FULLSCREEN_CB, fullscreen_idx), modifiers));
					}

					// save monitor hotkey
					if (monitor_idx > 0)
					{
						modifiers = 0;

						if (IsDlgButtonChecked (hwnd, IDC_MONITOR_SHIFT) == BST_CHECKED)
							modifiers |= HOTKEYF_SHIFT;

						if (IsDlgButtonChecked (hwnd, IDC_MONITOR_CTRL) == BST_CHECKED)
							modifiers |= HOTKEYF_CONTROL;

						if (IsDlgButtonChecked (hwnd, IDC_MONITOR_ALT) == BST_CHECKED)
							modifiers |= HOTKEYF_ALT;

						_r_config_setulong (L"HotkeyMonitor", (ULONG)MAKEWORD (_r_combobox_getitemparam (hwnd, IDC_MONITOR_CB, monitor_idx), modifiers));
					}

					// save window hotkey
					if (window_idx > 0)
					{
						modifiers = 0;

						if (IsDlgButtonChecked (hwnd, IDC_WINDOW_SHIFT) == BST_CHECKED)
							modifiers |= HOTKEYF_SHIFT;

						if (IsDlgButtonChecked (hwnd, IDC_WINDOW_CTRL) == BST_CHECKED)
							modifiers |= HOTKEYF_CONTROL;

						if (IsDlgButtonChecked (hwnd, IDC_WINDOW_ALT) == BST_CHECKED)
							modifiers |= HOTKEYF_ALT;

						_r_config_setulong (L"HotkeyWindow", (ULONG)MAKEWORD (_r_combobox_getitemparam (hwnd, IDC_WINDOW_CB, window_idx), modifiers));
					}

					// save region hotkey
					if (region_idx > 0)
					{
						modifiers = 0;

						if (IsDlgButtonChecked (hwnd, IDC_REGION_SHIFT) == BST_CHECKED)
							modifiers |= HOTKEYF_SHIFT;

						if (IsDlgButtonChecked (hwnd, IDC_REGION_CTRL) == BST_CHECKED)
							modifiers |= HOTKEYF_CONTROL;

						if (IsDlgButtonChecked (hwnd, IDC_REGION_ALT) == BST_CHECKED)
							modifiers |= HOTKEYF_ALT;

						_r_config_setulong (L"HotkeyRegion", (ULONG)MAKEWORD (_r_combobox_getitemparam (hwnd, IDC_REGION_CB, region_idx), modifiers));
					}

					if (!_app_hotkeyinit (_r_app_gethwnd (), hwnd))
					{
						SetWindowLongPtr (hwnd, DWLP_MSGRESULT, -1);
						return (INT_PTR)-1;
					}

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			INT ctrl_id = LOWORD (wparam);
			INT notify_code = HIWORD (wparam);

			if (notify_code == CBN_SELCHANGE && (ctrl_id == IDC_FULLSCREEN_CB || ctrl_id == IDC_MONITOR_CB || ctrl_id == IDC_WINDOW_CB || ctrl_id == IDC_REGION_CB))
			{
				BOOLEAN is_disable = _r_combobox_getcurrentitem (hwnd, ctrl_id) > 0;

				_r_ctrl_enable (hwnd, ctrl_id - 3, is_disable); // shift
				_r_ctrl_enable (hwnd, ctrl_id - 2, is_disable); // ctrl
				_r_ctrl_enable (hwnd, ctrl_id - 1, is_disable); // alt

				break;
			}
		}
	}

	return FALSE;
}

VOID _app_initdropdownmenu (_In_ HMENU hmenu, _In_ BOOLEAN is_button)
{
	PIMAGE_FORMAT format;
	HMENU hsubmenu;
	PR_STRING string;
	INT delay_idx;

	_r_menu_setitemtext (hmenu, FORMAT_MENU, TRUE, _r_locale_getstring (IDS_IMAGEFORMAT));
	_r_menu_setitemtext (hmenu, FILENAME_MENU, TRUE, _r_locale_getstring (IDS_FILENAME));
	_r_menu_setitemtext (hmenu, DELAY_MENU, TRUE, _r_locale_getstring (IDS_DELAY));

	_r_menu_setitemtext (hmenu, IDM_DELAY_DISABLE, FALSE, _r_locale_getstring (IDS_DISABLE));

	_r_menu_setitemtext (hmenu, IDM_INCLUDEMOUSECURSOR_CHK, FALSE, _r_locale_getstring (IDS_INCLUDEMOUSECURSOR_CHK));
	_r_menu_setitemtext (hmenu, IDM_INCLUDEWINDOWSHADOW_CHK, FALSE, _r_locale_getstring (IDS_INCLUDEWINDOWSHADOW_CHK));
	_r_menu_setitemtext (hmenu, IDM_CLEARBACKGROUND_CHK, FALSE, _r_locale_getstring (IDS_CLEARBACKGROUND_CHK));
	_r_menu_setitemtext (hmenu, IDM_DISABLEAEROONWND_CHK, FALSE, _r_locale_getstring (IDS_DISABLEAEROONWND_CHK));

	_r_menu_setitemtext (hmenu, IDM_COPYTOCLIPBOARD_CHK, FALSE, _r_locale_getstring (IDS_COPYTOCLIPBOARD_CHK));
	_r_menu_setitemtext (hmenu, IDM_PLAYSOUNDS_CHK, FALSE, _r_locale_getstring (IDS_PLAYSOUNDS_CHK));

	_r_menu_setitemtextformat (hmenu, IDM_HOTKEYS, FALSE, L"%s%s", _r_locale_getstring (IDS_HOTKEYS), is_button ? L"...\tF3" : L"...");

	format = _app_getimageformat_data ();

	string = _r_config_getstring (L"FilenamePrefix", FILE_FORMAT_NAME_PREFIX);

	_r_menu_setitemtextformat (hmenu, IDM_FILENAME_INDEX, FALSE, FILE_FORMAT_NAME_FORMAT L".%s", _r_obj_getstringordefault (string, FILE_FORMAT_NAME_PREFIX), START_IDX, format->ext);
	_r_menu_setitemtextformat (hmenu, IDM_FILENAME_DATE, FALSE, L"%s" FILE_FORMAT_DATE_FORMAT_1 L"-" FILE_FORMAT_DATE_FORMAT_2 L".%s", _r_obj_getstringordefault (string, FILE_FORMAT_NAME_PREFIX), format->ext);

	if (string)
		_r_obj_dereference (string);

	// initialize formats
	UINT_PTR formats_count = (UINT_PTR)_r_obj_getarraysize (config.formats);
	hsubmenu = GetSubMenu (hmenu, FORMAT_MENU);

	if (hsubmenu)
	{
		_r_menu_clearitems (hsubmenu);

		for (UINT_PTR i = 0; i < formats_count; i++)
		{
			format = _r_obj_getarrayitem (config.formats, i);

			if (format)
				AppendMenu (hsubmenu, MF_BYPOSITION, IDX_FORMATS + i, format->ext);
		}
	}

	// initialize delays
	hsubmenu = GetSubMenu (hmenu, DELAY_MENU);

	if (hsubmenu)
	{
		for (UINT_PTR i = 0; i < RTL_NUMBER_OF (timer_array); i++)
		{
			string = _r_format_interval (timer_array[i], 1);

			if (string)
			{
				AppendMenu (hsubmenu, MF_BYPOSITION, IDX_DELAY + i, string->buffer);

				_r_obj_dereference (string);
			}
		}
	}

	delay_idx = _app_getdelay_id ();

	if (delay_idx == -1)
	{
		_r_menu_checkitem (hmenu, IDM_DELAY_DISABLE, IDM_DELAY_DISABLE, MF_BYCOMMAND, IDM_DELAY_DISABLE);
	}
	else
	{
		_r_menu_checkitem (hmenu, IDX_DELAY, IDX_DELAY + RTL_NUMBER_OF (timer_array), MF_BYCOMMAND, IDX_DELAY + delay_idx);
	}

	CheckMenuRadioItem (hmenu, IDX_FORMATS, IDX_FORMATS + (UINT)formats_count, IDX_FORMATS + _app_getimageformat_id (), MF_BYCOMMAND);
	CheckMenuRadioItem (hmenu, IDM_FILENAME_INDEX, IDM_FILENAME_DATE, IDM_FILENAME_INDEX + _app_getimagename_id (), MF_BYCOMMAND);

	CheckMenuItem (hmenu, IDM_INCLUDEMOUSECURSOR_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsIncludeMouseCursor", FALSE) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_INCLUDEWINDOWSHADOW_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsIncludeWindowShadow", TRUE) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_CLEARBACKGROUND_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsClearBackground", TRUE) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_DISABLEAEROONWND_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsDisableAeroOnWnd", FALSE) ? MF_CHECKED : MF_UNCHECKED));

	CheckMenuItem (hmenu, IDM_COPYTOCLIPBOARD_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"CopyToClipboard", FALSE) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_PLAYSOUNDS_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsPlaySound", TRUE) ? MF_CHECKED : MF_UNCHECKED));
}

VOID _app_initialize ()
{
	static R_INITONCE init_once = PR_INITONCE_INIT;

	if (_r_initonce_begin (&init_once))
	{
		WNDCLASSEX wcex;
		HCURSOR hcursor;

		hcursor = LoadCursor (_r_sys_getimagebase (), MAKEINTRESOURCE (IDI_MAIN));

		RtlZeroMemory (&wcex, sizeof (wcex));

		wcex.cbSize = sizeof (wcex);
		wcex.style = CS_VREDRAW | CS_HREDRAW;
		wcex.hInstance = _r_sys_getimagebase ();

		// register dummy class
		wcex.lpszClassName = DUMMY_CLASS_DLG;
		wcex.lpfnWndProc = &DefWindowProc;
		wcex.hbrBackground = GetSysColorBrush (COLOR_WINDOW);
		wcex.hCursor = hcursor;

		RegisterClassEx (&wcex);

		config.hdummy = CreateWindowEx (0, DUMMY_CLASS_DLG, _r_app_getname (), WS_POPUP | WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, _r_sys_getimagebase (), NULL);

		// register region class
		wcex.lpszClassName = REGION_CLASS_DLG;
		wcex.lpfnWndProc = &RegionProc;
		wcex.hCursor = hcursor;

		RegisterClassEx (&wcex);

		// register timer class
		wcex.lpszClassName = TIMER_CLASS_DLG;
		wcex.lpfnWndProc = &TimerProc;
		wcex.hCursor = hcursor;

		RegisterClassEx (&wcex);

		// create region event
		config.hregion_mutex = CreateEvent (NULL, TRUE, TRUE, NULL);

		// initialize formats
		LPCWSTR szext[] = {
			L"bmp",
			L"jpeg",
			L"png",
			L"jxr",
			L"tiff",
		};

		LPCGUID guids[] = {
			&GUID_ContainerFormatBmp,
			&GUID_ContainerFormatJpeg,
			&GUID_ContainerFormatPng,
			&GUID_ContainerFormatWmp,
			&GUID_ContainerFormatTiff,
		};

		C_ASSERT (RTL_NUMBER_OF (szext) == RTL_NUMBER_OF (guids));

		config.formats = _r_obj_createarray (sizeof (IMAGE_FORMAT), NULL);

		IMAGE_FORMAT image_format;

		for (SIZE_T i = 0; i < RTL_NUMBER_OF (szext); i++)
		{
			RtlZeroMemory (&image_format, sizeof (image_format));

			_r_str_copy (image_format.ext, RTL_NUMBER_OF (image_format.ext), szext[i]);
			RtlCopyMemory (&image_format.guid, guids[i], sizeof (image_format.guid));

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
			_r_app_sethwnd (hwnd); // HACK!!!

			// set edit control configuration
			SHAutoComplete (GetDlgItem (hwnd, IDC_FOLDER), SHACF_FILESYS_ONLY | SHACF_FILESYS_DIRS | SHACF_AUTOSUGGEST_FORCE_ON | SHACF_USETAB);

			_r_settings_addpage (IDD_SETTINGS_HOTKEYS, IDS_HOTKEYS);

			_app_initialize ();

			break;
		}

		case RM_INITIALIZE:
		{
			PR_STRING string;

			HMENU hmenu;
			HICON hicon;

			LONG dpi_value;

			LONG icon_small_x;
			LONG icon_small_y;

			dpi_value = _r_dc_gettaskbardpi ();

			icon_small_x = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);
			icon_small_y = _r_dc_getsystemmetrics (SM_CYSMICON, dpi_value);

			hicon = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCE (IDI_MAIN), icon_small_x, icon_small_y);

			// initialize tray icon
			_r_tray_create (hwnd, &GUID_TrayIcon, RM_TRAYICON, hicon, _r_app_getname (), FALSE);

			// set directory path
			string = _app_getdirectory ();
			_r_ctrl_setstring (hwnd, IDC_FOLDER, string->buffer);

			_r_obj_dereference (string);

			CheckDlgButton (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, _r_config_getboolean (L"IsIncludeMouseCursor", FALSE) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_INCLUDEWINDOWSHADOW_CHK, _r_config_getboolean (L"IsIncludeWindowShadow", TRUE) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_CLEARBACKGROUND_CHK, _r_config_getboolean (L"IsClearBackground", TRUE) ? BST_CHECKED : BST_UNCHECKED);

			CheckRadioButton (hwnd, IDC_MODE_MONITOR, IDC_MODE_REGION, IDC_MODE_MONITOR + _app_getmode_id () - 1);

			// configure menu
			hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				CheckMenuItem (hmenu, IDM_ALWAYSONTOP_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"AlwaysOnTop", FALSE) ? MF_CHECKED : MF_UNCHECKED));
				CheckMenuItem (hmenu, IDM_LOADONSTARTUP_CHK, MF_BYCOMMAND | (_r_autorun_isenabled () ? MF_CHECKED : MF_UNCHECKED));
				CheckMenuItem (hmenu, IDM_STARTMINIMIZED_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsStartMinimized", FALSE) ? MF_CHECKED : MF_UNCHECKED));
				CheckMenuItem (hmenu, IDM_HIDEME_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"IsHideMe", TRUE) ? MF_CHECKED : MF_UNCHECKED));
				CheckMenuItem (hmenu, IDM_CHECKUPDATES_CHK, MF_BYCOMMAND | (_r_update_isenabled (FALSE) ? MF_CHECKED : MF_UNCHECKED));
			}

			break;
		}

		case RM_TASKBARCREATED:
		{
			HICON hicon;

			LONG dpi_value;

			LONG icon_small_x;
			LONG icon_small_y;

			dpi_value = _r_dc_gettaskbardpi ();

			icon_small_x = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);
			icon_small_y = _r_dc_getsystemmetrics (SM_CYSMICON, dpi_value);

			hicon = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCE (IDI_MAIN), icon_small_x, icon_small_y);

			_r_tray_create (hwnd, &GUID_TrayIcon, RM_TRAYICON, hicon, _r_app_getname (), FALSE);

			break;
		}

		case RM_LOCALIZE:
		{
			HMENU hmenu;

			// configure button
			_r_ctrl_setstringformat (hwnd, IDC_TITLE_FOLDER, L"%s:", _r_locale_getstring (IDS_FOLDER));
			_r_ctrl_setstringformat (hwnd, IDC_TITLE_SETTINGS, L"%s:", _r_locale_getstring (IDS_QUICKSETTINGS));
			_r_ctrl_setstringformat (hwnd, IDC_TITLE_MODE, L"%s:", _r_locale_getstring (IDS_MODE));

			_r_ctrl_setstring (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, _r_locale_getstring (IDS_INCLUDEMOUSECURSOR_CHK));
			_r_ctrl_setstring (hwnd, IDC_INCLUDEWINDOWSHADOW_CHK, _r_locale_getstring (IDS_INCLUDEWINDOWSHADOW_CHK));
			_r_ctrl_setstring (hwnd, IDC_CLEARBACKGROUND_CHK, _r_locale_getstring (IDS_CLEARBACKGROUND_CHK));

			_r_ctrl_setstring (hwnd, IDC_MODE_MONITOR, _r_locale_getstring (IDS_MODE_MONITOR));
			_r_ctrl_setstring (hwnd, IDC_MODE_WINDOW, _r_locale_getstring (IDS_MODE_WINDOW));
			_r_ctrl_setstring (hwnd, IDC_MODE_REGION, _r_locale_getstring (IDS_MODE_REGION));

			_r_ctrl_setstring (hwnd, IDC_SETTINGS, _r_locale_getstring (IDS_SETTINGS));
			_r_ctrl_setstring (hwnd, IDC_SCREENSHOT, _r_locale_getstring (IDS_SCREENSHOT));
			_r_ctrl_setstring (hwnd, IDC_EXIT, _r_locale_getstring (IDS_EXIT));

			// localize menu
			hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				_r_menu_setitemtext (hmenu, 0, TRUE, _r_locale_getstring (IDS_FILE));
				_r_menu_setitemtext (hmenu, 1, TRUE, _r_locale_getstring (IDS_SETTINGS));
				_r_menu_setitemtextformat (GetSubMenu (hmenu, 1), LANG_MENU, TRUE, L"%s (Language)", _r_locale_getstring (IDS_LANGUAGE));
				_r_menu_setitemtext (hmenu, 2, TRUE, _r_locale_getstring (IDS_SCREENSHOT));
				_r_menu_setitemtext (hmenu, 3, TRUE, _r_locale_getstring (IDS_HELP));

				_r_menu_setitemtextformat (hmenu, IDM_EXPLORE, FALSE, L"%s...\tCtrl+E", _r_locale_getstring (IDS_EXPLORE));
				_r_menu_setitemtext (hmenu, IDM_EXIT, FALSE, _r_locale_getstring (IDS_EXIT));

				_r_menu_setitemtext (hmenu, IDM_ALWAYSONTOP_CHK, FALSE, _r_locale_getstring (IDS_ALWAYSONTOP_CHK));
				_r_menu_setitemtext (hmenu, IDM_LOADONSTARTUP_CHK, FALSE, _r_locale_getstring (IDS_LOADONSTARTUP_CHK));
				_r_menu_setitemtext (hmenu, IDM_STARTMINIMIZED_CHK, FALSE, _r_locale_getstring (IDS_STARTMINIMIZED_CHK));
				_r_menu_setitemtext (hmenu, IDM_HIDEME_CHK, FALSE, _r_locale_getstring (IDS_HIDEME_CHK));

				_r_menu_setitemtextformat (hmenu, IDM_TAKE_FULLSCREEN, FALSE, L"%s\tCtrl+1", _r_locale_getstring (IDS_MODE_FULLSCREEN));
				_r_menu_setitemtextformat (hmenu, IDM_TAKE_MONITOR, FALSE, L"%s\tCtrl+2", _r_locale_getstring (IDS_MODE_MONITOR));
				_r_menu_setitemtextformat (hmenu, IDM_TAKE_WINDOW, FALSE, L"%s\tCtrl+3", _r_locale_getstring (IDS_MODE_WINDOW));
				_r_menu_setitemtextformat (hmenu, IDM_TAKE_REGION, FALSE, L"%s\tCtrl+4", _r_locale_getstring (IDS_MODE_REGION));

				_r_menu_setitemtext (hmenu, IDM_CHECKUPDATES_CHK, FALSE, _r_locale_getstring (IDS_CHECKUPDATES_CHK));
				_r_menu_setitemtext (hmenu, IDM_WEBSITE, FALSE, _r_locale_getstring (IDS_WEBSITE));
				_r_menu_setitemtext (hmenu, IDM_CHECKUPDATES, FALSE, _r_locale_getstring (IDS_CHECKUPDATES));
				_r_menu_setitemtextformat (hmenu, IDM_ABOUT, FALSE, L"%s\tF1", _r_locale_getstring (IDS_ABOUT));

				_r_locale_enum (GetSubMenu (hmenu, 1), LANG_MENU, IDX_LANGUAGE); // enum localizations
			}

			break;
		}

		case RM_INITIALIZE_POST:
		{
			// initialize hotkey
			_app_hotkeyinit (hwnd, NULL);

			break;
		}

		case WM_DPICHANGED:
		{
			HICON hicon;

			LONG dpi_value;

			LONG icon_small_x;
			LONG icon_small_y;

			dpi_value = _r_dc_gettaskbardpi ();

			icon_small_x = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);
			icon_small_y = _r_dc_getsystemmetrics (SM_CYSMICON, dpi_value);

			hicon = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCE (IDI_MAIN), icon_small_x, icon_small_y);

			_r_tray_setinfo (hwnd, &GUID_TrayIcon, hicon, NULL);

			break;
		}

		case RM_UNINITIALIZE:
		{
			_r_tray_destroy (hwnd, &GUID_TrayIcon);
			break;
		}

		case WM_DESTROY:
		{
			SAFE_DELETE_HANDLE (config.hregion_mutex);

			UnregisterClass (DUMMY_CLASS_DLG, _r_sys_getimagebase ());
			UnregisterClass (REGION_CLASS_DLG, _r_sys_getimagebase ());

			_r_tray_destroy (hwnd, &GUID_TrayIcon);

			PostQuitMessage (0);

			break;
		}

		case WM_HOTKEY:
		{
			INT command_id;

			if (wparam == HOTKEY_ID_FULLSCREEN)
			{
				command_id = IDM_TAKE_FULLSCREEN;
			}
			else if (wparam == HOTKEY_ID_MONITOR)
			{
				command_id = IDM_TAKE_MONITOR;
			}
			else if (wparam == HOTKEY_ID_WINDOW)
			{
				command_id = IDM_TAKE_WINDOW;
			}
			else if (wparam == HOTKEY_ID_REGION)
			{
				command_id = IDM_TAKE_REGION;
			}
			else
			{
				break;
			}

			SendMessage (hwnd, WM_COMMAND, MAKEWPARAM (command_id, 0), 0);

			break;
		}

		case RM_TRAYICON:
		{
			switch (LOWORD (lparam))
			{
				case NIN_KEYSELECT:
				{
					if (GetForegroundWindow () != hwnd)
					{
						_r_wnd_toggle (hwnd, FALSE);
					}

					break;
				}

				case WM_LBUTTONUP:
				{
					PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDM_TRAY_SHOW, 0), 0);
					break;
				}

				case WM_MBUTTONUP:
				{
					PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDM_EXPLORE, 0), 0);
					break;
				}

				case WM_CONTEXTMENU:
				{
					SetForegroundWindow (hwnd); // don't touch

					HMENU hmenu;
					HMENU hsubmenu;
					HMENU hmenu_settings;

					hmenu = LoadMenu (NULL, MAKEINTRESOURCE (IDM_TRAY));

					if (hmenu)
					{
						hsubmenu = GetSubMenu (hmenu, 0);

						if (hsubmenu)
						{
							WCHAR mode_fullscreen[128];
							WCHAR mode_monitor[128];
							WCHAR mode_window[128];
							WCHAR mode_region[128];
							WCHAR key_string[128];

							_r_str_copy (mode_fullscreen, RTL_NUMBER_OF (mode_fullscreen), _r_locale_getstring (IDS_MODE_FULLSCREEN));
							_r_str_copy (mode_monitor, RTL_NUMBER_OF (mode_monitor), _r_locale_getstring (IDS_MODE_MONITOR));
							_r_str_copy (mode_window, RTL_NUMBER_OF (mode_window), _r_locale_getstring (IDS_MODE_WINDOW));
							_r_str_copy (mode_region, RTL_NUMBER_OF (mode_region), _r_locale_getstring (IDS_MODE_REGION));

							if (_r_config_getboolean (L"HotkeyFullscreenEnabled", FALSE))
							{
								_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getlong (L"HotkeyFullscreen2", HOTKEY_FULLSCREEN));
								_r_str_appendformat (mode_fullscreen, RTL_NUMBER_OF (mode_fullscreen), L"\t%s", key_string);
							}

							if (_r_config_getboolean (L"HotkeyMonitorEnabled", TRUE))
							{
								_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getlong (L"HotkeyMonitor", HOTKEY_MONITOR));
								_r_str_appendformat (mode_monitor, RTL_NUMBER_OF (mode_monitor), L"\t%s", key_string);
							}

							if (_r_config_getboolean (L"HotkeyWindowEnabled", TRUE))
							{
								_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getlong (L"HotkeyWindow", HOTKEY_WINDOW));
								_r_str_appendformat (mode_window, RTL_NUMBER_OF (mode_window), L"\t%s", key_string);
							}

							if (_r_config_getboolean (L"HotkeyRegionEnabled", TRUE))
							{
								_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getlong (L"HotkeyRegion", HOTKEY_REGION));
								_r_str_appendformat (mode_region, RTL_NUMBER_OF (mode_region), L"\t%s", key_string);
							}

							// localize
							_r_menu_setitemtext (hsubmenu, IDM_TRAY_SHOW, FALSE, _r_locale_getstring (IDS_TRAY_SHOW));
							_r_menu_setitemtextformat (hsubmenu, IDM_TRAY_TAKE_FULLSCREEN, FALSE, L"%s: %s", _r_locale_getstring (IDS_SCREENSHOT), mode_fullscreen);
							_r_menu_setitemtextformat (hsubmenu, IDM_TRAY_TAKE_MONITOR, FALSE, L"%s: %s", _r_locale_getstring (IDS_SCREENSHOT), mode_monitor);
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

								if (hsubmenu_settings)
								{
									_app_initdropdownmenu (hsubmenu_settings, false);

									MENUITEMINFO mii = {0};

									mii.cbSize = sizeof (mii);
									mii.fMask = MIIM_SUBMENU;
									mii.hSubMenu = hsubmenu_settings;

									SetMenuItemInfo (hsubmenu, SETTINGS_MENU, TRUE, &mii);
								}
							}

							_r_menu_popup (hsubmenu, hwnd, NULL, TRUE);

							if (hmenu_settings)
								DestroyMenu (hmenu_settings);
						}

						DestroyMenu (hmenu);
					}

					break;
				}
			}

			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR nmlp;
			LPNMBCDROPDOWN lpdropdown;

			nmlp = (LPNMHDR)lparam;

			switch (nmlp->code)
			{
				case BCN_DROPDOWN:
				{
					lpdropdown = (LPNMBCDROPDOWN)lparam;

					if (lpdropdown->hdr.idFrom == IDC_SETTINGS)
					{
						SendMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_SETTINGS, 0), 0);

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

			if (notify_code == 0 && ctrl_id >= IDX_LANGUAGE && ctrl_id <= IDX_LANGUAGE + (INT)(INT_PTR)_r_locale_getcount ())
			{
				_r_locale_apply (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), ctrl_id, IDX_LANGUAGE);

				return FALSE;
			}
			else if ((ctrl_id >= IDX_FORMATS && ctrl_id <= IDX_FORMATS + (INT)(INT_PTR)_r_obj_getarraysize (config.formats)))
			{
				INT index;

				index = (ctrl_id - IDX_FORMATS);

				_r_config_setlong (L"ImageFormat", _r_calc_clamp32 (index, 0, (LONG)_r_obj_getarraysize (config.formats) - 1));

				return FALSE;
			}
			else if ((ctrl_id >= IDX_DELAY && ctrl_id <= IDX_DELAY + RTL_NUMBER_OF (timer_array)))
			{
				INT index;

				index = (ctrl_id - IDX_DELAY);

				_r_config_setlong (L"Delay", _r_calc_clamp32 (index, 0, RTL_NUMBER_OF (timer_array)) + 1);

				return FALSE;
			}
			else if (ctrl_id == IDC_FOLDER && notify_code == EN_KILLFOCUS)
			{
				PR_STRING path_string;

				path_string = _r_ctrl_getstring (hwnd, ctrl_id);

				if (path_string)
				{
					_r_config_setstringexpand (L"Folder", path_string->buffer);

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
					RECT rect;
					HWND hbutton;
					HMENU hmenu;
					HMENU hsubmenu;

					hbutton = GetDlgItem (hwnd, IDC_SETTINGS);

					if (hbutton)
					{
						if (GetWindowRect (hbutton, &rect))
						{
							hmenu = LoadMenu (NULL, MAKEINTRESOURCE (IDM_SETTINGS));

							if (hmenu)
							{
								hsubmenu = GetSubMenu (hmenu, 0);

								if (hsubmenu)
								{
									_app_initdropdownmenu (hsubmenu, TRUE);

									_r_menu_popup (hsubmenu, hwnd, (PPOINT)&rect, TRUE);
								}

								DestroyMenu (hmenu);
							}
						}
					}

					break;
				}

				case IDM_WEBSITE:
				case IDM_TRAY_WEBSITE:
				{
					_r_shell_opendefault (_r_app_getwebsite_url ());
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
					PR_STRING string;

					string = _app_getdirectory ();

					_r_shell_opendefault (string->buffer);
					_r_obj_dereference (string);

					break;
				}

				case IDM_ALWAYSONTOP_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"AlwaysOnTop", FALSE);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_config_setboolean (L"AlwaysOnTop", new_val);

					_r_wnd_top (hwnd, new_val);

					break;
				}

				case IDM_LOADONSTARTUP_CHK:
				{
					BOOLEAN new_val = !_r_autorun_isenabled ();

					_r_autorun_enable (hwnd, new_val);
					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (_r_autorun_isenabled () ? MF_CHECKED : MF_UNCHECKED));

					break;
				}

				case IDM_STARTMINIMIZED_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"IsStartMinimized", FALSE);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_config_setboolean (L"IsStartMinimized", new_val);

					break;
				}

				case IDM_HIDEME_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"IsHideMe", TRUE);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_config_setboolean (L"IsHideMe", new_val);

					break;
				}

				case IDM_CHECKUPDATES_CHK:
				{
					BOOLEAN new_val = !_r_update_isenabled (FALSE);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_update_enable (new_val);

					break;
				}

				case IDC_BROWSE_BTN:
				{
					R_FILE_DIALOG file_dialog;
					PR_STRING path;

					if (_r_filedialog_initialize (&file_dialog, PR_FILEDIALOG_OPENDIR))
					{
						path = _app_getdirectory ();

						_r_filedialog_setpath (&file_dialog, path->buffer);
						_r_obj_dereference (path);

						if (_r_filedialog_show (hwnd, &file_dialog))
						{
							path = _r_filedialog_getpath (&file_dialog);

							if (path)
							{
								_r_config_setstringexpand (L"Folder", path->buffer);

								_r_ctrl_setstring (hwnd, IDC_FOLDER, path->buffer);

								_r_obj_dereference (path);
							}
						}

						_r_filedialog_destroy (&file_dialog);

					}

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
					_r_settings_createwindow (hwnd, &SettingsProc, IDD_SETTINGS_HOTKEYS);
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

				case IDM_DELAY_DISABLE:
				{
					_r_config_setlong (L"Delay", 0);
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

				case IDC_MODE_MONITOR:
				case IDC_MODE_WINDOW:
				case IDC_MODE_REGION:
				{
					if (ctrl_id == IDC_MODE_WINDOW)
					{
						_r_config_setlong (L"Mode", SHOT_WINDOW);
					}
					else if (ctrl_id == IDC_MODE_REGION)
					{
						_r_config_setlong (L"Mode", SHOT_REGION);
					}
					else if (ctrl_id == IDC_MODE_MONITOR)
					{
						_r_config_setlong (L"Mode", SHOT_MONITOR);
					}

					break;
				}

				case IDM_FILENAME_INDEX:
				case IDM_FILENAME_DATE:
				{
					if (ctrl_id == IDM_FILENAME_DATE)
					{
						_r_config_setlong (L"FilenameType", NAME_DATE);
					}
					else
					{
						_r_config_setlong (L"FilenameType", NAME_INDEX);
					}

					break;
				}

				case IDM_TAKE_FULLSCREEN:
				case IDM_TRAY_TAKE_FULLSCREEN:
				case IDM_TAKE_MONITOR:
				case IDM_TRAY_TAKE_MONITOR:
				case IDM_TAKE_WINDOW:
				case IDM_TRAY_TAKE_WINDOW:
				case IDM_TAKE_REGION:
				case IDM_TRAY_TAKE_REGION:
				case IDC_SCREENSHOT:
				{
					ENUM_TYPE_SCREENSHOT mode;
					HWND hwindow;

					if (ctrl_id == IDM_TRAY_TAKE_WINDOW || ctrl_id == IDM_TAKE_WINDOW)
					{
						mode = SHOT_WINDOW;
					}
					else if (ctrl_id == IDM_TRAY_TAKE_REGION || ctrl_id == IDM_TAKE_REGION)
					{
						mode = SHOT_REGION;
					}
					else if (ctrl_id == IDM_TAKE_FULLSCREEN || ctrl_id == IDM_TRAY_TAKE_FULLSCREEN)
					{
						mode = SHOT_FULLSCREEN;
					}
					else if (ctrl_id == IDM_TAKE_MONITOR || ctrl_id == IDM_TRAY_TAKE_MONITOR)
					{
						mode = SHOT_MONITOR;
					}
					else
					{
						mode = _app_getmode_id ();
					}

					if (mode == SHOT_WINDOW)
					{
						POINT pt;

						GetCursorPos (&pt);
						hwindow = WindowFromPoint (pt);

						if (!hwindow)
							break;

						hwindow = GetAncestor (hwindow, GA_ROOT);

						if (!hwindow)
							break;

						_r_wnd_toggle (hwindow, TRUE);
					}
					else
					{
						hwindow = NULL;
					}

					_app_screenshot (hwindow, mode);

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
	HWND hwnd;

	if (!_r_app_initialize ())
		return ERROR_APP_INIT_FAILURE;

	hwnd = _r_app_createwindow (MAKEINTRESOURCE (IDD_MAIN), MAKEINTRESOURCE (IDI_MAIN), &DlgProc);

	if (!hwnd)
		return ERROR_APP_INIT_FAILURE;

	return _r_wnd_messageloop (hwnd, MAKEINTRESOURCE (IDA_MAIN));
}

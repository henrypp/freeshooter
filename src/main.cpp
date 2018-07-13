// Free Shooter
// Copyright (c) 2009, 2010, 2018 Henry++

#include <windows.h>
#include <dwmapi.h>
#include <gdiplus.h>
#include <shlobj.h>

#include "main.hpp"
#include "rapp.hpp"
#include "routine.hpp"

#include "resource.hpp"

STATIC_DATA config;
ULONG_PTR gdiplusToken;

std::vector<IMAGE_FORMAT> formats;

rapp app (APP_NAME, APP_NAME_SHORT, APP_VERSION, APP_COPYRIGHT);

//void dump_wnd_info (HWND hwnd)
//{
//	WCHAR title[100] = {0};
//	GetWindowText (hwnd, title, _countof (title));
//
//	RECT rc = {0};
//	_app_getwindowrect (hwnd, &rc);
//
//	if (!title[0])
//		StringCchCopy (title, _countof (title), L"n/a");
//
//	RDBG (L"0x%08x | % 20s | left: %.4d, top: %.4d, width: %.4d, height: %.4d", hwnd, title, rc.left, rc.top, _R_RECT_WIDTH (&rc), _R_RECT_HEIGHT (&rc));
//}

size_t _app_getimageformat ()
{
	if (formats.empty ())
		return 0;

	const INT size = (INT)formats.size ();
	const INT current = app.ConfigGet (L"ImageFormat", FormatJpeg).AsInt ();

	return (max (min (current, size - 1), 0));
}

rstring _app_getdirectory ()
{
	rstring result = _r_path_expand (app.ConfigGet (L"Folder", config.default_folder));

	if (!_r_fs_exists (result))
		result = _r_path_expand (config.default_folder);

	return result;
}

rstring _app_uniquefilename (LPCWSTR directory, EnumImageName name_type)
{
	const rstring fext = formats.at (_app_getimageformat ()).ext;

	WCHAR result[MAX_PATH] = {0};

	if (name_type == NameDate)
	{
		WCHAR date_format[MAX_PATH] = {0};
		WCHAR time_format[MAX_PATH] = {0};

		SYSTEMTIME st = {0};
		GetLocalTime (&st);

		if (
			GetDateFormat (LOCALE_SYSTEM_DEFAULT, 0, &st, FILE_FORMAT_DATE_FORMAT_1, date_format, _countof (date_format)) &&
			GetTimeFormat (LOCALE_SYSTEM_DEFAULT, 0, &st, FILE_FORMAT_DATE_FORMAT_2, time_format, _countof (time_format))
			)
		{
			StringCchPrintf (result, _countof (result), L"%s\\%s %s.%s", directory, date_format, time_format, fext.GetString ());

			if (!_r_fs_exists (result))
				return result;

			if (PathYetAnotherMakeUniqueName (result, _r_fmt (L"%s\\%s %s.%s", directory, date_format, time_format, fext.GetString ()), nullptr, _r_fmt (L"%s %s.%s", date_format, time_format, fext.GetString ())))
				return result;
		}
	}
	else
	{
		static const USHORT idx = 1;
		const rstring name = app.ConfigGet (L"FilenamePrefix", FILE_FORMAT_NAME_PREFIX);

		for (USHORT i = idx; i < USHRT_MAX; i++)
		{
			StringCchPrintf (result, _countof (result), L"%s\\" FILE_FORMAT_NAME_FORMAT L".%s", directory, name.GetString (), i, fext.GetString ());

			if (!_r_fs_exists (result))
				return result;
		}



		if (PathYetAnotherMakeUniqueName (result, _r_fmt (L"%s\\%s.%s", directory, name.GetString (), fext.GetString ()), nullptr, _r_fmt (L"%s.%s", name.GetString (), fext.GetString ())))
			return result;
	}

	return result;
}

bool GetEncoderClsid (LPCWSTR exif, CLSID *pClsid)
{
	UINT num = 0, size = 0;
	const size_t len = wcslen (exif);
	Gdiplus::GetImageEncodersSize (&num, &size);

	if (size)
	{
		Gdiplus::ImageCodecInfo *pImageCodecInfo = (Gdiplus::ImageCodecInfo*)new BYTE[size];

		if (pImageCodecInfo)
		{
			GetImageEncoders (num, size, pImageCodecInfo);

			for (unsigned int j = 0; j < num; ++j)
			{
				if (_wcsnicmp (pImageCodecInfo[j].MimeType, exif, len) == 0)
				{
					*pClsid = pImageCodecInfo[j].Clsid;
					delete[] pImageCodecInfo;

					return true;
				}
			}

			delete[] pImageCodecInfo;
		}
	}

	return false;
}

bool _app_savehbitmap (HBITMAP hbitmap, LPCWSTR filepath)
{
	bool result = false;

	Gdiplus::Bitmap *pScreenShot = new Gdiplus::Bitmap (hbitmap, nullptr);

	if (pScreenShot)
	{
		CLSID* imageCLSID = &formats.at (_app_getimageformat ()).clsid;
		ULONG uQuality = app.ConfigGet (L"JPEGQuality", JPEG_QUALITY).AsUlong ();

		Gdiplus::EncoderParameters encoderParams = {0};

		encoderParams.Count = 1;
		encoderParams.Parameter[0].NumberOfValues = 1;
		encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
		encoderParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
		encoderParams.Parameter[0].Value = &uQuality;

		result = (pScreenShot->Save (filepath, imageCLSID, &encoderParams) == Gdiplus::Ok);

		delete pScreenShot;
	}

	return result;
}

HBITMAP _app_createbitmap (HDC hdc, LONG width, LONG height)
{
	BITMAPINFO bmi = {0};

	bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32; // four 8-bit components
	bmi.bmiHeader.biSizeImage = (width * height) * 4;

	//LPBYTE lpCapture = nullptr;

	return CreateDIBSection (hdc, &bmi, DIB_RGB_COLORS, nullptr /*(LPVOID*)&lpCapture*/, nullptr, 0);
}

void _app_dofinishjob (HBITMAP hbitmap, INT width, INT height)
{
	if (app.ConfigGet (L"CopyToClipboard", false).AsBool ())
	{
		if (OpenClipboard (app.GetHWND ()))
		{
			if (EmptyClipboard ())
			{
				HBITMAP hbitmap_copy = (HBITMAP)CopyImage (hbitmap, IMAGE_BITMAP, width, height, 0);

				SetClipboardData (CF_BITMAP, hbitmap_copy);

				CloseClipboard ();
			}

			CloseClipboard ();
		}
	}

	WCHAR full_path[MAX_PATH] = {0};
	StringCchCopy (full_path, _countof (full_path), _app_uniquefilename (_app_getdirectory (), (EnumImageName)app.ConfigGet (L"FilenameType", NameIndex).AsUint ()));

	_app_savehbitmap (hbitmap, full_path);
}

void _app_screenshot (INT x, INT y, INT width, INT height, bool is_cursor)
{
	const HWND hdesktop = GetDesktopWindow ();
	const HDC hdc = GetDC (hdesktop);
	const HDC hcapture = CreateCompatibleDC (hdc);

	const HBITMAP hbitmap = _app_createbitmap (hdc, width, height);

	if (hbitmap)
	{
		SelectObject (hcapture, hbitmap);
		BitBlt (hcapture, 0, 0, width, height, hdc, x, y, SRCCOPY);

		if (is_cursor)
		{
			CURSORINFO cursorinfo = {0};
			cursorinfo.cbSize = sizeof (cursorinfo);

			if (GetCursorInfo (&cursorinfo))
			{
				const HICON hicon = CopyIcon (cursorinfo.hCursor);

				if (hicon)
				{
					ICONINFO iconinfo = {0};
					GetIconInfo (hicon, &iconinfo);

					DrawIcon (hcapture, cursorinfo.ptScreenPos.x - iconinfo.xHotspot - x, cursorinfo.ptScreenPos.y - iconinfo.yHotspot - y, hicon);

					DestroyIcon (hicon);
				}
			}
		}

		_app_dofinishjob (hbitmap, width, height);

		DeleteObject (hbitmap);
	}

	DeleteDC (hcapture);
	ReleaseDC (hdesktop, hdc);
}

void _app_switchaeroonwnd (HWND hwnd, bool is_disable)
{
	const HMODULE hlib = GetModuleHandle (L"dwmapi.dll");

	if (hlib)
	{
		const DWMSWA _DwmSetWindowAttribute = (DWMSWA)GetProcAddress (hlib, "DwmSetWindowAttribute"); // vista+

		if (_DwmSetWindowAttribute)
		{
			const DWMNCRENDERINGPOLICY policy = is_disable ? DWMNCRP_DISABLED : DWMNCRP_ENABLED;

			_DwmSetWindowAttribute (hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof (policy));
		}
	}
}

void _app_getwindowrect (HWND hwnd, LPRECT lprect)
{
	bool is_dwmsuccess = false;

	if (app.IsVistaOrLater ())
	{
		const HMODULE hlib = GetModuleHandle (L"dwmapi.dll");

		if (hlib)
		{
			const DWMGWA _DwmGetWindowAttribute = (DWMGWA)GetProcAddress (hlib, "DwmGetWindowAttribute"); // vista+

			if (_DwmGetWindowAttribute && _DwmGetWindowAttribute (hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, lprect, sizeof (RECT)) == S_OK)
				is_dwmsuccess = true;
		}
	}

	if (!is_dwmsuccess)
		GetWindowRect (hwnd, lprect);
}

bool _app_isnormalwindow (HWND hwnd)
{
	return hwnd && IsWindowVisible (hwnd) && !IsIconic (hwnd) && hwnd != GetShellWindow () && hwnd != GetDesktopWindow ();
}

int _app_getwindowzorder (HWND hwnd)
{
	int z = 0;

	for (HWND h = hwnd; h != nullptr; h = GetWindow (h, GW_HWNDPREV)) z++;

	return z;
}

bool _app_iswndoverlapped (HWND hwnd, LPRECT lprect)
{
	RECT rc = {0};
	_app_getwindowrect (hwnd, &rc);

	POINT pt = {0};

	pt.x = rc.left;
	pt.y = rc.top;

	if (PtInRect (lprect, pt))
		return true;

	pt.x = rc.left;
	pt.y = rc.bottom;

	if (PtInRect (lprect, pt))
		return true;

	pt.x = rc.right;
	pt.y = rc.top;

	if (PtInRect (lprect, pt))
		return true;

	pt.x = rc.bottom;
	pt.y = rc.right;

	if (PtInRect (lprect, pt))
		return true;

	return false;
}

BOOL CalculateChildsRect (HWND hwnd, LPARAM lparam)
{
	ENUM_INFO* enum_info = (ENUM_INFO*)lparam;

	if (
		!enum_info ||
		(enum_info->hroot == hwnd) ||
		!_app_isnormalwindow (hwnd) ||
		!_app_iswndoverlapped (hwnd, enum_info->lprect) ||
		(_app_getwindowzorder (hwnd) > _app_getwindowzorder (enum_info->hroot))
		)
	{
		return TRUE;
	}

	RECT rc = {0};
	_app_getwindowrect (hwnd, &rc);

	if (rc.left < enum_info->lprect->left)
		enum_info->lprect->left -= (enum_info->lprect->left - rc.left);

	if (rc.top < enum_info->lprect->top)
		enum_info->lprect->top -= (enum_info->lprect->top - rc.top);

	if (rc.bottom > enum_info->lprect->bottom)
		enum_info->lprect->bottom += (rc.bottom - enum_info->lprect->bottom);

	if (rc.right > enum_info->lprect->right)
		enum_info->lprect->right += (rc.right - enum_info->lprect->right);

	return TRUE;
}

void _app_getshadowsize (PINT px, PINT py)
{
	if (!px || !py)
		return;

	INT shadowX, shadowY;
	const INT fallbackX = GetSystemMetrics (SM_CXSIZEFRAME) * 2;
	const INT fallbackY = GetSystemMetrics (SM_CYSIZEFRAME) * 2;

	if (app.ConfigGet (L"IsCustomShadow", false).AsBool ())
	{
		shadowX = app.GetDPI (app.ConfigGet (L"CustomShadowX", fallbackX).AsInt ());
		shadowY = app.GetDPI (app.ConfigGet (L"CustomShadowY", fallbackY).AsInt ());
	}
	else
	{
		shadowX = fallbackX;
		shadowY = fallbackY;
	}

	if (!shadowX)
		shadowX = fallbackX;

	if (!shadowY)
		shadowY = fallbackY;

	*px = max (shadowX, 0);
	*py = max (shadowY, 0);
}

void _app_takeshot (HWND hwnd, EnumScreenshot mode)
{
	bool result = false;

	const HWND myWindow = app.GetHWND ();

	const bool is_includecursor = app.ConfigGet (L"IsIncludeMouseCursor", false).AsBool ();
	const bool is_hideme = app.ConfigGet (L"IsHideMe", true).AsBool ();
	const bool is_windowdisplayed = IsWindowVisible (myWindow) && !IsIconic (myWindow);

	RECT prev_rect = {0};

	if (is_hideme)
	{
		if (is_windowdisplayed)
			ShowWindow (myWindow, SW_HIDE);

		app.TrayToggle (myWindow, UID, nullptr, false);
	}

	if (mode != ScreenshotRegion && app.ConfigGet (L"IsPlaySound", true).AsBool ())
		PlaySound (MAKEINTRESOURCE (IDW_MAIN), app.GetHINSTANCE (), SND_SENTRY | SND_RESOURCE | SND_ASYNC);

	if (mode == ScreenshotFullscreen)
	{
		POINT pt = {0};
		GetCursorPos (&pt);

		const HMONITOR hmonitor = MonitorFromPoint (pt, MONITOR_DEFAULTTONEAREST);

		MONITORINFO monitorInfo = {0};
		monitorInfo.cbSize = sizeof (monitorInfo);

		if (GetMonitorInfo (hmonitor, &monitorInfo))
		{
			const LPRECT lprc = &monitorInfo.rcMonitor;
			_app_screenshot (lprc->left, lprc->top, _R_RECT_WIDTH (lprc), _R_RECT_HEIGHT (lprc), is_includecursor);
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
			HWND hdummy = nullptr;

			const bool is_includewindowshadow = app.ConfigGet (L"IsIncludeWindowShadow", true).AsBool ();
			const bool is_clearbackground = app.ConfigGet (L"IsClearBackground", true).AsBool ();
			const bool is_disableaeroonwnd = app.IsVistaOrLater () && app.ConfigGet (L"IsDisableAeroOnWnd", false).AsBool ();

			if (is_disableaeroonwnd)
				_app_switchaeroonwnd (hwnd, true);

			RECT window_rect = {0};
			_app_getwindowrect (hwnd, &window_rect);

			// calculate window rectangle and all overlapped windows
			{
				ENUM_INFO enuminfo = {0};

				enuminfo.hroot = hwnd;
				enuminfo.lprect = &window_rect;

				EnumWindows (&CalculateChildsRect, (LPARAM)&enuminfo);
			}

			// calculate shadow padding
			if (is_includewindowshadow)
			{
				int shadowX, shadowY;
				_app_getshadowsize (&shadowX, &shadowY);

				window_rect.left -= shadowX;
				window_rect.right += shadowX;

				window_rect.top -= shadowY;
				window_rect.bottom += shadowY;
			}

			if (is_clearbackground)
			{
				hdummy = CreateWindowEx (0, DUMMY_CLASS_DLG, APP_NAME, WS_POPUP | WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, app.GetHINSTANCE (), nullptr);

				if (hdummy)
				{
					if (!SetWindowPos (hdummy, hwnd, window_rect.left, window_rect.top, _R_RECT_WIDTH (&window_rect), _R_RECT_HEIGHT (&window_rect), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOSENDCHANGING))
					{
						// fucked uipi-fix
						SetWindowPos (hdummy, HWND_BOTTOM, window_rect.left, window_rect.top, _R_RECT_WIDTH (&window_rect), _R_RECT_HEIGHT (&window_rect), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
					}
				}
			}

			_r_sleep (WND_SLEEP);
			_app_screenshot (window_rect.left, window_rect.top, _R_RECT_WIDTH (&window_rect), _R_RECT_HEIGHT (&window_rect), is_includecursor);

			if (is_disableaeroonwnd)
				_app_switchaeroonwnd (hwnd, false);

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
			config.hregion = CreateWindowEx (WS_EX_TOPMOST, REGION_CLASS_DLG, APP_NAME, WS_POPUP | WS_OVERLAPPED, 0, 0, 0, 0, myWindow, nullptr, app.GetHINSTANCE (), nullptr);

			if (!config.hregion)
			{
				SetEvent (config.hregion_mutex);
			}
			else
			{
				SetWindowPos (config.hregion, HWND_TOPMOST, 0, 0, GetSystemMetrics (SM_CXVIRTUALSCREEN), GetSystemMetrics (SM_CYVIRTUALSCREEN), SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
			}
		}
	}

	if (is_hideme)
	{
		if (is_windowdisplayed)
			ShowWindow (myWindow, SW_SHOWNA);

		app.TrayToggle (myWindow, UID, nullptr, true);
		app.TraySetInfo (myWindow, UID, nullptr, nullptr, APP_NAME);
	}
}

rstring _app_key2string (UINT key)
{
	rstring result;

	if ((HIBYTE (key) & HOTKEYF_CONTROL) != 0)
		result.Append (L"Ctrl+");

	if ((HIBYTE (key) & HOTKEYF_ALT) != 0)
		result.Append (L"Alt+");

	if ((HIBYTE (key) & HOTKEYF_SHIFT) != 0)
		result.Append (L"Shift+");

	WCHAR name[64] = {0};

	if (LOBYTE (key) == VK_SNAPSHOT)
	{
		StringCchCopy (name, _countof (name), L"Prnt scrn");
	}
	else
	{
		const UINT scan_code = MapVirtualKey (LOBYTE (key), MAPVK_VK_TO_VSC);
		GetKeyNameText ((scan_code << 16), name, _countof (name));
	}

	return result.Append (name);
}

void _app_hotkeyinit (HWND hwnd)
{
	bool is_nofullscreen = false;
	bool is_nowindow = false;
	bool is_noregion = false;

	UnregisterHotKey (hwnd, HOTKEY_ID_FULLSCREEN);
	UnregisterHotKey (hwnd, HOTKEY_ID_WINDOW);
	UnregisterHotKey (hwnd, HOTKEY_ID_REGION);

	const UINT hk_fullscreen = app.ConfigGet (L"HotkeyFullscreen", HOTKEY_FULLSCREEN).AsUint ();
	const UINT hk_window = app.ConfigGet (L"HotkeyWindow", HOTKEY_WINDOW).AsUint ();
	const UINT hk_region = app.ConfigGet (L"HotkeyRegion", HOTKEY_REGION).AsUint ();

	if (app.ConfigGet (L"HotkeyFullscreenEnabled", true).AsBool ())
	{
		if (hk_fullscreen)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_FULLSCREEN, (HIBYTE (hk_fullscreen) & 2) | ((HIBYTE (hk_fullscreen) & 4) >> 2) | ((HIBYTE (hk_fullscreen) & 1) << 2), LOBYTE (hk_fullscreen)))
				is_nofullscreen = true;
		}
	}

	if (app.ConfigGet (L"HotkeyWindowEnabled", true).AsBool ())
	{
		if (hk_window)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_WINDOW, (HIBYTE (hk_window) & 2) | ((HIBYTE (hk_window) & 4) >> 2) | ((HIBYTE (hk_window) & 1) << 2), LOBYTE (hk_window)))
				is_nowindow = true;
		}
	}

	if (app.ConfigGet (L"HotkeyRegionEnabled", true).AsBool ())
	{
		if (hk_region)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_REGION, (HIBYTE (hk_region) & 2) | ((HIBYTE (hk_region) & 4) >> 2) | ((HIBYTE (hk_region) & 1) << 2), LOBYTE (hk_region)))
				is_noregion = true;
		}
	}

	// show warning
	if (is_nofullscreen || is_nowindow || is_noregion)
	{
		rstring buffer;

		if (is_nofullscreen)
			buffer.AppendFormat (L"%s [%s]\r\n", app.LocaleString (IDS_MODE_FULLSCREEN, nullptr).GetString (), _app_key2string (hk_fullscreen).GetString ());

		if (is_nowindow)
			buffer.AppendFormat (L"%s [%s]\r\n", app.LocaleString (IDS_MODE_WINDOW, nullptr).GetString (), _app_key2string (hk_window).GetString ());

		if (is_noregion)
			buffer.AppendFormat (L"%s [%s]\r\n", app.LocaleString (IDS_MODE_REGION, nullptr).GetString (), _app_key2string (hk_region).GetString ());

		app.ConfirmMessage (hwnd, app.LocaleString (IDS_WARNING_HOTKEYS, nullptr), buffer.Trim (L"\r\n"), L"IsWarnHotkeys");
	}
}

LRESULT CALLBACK RegionProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static HDC hcapture = nullptr;
	static HDC hcapture_mask = nullptr;

	static HBITMAP hbitmap = nullptr;
	static HBITMAP hbitmap_mask = nullptr;

	static HPEN hpen = nullptr;

	static RECT wndRect = {0};

	static POINT ptStart = {0};
	static POINT ptEnd = {0};

	static bool fDraw = false;

	switch (msg)
	{
		case WM_CREATE:
		{
			ResetEvent (config.hregion_mutex);

			wndRect.right = GetSystemMetrics (SM_CXVIRTUALSCREEN);
			wndRect.bottom = GetSystemMetrics (SM_CYVIRTUALSCREEN);

			const HWND hdesktop = GetDesktopWindow ();
			const HDC hdc = GetDC (hdesktop);

			hpen = CreatePen (PS_SOLID, app.GetDPI (REGION_PEN_SIZE), REGION_PEN_COLOR);

			hbitmap = _app_createbitmap (hdc, wndRect.right, wndRect.bottom);
			hbitmap_mask = _app_createbitmap (hdc, wndRect.right, wndRect.bottom);

			if (hbitmap)
			{
				hcapture = CreateCompatibleDC (hdc);

				SelectObject (hcapture, hbitmap);
				BitBlt (hcapture, 0, 0, wndRect.right, wndRect.bottom, hdc, 0, 0, SRCCOPY);
			}

			if (hbitmap_mask)
			{
				hcapture_mask = CreateCompatibleDC (hdc);

				if (hcapture_mask)
				{
					SelectObject (hcapture_mask, hbitmap_mask);

					SetDCPenColor (hcapture_mask, REGION_COLOR_BK);
					SetDCBrushColor (hcapture_mask, REGION_COLOR_BK);

					_r_dc_fillrect (hcapture_mask, &wndRect, REGION_COLOR_BK);

					BLENDFUNCTION blend = {AC_SRC_OVER, 0, (BYTE)REGION_BLEND, 0};
					AlphaBlend (hdc, wndRect.left, wndRect.top, wndRect.right, wndRect.bottom, hcapture_mask, wndRect.left, wndRect.top, wndRect.right, wndRect.bottom, blend);

					BitBlt (hcapture_mask, 0, 0, wndRect.right, wndRect.bottom, hdc, 0, 0, SRCCOPY);
				}
			}

			ReleaseDC (hdesktop, hdc);

			break;
		}

		case WM_DESTROY:
		{
			SetEvent (config.hregion_mutex);

			fDraw = false;

			if (hpen)
				DeleteObject (hpen);

			if (hbitmap)
				DeleteObject (hbitmap);

			if (hbitmap_mask)
				DeleteObject (hbitmap_mask);

			if (hcapture)
				DeleteDC (hcapture);

			if (hcapture_mask)
				DeleteDC (hcapture_mask);

			return TRUE;
		}

		case WM_LBUTTONDOWN:
		{
			if (!fDraw)
			{
				fDraw = true;

				ptStart.x = LOWORD (lparam);
				ptStart.y = HIWORD (lparam);
			}
			else
			{
				fDraw = false;

				ptEnd.x = LOWORD (lparam);
				ptEnd.y = HIWORD (lparam);

				const INT x = min (ptStart.x, ptEnd.x);
				const INT y = min (ptStart.y, ptEnd.y);
				const INT width = max (ptStart.x, ptEnd.x) - x;
				const INT height = max (ptStart.y, ptEnd.y) - y;

				if (width && height)
				{
					// save region to a file
					const HWND hdesktop = GetDesktopWindow ();

					const HDC hdc = GetDC (hdesktop);
					const HDC hcapture_finish = CreateCompatibleDC (hdc);

					if (hcapture_finish)
					{
						const HBITMAP hbitmap_finish = _app_createbitmap (hdc, width, height);

						if (hbitmap_finish)
						{
							SelectObject (hcapture_finish, hbitmap_finish);
							BitBlt (hcapture_finish, 0, 0, width, height, hcapture, x, y, SRCCOPY);

							if (app.ConfigGet (L"IsPlaySound", true).AsBool ())
								PlaySound (MAKEINTRESOURCE (IDW_MAIN), app.GetHINSTANCE (), SND_SENTRY | SND_RESOURCE | SND_ASYNC);

							_app_dofinishjob (hbitmap_finish, width, height);

							DeleteObject (hbitmap_finish);
						}

						DeleteDC (hcapture_finish);
					}

					ReleaseDC (hdesktop, hdc);
					DestroyWindow (hwnd);
				}
			}

			return TRUE;
		}

		case WM_MBUTTONDOWN:
		{
			if (fDraw)
			{
				fDraw = false;
				ptStart.x = ptStart.y = ptEnd.x = ptEnd.y = 0;

				InvalidateRect (hwnd, nullptr, TRUE);
			}
			else
			{
				DestroyWindow (hwnd);
			}

			return TRUE;
		}

		case WM_MOUSEMOVE:
		{
			InvalidateRect (hwnd, nullptr, TRUE);
			return TRUE;
		}

		case WM_ERASEBKGND:
		{
			const HDC hdc = (HDC)wparam;

			POINT pt = {0};
			GetCursorPos (&pt);

			BitBlt (hdc, wndRect.left, wndRect.top, _R_RECT_WIDTH (&wndRect), _R_RECT_HEIGHT (&wndRect), hcapture_mask, wndRect.left, wndRect.top, SRCCOPY);

			const HPEN old_pen = (HPEN)SelectObject (hdc, hpen);
			const HGDIOBJ old_brush = SelectObject (hdc, GetStockObject (NULL_BRUSH));

			if (fDraw)
			{
				// draw region rectangle
				BitBlt (hdc, min (ptStart.x, pt.x), min (ptStart.y, pt.y), max (ptStart.x, pt.x) - min (ptStart.x, pt.x), max (ptStart.y, pt.y) - min (ptStart.y, pt.y), hcapture, min (ptStart.x, pt.x), min (ptStart.y, pt.y), SRCCOPY);

				Rectangle (hdc, ptStart.x, ptStart.y, pt.x, pt.y);
			}
			else
			{
				LPRECT lpcrosshairRect = &wndRect;

				// draw cursor crosshair
				MoveToEx (hdc, pt.x, lpcrosshairRect->top, nullptr);
				LineTo (hdc, pt.x, _R_RECT_HEIGHT (lpcrosshairRect));

				MoveToEx (hdc, lpcrosshairRect->left, pt.y, nullptr);
				LineTo (hdc, _R_RECT_WIDTH (lpcrosshairRect), pt.y);
			}

			SelectObject (hdc, old_brush);
			SelectObject (hdc, old_pen);

			return TRUE;
		}

		case WM_KEYDOWN:
		{
			switch (wparam)
			{
				case VK_ESCAPE:
				{
					if (fDraw)
					{
						fDraw = false;
						ptStart.x = ptStart.y = ptEnd.x = ptEnd.y = 0;

						InvalidateRect (hwnd, nullptr, true);
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

INT_PTR CALLBACK HotkeysProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			// configure window
			_r_wnd_center (hwnd, GetParent (hwnd));

			// localize window
			SetWindowText (hwnd, app.LocaleString (IDS_HOTKEYS, nullptr));

			SetDlgItemText (hwnd, IDC_TITLE_FULLSCREEN, app.LocaleString (IDS_MODE_FULLSCREEN, L":"));
			SetDlgItemText (hwnd, IDC_TITLE_WINDOW, app.LocaleString (IDS_MODE_WINDOW, L":"));
			SetDlgItemText (hwnd, IDC_TITLE_REGION, app.LocaleString (IDS_MODE_REGION, L":"));

			SetDlgItemText (hwnd, IDC_SAVE, app.LocaleString (IDS_SAVE, nullptr));
			SetDlgItemText (hwnd, IDC_CLOSE, app.LocaleString (IDS_CLOSE, nullptr));

			SendDlgItemMessage (hwnd, IDC_FULLSCREEN_CB, CB_INSERTSTRING, 0, (LPARAM)app.LocaleString (IDS_DISABLE, nullptr).GetString ());
			SendDlgItemMessage (hwnd, IDC_WINDOW_CB, CB_INSERTSTRING, 0, (LPARAM)app.LocaleString (IDS_DISABLE, nullptr).GetString ());
			SendDlgItemMessage (hwnd, IDC_REGION_CB, CB_INSERTSTRING, 0, (LPARAM)app.LocaleString (IDS_DISABLE, nullptr).GetString ());

			// show config
			const UINT fullscreen_code = app.ConfigGet (L"HotkeyFullscreen", HOTKEY_FULLSCREEN).AsUint ();
			const UINT window_code = app.ConfigGet (L"HotkeyWindow", HOTKEY_WINDOW).AsUint ();
			const UINT region_code = app.ConfigGet (L"HotkeyRegion", HOTKEY_REGION).AsUint ();

			const bool fullscreen_allowed = app.ConfigGet (L"HotkeyFullscreenEnabled", true).AsBool ();
			const bool window_allowed = app.ConfigGet (L"HotkeyWindowEnabled", true).AsBool ();
			const bool region_allowed = app.ConfigGet (L"HotkeyRegionEnabled", true).AsBool ();

			CheckDlgButton (hwnd, IDC_FULLSCREEN_SHIFT, ((HIBYTE (fullscreen_code) & HOTKEYF_SHIFT) != 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_FULLSCREEN_CTRL, ((HIBYTE (fullscreen_code) & HOTKEYF_CONTROL) != 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_FULLSCREEN_ALT, ((HIBYTE (fullscreen_code) & HOTKEYF_ALT) != 0) ? BST_CHECKED : BST_UNCHECKED);

			CheckDlgButton (hwnd, IDC_WINDOW_SHIFT, ((HIBYTE (window_code) & HOTKEYF_SHIFT) != 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_WINDOW_CTRL, ((HIBYTE (window_code) & HOTKEYF_CONTROL) != 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_WINDOW_ALT, ((HIBYTE (window_code) & HOTKEYF_ALT) != 0) ? BST_CHECKED : BST_UNCHECKED);

			CheckDlgButton (hwnd, IDC_REGION_SHIFT, ((HIBYTE (region_code) & HOTKEYF_SHIFT) != 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_REGION_CTRL, ((HIBYTE (region_code) & HOTKEYF_CONTROL) != 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_REGION_ALT, ((HIBYTE (region_code) & HOTKEYF_ALT) != 0) ? BST_CHECKED : BST_UNCHECKED);

			static const UINT keys[] = {
				VK_SNAPSHOT,
				VK_SPACE,
				VK_RETURN,
				VK_F1,
				VK_F2,
				VK_F3,
				VK_F4,
				VK_F5,
				VK_F6,
				VK_F7,
				VK_F8,
				VK_F9,
				VK_F10,
				VK_F11,
				VK_F12
			};

			for (INT i = 0; i < _countof (keys); i++)
			{
				WCHAR name[64] = {0};
				StringCchCopy (name, _countof (name), _app_key2string (MAKEWORD (keys[i], 0)));

				const UINT idx = i + 1;

				SendDlgItemMessage (hwnd, IDC_FULLSCREEN_CB, CB_INSERTSTRING, idx, (LPARAM)name);
				SendDlgItemMessage (hwnd, IDC_WINDOW_CB, CB_INSERTSTRING, idx, (LPARAM)name);
				SendDlgItemMessage (hwnd, IDC_REGION_CB, CB_INSERTSTRING, idx, (LPARAM)name);

				SendDlgItemMessage (hwnd, IDC_FULLSCREEN_CB, CB_SETITEMDATA, idx, (LPARAM)keys[i]);
				SendDlgItemMessage (hwnd, IDC_WINDOW_CB, CB_SETITEMDATA, idx, (LPARAM)keys[i]);
				SendDlgItemMessage (hwnd, IDC_REGION_CB, CB_SETITEMDATA, idx, (LPARAM)keys[i]);

				if (fullscreen_allowed && LOBYTE (fullscreen_code) == keys[i])
					SendDlgItemMessage (hwnd, IDC_FULLSCREEN_CB, CB_SETCURSEL, idx, 0);

				if (window_allowed && LOBYTE (window_code) == keys[i])
					SendDlgItemMessage (hwnd, IDC_WINDOW_CB, CB_SETCURSEL, idx, 0);

				if (region_allowed && LOBYTE (region_code) == keys[i])
					SendDlgItemMessage (hwnd, IDC_REGION_CB, CB_SETCURSEL, idx, 0);
			}

			if (SendDlgItemMessage (hwnd, IDC_FULLSCREEN_CB, CB_GETCURSEL, 0, 0) == CB_ERR)
				SendDlgItemMessage (hwnd, IDC_FULLSCREEN_CB, CB_SETCURSEL, 0, 0);

			if (SendDlgItemMessage (hwnd, IDC_WINDOW_CB, CB_GETCURSEL, 0, 0) == CB_ERR)
				SendDlgItemMessage (hwnd, IDC_WINDOW_CB, CB_SETCURSEL, 0, 0);

			if (SendDlgItemMessage (hwnd, IDC_REGION_CB, CB_GETCURSEL, 0, 0) == CB_ERR)
				SendDlgItemMessage (hwnd, IDC_REGION_CB, CB_SETCURSEL, 0, 0);

			_r_wnd_addstyle (hwnd, IDC_SAVE, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);
			_r_wnd_addstyle (hwnd, IDC_CLOSE, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);

			PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_FULLSCREEN_CB, CBN_SELCHANGE), 0);
			PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_WINDOW_CB, CBN_SELCHANGE), 0);
			PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_REGION_CB, CBN_SELCHANGE), 0);

			break;
		}

		case WM_COMMAND:
		{
			if (HIWORD (wparam) == CBN_SELCHANGE && (LOWORD (wparam) == IDC_FULLSCREEN_CB || LOWORD (wparam) == IDC_WINDOW_CB || LOWORD (wparam) == IDC_REGION_CB))
			{
				const bool is_disable = (DWORD)SendDlgItemMessage (hwnd, LOWORD (wparam), CB_GETCURSEL, 0, 0) > 0;

				_r_ctrl_enable (hwnd, LOWORD (wparam) - 3, is_disable); // shift
				_r_ctrl_enable (hwnd, LOWORD (wparam) - 2, is_disable); // ctrl
				_r_ctrl_enable (hwnd, LOWORD (wparam) - 1, is_disable); // alt

				break;
			}

			switch (LOWORD (wparam))
			{
				case IDOK: // process Enter key
				case IDC_SAVE:
				{
					const DWORD fullscreen_idx = (DWORD)SendDlgItemMessage (hwnd, IDC_FULLSCREEN_CB, CB_GETCURSEL, 0, 0);
					const DWORD window_idx = (DWORD)SendDlgItemMessage (hwnd, IDC_WINDOW_CB, CB_GETCURSEL, 0, 0);
					const DWORD region_idx = (DWORD)SendDlgItemMessage (hwnd, IDC_REGION_CB, CB_GETCURSEL, 0, 0);

					app.ConfigSet (L"HotkeyFullscreenEnabled", bool (fullscreen_idx > 0));
					app.ConfigSet (L"HotkeyWindowEnabled", bool (window_idx > 0));
					app.ConfigSet (L"HotkeyRegionEnabled", bool (region_idx > 0));

					if (fullscreen_idx > 0)
					{
						DWORD modifiers = 0;

						if (IsDlgButtonChecked (hwnd, IDC_FULLSCREEN_SHIFT) == BST_CHECKED)
							modifiers |= HOTKEYF_SHIFT;

						if (IsDlgButtonChecked (hwnd, IDC_FULLSCREEN_CTRL) == BST_CHECKED)
							modifiers |= HOTKEYF_CONTROL;

						if (IsDlgButtonChecked (hwnd, IDC_FULLSCREEN_ALT) == BST_CHECKED)
							modifiers |= HOTKEYF_ALT;

						app.ConfigSet (L"HotkeyFullscreen", (DWORD)MAKEWORD (SendDlgItemMessage (hwnd, IDC_FULLSCREEN_CB, CB_GETITEMDATA, (WPARAM)fullscreen_idx, 0), modifiers));
					}

					if (window_idx > 0)
					{
						DWORD modifiers = 0;

						if (IsDlgButtonChecked (hwnd, IDC_WINDOW_SHIFT) == BST_CHECKED)
							modifiers |= HOTKEYF_SHIFT;

						if (IsDlgButtonChecked (hwnd, IDC_WINDOW_CTRL) == BST_CHECKED)
							modifiers |= HOTKEYF_CONTROL;

						if (IsDlgButtonChecked (hwnd, IDC_WINDOW_ALT) == BST_CHECKED)
							modifiers |= HOTKEYF_ALT;

						app.ConfigSet (L"HotkeyWindow", (DWORD)MAKEWORD (SendDlgItemMessage (hwnd, IDC_WINDOW_CB, CB_GETITEMDATA, (WPARAM)window_idx, 0), modifiers));
					}

					if (region_idx > 0)
					{
						DWORD modifiers = 0;

						if (IsDlgButtonChecked (hwnd, IDC_REGION_SHIFT) == BST_CHECKED)
							modifiers |= HOTKEYF_SHIFT;

						if (IsDlgButtonChecked (hwnd, IDC_REGION_CTRL) == BST_CHECKED)
							modifiers |= HOTKEYF_CONTROL;

						if (IsDlgButtonChecked (hwnd, IDC_REGION_ALT) == BST_CHECKED)
							modifiers |= HOTKEYF_ALT;

						app.ConfigSet (L"HotkeyRegion", (DWORD)MAKEWORD (SendDlgItemMessage (hwnd, IDC_REGION_CB, CB_GETITEMDATA, (WPARAM)region_idx, 0), modifiers));
					}

					// without break;
				}

				case IDCANCEL: // process Esc key
				case IDC_CLOSE:
				{
					_app_hotkeyinit (app.GetHWND ());
					EndDialog (hwnd, 0);

					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

void _app_initdropdownmenu (HMENU hmenu, bool is_button)
{
	app.LocaleMenu (hmenu, IDS_COPYTOCLIPBOARD_CHK, IDM_COPYTOCLIPBOARD_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_PLAYSOUNDS_CHK, IDM_PLAYSOUNDS_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_INCLUDEMOUSECURSOR_CHK, IDM_INCLUDEMOUSECURSOR_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_HIDEME_CHK, IDM_HIDEME_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_INCLUDEWINDOWSHADOW_CHK, IDM_INCLUDEWINDOWSHADOW_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_CLEARBACKGROUND_CHK, IDM_CLEARBACKGROUND_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_DISABLEAEROONWND_CHK, IDM_DISABLEAEROONWND_CHK, false, L" (vista+)");
	app.LocaleMenu (hmenu, IDS_FILENAME, FILENAME_MENU, true, nullptr);
	app.LocaleMenu (hmenu, IDS_IMAGEFORMAT, FORMAT_MENU, true, nullptr);
	app.LocaleMenu (hmenu, IDS_HOTKEYS, IDM_HOTKEYS, false, is_button ? L"...\tF3" : L"...");

	app.LocaleMenu (hmenu, 0, IDM_FILENAME_INDEX, false, _r_fmt (FILE_FORMAT_NAME_FORMAT L".%s", app.ConfigGet (L"FilenamePrefix", FILE_FORMAT_NAME_PREFIX).GetString (), 1, formats.at (_app_getimageformat ()).ext));
	app.LocaleMenu (hmenu, 0, IDM_FILENAME_DATE, false, _r_fmt (FILE_FORMAT_DATE_FORMAT_1 L" " FILE_FORMAT_DATE_FORMAT_2 L".%s", formats.at (_app_getimageformat ()).ext));

	// initialize formats
	{
		const HMENU submenu = GetSubMenu (hmenu, FORMAT_MENU);
		DeleteMenu (submenu, 0, MF_BYPOSITION);

		for (size_t i = 0; i < formats.size (); i++)
			AppendMenu (submenu, MF_BYPOSITION, IDX_FORMATS + i, formats.at (i).ext);
	}

	CheckMenuItem (hmenu, IDM_COPYTOCLIPBOARD_CHK, MF_BYCOMMAND | (app.ConfigGet (L"CopyToClipboard", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_INCLUDEMOUSECURSOR_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsIncludeMouseCursor", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_PLAYSOUNDS_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsPlaySound", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_HIDEME_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsHideMe", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_INCLUDEWINDOWSHADOW_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsIncludeWindowShadow", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_CLEARBACKGROUND_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsClearBackground", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_DISABLEAEROONWND_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsDisableAeroOnWnd", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));

	CheckMenuRadioItem (hmenu, IDM_FILENAME_INDEX, IDM_FILENAME_DATE, IDM_FILENAME_INDEX + app.ConfigGet (L"FilenameType", NameIndex).AsUint (), MF_BYCOMMAND);
	CheckMenuRadioItem (hmenu, IDX_FORMATS, IDX_FORMATS + UINT (formats.size ()), IDX_FORMATS + UINT (_app_getimageformat ()), MF_BYCOMMAND);

	if (!app.IsVistaOrLater ())
		EnableMenuItem (hmenu, IDM_DISABLEAEROONWND_CHK, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
}

INT_PTR CALLBACK DlgProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			// create dummy window
			{
				WNDCLASSEX wcex = {0};

				wcex.cbSize = sizeof (wcex);
				wcex.style = CS_VREDRAW | CS_HREDRAW;
				wcex.hInstance = app.GetHINSTANCE ();

				// dummy class
				wcex.lpszClassName = DUMMY_CLASS_DLG;
				wcex.lpfnWndProc = &DefWindowProc;
				wcex.hbrBackground = GetSysColorBrush (COLOR_WINDOW);
				wcex.hCursor = LoadCursor (nullptr, IDC_ARROW);

				RegisterClassEx (&wcex);

				// region class
				wcex.lpszClassName = REGION_CLASS_DLG;
				wcex.lpfnWndProc = &RegionProc;
				wcex.hbrBackground = (HBRUSH)GetStockObject (DKGRAY_BRUSH);
				wcex.hCursor = LoadCursor (app.GetHINSTANCE (), MAKEINTRESOURCE (IDI_MAIN));

				RegisterClassEx (&wcex);
			}

			SendDlgItemMessage (hwnd, IDC_FOLDER, EM_SETLIMITTEXT, MAX_PATH, 0);

			// set default folder
			{
				if (SHGetFolderPath (hwnd, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT | SHGFP_TYPE_DEFAULT, config.default_folder) != S_OK &&
					SHGetFolderPath (hwnd, CSIDL_MYPICTURES, nullptr, SHGFP_TYPE_CURRENT | SHGFP_TYPE_DEFAULT, config.default_folder) != S_OK)
				{
					StringCchCopy (config.default_folder, _countof (config.default_folder), DEFAULT_DIRECTORY);
				}

				app.ConfigSet (L"Folder", _r_path_unexpand (_app_getdirectory ()));
			}

			// add splitbutton style (vista+)
			if (app.IsVistaOrLater ())
				_r_wnd_addstyle (hwnd, IDC_SETTINGS, BS_SPLITBUTTON, BS_SPLITBUTTON, GWL_STYLE);

			config.hregion_mutex = CreateEvent (nullptr, TRUE, TRUE, nullptr);

			// initialize gdi+
			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
			Gdiplus::GdiplusStartup (&gdiplusToken, &gdiplusStartupInput, nullptr);

			// initialize formats
			{
				static LPCWSTR szexif[] = {
					L"image/bmp",
					L"image/jpeg",
					L"image/png",
					L"image/gif",
					L"image/tiff"
				};

				static LPCWSTR szext[] = {
					L"bmp",
					L"jpg",
					L"png",
					L"gif",
					L"tiff"
				};

				for (size_t i = 0; i < _countof (szexif); i++)
				{
					IMAGE_FORMAT tagImage = {0};

					if (GetEncoderClsid (szexif[i], &tagImage.clsid))
					{
						StringCchCopy (tagImage.ext, _countof (tagImage.ext), szext[i]);

						formats.push_back (tagImage);
					}
				}
			}

			_app_hotkeyinit (hwnd);

			break;
		}

		case RM_INITIALIZE:
		{
			SetDlgItemText (hwnd, IDC_FOLDER, _app_getdirectory ());

			CheckDlgButton (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, app.ConfigGet (L"IsIncludeMouseCursor", false).AsBool () ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_INCLUDEWINDOWSHADOW_CHK, app.ConfigGet (L"IsIncludeWindowShadow", true).AsBool () ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_CLEARBACKGROUND_CHK, app.ConfigGet (L"IsClearBackground", true).AsBool () ? BST_CHECKED : BST_UNCHECKED);

			CheckRadioButton (hwnd, IDC_MODE_FULLSCREEN, IDC_MODE_REGION, IDC_MODE_FULLSCREEN + app.ConfigGet (L"Mode", 0).AsUint ());

			app.TrayCreate (hwnd, UID, nullptr, WM_TRAYICON, _r_loadicon (app.GetHINSTANCE (), MAKEINTRESOURCE (IDI_MAIN), GetSystemMetrics (SM_CXSMICON)), false);
			app.TraySetInfo (hwnd, UID, nullptr, nullptr, APP_NAME);

			// configure menu
			CheckMenuItem (GetMenu (hwnd), IDM_ALWAYSONTOP_CHK, MF_BYCOMMAND | (app.ConfigGet (L"AlwaysOnTop", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem (GetMenu (hwnd), IDM_CLASSICUI_CHK, MF_BYCOMMAND | (app.ConfigGet (L"ClassicUI", _APP_CLASSICUI).AsBool () ? MF_CHECKED : MF_UNCHECKED));

			CheckMenuItem (GetMenu (hwnd), IDM_STARTMINIMIZED_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsStartMinimized", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem (GetMenu (hwnd), IDM_LOADONSTARTUP_CHK, MF_BYCOMMAND | (app.AutorunIsEnabled () ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem (GetMenu (hwnd), IDM_CHECKUPDATES_CHK, MF_BYCOMMAND | (app.ConfigGet (L"CheckUpdates", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));

			break;
		}

		case RM_LOCALIZE:
		{
			// configure button
			SetDlgItemText (hwnd, IDC_TITLE_FOLDER, app.LocaleString (IDS_FOLDER, L":"));
			SetDlgItemText (hwnd, IDC_TITLE_SETTINGS, app.LocaleString (IDS_QUICKSETTINGS, L":"));
			SetDlgItemText (hwnd, IDC_TITLE_MODE, app.LocaleString (IDS_MODE, L":"));

			SetDlgItemText (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, app.LocaleString (IDS_INCLUDEMOUSECURSOR_CHK, nullptr));
			SetDlgItemText (hwnd, IDC_INCLUDEWINDOWSHADOW_CHK, app.LocaleString (IDS_INCLUDEWINDOWSHADOW_CHK, nullptr));
			SetDlgItemText (hwnd, IDC_CLEARBACKGROUND_CHK, app.LocaleString (IDS_CLEARBACKGROUND_CHK, nullptr));

			SetDlgItemText (hwnd, IDC_MODE_FULLSCREEN, app.LocaleString (IDS_MODE_FULLSCREEN, nullptr));
			SetDlgItemText (hwnd, IDC_MODE_WINDOW, app.LocaleString (IDS_MODE_WINDOW, nullptr));
			SetDlgItemText (hwnd, IDC_MODE_REGION, app.LocaleString (IDS_MODE_REGION, nullptr));

			SetDlgItemText (hwnd, IDC_SETTINGS, app.LocaleString (IDS_SETTINGS, nullptr));
			SetDlgItemText (hwnd, IDC_SCREENSHOT, app.LocaleString (IDS_SCREENSHOT, nullptr));
			SetDlgItemText (hwnd, IDC_EXIT, app.LocaleString (IDS_EXIT, nullptr));

			_r_wnd_addstyle (hwnd, IDC_BROWSE_BTN, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);
			_r_wnd_addstyle (hwnd, IDC_SETTINGS, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);
			_r_wnd_addstyle (hwnd, IDC_SCREENSHOT, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);
			_r_wnd_addstyle (hwnd, IDC_EXIT, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);

			// localize menu
			const HMENU menu = GetMenu (hwnd);

			// fucked compiler
			rstring mode_fullscreen = app.LocaleString (IDS_MODE_FULLSCREEN, nullptr).GetString ();
			rstring mode_window = app.LocaleString (IDS_MODE_WINDOW, nullptr).GetString ();
			rstring mode_region = app.LocaleString (IDS_MODE_REGION, nullptr).GetString ();

			if (app.ConfigGet (L"HotkeyFullscreenEnabled", true).AsBool ())
				mode_fullscreen.ToLower ().AppendFormat (L"\t%s", _app_key2string (app.ConfigGet (L"HotkeyFullscreen", HOTKEY_FULLSCREEN).AsUint ()).GetString ());

			if (app.ConfigGet (L"HotkeyWindowEnabled", true).AsBool ())
				mode_window.ToLower ().AppendFormat (L"\t%s", _app_key2string (app.ConfigGet (L"HotkeyWindow", HOTKEY_WINDOW).AsUint ()).GetString ());

			if (app.ConfigGet (L"HotkeyRegionEnabled", true).AsBool ())
				mode_region.ToLower ().AppendFormat (L"\t%s", _app_key2string (app.ConfigGet (L"HotkeyRegion", HOTKEY_REGION).AsUint ()).GetString ());

			app.LocaleMenu (menu, IDS_FILE, 0, true, nullptr);
			app.LocaleMenu (menu, IDS_EXPLORE, IDM_EXPLORE, false, L"...\tCtrl+E");
			app.LocaleMenu (menu, IDS_SCREENSHOT, IDM_TAKE_FULLSCREEN, false, _r_fmt (L": %s", mode_fullscreen.GetString ()));
			app.LocaleMenu (menu, IDS_SCREENSHOT, IDM_TAKE_WINDOW, false, _r_fmt (L": %s", mode_window.GetString ()));
			app.LocaleMenu (menu, IDS_SCREENSHOT, IDM_TAKE_REGION, false, _r_fmt (L": %s", mode_region.GetString ()));
			app.LocaleMenu (menu, IDS_EXIT, IDM_EXIT, false, L"\tAlt+F4");
			app.LocaleMenu (menu, IDS_VIEW, 1, true, nullptr);
			app.LocaleMenu (menu, IDS_ALWAYSONTOP_CHK, IDM_ALWAYSONTOP_CHK, false, nullptr);
			app.LocaleMenu (menu, IDS_CLASSICUI_CHK, IDM_CLASSICUI_CHK, false, nullptr);

			app.LocaleMenu (menu, IDS_STARTMINIMIZED_CHK, IDM_STARTMINIMIZED_CHK, false, nullptr);
			app.LocaleMenu (menu, IDS_LOADONSTARTUP_CHK, IDM_LOADONSTARTUP_CHK, false, nullptr);
			app.LocaleMenu (menu, IDS_CHECKUPDATES_CHK, IDM_CHECKUPDATES_CHK, false, nullptr);

			app.LocaleMenu (GetSubMenu (menu, 1), IDS_LANGUAGE, LANG_MENU, true, L" (Language)");
			app.LocaleMenu (menu, IDS_HELP, 2, true, nullptr);
			app.LocaleMenu (menu, IDS_WEBSITE, IDM_WEBSITE, false, nullptr);
			app.LocaleMenu (menu, IDS_CHECKUPDATES, IDM_CHECKUPDATES, false, nullptr);
			app.LocaleMenu (menu, IDS_ABOUT, IDM_ABOUT, false, L"\tF1");

			app.LocaleEnum ((HWND)GetSubMenu (menu, 1), LANG_MENU, true, IDX_LANGUAGE); // enum localizations

			break;
		}

		case RM_UNINITIALIZE:
		{
			app.TrayDestroy (hwnd, UID, nullptr);
			break;
		}

		case WM_DESTROY:
		{
			CloseHandle (config.hregion_mutex);

			UnregisterClass (DUMMY_CLASS_DLG, app.GetHINSTANCE ());
			UnregisterClass (REGION_CLASS_DLG, app.GetHINSTANCE ());

			Gdiplus::GdiplusShutdown (gdiplusToken);

			PostQuitMessage (0);

			break;
		}

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORBTN:
		{
			SetBkMode ((HDC)wparam, TRANSPARENT); // background-hack

			return (INT_PTR)GetSysColorBrush (COLOR_BTNFACE);
		}

		case WM_HOTKEY:
		{
			if (wparam == HOTKEY_ID_FULLSCREEN)
				_app_takeshot (nullptr, ScreenshotFullscreen);

			else if (wparam == HOTKEY_ID_WINDOW)
				//_app_takeshot (GetForegroundWindow (), ScreenshotWindow);
				_app_takeshot (nullptr, ScreenshotWindow);

			else if (wparam == HOTKEY_ID_REGION)
				_app_takeshot (nullptr, ScreenshotRegion);

			break;
		}

		case WM_TRAYICON:
		{
			switch (LOWORD (lparam))
			{
				case WM_LBUTTONDBLCLK:
				{
					_r_wnd_toggle (hwnd, false);
					break;
				}

				case WM_LBUTTONUP:
				{
					SetForegroundWindow (hwnd);
					break;
				}

				case WM_MBUTTONUP:
				{
					SendMessage (hwnd, WM_COMMAND, IDM_EXPLORE, 0);
					break;
				}

				case WM_RBUTTONUP:
				{
					SetForegroundWindow (hwnd); // don't touch

					const HMENU hmenu = LoadMenu (nullptr, MAKEINTRESOURCE (IDM_TRAY));
					const HMENU hsubmenu = GetSubMenu (hmenu, 0);
					HMENU hmenu_settings = nullptr;

					// fucked compiler
					rstring mode_fullscreen = app.LocaleString (IDS_MODE_FULLSCREEN, nullptr).GetString ();
					rstring mode_window = app.LocaleString (IDS_MODE_WINDOW, nullptr).GetString ();
					rstring mode_region = app.LocaleString (IDS_MODE_REGION, nullptr).GetString ();

					if (app.ConfigGet (L"HotkeyFullscreenEnabled", true).AsBool ())
						mode_fullscreen.ToLower ().AppendFormat (L"\t%s", _app_key2string (app.ConfigGet (L"HotkeyFullscreen", HOTKEY_FULLSCREEN).AsUint ()).GetString ());

					if (app.ConfigGet (L"HotkeyWindowEnabled", true).AsBool ())
						mode_window.ToLower ().AppendFormat (L"\t%s", _app_key2string (app.ConfigGet (L"HotkeyWindow", HOTKEY_WINDOW).AsUint ()).GetString ());

					if (app.ConfigGet (L"HotkeyRegionEnabled", true).AsBool ())
						mode_region.ToLower ().AppendFormat (L"\t%s", _app_key2string (app.ConfigGet (L"HotkeyRegion", HOTKEY_REGION).AsUint ()).GetString ());

					// localize
					app.LocaleMenu (hsubmenu, IDS_TRAY_SHOW, IDM_TRAY_SHOW, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_SCREENSHOT, IDM_TRAY_TAKE_FULLSCREEN, false, _r_fmt (L": %s", mode_fullscreen.GetString ()));
					app.LocaleMenu (hsubmenu, IDS_SCREENSHOT, IDM_TRAY_TAKE_WINDOW, false, _r_fmt (L": %s", mode_window.GetString ()));
					app.LocaleMenu (hsubmenu, IDS_SCREENSHOT, IDM_TRAY_TAKE_REGION, false, _r_fmt (L": %s", mode_region.GetString ()));
					app.LocaleMenu (hsubmenu, IDS_SETTINGS, SETTINGS_MENU, true, nullptr);
					app.LocaleMenu (hsubmenu, IDS_WEBSITE, IDM_TRAY_WEBSITE, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_ABOUT, IDM_TRAY_ABOUT, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_EXIT, IDM_TRAY_EXIT, false, nullptr);

					// initialize settings submenu
					{
						hmenu_settings = LoadMenu (nullptr, MAKEINTRESOURCE (IDM_SETTINGS));
						const HMENU hsubmenu_settings = GetSubMenu (hmenu_settings, 0);

						_app_initdropdownmenu (hsubmenu_settings, false);

						MENUITEMINFO menuinfo = {0};

						menuinfo.cbSize = sizeof (menuinfo);
						menuinfo.fMask = MIIM_SUBMENU;
						menuinfo.hSubMenu = hsubmenu_settings;

						SetMenuItemInfo (hsubmenu, SETTINGS_MENU, true, &menuinfo);
					}

					POINT pt = {0};
					GetCursorPos (&pt);

					TrackPopupMenuEx (hsubmenu, TPM_RIGHTBUTTON | TPM_LEFTBUTTON, pt.x, pt.y, hwnd, nullptr);

					DestroyMenu (hmenu);
					DestroyMenu (hmenu_settings);

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
			if (HIWORD (wparam) == 0 && LOWORD (wparam) >= IDX_LANGUAGE && LOWORD (wparam) <= IDX_LANGUAGE + app.LocaleGetCount ())
			{
				app.LocaleApplyFromMenu (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), LOWORD (wparam), IDX_LANGUAGE);
				return FALSE;
			}
			else if ((LOWORD (wparam) >= IDX_FORMATS && LOWORD (wparam) <= IDX_FORMATS + formats.size ()))
			{
				const size_t idx = (LOWORD (wparam) - IDX_FORMATS);

				app.ConfigSet (L"ImageFormat", (LONGLONG)idx);

				return FALSE;
			}

			switch (LOWORD (wparam))
			{
				case IDM_EXPLORE:
				{
					ShellExecute (hwnd, nullptr, _app_getdirectory (), nullptr, nullptr, SW_SHOWDEFAULT);
					break;
				}

				case IDM_ALWAYSONTOP_CHK:
				{
					const bool new_val = !app.ConfigGet (L"AlwaysOnTop", false).AsBool ();

					CheckMenuItem (GetMenu (hwnd), LOWORD (wparam), MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"AlwaysOnTop", new_val);

					_r_wnd_top (hwnd, new_val);

					break;
				}

				case IDM_HIDEME_CHK:
				{
					const bool new_val = !app.ConfigGet (L"IsHideMe", true).AsBool ();
					app.ConfigSet (L"IsHideMe", new_val);

					break;
				}

				case IDM_CLASSICUI_CHK:
				{
					const bool new_val = !app.ConfigGet (L"ClassicUI", _APP_CLASSICUI).AsBool ();
					CheckMenuItem (GetMenu (hwnd), LOWORD (wparam), MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"ClassicUI", new_val);

					break;
				}

				case IDM_STARTMINIMIZED_CHK:
				{
					const bool new_val = !app.ConfigGet (L"IsStartMinimized", false).AsBool ();
					CheckMenuItem (GetMenu (hwnd), LOWORD (wparam), MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"IsStartMinimized", new_val);

					break;
				}

				case IDM_LOADONSTARTUP_CHK:
				{
					const bool new_val = !app.AutorunIsEnabled ();
					CheckMenuItem (GetMenu (hwnd), LOWORD (wparam), MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.AutorunEnable (new_val);

					break;
				}

				case IDM_CHECKUPDATES_CHK:
				{
					const bool new_val = !app.ConfigGet (L"CheckUpdates", true).AsBool ();
					CheckMenuItem (GetMenu (hwnd), LOWORD (wparam), MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"CheckUpdates", new_val);

					break;
				}

				case IDM_COPYTOCLIPBOARD_CHK:
				{
					const bool new_val = !app.ConfigGet (L"CopyToClipboard", false).AsBool ();
					app.ConfigSet (L"CopyToClipboard", new_val);

					break;
				}

				case IDM_DISABLEAEROONWND_CHK:
				{
					const bool new_val = !app.ConfigGet (L"IsDisableAeroOnWnd", false).AsBool ();
					app.ConfigSet (L"IsDisableAeroOnWnd", new_val);

					break;
				}

				case IDC_BROWSE_BTN:
				{
					if (SUCCEEDED (CoInitialize (nullptr)))
					{
						BROWSEINFO browseInfo = {0};

						browseInfo.hwndOwner = hwnd;
						browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_VALIDATE;

						LPITEMIDLIST pidl = SHBrowseForFolder (&browseInfo);

						if (pidl)
						{
							WCHAR buffer[MAX_PATH] = {0};

							if (SHGetPathFromIDList (pidl, buffer))
							{
								app.ConfigSet (L"Folder", _r_path_unexpand (buffer));
								SetDlgItemText (hwnd, IDC_FOLDER, buffer);
							}

							CoTaskMemFree (pidl);
						}

						CoUninitialize ();
					}

					break;
				}

				case IDM_PLAYSOUNDS_CHK:
				{
					const bool new_val = !app.ConfigGet (L"IsPlaySound", true).AsBool ();
					app.ConfigSet (L"IsPlaySound", new_val);

					break;
				}

				case IDM_INCLUDEMOUSECURSOR_CHK:
				case IDC_INCLUDEMOUSECURSOR_CHK:
				{
					const bool new_val = !app.ConfigGet (L"IsIncludeMouseCursor", false).AsBool ();

					app.ConfigSet (L"IsIncludeMouseCursor", new_val);
					CheckDlgButton (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, new_val ? BST_CHECKED : BST_UNCHECKED);

					break;
				}

				case IDM_CLEARBACKGROUND_CHK:
				case IDC_CLEARBACKGROUND_CHK:
				{
					const bool new_val = !app.ConfigGet (L"IsClearBackground", true).AsBool ();

					app.ConfigSet (L"IsClearBackground", new_val);
					CheckDlgButton (hwnd, IDC_CLEARBACKGROUND_CHK, new_val ? BST_CHECKED : BST_UNCHECKED);

					break;
				}

				case IDM_INCLUDEWINDOWSHADOW_CHK:
				case IDC_INCLUDEWINDOWSHADOW_CHK:
				{
					const bool new_val = !app.ConfigGet (L"IsIncludeWindowShadow", true).AsBool ();

					app.ConfigSet (L"IsIncludeWindowShadow", new_val);
					CheckDlgButton (hwnd, IDC_INCLUDEWINDOWSHADOW_CHK, new_val ? BST_CHECKED : BST_UNCHECKED);

					break;
				}

				case IDC_MODE_FULLSCREEN:
				case IDC_MODE_WINDOW:
				case IDC_MODE_REGION:
				{
					EnumScreenshot val;

					if (LOWORD (wparam) == IDC_MODE_WINDOW)
						val = ScreenshotWindow;

					else if (LOWORD (wparam) == IDC_MODE_REGION)

						val = ScreenshotRegion;
					else
						val = ScreenshotFullscreen;

					app.ConfigSet (L"Mode", (LONGLONG)val);
					break;
				}

				case IDM_FILENAME_INDEX:
				case IDM_FILENAME_DATE:
				{
					EnumImageName val;

					if (LOWORD (wparam) == IDM_FILENAME_DATE)
						val = NameDate;

					else
						val = NameIndex;

					app.ConfigSet (L"FilenameType", (LONGLONG)val);

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
					const UINT ctrl_id = LOWORD (wparam);

					EnumScreenshot mode;
					HWND hwindow = nullptr;

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
						mode = (EnumScreenshot)app.ConfigGet (L"Mode", 0).AsUint ();
					}

					_app_takeshot (hwindow, mode);

					break;
				}

				case IDC_SETTINGS:
				{
					RECT rc = {0};
					GetWindowRect (GetDlgItem (hwnd, IDC_SETTINGS), &rc);

					const HMENU hmenu = LoadMenu (nullptr, MAKEINTRESOURCE (IDM_SETTINGS));
					const HMENU submenu = GetSubMenu (hmenu, 0);

					_app_initdropdownmenu (submenu, true);

					TrackPopupMenuEx (submenu, TPM_RIGHTBUTTON | TPM_LEFTBUTTON, rc.left, rc.bottom, hwnd, nullptr);

					DestroyMenu (hmenu);

					break;
				}

				case IDM_HOTKEYS:
				{
					DialogBox (nullptr, MAKEINTRESOURCE (IDD_HOTKEYS), hwnd, &HotkeysProc);
					break;
				}

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
					_r_wnd_toggle (hwnd, false);
					break;
				}

				case IDM_WEBSITE:
				case IDM_TRAY_WEBSITE:
				{
					ShellExecute (hwnd, nullptr, _APP_WEBSITE_URL, nullptr, nullptr, SW_SHOWDEFAULT);
					break;
				}

				case IDM_CHECKUPDATES:
				{
					app.UpdateCheck (true);
					break;
				}

				case IDM_ABOUT:
				case IDM_TRAY_ABOUT:
				{
					app.CreateAboutWindow (hwnd);
					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

INT APIENTRY wWinMain (HINSTANCE, HINSTANCE, LPWSTR, INT)
{
	MSG msg = {0};

	if (app.CreateMainWindow (IDD_MAIN, IDI_MAIN, &DlgProc))
	{
		const HACCEL haccel = LoadAccelerators (app.GetHINSTANCE (), MAKEINTRESOURCE (IDA_MAIN));

		if (haccel)
		{
			while (GetMessage (&msg, nullptr, 0, 0) > 0)
			{
				TranslateAccelerator (app.GetHWND (), haccel, &msg);

				if (!IsDialogMessage (app.GetHWND (), &msg))
				{
					TranslateMessage (&msg);
					DispatchMessage (&msg);
				}
			}

			DestroyAcceleratorTable (haccel);
		}
	}

	return (INT)msg.wParam;
}

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

rapp app (APP_NAME, APP_NAME_SHORT, APP_VERSION, APP_COPYRIGHT);

EnumScreenshot _app_getmode ()
{
	return (EnumScreenshot)app.ConfigGet (L"Mode", 0).AsUint ();
}

rstring _app_getfolder ()
{
	return app.ConfigGet (L"Folder", config.default_folder);
}

rstring _app_getimageformat (EnumImageFormat imageType, bool is_exif)
{
	rstring result;

	if (imageType == ImageJpeg)
		result = is_exif ? L"image/jpeg" : L"jpg";

	else if (imageType == ImagePng)
		result = is_exif ? L"image/png" : L"png";

	else if (imageType == ImageGif)
		result = is_exif ? L"image/gif" : L"gif";

	else if (imageType == ImageTiff)
		result = is_exif ? L"image/tiff" : L"tiff";

	else
		result = is_exif ? L"image/bmp" : L"bmp";

	return result;
}

rstring _app_uniquefilename (LPCWSTR path, EnumImageFormat imageType)
{
	const ULONG idx = 1;

	LPCWSTR fname = L"sshot";
	rstring fext = _app_getimageformat (imageType, false);

	WCHAR buffer[MAX_PATH] = {0};

	for (ULONG i = idx; i < 256; i++)
	{
		StringCchPrintf (buffer, _countof (buffer), L"%s\\%s_%d.%s", path, fname, i, fext);

		if (!_r_fs_exists (buffer))
			return buffer;
	}

	PathYetAnotherMakeUniqueName (buffer, _r_fmt (L"%s\\%s.%s", path, fname, fext), nullptr, _r_fmt (L"%s.%s", fname, fext));

	return buffer;
}

bool GetEncoderClsid (LPCWSTR format, CLSID *pClsid)
{
	unsigned int num = 0, size = 0;
	const size_t len = wcslen (format);
	Gdiplus::GetImageEncodersSize (&num, &size);

	if (size)
	{
		Gdiplus::ImageCodecInfo *pImageCodecInfo = (Gdiplus::ImageCodecInfo *)(malloc (size));

		if (pImageCodecInfo)
		{
			GetImageEncoders (num, size, pImageCodecInfo);

			for (unsigned int j = 0; j < num; ++j)
			{
				if (_wcsnicmp (pImageCodecInfo[j].MimeType, format, len) == 0)
				{
					*pClsid = pImageCodecInfo[j].Clsid;
					free (pImageCodecInfo);

					return true;
				}
			}

			free (pImageCodecInfo);
		}
	}

	return false;
}

bool _app_savehbitmap (HBITMAP hbitmap, LPCWSTR filepath, EnumImageFormat imageType)
{
	bool result = false;

	Gdiplus::Bitmap *pScreenShot = new Gdiplus::Bitmap (hbitmap, nullptr);

	if (pScreenShot)
	{
		CLSID imageCLSID;
		ULONG uQuality = app.ConfigGet (L"JPEGQuality", 90).AsUlong ();

		Gdiplus::EncoderParameters encoderParams = {0};

		encoderParams.Count = 1;
		encoderParams.Parameter[0].NumberOfValues = 1;
		encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
		encoderParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
		encoderParams.Parameter[0].Value = &uQuality;

		if (GetEncoderClsid (_app_getimageformat (imageType, true), &imageCLSID))
			result = (pScreenShot->Save (filepath, &imageCLSID, &encoderParams) == Gdiplus::Ok);

		delete pScreenShot;
	}

	return result;
}

HBITMAP _app_createbitmap (HDC hdc, LONG width, LONG height)
{
	BITMAPINFO bmiCapture = {0};
	bmiCapture.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	bmiCapture.bmiHeader.biWidth = width;
	bmiCapture.bmiHeader.biHeight = height;
	bmiCapture.bmiHeader.biPlanes = 1;
	bmiCapture.bmiHeader.biBitCount = GetDeviceCaps (hdc, BITSPIXEL);

	LPBYTE lpCapture = nullptr;

	return CreateDIBSection (hdc, &bmiCapture, DIB_PAL_COLORS, (LPVOID*)&lpCapture, nullptr, 0);
}

void _app_dofinishjob (HBITMAP hbitmap, INT width, INT height, EnumImageFormat imageType)
{
	if (app.ConfigGet (L"IsPlaySound", true).AsBool ())
		PlaySound (MAKEINTRESOURCE (IDW_MAIN), app.GetHINSTANCE (), SND_SENTRY | SND_RESOURCE | SND_ASYNC);

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
	StringCchCopy (full_path, _countof (full_path), _app_uniquefilename (_app_getfolder (), imageType));

	_app_savehbitmap (hbitmap, full_path, imageType);
}

void _app_screenshot (INT x, INT y, INT width, INT height, bool is_cursor, EnumImageFormat imageType)
{
	const HWND hwnd = GetDesktopWindow ();
	const HDC hdc = GetDC (hwnd);
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
			GetCursorInfo (&cursorinfo);

			const HICON hicon = CopyIcon (cursorinfo.hCursor);

			ICONINFO iconinfo = {0};
			GetIconInfo (hicon, &iconinfo);

			DrawIcon (hcapture, cursorinfo.ptScreenPos.x - iconinfo.xHotspot - x, cursorinfo.ptScreenPos.y - iconinfo.yHotspot - y, hicon);

			DestroyIcon (hicon);
		}

		_app_dofinishjob (hbitmap, width, height, imageType);

		DeleteObject (hbitmap);
	}

	DeleteDC (hcapture);
	ReleaseDC (hwnd, hdc);
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

bool _app_islastwindow (HWND hwnd)
{
	if (!IsWindowVisible (hwnd))
		return false;

	HWND hwalk = nullptr;
	HWND hwndTry = GetAncestor (hwnd, GA_ROOT);

	while (hwndTry != hwalk)
	{
		hwalk = hwndTry;
		hwndTry = GetLastActivePopup (hwalk);

		if (IsWindowVisible (hwndTry))
			break;
	}

	if (hwalk != hwnd)
		return false;

	// the following removes some task tray programs and "program manager"
	TITLEBARINFO ti = {0};
	ti.cbSize = sizeof (ti);

	if (GetTitleBarInfo (hwnd, &ti))
	{
		if (ti.rgstate[0] & STATE_SYSTEM_INVISIBLE)
			return false;
	}

	return true;
}

bool _app_isnormalwindow (HWND hwnd)
{
	return hwnd && IsWindowVisible (hwnd) && !IsIconic (hwnd) && hwnd != GetShellWindow () && hwnd != GetDesktopWindow ();
}

BOOL CALLBACK _app_enumcallback (HWND hwnd, LPARAM)
{
	if (!_app_isnormalwindow (hwnd))
		return TRUE;

	if (_app_islastwindow (hwnd) && GetAncestor (hwnd, GA_ROOT) != app.GetHWND ())
	{
		config.hactive = hwnd;

		return FALSE;
	}

	return TRUE;
}

void _app_takeshot (HWND hwnd, EnumScreenshot mode)
{
	bool result = false;

	const bool is_includecursor = app.ConfigGet (L"IsIncludeMouseCursor", false).AsBool ();
	const bool is_hideme = app.ConfigGet (L"IsHideMe", true).AsBool ();
	const bool is_windowdisplayed = IsWindowVisible (app.GetHWND ()) && !IsIconic (app.GetHWND ());
	const EnumImageFormat imageType = (EnumImageFormat)app.ConfigGet (L"ImageFormat", ImageJpeg).AsUint ();

	RECT prev_rect = {0};

	if (is_hideme)
	{
		if (is_windowdisplayed)
		{
			GetWindowRect (app.GetHWND (), &prev_rect);
			SetWindowPos (app.GetHWND (), nullptr, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOREDRAW | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
		}

		app.TrayToggle (UID, false);
	}

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
			_app_screenshot (lprc->left, lprc->top, _R_RECT_WIDTH (lprc), _R_RECT_HEIGHT (lprc), is_includecursor, imageType);
		}
	}
	else if (mode == ScreenshotWindow)
	{
		// find active window
		if (!hwnd)
		{
			config.hactive = nullptr;
			EnumWindows (&_app_enumcallback, 0);
			hwnd = config.hactive;
		}

		if (_app_isnormalwindow (hwnd))
		{
			const bool is_disableaeroonwnd = app.IsVistaOrLater () && app.ConfigGet (L"IsDisableAeroOnWnd", false).AsBool ();
			const bool is_clearbackground = app.IsVistaOrLater () && app.ConfigGet (L"IsClearBackground", true).AsBool ();

			if (is_disableaeroonwnd)
				_app_switchaeroonwnd (hwnd, true);

			RECT window_rect = {0};
			_app_getwindowrect (hwnd, &window_rect);

			if (is_clearbackground)
			{
				if (config.hdummy)
					SetWindowPos (config.hdummy, hwnd, window_rect.left, window_rect.top, _R_RECT_WIDTH (&window_rect), _R_RECT_HEIGHT (&window_rect), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
			}

			SetForegroundWindow (hwnd);
			SwitchToThisWindow (hwnd, TRUE);

			_r_sleep (WND_SLEEP);

			//SetForegroundWindow (hwnd);
			_app_screenshot (window_rect.left, window_rect.top, _R_RECT_WIDTH (&window_rect), _R_RECT_HEIGHT (&window_rect), is_includecursor, imageType);

			if (is_disableaeroonwnd)
				_app_switchaeroonwnd (hwnd, false);

			if (is_clearbackground)
			{
				if (config.hdummy)
					SetWindowPos (config.hdummy, app.GetHWND (), 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
			}
		}
	}
	else if (mode == ScreenshotRegion)
	{
		config.hregion = CreateWindowEx (WS_EX_TOPMOST, REGION_CLASS_DLG, nullptr, WS_POPUP, 0, 0, 0, 0, app.GetHWND (), nullptr, app.GetHINSTANCE (), nullptr);
		SetWindowPos (config.hregion, HWND_TOPMOST, 0, 0, GetSystemMetrics (SM_CXVIRTUALSCREEN), GetSystemMetrics (SM_CYVIRTUALSCREEN), SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
	}

	if (is_hideme)
	{
		if (is_windowdisplayed)
			SetWindowPos (app.GetHWND (), nullptr, prev_rect.left, prev_rect.top, _R_RECT_WIDTH (&prev_rect), _R_RECT_HEIGHT (&prev_rect), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);

		app.TrayToggle (UID, true);
		app.TraySetInfo (UID, nullptr, APP_NAME);
	}
}

void _app_hotkeyuninit (HWND hwnd)
{
	UnregisterHotKey (hwnd, HOTKEY_ID_FULLSCREEN);
	UnregisterHotKey (hwnd, HOTKEY_ID_WINDOW);
	UnregisterHotKey (hwnd, HOTKEY_ID_REGION);
}

void _app_hotkeyinit (HWND hwnd)
{
	bool is_nofullscreen = false;
	bool is_nowindow = false;
	bool is_noregion = false;

	if (app.ConfigGet (L"HotkeyFullscreenEnabled", true).AsBool ())
	{
		const UINT hk_fullscreen = app.ConfigGet (L"HotkeyFullscreen", HOTKEY_FULLSCREEN).AsUint ();

		if (hk_fullscreen)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_FULLSCREEN, (HIBYTE (hk_fullscreen) & 2) | ((HIBYTE (hk_fullscreen) & 4) >> 2) | ((HIBYTE (hk_fullscreen) & 1) << 2), LOBYTE (hk_fullscreen)))
				is_nofullscreen = true;
		}
	}

	if (app.ConfigGet (L"HotkeyWindowEnabled", true).AsBool ())
	{
		const UINT hk_window = app.ConfigGet (L"HotkeyWindow", HOTKEY_WINDOW).AsUint ();

		if (hk_window)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_WINDOW, (HIBYTE (hk_window) & 2) | ((HIBYTE (hk_window) & 4) >> 2) | ((HIBYTE (hk_window) & 1) << 2), LOBYTE (hk_window)))
				is_nowindow = true;
		}
	}

	if (app.ConfigGet (L"HotkeyRegionEnabled", true).AsBool ())
	{
		const UINT hk_region = app.ConfigGet (L"HotkeyRegion", HOTKEY_REGION).AsUint ();

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
			buffer.AppendFormat (L"- %s\r\n", app.LocaleString (IDS_MODE_FULLSCREEN, nullptr));

		if (is_nowindow)
			buffer.AppendFormat (L"- %s\r\n", app.LocaleString (IDS_MODE_WINDOW, nullptr));

		if (is_noregion)
			buffer.AppendFormat (L"- %s\r\n", app.LocaleString (IDS_MODE_REGION, nullptr));

		app.ConfirmMessage (hwnd, L"Hotkeys already in use!", buffer, L"NoHotkeysWarning");
	}
}

BOOL initializer_callback (HWND hwnd, DWORD msg, LPVOID, LPVOID)
{
	switch (msg)
	{
		case _RM_INITIALIZE:
		{
			_app_hotkeyinit (hwnd);

			SetDlgItemText (hwnd, IDC_FOLDER, _app_getfolder ());

			CheckDlgButton (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, app.ConfigGet (L"IsIncludeMouseCursor", false).AsBool () ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_PLAYSOUNDS_CHK, app.ConfigGet (L"IsPlaySound", true).AsBool () ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_CLEARBACKGROUND_CHK, app.ConfigGet (L"IsClearBackground", true).AsBool () ? BST_CHECKED : BST_UNCHECKED);

			CheckRadioButton (hwnd, IDC_MODE_FULLSCREEN, IDC_MODE_REGION, IDC_MODE_FULLSCREEN + app.ConfigGet (L"Mode", 0).AsUint ());

			app.TrayCreate (hwnd, UID, WM_TRAYICON, _r_loadicon (app.GetHINSTANCE (), MAKEINTRESOURCE (IDI_MAIN), GetSystemMetrics (SM_CXSMICON)), false);
			app.TraySetInfo (UID, nullptr, APP_NAME);

			// configure menu
			CheckMenuItem (GetMenu (hwnd), IDM_ALWAYSONTOP_CHK, MF_BYCOMMAND | (app.ConfigGet (L"AlwaysOnTop", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem (GetMenu (hwnd), IDM_HIDEME_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsHideMe", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem (GetMenu (hwnd), IDM_CLASSICUI_CHK, MF_BYCOMMAND | (app.ConfigGet (L"ClassicUI", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));

			if (!app.IsVistaOrLater ())
				_r_ctrl_enable (hwnd, IDC_CLEARBACKGROUND_CHK, false);

			break;
		}

		case _RM_LOCALIZE:
		{
			// localize menu
			const HMENU menu = GetMenu (hwnd);

			app.LocaleMenu (menu, IDS_FILE, 0, true, nullptr);
			app.LocaleMenu (menu, IDS_EXPLORE, IDM_EXPLORE, false, L"\tCtrl+E");
			app.LocaleMenu (menu, IDS_EXIT, IDM_EXIT, false, nullptr);
			app.LocaleMenu (menu, IDS_VIEW, 1, true, nullptr);
			app.LocaleMenu (menu, IDS_ALWAYSONTOP_CHK, IDM_ALWAYSONTOP_CHK, false, nullptr);
			app.LocaleMenu (menu, IDS_HIDEME_CHK, IDM_HIDEME_CHK, false, nullptr);
			app.LocaleMenu (menu, IDS_CLASSICUI_CHK, IDM_CLASSICUI_CHK, false, nullptr);
			app.LocaleMenu (GetSubMenu (menu, 1), IDS_LANGUAGE, LANG_MENU, true, L" (Language)");
			app.LocaleMenu (menu, IDS_HELP, 2, true, nullptr);
			app.LocaleMenu (menu, IDS_WEBSITE, IDM_WEBSITE, false, nullptr);
			app.LocaleMenu (menu, IDS_CHECKUPDATES, IDM_CHECKUPDATES, false, nullptr);
			app.LocaleMenu (menu, IDS_ABOUT, IDM_ABOUT, false, L"\tF1");

			// configure button
			SetDlgItemText (hwnd, IDC_TITLE_FOLDER, app.LocaleString (IDS_FOLDER, L":"));
			SetDlgItemText (hwnd, IDC_TITLE_SETTINGS, app.LocaleString (IDS_QUICKSETTINGS, L":"));
			SetDlgItemText (hwnd, IDC_TITLE_MODE, app.LocaleString (IDS_MODE, L":"));

			SetDlgItemText (hwnd, IDC_PLAYSOUNDS_CHK, app.LocaleString (IDS_PLAYSOUNDS_CHK, nullptr));
			SetDlgItemText (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, app.LocaleString (IDS_INCLUDEMOUSECURSOR_CHK, nullptr));
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

			app.LocaleEnum ((HWND)GetSubMenu (menu, 1), LANG_MENU, true, IDX_LANGUAGE); // enum localizations

			break;
		}

		case _RM_UNINITIALIZE:
		{
			app.TrayDestroy (UID);
			break;
		}
	}

	return FALSE;
}

LRESULT CALLBACK DummyProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//switch (msg)
	//{
	//	case WM_CTLCOLORDLG:
	//	{
	//		return (LRESULT)GetSysColorBrush (COLOR_WINDOW);
	//	}
	//}

	return DefWindowProc (hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK RegionProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static HBITMAP hbitmap = nullptr;
	static HDC hcapture = nullptr;
	static HPEN hpen = nullptr;

	static POINT ptStart = {0};
	static POINT ptEnd = {0};
	static bool fDraw = false;

	switch (msg)
	{
		case WM_CREATE:
		{
			const INT width = GetSystemMetrics (SM_CXVIRTUALSCREEN);
			const INT height = GetSystemMetrics (SM_CYVIRTUALSCREEN);

			const HWND hdesktop = GetDesktopWindow ();
			const HDC hdc = GetDC (hdesktop);

			hcapture = CreateCompatibleDC (hdc);
			hbitmap = _app_createbitmap (hdc, width, height);

			if (hbitmap)
			{
				SelectObject (hcapture, hbitmap);
				BitBlt (hcapture, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
			}

			ReleaseDC (hdesktop, hdc);

			hpen = CreatePen (PS_DASHDOT, GetSystemMetrics (SM_CXBORDER), PEN_COLOR);

			break;
		}

		case WM_DESTROY:
		{
			fDraw = false;

			if (hpen)
			{
				DeleteObject (hpen);
				hpen = nullptr;
			}

			if (hbitmap)
			{
				DeleteObject (hbitmap);
				hbitmap = nullptr;
			}

			if (hcapture)
			{
				DeleteDC (hcapture);
				hcapture = nullptr;
			}

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

				if (x || y || width || height)
				{
					// save region to a file
					const EnumImageFormat imageType = (EnumImageFormat)app.ConfigGet (L"ImageFormat", ImageJpeg).AsUint ();

					const HDC hdc_finish = GetDC (GetDesktopWindow ());
					const HDC hcapture_finish = CreateCompatibleDC (hdc_finish);

					const HBITMAP hbitmap_finish = _app_createbitmap (hdc_finish, width, height);

					if (hbitmap_finish)
					{
						SelectObject (hcapture_finish, hbitmap_finish);
						BitBlt (hcapture_finish, 0, 0, width, height, hcapture, x, y, SRCCOPY);

						_app_dofinishjob (hbitmap_finish, width, height, imageType);
					}

					DeleteObject (hbitmap_finish);
					DeleteDC (hcapture_finish);
					ReleaseDC (GetDesktopWindow (), hdc_finish);
				}

				DestroyWindow (hwnd);
			}

			return TRUE;
		}

		case WM_MBUTTONDOWN:
		{
			DestroyWindow (hwnd);
			return TRUE;
		}

		case WM_MOUSEMOVE:
		{
			InvalidateRect (hwnd, nullptr, true);
			return TRUE;
		}

		case WM_ERASEBKGND:
		{
			const HDC hdc = (HDC)wparam;

			RECT rc = {0};
			GetClientRect (hwnd, &rc);

			POINT pt = {0};
			GetCursorPos (&pt);

			BitBlt (hdc, 0, 0, _R_RECT_WIDTH (&rc), _R_RECT_HEIGHT (&rc), hcapture, rc.left, rc.top, SRCCOPY);

			const HPEN old_pen = (HPEN)SelectObject (hdc, hpen);
			const HGDIOBJ old_brush = SelectObject (hdc, GetStockObject (NULL_BRUSH));

			SetBkColor (hdc, PEN_COLOR_BK);

			if (fDraw)
			{
				// draw region rectangle
				Rectangle (hdc, ptStart.x, ptStart.y, pt.x, pt.y);
			}
			else
			{
				// draw cursor crosshair
				MoveToEx (hdc, pt.x, rc.top, nullptr);
				LineTo (hdc, pt.x, _R_RECT_HEIGHT (&rc));

				MoveToEx (hdc, rc.left, pt.y, nullptr);
				LineTo (hdc, _R_RECT_WIDTH (&rc), pt.y);
			}

			SelectObject (hdc, old_pen);
			SelectObject (hdc, old_brush);

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
			_app_hotkeyuninit (app.GetHWND ());

			// configure window
			_r_wnd_center (hwnd, GetParent (hwnd));

			app.SetIcon (hwnd, 0, false);

			// localize window
			SetWindowText (hwnd, app.LocaleString (IDS_SETTINGS, nullptr));

			SetDlgItemText (hwnd, IDC_TITLE_HOTKEYS, app.LocaleString (IDS_HOTKEYS, L":"));

			SetDlgItemText (hwnd, IDC_ENABLE_FULLSCREEN_CHK, app.LocaleString (IDS_MODE_FULLSCREEN, nullptr));
			SetDlgItemText (hwnd, IDC_ENABLE_WINDOW_CHK, app.LocaleString (IDS_MODE_WINDOW, nullptr));
			SetDlgItemText (hwnd, IDC_ENABLE_REGION_CHK, app.LocaleString (IDS_MODE_REGION, nullptr));

			SetDlgItemText (hwnd, IDC_SAVE, app.LocaleString (IDS_SAVE, nullptr));
			SetDlgItemText (hwnd, IDC_CANCEL, app.LocaleString (IDS_CANCEL, nullptr));

			CheckDlgButton (hwnd, IDC_ENABLE_FULLSCREEN_CHK, app.ConfigGet (L"HotkeyFullscreenEnabled", true).AsBool () ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_ENABLE_WINDOW_CHK, app.ConfigGet (L"HotkeyWindowEnabled", true).AsBool () ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton (hwnd, IDC_ENABLE_REGION_CHK, app.ConfigGet (L"HotkeyRegionEnabled", true).AsBool () ? BST_CHECKED : BST_UNCHECKED);

			SendDlgItemMessage (hwnd, IDC_HOTKEY_FULLSCREEN, HKM_SETHOTKEY, app.ConfigGet (L"HotkeyFullscreen", HOTKEY_FULLSCREEN).AsUint (), 0);
			SendDlgItemMessage (hwnd, IDC_HOTKEY_WINDOW, HKM_SETHOTKEY, app.ConfigGet (L"HotkeyWindow", HOTKEY_WINDOW).AsUint (), 0);
			SendDlgItemMessage (hwnd, IDC_HOTKEY_REGION, HKM_SETHOTKEY, app.ConfigGet (L"HotkeyRegion", HOTKEY_REGION).AsUint (), 0);

			_r_wnd_addstyle (hwnd, IDC_SAVE, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);
			_r_wnd_addstyle (hwnd, IDC_CANCEL, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);

			PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_ENABLE_FULLSCREEN_CHK, 0), 0);
			PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_ENABLE_WINDOW_CHK, 0), 0);
			PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDC_ENABLE_REGION_CHK, 0), 0);

			break;
		}

		case WM_DESTROY:
		{
			_app_hotkeyinit (app.GetHWND ());
			break;
		}

		case WM_COMMAND:
		{
			switch (LOWORD (wparam))
			{
				case IDOK: // process Enter key
				case IDC_SAVE:
				{
					app.ConfigSet (L"HotkeyFullscreenEnabled", (IsDlgButtonChecked (hwnd, IDC_ENABLE_FULLSCREEN_CHK) == BST_CHECKED) ? true : false);
					app.ConfigSet (L"HotkeyWindowEnabled", (IsDlgButtonChecked (hwnd, IDC_ENABLE_WINDOW_CHK) == BST_CHECKED) ? true : false);
					app.ConfigSet (L"HotkeyRegionEnabled", (IsDlgButtonChecked (hwnd, IDC_ENABLE_REGION_CHK) == BST_CHECKED) ? true : false);

					app.ConfigSet (L"HotkeyFullscreen", (DWORD)SendDlgItemMessage (hwnd, IDC_HOTKEY_FULLSCREEN, HKM_GETHOTKEY, 0, 0));
					app.ConfigSet (L"HotkeyWindow", (DWORD)SendDlgItemMessage (hwnd, IDC_HOTKEY_WINDOW, HKM_GETHOTKEY, 0, 0));
					app.ConfigSet (L"HotkeyRegion", (DWORD)SendDlgItemMessage (hwnd, IDC_HOTKEY_REGION, HKM_GETHOTKEY, 0, 0));

					// without break;
				}

				case IDCANCEL: // process Esc key
				case IDC_CANCEL:
				{
					EndDialog (hwnd, 0);
					break;
				}

				case IDC_ENABLE_FULLSCREEN_CHK:
				case IDC_ENABLE_WINDOW_CHK:
				case IDC_ENABLE_REGION_CHK:
				{
					const bool is_enabled = (IsDlgButtonChecked (hwnd, LOWORD (wparam)) == BST_CHECKED);

					_r_ctrl_enable (hwnd, LOWORD (wparam) + 1, is_enabled);

					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

void _app_initdropdownmenu (HMENU hmenu)
{
	app.LocaleMenu (hmenu, IDS_LOADONSTARTUP_CHK, IDM_LOADONSTARTUP_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_CHECKUPDATES_CHK, IDM_CHECKUPDATES_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_COPYTOCLIPBOARD_CHK, IDM_COPYTOCLIPBOARD_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_PLAYSOUNDS_CHK, IDM_PLAYSOUNDS_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_INCLUDEMOUSECURSOR_CHK, IDM_INCLUDEMOUSECURSOR_CHK, false, nullptr);
	app.LocaleMenu (hmenu, IDS_CLEARBACKGROUND_CHK, IDM_CLEARBACKGROUND_CHK, false, L" (vista+)");
	app.LocaleMenu (hmenu, IDS_DISABLEAEROONWND_CHK, IDM_DISABLEAEROONWND_CHK, false, L" (vista+) [BETA]");
	app.LocaleMenu (hmenu, IDS_IMAGEFORMAT, FORMAT_MENU, true, nullptr);
	app.LocaleMenu (hmenu, IDS_HOTKEYS, IDM_HOTKEYS, false, L"...\tF3");

	CheckMenuItem (hmenu, IDM_LOADONSTARTUP_CHK, MF_BYCOMMAND | (app.AutorunIsEnabled () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_CHECKUPDATES_CHK, MF_BYCOMMAND | (app.ConfigGet (L"CheckUpdates", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_COPYTOCLIPBOARD_CHK, MF_BYCOMMAND | (app.ConfigGet (L"CopyToClipboard", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_INCLUDEMOUSECURSOR_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsIncludeMouseCursor", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_PLAYSOUNDS_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsPlaySound", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_CLEARBACKGROUND_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsClearBackground", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem (hmenu, IDM_DISABLEAEROONWND_CHK, MF_BYCOMMAND | (app.ConfigGet (L"IsDisableAeroOnWnd", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuRadioItem (hmenu, IDM_FORMAT_BMP, IDM_FORMAT_TIFF, IDM_FORMAT_BMP + app.ConfigGet (L"ImageFormat", ImageJpeg).AsUint (), MF_BYCOMMAND);

	if (!app.IsVistaOrLater ())
	{
		EnableMenuItem (hmenu, IDM_CLEARBACKGROUND_CHK, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		EnableMenuItem (hmenu, IDM_DISABLEAEROONWND_CHK, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}
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
				wcex.lpfnWndProc = &DummyProc;
				wcex.hbrBackground = GetSysColorBrush (COLOR_WINDOW);
				wcex.hCursor = LoadCursor (nullptr, IDC_ARROW);

				RegisterClassEx (&wcex);

				// region class
				wcex.lpszClassName = REGION_CLASS_DLG;
				wcex.lpfnWndProc = &RegionProc;
				wcex.hbrBackground = (HBRUSH)GetStockObject (DKGRAY_BRUSH);
				wcex.hCursor = LoadCursor (app.GetHINSTANCE (), MAKEINTRESOURCE (IDI_MAIN));

				RegisterClassEx (&wcex);

				config.hdummy = CreateWindow (DUMMY_CLASS_DLG, nullptr, WS_POPUP, 0, 0, 0, 0, hwnd, nullptr, wcex.hInstance, nullptr);
			}

			SendDlgItemMessage (hwnd, IDC_FOLDER, EM_SETLIMITTEXT, MAX_PATH, 0);

			// set default folder
			if (SHGetFolderPath (hwnd, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT | SHGFP_TYPE_DEFAULT, config.default_folder) != S_OK)
				StringCchCopy (config.default_folder, _countof (config.default_folder), app.GetDirectory ());

			// add splitbutton style (vista+)
			if (app.IsVistaOrLater ())
				_r_wnd_addstyle (hwnd, IDC_SETTINGS, BS_SPLITBUTTON, BS_SPLITBUTTON, GWL_STYLE);

			// initialize gdi+
			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
			Gdiplus::GdiplusStartup (&gdiplusToken, &gdiplusStartupInput, nullptr);

			break;
		}

		case WM_DESTROY:
		{
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
			{
				_app_takeshot (nullptr, ScreenshotFullscreen);
			}
			else if (wparam == HOTKEY_ID_WINDOW)
			{
				HWND hwindow = GetForegroundWindow ();

				if (!_app_isnormalwindow (hwindow))
				{
					POINT pt = {0};
					GetCursorPos (&pt);

					hwindow = GetAncestor (WindowFromPoint (pt), GA_ROOT);
				}

				_app_takeshot (hwindow, ScreenshotWindow);
			}
			else if (wparam == HOTKEY_ID_REGION)
			{
				_app_takeshot (nullptr, ScreenshotRegion);
			}

			break;
		}

		case WM_TRAYICON:
		{
			switch (LOWORD (lparam))
			{
				case WM_LBUTTONUP:
				{
					SetForegroundWindow (hwnd);
					break;
				}

				case WM_LBUTTONDBLCLK:
				{
					_r_wnd_toggle (hwnd, false);
					break;
				}

				case WM_MBUTTONDOWN:
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

					// localize
					app.LocaleMenu (hsubmenu, IDS_TRAY_SHOW, IDM_TRAY_SHOW, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_MODE_FULLSCREEN, IDM_TRAY_TAKE_FULLSCREEN, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_MODE_WINDOW, IDM_TRAY_TAKE_WINDOW, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_MODE_REGION, IDM_TRAY_TAKE_REGION, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_SETTINGS, SETTINGS_MENU, true, nullptr);
					app.LocaleMenu (hsubmenu, IDS_WEBSITE, IDM_TRAY_WEBSITE, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_ABOUT, IDM_TRAY_ABOUT, false, nullptr);
					app.LocaleMenu (hsubmenu, IDS_EXIT, IDM_TRAY_EXIT, false, nullptr);

					// initialize settings submenu
					{
						hmenu_settings = LoadMenu (nullptr, MAKEINTRESOURCE (IDM_SETTINGS));
						const HMENU hsubmenu_settings = GetSubMenu (hmenu_settings, 0);

						_app_initdropdownmenu (hsubmenu_settings);

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

			switch (LOWORD (wparam))
			{
				case IDM_EXPLORE:
				{
					ShellExecute (hwnd, nullptr, _app_getfolder (), nullptr, nullptr, SW_SHOWDEFAULT);
					break;
				}

				case IDM_ALWAYSONTOP_CHK:
				{
					const bool new_val = !app.ConfigGet (L"AlwaysOnTop", false).AsBool ();

					CheckMenuItem (GetMenu (hwnd), IDM_ALWAYSONTOP_CHK, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"AlwaysOnTop", new_val);

					_r_wnd_top (hwnd, new_val);

					break;
				}

				case IDM_HIDEME_CHK:
				{
					const bool new_val = !app.ConfigGet (L"IsHideMe", true).AsBool ();
					CheckMenuItem (GetMenu (hwnd), IDM_HIDEME_CHK, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"IsHideMe", new_val);

					break;
				}

				case IDM_CLASSICUI_CHK:
				{
					const bool new_val = !app.ConfigGet (L"ClassicUI", false).AsBool ();
					CheckMenuItem (GetMenu (hwnd), IDM_CLASSICUI_CHK, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"ClassicUI", new_val);

					break;
				}

				case IDM_LOADONSTARTUP_CHK:
				{
					const bool new_val = !app.AutorunIsEnabled ();
					app.AutorunEnable (new_val);

					break;
				}

				case IDM_CHECKUPDATES_CHK:
				{
					const bool new_val = !app.ConfigGet (L"CheckUpdates", true).AsBool ();
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
					CoInitialize (nullptr);

					BROWSEINFO browseInfo = {0};

					browseInfo.hwndOwner = hwnd;
					browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_VALIDATE;

					LPITEMIDLIST pidl = SHBrowseForFolder (&browseInfo);

					if (pidl)
					{
						WCHAR buffer[MAX_PATH] = {0};

						if (SHGetPathFromIDList (pidl, buffer))
						{
							app.ConfigSet (L"Folder", buffer);
							SetDlgItemText (hwnd, IDC_FOLDER, buffer);
						}

						CoTaskMemFree (pidl);
					}

					CoUninitialize ();

					break;
				}

				case IDM_PLAYSOUNDS_CHK:
				case IDC_PLAYSOUNDS_CHK:
				{
					const bool new_val = !app.ConfigGet (L"IsPlaySound", true).AsBool ();

					app.ConfigSet (L"IsPlaySound", new_val);
					CheckDlgButton (hwnd, IDC_PLAYSOUNDS_CHK, new_val ? BST_CHECKED : BST_UNCHECKED);

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

				case IDM_FORMAT_BMP:
				case IDM_FORMAT_JPG:
				case IDM_FORMAT_PNG:
				case IDM_FORMAT_GIF:
				case IDM_FORMAT_TIFF:
				{
					EnumImageFormat val;

					if (LOWORD (wparam) == IDM_FORMAT_JPG)
						val = ImageJpeg;

					else if (LOWORD (wparam) == IDM_FORMAT_PNG)
						val = ImagePng;

					else if (LOWORD (wparam) == IDM_FORMAT_GIF)
						val = ImageGif;

					else if (LOWORD (wparam) == IDM_FORMAT_TIFF)
						val = ImageTiff;

					else
						val = ImageBitmap;

					app.ConfigSet (L"ImageFormat", (LONGLONG)val);

					CheckMenuRadioItem (GetMenu (hwnd), IDM_FORMAT_BMP, IDM_FORMAT_TIFF, IDM_FORMAT_BMP + val, MF_BYCOMMAND);

					break;
				}

				case IDM_TRAY_TAKE_FULLSCREEN:
				case IDM_TRAY_TAKE_WINDOW:
				case IDM_TRAY_TAKE_REGION:
				{
					EnumScreenshot val;

					if (LOWORD (wparam) == IDM_TRAY_TAKE_WINDOW)
						val = ScreenshotWindow;

					else if (LOWORD (wparam) == IDM_TRAY_TAKE_REGION)
						val = ScreenshotRegion;

					else
						val = ScreenshotFullscreen;

					_app_takeshot (nullptr, val);

					break;
				}

				case IDC_SETTINGS:
				{
					RECT rc = {0};
					GetWindowRect (GetDlgItem (hwnd, IDC_SETTINGS), &rc);

					const HMENU hmenu = LoadMenu (nullptr, MAKEINTRESOURCE (IDM_SETTINGS));
					const HMENU submenu = GetSubMenu (hmenu, 0);

					_app_initdropdownmenu (submenu);

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

				case IDOK: // process Enter key
				case IDC_SCREENSHOT:
				{
					_app_takeshot (nullptr, _app_getmode ());
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
					app.CheckForUpdates (false);
					break;
				}

				case IDM_ABOUT:
				case IDM_TRAY_ABOUT:
				{
					app.CreateAboutWindow (hwnd, app.LocaleString (IDS_DONATE, nullptr));
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

	if (app.CreateMainWindow (IDD_MAIN, IDI_MAIN, &DlgProc, &initializer_callback))
	{
		const HACCEL haccel = LoadAccelerators (app.GetHINSTANCE (), MAKEINTRESOURCE (IDA_MAIN));

		while (GetMessage (&msg, nullptr, 0, 0) > 0)
		{
			if (haccel)
				TranslateAccelerator (app.GetHWND (), haccel, &msg);

			if (!IsDialogMessage (app.GetHWND (), &msg))
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}

		if (haccel)
			DestroyAcceleratorTable (haccel);
	}

	return (INT)msg.wParam;
}

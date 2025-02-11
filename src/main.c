// Free Shooter
// Copyright (c) 2009-2023 Henry++

#include "routine.h"

#include "global.h"

INT_PTR CALLBACK SettingsProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
);

VOID _app_key2string (
	_Out_writes_ (length) LPWSTR buffer,
	_In_ ULONG_PTR length,
	_In_ UINT key
)
{
	WCHAR key_name[64];
	UINT vk_code;
	UINT modifiers;
	UINT scan_code;

	vk_code = LOBYTE (key);
	modifiers = HIBYTE (key);

	*buffer = UNICODE_NULL;

	if (modifiers & HOTKEYF_CONTROL)
		_r_str_append (buffer, length, L"Ctrl+");

	if (modifiers & HOTKEYF_ALT)
		_r_str_append (buffer, length, L"Alt+");

	if (modifiers & HOTKEYF_SHIFT)
		_r_str_append (buffer, length, L"Shift+");

	if (vk_code == VK_SNAPSHOT)
	{
		_r_str_copy (key_name, RTL_NUMBER_OF (key_name), L"Prnt scrn");
	}
	else
	{
		scan_code = MapVirtualKeyW (vk_code, MAPVK_VK_TO_VSC);

		GetKeyNameTextW ((scan_code << 16), key_name, RTL_NUMBER_OF (key_name));
	}

	_r_str_append (buffer, length, key_name);
}

BOOLEAN _app_hotkeyinit (
	_In_ HWND hwnd,
	_In_opt_ HWND hwnd_hotkey
)
{
	WCHAR buffer[256] = {0};
	WCHAR key_string[128];
	UINT hk_fullscreen;
	UINT hk_monitor;
	UINT hk_window;
	UINT hk_region;
	BOOLEAN is_nofullscreen = FALSE;
	BOOLEAN is_nomonitor = FALSE;
	BOOLEAN is_nowindow = FALSE;
	BOOLEAN is_noregion = FALSE;

	UnregisterHotKey (hwnd, HOTKEY_ID_FULLSCREEN);
	UnregisterHotKey (hwnd, HOTKEY_ID_MONITOR);
	UnregisterHotKey (hwnd, HOTKEY_ID_WINDOW);
	UnregisterHotKey (hwnd, HOTKEY_ID_REGION);

	hk_fullscreen = _r_config_getlong (L"HotkeyFullscreen2", HOTKEY_FULLSCREEN, NULL);
	hk_monitor = _r_config_getlong (L"HotkeyMonitor", HOTKEY_MONITOR, NULL);
	hk_window = _r_config_getlong (L"HotkeyWindow", HOTKEY_WINDOW, NULL);
	hk_region = _r_config_getlong (L"HotkeyRegion", HOTKEY_REGION, NULL);

	if (_r_config_getboolean (L"HotkeyFullscreenEnabled", FALSE, NULL))
	{
		if (hk_fullscreen)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_FULLSCREEN, (HIBYTE (hk_fullscreen) & 2) | ((HIBYTE (hk_fullscreen) & 4) >> 2) | ((HIBYTE (hk_fullscreen) & 1) << 2), LOBYTE (hk_fullscreen)))
				is_nofullscreen = TRUE;
		}
	}

	if (_r_config_getboolean (L"HotkeyMonitorEnabled", TRUE, NULL))
	{
		if (hk_monitor)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_MONITOR, (HIBYTE (hk_monitor) & 2) | ((HIBYTE (hk_monitor) & 4) >> 2) | ((HIBYTE (hk_monitor) & 1) << 2), LOBYTE (hk_monitor)))
				is_nomonitor = TRUE;
		}
	}

	if (_r_config_getboolean (L"HotkeyWindowEnabled", TRUE, NULL))
	{
		if (hk_window)
		{
			if (!RegisterHotKey (hwnd, HOTKEY_ID_WINDOW, (HIBYTE (hk_window) & 2) | ((HIBYTE (hk_window) & 4) >> 2) | ((HIBYTE (hk_window) & 1) << 2), LOBYTE (hk_window)))
				is_nowindow = TRUE;
		}
	}

	if (_r_config_getboolean (L"HotkeyRegionEnabled", TRUE, NULL))
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
		if (is_nofullscreen)
		{
			_app_key2string (key_string, RTL_NUMBER_OF (key_string), hk_fullscreen);

			_r_str_appendformat (
				buffer,
				RTL_NUMBER_OF (buffer),
				L"%s [%s]\r\n",
				_r_locale_getstring (IDS_MODE_FULLSCREEN),
				key_string
			);
		}

		if (is_nomonitor)
		{
			_app_key2string (key_string, RTL_NUMBER_OF (key_string), hk_monitor);

			_r_str_appendformat (
				buffer,
				RTL_NUMBER_OF (buffer),
				L"%s [%s]\r\n",
				_r_locale_getstring (IDS_MODE_MONITOR),
				key_string
			);
		}

		if (is_nowindow)
		{
			_app_key2string (key_string, RTL_NUMBER_OF (key_string), hk_window);

			_r_str_appendformat (
				buffer,
				RTL_NUMBER_OF (buffer),
				L"%s [%s]\r\n",
				_r_locale_getstring (IDS_MODE_WINDOW),
				key_string
			);
		}

		if (is_noregion)
		{
			_app_key2string (key_string, RTL_NUMBER_OF (key_string), hk_region);

			_r_str_appendformat (
				buffer,
				RTL_NUMBER_OF (buffer),
				L"%s [%s]\r\n",
				_r_locale_getstring (IDS_MODE_REGION),
				key_string
			);
		}

		StrTrimW (buffer, L"\r\n");

		if (_r_show_message (hwnd_hotkey ? hwnd_hotkey : hwnd, MB_YESNO | MB_ICONWARNING, _r_locale_getstring (IDS_WARNING_HOTKEYS), buffer) == IDYES)
		{
			if (!hwnd_hotkey)
				_r_settings_createwindow (hwnd, &SettingsProc, IDD_SETTINGS_HOTKEYS);

			return FALSE;
		}
	}

	return TRUE;
}

VOID generate_keys_array (
	_Out_writes_ (count) PCHAR keys,
	_In_ ULONG_PTR count
)
{
	static const UINT predefined_keys[] = {
		VK_SNAPSHOT,
		VK_BACK,
		VK_TAB,
		VK_RETURN,
		VK_SPACE,
		VK_DELETE,
	};

	ULONG_PTR index = 0;
	CHAR i;

	for (i = 0; i < RTL_NUMBER_OF (predefined_keys); i++)
	{
		if (count <= index)
			goto CleanupExit;

		keys[index++] = MAKEWORD (predefined_keys[i], 0);
	}

	for (i = 'A'; i <= 'Z'; i++)
	{
		if (count <= index)
			goto CleanupExit;

		keys[index++] = MAKEWORD (i, 0);
	}

	for (i = '0'; i <= '9'; i++)
	{
		if (count <= index)
			goto CleanupExit;

		keys[index++] = MAKEWORD (i, 0);
	}

	for (i = VK_F1; i <= VK_F12; i++)
	{
		if (count <= index)
			goto CleanupExit;

		keys[index++] = MAKEWORD (i, 0);
	}

CleanupExit:

	keys[index] = ANSI_NULL;
}

INT_PTR CALLBACK SettingsProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
)
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
					CHAR keys[64];
					WCHAR key_string[64];
					LPCWSTR disabled_string;
					UINT fullscreen_code;
					UINT monitor_code;
					UINT window_code;
					UINT region_code;
					UINT key_code;
					BOOLEAN fullscreen_allowed;
					BOOLEAN monitor_allowed;
					BOOLEAN window_allowed;
					BOOLEAN region_allowed;

					fullscreen_code = _r_config_getlong (L"HotkeyFullscreen2", HOTKEY_FULLSCREEN, NULL);
					monitor_code = _r_config_getlong (L"HotkeyMonitor", HOTKEY_MONITOR, NULL);
					window_code = _r_config_getlong (L"HotkeyWindow", HOTKEY_WINDOW, NULL);
					region_code = _r_config_getlong (L"HotkeyRegion", HOTKEY_REGION, NULL);

					fullscreen_allowed = _r_config_getboolean (L"HotkeyFullscreenEnabled", FALSE, NULL);
					monitor_allowed = _r_config_getboolean (L"HotkeyMonitorEnabled", TRUE, NULL);
					window_allowed = _r_config_getboolean (L"HotkeyWindowEnabled", TRUE, NULL);
					region_allowed = _r_config_getboolean (L"HotkeyRegionEnabled", TRUE, NULL);

					_r_ctrl_checkbutton (hwnd, IDC_FULLSCREEN_SHIFT, ((HIBYTE (fullscreen_code) & HOTKEYF_SHIFT) != 0));
					_r_ctrl_checkbutton (hwnd, IDC_FULLSCREEN_CTRL, ((HIBYTE (fullscreen_code) & HOTKEYF_CONTROL) != 0));
					_r_ctrl_checkbutton (hwnd, IDC_FULLSCREEN_ALT, ((HIBYTE (fullscreen_code) & HOTKEYF_ALT) != 0));

					_r_ctrl_checkbutton (hwnd, IDC_MONITOR_SHIFT, ((HIBYTE (monitor_code) & HOTKEYF_SHIFT) != 0));
					_r_ctrl_checkbutton (hwnd, IDC_MONITOR_CTRL, ((HIBYTE (monitor_code) & HOTKEYF_CONTROL) != 0));
					_r_ctrl_checkbutton (hwnd, IDC_MONITOR_ALT, ((HIBYTE (monitor_code) & HOTKEYF_ALT) != 0));

					_r_ctrl_checkbutton (hwnd, IDC_WINDOW_SHIFT, ((HIBYTE (window_code) & HOTKEYF_SHIFT) != 0));
					_r_ctrl_checkbutton (hwnd, IDC_WINDOW_CTRL, ((HIBYTE (window_code) & HOTKEYF_CONTROL) != 0));
					_r_ctrl_checkbutton (hwnd, IDC_WINDOW_ALT, ((HIBYTE (window_code) & HOTKEYF_ALT) != 0));

					_r_ctrl_checkbutton (hwnd, IDC_REGION_SHIFT, ((HIBYTE (region_code) & HOTKEYF_SHIFT) != 0));
					_r_ctrl_checkbutton (hwnd, IDC_REGION_CTRL, ((HIBYTE (region_code) & HOTKEYF_CONTROL) != 0));
					_r_ctrl_checkbutton (hwnd, IDC_REGION_ALT, ((HIBYTE (region_code) & HOTKEYF_ALT) != 0));

					disabled_string = _r_locale_getstring (IDS_DISABLE);

					_r_combobox_insertitem (hwnd, IDC_FULLSCREEN_CB, 0, disabled_string, 0);
					_r_combobox_insertitem (hwnd, IDC_MONITOR_CB, 0, disabled_string, 0);
					_r_combobox_insertitem (hwnd, IDC_WINDOW_CB, 0, disabled_string, 0);
					_r_combobox_insertitem (hwnd, IDC_REGION_CB, 0, disabled_string, 0);

					generate_keys_array (keys, RTL_NUMBER_OF (keys));

					for (UINT i = 0, index = 0; i < RTL_NUMBER_OF (keys); i++)
					{
						key_code = keys[i];

						if (!key_code)
							break;

						index += 1;

						_app_key2string (key_string, RTL_NUMBER_OF (key_string), MAKEWORD (key_code, 0));

						_r_combobox_insertitem (hwnd, IDC_FULLSCREEN_CB, index, key_string, (LPARAM)key_code);
						_r_combobox_insertitem (hwnd, IDC_MONITOR_CB, index, key_string, (LPARAM)key_code);
						_r_combobox_insertitem (hwnd, IDC_WINDOW_CB, index, key_string, (LPARAM)key_code);
						_r_combobox_insertitem (hwnd, IDC_REGION_CB, index, key_string, (LPARAM)key_code);

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

					_r_wnd_sendmessage (hwnd, 0, WM_COMMAND, MAKEWPARAM (IDC_FULLSCREEN_CB, CBN_SELCHANGE), 0);
					_r_wnd_sendmessage (hwnd, 0, WM_COMMAND, MAKEWPARAM (IDC_MONITOR_CB, CBN_SELCHANGE), 0);
					_r_wnd_sendmessage (hwnd, 0, WM_COMMAND, MAKEWPARAM (IDC_WINDOW_CB, CBN_SELCHANGE), 0);
					_r_wnd_sendmessage (hwnd, 0, WM_COMMAND, MAKEWPARAM (IDC_REGION_CB, CBN_SELCHANGE), 0);

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

					_r_config_setboolean (L"HotkeyFullscreenEnabled", fullscreen_idx > 0, NULL);
					_r_config_setboolean (L"HotkeyMonitorEnabled", monitor_idx > 0, NULL);
					_r_config_setboolean (L"HotkeyWindowEnabled", window_idx > 0, NULL);
					_r_config_setboolean (L"HotkeyRegionEnabled", region_idx > 0, NULL);

					// save fullscreen hotkey
					if (fullscreen_idx > 0)
					{
						modifiers = 0;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_FULLSCREEN_SHIFT))
							modifiers |= HOTKEYF_SHIFT;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_FULLSCREEN_CTRL))
							modifiers |= HOTKEYF_CONTROL;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_FULLSCREEN_ALT))
							modifiers |= HOTKEYF_ALT;

						_r_config_setulong (L"HotkeyFullscreen2", (ULONG)MAKEWORD (_r_combobox_getitemlparam (hwnd, IDC_FULLSCREEN_CB, fullscreen_idx), modifiers), NULL);
					}

					// save monitor hotkey
					if (monitor_idx > 0)
					{
						modifiers = 0;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_MONITOR_SHIFT))
							modifiers |= HOTKEYF_SHIFT;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_MONITOR_CTRL))
							modifiers |= HOTKEYF_CONTROL;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_MONITOR_ALT))
							modifiers |= HOTKEYF_ALT;

						_r_config_setulong (L"HotkeyMonitor", (ULONG)MAKEWORD (_r_combobox_getitemlparam (hwnd, IDC_MONITOR_CB, monitor_idx), modifiers), NULL);
					}

					// save window hotkey
					if (window_idx > 0)
					{
						modifiers = 0;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_WINDOW_SHIFT))
							modifiers |= HOTKEYF_SHIFT;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_WINDOW_CTRL))
							modifiers |= HOTKEYF_CONTROL;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_WINDOW_ALT))
							modifiers |= HOTKEYF_ALT;

						_r_config_setulong (L"HotkeyWindow", (ULONG)MAKEWORD (_r_combobox_getitemlparam (hwnd, IDC_WINDOW_CB, window_idx), modifiers), NULL);
					}

					// save region hotkey
					if (region_idx > 0)
					{
						modifiers = 0;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_REGION_SHIFT))
							modifiers |= HOTKEYF_SHIFT;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_REGION_CTRL))
							modifiers |= HOTKEYF_CONTROL;

						if (_r_ctrl_isbuttonchecked (hwnd, IDC_REGION_ALT))
							modifiers |= HOTKEYF_ALT;

						_r_config_setulong (L"HotkeyRegion", (ULONG)MAKEWORD (_r_combobox_getitemlparam (hwnd, IDC_REGION_CB, region_idx), modifiers), NULL);
					}

					if (!_app_hotkeyinit (_r_app_gethwnd (), hwnd))
					{
						SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, INT_ERROR);

						return (INT_PTR)INT_ERROR;
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
				BOOLEAN is_disable;

				is_disable = _r_combobox_getcurrentitem (hwnd, ctrl_id) > 0;

				_r_ctrl_enable (hwnd, ctrl_id - 3, is_disable); // shift
				_r_ctrl_enable (hwnd, ctrl_id - 2, is_disable); // ctrl
				_r_ctrl_enable (hwnd, ctrl_id - 1, is_disable); // alt

				break;
			}
		}
	}

	return FALSE;
}

VOID _app_initdropdownmenu (
	_In_ HMENU hmenu,
	_In_ BOOLEAN is_button
)
{
	PIMAGE_FORMAT format;
	HMENU hsubmenu;
	PR_STRING string;
	UINT formats_count;
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

	string = _r_config_getstring (L"FilenamePrefix", FILE_FORMAT_NAME_PREFIX, NULL);

	_r_menu_setitemtextformat (
		hmenu,
		IDM_FILENAME_INDEX,
		FALSE,
		FILE_FORMAT_NAME_FORMAT L".%s",
		_r_obj_getstringordefault (string, FILE_FORMAT_NAME_PREFIX),
		START_IDX,
		format->ext
	);

	_r_menu_setitemtextformat (
		hmenu,
		IDM_FILENAME_DATE,
		FALSE,
		L"%s" FILE_FORMAT_DATE_FORMAT_1 L"-" FILE_FORMAT_DATE_FORMAT_2 L".%s",
		_r_obj_getstringordefault (string, FILE_FORMAT_NAME_PREFIX),
		format->ext
	);

	if (string)
		_r_obj_dereference (string);

	// initialize formats
	formats_count = (UINT)(UINT_PTR)_r_obj_getarraysize (config.formats);
	hsubmenu = GetSubMenu (hmenu, FORMAT_MENU);

	if (hsubmenu)
	{
		_r_menu_clearitems (hsubmenu);

		for (UINT i = 0; i < formats_count; i++)
		{
			format = _r_obj_getarrayitem (config.formats, i);

			if (format)
				_r_menu_additem (hsubmenu, IDX_FORMATS + i, format->ext);
		}
	}

	// initialize delays
	hsubmenu = GetSubMenu (hmenu, DELAY_MENU);

	if (hsubmenu)
	{
		for (UINT i = 0; i < RTL_NUMBER_OF (timer_array); i++)
		{
			string = _r_format_interval (timer_array[i], FALSE);

			if (string)
			{
				_r_menu_additem (hsubmenu, IDX_DELAY + i, string->buffer);

				_r_obj_dereference (string);
			}
		}
	}

	delay_idx = _app_getdelay_id ();

	if (delay_idx == INT_ERROR)
	{
		_r_menu_checkitem (hmenu, IDM_DELAY_DISABLE, IDM_DELAY_DISABLE, MF_BYCOMMAND, IDM_DELAY_DISABLE);
	}
	else
	{
		_r_menu_checkitem (hmenu, IDX_DELAY, IDX_DELAY + RTL_NUMBER_OF (timer_array), MF_BYCOMMAND, IDX_DELAY + delay_idx);
	}

	_r_menu_checkitem (hmenu, IDX_FORMATS, IDX_FORMATS + formats_count, MF_BYCOMMAND, IDX_FORMATS + _app_getimageformat_id ());
	_r_menu_checkitem (hmenu, IDM_FILENAME_INDEX, IDM_FILENAME_DATE, MF_BYCOMMAND, IDM_FILENAME_INDEX + _app_getimagename_id ());
	_r_menu_checkitem (hmenu, IDM_INCLUDEMOUSECURSOR_CHK, 0, MF_BYCOMMAND, _r_config_getboolean (L"IsIncludeMouseCursor", FALSE, NULL));
	_r_menu_checkitem (hmenu, IDM_INCLUDEWINDOWSHADOW_CHK, 0, MF_BYCOMMAND, _r_config_getboolean (L"IsIncludeWindowShadow", TRUE, NULL));
	_r_menu_checkitem (hmenu, IDM_CLEARBACKGROUND_CHK, 0, MF_BYCOMMAND, _r_config_getboolean (L"IsClearBackground", TRUE, NULL));
	_r_menu_checkitem (hmenu, IDM_DISABLEAEROONWND_CHK, 0, MF_BYCOMMAND, _r_config_getboolean (L"IsDisableAeroOnWnd", FALSE, NULL));
	_r_menu_checkitem (hmenu, IDM_COPYTOCLIPBOARD_CHK, 0, MF_BYCOMMAND, _r_config_getboolean (L"CopyToClipboard", FALSE, NULL));
	_r_menu_checkitem (hmenu, IDM_PLAYSOUNDS_CHK, 0, MF_BYCOMMAND, _r_config_getboolean (L"IsPlaySound", TRUE, NULL));
}

VOID _app_initialize ()
{
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
	   &GUID_ContainerFormatTiff,
	   &GUID_ContainerFormatWmp,
	};

	C_ASSERT (RTL_NUMBER_OF (szext) == RTL_NUMBER_OF (guids));

	WNDCLASSEX wcex = {0};
	IMAGE_FORMAT image_format;

	config.formats = _r_obj_createarray (sizeof (IMAGE_FORMAT), RTL_NUMBER_OF (szext), NULL);

	for (ULONG_PTR i = 0; i < RTL_NUMBER_OF (szext); i++)
	{
		RtlZeroMemory (&image_format, sizeof (IMAGE_FORMAT));

		_r_str_copy (image_format.ext, RTL_NUMBER_OF (image_format.ext), szext[i]);

		RtlCopyMemory (&image_format.guid, guids[i], sizeof (GUID));

		_r_obj_addarrayitem (config.formats, &image_format, NULL);
	}

	_r_freelist_initialize (&context_list, sizeof (MONITOR_CONTEXT), 8);

	NtCreateEvent (&config.hregion_mutex, EVENT_ALL_ACCESS, NULL, NotificationEvent, TRUE);
	NtCreateEvent (&config.hshot_evt, EVENT_ALL_ACCESS, NULL, NotificationEvent, TRUE);

	wcex.cbSize = sizeof (wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.hInstance = _r_sys_getimagebase ();
	wcex.lpszClassName = DUMMY_CLASS_DLG;
	wcex.lpfnWndProc = &DummyProc;
	wcex.hbrBackground = GetSysColorBrush (COLOR_WINDOW);
	wcex.hCursor = LoadCursorW (NULL, IDC_ARROW);

	RegisterClassExW (&wcex);
}

INT_PTR CALLBACK DlgProc (
	_In_ HWND hwnd,
	_In_  UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			HWND hctrl;

			_r_app_sethwnd (hwnd); // HACK!!!

			// set edit control configuration
			hctrl = GetDlgItem (hwnd, IDC_FOLDER);

			if (hctrl)
				SHAutoComplete (hctrl, SHACF_FILESYS_ONLY | SHACF_FILESYS_DIRS | SHACF_AUTOSUGGEST_FORCE_ON | SHACF_USETAB);

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
			LONG icon_small;

			dpi_value = _r_dc_gettaskbardpi ();

			icon_small = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);

			hicon = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCEW (IDI_MAIN), icon_small);

			// initialize tray icon
			_r_tray_create (hwnd, &GUID_TrayIcon, RM_TRAYICON, hicon, _r_app_getname (), FALSE);

			// set directory path
			string = _app_getdirectory ();

			_r_ctrl_setstring (hwnd, IDC_FOLDER, string->buffer);

			_r_ctrl_checkbutton (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, _r_config_getboolean (L"IsIncludeMouseCursor", FALSE, NULL));
			_r_ctrl_checkbutton (hwnd, IDC_INCLUDEWINDOWSHADOW_CHK, _r_config_getboolean (L"IsIncludeWindowShadow", TRUE, NULL));
			_r_ctrl_checkbutton (hwnd, IDC_CLEARBACKGROUND_CHK, _r_config_getboolean (L"IsClearBackground", TRUE, NULL));

			CheckRadioButton (hwnd, IDC_MODE_MONITOR, IDC_MODE_REGION, IDC_MODE_MONITOR + _app_getmode_id () - 1);

			// configure menu
			hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				_r_menu_checkitem (hmenu, IDM_ALWAYSONTOP_CHK, 0, MF_BYCOMMAND, _r_config_getboolean (L"AlwaysOnTop", FALSE, NULL));
				_r_menu_checkitem (hmenu, IDM_DARKMODE_CHK, 0, MF_BYCOMMAND, _r_theme_isenabled ());
				_r_menu_checkitem (hmenu, IDM_LOADONSTARTUP_CHK, 0, MF_BYCOMMAND, _r_autorun_isenabled ());
				_r_menu_checkitem (hmenu, IDM_STARTMINIMIZED_CHK, 0, MF_BYCOMMAND, _r_config_getboolean (L"IsStartMinimized", FALSE, NULL));
				_r_menu_checkitem (hmenu, IDM_HIDEME_CHK, 0, MF_BYCOMMAND, _r_config_getboolean (L"IsHideMe", TRUE, NULL));
				_r_menu_checkitem (hmenu, IDM_CHECKUPDATES_CHK, 0, MF_BYCOMMAND, _r_update_isenabled (FALSE));
			}

			_r_obj_dereference (string);

			break;
		}

		case RM_TASKBARCREATED:
		{
			HICON hicon;
			LONG dpi_value;
			LONG icon_small;

			dpi_value = _r_dc_gettaskbardpi ();

			icon_small = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);

			hicon = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCEW (IDI_MAIN), icon_small);

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

			if (!hmenu)
				break;

			_r_menu_setitemtext (hmenu, 0, TRUE, _r_locale_getstring (IDS_FILE));
			_r_menu_setitemtext (hmenu, 1, TRUE, _r_locale_getstring (IDS_SETTINGS));
			_r_menu_setitemtextformat (GetSubMenu (hmenu, 1), LANG_MENU, TRUE, L"%s (Language)", _r_locale_getstring (IDS_LANGUAGE));
			_r_menu_setitemtext (hmenu, 2, TRUE, _r_locale_getstring (IDS_SCREENSHOT));
			_r_menu_setitemtext (hmenu, 3, TRUE, _r_locale_getstring (IDS_HELP));
			_r_menu_setitemtextformat (hmenu, IDM_EXPLORE, FALSE, L"%s...\tCtrl+E", _r_locale_getstring (IDS_EXPLORE));
			_r_menu_setitemtext (hmenu, IDM_EXIT, FALSE, _r_locale_getstring (IDS_EXIT));
			_r_menu_setitemtext (hmenu, IDM_ALWAYSONTOP_CHK, FALSE, _r_locale_getstring (IDS_ALWAYSONTOP_CHK));
			_r_menu_setitemtext (hmenu, IDM_DARKMODE_CHK, FALSE, _r_locale_getstring (IDS_DARKMODE_CHK));
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

			// enum localizations
			_r_locale_enum (GetSubMenu (hmenu, 1), LANG_MENU, IDX_LANGUAGE);

			break;
		}

		case RM_INITIALIZE_POST:
		{
			_app_hotkeyinit (hwnd, NULL); // initialize hotkey
			break;
		}

		case WM_DPICHANGED:
		{
			HICON hicon;
			LONG dpi_value;
			LONG icon_small;

			dpi_value = _r_dc_gettaskbardpi ();

			icon_small = _r_dc_getsystemmetrics (SM_CXSMICON, dpi_value);

			hicon = _r_sys_loadsharedicon (_r_sys_getimagebase (), MAKEINTRESOURCEW (IDI_MAIN), icon_small);

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
			SAFE_DELETE_HANDLE (config.hshot_evt);

			UnregisterClassW (DUMMY_CLASS_DLG, _r_sys_getimagebase ());

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

			_r_wnd_sendmessage (hwnd, 0, WM_COMMAND, MAKEWPARAM (command_id, 0), 0);

			break;
		}

		case RM_TRAYICON:
		{
			switch (LOWORD (lparam))
			{
				case NIN_KEYSELECT:
				{
					if (GetForegroundWindow () != hwnd)
						_r_wnd_toggle (hwnd, FALSE);

					break;
				}

				case WM_LBUTTONUP:
				{
					_r_wnd_sendmessage (hwnd, 0, WM_COMMAND, MAKEWPARAM (IDM_TRAY_SHOW, 0), 0);
					break;
				}

				case WM_MBUTTONUP:
				{
					_r_wnd_sendmessage (hwnd, 0, WM_COMMAND, MAKEWPARAM (IDM_EXPLORE, 0), 0);
					break;
				}

				case WM_CONTEXTMENU:
				{
					WCHAR mode_fullscreen[128];
					WCHAR mode_monitor[128];
					WCHAR mode_window[128];
					WCHAR mode_region[128];
					WCHAR key_string[128];
					HMENU hsubmenu_settings;
					HMENU hmenu_settings;
					HMENU hsubmenu;
					HMENU hmenu;

					SetForegroundWindow (hwnd); // don't touch

					hmenu = LoadMenuW (NULL, MAKEINTRESOURCEW (IDM_TRAY));

					if (hmenu)
					{
						hsubmenu = GetSubMenu (hmenu, 0);

						if (hsubmenu)
						{
							_r_str_copy (mode_fullscreen, RTL_NUMBER_OF (mode_fullscreen), _r_locale_getstring (IDS_MODE_FULLSCREEN));
							_r_str_copy (mode_monitor, RTL_NUMBER_OF (mode_monitor), _r_locale_getstring (IDS_MODE_MONITOR));
							_r_str_copy (mode_window, RTL_NUMBER_OF (mode_window), _r_locale_getstring (IDS_MODE_WINDOW));
							_r_str_copy (mode_region, RTL_NUMBER_OF (mode_region), _r_locale_getstring (IDS_MODE_REGION));

							if (_r_config_getboolean (L"HotkeyFullscreenEnabled", FALSE, NULL))
							{
								_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getlong (L"HotkeyFullscreen2", HOTKEY_FULLSCREEN, NULL));

								_r_str_appendformat (mode_fullscreen, RTL_NUMBER_OF (mode_fullscreen), L"\t%s", key_string);
							}

							if (_r_config_getboolean (L"HotkeyMonitorEnabled", TRUE, NULL))
							{
								_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getlong (L"HotkeyMonitor", HOTKEY_MONITOR, NULL));

								_r_str_appendformat (mode_monitor, RTL_NUMBER_OF (mode_monitor), L"\t%s", key_string);
							}

							if (_r_config_getboolean (L"HotkeyWindowEnabled", TRUE, NULL))
							{
								_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getlong (L"HotkeyWindow", HOTKEY_WINDOW, NULL));

								_r_str_appendformat (mode_window, RTL_NUMBER_OF (mode_window), L"\t%s", key_string);
							}

							if (_r_config_getboolean (L"HotkeyRegionEnabled", TRUE, NULL))
							{
								_app_key2string (key_string, RTL_NUMBER_OF (key_string), _r_config_getlong (L"HotkeyRegion", HOTKEY_REGION, NULL));

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
							hmenu_settings = LoadMenuW (NULL, MAKEINTRESOURCEW (IDM_SETTINGS));

							if (hmenu_settings)
							{
								hsubmenu_settings = GetSubMenu (hmenu_settings, 0);

								if (hsubmenu_settings)
								{
									_app_initdropdownmenu (hsubmenu_settings, FALSE);

									_r_menu_addsubmenu (hsubmenu, SETTINGS_MENU, hsubmenu_settings, _r_locale_getstring (IDS_SETTINGS));
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

			nmlp = (LPNMHDR)lparam;

			switch (nmlp->code)
			{
				case BCN_DROPDOWN:
				{
					LPNMBCDROPDOWN lpdropdown;

					lpdropdown = (LPNMBCDROPDOWN)lparam;

					if (lpdropdown->hdr.idFrom != IDC_SETTINGS)
						break;

					_r_wnd_sendmessage (hwnd, 0, WM_COMMAND, MAKEWPARAM (IDC_SETTINGS, 0), 0);

					SetWindowLongPtrW (hwnd, DWLP_MSGRESULT, TRUE);

					return TRUE;
				}
			}

			return FALSE;
		}

		case WM_COMMAND:
		{
			INT ctrl_id = LOWORD (wparam);
			INT notify_code = HIWORD (wparam);

			if (notify_code == 0 && ctrl_id >= IDX_LANGUAGE && ctrl_id <= IDX_LANGUAGE + (INT)(INT_PTR)_r_locale_getcount () + 1)
			{
				_r_locale_apply (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), ctrl_id, IDX_LANGUAGE);

				return FALSE;
			}
			else if ((ctrl_id >= IDX_FORMATS && ctrl_id <= IDX_FORMATS + (INT)(INT_PTR)_r_obj_getarraysize (config.formats)))
			{
				LONG index;

				index = (ctrl_id - IDX_FORMATS);

				_r_config_setlong (L"ImageFormat", _r_calc_clamp (index, 0, (LONG)_r_obj_getarraysize (config.formats) - 1), NULL);

				return FALSE;
			}
			else if ((ctrl_id >= IDX_DELAY && ctrl_id <= IDX_DELAY + RTL_NUMBER_OF (timer_array)))
			{
				LONG index;

				index = (ctrl_id - IDX_DELAY);

				_r_config_setlong (L"Delay", _r_calc_clamp (index, 0, RTL_NUMBER_OF (timer_array)) + 1, NULL);

				return FALSE;
			}
			else if (ctrl_id == IDC_FOLDER && notify_code == EN_KILLFOCUS)
			{
				PR_STRING path_string;

				path_string = _r_ctrl_getstring (hwnd, ctrl_id);

				if (path_string)
				{
					_r_config_setstringexpand (L"Folder", path_string->buffer, NULL);

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

					if (!hbutton)
						break;

					if (!GetWindowRect (hbutton, &rect))
						break;

					hmenu = LoadMenuW (_r_sys_getimagebase (), MAKEINTRESOURCEW (IDM_SETTINGS));

					if (!hmenu)
						break;

					hsubmenu = GetSubMenu (hmenu, 0);

					if (hsubmenu)
					{
						_app_initdropdownmenu (hsubmenu, TRUE);

						_r_menu_popup (hsubmenu, hwnd, (PPOINT)&rect, TRUE);
					}

					DestroyMenu (hmenu);

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
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"AlwaysOnTop", FALSE, NULL);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);
					_r_config_setboolean (L"AlwaysOnTop", new_val, NULL);

					_r_wnd_top (hwnd, new_val);

					break;
				}

				case IDM_DARKMODE_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_theme_isenabled ();

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);

					_r_theme_enable (hwnd, new_val);

					break;
				}

				case IDM_LOADONSTARTUP_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_autorun_isenabled ();

					_r_autorun_enable (hwnd, new_val);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, _r_autorun_isenabled ());

					break;
				}

				case IDM_STARTMINIMIZED_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsStartMinimized", FALSE, NULL);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);
					_r_config_setboolean (L"IsStartMinimized", new_val, NULL);

					break;
				}

				case IDM_HIDEME_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsHideMe", TRUE, NULL);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);
					_r_config_setboolean (L"IsHideMe", new_val, NULL);

					break;
				}

				case IDM_CHECKUPDATES_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_update_isenabled (FALSE);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);
					_r_update_enable (new_val);

					break;
				}

				case IDC_BROWSE_BTN:
				{
					R_FILE_DIALOG file_dialog;
					PR_STRING path;
					HRESULT status;

					status = _r_filedialog_initialize (&file_dialog, PR_FILEDIALOG_OPENDIR);

					if (SUCCEEDED (status))
					{
						path = _app_getdirectory ();

						_r_filedialog_setpath (&file_dialog, path->buffer);
						_r_obj_dereference (path);

						status = _r_filedialog_show (hwnd, &file_dialog);

						if (SUCCEEDED (status))
						{
							status = _r_filedialog_getpath (&file_dialog, &path);

							if (SUCCEEDED (status))
							{
								_r_config_setstringexpand (L"Folder", path->buffer, NULL);

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
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsPlaySound", TRUE, NULL);

					_r_config_setboolean (L"IsPlaySound", new_val, NULL);

					break;
				}

				case IDM_HOTKEYS:
				{
					_r_settings_createwindow (hwnd, &SettingsProc, IDD_SETTINGS_HOTKEYS);
					break;
				}

				case IDM_COPYTOCLIPBOARD_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"CopyToClipboard", FALSE, NULL);

					_r_config_setboolean (L"CopyToClipboard", new_val, NULL);

					break;
				}

				case IDM_DISABLEAEROONWND_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsDisableAeroOnWnd", FALSE, NULL);

					_r_config_setboolean (L"IsDisableAeroOnWnd", new_val, NULL);

					break;
				}

				case IDM_DELAY_DISABLE:
				{
					_r_config_setlong (L"Delay", 0, NULL);
					break;
				}

				case IDM_INCLUDEMOUSECURSOR_CHK:
				case IDC_INCLUDEMOUSECURSOR_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsIncludeMouseCursor", FALSE, NULL);

					_r_config_setboolean (L"IsIncludeMouseCursor", new_val, NULL);
					_r_ctrl_checkbutton (hwnd, IDC_INCLUDEMOUSECURSOR_CHK, new_val);

					break;
				}

				case IDM_CLEARBACKGROUND_CHK:
				case IDC_CLEARBACKGROUND_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsClearBackground", TRUE, NULL);

					_r_config_setboolean (L"IsClearBackground", new_val, NULL);
					_r_ctrl_checkbutton (hwnd, IDC_CLEARBACKGROUND_CHK, new_val);

					break;
				}

				case IDM_INCLUDEWINDOWSHADOW_CHK:
				case IDC_INCLUDEWINDOWSHADOW_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"IsIncludeWindowShadow", TRUE, NULL);

					_r_config_setboolean (L"IsIncludeWindowShadow", new_val, NULL);
					_r_ctrl_checkbutton (hwnd, IDC_INCLUDEWINDOWSHADOW_CHK, new_val);

					break;
				}

				case IDC_MODE_MONITOR:
				case IDC_MODE_WINDOW:
				case IDC_MODE_REGION:
				{
					if (ctrl_id == IDC_MODE_WINDOW)
					{
						_r_config_setlong (L"Mode", SHOT_WINDOW, NULL);
					}
					else if (ctrl_id == IDC_MODE_REGION)
					{
						_r_config_setlong (L"Mode", SHOT_REGION, NULL);
					}
					else if (ctrl_id == IDC_MODE_MONITOR)
					{
						_r_config_setlong (L"Mode", SHOT_MONITOR, NULL);
					}

					break;
				}

				case IDM_FILENAME_INDEX:
				case IDM_FILENAME_DATE:
				{
					if (ctrl_id == IDM_FILENAME_DATE)
					{
						_r_config_setlong (L"FilenameType", NAME_DATE, NULL);
					}
					else
					{
						_r_config_setlong (L"FilenameType", NAME_INDEX, NULL);
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
					POINT pt;

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

INT APIENTRY wWinMain (
	_In_ HINSTANCE hinst,
	_In_opt_ HINSTANCE prev_hinst,
	_In_ LPWSTR cmdline,
	_In_ INT show_cmd
)
{
	HWND hwnd;

	if (!_r_app_initialize (NULL))
		return ERROR_APP_INIT_FAILURE;

	hwnd = _r_app_createwindow (hinst, MAKEINTRESOURCEW (IDD_MAIN), MAKEINTRESOURCEW (IDI_MAIN), &DlgProc);

	if (!hwnd)
		return ERROR_APP_INIT_FAILURE;

	return _r_wnd_message_callback (hwnd, MAKEINTRESOURCEW (IDA_MAIN));
}

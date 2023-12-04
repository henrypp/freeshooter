// Free Shooter
// Copyright (c) 2009-2023 Henry++

#pragma once

BOOL CALLBACK enum_monitor_timer_callback (
	_In_ HMONITOR hmonitor,
	_In_ HDC hdc,
	_In_ LPRECT rect,
	_In_ LPARAM lparam
);

VOID _app_createregion ();

VOID _app_createtimer (
	_In_ LONG delay_id,
	_In_ PSHOT_INFO shot_info
);

VOID _app_initializeregion (
	_Inout_ PMONITOR_CONTEXT monitor_context
);

VOID _app_initializetimer (
	_Inout_ PMONITOR_CONTEXT monitor_context
);

VOID _app_destroyregion (
	_Inout_ PMONITOR_CONTEXT monitor_context
);

VOID _app_destroytimer (
	_Inout_ PMONITOR_CONTEXT monitor_context
);

HWND _app_showdummy (
	_In_opt_ HWND hdummy,
	_In_opt_ HWND hwnd,
	_In_opt_ LPCRECT rect
);

VOID _app_updateregionrect (
	_In_ PMONITOR_CONTEXT monitor_context
);

LRESULT WINAPI DummyProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
);

INT_PTR CALLBACK RegionProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
);

INT_PTR CALLBACK TimerProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
);

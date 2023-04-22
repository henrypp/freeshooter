// Free Shooter
// Copyright (c) 2009-2023 Henry++

#pragma once

#define DUMMY_CLASS_DLG L"DummyDlg"

typedef struct _MONITOR_CONTEXT
{
	HWND hwnd;
	HCURSOR hcursor;
	RECT rect;

	union
	{
		struct
		{
			HDC hcapture;
			HDC hcapture_mask;
			HBITMAP hbitmap;
			HBITMAP hbitmap_mask;
			HPEN hpen;
			HPEN hpen_draw;
			HDC hdc;

			POINT pt_start;
			POINT pt_end;

			BOOLEAN is_drawing;
		} region;

		struct
		{
			PSHOT_INFO shot_info;
			HFONT hfont;
			volatile LONG timer_value;
		} timer;
	};
} MONITOR_CONTEXT, *PMONITOR_CONTEXT;

typedef struct _TIMER_CONTEXT
{
	HWND hwnd;
	LONG timer_value;
	PSHOT_INFO shot_info;
} TIMER_CONTEXT, *PTIMER_CONTEXT;

typedef struct _DUMMY_CONTEXT
{
	HWND hwnd;
	RECT rect;
} DUMMY_CONTEXT, *PDUMMY_CONTEXT;

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

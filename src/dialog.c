// Free Shooter
// Copyright (c) 2009-2023 Henry++

#include "global.h"

BOOL CALLBACK enum_monitor_timer_callback (
	_In_ HMONITOR hmonitor,
	_In_ HDC hdc,
	_In_ LPRECT rect,
	_In_ LPARAM lparam
)
{
	PMONITOR_CONTEXT monitor_context;
	PTIMER_CONTEXT timer_context;

	monitor_context = _r_freelist_allocateitem (&context_list);

	CopyRect (&monitor_context->rect, rect);

	timer_context = (PTIMER_CONTEXT)lparam;

	monitor_context->timer.timer_value = timer_context->timer_value;

	if (timer_context->shot_info)
	{
		monitor_context->timer.shot_info = _r_obj_reference (timer_context->shot_info);
		timer_context->shot_info = NULL;
	}

	_r_wnd_createwindow (_r_sys_getimagebase (), MAKEINTRESOURCE (IDD_DUMMY), _r_app_gethwnd (), &TimerProc, monitor_context);

	return TRUE;
}

VOID _app_createregion ()
{
	PMONITOR_CONTEXT monitor_context;
	ULONG status;

	status = WaitForSingleObjectEx (config.hregion_mutex, 0, FALSE);

	if (status != WAIT_OBJECT_0)
		return;

	ResetEvent (config.hregion_mutex);

	monitor_context = _r_freelist_allocateitem (&context_list);

	_r_wnd_createwindow (_r_sys_getimagebase (), MAKEINTRESOURCE (IDD_DUMMY), _r_app_gethwnd (), &RegionProc, monitor_context);
}

VOID _app_createtimer (
	_In_ LONG delay_id,
	_In_ PSHOT_INFO shot_info
)
{
	TIMER_CONTEXT timer_context = {0};

	timer_context.timer_value = timer_array[delay_id];
	timer_context.shot_info = shot_info;

	EnumDisplayMonitors (NULL, NULL, &enum_monitor_timer_callback, (LPARAM)&timer_context);
}

VOID _app_initializeregion (
	_Inout_ PMONITOR_CONTEXT monitor_context
)
{
	HDC hdc;
	LONG dpi_value;
	COLORREF pen_clr;
	COLORREF pen_draw_clr;
	LONG pen_size;
	LONG width;
	LONG height;
	COLORREF clr;
	R_BYTEREF bmp_bytes;
	PULONG bmp_buffer;

	// cleanup resources
	_app_destroyregion (monitor_context);

	dpi_value = _r_dc_getwindowdpi (monitor_context->hwnd);

	width = _r_calc_rectwidth (&monitor_context->rect);
	height = _r_calc_rectheight (&monitor_context->rect);

	// load cursor
	monitor_context->hcursor = LoadCursor (_r_sys_getimagebase (), MAKEINTRESOURCE (IDI_MAIN));

	// create pen
	pen_size = _r_dc_getsystemmetrics (SM_CXBORDER, dpi_value) * 4;

	pen_clr = _r_dc_getcoloraccent ();
	pen_draw_clr = pen_clr;
	//pen_draw_clr = _r_dc_getcolorinverse (pen_clr);

	monitor_context->region.hpen = CreatePen (PS_SOLID, pen_size, pen_clr);
	monitor_context->region.hpen_draw = CreatePen (PS_SOLID, pen_size, pen_draw_clr);

	// create bitmap
	hdc = GetDC (NULL);

	if (hdc)
	{
		monitor_context->region.hdc = CreateCompatibleDC (hdc);

		// create original bitmap
		monitor_context->region.hcapture = CreateCompatibleDC (hdc);
		monitor_context->region.hbitmap = _app_image_createbitmap (hdc, width, height, NULL);

		if (monitor_context->region.hcapture && monitor_context->region.hbitmap)
		{
			SelectObject (monitor_context->region.hcapture, monitor_context->region.hbitmap);

			BitBlt (monitor_context->region.hcapture, 0, 0, width, height, hdc, monitor_context->rect.left, monitor_context->rect.top, SRCCOPY);
		}

		// create bitmap mask
		monitor_context->region.hcapture_mask = CreateCompatibleDC (hdc);
		monitor_context->region.hbitmap_mask = _app_image_createbitmap (hdc, width, height, &bmp_bytes);

		if (monitor_context->region.hcapture_mask && monitor_context->region.hbitmap_mask)
		{
			SelectObject (monitor_context->region.hcapture_mask, monitor_context->region.hbitmap_mask);

			BitBlt (monitor_context->region.hcapture_mask, 0, 0, width, height, hdc, monitor_context->rect.left, monitor_context->rect.top, SRCCOPY);

			// blend bitmap bits
			for (SIZE_T i = 0; i < bmp_bytes.length / sizeof (COLORREF); i++)
			{
				bmp_buffer = PTR_ADD_OFFSET (bmp_bytes.buffer, i * sizeof (COLORREF));

				clr = *bmp_buffer;

				*bmp_buffer = RGB (GetRValue (clr) / BLEND, GetGValue (clr) / BLEND, GetBValue (clr) / BLEND);
			}
		}

		ReleaseDC (NULL, hdc);
	}

	SetWindowPos (
		monitor_context->hwnd,
		HWND_TOPMOST,
		monitor_context->rect.left,
		monitor_context->rect.top,
		width,
		height,
		SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_SHOWWINDOW
	);

	InvalidateRect (monitor_context->hwnd, NULL, TRUE);
}

VOID _app_initializetimer (
	_Inout_ PMONITOR_CONTEXT monitor_context
)
{
	LOGFONT logfont = {0};
	LONG dpi_value;
	LONG width;
	LONG height;

	// cleanup resources
	_app_destroytimer (monitor_context);

	// set window configuration
	_r_wnd_setstyle_ex (monitor_context->hwnd, _r_wnd_getstyle_ex (monitor_context->hwnd) | WS_EX_LAYERED);

	SetLayeredWindowAttributes (monitor_context->hwnd, 0, 0, LWA_COLORKEY);

	dpi_value = _r_dc_getmonitordpi (&monitor_context->rect);

	// initialize font
	_r_str_copy (logfont.lfFaceName, LF_FACESIZE, L"Segoe UI"); // face name

	logfont.lfWeight = FW_BOLD;
	logfont.lfHeight = _r_dc_fontsizetoheight (90, dpi_value); // size

	monitor_context->timer.hfont = CreateFontIndirect (&logfont);

	width = _r_calc_percentval (18, _r_calc_rectwidth (&monitor_context->rect));
	height = _r_calc_percentval (24, _r_calc_rectheight (&monitor_context->rect));

	SetTimer (monitor_context->hwnd, 5431, 1000, NULL);

	SetWindowPos (
		monitor_context->hwnd,
		HWND_TOPMOST,
		monitor_context->rect.left + _r_calc_rectwidth (&monitor_context->rect) - width - 100,
		monitor_context->rect.top + _r_calc_rectheight (&monitor_context->rect) - height - 100,
		width,
		height,
		SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_SHOWWINDOW
	);
}

VOID _app_destroyregion (
	_Inout_ PMONITOR_CONTEXT monitor_context
)
{
	SAFE_DELETE_ICON (monitor_context->hcursor);

	SAFE_DELETE_OBJECT (monitor_context->region.hpen);
	SAFE_DELETE_OBJECT (monitor_context->region.hpen_draw);

	SAFE_DELETE_OBJECT (monitor_context->region.hbitmap);
	SAFE_DELETE_OBJECT (monitor_context->region.hbitmap_mask);

	SAFE_DELETE_DC (monitor_context->region.hcapture);
	SAFE_DELETE_DC (monitor_context->region.hcapture_mask);

	SAFE_DELETE_DC (monitor_context->region.hdc);
}

VOID _app_destroytimer (
	_Inout_ PMONITOR_CONTEXT monitor_context
)
{
	SAFE_DELETE_ICON (monitor_context->hcursor);

	SAFE_DELETE_OBJECT (monitor_context->timer.hfont);
}

HWND _app_showdummy (
	_In_opt_ HWND hdummy,
	_In_opt_ HWND hwnd,
	_In_opt_ LPCRECT rect
)
{
	DUMMY_CONTEXT dummy_context;

	if (hdummy)
	{
		DestroyWindow (hdummy);
	}
	else if (hwnd && rect)
	{
		dummy_context.hwnd = hwnd;

		CopyRect (&dummy_context.rect, rect);

		hdummy = CreateWindowEx (0, DUMMY_CLASS_DLG, _r_app_getname (), WS_POPUP | WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, _r_sys_getimagebase (), &dummy_context);

		return hdummy;
	}

	return NULL;
}

VOID _app_updateregionrect (
	_In_ PMONITOR_CONTEXT monitor_context
)
{
	_app_getmonitorrect (&monitor_context->rect);
}

LRESULT WINAPI DummyProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
)
{
	switch (msg)
	{
		case WM_CREATE:
		{
			LPCREATESTRUCT lpcs;
			PDUMMY_CONTEXT dummy_context;
			BOOL is_success;

			lpcs = (LPCREATESTRUCT)lparam;
			dummy_context = lpcs->lpCreateParams;

			is_success = SetWindowPos (
				hwnd,
				dummy_context->hwnd,
				dummy_context->rect.left,
				dummy_context->rect.top,
				_r_calc_rectwidth (&dummy_context->rect),
				_r_calc_rectheight (&dummy_context->rect),
				SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOSENDCHANGING
			);

			if (!is_success)
			{
				// uipi fix
				is_success = SetWindowPos (
					hwnd,
					HWND_BOTTOM,
					dummy_context->rect.left,
					dummy_context->rect.top,
					_r_calc_rectwidth (&dummy_context->rect),
					_r_calc_rectheight (&dummy_context->rect),
					SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_NOSENDCHANGING
				);
			}

			InvalidateRect (hwnd, NULL, TRUE);

			break;
		}

		case WM_ERASEBKGND:
		{
			RECT rect;
			HDC hdc;

			if (!GetClientRect (hwnd, &rect))
				break;

			hdc = (HDC)wparam;

			_r_dc_fillrect (hdc, &rect, BG_COLOR_WINDOW);

			return TRUE;
		}
	}

	return DefWindowProc (hwnd, msg, wparam, lparam);
}

INT_PTR CALLBACK RegionProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
)
{
	PMONITOR_CONTEXT context;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			context = (PMONITOR_CONTEXT)lparam;

			context->hwnd = hwnd;

			_r_wnd_setcontext (hwnd, LONG_MAX, context);

			_app_updateregionrect (context);
			_app_initializeregion (context);

			break;
		}

		case WM_NCDESTROY:
		{
			context = _r_wnd_getcontext (hwnd, LONG_MAX);

			if (!context)
				break;

			_r_wnd_removecontext (hwnd, LONG_MAX);

			_app_destroyregion (context);

			SetEvent (config.hregion_mutex);

			_r_freelist_deleteitem (&context_list, context);

			break;
		}

		case WM_DPICHANGED:
		case WM_DISPLAYCHANGE:
		{
			context = _r_wnd_getcontext (hwnd, LONG_MAX);

			if (!context)
			{
				DestroyWindow (hwnd);
				break;
			}

			_app_updateregionrect (context);
			_app_initializeregion (context);

			break;
		}

		case WM_LBUTTONDOWN:
		{
			PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDM_REGION_START, 0), lparam);
			break;
		}

		case WM_MBUTTONDOWN:
		{
			PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDM_REGION_CANCEL, 0), 0);
			break;
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

			if (!hmenu)
				break;

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

			break;
		}

		case WM_SETCURSOR:
		{
			context = _r_wnd_getcontext (hwnd, LONG_MAX);

			if (!context)
				break;

			if (context->hcursor)
			{
				SetCursor (context->hcursor);
				return TRUE;
			}

			break;
		}

		case WM_MOUSEMOVE:
		{
			InvalidateRect (hwnd, NULL, TRUE);
			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rect;
			POINT pt;
			HDC hdc;
			HDC hdc_buffered;
			BP_PAINTPARAMS bpp;
			HPAINTBUFFER hdpaint;
			HGDIOBJ old_pen;
			HGDIOBJ old_brush;

			context = _r_wnd_getcontext (hwnd, LONG_MAX);

			if (!context)
				break;

			if (!GetCursorPos (&pt))
				break;

			if (!GetClientRect (hwnd, &rect))
				break;

			MapWindowPoints (HWND_DESKTOP, hwnd, &pt, 1);

			hdc = BeginPaint (hwnd, &ps);

			if (!hdc)
				break;

			RtlZeroMemory (&bpp, sizeof (bpp));

			bpp.cbSize = sizeof (bpp);
			bpp.dwFlags = BPPF_NOCLIP;

			hdpaint = BeginBufferedPaint (hdc, &rect, BPBF_TOPDOWNDIB, &bpp, &hdc_buffered);

			if (hdpaint)
			{
				BitBlt (hdc_buffered, 0, 0, _r_calc_rectwidth (&context->rect), _r_calc_rectheight (&context->rect), context->region.hcapture_mask, 0, 0, SRCCOPY);

				old_pen = SelectObject (hdc_buffered, context->region.is_drawing ? context->region.hpen_draw : context->region.hpen);

				old_brush = SelectObject (hdc_buffered, GetStockObject (NULL_BRUSH));

				if (context->region.is_drawing)
				{
					// draw region rectangle
					SetRect (
						&rect,
						min (context->region.pt_start.x, pt.x),
						min (context->region.pt_start.y, pt.y),
						max (context->region.pt_start.x, pt.x),
						max (context->region.pt_start.y, pt.y)
					);

					BitBlt (hdc_buffered, rect.left, rect.top, _r_calc_rectwidth (&rect), _r_calc_rectheight (&rect), context->region.hcapture, rect.left, rect.top, SRCCOPY);

					Rectangle (hdc_buffered, context->region.pt_start.x, context->region.pt_start.y, pt.x, pt.y);
				}
				else
				{
					// draw cursor crosshair
					MoveToEx (hdc_buffered, 0, pt.y, NULL);
					LineTo (hdc_buffered, _r_calc_rectwidth (&rect), pt.y);

					MoveToEx (hdc_buffered, pt.x, 0, NULL);
					LineTo (hdc_buffered, pt.x, _r_calc_rectheight (&rect));
				}

				SelectObject (hdc_buffered, old_brush);
				SelectObject (hdc_buffered, old_pen);

				EndBufferedPaint (hdpaint, TRUE);
			}

			EndPaint (hwnd, &ps);

			break;
		}

		case WM_ERASEBKGND:
		{
			return TRUE;
		}

		case WM_KEYDOWN:
		{
			if (wparam == VK_ESCAPE)
				PostMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDM_REGION_CANCEL, 0), 0);

			break;
		}

		case WM_COMMAND:
		{
			INT ctrl_id = LOWORD (wparam);
			INT notify_code = HIWORD (wparam);

			switch (ctrl_id)
			{
				case IDM_REGION_START:
				{
					PSHOT_INFO shot_info;
					RECT rect;

					context = _r_wnd_getcontext (hwnd, LONG_MAX);

					if (!context)
						break;

					if (!context->region.is_drawing)
					{
						context->region.is_drawing = TRUE;

						context->region.pt_start.x = LOWORD (lparam);
						context->region.pt_start.y = HIWORD (lparam);

						InvalidateRect (hwnd, NULL, TRUE);
					}
					else
					{
						context->region.is_drawing = FALSE;

						context->region.pt_end.x = LOWORD (lparam);
						context->region.pt_end.y = HIWORD (lparam);

						SetRect (
							&rect,
							min (context->region.pt_start.x, context->region.pt_end.x),
							min (context->region.pt_start.y, context->region.pt_end.y),
							max (context->region.pt_start.x, context->region.pt_end.x),
							max (context->region.pt_start.y, context->region.pt_end.y)
						);

						if (!IsRectEmpty (&rect))
						{
							shot_info = _r_obj_allocate (sizeof (SHOT_INFO), NULL);

							_r_wnd_recttorectangle (&shot_info->rectangle, &rect);

							shot_info->hcapture = context->region.hcapture;
							context->region.hcapture = NULL;

							_app_savescreenshot (shot_info);

							_r_obj_dereference (shot_info);

							DestroyWindow (hwnd);
						}
					}

					break;
				}

				case IDM_REGION_CANCEL:
				{
					context = _r_wnd_getcontext (hwnd, LONG_MAX);

					if (!context)
						break;

					if (context->region.is_drawing)
					{
						context->region.is_drawing = FALSE;

						context->region.pt_start.x = context->region.pt_start.y = 0;
						context->region.pt_end.x = context->region.pt_end.y = 0;

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

	return FALSE;
}

INT_PTR CALLBACK TimerProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
)
{
	PMONITOR_CONTEXT context;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			context = (PMONITOR_CONTEXT)lparam;

			context->hwnd = hwnd;

			_r_wnd_setcontext (hwnd, SHORT_MAX, context);

			_app_initializetimer (context);

			break;
		}

		case WM_CLOSE:
		{
			DestroyWindow (hwnd);
			break;
		}

		case WM_NCDESTROY:
		{
			context = _r_wnd_getcontext (hwnd, SHORT_MAX);

			if (!context)
				break;

			_r_wnd_removecontext (hwnd, SHORT_MAX);

			_app_destroytimer (context);

			_r_freelist_deleteitem (&context_list, context);

			break;
		}

		case WM_TIMER:
		{
			LONG seconds;
			LONG dpi_value;

			context = _r_wnd_getcontext (hwnd, SHORT_MAX);

			if (!context)
			{
				KillTimer (hwnd, wparam);
				DestroyWindow (hwnd);

				break;
			}

			seconds = InterlockedDecrement (&context->timer.timer_value);

			InvalidateRect (hwnd, NULL, TRUE);

			if (seconds <= 0)
			{
				dpi_value = _r_dc_getwindowdpi (hwnd);

				KillTimer (hwnd, wparam);

				SetWindowPos (
					hwnd,
					NULL,
					-_r_dc_getsystemmetrics (SM_CXVIRTUALSCREEN, dpi_value),
					-_r_dc_getsystemmetrics (SM_CYVIRTUALSCREEN, dpi_value),
					0,
					0,
					SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_HIDEWINDOW
				);

				if (context->timer.shot_info)
				{
					_app_proceedscreenshot (context->timer.shot_info);
					_r_obj_clearreference (&context->timer.shot_info);
				}

				DestroyWindow (hwnd);
			}

			break;
		}

		case WM_ERASEBKGND:
		{
			return TRUE;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			HDC hdc_buffered;
			BP_PAINTPARAMS bpp;
			HPAINTBUFFER hdpaint;
			WCHAR text[8];
			RECT rect;
			INT length;

			context = _r_wnd_getcontext (hwnd, SHORT_MAX);

			if (!context)
				break;

			if (!GetClientRect (hwnd, &rect))
				break;

			hdc = BeginPaint (hwnd, &ps);

			if (!hdc)
				break;

			RtlZeroMemory (&bpp, sizeof (bpp));

			bpp.cbSize = sizeof (bpp);
			bpp.dwFlags = BPPF_ERASE;

			hdpaint = BeginBufferedPaint (hdc, &rect, BPBF_DIB, &bpp, &hdc_buffered);

			if (hdpaint)
			{
				BitBlt (hdc, 0, 0, rect.right, rect.bottom, hdc_buffered, 0, 0, SRCERASE);

				SetBkMode (hdc_buffered, TRANSPARENT);

				SetTextColor (hdc_buffered, _r_dc_getcoloraccent ());
				SelectObject (hdc_buffered, context->timer.hfont);

				_r_str_fromlong (text, RTL_NUMBER_OF (text), context->timer.timer_value);

				length = (INT)(INT_PTR)_r_str_getlength (text);

				DrawTextEx (hdc_buffered, text, length, &rect, DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX, NULL);

				EndBufferedPaint (hdpaint, TRUE);
			}

			EndPaint (hwnd, &ps);

			break;
		}
	}

	return FALSE;
}

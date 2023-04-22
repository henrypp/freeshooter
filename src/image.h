// Free Shooter
// Copyright (c) 2009-2023 Henry++

#pragma once

VOID _app_image_wicsetoptions (
	_In_ LPCGUID guid,
	_Inout_ IPropertyBag2 *property_bag
);

BOOLEAN _app_image_wicsavehbitmap (
	_In_ HBITMAP hbitmap,
	_In_ LPCWSTR filepath
);

HBITMAP _app_image_createbitmap (
	_In_opt_ HDC hdc,
	_In_ LONG width,
	_In_ LONG height,
	_Out_opt_ PR_BYTEREF out_bytes
);

VOID _app_image_savebitmaptofile (
	_In_ HBITMAP hbitmap,
	_In_ INT width,
	_In_ INT height
);

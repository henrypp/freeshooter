// Free Shooter
// Copyright (c) 2009-2023 Henry++

#include "global.h"

VOID _app_image_wicsetoptions (
	_In_ LPCGUID guid,
	_Inout_ IPropertyBag2 *property_bag
)
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
			_r_log (LOG_LEVEL_WARNING, 0, L"IPropertyBag2_Write", hr, NULL);
	}
}

BOOLEAN _app_image_wicsavehbitmap (
	_In_ HWND hwnd,
	_In_ HBITMAP hbitmap,
	_In_ LPCWSTR filepath
)
{
	IWICStream *wic_stream = NULL;
	IWICBitmap *wic_bitmap = NULL;
	IWICBitmapEncoder *wic_encoder = NULL;
	IWICBitmapFrameEncode *wic_frame = NULL;
	IWICImagingFactory2 *wic_factory = NULL;
	IPropertyBag2 *property_bag = NULL;
	PIMAGE_FORMAT format;
	BITMAP bitmap = {0};
	GUID pixel_format;
	BOOLEAN is_success = FALSE;
	HRESULT status;

	format = _app_getimageformat_data ();

	status = CoCreateInstance (&CLSID_WICImagingFactory2, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory2, &wic_factory);

	if (FAILED (status))
		goto CleanupExit;

	status = IWICImagingFactory_CreateBitmapFromHBITMAP (wic_factory, hbitmap, NULL, WICBitmapIgnoreAlpha, &wic_bitmap);

	if (FAILED (status))
		goto CleanupExit;

	status = IWICImagingFactory_CreateStream (wic_factory, &wic_stream);

	if (FAILED (status))
		goto CleanupExit;

	status = IWICStream_InitializeFromFilename (wic_stream, filepath, GENERIC_WRITE);

	if (FAILED (status))
		goto CleanupExit;

	status = IWICImagingFactory_CreateEncoder (wic_factory, &format->guid, NULL, &wic_encoder);

	if (FAILED (status))
		goto CleanupExit;

	status = IWICBitmapEncoder_Initialize (wic_encoder, (IStream*)wic_stream, WICBitmapEncoderNoCache);

	if (FAILED (status))
		goto CleanupExit;

	if (!GetObject (hbitmap, sizeof (bitmap), &bitmap))
		goto CleanupExit;

	status = IWICBitmapEncoder_CreateNewFrame (wic_encoder, &wic_frame, &property_bag);

	if (FAILED (status))
		goto CleanupExit;

	_app_image_wicsetoptions (&format->guid, property_bag);

	status = IWICBitmapFrameEncode_Initialize (wic_frame, property_bag);

	if (FAILED (status))
		goto CleanupExit;

	status = IWICBitmapFrameEncode_SetSize (wic_frame, bitmap.bmWidth, bitmap.bmHeight);

	if (FAILED (status))
	{
		pixel_format = GUID_WICPixelFormat24bppBGR;

		status = IWICBitmapFrameEncode_SetPixelFormat (wic_frame, &pixel_format);

		if (FAILED (status))
			goto CleanupExit;
	}

	status = IWICBitmapFrameEncode_WriteSource (wic_frame, (IWICBitmapSource*)wic_bitmap, NULL);

	if (FAILED (status))
		goto CleanupExit;

	status = IWICBitmapFrameEncode_Commit (wic_frame);

	if (FAILED (status))
		goto CleanupExit;

	status = IWICBitmapEncoder_Commit (wic_encoder);

	if (FAILED (status))
		goto CleanupExit;

	is_success = TRUE;

CleanupExit:

	if (FAILED (status))
	{
		_r_log (LOG_LEVEL_ERROR, 0, TEXT (__FUNCTION__), status, NULL);

		_r_show_errormessage (hwnd, NULL, status, NULL, FALSE);
	}

	if (property_bag)
		IPropertyBag2_Release (property_bag);

	if (wic_bitmap)
		IWICBitmap_Release (wic_bitmap);

	if (wic_frame)
		IWICBitmapFrameEncode_Release (wic_frame);

	if (wic_stream)
		IWICStream_Release (wic_stream);

	if (wic_encoder)
		IWICBitmapDecoder_Release (wic_encoder);

	if (wic_factory)
		IWICImagingFactory_Release (wic_factory);

	return is_success;
}

HBITMAP _app_image_createbitmap (
	_In_opt_ HDC hdc,
	_In_ LONG width,
	_In_ LONG height,
	_Out_opt_ PR_BYTEREF out_bytes
)
{
	BITMAPINFO bmi = {0};
	PVOID bits;
	HBITMAP hbitmap;

	bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32; // four 8-bit components
	bmi.bmiHeader.biSizeImage = (width * height) * 4; // rgba

	hbitmap = CreateDIBSection (hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);

	if (out_bytes)
	{
		out_bytes->buffer = bits;
		out_bytes->length = bmi.bmiHeader.biSizeImage;
	}

	return hbitmap;
}

VOID _app_image_savebitmaptofile (
	_In_ HWND hwnd,
	_In_ HBITMAP hbitmap,
	_In_ INT width,
	_In_ INT height
)
{
	PR_STRING path_string;
	PR_STRING unique_path_string;
	HBITMAP hbitmap_copy;

	_app_playsound ();

	if (_r_config_getboolean (L"CopyToClipboard", FALSE))
	{
		if (OpenClipboard (hwnd))
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

	_app_image_wicsavehbitmap (hwnd, hbitmap, path_string->buffer);

	_r_obj_dereference (path_string);
}

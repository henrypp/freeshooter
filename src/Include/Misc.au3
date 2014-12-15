#include <FontConstants.au3>

; #FUNCTION# ====================================================================================================================
; Name...........: _GUICtrlMenu_CheckRadioItem
; Description ...: Checks a specified menu item and makes it a radio item
; Syntax.........: _GUICtrlMenu_CheckRadioItem($hMenu, $iFirst, $iLast, $iCheck[, $fByPos = True])
; Parameters ....: $hMenu       - Handle of the menu
;                  $iFirst      - Identifier or position of the first menu item in the group
;                  $iLast       - Identifier or position of the last menu item in the group
;                  $iCheck      - Identifier or position of the menu item to check
;                  $fByPos      - Menu identifier flag:
;                  | True - $iFirst, $iLast and $iCheck are a zero based item position
;                  |False - $iFirst, $iLast and $iCheck are a menu item identifier
; Return values .: Success      - True
;                  Failure      - False
; Author ........: Paul Campbell (PaulIA)
; Modified.......:
; Remarks .......: This function sets the $MFT_RADIOCHECK type flag and the $MFS_CHECKED state for the item specified by  $iCheck
;                  and, at the same time, clears both flags for all other items in the group. The checked item is displayed using
;                  a bullet bitmap instead of a check-mark bitmap.
; Related .......: _GUICtrlMenu_CheckMenuItem
; Link ..........: @@MsdnLink@@ CheckMenuRadioItem
; Example .......: Yes
; ===============================================================================================================================
Func _GUICtrlMenu_CheckRadioItem($hMenu, $iFirst, $iLast, $iCheck, $fByPos = True)
	Local $iByPos = 0

	If $fByPos Then $iByPos = $MF_BYPOSITION
	Local $aResult = DllCall("User32.dll", "bool", "CheckMenuRadioItem", "handle", $hMenu, "uint", $iFirst, "uint", $iLast, "uint", $iCheck, "uint", $iByPos)
	If @error Then Return SetError(@error, @extended, False)
	Return $aResult[0]
EndFunc   ;==>_GUICtrlMenu_CheckRadioItem

; #FUNCTION# ====================================================================================================================
; Name...........: _ChooseFont
; Description ...: Creates a Font dialog box that enables the user to choose attributes for a logical font.
; Syntax.........: _ChooseFont([$sFontName = "Courier New"[, $iPointSize = 10[, $iColorRef = 0[, $iFontWeight = 0[, $iItalic = False[, $iUnderline = False[, $iStrikethru = False[, $hWndOwner = 0]]]]]]]])
; Parameters ....: $sFontName   - Default font name
;                  $iPointSize  - Pointsize of font
;                  $iColorRef   - COLORREF rgbColors
;                  $iFontWeight - Font Weight
;                  $iItalic     - Italic
;                  $iUnderline  - Underline
;                  $iStrikethru - Optional: Strikethru
;                  $hWndOwnder  - Handle to the window that owns the dialog box
; Return values .: Success      - Array in the following format:
;                  |[0] - contains the number of elements
;                  |[1] - attributes = BitOr of italic:2, undeline:4, strikeout:8
;                  |[2] - fontname
;                  |[3] - font size = point size
;                  |[4] - font weight = = 0-1000
;                  |[5] - COLORREF rgbColors
;                  |[6] - Hex BGR Color
;                  |[7] - Hex RGB Color
;                  Failure      - -1
; Author ........: Gary Frost (gafrost)
; Modified.......:
; Remarks .......:
; Related .......:
; Link ..........;
; Example .......; Yes
; ===============================================================================================================================
Func _ChooseFont($sFontName = "Courier New", $iPointSize = 10, $iColorRef = 0, $iFontWeight = 0, $iItalic = False, $iUnderline = False, $iStrikethru = False, $hWndOwner = 0)
	Local $tLogFont, $tChooseFont, $lngDC, $lfHeight, $iResult
	Local $fontname, $italic = 0, $underline = 0, $strikeout = 0
	Local $attributes, $size, $weight, $colorref, $color_picked


	$lngDC = _MISC_GetDC(0)
	$lfHeight = Round(($iPointSize * _MISC_GetDeviceCaps($lngDC, $LOGPIXELSX)) / 72, 0)
	_MISC_ReleaseDC(0, $lngDC)

	$tChooseFont = DllStructCreate($tagCHOOSEFONT)
	$tLogFont = DllStructCreate($tagLOGFONT)

	DllStructSetData($tChooseFont, "Size", DllStructGetSize($tChooseFont))
	DllStructSetData($tChooseFont, "hWndOwner", $hWndOwner)
	DllStructSetData($tChooseFont, "LogFont", DllStructGetPtr($tLogFont))
	DllStructSetData($tChooseFont, "PointSize", $iPointSize)
	DllStructSetData($tChooseFont, "Flags", BitOR($CF_SCREENFONTS, $CF_PRINTERFONTS, $CF_EFFECTS, $CF_INITTOLOGFONTSTRUCT, $CF_NOSCRIPTSEL))
	DllStructSetData($tChooseFont, "rgbColors", $iColorRef)
	DllStructSetData($tChooseFont, "FontType", 0)

	DllStructSetData($tLogFont, "Height", $lfHeight)
	DllStructSetData($tLogFont, "Weight", $iFontWeight)
	DllStructSetData($tLogFont, "Italic", $iItalic)
	DllStructSetData($tLogFont, "Underline", $iUnderline)
	DllStructSetData($tLogFont, "Strikeout", $iStrikethru)
	DllStructSetData($tLogFont, "FaceName", $sFontName)

	$iResult = DllCall("comdlg32.dll", "long", "ChooseFont", "ptr", DllStructGetPtr($tChooseFont))
	If ($iResult[0] == 0) Then Return SetError(-3, -3, -1) ; user selected cancel or struct settings incorrect

	$fontname = DllStructGetData($tLogFont, "FaceName")
	If (StringLen($fontname) == 0 And StringLen($sFontName) > 0) Then $fontname = $sFontName

	If (DllStructGetData($tLogFont, "Italic")) Then $italic = 2
	If (DllStructGetData($tLogFont, "Underline")) Then $underline = 4
	If (DllStructGetData($tLogFont, "Strikeout")) Then $strikeout = 8

	$attributes = BitOR($italic, $underline, $strikeout)
	$size = DllStructGetData($tChooseFont, "PointSize") / 10
	$colorref = DllStructGetData($tChooseFont, "rgbColors")
	$weight = DllStructGetData($tLogFont, "Weight")

	$color_picked = Hex(String($colorref), 6)

	Return StringSplit($attributes & "," & $fontname & "," & $size & "," & $weight & "," & $colorref & "," & '0x' & $color_picked & "," & '0x' & StringMid($color_picked, 5, 2) & StringMid($color_picked, 3, 2) & StringMid($color_picked, 1, 2), ",")
EndFunc   ;==>_ChooseFont

; #INTERNAL_USE_ONLY#============================================================================================================
; Name...........: _MISC_GetDC
; Description ...: Retrieves a handle of a display device context for the client area a window
; Syntax.........: _MISC_GetDC($hWnd)
; Parameters ....: $hWnd        - Handle of window
; Return values .: Success      - The device context for the given window's client area
;                  Failure      - 0
; Author ........: Paul Campbell (PaulIA)
; Modified.......:
; Remarks .......: After painting with a common device context, the _MISC_ReleaseDC function must be called to release the DC
; Related .......:
; Link ..........; @@MsdnLink@@ GetDC
; Example .......;
; ===============================================================================================================================
Func _MISC_GetDC($hWnd)
	Local $aResult

	$aResult = DllCall("User32.dll", "hwnd", "GetDC", "hwnd", $hWnd)
	If @error Then Return SetError(@error, 0, 0)
	Return $aResult[0]
EndFunc   ;==>_MISC_GetDC

; #INTERNAL_USE_ONLY#============================================================================================================
; Name...........: _MISC_GetDeviceCaps
; Description ...: Retrieves device specific information about a specified device
; Syntax.........: _MISC_GetDeviceCaps($hDC, $iIndex)
; Parameters ....: $hDC         - Identifies the device context
;                  $iIndex      - Specifies the item to return
; Return values .: Success      - The value of the desired item
; Author ........: Paul Campbell (PaulIA)
; Modified.......:
; Remarks .......:
; Related .......:
; Link ..........; @@MsdnLink@@ GetDeviceCaps
; Example .......;
; ===============================================================================================================================
Func _MISC_GetDeviceCaps($hDC, $iIndex)
	Local $aResult

	$aResult = DllCall("GDI32.dll", "int", "GetDeviceCaps", "hwnd", $hDC, "int", $iIndex)
	Return $aResult[0]
EndFunc   ;==>_MISC_GetDeviceCaps

; #INTERNAL_USE_ONLY#============================================================================================================
; Name...........: _MISC_ReleaseDC
; Description ...: Releases a device context
; Syntax.........: _MISC_ReleaseDC($hWnd, $hDC)
; Parameters ....: $hWnd        - Handle of window
;                  $hDC         - Identifies the device context to be released
; Return values .: Success      - True
;                  Failure      - False
; Author ........: Paul Campbell (PaulIA)
; Modified.......:
; Remarks .......: The application must call the _MISC_ReleaseDC function for each call to the _WinAPI_GetWindowDC function  and  for
;                  each call to the _WinAPI_GetDC function that retrieves a common device context.
; Related .......: _WinAPI_GetDC, _WinAPI_GetWindowDC
; Link ..........; @@MsdnLink@@ ReleaseDC
; Example .......;
; ===============================================================================================================================
Func _MISC_ReleaseDC($hWnd, $hDC)
	Local $aResult

	$aResult = DllCall("User32.dll", "int", "ReleaseDC", "hwnd", $hWnd, "hwnd", $hDC)
	Return $aResult[0] <> 0
EndFunc   ;==>_MISC_ReleaseDC
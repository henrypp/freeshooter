#include-once

#include <Memory.au3>
#include <WinAPI.au3>

; #INDEX# =======================================================================================================================
; Title .........: Clipboard
; AutoIt Version: 3.2.8++
; Language:       English
; Description ...: The clipboard is a set of functions and messages that  enable  applications  to  transfer  data.  Because  all
;                  applications have access to the clipboard, data can be easily transferred between applications  or  within  an
;                  application.
; Author ........: Paul Campbell (PaulIA)
; ===============================================================================================================================

; #CONSTANTS# ===================================================================================================================
Global Const $CF_TEXT = 1 ; Text format
Global Const $CF_BITMAP = 2 ; Handle to a bitmap (HBITMAP)
Global Const $CF_METAFILEPICT = 3 ; Handle to a metafile picture (METAFILEPICT)
Global Const $CF_SYLK = 4 ; Microsoft Symbolic Link (SYLK) format
Global Const $CF_DIF = 5 ; Software Arts' Data Interchange Format
Global Const $CF_TIFF = 6 ; Tagged image file format
Global Const $CF_OEMTEXT = 7 ; Text format containing characters in the OEM character set
Global Const $CF_DIB = 8 ; BITMAPINFO structure followed by the bitmap bits
Global Const $CF_PALETTE = 9 ; Handle to a color palette
Global Const $CF_PENDATA = 10 ; Data for the pen extensions to Pen Computing
Global Const $CF_RIFF = 11 ; Represents audio data in RIFF format
Global Const $CF_WAVE = 12 ; Represents audio data in WAVE format
Global Const $CF_UNICODETEXT = 13 ; Unicode text format
Global Const $CF_ENHMETAFILE = 14 ; Handle to an enhanced metafile (HENHMETAFILE)
Global Const $CF_HDROP = 15 ; Handle to type HDROP that identifies a list of files
Global Const $CF_LOCALE = 16 ; Handle to the locale identifier associated with text in the clipboard
Global Const $CF_DIBV5 = 17 ; BITMAPV5HEADER structure followed by bitmap color and the bitmap bits
Global Const $CF_OWNERDISPLAY = 0x0080 ; Owner display format
Global Const $CF_DSPTEXT = 0x0081 ; Text display format associated with a private format
Global Const $CF_DSPBITMAP = 0x0082 ; Bitmap display format associated with a private format
Global Const $CF_DSPMETAFILEPICT = 0x0083 ; Metafile picture display format associated with a private format
Global Const $CF_DSPENHMETAFILE = 0x008E ; Enhanced metafile display format associated with a private format
Global Const $CF_PRIVATEFIRST = 0x0200 ; Range of integer values for private clipboard formats
Global Const $CF_PRIVATELAST = 0x02FF ; Range of integer values for private clipboard formats
Global Const $CF_GDIOBJFIRST = 0x0300 ; Range for (GDI) object clipboard formats
Global Const $CF_GDIOBJLAST = 0x03FF ; Range for (GDI) object clipboard formats

; #FUNCTION# ====================================================================================================================
; Name...........: _ClipBoard_Close
; Description ...: Closes the clipboard
; Syntax.........: _ClipBoard_Close()
; Parameters ....:
; Return values .: Success      - True
;                  Failure      - False
; Author ........: Paul Campbell (PaulIA)
; Modified.......:
; Remarks .......: When the window has finished examining or changing the clipboard close the clipboard by calling this function.
;                  This enables other windows to access the clipboard.  Do not place an object on the clipboard after calling
;                  this function.
; Related .......: _ClipBoard_Open
; Link ..........; @@MsdnLink@@ CloseClipboard
; Example .......; Yes
; ===============================================================================================================================
Func _ClipBoard_Close()
	Local $aResult

	$aResult = DllCall("User32.dll", "int", "CloseClipboard")
	Return SetError($aResult[0] = 0, 0, $aResult[0] <> 0)
EndFunc   ;==>_ClipBoard_Close

; #FUNCTION# ====================================================================================================================
; Name...........: _ClipBoard_Empty
; Description ...: Empties the clipboard and frees handles to data in the clipboard
; Syntax.........: _ClipBoard_Empty()
; Parameters ....:
; Return values .: Success      - True
;                  Failure      - False
; Author ........: Paul Campbell (PaulIA)
; Modified.......:
; Remarks .......: Before calling this function, you must open the clipboard by using the _ClipBoard_Open function.  If you specified
;                  a NULL window handle when opening the clipboard, this function succeeds but sets the clipboard owner to NULL.
;                  Note that this causes _ClipBoard_SetData to fail.
; Related .......: _ClipBoard_Open, _ClipBoard_SetData
; Link ..........; @@MsdnLink@@ EmptyClipboard
; Example .......; Yes
; ===============================================================================================================================
Func _ClipBoard_Empty()
	Local $aResult

	$aResult = DllCall("User32.dll", "int", "EmptyClipboard")
	Return SetError($aResult[0] = 0, 0, $aResult[0] <> 0)
EndFunc   ;==>_ClipBoard_Empty

; #FUNCTION# ====================================================================================================================
; Name...........: _ClipBoard_Open
; Description ...: Opens the clipboard and prevents other applications from modifying the clipboard
; Syntax.........: _ClipBoard_Open($hOwner)
; Parameters ....: $hOwner      - Handle to the window to be associated with the open clipboard. If this parameter is 0, the open
;                  +clipboard is associated with the current task.
; Return values .: Success      - True
;                  Failure      - False
; Author ........: Paul Campbell (PaulIA)
; Modified.......:
; Remarks .......: This function fails if another window has the clipboard  open.  Call the _ClipBoard_Close function after every
;                  successful call to this function. The window identified by the $hOwner parameter does not become the clipboard
;                  owner unless the _ClipBoard_Empty function is called.  If you call _ClipBoard_Open with hwnd set to 0, _ClipBoard_Empty sets
;                  the clipboard owner to0 which causes _ClipBoard_SetData to fail.
; Related .......: _ClipBoard_Close, _ClipBoard_Empty
; Link ..........; @@MsdnLink@@ OpenClipboard
; Example .......; Yes
; ===============================================================================================================================
Func _ClipBoard_Open($hOwner)
	Local $aResult

	$aResult = DllCall("User32.dll", "int", "OpenClipboard", "hwnd", $hOwner)
	Return SetError($aResult[0] = 0, 0, $aResult[0] <> 0)
EndFunc   ;==>_ClipBoard_Open

; #FUNCTION# ====================================================================================================================
; Name...........: _ClipBoard_SetData
; Description ...: Places data on the clipboard in a specified clipboard format
; Syntax.........: _ClipBoard_SetData($vData[, $iFormat = 1])
; Parameters ....: $hMemory     - Handle to the data in the specified format.  This parameter can be NULL, indicating that the
;                  +window provides data in the specified clipboard format upon request.  If a window delays rendering, it must
;                  +process the $WM_RENDERFORMAT and $WM_RENDERALLFORMATS messages.  If this function succeeds, the system owns
;                  +the object identified by the $hMemory parameter.  The application may not write to or free the data once
;                  +ownership has been transferred to the system, but it can lock and read from the data until the _ClipBoard_Close
;                  +function is called.  The memory must be unlocked before the clipboard is closed.  If the $hMemory parameter
;                  +identifies a memory object, the object must have been allocated using the function with the $GMEM_MOVEABLE
;                  +flag.
;                  $iFormat     - Specifies a clipboard format:
;                  |$CF_TEXT            - Text format
;                  |$CF_BITMAP          - Handle to a bitmap (HBITMAP)
;                  |$CF_METAFILEPICT    - Handle to a metafile picture (METAFILEPICT)
;                  |$CF_SYLK            - Microsoft Symbolic Link (SYLK) format
;                  |$CF_DIF             - Software Arts' Data Interchange Format
;                  |$CF_TIFF            - Tagged image file format
;                  |$CF_OEMTEXT         - Text format containing characters in the OEM character set
;                  |$CF_DIB             - BITMAPINFO structure followed by the bitmap bits
;                  |$CF_PALETTE         - Handle to a color palette
;                  |$CF_PENDATA         - Data for the pen extensions to Pen Computing
;                  |$CF_RIFF            - Represents audio data in RIFF format
;                  |$CF_WAVE            - Represents audio data in WAVE format
;                  |$CF_UNICODETEXT     - Unicode text format
;                  |$CF_ENHMETAFILE     - Handle to an enhanced metafile (HENHMETAFILE)
;                  |$CF_HDROP           - Handle to type HDROP that identifies a list of files
;                  |$CF_LOCALE          - Handle to the locale identifier associated with text in the clipboard
;                  |$CF_DIBV5           - BITMAPV5HEADER structure followed by bitmap color and the bitmap bits
;                  |$CF_OWNERDISPLAY    - Owner display format
;                  |$CF_DSPTEXT         - Text display format associated with a private format
;                  |$CF_DSPBITMAP       - Bitmap display format associated with a private format
;                  |$CF_DSPMETAFILEPICT - Metafile picture display format associated with a private format
;                  |$CF_DSPENHMETAFILE  - Enhanced metafile display format associated with a private format
; Return values .: Success      - The handle to the data
;                  Failure      - 0
; Author ........: Paul Campbell (PaulIA)
; Modified.......:
; Remarks .......: This function performs all of the steps neccesary to put data on the clipboard.  It will allocate the global
;                  memory object, open the clipboard, place the data on the clipboard and close the clipboard.  If you need more
;                  control over putting data on the clipboard, you may want to use the _ClipBoard_SetDataEx function.
; Related .......: _ClipBoard_GetData, _ClipBoard_SetDataEx
; Link ..........;
; Example .......; Yes
; ===============================================================================================================================
Func _ClipBoard_SetData($vData, $iFormat = 1)
	Local $tData, $hLock, $hMemory, $iSize

	Switch $iFormat
		Case $CF_TEXT, $CF_OEMTEXT
			$iSize = StringLen($vData) + 1
			$hMemory = _MemGlobalAlloc($iSize, $GHND)
			If $hMemory = 0 Then Return SetError(-1, 0, 0)
			$hLock = _MemGlobalLock($hMemory)
			If $hLock = 0 Then Return SetError(-2, 0, 0)
			$tData = DllStructCreate("char Text[" & $iSize & "]", $hLock)
			DllStructSetData($tData, "Text", $vData)
			_MemGlobalUnlock($hMemory)
		Case Else
			; Assume all other formats are a pointer or a handle (until users tell me otherwise) :)
			$hMemory = $vData
	EndSwitch

	If Not _ClipBoard_Open(0) Then Return SetError(-5, 0, 0)
	If Not _ClipBoard_Empty() Then Return SetError(-6, 0, 0)
	If Not _ClipBoard_SetDataEx($hMemory, $iFormat) Then
		_ClipBoard_Close()
		Return SetError(-7, 0, 0)
	EndIf

	_ClipBoard_Close()
	Return $hMemory
EndFunc   ;==>_ClipBoard_SetData

; #FUNCTION# ====================================================================================================================
; Name...........: _ClipBoard_SetDataEx
; Description ...: Places data on the clipboard in a specified clipboard format
; Syntax.........: _ClipBoard_SetDataEx(ByRef $hMemory[, $iFormat = 1])
; Parameters ....: $hMemory     - Handle to the data in the specified format.  This parameter can be NULL, indicating that the
;                  +window provides data in the specified clipboard format upon request.  If a window delays rendering, it must
;                  +process the $WM_RENDERFORMAT and $WM_RENDERALLFORMATS messages.  If this function succeeds, the system owns
;                  +the object identified by the $hMemory parameter.  The application may not write to or free the data once
;                  +ownership has been transferred to the system, but it can lock and read from the data until the _ClipBoard_Close
;                  +function is called.  The memory must be unlocked before the clipboard is closed.  If the $hMemory parameter
;                  +identifies a memory object, the object must have been allocated using the function with the $GMEM_MOVEABLE
;                  +flag.
;                  $iFormat     - Specifies a clipboard format:
;                  |$CF_TEXT            - Text format
;                  |$CF_BITMAP          - Handle to a bitmap (HBITMAP)
;                  |$CF_METAFILEPICT    - Handle to a metafile picture (METAFILEPICT)
;                  |$CF_SYLK            - Microsoft Symbolic Link (SYLK) format
;                  |$CF_DIF             - Software Arts' Data Interchange Format
;                  |$CF_TIFF            - Tagged image file format
;                  |$CF_OEMTEXT         - Text format containing characters in the OEM character set
;                  |$CF_DIB             - BITMAPINFO structure followed by the bitmap bits
;                  |$CF_PALETTE         - Handle to a color palette
;                  |$CF_PENDATA         - Data for the pen extensions to Pen Computing
;                  |$CF_RIFF            - Represents audio data in RIFF format
;                  |$CF_WAVE            - Represents audio data in WAVE format
;                  |$CF_UNICODETEXT     - Unicode text format
;                  |$CF_ENHMETAFILE     - Handle to an enhanced metafile (HENHMETAFILE)
;                  |$CF_HDROP           - Handle to type HDROP that identifies a list of files
;                  |$CF_LOCALE          - Handle to the locale identifier associated with text in the clipboard
;                  |$CF_DIBV5           - BITMAPV5HEADER structure followed by bitmap color and the bitmap bits
;                  |$CF_OWNERDISPLAY    - Owner display format
;                  |$CF_DSPTEXT         - Text display format associated with a private format
;                  |$CF_DSPBITMAP       - Bitmap display format associated with a private format
;                  |$CF_DSPMETAFILEPICT - Metafile picture display format associated with a private format
;                  |$CF_DSPENHMETAFILE  - Enhanced metafile display format associated with a private format
; Return values .: Success      - The handle to the data
;                  Failure      - 0
; Author ........: Paul Campbell (PaulIA)
; Modified.......:
; Remarks .......: The $iFormat parameter can identify a registered clipboard format, or it can be one of the standard clipboard
;                  formats. If an application calls this function in response to $WM_RENDERFORMAT or $WM_RENDERALLFORMATS, the
;                  application should not use the handle after this function has been called.  If an application calls _ClipBoard_Open
;                  with a NULL handle, _ClipBoard_Empty sets the clipboard owner to NULL; this causes this function to fail.
; Related .......: _ClipBoard_Empty, _ClipBoard_GetData, _ClipBoard_Open
; Link ..........; @@MsdnLink@@ SetClipboardData
; Example .......; Yes
; ===============================================================================================================================
Func _ClipBoard_SetDataEx(ByRef $hMemory, $iFormat = 1)
	Local $aResult

	$aResult = DllCall("User32.dll", "int", "SetClipboardData", "int", $iFormat, "hwnd", $hMemory)
	Return SetError($aResult[0] = 0, 0, $aResult[0])
EndFunc   ;==>_ClipBoard_SetDataEx
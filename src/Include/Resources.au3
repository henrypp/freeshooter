#include-once
#include <WinAPI.au3>
#include <GDIPlus.au3>
#include <Memory.au3>

_GDIPlus_Startup()

Global Const $RT_CURSOR = 1
Global Const $RT_BITMAP = 2
Global Const $RT_ICON = 3
Global Const $RT_MENU = 4
Global Const $RT_DIALOG = 5
Global Const $RT_STRING = 6
Global Const $RT_FONTDIR = 7
Global Const $RT_FONT = 8
Global Const $RT_ACCELERATOR = 9
Global Const $RT_RCDATA = 10
Global Const $RT_MESSAGETABLE = 11
Global Const $RT_GROUP_CURSOR = 12
Global Const $RT_GROUP_ICON = 14
Global Const $RT_VERSION = 16
Global Const $RT_DLGINCLUDE = 17
Global Const $RT_PLUGPLAY = 19
Global Const $RT_VXD = 20
Global Const $RT_ANICURSOR = 21
Global Const $RT_ANIICON = 22
Global Const $RT_HTML = 23
Global Const $RT_MANIFEST = 24

Global Const $SND_RESOURCE = 0x00040004
Global Const $SND_SYNC = 0x0
Global Const $SND_ASYNC = 0x1
Global Const $SND_MEMORY = 0x4
Global Const $SND_LOOP = 0x8
Global Const $SND_NOSTOP = 0x10
Global Const $SND_NOWAIT = 0x2000
Global Const $SND_PURGE = 0x40

Func _ResourceGet($ResName, $ResType = 10, $ResLang = 0, $DLL = -1) ; $RT_RCDATA = 10
	Local Const $IMAGE_BITMAP = 0
	Local $hInstance, $hBitmap, $InfoBlock, $GlobalMemoryBlock, $MemoryPointer, $ResSize
	
	If $DLL = -1 Then
	  $hInstance = _WinAPI_GetModuleHandle("")
	Else
	  $hInstance = _WinAPI_LoadLibrary($DLL)
	EndIf
	If $hInstance = 0 Then Return SetError(1, 0, 0)

	If $ResType = $RT_BITMAP Then
		$hBitmap = _WinAPI_LoadImage($hInstance, $ResName, $IMAGE_BITMAP, 0, 0, 0)
		If @error Then Return SetError(2, 0, 0)
		Return $hBitmap ; returns handle to Bitmap
	EndIf

	If $ResLang <> 0 Then
		$InfoBlock = DllCall("kernel32.dll", "int", "FindResourceExA", "int", $hInstance, "long", $ResType, "str", $ResName, "short", $ResLang)
	Else
		$InfoBlock = DllCall("kernel32.dll", "int", "FindResourceA", "int", $hInstance, "str", $ResName, "long", $ResType)
	EndIf
	
	If @error Then Return SetError(3, 0, 0)
	$InfoBlock = $InfoBlock[0]
	If $InfoBlock = 0 Then Return SetError(4, 0, 0)
	
	$ResSize = DllCall("kernel32.dll", "dword", "SizeofResource", "int", $hInstance, "int", $InfoBlock)
	If @error Then Return SetError(5, 0, 0)
	$ResSize = $ResSize[0]
	If $ResSize = 0 Then Return SetError(6, 0, 0)
	
	$GlobalMemoryBlock = DllCall("kernel32.dll", "int", "LoadResource", "int", $hInstance, "int", $InfoBlock)
	If @error Then Return SetError(7, 0, 0)
	$GlobalMemoryBlock = $GlobalMemoryBlock[0]
	If $GlobalMemoryBlock = 0 Then Return SetError(8, 0, 0)
	
	$MemoryPointer = DllCall("kernel32.dll", "int", "LockResource", "int", $GlobalMemoryBlock)
	If @error Then Return SetError(9, 0, 0)
	$MemoryPointer = $MemoryPointer[0]
	If $MemoryPointer = 0 Then Return SetError(10, 0, 0)
	
	If $DLL <> -1 Then _WinAPI_FreeLibrary($hInstance)
	If @error Then Return SetError(11, 0, 0)
    
	SetExtended($ResSize)
	Return $MemoryPointer
EndFunc

; for ANSI strings
Func _ResourceGetAsString($ResName, $ResType = 10, $ResLang = 0, $DLL = -1) ; $RT_RCDATA = 10
	Local $ResPointer, $ResSize, $struct

	$ResPointer = _ResourceGet($ResName, $ResType, $ResLang, $DLL)
	If @error Then
		SetError(1, 0, 0)
		Return ''
	EndIf
	$ResSize = @extended
	$struct = DllStructCreate("char[" & $ResSize & "]", $ResPointer)
	Return DllStructGetData($struct, 1) ; returns string
EndFunc

; for Unicode strings (Widechar)
Func _ResourceGetAsStringW($ResName, $ResType = 10, $ResLang = 0, $DLL = -1) ; $RT_RCDATA = 10
    Local $ResPointer, $ResSize, $struct

    $ResPointer = _ResourceGet($ResName, $ResType, $ResLang, $DLL)
    If @error Then
        SetError(1, 0, 0)
        Return ''
    EndIf
    $ResSize = @extended
    $struct = DllStructCreate("wchar[" & $ResSize & "]", $ResPointer)
    Return DllStructGetData($struct, 1) ; returns string
EndFunc

; _ResourceGetAsBytes() doesn't work for RT_BITMAP type
; because _ResourceGet() returns hBitmap instead of memory pointer in this case
Func _ResourceGetAsBytes($ResName, $ResType = 10, $ResLang = 0, $DLL = -1) ; $RT_RCDATA = 10
	Local $ResPointer, $ResSize

	$ResPointer = _ResourceGet($ResName, $ResType, $ResLang, $DLL)
	If @error Then Return SetError(1, 0, 0)
	$ResSize = @extended
	Return DllStructCreate("byte[" & $ResSize & "]", $ResPointer) ; returns struct with bytes
EndFunc

; returned hImage can be used in many GDI+ functions:
; $width =  _GDIPlus_ImageGetWidth ($hImage)
; $height = _GDIPlus_ImageGetHeight($hImage)
Func _ResourceGetAsImage($ResName, $ResType = 10, $DLL = -1) ; $RT_RCDATA = 10
	Local $ResData, $nSize, $hData, $pData, $pStream, $pBitmap, $hBitmap

	$ResData = _ResourceGet($ResName, $ResType, 0, $DLL)
	If @error Then Return SetError(1, 0, 0)
	$nSize = @extended
	
	If $ResType = $RT_BITMAP Then
 		; $ResData is hBitmap type
		$hImage = _GDIPlus_BitmapCreateFromHBITMAP($ResData)
	Else
 		; $ResData is memory pointer
		; thanks ProgAndy
		$hData = _MemGlobalAlloc($nSize,2)
		$pData = _MemGlobalLock($hData)
		_MemMoveMemory($ResData,$pData,$nSize)
		_MemGlobalUnlock($hData)
		$pStream = DllCall( "ole32.dll","int","CreateStreamOnHGlobal", "int",$hData, "long",1, "Int*",0)
		$pStream = $pStream[3]
		$hImage = DllCall($ghGDIPDll,"int","GdipCreateBitmapFromStream", "ptr",$pStream, "int*",0)
		$hImage = $hImage[2]
		_WinAPI_DeleteObject($pStream)
		; next line must be commented otherwise animated GIFs will not work
;~ 		_MemGlobalFree($hData)
	EndIf

	Return $hImage ; hImage type
EndFunc

Func _ResourceGetAsBitmap($ResName, $ResType = 10, $DLL = -1) ; $RT_RCDATA = 10
	$hImage = _ResourceGetAsImage($ResName, $ResType, $DLL)
	If @error Then Return SetError(1, 0, 0)
	$hBitmap = _GDIPlus_BitmapCreateHBITMAPFromBitmap($hImage)
	Return $hBitmap ; hBitmap type
EndFunc

Func _ResourceSaveToFile($FileName, $ResName, $ResType = 10, $ResLang = 0, $CreatePath = 0, $DLL = -1) ; $RT_RCDATA = 10
	Local $ResStruct, $ResSize, $FileHandle
	
	If $CreatePath Then $CreatePath = 8 ; mode 8 = Create directory structure if it doesn't exist in FileOpen()

	If $ResType = $RT_BITMAP Then
		; workaround: for RT_BITMAP _ResourceGetAsBytes() doesn't work so use _ResourceGetAsImage()
		$hImage = _ResourceGetAsImage($ResName, $ResType)
		If @error Then Return SetError(10, 0, 0)
		
		; create filepath if doesn't exist
		$FileHandle = FileOpen($FileName, 2+16+$CreatePath)
		If @error Then Return SetError(11, 0, 0)
		FileClose($FileHandle)
		If @error Then Return SetError(12, 0, 0)
		
		_GDIPlus_ImageSaveToFile($hImage, $FileName)
		_GDIPlus_ImageDispose($hImage)

		$ResSize = FileGetSize($FileName)
	Else
		; standard way
		$ResStruct = _ResourceGetAsBytes($ResName, $ResType, $ResLang, $DLL)
		If @error Then Return SetError(1, 0, 0)
		$ResSize = DllStructGetSize($ResStruct)
		
		$FileHandle = FileOpen($FileName, 2+16+$CreatePath)
		If @error Then Return SetError(2, 0, 0)
		FileWrite($FileHandle, DllStructGetData($ResStruct, 1))
		If @error Then Return SetError(3, 0, 0)
		FileClose($FileHandle)
		If @error Then Return SetError(4, 0, 0)
	EndIf

	Return $ResSize
EndFunc

Func _ResourceSetImageToCtrl($CtrlId, $ResName, $ResType = 10, $DLL = -1) ; $RT_RCDATA = 10
	Local $ResData, $nSize, $hData, $pData, $pStream, $pBitmap, $hBitmap
	
	$ResData = _ResourceGet($ResName, $ResType, 0, $DLL)
	If @error Then Return SetError(1, 0, 0)
	$nSize = @extended
	
	If $ResType = $RT_BITMAP Then
		_SetBitmapToCtrl($CtrlId, $ResData)
		If @error Then Return SetError(2, 0, 0)
	Else
		; thanks ProgAndy
		; for other types than BITMAP use GDI+ for converting to bitmap first
		$hData = _MemGlobalAlloc($nSize,2)
		$pData = _MemGlobalLock($hData)
		_MemMoveMemory($ResData,$pData,$nSize)
		_MemGlobalUnlock($hData)
		$pStream = DllCall( "ole32.dll","int","CreateStreamOnHGlobal", "int",$hData, "long",1, "Int*",0)
		$pStream = $pStream[3]
		$pBitmap = DllCall($ghGDIPDll,"int","GdipCreateBitmapFromStream", "ptr",$pStream, "int*",0)
		$pBitmap = $pBitmap[2]
		$hBitmap = _GDIPlus_BitmapCreateHBITMAPFromBitmap($pBitmap)
		_SetBitmapToCtrl($CtrlId, $hBitmap)
		If @error Then SetError(3, 0, 0)
		_GDIPlus_BitmapDispose($pBitmap)
		_WinAPI_DeleteObject($pStream)
		_MemGlobalFree($hData)
	EndIf

	Return 1
EndFunc

; internal helper function
Func _SetBitmapToCtrl($CtrlId, $hBitmap)
    Local Const $STM_SETIMAGE = 0x0172
    Local Const $IMAGE_BITMAP = 0
	Local Const $SS_BITMAP = 0xE
	Local Const $GWL_STYLE = -16

    Local $hWnd = GUICtrlGetHandle($CtrlId)
	If $hWnd = 0 Then Return SetError(1, 0, 0)
	
	; set SS_BITMAP style to control
	Local $oldStyle = _WinAPI_GetWindowLong($hWnd, $GWL_STYLE)
	If @error Then Return SetError(2, 0, 0)
	_WinAPI_SetWindowLong($hWnd, $GWL_STYLE, BitOR($oldStyle, $SS_BITMAP))
	If @error Then Return SetError(3, 0, 0)
	
	Local $oldBmp = _SendMessage($hWnd, $STM_SETIMAGE, $IMAGE_BITMAP, $hBitmap)
	If @error Then Return SetError(4, 0, 0)
    If $oldBmp <> 0 Then _WinAPI_DeleteObject($oldBmp)
	Return 1
EndFunc

; thanks Larry,ProgAndy
; MSDN: http://msdn2.microsoft.com/en-us/library/ms712879.aspx
; default flag is $SND_SYNC = 0
Func _ResourcePlaySound($ResName, $Flag = 0, $DLL = -1)
	If $DLL = -1 Then
	  $hInstance = 0
	Else
	  $hInstance = _WinAPI_LoadLibrary($DLL)
	EndIf
  
	Local $ret = DllCall("winmm.dll", "int", "PlaySound", "str", $ResName, "hwnd", $hInstance, "int", BitOr($SND_RESOURCE,$Flag))
	If @error Then Return SetError(1, 0, 0)

	If $DLL <> -1 Then _WinAPI_FreeLibrary($hInstance)
	If @error Then Return SetError(2, 0, 0)
  
	Return $ret[0]
EndFunc

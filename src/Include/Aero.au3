$Struct = DllStructCreate("int cxLeftWidth;int cxRightWidth;int cyTopHeight;int cyBottomHeight;")
$sStruct = DllStructCreate("dword;int;ptr;int")

; _DwmEnableComposition
Global Const $DWM_EC_DISABLECOMPOSITION = 0
Global Const $DWM_EC_ENABLECOMPOSITION = 1

; _DwmEnableBlurBehindWindow
Global Const $DWM_BB_ENABLE = 0x00000001
Global Const $DWM_BB_BLURREGION = 0x00000002
Global Const $DWM_BB_TRANSITIONONMAXIMIZED = 0x00000004

Func _DwmIsCompositionEnabled()
     $Struct = DllStructCreate("int;")
     $Ret = DllCall("dwmapi.dll", "int", "DwmIsCompositionEnabled", "ptr", DllStructGetPtr($Struct))
     If @error Then
         Return 0
     Else
         Return DllStructGetData($Struct, 1)
     EndIf
EndFunc
 
Func _DwmEnableComposition($toggle = 1)
    If $toggle = 1 Then
        DllCall("dwmapi.dll", "hwnd", "DwmEnableComposition", "uint", $DWM_EC_DISABLECOMPOSITION)
    Else
        DllCall("dwmapi.dll", "hwnd", "DwmEnableComposition", "uint", $DWM_EC_ENABLECOMPOSITION)
    EndIf
EndFunc

Func _DwmEnableBlurBehindWindow($hWnd)
	$Struct = DllStructCreate("dword;int;ptr;int")
	DllStructSetData($Struct, 1, BitOr($DWM_BB_ENABLE, $DWM_BB_TRANSITIONONMAXIMIZED))
	DllStructSetData($Struct, 2, 1)
	DllStructSetData($Struct, 4, 1)
	DllCall("dwmapi.dll", "int", "DwmEnableBlurBehindWindow", "hwnd", $hWnd, "ptr", DllStructGetPtr($Struct))
EndFunc

Func _DwmExtendFrameIntoClientArea($hWnd)
	$Ret = DllCall("dwmapi.dll", "long", "DwmExtendFrameIntoClientArea", "hwnd", $hWnd, "long*", DllStructGetPtr($Struct))
	If @error Then
		Return 0
	Else
		Return $Ret
	EndIf
EndFunc

Func _DwmAPI_SwitchAeroOnWnd($hWnd, $Mode = 1)
    Local $struct = DllStructCreate("int")

    if ($Mode) Then
        DllStructSetData($struct, 1, 1)
    Else
        DllStructSetData($struct, 1, 0)
    EndIf

    Local $dllReturn = DllCall("dwmapi.dll", "int", "DwmSetWindowAttribute", "hwnd", $hWnd, "dword", 2, "ptr", DllStructGetPtr($struct), "dword", DllStructGetSize($struct))
    If (@error = 0 And $dllReturn[0] = 0) Then
        Return 1
    Else
        Return 0
    EndIf
EndFunc
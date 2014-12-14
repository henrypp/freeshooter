#Region Header

#cs

    Title:          WinAPI Extended UDF Library for AutoIt3
    Filename:       WinAPIEx.au3
    Description:    Additional variables, constants and functions for the WinAPI.au3
    Author:         Yashied
    Version:        1.9
    Requirements:   AutoIt v3.3 +, Developed/Tested on WindowsXP Pro Service Pack 2
    Uses:           StructureConstants.au3, WinAPI.au3
    Notes:          -

    Available functions:

    _WinAPI_AboutDlg
    _WinAPI_AddFontResource
    _WinAPI_AlphaBlend
    _WinAPI_AnimateWindow
    _WinAPI_ArrayToStruct
    _WinAPI_AssocQueryString
    _WinAPI_BeginPaint
    _WinAPI_BringWindowToTop
    _WinAPI_CharToOem
    _WinAPI_ChildWindowFromPointEx
    _WinAPI_CloseWindow
    _WinAPI_CopyFileEx
    _WinAPI_CopyImage
    _WinAPI_CopyRect
    _WinAPI_CreateBrushIndirect
    _WinAPI_CreateCompatibleBitmapEx
    _WinAPI_CreateDIBSection
    _WinAPI_CreateEllipticRgn
    _WinAPI_CreateFileEx
    _WinAPI_CreateIconIndirect
    _WinAPI_CreatePolygonRgn
    _WinAPI_CreateRect
    _WinAPI_CreateRectEx
    _WinAPI_CreateRectRgnIndirect
    _WinAPI_CreateSemaphore
    _WinAPI_DefineDosDevice
    _WinAPI_DeleteVolumeMountPoint
    _WinAPI_DeregisterShellHookWindow
    _WinAPI_DllInstall
    _WinAPI_DllUninstall
    _WinAPI_DragAcceptFiles
    _WinAPI_DragFinish
    _WinAPI_DragQueryFileEx
    _WinAPI_DragQueryPoint
    _WinAPI_DrawAnimatedRects
    _WinAPI_DrawBitmap
    _WinAPI_DrawShadowText
    _WinAPI_DuplicateBitmap
    _WinAPI_DuplicateCursor
    _WinAPI_DuplicateIcon
    _WinAPI_EjectMedia
    _WinAPI_Ellipse
    _WinAPI_EmptyWorkingSet
    _WinAPI_EndPaint
    _WinAPI_EnumChildWindows
    _WinAPI_EnumDisplaySettings
    _WinAPI_EnumProcessThreads
    _WinAPI_EnumProcessWindows
    _WinAPI_EnumResourceLanguages
    _WinAPI_EnumResourceNames
    _WinAPI_EnumResourceTypes
    _WinAPI_EqualRect
    _WinAPI_EqualRgn
    _WinAPI_ExtractAssociatedIcon
    _WinAPI_ExtSelectClipRgn
    _WinAPI_FatalExit
    _WinAPI_FileTimeToLocalFileTime
    _WinAPI_FileTimeToSystemTime
   *_WinAPI_FillRect
    _WinAPI_FillRgn
    _WinAPI_FillStruct
    _WinAPI_FindResource
    _WinAPI_FindResourceEx
    _WinAPI_FitToBitmap
    _WinAPI_FormatDriveDlg
   *_WinAPI_FrameRect
    _WinAPI_FrameRgn
    _WinAPI_FreeCursor
    _WinAPI_FreeHandle
    _WinAPI_FreeIcon
    _WinAPI_FreeObject
    _WinAPI_FreeResource
    _WinAPI_GetActiveWindow
    _WinAPI_GetBitmapDimension
    _WinAPI_GetBkColor
    _WinAPI_GetBValue
    _WinAPI_GetClassLong
    _WinAPI_GetCompression
    _WinAPI_GetCurrentDirectory
    _WinAPI_GetCursor
    _WinAPI_GetDefaultPrinter
    _WinAPI_GetDiskFreeSpaceEx
    _WinAPI_GetDriveBusType
    _WinAPI_GetDriveGeometryEx
    _WinAPI_GetDriveNumber
    _WinAPI_GetDriveType
    _WinAPI_GetFontResourceInfo
   *_WinAPI_GetForegroundWindow
    _WinAPI_GetGValue
    _WinAPI_GetHandleInformation
    _WinAPI_GetIconBitmap
    _WinAPI_GetIconDimension
    _WinAPI_GetIconMask
    _WinAPI_GetIdleTime
    _WinAPI_GetKeyboardLayout
    _WinAPI_GetKeyboardLayoutList
    _WinAPI_GetKeyboardState
    _WinAPI_GetKeyNameText
    _WinAPI_GetKeyState
   *_WinAPI_GetLayeredWindowAttributes
    _WinAPI_GetLocaleInfo
    _WinAPI_GetModuleFileNameEx
    _WinAPI_GetObjectEx
    _WinAPI_GetObjectType
    _WinAPI_GetPixel
    _WinAPI_GetProcAddress
    _WinAPI_GetProcessCreationTime
    _WinAPI_GetProcessMemoryInfo
    _WinAPI_GetRgnBox
    _WinAPI_GetROP2
    _WinAPI_GetRValue
    _WinAPI_GetSystemPowerStatus
    _WinAPI_GetTempFileName
    _WinAPI_GetTextColor
    _WinAPI_GetTextMetrics
    _WinAPI_GetThemeAppProperties
    _WinAPI_GetThemeColor
    _WinAPI_GetTickCount
    _WinAPI_GetTopWindow
    _WinAPI_GetUserDefaultLCID
    _WinAPI_GetVersionEx
    _WinAPI_GetVolumeNameForVolumeMountPoint
    _WinAPI_GetWindowInfo
    _WinAPI_GetWindowModuleFileName
    _WinAPI_GetWorkArea
    _WinAPI_GradientFill
    _WinAPI_InflateRect
    _WinAPI_IntersectRect
   *_WinAPI_InvalidateRect
    _WinAPI_InvalidateRgn
    _WinAPI_InvertRect
    _WinAPI_InvertRgn
    _WinAPI_IsChild
    _WinAPI_IsDoorOpen
    _WinAPI_IsHungAppWindow
    _WinAPI_IsIconic
    _WinAPI_IsNetworkAlive
    _WinAPI_IsPressed
    _WinAPI_IsRectEmpty
    _WinAPI_IsThemeActive
    _WinAPI_IsWindowEnabled
    _WinAPI_IsWindowUnicode
    _WinAPI_IsWritable
    _WinAPI_IsZoomed
    _WinAPI_Keybd_Event
    _WinAPI_KillTimer
    _WinAPI_LoadCursor
    _WinAPI_LoadCursorFromFile
    _WinAPI_LoadMedia
    _WinAPI_LoadResource
    _WinAPI_LockDevice
    _WinAPI_LockResource
    _WinAPI_LockWorkStation
    _WinAPI_MoveFileEx
    _WinAPI_OemToChar
    _WinAPI_OffsetClipRgn
    _WinAPI_OffsetRect
    _WinAPI_OffsetRgn
    _WinAPI_OpenIcon
    _WinAPI_OpenSemaphore
    _WinAPI_PaintRgn
    _WinAPI_PathCompactPath
    _WinAPI_PathFindOnPath
    _WinAPI_PathIsDirectory
    _WinAPI_PathIsDirectoryEmpty
    _WinAPI_PathMatchSpec
    _WinAPI_PathSearchAndQualify
    _WinAPI_PathYetAnotherMakeUniqueName
    _WinAPI_PickIconDlg
    _WinAPI_Polygon
  **_WinAPI_PrivateExtractIcon
    _WinAPI_PtInRectEx
    _WinAPI_PtInRegion
    _WinAPI_QueryDosDevice
    _WinAPI_Rectangle
    _WinAPI_RegisterShellHookWindow
    _WinAPI_ReleaseSemaphore
    _WinAPI_RemoveFontResource
    _WinAPI_RestartDlg
    _WinAPI_RestoreDC
    _WinAPI_RGB
    _WinAPI_RoundRect
    _WinAPI_SaveDC
    _WinAPI_SetActiveWindow
    _WinAPI_SetClassLong
    _WinAPI_SetCompression
    _WinAPI_SetCurrentDirectory
   *_WinAPI_SetDefaultPrinter
    _WinAPI_SetDCBrushColor
    _WinAPI_SetDCPenColor
    _WinAPI_SetFilePointerEx
    _WinAPI_SetForegroundWindow
   *_WinAPI_SetHandleInformation
    _WinAPI_SetKeyboardLayout
    _WinAPI_SetKeyboardState
   *_WinAPI_SetLayeredWindowAttributes
    _WinAPI_SetLibraryColorMode
   *_WinAPI_SetParent
    _WinAPI_SetPixel
    _WinAPI_SetROP2
    _WinAPI_SetStretchBltMode
    _WinAPI_SetSystemCursor
    _WinAPI_SetThemeAppProperties
    _WinAPI_SetTimer
    _WinAPI_SetVolumeMountPoint
    _WinAPI_ShareFolderDlg
    _WinAPI_ShellExtractIcons
    _WinAPI_ShellGetFileInfo
    _WinAPI_ShellGetSpecialFolderPath
    _WinAPI_ShellNotifyIcon
    _WinAPI_ShowLastError
    _WinAPI_ShowOwnedPopups
    _WinAPI_SizeofResource
    _WinAPI_StretchBlt
    _WinAPI_StructToArray
    _WinAPI_SubtractRect
    _WinAPI_SwitchColor
  **_WinAPI_SwitchToThisWindow
    _WinAPI_TransparentBlt
    _WinAPI_TextOut
    _WinAPI_UnionRect
    _WinAPI_ValidateRect
    _WinAPI_ValidateRgn
    _WinAPI_WindowFromDC
    _WinAPI_WinHelp

   * Available in native AutoIt library
  ** Deprecated

#ce

#Include-once

#Include <StructureConstants.au3>
#Include <WinAPI.au3>

#EndRegion Header

#Region Global Variables and Constants

; ===============================================================================================================================
; _WinAPI_GetClassLong(), _WinAPI_SetClassLong()
; ===============================================================================================================================

Global Const $GCL_CBCLSEXTRA = -20
Global Const $GCL_CBWNDEXTRA = -18
Global Const $GCL_HBRBACKGROUND = -10
Global Const $GCL_HCURSOR = -12
Global Const $GCL_HICON = -14
Global Const $GCL_HICONSM = -34
Global Const $GCL_HMODULE = -16
Global Const $GCL_MENUNAME = -8
Global Const $GCL_STYLE = -26
Global Const $GCL_WNDPROC = -24

#EndRegion Global Variables and Constants

#Region Local Variables and Constants

Global $__Data, $__RGB = True

#EndRegion Local Variables and Constants

#Region Public Functions

; #FUNCTION# ====================================================================================================================
; Name...........: _WinAPI_FreeHandle
; Description....: Closes an open object handle.
; Syntax.........: _WinAPI_FreeHandle ( $hObject )
; Parameters.....: $hObject - Handle to an open object.
; Return values..: Success  - 1.
;                  Failure  - 0 and sets the @error flag to non-zero.
; Author.........: Yashied
; Modified.......:
; Remarks........: The _WinAPI_FreeHandle() function closes handles to the following objects:
;
;                  Access token
;                  Communications device
;                  Console input
;                  Console screen buffer
;                  Event
;                  File
;                  File mapping
;                  I/O completion port
;                  Job
;                  Mailslot
;                  Memory resource notification
;                  Mutex
;                  Named pipe
;                  Pipe
;                  Process
;                  Semaphore
;                  Thread
;                  Transaction
;                  Waitable timer
;
; Related........:
; Link...........: @@MsdnLink@@ CloseHandle
; Example........: Yes
; ===============================================================================================================================

Func _WinAPI_FreeHandle($hObject)

	Local $Ret = DllCall('kernel32.dll', 'int', 'CloseHandle', 'ptr', $hObject)

	If (@error) Or ($Ret[0] = 0) Then
		Return SetError(1, 0, 0)
	EndIf
	Return 1
EndFunc   ;==>_WinAPI_FreeHandle

; #FUNCTION# ====================================================================================================================
; Name...........: _WinAPI_FreeIcon
; Description....: Destroys an icon and frees any memory the icon occupied.
; Syntax.........: _WinAPI_FreeIcon ( $hIcon )
; Parameters.....: $hIcon  - Handle to the icon to be destroyed.
; Return values..: Success - 1.
;                  Failure - 0 and sets the @error flag to non-zero.
; Author.........: Yashied
; Modified.......:
; Remarks........: Do not use this function to destroy a shared icon. A shared icon is valid as long as the module from which it
;                  was loaded remains in memory.
; Related........:
; Link...........: @@MsdnLink@@ DestroyIcon
; Example........: Yes
; ===============================================================================================================================

Func _WinAPI_FreeIcon($hIcon)

	Local $Ret = DllCall('user32.dll', 'int', 'DestroyIcon', 'ptr', $hIcon)

	If (@error) Or ($Ret[0] = 0) Then
		Return SetError(1, 0, 0)
	EndIf
	Return 1
EndFunc   ;==>_WinAPI_FreeIcon

; #FUNCTION# ====================================================================================================================
; Name...........: _WinAPI_GetClassLong
; Description....: Retrieves the specified 32-bit (long) value associated with the specified window.
; Syntax.........: _WinAPI_GetClassLong ( $hWnd, $iIndex )
; Parameters.....: $hWnd   - Handle to the window and, indirectly, the class to which the window belongs.
;                  $iIndex - The 32-bit value to retrieve. This parameter can be one of the following values.
;
;                            $GCL_CBCLSEXTRA
;                            $GCL_CBWNDEXTRA
;                            $GCL_HBRBACKGROUND
;                            $GCL_HCURSOR
;                            $GCL_HICON
;                            $GCL_HICONSM
;                            $GCL_HMODULE
;                            $GCL_MENUNAME
;                            $GCL_STYLE
;                            $GCL_WNDPROC
;
; Return values..: Success - The value is the requested 32-bit value.
;                  Failure - 0 and sets the @error flag to non-zero.
; Author.........: Yashied
; Modified.......:
; Remarks........: None
; Related........:
; Link...........: @@MsdnLink@@ GetClassLong
; Example........: Yes
; ===============================================================================================================================

Func _WinAPI_GetClassLong($hWnd, $iIndex)

	Local $Ret = DllCall('user32.dll', 'int', 'GetClassLong', 'hwnd', $hWnd, 'int', $iIndex)

	If (@error) Or ($Ret[0] = 0) Then
		Return SetError(1, 0, 0)
	EndIf
	Return $Ret[0]
EndFunc   ;==>_WinAPI_GetClassLong

; #FUNCTION# ====================================================================================================================
; Name...........: _WinAPI_SetClassLong
; Description....: Replaces the specified 32-bit (long) value into the specified window belongs.
; Syntax.........: _WinAPI_SetClassLong ( $hWnd, $iIndex, $dwNewLong )
; Parameters.....: $hWnd     - Handle to the window and, indirectly, the class to which the window belongs.
;                  $iIndex   - The 32-bit value to replace. This parameter can be one of the following values.
;
;                              $GCL_CBCLSEXTRA
;                              $GCL_CBWNDEXTRA
;                              $GCL_HBRBACKGROUND
;                              $GCL_HCURSOR
;                              $GCL_HICON
;                              $GCL_HICONSM
;                              $GCL_HMODULE
;                              $GCL_MENUNAME
;                              $GCL_STYLE
;                              $GCL_WNDPROC
;
;                  $iNewLong - The replacement value.
; Return values..: Success   - The previous value of the specified 32-bit integer. If the value was not previously set, the return value is zero.
;                  Failure   - 0 and sets the @error flag to non-zero.
; Author.........: Yashied
; Modified.......:
; Remarks........: None
; Related........:
; Link...........: @@MsdnLink@@ SetClassLong
; Example........: Yes
; ===============================================================================================================================

Func _WinAPI_SetClassLong($hWnd, $iIndex, $iNewLong)

	Local $Ret = DllCall('user32.dll', 'int', 'SetClassLong', 'hwnd', $hWnd, 'int', $iIndex, 'long', $iNewLong)

	If (@error) Or ($Ret[0] = 0) Then
		Return SetError(1, 0, 0)
	EndIf
	Return $Ret[0]
EndFunc   ;==>_WinAPI_SetClassLong

; #FUNCTION# ====================================================================================================================
; Name...........: _WinAPI_StretchBlt
; Description....: Copies a bitmap from a source rectangle into a destination rectangle, stretching or compressing the bitmap
;                  to fit the dimensions of the destination rectangle, if necessary.
; Syntax.........: _WinAPI_StretchBlt ( $hDestDC, $iXDest, $iYDest, $iWidthDest, $iHeightDest, $hSrcDC, $iXSrc, $iYSrc, $iWidthSrc, $iHeightSrc, $iRop )
; Parameters.....: $hDestDC     - Handle to the destination device context.
;                  $iXDest      - The x-coordinate, in logical units, of the upper-left corner of the destination rectangle.
;                  $iYDest      - The y-coordinate, in logical units, of the upper-left corner of the destination rectangle.
;                  $iWidthDest  - The width, in logical units, of the destination rectangle.
;                  $iHeightDest - The height, in logical units, of the destination rectangle.
;                  $hSrcDC      - Handle to the source device context.
;                  $iXSrc       - The x-coordinate, in logical units, of the upper-left corner of the source rectangle.
;                  $iYSrc       - The y-coordinate, in logical units, of the upper-left corner of the source rectangle.
;                  $iWidthSrc   - The width, in logical units, of the source rectangle.
;                  $iHeightSrc  - The height, in logical units, of the source rectangle.
;                  $iRop        - The raster-operation code. These codes define how the color data for the source rectangle is
;                                 to be combined with the color data for the destination rectangle to achieve the final color.
;                                 This parameter must be 0 or one of the following values.
;
;                                 $BLACKNESS
;                                 $CAPTUREBLT
;                                 $DSTINVERT
;                                 $MERGECOPY
;                                 $MERGEPAINT
;                                 $NOMIRRORBITMAP
;                                 $NOTSRCCOPY
;                                 $NOTSRCERASE
;                                 $PATCOPY
;                                 $PATINVERT
;                                 $PATPAINT
;                                 $SRCAND
;                                 $SRCCOPY
;                                 $SRCERASE
;                                 $SRCINVERT
;                                 $SRCPAINT
;                                 $WHITENESS
;
; Return values..: Success      - 1.
;                  Failure      - 0 and sets the @error flag to non-zero.
; Author.........: Yashied
; Modified.......:
; Remarks........: The system stretches or compresses the bitmap according to the stretching mode currently set in the
;                  destination device context.
; Related........:
; Link...........: @@MsdnLink@@ StretchBlt
; Example........: Yes
; ===============================================================================================================================

Func _WinAPI_StretchBlt($hDestDC, $iXDest, $iYDest, $iWidthDest, $iHeightDest, $hSrcDC, $iXSrc, $iYSrc, $iWidthSrc, $iHeightSrc, $iRop)

	Local $Ret  = DllCall('gdi32.dll', 'int', 'StretchBlt', 'hwnd', $hDestDC, 'int', $iXDest, 'int', $iYDest, 'int', $iWidthDest, 'int', $iHeightDest, 'hwnd', $hSrcDC, 'int', $iXSrc, 'int', $iYSrc, 'int', $iWidthSrc, 'int', $iHeightSrc, 'dword', $iRop)

	If (@error) Or ($Ret[0] = 0) Then
		Return SetError(1, 0, 0)
	EndIf
	Return 1
EndFunc   ;==>_WinAPI_StretchBlt
#include-once

$WM_CAP_START = 0x400
$WM_CAP_UNICODE_START = $WM_CAP_START +100
$WM_CAP_PAL_SAVEA = $WM_CAP_START + 81
$WM_CAP_PAL_SAVEW = $WM_CAP_UNICODE_START + 81
$WM_CAP_UNICODE_END = $WM_CAP_PAL_SAVEW
$WM_CAP_ABORT = $WM_CAP_START + 69
$WM_CAP_DLG_VIDEOCOMPRESSION = $WM_CAP_START + 46
$WM_CAP_DLG_VIDEODISPLAY = $WM_CAP_START + 43
$WM_CAP_DLG_VIDEOFORMAT = $WM_CAP_START + 41
$WM_CAP_DLG_VIDEOSOURCE = $WM_CAP_START + 42
$WM_CAP_DRIVER_CONNECT = $WM_CAP_START + 10
$WM_CAP_DRIVER_DISCONNECT = $WM_CAP_START + 11
$WM_CAP_DRIVER_GET_CAPS = $WM_CAP_START + 14
$WM_CAP_DRIVER_GET_NAMEA = $WM_CAP_START + 12
$WM_CAP_DRIVER_GET_NAMEW = $WM_CAP_UNICODE_START + 12
$WM_CAP_DRIVER_GET_VERSIONA = $WM_CAP_START + 13
$WM_CAP_DRIVER_GET_VERSIONW = $WM_CAP_UNICODE_START + 13
$WM_CAP_EDIT_COPY = $WM_CAP_START + 30
$WM_CAP_END = $WM_CAP_UNICODE_END
$WM_CAP_FILE_ALLOCATE = $WM_CAP_START + 22
$WM_CAP_FILE_GET_CAPTURE_FILEA = $WM_CAP_START + 21
$WM_CAP_FILE_GET_CAPTURE_FILEW = $WM_CAP_UNICODE_START + 21
$WM_CAP_FILE_SAVEASA = $WM_CAP_START + 23
$WM_CAP_FILE_SAVEASW = $WM_CAP_UNICODE_START + 23
$WM_CAP_FILE_SAVEDIBA = $WM_CAP_START + 25
$WM_CAP_FILE_SAVEDIBW = $WM_CAP_UNICODE_START + 25
$WM_CAP_FILE_SET_CAPTURE_FILEA = $WM_CAP_START + 20
$WM_CAP_FILE_SET_CAPTURE_FILEW = $WM_CAP_UNICODE_START + 20
$WM_CAP_FILE_SET_INFOCHUNK = $WM_CAP_START + 24
$WM_CAP_GET_AUDIOFORMAT = $WM_CAP_START + 36
$WM_CAP_GET_CAPSTREAMPTR = $WM_CAP_START + 1
$WM_CAP_GET_MCI_DEVICEA = $WM_CAP_START + 67
$WM_CAP_GET_MCI_DEVICEW = $WM_CAP_UNICODE_START + 67
$WM_CAP_GET_SEQUENCE_SETUP = $WM_CAP_START + 65
$WM_CAP_GET_STATUS = $WM_CAP_START + 54
$WM_CAP_GET_USER_DATA = $WM_CAP_START + 8
$WM_CAP_GET_VIDEOFORMAT = $WM_CAP_START + 44
$WM_CAP_GRAB_FRAME = $WM_CAP_START + 60
$WM_CAP_GRAB_FRAME_NOSTOP = $WM_CAP_START + 61
$WM_CAP_PAL_AUTOCREATE = $WM_CAP_START + 83
$WM_CAP_PAL_MANUALCREATE = $WM_CAP_START + 84
$WM_CAP_PAL_OPENA = $WM_CAP_START + 80
$WM_CAP_PAL_OPENW = $WM_CAP_UNICODE_START + 80
$WM_CAP_PAL_PASTE = $WM_CAP_START + 82
$WM_CAP_SEQUENCE = $WM_CAP_START + 62
$WM_CAP_SEQUENCE_NOFILE = $WM_CAP_START + 63
$WM_CAP_SET_AUDIOFORMAT = $WM_CAP_START + 35
$WM_CAP_SET_CALLBACK_CAPCONTROL = $WM_CAP_START + 85
$WM_CAP_SET_CALLBACK_ERRORA = $WM_CAP_START + 2
$WM_CAP_SET_CALLBACK_ERRORW = $WM_CAP_UNICODE_START + 2
$WM_CAP_SET_CALLBACK_FRAME = $WM_CAP_START + 5
$WM_CAP_SET_CALLBACK_STATUSA = $WM_CAP_START + 3
$WM_CAP_SET_CALLBACK_STATUSW = $WM_CAP_UNICODE_START + 3
$WM_CAP_SET_CALLBACK_VIDEOSTREAM = $WM_CAP_START + 6
$WM_CAP_SET_CALLBACK_WAVESTREAM = $WM_CAP_START + 7
$WM_CAP_SET_CALLBACK_YIELD = $WM_CAP_START + 4
$WM_CAP_SET_MCI_DEVICEA = $WM_CAP_START + 66
$WM_CAP_SET_MCI_DEVICEW = $WM_CAP_UNICODE_START + 66
$WM_CAP_SET_OVERLAY = $WM_CAP_START + 51
$WM_CAP_SET_PREVIEW = $WM_CAP_START + 50
$WM_CAP_SET_PREVIEWRATE = $WM_CAP_START + 52
$WM_CAP_SET_SCALE = $WM_CAP_START + 53
$WM_CAP_SET_SCROLL = $WM_CAP_START + 55
$WM_CAP_SET_SEQUENCE_SETUP = $WM_CAP_START + 64
$WM_CAP_SET_USER_DATA = $WM_CAP_START + 9
$WM_CAP_SET_VIDEOFORMAT = $WM_CAP_START + 45
$WM_CAP_SINGLE_FRAME = $WM_CAP_START + 72
$WM_CAP_SINGLE_FRAME_CLOSE = $WM_CAP_START + 71
$WM_CAP_SINGLE_FRAME_OPEN = $WM_CAP_START + 70
$WM_CAP_STOP = $WM_CAP_START + 68
#include <GUIConstants.au3>
$avi = DllOpen("avicap32.dll")
$user = DllOpen("user32.dll")

;===============================================================================
;
; Description:      Open's a webcam preview screen in your gui
; Syntax:           _WebcamOpen($sHwnd, $sLeft, $sTop, $sWidth, $sHeight)
; Parameter(s):     $sHwnd     - The handle of the gui
;                   $sLeft     - Left coord. of the preview screen
;                   $sTop      - Top coord. of the preview screen
;                   $sWidth    - Width of the preview screen
;                   $sHeight   - Height of the preview screen
; Requirement(s):   A webcam
; Return Value(s):  On Success - Returns id needed for other controls
;                   On Failure - Returns -1
; Author(s):        Ludocus
; Note(s):          None
;
;===============================================================================
func _WebcamOpen($sHwnd, $sLeft, $sTop, $sWidth, $sHeight)
$cap = DllCall($avi, "int", "capCreateCaptureWindow", "str", "cap", "int", BitOR(1073741824, 268435456), "int", $sLeft, "int", $sTop, "int", $sWidth, "int", $sHeight, "hwnd", $sHwnd, "int", 1)
DllCall($user, "int", "SendMessage", "hWnd", $cap[0], "int", $WM_CAP_DRIVER_CONNECT, "int", 0, "int", 0)
DllCall($user, "int", "SendMessage", "hWnd", $cap[0], "int", $WM_CAP_SET_SCALE, "int", 1, "int", 0)
DllCall($user, "int", "SendMessage", "hWnd", $cap[0], "int", $WM_CAP_SET_OVERLAY, "int", 1, "int", 0)
DllCall($user, "int", "SendMessage", "hWnd", $cap[0], "int", $WM_CAP_SET_PREVIEW, "int", 1, "int", 0)
DllCall($user, "int", "SendMessage", "hWnd", $cap[0], "int", $WM_CAP_SET_PREVIEWRATE, "int", 30, "int", 0)
if @error then return -1
return $cap[0]
EndFunc

;===============================================================================
;
; Description:      Creates a Snapshot from a webcam
; Syntax:           _WebcamSnap($sId, $sFile)
; Parameter(s):     $sId       - Id (returned from _WebcamOpen)
;                   $sFile     - File to save the snapshot to (*.bmp)
; Requirement(s):   A webcam
; Return Value(s):  On Success - Returns 1
;                   On Failure - Returns 0
; Author(s):        Ludocus
; Note(s):          None
;
;===============================================================================
Func _WebcamSnap($sId, $sFile)
    DllCall($user, "int", "SendMessage", "hWnd", $sId, "int", $WM_CAP_GRAB_FRAME_NOSTOP, "int", 0, "int", 0)
    DllCall($user, "int", "SendMessage", "hWnd", $sId, "int", $WM_CAP_FILE_SAVEDIBA, "int", 0, "str", $sFile)
    if @error Then
        return 0
    Else
        return 1
    EndIf
EndFunc

;===============================================================================
;
; Description:      Closes the preview screen created with _WebcamOpen
; Syntax:           _WebcamClose($sId)
; Parameter(s):     $sId       - Id (returned from _WebcamOpen)
; Requirement(s):   A webcam
; Return Value(s):  On Success - Returns 1
;                   On Failure - Returns 0
; Author(s):        Ludocus
; Note(s):          None
;
;===============================================================================
Func _WebcamClose($sId)
    DllCall($user, "int", "SendMessage", "hWnd", $sId, "int", $WM_CAP_END, "int", 0, "int", 0)
    DllCall($user, "int", "SendMessage", "hWnd", $sId, "int", $WM_CAP_DRIVER_DISCONNECT, "int", 0, "int", 0)
    DllClose($user)
    if @error Then
        return 0
    Else
        return 1
    EndIf
EndFunc

;===============================================================================
;
; Description:      Starts recording the webcam to a file
; Syntax:           _WebcamRecordStart($sFile, $sId)
; Parameter(s):     $sId       - Id (returned from _WebcamOpen)
;                   $sFile     - File to save the movie to (*.avi)
; Requirement(s):   A webcam
; Return Value(s):  On Success - Returns 1
;                   On Failure - Returns 0
; Author(s):        Ludocus
; Note(s):          Stop recording by: _WebcamRecordStop($Id)
;
;===============================================================================
Func _WebcamRecordStart($sFile, $sId)
    DllCall($user, "int", "SendMessage", "hWnd", $sId, "int", $WM_CAP_FILE_SET_CAPTURE_FILEA, "int", 0, "str", $sFile)
    DllCall($user, "int", "SendMessage", "hWnd", $sId, "int", $WM_CAP_SEQUENCE, "int", 0, "int", 0)
    if @error Then
        return 0
    Else
        return 1
    EndIf
    
EndFunc

;===============================================================================
;
; Description:      Stops recording.
; Syntax:           _WebcamRecordStop($sId)
; Parameter(s):     $sId       - Id (returned from _WebcamOpen)
; Requirement(s):   A webcam
; Return Value(s):  On Success - Returns 1
;                   On Failure - Returns 0
; Author(s):        Ludocus
; Note(s):          None
;
;===============================================================================
Func _WebcamRecordStop($sId)
    DllCall($user, "int", "SendMessage", "hWnd", $sId, "int", $WM_CAP_STOP, "int", 0, "int", 0)
    if @error Then
        return 0
    Else
        return 1
    EndIf
EndFunc ;==> _WebcamRecordStop
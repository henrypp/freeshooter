#include-once

; #INDEX# =======================================================================================================================
; Title .........: File
; AutoIt Version: 33.0
; Language:       English
; Description ...: Functions that assist with files and directories.
; ===============================================================================================================================

; #FUNCTION# ====================================================================================================================
; Name...........: _FileListToArray
; Description ...: Lists files and\or folders in a specified path (Similar to using Dir with the /B Switch)
; Syntax.........: _FileListToArray($sPath[, $sFilter = "*"[, $iFlag = 0]])
; Parameters ....: $sPath   - Path to generate filelist for.
;                  $sFilter - Optional the filter to use, default is *. Search the Autoit3 helpfile for the word "WildCards" For details.
;                  $iFlag   - Optional: specifies whether to return files folders or both
;                  |$iFlag=0(Default) Return both files and folders
;                  |$iFlag=1 Return files only
;                  |$iFlag=2 Return Folders only
; Return values .: @Error - 1 = Path not found or invalid
;                  |2 = Invalid $sFilter
;                  |3 = Invalid $iFlag
;                  |4 = No File(s) Found
; Author ........: SolidSnake <MetalGX91 at GMail dot com>
; Modified.......:
; Remarks .......: The array returned is one-dimensional and is made up as follows:
;                                $array[0] = Number of Files\Folders returned
;                                $array[1] = 1st File\Folder
;                                $array[2] = 2nd File\Folder
;                                $array[3] = 3rd File\Folder
;                                $array[n] = nth File\Folder
; Related .......:
; Link ..........;
; Example .......; Yes
; ===============================================================================================================================
;Special Thanks to Helge and Layer for help with the $iFlag update
; speed optimization by code65536
;===============================================================================
Func _FileListToArray($sPath, $sFilter = "*", $iFlag = 0)
	Local $hSearch, $sFile, $asFileList[1]
	If Not FileExists($sPath) Then Return SetError(1, 1, "")
	If (StringInStr($sFilter, "\")) Or (StringInStr($sFilter, "/")) Or (StringInStr($sFilter, ":")) Or (StringInStr($sFilter, ">")) Or (StringInStr($sFilter, "<")) Or (StringInStr($sFilter, "|")) Or (StringStripWS($sFilter, 8) = "") Then Return SetError(2, 2, "")
	If Not ($iFlag = 0 Or $iFlag = 1 Or $iFlag = 2) Then Return SetError(3, 3, "")
	If (StringMid($sPath, StringLen($sPath), 1) = "\") Then $sPath = StringTrimRight($sPath, 1) ; needed for Win98 for x:\  root dir
	$hSearch = FileFindFirstFile($sPath & "\" & $sFilter)
	If $hSearch = -1 Then Return SetError(4, 4, "")
	While 1
		$sFile = FileFindNextFile($hSearch)
		If @error Then
			SetError(0)
			ExitLoop
		EndIf
		If $iFlag = 1 And StringInStr(FileGetAttrib($sPath & "\" & $sFile), "D") <> 0 Then ContinueLoop
		If $iFlag = 2 And StringInStr(FileGetAttrib($sPath & "\" & $sFile), "D") = 0 Then ContinueLoop
		$asFileList[0] += 1
		If UBound($asFileList) <= $asFileList[0] Then ReDim $asFileList[UBound($asFileList) * 2]
		$asFileList[$asFileList[0]] = $sFile
	WEnd
	FileClose($hSearch)
	ReDim $asFileList[$asFileList[0] + 1] ; Trim unused slots
	Return $asFileList
EndFunc   ;==>_FileListToArray
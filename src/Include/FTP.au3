;===============================================================================
;
; Function Name:    _FTPOpen()
; Description:      Opens an FTP session.
; Parameter(s):     $s_Agent      	- Random name. ( like "myftp" )
;                   $l_AccessType 	- I dont got a clue what this does.
;                   $s_ProxyName  	- ProxyName.
;                   $s_ProxyBypass	- ProxyByPasses's.
;                   $l_Flags       	- Special flags.
; Requirement(s):   DllCall, wininet.dll
; Return Value(s):  On Success - Returns an indentifier.
;                   On Failure - 0  and sets @ERROR
; Author(s):        Wouter van Kesteren.
;
;===============================================================================

Func _FTPOpen($s_Agent, $l_AccessType = 1, $s_ProxyName = '', $s_ProxyBypass = '', $l_Flags = 0)
	
	Local $ai_InternetOpen = DllCall('wininet.dll', 'long', 'InternetOpen', 'str', $s_Agent, 'long', $l_AccessType, 'str', $s_ProxyName, 'str', $s_ProxyBypass, 'long', $l_Flags)
	If @error OR $ai_InternetOpen[0] = 0 Then
		SetError(-1)
		Return 0
	EndIf
		
	Return $ai_InternetOpen[0]
	
EndFunc ;==> _FTPOpen()

;===============================================================================
;
; Function Name:    _FTPConnect()
; Description:      Connects to an FTP server.
; Parameter(s):     $l_InternetSession	- The Long from _FTPOpen()
;                   $s_ServerName 		- Server name/ip.
;                   $s_Username  		- Username.
;                   $s_Password			- Password.
;                   $i_ServerPort  		- Server port ( 0 is default (21) )
;					$l_Service			- I dont got a clue what this does.
;					$l_Flags			- Special flags.
;					$l_Context			- I dont got a clue what this does.
; Requirement(s):   DllCall, wininet.dll
; Return Value(s):  On Success - Returns an indentifier.
;                   On Failure - 0  and sets @ERROR
; Author(s):        Wouter van Kesteren
;
;===============================================================================

Func _FTPConnect($l_InternetSession, $s_ServerName, $s_Username, $s_Password, $i_ServerPort = 0, $l_Service = 1, $l_Flags = 0, $l_Context = 0)
	
	Local $ai_InternetConnect = DllCall('wininet.dll', 'long', 'InternetConnect', 'long', $l_InternetSession, 'str', $s_ServerName, 'int', $i_ServerPort, 'str', $s_Username, 'str', $s_Password, 'long', $l_Service, 'long', $l_Flags, 'long', $l_Context)
	If @error OR $ai_InternetConnect[0] = 0 Then
		SetError(-1)
		Return 0
	EndIf
			
	Return $ai_InternetConnect[0]
	
EndFunc ;==> _FTPConnect()

;===============================================================================
;
; Function Name:    _FTPPutFile()
; Description:      Puts an file on an FTP server.
; Parameter(s):     $l_FTPSession	- The Long from _FTPConnect()
;                   $s_LocalFile 	- The local file.
;                   $s_RemoteFile  	- The remote Location for the file.
;                   $l_Flags		- Special flags.
;                   $l_Context  	- I dont got a clue what this does.
; Requirement(s):   DllCall, wininet.dll
; Return Value(s):  On Success - 1
;                   On Failure - 0
; Author(s):        Wouter van Kesteren
;
;===============================================================================

Func _FTPPutFile($l_FTPSession, $s_LocalFile, $s_RemoteFile, $l_Flags = 0, $l_Context = 0)

	Local $ai_FTPPutFile = DllCall('wininet.dll', 'int', 'FtpPutFile', 'long', $l_FTPSession, 'str', $s_LocalFile, 'str', $s_RemoteFile, 'long', $l_Flags, 'long', $l_Context)
	If @error OR $ai_FTPPutFile[0] = 0 Then
		SetError(-1)
		Return 0
	EndIf
	
	Return $ai_FTPPutFile[0]
	
EndFunc ;==> _FTPPutFile()

;===============================================================================
;
; Function Name:    _FTPDelFile()
; Description:      Delete an file from an FTP server.
; Parameter(s):     $l_FTPSession	- The Long from _FTPConnect()
;                   $s_RemoteFile  	- The remote Location for the file.
; Requirement(s):   DllCall, wininet.dll
; Return Value(s):  On Success - 1
;                   On Failure - 0
; Author(s):        Wouter van Kesteren
;
;===============================================================================

Func _FTPDelFile($l_FTPSession, $s_RemoteFile)
	
	Local $ai_FTPPutFile = DllCall('wininet.dll', 'int', 'FtpDeleteFile', 'long', $l_FTPSession, 'str', $s_RemoteFile)
	If @error OR $ai_FTPPutFile[0] = 0 Then
		SetError(-1)
		Return 0
	EndIf
	
	Return $ai_FTPPutFile[0]
	
EndFunc ;==> _FTPDelFile()

;===============================================================================
;
; Function Name:    _FTPRenameFile()
; Description:      Renames an file on an FTP server.
; Parameter(s):     $l_FTPSession	- The Long from _FTPConnect()
;                   $s_Existing 	- The old file name.
;                   $s_New  		- The new file name.
; Requirement(s):   DllCall, wininet.dll
; Return Value(s):  On Success - 1
;                   On Failure - 0
; Author(s):        Wouter van Kesteren
;
;===============================================================================

Func _FTPRenameFile($l_FTPSession, $s_Existing, $s_New)
	
	Local $ai_FTPRenameFile = DllCall('wininet.dll', 'int', 'FtpRenameFile', 'long', $l_FTPSession, 'str', $s_Existing, 'str', $s_New)
	If @error OR $ai_FTPRenameFile[0] = 0 Then
		SetError(-1)
		Return 0
	EndIf
	
	Return $ai_FTPRenameFile[0]
	
EndFunc ;==> _FTPRenameFile()

;===============================================================================
;
; Function Name:    _FTPMakeDir()
; Description:      Makes an Directory on an FTP server.
; Parameter(s):     $l_FTPSession	- The Long from _FTPConnect()
;                   $s_Remote 		- The file name to be deleted.
; Requirement(s):   DllCall, wininet.dll
; Return Value(s):  On Success - 1
;                   On Failure - 0
; Author(s):        Wouter van Kesteren
;
;===============================================================================

Func _FTPMakeDir($l_FTPSession, $s_Remote)
	
	Local $ai_FTPMakeDir = DllCall('wininet.dll', 'int', 'FtpCreateDirectory', 'long', $l_FTPSession, 'str', $s_Remote)
	If @error OR $ai_FTPMakeDir[0] = 0 Then
		SetError(-1)
		Return 0
	EndIf
	
	Return $ai_FTPMakeDir[0]
	
EndFunc ;==> _FTPMakeDir()

;===============================================================================
;
; Function Name:    _FTPDelDir()
; Description:      Delete's an Directory on an FTP server.
; Parameter(s):     $l_FTPSession	- The Long from _FTPConnect()
;                   $s_Remote 		- The Directory to be deleted.
; Requirement(s):   DllCall, wininet.dll
; Return Value(s):  On Success - 1
;                   On Failure - 0
; Author(s):        Wouter van Kesteren
;
;===============================================================================

Func _FTPDelDir($l_FTPSession, $s_Remote)
	
	Local $ai_FTPDelDir = DllCall('wininet.dll', 'int', 'FtpRemoveDirectory', 'long', $l_FTPSession, 'str', $s_Remote)
	If @error OR $ai_FTPDelDir[0] = 0 Then
		SetError(-1)
		Return 0
	EndIf
		
	Return $ai_FTPDelDir[0]
	
EndFunc ;==> _FTPDelDir()

;===============================================================================
;
; Function Name:    _FTPClose()
; Description:      Closes the _FTPOpen session.
; Parameter(s):     $l_InternetSession	- The Long from _FTPOpen()
; Requirement(s):   DllCall, wininet.dll
; Return Value(s):  On Success - 1
;                   On Failure - 0
; Author(s):        Wouter van Kesteren
;
;===============================================================================

Func _FTPClose($l_InternetSession)
	
	Local $ai_InternetCloseHandle = DllCall('wininet.dll', 'int', 'InternetCloseHandle', 'long', $l_InternetSession)
	If @error OR $ai_InternetCloseHandle[0] = 0 Then
		SetError(-1)
		Return 0
	EndIf
	
	Return $ai_InternetCloseHandle[0]
	
EndFunc ;==> _FTPClose()
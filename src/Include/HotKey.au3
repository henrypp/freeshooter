Func _GetHotKeySetValue($iCtrlKey_Mode, $iAltKey_Mode, $iShiftKey_Mode, $sExtraKey)
	Local $sRet_HotKey = ""
	
	If $iCtrlKey_Mode = 1 Then $sRet_HotKey &= "^"
	If $iAltKey_Mode = 1 Then $sRet_HotKey &= "!"
	If $iShiftKey_Mode= 1 Then $sRet_HotKey &= "+"
	
	If StringStripWS($sExtraKey, 8) <> "" Then
		If StringLen($sExtraKey) > 1 Then Return $sRet_HotKey & "{" & $sExtraKey & "}"
		$sRet_HotKey &= $sExtraKey
	EndIf
	
	Return $sRet_HotKey
EndFunc

Func _GetHotKeyTrueValue($sHotKey)
	Local $sRet_HotKey = $sHotKey
	
	$sRet_HotKey = StringReplace($sRet_HotKey, "+", "+SHIFT+")
	$sRet_HotKey = StringReplace($sRet_HotKey, "^", "+CTRL+")
	$sRet_HotKey = StringReplace($sRet_HotKey, "!", "+ALT+")
	$sRet_HotKey = StringRegExpReplace($sRet_HotKey, "[\}\{]", "")
	
	$sRet_HotKey = StringStripWS($sRet_HotKey, 8)
	$sRet_HotKey = _StringStripChars($sRet_HotKey, "+", 3)
	$sRet_HotKey = StringRegExpReplace($sRet_HotKey, "\++", " + ")
	
	Return StringStripWS($sRet_HotKey, 3)
EndFunc

Func _GetHotKeysList()
	Local $sRetList = " |"
	
	For $i = 1 To 12
		$sRetList &= "F" & $i & "|"
	Next
	
	For $i = 0 To 9
		$sRetList &= $i & "|"
	Next
	
	For $i = 97 To 122
		$sRetList &= StringUpper(Chr($i)) & "|"
	Next
	
	$sRetList &= "PrintScreen"
	
	Return _StringStripChars($sRetList, "|", 3)
EndFunc

Func _StringStripChars($String, $SubString, $Flag=0, $Count=0)
	If StringLen($String) = 0 Then Return SetError(1, 0, $String)
	
	Local $GroupChar_a = '(', $GroupChar_b = ')'
	If $Count < 0 Then Local $GroupChar_a = '[', $GroupChar_b = ']'
	
	$SubString = StringRegExpReplace($SubString, '([][{}()|.?+*\\^])', '\\\1')
	Local $Pattern = '(?i)' & $GroupChar_a & $SubString & $GroupChar_b, $sCnt = '{1,'& $Count &'}'
	
	If $Count <= 0 Then $sCnt = '+'
	If $Flag <> 0 Then $Count = 0
	If $Flag = 1 Then $Pattern = '(?i)^' & $GroupChar_a & $SubString & $GroupChar_b & $sCnt
	If $Flag = 2 Then $Pattern = '(?i)' & $GroupChar_a & $SubString & $GroupChar_b & $sCnt & '$'
	If $Flag = 3 Then $Pattern = '(?i)^' & $GroupChar_a & $SubString & $GroupChar_b & $sCnt & '|' & _
		$GroupChar_a & $SubString & $GroupChar_b & $sCnt & '$'
	Return StringRegExpReplace($String, $Pattern, '', $Count)
EndFunc
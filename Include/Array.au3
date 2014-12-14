#include-Once

; #INDEX# =======================================================================================================================
; Title .........: Array
; AutoIt Version : 3.2.10++
; Language ......: English
; Description ...: This module contains various functions for manipulating arrays.
; ===============================================================================================================================

; #FUNCTION# ====================================================================================================================
; Name...........: _ArrayReverse
; Description ...: Takes the given array and reverses the order in which the elements appear in the array.
; Syntax.........: _ArrayReverse(ByRef $avArray[, $iStart = 0[, $iEnd = 0]])
; Parameters ....: $avArray - Array to modify
;                  $iStart  - [optional] Index of array to start modifying at
;                  $iEnd    - [optional] Index of array to stop modifying at
; Return values .: Success - 1
;                  Failure - 0, sets @error:
;                  |1 - $avArray is not an array
;                  |2 - $iStart is greater than $iEnd
;                  |3 - $avArray is not a 1 dimensional array
; Author ........: Brian Keene
; Modified.......: Jos van der Zande <jdeb at autoitscript dot com> -  added $iStart parameter and logic, Tylo - added $iEnd parameter and rewrote it for speed, Ultima - code cleanup, minor optimization
; Remarks .......:
; Related .......: _ArraySwap
; Link ..........;
; Example .......; Yes
; ===============================================================================================================================
Func _ArrayReverse(ByRef $avArray, $iStart = 0, $iEnd = 0)
	If Not IsArray($avArray) Then Return SetError(1, 0, 0)
	If UBound($avArray, 0) <> 1 Then Return SetError(3, 0, 0)

	Local $vTmp, $iUBound = UBound($avArray) - 1

	; Bounds checking
	If $iEnd < 1 Or $iEnd > $iUBound Then $iEnd = $iUBound
	If $iStart < 0 Then $iStart = 0
	If $iStart > $iEnd Then Return SetError(2, 0, 0)

	; Reverse
	For $i = $iStart To Int(($iStart + $iEnd - 1) / 2)
		$vTmp = $avArray[$i]
		$avArray[$i] = $avArray[$iEnd]
		$avArray[$iEnd] = $vTmp
		$iEnd -= 1
	Next

	Return 1
EndFunc   ;==>_ArrayReverse

; #FUNCTION# ====================================================================================================================
; Name...........: _ArraySort
; Description ...: Sort a 1D or 2D array on a specific index using the quicksort/insertionsort algorithms.
; Syntax.........: _ArraySort(ByRef $avArray[, $iDescending = 0[, $iStart = 0[, $iEnd = 0[, $iSubItem = 0]]]])
; Parameters ....: $avArray     - Array to sort
;                  $iDescending - [optional] If set to 1, sort descendingly
;                  $iStart      - [optional] Index of array to start sorting at
;                  $iEnd        - [optional] Index of array to stop sorting at
;                  $iSubItem    - [optional] Sub-index to sort on in 2D arrays
; Return values .: Success - 1
;                  Failure - 0, sets @error:
;                  |1 - $avArray is not an array
;                  |2 - $iStart is greater than $iEnd
;                  |3 - $iSubItem is greater than subitem count
;                  |4 - $avArray has too many dimensions
; Author ........: Jos van der Zande <jdeb at autoitscript dot com>
; Modified.......: LazyCoder - added $iSubItem option, Tylo - implemented stable QuickSort algo, Jos van der Zande - changed logic to correctly Sort arrays with mixed Values and Strings, Ultima - major optimization, code cleanup, removed $i_Dim parameter
; Remarks .......:
; Related .......:
; Link ..........;
; Example .......; Yes
; ===============================================================================================================================
Func _ArraySort(ByRef $avArray, $iDescending = 0, $iStart = 0, $iEnd = 0, $iSubItem = 0)
	If Not IsArray($avArray) Then Return SetError(1, 0, 0)

	Local $iUBound = UBound($avArray) - 1

	; Bounds checking
	If $iEnd < 1 Or $iEnd > $iUBound Then $iEnd = $iUBound
	If $iStart < 0 Then $iStart = 0
	If $iStart > $iEnd Then Return SetError(2, 0, 0)

	; Sort
	Switch UBound($avArray, 0)
		Case 1
			__ArrayQuickSort1D($avArray, $iStart, $iEnd)
			If $iDescending Then _ArrayReverse($avArray, $iStart, $iEnd)
		Case 2
			Local $iSubMax = UBound($avArray, 2) - 1
			If $iSubItem > $iSubMax Then Return SetError(3, 0, 0)

			If $iDescending Then
				$iDescending = -1
			Else
				$iDescending = 1
			EndIf

			__ArrayQuickSort2D($avArray, $iDescending, $iStart, $iEnd, $iSubItem, $iSubMax)
		Case Else
			Return SetError(4, 0, 0)
	EndSwitch

	Return 1
EndFunc   ;==>_ArraySort

; #INTERNAL_USE_ONLY#============================================================================================================
; Name...........: __ArrayQuickSort1D
; Description ...: Helper function for sorting 1D arrays
; Syntax.........: __ArrayQuickSort1D(ByRef $avArray, ByRef $iStart, ByRef $iEnd)
; Parameters ....: $avArray - Array to sort
;                  $iStart  - Index of array to start sorting at
;                  $iEnd    - Index of array to stop sorting at
; Return values .: None
; Author ........: Jos van der Zande, LazyCoder, Tylo, Ultima
; Modified.......:
; Remarks .......: For Internal Use Only
; Related .......:
; Link ..........;
; Example .......;
; ===============================================================================================================================
Func __ArrayQuickSort1D(ByRef $avArray, ByRef $iStart, ByRef $iEnd)
	If $iEnd <= $iStart Then Return

	Local $vTmp

	; InsertionSort (faster for smaller segments)
	If ($iEnd - $iStart) < 15 Then
		Local $i, $j, $vCur
		For $i = $iStart + 1 To $iEnd
			$vTmp = $avArray[$i]

			If IsNumber($vTmp) Then
				For $j = $i - 1 To $iStart Step -1
					$vCur = $avArray[$j]
					; If $vTmp >= $vCur Then ExitLoop
					If ($vTmp >= $vCur And IsNumber($vCur)) Or (Not IsNumber($vCur) And StringCompare($vTmp, $vCur) >= 0) Then ExitLoop
					$avArray[$j + 1] = $vCur
				Next
			Else
				For $j = $i - 1 To $iStart Step -1
					If (StringCompare($vTmp, $avArray[$j]) >= 0) Then ExitLoop
					$avArray[$j + 1] = $avArray[$j]
				Next
			EndIf

			$avArray[$j + 1] = $vTmp
		Next
		Return
	EndIf

	; QuickSort
	Local $L = $iStart, $R = $iEnd, $vPivot = $avArray[Int(($iStart + $iEnd) / 2)], $fNum = IsNumber($vPivot)
	Do
		If $fNum Then
			; While $avArray[$L] < $vPivot
			While ($avArray[$L] < $vPivot And IsNumber($avArray[$L])) Or (Not IsNumber($avArray[$L]) And StringCompare($avArray[$L], $vPivot) < 0)
				$L += 1
			WEnd
			; While $avArray[$R] > $vPivot
			While ($avArray[$R] > $vPivot And IsNumber($avArray[$R])) Or (Not IsNumber($avArray[$R]) And StringCompare($avArray[$R], $vPivot) > 0)
				$R -= 1
			WEnd
		Else
			While (StringCompare($avArray[$L], $vPivot) < 0)
				$L += 1
			WEnd
			While (StringCompare($avArray[$R], $vPivot) > 0)
				$R -= 1
			WEnd
		EndIf

		; Swap
		If $L <= $R Then
			$vTmp = $avArray[$L]
			$avArray[$L] = $avArray[$R]
			$avArray[$R] = $vTmp
			$L += 1
			$R -= 1
		EndIf
	Until $L > $R

	__ArrayQuickSort1D($avArray, $iStart, $R)
	__ArrayQuickSort1D($avArray, $L, $iEnd)
EndFunc   ;==>__ArrayQuickSort1D
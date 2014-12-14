Global enum _
	$RotateNoneFlipNone = 0 , _
	$Rotate90FlipNone = 1 , _
	$Rotate180FlipNone = 2 , _
	$Rotate270FlipNone = 3 , _
	$RotateNoneFlipX = 4 , _
	$Rotate90FlipX = 5 , _
	$Rotate180FlipX = 6 , _
	$Rotate270FlipX = 7 , _
	$RotateNoneFlipY = 6 , _
	$Rotate90FlipY = 7 , _
	$Rotate180FlipY = 4 , _
	$Rotate270FlipY = 5 , _
	$RotateNoneFlipXY = 6 , _
	$Rotate90FlipXY = 7 , _
	$Rotate180FlipXY = 0 , _
	$Rotate270FlipXY = 1

Func _RotateImage($ImgFile, $hSaveTo,  $hMode)
	_GDIPlus_Startup ()
	$hImage = _GDIPlus_ImageLoadFromFile ($ImgFile)
	
	DllCall($ghGDIPDll,"int","GdipImageRotateFlip","hwnd",$hImage,"long", $hMode)
	_GDIPlus_ImageSaveToFile ($hImage, $hSaveTo)
	_WinAPI_DeleteObject ($hImage)
	_GDIPlus_ShutDown ()
EndFunc
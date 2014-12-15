#cs
	Name: Free Shooter
	Version: 1.9.6
	Author: [Nuker-Hoax]
	HomePage: http://freeshooter.sourceforge.net
#ce

If $cmdline[0] = 0 Then
	If _Mutex("FREE_SHOOTER") = 1 Then 
		MsgBox(64, "Ошибка", "Программа уже запущена")
		Exit
	EndIf
Else
	If StringInStr($cmdline[1], "/fromrestart") Then _Mutex("FREE_SHOOTER")
EndIf

#include <Constants.au3>
#include <GUIConstantsEx.au3>
#include <include\Aero.au3>
#include <include\Array.au3>
#include <include\Clipboard.au3>
#include <include\File.au3>
#include <include\FTP.au3>
#include <include\HotKey.au3>
#include <include\ImageResize.au3>
#include <include\Resources.au3>
#include <include\RotateImage.au3>
#include <include\ScreenCapture.au3>
#include <include\Webcam.au3>
#include <include\WinAPIEx.au3>

#include <WinAPI.au3>

Opt("TrayOnEventMode", 1)
Opt("TrayMenuMode", 1)

;Global Application Constants
Global $application = "Free Shooter"
Global $version = "1.9.6"
Global $homepage = "http://freeshooter.sourceforge.net"
Global $settings_file = @ScriptDir &"\"& StringTrimRight(@ScriptName, 4) &".ini"
Global $manifest = IniRead($settings_file, "freeshooter", "UseManifest", "0")

Global $imageupload_dll = @ScriptDir &"\UploadFile.dll"

;Global Language Settings
Global $lang_dir = @ScriptDir &"\Lang"
Global $language = IniRead($settings_file, "freeshooter", "Language", "Русский")
Global $default_lang = $lang_dir &"\"& $language &".lng"
Global $translator = IniRead($default_lang, "Language", "Translator", "")
	
;Global GUI Constants
Global $WM_HSCROLL = 0x0114
Global $ES_PASSWORD = 0x0020
Global $WS_POPUP = -2147483648
Global $WS_EX_TOPMOST = 8
Global $WS_EX_TOOLWINDOW = 128
Global $WS_BORDER = 8388608
Global $SRCCOPY = 13369376
Global $WS_EX_ACCEPTFILES = 16

Global $pos_x_1, $pos_y_1, $pos_x_2, $pos_y_2, $aPos
Global $main_dlg, $wc_dlg, $opt_dlg, $hHwnd, $hTab, $sFile

If $cmdline[0] Then
	If not StringInStr($cmdline[1], "/fromrestart") Then
		If StringInStr($cmdline[1], "/h") or StringInStr($cmdline[1], "/help") or StringInStr($cmdline[1], "-h") or StringInStr($cmdline[1], "-help") or StringInStr($cmdline[1], "-?") or StringInStr($cmdline[1], "/?") Then 
			MsgBox(64, T("CmdHelpMessageTitle", "Параметры коммандной строки"), T("CmdHelpMessageText", "Параметры запуска программы:" &@CRLF&@CRLF& "/fullscreen - снимок всего экрана" &@CRLF& "/window - снимок выбранного окна" &@CRLF& "/region - снимок выделенной области"))
		EndIf
		If StringInStr($cmdline[1], "/fullscreen") Then _ScreenShoot_FullScreen()
		If StringInStr($cmdline[1], "/window") Then _ScreenShoot_SelectedWindow()
		If StringInStr($cmdline[1], "/region") Then _ScreenShoot_Region()
		Exit
	EndIf
EndIf

TraySetState(1)
TraySetClick(16)
TraySetToolTip($application &" "& $version)
TraySetIcon(@ScriptFullPath, -1)

TraySetOnEvent($TRAY_EVENT_PRIMARYUP, "maximize_gui")

Global $main_dlg = GUICreate($application &" "& $version, 469, 250)
GUISetIcon(@ScriptFullPath, -1, $main_dlg)

If $manifest = 0 Then 
	DllCall("uxtheme.dll", "none", "SetThemeAppProperties", "int", 1)
Else
	DllCall("uxtheme.dll", "none", "SetThemeAppProperties", "int", 3)
EndIf

$file_menu = GUICtrlCreateMenu(T("FileMenu", "Файл"))
$doit_item = GUICtrlCreateMenuItem(T("DoItItem", "Сделать скриншот"), $file_menu)
$screen_folder_open_item = GUICtrlCreateMenuItem(T("ScreenFolderOpenItem", "Открыть папку скриншотов"), $file_menu)
$exit_item = GUICtrlCreateMenuItem(T("ExitItem", "Выйти"), $file_menu)

$tools_menu = GUICtrlCreateMenu(T("ToolsMenu", "Инструменты"))
$wc_shoot_item = GUICtrlCreateMenuItem(T("WcShootItem", "Захват с веб-камеры"), $tools_menu)
$is_uploading_item = GUICtrlCreateMenuItem(T("IsUploadingItem", "Загрузка скриншотов на ImageShack"), $tools_menu)
$aero_switch_menu = GUICtrlCreateMenu(T("AeroSwitchMenu", "Режим Aero"), $tools_menu)
$aero_enable_item = GUICtrlCreateMenuItem(T("AeroSwitchEnable", "Включить"), $aero_switch_menu)
$aero_disable_item = GUICtrlCreateMenuItem(T("AeroSwitchDisable", "Отключить"), $aero_switch_menu)

If FileExists($imageupload_dll) = 0 Then GUICtrlSetState($is_uploading_item, $GUI_DISABLE)
GUICtrlSetState($aero_switch_menu, $GUI_DISABLE)
If @OSVersion = "WIN_VISTA" or @OSVersion = "WIN_7" Then GUICtrlSetState($aero_switch_menu, $GUI_ENABLE)
	
$settings_menu = GUICtrlCreateMenu(T("SettingsMenu", "Настройки"))
$hotkey_item = GUICtrlCreateMenuItem(T("HotKeyItem", "Назначить горячие клавиши"), $settings_menu)
$settings_item = GUICtrlCreateMenuItem(T("SettingsItem", "Настройки..."), $settings_menu)

$help_menu = GUICtrlCreateMenu(T("HelpMenu", "Помощь"))
$website_item = GUICtrlCreateMenuItem(T("WebSiteItem", "Сайт программы"), $help_menu)
$about_item = GUICtrlCreateMenuItem(T("AboutItem", "О программе"), $help_menu)

GUICtrlCreateGroup(T("FolderToSaveImages", "Папка для сохранения скриншотов"), 10, 10, 449, 65)
$path_to_save = GUICtrlCreateInput("", 24, 32, 385, 21)
$browse_btn = GUICtrlCreateButton("...", 416, 30, 27, 25, Default, 131072)

GUICtrlCreateGroup(T("SettingsGroup", "Настройки программы"), 10, 80, 209, 101)
$collapse_at_start_chk = GUICtrlCreateCheckbox(T("CollapseAtStart", "Запускать свёрнутым"), 24, 104, 180, 17)
$use_sounds_chk = GUICtrlCreateCheckbox(T("UseSounds", "Использовать звуки"), 24, 128, 180, 17)
$include_cursor_chk = GUICtrlCreateCheckbox(T("IncludeCursor", "Захватывать курсор"), 24, 152, 180, 17)

GUICtrlCreateGroup(T("ShootMode", "Режим сьёмки"), 226, 80, 233, 101)
$all_screen_combo = GUICtrlCreateRadio(T("AllScreenMode", "Весь экран"), 240, 104, 121, 17)
$selected_window_combo = GUICtrlCreateRadio(T("SelectedWindowMode", "Выбранное окно (под курсором)"), 240, 128, 200, 17)
$region_shoot_combo = GUICtrlCreateRadio(T("RegionShootMode", "Выделенная область"), 240, 152, 200, 17)

$settings_btn = GUICtrlCreateButton(T("SettingsButton", "Настройки"), 10, 195, 75, 25, Default, 131072)
$doit_btn = GUICtrlCreateButton(T("DoItButton", "Скриншот"), 226, 195, 74, 25, Default, 131072) 
$hotkey_btn = GUICtrlCreateButton(T("HotKeyButton", "Клавиши"), 306, 195, 74, 25, Default, 131072)
$exit_btn = GUICtrlCreateButton(T("ExitButton", "Выйти"), 385, 195, 74, 25, Default, 131072)

$snap_full_item_tray = TrayCreateItem(T("AllScreenMode", "Весь экран"))
$snap_window_item_tray = TrayCreateItem(T("SelectedWindowMode", "Выбранное окно (под курсором)"))
$snap_region_item_tray = TrayCreateItem(T("RegionShootMode", "Выделенная область"))
TrayCreateItem("")
$settings_item_tray = TrayCreateItem(T("SettingsItem", "Настройки..."))
TrayCreateItem("")
$is_uploading_item_tray = TrayCreateItem(T("IsUploadingItem", "Загрузка скриншотов на ImageShack"))
$wc_item_tray = TrayCreateItem(T("WcShootItem", "Захват с веб-камеры"))
TrayCreateItem("")
$exit_item_tray = TrayCreateItem(T("ExitItem", "Выйти"))

If FileExists($imageupload_dll) = 0 Then TrayItemSetState($is_uploading_item_tray, $TRAY_DISABLE)
	
TrayItemSetOnEvent($snap_full_item_tray, "_ScreenShoot_FullScreen")
TrayItemSetOnEvent($snap_window_item_tray, "_ScreenShoot_SelectedWindow")
TrayItemSetOnEvent($snap_region_item_tray, "_ScreenShoot_Region")

TrayItemSetOnEvent($settings_item_tray, "_SettingsWindow")

TrayItemSetOnEvent($is_uploading_item_tray, "_ImageShack_UploadDlg")
TrayItemSetOnEvent($wc_item_tray, "_WebCab_View")
TrayItemSetOnEvent($exit_item_tray, "_Exit")

_AeroModeCheck()
_ReduceMemory()
read_settings()
use_settings()

If $collapse_at_start_read = 1 Then
	GUISetState(@SW_MINIMIZE)
	minimize_gui()
Else
	GUISetState(@SW_SHOW)
EndIf

While 1
	AdlibRegister("_AeroModeCheck", 100)
	$msg = GUIGetMsg()
		Select
			Case $msg = $GUI_EVENT_CLOSE or $msg = $exit_btn or $msg = $exit_item
				_Exit()
			Case $msg = $GUI_EVENT_MINIMIZE
				minimize_gui()
			Case $msg = $doit_btn or $msg = $doit_item
				screenshoot()
			Case $msg = $screen_folder_open_item
				Local $folder_path = GuiCtrlRead($path_to_save)
				If _IsFolder($folder_path) <> 0 and FileExists($folder_path) = 1 Then
					ShellExecute($folder_path)
				Else
					MsgBox(64, T("FolderErrorTitle", "Ошибка"), T("FolderErrorText", "Введённый путь в строке адреса неправильный или не существует"))
				EndIf
			Case $msg = $wc_shoot_item
				_WebCab_View()
			Case $msg = $is_uploading_item
				_ImageShack_UploadDlg()
			Case $msg = $aero_enable_item
				_DwmEnableComposition(0)
			Case $msg = $aero_disable_item
				_DwmEnableComposition(1)
			Case $msg = $hotkey_btn or $msg = $hotkey_item
				save_settings()
				_SettingsWindow("HK_TAB", $main_dlg)
				read_settings()
				use_settings()
			Case $msg = $settings_item or $msg = $settings_btn
				save_settings()
				GUISetState(@SW_DISABLE, $main_dlg)
				_SettingsWindow($last_tab_read, $main_dlg)
				GUISetState(@SW_ENABLE, $main_dlg)
				WinActivate($main_dlg)
				read_settings()
				use_settings()
			Case $msg = $website_item
				ShellExecute($homepage)
			Case $msg =  $about_item
				If $translator = "" Then 
					Local $translator_text = ""
				Else
					Local $translator_text = @CRLF&@CRLF& T("TranslatorLabel", "Перевод:")&" "& $translator
				EndIf
				MsgBox(64, T("AboutDlgTitle", "О программе"), $application &" "& $version &@CRLF& "Copyright © 2010 [Nuker-Hoax]"& $translator_text &@CRLF&@CRLF& T("GratitudesLabel", "Благодарности:") &@CRLF& "Mr.Creator, Yashied, Ludocus, Larry, wOuter" &@CRLF& "ProgAndy, Rajesh V R, ezzetabi, Jon" &@CRLF& "Rob Saunders, Paul Campbell, Suppir" &@CRLF&@CRLF& T("QuickLicenseText", "Эта программа бесплатна и распространяется\nпод лицензией GNU General Public License.") &@CRLF&@CRLF& $homepage, 0, $main_dlg )
			Case $msg = $browse_btn
				$folder = FileSelectFolder(T("ChangeFolderTitle", "Выберите папку для сохранения скриншотов..."), "", 7)
				if not @error or FileExists($folder) = 1 then GUICtrlSetData($path_to_save, $folder)
		EndSelect
Wend

Func _AeroModeCheck()
	If @OSVersion = "WIN_VISTA" or @OSVersion = "WIN_7" Then
		If _DwmIsCompositionEnabled() = 1 Then 
			GUICtrlSetState($aero_enable_item, $GUI_CHECKED)
			GUICtrlSetState($aero_disable_item, $GUI_UNCHECKED)
		ElseIf _DwmIsCompositionEnabled() = 0 Then
			GUICtrlSetState($aero_enable_item, $GUI_UNCHECKED)
			GUICtrlSetState($aero_disable_item, $GUI_CHECKED)
		EndIf
	EndIf
EndFunc

Func screenshoot()
	save_settings()
	read_settings()
	use_settings()
	
	Local $ImageFullPath, $resized_name
	
	If GUICtrlRead($all_screen_combo) = $GUI_CHECKED Then _ScreenShoot_FullScreen()
	If GUICtrlRead($selected_window_combo) = $GUI_CHECKED Then _ScreenShoot_SelectedWindow()
	If GUICtrlRead($region_shoot_combo) = $GUI_CHECKED Then _ScreenShoot_Region()
EndFunc

Func _ScreenShoot_FullScreen()
	If not $cmdline[0] Then save_settings()
	read_settings()
	If not $cmdline[0] Then use_settings()
	
	If $include_cursor_read = 1 Then 
		Global $incl_cursor = True
	Else
		Global $incl_cursor = False
	EndIf
	
	Local $ImageFullPath, $resized_name
	If FileExists($save_path_read) = 0 Then DirCreate($save_path_read)
	
	If $name_read = 1 Then $ImageFullPath = $save_path_read &"\"& _FileGetNumberName($save_path_read, "screenshoot", $format_read)
	If $name_read = 2 Then $ImageFullPath = $save_path_read & "\screenshoot_"& _FileGetRandomName("."& $format_read, 5)
	If $name_read = 3 Then $ImageFullPath = $save_path_read & "\screenshoot_"& @MDAY & "." & @MON &"."& @YEAR &"_"& @HOUR &"."& @MIN &"."& @SEC &"."& $format_read
	Local $resized_name = StringTrimRight($ImageFullPath, 4) &"_resized."& $format_read
	
	If $use_delay_read = 1 Then Sleep($delay_time_read)
	If $use_sounds_read = 1 Then _PlayWAVResource("SHOOT")
		
	Local $current_win_state = Bitand(WinGetState($main_dlg), 2)
	_Hide_Gui()
	
	Local $shoot = _ScreenCapture_Capture("")
	
	_Show_Gui($current_win_state)
	
	If $saving_read = 1 Then 
		_ScreenCapture_SaveImage($ImageFullPath, $shoot)
		If $ur_read = 1 Then 
			_ImageResize($ImageFullPath, $resized_name, $rw_read, $rh_read)
			FileDelete($ImageFullPath)
			_ImageRotate($resized_name)
		EndIf
		_ImageRotate($ImageFullPath)
	EndIf
	If $saving_read = 2 Then 
		_SaveToClip($shoot)
	EndIf
	If $ur_read = 1 Then 
		_FinishExecute($resized_name)
	Else
		_FinishExecute($ImageFullPath)
	EndIf
	If $enable_ftp_read = 1 Then 
		_FtpUpload($ImageFullPath)
		_FtpUpload($resized_name)
	EndIf
	If $enable_ishack_read = 1 Then 
		If FileExists($resized_name) Then
			_ImageShack_UploadDlg($resized_name)
		Else
			_ImageShack_UploadDlg($ImageFullPath)
		EndIf
	EndIf
EndFunc

Func _ScreenShoot_SelectedWindow()
	If not $cmdline[0] Then save_settings()
	read_settings()
	If not $cmdline[0] Then use_settings()
	
	If $include_cursor_read = 1 Then 
		Global $incl_cursor = True
	Else
		Global $incl_cursor = False
	EndIf
	
	Local $ImageFullPath, $resized_name
	If FileExists($save_path_read) = 0 Then DirCreate($save_path_read)
	
	If $name_read = 1 Then $ImageFullPath = $save_path_read &"\"& _FileGetNumberName($save_path_read, "screenshoot", $format_read)
	If $name_read = 2 Then $ImageFullPath = $save_path_read & "\screenshoot_"& _FileGetRandomName("."& $format_read, 5)
	If $name_read = 3 Then $ImageFullPath = $save_path_read & "\screenshoot_"& @MDAY & "." & @MON &"."& @YEAR &"_"& @HOUR &"."& @MIN &"."& @SEC &"."& $format_read
	Local $resized_name = StringTrimRight($ImageFullPath, 4) &"_resized."& $format_read
	
	If $use_delay_read = 1 Then Sleep($delay_time_read)
	If $use_sounds_read = 1 Then _PlayWAVResource("SHOOT")
		
	Local $current_win_state = Bitand(WinGetState($main_dlg), 2)
	_Hide_Gui()
	
	Local $hWndCtrl = HWnd(_Get_Hovered_Handle())
	Local $hWndWin = _WinAPI_GetAncestor($hWndCtrl, 2)
	
	WinActivate($hWndWin)
	If @OSVersion = "WIN_VISTA" or @OSVersion = "WIN_7" Then
		If $disable_aero_on_wnd_read = 1 Then _DwmAPI_SwitchAeroOnWnd($hWndWin, 1)
		Local $wnd_shot = _ScreenCapture_CaptureAeroWnd("", $hWndWin)
		If $disable_aero_on_wnd_read = 1 Then _DwmAPI_SwitchAeroOnWnd($hWndWin, 0)
	Else
		Local $wnd_shot = _ScreenCapture_CaptureWnd("", $hWndWin)
	EndIf
	_Show_Gui($current_win_state)
	
	If $saving_read = 1 Then 
		_ScreenCapture_SaveImage($ImageFullPath, $wnd_shot)
		If $ur_read = 1 Then 
			_ImageResize($ImageFullPath, $resized_name, $rw_read, $rh_read)
			FileDelete($ImageFullPath)
			_ImageRotate($resized_name)
		EndIf
		_ImageRotate($ImageFullPath)
	EndIf
	If $saving_read = 2 Then 
		_SaveToClip($wnd_shot)
	EndIf
	If $ur_read = 1 Then 
		_FinishExecute($resized_name)
	Else
		_FinishExecute($ImageFullPath)
	EndIf
	If $enable_ftp_read = 1 Then 
		_FtpUpload($ImageFullPath)
		_FtpUpload($resized_name)
	EndIf
	If $enable_ishack_read = 1 Then 
		If FileExists($resized_name) Then
			_ImageShack_UploadDlg($resized_name)
		Else
			_ImageShack_UploadDlg($ImageFullPath)
		EndIf
	EndIf
EndFunc

Func _ScreenShoot_Region()
	If not $cmdline[0] Then save_settings()
	read_settings()
	If not $cmdline[0] Then use_settings()
	
	If $include_cursor_read = 1 Then 
		Global $incl_cursor = True
	Else
		Global $incl_cursor = False
	EndIf
	
	Local $ImageFullPath, $resized_name
	If FileExists($save_path_read) = 0 Then DirCreate($save_path_read)
	
	If $name_read = 1 Then $ImageFullPath = $save_path_read &"\"& _FileGetNumberName($save_path_read, "screenshoot", $format_read)
	If $name_read = 2 Then $ImageFullPath = $save_path_read & "\screenshoot_"& _FileGetRandomName("."& $format_read, 5)
	If $name_read = 3 Then $ImageFullPath = $save_path_read & "\screenshoot_"& @MDAY & "." & @MON &"."& @YEAR &"_"& @HOUR &"."& @MIN &"."& @SEC &"."& $format_read
	Local $resized_name = StringTrimRight($ImageFullPath, 4) &"_resized."& $format_read
	
	Local $region_code = GetRegion()

	ToolTip("")
	If $region_code <> 0 and ($pos_x_1 - $pos_x_2) <> 0 and ($pos_y_1 - $pos_y_2) <> 0 Then
		If $use_delay_read = 1 Then Sleep($delay_time_read)
		If $use_sounds_read = 1 Then _PlayWAVResource("SHOOT")
		
		Local $region_shot = _ScreenCapture_Capture("", $pos_x_1, $pos_y_1, $pos_x_2, $pos_y_2)
	
		If $saving_read = 1 Then 
			_ScreenCapture_SaveImage($ImageFullPath, $region_shot)
			If $ur_read = 1 Then 
				_ImageResize($ImageFullPath, $resized_name, $rw_read, $rh_read)
				FileDelete($ImageFullPath)
				_ImageRotate($resized_name)
			EndIf
			_ImageRotate($ImageFullPath)
		EndIf
		If $saving_read = 2 Then 
			_SaveToClip($region_shot)
		EndIf
		If $ur_read = 1 Then 
			_FinishExecute($resized_name)
		Else
			_FinishExecute($ImageFullPath)
		EndIf
		If $enable_ftp_read = 1 Then 
			_FtpUpload($ImageFullPath)
			_FtpUpload($resized_name)
		EndIf
		If $enable_ishack_read = 1 Then 
			If FileExists($resized_name) Then
				_ImageShack_UploadDlg($resized_name)
			Else
				_ImageShack_UploadDlg($ImageFullPath)
			EndIf
		EndIf
	EndIf
EndFunc

Func _WebCab_View($hWnd = $main_dlg)
	save_settings()
	read_settings()
	use_settings()
	
	GUISetState(@SW_DISABLE, $hWnd)
	Local $wc_dlg = GUICreate(T("WcShootTitle", "Захват с веб-камеры"), 372, 481, -1, -1, -1, -1, $hHwnd)
	GUISetIcon(@ScriptFullPath, -1, $wc_dlg)

	If $use_trans_read = 1 Then WinSetTrans($wc_dlg, "", (255 / 100) * $trans_ratio_read)
	If $always_on_top_read = 1 Then WinSetOnTop($wc_dlg, "", 1)

	Local $wc_open = _WebcamOpen($wc_dlg, 10, 10, 352, 288)

	Local $wc_shoot_btn = GUICtrlCreateButton(T("WCScreenShootButton", "Скриншот"), 10, 440, 75, 25, -1, 131072)
	Local $wc_start_btn = GUICtrlCreateButton(T("WCRecordButton", "Запись"), 92, 440, 75, 25, -1, 131072)
	Local $wc_stop_btn = GUICtrlCreateButton(T("WCStopButton", "Остановить"), 175, 440, 75, 25, -1, 131072)
	Local $wc_close_btn = GUICtrlCreateButton(T("WCCloseButton", "Закрыть"), 287, 440, 75, 25, -1, 131072)

	GUICtrlCreateGroup(T("WCVideoSettingsGroup", "Параметры видео"), 10, 302, 352, 128)

	GUICtrlCreateLabel(T("WCVideoSourceLabel", "Настроить источник видео:"), 25, 327, 150, 25, 0x0200)
	Local $video_source_btn = GUICtrlCreateButton("...", 322, 327, 25, 25, -1, 131072)
	
	GUICtrlCreateLabel(T("WCVideoFormatLabel", "Настроить формат видео:"), 25, 357, 150, 25, 0x0200)
	Local $video_format_btn = GUICtrlCreateButton("...", 322, 357, 25, 25, -1, 131072)

	GUICtrlCreateLabel(T("WCVideoCompressionLabel", "Настроить сжатие видео:"), 25, 387, 150, 25, 0x0200)
	Local $video_compression_btn = GUICtrlCreateButton("...", 322, 387, 25, 25, -1, 131072)

	GUISetState(@SW_SHOW)

	While 1
		$nMsg = GUIGetMsg()
		Switch $nMsg
			Case $GUI_EVENT_CLOSE, $wc_close_btn
				_WebcamRecordStop($wc_open)
				$wc_close = _WebcamClose($wc_open)
				If $wc_close = 0 Then MsgBox(16, T("WCErrorTitle", "Ошибка"), T("WCErrorUnloadText", "Ошибка при отключении от веб-камеры"))
				DllCall('user32.dll', 'int', 'SetActiveWindow', 'hwnd', $hWnd)
				GuiDelete($wc_dlg)
				GUISetState(@SW_ENABLE, $hWnd)
				_WinAPI_SetFocus(_WinAPI_GetFocus())
				ExitLoop
			Case $wc_shoot_btn
				Local $ImageFullPath
				If FileExists($save_path_read) = 0 Then DirCreate($save_path_read)
					
				If $name_read = 1 Then $ImageFullPath = $save_path_read &"\"& _FileGetNumberName($save_path_read, "screenshoot", $format_read)
				If $name_read = 2 Then $ImageFullPath = $save_path_read & "\screenshoot_"& _FileGetRandomName("."& $format_read, 5)
				If $name_read = 3 Then $ImageFullPath = $save_path_read & "\screenshoot_"& @MDAY & "." & @MON &"."& @YEAR &"_"& @HOUR &"."& @MIN &"."& @SEC &"."& $format_read
				Local $resized_name = StringTrimRight($ImageFullPath, 4) &"_resized."& $format_read
				
				If $use_sounds_read = 1 Then _PlayWAVResource("SHOOT")
				$wc_shoot = _WebcamSnap($wc_open, $ImageFullPath)
				If $wc_shoot = 0 Then MsgBox(16, T("WCErrorTitle", "Ошибка"), T("WCErrorShootText", "Ошибка сохранения / создания изображения"))
			Case $wc_start_btn
				_WebcamRecordStart($save_path_read &"\"& _FileGetNumberName($save_path_read, "web_cab_movie", $format_read), $wc_open)	
			Case $wc_stop_btn
				If $use_sounds_read = 1 Then _PlayWAVResource("SHOOT")
				$wc_start_rec = _WebcamRecordStop($wc_open)
				If $wc_start_rec = 0 Then MsgBox(64, T("WCErrorTitle", "Ошибка"), T("WCErrorStopText", "Ошибка остановки записи"))
			Case $video_source_btn
				DllCall("user32.dll", "int", "SendMessage", "hWnd", $wc_open, "int", $WM_CAP_DLG_VIDEOSOURCE, "int", 0, "int", 0)
			Case $video_format_btn
				DllCall("user32.dll", "int", "SendMessage", "hWnd", $wc_open, "int", $WM_CAP_DLG_VIDEOFORMAT, "int", 0, "int", 0)
			Case $video_compression_btn
				DllCall("user32.dll", "int", "SendMessage", "hWnd", $wc_open, "int", $WM_CAP_DLG_VIDEOCOMPRESSION, "int", 0, "int", 0)
		EndSwitch
	WEnd
EndFunc

Func _ImageShack_UploadDlg($sFile = "", $hHwnd = $main_dlg)
	Local $UploadDll = @ScriptDir & "\UploadFile.dll", $FileStr

	If FileExists($UploadDll) Then
		read_settings()
		
		If FileExists($sFile) Then $FileStr = $sFile
		GUISetState(@SW_DISABLE, $hHwnd)
		Local $imageshack_dlg = GUICreate(T("ImageShackDlgTitle", "Загрузка на ImageShack...") , 468, 246, -1, -1, -1, $WS_EX_ACCEPTFILES, $hHwnd)
		GUISetIcon(@ScriptFullPath, 101, $imageshack_dlg)

		If $use_trans_read = 1 Then WinSetTrans($wc_dlg, "", (255 / 100) * $trans_ratio_read)
		If $always_on_top_read = 1 Then WinSetOnTop($wc_dlg, "", 1)
		
		Local $drop_zone = GUICtrlCreateLabel("", 0, 65, 468, 181)
		
		GUICtrlCreatePic("", 0, 0, 468, 60, 256 + 131072 + 67108864, 0x00100000)
		_ResourceSetImageToCtrl(-1, "IMAGESHACK", $RT_BITMAP)
		
		GUICtrlCreateGroup(T("ImageShackImageGroup", "Изображение для загрузки"), 10, 64, 447, 65)
		Local $file_path_input = GUICtrlCreateInput($FileStr, 24, 88, 377, 21)
		
		Local $browse_file_btn = GUICtrlCreateButton("...", 408, 88, 35, 22, Default, 131072)

		GUICtrlCreateGroup(T("ImageShackLinksGroup", "Ссылка на изображение"), 10, 133, 447, 65)
		Local $img_link = GUICtrlCreateInput("", 24, 157, 377, 20, 2048)
		Local $copy_link_btn = GUICtrlCreateButton("copy", 408, 157, 35, 22, Default, 131072)

		Local $upload_btn = GUICtrlCreateButton(T("ImageShackUploadBtn", "Загрузить"), 300, 210, 75, 25, Default, 131072)
		Local $close_btn = GUICtrlCreateButton(T("ImageShackCloseBtn", "Закрыть"), 383, 210, 75, 25, Default, 131072)

		GUICtrlSetBkColor($drop_zone, $GUI_BKCOLOR_TRANSPARENT)
		GUICtrlSetState($drop_zone, $GUI_DROPACCEPTED)
		GUICtrlSetState($drop_zone, $GUI_DISABLE)

		GUISetState(@SW_SHOW)

		While 1
			$msg = GUIGetMsg()
			Select
				Case $msg = $GUI_EVENT_CLOSE or $msg = $close_btn
					DllCall('user32.dll', 'int', 'SetActiveWindow', 'hwnd', $hHwnd)
					GUIDelete()
					GUISetState(@SW_ENABLE, $hHwnd)
					_WinAPI_SetFocus(_WinAPI_GetFocus())
					ExitLoop
				Case $msg = $GUI_EVENT_DROPPED
					If FileExists(@GUI_DragFile) Then
						If _GetContentType(@GUI_DragFile) = "" Then
							MsgBox(16, T("ImageShackErrorTitle", "Ошибка"), T("ImageShackContentTypeErrorText", 'Невозможно определить "Content Type" файла'))
						Else
							GUICtrlSetData($file_path_input, @GUI_DragFile)
						EndIf
					EndIf
				Case $msg = $copy_link_btn
					$link = GuiCtrlRead($img_link)
					If $link <> "" or StringInStr($link, "504 Gateway Time-out") = 0 Then ClipPut($link)
				Case $msg = $browse_file_btn
					Local $image = FileOpenDialog(T("ImageShackSelectImageTitle", "Выберите изображения..."), "", T("ImageShackSelectImage", "Изображения") &" (*.jpg;*.jpeg;*.bmp;*.png;*.gif;*.tif;*.tiff;*.pdf;*.swf)", 1)
					If Not @error Then GUICtrlSetData($file_path_input, $image)
				Case $msg = $upload_btn
					Local $file_read = GuiCtrlRead($file_path_input)
					
					If $file_read = "" or FileExists($file_read) = 0 Then
						MsgBox(16, T("ImageShackErrorTitle", "Ошибка"), T("ImageShackNotExistsText", "Необходимо выбрать файл для загрузки"))
					Else
					
					Local $content_type = _GetContentType($file_read)
					
					If $content_type = "" Then
						MsgBox(16, T("ImageShackErrorTitle", "Ошибка"), T("ImageShackContentTypeErrorText", 'Невозможно определить "Content Type" файла'))
					Else
						Local $aRet = DllCall($UploadDll, "str", "UploadFile", "str", "http://www.imageshack.us/upload_api.php", "str", "fileupload", "str", $content_type, "str", $file_read)
						Local $link = StringRegExpReplace($aRet[0], '(?is).*<(image_link)>(.*?)</\1>.*', '\2')
						If StringInStr($link, "504 Gateway Time-out") Then
							GUICtrlSetData($img_link, "504 Gateway Time-out")
						Else
							GUICtrlSetData($img_link, $link)
						EndIf
					EndIf
					EndIf
			EndSelect
		WEnd
	Else
		MsgBox(16, T("ImageShackErrorTitle", "Ошибка"), T("ImageShackDllNotExistsText", "Отсутствует файл UploadFile.dll необходимый для загрузки скриншотов"))
	EndIf
EndFunc

Func _SettingsWindow($hTab = "MAIN_TAB", $hHwnd = $main_dlg)
	;Read Settings
	Local $resize_width_read = IniRead($settings_file, "freeshooter", "ResizeWidth", "640")
	Local $resize_height_read = IniRead($settings_file, "freeshooter", "ResizeHeight", "480")
	Local $use_resize_read = IniRead($settings_file, "freeshooter", "UseResize", "0")
	Local $image_format_read = IniRead($settings_file, "freeshooter", "ImageFormat", "PNG")
	Local $delay_read = IniRead($settings_file, "freeshooter", "DelayTime", "3")
	Local $delay_use_read = IniRead($settings_file, "freeshooter", "DelayUse", "0")
	Local $always_on_top_read = IniRead($settings_file, "freeshooter", "AlwaysOnTop", "0")
	Local $collapse_at_start_read = IniRead($settings_file, "freeshooter", "CollapseAtStart", "0")
	Local $use_sounds_read = IniRead($settings_file, "freeshooter", "UseSounds", "1")
	Local $include_cursor_read = IniRead($settings_file, "freeshooter", "IncludeCursor", "0")
	Local $name_mode_read = IniRead($settings_file, "freeshooter", "NameMode", "1")
	Local $saving_mode_read = IniRead($settings_file, "freeshooter", "SavingMode", "1")
	Local $use_manifest_read = IniRead($settings_file, "freeshooter", "UseManifest", "0")
	Local $use_trans_read = IniRead($settings_file, "freeshooter", "UseTransparent", "0")
	Local $trans_ratio_read = IniRead($settings_file, "freeshooter", "TransparentRatio", "100")
	Local $enable_ishack_read = IniRead($settings_file, "freeshooter", "EnableIShack", "0")
	Local $finish_mode_read = IniRead($settings_file, "freeshooter", "FinishMode", "1")
	Local $finish_mode_path_read = IniRead($settings_file, "freeshooter", "FinishModePath", "")
	Local $disable_aero_on_wnd_read = IniRead($settings_file, "freeshooter", "DisableAeroOnWnd", "0")
	Local $rotate_read = IniRead($settings_file, "freeshooter", "RotateMode", "0")
	Local $enable_ftp_read = IniRead($settings_file, "FTP", "Enable", "0")
	Local $ftp_server_read = IniRead($settings_file, "FTP", "Server", "")
	Local $ftp_username_read = IniRead($settings_file, "FTP", "Username", "")
	Local $ftp_password_read = IniRead($settings_file, "FTP", "Password", "")
	Local $ftp_folder_read = IniRead($settings_file, "FTP", "Folder", "/")
	Local $no_ftp_gui_read = IniRead($settings_file, "FTP", "NoFtpGui", "0")
	Local $ftp_del_after_upload_read = IniRead($settings_file, "FTP", "DelAfterUpload", "0")
	Local $enable_aero_switch_read = IniRead($settings_file, "freeshooter", "AeroSwitch", "0")
	Local $last_tab_read = IniRead($settings_file, "freeshooter", "LastSettingsTab", "MAIN_TAB")

    GUISetState(@SW_DISABLE, $hHwnd)

	Global $opt_dlg = GUICreate(T("SettingsTitle", "Настройки"), 462, 310, -1, -1, -1, -1, $hHwnd)
	GUISetIcon(@ScriptFullPath, -1, $opt_dlg)
	If $always_on_top_read = 1 Then WinSetOnTop($opt_dlg, "", 1)
	If $use_trans_read = 1 Then WinSetTrans($opt_dlg, "", (255 / 100) * $trans_ratio_read)
		
	GUICtrlCreateLabel("* - "& T("SettingsNeedRestart", "необходим перезапуск"), 20, 277, 200, 15)
	GUICtrlSetColor(-1, 0xc9c9c9)
		
	Local $opt_ok_btn = GUICtrlCreateButton(T("SettingsOkButton", "OK"), 298, 272, 75, 25, 0x0001, 131072)
	Local $opt_cancel_btn = GUICtrlCreateButton(T("SettingsCancelButton", "Отмена"), 377, 272, 75, 25, Default, 131072)

	$settings_tab = GUICtrlCreateTab(10, 10, 444, 249)

	Local $main_tab = GUICtrlCreateTabItem(T("SettingsMainTab", "Основные"))
	GUICtrlCreateGroup("Параметры", 24, 40, 416, 124)
	Local $always_on_top_opt_chk = GUICtrlCreateCheckbox(T("SettingsAlwaysOnTop", "Поверх остальных окон"), 38, 58, 250, 17)
		If $always_on_top_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $use_manifest_chk = GUICtrlCreateCheckbox(T("SettingsUseManifest", "Использовать оформление системы") &" *", 38, 78, 250, 17)
		If $use_manifest_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $collapse_at_start_opt_chk = GUICtrlCreateCheckbox(T("SettingsCollapseAtStart", "Запускать свёрнутым"), 38, 98, 250, 17)
		If $collapse_at_start_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $use_sounds_opt_chk = GUICtrlCreateCheckbox(T("SettingsUseSounds", "Использовать звуки"), 38, 118, 250, 17)
		If $use_sounds_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $include_cursor_opt_chk = GUICtrlCreateCheckbox(T("SettingsIncludeCursor", "Захватывать курсор"), 38, 138, 250, 17)
		If $include_cursor_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	
	GUICtrlCreateGroup(T("SettingsLanguageGroup", "Язык интерфейса") &" *", 24, 165, 174, 80)
	GUICtrlCreateLabel(T("SettingsSelectLangText", "Выберите язык из списка:"), 38, 189, 180, 17)
	Local $lang_combo = GUICtrlCreateCombo("", 38, 209, 140, 22, 0x00200003)
	GUICtrlSetData(-1, _GetLangList(), $language)
	
	GUICtrlCreateGroup(T("SettingsTransparencyGroup", "Прозрачность окон программы"), 205, 165, 235, 80)
	Global $use_trans_chk = GUICtrlCreateCheckbox(T("SettingsUseTransparency", "Использовать прозрачность"), 219, 189, 180, 17)
		If $use_trans_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Global $trans_slider = GuiCtrlCreateSlider(217, 209, 170, 17)
	GUICtrlSetLimit(-1, 100, 25)
	GUICtrlSetData(-1, $trans_ratio_read)
	Global $trans_input = GuiCtrlCreateInput($trans_ratio_read, 390, 212, 34, 17, 2048 + 8192 + 1)

	GUIRegisterMsg($WM_HSCROLL, "TRANS_HSCROLL")

	Local $shoot_tab = GUICtrlCreateTabItem(T("SettingsScreenshootTab", "Скриншоты"))
	GUICtrlCreateGroup(T("SettingsImagesFormat", "Формат изображений"), 24, 40, 172, 80)
	Local $format_combo = GUICtrlCreateCombo("", 38, 63, 140, 17, 0x00200003)
	GUICtrlSetData($format_combo, "PNG|JPG|BMP|GIF|TIF", $image_format_read)
	Local $info_about_format_label = GUICtrlCreateLabel(T("SettingsFormatInfo", "Сведения о формате (Wiki)"), 38, 93, 150, 15)
	
	GUICtrlSetCursor(-1, 0)
	GUICtrlSetFont(-1, 8.5, 400, 4)
	GUICtrlSetColor(-1, 0x0000FF)
	
	GUICtrlCreateGroup(T("SettingsDelayGroup", "Задержка"), 204, 40, 236, 80)
	Local $use_delay_chk = GUICtrlCreateCheckbox(T("SettingsUseDelay", "Использовать задержку"), 220, 60, 150, 17)
	If $delay_use_read = 1 Then GuiCtrlSetState($use_delay_chk, $GUI_CHECKED)
	GUICtrlCreateLabel(T("SettingsDelayLabel", "Задержка:"), 220, 92, 55, 124)
	Local $delay_input = GUICtrlCreateInput($delay_read, 281, 85, 31, 20, 0x2000 + 0x0001)
	GUICtrlCreateLabel(T("SettingsDelaySecLabel", "сек."), 320, 92, 50, 17)
	
	GUICtrlCreateGroup(T("SettingsShootNameGroup", "Имя скриншотов"), 24, 121, 416, 61)
	Local $number_name_radio = GUICtrlCreateRadio("screenshoot_[ "& T("SettingsNumbersName", "001...xxx") &" ]", 38, 138, 170, 17)
		If $name_mode_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $random_name_radio = GUICtrlCreateRadio("screenshoot_[ "& T("SettingsRandomName", "случайные символы") &" ]", 38, 156, 200, 17)
		If $name_mode_read = "2" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $date_name_radio = GUICtrlCreateRadio("screenshoot_[ "& T("SettingsDateName", "дата и время") &" ]", 258, 138, 164, 17)
		If $name_mode_read = "3" Then GuiCtrlSetState(-1, $GUI_CHECKED)

	GUICtrlCreateGroup(T("SettingsSavingModeGroup", "Режим сохранения"), 24, 183, 416, 62)
	Local $save_to_file_radio = GUICtrlCreateRadio(T("SettingsSaveToFile", "Сохранение в файл"), 38, 200, 350, 17)
		If $saving_mode_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $save_to_clip_radio = GUICtrlCreateRadio(T("SettingsSaveToClip", "Помещать изображение в буфер обмена (без эффектов)"), 38, 218, 350, 17)
		If $saving_mode_read = "2" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	
	Local $effects_tab = GUICtrlCreateTabItem(T("SettingsEffectsTab", "Эффекты"))
	GUICtrlCreateGroup(T("SettingsResizeGroup", "Изменение размеров"), 24, 40, 416, 94)
	Local $resize_enable_chk = GUICtrlCreateCheckbox(T("SettingsUseResize", "Изменять размеры"), 38, 60, 350, 17)
	If $use_resize_read = 1 Then GuiCtrlSetState($resize_enable_chk, $GUI_CHECKED)
	
	GUICtrlCreateLabel(T("SettingsResizeWidth", "Ширина:"), 38, 90, 46, 17)
	Local $width_size = GUICtrlCreateInput("", 86, 84, 40, 20, 0x2000 + 0x0001)
	GUICtrlSetData($width_size, $resize_width_read)
	GUICtrlCreateLabel(T("SettingsResizeHeight", "Высота:"), 147, 90, 45, 17)
	Local $height_size = GUICtrlCreateInput("", 197, 84, 40, 20, 0x2000 + 0x0001)
	GUICtrlSetData($height_size, $resize_height_read)
	
	GUICtrlCreateGroup(T("SettingsAeroGroup", "Aero"), 24, 135, 206, 110)
	Local $disable_aero_on_wnd_chk = GUICtrlCreateCheckbox(T("SettingsDisableAeroOnWnd", "Отключать Aero окна"), 38, 155, 180, 17)
 		If $disable_aero_on_wnd_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	GUICtrlSetState($disable_aero_on_wnd_chk, $GUI_DISABLE)
	If @OSVersion = "WIN_VISTA" or @OSVersion = "WIN_7" Then GUICtrlSetState($disable_aero_on_wnd_chk, $GUI_ENABLE)
	GUICtrlCreateLabel(T("SettingsDisableAeroOnWndText", 'Перед созданием скриншота в режиме "Выбранное окно", Aero окна отключится, затем вернётся в прежнее состояние'), 38, 175, 180, 51, $GUI_FOCUS)
		
	GUICtrlCreateGroup(T("SettingsRotateGroup", "Вращение"), 238, 135, 202, 110)
	Local $no_rotate_radio = GUICtrlCreateRadio(T("SettingsRotateDisable", "Отключить"), 252, 155, 150, 17)
		If $rotate_read = "0" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $right_rotate_radio = GUICtrlCreateRadio(T("SettingsRotate90", "По часовой стрелке"), 252, 175, 150, 17)
		If $rotate_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $left_rotate_radio = GUICtrlCreateRadio(T("SettingsRotate270", "Против часовой стрелки"), 252, 195, 150, 17)
		If $rotate_read = "2" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $top_rotate_radio = GUICtrlCreateRadio(T("SettingsRotate180", "Перевернуть"), 252, 215, 150, 17)
		If $rotate_read = "3" Then GuiCtrlSetState(-1, $GUI_CHECKED)

	Local $hk_tab = GUICtrlCreateTabItem(T("SettingsHotKeyTab", "Горячие клавиши"))
	GUICtrlCreateGroup(T("SettingsHotKeyAllScreen", "Во весь экран"), 24, 40, 416, 67)
	Local $all_screen_ctrl_chk = GUICtrlCreateCheckbox("CTRL", 38, 67, 50, 17)
		If $all_screen_ctrl_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $all_screen_alt_chk = GUICtrlCreateCheckbox("ALT", 138, 67, 50, 17)
		If $all_screen_alt_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $all_screen_shift_chk = GUICtrlCreateCheckbox("SHIFT", 238, 67, 50, 17)
		If $all_screen_shift_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $all_screen_extra_key = GuiCtrlCreateCombo("", 325, 67, 85, 22, 0x00200003)
	GUICtrlSetData(-1, _GetHotKeysList(), $all_screen_extra_read)
	
	GUICtrlCreateGroup(T("SettingsHotKeySelectedWindow", "Выбранное окно (под курсором)"), 24, 109, 416, 67)
	Local $selected_win_ctrl_chk = GUICtrlCreateCheckbox("CTRL", 38, 135, 50, 17)
		If $selected_win_ctrl_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $selected_win_alt_chk = GUICtrlCreateCheckbox("ALT", 138, 135, 50, 17)
		If $selected_win_alt_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $selected_win_shift_chk = GUICtrlCreateCheckbox("SHIFT", 238, 135, 50, 17)
		If $selected_win_shift_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $selected_win_extra_key = GuiCtrlCreateCombo("", 325, 135, 85, 22, 0x00200003)
	GUICtrlSetData(-1, _GetHotKeysList(), $selected_win_extra_read)
	
	GUICtrlCreateGroup(T("SettingsHotKeyRegion", "Выделенная область"), 24, 178, 416, 67)
	Local $region_ctrl_chk = GUICtrlCreateCheckbox("CTRL", 38, 203, 50, 17)
		If $region_ctrl_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $region_alt_chk = GUICtrlCreateCheckbox("ALT", 138, 203, 50, 17)
		If $region_alt_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $region_shift_chk = GUICtrlCreateCheckbox("SHIFT", 238, 203, 50, 17)
		If $region_shift_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $region_extra_key = GuiCtrlCreateCombo("", 325, 203, 85, 22, 0x00200003)
	GUICtrlSetData(-1, _GetHotKeysList(), $region_extra_read)
	
	Local $action_tab = GUICtrlCreateTabItem(T("SettingsActionTab", "Действия"))
	GUICtrlCreateGroup(T("SettingsActionAfterShoot", "После создания скриншота"), 24, 40, 416, 205)
	Local $enable_ishack_uploading_chk = GUICtrlCreateCheckbox(T("SettingsActionIShack", "Включить загрузку скриншотов на ImageShack"), 38, 60, 270, 17)
		If $enable_ishack_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
		If FileExists($imageupload_dll) = 0 Then GuiCtrlSetState(-1, $GUI_DISABLE)
	Local $do_nothing_chk = GUICtrlCreateRadio(T("SettingsActionSilent", "Не выполнять никаких действий (тихий режим)"), 38, 80, 270, 17)
		If $finish_mode_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $shellex_chk = GUICtrlCreateRadio(T("SettingsActionShellEx", "Открыть в программе по умолчанию"), 38, 100, 270, 17)
		If $finish_mode_read = "2" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $set_shellex_chk = GUICtrlCreateRadio(T("SettingsActionExecute", "Выполнить программу"), 38, 120, 270, 17)
		If $finish_mode_read = "3" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	Local $set_shellex_input = GuiCtrlCreateInput($finish_mode_path_read, 38, 145, 240, 22)
	Local $browse_shellex_btn = GUICtrlCreateButton("...", 285, 145, 22, 22, Default, 131072)
	GuiCtrlCreateLabel(T("SettingsActionHelp", "Вы можете задать параметры для выполняемого приложения:" &@CRLF& "%s - полный путь к созданному скриншоту" &@CRLF& "%f - папка программы"), 38, 175, 400, 60, 256)

	Local $ftp_tab = GUICtrlCreateTabItem(T("SettingsFtpTab", "FTP"))
	GUICtrlCreateGroup(T("SettingsFtpUploadGroup", "Загрузка на FTP сервер"), 24, 40, 416, 205)
	Local $enable_ftp_chk = GUICtrlCreateCheckbox(T("SettingsEnableFtp", "Включить загрузку"), 38, 60, 270, 17)
		If $enable_ftp_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
	GUICtrlCreateLabel(T("SettingsFtpServer", "Сервер:"), 38, 90, 120, 17)
	Local $ftp_server_input = GuiCtrlCreateInput($ftp_server_read, 150, 85, 150, 18)
	GUICtrlCreateLabel(T("SettingsFtpUser", "Пользователь:"), 38, 116, 120, 17)
	Local $ftp_name_input = GuiCtrlCreateInput($ftp_username_read, 150, 112, 150, 18)
	GUICtrlCreateLabel(T("SettingsFtpPassword", "Пароль:"), 38, 145, 120, 17)
	Local $ftp_password_input = GuiCtrlCreateInput($ftp_password_read, 150, 140, 150, 18, $ES_PASSWORD)
	GUICtrlCreateLabel(T("SettingsFtpFolder", "Папка:"), 38, 172, 120, 17)
	Local $ftp_folder_input = GuiCtrlCreateInput($ftp_folder_read, 150, 167, 150, 18)

	Local $no_ftp_gui_chk = GUICtrlCreateCheckbox(T("SettingsNoFtpGui", "Не показывать  диалог загрузки"), 38, 200, 270, 17)
		If $no_ftp_gui_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)

	Local $ftp_del_after_upload_chk = GUICtrlCreateCheckbox(T("SettingsFtpDelAfterUpload", "Удалять исходный файл после загрузки"), 38, 220, 270, 17)
		If $ftp_del_after_upload_read = "1" Then GuiCtrlSetState(-1, $GUI_CHECKED)
			
	GUISetState(@SW_SHOW)
	
	Select
		Case $last_tab_read = "MAIN_TAB" or $hTab = "MAIN_TAB"
			GUICtrlSetState($main_tab, $GUI_SHOW)
		Case $last_tab_read = "SHOOT_TAB" or $hTab = "SHOOT_TAB"
			GUICtrlSetState($shoot_tab, $GUI_SHOW)
		Case $last_tab_read = "EFFECTS_TAB" or $hTab = "EFFECTS_TAB"
			GUICtrlSetState($effects_tab, $GUI_SHOW)
		Case $last_tab_read = "HK_TAB" or $hTab = "HK_TAB"
			GUICtrlSetState($hk_tab, $GUI_SHOW)
		Case $last_tab_read = "ACTION_TAB" or $hTab = "ACTION_TAB"
			GUICtrlSetState($action_tab, $GUI_SHOW)
		Case $last_tab_read = "FTP_TAB" or $hTab = "FTP_TAB"
			GUICtrlSetState($ftp_tab, $GUI_SHOW)
	EndSelect
		
	If $hTab = "HK_TAB" Then GUICtrlSetState($hk_tab, $GUI_SHOW)

	While 1
		$nMsg = GUIGetMsg()
		Select
			Case $nMsg = $browse_shellex_btn
				$shellex_file = FileOpenDialog(T("SettingsActionShellExChangeFile", "Выберите файл..."), "", T("SettingsApplicationType", "Приложения") &" (*.exe;*.com;*.bat;*.cmd)|"& T("SettingsAllFilesType", "Все файлы") &" (*.*)", 1+2)
				If not @error Then
					GUICtrlSetData($set_shellex_input, $shellex_file)
				EndIf
			Case $nMsg = $info_about_format_label
				If GUICtrlRead($format_combo) = "PNG" Then ShellExecute("http://ru.wikipedia.org/wiki/PNG")
				If GUICtrlRead($format_combo) = "JPG" Then ShellExecute("http://ru.wikipedia.org/wiki/JPG")
				If GUICtrlRead($format_combo) = "BMP" Then ShellExecute("http://ru.wikipedia.org/wiki/BMP")
				If GUICtrlRead($format_combo) = "GIF" Then ShellExecute("http://ru.wikipedia.org/wiki/GIF")
				If GUICtrlRead($format_combo) = "TIF" Then ShellExecute("http://ru.wikipedia.org/wiki/TIF")
			Case $nMsg = $GUI_EVENT_CLOSE or $nMsg = $opt_cancel_btn
				DllCall('user32.dll', 'int', 'SetActiveWindow', 'hwnd', $hHwnd)
				GUIDelete()
				GUISetState(@SW_ENABLE, $hHwnd)
				_WinAPI_SetFocus(_WinAPI_GetFocus())
				ExitLoop
			Case $nMsg = $opt_ok_btn
				;Save Last Tab Settings
				If GUICtrlRead($settings_tab) = 0 Then IniWrite($settings_file, "freeshooter", "LastSettingsTab", "MAIN_TAB")
				If GUICtrlRead($settings_tab) = 1 Then IniWrite($settings_file, "freeshooter", "LastSettingsTab", "SHOOT_TAB")
				If GUICtrlRead($settings_tab) = 2 Then IniWrite($settings_file, "freeshooter", "LastSettingsTab", "EFFECTS_TAB")
				If GUICtrlRead($settings_tab) = 3 Then IniWrite($settings_file, "freeshooter", "LastSettingsTab", "HK_TAB")
				If GUICtrlRead($settings_tab) = 4 Then IniWrite($settings_file, "freeshooter", "LastSettingsTab", "ACTION_TAB")
				If GUICtrlRead($settings_tab) = 5 Then IniWrite($settings_file, "freeshooter", "LastSettingsTab", "FTP_TAB")
				
				;Hotkeys Settings (All Screen)
				Local $all_screen_ctrl_key = Number(GUICtrlRead($all_screen_ctrl_chk) = $GUI_CHECKED)
				Local $all_screen_alt_key = Number(GUICtrlRead($all_screen_alt_chk) = $GUI_CHECKED)
				Local $all_screen_shift_key = Number(GUICtrlRead($all_screen_shift_chk) = $GUI_CHECKED)
				Local $all_screen_extra_read = GUICtrlRead($all_screen_extra_key)
				IniWrite($settings_file, "HotKey", "AllScreenCtrl", $all_screen_ctrl_key)
				IniWrite($settings_file, "HotKey", "AllScreenAlt", $all_screen_alt_key)
				IniWrite($settings_file, "HotKey", "AllScreenShift", $all_screen_shift_key)
				IniWrite($settings_file, "HotKey", "AllScreenExtra", $all_screen_extra_read)
				IniWrite($settings_file, "freeshooter", "HotKeyAllScreen", _GetHotKeySetValue($all_screen_ctrl_key, $all_screen_alt_key, $all_screen_shift_key, $all_screen_extra_read))
				
				;Hotkeys Settings (Selected Window)
				Local $selected_win_ctrl_key = Number(GUICtrlRead($selected_win_ctrl_chk) = $GUI_CHECKED)
				Local $selected_win_alt_key = Number(GUICtrlRead($selected_win_alt_chk) = $GUI_CHECKED)
				Local $selected_win_shift_key = Number(GUICtrlRead($selected_win_shift_chk) = $GUI_CHECKED)
				Local $selected_win_extra_read = GUICtrlRead($selected_win_extra_key)
				IniWrite($settings_file, "HotKey", "SelectedWindowCtrl", $selected_win_ctrl_key)
				IniWrite($settings_file, "HotKey", "SelectedWindowAlt", $selected_win_alt_key)
				IniWrite($settings_file, "HotKey", "SelectedWindowShift", $selected_win_shift_key)
				IniWrite($settings_file, "HotKey", "SelectedWindowExtra", $selected_win_extra_read)
				IniWrite($settings_file, "freeshooter", "HotKeySelectedWindow", _GetHotKeySetValue($selected_win_ctrl_key, $selected_win_alt_key, $selected_win_shift_key, $selected_win_extra_read))
				
				;Hotkeys Settings (Region Shoot)
				Local $region_ctrl_key = Number(GUICtrlRead($region_ctrl_chk) = $GUI_CHECKED)
				Local $region_alt_key = Number(GUICtrlRead($region_alt_chk) = $GUI_CHECKED)
				Local $region_shift_key = Number(GUICtrlRead($region_shift_chk) = $GUI_CHECKED)
				Local $region_extra_read = GUICtrlRead($region_extra_key)
				IniWrite($settings_file, "HotKey", "RegionCtrl", $region_ctrl_key)
				IniWrite($settings_file, "HotKey", "RegionAlt", $region_alt_key)
				IniWrite($settings_file, "HotKey", "RegionShift", $region_shift_key)
				IniWrite($settings_file, "HotKey", "RegionExtra", $region_extra_read)
				IniWrite($settings_file, "freeshooter", "HotKeyRegion", _GetHotKeySetValue($region_ctrl_key, $region_alt_key, $region_shift_key, $region_extra_read))
				
				;Options
				IniWrite($settings_file, "freeshooter", "UseManifest", Number(GUICtrlRead($use_manifest_chk) = $GUI_CHECKED))
				IniWrite($settings_file, "freeshooter", "AlwaysOnTop", Number(GUICtrlRead($always_on_top_opt_chk) = $GUI_CHECKED))
				IniWrite($settings_file, "freeshooter", "CollapseAtStart", Number(GUICtrlRead($collapse_at_start_opt_chk) = $GUI_CHECKED))
				IniWrite($settings_file, "freeshooter", "UseSounds", Number(GUICtrlRead($use_sounds_opt_chk) = $GUI_CHECKED))
				IniWrite($settings_file, "freeshooter", "IncludeCursor", Number(GUICtrlRead($include_cursor_opt_chk) = $GUI_CHECKED))
				
				;Name Settings
				If GUICtrlRead($number_name_radio) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "NameMode", "1")
				If GUICtrlRead($random_name_radio) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "NameMode", "2")
				If GUICtrlRead($date_name_radio) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "NameMode", "3")		

				;Saving Settings
				If GUICtrlRead($save_to_file_radio) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "SavingMode", "1")
				If GUICtrlRead($save_to_clip_radio) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "SavingMode", "2")
					
				;Action Settings
				IniWrite($settings_file, "freeshooter", "EnableIShack", Number(GUICtrlRead($enable_ishack_uploading_chk) = $GUI_CHECKED))
				If GUICtrlRead($do_nothing_chk) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "FinishMode", "1")
				If GUICtrlRead($shellex_chk) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "FinishMode", "2")
				If GUICtrlRead($set_shellex_chk) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "FinishMode", "3")
				IniWrite($settings_file, "freeshooter", "FinishModePath", GUICtrlRead($set_shellex_input))		
				
				;FTP Settings
				IniWrite($settings_file, "FTP", "Enable", Number(GUICtrlRead($enable_ftp_chk) = $GUI_CHECKED))
				IniWrite($settings_file, "FTP", "Server", GUICtrlRead($ftp_server_input))
				IniWrite($settings_file, "FTP", "Username", GUICtrlRead($ftp_name_input))
				IniWrite($settings_file, "FTP", "Password", GUICtrlRead($ftp_password_input))
				IniWrite($settings_file, "FTP", "Folder", GUICtrlRead($ftp_folder_input))
				IniWrite($settings_file, "FTP", "NoFtpGui", Number(GUICtrlRead($no_ftp_gui_chk) = $GUI_CHECKED))
				IniWrite($settings_file, "FTP", "DelAfterUpload", Number(GUICtrlRead($ftp_del_after_upload_chk) = $GUI_CHECKED))
				
				;Image Format
				IniWrite($settings_file, "freeshooter", "ImageFormat", GUICtrlRead($format_combo))

				;Resizeing Image
				IniWrite($settings_file, "freeshooter", "UseResize", Number(GUICtrlRead($resize_enable_chk) = $GUI_CHECKED))
				IniWrite($settings_file, "freeshooter", "ResizeWidth", GuiCtrlRead($width_size))
				IniWrite($settings_file, "freeshooter", "ResizeHeight", GuiCtrlRead($height_size))
				
				;Aero
				IniWrite($settings_file, "freeshooter", "DisableAeroOnWnd", Number(GUICtrlRead($disable_aero_on_wnd_chk) = $GUI_CHECKED))

				;Rotate Image
				If GUICtrlRead($no_rotate_radio) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "RotateMode", "0")
				If GUICtrlRead($right_rotate_radio) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "RotateMode", "1")
				If GUICtrlRead($left_rotate_radio) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "RotateMode", "2")
				If GUICtrlRead($top_rotate_radio) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "RotateMode", "3")
				
				;Delaying
				IniWrite($settings_file, "freeshooter", "DelayUse", Number(GUICtrlRead($use_delay_chk) = $GUI_CHECKED))
				IniWrite($settings_file, "freeshooter", "DelayTime", GuiCtrlRead($delay_input))
				
				;Transparent Window
				IniWrite($settings_file, "freeshooter", "UseTransparent", Number(GUICtrlRead($use_trans_chk) = $GUI_CHECKED))
				IniWrite($settings_file, "freeshooter", "TransparentRatio", GuiCtrlRead($trans_slider))
				
				;Language Settings
				Local $check_lng = IniRead($settings_file, "freeshooter", "Language", GuiCtrlRead($lang_combo))
				Local $check_manifest = IniRead($settings_file, "freeshooter", "UseManifest", "")
				IniWrite($settings_file, "freeshooter", "Language", GuiCtrlRead($lang_combo))
				
				If $check_lng <> GuiCtrlRead($lang_combo) or $use_manifest_read <> $check_manifest Then 
					If MsgBox(64 + 4, T("SettingsWarningTitle", "Внимание"), T("SettingsWarningText", "Для принятия некоторых настроек необходим перезапуск. Перезапустить?")) = 6 Then _RestartApp()
				EndIf

				DllCall('user32.dll', 'int', 'SetActiveWindow', 'hwnd', $hHwnd)
				GUIDelete()
				GUISetState(@SW_ENABLE, $hHwnd)
				_WinAPI_SetFocus(_WinAPI_GetFocus())
				ExitLoop
		EndSelect
	WEnd
EndFunc

Func TRANS_HSCROLL($hWnd, $iMsg, $iwParam, $ilParam)
	GuiCtrlSetData($trans_input, GUICtrlRead($trans_slider))
	WinSetTrans($main_dlg, "", (255 / 100) * GUICtrlRead($trans_slider))
	WinSetTrans($opt_dlg, "", (255 / 100) * GUICtrlRead($trans_slider))
EndFunc

Func read_settings()
	Global $language = IniRead($settings_file, "freeshooter", "Language", "Русский")
	Global $shoot_mode_read = IniRead($settings_file, "freeshooter", "ShootMode", "1")
	Global $collapse_at_start_read = IniRead($settings_file, "freeshooter", "CollapseAtStart", "0")
	Global $use_sounds_read = IniRead($settings_file, "freeshooter", "UseSounds", "1")
	Global $include_cursor_read = IniRead($settings_file, "freeshooter", "IncludeCursor", "0")
	Global $save_path_read = IniRead($settings_file, "freeshooter", "PathToSave", @ScriptDir)
	Global $hotkey_read = IniRead($settings_file, "freeshooter", "HotKey", "CTRL+PrintScreen")
	
	;Last Settings Tab
	Global $last_tab_read = StringLower(IniRead($settings_file, "freeshooter", "LastSettingsTab", "MAIN_TAB"))
	
	;Image Format
	Global $format_read = StringLower(IniRead($settings_file, "freeshooter", "ImageFormat", "PNG"))
	
	;Resizeing Settings
	Global $ur_read = IniRead($settings_file, "freeshooter", "UseResize", "0")
	Global $rh_read = IniRead($settings_file, "freeshooter", "ResizeHeight", "")
	Global $rw_read = IniRead($settings_file, "freeshooter", "ResizeWidth", "")
	
	;Aeros
	Global $disable_aero_on_wnd_read = IniRead($settings_file, "freeshooter", "DisableAeroOnWnd", "0")
	
	;Rotating Images
	Global $rotate_read = IniRead($settings_file, "freeshooter", "RotateMode", "0")
	
	;Delay Settings
	Global $delay_time_read = IniRead($settings_file, "freeshooter", "DelayTime", "3")*1000
	Global $use_delay_read = IniRead($settings_file, "freeshooter", "DelayUse", "0")
	
	;Always On Top
	Global $always_on_top_read = IniRead($settings_file, "freeshooter", "AlwaysOnTop", "0")	
	
	;Name Settings
	Global $name_read = IniRead($settings_file, "freeshooter", "NameMode", "1")
	
	;Saving Settings
	Global $saving_read = IniRead($settings_file, "freeshooter", "SavingMode", "1")
	
	;Transparent Settings
	Global $use_trans_read = IniRead($settings_file, "freeshooter", "UseTransparent", "0")
	Global $trans_ratio_read = IniRead($settings_file, "freeshooter", "TransparentRatio", "100")
	
	;Action's Settings
	Global $enable_ishack_read = IniRead($settings_file, "freeshooter", "EnableIShack", "0")
	Global $finish_mode_read = IniRead($settings_file, "freeshooter", "FinishMode", "1")
	Global $finish_mode_path_read = IniRead($settings_file, "freeshooter", "FinishModePath", "")
	
	;HotKey Settings 
	Global $all_screen_ctrl_read = IniRead($settings_file, "HotKey", "AllScreenCtrl", "0")
	Global $all_screen_alt_read = IniRead($settings_file, "HotKey", "AllScreenAlt", "0")
	Global $all_screen_shift_read = IniRead($settings_file, "HotKey", "AllScreenShift", "0")
	Global $all_screen_extra_read = IniRead($settings_file, "HotKey", "AllScreenExtra", "PrintScreen")
				
	Global $selected_win_ctrl_read = IniRead($settings_file, "HotKey", "SelectedWindowCtrl", "0")
	Global $selected_win_alt_read = IniRead($settings_file, "HotKey", "SelectedWindowAlt", "1")
	Global $selected_win_shift_read = IniRead($settings_file, "HotKey", "SelectedWindowShift", "0")
	Global $selected_win_extra_read = IniRead($settings_file, "HotKey", "SelectedWindowExtra", "PrintScreen")

	Global $region_ctrl_read = IniRead($settings_file, "HotKey", "RegionCtrl", "1")
	Global $region_alt_read = IniRead($settings_file, "HotKey", "RegionAlt", "0")
	Global $region_shift_read = IniRead($settings_file, "HotKey", "RegionShift", "0")
	Global $region_extra_read = IniRead($settings_file, "HotKey", "RegionExtra", "PrintScreen")
	
	Global $hk_all_screen_read = IniRead($settings_file, "freeshooter", "HotKeyAllScreen", "")
	Global $hk_selected_window_read = IniRead($settings_file, "freeshooter", "HotKeySelectedWindow", "")
	Global $hk_region_read = IniRead($settings_file, "freeshooter", "HotKeyRegion", "")
	
	;FTP Settings
	Global $enable_ftp_read = IniRead($settings_file, "FTP", "Enable", "0")
	Global $ftp_server_read = IniRead($settings_file, "FTP", "Server", "")
	Global $ftp_username_read = IniRead($settings_file, "FTP", "Username", "")
	Global $ftp_password_read = IniRead($settings_file, "FTP", "Password", "")
	Global $ftp_folder_read = IniRead($settings_file, "FTP", "Folder", "/")
	Global $no_ftp_gui_read = IniRead($settings_file, "FTP", "NoFtpGui", "0")
	Global $ftp_del_after_upload_read = IniRead($settings_file, "FTP", "DelAfterUpload", "0")

	;Aero Settings
	Global $enable_aero_switch_read = IniRead($settings_file, "freeshooter", "AeroSwitch", "0")
EndFunc

Func save_settings()
	If GUICtrlRead($all_screen_combo) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "ShootMode", "1")
	If GUICtrlRead($selected_window_combo) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "ShootMode", "2")
	If GUICtrlRead($region_shoot_combo) = $GUI_CHECKED Then IniWrite($settings_file, "freeshooter", "ShootMode", "3")
					
	If GUICtrlRead($collapse_at_start_chk) = $GUI_CHECKED Then
		IniWrite($settings_file, "freeshooter", "CollapseAtStart", "1")
	Else
		IniWrite($settings_file, "freeshooter", "CollapseAtStart", "0")
	EndIf	

	If GUICtrlRead($use_sounds_chk) = $GUI_CHECKED Then
		IniWrite($settings_file, "freeshooter", "UseSounds", "1")
	Else
		IniWrite($settings_file, "freeshooter", "UseSounds", "0")
	EndIf	
	
	If GUICtrlRead($include_cursor_chk) = $GUI_CHECKED Then
		IniWrite($settings_file, "freeshooter", "IncludeCursor", "1")
	Else
		IniWrite($settings_file, "freeshooter", "IncludeCursor", "0")
	EndIf	
	
	Local $save_path_setting = GUICtrlRead($path_to_save)
	If $save_path_setting <> "" Then IniWrite($settings_file, "freeshooter", "PathToSave", $save_path_setting)
EndFunc

Func use_settings()
	HotKeySet($hk_all_screen_read, "_ScreenShoot_FullScreen")
	HotKeySet($hk_selected_window_read, "_ScreenShoot_SelectedWindow")
	HotKeySet($hk_region_read, "_ScreenShoot_Region")
	
	GUICtrlSetData($path_to_save, $save_path_read)
	
	If $shoot_mode_read = 1 Then GuiCtrlSetState($all_screen_combo, $GUI_CHECKED)
	If $shoot_mode_read = 2 Then GuiCtrlSetState($selected_window_combo, $GUI_CHECKED)
	If $shoot_mode_read = 3 Then GuiCtrlSetState($region_shoot_combo, $GUI_CHECKED)
	
	If $collapse_at_start_read = 1 Then
		GUICtrlSetState($collapse_at_start_chk, $GUI_CHECKED)
	Else
		GUICtrlSetState($collapse_at_start_chk, $GUI_UNCHECKED)
	EndIf
	If $use_sounds_read = 1 Then
		GUICtrlSetState($use_sounds_chk, $GUI_CHECKED)
	Else
		GUICtrlSetState($use_sounds_chk, $GUI_UNCHECKED)
	EndIf	
	If $include_cursor_read = 1 Then
		GUICtrlSetState($include_cursor_chk, $GUI_CHECKED)
		Global $incl_cursor = True
	Else
		GUICtrlSetState($include_cursor_chk, $GUI_UNCHECKED)
		Global $incl_cursor = False
	EndIf
	If $use_trans_read = 1 Then 
		WinSetTrans($main_dlg, "", (255 / 100) * $trans_ratio_read)
	Else
		WinSetTrans($main_dlg, "", 255)
	EndIf
	If $always_on_top_read = 1 Then 
		WinSetOnTop($main_dlg, "", 1)
	Else
		WinSetOnTop($main_dlg, "", 0)
	EndIf
EndFunc

Func _Hide_Gui()
	TraySetState(2)
	GuiSetState(@SW_HIDE)
EndFunc

Func _Show_Gui($WinState = 0)
	TraySetState(1)
	If not $WinState = 0 Then GuiSetState(@SW_SHOW)
EndFunc

Func minimize_gui()
	TraySetOnEvent($TRAY_EVENT_PRIMARYUP, "maximize_gui")
	GuiSetState(@SW_HIDE)
EndFunc

Func maximize_gui()
	TraySetOnEvent($TRAY_EVENT_PRIMARYUP, "minimize_gui")
	GuiSetState(@SW_SHOW)
	GuiSetState(@SW_RESTORE)
EndFunc

Func _HideWin($hHwnd)
	$hIcon = _WinAPI_GetClassLong($hHwnd, $GCL_HICON)
	_WinAPI_FreeIcon($hIcon)
	_WinAPI_SetClassLong($hHwnd, $GCL_HICON, 0)
	_WinAPI_SetClassLong($hHwnd, $GCL_HICONSM, 0)
EndFunc

Func _GetContentType($sPath)
	$Ext = StringRegExpReplace($sPath, "^.*\.", "")
	$Ct_Read = RegRead("HKEY_CLASSES_ROOT\."& $Ext, "Content Type")
	
	If $Ct_Read = "" Then
		Return
	Else
		Return $Ct_Read
	EndIf
EndFunc

Func _IsFolder($Path)
	Return StringInStr(FileGetAttrib($Path), "D")
EndFunc

Func _FileGetNumberName($Folder, $AppendName, $Ext)
    Local $i = 1
    Local $NumberName = $AppendName &"_"& StringFormat("%03d", $i) &"."& $Ext

    While FileExists($Folder & "\"& $NumberName)
        $i += 1
        $NumberName = $AppendName &"_"& StringFormat("%03d", $i) &"."& $Ext
    Wend
	
	Return $NumberName
EndFunc

Func _FileGetRandomName($sExt, $i_RandomLength = 7)
    Local $s_TempName = ""
 
    While StringLen($s_TempName) < $i_RandomLength
        $s_TempName &= Chr(Random(97, 122, 1))
    WEnd
 
    Return $s_TempName & $sExt
EndFunc

Func _FtpUpload($hFile)
	Global $ftp_open, $ftp_connect, $Bytes = -32
	Global $upload_dlg, $upload_progress, $upload_status
	
	read_settings()
	use_settings()
	
	If $no_ftp_gui_read = 0 Then
		$hParent = GUICreate('', 0, 0, 0, 0, 0, $WS_EX_TOOLWINDOW)
		
		Global $upload_dlg = GUICreate(T("FtpUploading", "Загрузка на FTP сервер..."), 250, 70, @DesktopWidth - (250 + 12), @DesktopHeight - (70 + 53), -2138570752 + 131072, Default, $hParent)
		_HideWin($upload_dlg)
		GUICtrlCreateLabel("", 0, 0, 250, 70, Default, 0x00100000)
		
		If $always_on_top_read = 1 Then WinSetOnTop($upload_dlg, "", 1)
		If $use_trans_read = 1 Then WinSetTrans($upload_dlg, "", (255 / 100) * $trans_ratio_read)
			
		Global $upload_progress = GUICtrlCreateProgress(25, 20, 200, 20)
		Global $upload_status = GUICtrlCreateLabel(T("FtpUploading", "Загрузка на FTP сервер..."), 25, 45, 200, 20)
		GUICtrlSetColor($upload_status, 0xffffff)
		GUISetBkColor ("0x5671c3")
		
		GUISetState(@SW_SHOW)
	EndIf

	Local $filename = StringRegExpReplace($hFile, "^.*\\", "")
	Local $ftp_open = _FTPOpen($application &" "& $version)
	
	If $no_ftp_gui_read = 0 Then
		Local $tData = DllStructCreate('dword')
		DllStructSetData($tData, 1, FileGetSize($hFile))
		Local $hProc = DllCallbackRegister('_InternetStatus', 'none', 'ptr;ptr;dword;ptr;dword')
		DllCall("wininet.dll", 'ptr', 'InternetSetStatusCallback', 'ptr', $ftp_open, 'ptr', DllCallbackGetPtr($hProc))
	EndIf
	
	If $no_ftp_gui_read = 0 Then GUICtrlSetData($upload_status, T("FtpConnecting", "Соединение с сервером..."))
	Local $ftp_connect = _FTPConnect($ftp_open, $ftp_server_read, $ftp_username_read, $ftp_password_read)
	If $ftp_connect = 0 and $no_ftp_gui_read = 0 Then 
		GUICtrlSetData($upload_status, T("FtpNoConnect", "Неудалось подключиться к серверу..."))
		GuiDelete($upload_dlg)
	Else
		GUICtrlSetData($upload_status, T("FtpUploading", "Загрузка файла на сервер..."))
	If $no_ftp_gui_read = 0 Then
		Local $ftp_upload = _FTPPutFile($ftp_connect, $hFile, $ftp_folder_read & $filename, 0, DllStructGetPtr($tData))
	Else
		Local $ftp_upload = _FTPPutFile($ftp_connect, $hFile, $ftp_folder_read & $filename, 0)
	EndIf
	If $ftp_del_after_upload_read = 1 Then
		If $ftp_upload = 1 Then FileDelete($hFile)
	EndIf
	If $no_ftp_gui_read = 0 Then
		DllCall("wininet.dll", 'ptr', 'InternetSetStatusCallback', 'ptr', $ftp_open, 'ptr', 0)
		DllCallbackFree($hProc)
		GuiDelete($upload_dlg)
	EndIf
		Local $ftp_close = _FTPClose($ftp_open)
	EndIf
EndFunc

Func _InternetStatus($hSession, $hContext, $iStatus, $hInformation, $iLenght)
	Local $INTERNET_STATUS_REQUEST_SENT = 31
    Switch $iStatus
        Case $INTERNET_STATUS_REQUEST_SENT

            Local $tFull = DllStructCreate('dword', $hContext)
            Local $tSize = DllStructCreate('dword', $hInformation)

            $Bytes += DllStructGetData($tSize, 1)
            If $Bytes > 0 Then
                GUICtrlSetData($upload_progress, $Bytes / DllStructGetData($tFull, 1) * 100)
            EndIf
    EndSwitch
EndFunc

Func _SaveToClip($hHwnd)
	_ClipBoard_Open(0)
	_ClipBoard_SetData($hHwnd, $CF_BITMAP)
	_ClipBoard_Close()
EndFunc
	
Func _ImageRotate($hFile)
	If $rotate_read = 1 Then _RotateImage($hFile, $hFile, $Rotate90FlipNone)
	If $rotate_read = 2 Then _RotateImage($hFile, $hFile, $Rotate270FlipNone)
	If $rotate_read = 3 Then _RotateImage($hFile, $hFile, $Rotate180FlipNone)
EndFunc

Func _FinishExecute($filepath)
	Local $text1 = StringReplace($finish_mode_path_read, "%s", $filepath)
	Local $text2 = StringReplace($text1, "%f", @ScriptDir)
	If $finish_mode_read = 2 Then ShellExecute($filepath)
	If $finish_mode_read = 3 Then Run($text2)
EndFunc

Func GetRegion()
	Opt("GUICloseOnESC", 0)
	
	Local $iSIZEX = 256, $iSIZEY = 256, $iZoom = 4, $iZoomX = Int($iSIZEX / $iZoom), $iZoomY = Int($iSIZEY / $iZoom)
	Local $hDeskDC = _WinAPI_GetDC(0), $hDC, $aLast_MPos[2] = [-1, -1], $zoom_dlg, $zoom_label_dlg, $crosshair_dlg, $zoom_Label, $error = 1

	Local $aMouse_Pos, $hMask, $hMaster_Mask, $iTemp
	Local $UserDLL = DllOpen("user32.dll")
	
	$hParent = GUICreate('', 0, 0, 0, 0, 0, $WS_EX_TOOLWINDOW)

	$zoom_dlg = GUICreate("", $iSIZEX, $iSIZEY, 0, 0, BitOR($WS_BORDER, $WS_POPUP), $WS_EX_TOPMOST, $hParent)
	_HideWin($zoom_dlg)
	
	GUISetBkColor(0xEEEEEE)
	
	$crosshair_dlg = GUICreate("", @DesktopWidth, @DesktopHeight, 0, 0, $WS_POPUP, $WS_EX_TOOLWINDOW + $WS_EX_TOPMOST, $hParent)
	_HideWin($crosshair_dlg)
	
	GUISetBkColor(0x000000)
	
	$zoom_label_dlg = GUICreate("", $iSIZEX, 60, 0, $iSIZEY-60, BitOR($WS_BORDER, $WS_POPUP), $WS_EX_TOPMOST, $zoom_dlg)
	_HideWin($zoom_label_dlg)
	
	$zoom_label = GUICtrlCreateLabel(T("RegionMouseLocationStartText", 'Нажмите "Левую кнопку мыши" для начала выделения\n\n"Правую кнопку мыши" для выхода из режима'), 5, 3, $iSIZEX-10, 55)

	GUISetState(@SW_SHOW, $zoom_dlg)
	GUISetState(@SW_SHOW, $zoom_label_dlg)
	
	$hCross_GUI = GUICreate("", @DesktopWidth, @DesktopHeight - 20, 0, 0, -2147483648, 8, $hParent)
	_HideWin($hCross_GUI)
	
	WinSetTrans($hCross_GUI, "", 8)
	GUISetState(@SW_SHOW, $hCross_GUI)
	GUISetCursor(3, 1, $hCross_GUI)

	Global $hRectangle_GUI = GUICreate("", @DesktopWidth, @DesktopHeight, 0, 0, -2147483648, 128 + 8, $hParent)
	_HideWin($hRectangle_GUI)
	GUISetBkColor(0x000000)
	
	$hDC = _WinAPI_GetDC($zoom_dlg)
	_WinAPI_SetBkMode($hDC, 1)

	$iZoomX = Int($iSIZEX / $iZoom)
	$iZoomY = Int($iSIZEY / $iZoom)

	While Not _IsPressed("01", $UserDLL)
		Sleep(10)
		
		If _IsPressed("02", $UserDLL) Or _IsPressed("1B", $UserDLL) Then
			$error = 0
			ExitLoop
		EndIf
		
		$aMPos = MouseGetPos()

		If ($aMPos[0] <> $aLast_MPos[0]) Or ($aMPos[1] <> $aLast_MPos[1]) Then
			If $aMPos[0] <= $iSIZEX And $aMPos[1] <= $iSIZEY Then
				WinMove($zoom_dlg, "", (@DesktopWidth - $iSIZEX), 0)
				WinMove($zoom_label_dlg, "", (@DesktopWidth - $iSIZEX), ($iSIZEY - 60))
			ElseIf $aMPos[0] >= (@DesktopWidth - $iSIZEX) And $aMPos[1] <= $iSIZEY Then
				WinMove($zoom_dlg, "", 0, 0)
				WinMove($zoom_label_dlg, "", 0, $iSIZEY - 60)
			EndIf
			
			_WinAPI_StretchBlt($hDC, 0, 0, $iSIZEX, $iSIZEY, $hDeskDC, _
				$aMPos[0] - ($iZoomX / 2), $aMPos[1] - ($iZoomY / 2), $iZoomX, $iZoomY, $SRCCOPY)
			$aLast_MPos = $aMPos
		EndIf

		If _IsPressed("02", $UserDLL) Or _IsPressed("1B", $UserDLL) Then
			GUIDelete($hRectangle_GUI)
			GUIDelete($hCross_GUI)
			DllClose($UserDLL)
			ExitLoop
			$error = 0
		EndIf
	WEnd

	$aMouse_Pos = MouseGetPos()
	$pos_x_1 = $aMouse_Pos[0]
	$pos_y_1 = $aMouse_Pos[1]
	
	$sData = T("RegionMouseLocationText", "Горизонталь [X]: Начало = %i, Длина = %i\nВертикаль [Y]: Начало = %i, Высота = %i\n\nОтпустите клавишу мыши для завершения")
	
	While _IsPressed("01", $UserDLL)
		$aMouse_Pos = MouseGetPos()

		If ($aMouse_Pos[0] <> $aLast_MPos[0]) Or ($aMouse_Pos[1] <> $aLast_MPos[1]) Then
			$nWidth = $aMouse_Pos[0] - $aMPos[0]
			$nHeight = $aMouse_Pos[1] - $aMPos[1]

			If $aMouse_Pos[0] < $aMPos[0] Then $nWidth = $aMPos[0] - $aMouse_Pos[0]
			If $aMouse_Pos[1] < $aMPos[1] Then $nHeight = $aMPos[1] - $aMouse_Pos[1]
				
			GUICtrlSetData($Zoom_Label, StringFormat($sData, $aMPos[0], $nWidth, $aMPos[1], $nHeight))
			
			If $aMouse_Pos[0] <= $iSIZEX And $aMouse_Pos[1] <= $iSIZEY Then
				WinMove($zoom_dlg, "", (@DesktopWidth - $iSIZEX), 0)
				WinMove($zoom_label_dlg, "", (@DesktopWidth - $iSIZEX), ($iSIZEY - 60))
			ElseIf $aMouse_Pos[0] >= (@DesktopWidth - $iSIZEX) And $aMouse_Pos[1] <= $iSIZEY Then
				WinMove($zoom_dlg, "", 0, 0)
				WinMove($zoom_label_dlg, "", 0, $iSIZEY - 60)
			EndIf

			_WinAPI_StretchBlt($hDC, 0, 0, $iSIZEX, $iSIZEY, $hDeskDC, _
				$aMouse_Pos[0] - ($iZoomX / 2), $aMouse_Pos[1] - ($iZoomY / 2), $iZoomX, $iZoomY, $SRCCOPY)
			$aLast_MPos = $aMouse_Pos
		EndIf

		$hMaster_Mask = _WinAPI_CreateRectRgn(0, 0, 0, 0)
		$hMask = _WinAPI_CreateRectRgn($pos_x_1, $aMouse_Pos[1], $aMouse_Pos[0], $aMouse_Pos[1] + 1)
		_WinAPI_CombineRgn($hMaster_Mask, $hMask, $hMaster_Mask, 2)
		$hMask = _WinAPI_CreateRectRgn($pos_x_1, $pos_y_1, $pos_x_1 + 1, $aMouse_Pos[1])
		_WinAPI_CombineRgn($hMaster_Mask, $hMask, $hMaster_Mask, 2)
		$hMask = _WinAPI_CreateRectRgn($pos_x_1 + 1, $pos_y_1 + 1, $aMouse_Pos[0], $pos_y_1)
		_WinAPI_CombineRgn($hMaster_Mask, $hMask, $hMaster_Mask, 2)
		$hMask = _WinAPI_CreateRectRgn($aMouse_Pos[0], $pos_y_1, $aMouse_Pos[0] + 1, $aMouse_Pos[1])
		_WinAPI_CombineRgn($hMaster_Mask, $hMask, $hMaster_Mask, 2)
		_WinAPI_SetWindowRgn($hRectangle_GUI, $hMaster_Mask)
		If WinGetState($hRectangle_GUI) < 15 Then GUISetState()

		Sleep(10)
	WEnd

	$pos_x_2 = $aMouse_Pos[0]
	$pos_y_2 = $aMouse_Pos[1]

	If $pos_x_2 < $pos_x_1 Then
		$iTemp = $pos_x_1
		$pos_x_1 = $pos_x_2
		$pos_x_2 = $iTemp
	EndIf

	If $pos_y_2 < $pos_y_1 Then
		$iTemp = $pos_y_1
		$pos_y_1 = $pos_y_2
		$pos_y_2 = $iTemp
	EndIf

	GUIDelete($hRectangle_GUI)
	GUIDelete($hCross_GUI)
	GUIDelete($zoom_dlg)
	GUIDelete($zoom_label_dlg)
	DllClose($UserDLL)

	_WinAPI_ReleaseDC(0, $hDeskDC)
	_WinAPI_ReleaseDC($zoom_dlg, $hDC)
	
	Opt("GUICloseOnESC", 1)
	Return $error
EndFunc

Func _Get_Hovered_Handle()
	Local $aRet = DllCall("user32.dll", "int", "WindowFromPoint", "long", MouseGetPos(0), "long", MouseGetPos(1))
	If Not IsArray($aRet) Then Return SetError(1, 0, 0)
	Return HWnd($aRet[0])
EndFunc

Func _ReduceMemory()
	DllCall("psapi.dll", "int", "EmptyWorkingSet", "long", -1)
EndFunc

Func _PlayWAVResource($resname)
	Local  Const $SND_RESOURCE = 0x00040004
	Local Const $SND_ASYNC = 1

	DllCall("winmm.dll", "int", "PlaySound", "str", $resname, "hwnd", 0, "int", $SND_RESOURCE)
EndFunc

Func _IsPressed($sHexKey, $vDLL = 'user32.dll')
	Local $a_R = DllCall($vDLL, "int", "GetAsyncKeyState", "int", '0x' & $sHexKey)
	If Not @error And BitAND($a_R[0], 0x8000) = 0x8000 Then Return 1
	Return 0
EndFunc

Func _Mutex($sMutex)
    Local $handle, $lastError
    
    $handle = DllCall("kernel32.dll", "int", "CreateMutex", "int", 0, "long", 1, "str", $sMutex)

    $lastError = DllCall("kernel32.dll", "int", "GetLastError")
    Return $lastError[0] = 183
EndFunc

Func _RestartApp()
	Run('"'& @ScriptFullPath &'"'&" /fromrestart")
	Exit
EndFunc

Func _GetLangList()
	$langlist = ""
	Dim $langarray[1]
	
	$temp = FileFindFirstFile($lang_dir &"\*.lng")
	If $temp <> -1 Then
		Local $langarray = _FileListToArray($lang_dir, "*.lng",1)
		FileClose($temp)
	EndIf
		_ArraySort($langarray)
	For $i = 0 To UBound($langarray)-1
		$langlist&= StringTrimRight($langarray[$i], 4) &"|"
	Next
	
	$langlist = StringTrimRight($langlist, 1)
	If $langlist = "" Then Return $language
	Return $langlist
EndFunc

Func T($text, $default)
	$return = IniRead($default_lang, "Language", $text, $default)
	$processed_n = StringReplace($return, "\n", @CRLF)
	$processed_t = StringReplace($processed_n, "\t", @TAB)
	Return $processed_t
EndFunc

Func _Exit()
	save_settings()
	Exit
EndFunc
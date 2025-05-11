v2.2.1 (11 May 2025)
- fixed dark theme of hotkey dialog
- updated project sdk

v2.2 (17 October 2024)
- revert back windows 7 and windows 8 support
- added mitigations (win10+)
- added dark theme support
- updated project sdk

v2.1.1 (12 February 2024)
- fixed pressing print screen key (or configured hotkey) freezes app (issue #43)
- fixed update installation (issue #42)

v2.1 (4 December 2023)
- dropped windows 7 and windows 8 support
- use windows image coding (wic) library
- improved multi-monitor support
- added right-click context menu on region screen
- added fullscreen mode capture
- added timer feature (issue #27)
- added arm64 build
- fixed overlapped windows rectangle calculation
- fixed update checker bug (issue #25)
- fixed region selection (issue #31)
- fixed shadow size calculation
- fixed painting cursor
- fixed internal bugs
- updated project sdk
- fixed bugs

v2.0.7 (22 October 2018)
- fixed region selection mask device context
- fixed calculating overlapped windows rectangle
- fixed calculating shadow size
- updated project sdk
- updated windows sdk
- fixed bugs

v2.0.6 (9 August 2018)
- set default image format to .png
- fixed copying to clipboard (issue #21)
- fixed playing sounds
- updated project sdk
- updated locale
- cosmetic fixes

v2.0.5 (18 July 2018)
- added autocompletion for folder edit
- fixed capturing context menus
- fixed shadow size
- filename cosmetics

v2.0.4 (17 July 2018)
- added buffered paint to avoid flickering (vista+)
- changed hotkeys error behaviour
- changed hotkeys set (issue #17)
- fixed possible clipboard memory leak
- fixed find active window

v2.0.3 (13 July 2018)
- added include all overlapped windows in result image for window mode
- added shadow for window screenshot (issue #13)
- added global hotkeys indicators for tray menu
- new region selection window
- improved clear window background
- changed shutter sound
- fixed taking shots of uipi windows
- fixed sound synchronization
- fixed ui bugs
- fixed bugs

v2.0.2 (10 July 2018)
- added date and time naming pattern (issue #12)
- middle mouse click to cancel region selection
- set jpeg quality to 100 by default
- fixed directory names portability
- fixed possible region selection duplicates
- fixed windows xp support
- updated project sdk
- fixed ui bugs
- fixed bugs

v2.0.1 (28 January 2018)
- revert "start minimized" option (issue #7)
- fixed hotkeys initialization in some cases
- fixed ui bugs

v2.0 (27 January 2018)
+ code rewritten on c++
+ added clear window background feature (vista+)
+ added multi-monitor support
- updated license gplv2 to gplv3
- removed useless features
- fixed ui bugs
- fixed bugs

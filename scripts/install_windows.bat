@echo off
REM Right-click -> Run as administrator
set DEST=%CommonProgramFiles%\VST3
echo Installing VST3 to "%DEST%" ...
xcopy /E /I /Y "%~dp0VST3\*" "%DEST%\"
echo.
echo Done. In FL Studio: Options ^> Manage plugins ^> Find plugins.
echo In Ableton: Preferences ^> Plug-ins ^> Rescan.
echo The standalone app is in the Standalone folder.
pause

@echo off
setlocal

set PYTHON=C:\Users\rayve\AppData\Local\Programs\Python\Python314\python.exe
set SCRIPT_DIR=%~dp0src

echo Testing telemetry listener...
echo Make sure waterfall.exe is running.
echo.

cd /d "%SCRIPT_DIR%"

echo Step 1: Enable CORR telemetry
"%PYTHON%" send_command.py ENABLE_TELEM CORR

echo.
echo Step 2: Listen for telemetry (Ctrl+C to stop)
"%PYTHON%" test_telemetry.py

pause

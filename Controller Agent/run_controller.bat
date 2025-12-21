@echo off
setlocal

set PYTHON=C:\Users\rayve\AppData\Local\Programs\Python\Python314\python.exe
set PIP=C:\Users\rayve\AppData\Local\Programs\Python\Python314\Scripts\pip.exe
set SCRIPT_DIR=%~dp0src

echo Installing dependencies...
"%PIP%" install numpy scipy

echo.
echo Running Controller Agent...
cd /d "%SCRIPT_DIR%"
"%PYTHON%" controller.py

pause

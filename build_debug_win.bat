@echo off
setlocal

cmake -S . -B builds
if errorlevel 1 exit /b %errorlevel%

cmake --build builds --config Debug
if errorlevel 1 exit /b %errorlevel%
@echo off
setlocal

cmake -S . -B solution -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b %errorlevel%
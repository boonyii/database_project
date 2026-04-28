@echo off
echo Building SC3020 Assignment 1...
g++ main.cpp storage.cpp bplustree.cpp deletion.cpp record.cpp utils.cpp -o main.exe
if %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    pause
    exit /b %ERRORLEVEL%
)
echo Build succeeded. Run main.exe to start.
pause
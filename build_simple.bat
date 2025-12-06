@echo off
echo Simple Build (No CMake Required)
echo ==================================
echo.

REM Check for MSVC
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo Setting up MSVC environment...
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
    if %errorlevel% neq 0 (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
        if %errorlevel% neq 0 (
            echo ERROR: Visual Studio not found!
            echo Please install Visual Studio 2017+ with C++ tools
            echo Or run this from "Developer Command Prompt for VS"
            exit /b 1
        )
    )
)

if not exist build mkdir build
cd build

echo Compiling test_bus.exe...
cl /EHsc /std:c++17 /O2 /I..\include ..\tests\integration_test.cpp /Fe:test_bus.exe
if %errorlevel% neq 0 goto :error

echo Compiling benchmark.exe...
cl /EHsc /std:c++17 /O2 /I..\include ..\tools\benchmark.cpp /Fe:benchmark.exe
if %errorlevel% neq 0 goto :error

echo Compiling publisher.exe...
cl /EHsc /std:c++17 /O2 /I..\include ..\examples\publisher.cpp /Fe:publisher.exe
if %errorlevel% neq 0 goto :error

echo Compiling subscriber.exe...
cl /EHsc /std:c++17 /O2 /I..\include ..\examples\subscriber.cpp /Fe:subscriber.exe
if %errorlevel% neq 0 goto :error

cd ..
echo.
echo ========================================
echo Build complete!
echo ========================================
echo.
echo Run tests:      .\build\test_bus.exe
echo Run benchmark:  .\build\benchmark.exe
echo Run publisher:  .\build\publisher.exe --create
echo Run subscriber: .\build\subscriber.exe
echo.
exit /b 0

:error
cd ..
echo.
echo Build failed!
exit /b 1

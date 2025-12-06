@echo off
echo Building LowLatencyBus...
echo.

if not exist build mkdir build
cd build

echo Detecting compiler...
where cl >nul 2>&1
if %errorlevel% equ 0 (
    echo Using MSVC
    cmake .. -DCMAKE_BUILD_TYPE=Release
) else (
    where g++ >nul 2>&1
    if %errorlevel% equ 0 (
        echo Using MinGW
        cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    ) else (
        echo ERROR: No C++ compiler found!
        echo.
        echo Please install one of:
        echo   1. Visual Studio 2017+ (with C++ tools)
        echo   2. MinGW-w64: https://www.mingw-w64.org/
        echo.
        echo Or use CMake GUI to configure manually.
        cd ..
        exit /b 1
    )
)

if %errorlevel% neq 0 (
    echo CMake configuration failed
    cd ..
    exit /b %errorlevel%
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed
    cd ..
    exit /b %errorlevel%
)

cd ..
echo.
echo ========================================
echo Build complete!
echo ========================================
echo.
echo Executables in: build\Release\ or build\
echo.
echo Run tests:      .\build\Release\test_bus.exe
echo Run benchmark:   .\build\Release\benchmark.exe
echo Run publisher:   .\build\Release\publisher.exe --create
echo Run subscriber:  .\build\Release\subscriber.exe
echo.

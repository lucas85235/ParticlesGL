@echo off
setlocal enabledelayedexpansion

echo ===================================
echo     Building ParticleGL...
echo ===================================

cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
if errorlevel 1 exit /b %errorlevel%

cmake --build build --config Debug -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 exit /b %errorlevel%

echo.
echo ===================================
echo     Running Tests...
echo ===================================

pushd build
ctest -C Debug --output-on-failure
set TEST_RESULT=%errorlevel%
popd
if not "%TEST_RESULT%"=="0" exit /b %TEST_RESULT%

echo.
echo ===================================
echo     Running ParticleGL...
echo ===================================

if exist "build\Debug\ParticleGL.exe" (
    "build\Debug\ParticleGL.exe"
) else if exist "build\ParticleGL.exe" (
    "build\ParticleGL.exe"
) else (
    echo ParticleGL.exe not found in build\Debug or build
    exit /b 1
)

endlocal

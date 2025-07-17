@echo off
setlocal enabledelayedexpansion

REM --------- Настройки ---------
set BUILD_DIR=.project
set SRC_DIR=..
set CONFIGS=Debug Release
REM ------------------------------

REM Создаём каталог сборки, если его нет
if not exist %BUILD_DIR% (
    echo Creating build directory: %BUILD_DIR%
    mkdir %BUILD_DIR%
) else (
    echo Build directory %BUILD_DIR% already exists.
)

REM Переходим в каталог сборки
cd %BUILD_DIR%

REM --- Шаг 1: Установка зависимостей Conan для каждой конфигурации ---
for %%C in (%CONFIGS%) do (
    echo.
    echo Running Conan install for %%C configuration...
    conan install %SRC_DIR% --build=missing -s build_type=%%C -g cmake_multi
    if !errorlevel! neq 0 (
        echo Error during Conan install for %%C. Check the output above.
        pause
        exit /b !errorlevel!
    )
)

REM --- Шаг 2: Генерация проекта CMake (один раз) ---
echo.
echo Running CMake to generate project files...
cmake %SRC_DIR% 
if errorlevel 1 (
    echo Error during CMake generation. Check the output above.
    pause
    exit /b 1
)

REM --- Шаг 3: Сборка проекта по каждой конфигурации ---
for %%C in (%CONFIGS%) do (
    echo.
    echo Building project in %%C configuration...
    cmake --build . --config %%C
    if !errorlevel! neq 0 (
        echo Error building %%C configuration. Check the output above.
        pause
        exit /b !errorlevel!
    )
)

echo.
echo Build completed successfully for all configurations!

pause
endlocal

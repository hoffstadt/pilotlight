@rem do NOT keep changes to environment variables
@setlocal

@pushd %~dp0
@set dir=%~dp0

@rem setup development environment
@set PL_RESULT=[1m[92mSuccessful.[0m
@set PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build;%PATH%
@set PATH=C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build;%PATH%
@set PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build;%PATH%
@set PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build;%PATH%
@set PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise/VC\Auxiliary\Build;%PATH%

@call vcvarsall.bat amd64 > nul

@set PL_CONFIG=Debug
:CheckOpts
@if "%~1"=="-c" (@set PL_CONFIG=%2) & @shift & @shift & @goto CheckOpts

@if "%PL_CONFIG%" equ "debug" ( goto debug )
@if "%PL_CONFIG%" equ "debugdx11" ( goto debugdx11 )


:debug
@set PL_HOT_RELOAD_STATUS=0
@echo off
@if not exist "../out" @mkdir "../out"2>nul (>>../out/pilot_light.exe echo off) && (@set PL_HOT_RELOAD_STATUS=0) || (@set PL_HOT_RELOAD_STATUS=1)
@if %PL_HOT_RELOAD_STATUS% equ 1 (
    @echo.
    @echo [1m[97m[41m--------[42m HOT RELOADING [41m--------[0m
)


@if %PL_HOT_RELOAD_STATUS% equ 0 (
    @echo.
    @if exist "../out/pilotlight.lib" del "../out/pilotlight.lib"
    @if exist "../out/pl_draw_extension.dll" del "../out/pl_draw_extension.dll"
    @if exist "../out/pl_draw_extension_*.dll" del "../out/pl_draw_extension_*.dll"
    @if exist "../out/pl_draw_extension_*.pdb" del "../out/pl_draw_extension_*.pdb"
    @if exist "../out/pl_draw_extension_*.so" del "../out/pl_draw_extension_*.so"
    @if exist "../out/pl_draw_extension_*.pdb" del "../out/pl_draw_extension_*.pdb"
    @if exist "../out/pl_draw_extension_*.so" del "../out/pl_draw_extension_*.so"
    @if exist "../out/pl_draw_extension_*.pdb" del "../out/pl_draw_extension_*.pdb"
    @if exist "../out/app.dll" del "../out/app.dll"
    @if exist "../out/app_*.dll" del "../out/app_*.dll"
    @if exist "../out/app_*.pdb" del "../out/app_*.pdb"
    @if exist "../out/app_*.so" del "../out/app_*.so"
    @if exist "../out/app_*.pdb" del "../out/app_*.pdb"
    @if exist "../out/app_*.so" del "../out/app_*.so"
    @if exist "../out/app_*.pdb" del "../out/app_*.pdb"
    @if exist "../out/pilot_light.exe" del "../out/pilot_light.exe"
)

@rem ################################################################################
@rem #                                debug | pl_lib                                #
@rem ################################################################################

@rem create output directory
@if not exist "../out" @mkdir "../out"

@echo LOCKING > "../out/lock.tmp"

@set PL_DEFINES=
@set PL_DEFINES=-DPL_VULKAN_BACKEND %PL_DEFINES%
@set PL_DEFINES=-D_USE_MATH_DEFINES %PL_DEFINES%
@set PL_DEFINES=-DPL_PROFILING_ON %PL_DEFINES%
@set PL_DEFINES=-DPL_ALLOW_HOT_RELOAD %PL_DEFINES%
@set PL_DEFINES=-DPL_ENABLE_VALIDATION_LAYERS %PL_DEFINES%
@set PL_DEFINES=-D_DEBUG %PL_DEFINES%

@set PL_INCLUDE_DIRECTORIES=
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\um" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\shared" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%DXSDK_DIR%Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%VULKAN_SDK%\Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../out" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../dependencies/stb" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../src" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../extensions" %PL_INCLUDE_DIRECTORIES%

@set PL_LINK_DIRECTORIES=
@set PL_LINK_DIRECTORIES=-LIBPATH:"%VULKAN_SDK%\Lib" %PL_LINK_DIRECTORIES%
@set PL_LINK_DIRECTORIES=-LIBPATH:"../out" %PL_LINK_DIRECTORIES%

@set PL_COMPILER_FLAGS=
@set PL_COMPILER_FLAGS=-Zc:preprocessor %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-nologo %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-std:c11 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-W4 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-WX %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4201 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4100 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4996 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4505 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4189 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd5105 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4115 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-permissive- %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Od %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-MDd %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Zi %PL_COMPILER_FLAGS%

@set PL_LINKER_FLAGS=

@set PL_LINK_LIBRARIES=

@rem run compiler
@echo.
@echo [1m[93mStep: pl_lib[0m
@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m
@echo [1m[36mCompiling...[0m
cl -c %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% pilotlight.c -Fe"../out/pilotlight.c.obj" -Fo"../out/"

@rem check build status
@set PL_BUILD_STATUS=%ERRORLEVEL%
@if %PL_BUILD_STATUS% NEQ 0 (
    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%
    @set PL_RESULT=[1m[91mFailed.[0m
    goto Cleanuppl_lib)

@rem link object files into a shared lib
@echo [1m[36mLinking...[0m
lib -nologo -OUT:"../out/pilotlight.lib" "../out/*.obj"

:Cleanuppl_lib
    @echo [1m[36mCleaning...[0m
    @del "../out/*.obj"  > nul 2> nul

@del "../out/lock.tmp"

@echo.
@echo [36mResult: [0m %PL_RESULT%
@echo [36m~~~~~~~~~~~~~~~~~~~~~~[0m

@rem ################################################################################
@rem #                          debug | pl_draw_extension                           #
@rem ################################################################################

@rem create output directory
@if not exist "../out" @mkdir "../out"

@echo LOCKING > "../out/lock.tmp"

@set PL_DEFINES=
@set PL_DEFINES=-DPL_VULKAN_BACKEND %PL_DEFINES%
@set PL_DEFINES=-D_USE_MATH_DEFINES %PL_DEFINES%
@set PL_DEFINES=-DPL_PROFILING_ON %PL_DEFINES%
@set PL_DEFINES=-DPL_ALLOW_HOT_RELOAD %PL_DEFINES%
@set PL_DEFINES=-DPL_ENABLE_VALIDATION_LAYERS %PL_DEFINES%
@set PL_DEFINES=-D_DEBUG %PL_DEFINES%

@set PL_INCLUDE_DIRECTORIES=
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\um" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\shared" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%DXSDK_DIR%Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%VULKAN_SDK%\Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../out" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../dependencies/stb" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../src" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../extensions" %PL_INCLUDE_DIRECTORIES%

@set PL_LINK_DIRECTORIES=
@set PL_LINK_DIRECTORIES=-LIBPATH:"%VULKAN_SDK%\Lib" %PL_LINK_DIRECTORIES%
@set PL_LINK_DIRECTORIES=-LIBPATH:"../out" %PL_LINK_DIRECTORIES%

@set PL_COMPILER_FLAGS=
@set PL_COMPILER_FLAGS=-Od %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-MDd %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Zi %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Zc:preprocessor %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-nologo %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-std:c11 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-W4 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-WX %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4201 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4100 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4996 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4505 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4189 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd5105 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4115 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-permissive- %PL_COMPILER_FLAGS%

@set PL_LINKER_FLAGS=
@set PL_LINKER_FLAGS=-noimplib %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-noexp %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-incremental:no %PL_LINKER_FLAGS%

@set PL_LINK_LIBRARIES=
@set PL_LINK_LIBRARIES=pilotlight.lib %PL_LINK_LIBRARIES%

@set PL_SOURCES=
@set PL_SOURCES="../extensions/pl_draw_extension.c" %PL_SOURCES%

@rem run compiler
@echo.
@echo [1m[93mStep: pl_draw_extension[0m
@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m
@echo [1m[36mCompiling and Linking...[0m
cl %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% %PL_SOURCES% -Fe"../out/pl_draw_extension.dll" -Fo"../out/" -LD -link %PL_LINKER_FLAGS% -PDB:"../out/pl_draw_extension_%random%.pdb" %PL_LINK_DIRECTORIES% %PL_LINK_LIBRARIES%

@rem check build status
@set PL_BUILD_STATUS=%ERRORLEVEL%
@if %PL_BUILD_STATUS% NEQ 0 (
    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%
    @set PL_RESULT=[1m[91mFailed.[0m
    goto Cleanuppl_draw_extension)


:Cleanuppl_draw_extension
    @echo [1m[36mCleaning...[0m
    @del "../out/*.obj"  > nul 2> nul

@del "../out/lock.tmp"

@echo.
@echo [36mResult: [0m %PL_RESULT%
@echo [36m~~~~~~~~~~~~~~~~~~~~~~[0m

@rem ################################################################################
@rem #                                 debug | app                                  #
@rem ################################################################################

@rem create output directory
@if not exist "../out" @mkdir "../out"

@echo LOCKING > "../out/lock.tmp"

@set PL_DEFINES=
@set PL_DEFINES=-DPL_VULKAN_BACKEND %PL_DEFINES%
@set PL_DEFINES=-D_USE_MATH_DEFINES %PL_DEFINES%
@set PL_DEFINES=-DPL_PROFILING_ON %PL_DEFINES%
@set PL_DEFINES=-DPL_ALLOW_HOT_RELOAD %PL_DEFINES%
@set PL_DEFINES=-DPL_ENABLE_VALIDATION_LAYERS %PL_DEFINES%

@set PL_INCLUDE_DIRECTORIES=
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\um" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\shared" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%DXSDK_DIR%Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%VULKAN_SDK%\Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../out" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../dependencies/stb" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../src" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../extensions" %PL_INCLUDE_DIRECTORIES%

@set PL_LINK_DIRECTORIES=
@set PL_LINK_DIRECTORIES=-LIBPATH:"%VULKAN_SDK%\Lib" %PL_LINK_DIRECTORIES%
@set PL_LINK_DIRECTORIES=-LIBPATH:"../out" %PL_LINK_DIRECTORIES%

@set PL_COMPILER_FLAGS=
@set PL_COMPILER_FLAGS=-Od %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-MDd %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Zi %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Zc:preprocessor %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-nologo %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-std:c11 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-W4 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-WX %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4201 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4100 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4996 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4505 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4189 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd5105 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4115 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-permissive- %PL_COMPILER_FLAGS%

@set PL_LINKER_FLAGS=
@set PL_LINKER_FLAGS=-noimplib %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-noexp %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-incremental:no %PL_LINKER_FLAGS%

@set PL_LINK_LIBRARIES=
@set PL_LINK_LIBRARIES=pilotlight.lib %PL_LINK_LIBRARIES%

@set PL_SOURCES=
@set PL_SOURCES="app_vulkan.c" %PL_SOURCES%

@rem run compiler
@echo.
@echo [1m[93mStep: app[0m
@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m
@echo [1m[36mCompiling and Linking...[0m
cl %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% %PL_SOURCES% -Fe"../out/app.dll" -Fo"../out/" -LD -link %PL_LINKER_FLAGS% -PDB:"../out/app_%random%.pdb" %PL_LINK_DIRECTORIES% %PL_LINK_LIBRARIES%

@rem check build status
@set PL_BUILD_STATUS=%ERRORLEVEL%
@if %PL_BUILD_STATUS% NEQ 0 (
    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%
    @set PL_RESULT=[1m[91mFailed.[0m
    goto Cleanupapp)


:Cleanupapp
    @echo [1m[36mCleaning...[0m
    @del "../out/*.obj"  > nul 2> nul

@del "../out/lock.tmp"

@echo.
@echo [36mResult: [0m %PL_RESULT%
@echo [36m~~~~~~~~~~~~~~~~~~~~~~[0m

@rem ################################################################################
@rem #                             debug | pilot_light                              #
@rem ################################################################################

@rem create output directory
@if not exist "../out" @mkdir "../out"

@echo LOCKING > "../out/lock.tmp"

@set PL_DEFINES=
@set PL_DEFINES=-DPL_VULKAN_BACKEND %PL_DEFINES%
@set PL_DEFINES=-D_USE_MATH_DEFINES %PL_DEFINES%
@set PL_DEFINES=-DPL_PROFILING_ON %PL_DEFINES%
@set PL_DEFINES=-DPL_ALLOW_HOT_RELOAD %PL_DEFINES%
@set PL_DEFINES=-DPL_ENABLE_VALIDATION_LAYERS %PL_DEFINES%
@set PL_DEFINES=-D_DEBUG %PL_DEFINES%

@set PL_INCLUDE_DIRECTORIES=
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\um" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\shared" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%DXSDK_DIR%Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%VULKAN_SDK%\Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../out" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../dependencies/stb" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../src" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../extensions" %PL_INCLUDE_DIRECTORIES%

@set PL_LINK_DIRECTORIES=
@set PL_LINK_DIRECTORIES=-LIBPATH:"%VULKAN_SDK%\Lib" %PL_LINK_DIRECTORIES%
@set PL_LINK_DIRECTORIES=-LIBPATH:"../out" %PL_LINK_DIRECTORIES%

@set PL_COMPILER_FLAGS=
@set PL_COMPILER_FLAGS=-Zc:preprocessor %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-nologo %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-std:c11 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-W4 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-WX %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4201 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4100 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4996 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4505 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4189 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd5105 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4115 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-permissive- %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Od %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-MDd %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Zi %PL_COMPILER_FLAGS%

@set PL_LINKER_FLAGS=
@set PL_LINKER_FLAGS=-noimplib %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-noexp %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-incremental:no %PL_LINKER_FLAGS%

@set PL_LINK_LIBRARIES=
@set PL_LINK_LIBRARIES=pilotlight.lib %PL_LINK_LIBRARIES%

@set PL_SOURCES=
@set PL_SOURCES="pl_main_win32.c" %PL_SOURCES%

@if %PL_HOT_RELOAD_STATUS% equ 1 ( goto Cleanuppilot_light )
@rem run compiler
@echo.
@echo [1m[93mStep: pilot_light[0m
@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m
@echo [1m[36mCompiling and Linking...[0m
cl %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% %PL_SOURCES% -Fe"../out/pilot_light.exe" -Fo"../out/" -link %PL_LINKER_FLAGS% %PL_LINK_DIRECTORIES% %PL_LINK_LIBRARIES%

@rem check build status
@set PL_BUILD_STATUS=%ERRORLEVEL%
@if %PL_BUILD_STATUS% NEQ 0 (
    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%
    @set PL_RESULT=[1m[91mFailed.[0m
    goto Cleanuppilot_light
)


:Cleanuppilot_light
    @echo [1m[36mCleaning...[0m
    @del "../out/*.obj"  > nul 2> nul


@del "../out/lock.tmp"

@echo.
@echo [36mResult: [0m %PL_RESULT%
@echo [36m~~~~~~~~~~~~~~~~~~~~~~[0m

goto ExitLabel
:debugdx11
@set PL_HOT_RELOAD_STATUS=0
@echo off
@if not exist "../out" @mkdir "../out"2>nul (>>../out/pilot_light.exe echo off) && (@set PL_HOT_RELOAD_STATUS=0) || (@set PL_HOT_RELOAD_STATUS=1)
@if %PL_HOT_RELOAD_STATUS% equ 1 (
    @echo.
    @echo [1m[97m[41m--------[42m HOT RELOADING [41m--------[0m
)


@if %PL_HOT_RELOAD_STATUS% equ 0 (
    @echo.
    @if exist "../out/pilotlight.lib" del "../out/pilotlight.lib"
    @if exist "../out/pl_draw_extension.dll" del "../out/pl_draw_extension.dll"
    @if exist "../out/pl_draw_extension_*.dll" del "../out/pl_draw_extension_*.dll"
    @if exist "../out/pl_draw_extension_*.pdb" del "../out/pl_draw_extension_*.pdb"
    @if exist "../out/app.dll" del "../out/app.dll"
    @if exist "../out/app_*.dll" del "../out/app_*.dll"
    @if exist "../out/app_*.pdb" del "../out/app_*.pdb"
    @if exist "../out/pilot_light.exe" del "../out/pilot_light.exe"
)

@rem ################################################################################
@rem #                              debugdx11 | pl_lib                              #
@rem ################################################################################

@rem create output directory
@if not exist "../out" @mkdir "../out"

@echo LOCKING > "../out/lock.tmp"

@set PL_DEFINES=
@set PL_DEFINES=-DPL_VULKAN_BACKEND %PL_DEFINES%
@set PL_DEFINES=-D_USE_MATH_DEFINES %PL_DEFINES%
@set PL_DEFINES=-DPL_PROFILING_ON %PL_DEFINES%
@set PL_DEFINES=-DPL_ALLOW_HOT_RELOAD %PL_DEFINES%
@set PL_DEFINES=-DPL_ENABLE_VALIDATION_LAYERS %PL_DEFINES%

@set PL_INCLUDE_DIRECTORIES=
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\um" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\shared" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%DXSDK_DIR%Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%VULKAN_SDK%\Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../out" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../dependencies/stb" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../src" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../extensions" %PL_INCLUDE_DIRECTORIES%

@set PL_LINK_DIRECTORIES=
@set PL_LINK_DIRECTORIES=-LIBPATH:"%VULKAN_SDK%\Lib" %PL_LINK_DIRECTORIES%
@set PL_LINK_DIRECTORIES=-LIBPATH:"../out" %PL_LINK_DIRECTORIES%

@set PL_COMPILER_FLAGS=
@set PL_COMPILER_FLAGS=-Zc:preprocessor %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-nologo %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-std:c11 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-W4 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-WX %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4201 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4100 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4996 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4505 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4189 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd5105 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4115 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-permissive- %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-O2 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-MD %PL_COMPILER_FLAGS%

@set PL_LINKER_FLAGS=

@set PL_LINK_LIBRARIES=

@rem run compiler
@echo.
@echo [1m[93mStep: pl_lib[0m
@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m
@echo [1m[36mCompiling...[0m
cl -c %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% pilotlight.c -Fe"../out/pilotlight.c.obj" -Fo"../out/"

@rem check build status
@set PL_BUILD_STATUS=%ERRORLEVEL%
@if %PL_BUILD_STATUS% NEQ 0 (
    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%
    @set PL_RESULT=[1m[91mFailed.[0m
    goto Cleanuppl_lib)

@rem link object files into a shared lib
@echo [1m[36mLinking...[0m
lib -nologo -OUT:"../out/pilotlight.lib" "../out/*.obj"

:Cleanuppl_lib
    @echo [1m[36mCleaning...[0m
    @del "../out/*.obj"  > nul 2> nul

@del "../out/lock.tmp"

@echo.
@echo [36mResult: [0m %PL_RESULT%
@echo [36m~~~~~~~~~~~~~~~~~~~~~~[0m

@rem ################################################################################
@rem #                        debugdx11 | pl_draw_extension                         #
@rem ################################################################################

@rem create output directory
@if not exist "../out" @mkdir "../out"

@echo LOCKING > "../out/lock.tmp"

@set PL_DEFINES=
@set PL_DEFINES=-DPL_VULKAN_BACKEND %PL_DEFINES%
@set PL_DEFINES=-D_USE_MATH_DEFINES %PL_DEFINES%
@set PL_DEFINES=-DPL_PROFILING_ON %PL_DEFINES%
@set PL_DEFINES=-DPL_ALLOW_HOT_RELOAD %PL_DEFINES%
@set PL_DEFINES=-DPL_ENABLE_VALIDATION_LAYERS %PL_DEFINES%
@set PL_DEFINES=-D_DEBUG %PL_DEFINES%

@set PL_INCLUDE_DIRECTORIES=
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\um" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\shared" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%DXSDK_DIR%Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%VULKAN_SDK%\Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../out" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../dependencies/stb" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../src" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../extensions" %PL_INCLUDE_DIRECTORIES%

@set PL_LINK_DIRECTORIES=
@set PL_LINK_DIRECTORIES=-LIBPATH:"%VULKAN_SDK%\Lib" %PL_LINK_DIRECTORIES%
@set PL_LINK_DIRECTORIES=-LIBPATH:"../out" %PL_LINK_DIRECTORIES%

@set PL_COMPILER_FLAGS=
@set PL_COMPILER_FLAGS=-Od %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-MDd %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Zi %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Zc:preprocessor %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-nologo %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-std:c11 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-W4 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-WX %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4201 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4100 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4996 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4505 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4189 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd5105 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4115 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-permissive- %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-O2 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-MD %PL_COMPILER_FLAGS%

@set PL_LINKER_FLAGS=
@set PL_LINKER_FLAGS=-noimplib %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-noexp %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-incremental:no %PL_LINKER_FLAGS%

@set PL_LINK_LIBRARIES=
@set PL_LINK_LIBRARIES=pilotlight.lib %PL_LINK_LIBRARIES%

@set PL_SOURCES=
@set PL_SOURCES="../extensions/pl_draw_extension.c" %PL_SOURCES%

@rem run compiler
@echo.
@echo [1m[93mStep: pl_draw_extension[0m
@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m
@echo [1m[36mCompiling and Linking...[0m
cl %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% %PL_SOURCES% -Fe"../out/pl_draw_extension.dll" -Fo"../out/" -LD -link %PL_LINKER_FLAGS% -PDB:"../out/pl_draw_extension_%random%.pdb" %PL_LINK_DIRECTORIES% %PL_LINK_LIBRARIES%

@rem check build status
@set PL_BUILD_STATUS=%ERRORLEVEL%
@if %PL_BUILD_STATUS% NEQ 0 (
    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%
    @set PL_RESULT=[1m[91mFailed.[0m
    goto Cleanuppl_draw_extension)


:Cleanuppl_draw_extension
    @echo [1m[36mCleaning...[0m
    @del "../out/*.obj"  > nul 2> nul

@del "../out/lock.tmp"

@echo.
@echo [36mResult: [0m %PL_RESULT%
@echo [36m~~~~~~~~~~~~~~~~~~~~~~[0m

@rem ################################################################################
@rem #                               debugdx11 | app                                #
@rem ################################################################################

@rem create output directory
@if not exist "../out" @mkdir "../out"

@echo LOCKING > "../out/lock.tmp"

@set PL_DEFINES=
@set PL_DEFINES=-DPL_VULKAN_BACKEND %PL_DEFINES%
@set PL_DEFINES=-D_USE_MATH_DEFINES %PL_DEFINES%
@set PL_DEFINES=-DPL_PROFILING_ON %PL_DEFINES%
@set PL_DEFINES=-DPL_ALLOW_HOT_RELOAD %PL_DEFINES%
@set PL_DEFINES=-DPL_ENABLE_VALIDATION_LAYERS %PL_DEFINES%

@set PL_INCLUDE_DIRECTORIES=
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\um" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\shared" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%DXSDK_DIR%Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%VULKAN_SDK%\Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../out" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../dependencies/stb" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../src" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../extensions" %PL_INCLUDE_DIRECTORIES%

@set PL_LINK_DIRECTORIES=
@set PL_LINK_DIRECTORIES=-LIBPATH:"%VULKAN_SDK%\Lib" %PL_LINK_DIRECTORIES%
@set PL_LINK_DIRECTORIES=-LIBPATH:"../out" %PL_LINK_DIRECTORIES%

@set PL_COMPILER_FLAGS=
@set PL_COMPILER_FLAGS=-Zc:preprocessor %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-nologo %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-std:c11 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-W4 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-WX %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4201 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4100 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4996 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4505 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4189 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd5105 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4115 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-permissive- %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Od %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-MDd %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Zi %PL_COMPILER_FLAGS%

@set PL_LINKER_FLAGS=
@set PL_LINKER_FLAGS=-noimplib %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-noexp %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-incremental:no %PL_LINKER_FLAGS%

@set PL_LINK_LIBRARIES=
@set PL_LINK_LIBRARIES=pilotlight.lib %PL_LINK_LIBRARIES%

@set PL_SOURCES=
@set PL_SOURCES="app_dx11.c" %PL_SOURCES%

@rem run compiler
@echo.
@echo [1m[93mStep: app[0m
@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m
@echo [1m[36mCompiling and Linking...[0m
cl %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% %PL_SOURCES% -Fe"../out/app.dll" -Fo"../out/" -LD -link %PL_LINKER_FLAGS% -PDB:"../out/app_%random%.pdb" %PL_LINK_DIRECTORIES% %PL_LINK_LIBRARIES%

@rem check build status
@set PL_BUILD_STATUS=%ERRORLEVEL%
@if %PL_BUILD_STATUS% NEQ 0 (
    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%
    @set PL_RESULT=[1m[91mFailed.[0m
    goto Cleanupapp)


:Cleanupapp
    @echo [1m[36mCleaning...[0m
    @del "../out/*.obj"  > nul 2> nul

@del "../out/lock.tmp"

@echo.
@echo [36mResult: [0m %PL_RESULT%
@echo [36m~~~~~~~~~~~~~~~~~~~~~~[0m

@rem ################################################################################
@rem #                           debugdx11 | pilot_light                            #
@rem ################################################################################

@rem create output directory
@if not exist "../out" @mkdir "../out"

@echo LOCKING > "../out/lock.tmp"

@set PL_DEFINES=
@set PL_DEFINES=-DPL_VULKAN_BACKEND %PL_DEFINES%
@set PL_DEFINES=-D_USE_MATH_DEFINES %PL_DEFINES%
@set PL_DEFINES=-DPL_PROFILING_ON %PL_DEFINES%
@set PL_DEFINES=-DPL_ALLOW_HOT_RELOAD %PL_DEFINES%
@set PL_DEFINES=-DPL_ENABLE_VALIDATION_LAYERS %PL_DEFINES%

@set PL_INCLUDE_DIRECTORIES=
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\um" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%WindowsSdkDir%Include\shared" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%DXSDK_DIR%Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"%VULKAN_SDK%\Include" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../out" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../dependencies/stb" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../src" %PL_INCLUDE_DIRECTORIES%
@set PL_INCLUDE_DIRECTORIES=-I"../extensions" %PL_INCLUDE_DIRECTORIES%

@set PL_LINK_DIRECTORIES=
@set PL_LINK_DIRECTORIES=-LIBPATH:"%VULKAN_SDK%\Lib" %PL_LINK_DIRECTORIES%
@set PL_LINK_DIRECTORIES=-LIBPATH:"../out" %PL_LINK_DIRECTORIES%

@set PL_COMPILER_FLAGS=
@set PL_COMPILER_FLAGS=-Zc:preprocessor %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-nologo %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-std:c11 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-W4 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-WX %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4201 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4100 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4996 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4505 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4189 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd5105 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-wd4115 %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-permissive- %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Od %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-MDd %PL_COMPILER_FLAGS%
@set PL_COMPILER_FLAGS=-Zi %PL_COMPILER_FLAGS%

@set PL_LINKER_FLAGS=
@set PL_LINKER_FLAGS=-noimplib %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-noexp %PL_LINKER_FLAGS%
@set PL_LINKER_FLAGS=-incremental:no %PL_LINKER_FLAGS%

@set PL_LINK_LIBRARIES=
@set PL_LINK_LIBRARIES=pilotlight.lib %PL_LINK_LIBRARIES%

@set PL_SOURCES=
@set PL_SOURCES="pl_main_win32.c" %PL_SOURCES%

@if %PL_HOT_RELOAD_STATUS% equ 1 ( goto Cleanuppilot_light )
@rem run compiler
@echo.
@echo [1m[93mStep: pilot_light[0m
@echo [1m[93m~~~~~~~~~~~~~~~~~~~~~~[0m
@echo [1m[36mCompiling and Linking...[0m
cl %PL_INCLUDE_DIRECTORIES% %PL_DEFINES% %PL_COMPILER_FLAGS% %PL_SOURCES% -Fe"../out/pilot_light.exe" -Fo"../out/" -link %PL_LINKER_FLAGS% %PL_LINK_DIRECTORIES% %PL_LINK_LIBRARIES%

@rem check build status
@set PL_BUILD_STATUS=%ERRORLEVEL%
@if %PL_BUILD_STATUS% NEQ 0 (
    @echo [1m[91mCompilation Failed with error code[0m: %PL_BUILD_STATUS%
    @set PL_RESULT=[1m[91mFailed.[0m
    goto Cleanuppilot_light
)


:Cleanuppilot_light
    @echo [1m[36mCleaning...[0m
    @del "../out/*.obj"  > nul 2> nul


@del "../out/lock.tmp"

@echo.
@echo [36mResult: [0m %PL_RESULT%
@echo [36m~~~~~~~~~~~~~~~~~~~~~~[0m

goto ExitLabel
:ExitLabel
@popd
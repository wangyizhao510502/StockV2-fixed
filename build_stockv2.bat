@echo off
REM =====================================================================
REM  StockV2 x64 Release 一键编译脚本（修复版源码）
REM  用法：在「x64 Native Tools Command Prompt for VS 2022」中，
REM        进入本目录 (TrafficMonitorPlugins\) 运行  build_stockv2.bat
REM  前置：已按编译指南第 2 步用 v143 静态编译 wxWidgets x64，
REM        并设置 WXWIN 指向其根目录。
REM =====================================================================
setlocal EnableExtensions
cd /d "%~dp0"

echo ============================================================
echo   StockV2 x64 Release 一键编译
echo ============================================================

REM --- 1. 查找 Visual Studio 2022 安装路径 ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo [错误] 未找到 vswhere，请确认已安装 Visual Studio 2022。
  goto :fail
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -property installationPath`) do set "VSINSTALL=%%i"
if "%VSINSTALL%"=="" (
  echo [错误] 未检测到 Visual Studio 2022（含 MSBuild 组件）。
  goto :fail
)
echo [OK] VS 安装路径: %VSINSTALL%

REM --- 2. 初始化 v143 x64 编译环境 ---
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
where cl >nul 2>&1
if errorlevel 1 (
  echo [错误] 无法初始化 v143 x64 编译环境（cl.exe 不在 PATH）。
  echo         请在「x64 Native Tools Command Prompt for VS 2022」中运行本脚本。
  goto :fail
)
echo [OK] 编译环境就绪 (cl.exe = %VSINSTALL%\VC\Tools...)

REM --- 3. 检查 wxWidgets 静态库 ---
if "%WXWIN%"=="" (
  echo [提示] 未设置 WXWIN，回退到默认 C:\wxWidgets-3.2.10
  set "WXWIN=C:\wxWidgets-3.2.10"
)
if not exist "%WXWIN%\lib\vc_x64_lib" (
  echo [错误] 未找到 wxWidgets 静态库: %WXWIN%\lib\vc_x64_lib
  echo         请按指南第 2 步用 v143 + SHARED=0 + TARGET_CPU=x64 编译 wxWidgets，并设置 WXWIN。
  goto :fail
)
echo [OK] wxWidgets 静态库: %WXWIN%\lib\vc_x64_lib

REM --- 4. 编译 x64 Release（含 NuGet 还原 Microsoft.Web.WebView2）---
echo.
echo [编译] msbuild TrafficMonitorPlugins.sln /p:Configuration=Release /p:Platform=x64 /t:StockV2 /restore
msbuild TrafficMonitorPlugins.sln /p:Configuration=Release /p:Platform=x64 /t:StockV2 /restore /v:minimal
if errorlevel 1 (
  echo [错误] 编译失败，请查看上方输出。
  goto :fail
)

REM --- 5. 定位产物 ---
echo.
echo [完成] 编译成功。自动查找产物：
set "FOUND="
for /r "." %%f in (StockV2.dll) do (
  echo   %%~ff
  set "FOUND=1"
)
if not defined FOUND echo   (未在仓库下找到 StockV2.dll，请手动在输出目录查找)

echo.
echo 部署（详见 StockV2_本地编译指南.md）：
echo   1. 把 StockV2.dll 重命名为 StockV2_x64.dll，放入 TrafficMonitor\plugins\
echo   2. 取 WebView2Loader.dll（去掉 _x64 后缀），放入 TrafficMonitor.exe 主目录
echo.
goto :eof

:fail
echo.
echo [中止] 编译未能进行，请按提示修复环境后重试。
exit /b 1

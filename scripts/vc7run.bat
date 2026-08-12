@echo off
if not defined DEVENV_PREFIX exit /b 2
set VS_BASE_PATH=%DEVENV_PREFIX%\Program Files\MICROSOFT VISUAL STUDIO .NET
set MS_VC_PATH=%VS_BASE_PATH%\VC7
set DXSDK_DIR=%DEVENV_PREFIX%\mssdk
set PATH=%MS_VC_PATH%\Bin;%VS_BASE_PATH%\FrameworkSDK\Bin;%VS_BASE_PATH%\Common7\IDE;%PATH%
set INCLUDE=%MS_VC_PATH%\INCLUDE;%MS_VC_PATH%\PlatformSDK\include\prerelease;%MS_VC_PATH%\PlatformSDK\include;%MS_VC_PATH%\PlatformSDK\common\include\prerelease;%MS_VC_PATH%\PlatformSDK\common\include;%DXSDK_DIR%\include
set LIB=%MS_VC_PATH%\LIB;%MS_VC_PATH%\PlatformSDK\lib\prerelease;%MS_VC_PATH%\PlatformSDK\lib;%MS_VC_PATH%\PlatformSDK\common\lib\prerelease;%MS_VC_PATH%\PlatformSDK\common\lib;%MS_VC_PATH%\PlatformSDK\x86\lib;%DXSDK_DIR%\lib
%*
exit /b %errorlevel%

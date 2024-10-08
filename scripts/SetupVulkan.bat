@echo off

IF not "%VULKAN_SDK%" == "" (
	echo Vulkan already installed!
	PAUSE
	EXIT /b
)

pushd %~dp0\..\

set version=1.3.290.0
set dir=./NormalMaker/vendor/VulkanSDK

set /p "r=Would you like to install Vulkan SDK? [Y/N]: "

IF not "%r%" == "y" IF not "%r%" == "Y" EXIT /b

set "url=https://sdk.lunarg.com/sdk/download/%version%/windows/VulkanSDK-%version%-Installer.exe?Human=true"
set "path=%dir%/VulkanSDK-%version%-Installer.exe"

IF not exist "%dir%" mkdir "%dir%"

call "%WINDIR%\system32\curl.exe" --ssl-no-revoke -o "%path%" "%url%"

call "%path%"

popd
PAUSE

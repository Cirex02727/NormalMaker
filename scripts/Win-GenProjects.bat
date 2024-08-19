@echo off

IF "%VULKAN_SDK%"=="" call "SetupVulkan.bat"

pushd %~dp0\..\
call vendor\premake\premake5.exe vs2022
popd

PAUSE

@echo off

for /f %%f in ('dir /b /a-d %cd%\res\shaders') do "%VULKAN_SDK%\Bin\glslc.exe" %cd%\res\shaders\%%f -o %cd%\res\shaders\spirv\%%f.spv --target-spv=spv1.3

echo Compiled!
pause

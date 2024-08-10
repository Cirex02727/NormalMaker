project "NativeFileDialog"
	kind "StaticLib"
	language "C++"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"src/common.h",
		"src/nfd_common.h",
		"src/nfd_common.c",
		
		"src/include/nfd.h"
	}
	
	includedirs
	{
		"src/include"
	}
	
	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}
	
	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "On"
		
		files
		{
			"src/nfd_win.cpp",
		}
	
	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "On"
		
		files
		{
			"src/nfd_gtk.c",
			"src/simple_exec.h"
		}
	
	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
	
	filter "configurations:Release"
		runtime "Release"
		optimize "on"

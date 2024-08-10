project "NormalMaker"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"
	
	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")
	
	pchheader "vkpch.h"
	pchsource "src/vkpch.cpp"
	
	files
	{
		"src/**.h",
		"src/**.cpp",
		"vendor/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl"
	}
	
	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}
	
	includedirs
	{
		"src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.NativeFileDialog}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.VulkanSDK}"
	}
	
	links
	{
		"GLFW",
		"NativeFileDialog",
		"ImGui",
		"yaml-cpp",
		"%{Library.Vulkan}"
	}
	
	postbuildcommands "%{wks.location}/CompileShaders.bat"
	
	filter "system:windows"
		systemversion "latest"
	
	filter "configurations:Debug"
		defines "VK_DEBUG"
		runtime "Debug"
		symbols "on"
	
	filter "configurations:Release"
		defines "VK_RELEASE"
		runtime "Release"
		optimize "on"
	
	filter "configurations:Dist"
		defines "VK_DIST"
		runtime "Release"
		optimize "on"

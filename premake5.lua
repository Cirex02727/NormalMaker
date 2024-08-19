include "Dependencies.lua"

require "cmake"

workspace "NormalMaker"
	architecture "x86_64"
	startproject "NormalMaker"
	
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}
	
	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "vendor/premake"
	include "NormalMaker/vendor/GLFW"
	include "NormalMaker/vendor/NativeFileDialog"
	include "NormalMaker/vendor/ImGui"
	include "NormalMaker/vendor/yaml-cpp"
group ""

group "Core"
	include "NormalMaker"
group ""

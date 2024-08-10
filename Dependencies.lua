
-- NormalMaker Dependencies

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["stb_image"] = "%{wks.location}/NormalMaker/vendor/stb_image"
IncludeDir["yaml_cpp"] = "%{wks.location}/NormalMaker/vendor/yaml-cpp/include"
IncludeDir["GLFW"] = "%{wks.location}/NormalMaker/vendor/GLFW/include"
IncludeDir["NativeFileDialog"] = "%{wks.location}/NormalMaker/vendor/NativeFileDialog/src/include"
IncludeDir["ImGui"] = "%{wks.location}/NormalMaker/vendor/ImGui"
IncludeDir["glm"] = "%{wks.location}/NormalMaker/vendor/glm"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

Library = {}

Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"

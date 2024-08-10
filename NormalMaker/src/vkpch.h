#pragma once

#define NOMINMAX
#include <Windows.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define GLFW_VULKAN_STATIC
#include <GLFW/glfw3.h>

#include <nfd.h>

#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image.h>
#include <stb_image_write.h>

#include "core/Core.h"
#include "core/VulkanUtils.h"

#include "core/ApplicationLayer.h"
#include "core/Image.h"
#include "core/Buffer.h"
#include "core/Shader.h"
#include "core/Camera.h"
#include "core/Window.h"
#include "core/Texture.h"
#include "core/SaveManager.h"
#include "core/Application.h"
#include "core/DebugRenderer.h"

#include "input/Input.h"

#include "utils/Utils.h"
#include "utils/Timer.h"
#include "utils/Benchmark.h"
#include "utils/ThreadPool.h"

#include <array>
#include <queue>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <thread>
#include <future>
#include <fstream>
#include <stdio.h>
#include <functional>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <condition_variable>

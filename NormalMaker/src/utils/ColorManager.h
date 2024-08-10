#pragma once

#include <glm/glm.hpp>

namespace Color
{
	glm::ivec3 NormalToColor(const glm::vec3& norm)
	{
		return glm::floor((glm::normalize(norm) * 0.5f + 0.5f) * 255.0f);
	}

	std::string ColorToHex(const glm::ivec3& color)
	{
		return std::format("{:0>2x}", color.x) +
			   std::format("{:0>2x}", color.y) +
			   std::format("{:0>2x}", color.z);
	}
}

#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <typeindex>
#include <any>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <queue>
#include <array>
#include <functional>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <filesystem>


// set condition
#define SET_BITS(var, mask)     ((var) |= (mask))
// delete condition
#define CLEAR_BITS(var, mask)   ((var) &= ~(mask))
// toggle condition
#define TOGGLE_BITS(var, mask)  ((var) ^= (mask))
// check if condition is set
#define IS_BIT_SET(var, mask)   (((var) & (mask)) != 0)


constexpr float TWO_THIRDS{ 0.666666f };
constexpr float THREE_FOURTHS{ 0.75000f };

constexpr int DEFAULT_WIDTH{ 1600 };
constexpr int DEFAULT_HEIGHT{ 900 };

constexpr float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
	// positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
};

// vertices for a unit cube
constexpr float cubeMapVertices[] = {
	// positions          
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};

struct Vertex
{
	glm::vec3 position{};
	glm::vec3 normal{};
	glm::vec2 texCoord{};
	glm::vec4 tangent{};
};

struct materialKeyFactors
{
	glm::vec4 baseColorFactor{ glm::vec4(1.0f)};
	float metallicFactor{ 1.0f };
	float roughnessFactor{ 1.0f };
	glm::vec3 emissiveFactor{ glm::vec3(0.0f) };
	float normalScale{ 1.0f };
	float occlusionStrength{ 1.0f };
};

#endif // !UTIL_H
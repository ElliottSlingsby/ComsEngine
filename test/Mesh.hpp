#pragma once

#include "Component.hpp"

#include <string>

struct Mesh : public Component<Mesh>{
	Mesh(const std::string& source) : source(source){}

	const std::string source;

	unsigned int meshId = 0;

	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int depth = 0;

	unsigned int vertexCount = 0;
	unsigned int faceCount = 0;
	unsigned int uvCount = 0;

	struct Origin{
		float x = 0.5f;
		float y = 0.5f;
		float z = 0.5f;
	} origin;

	bool loaded = false;
};
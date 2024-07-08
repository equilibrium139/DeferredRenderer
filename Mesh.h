#pragma once

#include "BBox.h"
#include <cstdint>
#include <glad/glad.h>
#include "PBRMaterial.h"
#include <tiny_gltf.h>
#include "VertexAttribute.h"

struct Submesh
{
	// TODO: make one VAO for each vertex layout, not for each submesh
	GLuint VAO;
	VertexAttribute flags = VertexAttribute::POSITION;
	int countVerticesOrIndices;
	int materialIndex;
	bool hasIndexBuffer;
	bool flatShading = false;
};

struct Mesh
{
	std::vector<Submesh> submeshes;
	BBox boundingBox {
		.minXYZ = glm::vec3(FLT_MAX),
		.maxXYZ = glm::vec3(-FLT_MAX)
	};
	bool HasMorphTargets() const;
};
#pragma once

#include "Mesh.h"
#include "VertexAttribute.h"

#include <span>
#include <string>
#include <tiny_gltf.h>
#include <unordered_map>
#include <vector>

class GLTFMeshParser
{
public:
	static Mesh Parse(const tinygltf::Mesh& mesh, const tinygltf::Model& model);
private:
	static int GetAttributeByteOffset(VertexAttribute attributes, VertexAttribute attribute);
	static VertexAttribute GetPrimitiveVertexLayout(const tinygltf::Primitive& primitive);
	static int GetVertexSizeBytes(VertexAttribute attributes);
	static void FillInterleavedBufferWithAttribute(std::vector<std::uint8_t>& interleavedBuffer, std::span<const std::uint8_t> attrData,
		int attrSizeBytes, int attrOffset, int vertexSizeBytes, int numVertices);
	static void FillInterleavedBufferWithAttribute(std::vector<std::uint8_t>& interleavedBuffer, const tinygltf::Accessor& accessor, int vertexSizeBytes, 
		VertexAttribute attribute, VertexAttribute attributes, const tinygltf::Model& model);
	static std::vector<std::uint8_t> GetInterleavedVertexBuffer(const tinygltf::Primitive& primitive, VertexAttribute attributes, const tinygltf::Model& model, 
		bool generateTangents);
	static std::vector<std::uint32_t> GetIndexBuffer(const tinygltf::Primitive& primitive, const tinygltf::Model& model, int offset);
	static BBox ComputeBoundingBox(const std::vector<std::uint8_t>& vertexBuffer, int stride);
	static void GenerateTangents(std::vector<std::uint8_t>& vertexBuffer, const std::vector<std::uint32_t>* indexBuffer, VertexAttribute attributes);

	static const inline std::unordered_map<std::string, VertexAttribute> vertexAttributeMapping =
	{
		{"POSITION", VertexAttribute::POSITION},
		{"TEXCOORD_0",  VertexAttribute::TEXCOORD},
		{"NORMAL", VertexAttribute::NORMAL},
		{"JOINTS_0", VertexAttribute::WEIGHTS},
		{"WEIGHTS_0", VertexAttribute::JOINTS},
		{"TANGENT", VertexAttribute::TANGENT },
		{"COLOR_0", VertexAttribute::COLOR },
	};
	static const inline std::vector<VertexAttribute> vertexAttributeOrdering =
	{
		VertexAttribute::POSITION,
		VertexAttribute::TEXCOORD,
		VertexAttribute::NORMAL,
		VertexAttribute::WEIGHTS,
		VertexAttribute::JOINTS,
		VertexAttribute::MORPH_TARGET0_POSITION,
		VertexAttribute::MORPH_TARGET1_POSITION,
		VertexAttribute::MORPH_TARGET0_NORMAL,
		VertexAttribute::MORPH_TARGET1_NORMAL,
		VertexAttribute::TANGENT,
		VertexAttribute::MORPH_TARGET0_TANGENT,
		VertexAttribute::MORPH_TARGET1_TANGENT,
		VertexAttribute::COLOR,
	};
	static const inline std::unordered_map<VertexAttribute, int> attributeByteSizes =
	{
		{VertexAttribute::POSITION, 12},
		{VertexAttribute::TEXCOORD, 8},
		{VertexAttribute::NORMAL, 12},
		{VertexAttribute::WEIGHTS, 16},
		{VertexAttribute::JOINTS, 4},
		{VertexAttribute::MORPH_TARGET0_POSITION, 12},
		{VertexAttribute::MORPH_TARGET1_POSITION, 12},
		{VertexAttribute::MORPH_TARGET0_NORMAL, 12},
		{VertexAttribute::MORPH_TARGET1_NORMAL, 12},
		{VertexAttribute::TANGENT, 16},
		{VertexAttribute::MORPH_TARGET0_TANGENT, 12},
		{VertexAttribute::MORPH_TARGET1_TANGENT, 12},
		{VertexAttribute::COLOR, 16}, // vertexColor is always converted to RGBA
	};
};
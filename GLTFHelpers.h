#pragma once

#include <cstdint>
#include <tiny_gltf.h>

inline int GetAccessorTypeSizeInBytes(const tinygltf::Accessor& accessor)
{
	return tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
}

std::vector<std::uint8_t> GetAccessorBytes(const tinygltf::Accessor& accessor, const tinygltf::Model& model);

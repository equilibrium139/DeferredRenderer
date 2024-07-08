#pragma once

#include "Scene.h"
#include <tiny_gltf.h>

class GLTFParser
{
public:
	static Scene Parse(const tinygltf::Scene& scene, const tinygltf::Model& model);
private:
	static Texture ParseTexture(int textureIdx, const tinygltf::Model& model);
	static Entity ParseNode(const tinygltf::Node& node, const tinygltf::Model& model, int& namelessEntitySuffix, std::vector<int>& lightOwningEntityIdx, const std::vector<Mesh>& meshes, int entityIdx);
	static Skeleton ParseSkin(const tinygltf::Skin& skin, const tinygltf::Model& model, const std::vector<Entity>& entities);
	static Animation ParseAnimation(const tinygltf::Animation& animation, const tinygltf::Model& model, int& namelessAnimSuffix, const std::vector<Entity>& entities);
	static Camera ParseCamera(const tinygltf::Camera& camera, const tinygltf::Model& model, int& namelessCameraSuffix);
	static Light ParseLight(const tinygltf::Light& light, const tinygltf::Model& model, const std::vector<int>& lightToEntityMap, int lightIdx);
	static PBRMaterial ParseMaterial(const tinygltf::Material& gltfMaterial, const tinygltf::Model& model);
};
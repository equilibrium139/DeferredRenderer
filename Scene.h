#pragma once

#include "Animation.h"
#include "Camera.h"
#include "Entity.h"
#include "Input.h"
#include "Light.h"
#include "Mesh.h"
#include "PBRMaterial.h"
#include "Skeleton.h"
#include "Texture.h"

#include <tiny_gltf.h>

#include <vector>

struct Scene
{
	std::vector<Entity> entities;
	std::vector<glm::mat4> globalTransforms;
	std::vector<Animation> animations;
	std::vector<std::uint8_t> animationEnabled;
	std::vector<Skeleton> skeletons;
	std::vector<Mesh> meshes;
	std::vector<PBRMaterial> materials;
	std::vector<Texture> textures;
	std::vector<Light> lights;
	std::vector<Camera> cameras;
	Camera controllableCamera;
	Camera* currentCamera = &controllableCamera;
	float exposure = 1.0f;
};
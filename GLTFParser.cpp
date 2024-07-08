#include "GLTFParser.h"
#include "GLTFMeshParser.h"
#include <iostream>

// TODO: separate scene parsing from renderer data logic (creating vertex arrays, vertex buffers, textures, etc.)
static bool IsLinearSpaceTexture(int textureIdx, const std::vector<tinygltf::Material>& materials)
{
	for (const tinygltf::Material& material : materials)
	{
		if (textureIdx == material.pbrMetallicRoughness.baseColorTexture.index || textureIdx == material.emissiveTexture.index)
		{
			return false;
		}
		else if (textureIdx == material.pbrMetallicRoughness.metallicRoughnessTexture.index || textureIdx == material.normalTexture.index || textureIdx == material.occlusionTexture.index)
		{
			return true;
		}
	}
	assert(false && "Should not be here");
	return false;
}

Scene GLTFParser::Parse(const tinygltf::Scene& gltfScene, const tinygltf::Model& model)
{
	Scene scene;

	for (const auto& extension : model.extensionsUsed)
	{
		std::cout << extension << '\n';
	}

	for (const auto& gltfMesh : model.meshes)
	{
		scene.meshes.push_back(GLTFMeshParser::Parse(gltfMesh, model));
	}
	for (int i = 0; i < model.textures.size(); i++)
	{
		scene.textures.emplace_back(ParseTexture(i, model));
	}
	// TODO: Scene should handle textures with negative idx by binding default texture before rendering use
	for (const auto& gltfMaterial : model.materials)
	{
		scene.materials.emplace_back(ParseMaterial(gltfMaterial, model));
	}

	std::vector<int> lightToEntityMap(model.lights.size());
	int namelessEntitySuffix = 0;
	for (const auto& node : model.nodes)
	{
		scene.entities.emplace_back(ParseNode(node, model, namelessEntitySuffix, lightToEntityMap, scene.meshes, scene.entities.size() - 1));
	}

	scene.globalTransforms.resize(scene.entities.size());

	// Set entity parents
	for (int i = 0; i < scene.entities.size(); i++)
	{
		const auto& entity = scene.entities[i];
		for (int j = 0; j < entity.children.size(); j++)
		{
			auto& child = scene.entities[entity.children[j]];
			child.parent = i;
		}
	}

	for (const auto& skin : model.skins)
	{
		scene.skeletons.emplace_back(ParseSkin(skin, model, scene.entities));
	}

	int namelessAnimSuffix = 0;
	for (const auto& animation : model.animations)
	{
		scene.animations.emplace_back(ParseAnimation(animation, model, namelessAnimSuffix, scene.entities));
	}
	scene.animationEnabled.resize(scene.animations.size(), true);

	int namelessCameraSuffix = 0;
	for (const tinygltf::Camera& camera : model.cameras)
	{
		scene.cameras.emplace_back(ParseCamera(camera, model, namelessCameraSuffix));
	}

	for (const tinygltf::Light& light : model.lights)
	{
		scene.lights.emplace_back(ParseLight(light, model, lightToEntityMap, scene.lights.size() - 1));
	}

	return scene;
}

Texture GLTFParser::ParseTexture(int textureIdx, const tinygltf::Model& model)
{
	const tinygltf::Texture& gltfTexture = model.textures[textureIdx];
	assert(gltfTexture.source >= 0);

	const tinygltf::Image& image = model.images[gltfTexture.source];
	bool linearSpaceTexture = IsLinearSpaceTexture(textureIdx, model.materials);

	int numComponents = image.component;
	GLenum internalFormat;
	GLenum format;
	if (numComponents == 1) {
		internalFormat = GL_RED;
		format = GL_RED;
	}
	if (numComponents == 2) {
		internalFormat = GL_RG;
		format = GL_RG;
	}
	else if (numComponents == 3) {
		internalFormat = linearSpaceTexture ? GL_RGB : GL_SRGB;
		format = GL_RGB;
	}
	else if (numComponents == 4) {
		internalFormat = linearSpaceTexture ? GL_RGBA : GL_SRGB_ALPHA;
		format = GL_RGBA;
	}
	else
	{
		std::cout << "Unsupported number of components: " << numComponents << " from file " << image.uri << '\n';
		std::exit(1);
	}

	Texture texture;

	//Texture 
	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_2D, texture.id);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.width, image.height, 0, format, image.pixel_type, image.image.data());
	glGenerateMipmap(GL_TEXTURE_2D);

	if (gltfTexture.sampler >= 0)
	{
		const tinygltf::Sampler& sampler = model.samplers[gltfTexture.sampler];
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
		if (sampler.minFilter != -1) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
		if (sampler.magFilter != -1) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter);
	}

	return texture;
}

Entity GLTFParser::ParseNode(const tinygltf::Node& node, const tinygltf::Model& model, int& namelessEntitySuffix, std::vector<int>& lightToEntityMap, const std::vector<Mesh>& meshes, int entityIdx)
{
	Entity entity;
	entity.name = node.name;
	if (entity.name.empty())
	{
		entity.name = "Entity" + std::to_string(namelessEntitySuffix++);
	}
	entity.localTransform = GetNodeTransform(node);
	entity.children = node.children;
	entity.meshIdx = node.mesh;

	if (node.mesh >= 0)
	{
		const Mesh& entityMesh = meshes[entity.meshIdx];
		// TODO: add support for more than 2 morph targets
		bool hasMorphTargets = entityMesh.HasMorphTargets();
		if (hasMorphTargets)
		{
			entity.morphTargetWeights.resize(2);
		}
	}

	entity.cameraIdx = node.camera;
	entity.skeletonIdx = node.skin;

	auto lightsExtension = node.extensions.find("KHR_lights_punctual");
	if (lightsExtension != node.extensions.end())
	{
		int lightIdx = lightsExtension->second.Get("light").GetNumberAsInt();
		entity.lightIdx = lightIdx;
		lightToEntityMap[lightIdx] = entityIdx;
	}

	return entity;
}

Skeleton GLTFParser::ParseSkin(const tinygltf::Skin& skin, const tinygltf::Model& model, const std::vector<Entity>& entities)
{
	Skeleton skeleton;
	int numJoints = skin.joints.size();

	std::vector<std::uint8_t> localToJointMatricesBytes = GetAccessorBytes(model.accessors[skin.inverseBindMatrices], model);
	assert(localToJointMatricesBytes.size() == sizeof(glm::mat4) * numJoints);
	std::span<glm::mat4> localToJointMatrices((glm::mat4*)(localToJointMatricesBytes.data()), numJoints);

	for (int i = 0; i < numJoints; i++)
	{
		skeleton.joints.emplace_back();
		auto& joint = skeleton.joints.back();
		joint.localToJoint = glm::mat4x3(localToJointMatrices[i]);
		joint.entityIndex = skin.joints[i];
		int parentEntityIndex = entities[joint.entityIndex].parent;
		if (parentEntityIndex < 0)
		{
			joint.parent = -1;
		}
		else
		{
			joint.parent = 0;
			while (joint.parent < skeleton.joints.size() && skeleton.joints[joint.parent].entityIndex != parentEntityIndex)
			{
				joint.parent++;
			}
			if (joint.parent >= skeleton.joints.size()) joint.parent = -1;
		}
	}
	return skeleton;
}

Animation GLTFParser::ParseAnimation(const tinygltf::Animation& gltfAnimation, const tinygltf::Model& model, int& namelessAnimSuffix, const std::vector<Entity>& entities)
{
	Animation animation;

	double animationDurationSeconds = GetAnimationDurationSeconds(gltfAnimation, model);
	animation.durationSeconds = (float)animationDurationSeconds;

	if (gltfAnimation.name.empty())
	{
		animation.name = "Anim" + std::to_string(namelessAnimSuffix++);
	}
	else
	{
		animation.name = gltfAnimation.name;
	}

	for (const auto& channel : gltfAnimation.channels)
	{
		const Entity* sceneEntity = &entities[channel.target_node];

		// Entity might already have another animated channel in this animation, check if so
		auto entityAnimationIter = std::find_if(animation.entityAnimations.begin(), animation.entityAnimations.end(),
			[&channel](const EntityAnimation& entityAnimation)
			{
				return entityAnimation.entityIdx == channel.target_node;
			});
		EntityAnimation* entityAnimation;
		if (entityAnimationIter != animation.entityAnimations.end())
		{
			entityAnimation = &(*entityAnimationIter);
		}
		else
		{
			animation.entityAnimations.emplace_back();
			entityAnimation = &animation.entityAnimations.back();
		}

		entityAnimation->entityIdx = channel.target_node;

		const auto& sampler = gltfAnimation.samplers[channel.sampler];
		InterpolationType method = InterpolationType::LINEAR;
		if (sampler.interpolation == "STEP") method = InterpolationType::STEP;
		else if (sampler.interpolation == "CUBICSPLINE") method = InterpolationType::CUBICSPLINE;
		assert(channel.target_path != "weights" || method == InterpolationType::LINEAR && "Non-linear interpolation not supported for weights currently");
		const auto& keyframeTimesAccessor = model.accessors[sampler.input];
		const auto& keyframeValuesAccessor = model.accessors[sampler.output];
		if (channel.target_path == "translation")
		{
			auto translationsBytes = GetAccessorBytes(keyframeValuesAccessor, model);
			std::span<glm::vec3> translations((glm::vec3*)translationsBytes.data(), keyframeValuesAccessor.count);

			auto timesBytes = GetAccessorBytes(keyframeTimesAccessor, model);
			std::span<float> times((float*)timesBytes.data(), keyframeTimesAccessor.count);

			entityAnimation->translations.values.assign(translations.begin(), translations.end());
			entityAnimation->translations.times.assign(times.begin(), times.end());
			entityAnimation->translations.method = method;
		}
		else if (channel.target_path == "scale")
		{
			auto scalesBytes = GetAccessorBytes(keyframeValuesAccessor, model);
			std::span<glm::vec3> scales((glm::vec3*)scalesBytes.data(), keyframeValuesAccessor.count);

			auto timesBytes = GetAccessorBytes(keyframeTimesAccessor, model);
			std::span<float> times((float*)timesBytes.data(), keyframeTimesAccessor.count);

			entityAnimation->scales.values.assign(scales.begin(), scales.end());
			entityAnimation->scales.times.assign(times.begin(), times.end());
			entityAnimation->scales.method = method;
		}
		else if (channel.target_path == "rotation")
		{
			auto rotationsBytes = GetAccessorBytes(keyframeValuesAccessor, model);
			std::span<glm::quat> rotations((glm::quat*)rotationsBytes.data(), keyframeValuesAccessor.count);

			auto timesBytes = GetAccessorBytes(keyframeTimesAccessor, model);
			std::span<float> times((float*)timesBytes.data(), keyframeTimesAccessor.count);

			entityAnimation->rotations.values.assign(rotations.begin(), rotations.end());
			entityAnimation->rotations.times.assign(times.begin(), times.end());
			entityAnimation->rotations.method = method;
		}
		else
		{
			auto weightsBytes = GetAccessorBytes(keyframeValuesAccessor, model);
			std::span<float> weights((float*)weightsBytes.data(), keyframeValuesAccessor.count);

			auto timesBytes = GetAccessorBytes(keyframeTimesAccessor, model);
			std::span<float> times((float*)timesBytes.data(), keyframeTimesAccessor.count);

			entityAnimation->weights.values.assign(weights.begin(), weights.end());
			entityAnimation->weights.times.assign(times.begin(), times.end());
			entityAnimation->weights.method = method;
		}
	}

	return animation;
}

Camera GLTFParser::ParseCamera(const tinygltf::Camera& gltfCamera, const tinygltf::Model& model, int& namelessCameraSuffix)
{
	bool perspective = gltfCamera.type == "perspective";
	if (!perspective)
	{
		std::cout << "Warning: Orthographic cameras not currently supported\n";
	}
	Camera camera;
	if (!gltfCamera.name.empty())
	{
		camera.name = gltfCamera.name;
	}
	else
	{
		camera.name = "Camera " + std::to_string(namelessCameraSuffix++);
	}
	if (perspective)
	{
		camera.zoom = glm::degrees(gltfCamera.perspective.yfov);
		camera.near = gltfCamera.perspective.znear;
		camera.far = gltfCamera.perspective.zfar;
		if (gltfCamera.perspective.aspectRatio > 0.0f)
		{
			camera.aspectRatio = gltfCamera.perspective.aspectRatio;
		}
	}

	return camera;
}

Light GLTFParser::ParseLight(const tinygltf::Light& gltfLight, const tinygltf::Model& model, const std::vector<int>& lightToEntityMap, int lightIdx)
{
	glm::vec3 color = gltfLight.color.size() == 3 ? glm::vec3((float)gltfLight.color[0], (float)gltfLight.color[1], (float)gltfLight.color[2]) : glm::vec3(1.0f);
	// TODO: implement this as bool indicating if light has infinite range?
	const float gltfLightRange = gltfLight.range != 0.0 ? (float)gltfLight.range : FLT_MAX;

	Light light;
	if (gltfLight.type == "point")
	{
		light = {
			.type = Light::Point,
			.color = color,
			.intensity = (float)gltfLight.intensity,
			.range = gltfLightRange,
			.entityIdx = lightToEntityMap[lightIdx]
		};
	}
	else if (gltfLight.type == "spot")
	{
		light = {
			.type = Light::Spot,
			.color = color,
			.intensity = (float)gltfLight.intensity,
			.range = gltfLightRange,
			.innerAngleCutoffDegrees = glm::degrees((float)gltfLight.spot.innerConeAngle),
			.outerAngleCutoffDegrees = glm::degrees((float)gltfLight.spot.outerConeAngle),
			.entityIdx = lightToEntityMap[lightIdx]
		};
	}
	else
	{
		assert(gltfLight.type == "directional");
		light = {
			.type = Light::Directional,
			.color = color,
			.intensity = (float)gltfLight.intensity,
			.entityIdx = lightToEntityMap[lightIdx]
		};
	}
	return light;
}

PBRMaterial GLTFParser::ParseMaterial(const tinygltf::Material& gltfMaterial, const tinygltf::Model& model)
{
	static int defaultMaterialNameSuffix = 0;
	const tinygltf::PbrMetallicRoughness& pbr = gltfMaterial.pbrMetallicRoughness;
	assert(pbr.baseColorTexture.texCoord == 0 &&
		pbr.metallicRoughnessTexture.texCoord == 0 &&
		gltfMaterial.normalTexture.texCoord == 0 &&
		gltfMaterial.occlusionTexture.texCoord == 0 &&
		"Multiple tex coords not currently supported");

	assert(pbr.baseColorTexture.index < 0 || model.images[model.textures[pbr.baseColorTexture.index].source].component == 4 && "Assuming RGBA for base color texture");

	PBRMaterial material;
	material.name = gltfMaterial.name;
	if (material.name.empty())
	{
		material.name = "Material" + defaultMaterialNameSuffix;
		defaultMaterialNameSuffix++;
	}
	material.baseColorFactor = glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
	material.baseColorTextureIdx = pbr.baseColorTexture.index;
	material.metallicFactor = (float)pbr.metallicFactor;
	material.roughnessFactor = (float)pbr.roughnessFactor;
	material.metallicRoughnessTextureIdx = pbr.metallicRoughnessTexture.index;
	material.normalTextureIdx = gltfMaterial.normalTexture.index;
	material.normalScale = gltfMaterial.normalTexture.scale;
	//assert(gltfMaterial.normalTexture.index < 0 || gltfMaterial.normalTexture.scale == 1.0f);

	material.occlusionStrength = gltfMaterial.occlusionTexture.strength;
	material.occlusionTextureIdx = gltfMaterial.occlusionTexture.index;

	return material;
}
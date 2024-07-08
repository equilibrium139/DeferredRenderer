#pragma once

#include "Mesh.h"
#include "Skeleton.h"
#include <string>
#include "Transform.h"
#include <vector>

struct Entity
{
	std::string name;
	std::vector<int> children;
	std::vector<float> morphTargetWeights;
	Transform localTransform;
	int parent = -1;
	int meshIdx = -1;
	int skeletonIdx = -1;
	int cameraIdx = -1;
	int lightIdx = -1;
};
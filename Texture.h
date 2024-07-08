#pragma once

#include <glad/glad.h>

// TODO: this class seems pretty useless... probably remove
struct Texture
{
	GLuint id;

	// Used as default textures in order to treat materials consistently, whether they have actual textures or not
	static const Texture& White1x1TextureRGBA();
	static const Texture& Max1x1TextureRed();
	static const Texture& DepthCubemap1x1();
};
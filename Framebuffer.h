#pragma once

#include <optional>
#include <vector>
#include <glad/glad.h>

struct ColorAttachmentInfo
{
	GLint internalFormat;

	// TODO: remove format and type since probably never used for framebuffer since data is never uploaded, but produced by rendering
	// and use glTexStorage instead of glTexImage while you're at it
	GLenum format;
	GLenum type; 

	GLint minFilterMode;
	GLint magFilterMode;
};

// TODO: possibly store state like blending mode and set it on Bind(). Blend mode in OpenGL is not tied to framebuffer, but it would be 
// convenient to treat it as if it were.
struct Framebuffer
{
	std::vector<GLuint> colorTextures;
	GLuint id;
	GLuint depthStencilRBO;
	int width, height;

	Framebuffer(int width, int height, const std::vector<ColorAttachmentInfo>& colorAttachmentsInfo, GLint depthStencilInternalFormat);
	void Bind();
};
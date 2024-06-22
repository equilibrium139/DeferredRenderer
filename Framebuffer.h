#pragma once

#include <vector>
#include <glad/glad.h>

struct ColorAttachmentInfo
{
	GLint internalFormat;
	GLenum format;
	GLenum type;
	GLint minFilterMode;
	GLint magFilterMode; 
};

struct Framebuffer
{
	std::vector<GLuint> colorTextures;
	GLuint id;
	GLuint depthStencilRBO;
	int width, height;

	Framebuffer(int width, int height, const std::vector<ColorAttachmentInfo>& colorAttachmentsInfo, GLint depthStencilInternalFormat);
	void Bind();
};
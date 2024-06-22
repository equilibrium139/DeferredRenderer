#include "Framebuffer.h"

#include <cassert>

Framebuffer::Framebuffer(int width, int height, const std::vector<ColorAttachmentInfo>& colorAttachmentsInfo, GLint depthStencilInternalFormat)
	:width(width), height(height)
{
	assert(colorAttachmentsInfo.size() > 0);

	glGenFramebuffers(1, &id);
	glBindFramebuffer(GL_FRAMEBUFFER, id);

	std::vector<GLenum> attachments;
	colorTextures.resize(colorAttachmentsInfo.size());
	for (int i = 0; i < colorTextures.size(); i++)
	{
		const ColorAttachmentInfo& info = colorAttachmentsInfo[i];
		GLuint& textureID = colorTextures[i];
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, info.internalFormat, width, height, 0, info.format, info.type, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, info.minFilterMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, info.magFilterMode);
		GLenum attachment = GL_COLOR_ATTACHMENT0 + i;
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, textureID, 0);
		attachments.push_back(attachment);
	}

	glDrawBuffers(attachments.size(), &attachments[0]);

	glGenRenderbuffers(1, &depthStencilRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRBO);
}

void Framebuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, id);
	glViewport(0, 0, width, height);
}

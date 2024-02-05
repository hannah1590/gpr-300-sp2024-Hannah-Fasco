#include "framebuffer.h"

hannah::Framebuffer hannah::createFramebuffer(unsigned int width, unsigned int height, int colorFormat)
{
	Framebuffer fbo;
	fbo.width = width;
	fbo.height = height;

	glGenFramebuffers(1, &fbo.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

	// color buffer
	glGenTextures(8, fbo.colorBuffer);
	for (unsigned int i = 0; i < 8; i++)
	{
		glGenTextures(1, &fbo.colorBuffer[i]);
		glBindTexture(GL_TEXTURE_2D, fbo.colorBuffer[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, fbo.colorBuffer[i], 0);
	}
	//Tell OpenGL to draw to 8 color buffers
	GLenum attachments[8] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(8, attachments);
	
	// depth buffer
	glGenTextures(1, &fbo.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, fbo.depthBuffer);
	//Create 16 bit depth buffer - must be same width/height of color buffer
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);
	//Attach to framebuffer (assuming FBO is bound)
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo.depthBuffer, 0);

	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return fbo;
}
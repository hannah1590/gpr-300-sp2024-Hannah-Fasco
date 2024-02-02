#include "framebuffer.h"

hannah::Framebuffer hannah::createFramebuffer(unsigned int width, unsigned int height, int colorFormat)
{
	Framebuffer fbo;
	fbo.width = width;
	fbo.height = height;

	//unsigned int framebuffer;
	glGenFramebuffers(1, &fbo.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

	glGenTextures(8, fbo.colorBuffer);
	for (unsigned int i = 0; i < 8; i++)
	{
		glGenTextures(1, &fbo.colorBuffer[0]);
		glBindTexture(GL_TEXTURE_2D, fbo.colorBuffer[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, width, height);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, fbo.colorBuffer[i], 0);
	}
	//Tell OpenGL to draw to 8 color buffers
	GLenum attachments[8] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(8, attachments);
	

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

	glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return fbo;
}
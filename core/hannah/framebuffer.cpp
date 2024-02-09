#include "framebuffer.h"

hannah::Framebuffer hannah::createFramebufferWithRBO(unsigned int width, unsigned int height, int colorFormat)
{
	Framebuffer fbo;
	fbo.width = width;
	fbo.height = height;

	glGenFramebuffers(1, &fbo.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

	// color buffer
	glGenTextures(1, &fbo.colorBuffer[0]);
	glBindTexture(GL_TEXTURE_2D, fbo.colorBuffer[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.colorBuffer[0], 0);

	// for assignment 1
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	
	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	return fbo;
}

hannah::Framebuffer hannah::createFramebufferWithDepthBuffer(unsigned int width, unsigned int height, int colorFormat)
{
	Framebuffer fbo;
	fbo.width = width;
	fbo.height = height;

	glGenFramebuffers(1, &fbo.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

	// color buffer
	glGenTextures(1, &fbo.colorBuffer[0]);
	glBindTexture(GL_TEXTURE_2D, fbo.colorBuffer[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.colorBuffer[0], 0);

	// depth buffer for assignment 2
	glGenTextures(1, &fbo.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, fbo.depthBuffer);
	//Create 16 bit depth buffer - must be same width/height of color buffer
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);
	//Attach to framebuffer (assuming FBO is bound)
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo.depthBuffer, 0);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	return fbo;
}

hannah::Framebuffer hannah::createFramebufferWithShadowMap(unsigned int width, unsigned int height, int colorFormat)
{
	Framebuffer shadowfbo;
	shadowfbo.width = width;
	shadowfbo.height = height;

	// framebuffer object
	glGenFramebuffers(1, &shadowfbo.fbo);

	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

	// depthbuffer
	glGenTextures(1, &shadowfbo.depthBuffer);
	glBindTexture(GL_TEXTURE_2D, shadowfbo.depthBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowfbo.fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowfbo.depthBuffer, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	return shadowfbo;
}
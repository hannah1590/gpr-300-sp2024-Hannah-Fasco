#include <stdio.h>
#include <math.h>

#include <glm/glm.hpp>
#include "external/stb_image.h"
#include "external/glad.h"

namespace hannah {
	struct Framebuffer {
		unsigned int fbo;
		unsigned int colorBuffer[8];
		unsigned int depthBuffer;
		unsigned int width;
		unsigned int height;
	};
	Framebuffer createFramebuffer(unsigned int width, unsigned int height, int colorFormat);
}
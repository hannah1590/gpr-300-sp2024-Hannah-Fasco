#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>
#include <ew/procGen.h>

#include <hannah/framebuffer.h>

#include <time.h> 
#include "vector"

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();


ew::Camera camera;
ew::Transform monkeyTransform;
ew::CameraController cameraController;
ew::Camera lightCam;

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

struct PointLight {
	glm::vec3 position;
	float radius;
	glm::vec4 color;
};
const int MAX_POINT_LIGHTS = 64;
PointLight pointLights[MAX_POINT_LIGHTS];

struct Transform {
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

	glm::mat4 modelMatrix() const {
		glm::mat4 m = glm::mat4(1.0f);
		m = glm::translate(m, position);
		m *= glm::mat4_cast(rotation);
		m = glm::scale(m, scale);
		return m;
	}
};

struct Node {
	glm::mat4 localTransform;
	glm::mat4 globalTransform;
	unsigned int parentIndex;
	Transform transform;
};

struct Hierarchy
{
	std::vector<Node*> nodes;
	unsigned int nodeCount;
};

//Assumes list is ordered by depth
void SolveFK(Hierarchy hierarchy)
{
	for each (Node *node in hierarchy.nodes)
	{
		if (node->parentIndex == -1)
		{
			node->globalTransform = node->localTransform;
		}
		else
		{
			node->globalTransform = hierarchy.nodes[node->parentIndex]->globalTransform * node->localTransform;
		}
	}
}

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

int shadowWidth = 1024;
int shadowHeight = 1024;

float gamma = 2.2f;

glm::vec3 lightDir = glm::vec3(0.0f, -1.0f, 0.0f);
float minBias = 0.005f;
float maxBias = 0.015f;

hannah::Framebuffer shadowFramebuffer;
hannah::Framebuffer gBuffer;

int main() {
	GLFWwindow* window = initWindow("Assignment 3", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader postProcess = ew::Shader("assets/post.vert", "assets/post.frag");
	ew::Shader depthShader = ew::Shader("assets/depth.vert", "assets/depth.frag");
	ew::Shader gShader = ew::Shader("assets/lit.vert", "assets/geometry.frag");
	ew::Shader deferredShader = ew::Shader("assets/deferredLit.vert", "assets/deferredLit.frag");
	ew::Shader lightOrbShader = ew::Shader("assets/lightOrb.vert", "assets/lightOrb.frag");
	ew::Model monkeyModel = ew::Model("assets/suzanne.fbx");
	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(10, 10, 5));
	ew::Transform planeTransform;
	planeTransform.position = glm::vec3(0.0f, -1.0f, 0.0f);
	planeTransform.scale = glm::vec3(10.0f);
	ew::Mesh sphereMesh = ew::Mesh(ew::createSphere(1.0f, 8));

	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	lightCam.aspectRatio = 1;
	lightCam.orthographic = true;
	lightCam.orthoHeight = 5;
	
	lightCam.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	lightCam.position = (lightCam.target - glm::normalize(lightDir)) * 5.0f;
	lightCam.farPlane = 10.0f;

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing

	unsigned int dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);

	hannah::Framebuffer framebuffer = hannah::createFramebufferWithDepthBuffer(screenWidth, screenHeight, GL_RGB16F);
	shadowFramebuffer = hannah::createFramebufferWithShadowMap(shadowWidth, shadowHeight, GL_RGB16F);
	gBuffer = hannah::createGBuffer(screenWidth, screenHeight);

	//Handles to OpenGL object are unsigned integers
	GLuint brickTexture = ew::loadTexture("assets/travertine_color.jpg");
	GLuint normalTexture = ew::loadTexture("assets/travertine_normal.jpg");

	//Make "_MainTex" sampler2D sample from the 2D texture bound to unit 0
	shader.use();
	shader.setInt("_MainTex", 0);
	shader.setInt("normalMap", 1);
	shader.setInt("_ShadowMap", 2);

	srand(time(0));
	
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		pointLights[i].position = glm::vec3(i - 25, 0, (i % 8) - 3);
		pointLights[i].radius = 5;
		pointLights[i].color = glm::vec4(rand() % 100, rand() % 100, rand() % 100, 100) * 0.01f;
	}

	Node head;
	Node body;
	head.parentIndex = -1;
	body.parentIndex = 0;

	Hierarchy hierarchy;
	hierarchy.nodes.push_back(&head);
	hierarchy.nodeCount++;
	hierarchy.nodes.push_back(&body);
	hierarchy.nodeCount++;

	head.transform.scale = glm::vec3(0.5f);
	body.transform.scale = glm::vec3(1.0f);
	body.transform.position = glm::vec3(0, 2.0f, 0.0f);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		//head.transform.position = glm::vec3(glm::sin(deltaTime) * 10);

		head.transform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		head.localTransform = head.transform.modelMatrix();
		body.localTransform = body.transform.modelMatrix();

		SolveFK(hierarchy);


		//RENDER SCENE TO G-BUFFER
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);
		glViewport(0, 0, gBuffer.width, gBuffer.height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindTextureUnit(0, brickTexture);
		glBindTextureUnit(1, normalTexture);

		gShader.use();
		gShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		gShader.setMat4("_LightViewProj", lightCam.projectionMatrix() * lightCam.viewMatrix());
		gShader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();
		//gShader.setMat4("_Model", monkeyTransform.modelMatrix());
		//monkeyModel.draw();
		gShader.setMat4("_Model", head.globalTransform);
		monkeyModel.draw();
		gShader.setMat4("_Model", body.globalTransform);
		monkeyModel.draw();


		//After geometry pass
		//LIGHTING PASS
		//if using post processing, we draw to our offscreen framebuffer
		lightCam.position = (lightCam.target - glm::normalize(lightDir)) * 5.0f;

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		glViewport(0, 0, framebuffer.width, framebuffer.height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		deferredShader.use();

		for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
			//Creates prefix "_PointLights[0]." etc
			std::string prefix = "_PointLights[" + std::to_string(i) + "].";
			deferredShader.setVec3(prefix + "position", pointLights[i].position);
			deferredShader.setVec3(prefix + "color", pointLights[i].color);
			deferredShader.setFloat(prefix + "radius", pointLights[i].radius);
		}

		deferredShader.setVec3("lightPos", lightCam.position);
		deferredShader.setVec3("_EyePos", camera.position);
		deferredShader.setVec3("_LightDirection", glm::normalize(lightDir));
		deferredShader.setFloat("_Material.Ka", material.Ka);
		deferredShader.setFloat("_Material.Kd", material.Kd);
		deferredShader.setFloat("_Material.Ks", material.Ks);
		deferredShader.setFloat("_Material.Shininess", material.Shininess);

		deferredShader.setFloat("minBias", minBias);
		deferredShader.setFloat("maxBias", maxBias);

		//Bind g-buffer textures
		glBindTextureUnit(0, gBuffer.colorBuffer[0]);
		glBindTextureUnit(1, gBuffer.colorBuffer[1]);
		glBindTextureUnit(2, gBuffer.colorBuffer[2]);
		glBindTextureUnit(3, shadowFramebuffer.depthBuffer); //For shadow mapping

		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		//Blit gBuffer depth to same framebuffer as fullscreen quad
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.fbo); //Read from gBuffer 
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.fbo); //Write to current fbo
		glBlitFramebuffer(
			0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST
		);
		//Draw all light orbs
		lightOrbShader.use();
		lightOrbShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			glm::mat4 m = glm::mat4(1.0f);
			m = glm::translate(m, pointLights[i].position);
			m = glm::scale(m, glm::vec3(0.2f)); //Whatever radius you want

			lightOrbShader.setMat4("_Model", m);
			lightOrbShader.setVec3("_Color", pointLights[i].color);
			sphereMesh.draw();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer.fbo);
		glBindTexture(GL_TEXTURE_2D, shadowFramebuffer.depthBuffer);
		glViewport(0, 0, shadowWidth, shadowHeight);
		glClear(GL_DEPTH_BUFFER_BIT);

		glCullFace(GL_FRONT);

		depthShader.use();
		depthShader.setMat4("_ViewProjection", lightCam.projectionMatrix() * lightCam.viewMatrix());
		depthShader.setMat4("_Model", monkeyTransform.modelMatrix());
		monkeyModel.draw();

		//RENDER
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		glBindTextureUnit(0, brickTexture);
		glBindTextureUnit(1, normalTexture);
		glBindTextureUnit(2, shadowFramebuffer.depthBuffer);
		//glViewport(0, 0, screenWidth, screenHeight);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glClearColor(0.6f, 0.8f, 0.92f, 1.0f);

		// reset viewport
		glViewport(0, 0, screenWidth, screenHeight);
		//glClear(GL_DEPTH_BUFFER_BIT);

		glCullFace(GL_BACK);

		shader.use();
		shader.setInt("_MainTex", 0);
		shader.setInt("normalMap", 1);
		shader.setInt("_ShadowMap", 2);
		shader.setMat4("_Model", glm::mat4(1.0f));
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		shader.setMat4("_LightViewProj", lightCam.projectionMatrix() * lightCam.viewMatrix());
		shader.setVec3("_EyePos", camera.position);
		shader.setVec3("_LightDirection", glm::normalize(lightDir));
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("_Material.Shininess", material.Shininess);

		shader.setFloat("minBias", minBias);
		shader.setFloat("maxBias", maxBias);

		shader.setMat4("_Model", planeTransform.modelMatrix());
		planeMesh.draw();

		//shader.setMat4("_Model", monkeyTransform.modelMatrix());
		//monkeyModel.draw(); //Draws monkey model using current shader

		
		
		//Rotate model around Y axis
		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));
	
		//transform.modelMatrix() combines translation, rotation, and scale into a 4x4 model matrix
		cameraController.move(window, &camera, deltaTime);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		postProcess.use();
		postProcess.setFloat("gamma", gamma);

		glBindTextureUnit(0, framebuffer.colorBuffer[0]);
		glBindVertexArray(dummyVAO);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffer[0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		drawUI();

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}



void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0, 0, 5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
		ImGui::SliderFloat("Gamma", &gamma, 0.0f, 5.0f);
		ImGui::SliderFloat3("LightDir", &lightDir.r, -1.0f, 1.0f);
		ImGui::SliderFloat("Height", &lightCam.orthoHeight, 4.0f, 10.0f);
		ImGui::SliderFloat("MinBias", &minBias, 0.0f, 1.0f);
		ImGui::SliderFloat("MaxBias", &maxBias, 0.0f, 1.0f);
	}
	ImGui::End();

	ImGui::Begin("Shadow Map");
	//Using a Child allow to fill all the space of the window.
	ImGui::BeginChild("Shadow Map");
	//Stretch image to be window size
	ImVec2 windowSize = ImGui::GetWindowSize();
	//Invert 0-1 V to flip vertically for ImGui display
	//shadowMap is the texture2D handle
	ImGui::Image((ImTextureID)shadowFramebuffer.depthBuffer, windowSize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::EndChild();
	ImGui::End();

	ImGui::Begin("GBuffers");
	ImVec2 texSize = ImVec2(gBuffer.width / 4, gBuffer.height / 4);
	for (size_t i = 0; i < 3; i++)
	{
		ImGui::Image((ImTextureID)gBuffer.colorBuffer[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
	}
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}


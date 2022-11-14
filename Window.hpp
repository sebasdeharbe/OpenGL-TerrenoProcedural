#ifndef WINDOW_H
#define WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"

///Este archivo contiene funciones para inicializar la ventana y los callbacks
//Tamaño de ventana
const GLint WIDTH = 1000, HEIGHT = 800;
//Ventana
GLFWwindow *pwindow;
//Cámara
Camera *pcamera; 
//Posición Inicial de cámara
bool firstMouse = true;
float lastX = WIDTH / 2;
float lastY = HEIGHT / 2;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float currentFrame;

///Callbacks
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	// Prevent camera jumping when mouse first enters screen
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	
	// yoffset is reversed since y-coords go from bottom to top
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	
	lastX = xpos;
	lastY = ypos;
	
	pcamera->ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	pcamera->ProcessMouseScroll(yoffset);
}


///GLFW
// Inicializar GLFW y GLAD
int init(GLFWwindow* &window, Camera &camera){
	glfwInit();
	window = pwindow;
	*pcamera = camera;
	
	// OPen GL version 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	// para macOS 
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	
	pwindow = glfwCreateWindow(WIDTH, HEIGHT, "Procedural Terrain - Terreno Procedural", nullptr, nullptr);
	
	// Account for macOS retina resolution
	int screenWidth, screenHeight;
	glfwGetFramebufferSize(pwindow, &screenWidth, &screenHeight);
	
	if (pwindow == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	
	glfwMakeContextCurrent(pwindow);
	glfwSetScrollCallback(pwindow, scroll_callback);
	glfwSetCursorPosCallback(pwindow, mouse_callback);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	
	glViewport(0, 0, screenWidth, screenHeight);
	
	// Enable z-buffer
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FRAMEBUFFER_SRGB);
	
	// Enable mouse input
	glfwSetInputMode(pwindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	return 0;
}

void processInput(GLFWwindow *window, Shader &shader, Camera &camera) {
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	
	// Enable wireframe mode
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	// Enable flat mode
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
		shader.use();
		shader.setBool("isFlat", false);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
		shader.use();
		shader.setBool("isFlat", true);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}





#endif

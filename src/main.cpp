#include <algorithm>
#include <stdexcept>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "ObjMesh.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"
#include "Window.hpp"
#include "Callbacks.hpp"
#include "Model.hpp"

#define VERSION 20221019
#include <iostream>
using namespace std;


void modifyMesh(const std::vector<glm::vec3> &v,std::vector<glm::vec3> &vertices,std::vector<glm::vec2> &coords) {
	/// @todo: generar el vector de coordenadas de texturas para los vertices de la botella
	
	vertices.resize(v.size());
	coords.resize(v.size());
	
	for(int i=0;i<v.size();i++) { 
		
		float random = ((float)rand()/RAND_MAX)/6.f;
		vertices[i] = glm::vec3(v[i].x,v[i].y + random,v[i].z);
		
		float s = min(vertices[i].y*7.f,1.f);
		float t = 0.1f;
		coords[i] = glm::vec2(s,t);
		
//		std::cout<<"("<<v[i].x<<", "<<v[i].y<<", "<<v[i].z<<")"<<std::endl;
//		std::cout<<"("<<vertices[i].x<<", "<<vertices[i].y<<", "<<vertices[i].z<<")"<<std::endl;
//		std::cout<<"("<<coords[i].x<<", "<<coords[i].y<<")"<<std::endl<<std::endl;
		
	}
}


int main() {
	
	// initialize window and setup callbacks
	Window window(win_width,win_height,"CG Texturas");
	setCommonCallbacks(window);
	
	// setup OpenGL state and load shaders
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS);
	glEnable(GL_BLEND); glad_glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.7f,0.7f,0.7f,1.f);
	Shader shader("shaders/texture");
	
	// load model and assign texture
	auto models = Model::load("malla16.16",Model::fKeepGeometry);
	Model &plane = models[0];
	
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> coords;
	modifyMesh(plane.geometry.positions,vertices,coords);
	
	plane.buffers.updatePositions(vertices,true);
	plane.buffers.updateTexCoords(coords,true);
	plane.texture = Texture("models/elevation_gradient.png",true,true);
	
	do {
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		shader.use();
		setMatrixes(shader);
		shader.setLight(glm::vec4{2.f,-2.f,-4.f,0.f}, glm::vec3{1.f,1.f,1.f}, 0.15f);
		for(Model &mod : models) {
			mod.texture.bind();
			shader.setMaterial(mod.material);
			shader.setBuffers(mod.buffers);
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
			mod.buffers.draw();
		}
		
		// finish frame
		glfwSwapBuffers(window);
		glfwPollEvents();
		
	} while( glfwGetKey(window,GLFW_KEY_ESCAPE)!=GLFW_PRESS && !glfwWindowShouldClose(window) );
}


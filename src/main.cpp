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
#include <cmath>
using namespace std;


typedef struct {
	int tamanioMapa = 31;
	int numeroDeOctavas = 8;  		//numero de octavas
	int fact = 1; 					//escala de ruido
	int freq = 1;			 		//frecuencia
	int amp = 1;					//amplitud
	int seed = 1;           		//seed para el srand()
	float persistency = 2.f;       	//
	float lacunarity = 0.5f;       	//
//	float 
}sets;
sets parametros;

float interpolacionBilineal(float x1,float y1,float x2,float y2, float v1,float v2,float v3,float v4,float tx,float ty){
	float sumV1=fabs((tx-x1)*(ty-y1))*v4;
	float sumV2=fabs((tx-x2)*(ty-y1))*v3;
	float sumV3=fabs((tx-x1)*(ty-y2))*v2;
	float sumV4=fabs((tx-x2)*(ty-y2))*v1;
	float areaTotal=(x2-x1)*(y2-y1);
	return (sumV1+sumV2+sumV3+sumV4)/areaTotal;
}

vector<vector<float>> createNoiseMap(){
	srand(parametros.seed); 
	vector<vector<float>> noiseMap(parametros.tamanioMapa+1,vector<float>(parametros.tamanioMapa+1));
	
	//armar la primera octava
	int tamanioSubdivision = parametros.tamanioMapa/parametros.freq;
	
//	if(tamanioSubdivision<1) tamanioSubdivision=1; //para que no rompa con cualca frecuecia.
	
	while(parametros.tamanioMapa%tamanioSubdivision){
//		--tamanioSubdivision; //esto verifica que la frecuencia sea potencia de 2. 
		cout<<"rompe el tamanio de subdiv";
	}
	
	for(int i=0;i<=parametros.tamanioMapa;i+=tamanioSubdivision){
		
		for(int j=0;j<=parametros.tamanioMapa;j+=tamanioSubdivision){
			
			//Crear nodos
			noiseMap[i][j] = parametros.amp*((float)rand()/RAND_MAX);
			
			//Interpolar puntos interiores
			if(i>=tamanioSubdivision && j>=tamanioSubdivision){
				
				for(int a=i-tamanioSubdivision; a<=i; a++){
					for(int b=j-tamanioSubdivision; b<=j; b++){
						
						int x1=i-tamanioSubdivision;
						int x2=i;
						int y1=j-tamanioSubdivision;
						int y2=j;
						noiseMap[a][b] = interpolacionBilineal(x1, y1, x2, y2, noiseMap[x1][y1], noiseMap[x2][y1], noiseMap[x1][y2], noiseMap[x2][y2], a, b);
					}
				} 
				
			}
		}
		
	}
	
	
//	
//	for(int i=0;i<noiseMap.size();i++) { 
//		for(int j=0;j<noiseMap[i].size();j++) { 
//			cout<<noiseMap[i][j]<<" ";
//		}
//		cout<<endl;
//	}
	
	return noiseMap;
	
}

void modifyMesh(const std::vector<glm::vec3> &v,std::vector<glm::vec3> &vertices,std::vector<glm::vec2> &coords, vector<vector<float>> &noiseMap) {
	
	vertices.resize(v.size());
	coords.resize(v.size());
	
	float xMin = v[0].x;
	float xMax = v[0].x;
	float zMin = v[0].z;
	float zMax = v[0].z;
	
	for(int i=0;i<v.size();i++) { 
		if(v[i].x<xMin) xMin = v[i].x;
		if(v[i].x>xMax) xMax = v[i].x;
		if(v[i].z<zMin) zMin = v[i].z;
		if(v[i].z>zMax) zMax = v[i].z;
	}
//	
//	cout<<xMin<<"  "<<xMax<<endl;
//	cout<<zMin<<"  "<<zMax<<endl<<endl<<endl;
//	
	
	float deltaX = abs(xMax - xMin);
	float deltaZ = abs(zMax - zMin);
	
	for(int i=0;i<v.size();i++) { 
		
//		cout<<"x: "<<v[i].x<<"   z:"<<v[i].z<<endl;
		
		float xRuido = ((v[i].x-(float)xMin)/((float)deltaX)) * (float)parametros.tamanioMapa;
		float zRuido = ((v[i].z-(float)zMin)/((float)deltaZ)) * (float)parametros.tamanioMapa;
		
//		cout<<xRuido<<"  "<<zRuido<<endl;
		
		float xInterpolacionMin = floor(xRuido);
		float xInterpolacionMax = ceil(xRuido);
		
		float zInterpolacionMin = floor(zRuido);
		float zInterpolacionMax = ceil(zRuido);
		
//		cout<<xInterpolacionMax<<" "<<xInterpolacionMin<<" "<<zInterpolacionMax<<" "<<zInterpolacionMin<<endl;
		
		float valorInterpolado = interpolacionBilineal(xInterpolacionMin, zInterpolacionMin, xInterpolacionMax, zInterpolacionMax,
														noiseMap[xInterpolacionMin][zInterpolacionMin], noiseMap[xInterpolacionMax][zInterpolacionMin],
														noiseMap[xInterpolacionMin][zInterpolacionMax], noiseMap[xInterpolacionMax][zInterpolacionMax],
														xRuido, zRuido);
		
//		cout<<valorInterpolado<<endl<<endl;
		vertices[i] = glm::vec3(v[i].x,valorInterpolado,v[i].z);
		
		float s = min(vertices[i].y*2.f,1.f);
		float t = 0.1f;
		coords[i] = glm::vec2(s,t);
		
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
	auto models = Model::load("mallaRefinada",Model::fKeepGeometry);
	Model &plane = models[0];
	
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> coords;
	
	vector<vector<float>> noiseMap = createNoiseMap();
	
	
	modifyMesh(plane.geometry.positions,vertices,coords, noiseMap);
	
	
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


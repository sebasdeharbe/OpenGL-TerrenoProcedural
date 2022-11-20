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
#include <cstdlib>
#include <ctime>
using namespace std;

///GLOBALES
typedef struct {
	int tamanioMapa = 32;
	int numeroDeOctavas = 5;  		//numero de octavas
	int fact = 1; 					//escala de ruido
	int freq = 1;			 		//frecuencia
	int amp = 1;					//amplitud
	int seed = 0;           		//seed para el srand()
	float persistency = 2.f;       	//factor de "conservaci�n" de la frecuencia
	float lacunarity = 0.5f;       	//factor de "desvanecimiento" de la amplitud
 
}sets;
sets parametros;
bool reload = true; 

///FUNCIONES
//Perlin
vector<vector<float>> createNoiseMap();
void generarOctava(vector<vector<float>> &nuevaOctava, float amplitud, int tamanioSubdivision);

//Modificar Malla
void modifyMesh(const std::vector<glm::vec3> &v, std::vector<glm::vec3> &vertices, std::vector<glm::vec2> &coords, vector<vector<float>> &noiseMap);

//Auxiliares
float interpolacionBilineal(float x1,float y1,float x2,float y2, float v1,float v2,float v3,float v4,float tx,float ty);
float interpolacionLineal(float x1,float x2, float v1,float v2, float tx);
float interpolarAltura(int i, float xMin, float xMax, float zMin, float zMax, const std::vector<glm::vec3> &v, vector<vector<float>> &noiseMap );

int main() {
	// initialize window and setup callbacks
	Window window(win_width,win_height,"Terreno Procedural");
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
	
	do {
		if(reload){
			vector<vector<float>> noiseMap = createNoiseMap();
			modifyMesh(plane.geometry.positions,vertices,coords, noiseMap);
			plane.buffers.updatePositions(vertices,true);
			plane.buffers.updateTexCoords(coords,true);
			plane.texture = Texture("models/elevation_gradient_3.png",true,true);
			reload = false;
		}
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
		
		// IMGUI
		window.ImGuiDialog("Parametros Perlin",[&](){
			if(ImGui::InputInt("Tamanio Mapa", &parametros.tamanioMapa)) {
				if(parametros.tamanioMapa<8) parametros.tamanioMapa = 8; //M�nimo debe existir 1 octava.
				reload = true;
			}
			if(ImGui::InputInt("Semilla Rand",&parametros.seed)) reload = true;
			if(ImGui::InputInt("Frecuencia", &parametros.freq)){
				if(parametros.freq<0) parametros.freq = 0; //M�nimo debe existir 1 octava.
				reload = true;
			}
			if(ImGui::InputInt("Amplitud", &parametros.amp)){
				if(parametros.amp<0) parametros.amp = 0; //M�nimo debe existir 1 octava.
				reload = true;
			}
			if(ImGui::InputInt("Cantidad Octavas", &parametros.numeroDeOctavas)){
				if(parametros.numeroDeOctavas<1) parametros.numeroDeOctavas = 1; //M�nimo debe existir 1 octava.
				reload = true;
			}
			if(ImGui::SliderFloat("Persistencia", &parametros.persistency, 1, 10)) reload = true;
			if(ImGui::SliderFloat("Lacunarity", &parametros.lacunarity, 0, 1)) reload = true;
		});
		
		// finish frame
		glfwSwapBuffers(window);
		glfwPollEvents();
		
	} while(glfwGetKey(window,GLFW_KEY_ESCAPE)!=GLFW_PRESS && !glfwWindowShouldClose(window) );
}

///IMPLEMENTACI�N FUNCIONES
float interpolacionBilineal(float x1,float y1,float x2,float y2, float v1,float v2,float v3,float v4,float tx,float ty){
	float sumV1=fabs((tx-x1)*(ty-y1))*v4;
	float sumV2=fabs((tx-x2)*(ty-y1))*v3;
	float sumV3=fabs((tx-x1)*(ty-y2))*v2;
	float sumV4=fabs((tx-x2)*(ty-y2))*v1;
	float areaTotal=(x2-x1)*(y2-y1);
	return (sumV1+sumV2+sumV3+sumV4)/areaTotal;
}	
float interpolacionLineal(float x1,float x2, float v1,float v2, float tx){
	float sumV1=fabs(tx-x1)*v2;
	float sumV2=fabs(tx-x2)*v1;
	float total=fabs(x2-x1);
	return (sumV1+sumV2)/total;
}
float interpolarAltura(int i, float xMin, float xMax, float zMin, float zMax, const std::vector<glm::vec3> &v, vector<vector<float>> &noiseMap ) {
	
	float deltaX = abs(xMax - xMin);
	float deltaZ = abs(zMax - zMin);
	
	float xRuido = ((v[i].x-(float)xMin)/((float)deltaX)) * (float)parametros.tamanioMapa;
	float zRuido = ((v[i].z-(float)zMin)/((float)deltaZ)) * (float)parametros.tamanioMapa;
	
	float xInterpolacionMin = floor(xRuido);
	float xInterpolacionMax = ceil(xRuido);
	
	float zInterpolacionMin = floor(zRuido);
	float zInterpolacionMax = ceil(zRuido);
	
	float valorInterpolado = 0;
	if(xInterpolacionMax == xInterpolacionMin){ //Si el punto a interpolar se encuentra perfectamente entre 2 puntos del mapa de ruido se hace una interpolaci�n lineal directamente.
		if(zInterpolacionMax == zInterpolacionMin) valorInterpolado = noiseMap[xInterpolacionMin][zInterpolacionMin]; //Si el punto se encuentra donde hay un valor en el mapa de ruido nisiquiera se interpola nada. Tomamos el valor y listo
		else valorInterpolado = interpolacionLineal(zInterpolacionMin, zInterpolacionMax, noiseMap[xInterpolacionMin][zInterpolacionMin], noiseMap[xInterpolacionMin][zInterpolacionMax], zRuido);
	}else if(zInterpolacionMax == zInterpolacionMin){
		valorInterpolado = interpolacionLineal(xInterpolacionMin, xInterpolacionMax, noiseMap[xInterpolacionMin][zInterpolacionMax], noiseMap[xInterpolacionMax][zInterpolacionMax], xRuido);
	}else{
		valorInterpolado = interpolacionBilineal(xInterpolacionMin, zInterpolacionMin, xInterpolacionMax, zInterpolacionMax,
												 noiseMap[xInterpolacionMin][zInterpolacionMin], noiseMap[xInterpolacionMax][zInterpolacionMin],
													 noiseMap[xInterpolacionMin][zInterpolacionMax], noiseMap[xInterpolacionMax][zInterpolacionMax],
														 xRuido, zRuido);
	}
	
	return valorInterpolado;
}
void generarOctava(vector<vector<float>> &nuevaOctava, float amplitud, int tamanioSubdivision){
	for(int i=0;i<=parametros.tamanioMapa;i+=tamanioSubdivision){
		for(int j=0;j<=parametros.tamanioMapa;j+=tamanioSubdivision){
			//Crear nodos
			nuevaOctava[i][j] = amplitud*((float)rand()/RAND_MAX);
			//Interpolar puntos interiores
			if(i>=tamanioSubdivision && j>=tamanioSubdivision){
				for(int a=i-tamanioSubdivision; a<=i; a++){
					for(int b=j-tamanioSubdivision; b<=j; b++){
						int x1=i-tamanioSubdivision;
						int x2=i;
						int y1=j-tamanioSubdivision;
						int y2=j;
						nuevaOctava[a][b] = interpolacionBilineal(x1, y1, x2, y2, nuevaOctava[x1][y1], nuevaOctava[x2][y1], nuevaOctava[x1][y2], nuevaOctava[x2][y2], a, b);
					}
				} 
			}
		}
	}
}
vector<vector<float>> createNoiseMap(){
	srand(parametros.seed); 
	vector<vector<float>> noiseMap(parametros.tamanioMapa+1,vector<float>(parametros.tamanioMapa+1));
	
	float frecuencia = parametros.freq;
	float amplitud = parametros.amp;
	int tamanioSubdivision = parametros.tamanioMapa/frecuencia;
	if(tamanioSubdivision<1) tamanioSubdivision=1; //NO DEBE EXISTIR UNA SUBDIVISION MENOR A 1
	cout<<"OCTAVA: "<<1<<endl<<"Amplitud: "<<amplitud<<" - Frecuencia "<<frecuencia<<"Subdivision: "<<tamanioSubdivision<<endl;
	
	///PRIMERA OCTAVA
	generarOctava(noiseMap, amplitud, tamanioSubdivision);
	
	///SEGUNDA Y DEM�S OCTAVAS
	for(int o=2;o<=parametros.numeroDeOctavas;o++) { 
		frecuencia *= parametros.persistency;
		amplitud *= parametros.lacunarity;
		tamanioSubdivision = parametros.tamanioMapa/frecuencia;
		if(tamanioSubdivision<1) tamanioSubdivision=1; //NO DEBE EXISTIR UNA SUBDIVISION MENOR A 1
		cout<<"OCTAVA: "<<o<<endl<<"Amplitud: "<<amplitud<<" - Frecuencia "<<frecuencia<<"Subdivision: "<<tamanioSubdivision<<endl;
		
		vector<vector<float>> nuevaOctava(parametros.tamanioMapa+1,vector<float>(parametros.tamanioMapa+1));
		generarOctava(nuevaOctava, amplitud, tamanioSubdivision);
		//Acumular la nuevaOctava
		for(int m=0; m<noiseMap.size();m++) { 
			for(int n=0; n<noiseMap[m].size();n++) { 
				noiseMap[m][n] += nuevaOctava[m][n];
			}
		}
	}
	return noiseMap;
}
void modifyMesh(const std::vector<glm::vec3> &v, std::vector<glm::vec3> &vertices, std::vector<glm::vec2> &coords, vector<vector<float>> &noiseMap) {
	
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
	
	float valorInterpolado = 0.f;
	for(int i=0;i<v.size();i++) { 
		valorInterpolado = interpolarAltura(i, xMin, xMax, zMin, zMax, v, noiseMap); //Saqu� toda la interpolaci�n a una funci�n
		
		vertices[i] = glm::vec3(v[i].x,valorInterpolado*0.3f,v[i].z);
		
		float s = vertices[i].y*2.f-0.2f;
		if(s<0.001f)s=0.001f;
		if(s>0.999f)s=0.999f;
		float t = 0.5f;
		coords[i] = glm::vec2(s,t);
	}
}

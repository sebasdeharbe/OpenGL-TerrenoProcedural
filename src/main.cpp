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
	int tamanioMapa = 64;
	int numeroDeOctavas = 8;  		//numero de octavas
	int fact = 1; 					//escala de ruido
	int freq = 1;			 		//frecuencia
	int amp = 1;					//amplitud
	int seed = 0;           		//seed para el srand()
	float persistency = 2.f;       	//factor de "conservaci?n" de la frecuencia
	float lacunarity = 0.5f;       	//factor de "desvanecimiento" de la amplitud
	float nivelMar = 0.4f;       	//esto sube el nivel del mar
	bool objetosActivados = false;	//lit
	bool wireframe = false;			//wireframe
}sets;
sets parametros;
bool reload = true; 

///FUNCIONES
//Perlin
vector<vector<float>> createNoiseMap();
void generarOctava(vector<vector<float>> &nuevaOctava, float amplitud, int tamanioSubdivision);

//Modificar Malla
void modifyMesh(const std::vector<glm::vec3> &v, const std::vector<glm::vec3> &n, std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals, std::vector<glm::vec2> &coords, vector<vector<float>> &noiseMap);

//Auxiliares
float interpolacionBilineal(float x1,float z1,float x2,float z2, float v1,float v2,float v3,float v4,float tx,float ty);
glm::vec3 interpolacionBilinealParanormal(float x1,float z1,float x2,float z2, float v1,float v2,float v3,float v4,float tx,float ty);
float interpolacionLineal(float x1,float x2, float v1,float v2, float tx);
void interpolarAltura(int i, float xMin, float xMax, float zMin, float zMax, const std::vector<glm::vec3> &v, vector<vector<float>> &noiseMap, float &valorInterpolado, glm::vec3 &normal );

int main() {
	
	// initialize window and setup callbacks
	Window window(win_width,win_height,"Terreno Procedural",true);
	setCommonCallbacks(window);
	
	// setup OpenGL state and load shaders
	glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS);
	glEnable(GL_BLEND); glad_glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.7f,0.7f,0.7f,1.f);
	
	
	Shader shader_phong("shaders/texture");
	Shader shader_wire("shaders/wireframe");
	
	// Este es el terreno
	auto models = Model::load("mallaRefinada",Model::fKeepGeometry);
	Model &plane = models[0];
	plane.texture = Texture("models/elevation_gradient_3.png",false,false);
	
	// Estos son los yuyos
	vector<Model> yuyos(20);
	vector<glm::mat4> yuyosMats(yuyos.size());
	for(int i=0;i<yuyos.size();i++) { 
		yuyos[i] = Model::loadSingle("bush",Model::fKeepGeometry);
		yuyos[i].texture = Texture("models/green.png",true,true);
	}
	
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normales;
	std::vector<glm::vec2> coords;
	
	do {
		
		if(parametros.numeroDeOctavas < 3) parametros.objetosActivados = false;
		
		if(reload){
			
			vector<vector<float>> noiseMap = createNoiseMap();
			
			modifyMesh(plane.geometry.positions, plane.geometry.normals, vertices, normales, coords, noiseMap);
			plane.buffers.updatePositions(vertices,true);
			plane.buffers.updateTexCoords(coords,true);
			plane.buffers.updateNormals(normales,true);
			
			//YUYOS
			for(int i=0;i<20;i++) { 
				float y = 0.f;
				float x = 0.f;
				float z = 0.f;
				while (((y - 0.4f<parametros.nivelMar)) && parametros.nivelMar<0.7f && parametros.objetosActivados){
					
					x = ((float)rand()/RAND_MAX)*2 - 1.f;
					z = ((float)rand()/RAND_MAX)*2 - 1.f;
					
					float xRuido = (x+1.f)/(2.f) * (float)parametros.tamanioMapa;
					float zRuido = (z+1.f)/(2.f) * (float)parametros.tamanioMapa;
					
					float xInterMin = floor(xRuido);
					float xInterMax = ceil(xRuido);
					
					float zInterMin = floor(zRuido);
					float zInterMax = ceil(zRuido);
					
					
					y = interpolacionBilineal(xInterMin, zInterMin, xInterMax, zInterMax,
													noiseMap[xInterMin][zInterMin], noiseMap[xInterMax][zInterMin],
														noiseMap[xInterMin][zInterMax], noiseMap[xInterMax][zInterMax],
															xRuido, zRuido);
					
					if(y != y) y=-999.f;
					
				}
				
				float size = ((float) rand()/RAND_MAX)*0.01f + 0.005f;
				
				yuyosMats[i] = glm::mat4(	size,	0.00f,  0.00f,	0.00f,
										  0.00f,	size,	0.00f,	0.00f,
										  0.00f,	0.00f,	size,	0.00f,
										  x,	(y-parametros.nivelMar+ size*0.5f), z,	1.00f);
				
				size *= 10000.f;
				yuyosMats[i] = yuyosMats[i] * 	glm::mat4(cos(size),	0.00f,  sin(size),	0.00f,
												0.0f,	1.00f,  0.00f,	0.00f,
												-1.f*sin(size),	0.00f,  cos(size),	0.00f,
												0.0f,	0.00f,  0.00f,	1.00f);
				
			}
			
			reload = false;
		}
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		Shader &shader = parametros.wireframe ? shader_wire : shader_phong;
		shader.use();
//		glColor3f(1.f,0.64f,0.f);
		setMatrixes(shader);
		shader.setLight(glm::vec4{0.f, 1.f, 1.f, 0.f}, glm::vec3{1.f,1.f,1.f}, 0.0f);
		for(Model &mod : models) {
			mod.texture.bind();
			shader.setMaterial(mod.material);
			shader.setBuffers(mod.buffers);
			glPolygonMode(GL_FRONT_AND_BACK,parametros.wireframe ? GL_LINE : GL_FILL);
			
			mod.buffers.draw();
		}
		
		if(!parametros.wireframe && parametros.objetosActivados){
			shader.use();
			int i=0;//Dibujar yuyos
			for(Model &mod : yuyos) {
				mod.texture.bind();
				glm::mat4 model_matrix = 	glm::rotate(glm::mat4(1.f), view_angle,glm::vec3{1.f,0.f,0.f}) *
											glm::rotate(glm::mat4(1.f), model_angle,glm::vec3{0.f,1.f,0.f}) * yuyosMats[i];
				
				glm::mat4 view_matrix = common_callbacks::getMatrixes()[1];
				glm::mat4 proyection_matrix = common_callbacks::getMatrixes()[2];
				
				shader.setMatrixes(model_matrix,view_matrix,proyection_matrix);
				shader.setMaterial(mod.material);
				shader.setBuffers(mod.buffers);
				glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
				mod.buffers.draw();
				i++;
			}
		}
		
		
		// IMGUI
		window.ImGuiDialog("Parametros Perlin",[&](){
			if(ImGui::InputInt("Tamanio mapa de ruido", &parametros.tamanioMapa)) {
				if(parametros.tamanioMapa<8) parametros.tamanioMapa = 8; //M?nimo debe existir 1 octava.
				reload = true;
			}
			if(ImGui::InputInt("Semilla rand",&parametros.seed)) reload = true;
			if(ImGui::InputInt("Frecuencia", &parametros.freq)){
				if(parametros.freq<1) parametros.freq = 1; 
				reload = true;
			}
			if(ImGui::InputInt("Amplitud", &parametros.amp)){
				if(parametros.amp<0) parametros.amp = 0;
				reload = true;
			}
			if(ImGui::InputInt("Cantidad octavas", &parametros.numeroDeOctavas)){
				if(parametros.numeroDeOctavas<1) parametros.numeroDeOctavas = 1; //M?nimo debe existir 1 octava.
				reload = true;
			}
			if(ImGui::SliderFloat("Persistencia", &parametros.persistency, 1, 10)) reload = true;
			if(ImGui::SliderFloat("Lacunarity", &parametros.lacunarity, 0, 1)) reload = true;
			if(ImGui::SliderFloat("Nivel del mar", &parametros.nivelMar, 0, 1)) reload = true;
			ImGui::Checkbox("Wireframe",&parametros.wireframe);
			if(ImGui::Checkbox("Objetos activados",&parametros.objetosActivados)) reload = true;
			if (ImGui::Button("Reset")) {
				parametros.tamanioMapa = 64;
				parametros.numeroDeOctavas = 8;
				parametros.fact = 1;
				parametros.freq = 1;
				parametros.amp = 1;
				parametros.seed = 0;
				parametros.persistency = 2.f; 
				parametros.lacunarity = 0.5f; 
				parametros.nivelMar = 0.4f; 
				parametros.objetosActivados = false;
				parametros.wireframe = false;
				reload = true;
			}
		});
		
		// finish frame
		glfwSwapBuffers(window);
		glfwPollEvents();
		
	} while(glfwGetKey(window,GLFW_KEY_ESCAPE)!=GLFW_PRESS && !glfwWindowShouldClose(window));
	
	cout<<"XD";
}

///IMPLEMENTACI?N FUNCIONES
float interpolacionBilineal(float x1,float z1,float x2,float z2, float v1,float v2,float v3,float v4,float tx,float ty){
	
	
	if(x1==x2) x2++;
	if(z1==z2) z2++;
	
	float sumV1=fabs((tx-x1)*(ty-z1))*v4;
	float sumV2=fabs((tx-x2)*(ty-z1))*v3;
	float sumV3=fabs((tx-x1)*(ty-z2))*v2;
	float sumV4=fabs((tx-x2)*(ty-z2))*v1;
	float areaTotal=(x2-x1)*(z2-z1);
	return (sumV1+sumV2+sumV3+sumV4)/areaTotal;
}	
	
glm::vec3 interpolacionBilinealParanormal(float x1,float z1,float x2,float z2, float v1,float v2,float v3,float v4,float tx,float ty){
	
	if(x1==x2) x2++;
	if(z1==z2) z2++;
	
	glm::vec3 n1 = glm::normalize(glm::cross( (glm::vec3(x1,v2,z2) - glm::vec3(x1,v1,z1)) , (glm::vec3(x2,v3,z1) - glm::vec3(x1,v1,z1))));
	glm::vec3 n2 = glm::normalize(glm::cross( (glm::vec3(x1,v1,z1) - glm::vec3(x2,v2,z1)) , (glm::vec3(x2,v4,z2) - glm::vec3(x2,v2,z1))));
	glm::vec3 n3 = glm::normalize(glm::cross( (glm::vec3(x2,v4,z2) - glm::vec3(x1,v3,z2)) , (glm::vec3(x1,v1,z1) - glm::vec3(x1,v3,z2))));
	glm::vec3 n4 = glm::normalize(glm::cross( (glm::vec3(x2,v3,z1) - glm::vec3(x2,v4,z2)) , (glm::vec3(x1,v2,z2) - glm::vec3(x2,v4,z2))));
	
	
	glm::vec3 sumV1=fabs((tx-x1)*(ty-z1))*n4;
	glm::vec3 sumV2=fabs((tx-x2)*(ty-z1))*n3;
	glm::vec3 sumV3=fabs((tx-x1)*(ty-z2))*n2;
	glm::vec3 sumV4=fabs((tx-x2)*(ty-z2))*n1;
	float areaTotal=(x2-x1)*(z2-z1);
	return (sumV1+sumV2+sumV3+sumV4)/areaTotal;
}	
	
	
float interpolacionLineal(float x1,float x2, float v1,float v2, float tx){
	float sumV1=fabs(tx-x1)*v2;
	float sumV2=fabs(tx-x2)*v1;
	float total=fabs(x2-x1);
	return (sumV1+sumV2)/total;
}
	
void interpolarAltura(int i, float xMin, float xMax, float zMin, float zMax, const std::vector<glm::vec3> &v, vector<vector<float>> &noiseMap, float &valorInterpolado, glm::vec3 &normal) {
	float deltaX = abs(xMax - xMin);
	float deltaZ = abs(zMax - zMin);
	
	float xRuido = ((v[i].x-(float)xMin)/((float)deltaX)) * (float)parametros.tamanioMapa;
	float zRuido = ((v[i].z-(float)zMin)/((float)deltaZ)) * (float)parametros.tamanioMapa;
	
	float xInterMin = floor(xRuido);
	float xInterMax = ceil(xRuido);
	
	float zInterMin = floor(zRuido);
	float zInterMax = ceil(zRuido);
	
	
	if(xInterMax == xInterMin){ //Si el punto a interpolar se encuentra perfectamente entre 2 puntos del mapa de ruido se hace una interpolaciï¿½n lineal directamente.
		if(zInterMax == zInterMin){//Si el punto se encuentra donde hay un valor en el mapa de ruido nisiquiera se interpola nada. Tomamos el valor y listo
			valorInterpolado = noiseMap[xInterMin][zInterMin]; 
			normal = interpolacionBilinealParanormal(xInterMin, zInterMin, xInterMax, zInterMax,
													 noiseMap[xInterMin][zInterMin], noiseMap[xInterMax][zInterMin],
														 noiseMap[xInterMin][zInterMax], noiseMap[xInterMax][zInterMax],
															 xRuido, zRuido);
		}
		else{
			valorInterpolado = interpolacionLineal(zInterMin, zInterMax, noiseMap[xInterMin][zInterMin], noiseMap[xInterMin][zInterMax], zRuido);
			
			
			normal = interpolacionBilinealParanormal(xInterMin, zInterMin, xInterMax, zInterMax,
													 noiseMap[xInterMin][zInterMin], noiseMap[xInterMax][zInterMin],
														 noiseMap[xInterMin][zInterMax], noiseMap[xInterMax][zInterMax],
															 xRuido, zRuido);
			
		}
	}else if(zInterMax == zInterMin){
		valorInterpolado = interpolacionLineal(xInterMin, xInterMax, noiseMap[xInterMin][zInterMax], noiseMap[xInterMax][zInterMax], xRuido);
		
		normal = interpolacionBilinealParanormal(xInterMin, zInterMin, xInterMax, zInterMax,
												 noiseMap[xInterMin][zInterMin], noiseMap[xInterMax][zInterMin],
													 noiseMap[xInterMin][zInterMax], noiseMap[xInterMax][zInterMax],
														 xRuido, zRuido);
		
	}else{
		valorInterpolado = interpolacionBilineal(xInterMin, zInterMin, xInterMax, zInterMax,
												 noiseMap[xInterMin][zInterMin], noiseMap[xInterMax][zInterMin],
													 noiseMap[xInterMin][zInterMax], noiseMap[xInterMax][zInterMax],
														 xRuido, zRuido);
		
		normal = interpolacionBilinealParanormal(xInterMin, zInterMin, xInterMax, zInterMax,
									   noiseMap[xInterMin][zInterMin], noiseMap[xInterMax][zInterMin],
										   noiseMap[xInterMin][zInterMax], noiseMap[xInterMax][zInterMax],
											   xRuido, zRuido);
		
	}
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
						int z1=j-tamanioSubdivision;
						int z2=j;
						nuevaOctava[a][b] = interpolacionBilineal(x1, z1, x2, z2, nuevaOctava[x1][z1], nuevaOctava[x2][z1], nuevaOctava[x1][z2], nuevaOctava[x2][z2], a, b);
					}
				} 
			}
		}
	}
	
}
	
vector<vector<float>> createNoiseMap(){
	srand(parametros.seed); 
	vector<vector<float>> noiseMap(parametros.tamanioMapa+1,vector<float>(parametros.tamanioMapa+1,0.f));
	
	float frecuencia = parametros.freq;
	float amplitud = parametros.amp;
	int tamanioSubdivision = parametros.tamanioMapa/frecuencia;
	
	if(tamanioSubdivision<1) tamanioSubdivision=1; //NO DEBE EXISTIR UNA SUBDIVISION MENOR A 1
	
	///PRIMERA OCTAVA
	generarOctava(noiseMap, amplitud, tamanioSubdivision);
	
	///SEGUNDA Y DEM?S OCTAVAS
	for(int o=1;o<parametros.numeroDeOctavas;o++) { 
		frecuencia *= parametros.persistency;
		amplitud *= parametros.lacunarity;
		
		tamanioSubdivision = parametros.tamanioMapa/frecuencia;
		if(tamanioSubdivision<1) tamanioSubdivision=1; //NO DEBE EXISTIR UNA SUBDIVISION MENOR A 1
		
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
void modifyMesh(const std::vector<glm::vec3> &v, const std::vector<glm::vec3> &n, std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals, std::vector<glm::vec2> &coords, vector<vector<float>> &noiseMap) {
	
	vertices.resize(v.size());
	normals.resize(v.size());
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
	glm::vec3 normal(0.f,1.f,0.f);
	
	for(int i=0;i<v.size();i++) { 
		interpolarAltura(i, xMin, xMax, zMin, zMax, v, noiseMap, valorInterpolado, normal); 
		
		vertices[i] = glm::vec3(v[i].x,valorInterpolado-parametros.nivelMar,v[i].z);
		normals[i] = normal;
		
		float s = 0.001f;
		if(parametros.amp>0.f) s = vertices[i].y / (parametros.amp);
		
		if(s<0.001f)s=0.001f;
		if(s>0.999f)s=0.999f;
		float t = 0.5f;
		coords[i] = glm::vec2(s,t);
	}
}

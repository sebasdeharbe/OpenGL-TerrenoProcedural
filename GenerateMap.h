#ifndef GENERATEMAP_H
#define GENERATEMAP_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <GL/gl.h>

struct terrainColor {
	terrainColor(float _height, glm::vec3 _color) {
		height = _height;
		color = _color;
	};
float height;
glm::vec3 color;
};

struct plant {
	std::string type;
	float xpos;
	float ypos;
	float zpos;
	int xOffset;
	int yOffset;
	
	plant(std::string _type, float _xpos, float _ypos, float _zpos, int _xOffset, int _yOffset) {
		type = _type;
		xpos = _xpos;
		ypos = _ypos;
		zpos = _zpos;
		xOffset = _xOffset;
		yOffset = _yOffset;
	}
};

class GenerateMap {
public:
	GenerateMap(int xMapChunks, int yMapChunks);
//	/// 6 - GENERAR MAPA
//	void generate_map_chunk(GLuint &VAO, int xOffset, int yOffset, std::vector<plant> &plants);
	
	
	//Adicional
	glm::vec3 get_color(int r, int g, int b) {
		return glm::vec3(r/255.0, g/255.0, b/255.0);
	}
	///Métodos
	/// 1 - GENERAR ÍNDICES
	std::vector<int> generate_indices();
	/// 2 - GENERAR MAPA DE RUIDO DE PERLIN
	std::vector<float> generate_noise_map(int xOffset, int yOffset);
	/// 3 - GENERAR VÉRTICES del mapa de Ruido
	std::vector<float> generate_vertices(const std::vector<float> &noise_map);
	/// 4 - GENERAR NORMALES
	std::vector<float> generate_normals(const std::vector<int> &indices, const std::vector<float> &vertices);
	/// 5 - GENERAR BIOMA
	std::vector<float> generate_biome(const std::vector<float> &vertices, std::vector<plant> &plants, int xOffset, int yOffset);
private:
	///Atributos
	float WATER_HEIGHT = 0.05;
	int chunk_render_distance = 6;
	int xMapChunks = 6;
	int yMapChunks = 6;
	int chunkWidth = 128;
	int chunkHeight = 128;
	int gridPosX = 0;
	int gridPosY = 0;
	float originX = (chunkWidth  * xMapChunks) / 2 - chunkWidth / 2;
	float originY = (chunkHeight * yMapChunks) / 2 - chunkHeight / 2;
	
	
	///Estos parámetros hacerlos en la clase de PERLIN_H
	int octaves = 8;
	float meshHeight = 255;  // Vertical scaling
	float noiseScale = 128;  // Horizontal scaling
	float persistence = 0.5;
	float lacunarity = 2;
	
	
	
};

#endif


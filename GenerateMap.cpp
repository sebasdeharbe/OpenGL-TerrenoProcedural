#include "GenerateMap.h"

#include <iostream>
#include <math.h>
#include <cstdlib>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include "Perlin.hpp"



GenerateMap::GenerateMap (int xMapChunks, int yMapChunks) {
	this->xMapChunks = xMapChunks; 
	this->yMapChunks = yMapChunks; 
}

std::vector<int> GenerateMap::generate_indices ( ) {
	std::vector<int> indices;
	
	for (int y = 0; y < chunkHeight; y++)
		for (int x = 0; x < chunkWidth; x++) {
			int pos = x + y*chunkWidth;
			
			if (x == chunkWidth - 1 || y == chunkHeight - 1) {
				// Don't create indices for right or top edge
				continue;
			} else {
				// Top left triangle of square
				indices.push_back(pos + chunkWidth);
				indices.push_back(pos);
				indices.push_back(pos + chunkWidth + 1);
				// Bottom right triangle of square
				indices.push_back(pos + 1);
				indices.push_back(pos + 1 + chunkWidth);
				indices.push_back(pos);
			}
	}
		
		return indices;
}

std::vector<float> GenerateMap::generate_noise_map (int xOffset, int yOffset) {
	std::vector<float> noiseValues;
	std::vector<float> normalizedNoiseValues;
	std::vector<int> p = get_permutation_vector();
	
	float amp  = 1;
	float freq = 1;
	float maxPossibleHeight = 0;
	
	for (int i = 0; i < octaves; i++) {
		maxPossibleHeight += amp;
		amp *= persistence;
	}
	
	for (int y = 0; y < chunkHeight; y++) {
		for (int x = 0; x < chunkWidth; x++) {
			amp  = 1;
			freq = 1;
			float noiseHeight = 0;
			for (int i = 0; i < octaves; i++) {
				float xSample = (x + xOffset * (chunkWidth-1))  / noiseScale * freq;
				float ySample = (y + yOffset * (chunkHeight-1)) / noiseScale * freq;
				
				float perlinValue = perlin_noise(xSample, ySample, p);
				noiseHeight += perlinValue * amp;
				
				// Lacunarity  --> Increase in frequency of octaves
				// Persistence --> Decrease in amplitude of octaves
				amp  *= persistence;
				freq *= lacunarity;
			}
			
			noiseValues.push_back(noiseHeight);
		}
	}
	
	for (int y = 0; y < chunkHeight; y++) {
		for (int x = 0; x < chunkWidth; x++) {
			// Inverse lerp and scale values to range from 0 to 1
			normalizedNoiseValues.push_back((noiseValues[x + y*chunkWidth] + 1) / maxPossibleHeight);
		}
	}
	
	return normalizedNoiseValues;
}

std::vector<float> GenerateMap::generate_vertices (const std::vector<float> & noise_map) {
	std::vector<float> v;
	
	for (int y = 0; y < chunkHeight; y++)
		for (int x = 0; x < chunkWidth; x++) {
			v.push_back(x);
			// Apply cubic easing to the noise
			
			float easedNoise = std::pow(noise_map[x + y*(chunkWidth)] * 1.1, 3);
			// Scale noise to match meshHeight
			// Pervent vertex height from being below WATER_HEIGHT
			v.push_back(std::fmax(easedNoise * meshHeight, WATER_HEIGHT * 0.5 * meshHeight));
			v.push_back(y);
	}
		
	return v;
}

std::vector<float> GenerateMap::generate_normals (const std::vector<int> & indices, const std::vector<float> & vertices) {
	int pos;
	glm::vec3 normal;
	std::vector<float> normals;
	std::vector<glm::vec3> verts;
	
	// Get the vertices of each triangle in mesh
	// For each group of indices
	for (int i = 0; i < indices.size(); i += 3) {
		
		// Get the vertices (point) for each index
		for (int j = 0; j < 3; j++) {
			pos = indices[i+j]*3;
			verts.push_back(glm::vec3(vertices[pos], vertices[pos+1], vertices[pos+2]));
		}
		
		// Get vectors of two edges of triangle
		glm::vec3 U = verts[i+1] - verts[i];
		glm::vec3 V = verts[i+2] - verts[i];
		
		// Calculate normal
		normal = glm::normalize(-glm::cross(U, V));
		normals.push_back(normal.x);
		normals.push_back(normal.y);
		normals.push_back(normal.z);
	}
	
	return normals;
}

std::vector<float> GenerateMap::generate_biome (const std::vector<float> & vertices, std::vector<plant> & plants, int xOffset, int yOffset) {
	std::vector<float> colors;
	std::vector<terrainColor> biomeColors;
	glm::vec3 color = get_color(255, 255, 255);
	
	// NOTE: Terrain color height is a value between 0 and 1
	//    biomeColors.push_back(terrainColor(WATER_HEIGHT * 0.5, get_color(60,  95, 190)));   // Deep water
	biomeColors.push_back(terrainColor(WATER_HEIGHT,        get_color(60, 100, 190)));  // Shallow water
	biomeColors.push_back(terrainColor(0.15, get_color(210, 215, 130)));                // Sand
	biomeColors.push_back(terrainColor(0.30, get_color( 95, 165,  30)));                // Grass 1
	//    biomeColors.push_back(terrainColor(0.40, get_color( 65, 115,  20)));                // Grass 2
	biomeColors.push_back(terrainColor(0.75, get_color( 90,  65,  60)));                // Rock 1
	//    biomeColors.push_back(terrainColor(0.80, get_color( 75,  60,  55)));                // Rock 2
	biomeColors.push_back(terrainColor(1.00, get_color(255, 255, 255)));                // Snow
	
	std::string plantType;
	
	// Determine which color to assign each vertex by its y-coord
	// Iterate through vertex y values
	for (int i = 1; i < vertices.size(); i += 3) {
		for (int j = 0; j < biomeColors.size(); j++) {
			// NOTE: The max height of a vertex is "meshHeight"
			if (vertices[i] <= biomeColors[j].height * meshHeight) {
				color = biomeColors[j].color;
				if (j == 3) {
					if (rand() % 1000 < 5) {
						if (rand() % 100 < 70) {
							plantType = "flower";
						} else {
							plantType = "tree";
						}
						plants.push_back(plant{plantType, vertices[i-1], vertices[i], vertices[i+1], xOffset, yOffset});
					}
				}
				break;
			}
		}
		colors.push_back(color.r);
		colors.push_back(color.g);
		colors.push_back(color.b);
	}
	return colors;
}



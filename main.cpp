#include <cmath>
#include <string>
#include <random>
#include <iostream>
#include <math.h>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "lib/tiny_obj_loader.h"
#include "Shader.h"
#include "Camera.h"
#include "GenerateMap.h"

const GLint WIDTH = 1000, HEIGHT = 800;

// Functions
int init();
void processInput(GLFWwindow *window, Shader &shader);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void render(std::vector<GLuint> &map_chunks, Shader &shader, glm::mat4 &view, glm::mat4 &model, glm::mat4 &projection, int &nIndices, std::vector<GLuint> &tree_chunks, std::vector<GLuint> &flower_chunks);

//std::vector<int> generate_indices();
//std::vector<float> generate_noise_map(int xOffset, int yOffset);
//std::vector<float> generate_vertices(const std::vector<float> &noise_map);
//std::vector<float> generate_normals(const std::vector<int> &indices, const std::vector<float> &vertices);
//std::vector<float> generate_biome(const std::vector<float> &vertices, std::vector<plant> &plants, int xOffset, int yOffset);
void generate_map_chunk(GenerateMap &Mapa, GLuint &VAO, int xOffset, int yOffset, std::vector<plant> &plants);

void load_model(GLuint &VAO, std::string filename);
void setup_instancing(GLuint &VAO, std::vector<GLuint> &plant_chunk, std::string plant_type, std::vector<plant> &plants, std::string filename);

GLFWwindow *window;

// Map params
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

// Noise params
int octaves = 8;
float meshHeight = 255;  // Vertical scaling
float noiseScale = 128;  // Horizontal scaling
float persistence = 0.5;
float lacunarity = 2;

// Model params
float MODEL_SCALE = 3;
float MODEL_BRIGHTNESS = 6;

// FPS
double lastTime = glfwGetTime();
int nbFrames = 0;

// Camera
Camera camera(glm::vec3(originX, chunkHeight*2.f, originY));
bool firstMouse = true;
float lastX = WIDTH / 2;
float lastY = HEIGHT / 2;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float currentFrame;

//Buffers
GLuint VBO, EBO;

int main() {
    // Initalize variables
    glm::mat4 view;
    glm::mat4 model;
    glm::mat4 projection;
    std::vector<plant> plants;

    // Initialize GLFW and GLAD
    if (init() != 0)
        return -1;
    
    Shader shaders("shaders/objectShader.vert", "shaders/objectShader.frag");
    
    // Default to coloring to flat mode
    shaders.use();
    shaders.setBool("isFlat", true);
    
    // Lighting intensities and direction
    shaders.setVec3("light.ambient", 0.2, 0.2, 0.2);
    shaders.setVec3("light.diffuse", 0.3, 0.3, 0.3);
    shaders.setVec3("light.specular", 1.0, 1.0, 1.0);
    shaders.setVec3("light.direction", -0.2f, -1.0f, -0.3f);
    
    std::vector<GLuint> map_chunks(xMapChunks * yMapChunks);
    
	///Mapa
	GenerateMap Mapa(xMapChunks, yMapChunks);
	
    for (int y = 0; y < yMapChunks; y++)
        for (int x = 0; x < xMapChunks; x++) {
            generate_map_chunk(Mapa, map_chunks[x + y*xMapChunks], x, y, plants);
        }
    
    int nIndices = chunkWidth * chunkHeight * 6;
    
    GLuint treeVAO, flowerVAO;
    std::vector<GLuint> tree_chunks(xMapChunks * yMapChunks);
    std::vector<GLuint> flower_chunks(xMapChunks * yMapChunks);
    
//    setup_instancing(treeVAO, tree_chunks, "tree", plants, "obj/tree.obj");
//    setup_instancing(flowerVAO, flower_chunks, "flower", plants, "obj/Flowers.obj");
    
	///LOOP PRINCIPAL
	do{
        shaders.use();
        projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, (float)chunkWidth * (chunk_render_distance - 1.2f));
        view = camera.GetViewMatrix();
        shaders.setMat4("u_projection", projection);
        shaders.setMat4("u_view", view);
        shaders.setVec3("u_viewPos", camera.Position);
        
        render(map_chunks, shaders, view, model, projection, nIndices, tree_chunks, flower_chunks);
    }while (glfwGetKey(window,GLFW_KEY_ESCAPE)!=GLFW_PRESS && !glfwWindowShouldClose(window));
    
    for (int i = 0; i < map_chunks.size(); i++) {
        glDeleteVertexArrays(1, &map_chunks[i]);
        glDeleteVertexArrays(1, &tree_chunks[i]);
        glDeleteVertexArrays(1, &flower_chunks[i]);
    }
    
    // TODO VBOs and EBOs aren't being deleted
	glDeleteBuffers(3, &VBO);
    glDeleteBuffers(1, &EBO);
    
    glfwTerminate();
    
    return 0;
}

void setup_instancing(GLuint &VAO, std::vector<GLuint> &plant_chunk, std::string plant_type, std::vector<plant> &plants, std::string filename) {
    std::vector<std::vector<float>> chunkInstances;
    chunkInstances.resize(xMapChunks * yMapChunks);
    
    // Instancing prep
    for (int i = 0; i < plants.size(); i++) {
        float xPos = plants[i].xpos / MODEL_SCALE;
        float yPos = plants[i].ypos / MODEL_SCALE;
        float zPos = plants[i].zpos / MODEL_SCALE;
        int pos = plants[i].xOffset + plants[i].yOffset*xMapChunks;
        
        if (plants[i].type == plant_type) {
            chunkInstances[pos].push_back(xPos);
            chunkInstances[pos].push_back(yPos);
            chunkInstances[pos].push_back(zPos);
        }
    }
    
    GLuint instancesVBO[xMapChunks * yMapChunks];
    glGenBuffers(xMapChunks * yMapChunks, instancesVBO);
    
    for (int y = 0; y < yMapChunks; y++) {
        for (int x = 0; x < xMapChunks; x++) {
            int pos = x + y*xMapChunks;
            load_model(plant_chunk[pos], filename);
            
            glBindVertexArray(plant_chunk[pos]);
            glBindBuffer(GL_ARRAY_BUFFER, instancesVBO[pos]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * chunkInstances[pos].size(), &chunkInstances[pos][0], GL_STATIC_DRAW);
            
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            
            // Instanced array
            // Move to next vertex attrib on next instance of object
            glVertexAttribDivisor(3, 1);
        }
    }
}

void render(std::vector<GLuint> &map_chunks, Shader &shader, glm::mat4 &view, glm::mat4 &model, glm::mat4 &projection, int &nIndices, std::vector<GLuint> &tree_chunks, std::vector<GLuint> &flower_chunks) {
    // Per-frame time logic
    currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    processInput(window, shader);
    
    glClearColor(0.53, 0.81, 0.92, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Measures number of map chunks away from origin map chunk the camera is
    gridPosX = (int)(camera.Position.x - originX) / chunkWidth + xMapChunks / 2;
    gridPosY = (int)(camera.Position.z - originY) / chunkHeight + yMapChunks / 2;
    
    // Render map chunks
    for (int y = 0; y < yMapChunks; y++)
        for (int x = 0; x < xMapChunks; x++) {
            // Only render chunk if it's within render distance
            if (std::abs(gridPosX - x) <= chunk_render_distance && (y - gridPosY) <= chunk_render_distance) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-chunkWidth / 2.0 + (chunkWidth - 1) * x, 0.0, -chunkHeight / 2.0 + (chunkHeight - 1) * y));
                shader.setMat4("u_model", model);
                
                // Terrain chunk
                glBindVertexArray(map_chunks[x + y*xMapChunks]);
                glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
                
                // Plant chunks
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-chunkWidth / 2.0 + (chunkWidth - 1) * x, 0.0, -chunkHeight / 2.0 + (chunkHeight - 1) * y));
                model = glm::scale(model, glm::vec3(MODEL_SCALE));
                shader.setMat4("u_model", model);

                glEnable(GL_CULL_FACE);
                glBindVertexArray(flower_chunks[x + y*xMapChunks]);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 1300, 16);

                glBindVertexArray(tree_chunks[x + y*xMapChunks]);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 10192, 8);
                glDisable(GL_CULL_FACE);
            }
        }
    
    // Measure speed in ms per frame
    double currentTime = glfwGetTime();
    nbFrames++;
    // If last prinf() was more than 1 sec ago printf and reset timer
    if (currentTime - lastTime >= 1.0 ){
//        printf("%f ms/frame\n", 1000.0/double(nbFrames));
        nbFrames = 0;
        lastTime += 1.0;
    }
    
    // Use double buffer
    // Only swap old frame with new when it is completed
    glfwPollEvents();
    glfwSwapBuffers(window);
}

void load_model(GLuint &VAO, std::string filename) {
    std::vector<float> vertices;
    std::vector<int> indices;
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());

    if (!warn.empty()) {
        std::cout << warn << std::endl;
    } else if (!err.empty()) {
        std::cerr << err << std::endl;
    }
    
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                vertices.push_back(attrib.vertices[3*idx.vertex_index+0]);
                vertices.push_back(attrib.vertices[3*idx.vertex_index+1]);
                vertices.push_back(attrib.vertices[3*idx.vertex_index+2]);
                vertices.push_back(attrib.normals[3*idx.normal_index+0]);
                vertices.push_back(attrib.normals[3*idx.normal_index+1]);
                vertices.push_back(attrib.normals[3*idx.normal_index+2]);
                vertices.push_back(materials[shapes[s].mesh.material_ids[f]].diffuse[0] * MODEL_BRIGHTNESS);
                vertices.push_back(materials[shapes[s].mesh.material_ids[f]].diffuse[1] * MODEL_BRIGHTNESS);
                vertices.push_back(materials[shapes[s].mesh.material_ids[f]].diffuse[2] * MODEL_BRIGHTNESS);
            }
            index_offset += fv;
        }
    }
    
//    GLuint VBO, EBO;
    
    // Create buffers and arrays
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenVertexArrays(1, &VAO);
    
    // Bind vertices to VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    
    // Configure vertex position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Configure vertex normals attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Configure vertex color attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}




// Initialize GLFW and GLAD
int init() {
    glfwInit();
    
    // Set OpenGL window to version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // macOS compatibility
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    window = glfwCreateWindow(WIDTH, HEIGHT, "Terrain Generator", nullptr, nullptr);
    
    // Account for macOS retina resolution
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, screenWidth, screenHeight);

    // Enable z-buffer
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    
    // Enable mouse input
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    return 0;
}

void processInput(GLFWwindow *window, Shader &shader) {
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

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

void generate_map_chunk (GenerateMap &Mapa, GLuint & VAO, int xOffset, int yOffset, std::vector<plant> &plants) {
	std::vector<int> indices;
	std::vector<float> noise_map;
	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> colors;
	
	// Generate map
	indices = Mapa.generate_indices();
	noise_map = Mapa.generate_noise_map(xOffset, yOffset);
	vertices = Mapa.generate_vertices(noise_map);
	normals = Mapa.generate_normals(indices, vertices);
	colors = Mapa.generate_biome(vertices, plants, xOffset, yOffset);
	
	GLuint VBO[3], EBO;
	
	// Create buffers and arrays
	glGenBuffers(3, VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);
	
	// Bind vertices to VBO
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
	
	// Create element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);
	
	// Configure vertex position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	
	// Bind vertices to VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);
	
	// Configure vertex normals attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	
	// Bind vertices to VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), &colors[0], GL_STATIC_DRAW); ///aca deber�a reemplazarse por textura
	
	
	// Configure vertex colors attribute
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(2);
}

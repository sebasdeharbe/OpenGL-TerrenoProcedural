#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//Enum para los posibles movimientos de cámara. 
enum Camera_Movement{
	FORWARD, BACKWARD, LEFT, RIGHT
};

//Valores predeteminados de la cámara
const float YAW = -90.f; //ángulo de giro eje z
const float PITCH = 0.0f; //ángulo de giro eje x
const float SPEED = 32.0f;
const float SENSITIVITY = 0.05f;
const float ZOOM = 45.0f;

class Camera {
public:
	///Atributos de cámara
	glm::vec3 Position; 
	glm::vec3 Front; 
	glm::vec3 Up; 
	glm::vec3 Right; 
	glm::vec3 WorldUp; 
	float Yaw;
	float Pitch; 
	float MovementSpeed;
	float MouseSensitivity; ///opcional, quizás se puede dinamitar
	float Zoom; 
	
	///Métodos
	//Constructores
	Camera(glm::vec3 position = glm::vec3(0.f), glm::vec3 up = glm::vec3(0.f, 1.f, 0.f), 
			float yaw = YAW, float pitch = PITCH, float speed = SPEED, float sensitivity = SENSITIVITY, float zoom = ZOOM);
	//Generales
	glm::mat4 GetViewMatrix();
	//Procesos para el movimiento
	void ProcessKeyboard(Camera_Movement direction, float deltaTime);
	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);
	void ProcessMouseScroll(float yoffset);
	
private:
	void updateCameraVectors();
};

#endif


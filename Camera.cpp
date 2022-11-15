#include "Camera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

///PRIVATE
void Camera::updateCameraVectors ( ) {
	//Recalcular el vector FRONT de cámara con los ángulos de rotación de los ejes X & Z 
	glm::vec3 front;
	front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	front.y = sin(glm::radians(Pitch));
	front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front = glm::normalize(front);
	//Recalcular el vector UP y el RIGHT
	Right = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	Up    = glm::normalize(glm::cross(Right, Front));
}

///PUBLIC
Camera::Camera (glm::vec3 position, glm::vec3 up, float yaw, float pitch, float speed, float sensitivity, float zoom) {
	Position = position;
	WorldUp = up;
	Yaw = yaw;
	Pitch = pitch;
	MovementSpeed = speed;
	MouseSensitivity = sensitivity;
	Zoom = zoom;
	updateCameraVectors();
}





glm::mat4 Camera::GetViewMatrix ( ) {
	return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard (Camera_Movement direction, float deltaTime) {
	float velocity = MovementSpeed * deltaTime;
	if (direction == FORWARD)
		Position += Front * velocity;
	if (direction == BACKWARD)
		Position -= Front * velocity;
	if (direction == LEFT)
		Position -= Right * velocity;
	if (direction == RIGHT)
		Position += Right * velocity;
	if (direction == UP)
		Position += WorldUp * velocity;
	if (direction == DOWN)
		Position += WorldUp * -velocity;
}

void Camera::ProcessMouseMovement (float xoffset, float yoffset, GLboolean constrainPitch) {
	xoffset *= MouseSensitivity;
	yoffset *= MouseSensitivity;
	
	Yaw   += xoffset;
	Pitch += yoffset;
	
	// Evitar giros de cámara en límites del mapa.
	if (constrainPitch) {
		if (Pitch > 89.0f)
			Pitch = 89.0f;
		if (Pitch < -89.0f)
			Pitch = -89.0f;
	}
	
	updateCameraVectors();
}

void Camera::ProcessMouseScroll (float yoffset) {
	//El scroll del mouse se usa para aumentar/disminuir el zoom.
	if (Zoom >= 1.0f && Zoom <= 45.0f)
		Zoom -= yoffset;
	if (Zoom <= 1.0f)
		Zoom = 1.0f;
	if (Zoom >= 45.0f)
		Zoom = 45.0f;
}


#include <iostream>

#define GLM_FORCE_RADIANS
// Use Vulkan depth range of 0.0 to 1.0 instead of OpenGL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "BufferUtils.h"
#include <iostream>

Camera::Camera(Device* device, float aspectRatio,int w,int h) : device(device), 
	width(w),
	height(h),
	fovy(45.0f),
	near_clip(0.1f),
	far_clip(1000),
	eye(glm::vec3(0.0f, 8.0f, -30.0f)),
	ref(glm::vec3(0.0f, 1.0f, 0.0f)),
	world_up(glm::vec3(0.0f, 1.0f, 0.0f))
{
    r = 10.0f;
    theta = 0.0f;
    phi = 0.0f;
	RecomputeAttributes();
    cameraBufferObject.viewMatrix = glm::lookAt(eye,ref,up);
    cameraBufferObject.projectionMatrix = glm::perspective(glm::radians(fovy), aspect, near_clip, far_clip);
    cameraBufferObject.projectionMatrix[1][1] *= -1; // y-coordinate is flipped
	cameraBufferObject.camPos = glm::vec4(eye,1.0f);
	cameraBufferObject.camDir = glm::vec4(look,1.0f);

    BufferUtils::CreateBuffer(device, sizeof(CameraBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, bufferMemory);
    vkMapMemory(device->GetVkDevice(), bufferMemory, 0, sizeof(CameraBufferObject), 0, &mappedData);
    memcpy(mappedData, &cameraBufferObject, sizeof(CameraBufferObject));
}

VkBuffer Camera::GetBuffer() const {
    return buffer;
}

void Camera::UpdateOrbit(float deltaX, float deltaY, float deltaZ) {
    theta += deltaX;
    phi += deltaY;
    r = glm::clamp(r - deltaZ, 0.1f, 1000.0f);

    float radTheta = glm::radians(theta);
    float radPhi = glm::radians(phi);

    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), radTheta, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), radPhi, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 finalTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f)) * rotation * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, r));

    cameraBufferObject.viewMatrix = glm::inverse(finalTransform);
	cameraBufferObject.camPos = glm::vec4(eye, 1.0f);
	cameraBufferObject.camDir = glm::vec4(look, 1.0f);
    memcpy(mappedData, &cameraBufferObject, sizeof(CameraBufferObject));
}

void Camera::UpdateViewMatrix() {
	cameraBufferObject.viewMatrix = glm::lookAt(eye, ref, up);
	cameraBufferObject.camPos = glm::vec4(eye,1.0f);
	cameraBufferObject.camDir = glm::vec4(look,1.0f);
	//std::cout << look.x << " "<< look.y << " " << look.z << std::endl;
	memcpy(mappedData, &cameraBufferObject, sizeof(CameraBufferObject));
}
void Camera::RecomputeAttributes()
{
	look = glm::normalize(ref - eye);
	right = glm::normalize(glm::cross(look, world_up));
	up = glm::cross(right, look);

	float tan_fovy = tan(glm::radians(fovy / 2));
	float len = glm::length(ref - eye);
	aspect = (float)width / float(height);
	V = up*len*tan_fovy;
	H = right*len*aspect*tan_fovy;
}


void Camera::UpdateAspectRatio(float aspectRatio,int w,int h) {
	aspect = aspectRatio;
	width = w;
	height = h;
	cameraBufferObject.projectionMatrix = glm::perspective(glm::radians(fovy), aspect, near_clip, far_clip);
	cameraBufferObject.projectionMatrix[1][1] *= -1; // y-coordinate is flipped
	memcpy(mappedData, &cameraBufferObject, sizeof(CameraBufferObject));
}

void Camera::RotateAboutUp(float deg)
{
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(deg), up);
	ref = ref - eye;
	ref = glm::vec3(rotation * glm::vec4(ref, 1));
	ref = ref + eye;
	RecomputeAttributes();
}
void Camera::RotateAboutRight(float deg)
{

	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(deg), right);
	glm::vec3 ref_test = ref;
	ref_test = ref_test - eye;
	ref_test = glm::vec3(rotation * glm::vec4(ref_test, 1));
	ref_test = ref_test + eye;
	glm::vec3 look_test = glm::normalize(ref_test - eye);
	if (glm::length(look_test - glm::vec3(0, 1, 0))<0.1f || \
		glm::length(look_test - glm::vec3(0, -1, 0))<0.1f)
		return;
	else
	{
		ref = ref_test;
		RecomputeAttributes();
	}
}

void Camera::TranslateAlongLook(float amt)
{
	glm::vec3 translation = look * amt;
	eye += translation;
	ref += translation;
}

void Camera::TranslateAlongRight(float amt)
{
	glm::vec3 translation = right * amt;
	eye += translation;
	ref += translation;
}
void Camera::TranslateAlongUp(float amt)
{
	glm::vec3 translation = up * amt;
	eye += translation;
	ref += translation;
}
void Camera::TranslateAlongWorldUp(float amt)
{
	glm::vec3 translation = world_up * amt;
	eye += translation;
	ref += translation;
}
void Camera::CameraRotate(float deltaX, float deltaY) {
	float delta_w = 2 * deltaX / float(width)*glm::length(H);
	float delta_h = 2 * deltaY / float(height)*glm::length(V);
	float theta = atan(delta_w / glm::length(ref - eye)) * 180 / M_PI;
	float fai = atan(delta_h / glm::length(ref - eye)) * 180 / M_PI;
	RotateAboutUp(-theta);
	RecomputeAttributes();
	RotateAboutRight(-fai);
	RecomputeAttributes();
	UpdateViewMatrix();
}
void Camera::CameraTranslate(float deltaX, float deltaY) {
	float sensitive = 0.05;
	TranslateAlongRight(deltaX*sensitive);
	RecomputeAttributes();
	TranslateAlongUp(-deltaY*sensitive);
	RecomputeAttributes();
	UpdateViewMatrix();
}
void Camera::CameraScale(float amt) {
	float sensitive = 1;
	TranslateAlongLook(amt*sensitive);
	RecomputeAttributes();
	UpdateViewMatrix();
}
Camera::~Camera() {
  vkUnmapMemory(device->GetVkDevice(), bufferMemory);
  vkDestroyBuffer(device->GetVkDevice(), buffer, nullptr);
  vkFreeMemory(device->GetVkDevice(), bufferMemory, nullptr);
}

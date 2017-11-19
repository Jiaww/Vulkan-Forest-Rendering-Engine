
#pragma once

#include <glm/glm.hpp>
#include "Device.h"

struct CameraBufferObject {
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;
};

class Camera {
private:
    Device* device;
    
    CameraBufferObject cameraBufferObject;
    
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;

    void* mappedData;


	float fovy;
	unsigned int width, height;  // Window dimensions
	float near_clip;  // Near clip plane distance
	float far_clip;  // Far clip plane distance

					 //Computed attributes
	float aspect;
	glm::vec3 eye,      //The position of the camera in world space
		ref,      //The point in world space towards which the camera is pointing
		look,     //The normalized vector from eye to ref. Is also known as the camera's "forward" vector.
		up,       //The normalized vector pointing upwards IN CAMERA SPACE. This vector is perpendicular to LOOK and RIGHT.
		right,    //The normalized vector pointing rightwards IN CAMERA SPACE. It is perpendicular to UP and LOOK.
		world_up, //The normalized vector pointing upwards IN WORLD SPACE. This is primarily used for computing the camera's initial UP vector.
		V,        //Represents the vertical component of the plane of the viewing frustum that passes through the camera's reference point. Used in Camera::Raycast.
		H;        //Represents the horizontal component of the plane of the viewing frustum that passes through the camera's reference point. Used in Camera::Raycast.


    float r, theta, phi;

public:
    Camera(Device* device, float aspectRatio,int w,int h);
    ~Camera();

    VkBuffer GetBuffer() const;
    
	void RecomputeAttributes();
	
	void RotateAboutUp(float deg);
	void RotateAboutRight(float deg);
	void TranslateAlongLook(float amt);
	void TranslateAlongRight(float amt);
	void TranslateAlongUp(float amt);
	void TranslateAlongWorldUp(float amt);
	void CameraTranslate(float deltaX, float deltaY);

    void UpdateOrbit(float deltaX, float deltaY, float deltaZ);
	void UpdateAspectRatio(float aspectRatio,int w,int h);
	void UpdateViewMatrix();
};

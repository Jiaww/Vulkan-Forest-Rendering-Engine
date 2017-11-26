#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

#include "Vertex.h"
#include "Device.h"
#include "stb_image.h"
#include "Model.h"
#include <cstdio>

class Terrain {
protected:
	Device* device;

	std::vector<Vertex> vertices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	std::vector<uint32_t> indices;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkBuffer modelBuffer;
	VkDeviceMemory modelBufferMemory;

	ModelBufferObject modelBufferObject;

	// diffusse map
	VkImage diffuseMap = VK_NULL_HANDLE;
	VkImageView diffuseMapView = VK_NULL_HANDLE;
	VkSampler diffuseMapSampler = VK_NULL_HANDLE;
	// normal map
	VkImage normalMap = VK_NULL_HANDLE;
	VkImageView normalMapView = VK_NULL_HANDLE;
	VkSampler normalMapSampler = VK_NULL_HANDLE;

	int width, height;
	float *heights;
	float terrainDim;

public:
	Terrain() = delete;
	Terrain(Device* device, VkCommandPool commandPool, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
	virtual ~Terrain();

	static Terrain* LoadTerrain(Device* device, VkCommandPool commandPool, char *filePath, char *rawPath, float terrainDim);
	void SetDiffuseMap(VkImage texture);
	void SetNormalMap(VkImage texture);

	const std::vector<Vertex>& getVertices() const;

	VkBuffer getVertexBuffer() const;

	const std::vector<uint32_t>& getIndices() const;

	VkBuffer getIndexBuffer() const;

	const ModelBufferObject& getModelBufferObject() const;

	VkBuffer GetModelBuffer() const;
	VkImageView GetDiffuseMapView() const;
	VkSampler GetDiffuseMapSampler() const;
	VkImageView GetNormalMapView() const;
	VkSampler GetNormalMapSampler() const;

	float GetHeight(float x, float z) const;
};
#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

#include "Vertex.h"
#include "Device.h"

struct ModelBufferObject {
    glm::mat4 modelMatrix;
};

class Model {
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
	// noise map
	VkImage noiseMap = VK_NULL_HANDLE;
	VkImageView noiseMapView = VK_NULL_HANDLE;
	VkSampler noiseMapSampler = VK_NULL_HANDLE;

public:
    Model() = delete;
    Model(Device* device, VkCommandPool commandPool, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
    virtual ~Model();

    void SetDiffuseMap(VkImage texture);
	void SetNormalMap(VkImage texture);
	void SetNoiseMap(VkImage texture);

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
	VkImageView GetNoiseMapView() const;
	VkSampler GetNoiseMapSampler() const;
};

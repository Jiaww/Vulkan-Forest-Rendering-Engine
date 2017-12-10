#pragma once
#include <vector>

#include "Vertex.h"
#include "Device.h"
#include "stb_image.h"
#include "Model.h"
#include <cstdio>

struct Position {
	glm::vec4 pos;
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Position);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	// Get the attribute descriptions, which describe how to handle vertex input
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);
		// Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Position, pos);
		return attributeDescriptions;
	}
};
class Skybox :public Model
{
protected:
	std::vector<Position> skybox_vertices;
	VkImage skyDiffuseMap[3] = {};
	VkImageView skyDiffuseMapView[3] = {};
	VkSampler skyDiffuseMapSampler[3] = {};
public:
	Skybox() = delete;
	Skybox(Device* device, VkCommandPool commandPool, const std::vector<Position> &vertices, const std::vector<uint32_t> &indices);
	void SetDiffuseMapIdx(VkImage texture, int idx);
	VkImageView GetDiffuseMapViewIdx(int idx) const;
	VkSampler GetDiffuseMapSamplerIdx(int idx) const;
	const std::vector<Position>& getSkyboxVertices() const;
	
	virtual ~Skybox();


};
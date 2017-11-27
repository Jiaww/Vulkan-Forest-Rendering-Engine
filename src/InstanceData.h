#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include "Device.h"
struct InstanceData {
	glm::vec3 pos;
	float scale;
	float theta;
	glm::vec3 tintColor;
	InstanceData() {};
	InstanceData(glm::vec3 position, float scale, float theta, glm::vec3 tintColor) :pos(position), scale(scale), theta(theta), tintColor(tintColor) {}
	// Get the binding description, which describes the rate to load data from memory
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 1;
		bindingDescription.stride = sizeof(InstanceData);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		return bindingDescription;
	}

	// Get the attribute descriptions, which describe how to handle vertex input
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

		// Position
		attributeDescriptions[0].binding = 1;
		attributeDescriptions[0].location = 6;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(InstanceData, pos);

		// scale
		attributeDescriptions[1].binding = 1;
		attributeDescriptions[1].location = 7;
		attributeDescriptions[1].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(InstanceData, scale);

		// theta
		attributeDescriptions[2].binding = 1;
		attributeDescriptions[2].location = 8;
		attributeDescriptions[2].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(InstanceData, theta);

		// Position
		attributeDescriptions[3].binding = 1;
		attributeDescriptions[3].location = 9;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(InstanceData, tintColor);
		return attributeDescriptions;
	}
};
class InstanceBuffer {
protected:
	Device* device;
	std::vector<InstanceData> Data;
	VkBuffer DataBuffer;
	VkDeviceMemory DataMemory;
	//No indices. Indices should corespond with Model.
	int InstanceCount = 0;

public:
	InstanceBuffer() = delete;
	InstanceBuffer(Device* device, VkCommandPool commandPool, const std::vector<InstanceData> &Data);
	virtual ~InstanceBuffer();
	VkBuffer GetInstanceDataBuffer() const;
	int GetInstanceCount() const;
	

};
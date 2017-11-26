#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include "Device.h"
struct InstanceData {
	glm::vec3 pos;
	InstanceData() {};
	InstanceData(glm::vec3 position) :pos(position) {}
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
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);

		// Position
		attributeDescriptions[0].binding = 1;
		attributeDescriptions[0].location = 6;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(InstanceData, pos);

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
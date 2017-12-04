#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include "Device.h"
struct InstanceData {
	glm::vec4 pos_scale;
	glm::vec4 tintColor_theta;
	InstanceData() {};
	InstanceData(glm::vec4 position_scale, glm::vec4 tintColor_theta) :pos_scale(position_scale), tintColor_theta(tintColor_theta) {}
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
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

		// Position & Scale
		attributeDescriptions[0].binding = 1;
		attributeDescriptions[0].location = 6;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(InstanceData, pos_scale);

		// Position
		attributeDescriptions[1].binding = 1;
		attributeDescriptions[1].location = 7;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(InstanceData, tintColor_theta);
		return attributeDescriptions;
	}
};

class InstanceBuffer {
protected:
	Device* device;
	std::vector<InstanceData> Data;
	VkBuffer DataBuffer;
	//LOD 0 & LOD 1
	VkBuffer culledDataBuffer[2];
	VkBuffer numDataBuffer[3];

	VkDeviceMemory DataMemory;
	VkDeviceMemory culledDataMemory[2];
	VkDeviceMemory numDataMemory[3];
	//No indices. Indices should corespond with Model.
	int InstanceCount = 0;

public:
	InstanceBuffer() = delete;
	InstanceBuffer(Device* device, VkCommandPool commandPool, const std::vector<InstanceData> &Data, int numBarkVertices, int numLeafVertices, int numBillboardsVertices);
	virtual ~InstanceBuffer();
	VkBuffer GetInstanceDataBuffer() const;
	VkBuffer GetCulledInstanceDataBuffer(int LOD_num) const;
	VkBuffer GetNumInstanceDataBuffer(int LOD_num) const;
	int GetInstanceCount() const;
	
	VkDeviceMemory GetInstanceDataMemory() const;
	VkDeviceMemory GetCulledInstanceDataMemory(int LOD_num) const;
	VkDeviceMemory GetNumInstanceDataMemory(int LOD_num) const;

};
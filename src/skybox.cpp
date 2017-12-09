#include "skybox.h"
#include "BufferUtils.h"
#include "Image.h"
#include <iostream>

Skybox::Skybox(Device* device, VkCommandPool commandPool, const std::vector<Position> &verts, const std::vector<uint32_t> &indices)
	: Model(device, commandPool, {}, indices), skybox_vertices(verts) {

	if (verts.size() > 0)
	{
		BufferUtils::CreateBufferFromData(device, commandPool, this->skybox_vertices.data(), verts.size() * sizeof(Position), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMemory);
	}
}

const std::vector<Position>& Skybox::getSkyboxVertices() const
{
	return this->skybox_vertices;
}


Skybox::~Skybox() {

	if (skybox_vertices.size() > 0)
	{
		vkDestroyBuffer(device->GetVkDevice(), vertexBuffer, nullptr);
		vkFreeMemory(device->GetVkDevice(), vertexBufferMemory, nullptr);
	}
	
}






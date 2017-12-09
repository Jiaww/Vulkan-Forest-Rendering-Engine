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

void Skybox::SetDiffuseMap(VkImage texture)
{
	this->diffuseMap = texture;
	this->diffuseMapView = Image::CreateView(device, texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT,true);

	// --- Specify all filters and transformations ---
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	// Interpolation of texels that are magnified or minified
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	// Addressing mode
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	// Anisotropic filtering
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;

	// Border color
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	// Choose coordinate system for addressing texels --> [0, 1) here
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	// Comparison function used for filtering operations
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	// Mipmapping
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device->GetVkDevice(), &samplerInfo, nullptr, &diffuseMapSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler");
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






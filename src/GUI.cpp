#include "GUI.h"
#include "BufferUtils.h"
#include "Image.h"
GUI::GUI(Device * device):device(device)
{
}


GUI::~GUI()
{
	for (int i = 0; i < IMGUI_VK_QUEUED_FRAMES; i++) {
		if (g_VertexBuffer[i])
			vkDestroyBuffer(device->GetVkDevice(), g_VertexBuffer[i], nullptr);
		if (g_VertexBufferMemory[i])
			vkFreeMemory(device->GetVkDevice(), g_VertexBufferMemory[i], nullptr);
		if (g_IndexBuffer[i])
			vkDestroyBuffer(device->GetVkDevice(), g_IndexBuffer[i], nullptr);
		if (g_IndexBufferMemory[i])
			vkFreeMemory(device->GetVkDevice(), g_IndexBufferMemory[i], nullptr);
	}

	if (FontTextureMapView != VK_NULL_HANDLE) {
		vkDestroyImageView(device->GetVkDevice(), FontTextureMapView, nullptr);
	}

	if (FontTextureMapSampler != VK_NULL_HANDLE) {
		vkDestroySampler(device->GetVkDevice(), FontTextureMapSampler, nullptr);
	}
}

void GUI::SetFontTextureMap(VkImage texture)
{
	this->FontTextureMap = texture;
	this->FontTextureMapView = Image::CreateView(device, texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, false);

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

	if (vkCreateSampler(device->GetVkDevice(), &samplerInfo, nullptr, &FontTextureMapSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler");
	}
}

VkBuffer GUI::getVertexBuffer() const
{
	return g_VertexBuffer[g_FrameIndex];
}

VkBuffer GUI::getIndexBuffer() const
{
	return g_IndexBuffer[g_FrameIndex];
}

VkImageView GUI::GetFontTextureMapView() const
{
	return FontTextureMapView;
}

VkSampler GUI::GetFontTextureMapSampler() const
{
	return FontTextureMapSampler;
}




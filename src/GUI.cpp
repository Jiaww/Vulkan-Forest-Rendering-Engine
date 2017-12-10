#include "Instance.h"
#include "GUI.h"

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




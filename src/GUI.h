#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "imgui.h"
#include "Device.h"

#define IMGUI_VK_QUEUED_FRAMES 2





class GUI {
public:
	Device* device;

	VkBuffer               g_VertexBuffer[IMGUI_VK_QUEUED_FRAMES] = {};
	VkBuffer               g_IndexBuffer[IMGUI_VK_QUEUED_FRAMES] = {};
	VkDeviceMemory         g_VertexBufferMemory[IMGUI_VK_QUEUED_FRAMES] = {};
	VkDeviceMemory         g_IndexBufferMemory[IMGUI_VK_QUEUED_FRAMES] = {};
	size_t                 g_VertexBufferSize[IMGUI_VK_QUEUED_FRAMES] = {};
	size_t                 g_IndexBufferSize[IMGUI_VK_QUEUED_FRAMES] = {};
	size_t                 g_BufferMemoryAlignment = 256;
	// diffusse map
	VkImage FontTextureMap = VK_NULL_HANDLE;
	VkImageView FontTextureMapView = VK_NULL_HANDLE;
	VkSampler FontTextureMapSampler = VK_NULL_HANDLE;


	GUI() = delete;
	GUI(Device* device);
	virtual ~GUI();

	void SetFontTextureMap(VkImage texture);
	VkBuffer getVertexBuffer() const;
	VkBuffer getIndexBuffer() const;
	VkImageView GetFontTextureMapView() const;
	VkSampler GetFontTextureMapSampler() const;
	int g_FrameIndex = 0;
};
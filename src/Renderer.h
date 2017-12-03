#pragma once

#include "Device.h"
#include "SwapChain.h"
#include "Scene.h"
#include "Camera.h"

class Renderer {
public:
    Renderer() = delete;
    Renderer(Device* device, SwapChain* swapChain, Scene* scene, Camera* camera);
    ~Renderer();

    void CreateCommandPools();

    void CreateRenderPass();

// Funcs: Descriptor Set Layout
    void CreateCameraDescriptorSetLayout();
    void CreateModelDescriptorSetLayout();
    void CreateGrassDescriptorSetLayout();
	void CreateTimeDescriptorSetLayout();
	void CreateComputeDescriptorSetLayout();
	void CreateCullingComputeDescriptorSetLayout();
	void CreateTerrainDescriptorSetLayout();

    void CreateDescriptorPool();

// Funcs: Descriptor Set
    void CreateCameraDescriptorSet();
    void CreateModelDescriptorSets();
    void CreateGrassDescriptorSets();
    void CreateTimeDescriptorSet();
    void CreateComputeDescriptorSets();
	void CreateCullingComputeDescriptorSets();
	void CreateTerrainDescriptorSet();

// Funcs: Pipeline(Correspond to how many different shaders)
    void CreateGraphicsPipeline();
    void CreateGrassPipeline();
    void CreateComputePipeline();
	void CreateCullingComputePipeline();
	void CreateBarkPipeline();
	void CreateLeafPipeline();
	void CreateBillboardPipeline();
	void CreateTerrainPipeline();

    void CreateFrameResources();
    void DestroyFrameResources();
    void RecreateFrameResources();

    void RecordCommandBuffers();
    void RecordComputeCommandBuffer();

    void Frame();

private:
    Device* device;
    VkDevice logicalDevice;
    SwapChain* swapChain;
    Scene* scene;
    Camera* camera;

    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;

    VkRenderPass renderPass;
// Vars: Descriptor Set Layout
	VkDescriptorSetLayout cameraDescriptorSetLayout;
	VkDescriptorSetLayout modelDescriptorSetLayout;
	VkDescriptorSetLayout grassDescriptorSetLayout;
	VkDescriptorSetLayout timeDescriptorSetLayout;
	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkDescriptorSetLayout cullingComputeDescriptorSetLayout;
	VkDescriptorSetLayout terrainDescriptorSetLayout;

	VkDescriptorPool descriptorPool;

// Vars: Descriptor Set
	VkDescriptorSet cameraDescriptorSet;
	std::vector<VkDescriptorSet> modelDescriptorSets;
	VkDescriptorSet timeDescriptorSet;
	std::vector<VkDescriptorSet> grassDescriptorSets;
	std::vector<VkDescriptorSet> computeDescriptorSets;
	std::vector<VkDescriptorSet> cullingComputeDescriptorSets;
	VkDescriptorSet terrainDescriptorSet;

// Vars: Pipeline Layout and pipeline
	VkPipelineLayout graphicsPipelineLayout;
	VkPipelineLayout barkPipelineLayout;
	VkPipelineLayout leafPipelineLayout;
	VkPipelineLayout billboardPipelineLayout;
	VkPipelineLayout grassPipelineLayout;
	VkPipelineLayout computePipelineLayout;
	VkPipelineLayout cullingComputePipelineLayout;
	VkPipelineLayout terrainPipelineLayout;

	VkPipeline graphicsPipeline;
	VkPipeline barkPipeline;
	VkPipeline leafPipeline;
	VkPipeline billboardPipeline;
	VkPipeline grassPipeline;
	VkPipeline computePipeline;
	VkPipeline cullingComputePipeline;
	VkPipeline terrainPipeline;

    std::vector<VkImageView> imageViews;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    std::vector<VkFramebuffer> framebuffers;

    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandBuffer computeCommandBuffer;
};

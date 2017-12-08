#include "Renderer.h"
#include "Instance.h"
#include "ShaderModule.h"
#include "Vertex.h"
#include "Blades.h"
#include "Camera.h"
#include "Image.h"

static constexpr unsigned int WORKGROUP_SIZE = 32;

#define LOD_FRUSTUM_CULLING 1

Renderer::Renderer(Device* device, SwapChain* swapChain, Scene* scene, Camera* camera)
	: device(device),
	logicalDevice(device->GetVkDevice()),
	swapChain(swapChain),
	scene(scene),
	camera(camera) {

	CreateCommandPools();
	CreateRenderPass();
// Funcs: Descriptor Set Layout
	CreateCameraDescriptorSetLayout();
	CreateModelDescriptorSetLayout();
	CreateGrassDescriptorSetLayout();
	CreateTimeDescriptorSetLayout();
	CreateComputeDescriptorSetLayout();
	CreateCullingComputeDescriptorSetLayout();
	CreateFakeCullingComputeDescriptorSetLayout();
	CreateTerrainDescriptorSetLayout();
	CreateLODInfoDescriptorSetLayout();

	CreateDescriptorPool();

// Funcs: Descriptor Set
	CreateCameraDescriptorSet();
	CreateModelDescriptorSets();
	CreateGrassDescriptorSets();
	CreateTimeDescriptorSet();
	CreateComputeDescriptorSets();
	CreateCullingComputeDescriptorSets();
	CreateFakeCullingComputeDescriptorSets();
	CreateTerrainDescriptorSet();
	CreateLODInfoDescriptorSets();

	CreateFrameResources();
	
// Funcs: Pipeline(Correspond to how many different shaders)
	CreateGraphicsPipeline();
	CreateBarkPipeline();
	CreateLeafPipeline();
	CreateBillboardPipeline();
	CreateGrassPipeline();
	CreateComputePipeline();
	CreateCullingComputePipeline();
	CreateFakeCullingComputePipeline();
	CreateTerrainPipeline();

	RecordCommandBuffers();
	RecordComputeCommandBuffer();
}

void Renderer::CreateCommandPools() {
	VkCommandPoolCreateInfo graphicsPoolInfo = {};
	graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicsPoolInfo.queueFamilyIndex = device->GetInstance()->GetQueueFamilyIndices()[QueueFlags::Graphics];
	graphicsPoolInfo.flags = 0;

	if (vkCreateCommandPool(logicalDevice, &graphicsPoolInfo, nullptr, &graphicsCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	}

	VkCommandPoolCreateInfo computePoolInfo = {};
	computePoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	computePoolInfo.queueFamilyIndex = device->GetInstance()->GetQueueFamilyIndices()[QueueFlags::Compute];
	computePoolInfo.flags = 0;

	if (vkCreateCommandPool(logicalDevice, &computePoolInfo, nullptr, &computeCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	}
}

void Renderer::CreateRenderPass() {
	// Color buffer attachment represented by one of the images from the swap chain
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChain->GetVkImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Create a color attachment reference to be used with subpass
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth buffer attachment
	VkFormat depthFormat = device->GetInstance()->GetSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Create a depth attachment reference
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Create subpass description
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	// Specify subpass dependency
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Create render pass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}
}

void Renderer::CreateCameraDescriptorSetLayout() {
	// Describe the binding of the descriptor set layout
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &cameraDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateModelDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding diffuseSamplerLayoutBinding = {};
	diffuseSamplerLayoutBinding.binding = 1;
	diffuseSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	diffuseSamplerLayoutBinding.descriptorCount = 1;
	diffuseSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	diffuseSamplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding normalSamplerLayoutBinding = {};
	normalSamplerLayoutBinding.binding = 2;
	normalSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalSamplerLayoutBinding.descriptorCount = 1;
	normalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	normalSamplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding noiseSamplerLayoutBinding = {};
	noiseSamplerLayoutBinding.binding = 3;
	noiseSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	noiseSamplerLayoutBinding.descriptorCount = 1;
	noiseSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	noiseSamplerLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding, diffuseSamplerLayoutBinding, normalSamplerLayoutBinding, noiseSamplerLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &modelDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateTimeDescriptorSetLayout() {
	// Describe the binding of the descriptor set layout
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
	;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &timeDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateGrassDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &grassDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateComputeDescriptorSetLayout() {
	// TODO: Create the descriptor set layout for the compute pipeline
	// Remember this is like a class definition stating why types of information
	// will be stored at each binding
	VkDescriptorSetLayoutBinding bladesBinding = {};
	bladesBinding.binding = 0;
	bladesBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bladesBinding.descriptorCount = 1;
	bladesBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bladesBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding culledBladesBinding = {};
	culledBladesBinding.binding = 1;
	culledBladesBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	culledBladesBinding.descriptorCount = 1;
	culledBladesBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	culledBladesBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding numBladesBinding = {};
	numBladesBinding.binding = 2;
	numBladesBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	numBladesBinding.descriptorCount = 1;
	numBladesBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	numBladesBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { bladesBinding, culledBladesBinding, numBladesBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateCullingComputeDescriptorSetLayout() {
	// TODO: Create the descriptor set layout for the compute pipeline
	// Remember this is like a class definition stating why types of information
	// will be stored at each binding
	VkDescriptorSetLayoutBinding instanceBinding = {};
	instanceBinding.binding = 0;
	instanceBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	instanceBinding.descriptorCount = 1;
	instanceBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	instanceBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding culledInstanceBinding[2] = {};
	culledInstanceBinding[0].binding = 1;
	culledInstanceBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	culledInstanceBinding[0].descriptorCount = 1;
	culledInstanceBinding[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	culledInstanceBinding[0].pImmutableSamplers = nullptr;

	culledInstanceBinding[1].binding = 2;
	culledInstanceBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	culledInstanceBinding[1].descriptorCount = 1;
	culledInstanceBinding[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	culledInstanceBinding[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding numInstanceBinding[3] = {};
	numInstanceBinding[0].binding = 3;
	numInstanceBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	numInstanceBinding[0].descriptorCount = 1;
	numInstanceBinding[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	numInstanceBinding[0].pImmutableSamplers = nullptr;

	numInstanceBinding[1].binding = 4;
	numInstanceBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	numInstanceBinding[1].descriptorCount = 1;
	numInstanceBinding[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	numInstanceBinding[1].pImmutableSamplers = nullptr;

	numInstanceBinding[2].binding = 5;
	numInstanceBinding[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	numInstanceBinding[2].descriptorCount = 1;
	numInstanceBinding[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	numInstanceBinding[2].pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { instanceBinding, culledInstanceBinding[0], culledInstanceBinding[1], numInstanceBinding[0], numInstanceBinding[1], numInstanceBinding[2] };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &cullingComputeDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateFakeCullingComputeDescriptorSetLayout() {
	// TODO: Create the descriptor set layout for the compute pipeline
	// Remember this is like a class definition stating why types of information
	// will be stored at each binding
	VkDescriptorSetLayoutBinding instanceBinding = {};
	instanceBinding.binding = 0;
	instanceBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	instanceBinding.descriptorCount = 1;
	instanceBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	instanceBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding culledInstanceBinding = {};
	culledInstanceBinding.binding = 1;
	culledInstanceBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	culledInstanceBinding.descriptorCount = 1;
	culledInstanceBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	culledInstanceBinding.pImmutableSamplers = nullptr;


	VkDescriptorSetLayoutBinding numInstanceBinding = {};
	numInstanceBinding.binding = 2;
	numInstanceBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	numInstanceBinding.descriptorCount = 1;
	numInstanceBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	numInstanceBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { instanceBinding, culledInstanceBinding, numInstanceBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &fakeCullingComputeDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateTerrainDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding diffuseSamplerLayoutBinding = {};
	diffuseSamplerLayoutBinding.binding = 1;
	diffuseSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	diffuseSamplerLayoutBinding.descriptorCount = 1;
	diffuseSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	diffuseSamplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding normalSamplerLayoutBinding = {};
	normalSamplerLayoutBinding.binding = 2;
	normalSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalSamplerLayoutBinding.descriptorCount = 1;
	normalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	normalSamplerLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding, diffuseSamplerLayoutBinding, normalSamplerLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &terrainDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateLODInfoDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding LODInfoLayoutBinding = {};
	LODInfoLayoutBinding.binding = 0;
	LODInfoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LODInfoLayoutBinding.descriptorCount = 1;
	LODInfoLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
	LODInfoLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { LODInfoLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &LODInfoDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateDescriptorPool() {
	// Describe which descriptor types that the descriptor sets will contain
	std::vector<VkDescriptorPoolSize> poolSizes = {
		// Camera
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 },

		// Models + blades (diffuse, normal, noise)
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 3 * (static_cast<uint32_t>(scene->GetModels().size() + scene->GetBlades().size())) },

		// Models + Blades
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , static_cast<uint32_t>(scene->GetModels().size() + scene->GetBlades().size() + scene->GetLODInfoBuffer().size()) },

		// Time (compute)
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 },

		// Compute
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , 3 * (uint32_t)scene->GetBlades().size() },

		// Culling Compute
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , 6 * (uint32_t)scene->GetInstanceBuffer().size() },

		// Fake Culling Compute
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , 3 * (uint32_t)scene->GetFakeInstanceBuffer().size() },

		// Terrain
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 2 },
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 22;//1*camera + 7*model + 2*model(faketrees) + 1*grass + 1*time + 1*compute + 1*terrain + 2*cullingCompute + 2*fakeCullingCompute + 4 * LODInfo

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void Renderer::CreateCameraDescriptorSet() {
	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { cameraDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &cameraDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	// Configure the descriptors to refer to buffers
	VkDescriptorBufferInfo cameraBufferInfo = {};
	cameraBufferInfo.buffer = camera->GetBuffer();
	cameraBufferInfo.offset = 0;
	cameraBufferInfo.range = sizeof(CameraBufferObject);

	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = cameraDescriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &cameraBufferInfo;
	descriptorWrites[0].pImageInfo = nullptr;
	descriptorWrites[0].pTexelBufferView = nullptr;

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateModelDescriptorSets() {
	modelDescriptorSets.resize(scene->GetModels().size());

	// Describe the desciptor set
	std::vector<VkDescriptorSetLayout> layouts;
	for (int i = 0; i < scene->GetModels().size(); ++i) {
		layouts.push_back(modelDescriptorSetLayout);
	}
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(modelDescriptorSets.size());
	allocInfo.pSetLayouts = layouts.data();

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, modelDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}


	for (uint32_t i = 0; i < scene->GetModels().size(); ++i) {
		std::vector<VkWriteDescriptorSet> descriptorWrites(4);

		VkDescriptorBufferInfo modelBufferInfo = {};
		modelBufferInfo.buffer = scene->GetModels()[i]->GetModelBuffer();
		modelBufferInfo.offset = 0;
		modelBufferInfo.range = sizeof(ModelBufferObject);

		// Bind image and sampler resources to the descriptor
		VkDescriptorImageInfo diffuseMapInfo = {};
		diffuseMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		diffuseMapInfo.imageView = scene->GetModels()[i]->GetDiffuseMapView();
		diffuseMapInfo.sampler = scene->GetModels()[i]->GetDiffuseMapSampler();
		VkDescriptorImageInfo normalMapInfo = {};
		normalMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalMapInfo.imageView = scene->GetModels()[i]->GetNormalMapView();
		normalMapInfo.sampler = scene->GetModels()[i]->GetNormalMapSampler();
		VkDescriptorImageInfo noiseMapInfo = {};
		noiseMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		noiseMapInfo.imageView = scene->GetModels()[i]->GetNoiseMapView();
		noiseMapInfo.sampler = scene->GetModels()[i]->GetNoiseMapSampler();

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = modelDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &modelBufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = modelDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &diffuseMapInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = modelDescriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &normalMapInfo;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = modelDescriptorSets[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = &noiseMapInfo;

		// Update descriptor sets
		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::CreateGrassDescriptorSets() {
	// TODO: Create Descriptor sets for the grass.
	// This should involve creating descriptor sets which point to the model matrix of each group of grass blades
	grassDescriptorSets.resize(scene->GetBlades().size());

	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { grassDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(grassDescriptorSets.size());
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, grassDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}


	for (uint32_t i = 0; i < scene->GetBlades().size(); ++i) {
		std::vector<VkWriteDescriptorSet> descriptorWrites(1);
		VkDescriptorBufferInfo modelBufferInfo = {};
		modelBufferInfo.buffer = scene->GetBlades()[i]->GetModelBuffer();
		modelBufferInfo.offset = 0;
		modelBufferInfo.range = sizeof(ModelBufferObject);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = grassDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &modelBufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;
		// Update descriptor sets
		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::CreateTimeDescriptorSet() {
	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { timeDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &timeDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	// Configure the descriptors to refer to buffers
	VkDescriptorBufferInfo timeBufferInfo = {};
	timeBufferInfo.buffer = scene->GetTimeBuffer();
	timeBufferInfo.offset = 0;
	timeBufferInfo.range = sizeof(Time);

	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = timeDescriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &timeBufferInfo;
	descriptorWrites[0].pImageInfo = nullptr;
	descriptorWrites[0].pTexelBufferView = nullptr;

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateComputeDescriptorSets() {
	// TODO: Create Descriptor sets for the compute pipeline
	// The descriptors should point to Storage buffers which will hold the grass blades, the culled grass blades, and the output number of grass blades 
	computeDescriptorSets.resize(scene->GetBlades().size());

	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { computeDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(computeDescriptorSets.size());
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, computeDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}


	for (uint32_t i = 0; i < scene->GetBlades().size(); ++i) {
		std::vector<VkWriteDescriptorSet> descriptorWrites(3);
		//Blades
		VkDescriptorBufferInfo bladesBufferInfo = {};
		bladesBufferInfo.buffer = scene->GetBlades()[i]->GetBladesBuffer();
		bladesBufferInfo.offset = 0;
		bladesBufferInfo.range = NUM_BLADES * sizeof(Blade);

		//CulledBlades
		VkDescriptorBufferInfo culledBaldesBufferInfo = {};
		culledBaldesBufferInfo.buffer = scene->GetBlades()[i]->GetCulledBladesBuffer();
		culledBaldesBufferInfo.offset = 0;
		culledBaldesBufferInfo.range = NUM_BLADES * sizeof(Blade);

		//Num of Blades
		VkDescriptorBufferInfo numBaldesBufferInfo = {};
		numBaldesBufferInfo.buffer = scene->GetBlades()[i]->GetNumBladesBuffer();
		numBaldesBufferInfo.offset = 0;
		numBaldesBufferInfo.range = sizeof(BladeDrawIndirect);


		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = computeDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bladesBufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = computeDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &culledBaldesBufferInfo;
		descriptorWrites[1].pImageInfo = nullptr;
		descriptorWrites[1].pTexelBufferView = nullptr;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = computeDescriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &numBaldesBufferInfo;
		descriptorWrites[2].pImageInfo = nullptr;
		descriptorWrites[2].pTexelBufferView = nullptr;
		// Update descriptor sets
		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::CreateCullingComputeDescriptorSets() {
	// TODO: Create Descriptor sets for the compute pipeline
	// The descriptors should point to Storage buffers which will hold the grass blades, the culled grass blades, and the output number of grass blades 
	cullingComputeDescriptorSets.resize(scene->GetInstanceBuffer().size());

	// Describe the desciptor set
	std::vector<VkDescriptorSetLayout> layouts;
	for (int i = 0; i < scene->GetInstanceBuffer().size(); i++) {
		layouts.push_back(cullingComputeDescriptorSetLayout);
	}
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(cullingComputeDescriptorSets.size());
	allocInfo.pSetLayouts = layouts.data();

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, cullingComputeDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}


	for (uint32_t i = 0; i < scene->GetInstanceBuffer().size(); ++i) {
		std::vector<VkWriteDescriptorSet> descriptorWrites(6);
		//InstanceBuffer
		VkDescriptorBufferInfo instanceDataBufferInfo = {};
		instanceDataBufferInfo.buffer = scene->GetInstanceBuffer()[i]->GetInstanceDataBuffer();
		instanceDataBufferInfo.offset = 0;
		instanceDataBufferInfo.range = scene->GetInstanceBuffer()[i]->GetInstanceCount() * sizeof(InstanceData);

		//CulledInstance Buffer LOD 0
		VkDescriptorBufferInfo culledDataBufferInfo[2] = {};
		culledDataBufferInfo[0].buffer = scene->GetInstanceBuffer()[i]->GetCulledInstanceDataBuffer(0);
		culledDataBufferInfo[0].offset = 0;
		culledDataBufferInfo[0].range = scene->GetInstanceBuffer()[i]->GetInstanceCount() * sizeof(InstanceData);

		//CulledInstance Buffer LOD 1
		culledDataBufferInfo[1].buffer = scene->GetInstanceBuffer()[i]->GetCulledInstanceDataBuffer(1);
		culledDataBufferInfo[1].offset = 0;
		culledDataBufferInfo[1].range = scene->GetInstanceBuffer()[i]->GetInstanceCount() * sizeof(InstanceData);

		//Num of culled Instance Buffer
		VkDescriptorBufferInfo numDataBufferInfo[3] = {};
		// LOD 0 Leaf
		numDataBufferInfo[0].buffer = scene->GetInstanceBuffer()[i]->GetNumInstanceDataBuffer(0);
		numDataBufferInfo[0].offset = 0;
		numDataBufferInfo[0].range = sizeof(VkDrawIndexedIndirectCommand);
		// LOD 0 Bark
		numDataBufferInfo[1].buffer = scene->GetInstanceBuffer()[i]->GetNumInstanceDataBuffer(1);
		numDataBufferInfo[1].offset = 0;
		numDataBufferInfo[1].range = sizeof(VkDrawIndexedIndirectCommand);
		// LOD 1 Billboard
		numDataBufferInfo[2].buffer = scene->GetInstanceBuffer()[i]->GetNumInstanceDataBuffer(2);
		numDataBufferInfo[2].offset = 0;
		numDataBufferInfo[2].range = sizeof(VkDrawIndexedIndirectCommand);


		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = cullingComputeDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &instanceDataBufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = cullingComputeDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &culledDataBufferInfo[0];
		descriptorWrites[1].pImageInfo = nullptr;
		descriptorWrites[1].pTexelBufferView = nullptr;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = cullingComputeDescriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &culledDataBufferInfo[1];
		descriptorWrites[2].pImageInfo = nullptr;
		descriptorWrites[2].pTexelBufferView = nullptr;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = cullingComputeDescriptorSets[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pBufferInfo = &numDataBufferInfo[0];
		descriptorWrites[3].pImageInfo = nullptr;
		descriptorWrites[3].pTexelBufferView = nullptr;

		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = cullingComputeDescriptorSets[i];
		descriptorWrites[4].dstBinding = 4;
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pBufferInfo = &numDataBufferInfo[1];
		descriptorWrites[4].pImageInfo = nullptr;
		descriptorWrites[4].pTexelBufferView = nullptr;

		descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[5].dstSet = cullingComputeDescriptorSets[i];
		descriptorWrites[5].dstBinding = 5;
		descriptorWrites[5].dstArrayElement = 0;
		descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[5].descriptorCount = 1;
		descriptorWrites[5].pBufferInfo = &numDataBufferInfo[2];
		descriptorWrites[5].pImageInfo = nullptr;
		descriptorWrites[5].pTexelBufferView = nullptr;
		// Update descriptor sets
		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::CreateFakeCullingComputeDescriptorSets() {
	// TODO: Create Descriptor sets for the compute pipeline
	// The descriptors should point to Storage buffers which will hold the grass blades, the culled grass blades, and the output number of grass blades 
	fakeCullingComputeDescriptorSets.resize(scene->GetFakeInstanceBuffer().size());

	// Describe the desciptor set
	std::vector<VkDescriptorSetLayout> layouts;
	for (int i = 0; i < scene->GetFakeInstanceBuffer().size(); i++) {
		layouts.push_back(fakeCullingComputeDescriptorSetLayout);
	}
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(fakeCullingComputeDescriptorSets.size());
	allocInfo.pSetLayouts = layouts.data();

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, fakeCullingComputeDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}


	for (uint32_t i = 0; i < scene->GetFakeInstanceBuffer().size(); ++i) {
		std::vector<VkWriteDescriptorSet> descriptorWrites(3);
		//InstanceBuffer
		VkDescriptorBufferInfo instanceDataBufferInfo = {};
		instanceDataBufferInfo.buffer = scene->GetFakeInstanceBuffer()[i]->GetInstanceDataBuffer();
		instanceDataBufferInfo.offset = 0;
		instanceDataBufferInfo.range = scene->GetFakeInstanceBuffer()[i]->GetInstanceCount() * sizeof(InstanceData);

		//CulledInstance Buffer
		VkDescriptorBufferInfo culledDataBufferInfo = {};
		culledDataBufferInfo.buffer = scene->GetFakeInstanceBuffer()[i]->GetCulledInstanceDataBuffer();
		culledDataBufferInfo.offset = 0;
		culledDataBufferInfo.range = scene->GetFakeInstanceBuffer()[i]->GetInstanceCount() * sizeof(InstanceData);

		//Num of culled Instance Buffer
		VkDescriptorBufferInfo numDataBufferInfo = {};
		// LOD 0 Leaf
		numDataBufferInfo.buffer = scene->GetFakeInstanceBuffer()[i]->GetNumInstanceDataBuffer();
		numDataBufferInfo.offset = 0;
		numDataBufferInfo.range = sizeof(VkDrawIndexedIndirectCommand);
		
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = fakeCullingComputeDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &instanceDataBufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = fakeCullingComputeDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &culledDataBufferInfo;
		descriptorWrites[1].pImageInfo = nullptr;
		descriptorWrites[1].pTexelBufferView = nullptr;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = fakeCullingComputeDescriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &numDataBufferInfo;
		descriptorWrites[2].pImageInfo = nullptr;
		descriptorWrites[2].pTexelBufferView = nullptr;

		// Update descriptor sets
		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::CreateTerrainDescriptorSet() {
	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { terrainDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(1);
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor set
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &terrainDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	std::vector<VkWriteDescriptorSet> descriptorWrites(3);

	VkDescriptorBufferInfo modelBufferInfo = {};
	modelBufferInfo.buffer = scene->GetTerrain()->GetModelBuffer();
	modelBufferInfo.offset = 0;
	modelBufferInfo.range = sizeof(ModelBufferObject);

	// Bind image and sampler resources to the descriptor
	VkDescriptorImageInfo diffuseMapInfo = {};
	diffuseMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	diffuseMapInfo.imageView = scene->GetTerrain()->GetDiffuseMapView();
	diffuseMapInfo.sampler = scene->GetTerrain()->GetDiffuseMapSampler();
	VkDescriptorImageInfo normalMapInfo = {};
	normalMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalMapInfo.imageView = scene->GetTerrain()->GetNormalMapView();
	normalMapInfo.sampler = scene->GetTerrain()->GetNormalMapSampler();

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = terrainDescriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &modelBufferInfo;
	descriptorWrites[0].pImageInfo = nullptr;
	descriptorWrites[0].pTexelBufferView = nullptr;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = terrainDescriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &diffuseMapInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = terrainDescriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pImageInfo = &normalMapInfo;

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateLODInfoDescriptorSets() {
	LODInfoDescriptorSets.resize(scene->GetLODInfoBuffer().size());
	
	// Describe the desciptor set
	std::vector<VkDescriptorSetLayout> layouts;
	for (int i = 0; i < scene->GetLODInfoBuffer().size(); ++i) {
		layouts.push_back(LODInfoDescriptorSetLayout);
	}
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(LODInfoDescriptorSets.size());
	allocInfo.pSetLayouts = layouts.data();

	// Allocate descriptor set
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, LODInfoDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	for (uint32_t i = 0; i < scene->GetLODInfoBuffer().size(); ++i) {
		std::vector<VkWriteDescriptorSet> descriptorWrites(1);

		VkDescriptorBufferInfo LODBufferInfo = {};
		LODBufferInfo.buffer = scene->GetLODInfoBuffer()[i];
		LODBufferInfo.offset = 0;
		LODBufferInfo.range = sizeof(glm::vec4);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = LODInfoDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &LODBufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		// Update descriptor sets
		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::CreateGraphicsPipeline() {
	VkShaderModule vertShaderModule = ShaderModule::Create("shaders/graphics.vert.spv", logicalDevice);
	VkShaderModule fragShaderModule = ShaderModule::Create("shaders/graphics.frag.spv", logicalDevice);

	// Assign each shader module to the appropriate stage in the pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// --- Set up fixed-function stages ---

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	bindingDescriptions = { Vertex::getBindingDescription() };
	attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewports and Scissors (rectangles that define in which regions pixels are stored)
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain->GetVkExtent().width);
	viewport.height = static_cast<float>(swapChain->GetVkExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain->GetVkExtent();

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling (turned off here)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Depth testing
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color blending (turned off here, but showing options for learning)
	// --> Configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// --> Global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, modelDescriptorSetLayout };

	// Pipeline layout: used to specify uniform values
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// --- Create graphics pipeline ---
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = graphicsPipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Renderer::CreateBarkPipeline() {
	VkShaderModule vertShaderModule = ShaderModule::Create("shaders/bark.vert.spv", logicalDevice);
	VkShaderModule fragShaderModule = ShaderModule::Create("shaders/bark.frag.spv", logicalDevice);

	// Assign each shader module to the appropriate stage in the pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// --- Set up fixed-function stages ---

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	std::vector<VkVertexInputAttributeDescription> instanceDescriptions;

	bindingDescriptions = { Vertex::getBindingDescription(),InstanceData::getBindingDescription()};
	attributeDescriptions = Vertex::getAttributeDescriptions();
	instanceDescriptions = InstanceData::getAttributeDescriptions();
	for (int i = 0; i < instanceDescriptions.size(); i++)
		attributeDescriptions.push_back(instanceDescriptions[i]);


	vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewports and Scissors (rectangles that define in which regions pixels are stored)
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain->GetVkExtent().width);
	viewport.height = static_cast<float>(swapChain->GetVkExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain->GetVkExtent();

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling (turned off here)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Depth testing
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color blending (turned off here, but showing options for learning)
	// --> Configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// --> Global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, modelDescriptorSetLayout, timeDescriptorSetLayout , LODInfoDescriptorSetLayout };

	// Pipeline layout: used to specify uniform values
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &barkPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// --- Create graphics pipeline ---
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = barkPipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &barkPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Renderer::CreateLeafPipeline() {
	VkShaderModule vertShaderModule = ShaderModule::Create("shaders/leaf.vert.spv", logicalDevice);
	VkShaderModule fragShaderModule = ShaderModule::Create("shaders/leaf.frag.spv", logicalDevice);

	// Assign each shader module to the appropriate stage in the pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// --- Set up fixed-function stages ---

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	std::vector<VkVertexInputAttributeDescription> instanceDescriptions;

	bindingDescriptions = { Vertex::getBindingDescription() };
	attributeDescriptions = Vertex::getAttributeDescriptions();
	instanceDescriptions = InstanceData::getAttributeDescriptions();
	for (int i = 0; i < instanceDescriptions.size(); i++)
		attributeDescriptions.push_back(instanceDescriptions[i]);

	vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewports and Scissors (rectangles that define in which regions pixels are stored)
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain->GetVkExtent().width);
	viewport.height = static_cast<float>(swapChain->GetVkExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain->GetVkExtent();

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	//rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling (turned off here)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Depth testing
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color blending (turned off here, but showing options for learning)
	// --> Configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// --> Global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, modelDescriptorSetLayout, timeDescriptorSetLayout , LODInfoDescriptorSetLayout };

	// Pipeline layout: used to specify uniform values
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &leafPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// --- Create graphics pipeline ---
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = leafPipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &leafPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Renderer::CreateBillboardPipeline() {
	VkShaderModule vertShaderModule = ShaderModule::Create("shaders/billboard.vert.spv", logicalDevice);
	VkShaderModule fragShaderModule = ShaderModule::Create("shaders/billboard.frag.spv", logicalDevice);

	// Assign each shader module to the appropriate stage in the pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// --- Set up fixed-function stages ---

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	std::vector<VkVertexInputAttributeDescription> instanceDescriptions;

	bindingDescriptions = { Vertex::getBindingDescription() };
	attributeDescriptions = Vertex::getAttributeDescriptions();
	instanceDescriptions = InstanceData::getAttributeDescriptions();
	for (int i = 0; i < instanceDescriptions.size(); i++)
		attributeDescriptions.push_back(instanceDescriptions[i]);

	vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewports and Scissors (rectangles that define in which regions pixels are stored)
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain->GetVkExtent().width);
	viewport.height = static_cast<float>(swapChain->GetVkExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain->GetVkExtent();

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling (turned off here)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Depth testing
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color blending (turned off here, but showing options for learning)
	// --> Configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// --> Global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, modelDescriptorSetLayout, timeDescriptorSetLayout , LODInfoDescriptorSetLayout };

	// Pipeline layout: used to specify uniform values
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &billboardPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// --- Create graphics pipeline ---
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = billboardPipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &billboardPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Renderer::CreateGrassPipeline() {
	// --- Set up programmable shaders ---
	VkShaderModule vertShaderModule = ShaderModule::Create("shaders/grass.vert.spv", logicalDevice);
	VkShaderModule tescShaderModule = ShaderModule::Create("shaders/grass.tesc.spv", logicalDevice);
	VkShaderModule teseShaderModule = ShaderModule::Create("shaders/grass.tese.spv", logicalDevice);
	VkShaderModule fragShaderModule = ShaderModule::Create("shaders/grass.frag.spv", logicalDevice);

	// Assign each shader module to the appropriate stage in the pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo tescShaderStageInfo = {};
	tescShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	tescShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	tescShaderStageInfo.module = tescShaderModule;
	tescShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo teseShaderStageInfo = {};
	teseShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	teseShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	teseShaderStageInfo.module = teseShaderModule;
	teseShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, tescShaderStageInfo, teseShaderStageInfo, fragShaderStageInfo };

	// --- Set up fixed-function stages ---

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;


	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	bindingDescriptions = { Blade::getBindingDescription() };
	attributeDescriptions = Blade::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewports and Scissors (rectangles that define in which regions pixels are stored)
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain->GetVkExtent().width);
	viewport.height = static_cast<float>(swapChain->GetVkExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain->GetVkExtent();

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling (turned off here)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Depth testing
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color blending (turned off here, but showing options for learning)
	// --> Configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// --> Global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, grassDescriptorSetLayout };

	// Pipeline layout: used to specify uniform values
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &grassPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// Tessellation state
	VkPipelineTessellationStateCreateInfo tessellationInfo = {};
	tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellationInfo.pNext = NULL;
	tessellationInfo.flags = 0;
	tessellationInfo.patchControlPoints = 1;

	// --- Create graphics pipeline ---
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 4;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pTessellationState = &tessellationInfo;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = grassPipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &grassPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	// No need for the shader modules anymore
	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, tescShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, teseShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Renderer::CreateComputePipeline() {
	// Set up programmable shaders
	VkShaderModule computeShaderModule = ShaderModule::Create("shaders/compute.comp.spv", logicalDevice);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = computeShaderModule;
	computeShaderStageInfo.pName = "main";

	// TODO: Add the compute dsecriptor set layout you create to this list
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, timeDescriptorSetLayout, computeDescriptorSetLayout };

	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// Create compute pipeline
	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = computeShaderStageInfo;
	pipelineInfo.layout = computePipelineLayout;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create compute pipeline");
	}

	// No need for shader modules anymore
	vkDestroyShaderModule(logicalDevice, computeShaderModule, nullptr);
}

void Renderer::CreateCullingComputePipeline() {
	// Set up programmable shaders
	VkShaderModule computeShaderModule = ShaderModule::Create("shaders/cullingCompute.comp.spv", logicalDevice);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = computeShaderModule;
	computeShaderStageInfo.pName = "main";

	// TODO: Add the compute dsecriptor set layout you create to this list
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, timeDescriptorSetLayout, cullingComputeDescriptorSetLayout, LODInfoDescriptorSetLayout };

	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &cullingComputePipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// Create compute pipeline
	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = computeShaderStageInfo;
	pipelineInfo.layout = cullingComputePipelineLayout;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &cullingComputePipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create compute pipeline");
	}

	// No need for shader modules anymore
	vkDestroyShaderModule(logicalDevice, computeShaderModule, nullptr);
}

void Renderer::CreateFakeCullingComputePipeline() {
	// Set up programmable shaders
	VkShaderModule computeShaderModule = ShaderModule::Create("shaders/fakeCullingCompute.comp.spv", logicalDevice);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = computeShaderModule;
	computeShaderStageInfo.pName = "main";

	// TODO: Add the compute dsecriptor set layout you create to this list
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, timeDescriptorSetLayout, fakeCullingComputeDescriptorSetLayout, LODInfoDescriptorSetLayout };

	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &fakeCullingComputePipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// Create compute pipeline
	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = computeShaderStageInfo;
	pipelineInfo.layout = fakeCullingComputePipelineLayout;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &fakeCullingComputePipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create compute pipeline");
	}

	// No need for shader modules anymore
	vkDestroyShaderModule(logicalDevice, computeShaderModule, nullptr);
}

void Renderer::CreateTerrainPipeline() {
	VkShaderModule vertShaderModule = ShaderModule::Create("shaders/terrain.vert.spv", logicalDevice);
	VkShaderModule fragShaderModule = ShaderModule::Create("shaders/terrain.frag.spv", logicalDevice);

	// Assign each shader module to the appropriate stage in the pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// --- Set up fixed-function stages ---

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewports and Scissors (rectangles that define in which regions pixels are stored)
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain->GetVkExtent().width);
	viewport.height = static_cast<float>(swapChain->GetVkExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain->GetVkExtent();

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling (turned off here)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Depth testing
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color blending (turned off here, but showing options for learning)
	// --> Configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// --> Global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, terrainDescriptorSetLayout };

	// Pipeline layout: used to specify uniform values
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &terrainPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// --- Create graphics pipeline ---
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = terrainPipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &terrainPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Renderer::CreateFrameResources() {
	imageViews.resize(swapChain->GetCount());

	for (uint32_t i = 0; i < swapChain->GetCount(); i++) {
		// --- Create an image view for each swap chain image ---
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChain->GetVkImage(i);

		// Specify how the image data should be interpreted
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChain->GetVkImageFormat();

		// Specify color channel mappings (can be used for swizzling)
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// Describe the image's purpose and which part of the image should be accessed
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		// Create the image view
		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views");
		}
	}

	VkFormat depthFormat = device->GetInstance()->GetSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	// CREATE DEPTH IMAGE
	Image::Create(device,
		swapChain->GetVkExtent().width,
		swapChain->GetVkExtent().height,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depthImage,
		depthImageMemory
	);

	depthImageView = Image::CreateView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	// Transition the image for use as depth-stencil
	Image::TransitionLayout(device, graphicsCommandPool, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


	// CREATE FRAMEBUFFERS
	framebuffers.resize(swapChain->GetCount());
	for (size_t i = 0; i < swapChain->GetCount(); i++) {
		std::vector<VkImageView> attachments = {
			imageViews[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChain->GetVkExtent().width;
		framebufferInfo.height = swapChain->GetVkExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer");
		}

	}
}

void Renderer::DestroyFrameResources() {
	for (size_t i = 0; i < imageViews.size(); i++) {
		vkDestroyImageView(logicalDevice, imageViews[i], nullptr);
	}

	vkDestroyImageView(logicalDevice, depthImageView, nullptr);
	vkFreeMemory(logicalDevice, depthImageMemory, nullptr);
	vkDestroyImage(logicalDevice, depthImage, nullptr);

	for (size_t i = 0; i < framebuffers.size(); i++) {
		vkDestroyFramebuffer(logicalDevice, framebuffers[i], nullptr);
	}
}

void Renderer::RecreateFrameResources() {
	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, grassPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, barkPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, leafPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, billboardPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, terrainPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, computePipeline, nullptr);
	vkDestroyPipeline(logicalDevice, cullingComputePipeline, nullptr);
	vkDestroyPipeline(logicalDevice, fakeCullingComputePipeline, nullptr);

	vkDestroyPipelineLayout(logicalDevice, graphicsPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, grassPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, barkPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, leafPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, billboardPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, terrainPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, computePipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, cullingComputePipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, fakeCullingComputePipelineLayout, nullptr);

	vkFreeCommandBuffers(logicalDevice, graphicsCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	DestroyFrameResources();
	CreateFrameResources();
	CreateGraphicsPipeline();
	CreateBarkPipeline();
	CreateLeafPipeline();
	CreateGrassPipeline();
	CreateComputePipeline();
	CreateCullingComputePipeline();
	CreateFakeCullingComputePipeline();
	CreateBillboardPipeline();
	CreateTerrainPipeline();
	RecordCommandBuffers();
}

void Renderer::RecordComputeCommandBuffer() {
	// Specify the command pool and number of buffers to allocate
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = computeCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &computeCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers");
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	// ~ Start recording ~
	if (vkBeginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording compute command buffer");
	}

#if LOD_FRUSTUM_CULLING

	std::vector<VkBufferMemoryBarrier> barriers(scene->GetInstanceBuffer().size() * 3 + scene->GetFakeInstanceBuffer().size());
	for (uint32_t j = 0; j < scene->GetInstanceBuffer().size(); ++j) {
		barriers[3 * j].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[3 * j].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[3 * j].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		barriers[3 * j].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
		barriers[3 * j].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
		barriers[3 * j].buffer = scene->GetInstanceBuffer()[j]->GetNumInstanceDataBuffer(0);
		barriers[3 * j].offset = 0;
		barriers[3 * j].size = sizeof(VkDrawIndexedIndirectCommand);

		barriers[3 * j + 1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[3 * j + 1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[3 * j + 1].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		barriers[3 * j + 1].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
		barriers[3 * j + 1].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
		barriers[3 * j + 1].buffer = scene->GetInstanceBuffer()[j]->GetNumInstanceDataBuffer(1);
		barriers[3 * j + 1].offset = 0;
		barriers[3 * j + 1].size = sizeof(VkDrawIndexedIndirectCommand);

		barriers[3 * j + 2].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[3 * j + 2].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[3 * j + 2].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		barriers[3 * j + 2].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
		barriers[3 * j + 2].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
		barriers[3 * j + 2].buffer = scene->GetInstanceBuffer()[j]->GetNumInstanceDataBuffer(2);
		barriers[3 * j + 2].offset = 0;
		barriers[3 * j + 2].size = sizeof(VkDrawIndexedIndirectCommand);
	}

// Fake Trees Barriers
	for (uint32_t j = 0; j < scene->GetFakeInstanceBuffer().size(); ++j) {
		barriers[scene->GetInstanceBuffer().size() * 3 + j].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[scene->GetInstanceBuffer().size() * 3 + j].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[scene->GetInstanceBuffer().size() * 3 + j].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		barriers[scene->GetInstanceBuffer().size() * 3 + j].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
		barriers[scene->GetInstanceBuffer().size() * 3 + j].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
		barriers[scene->GetInstanceBuffer().size() * 3 + j].buffer = scene->GetFakeInstanceBuffer()[j]->GetNumInstanceDataBuffer();
		barriers[scene->GetInstanceBuffer().size() * 3 + j].offset = 0;
		barriers[scene->GetInstanceBuffer().size() * 3 + j].size = sizeof(VkDrawIndexedIndirectCommand);
	}

	vkCmdPipelineBarrier(computeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, barriers.size(), barriers.data(), 0, nullptr);

//Real Trees LOD
	// Bind to the compute pipeline
	vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cullingComputePipeline);

	// Bind camera descriptor set
	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cullingComputePipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);

	// Bind descriptor set for time uniforms
	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cullingComputePipelineLayout, 1, 1, &timeDescriptorSet, 0, nullptr);

	for (int i = 0; i < scene->GetInstanceBuffer().size(); ++i) {
		vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cullingComputePipelineLayout, 2, 1, &cullingComputeDescriptorSets[i], 0, nullptr);
		vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cullingComputePipelineLayout, 3, 1, &LODInfoDescriptorSets[i], 0, nullptr);
		vkCmdDispatch(computeCommandBuffer, (int)(scene->GetInstanceBuffer()[i]->GetInstanceCount() / WORKGROUP_SIZE + 1), 1, 1);
	}
	// TODO: For each group of blades bind its descriptor set and dispatch
	/*for (int i = 0; i < scene->GetBlades().size(); ++i) {
		vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 2, 1, &computeDescriptorSets[i], 0, nullptr);
		vkCmdDispatch(computeCommandBuffer, (int)(NUM_BLADES / WORKGROUP_SIZE + 1), 1, 1);
	}*/

// Fake Trees
	// Bind to the compute pipeline
	vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, fakeCullingComputePipeline);

	// Bind camera descriptor set
	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, fakeCullingComputePipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);

	// Bind descriptor set for time uniforms
	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, fakeCullingComputePipelineLayout, 1, 1, &timeDescriptorSet, 0, nullptr);

	for (int i = 0; i < scene->GetFakeInstanceBuffer().size(); ++i) {
		vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, fakeCullingComputePipelineLayout, 2, 1, &fakeCullingComputeDescriptorSets[i], 0, nullptr);
		vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, fakeCullingComputePipelineLayout, 3, 1, &LODInfoDescriptorSets[scene->GetInstanceBuffer().size() + i], 0, nullptr);
		vkCmdDispatch(computeCommandBuffer, (int)(scene->GetFakeInstanceBuffer()[i]->GetInstanceCount() / WORKGROUP_SIZE + 1), 1, 1);
	}
	for (uint32_t j = 0; j < scene->GetInstanceBuffer().size(); ++j) {
		barriers[3 * j].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[3 * j].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[3 * j].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		barriers[3 * j].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
		barriers[3 * j].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
		barriers[3 * j].buffer = scene->GetInstanceBuffer()[j]->GetNumInstanceDataBuffer(0);
		barriers[3 * j].offset = 0;
		barriers[3 * j].size = sizeof(VkDrawIndexedIndirectCommand);

		barriers[3 * j + 1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[3 * j + 1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[3 * j + 1].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		barriers[3 * j + 1].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
		barriers[3 * j + 1].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
		barriers[3 * j + 1].buffer = scene->GetInstanceBuffer()[j]->GetNumInstanceDataBuffer(1);
		barriers[3 * j + 1].offset = 0;
		barriers[3 * j + 1].size = sizeof(VkDrawIndexedIndirectCommand);

		barriers[3 * j + 2].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[3 * j + 2].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[3 * j + 2].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		barriers[3 * j + 2].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
		barriers[3 * j + 2].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
		barriers[3 * j + 2].buffer = scene->GetInstanceBuffer()[j]->GetNumInstanceDataBuffer(2);
		barriers[3 * j + 2].offset = 0;
		barriers[3 * j + 2].size = sizeof(VkDrawIndexedIndirectCommand);
	}

	for (uint32_t j = 0; j < scene->GetFakeInstanceBuffer().size(); ++j) {
		barriers[scene->GetInstanceBuffer().size() * 3 + j].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[scene->GetInstanceBuffer().size() * 3 + j].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barriers[scene->GetInstanceBuffer().size() * 3 + j].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		barriers[scene->GetInstanceBuffer().size() * 3 + j].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
		barriers[scene->GetInstanceBuffer().size() * 3 + j].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
		barriers[scene->GetInstanceBuffer().size() * 3 + j].buffer = scene->GetFakeInstanceBuffer()[j]->GetNumInstanceDataBuffer();
		barriers[scene->GetInstanceBuffer().size() * 3 + j].offset = 0;
		barriers[scene->GetInstanceBuffer().size() * 3 + j].size = sizeof(VkDrawIndexedIndirectCommand);
	}

	vkCmdPipelineBarrier(computeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, barriers.size(), barriers.data(), 0, nullptr);
#endif

	////Fake Tree Compute Shader
	//std::vector<VkBufferMemoryBarrier> fakeBarriers(scene->GetFakeInstanceBuffer().size());
	//for (uint32_t j = 0; j < scene->GetFakeInstanceBuffer().size(); ++j) {
	//	fakeBarriers[j].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	//	fakeBarriers[j].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	//	fakeBarriers[j].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
	//	fakeBarriers[j].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
	//	fakeBarriers[j].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
	//	fakeBarriers[j].buffer = scene->GetFakeInstanceBuffer()[j]->GetNumInstanceDataBuffer();
	//	fakeBarriers[j].offset = 0;
	//	fakeBarriers[j].size = sizeof(VkDrawIndexedIndirectCommand);
	//}

	//vkCmdPipelineBarrier(computeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, fakeBarriers.size(), fakeBarriers.data(), 0, nullptr);

	//// Bind to the compute pipeline
	//vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, fakeCullingComputePipeline);

	//// Bind camera descriptor set
	//vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, fakeCullingComputePipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);

	//// Bind descriptor set for time uniforms
	//vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, fakeCullingComputePipelineLayout, 1, 1, &timeDescriptorSet, 0, nullptr);

	//for (int i = 0; i < scene->GetFakeInstanceBuffer().size(); ++i) {
	//	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, fakeCullingComputePipelineLayout, 2, 1, &fakeCullingComputeDescriptorSets[i], 0, nullptr);
	//	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, fakeCullingComputePipelineLayout, 3, 1, &LODInfoDescriptorSets[scene->GetInstanceBuffer().size() + i], 0, nullptr);
	//	vkCmdDispatch(computeCommandBuffer, (int)(scene->GetFakeInstanceBuffer()[i]->GetInstanceCount() / WORKGROUP_SIZE + 1), 1, 1);
	//}

	//for (uint32_t j = 0; j < scene->GetFakeInstanceBuffer().size(); ++j) {
	//	fakeBarriers[j].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	//	fakeBarriers[j].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	//	fakeBarriers[j].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
	//	fakeBarriers[j].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
	//	fakeBarriers[j].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
	//	fakeBarriers[j].buffer = scene->GetFakeInstanceBuffer()[j]->GetNumInstanceDataBuffer();
	//	fakeBarriers[j].offset = 0;
	//	fakeBarriers[j].size = sizeof(VkDrawIndexedIndirectCommand);
	//}

	//vkCmdPipelineBarrier(computeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, fakeBarriers.size(), fakeBarriers.data(), 0, nullptr);
	// ~ End recording ~
	if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record compute command buffer");
	}
}

void Renderer::RecordCommandBuffers() {
	commandBuffers.resize(swapChain->GetCount());

	// Specify the command pool and number of buffers to allocate
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = graphicsCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers");
	}

	// Start command buffer recording
	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		// ~ Start recording ~
		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer");
		}

		// Begin the render pass
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChain->GetVkExtent();

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		//Terrain: terrain
		{
			// Bind the terrain pipeline
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, terrainPipeline);

			// Bind the vertex and index buffers
			VkBuffer vertexBuffers[] = { scene->GetTerrain()->getVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffers[i], scene->GetTerrain()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			// Bind the camera descriptor set. This is set 0 in all pipelines so it will be inherited
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, terrainPipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
			// Bind the descriptor set for terrain
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, terrainPipelineLayout, 1, 1, &terrainDescriptorSet, 0, nullptr);

			// Draw
			std::vector<uint32_t> indices = scene->GetTerrain()->getIndices();
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		}

		//Planes: graphics
		{
			// Bind the graphics pipeline
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			// Bind the vertex and index buffers
			VkBuffer vertexBuffers[] = { scene->GetModels()[0]->getVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffers[i], scene->GetModels()[0]->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			// Bind the camera descriptor set. This is set 0 in all pipelines so it will be inherited
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
			// Bind the descriptor set for each model
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 1, 1, &modelDescriptorSets[0], 0, nullptr);

			// Draw
			std::vector<uint32_t> indices = scene->GetModels()[0]->getIndices();
			//vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		}

		// Iterate through different types of tree
		for (int k = 0; k < scene->GetInstanceBuffer().size(); k++) {

			//Bark: bark pipeline
			{
				// Bind the bark pipeline
				vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, barkPipeline);
				// Bind the vertex and index buffers
				VkBuffer vertexBuffers[] = { scene->GetModels()[3 * k + 1]->getVertexBuffer() };
				
#if LOD_FRUSTUM_CULLING
				VkBuffer instanceBuffer[] = { scene->GetInstanceBuffer()[k]->GetCulledInstanceDataBuffer(0) };
#else
				VkBuffer instanceBuffer[] = { scene->GetInstanceBuffer()[k]->GetInstanceDataBuffer() };
#endif
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
				// Bind Instance Buffer
				vkCmdBindVertexBuffers(commandBuffers[i], 1, 1, instanceBuffer, offsets);

				vkCmdBindIndexBuffer(commandBuffers[i], scene->GetModels()[3 * k +1]->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				// Bind the camera descriptor set. This is set 0 in all pipelines so it will be inherited
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, barkPipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
				// Bind the descriptor set for each model
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, barkPipelineLayout, 1, 1, &modelDescriptorSets[3 * k +1], 0, nullptr);
				// Bind the time descriptor.
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, barkPipelineLayout, 2, 1, &timeDescriptorSet, 0, nullptr);
				// Bind the LOD descriptor
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, barkPipelineLayout, 3, 1, &LODInfoDescriptorSets[k], 0, nullptr);
#if LOD_FRUSTUM_CULLING
				// Indirect Draw
				vkCmdDrawIndexedIndirect(commandBuffers[i], scene->GetInstanceBuffer()[k]->GetNumInstanceDataBuffer(0), 0, 1, 0);
#else
				// Draw
				std::vector<uint32_t> indices = scene->GetModels()[3*k + 1]->getIndices();
				vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), scene->GetInstanceBuffer()[k]->GetInstanceCount(), 0, 0, 0);
#endif
			}

			//Leaf: leaf pipeline
			{
				// Bind the leaf pipeline
				vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, leafPipeline);

				// Bind the vertex and index buffers
				VkBuffer vertexBuffers[] = { scene->GetModels()[3 * k +2]->getVertexBuffer() };
#if LOD_FRUSTUM_CULLING
				VkBuffer instanceBuffer[] = { scene->GetInstanceBuffer()[k]->GetCulledInstanceDataBuffer(0) };
#else
				VkBuffer instanceBuffer[] = { scene->GetInstanceBuffer()[k]->GetInstanceDataBuffer() };
#endif
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
				vkCmdBindVertexBuffers(commandBuffers[i], 1, 1, instanceBuffer, offsets);
				vkCmdBindIndexBuffer(commandBuffers[i], scene->GetModels()[3 * k +2]->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				// Bind the camera descriptor set. This is set 0 in all pipelines so it will be inherited
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, leafPipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
				// Bind the descriptor set for each model
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, leafPipelineLayout, 1, 1, &modelDescriptorSets[3 * k +2], 0, nullptr);
				// Bind the time descriptor.
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, leafPipelineLayout, 2, 1, &timeDescriptorSet, 0, nullptr);
				// Bind the LOD descriptor
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, leafPipelineLayout, 3, 1, &LODInfoDescriptorSets[k], 0, nullptr);
#if LOD_FRUSTUM_CULLING
				// Indirect Draw
				vkCmdDrawIndexedIndirect(commandBuffers[i], scene->GetInstanceBuffer()[k]->GetNumInstanceDataBuffer(1), 0, 1, 0);
#else
				// Draw
				std::vector<uint32_t> indices = scene->GetModels()[3 *k + 2]->getIndices();
				vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), scene->GetInstanceBuffer()[k]->GetInstanceCount(), 0, 0, 0);
#endif
			}

			//Billboard: billboard pipeline
			{
				// Bind the leaf pipeline
				vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, billboardPipeline);

				// Bind the vertex and index buffers
				VkBuffer vertexBuffers[] = { scene->GetModels()[3 * k +3]->getVertexBuffer() };
#if LOD_FRUSTUM_CULLING
				VkBuffer instanceBuffer[] = { scene->GetInstanceBuffer()[k]->GetCulledInstanceDataBuffer(1) };
#else
				VkBuffer instanceBuffer[] = { scene->GetInstanceBuffer()[k]->GetInstanceDataBuffer() };
#endif
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
				vkCmdBindVertexBuffers(commandBuffers[i], 1, 1, instanceBuffer, offsets);
				vkCmdBindIndexBuffer(commandBuffers[i], scene->GetModels()[3 * k +3]->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				// Bind the camera descriptor set. This is set 0 in all pipelines so it will be inherited
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, billboardPipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
				// Bind the descriptor set for each model
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, billboardPipelineLayout, 1, 1, &modelDescriptorSets[3 * k +3], 0, nullptr);
				// Bind the time descriptor.
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, billboardPipelineLayout, 2, 1, &timeDescriptorSet, 0, nullptr);
				// Bind the LOD descriptor
				vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, billboardPipelineLayout, 3, 1, &LODInfoDescriptorSets[k], 0, nullptr);
#if LOD_FRUSTUM_CULLING
				// Indirect Draw
				vkCmdDrawIndexedIndirect(commandBuffers[i], scene->GetInstanceBuffer()[k]->GetNumInstanceDataBuffer(2), 0, 1, 0);
#else
				// Draw
				std::vector<uint32_t> indices = scene->GetModels()[3*k + 3]->getIndices();
				vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), scene->GetInstanceBuffer()[k]->GetInstanceCount(), 0, 0, 0);
#endif
			}
		}

		// Fake Tree
		for (int k = 0; k < scene->GetFakeInstanceBuffer().size(); k++) {
			// Bind the leaf pipeline
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, billboardPipeline);

			// Bind the vertex and index buffers
			VkBuffer vertexBuffers[] = { scene->GetModels()[7 + k]->getVertexBuffer() };
#if LOD_FRUSTUM_CULLING
			VkBuffer instanceBuffer[] = { scene->GetFakeInstanceBuffer()[k]->GetInstanceDataBuffer() };
#else
			VkBuffer instanceBuffer[] = { scene->GetFakeInstanceBuffer()[k]->GetInstanceDataBuffer() };
#endif
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindVertexBuffers(commandBuffers[i], 1, 1, instanceBuffer, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], scene->GetModels()[7 + k]->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			// Bind the camera descriptor set. This is set 0 in all pipelines so it will be inherited
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, billboardPipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
			// Bind the descriptor set for each model
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, billboardPipelineLayout, 1, 1, &modelDescriptorSets[7 + k], 0, nullptr);
			// Bind the time descriptor.
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, billboardPipelineLayout, 2, 1, &timeDescriptorSet, 0, nullptr);
			// Bind the LOD descriptor
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, billboardPipelineLayout, 3, 1, &LODInfoDescriptorSets[2 + k], 0, nullptr);
#if LOD_FRUSTUM_CULLING
			// Indirect Draw
			vkCmdDrawIndexedIndirect(commandBuffers[i], scene->GetFakeInstanceBuffer()[k]->GetNumInstanceDataBuffer(), 0, 1, 0);
#else
			// Draw
			std::vector<uint32_t> indices = scene->GetModels()[3 * k + 3]->getIndices();
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), scene->GetInstanceBuffer()[k]->GetInstanceCount(), 0, 0, 0);
#endif
		}
		//// Grass
		//vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, grassPipeline);

		//for (uint32_t j = 0; j < scene->GetBlades().size(); ++j) {
		//	VkBuffer vertexBuffers[] = { scene->GetBlades()[j]->GetCulledBladesBuffer() };
		//	//VkBuffer vertexBuffers[] = { scene->GetBlades()[j]->GetBladesBuffer() };

		//	VkDeviceSize offsets[] = { 0 };
		//	// TODO: Uncomment this when the buffers are populated
		//	vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

		//	// Bind the camera descriptor set. This is set 0 in all pipelines so it will be inherited
		//	vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, grassPipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);
		//	// TODO: Bind the descriptor set for each grass blades model
		//	vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, grassPipelineLayout, 1, 1, &grassDescriptorSets[j], 0, nullptr);

		//	// Draw
		//	// TODO: Uncomment this when the buffers are populated
		//	//vkCmdDrawIndirect(commandBuffers[i], scene->GetBlades()[j]->GetNumBladesBuffer(), 0, 1, sizeof(BladeDrawIndirect));
		//}

		// End render pass
		vkCmdEndRenderPass(commandBuffers[i]);

		// ~ End recording ~
		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer");
		}
	}
}

void Renderer::Frame() {

	VkSubmitInfo computeSubmitInfo = {};
	computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	computeSubmitInfo.commandBufferCount = 1;
	computeSubmitInfo.pCommandBuffers = &computeCommandBuffer;

	if (vkQueueSubmit(device->GetQueue(QueueFlags::Compute), 1, &computeSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer");
	}

	if (!swapChain->Acquire()) {
		RecreateFrameResources();
		return;
	}

	// Submit the command buffer
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { swapChain->GetImageAvailableVkSemaphore() };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[swapChain->GetIndex()];

	VkSemaphore signalSemaphores[] = { swapChain->GetRenderFinishedVkSemaphore() };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(device->GetQueue(QueueFlags::Graphics), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer");
	}

	if (!swapChain->Present()) {
		RecreateFrameResources();
	}
}

Renderer::~Renderer() {
	vkDeviceWaitIdle(logicalDevice);

	// TODO: destroy any resources you created

	vkFreeCommandBuffers(logicalDevice, graphicsCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	vkFreeCommandBuffers(logicalDevice, computeCommandPool, 1, &computeCommandBuffer);

	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, barkPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, leafPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, grassPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, billboardPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, terrainPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, computePipeline, nullptr);
	vkDestroyPipeline(logicalDevice, cullingComputePipeline, nullptr);
	vkDestroyPipeline(logicalDevice, fakeCullingComputePipeline, nullptr);


	vkDestroyPipelineLayout(logicalDevice, graphicsPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, barkPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, leafPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, grassPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, billboardPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, terrainPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, computePipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, cullingComputePipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, fakeCullingComputePipelineLayout, nullptr);

	vkDestroyDescriptorSetLayout(logicalDevice, cameraDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, modelDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, timeDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, grassDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, terrainDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, computeDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, cullingComputeDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, fakeCullingComputeDescriptorSetLayout, nullptr);


	vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
	DestroyFrameResources();
	vkDestroyCommandPool(logicalDevice, computeCommandPool, nullptr);
	vkDestroyCommandPool(logicalDevice, graphicsCommandPool, nullptr);
}

#include <vulkan/vulkan.h>
#include <ctime>
#include "Instance.h"
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "Image.h"
#include "FbxLoader.h"
#include "Terrain.h"

Device* device;
SwapChain* swapChain;
Renderer* renderer;
Camera* camera;

namespace {
	void resizeCallback(GLFWwindow* window, int width, int height) {
		if (width == 0 || height == 0) return;

		vkDeviceWaitIdle(device->GetVkDevice());
		swapChain->Recreate(width, height);
		camera->UpdateAspectRatio(float(width) / height,width,height);
		renderer->RecreateFrameResources();
	}

	bool leftMouseDown = false;
	bool rightMouseDown = false;
	double previousX = 0.0;
	double previousY = 0.0;

	void mouseDownCallback(GLFWwindow* window, int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS) {
				leftMouseDown = true;
				glfwGetCursorPos(window, &previousX, &previousY);
			}
			else if (action == GLFW_RELEASE) {
				leftMouseDown = false;
			}
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (action == GLFW_PRESS) {
				rightMouseDown = true;
				glfwGetCursorPos(window, &previousX, &previousY);
			}
			else if (action == GLFW_RELEASE) {
				rightMouseDown = false;
			}
		}
	}

	void mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition) {
		double sensitivity = 1;
		float deltaX = static_cast<float>((previousX - xPosition) * sensitivity);
		float deltaY = static_cast<float>((previousY - yPosition) * sensitivity);
		if (leftMouseDown) {

			camera->CameraRotate(deltaX, deltaY);
			//camera->UpdateOrbit(deltaX, deltaY, 0.0f);
			//previousX = xPosition;
			//previousY = yPosition;
		}
		else if (rightMouseDown) {

			
			/*double deltaZ = static_cast<float>((previousY - yPosition) * 0.05);
			
			camera->UpdateOrbit(0.0f, 0.0f, deltaZ);
			previousX = xPosition;
			previousY = yPosition;*/
			camera->CameraTranslate(deltaX, deltaY);

		}
		previousX = xPosition;
		previousY = yPosition;
	}
	void mouseWheelCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		camera->CameraScale(yoffset);
	}
}

int main() {
	static constexpr char* applicationName = "Vulkan Grass Rendering";
	int width = 1280, height = 960;
	InitializeWindow(width, height, applicationName);

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	Instance* instance = new Instance(applicationName, glfwExtensionCount, glfwExtensions);

	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(instance->GetVkInstance(), GetGLFWWindow(), nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}

	instance->PickPhysicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, surface);

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.tessellationShader = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	device = instance->CreateDevice(QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, deviceFeatures);

	swapChain = device->CreateSwapChain(surface, 5);

	camera = new Camera(device, float(width) /float(height),width,height);

	VkCommandPoolCreateInfo transferPoolInfo = {};
	transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolInfo.queueFamilyIndex = device->GetInstance()->GetQueueFamilyIndices()[QueueFlags::Transfer];
	transferPoolInfo.flags = 0;

	VkCommandPool transferCommandPool;
	if (vkCreateCommandPool(device->GetVkDevice(), &transferPoolInfo, nullptr, &transferCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	}

// Texture Loading
	// Terrain
	VkImage terrainImage;
	VkDeviceMemory terrainImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/terrain/diffuseMap03.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		terrainImage,
		terrainImageMemory
	);
	VkImage terrainNormalImage;
	VkDeviceMemory terrainNormalImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/terrain/normalMap03.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		terrainNormalImage,
		terrainNormalImageMemory
	);
	// G
	VkImage grassImage;
	VkDeviceMemory grassImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"images/grass.jpg",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		grassImage,
		grassImageMemory
	);
	// Bark
	VkImage barkImage;
	VkDeviceMemory barkImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Bark_png/BroadleafBark_Tex_Tree0.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		barkImage,
		barkImageMemory
	);
	VkImage barkNormalImage;
	VkDeviceMemory barkNormalImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Bark_png/BroadleafBark_Normal_Tex_Tree0.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		barkNormalImage,
		barkNormalImageMemory
	);
	// Leaf
	VkImage leafImage;
	VkDeviceMemory leafImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Leaf_png/leaf_Tex_Tree0.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		leafImage,
		leafImageMemory
	);
	VkImage leafNormalImage;
	VkDeviceMemory leafNormalImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Leaf_png/Normal_Tex_Tree0.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		leafNormalImage,
		leafNormalImageMemory
	);
	// Leaf
	VkImage billboardImage;
	VkDeviceMemory billboardImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Billboard_png/Billboards_Tex_Tree0.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		billboardImage,
		billboardImageMemory
	);
	VkImage billboardNormalImage;
	VkDeviceMemory billboardNormalImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Billboard_png/Billboards_Normal_Tex_Tree0.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		billboardNormalImage,
		billboardNormalImageMemory
	);

	//Noise
	VkImage noiseImage;
	VkDeviceMemory noiseImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/noise.jpg",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		noiseImage,
		noiseImageMemory
	);

// Terrain Initializations
	char* terrainPath = "../../media/terrain/heightMap03.png";
	char* rawPath = "../../media/terrain/heightMap03.r16";
	Terrain *terrain = Terrain::LoadTerrain(device, transferCommandPool, terrainPath, rawPath, 256.0f);
	terrain->SetDiffuseMap(terrainImage);
	terrain->SetNormalMap(terrainNormalImage);
// Model Initializations
	// Plane
	float planeDim = 50.f;
	float halfWidth = planeDim * 0.5f;
	Model* plane = new Model(device, transferCommandPool,
	{
		{ { -halfWidth, 0.0f, halfWidth },{ 1.0f, 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f } },
		{ { halfWidth, 0.0f, halfWidth },{ 0.0f, 1.0f, 0.0f, 1.0f },{ 0.0f, 0.0f } },
		{ { halfWidth, 0.0f, -halfWidth },{ 0.0f, 0.0f, 1.0f, 1.0f },{ 0.0f, 1.0f } },
		{ { -halfWidth, 0.0f, -halfWidth },{ 1.0f, 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f } }
	},
	{ 0, 1, 2, 2, 3, 0 }
	);
	plane->SetDiffuseMap(grassImage);
	plane->SetNormalMap(grassImage);
	plane->SetNoiseMap(grassImage);

	FbxLoader *fbxloader;
	// Bark
	fbxloader = new FbxLoader("../../media/models/tree1_bark_rgba.FBX");
	Model* bark = new Model(device, transferCommandPool,
		fbxloader->vertices,
		fbxloader->indices
	);
	bark->SetDiffuseMap(barkImage);
	bark->SetNormalMap(barkNormalImage);
	bark->SetNoiseMap(noiseImage);
	// Leaf
	fbxloader = new FbxLoader("../../media/models/tree1_leaf_rgba.FBX");
	Model* leaf = new Model(device, transferCommandPool,
		fbxloader->vertices,
		fbxloader->indices
	);
	leaf->SetDiffuseMap(leafImage);
	leaf->SetNormalMap(leafNormalImage);
	leaf->SetNoiseMap(noiseImage);
	// Billboard
	float billWidth = 24.0f;
	float billheigth = 20.0f;
	Model* billboard = new Model(device, transferCommandPool,
	{
		{ { -billWidth / 2.0, billheigth, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f },{ 0.333f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { -billWidth / 2.0, 0.0f, 0.0f },	{ 0.0f, 1.0f, 0.0f, 1.0f },{ 0.333f, 0.333f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { billWidth / 2.0, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f },{ 0.650f, 0.333f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { billWidth / 2.0, billheigth, 0.0f },{ 1.0f, 1.0f, 1.0f, 1.0f },{ 0.650f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } }
	},
	{ 1, 2, 0, 0, 2, 3 }
	);
	billboard->SetDiffuseMap(billboardImage);
	billboard->SetNormalMap(billboardNormalImage);
	billboard->SetNoiseMap(noiseImage);

	// Insert Trees
	//Instance Data
	printf("Starting Insert Trees Randomly\n");
	std::vector<InstanceData> instanceData;
	srand((unsigned int)time(0));
	int numTrees = 200;
	int randRange = terrain->GetTerrainDim()-2;
	for (int i = 0; i < numTrees; i++) {
		float posX = (rand() % (randRange * 5)) / 5.0f;
		float posZ = (rand() % (randRange * 5)) / 5.0f;
		float posY = terrain->GetHeight(posX, posZ);
		glm::vec3 position(posX, posY, posZ);
		float scale = 0.9f + float(rand() % 200) / 1000.0f;
		float r = (210 + float(rand() % 45)) / 255.0f;
		float g = (210 + float(rand() % 45)) / 255.0f;
		float b = (210 + float(rand() % 45)) / 255.0f;
		float theta = float(rand() % 3145) / 1000.0f;
		printf("|| Tree No.%d: <position: %f %f %f> <scale: %f> <theta: %f> <tintColor: %f %f %f>||\n", i, posX, posY, posZ, scale, theta, r, g, b);
		instanceData.push_back(InstanceData(position, scale, theta, glm::vec3(r, g, b)));
	}
	InstanceBuffer* instanceBuffer = new InstanceBuffer(device, transferCommandPool, instanceData);
	printf("Finish Insert Trees Randomly\n");

	// Blades
	printf("Building Blades\n");
	Blades* blades = new Blades(device, transferCommandPool, planeDim, terrain);
	printf("Finish Building Blades\n");

	vkDestroyCommandPool(device->GetVkDevice(), transferCommandPool, nullptr);

// Scene Initialization
	Scene* scene = new Scene(device);
	scene->SetTerrain(terrain);
	scene->AddModel(plane);
	scene->AddModel(bark);
	scene->AddModel(leaf);
	scene->AddModel(billboard);
	scene->AddBlades(blades);
	scene->AddInstanceBuffer(instanceBuffer);

	

	//float yy = scene->GetTerrain()->GetHeight(0.25,0.25);
	//scene->InsertRandomTrees(20, device, transferCommandPool);

	renderer = new Renderer(device, swapChain, scene, camera);

	glfwSetWindowSizeCallback(GetGLFWWindow(), resizeCallback);
	glfwSetMouseButtonCallback(GetGLFWWindow(), mouseDownCallback);
	glfwSetCursorPosCallback(GetGLFWWindow(), mouseMoveCallback);
	glfwSetScrollCallback(GetGLFWWindow(), mouseWheelCallback);

	int time_start = GetTickCount();
	int count = 0;

	while (!ShouldQuit()) {
		glfwPollEvents();
		scene->UpdateTime();
		renderer->Frame();
		count++;
		if (count == 1000) {
			int total_time = GetTickCount() - time_start;
			printf("Total Time for 500 frames: %d\n", total_time);
			printf("Time per frame: %f\n", float(total_time) / 1000.0);
			printf("fps: %f\n", 1000000.0 / float(total_time));
		}
	}

	vkDeviceWaitIdle(device->GetVkDevice());

	vkDestroyImage(device->GetVkDevice(), grassImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), grassImageMemory, nullptr);

	delete scene;
	delete plane;
	delete bark;
	delete leaf;
	delete billboard;
	//delete instanceBuffer;
	delete blades;
	delete camera;
	delete renderer;
	delete swapChain;
	delete device;
	delete instance;
	DestroyWindow();
	return 0;
}

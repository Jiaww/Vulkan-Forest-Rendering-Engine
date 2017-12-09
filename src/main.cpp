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
#include "skybox.h"

Device* device;
SwapChain* swapChain;
Renderer* renderer;
Camera* camera;

static float LOD0 = 0.6;
static float LOD1 = 0.43;

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
	int width = 1920, height = 1080;
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
	VkImage leafImage2;
	VkDeviceMemory leafImageMemory2;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Leaf_png/leaf_Tex_Tree2.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		leafImage2,
		leafImageMemory2
	);
	VkImage leafNormalImage2;
	VkDeviceMemory leafNormalImageMemory2;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Leaf_png/Normal_Tex_Tree2.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		leafNormalImage2,
		leafNormalImageMemory2
	);
	// Billboard
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
	VkImage billboardImage2;
	VkDeviceMemory billboardImageMemory2;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Billboard_png/Billboards_Tex_Tree2.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		billboardImage2,
		billboardImageMemory2
	);
	VkImage billboardNormalImage2;
	VkDeviceMemory billboardNormalImageMemory2;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Billboard_png/Billboards_Normal_Tex_Tree2.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		billboardNormalImage2,
		billboardNormalImageMemory2
	);
	// Fake Trees
	VkImage faketreeImage;
	VkDeviceMemory faketreeImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Fake_png/fake01.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		faketreeImage,
		faketreeImageMemory
	);
	VkImage faketreeNormalImage;
	VkDeviceMemory faketreeNormalImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Fake_png/blueNor.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		faketreeNormalImage,
		faketreeNormalImageMemory
	);
	VkImage faketreeImage2;
	VkDeviceMemory faketreeImageMemory2;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Fake_png/fake02.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		faketreeImage2,
		faketreeImageMemory2
	);
	VkImage faketreeNormalImage2;
	VkDeviceMemory faketreeNormalImageMemory2;
	Image::FromFile(device,
		transferCommandPool,
		"../../media/textures/Fake_png/blueNor.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		faketreeNormalImage2,
		faketreeNormalImageMemory2
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
	fbxloader = new FbxLoader("../../media/models/tree1_bark_LOD0.fbx");
	Model* bark = new Model(device, transferCommandPool,
		fbxloader->vertices,
		fbxloader->indices
	);
	bark->SetDiffuseMap(barkImage);
	bark->SetNormalMap(barkNormalImage);
	bark->SetNoiseMap(noiseImage);
	// Leaf
	fbxloader = new FbxLoader("../../media/models/tree1_leaf_LOD0.fbx");
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

	// Bark
	fbxloader = new FbxLoader("../../media/models/tree2_bark_rgba.FBX");
	Model* bark2 = new Model(device, transferCommandPool,
		fbxloader->vertices,
		fbxloader->indices
	);
	bark2->SetDiffuseMap(barkImage);
	bark2->SetNormalMap(barkNormalImage);
	bark2->SetNoiseMap(noiseImage);
	// Leaf
	fbxloader = new FbxLoader("../../media/models/tree2_leaf_rgba.FBX");
	Model* leaf2 = new Model(device, transferCommandPool,
		fbxloader->vertices,
		fbxloader->indices
	);
	leaf2->SetDiffuseMap(leafImage2);
	leaf2->SetNormalMap(leafNormalImage2);
	leaf2->SetNoiseMap(noiseImage);
	// Billboard
	Model* billboard2 = new Model(device, transferCommandPool,
	{
		{ { -billWidth / 2.0, billheigth, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f },{ 0.583f, 0.344f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { -billWidth / 2.0, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f },{ 0.583f, 0.547f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { billWidth / 2.0, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f },{ 0.861f, 0.547f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { billWidth / 2.0, billheigth, 0.0f },{ 1.0f, 1.0f, 1.0f, 1.0f },{ 0.861f, 0.344f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } }
	},
	{ 1, 2, 0, 0, 2, 3 }
	);
	billboard2->SetDiffuseMap(billboardImage2);
	billboard2->SetNormalMap(billboardNormalImage2);
	billboard2->SetNoiseMap(noiseImage);
	
	// Fake Trees
	Model* fakeTree = new Model(device, transferCommandPool,
	{
		{ { -billWidth / 2.0, billheigth, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f },{ 0.020f, 0.725f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { -billWidth / 2.0, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f },{ 0.020f, 0.871f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { billWidth / 2.0, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f },{ 0.161f, 0.871f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { billWidth / 2.0, billheigth, 0.0f },{ 1.0f, 1.0f, 1.0f, 1.0f },{ 0.161f, 0.725f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } }
	},
	{ 1, 2, 0, 0, 2, 3 }
	);
	fakeTree->SetDiffuseMap(faketreeImage);
	fakeTree->SetNormalMap(faketreeNormalImage);
	fakeTree->SetNoiseMap(noiseImage);

	Model* fakeTree2 = new Model(device, transferCommandPool,
	{
		{ { -billWidth / 2.0, billheigth, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f },{ 0.209f, 0.359f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { -billWidth / 2.0, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f },{ 0.398f, 0.359f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { billWidth / 2.0, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f },{ 0.398f, 0.189f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } },
		{ { billWidth / 2.0, billheigth, 0.0f },{ 1.0f, 1.0f, 1.0f, 1.0f },{ 0.209f, 0.189f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, -1.0f, 0.0f } }
	},
	{ 1, 2, 0, 0, 2, 3 }
	);
	fakeTree2->SetDiffuseMap(faketreeImage2);
	fakeTree2->SetNormalMap(faketreeNormalImage2);
	fakeTree2->SetNoiseMap(noiseImage);

	//skybox
	Skybox* skybox = new Skybox(device, transferCommandPool,
	{
		{ { 1,1,1,1 } },{ { -1,1,1,1 } },{ { -1,1,-1,1 } },{ { 1,1,-1,1 } },
		{ { 1,-1,1,1 } },{ { -1,-1,1,1 } },{ { -1,-1,-1,1 } },{ { 1,-1,-1,1 } }
	}
		, { { 2,1,0,3,2,0,//top
		4,5,6,4,6,7,//bottom
		6,2,3,7,6,3,//front
		0,1,5,0,5,4,//back
		7,3,0,4,7,0,//right
		5,1,2,6,5,2,//left
			} });

	srand(213910);
	// Blades
	printf("Building Blades\n");
	Blades* blades = new Blades(device, transferCommandPool, planeDim, terrain);
	printf("Finish Building Blades\n");


// Scene Initialization
	Scene* scene = new Scene(device);
	scene->SetTerrain(terrain);
	scene->SetSkybox(skybox);
	scene->AddModel(plane);
	scene->AddModel(bark);
	scene->AddModel(leaf);
	scene->AddModel(billboard);
	scene->AddModel(bark2);
	scene->AddModel(leaf2);
	scene->AddModel(billboard2);
	scene->AddModel(fakeTree);
	scene->AddModel(fakeTree2);
	scene->AddBlades(blades);
	// Insert Trees
	//Instance Data
	printf("Starting Insert Trees Randomly\n");
	//srand((unsigned int)time(0));
	printf("Tree 1\n");
	scene->InsertRandomTrees(100, 0.015f, 1, device, transferCommandPool);
	scene->AddLODInfoBuffer(glm::vec4(LOD0, LOD1, 20.0f, scene->GetInstanceBuffer()[0]->GetInstanceCount()));
	printf("Tree 2\n");
	scene->InsertRandomTrees(35, 0.021f, 4, device, transferCommandPool);
	scene->AddLODInfoBuffer(glm::vec4(LOD0, LOD1, 20.0f, scene->GetInstanceBuffer()[1]->GetInstanceCount()));
	printf("Finish Insert Trees Randomly\n");
	printf("Gathering Fake Trees\n");
	scene->GatherFakeTrees(device, transferCommandPool);
	printf("Finish Gather Fake Trees\n");
	vkDestroyCommandPool(device->GetVkDevice(), transferCommandPool, nullptr);

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
		//scene->UpdateLODInfo(LOD0, LOD1);
		renderer->Frame();
		count++;
		if (count == 100) {
			int total_time = GetTickCount() - time_start;
			float distance = glm::length(glm::vec3(128,0,128)-camera->GetEyePos());
			printf("Camera Distance %f\n", distance);
			printf("Total Time for 100 frames: %d\n", total_time);
			printf("Time per frame: %f\n", float(total_time) / 100.0f);
			printf("fps: %f\n", 100000.0 / float(total_time));
			count = 0;
			time_start = GetTickCount();
		}
	}

	vkDeviceWaitIdle(device->GetVkDevice());

	vkDestroyImage(device->GetVkDevice(), grassImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), grassImageMemory, nullptr);

	vkDestroyImage(device->GetVkDevice(), terrainImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), terrainImageMemory, nullptr);

	vkDestroyImage(device->GetVkDevice(), barkImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), barkImageMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), barkNormalImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), barkNormalImageMemory, nullptr);

	vkDestroyImage(device->GetVkDevice(), leafImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), leafImageMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), leafNormalImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), leafNormalImageMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), leafImage2, nullptr);
	vkFreeMemory(device->GetVkDevice(), leafImageMemory2, nullptr);
	vkDestroyImage(device->GetVkDevice(), leafNormalImage2, nullptr);
	vkFreeMemory(device->GetVkDevice(), leafNormalImageMemory2, nullptr);


	vkDestroyImage(device->GetVkDevice(), billboardImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), billboardImageMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), billboardNormalImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), billboardNormalImageMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), billboardImage2, nullptr);
	vkFreeMemory(device->GetVkDevice(), billboardImageMemory2, nullptr);
	vkDestroyImage(device->GetVkDevice(), billboardNormalImage2, nullptr);
	vkFreeMemory(device->GetVkDevice(), billboardNormalImageMemory2, nullptr);

	vkDestroyImage(device->GetVkDevice(), faketreeImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), faketreeImageMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), faketreeNormalImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), faketreeNormalImageMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), faketreeImage2, nullptr);
	vkFreeMemory(device->GetVkDevice(), faketreeImageMemory2, nullptr);
	vkDestroyImage(device->GetVkDevice(), faketreeNormalImage2, nullptr);
	vkFreeMemory(device->GetVkDevice(), faketreeNormalImageMemory2, nullptr);

	vkDestroyImage(device->GetVkDevice(), noiseImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), noiseImageMemory, nullptr);

	//vkDestroyImage(device->GetVkDevice(), SkyboxTexture, nullptr);
	//vkFreeMemory(device->GetVkDevice(), SkyboxTextureMemory, nullptr);

	delete scene;
	delete plane;
	delete bark;
	delete leaf;
	delete billboard;
	delete bark2;
	delete leaf2;
	delete billboard2;
	delete terrain;
	delete skybox;
	delete blades;
	delete camera;
	delete renderer;
	delete swapChain;
	delete device;
	delete instance;
	DestroyWindow();
	return 0;
}

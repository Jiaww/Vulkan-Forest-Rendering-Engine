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

#include "GUI.h"

Device* device;
SwapChain* swapChain;
Renderer* renderer;
Camera* camera;
GUI* gui = new GUI(nullptr);
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;

static float LOD0 = 0.6;
static float LOD1 = 0.43;

static bool DistanceCulling = true;
static bool FrustrumCulling = true;
static bool BarkModel = true;
static bool LeaveModel = true;
static bool BillboardModel = true;

static int plotIdx = 0;
static float fps[90] = { 0 };

//Wind
static float WindDirection[3] = { 0.5, 0.0, 1.0 };
static float windForce = 15.0f;
static float windSpeed = 12.0f;
static float waveInterval = 15.0f;
//Day&Night
static float Daylength = 30.0f;
static bool DayNightActivation = true;

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

	/////////////////////////////////////////////////////////////////////////////////
	//GUI

	void ImGui_ImplGlfwVulkan_RenderDrawLists(ImDrawData * draw_data)
	{
		if (!gui->device) return;
		ImGuiIO& io = ImGui::GetIO();

		// Create the Vertex Buffer:
		size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
		if (!gui->g_VertexBuffer[gui->g_FrameIndex] || gui->g_VertexBufferSize[gui->g_FrameIndex] < vertex_size)
		{
			if (gui->g_VertexBuffer[gui->g_FrameIndex])
				vkDestroyBuffer(gui->device->GetVkDevice(), gui->g_VertexBuffer[gui->g_FrameIndex], nullptr);
			if (gui->g_VertexBufferMemory[gui->g_FrameIndex])
				vkFreeMemory(gui->device->GetVkDevice(), gui->g_VertexBufferMemory[gui->g_FrameIndex], nullptr);
			size_t vertex_buffer_size = ((vertex_size - 1) / gui->g_BufferMemoryAlignment + 1) * gui->g_BufferMemoryAlignment;
			VkBufferCreateInfo buffer_info = {};
			buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_info.size = vertex_buffer_size;
			buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			vkCreateBuffer(gui->device->GetVkDevice(), &buffer_info, nullptr, &gui->g_VertexBuffer[gui->g_FrameIndex]);

			VkMemoryRequirements req;
			vkGetBufferMemoryRequirements(gui->device->GetVkDevice(), gui->g_VertexBuffer[gui->g_FrameIndex], &req);
			gui->g_BufferMemoryAlignment = (gui->g_BufferMemoryAlignment > req.alignment) ? gui->g_BufferMemoryAlignment : req.alignment;
			VkMemoryAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize = req.size;
			alloc_info.memoryTypeIndex = device->GetInstance()->GetMemoryTypeIndex(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); 
			vkAllocateMemory(gui->device->GetVkDevice(), &alloc_info, nullptr, &gui->g_VertexBufferMemory[gui->g_FrameIndex]);
			vkBindBufferMemory(gui->device->GetVkDevice(), gui->g_VertexBuffer[gui->g_FrameIndex], gui->g_VertexBufferMemory[gui->g_FrameIndex], 0);
			gui->g_VertexBufferSize[gui->g_FrameIndex] = vertex_buffer_size;
		}

		// Create the Index Buffer:
		size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
		if (!gui->g_IndexBuffer[gui->g_FrameIndex] || gui->g_IndexBufferSize[gui->g_FrameIndex] < index_size)
		{
			if (gui->g_IndexBuffer[gui->g_FrameIndex])
				vkDestroyBuffer(gui->device->GetVkDevice(), gui->g_IndexBuffer[gui->g_FrameIndex], nullptr);
			if (gui->g_IndexBufferMemory[gui->g_FrameIndex])
				vkFreeMemory(gui->device->GetVkDevice(), gui->g_IndexBufferMemory[gui->g_FrameIndex], nullptr);
			size_t index_buffer_size = ((index_size - 1) / gui->g_BufferMemoryAlignment + 1) * gui->g_BufferMemoryAlignment;
			VkBufferCreateInfo buffer_info = {};
			buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_info.size = index_buffer_size;
			buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			vkCreateBuffer(gui->device->GetVkDevice(), &buffer_info, nullptr, &gui->g_IndexBuffer[gui->g_FrameIndex]);		

			VkMemoryRequirements req;
			vkGetBufferMemoryRequirements(gui->device->GetVkDevice(), gui->g_IndexBuffer[gui->g_FrameIndex], &req);
			gui->g_BufferMemoryAlignment = (gui->g_BufferMemoryAlignment > req.alignment) ? gui->g_BufferMemoryAlignment : req.alignment;
			VkMemoryAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize = req.size;
			alloc_info.memoryTypeIndex = device->GetInstance()->GetMemoryTypeIndex(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); 
			vkAllocateMemory(gui->device->GetVkDevice(), &alloc_info, nullptr, &gui->g_IndexBufferMemory[gui->g_FrameIndex]);

			vkBindBufferMemory(gui->device->GetVkDevice(), gui->g_IndexBuffer[gui->g_FrameIndex], gui->g_IndexBufferMemory[gui->g_FrameIndex], 0);
			gui->g_IndexBufferSize[gui->g_FrameIndex] = index_buffer_size;
		}

		// Upload Vertex and index Data:
		{
			ImDrawVert* vtx_dst;
			ImDrawIdx* idx_dst;
			vkMapMemory(gui->device->GetVkDevice(), gui->g_VertexBufferMemory[gui->g_FrameIndex], 0, vertex_size, 0, (void**)(&vtx_dst));
			vkMapMemory(gui->device->GetVkDevice(), gui->g_IndexBufferMemory[gui->g_FrameIndex], 0, index_size, 0, (void**)(&idx_dst));

			for (int n = 0; n < draw_data->CmdListsCount; n++)
			{
				const ImDrawList* cmd_list = draw_data->CmdLists[n];
				memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
				vtx_dst += cmd_list->VtxBuffer.Size;
				idx_dst += cmd_list->IdxBuffer.Size;
			}
			VkMappedMemoryRange range[2] = {};
			range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = gui->g_VertexBufferMemory[gui->g_FrameIndex];
			range[0].size = VK_WHOLE_SIZE;
			range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[1].memory = gui->g_IndexBufferMemory[gui->g_FrameIndex];
			range[1].size = VK_WHOLE_SIZE;
			vkFlushMappedMemoryRanges(gui->device->GetVkDevice(), 2, range);

			vkUnmapMemory(gui->device->GetVkDevice(), gui->g_VertexBufferMemory[gui->g_FrameIndex]);
			vkUnmapMemory(gui->device->GetVkDevice(), gui->g_IndexBufferMemory[gui->g_FrameIndex]);
		}
		gui->draw_data = draw_data;
	}

	void ImGui_ImplGlfwVulkan_SetClipboardText(void * user_data, const char * text)
	{
		glfwSetClipboardString((GLFWwindow*)user_data, text);
	}

	const char * ImGui_ImplGlfwVulkan_GetClipboardText(void * user_data)
	{
		return glfwGetClipboardString((GLFWwindow*)user_data);
	}

	void ImGui_ImplGlfwVulkan_NewFrame(GLFWwindow * window)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Setup display size (every frame to accommodate for window resizing)
		int w, h;
		int display_w, display_h;
		glfwGetWindowSize(window, &w, &h);
		glfwGetFramebufferSize(window, &display_w, &display_h);
		io.DisplaySize = ImVec2((float)w, (float)h);
		io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

		// Setup time step
		double current_time =  glfwGetTime();
		io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
		g_Time = current_time;

		// Setup inputs
		// (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
		if (glfwGetWindowAttrib(window, GLFW_FOCUSED))
		{
			double mouse_x, mouse_y;
			glfwGetCursorPos(window, &mouse_x, &mouse_y);
			io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
		}
		else
		{
			io.MousePos = ImVec2(-FLT_MAX,-FLT_MAX);
		}

		for (int i = 0; i < 3; i++)
		{
			io.MouseDown[i] = g_MousePressed[i] || glfwGetMouseButton(window, i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
			g_MousePressed[i] = false;
		}

		io.MouseWheel = g_MouseWheel;
		g_MouseWheel = 0.0f;

		// Hide OS mouse cursor if ImGui is drawing it
		glfwSetInputMode(window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

		// Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
		ImGui::NewFrame();
	}
	bool ImGui_ImplGlfwVulkan_Init(GLFWwindow * window)
	{

		ImGuiIO& io = ImGui::GetIO();
		io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                         // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
		io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
		io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
		io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
		io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
		io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
		io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
		io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
		io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
		io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
		io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
		io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
		io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
		io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
		io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

		io.RenderDrawListsFn = ImGui_ImplGlfwVulkan_RenderDrawLists;       // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
		io.SetClipboardTextFn = ImGui_ImplGlfwVulkan_SetClipboardText;
		io.GetClipboardTextFn = ImGui_ImplGlfwVulkan_GetClipboardText;
		io.ClipboardUserData =  window;
		return true;
	}
	
	void InitialGuiContent() {
		ImGui::Text("Vulkan Forest Rendering Engine");
		ImGui::SliderFloat("LOD0", &LOD0, 0.0f, 1.0f);
		ImGui::SliderFloat("LOD1", &LOD1, 0.0f, 1.0f);
		ImGui::Checkbox("Frustrum Culling", &FrustrumCulling);
		ImGui::Checkbox("Distance Culling", &FrustrumCulling);
		ImGui::Checkbox("Bark Model", &BarkModel);
		ImGui::Checkbox("Leaves Model", &LeaveModel);
		ImGui::Checkbox("Billboard Model", &BillboardModel);
		ImGui::Text("Wind");
		ImGui::SliderFloat3("Wind Direction", WindDirection, -1.0f, 1.0f);
		ImGui::SliderFloat("Wind Force", &windForce, 0.0f, 100.0f);
		ImGui::SliderFloat("Wind Speed", &windSpeed, 0.1f, 100.0f);
		ImGui::SliderFloat("Wave Interval", &waveInterval, 0.1f, 100.0f);
		ImGui::Text("Day & Night");
		ImGui::SliderFloat("Day Length", &Daylength, 10.0f, 200.0f);
		ImGui::Checkbox("Day & Night Cycle", &DayNightActivation);
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Text("Performance");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Spacing();
		/*if (plotIdx >= 90) plotIdx = 0;
		fps[plotIdx] = ImGui::GetIO().Framerate;
		plotIdx++;
		static int offset = 0;
		ImGui::PlotLines("FPS", fps, IM_ARRAYSIZE(fps), offset, "", 0.0f, 400.0f, ImVec2(0, 100));
*/
	}
	//////////////////////////////////////////////////////////////////////////////////////////

}


	


int main() {
	static constexpr char* applicationName = "Vulkan Forest Render Engine";
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

//GUI Initialize
	gui->device = device;
	ImGui_ImplGlfwVulkan_Init(GetGLFWWindow());

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
	//skybox Texture
	VkImage skyboxImageDay;
	VkDeviceMemory skyboxImageDayMemory;
	std::vector<char*> cubemap_images_day = {
		"../../media/textures/Skybox_jpg/TropicalSunnyDay/TropicalSunnyDayLeft2048.png",
		"../../media/textures/Skybox_jpg/TropicalSunnyDay/TropicalSunnyDayRight2048.png",
		"../../media/textures/Skybox_jpg/TropicalSunnyDay/TropicalSunnyDayUp2048.png",
		"../../media/textures/Skybox_jpg/TropicalSunnyDay/TropicalSunnyDayDown2048.png",
		"../../media/textures/Skybox_jpg/TropicalSunnyDay/TropicalSunnyDayFront2048.png",
		"../../media/textures/Skybox_jpg/TropicalSunnyDay/TropicalSunnyDayBack2048.png",
	};
	Image::FromMultiFile(device,
		transferCommandPool,
		cubemap_images_day,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		skyboxImageDay,
		skyboxImageDayMemory
	);
	VkImage skyboxImageAfternoon;
	VkDeviceMemory skyboxImageAfternoonMemory;
	std::vector<char*> cubemap_images_afternoon = {
		"../../media/textures/Skybox_jpg/SunSet/SunSetLeft2048.png",
		"../../media/textures/Skybox_jpg/SunSet/SunSetRight2048.png",
		"../../media/textures/Skybox_jpg/SunSet/SunSetUp2048.png",
		"../../media/textures/Skybox_jpg/SunSet/SunSetDown2048.png",
		"../../media/textures/Skybox_jpg/SunSet/SunSetFront2048.png",
		"../../media/textures/Skybox_jpg/SunSet/SunSetBack2048.png",
	};
	Image::FromMultiFile(device,
		transferCommandPool,
		cubemap_images_afternoon,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		skyboxImageAfternoon,
		skyboxImageAfternoonMemory
	);
	VkImage skyboxImageNight;
	VkDeviceMemory skyboxImageNightMemory;
	std::vector<char*> cubemap_images_night = {
		"../../media/textures/Skybox_jpg/FullMoon/FullMoonLeft2048.png",
		"../../media/textures/Skybox_jpg/FullMoon/FullMoonRight2048.png",
		"../../media/textures/Skybox_jpg/FullMoon/FullMoonUp2048.png",
		"../../media/textures/Skybox_jpg/FullMoon/FullMoonDown2048.png",
		"../../media/textures/Skybox_jpg/FullMoon/FullMoonFront2048.png",
		"../../media/textures/Skybox_jpg/FullMoon/FullMoonBack2048.png",
	};
	Image::FromMultiFile(device,
		transferCommandPool,
		cubemap_images_night,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		skyboxImageNight,
		skyboxImageNightMemory
	);

	VkImage FontTexture;
	VkDeviceMemory FontTextureMemory;
	Image::FromGuiTexture(device,
		transferCommandPool,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		FontTexture,
		FontTextureMemory
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
	skybox->SetDiffuseMapIdx(skyboxImageDay, 0);
	skybox->SetDiffuseMapIdx(skyboxImageAfternoon, 1);
	skybox->SetDiffuseMapIdx(skyboxImageNight, 2);

	gui->SetFontTextureMap(FontTexture);

	//srand(213910);
	srand((unsigned int)time(0));
	// Blades
	printf("Building Blades\n");
	Blades* blades = new Blades(device, transferCommandPool, planeDim, terrain);
	printf("Finish Building Blades\n");

// Scene Initialization

	Scene* scene = new Scene(device);
	scene->SetTerrain(terrain);
	scene->SetSkybox(skybox);
	scene->SetGui(gui);
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
	scene->InsertRandomTrees(150, 0.015f, 1, device, transferCommandPool);
	scene->AddLODInfoBuffer(glm::vec4(LOD0, LOD1, 20.0f, scene->GetInstanceBuffer()[0]->GetInstanceCount()));
	printf("Tree 2\n");
	scene->InsertRandomTrees(40, 0.021f, 4, device, transferCommandPool);
	scene->AddLODInfoBuffer(glm::vec4(LOD0, LOD1, 20.0f, scene->GetInstanceBuffer()[1]->GetInstanceCount()));
	printf("Finish Insert Trees Randomly\n");
	printf("Gathering Fake Trees\n");
	scene->GatherFakeTrees(device, transferCommandPool);
	printf("Finish Gather Fake Trees\n");
	vkDestroyCommandPool(device->GetVkDevice(), transferCommandPool, nullptr);

	//float yy = scene->GetTerrain()->GetHeight(0.25,0.25);
	//scene->InsertRandomTrees(20, device, transferCommandPool);

	//First Initialization
	ImGui_ImplGlfwVulkan_NewFrame(GetGLFWWindow());
	InitialGuiContent();
	ImGui::Render();
	

	renderer = new Renderer(device, swapChain, scene, camera);
	gui->g_FrameIndex = (gui->g_FrameIndex + 1) % IMGUI_VK_QUEUED_FRAMES;
	
	glfwSetWindowSizeCallback(GetGLFWWindow(), resizeCallback);
	glfwSetMouseButtonCallback(GetGLFWWindow(), mouseDownCallback);
	glfwSetCursorPosCallback(GetGLFWWindow(), mouseMoveCallback);
	glfwSetScrollCallback(GetGLFWWindow(), mouseWheelCallback);

	int time_start = GetTickCount();
	int count = 0;

	while (!ShouldQuit()) {
		glfwPollEvents();
		ImGui_ImplGlfwVulkan_NewFrame(GetGLFWWindow());

		InitialGuiContent();
		ImGui::Render();

		scene->UpdateTime();
		scene->UpdateLODInfo(LOD0, LOD1);
		scene->UpdateWindInfo(glm::vec4(WindDirection[0],  WindDirection[1], WindDirection[2], 1.0f), glm::vec4(windForce, windSpeed, waveInterval, 1.0f));
		scene->UpdateDayNightInfo(Daylength, DayNightActivation);
		renderer->Frame();
		count++;
		if (count == 100) {
		//	int total_time = GetTickCount() - time_start;
			float distance = glm::length(glm::vec3(128,0,128)-camera->GetEyePos());
			printf("Camera Distance %f\n", distance);
		//	printf("Total Time for 100 frames: %d\n", total_time);
		//	printf("Time per frame: %f\n", float(total_time) / 100.0f);
		//	printf("fps: %f\n", 100000.0 / float(total_time));
			count = 0;
			time_start = GetTickCount();
		}
		gui->g_FrameIndex = (gui->g_FrameIndex + 1) % IMGUI_VK_QUEUED_FRAMES;
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


	vkDestroyImage(device->GetVkDevice(), skyboxImageDay, nullptr);
	vkFreeMemory(device->GetVkDevice(), skyboxImageDayMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), skyboxImageAfternoon, nullptr);
	vkFreeMemory(device->GetVkDevice(), skyboxImageAfternoonMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), skyboxImageNight, nullptr);
	vkFreeMemory(device->GetVkDevice(), skyboxImageNightMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), noiseImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), noiseImageMemory, nullptr);
	vkDestroyImage(device->GetVkDevice(), FontTexture, nullptr);
	vkFreeMemory(device->GetVkDevice(), FontTextureMemory, nullptr);


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
	delete gui;
	delete renderer;
	ImGui::Shutdown();
	delete swapChain;
	delete device;
	delete instance;
	DestroyWindow();
	return 0;
}

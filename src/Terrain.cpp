#include "Terrain.h"
#include "BufferUtils.h"
#include "Image.h"

Terrain::Terrain(Device* device, VkCommandPool commandPool, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
	: device(device), vertices(vertices), indices(indices) {

	if (vertices.size() > 0) {
		BufferUtils::CreateBufferFromData(device, commandPool, this->vertices.data(), vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMemory);
	}

	if (indices.size() > 0) {
		BufferUtils::CreateBufferFromData(device, commandPool, this->indices.data(), indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferMemory);
	}

	modelBufferObject.modelMatrix = glm::mat4(1.0f);
	BufferUtils::CreateBufferFromData(device, commandPool, &modelBufferObject, sizeof(ModelBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, modelBuffer, modelBufferMemory);
}

Terrain::~Terrain() {
	if (indices.size() > 0) {
		vkDestroyBuffer(device->GetVkDevice(), indexBuffer, nullptr);
		vkFreeMemory(device->GetVkDevice(), indexBufferMemory, nullptr);
	}

	if (vertices.size() > 0) {
		vkDestroyBuffer(device->GetVkDevice(), vertexBuffer, nullptr);
		vkFreeMemory(device->GetVkDevice(), vertexBufferMemory, nullptr);
	}

	vkDestroyBuffer(device->GetVkDevice(), modelBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), modelBufferMemory, nullptr);

	if (diffuseMapView != VK_NULL_HANDLE) {
		vkDestroyImageView(device->GetVkDevice(), diffuseMapView, nullptr);
	}

	if (diffuseMapSampler != VK_NULL_HANDLE) {
		vkDestroySampler(device->GetVkDevice(), diffuseMapSampler, nullptr);
	}

	if (normalMapView != VK_NULL_HANDLE) {
		vkDestroyImageView(device->GetVkDevice(), normalMapView, nullptr);
	}

	if (normalMapSampler != VK_NULL_HANDLE) {
		vkDestroySampler(device->GetVkDevice(), normalMapSampler, nullptr);
	}
}

void Terrain::SetDiffuseMap(VkImage texture) {
	this->diffuseMap = texture;
	this->diffuseMapView = Image::CreateView(device, texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

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

void Terrain::SetNormalMap(VkImage texture) {
	this->normalMap = texture;
	this->normalMapView = Image::CreateView(device, texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

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

	if (vkCreateSampler(device->GetVkDevice(), &samplerInfo, nullptr, &normalMapSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler");
	}
}

const std::vector<Vertex>& Terrain::getVertices() const {
	return vertices;
}

VkBuffer Terrain::getVertexBuffer() const {
	return vertexBuffer;
}

const std::vector<uint32_t>& Terrain::getIndices() const {
	return indices;
}

VkBuffer Terrain::getIndexBuffer() const {
	return indexBuffer;
}

const ModelBufferObject& Terrain::getModelBufferObject() const {
	return modelBufferObject;
}

VkBuffer Terrain::GetModelBuffer() const {
	return modelBuffer;
}

VkImageView Terrain::GetDiffuseMapView() const {
	return diffuseMapView;
}

VkSampler Terrain::GetDiffuseMapSampler() const {
	return diffuseMapSampler;
}

VkImageView Terrain::GetNormalMapView() const {
	return normalMapView;
}

VkSampler Terrain::GetNormalMapSampler() const {
	return normalMapSampler;
}

Terrain* Terrain::LoadTerrain(Device* device, VkCommandPool commandPool, char *filePath, char *rawPath, float terrainDim) {
	printf("Loading terrain height Map...\n");
	int mapWidth, mapHeight, mapBpp;
	uint8_t* rgb_image = stbi_load(filePath, &mapWidth, &mapHeight, &mapBpp, 1);
	int size = mapHeight * mapWidth;
	float* mapHeights = new float[size];
	FILE *fp = fopen(rawPath, "r");
	uint16_t *heightsBuffer = new uint16_t[size];
	fread(heightsBuffer, sizeof(uint16_t), size, fp);
	for (int i = 0; i<size; i++) {
		mapHeights[i] = heightsBuffer[i] / 65536.0f * 256.0f * 0.5f - 40.0f;
	}
	delete heightsBuffer;
	printf("Building terrain...\n");
	float half_width = 0.5f*(float)mapWidth;
	float half_height = 0.5f*(float)mapHeight;

	const float inv_height = 1.0f / (float)mapHeight;
	const float inv_width = 1.0f / (float)mapWidth;

	std::vector<Vertex> vertices;
	int num_vertices = (mapHeight - 2) * (mapWidth - 2);
	vertices.resize((mapHeight - 2) * (mapWidth - 2));

	//vertices_data
	for (int z = 1; z<mapHeight - 1; z++)
	{
		for (int x = 1; x<mapWidth - 1; x++)
		{
			float fx = (float)x;
			float fz = (float)z;
			//Vertex 1
			//Position
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].pos.x = fx / (mapWidth / terrainDim);
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].pos.y = mapHeights[x + z*mapWidth];
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].pos.z = fz / (mapHeight / terrainDim);
			//Color(doen't use)
			//Position
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].color.x = 0;
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].color.y = 0;
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].color.z = 0;
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].color.z = 0;
			//TexCoords
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].texCoord.x = fx*inv_width;
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].texCoord.y = fz*inv_height;
			//Normal(doen's use)
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].normal.x = 0;
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].normal.y = 0;
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].normal.z = 0;
			//Tangent(doen's use)
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].tangent.x = 0;
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].tangent.y = 0;
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].tangent.z = 0;
			//Bitangent(doen's use)
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].bitangent.x = 0;
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].bitangent.y = 0;
			vertices[((x - 1) + (z - 1)*(mapWidth - 2))].bitangent.z = 0;
		}
	}
	//indices data
	std::vector<uint32_t>indices;
	for (int z = 1; z<mapHeight - 2; z++)
	{
		for (int x = 1; x<mapWidth - 2; x++)
		{
			indices.push_back((x - 1) + (z - 1) * (mapWidth - 2));
			indices.push_back((x - 1) + z * (mapWidth - 2));
			indices.push_back(x + (z - 1) * (mapWidth - 2));

			indices.push_back((x - 1) + z * (mapWidth - 2));
			indices.push_back(x + z * (mapWidth - 2));
			indices.push_back(x + (z - 1) * (mapWidth - 2));
		}
	}
	printf("Generating new Terrain Object...\n");

	Terrain* terrain = new Terrain(device, commandPool, vertices, indices);
	terrain->width = mapWidth;
	terrain->height = mapHeight;
	terrain->heights = mapHeights;
	terrain->terrainDim = terrainDim;
	return terrain;
}

float Terrain::GetHeight(float x, float z) const{
	if (x<0 || z<0 || x>terrainDim-2 || z>terrainDim-2)
		return 0;
	//float Epsilon = 0.005;
	//float idx = x*height / terrainDim + width*z*width / terrainDim;
	//if ((idx - int(idx))<Epsilon)
	//	return heights[int(idx)];
//Bilinear Interpolation
	float u = (x * (height / terrainDim) - int(x * (height / terrainDim)));
	float v = (z * (height / terrainDim) - int(z * (height / terrainDim)));

	float y = 
		(heights[int(floor(x) * (height / terrainDim) + width * (floor(z) * (height / terrainDim)))] * (1-u)+
		heights[int(floor(x) * (height / terrainDim) + 1 + width * (floor(z) * (height / terrainDim)))] * u) * (1-v) +
		(heights[int(floor(x) * (height / terrainDim) + width * (floor(z) * (height / terrainDim) + 1))] * (1-u) +
		heights[int(floor(x) * (height / terrainDim) + 1 + width * (floor(z) * (height / terrainDim) + 1))] * u) * v;
	return y;
}

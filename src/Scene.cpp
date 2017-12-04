#include "Scene.h"
#include "BufferUtils.h"

Scene::Scene(Device* device) : device(device) {
    BufferUtils::CreateBuffer(device, sizeof(Time), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, timeBuffer, timeBufferMemory);
    vkMapMemory(device->GetVkDevice(), timeBufferMemory, 0, sizeof(Time), 0, &mappedData);
    memcpy(mappedData, &time, sizeof(Time));
}

const Terrain* Scene::GetTerrain() const {
	return terrain;
}

const std::vector<Model*>& Scene::GetModels() const {
    return models;
}

const std::vector<Blades*>& Scene::GetBlades() const {
  return blades;
}

const std::vector<InstanceBuffer*>& Scene::GetInstanceBuffer() const
{
	return instanceData;
}

void Scene::SetTerrain(Terrain* terrain) {
	this->terrain = terrain;
}
void Scene::AddModel(Model* model) {
    models.push_back(model);
}

void Scene::AddBlades(Blades* blades) {
  this->blades.push_back(blades);
}

void Scene::AddInstanceBuffer(InstanceBuffer * Data)
{
	instanceData.push_back(Data);
}

void Scene::UpdateTime() {
    high_resolution_clock::time_point currentTime = high_resolution_clock::now();
    duration<float> nextDeltaTime = duration_cast<duration<float>>(currentTime - startTime);
    startTime = currentTime;

    time.TimeInfo[0] = nextDeltaTime.count();
    time.TimeInfo[1] += time.TimeInfo[0];

    memcpy(mappedData, &time, sizeof(Time));
}

VkBuffer Scene::GetTimeBuffer() const {
    return timeBuffer;
}

void Scene::AddLODInfoBuffer(glm::vec4 LODInfo) {
	LODInfoVec.push_back(LODInfo);
	void* p;
	VkBuffer LODInfoBuf;
	VkDeviceMemory LODInfoBufMemory;
	BufferUtils::CreateBuffer(device, sizeof(glm::vec4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, LODInfoBuf, LODInfoBufMemory);
	vkMapMemory(device->GetVkDevice(), LODInfoBufMemory, 0, sizeof(glm::vec4), 0, &p);
	memcpy(p, &LODInfoVec[LODInfoVec.size() - 1], sizeof(glm::vec4));
	LODmappedData.push_back(p);
	LODInfoBuffer.push_back(LODInfoBuf);
	LODInfoBufferMemory.push_back(LODInfoBufMemory);
}

std::vector<VkBuffer> Scene::GetLODInfoBuffer() const {
	return LODInfoBuffer;
}

void Scene::UpdateLODInfo(float LOD0, float LOD1) {
	for (int i = 0; i < LODInfoVec.size(); i++) {
		LODInfoVec[i][0] = LOD0;
		LODInfoVec[i][1] = LOD1;
		memcpy(LODmappedData[i], &LODInfoVec[i], sizeof(glm::vec4));
	}
}
bool Scene::InsertRandomTrees(int numTrees, Device* device, VkCommandPool commandPool) {
//	std::vector<InstanceData> instanceData;
//	srand(33974);
//	int randRange = terrain->GetTerrainDim();
//	for (int i = 0; i < numTrees; i++) {
//		float posX = (rand() % (randRange * 100)) / 100.0f;
//		float posZ = (rand() % (randRange * 100)) / 100.0f;
//		float posY = terrain->GetHeight(posX, posZ);
//		printf("|| Tree No.%f: <position: %f %f %f> ||\n", i, posX, posY, posZ);
//		glm::vec3 position(posX, posY, posZ);
////		instanceData.push_back(InstanceData(position));
//	}
//	InstanceBuffer* instanceBuffer = new InstanceBuffer(device, commandPool, instanceData);
//	AddInstanceBuffer(instanceBuffer);
	return true;
}

Scene::~Scene() {
	vkUnmapMemory(device->GetVkDevice(), timeBufferMemory);
	vkDestroyBuffer(device->GetVkDevice(), timeBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), timeBufferMemory, nullptr);
}

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

const std::vector<InstanceBuffer*>& Scene::GetCulledInstanceBuffer() const
{
	return culledInstanceData;
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

void Scene::AddCulledInstanceBuffer(InstanceBuffer * Data)
{
	culledInstanceData.push_back(Data);
}

void Scene::UpdateTime() {
    high_resolution_clock::time_point currentTime = high_resolution_clock::now();
    duration<float> nextDeltaTime = duration_cast<duration<float>>(currentTime - startTime);
    startTime = currentTime;

    time.deltaTime = nextDeltaTime.count();
    time.totalTime += time.deltaTime;

    memcpy(mappedData, &time, sizeof(Time));
}

VkBuffer Scene::GetTimeBuffer() const {
    return timeBuffer;
}

bool Scene::InsertRandomTrees(int numTrees, Device* device, VkCommandPool commandPool) {
	std::vector<InstanceData> instanceData;
	srand(33974);
	int randRange = terrain->GetTerrainDim();
	for (int i = 0; i < numTrees; i++) {
		float posX = (rand() % (randRange * 100)) / 100.0f;
		float posZ = (rand() % (randRange * 100)) / 100.0f;
		float posY = terrain->GetHeight(posX, posZ);
		printf("|| Tree No.%f: <position: %f %f %f> ||\n", i, posX, posY, posZ);
		glm::vec3 position(posX, posY, posZ);
//		instanceData.push_back(InstanceData(position));
	}
	InstanceBuffer* instanceBuffer = new InstanceBuffer(device, commandPool, instanceData);
	AddInstanceBuffer(instanceBuffer);
	return true;
}

Scene::~Scene() {
	vkUnmapMemory(device->GetVkDevice(), timeBufferMemory);
	vkDestroyBuffer(device->GetVkDevice(), timeBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), timeBufferMemory, nullptr);
}

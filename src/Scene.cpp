#include "Scene.h"
#include "BufferUtils.h"

Scene::Scene(Device* device) : device(device) {
    BufferUtils::CreateBuffer(device, sizeof(Time), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, timeBuffer, timeBufferMemory);
    vkMapMemory(device->GetVkDevice(), timeBufferMemory, 0, sizeof(Time), 0, &mappedData);
    memcpy(mappedData, &time, sizeof(Time));
	numFakeTree = 0;
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
	return instanceBuffers;
}

const std::vector<FakeInstanceBuffer*>& Scene::GetFakeInstanceBuffer() const
{
	return fakeInstanceBuffers;
}

void Scene::SetTerrain(Terrain* terrain) {
	this->terrain = terrain;
	meshDim = terrain->GetTerrainDim() - 2;
	densityMesh = new int[meshDim * meshDim];
	memset(densityMesh, 0, meshDim * meshDim * sizeof(int));
	/*densityVector.resize(meshDim);
	for (int i = 0; i < meshDim; ++i)
		densityVector[i].resize(meshDim, 0);*/
}
void Scene::AddModel(Model* model) {
    models.push_back(model);
}

void Scene::AddBlades(Blades* blades) {
  this->blades.push_back(blades);
}

void Scene::AddInstanceBuffer(InstanceBuffer * Data)
{
	instanceBuffers.push_back(Data);
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
bool Scene::InsertRandomTrees(int numTrees, float treeBaseScale, int modelId, Device* device, VkCommandPool commandPool) {
	std::vector<InstanceData> instanceData;
	int randRange = terrain->GetTerrainDim() - 2;
	for (int i = 0; i < numTrees; i++) {
		float posX = (rand() % (randRange * 5)) / 5.0f;
		float posZ = (rand() % (randRange * 5)) / 5.0f;
		float posY = terrain->GetHeight(posX, posZ);
		glm::vec3 position(posX, posY, posZ);
		float scale = 0.9f + float(rand() % 200) / 1000.0f;
		scale *= treeBaseScale;
		float r = (175 + float(rand() % 80)) / 255.0f;
		float g = (175 + float(rand() % 80)) / 255.0f;
		float b = (175 + float(rand() % 80)) / 255.0f;
		float theta = float(rand() % 3145) / 1000.0f;
		printf("|| Tree No.%d: <position: %f %f %f> <scale: %f> <theta: %f> <tintColor: %f %f %f>||\n", i, posX, posY, posZ, scale, theta, r, g, b);
		instanceData.push_back(InstanceData(glm::vec4(position, scale), glm::vec4(r, g, b, theta)));
		UpdateDensityDistribution(int(posX), int(posZ));
	}
	InstanceBuffer* instanceBuffer = new InstanceBuffer(device, commandPool, instanceData, models[modelId]->getIndices().size(), models[modelId+1]->getIndices().size(), models[modelId+2]->getIndices().size());
	AddInstanceBuffer(instanceBuffer);
	return true;
}

int Scene::GetDensityMeshValue(int x, int z) {
	if (x > meshDim - 1 || x < 0 || z > meshDim - 1 || z < 0)
		return -1;
	else
		return densityMesh[x*meshDim + z];
		//return densityVector[x][z];
}

void Scene::SetDensityMeshValue(int x, int z, int value) {
	if (x > meshDim - 1 || x < 0 || z > meshDim - 1 || z < 0)
		return;
	else {
		if (value == -1) { // Set Negative
			if(densityMesh[x*meshDim + z] == 0)
				densityMesh[x*meshDim + z] = value;
			/*if (densityVector[x][z] == 0)
				densityVector[x][z] = value;*/
		}
		else {
			densityMesh[x*meshDim + z] = value;
			//densityVector[x][z] = value;
		}
	}
}

void Scene::UpdateDensityDistribution(int x, int z) {
	// -1: Cannot insert fake tree
	// 0:  Can insert fake tree
	// 1:  already insert a real tree
	// >2: already insert a fake tree
	// x: 0~MeshDim-1 z:0~MeshDim-1
	if (x > meshDim - 1 || x < 0 || z > meshDim - 1 || z < 0)
		return;
	densityMesh[x*meshDim + z] = 1;
	//densityVector[x][z] = 1;
	int influence_range_max = 8;
	int influence_range_min = 5;
	int nearbyDistance = 7;
	int maxNumNearbyFake = 1;// 1 or 2
	for (int m = int(x) - nearbyDistance; m <= int(x) + nearbyDistance; m++)
		for (int n = int(z) - nearbyDistance; n <= int(z) + nearbyDistance; n++) {
			if (m == int(x) && n == int(z))
				continue;
			SetDensityMeshValue(m, n, -1);
		}

	std::vector<glm::vec2> squares;
	for(int i = - influence_range_max; i <= influence_range_max; i++)
		for (int j = -influence_range_max; j <= influence_range_max; j++) {
			if (abs(i) > influence_range_min && abs(j) > influence_range_min) {
				if(GetDensityMeshValue(x+i, z+j) == 0) // Can insert fake tree
					squares.push_back(glm::vec2(x+i,z+j));
			}
		}

	std::random_shuffle(squares.begin(), squares.end());
	maxNumNearbyFake = glm::min(maxNumNearbyFake, int(squares.size()));
	for (int i = 0; i < maxNumNearbyFake; i++) {
		if (GetDensityMeshValue(squares[i].x, squares[i].y) == 0) { // can insert fake tree
			SetDensityMeshValue(squares[i].x, squares[i].y, 2 + numFakeTree);
			numFakeTree++;
			for(int m = int(squares[i].x) - nearbyDistance; m <= int(squares[i].x) + nearbyDistance; m++)
				for (int n = int(squares[i].y) - nearbyDistance; n <= int(squares[i].y) + nearbyDistance; n++) {
					if (m == int(squares[i].x) && n == int(squares[i].y))
						continue;
					SetDensityMeshValue(m, n, -1);
				}
		}
	}
}

void Scene::GatherFakeTrees(Device* device, VkCommandPool commandPool) {
	std::vector<InstanceData> instanceData;
	std::vector<InstanceData> instanceData2;
	for(int i = 0; i < meshDim; i++)
		for (int j = 0; j < meshDim; j++) {
			if (GetDensityMeshValue(i, j) > 1) {
				float height = terrain->GetHeight(i, j);
				glm::vec3 position(i, height, j);
				float scale = 0.9f + float(rand() % 200) / 1000.0f;
				float r = (175 + float(rand() % 80)) / 255.0f;
				float g = (175 + float(rand() % 80)) / 255.0f;
				float b = (175 + float(rand() % 80)) / 255.0f;
				// theta = -1 means fake trees
				float theta = -1;
				if(GetDensityMeshValue(i, j)%10 < 7)
					instanceData.push_back(InstanceData(glm::vec4(position, scale), glm::vec4(r, g, b, theta)));
				else
					instanceData2.push_back(InstanceData(glm::vec4(position, scale), glm::vec4(r, g, b, theta)));
			}
		}
	FakeInstanceBuffer* fakeInstanceBuffer = new FakeInstanceBuffer(device, commandPool, instanceData);
	FakeInstanceBuffer* fakeInstanceBuffer2 = new FakeInstanceBuffer(device, commandPool, instanceData2);
	AddFakeInstanceBuffer(fakeInstanceBuffer);
	AddLODInfoBuffer(glm::vec4(0.65, 0.48, 20.0f, instanceData.size()));
	AddFakeInstanceBuffer(fakeInstanceBuffer2);
	AddLODInfoBuffer(glm::vec4(0.65, 0.48, 20.0f, instanceData2.size()));
}

void Scene::AddFakeInstanceBuffer(FakeInstanceBuffer * Data)
{
	fakeInstanceBuffers.push_back(Data);
}

Scene::~Scene() {
	vkUnmapMemory(device->GetVkDevice(), timeBufferMemory);
	vkDestroyBuffer(device->GetVkDevice(), timeBuffer, nullptr);
	vkFreeMemory(device->GetVkDevice(), timeBufferMemory, nullptr);
}

#pragma once

#include <glm/glm.hpp>
#include <chrono>
#include <cstdlib> 
#include <math.h>

#include "Model.h"
#include "Blades.h"
#include "InstanceData.h"
#include "Terrain.h"
#include "skybox.h"

using namespace std::chrono;

struct Time {
	glm::vec2 TimeInfo = glm::vec2(0.0f, 0.0f);
	// 0: deltaTime 1: totalTime
};

class Scene {
private:
    Device* device;
    
    VkBuffer timeBuffer;
    VkDeviceMemory timeBufferMemory;
    Time time;

	std::vector<VkBuffer> LODInfoBuffer;
	std::vector<VkDeviceMemory> LODInfoBufferMemory;
	std::vector<glm::vec4> LODInfoVec;

    void* mappedData;
	std::vector<void*> LODmappedData;

	Terrain* terrain;
	Skybox* skybox;
    std::vector<Model*> models;
    std::vector<Blades*> blades;
	std::vector<InstanceBuffer*> instanceBuffers;
	std::vector<FakeInstanceBuffer*> fakeInstanceBuffers;
	//Density Multiplication
	//std::vector<std::vector<int>> densityVector;
	int *densityMesh;
	int meshDim;
	int numFakeTree;

	high_resolution_clock::time_point startTime = high_resolution_clock::now();

public:
    Scene() = delete;
    Scene(Device* device);
    ~Scene();


	const Terrain* GetTerrain() const;
	const Skybox* GetSkybox() const;
    const std::vector<Model*>& GetModels() const;
    const std::vector<Blades*>& GetBlades() const;
	const std::vector<InstanceBuffer*>& GetInstanceBuffer() const;
    std::vector<VkBuffer> GetLODInfoBuffer() const;
	const std::vector<FakeInstanceBuffer*>& GetFakeInstanceBuffer() const;

	void SetTerrain(Terrain* terrain);
	void SetSkybox(Skybox* skybox);
    void AddModel(Model* model);
    void AddBlades(Blades* blades);
	void AddInstanceBuffer(InstanceBuffer* Data);
	bool InsertRandomTrees(int numTrees, float treeBaseScale, int modelId, Device* device, VkCommandPool commandPool);
    VkBuffer GetTimeBuffer() const;
	void AddLODInfoBuffer(glm::vec4 LODInfo);

    void UpdateTime();
	void UpdateLODInfo(float LOD0, float LOD1);

	int GetDensityMeshValue(int x, int z);
	void SetDensityMeshValue(int x, int z, int value);
	void UpdateDensityDistribution(int x, int z);
	void GatherFakeTrees(Device* device, VkCommandPool commandPool);
	void AddFakeInstanceBuffer(FakeInstanceBuffer* Data);
	int GetNumFakeTree() { return numFakeTree; }
};

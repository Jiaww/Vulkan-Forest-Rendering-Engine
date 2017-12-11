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
#include "GUI.h"

using namespace std::chrono;

struct Time {
	glm::vec2 TimeInfo = glm::vec2(0.0f, 0.0f);
	// 0: deltaTime 1: totalTime
};

struct WindInfo {
	glm::vec4 WindDir = glm::vec4(0.5, 0.0, 1.0, 1.0);
	//0: windFroce(power), 1: windSpeed, 2: waveInterval
	glm::vec4 WindData = glm::vec4(15.0, 12.0, 15.0, 1.0);
};

struct DayNightInfo {
	//0: Daylength, 1: Activate
	glm::vec2 DayNightData = glm::vec2(30, 1);
};

class Scene {
private:
    Device* device;
    //Time
    VkBuffer timeBuffer;
    VkDeviceMemory timeBufferMemory;
    Time time;
	//LOD
	std::vector<VkBuffer> LODInfoBuffer;
	std::vector<VkDeviceMemory> LODInfoBufferMemory;
	std::vector<glm::vec4> LODInfoVec;
	//Wind
	VkBuffer windBuffer;
	VkDeviceMemory windBufferMemory;
	WindInfo wind;
	//Day&Night Cycle
	VkBuffer dayNightBuffer;
	VkDeviceMemory dayNightBufferMemory;
	DayNightInfo dayNight;

    void* mappedData;
	std::vector<void*> LODmappedData;
	void* WindmappedData;
	void* DayNightmappedData;

	Terrain* terrain;
	Skybox* skybox;
	GUI* gui;
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
	const GUI* GetGui() const;
    const std::vector<Model*>& GetModels() const;
    const std::vector<Blades*>& GetBlades() const;
	const std::vector<InstanceBuffer*>& GetInstanceBuffer() const;
	const std::vector<FakeInstanceBuffer*>& GetFakeInstanceBuffer() const;

	void SetTerrain(Terrain* terrain);
	void SetSkybox(Skybox* skybox);
	void SetGui(GUI* gui);
    void AddModel(Model* model);
    void AddBlades(Blades* blades);
	void AddInstanceBuffer(InstanceBuffer* Data);
	bool InsertRandomTrees(int numTrees, float treeBaseScale, int modelId, Device* device, VkCommandPool commandPool);
    VkBuffer GetTimeBuffer() const;
	void AddLODInfoBuffer(glm::vec4 LODInfo);
	std::vector<VkBuffer> GetLODInfoBuffer() const;
	VkBuffer GetWindBuffer() const;
	VkBuffer GetDayNightBuffer() const;

    void UpdateTime();
	void UpdateLODInfo(float LOD0, float LOD1);
	void UpdateWindInfo(glm::vec4 dir, glm::vec4 data);
	void UpdateDayNightInfo(float dlen, bool act);

	int GetDensityMeshValue(int x, int z);
	void SetDensityMeshValue(int x, int z, int value);
	void UpdateDensityDistribution(int x, int z);
	void GatherFakeTrees(Device* device, VkCommandPool commandPool);
	void AddFakeInstanceBuffer(FakeInstanceBuffer* Data);
	int GetNumFakeTree() { return numFakeTree; }
};

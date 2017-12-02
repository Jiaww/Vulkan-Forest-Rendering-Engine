#pragma once

#include <glm/glm.hpp>
#include <chrono>
#include <cstdlib> 

#include "Model.h"
#include "Blades.h"
#include "InstanceData.h"
#include "Terrain.h"

using namespace std::chrono;

struct Time {
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
};

class Scene {
private:
    Device* device;
    
    VkBuffer timeBuffer;
    VkDeviceMemory timeBufferMemory;
    Time time;
    
    void* mappedData;

	Terrain* terrain;
    std::vector<Model*> models;
    std::vector<Blades*> blades;
	std::vector<InstanceBuffer*> instanceData;
	std::vector<InstanceBuffer*> culledInstanceData;


high_resolution_clock::time_point startTime = high_resolution_clock::now();

public:
    Scene() = delete;
    Scene(Device* device);
    ~Scene();

	const Terrain* GetTerrain() const;
    const std::vector<Model*>& GetModels() const;
    const std::vector<Blades*>& GetBlades() const;
	const std::vector<InstanceBuffer*>& GetInstanceBuffer() const;
	const std::vector<InstanceBuffer*>& GetCulledInstanceBuffer() const;
    
	void SetTerrain(Terrain* terrain);
    void AddModel(Model* model);
    void AddBlades(Blades* blades);
	void AddInstanceBuffer(InstanceBuffer* Data);
	void AddCulledInstanceBuffer(InstanceBuffer* Data);
	bool InsertRandomTrees(int numTrees, Device* device, VkCommandPool commandPool);
    VkBuffer GetTimeBuffer() const;

    void UpdateTime();
};

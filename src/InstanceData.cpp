#include "InstanceData.h"
#include "BufferUtils.h"


//Instance Buffer
InstanceBuffer::InstanceBuffer(Device* device, VkCommandPool commandPool, const std::vector<InstanceData> &Data, int numBarkVertices, int numLeafVertices, int numBillboardsVertices)
	:device(device),Data(Data),InstanceCount(Data.size()){

	VkDrawIndexedIndirectCommand indirectCmd[3] = {};
	// Bark
	indirectCmd[0].instanceCount = Data.size();
	indirectCmd[0].firstInstance = 0;
	indirectCmd[0].vertexOffset = 0;
	indirectCmd[0].indexCount = numBarkVertices;
	indirectCmd[0].firstIndex = 0;
	// Leaf
	indirectCmd[1].instanceCount = Data.size();
	indirectCmd[1].firstInstance = 0;
	indirectCmd[1].vertexOffset = 0;
	indirectCmd[1].indexCount = numLeafVertices;
	indirectCmd[1].firstIndex = 0;
	// billboard
	indirectCmd[2].instanceCount = Data.size();
	indirectCmd[2].firstInstance = 0;
	indirectCmd[2].vertexOffset = 0;
	indirectCmd[2].indexCount = numBillboardsVertices;
	indirectCmd[2].firstIndex = 0;
	//leaf LOD0
	//billboard LOD1
	if (Data.size() > 0) {
		BufferUtils::CreateBufferFromData(device, commandPool, this->Data.data(), Data.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, DataBuffer, DataMemory);
		/*BufferUtils::CreateBufferFromData(device, commandPool, this->Data.data(), Data.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, culledDataBuffer[0], culledDataMemory[0]);
		BufferUtils::CreateBufferFromData(device, commandPool, this->Data.data(), Data.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, culledDataBuffer[1], culledDataMemory[1]);*/
		BufferUtils::CreateBuffer(device, Data.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, culledDataBuffer[0], culledDataMemory[0]);
		BufferUtils::CreateBuffer(device, Data.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, culledDataBuffer[1], culledDataMemory[1]);
		BufferUtils::CreateBufferFromData(device, commandPool, &indirectCmd[0], sizeof(VkDrawIndexedIndirectCommand), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, numDataBuffer[0], numDataMemory[0]);
		BufferUtils::CreateBufferFromData(device, commandPool, &indirectCmd[1], sizeof(VkDrawIndexedIndirectCommand), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, numDataBuffer[1], numDataMemory[1]);
		BufferUtils::CreateBufferFromData(device, commandPool, &indirectCmd[2], sizeof(VkDrawIndexedIndirectCommand), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, numDataBuffer[2], numDataMemory[2]);
	}
}
VkBuffer InstanceBuffer::GetInstanceDataBuffer() const{
	return DataBuffer;
}
VkBuffer InstanceBuffer::GetCulledInstanceDataBuffer(int num) const {
	return culledDataBuffer[num];
}
VkBuffer InstanceBuffer::GetNumInstanceDataBuffer(int num) const {
	return numDataBuffer[num];
}
VkDeviceMemory InstanceBuffer::GetInstanceDataMemory() const {
	return DataMemory;
}
VkDeviceMemory InstanceBuffer::GetCulledInstanceDataMemory(int num) const {
	return culledDataMemory[num];
}
VkDeviceMemory InstanceBuffer::GetNumInstanceDataMemory(int num) const {
	return numDataMemory[num];
}
InstanceBuffer::~InstanceBuffer() {
	if (Data.size() > 0) {
		vkDestroyBuffer(device->GetVkDevice(), DataBuffer, nullptr);
		vkDestroyBuffer(device->GetVkDevice(), culledDataBuffer[0], nullptr);
		vkDestroyBuffer(device->GetVkDevice(), culledDataBuffer[1], nullptr);
		vkDestroyBuffer(device->GetVkDevice(), numDataBuffer[0], nullptr);
		vkDestroyBuffer(device->GetVkDevice(), numDataBuffer[1], nullptr);
		vkFreeMemory(device->GetVkDevice(), DataMemory, nullptr);
		vkFreeMemory(device->GetVkDevice(), culledDataMemory[0], nullptr);
		vkFreeMemory(device->GetVkDevice(), culledDataMemory[1], nullptr);
		vkFreeMemory(device->GetVkDevice(), numDataMemory[0], nullptr);
		vkFreeMemory(device->GetVkDevice(), numDataMemory[1], nullptr);
	}
}
int InstanceBuffer::GetInstanceCount() const {
	return InstanceCount;
}

//Fake
FakeInstanceBuffer::FakeInstanceBuffer(Device* device, VkCommandPool commandPool, const std::vector<InstanceData> &Data)
	:device(device), Data(Data), InstanceCount(Data.size()) {

	VkDrawIndexedIndirectCommand indirectCmd = {};
	indirectCmd.instanceCount = Data.size();
	indirectCmd.firstInstance = 0;
	indirectCmd.vertexOffset = 0;
	indirectCmd.indexCount = 6;
	indirectCmd.firstIndex = 0;

	//leaf LOD0
	//billboard LOD1
	if (Data.size() > 0) {
		BufferUtils::CreateBufferFromData(device, commandPool, this->Data.data(), Data.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, FakeDataBuffer, FakeDataMemory);
		/*BufferUtils::CreateBufferFromData(device, commandPool, this->Data.data(), Data.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, culledDataBuffer[0], culledDataMemory[0]);
		BufferUtils::CreateBufferFromData(device, commandPool, this->Data.data(), Data.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, culledDataBuffer[1], culledDataMemory[1]);*/
		BufferUtils::CreateBuffer(device, Data.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, fakeCulledDataBuffer, fakeCulledDataMemory);
		BufferUtils::CreateBufferFromData(device, commandPool, &indirectCmd, sizeof(VkDrawIndexedIndirectCommand), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, fakeNumDataBuffer, fakeNumDataMemory);
	}
}
VkBuffer FakeInstanceBuffer::GetInstanceDataBuffer() const {
	return FakeDataBuffer;
}
VkBuffer FakeInstanceBuffer::GetCulledInstanceDataBuffer() const {
	return fakeCulledDataBuffer;
}
VkBuffer FakeInstanceBuffer::GetNumInstanceDataBuffer() const {
	return fakeNumDataBuffer;
}
VkDeviceMemory FakeInstanceBuffer::GetInstanceDataMemory() const {
	return FakeDataMemory;
}
VkDeviceMemory FakeInstanceBuffer::GetCulledInstanceDataMemory() const {
	return fakeCulledDataMemory;
}
VkDeviceMemory FakeInstanceBuffer::GetNumInstanceDataMemory() const {
	return fakeNumDataMemory;
}
FakeInstanceBuffer::~FakeInstanceBuffer() {
	if (Data.size() > 0) {
		vkDestroyBuffer(device->GetVkDevice(), FakeDataBuffer, nullptr);
		vkDestroyBuffer(device->GetVkDevice(), fakeCulledDataBuffer, nullptr);
		vkDestroyBuffer(device->GetVkDevice(), fakeNumDataBuffer, nullptr);
		vkFreeMemory(device->GetVkDevice(), FakeDataMemory, nullptr);
		vkFreeMemory(device->GetVkDevice(), fakeCulledDataMemory, nullptr);
		vkFreeMemory(device->GetVkDevice(), fakeNumDataMemory, nullptr);
	}
}
int FakeInstanceBuffer::GetInstanceCount() const {
	return InstanceCount;
}

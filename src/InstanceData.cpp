#include "InstanceData.h"
#include "BufferUtils.h"


//Instance Buffer
InstanceBuffer::InstanceBuffer(Device* device, VkCommandPool commandPool, const std::vector<InstanceData> &Data)
	:device(device),Data(Data),InstanceCount(Data.size()){
	if (Data.size() > 0) {
		BufferUtils::CreateBufferFromData(device, commandPool, this->Data.data(), Data.size() * sizeof(InstanceData), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, DataBuffer, DataMemory);
	}
}
VkBuffer InstanceBuffer::GetInstanceDataBuffer() const{
	return DataBuffer;
}
InstanceBuffer::~InstanceBuffer() {
	if (Data.size() > 0) {
		vkDestroyBuffer(device->GetVkDevice(), DataBuffer, nullptr);
		vkFreeMemory(device->GetVkDevice(), DataMemory, nullptr);
	}
}
int InstanceBuffer::GetInstanceCount() const {
	return InstanceCount;
}

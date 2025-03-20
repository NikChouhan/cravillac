#include "PipelineManager.h"

#include <stdexcept>

#include "Log.h"
#include "ResourceManager.h"

Cravillac::PipelineManager::PipelineManager(ResourceManager* resourceManager) : resourceManager(resourceManager), device(resourceManager->getDevice()) {}

VkPipeline PipelineManager::getPipeline(const std::string& pipelineKey) {
    auto it = pipelineCache.find(pipelineKey);
    if (it != pipelineCache.end()) {
        return it->second;
    }
    Log::Error("[PIPELINE] Pipeline not found: {}", pipelineKey);
    throw std::runtime_error("Pipeline not found");
}


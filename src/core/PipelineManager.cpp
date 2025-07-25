#include <pch.h>

#include "PipelineManager.h"
#include <stdexcept>
#include "Log.h"
#include "ResourceManager.h"
#include "Vertex.h"
#include "vk_utils.h"
#include "renderer.h"

namespace CV
{
    PipelineManager::PipelineManager(ResourceManager* resourceManager, const std::shared_ptr<Renderer>& renderer) : _resourceManager(resourceManager), _renderer(renderer) {}

    vk::Pipeline PipelineManager::getPipeline(const std::string& pipelineKey)
    {
        auto it = m_pipelineCache.find(pipelineKey);
        if (it != m_pipelineCache.end()) {
            return it->second;
        }
        printl(Log::LogLevel::Error,"[PIPELINE] Pipeline not found: {}", pipelineKey);
        throw std::runtime_error("Pipeline not found");
    }

    vk::PipelineLayout PipelineManager::getPipelineLayout(const std::string& pipelineLayoutKey)
    {
        if (m_pipelineLayoutCache.contains(pipelineLayoutKey))
        {
            return m_pipelineLayoutCache[pipelineLayoutKey];
        }
        return nullptr;
    }

    PipelineManager::Builder::Builder(PipelineManager* manager) : m_manager(manager)
    {
    }

    PipelineManager::Builder& PipelineManager::Builder::setVertexShader(const std::string& path)
    {
        m_vertShaderPath = path;
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::setMeshShader(const std::string& path)
    {
        m_meshShaderPath = path;
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::setFragmentShader(const std::string& path)
    {
        m_fragShaderPath = path;
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::setPipelineLayout(vk::PipelineLayout pipelineLayout)
    {
        m_pipelineLayout = pipelineLayout;
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::addDescriptorSetLayout(const std::string& key)
    {
        m_descriptorSetLayoutKeys.push_back(key);
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::setDynamicStates(const std::vector<vk::DynamicState>& dynamicStates)
    {
        m_dynamicStates = dynamicStates;
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::setTopology(vk::PrimitiveTopology topology)
    {
        m_topology = topology;
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::setDepthTest(bool enable)
    {
        m_depthTest = enable;
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::setBlendMode(bool enable)
    {
        m_blendMode = enable;
        return *this;
    }

    vk::Pipeline PipelineManager::Builder::build(const std::string& pipelineKey)
    {
        return m_manager->createPipeline(pipelineKey, *this);
    }

    vk::Pipeline PipelineManager::createPipeline(const std::string& pipelineKey, const Builder& builder)
    {
        if (m_pipelineCache.contains(pipelineKey))
        {
            return m_pipelineCache[pipelineKey];
        }

        vk::PipelineLayout pipelineLayout = createPipelineLayout(builder.m_descriptorSetLayoutKeys);

#if MESH_SHADING
        vk::ShaderModule meshShaderModule = _resourceManager->getShaderModule(builder.m_meshShaderPath);
#else
        vk::ShaderModule vertShaderModule = _resourceManager->getShaderModule(builder.m_vertShaderPath);
#endif
        vk::ShaderModule fragShaderModule = _resourceManager->getShaderModule(builder.m_fragShaderPath);

        // Shader stages
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
        fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

#if MESH_SHADING
        vk::PipelineShaderStageCreateInfo meshShaderStageInfo;
        meshShaderStageInfo.stage = vk::ShaderStageFlagBits::eMeshEXT;
        meshShaderStageInfo.module = meshShaderModule;
        meshShaderStageInfo.pName = "main";
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = { meshShaderStageInfo, fragShaderStageInfo };
#else
        vk::PipelineShaderStageCreateInfo vertexShaderStageInfo;
        vertexShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertexShaderStageInfo.module = vertShaderModule;
        vertexShaderStageInfo.pName = "main";
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = { vertexShaderStageInfo, fragShaderStageInfo };

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
#endif

        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(_renderer->_swapChainExtent.width);
        viewport.height = static_cast<float>(_renderer->_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D{ 0, 0 };
        scissor.extent = _renderer->_swapChainExtent;

        vk::PipelineDynamicStateCreateInfo dynamicState;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(builder.m_dynamicStates.size());
        dynamicState.pDynamicStates = builder.m_dynamicStates.data();

        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.viewportCount = 1;
        viewportState.pViewports = nullptr;  // Null for dynamic
        viewportState.scissorCount = 1;
        viewportState.pScissors = nullptr;   // Null for dynamic

        vk::PipelineRasterizationStateCreateInfo rasterizer;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.cullMode = vk::CullModeFlagBits::eNone;
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;
        rasterizer.lineWidth = 1.0f;

        vk::PipelineMultisampleStateCreateInfo multisampling;
        multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.blendEnable = builder.m_blendMode;
        colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
        colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
        colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo colorBlending;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = vk::LogicOp::eCopy;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants = { { 0.0f, 0.0f, 0.0f, 0.0f } };

        vk::PipelineDepthStencilStateCreateInfo depthInfo;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_TRUE;
        depthInfo.depthCompareOp = vk::CompareOp::eLess;
        depthInfo.depthBoundsTestEnable = VK_FALSE;
        depthInfo.stencilTestEnable = VK_FALSE;
        depthInfo.front = vk::StencilOpState{};
        depthInfo.back = vk::StencilOpState{};
        depthInfo.minDepthBounds = 0.0f;
        depthInfo.maxDepthBounds = 1.0f;

        auto device = _resourceManager->getDevice();

        vk::PipelineRenderingCreateInfo pipelineRenderingInfo;
        pipelineRenderingInfo.colorAttachmentCount = 1;
        pipelineRenderingInfo.pColorAttachmentFormats = &_renderer->_swapChainImageFormat;
        pipelineRenderingInfo.depthAttachmentFormat = _renderer->_depthImageFormat;

        vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.pNext = &pipelineRenderingInfo;
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCreateInfo.pStages = shaderStages.data();
#if !MESH_SHADING
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
#endif
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pRasterizationState = &rasterizer;
        pipelineCreateInfo.pMultisampleState = &multisampling;
        pipelineCreateInfo.pDepthStencilState = &depthInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlending;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.layout = pipelineLayout;
        pipelineCreateInfo.renderPass = nullptr;

        try
        {
            auto result = device.createGraphicsPipelines(nullptr, { pipelineCreateInfo });
            vk::Pipeline graphicsPipeline = result.value[0];

            printl(Log::LogLevel::InfoDebug,"[PIPELINE] Pipeline created with key: ", pipelineKey);
            m_pipelineCache[pipelineKey] = graphicsPipeline;
            return graphicsPipeline;
        }
        catch (vk::SystemError& err)
        {
            printl(Log::LogLevel::Error,"[VULKAN] Pipeline creation Failure : {} ", std::string(err.what()));
            throw;
        }
    }

    vk::PipelineLayout PipelineManager::createPipelineLayout(const std::vector<std::string>& descLayoutKeys)
    {
        std::string pipelineLayoutKey{};

        for (const auto& key : descLayoutKeys)
        {
            pipelineLayoutKey += key + ";";
        }

        if (m_pipelineLayoutCache.contains(pipelineLayoutKey))
        {
            return m_pipelineLayoutCache[pipelineLayoutKey];
        }

        std::vector<vk::DescriptorSetLayout> layouts;
        for (const auto& key : descLayoutKeys)
        {
            layouts.push_back(_resourceManager->getDescriptorSetLayout(key));
        }

        vk::PushConstantRange pushConstantRange;
#if MESH_SHADING
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment;
#else
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
#endif
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstants);

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        try
        {
            vk::PipelineLayout pipelineLayout = _renderer->_device.createPipelineLayout(pipelineLayoutInfo);
            printl(Log::LogLevel::Info,"[VULKAN] Pipeline Layout creation Success");
            m_pipelineLayoutCache[pipelineLayoutKey] = pipelineLayout;
            return pipelineLayout;
        }
        catch (vk::SystemError& err)
        {
            printl(Log::LogLevel::Error,"[VULKAN] Pipeline Layout creation Failure:{} ", std::string(err.what()));
            throw;
        }
    }
}
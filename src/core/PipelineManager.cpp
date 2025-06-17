#include "PipelineManager.h"

#include <stdexcept>

#include "Log.h"
#include "ResourceManager.h"
#include "Vertex.h"
#include "vk_utils.h"
#include "renderer.h"

namespace Cravillac
{

    PipelineManager::PipelineManager(ResourceManager* resourceManager, const std::shared_ptr<Renderer>& renderer) : m_resourceManager(resourceManager), m_renderer(renderer)  {}

    VkPipeline PipelineManager::getPipeline(const std::string& pipelineKey)
	{
        auto it = m_pipelineCache.find(pipelineKey);
        if (it != m_pipelineCache.end()) {
            return it->second;
        }
        Log::Error("[PIPELINE] Pipeline not found: {}", pipelineKey);
        throw std::runtime_error("Pipeline not found");
    }

    VkPipelineLayout PipelineManager::getPipelineLayout(const std::string& pipelineLayoutKey)
    {
        if (m_pipelineLayoutCache.contains(pipelineLayoutKey))
        {
            return m_pipelineLayoutCache[pipelineLayoutKey];
        }
        return VK_NULL_HANDLE;
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

    PipelineManager::Builder& PipelineManager::Builder::setPipelineLayout(VkPipelineLayout pipelineLayout)
    {
        m_pipelineLayout = pipelineLayout;
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::addDescriptorSetLayout(const std::string& key)
    {
        m_descriptorSetLayoutKeys.push_back(key);
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::setDynamicStates(const std::vector<VkDynamicState>& dynamicStates)
    {
        m_dynamicStates.resize(dynamicStates.size());
        m_dynamicStates = dynamicStates;
        return *this;
    }

    PipelineManager::Builder& PipelineManager::Builder::setTopology(VkPrimitiveTopology topology)
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

    VkPipeline PipelineManager::Builder::build(const std::string& pipelineKey)
    {
        return m_manager->createPipeline(pipelineKey, *this);
    }

    VkPipeline PipelineManager::createPipeline(const std::string& pipelineKey, const Builder& builder)
    {
        if (m_pipelineCache.contains(pipelineKey))
        {
            return m_pipelineCache[pipelineKey];
        }

        VkPipelineLayout pipelineLayout = createPipelineLayout(builder.m_descriptorSetLayoutKeys);

        /*Log::InfoDebug("[SHADER] Vert buffer Size: ", vertShaderCode.size());
        Log::InfoDebug("[SHADER] Frag buffer Size: ", fragShaderCode.size());*/
#if MESH_SHADING
        VkShaderModule meshShaderModule = m_resourceManager->getShaderModule(builder.m_meshShaderPath);
#else
        VkShaderModule vertShaderModule = m_resourceManager->getShaderModule(builder.m_vertShaderPath);
#endif
        VkShaderModule fragShaderModule = m_resourceManager->getShaderModule(builder.m_fragShaderPath);

        // do stuff
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShaderModule,
            .pName = "main" };
#if MESH_SHADING
        VkPipelineShaderStageCreateInfo meshShaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_MESH_BIT_NV,
            .module = meshShaderModule,
            .pName = "main" };
        VkPipelineShaderStageCreateInfo shaderStages[] = { meshShaderStageInfo, fragShaderStageInfo };
#else
        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShaderModule,
            .pName = "main" };
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
#endif

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_renderer->m_swapChainExtent.width);
        viewport.height = static_cast<float>(m_renderer->m_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_renderer->m_swapChainExtent;

        std::vector<VkDynamicState> dynamicStates = builder.m_dynamicStates;

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = nullptr;  // Null for dynamic
        viewportState.scissorCount = 1;
        viewportState.pScissors = nullptr;   // Null for dynamic

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f;          // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;          // Optional
        multisampling.pSampleMask = nullptr;            // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE;      // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = builder.m_blendMode;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_TRUE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthInfo.depthBoundsTestEnable = VK_FALSE;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;
        depthInfo.stencilTestEnable = VK_FALSE;
        depthInfo.front = {};
        depthInfo.back = {};


        auto device = m_resourceManager->getDevice();

        VkPipelineRenderingCreateInfo pipelineRenderingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &m_renderer->m_swapChainImageFormat,
            .depthAttachmentFormat = m_renderer->m_depthImageFormat };

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &pipelineRenderingInfo,
            .stageCount = 2,
            .pStages = shaderStages,
            //remember its !MESH_SHADING
#if !MESH_SHADING
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
#endif
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthInfo,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout,
            .renderPass = VK_NULL_HANDLE,
        };

        VkPipeline graphicsPipeline;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Pipeline creation Failure");
        }
        else
        {
            Log::InfoDebug("[PIPELINE] Pipeline created with key: ", pipelineKey);
        }
        assert(graphicsPipeline);
        m_pipelineCache[pipelineKey] = graphicsPipeline;
        return graphicsPipeline;
    }

    VkPipelineLayout PipelineManager::createPipelineLayout(const std::vector<std::string>& descLayoutKeys)
    {
        std::string pipelineLayoutKey{};

        for (const auto& key:descLayoutKeys)
        {
            pipelineLayoutKey += key + ";";
        }

        if (m_pipelineCache.contains(pipelineLayoutKey))
        {
            return m_pipelineLayoutCache[pipelineLayoutKey];
        }

        std::vector<VkDescriptorSetLayout> layouts;
        for (const auto& key : descLayoutKeys)
        {
            layouts.push_back(m_resourceManager->getDescriptorSetLayout(key));
        }
        VkPushConstantRange pushConstantRange{};
#if MESH_SHADING
        pushConstantRange.stageFlags = VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT;
#else
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
#endif
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstants);  // Size for material index

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        VkPipelineLayout pipelineLayout;
        if (vkCreatePipelineLayout(m_renderer->m_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            Log::Error("[VULKAN] Pipeline Layout creation Failure");
        }
        else
            Log::Info("[VULKAN] Pipeline Layout creation Success");
        m_pipelineLayoutCache[pipelineLayoutKey] = pipelineLayout;

        return pipelineLayout;
    }
}


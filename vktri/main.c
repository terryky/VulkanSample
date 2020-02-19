/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2019 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "vk_tools.h"
#include "vk_init.h"
#include "vk_render.h"


static vk_buffer_t  s_vtx_buf;
static VkPipeline   s_pipeline;

static float vertices[] = 
{ /* ( x,     y)      ( r,    g,    b) */
    -0.5f,  0.5f,     1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
     0.5f,  0.5f,     0.0f, 0.0f, 1.0f
};

static int
init_pipeline (vk_t *vk)
{
    /* ---------------------------------------------------- *
     *  Shader Stage
     * ---------------------------------------------------- */
    VkShaderModule shaders[2] = {0};
    vk_load_shader_module (vk, "shaders/shader.vert.spv", &shaders[0]);
    vk_load_shader_module (vk, "shaders/shader.frag.spv", &shaders[1]);

    VkPipelineShaderStageCreateInfo shaderStages[2] = {0};
    shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = shaders[0];
    shaderStages[0].pName  = "main";
    shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = shaders[1];
    shaderStages[1].pName  = "main";


    /* ---------------------------------------------------- *
     *  Vertex Input Attributes
     * ---------------------------------------------------- */
    VkVertexInputBindingDescription inputBinding = {0};
    inputBinding.binding    = 0;
    inputBinding.stride     = (2 + 3) * sizeof (float);
    inputBinding.inputRate  = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription inputAttribs[2] = {0};
    inputAttribs[0].location = 0;
    inputAttribs[0].binding  = 0;
    inputAttribs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    inputAttribs[0].offset   = 0;
    inputAttribs[1].location = 1;
    inputAttribs[1].binding  = 0;
    inputAttribs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    inputAttribs[1].offset   = 2 * sizeof (float);

    VkPipelineVertexInputStateCreateInfo vertexInputCI = {0};
    vertexInputCI.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCI.vertexBindingDescriptionCount   = 1;
    vertexInputCI.pVertexBindingDescriptions      = &inputBinding;
    vertexInputCI.vertexAttributeDescriptionCount = ARRAY_LENGTH (inputAttribs);
    vertexInputCI.pVertexAttributeDescriptions    = inputAttribs;


    /* ---------------------------------------------------- *
     *  Primitive Type
     * ---------------------------------------------------- */
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI = {0};
    inputAssemblyCI.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;


    /* ---------------------------------------------------- *
     *  Viewport, Scissor
     * ---------------------------------------------------- */
    VkViewport viewport = {0};
    viewport.x          = 0.0f;
    viewport.y          = vk->swapchain_extent.height;
    viewport.width      = vk->swapchain_extent.width;
    viewport.height     = -1.0f * vk->swapchain_extent.height;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent   = vk->swapchain_extent;

    VkPipelineViewportStateCreateInfo viewportCI = {0};
    viewportCI.sType            = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCI.viewportCount    = 1;
    viewportCI.pViewports       = &viewport;
    viewportCI.scissorCount     = 1;
    viewportCI.pScissors        = &scissor;


    /* ---------------------------------------------------- *
     *  Rasterizer
     * ---------------------------------------------------- */
    VkPipelineRasterizationStateCreateInfo rasterizerCI = {0};
    rasterizerCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCI.polygonMode    = VK_POLYGON_MODE_FILL;
    rasterizerCI.cullMode       = VK_CULL_MODE_NONE;
    rasterizerCI.frontFace      = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerCI.lineWidth      = 1.0f;


    /* ---------------------------------------------------- *
     *  Multi Sample
     * ---------------------------------------------------- */
    VkPipelineMultisampleStateCreateInfo multisampleCI = {0};
    multisampleCI.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCI.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;


    /* ---------------------------------------------------- *
     *  Depth test, Stencil test
     * ---------------------------------------------------- */
    VkPipelineDepthStencilStateCreateInfo depthStencilCI = {0};
    depthStencilCI.sType                = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCI.depthTestEnable      = VK_TRUE;
    depthStencilCI.depthCompareOp       = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilCI.depthWriteEnable     = VK_TRUE;
    depthStencilCI.stencilTestEnable    = VK_FALSE;


    /* ---------------------------------------------------- *
     *  Blend
     * ---------------------------------------------------- */
    const VkColorComponentFlagBits colorWriteAll = 
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendAttachmentState blendAttachment = {0};
    blendAttachment.blendEnable         = VK_TRUE;
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    blendAttachment.colorWriteMask      = colorWriteAll;

    VkPipelineColorBlendStateCreateInfo cbCI = {0};
    cbCI.sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbCI.attachmentCount    = 1;
    cbCI.pAttachments       = &blendAttachment;


    /* ---------------------------------------------------- *
     *  Pipeline Layout
     * ---------------------------------------------------- */
    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutCI = {0};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vkCreatePipelineLayout (vk->dev, &pipelineLayoutCI, NULL, &pipelineLayout);



    /* create pipeline */
    VkGraphicsPipelineCreateInfo ci = {0};
    ci.sType                = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.stageCount           = ARRAY_LENGTH (shaderStages);
    ci.pStages              = shaderStages;
    ci.pInputAssemblyState  = &inputAssemblyCI;
    ci.pVertexInputState    = &vertexInputCI;
    ci.pRasterizationState  = &rasterizerCI;
    ci.pDepthStencilState   = &depthStencilCI;
    ci.pMultisampleState    = &multisampleCI;
    ci.pViewportState       = &viewportCI;
    ci.pColorBlendState     = &cbCI;
    ci.renderPass           = vk->render_pass;
    ci.layout               = pipelineLayout;

    VK_CHECK (vkCreateGraphicsPipelines (vk->dev, VK_NULL_HANDLE, 1, &ci, NULL, &s_pipeline));

    for (uint32_t i = 0; i < ARRAY_LENGTH (shaderStages); i ++)
    {
        vkDestroyShaderModule (vk->dev, shaderStages[i].module, NULL);
    }

    return 0;
}


static int
init_scene (vk_t *vk)
{
    VkBufferUsageFlags    usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkMemoryPropertyFlags mflag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vk_create_buffer (vk, sizeof(vertices), usage, mflag, vertices, &s_vtx_buf);

    init_pipeline (vk);

    return 0;
}


static void
cb_make_command (VkCommandBuffer command, void *usr_data)
{
    UNUSED (usr_data);

    vkCmdBindPipeline (command, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers (command, 0, 1, &s_vtx_buf.buf, &offset);

    vkCmdDraw (command, 3, 1, 0, 0);
}


int
main (int argc, char *argv[])
{
    vk_t *vk = vk_init (960, 540);

    init_scene (vk);

    while (1)
    {
        vk_render (vk, 0, cb_make_command, vk);
    }
}

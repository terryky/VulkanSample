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
#include "util_matrix.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

static VkDescriptorSetLayout    s_descriptorSetLayout;
static VkDescriptorPool         s_descriptorPool;
static VkDescriptorSet          s_descriptorSet[3];
static VkPipelineLayout         s_pipelineLayout;

static vk_buffer_t              s_vtx_buf;
static vk_buffer_t              s_nrm_buf;
static vk_buffer_t              s_uv_buf;
static vk_texture_t             s_texture;
static VkPipeline               s_pipeline;

static vk_buffer_t              m_ubo[3];

static float s_matPrj[16];

static float s_vtx[] =
{
    -1.0f, 1.0f,  1.0f,
    -1.0f,-1.0f,  1.0f,
     1.0f, 1.0f,  1.0f,
     1.0f,-1.0f,  1.0f,

     1.0f, 1.0f, -1.0f,
     1.0f,-1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f,-1.0f, -1.0f,

     1.0f,  1.0f, 1.0f,
     1.0f, -1.0f, 1.0f,
     1.0f,  1.0f,-1.0f,
     1.0f, -1.0f,-1.0f,

    -1.0f,  1.0f,-1.0f,
    -1.0f, -1.0f,-1.0f,
    -1.0f,  1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
    
     1.0f,  1.0f, 1.0f,
     1.0f,  1.0f,-1.0f,
    -1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f,-1.0f,
    
    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f,-1.0f,
     1.0f, -1.0f, 1.0f,
     1.0f, -1.0f,-1.0f,
};

static float s_nrm[] =
{
     0.0f,  0.0f,  1.0f,
     0.0f,  0.0f, -1.0f,
     1.0f,  0.0f,  0.0f,
    -1.0f,  0.0f,  0.0f,
     0.0f,  1.0f,  0.0f,
     0.0f, -1.0f,  0.0f,
};

static float s_uv [] =
{
#if 0
     0.0f, 1.0f,
     0.0f, 0.0f,
     1.0f, 1.0f,
     1.0f, 0.0f,
#else
     0.0f, 0.0f,
     0.0f, 1.0f,
     1.0f, 0.0f,
     1.0f, 1.0f,
#endif
};


typedef struct ShaderParameters
{
    float matPMV   [16];
    float matMV    [16];
    float matMVI3x3[ 9];
} ShaderParameters;


static void 
create_descriptor_set_layout (vk_t *vk)
{
    VkDescriptorSetLayoutBinding bindings[2] = {0};

    /* binding for UBO */
    bindings[0].binding         = 0;
    bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].descriptorCount = 1;

    /* binding for Sampler */
    bindings[1].binding         = 1;
    bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].descriptorCount = 1;

    VkDescriptorSetLayoutCreateInfo ci = {0};
    ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.bindingCount = ARRAY_LENGTH (bindings);
    ci.pBindings    = bindings;
    VK_CHECK (vkCreateDescriptorSetLayout (vk->dev, &ci, NULL, &s_descriptorSetLayout));
}


static void
create_descriptor_pool (vk_t *vk)
{
    VkDescriptorPoolSize descPoolSize[2];
    descPoolSize[0].descriptorCount = 1;
    descPoolSize[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descPoolSize[1].descriptorCount = 1;
    descPoolSize[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    VkDescriptorPoolCreateInfo ci = {0};
    ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci.maxSets       = 3;
    ci.poolSizeCount = ARRAY_LENGTH (descPoolSize);
    ci.pPoolSizes    = descPoolSize;

    VK_CHECK (vkCreateDescriptorPool (vk->dev, &ci, NULL, &s_descriptorPool));
}


static void
init_descriptor_set (vk_t *vk)
{
    VkDescriptorSetLayout layouts[3];

    for (int i = 0; i < 3; i++)
    {
        layouts[i] = s_descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo ai = {0};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = s_descriptorPool;
    ai.descriptorSetCount = 3;
    ai.pSetLayouts        = layouts;

    VK_CHECK (vkAllocateDescriptorSets (vk->dev, &ai, s_descriptorSet));

    /* write to DescriptorSet */
    for (int i = 0; i < 3; i++)
    {
        VkWriteDescriptorSet writeSets[2] = {0};

        /* UBO */
        VkDescriptorBufferInfo descUBO = {0};
        descUBO.buffer      = m_ubo[i].buf;
        descUBO.offset      = 0;
        descUBO.range       = VK_WHOLE_SIZE;

        writeSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSets[0].dstBinding      = 0;
        writeSets[0].descriptorCount = 1;
        writeSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeSets[0].pBufferInfo     = &descUBO;
        writeSets[0].dstSet          = s_descriptorSet[i];

        /* texture sampler */
        VkDescriptorImageInfo descImage = {0};
        descImage.imageView   = s_texture.view;
        descImage.sampler     = s_texture.sampler;
        descImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        writeSets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSets[1].dstBinding      = 1;
        writeSets[1].descriptorCount = 1;
        writeSets[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSets[1].pImageInfo      = &descImage;
        writeSets[1].dstSet          = s_descriptorSet[i];

        vkUpdateDescriptorSets (vk->dev, ARRAY_LENGTH(writeSets), writeSets, 0, NULL);
    }
}


/*
 *
 * +------------------+-------------------------------------+-----------------------------------------+
 * | oldLayout        |   VK_IMAGE_LAYOUT_UNDEFINED         | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL    |
 * +------------------+-------------------------------------+-----------------------------------------+
 * | imb.srcAccessMask|               0                     | VK_ACCESS_TRANSFER_WRITE_BIT            |
 * | srcSgage         |   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT| VK_PIPELINE_STAGE_TRANSFER_BIT          |
 * +------------------+-------------------------------------+-----------------------------------------+
 *
 * +------------------+-------------------------------------+-----------------------------------------+
 * |newLayout         | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL| VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL|
 * +------------------+-------------------------------------+-----------------------------------------+
 * | imb.dstAccessMask|   VK_ACCESS_TRANSFER_WRITE_BIT      |  VK_ACCESS_SHADER_READ_BIT              |
 * | dstSgage         |   VK_PIPELINE_STAGE_TRANSFER_BIT    |  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT  |
 * +------------------+-------------------------------------+-----------------------------------------+
 *
 */
void 
setImageMemoryBarrier (VkCommandBuffer command, VkImage image,
                       VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier imb = {0};
    imb.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imb.oldLayout                       = oldLayout;
    imb.newLayout                       = newLayout;
    imb.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    imb.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    imb.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imb.subresourceRange.baseMipLevel   = 0;
    imb.subresourceRange.levelCount     = 1;
    imb.subresourceRange.baseArrayLayer = 0;
    imb.subresourceRange.layerCount     = 1;
    imb.image                           = image;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    switch (oldLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        imb.srcAccessMask = 0;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage          = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    default:
        break;
    }

    switch (newLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        imb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        dstStage          = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstStage          = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstStage          = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
    default:
        break;
    }

    vkCmdPipelineBarrier (command, srcStage, dstStage, 0, 0,  // memoryBarrierCount
                            NULL, 0,  // bufferMemoryBarrierCount
                            NULL, 1,  // imageMemoryBarrierCount
                            &imb);
}


static int
create_texture (vk_t *vk, const char* fileName, vk_texture_t *ptexture)
{
    int width, height, channels;
    uint8_t     *pImage = stbi_load(fileName, &width, &height, &channels, 0);
    VkFormat    format = VK_FORMAT_R8G8B8A8_UNORM;
    vk_texture_t texture = {0};


    /* create VkImage */
    vk_create_texture (vk, width, height, format, &texture);


    /* create staging buffer */
    vk_buffer_t staging_buf;
    uint32_t imageSize = width * height * sizeof(uint32_t);
    VkBufferUsageFlags    usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags mflag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vk_create_buffer (vk, imageSize, usage, mflag, pImage, &staging_buf);


    /* create Command buffer */
    VkCommandBuffer command;
    VkCommandBufferAllocateInfo ai = {0};
    ai.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandBufferCount   = 1;
    ai.commandPool          = vk->cmd_pool;
    ai.level                = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_CHECK (vkAllocateCommandBuffers (vk->dev, &ai, &command));


    /* Copy from (staging buffer) ==> (texture buffer) */
    VkCommandBufferBeginInfo commandBI = {0};
    commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK (vkBeginCommandBuffer (command, &commandBI));

    setImageMemoryBarrier (command, texture.img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copyRegion = {0};
    copyRegion.imageExtent.width               = width;
    copyRegion.imageExtent.height              = height;
    copyRegion.imageExtent.depth               = 1;
    copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel       = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount     = 1;

    vkCmdCopyBufferToImage (command, staging_buf.buf, texture.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    setImageMemoryBarrier (command, texture.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VK_CHECK (vkEndCommandBuffer (command));


    /* Submit command */
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &command;
    VK_CHECK (vkQueueSubmit (vk->devq, 1, &submitInfo, VK_NULL_HANDLE));

    /* wait copy done */
    VK_CHECK (vkDeviceWaitIdle (vk->dev));

    vkFreeCommandBuffers (vk->dev, vk->cmd_pool, 1, &command);


    /* clean up staging buffer */
    vkFreeMemory (vk->dev, staging_buf.mem, NULL);
    vkDestroyBuffer (vk->dev, staging_buf.buf, NULL);

    stbi_image_free (pImage);

    *ptexture = texture;
    return 0;
}


static int
create_ubo (vk_t *vk)
{
    VkMemoryPropertyFlags uboFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for (uint32_t i = 0; i < 3; i ++)
    {
        vk_create_buffer (vk, sizeof(ShaderParameters), 
                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags,
                          NULL, &m_ubo[i]);
    }
    return 0;
}



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
    VkVertexInputBindingDescription inputBinding[3] = {0};
    inputBinding[0].binding    = 0;                         /* vbo[0] vtx */
    inputBinding[0].stride     = 3 * sizeof (float);
    inputBinding[0].inputRate  = VK_VERTEX_INPUT_RATE_VERTEX;
    inputBinding[1].binding    = 1;                         /* vbo[1] nrm */
    inputBinding[1].stride     = 0;
    inputBinding[1].inputRate  = VK_VERTEX_INPUT_RATE_VERTEX;
    inputBinding[2].binding    = 2;                         /* vbo[2] uv */
    inputBinding[2].stride     = 2 * sizeof (float);
    inputBinding[2].inputRate  = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription inputAttribs[3] = {0};
    inputAttribs[0].location = 0;                           /* loc[0] vtx */
    inputAttribs[0].binding  = 0;
    inputAttribs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    inputAttribs[0].offset   = 0;
    inputAttribs[1].location = 1;                           /* loc[1] nrm */
    inputAttribs[1].binding  = 1;
    inputAttribs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    inputAttribs[1].offset   = 0;
    inputAttribs[2].location = 2;                           /* loc[2] uv */
    inputAttribs[2].binding  = 2;
    inputAttribs[2].format   = VK_FORMAT_R32G32B32_SFLOAT ;
    inputAttribs[2].offset   = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputCI = {0};
    vertexInputCI.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCI.vertexBindingDescriptionCount   = ARRAY_LENGTH (inputBinding);
    vertexInputCI.pVertexBindingDescriptions      = inputBinding;
    vertexInputCI.vertexAttributeDescriptionCount = ARRAY_LENGTH (inputAttribs);
    vertexInputCI.pVertexAttributeDescriptions    = inputAttribs;


    /* ---------------------------------------------------- *
     *  Primitive Type
     * ---------------------------------------------------- */
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI = {0};
    inputAssemblyCI.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ;


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
    VkPipelineLayoutCreateInfo pipelineLayoutCI = {0};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = 1;
    pipelineLayoutCI.pSetLayouts = &s_descriptorSetLayout;
    VK_CHECK (vkCreatePipelineLayout (vk->dev, &pipelineLayoutCI, NULL, &s_pipelineLayout));



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
    ci.layout               = s_pipelineLayout;

    VK_CHECK (vkCreateGraphicsPipelines (vk->dev, VK_NULL_HANDLE, 1, &ci, NULL, &s_pipeline));

    for (uint32_t i = 0; i < ARRAY_LENGTH (shaderStages); i ++)
    {
        vkDestroyShaderModule (vk->dev, shaderStages[i].module, NULL);
    }

    return 0;
}


static int
init_scene (vk_t *vk, int win_w, int win_h)
{
    VkBufferUsageFlags    usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkMemoryPropertyFlags mflag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vk_create_buffer (vk, sizeof(s_vtx), usage, mflag, s_vtx, &s_vtx_buf);
    vk_create_buffer (vk, sizeof(s_nrm), usage, mflag, s_nrm, &s_nrm_buf);
    vk_create_buffer (vk, sizeof(s_uv),  usage, mflag, s_uv,  &s_uv_buf);

    create_ubo (vk);
    create_texture (vk, "texture.tga", &s_texture);

    create_descriptor_set_layout (vk);
    create_descriptor_pool (vk);

    init_descriptor_set (vk);

    init_pipeline (vk);

    matrix_proj_perspective (s_matPrj, 72.0f, (float)win_w/(float)win_h, 0.1f, 1000.f);

    return 0;
}

static void
update_scene (vk_t *vk)
{
    float matMV[16], matPMV[16], matMVI4x4[16], matMVI3x3[9];
    static int count = 0;

    count ++;
    matrix_identity (matMV);
    matrix_translate (matMV, 0.0f, 0.0f, -3.5f);
    matrix_rotate (matMV, 30.0f * sinf (count*0.01f), 1.0f, 0.0f, 0.0f);
    matrix_rotate (matMV, count*1.0f, 0.0f, 1.0f, 0.0f);

    matrix_copy (matMVI4x4, matMV);
    matrix_invert   (matMVI4x4);
    matrix_transpose(matMVI4x4);
    matMVI3x3[0] = matMVI4x4[0];
    matMVI3x3[1] = matMVI4x4[1];
    matMVI3x3[2] = matMVI4x4[2];
    matMVI3x3[3] = matMVI4x4[4];
    matMVI3x3[4] = matMVI4x4[5];
    matMVI3x3[5] = matMVI4x4[6];
    matMVI3x3[6] = matMVI4x4[8];
    matMVI3x3[7] = matMVI4x4[9];
    matMVI3x3[8] = matMVI4x4[10];

    matrix_mult (matPMV, s_matPrj, matMV);

    /* update Uniform Buffer */
    ShaderParameters shaderParam = {0};
    memcpy (shaderParam.matPMV,    matPMV,    sizeof (matPMV   ));
    memcpy (shaderParam.matMV,     matMV,     sizeof (matMV    ));
    memcpy (shaderParam.matMVI3x3, matMVI3x3, sizeof (matMVI3x3));

    VkDeviceMemory mem = m_ubo[vk->image_index].mem;
    void *p;
    vkMapMemory (vk->dev, mem, 0, VK_WHOLE_SIZE, 0, &p);
    memcpy (p, &shaderParam, sizeof(shaderParam));
    vkUnmapMemory (vk->dev, mem);
}

static void
cb_make_command (VkCommandBuffer command, void *usr_data)
{
    vk_t *vk = (vk_t *)usr_data;

    update_scene (vk);

    vkCmdBindPipeline (command, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipeline);

    vkCmdBindDescriptorSets (command, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipelineLayout, 0, 1, 
                             &s_descriptorSet[vk->image_index], 0, NULL);

    for (uint32_t i = 0; i < 6; i ++)
    {
        VkBuffer     buffer[3];
        VkDeviceSize offset[3];

        buffer[0] = s_vtx_buf.buf;
        offset[0] = 4 * 3 * i * sizeof(float);
        buffer[1] = s_nrm_buf.buf;
        offset[1] = 1 * 3 * i * sizeof(float);;
        buffer[2] = s_uv_buf.buf;
        offset[2] = 0;

        vkCmdBindVertexBuffers (command, 0, 3, buffer,  offset);

        vkCmdDraw (command, 4, 1, 0, 0);
    }
}


int
main (int argc, char *argv[])
{
    vk_t *vk = vk_init (960, 540);

    init_scene (vk, 960, 540);

    while (1)
    {
        vk_render (vk, 0, cb_make_command, vk);
    }
}

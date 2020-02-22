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

static VkPipeline               s_pipeline;

static vk_buffer_t              s_vtx_buf;
static vk_buffer_t              s_nrm_buf;
static vk_buffer_t              s_uv_buf;
static vk_texture_t             s_texture;

static vk_buffer_t              s_ubo_vs[3];

static float s_matPrj[16];

typedef struct _ubo_vs_t
{
    float matPMV[16];
    float matMV [16];
    float matMVI[16];   /* mat3 requires same alignment as mat4 */
} ubo_vs_t;


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




/*
 *  +--------------------------------------------+
 *  |               Descriptor Set               |
 *  +--------------------------+                 |
 *  |  Descriptor Set Layout   |                 |
 *  +---+----+-----------------+-----------------+
 *  |set|bind|  type   | stage |    buffer       |
 *  +---+----+-----------------+-----------------+
 *  | 0 |  0 | UNIFORM | VERT  |s_ubo_vs         |
 *  | 0 |  1 | SAMPLER | FRAG  |s_texture        |
 *  +---+----+-----------------+-----------------+
 */
static void 
create_descriptor_set_layout (vk_t *vk)
{
    VkDescriptorSetLayoutBinding bindings[2] = {0};

    /* binding for Vertex Shader UBO */
    bindings[0].binding         = 0;
    bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].descriptorCount = 1;

    /* binding for Fragment Shader Sampler */
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

        /* Vertex Shader UBO */
        VkDescriptorBufferInfo desc_ubo_vs = {0};
        desc_ubo_vs.buffer           = s_ubo_vs[i].buf;
        desc_ubo_vs.offset           = 0;
        desc_ubo_vs.range            = VK_WHOLE_SIZE;

        writeSets[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSets[0].dstBinding      = 0;
        writeSets[0].descriptorCount = 1;
        writeSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeSets[0].pBufferInfo     = &desc_ubo_vs;
        writeSets[0].dstSet          = s_descriptorSet[i];

        /* Fragment Shader Sampler */
        VkDescriptorImageInfo desc_img_fs = {0};
        desc_img_fs.imageView        = s_texture.view;
        desc_img_fs.sampler          = s_texture.sampler;
        desc_img_fs.imageLayout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        writeSets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSets[1].dstBinding      = 1;
        writeSets[1].descriptorCount = 1;
        writeSets[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSets[1].pImageInfo      = &desc_img_fs;
        writeSets[1].dstSet          = s_descriptorSet[i];

        vkUpdateDescriptorSets (vk->dev, ARRAY_LENGTH(writeSets), writeSets, 0, NULL);
    }
}


static int
create_texture (vk_t *vk, const char* fileName, vk_texture_t *ptexture)
{
    int width, height, channels;
    uint8_t     *pImage = stbi_load(fileName, &width, &height, &channels, 0);
    VkFormat    format = VK_FORMAT_R8G8B8A8_UNORM;

    /* create texture and upload image data. */
    vk_create_texture (vk, width, height, format, pImage, ptexture);

    stbi_image_free (pImage);

    return 0;
}


static int
create_ubo (vk_t *vk)
{
    VkMemoryPropertyFlags uboFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for (uint32_t i = 0; i < 3; i ++)
    {
        vk_create_buffer (vk, sizeof(ubo_vs_t), 
                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags,
                          NULL, &s_ubo_vs[i]);
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
     *  +---+----+-------------------+--------+------------------+--------+
     *  |loc|bind|      stride       |  rate  |      format      | offset |
     *  +---+----+-------------------+--------+------------------+--------+
     *  | 0 |  0 | 3 * sizeof(float) | VERTEX | R32G32B32_SFLOAT |   0    |
     *  | 1 |  1 | 0                 | VERTEX | R32G32B32_SFLOAT |   0    |
     *  | 2 |  2 | 2 * sizeof(float) | VERTEX | R32G32B32_SFLOAT |   0    |
     *  +---+----+-------------------+--------+------------------+--------+
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


    /* Primitive Type */
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI = {0};
    vk_get_default_input_assembly_state (vk, &inputAssemblyCI, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

    /* Viewport, Scissor */
    VkPipelineViewportStateCreateInfo viewportCI = {0};
    vk_get_default_viewport_state (vk, &viewportCI);

    /* Rasterizer */
    VkPipelineRasterizationStateCreateInfo rasterizerCI = {0};
    vk_get_default_rasterizer_state (vk, &rasterizerCI);

    /* Multi Sample */
    VkPipelineMultisampleStateCreateInfo multisampleCI = {0};
    vk_get_default_multisample_state (vk, &multisampleCI);

    /* Depth test, Stencil test */
    VkPipelineDepthStencilStateCreateInfo depthStencilCI = {0};
    int depth_en   = 1;
    int stencil_en = 0;
    vk_get_default_depth_stencil_state (vk, &depthStencilCI, depth_en, stencil_en);

    /* Blend */
    VkPipelineColorBlendStateCreateInfo cblendCI = {0};
    int blend_en = 0;
    vk_get_default_blend_state (vk, &cblendCI, blend_en);


    /* ---------------------------------------------------- *
     *  Pipeline Layout
     * ---------------------------------------------------- */
    VkPipelineLayoutCreateInfo pipelineLayoutCI = {0};
    pipelineLayoutCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = 1;
    pipelineLayoutCI.pSetLayouts    = &s_descriptorSetLayout;
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
    ci.pColorBlendState     = &cblendCI;
    ci.renderPass           = vk->render_pass;
    ci.layout               = s_pipelineLayout;

    VK_CHECK (vkCreateGraphicsPipelines (vk->dev, VK_NULL_HANDLE, 1, &ci, NULL, &s_pipeline));


    /* --------------------- *
     *  clean up resources
     * --------------------- */
    for (uint32_t i = 0; i < ARRAY_LENGTH (shaderStages); i ++)
    {
        vkDestroyShaderModule (vk->dev, shaderStages[i].module, NULL);
    }

    vk_destroy_default_viewport_state (vk, &viewportCI);
    vk_destroy_default_rasterizer_state (vk, &rasterizerCI);
    vk_destroy_default_multisample_state (vk, &multisampleCI);
    vk_destroy_default_depth_stencil_state (vk, &depthStencilCI);
    vk_destroy_default_blend_state (vk, &cblendCI);

    return 0;
}


static int
init_texcube (vk_t *vk, int win_w, int win_h)
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
draw_texcube (vk_t *vk, int count)
{
    float matMV[16], matPMV[16], matMVI[16];

    matrix_identity (matMV);
    matrix_translate (matMV, 0.0f, 0.0f, -3.5f);
    matrix_rotate (matMV, 30.0f * sinf (count*0.01f), 1.0f, 0.0f, 0.0f);
    matrix_rotate (matMV, count*1.0f, 0.0f, 1.0f, 0.0f);

    matrix_copy (matMVI, matMV);
    matrix_invert   (matMVI);
    matrix_transpose(matMVI);

    matrix_mult (matPMV, s_matPrj, matMV);

    /* ----------------------------------------------------------------------- *
     *    Vulkan Render
     * ----------------------------------------------------------------------- */
    VkCommandBuffer command = vk->cmd_bufs[vk->image_index];
    vkCmdBindPipeline (command, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipeline);

    vkCmdBindDescriptorSets (command, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipelineLayout, 0, 1, 
                             &s_descriptorSet[vk->image_index], 0, NULL);

    /* ------------------------- *
     *  update UBO
     * ------------------------- */
    ubo_vs_t ubo_vs = {0};
    memcpy (ubo_vs.matPMV, matPMV, sizeof (matPMV));
    memcpy (ubo_vs.matMV,  matMV,  sizeof (matMV ));
    memcpy (ubo_vs.matMVI, matMVI, sizeof (matMVI));

    /* Upload UBO */
    vk_devmemcpy (vk, s_ubo_vs[vk->image_index].mem, &ubo_vs, sizeof(ubo_vs));


    /* ------------------------------------ *
     *  Bind Vertex buffer and Draw
     * ------------------------------------ */
    for (uint32_t i = 0; i < 6; i ++)
    {
        VkBuffer     buffer[3] = {0};
        VkDeviceSize offset[3] = {0};

        buffer[0] = s_vtx_buf.buf;
        offset[0] = 4 * 3 * i * sizeof(float);
        buffer[1] = s_nrm_buf.buf;
        offset[1] = 1 * 3 * i * sizeof(float);;
        buffer[2] = s_uv_buf.buf;
        offset[2] = 0;

        vkCmdBindVertexBuffers (command, 0, 3, buffer, offset);

        vkCmdDraw (command, 4, 1, 0, 0);
    }
}


static void
cb_make_command (VkCommandBuffer command, void *usr_data)
{
    vk_t *vk = (vk_t *)usr_data;
    static int count = 0;

    draw_texcube (vk, count);

    count ++;
}


int
main (int argc, char *argv[])
{
    vk_t *vk = vk_init (960, 540);

    init_texcube (vk, 960, 540);

    while (1)
    {
        vk_render (vk, 0, cb_make_command, vk);
    }
}

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
#include "util_pmeter.h"



static int    s_laptime_idx[10] = {0};
static int    s_laptime_num[10] = {0};
static float  s_laptime_stack[10][PMETER_MAX_LAP_NUM];
static double s_last_laptime[10] = {0};

double
pmeter_get_time_ms ()
{
    struct timespec tv;
    clock_gettime (CLOCK_MONOTONIC, &tv);
    return  (tv.tv_sec*1000 + (float)tv.tv_nsec/1000000.0);
} 

void
pmeter_reset_lap (int id)
{
    s_laptime_idx[id] = 0;
}

void
pmeter_set_lap (int id)
{
    if (s_laptime_idx[id] >= PMETER_MAX_LAP_NUM)
        return;

    double laptime = pmeter_get_time_ms ();
    s_laptime_stack[id][s_laptime_idx[id]] = laptime - s_last_laptime[id];
    s_laptime_idx[id] ++;
    s_laptime_num[id] ++;

    s_last_laptime[id] = laptime;
}

static void
pmeter_get_laptime (int id, int *num, float **laptime)
{
    *num = s_laptime_num[id];
    *laptime = s_laptime_stack[id];

    s_laptime_num[id] = 0;
}



#define MAX_INSTANCE_NUM 128

static VkDescriptorSetLayout    s_descriptorSetLayout;
static VkDescriptorPool         s_descriptorPool;
static VkDescriptorSet          s_descriptorSet[3];
static VkPipelineLayout         s_pipelineLayout;

static VkPipeline               s_pipeline;

static vk_buffer_t              s_vtx_axis_buf;
static vk_buffer_t              s_vtx_cursor_buf;
static vk_buffer_t              s_vtx_graph_buf;

static vk_buffer_t              s_ubo_vs[3];
static vk_buffer_t              s_ubo_vs_instance[3];

typedef struct _ubo_vs_t
{
    float PrjMul[4];
    float PrjAdd[4];
} ubo_vs_t;

typedef struct _ubo_vs_instance_t
{
    float ofst_vtx[4];
    float color [4];
} ubo_vs_instance_t;

static ubo_vs_instance_t        s_vs_instance[MAX_INSTANCE_NUM];


#define PMETER_DPY_NUM  10
#define PMETER_NUM      4
#define PMETER_DATA_NUM 1000

static int      s_wndW, s_wndH;
static int      s_data_num;
static int      s_pm_idx[PMETER_DPY_NUM];
static float    s_vertPM[PMETER_DPY_NUM][PMETER_NUM][PMETER_DATA_NUM*2];

static float s_vtx_axis[] = 
{
    0.0f, 0.0f,
    0.0f, (float)PMETER_DATA_NUM
};

static float s_vtx_cursor[] = 
{
      0.0f, 0.0f,
    100.0f, 0.0f
};


static void 
create_descriptor_set_layout (vk_t *vk)
{
    VkDescriptorSetLayoutBinding bindings[2] = {0};

    /* binding for Vertex Shader UBO */
    bindings[0].binding         = 0;
    bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].descriptorCount = 1;

    /* binding for Vertex Shader UBO */
    bindings[1].binding         = 1;
    bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
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
    descPoolSize[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

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

        /* Vertex Shader UBO */
        VkDescriptorBufferInfo desc_ubo_vs_inst = {0};
        desc_ubo_vs_inst.buffer      = s_ubo_vs_instance[i].buf;
        desc_ubo_vs_inst.offset      = 0;
        desc_ubo_vs_inst.range       = VK_WHOLE_SIZE;

        writeSets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSets[1].dstBinding      = 1;
        writeSets[1].descriptorCount = 1;
        writeSets[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeSets[1].pBufferInfo     = &desc_ubo_vs_inst;
        writeSets[1].dstSet          = s_descriptorSet[i];

        vkUpdateDescriptorSets (vk->dev, ARRAY_LENGTH(writeSets), writeSets, 0, NULL);
    }
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

        vk_create_buffer (vk, sizeof(ubo_vs_instance_t) * MAX_INSTANCE_NUM, 
                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags,
                          NULL, &s_ubo_vs_instance[i]);
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
    vk_load_shader_module (vk, "shaders/util_pmeter.vert.spv", &shaders[0]);
    vk_load_shader_module (vk, "shaders/util_pmeter.frag.spv", &shaders[1]);

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
    VkVertexInputBindingDescription inputBinding[1] = {0};
    inputBinding[0].binding    = 0;                         /* vbo[0] vtx */
    inputBinding[0].stride     = 2 * sizeof (float);
    inputBinding[0].inputRate  = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription inputAttribs[1] = {0};
    inputAttribs[0].location = 0;                           /* loc[0] vtx */
    inputAttribs[0].binding  = 0;
    inputAttribs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    inputAttribs[0].offset   = 0;

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
    inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP  ;


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
    depthStencilCI.depthTestEnable      = VK_FALSE;
    depthStencilCI.depthCompareOp       = VK_COMPARE_OP_ALWAYS;
    depthStencilCI.depthWriteEnable     = VK_FALSE;
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


int
init_pmeter (vk_t *vk, int win_w, int win_h, int data_num)
{
    int i, j, k;
    for ( k = 0; k < PMETER_DPY_NUM; k ++)
    {
        s_pm_idx[k] = 0;

        for ( i = 0; i < PMETER_NUM; i ++ )
        {
            for ( j = 0; j < PMETER_DATA_NUM; j ++ )
            {
                s_vertPM[k][i][ 2 * j    ] = 0.0f;
                s_vertPM[k][i][ 2 * j + 1] = (float)j;
            }
        }
    }

    s_wndW = win_w;
    s_wndH = win_h;
    s_data_num = data_num;


    VkBufferUsageFlags    usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkMemoryPropertyFlags mflag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    s_vtx_axis[3] = (float)data_num;
    vk_create_buffer (vk, sizeof(s_vtx_axis),   usage, mflag, s_vtx_axis,   &s_vtx_axis_buf);
    vk_create_buffer (vk, sizeof(s_vtx_cursor), usage, mflag, s_vtx_cursor, &s_vtx_cursor_buf);
    vk_create_buffer (vk, sizeof(s_vertPM),     usage, mflag, s_vertPM,     &s_vtx_graph_buf);

    create_ubo (vk);
    create_descriptor_set_layout (vk);
    create_descriptor_pool (vk);

    init_descriptor_set (vk);

    init_pipeline (vk);

    return 0;
}

static int set_pmeter_val (int dpy_id, int id, float val)
{
    if ( id >= PMETER_NUM )
        return -1;

    if ( dpy_id >= PMETER_DPY_NUM )
        return -1;

    s_vertPM[dpy_id][id][2 * s_pm_idx[dpy_id] + 0] = val;
    return 0;
}

int
draw_pmeter_ex (vk_t *vk, int dpy_id, int x, int y, float scale)
{
    int i, num_time;
    float *laptime,sumval;
    static int   s_ncnt[PMETER_DPY_NUM] = {0};
    static float s_lap[PMETER_DPY_NUM][10] = {{0}};
    float col_gray[]  = {0.5f, 0.5f, 0.5f, 1.0f};
    float col_red[]   = {1.0f, 0.0f, 0.0f, 1.0f};
    float col_green[] = {0.0f, 1.0f, 0.0f, 1.0f};
    int   draw_cnt_axis  = 0;
    int   draw_cnt_graph = 0;
    int   draw_cnt_cursor = 0;
    int   draw_cnt = 0;
    
    pmeter_get_laptime (dpy_id, &num_time, &laptime);

    sumval = 0;
    for (i = 0; i < num_time; i ++)
    {
        s_lap[dpy_id][i] += laptime[i];
        sumval += laptime[i];
    }
    s_lap[dpy_id][i] += sumval;
    s_ncnt[dpy_id] ++;

    if (laptime[0] > 100.0f) laptime[0] = 100.0f;
    if (laptime[1] > 100.0f) laptime[1] = 100.0f;
    if (laptime[2] > 100.0f) laptime[2] = 100.0f;
    if (sumval     > 100.0f) sumval     = 100.0f;
    set_pmeter_val (dpy_id, 0, laptime[0]); /* BLUE:    render */
    set_pmeter_val (dpy_id, 1, laptime[1]); /* SKYBLUE: render */
    set_pmeter_val (dpy_id, 2, laptime[2]); /* SKYBLUE: render */
    set_pmeter_val (dpy_id, 3, sumval);     /* RED:     total  */

    s_pm_idx[dpy_id] ++;
    if (s_pm_idx[dpy_id] >= s_data_num)
        s_pm_idx[dpy_id] = 0;

    VkCommandBuffer command = vk->cmd_bufs[vk->image_index];
    vkCmdBindPipeline (command, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipeline);

    vkCmdBindDescriptorSets (command, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipelineLayout, 0, 1, 
                             &s_descriptorSet[vk->image_index], 0, NULL);

    /* ------------------------- *
     *  update UBO
     * ------------------------- */
    ubo_vs_t ubo_vs = {0};
    ubo_vs.PrjMul[0] =  2.0f / s_wndW;
    ubo_vs.PrjMul[1] = -2.0f / s_wndH;
    ubo_vs.PrjMul[2] =  0.0f;
    ubo_vs.PrjMul[3] =  0.0f;
    ubo_vs.PrjAdd[0] = -1.0f;
    ubo_vs.PrjAdd[1] =  1.0f;
    ubo_vs.PrjAdd[2] =  1.0f;
    ubo_vs.PrjAdd[3] =  1.0f;

    {
        VkDeviceMemory mem = s_ubo_vs[vk->image_index].mem;
        void *p;
        VK_CHECK (vkMapMemory (vk->dev, mem, 0, VK_WHOLE_SIZE, 0, &p));
        memcpy (p, &ubo_vs, sizeof(ubo_vs));
        vkUnmapMemory (vk->dev, mem);
    }

    /* ------------------------- *
     *  update Instance UBO
     * ------------------------- */

    /* axis */
    draw_cnt_axis = 10;
    for (i = 0; i < draw_cnt_axis; i ++)
    {
        ubo_vs_instance_t *inst = &s_vs_instance[draw_cnt];
        inst->ofst_vtx[0] = x + (i + 1) * 10.0f;
        inst->ofst_vtx[1] = y;
        memcpy (inst->color, col_gray, sizeof (inst->color));

        draw_cnt ++;
    }

    /* graph */
    draw_cnt_graph = 1;
    for (i = 0; i < draw_cnt_graph; i ++)
    {
        ubo_vs_instance_t *inst = &s_vs_instance[draw_cnt];
        inst->ofst_vtx[0] = x;
        inst->ofst_vtx[1] = y;
        memcpy (inst->color, col_red, sizeof (inst->color));

        draw_cnt ++;
    }

    /* cursor */
    draw_cnt_cursor = 1;
    for (i = 0; i < draw_cnt_cursor; i ++)
    {
        ubo_vs_instance_t *inst = &s_vs_instance[draw_cnt];
        inst->ofst_vtx[0] = x;
        inst->ofst_vtx[1] = y + s_pm_idx[dpy_id];
        memcpy (inst->color, col_green, sizeof (inst->color));

        draw_cnt ++;
    }

    /* update Uniform Buffer of Vertex Shader Instance */
    {
        VkDeviceMemory mem = s_ubo_vs_instance[vk->image_index].mem;
        void *p;
        VK_CHECK (vkMapMemory (vk->dev, mem, 0, VK_WHOLE_SIZE, 0, &p));
        memcpy (p, s_vs_instance, sizeof(s_vs_instance[0]) * draw_cnt);
        vkUnmapMemory (vk->dev, mem);
    }


    /* ------------------------- *
     *  update Vertex buffer
     * ------------------------- */

    /* graph */
    {
        VkDeviceMemory mem = s_vtx_graph_buf.mem;
        void *p;
        VK_CHECK (vkMapMemory (vk->dev, mem, 0, VK_WHOLE_SIZE, 0, &p));
        memcpy (p, s_vertPM[dpy_id][3], sizeof(float) * 2 * s_data_num);
        vkUnmapMemory (vk->dev, mem);
    }

    /* ------------------------------------ *
     *  Bind Vertex buffer and Draw
     * ------------------------------------ */
    VkBuffer     buffer[1] = {0};
    VkDeviceSize offset[1] = {0};
    int first_instance = 0;
    
    /* axis */
    buffer[0] = s_vtx_axis_buf.buf;
    vkCmdBindVertexBuffers (command, 0, 1, buffer, offset);
    vkCmdDraw (command, 2, draw_cnt_axis, 0, first_instance);

    /* graph */
    buffer[0] = s_vtx_graph_buf.buf;
    first_instance += draw_cnt_axis;
    vkCmdBindVertexBuffers (command, 0, 1, buffer, offset);
    vkCmdDraw (command, s_data_num, draw_cnt_graph, 0, first_instance);

    /* cursor */
    buffer[0] = s_vtx_cursor_buf.buf;
    first_instance += draw_cnt_graph;
    vkCmdBindVertexBuffers (command, 0, 1, buffer, offset);
    vkCmdDraw (command, 2, draw_cnt_cursor, 0, first_instance);

    return 0;
}


int
draw_pmeter (vk_t *vk, int x, int y)
{
    return draw_pmeter_ex (vk, 0, x, y, 1.0f);
}



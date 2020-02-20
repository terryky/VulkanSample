/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include "vk_tools.h"
#include "vk_init.h"
#include "vk_render_target.h"


static int
create_render_pass (vk_t *vk, VkRenderPass *rnd_pass, VkFormat cformat, VkFormat dformat)
{
    VkAttachmentDescription attachments[2] = {0};

    /* color */
    attachments[0].format         = cformat;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    /* depth */
    attachments[1].format         = dformat;
    attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {0};
    colorReference.attachment   = 0;
    colorReference.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {0};
    depthReference.attachment   = 1;
    depthReference.layout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc = {0};
    subpassDesc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount    = 1;
    subpassDesc.pColorAttachments       = &colorReference;
    subpassDesc.pDepthStencilAttachment = &depthReference;


    VkRenderPassCreateInfo ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount  = 2;
    ci.pAttachments     = attachments;
    ci.subpassCount     = 1;
    ci.pSubpasses       = &subpassDesc;

    VK_CHECK (vkCreateRenderPass (vk->dev, &ci, NULL, rnd_pass));

    return 0;
}

static int
create_rtarget_color (vk_t *vk, vk_texture_t *vk_tex, uint32_t width, uint32_t height, VkFormat format)
{
    VkImage         img;
    VkDeviceMemory  mem;
    VkImageView     view;
    VkSampler       sampler;

    /* create VkImage */
    VkImageCreateInfo ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType        = VK_IMAGE_TYPE_2D;
    ci.format           = format;
    ci.extent.width     = width;
    ci.extent.height    = height;
    ci.extent.depth     = 1;
    ci.mipLevels        = 1;
    ci.arrayLayers      = 1;
    ci.samples          = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling           = VK_IMAGE_TILING_OPTIMAL;
    ci.usage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ci.sharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK (vkCreateImage (vk->dev, &ci, NULL, &img));

    /* Allocate Memory for VkImage */
    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements (vk->dev, img, &reqs);

    VkMemoryAllocateInfo ai = {0};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = reqs.size;
    ai.memoryTypeIndex = vk_get_memory_type_index (reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK (vkAllocateMemory (vk->dev, &ai, NULL, &mem));
    VK_CHECK (vkBindImageMemory (vk->dev, img, mem, 0));

    /* create VkImageView */
    VkImageViewCreateInfo vci = {0};
    vci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    vci.image            = img;
    vci.format           = format;
    vci.components.r     = VK_COMPONENT_SWIZZLE_R;
    vci.components.g     = VK_COMPONENT_SWIZZLE_G;
    vci.components.b     = VK_COMPONENT_SWIZZLE_B;
    vci.components.a     = VK_COMPONENT_SWIZZLE_A;
    vci.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
    vci.subresourceRange.baseMipLevel    = 0;
    vci.subresourceRange.levelCount      = 1;
    vci.subresourceRange.baseArrayLayer  = 0;
    vci.subresourceRange.layerCount      = 1;

    VK_CHECK (vkCreateImageView (vk->dev, &vci, NULL, &view));

    /* create VkSampler */
    VkSamplerCreateInfo sci = {0};
    sci.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.minFilter     = VK_FILTER_LINEAR;
    sci.magFilter     = VK_FILTER_LINEAR;
    sci.addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.maxAnisotropy = 1.0f;
    sci.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    VK_CHECK (vkCreateSampler (vk->dev, &sci, NULL, &sampler));


    vk_tex->img     = img;
    vk_tex->mem     = mem;
    vk_tex->view    = view;
    vk_tex->sampler = sampler;

    return 0;
    
}


static int
create_rtarget_depth (vk_t *vk, vk_texture_t *depth, uint32_t width, uint32_t height, VkFormat format)
{
    VkImage         img;
    VkDeviceMemory  mem;
    VkImageView     view;

    VkImageCreateInfo ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType        = VK_IMAGE_TYPE_2D;
    ci.format           = format;
    ci.extent.width     = width;
    ci.extent.height    = height;
    ci.extent.depth     = 1;
    ci.mipLevels        = 1;
    ci.arrayLayers      = 1;
    ci.samples          = VK_SAMPLE_COUNT_1_BIT;
    ci.usage            = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ci.sharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK (vkCreateImage (vk->dev, &ci, NULL, &img));

    /* Allocate memory for Depth Buffer */
    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements (vk->dev, img, &reqs);

    VkMemoryAllocateInfo ai = {0};
    ai.sType            = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize   = reqs.size;
    ai.memoryTypeIndex  = vk_get_memory_type_index (reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK (vkAllocateMemory  (vk->dev, &ai, NULL, &mem));
    VK_CHECK (vkBindImageMemory (vk->dev, img, mem, 0));

    /* create VkImageView */
    VkImageViewCreateInfo vci = {0};
    vci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    vci.image            = img;
    vci.format           = format;
    vci.components.r     = VK_COMPONENT_SWIZZLE_R;
    vci.components.g     = VK_COMPONENT_SWIZZLE_G;
    vci.components.b     = VK_COMPONENT_SWIZZLE_B;
    vci.components.a     = VK_COMPONENT_SWIZZLE_A;
    vci.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_DEPTH_BIT;
    vci.subresourceRange.baseMipLevel    = 0;
    vci.subresourceRange.levelCount      = 1;
    vci.subresourceRange.baseArrayLayer  = 0;
    vci.subresourceRange.layerCount      = 1;

    VK_CHECK (vkCreateImageView (vk->dev, &vci, NULL, &view));

    depth->img  = img;
    depth->mem  = mem;
    depth->view = view;

    return 0;
}



static int
create_rtarget_frame_buffer (vk_t *vk, VkFramebuffer *fb, uint32_t width, uint32_t height, VkRenderPass render_pass,
                             vk_texture_t color_tgt, vk_texture_t depth_tgt)
{
    VkImageView attachments[2];
    attachments[0] = color_tgt.view;
    attachments[1] = depth_tgt.view;

    VkFramebufferCreateInfo ci = {0};
    ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.renderPass      = render_pass;
    ci.attachmentCount = 2;
    ci.pAttachments    = attachments;
    ci.width           = width;
    ci.height          = height;
    ci.layers          = 1;

    VK_CHECK (vkCreateFramebuffer (vk->dev, &ci, NULL, fb));

    return 0;
}



int
vk_create_render_target (vk_t *vk, vk_rtarget_t *rtarget)
{
    VkFormat     cformat = VK_FORMAT_B8G8R8A8_UNORM;
    VkFormat     dformat = VK_FORMAT_D32_SFLOAT;
    uint32_t     width  = 960;
    uint32_t     height = 540;
    VkRenderPass rnd_pass;
    vk_texture_t color_tgt;
    vk_texture_t depth_tgt;
    VkFramebuffer tgt_fb;

    create_render_pass (vk, &rnd_pass, cformat, dformat);

    create_rtarget_color (vk, &color_tgt, width, height, cformat);
    create_rtarget_depth (vk, &depth_tgt, width, height, dformat);

    create_rtarget_frame_buffer (vk, &tgt_fb, width, height, rnd_pass, color_tgt, depth_tgt);

    rtarget->framebuffer = tgt_fb;
    rtarget->render_pass = rnd_pass;
    rtarget->color_tgt   = color_tgt;
    rtarget->depth_tgt   = depth_tgt;
    rtarget->width       = width;
    rtarget->height      = height;

    return 0;
}


int
vk_begin_render_target (vk_t *vk, vk_rtarget_t *rtarget)
{
    uint32_t fb_idx = vk->image_index;
    VkCommandBuffer command = vk->cmd_bufs[fb_idx];

    /* Clear */
    VkClearValue clear_val[2];
    clear_val[0].color.float32[0] = 0.25f;
    clear_val[0].color.float32[1] = 0.25f;
    clear_val[0].color.float32[2] = 0.25f;
    clear_val[0].color.float32[3] = 1.00f;
    clear_val[1].depthStencil.depth   = 1.0f;
    clear_val[1].depthStencil.stencil = 0;

    /* RenderPass for FB/FBO */
    VkRenderPassBeginInfo renderPassBI = {0};
    renderPassBI.sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.renderArea.offset.x= 0;
    renderPassBI.renderArea.offset.y= 0;
    renderPassBI.clearValueCount    = 2;
    renderPassBI.pClearValues       = clear_val;

    /* FBO */
    if (rtarget)
    {
        renderPassBI.renderPass               = rtarget->render_pass;
        renderPassBI.framebuffer              = rtarget->framebuffer;
        renderPassBI.renderArea.extent.width  = rtarget->width;
        renderPassBI.renderArea.extent.height = rtarget->height;
    }
    /* Default FB */
    else
    {
        renderPassBI.renderPass         = vk->render_pass;
        renderPassBI.framebuffer        = vk->framebuffers[fb_idx];
        renderPassBI.renderArea.extent  = vk->swapchain_extent;
    }

    vkCmdBeginRenderPass (command, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

    return 0;
}


int
vk_end_render_target (vk_t *vk, vk_rtarget_t *rtarget)
{
    VkCommandBuffer command = vk->cmd_bufs[vk->image_index];

    vkCmdEndRenderPass (command);

    return 0;
}

int
vk_barrier_render_target (vk_t *vk, vk_rtarget_t *rtarget)
{
    if (rtarget == NULL)
        return 0;

    VkCommandBuffer command = vk->cmd_bufs[vk->image_index];

    /*
     * +----------------------+--------------------------+--------------------------+
     * |                      |           old      ===>  |  ===>     new            |
     * +----------------------+--------------------------+--------------------------+
     * |Pipeline stage barrier| COLOR_ATTACHMENT_OUTPUT  | FRAGMENT_SHADER          |
     * +--------+-------------+--------------------------+--------------------------+
     * | Image  | Access      | COLOR_ATTACHMENT_WRITE   | SHADER_READ_BIT          |
     * | Memory | Layout      | COLOR_ATTACHMENT_OPTIMAL | SHADER_READ_ONLY_OPTIMAL |
     * | Barrier| QueueFamily | IGNORED                  | IGNORED                  |
     * +--------+-------------+--------------------------+--------------------------+
     */
    VkImageMemoryBarrier img_barrier = {0};
    img_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    img_barrier.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    img_barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
    img_barrier.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    img_barrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    img_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.image               = rtarget->color_tgt.img;
    img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_barrier.subresourceRange.baseMipLevel   = 0;
    img_barrier.subresourceRange.levelCount     = 1;
    img_barrier.subresourceRange.baseArrayLayer = 0;
    img_barrier.subresourceRange.layerCount     = 1;

    vkCmdPipelineBarrier (command,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  /* Src stage */
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,          /* Dst stage */
        VK_DEPENDENCY_BY_REGION_BIT,                    /* dependency */
        0, NULL,                                        /* Memory barrier */
        0, NULL,                                        /* Bufer memory barrier */
        1, &img_barrier);                               /* Image memory barrier */

    return 0;
}


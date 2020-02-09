/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include "vk_tools.h"
#include "vk_render.h"

extern vk_t vk;


int
vk_render ()
{
    uint32_t nextImageIndex = 0;
    VK_CHECK (vkAcquireNextImageKHR (vk.dev, vk.swapchain, UINT64_MAX, 
                           vk.sem_present_complete, VK_NULL_HANDLE, &nextImageIndex));

    VkFence cmd_fence = vk.fences[nextImageIndex];
    VK_CHECK (vkWaitForFences (vk.dev, 1, &cmd_fence, VK_TRUE, UINT64_MAX));

    /* Clear */
    VkClearValue clear_val[2];
    clear_val[0].color.float32[0] = 0.25f;
    clear_val[0].color.float32[1] = 0.25f;
    clear_val[0].color.float32[2] = 0.25f;
    clear_val[0].color.float32[3] = 1.00f;
    clear_val[1].depthStencil.depth   = 1.0f;
    clear_val[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo renderPassBI = {0};
    renderPassBI.sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.renderPass         = vk.render_pass;
    renderPassBI.framebuffer        = vk.framebuffers[nextImageIndex];
    renderPassBI.renderArea.offset.x= 0;
    renderPassBI.renderArea.offset.y= 0;
    renderPassBI.renderArea.extent  = vk.swapchain_extent;
    renderPassBI.clearValueCount    = 2;
    renderPassBI.pClearValues       = clear_val;

    /* Begin Command buffer, Render pass */
    VkCommandBufferBeginInfo commandBI = {0};
    commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkCommandBuffer command = vk.cmd_bufs[nextImageIndex];
    VK_CHECK (vkBeginCommandBuffer (command, &commandBI));

    vkCmdBeginRenderPass (command, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

    vk.image_index = nextImageIndex;

    /* End Command buffer, Render pass */
    vkCmdEndRenderPass (command);
    vkEndCommandBuffer (command);


    /* Submit command */
    VkSubmitInfo submitInfo = {0};
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &command;
    submitInfo.pWaitDstStageMask    = &waitStageMask;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &vk.sem_present_complete;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &vk.sem_render_complete;
    vkResetFences (vk.dev, 1, &cmd_fence);
    vkQueueSubmit (vk.devq, 1, &submitInfo, cmd_fence);


    /* Present */
    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount      = 1;
    presentInfo.pSwapchains         = &vk.swapchain;
    presentInfo.pImageIndices       = &nextImageIndex;
    presentInfo.waitSemaphoreCount  = 1;
    presentInfo.pWaitSemaphores     = &vk.sem_render_complete;
    vkQueuePresentKHR (vk.devq, &presentInfo);

    return 0;
}


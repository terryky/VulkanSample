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
#include "vk_winsys.h"

#define USE_SEMAPHORE_TO_WAIT_PRESENT_COMPLETE
//#define USE_MEASURE_SWAP_TIME

#if defined (USE_MEASURE_SWAP_TIME)
#include "util_pmeter.h"
#endif

int
vk_render (vk_t *vk, uint32_t flags, void (*cb_make_command)(VkCommandBuffer, void *), void *usr_data)
{
    uint32_t run_index = vk->swapchain_run_index;
    uint32_t img_index = 0;

#if defined (USE_MEASURE_SWAP_TIME)
    static double ttime[10];
    ttime[0] = pmeter_get_time_ms ();
#endif

    /*
     *  Get the index of the next image to use. (returned image may not be ready yet.)
     *   - Before CPU use, need to wait for the FENCE.
     *   - Before GPU use, need to wait for the SEMAPHORE.
     */
#if defined (USE_SEMAPHORE_TO_WAIT_PRESENT_COMPLETE)
    VK_CHECK (vkAcquireNextImageKHR (vk->dev, vk->swapchain, UINT64_MAX, 
                           vk->sem_present_complete[run_index], VK_NULL_HANDLE, &img_index));
#else
    VK_CHECK (vkResetFences (vk->dev, 1, &vk->fence_present_complete[run_index]));
    VK_CHECK (vkAcquireNextImageKHR (vk->dev, vk->swapchain, UINT64_MAX, 
                           VK_NULL_HANDLE, vk->fence_present_complete[run_index], &img_index));
    VK_CHECK (vkWaitForFences (vk->dev, 1, &vk->fence_present_complete[run_index], VK_TRUE, UINT64_MAX));
#endif
    
#if defined (USE_MEASURE_SWAP_TIME)
    ttime[1] = pmeter_get_time_ms ();
#endif

    VkCommandBuffer command   = vk->cmd_bufs[img_index];
    VkFence         cmd_fence = vk->fences[img_index];

    VK_CHECK (vkWaitForFences (vk->dev, 1, &cmd_fence, VK_TRUE, UINT64_MAX));
    VK_CHECK (vkResetFences (vk->dev, 1, &cmd_fence));

#if defined (USE_MEASURE_SWAP_TIME)
    ttime[2] = pmeter_get_time_ms ();
#endif

    /* Begin Command buffer */
    VkCommandBufferBeginInfo commandBI = {0};
    commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK (vkBeginCommandBuffer (command, &commandBI));

    vk->image_index = img_index;

    /* if use default RenderPass, begin it. */
    if (!flags)
    {
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
        renderPassBI.renderPass         = vk->render_pass;
        renderPassBI.framebuffer        = vk->framebuffers[img_index];
        renderPassBI.renderArea.offset.x= 0;
        renderPassBI.renderArea.offset.y= 0;
        renderPassBI.renderArea.extent  = vk->swapchain_extent;
        renderPassBI.clearValueCount    = 2;
        renderPassBI.pClearValues       = clear_val;

        vkCmdBeginRenderPass (command, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
    }

#if defined (USE_MEASURE_SWAP_TIME)
    ttime[3] = pmeter_get_time_ms ();
#endif

    /* ------------------------------------------- *
     *  Callback to make render commands
     * ------------------------------------------- */
    if (cb_make_command)
    {
        cb_make_command (command, usr_data);
    }

#if defined (USE_MEASURE_SWAP_TIME)
    ttime[4] = pmeter_get_time_ms ();
#endif

    /* if use default RenderPass, end it. */
    if (!flags)
    {
        vkCmdEndRenderPass (command);
    }

    /* End Command buffer */
    vkEndCommandBuffer (command);

    /* Submit command */
    VkSubmitInfo submitInfo = {0};
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &command;
    submitInfo.pWaitDstStageMask    = &waitStageMask;
#if defined (USE_SEMAPHORE_TO_WAIT_PRESENT_COMPLETE)
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &vk->sem_present_complete[run_index];
#endif
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &vk->sem_render_complete[run_index];
    vkQueueSubmit (vk->devq, 1, &submitInfo, cmd_fence);

#if defined (USE_MEASURE_SWAP_TIME)
    ttime[5] = pmeter_get_time_ms ();
#endif

    /* Present */
    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount      = 1;
    presentInfo.pSwapchains         = &vk->swapchain;
    presentInfo.pImageIndices       = &img_index;
    presentInfo.waitSemaphoreCount  = 1;
    presentInfo.pWaitSemaphores     = &vk->sem_render_complete[run_index];
    vkQueuePresentKHR (vk->devq, &presentInfo);

    vkin_winsys_swap (vk);

    vk->swapchain_run_index = (run_index + 1) % vk->swapchain_img_count;

#if defined (USE_MEASURE_SWAP_TIME)
    ttime[6] = pmeter_get_time_ms ();
    fprintf (stderr, "[%d:%d], %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f\n", run_index, img_index,
                ttime[1]-ttime[0],
                ttime[2]-ttime[1],
                ttime[3]-ttime[2],
                ttime[4]-ttime[3],
                ttime[5]-ttime[4],
                ttime[6]-ttime[5]);
#endif

    return 0;
}


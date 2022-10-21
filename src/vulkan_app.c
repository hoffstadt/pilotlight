/*
   vulkan_app.c
*/

/*
Index of this file:
// [SECTION] includes
// [SECTION] structs
// [SECTION] pl_app_load
// [SECTION] pl_app_setup
// [SECTION] pl_app_shutdown
// [SECTION] pl_app_resize
// [SECTION] pl_app_render
*/

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include "pl.h"
#include "vulkan_pl_graphics.h"
#include "pl_profile.h"
#include "pl_log.h"
#include "pl_ds.h"
#include "pl_io.h"
#include "pl_memory.h"
#include "vulkan_pl_drawing.h"
#include <string.h> // memset

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct plAppData_t
{
    plVulkanDevice    device;
    plVulkanGraphics  graphics;
    plVulkanSwapchain swapchain;
    plDrawContext*    ctx;
    plDrawList*       drawlist;
    plDrawLayer*      fgDrawLayer;
    plDrawLayer*      bgDrawLayer;
    plFontAtlas       fontAtlas;
    plProfileContext  tProfileCtx;
    plLogContext      tLogCtx;
    plMemoryContext   tMemoryCtx;
} plAppData;

//-----------------------------------------------------------------------------
// [SECTION] pl_app_load
//-----------------------------------------------------------------------------

PL_EXPORT void*
pl_app_load(plIOContext* ptIOCtx, plAppData* ptAppData)
{
    plAppData* tPNewData = NULL;

    if(ptAppData) // reload
    {
        tPNewData = ptAppData;
    }
    else // first run
    {
        tPNewData = malloc(sizeof(plAppData));
        memset(tPNewData, 0, sizeof(plAppData));
    }

    pl_set_log_context(&tPNewData->tLogCtx);
    pl_set_profile_context(&tPNewData->tProfileCtx);
    pl_set_memory_context(&tPNewData->tMemoryCtx);
    pl_set_io_context(ptIOCtx);
    return tPNewData;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_setup
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_setup(plAppData* ptAppData)
{
    // get io context
    plIOContext* ptIOCtx = pl_get_io_context();

    // create vulkan instance
    pl_create_instance(&ptAppData->graphics, VK_API_VERSION_1_1, true);

    // create surface
    #ifdef _WIN32
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext = NULL,
            .flags = 0,
            .hinstance = GetModuleHandle(NULL),
            .hwnd = *(HWND*)ptIOCtx->pBackendPlatformData
        };
        PL_VULKAN(vkCreateWin32SurfaceKHR(ptAppData->graphics.instance, &surfaceCreateInfo, NULL, &ptAppData->graphics.surface));
    #else
    #endif

    // create devices
    pl_create_device(ptAppData->graphics.instance, ptAppData->graphics.surface, &ptAppData->device, true);
    
    // create swapchain
    pl_create_swapchain(&ptAppData->device, ptAppData->graphics.surface, (uint32_t)ptIOCtx->afMainViewportSize[0], (uint32_t)ptIOCtx->afMainViewportSize[1], &ptAppData->swapchain);

    // setup memory context
    pl_initialize_memory_context(&ptAppData->tMemoryCtx);

    // setup profiling context
    pl_initialize_profile_context(&ptAppData->tProfileCtx);

    // setup logging
    pl_initialize_log_context(&ptAppData->tLogCtx);
    pl_add_log_channel("Default", PL_CHANNEL_TYPE_CONSOLE);
    pl_log_info(0, "Setup logging");

    // create render pass
    VkAttachmentDescription colorAttachment = {
        .flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        .format = ptAppData->swapchain.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference attachmentReference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentReference,
        .pDepthStencilAttachment = VK_NULL_HANDLE
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1u,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 0,
        .pDependencies = VK_NULL_HANDLE
    };
    PL_VULKAN(vkCreateRenderPass(ptAppData->device.logicalDevice, &renderPassInfo, NULL, &ptAppData->graphics.renderPass));

    // create frame buffers
    pl_create_framebuffers(&ptAppData->device, ptAppData->graphics.renderPass, &ptAppData->swapchain);
    
    // create per frame resources
    pl_create_frame_resources(&ptAppData->graphics, &ptAppData->device);
    
    // setup drawing api
    ptAppData->ctx = pl_create_draw_context_vulkan(ptAppData->device.physicalDevice, 3, ptAppData->device.logicalDevice);
    ptAppData->drawlist = pl_create_drawlist(ptAppData->ctx);
    pl_setup_drawlist_vulkan(ptAppData->drawlist, ptAppData->graphics.renderPass);
    ptAppData->bgDrawLayer = pl_request_draw_layer(ptAppData->drawlist, "Background Layer");
    ptAppData->fgDrawLayer = pl_request_draw_layer(ptAppData->drawlist, "Foreground Layer");

    // create font atlas
    pl_add_default_font(&ptAppData->fontAtlas);
    pl_build_font_atlas(ptAppData->ctx, &ptAppData->fontAtlas);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_shutdown(plAppData* ptAppData)
{
    // ensure device is finished
    vkDeviceWaitIdle(ptAppData->device.logicalDevice);

    // cleanup font atlas
    pl_cleanup_font_atlas(&ptAppData->fontAtlas);

    // cleanup drawing api
    pl_cleanup_draw_context(ptAppData->ctx);

    // destroy swapchain
    for (uint32_t i = 0u; i < ptAppData->swapchain.imageCount; i++)
    {
        vkDestroyImageView(ptAppData->device.logicalDevice, ptAppData->swapchain.imageViews[i], NULL);
        vkDestroyFramebuffer(ptAppData->device.logicalDevice, ptAppData->swapchain.frameBuffers[i], NULL);
    }

    // destroy default render pass
    vkDestroyRenderPass(ptAppData->device.logicalDevice, ptAppData->graphics.renderPass, NULL);
    vkDestroySwapchainKHR(ptAppData->device.logicalDevice, ptAppData->swapchain.swapChain, NULL);

    // cleanup graphics context
    pl_cleanup_graphics(&ptAppData->graphics, &ptAppData->device);

    // cleanup profiling context
    pl__cleanup_profile_context();

    // cleanup logging context
    pl_cleanup_log_context();

    // cleanup memory context
    pl_cleanup_memory_context();
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_resize(plAppData* ptAppData)
{
    // get io context
    plIOContext* ptIOCtx = pl_get_io_context();

    pl_create_swapchain(&ptAppData->device, ptAppData->graphics.surface, (uint32_t)ptIOCtx->afMainViewportSize[0], (uint32_t)ptIOCtx->afMainViewportSize[1], &ptAppData->swapchain);
    pl_create_framebuffers(&ptAppData->device, ptAppData->graphics.renderPass, &ptAppData->swapchain);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_render
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_render(plAppData* ptAppData)
{
    // get io context
    plIOContext* ptIOCtx = pl_get_io_context();

    pl_new_draw_frame(ptAppData->ctx);

    // begin profiling frame (temporarily using drawing context frame count)
    pl__begin_profile_frame(ptAppData->ctx->frameCount);

    VkClearValue clearValues[2] = 
    {
        {
            .color.float32[0] = 0.1f,
            .color.float32[1] = 0.0f,
            .color.float32[2] = 0.0f,
            .color.float32[3] = 1.0f
        },
        {
            .depthStencil.depth = 1.0f,
            .depthStencil.stencil = 0
        }    
    };

    plVulkanFrameContext* currentFrame = pl_get_frame_resources(&ptAppData->graphics);

    // begin frame
    PL_VULKAN(vkWaitForFences(ptAppData->device.logicalDevice, 1, &currentFrame->inFlight, VK_TRUE, UINT64_MAX));
    VkResult err = vkAcquireNextImageKHR(ptAppData->device.logicalDevice, ptAppData->swapchain.swapChain, UINT64_MAX, currentFrame->imageAvailable,VK_NULL_HANDLE, &ptAppData->swapchain.currentImageIndex);
    if(err == VK_SUBOPTIMAL_KHR || err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if(err == VK_ERROR_OUT_OF_DATE_KHR)
        {
            pl_create_swapchain(&ptAppData->device, ptAppData->graphics.surface, (uint32_t)ptIOCtx->afMainViewportSize[0], (uint32_t)ptIOCtx->afMainViewportSize[1], &ptAppData->swapchain);
            pl_create_framebuffers(&ptAppData->device, ptAppData->graphics.renderPass, &ptAppData->swapchain);
            return;
        }
    }
    else
    {
        PL_VULKAN(err);
    }

    if (currentFrame->inFlight != VK_NULL_HANDLE)
        PL_VULKAN(vkWaitForFences(ptAppData->device.logicalDevice, 1, &currentFrame->inFlight, VK_TRUE, UINT64_MAX));

    // begin recording
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    PL_VULKAN(vkBeginCommandBuffer(currentFrame->cmdBuf, &beginInfo));

    // begin render pass
    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = ptAppData->graphics.renderPass,
        .framebuffer = ptAppData->swapchain.frameBuffers[ptAppData->swapchain.currentImageIndex],
        .renderArea.offset.x = 0,
        .renderArea.offset.y = 0,
        .renderArea.extent = ptAppData->swapchain.extent,
        .clearValueCount = 2,
        .pClearValues = clearValues
    };
    vkCmdBeginRenderPass(currentFrame->cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // set viewport
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)ptAppData->swapchain.extent.width,
        .height = (float)ptAppData->swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(currentFrame->cmdBuf, 0, 1, &viewport);

    // set scissor
    VkRect2D dynamicScissor = {.extent = ptAppData->swapchain.extent};
    vkCmdSetScissor(currentFrame->cmdBuf, 0, 1, &dynamicScissor);

    // draw profiling info
    pl_begin_profile_sample("Draw Profiling Info");
    static char pcDeltaTime[64] = {0};
    pl_sprintf(pcDeltaTime, "%.6f ms", ptIOCtx->fDeltaTime);
    pl_add_text(ptAppData->fgDrawLayer, &ptAppData->fontAtlas.sbFonts[0], 13.0f, (plVec2){10.0f, 10.0f}, (plVec4){1.0f, 1.0f, 0.0f, 1.0f}, pcDeltaTime, 0.0f);
    char cPProfileValue[64] = {0};
    for(uint32_t i = 0u; i < pl_sb_size(ptAppData->tProfileCtx.tPLastFrame->sbSamples); i++)
    {
        plProfileSample* tPSample = &ptAppData->tProfileCtx.tPLastFrame->sbSamples[i];
        pl_add_text(ptAppData->fgDrawLayer, &ptAppData->fontAtlas.sbFonts[0], 13.0f, (plVec2){10.0f + (float)tPSample->uDepth * 15.0f, 50.0f + (float)i * 15.0f}, (plVec4){1.0f, 1.0f, 1.0f, 1.0f}, tPSample->cPName, 0.0f);
        plVec2 sampleTextSize = pl_calculate_text_size(&ptAppData->fontAtlas.sbFonts[0], 13.0f, tPSample->cPName, 0.0f);
        pl_sprintf(cPProfileValue, ": %0.5f", tPSample->dDuration);
        pl_add_text(ptAppData->fgDrawLayer, &ptAppData->fontAtlas.sbFonts[0], 13.0f, (plVec2){sampleTextSize.x + 15.0f + (float)tPSample->uDepth * 15.0f, 50.0f + (float)i * 15.0f}, (plVec4){1.0f, 1.0f, 1.0f, 1.0f}, cPProfileValue, 0.0f);
    }
    pl_end_profile_sample();

    // draw commands
    pl_begin_profile_sample("Add draw commands");
    pl_add_text(ptAppData->fgDrawLayer, &ptAppData->fontAtlas.sbFonts[0], 13.0f, (plVec2){300.0f, 10.0f}, (plVec4){0.1f, 0.5f, 0.0f, 1.0f}, "Pilot Light\nGraphics", 0.0f);
    pl_add_triangle_filled(ptAppData->bgDrawLayer, (plVec2){300.0f, 50.0f}, (plVec2){300.0f, 150.0f}, (plVec2){350.0f, 50.0f}, (plVec4){1.0f, 0.0f, 0.0f, 1.0f});
    pl__begin_profile_sample("Calculate text size");
    plVec2 textSize = pl_calculate_text_size(&ptAppData->fontAtlas.sbFonts[0], 13.0f, "Pilot Light\nGraphics", 0.0f);
    pl__end_profile_sample();
    pl_add_rect_filled(ptAppData->bgDrawLayer, (plVec2){300.0f, 10.0f}, (plVec2){300.0f + textSize.x, 10.0f + textSize.y}, (plVec4){0.0f, 0.0f, 0.8f, 0.5f});
    pl_add_line(ptAppData->bgDrawLayer, (plVec2){500.0f, 10.0f}, (plVec2){10.0f, 500.0f}, (plVec4){1.0f, 1.0f, 1.0f, 0.5f}, 2.0f);
    pl_end_profile_sample();

    // submit draw layers
    pl_begin_profile_sample("Submit draw layers");
    pl_submit_draw_layer(ptAppData->bgDrawLayer);
    pl_submit_draw_layer(ptAppData->fgDrawLayer);
    pl_end_profile_sample();

    // submit draw lists
    pl_submit_drawlist_vulkan(ptAppData->drawlist, (float)ptIOCtx->afMainViewportSize[0], (float)ptIOCtx->afMainViewportSize[1], currentFrame->cmdBuf, (uint32_t)ptAppData->graphics.currentFrameIndex);

    // end render pass
    vkCmdEndRenderPass(currentFrame->cmdBuf);

    // end recording
    PL_VULKAN(vkEndCommandBuffer(currentFrame->cmdBuf));

    // submit
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &currentFrame->imageAvailable,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &currentFrame->cmdBuf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &currentFrame->renderFinish
    };
    PL_VULKAN(vkResetFences(ptAppData->device.logicalDevice, 1, &currentFrame->inFlight));
    PL_VULKAN(vkQueueSubmit(ptAppData->device.graphicsQueue, 1, &submitInfo, currentFrame->inFlight));          
    
    // present                        
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &currentFrame->renderFinish,
        .swapchainCount = 1,
        .pSwapchains = &ptAppData->swapchain.swapChain,
        .pImageIndices = &ptAppData->swapchain.currentImageIndex,
    };
    VkResult result = vkQueuePresentKHR(ptAppData->device.presentQueue, &presentInfo);
    if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        pl_create_swapchain(&ptAppData->device, ptAppData->graphics.surface, (uint32_t)ptIOCtx->afMainViewportSize[0], (uint32_t)ptIOCtx->afMainViewportSize[1], &ptAppData->swapchain);
        pl_create_framebuffers(&ptAppData->device, ptAppData->graphics.renderPass, &ptAppData->swapchain);
    }
    else
    {
        PL_VULKAN(result);
    }

    ptAppData->graphics.currentFrameIndex = (ptAppData->graphics.currentFrameIndex + 1) % ptAppData->graphics.framesInFlight;

    // end profiling frame
    pl_end_profile_frame();
}
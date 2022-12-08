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
// [SECTION] pl_app_update
*/

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <string.h> // memset
#include "pilotlight.h"
#include "pl_graphics_vulkan.h"
#include "pl_profile.h"
#include "pl_log.h"
#include "pl_ds.h"
#include "pl_io.h"
#include "pl_memory.h"
#include "pl_draw_vulkan.h"
#include "pl_math.h"
#include "pl_camera.h"
#include "pl_registry.h" // data registry
#include "pl_ext.h"      // extension registry

// extensions
#include "pl_draw_extension.h"
#include "pl_gltf_ext.h"

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plAppData
{
    plGraphics          tGraphics;
    plDrawContext       ctx;
    plDrawList          drawlist;
    plDrawLayer*        fgDrawLayer;
    plDrawLayer*        bgDrawLayer;
    plFontAtlas         fontAtlas;
    plProfileContext    tProfileCtx;
    plLogContext        tLogCtx;
    plMemoryContext     tMemoryCtx;
    plDataRegistry      tDataRegistryCtx;
    plExtensionRegistry tExtensionRegistryCtx;

    // extension apis
    plDrawExtension* ptDrawExtApi;

    // testing
    plCamera        tCamera;
    uint32_t        ulConstantBuffer;
    uint32_t        ulColorTexture;

    // new
    plShader     tShader0;
    plShader     tShader1;
    plBindGroup  tBindGroup0;
    plDraw*      sbtDraws;
    plDrawArea*  sbtDrawAreas;
    plMat4*      sbtModelMats;
    plGltf       tGltf;
} plAppData;

typedef struct _plTransforms
{
    plMat4 tModel;
    plMat4 tModelView;
    plMat4 tModelViewProjection;
} plTransforms;

static inline float frandom(float fMax){ return (float)rand()/(float)(RAND_MAX/fMax);}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_load
//-----------------------------------------------------------------------------

PL_EXPORT void*
pl_app_load(plIOContext* ptIOCtx, plAppData* ptAppData)
{
    if(ptAppData) // reload
    {
        pl_set_log_context(&ptAppData->tLogCtx);
        pl_set_profile_context(&ptAppData->tProfileCtx);
        pl_set_memory_context(&ptAppData->tMemoryCtx);
        pl_set_data_registry(&ptAppData->tDataRegistryCtx);
        pl_set_extension_registry(&ptAppData->tExtensionRegistryCtx);
        pl_set_io_context(ptIOCtx);

        plExtension* ptExtension = pl_get_extension(PL_EXT_DRAW);
        ptAppData->ptDrawExtApi = pl_get_api(ptExtension, PL_EXT_API_DRAW);

        return ptAppData;
    }

    plAppData* tPNewData = malloc(sizeof(plAppData));
    memset(tPNewData, 0, sizeof(plAppData));

    pl_set_io_context(ptIOCtx);
    pl_initialize_memory_context(&tPNewData->tMemoryCtx);
    pl_initialize_profile_context(&tPNewData->tProfileCtx);
    pl_initialize_data_registry(&tPNewData->tDataRegistryCtx);

    // setup logging
    pl_initialize_log_context(&tPNewData->tLogCtx);
    pl_add_log_channel("Default", PL_CHANNEL_TYPE_CONSOLE);
    pl_log_info(0, "Setup logging");

    // setup extension registry
    pl_initialize_extension_registry(&tPNewData->tExtensionRegistryCtx);
    pl_register_data("memory", &tPNewData->tMemoryCtx);
    pl_register_data("profile", &tPNewData->tProfileCtx);
    pl_register_data("log", &tPNewData->tLogCtx);
    pl_register_data("io", ptIOCtx);

    plExtension tExtension = {0};
    pl_get_draw_extension_info(&tExtension);
    pl_load_extension(&tExtension);

    plExtension* ptExtension = pl_get_extension(PL_EXT_DRAW);
    tPNewData->ptDrawExtApi = pl_get_api(ptExtension, PL_EXT_API_DRAW);

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

    // setup renderer
    pl_setup_graphics(&ptAppData->tGraphics);
    
    // setup drawing api
    pl_initialize_draw_context_vulkan(&ptAppData->ctx, ptAppData->tGraphics.tDevice.tPhysicalDevice, 3, ptAppData->tGraphics.tDevice.tLogicalDevice);
    pl_register_drawlist(&ptAppData->ctx, &ptAppData->drawlist);
    pl_setup_drawlist_vulkan(&ptAppData->drawlist, ptAppData->tGraphics.tRenderPass, ptAppData->tGraphics.tSwapchain.tMsaaSamples);
    ptAppData->bgDrawLayer = pl_request_draw_layer(&ptAppData->drawlist, "Background Layer");
    ptAppData->fgDrawLayer = pl_request_draw_layer(&ptAppData->drawlist, "Foreground Layer");

    // create font atlas
    pl_add_default_font(&ptAppData->fontAtlas);
    pl_build_font_atlas(&ptAppData->ctx, &ptAppData->fontAtlas);

    // testing
    ptAppData->tCamera = pl_create_perspective_camera((plVec3){0.0f, 0.0f, 8.5f}, PL_PI_3, ptIOCtx->afMainViewportSize[0] / ptIOCtx->afMainViewportSize[1], 0.1f, 100.0f);

    // pl_ext_load_gltf(&ptAppData->tGraphics, "../../mvImporter/data/glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf", &ptAppData->tGltf);
    // pl_ext_load_gltf(&ptAppData->tGraphics, "../../mvImporter/data/glTF-Sample-Models/2.0/DamagedHelmet/glTF/DamagedHelmet.gltf", &ptAppData->tGltf);
    pl_ext_load_gltf(&ptAppData->tGraphics, "../data/glTF-Sample-Models-master/2.0/FlightHelmet/glTF/FlightHelmet.gltf", &ptAppData->tGltf);

    ptAppData->ulConstantBuffer = pl_create_constant_buffer(&ptAppData->tGraphics.tResourceManager, sizeof(plTransforms), ptAppData->tGraphics.uFramesInFlight * pl_sb_size(ptAppData->tGltf.sbtMeshes));

    pl_sb_reserve(ptAppData->sbtDraws, pl_sb_size(ptAppData->tGltf.sbtMeshes));
    for(uint32_t i = 0; i < pl_sb_size(ptAppData->tGltf.sbtMeshes); i++)
    {
        pl_sb_push(ptAppData->sbtModelMats, pl_identity_mat4()); 
        pl_sb_push(ptAppData->sbtDraws, ((plDraw){
            .ptShader     = ptAppData->tGltf.sbtHasTangent[i] ? &ptAppData->tShader0 : &ptAppData->tShader1,
            .ptMesh       = &ptAppData->tGltf.sbtMeshes[i],
            .ptBindGroup2 = &ptAppData->tGltf.sbtBindGroups[i]
            }));
    }

    pl_sb_push(ptAppData->sbtDrawAreas, ((plDrawArea){
        .ptBindGroup0 = &ptAppData->tBindGroup0,
        .uDrawOffset  = 0,
        .uDrawCount   = pl_sb_size(ptAppData->tGltf.sbtMeshes)
    }));

    // create bind group layouts
    static plBufferBinding tBufferBindings[] = {
        {
            .tType       = PL_BUFFER_BINDING_TYPE_UNIFORM,
            .uSlot       = 0,
            .tStageFlags = VK_SHADER_STAGE_VERTEX_BIT
        }
    };

    static plBufferBinding tBufferBindings2[] = {
        {
            .tType       = PL_BUFFER_BINDING_TYPE_STORAGE,
            .uSlot       = 0,
            .tStageFlags = VK_SHADER_STAGE_VERTEX_BIT
        }
    };

    static plTextureBinding tTextureBindings[] = {
        {
            .tType       = PL_TEXTURE_BINDING_TYPE_SAMPLED,
            .uSlot       = 1,
            .tStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        }
    };

    static plBindGroupLayout atGroupLayouts[] = {
        {
            .uBufferCount = 1,
            .aBuffers      = tBufferBindings,
            .uTextureCount = 0
        },
        {
            .uBufferCount = 1,
            .aBuffers      = tBufferBindings2,
            .uTextureCount = 1,
            .aTextures     = tTextureBindings
        }
    };

    pl_create_bind_group(&ptAppData->tGraphics, &atGroupLayouts[0], &ptAppData->tBindGroup0);
    pl_create_bind_group(&ptAppData->tGraphics, &atGroupLayouts[1], NULL);

    // create shaders
    plShaderDesc tShaderDesc = {
        ._tRenderPass                       = ptAppData->tGraphics.tRenderPass,
        .pcPixelShader                      = "simple.frag.spv",
        .pcVertexShader                     = "simple.vert.spv",
        .tMeshFormatFlags0                  = PL_MESH_FORMAT_FLAG_HAS_POSITION,
        .tMeshFormatFlags1                  = PL_MESH_FORMAT_FLAG_HAS_NORMAL | PL_MESH_FORMAT_FLAG_HAS_TEXCOORD | PL_MESH_FORMAT_FLAG_HAS_TANGENT,
        .tGraphicsState.bDepthEnabled       = true,
        .tGraphicsState.bFrontFaceClockWise = false,
        .tGraphicsState.fDepthBias          = 0.0f,
        .tGraphicsState.tBlendMode          = PL_BLEND_MODE_ALPHA,
        .tGraphicsState.tCullMode           = VK_CULL_MODE_BACK_BIT,
        .atBindGroupLayouts                 = atGroupLayouts,
        .uBindGroupLayoutCount              = 2
    };
    pl_create_shader(&ptAppData->tGraphics, &tShaderDesc, &ptAppData->tShader0);
    tShaderDesc.tMeshFormatFlags1 = PL_MESH_FORMAT_FLAG_HAS_NORMAL | PL_MESH_FORMAT_FLAG_HAS_TEXCOORD;
    pl_create_shader(&ptAppData->tGraphics, &tShaderDesc, &ptAppData->tShader1);

    // just to reuse the above
    tBufferBindings[0].tBuffer = ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->ulConstantBuffer];
    tBufferBindings[0].szSize  = sizeof(plTransforms);

    plBindUpdate tBindUpdate = {
        .uBufferCount = 1,
        .aBuffers      = tBufferBindings
    };
    pl_update_bind_group(&ptAppData->tGraphics, &ptAppData->tBindGroup0, &tBindUpdate);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_shutdown(plAppData* ptAppData)
{
    vkDeviceWaitIdle(ptAppData->tGraphics.tDevice.tLogicalDevice);

    for(uint32_t i = 0; i < pl_sb_size(ptAppData->tGltf.sbtBindGroups); i++)
    {
        vkDestroyDescriptorSetLayout(ptAppData->tGraphics.tDevice.tLogicalDevice, ptAppData->tGltf.sbtBindGroups[i].tLayout._tDescriptorSetLayout, NULL);
    }

    pl_cleanup_font_atlas(&ptAppData->fontAtlas);
    pl_cleanup_draw_context(&ptAppData->ctx);
    pl_cleanup_shader(&ptAppData->tGraphics, &ptAppData->tShader0);
    pl_cleanup_shader(&ptAppData->tGraphics, &ptAppData->tShader1);
    pl_cleanup_graphics(&ptAppData->tGraphics);
    pl_cleanup_profile_context();
    pl_cleanup_extension_registry();
    pl_cleanup_log_context();
    pl_cleanup_data_registry();
    pl_cleanup_memory_context();
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_resize(plAppData* ptAppData)
{
    plIOContext* ptIOCtx = pl_get_io_context();
    pl_resize_graphics(&ptAppData->tGraphics);
    pl_camera_set_aspect(&ptAppData->tCamera, ptIOCtx->afMainViewportSize[0] / ptIOCtx->afMainViewportSize[1]);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(plAppData* ptAppData)
{
    pl_begin_profile_frame(ptAppData->ctx.frameCount);
    plIOContext* ptIOCtx = pl_get_io_context();
    pl_handle_extension_reloads();
    pl_new_io_frame();
    pl_new_draw_frame(&ptAppData->ctx);
    pl_process_cleanup_queue(&ptAppData->tGraphics.tResourceManager, 1);

    plFrameContext* ptCurrentFrame = pl_get_frame_resources(&ptAppData->tGraphics);

    if(pl_begin_frame(&ptAppData->tGraphics))
    {
        pl_begin_recording(&ptAppData->tGraphics);

        pl_begin_main_pass(&ptAppData->tGraphics);

        static const float fCameraTravelSpeed = 8.0f;
        if(pl_is_key_pressed(PL_KEY_W, true)) { pl_camera_translate(&ptAppData->tCamera,  0.0f,  0.0f,  fCameraTravelSpeed * ptIOCtx->fDeltaTime); }
        if(pl_is_key_pressed(PL_KEY_S, true)) { pl_camera_translate(&ptAppData->tCamera,  0.0f,  0.0f, -fCameraTravelSpeed* ptIOCtx->fDeltaTime); }
        if(pl_is_key_pressed(PL_KEY_A, true)) { pl_camera_translate(&ptAppData->tCamera, -fCameraTravelSpeed * ptIOCtx->fDeltaTime,  0.0f,  0.0f); }
        if(pl_is_key_pressed(PL_KEY_D, true)) { pl_camera_translate(&ptAppData->tCamera,  fCameraTravelSpeed * ptIOCtx->fDeltaTime,  0.0f,  0.0f); }
        if(pl_is_key_pressed(PL_KEY_F, true)) { pl_camera_translate(&ptAppData->tCamera,  0.0f, -fCameraTravelSpeed * ptIOCtx->fDeltaTime,  0.0f); }
        if(pl_is_key_pressed(PL_KEY_R, true)) { pl_camera_translate(&ptAppData->tCamera,  0.0f,  fCameraTravelSpeed * ptIOCtx->fDeltaTime,  0.0f); }

        if(pl_is_mouse_dragging(PL_MOUSE_BUTTON_LEFT, -0.0f))
        {
            const plVec2 tMouseDelta = pl_get_mouse_drag_delta(PL_MOUSE_BUTTON_LEFT, -0.0f);
            pl_camera_rotate(&ptAppData->tCamera,  -tMouseDelta.y * 0.1f * ptIOCtx->fDeltaTime,  -tMouseDelta.x * 0.1f * ptIOCtx->fDeltaTime);
            pl_reset_mouse_drag_delta(PL_MOUSE_BUTTON_LEFT);
        }

        uint32_t uCurrentDraw = 0;

        for(uint32_t i = 0; i < pl_sb_size(ptAppData->tGltf.sbtMeshes); i++)
        {
            const uint32_t uConstantBufferOffset = pl_sb_size(ptAppData->tGltf.sbtMeshes) * sizeof(plTransforms) * (uint32_t)ptAppData->tGraphics.szCurrentFrameIndex + uCurrentDraw * sizeof(plTransforms);

            plTransforms tTransforms = {
                .tModel     = ptAppData->sbtModelMats[i],
                .tModelView = pl_mul_mat4(&ptAppData->tCamera.tViewMat, &ptAppData->sbtModelMats[i])
            };
            tTransforms.tModelViewProjection = pl_mul_mat4(&ptAppData->tCamera.tProjMat, &tTransforms.tModelView);
            memcpy(&ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->ulConstantBuffer].pucMapping[uConstantBufferOffset], &tTransforms, sizeof(plTransforms));

            ptAppData->sbtDraws[uCurrentDraw].uDynamicBufferOffset0 = uConstantBufferOffset;
            uCurrentDraw++;
        }


        pl_draw_areas(&ptAppData->tGraphics, pl_sb_size(ptAppData->sbtDrawAreas), ptAppData->sbtDrawAreas, ptAppData->sbtDraws);

        ptAppData->ptDrawExtApi->pl_add_text(ptAppData->fgDrawLayer, &ptAppData->fontAtlas.sbFonts[0], 13.0f, (plVec2){100.0f, 100.0f}, (plVec4){1.0f, 1.0f, 0.0f, 1.0f}, "extension baby");

        // draw profiling info
        static char pcDeltaTime[64] = {0};
        pl_sprintf(pcDeltaTime, "%.6f ms", ptIOCtx->fDeltaTime);
        pl_add_text(ptAppData->fgDrawLayer, &ptAppData->fontAtlas.sbFonts[0], 13.0f, (plVec2){10.0f, 10.0f}, (plVec4){1.0f, 1.0f, 0.0f, 1.0f}, pcDeltaTime, 0.0f);

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
        pl_submit_drawlist_vulkan(&ptAppData->drawlist, (float)ptIOCtx->afMainViewportSize[0], (float)ptIOCtx->afMainViewportSize[1], ptCurrentFrame->tCmdBuf, (uint32_t)ptAppData->tGraphics.szCurrentFrameIndex);

        pl_end_main_pass(&ptAppData->tGraphics);
        pl_end_recording(&ptAppData->tGraphics);
        pl_end_frame(&ptAppData->tGraphics);
    }
    pl_end_io_frame();
    pl_end_profile_frame();
}
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
#include "stb_image.h"

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
    uint32_t        uGlobalConstantBuffer;
    uint32_t        uMaterialConstantBuffer;
    uint32_t        ulColorTexture;

    // grass
    uint32_t        uGrassTexture;
    plBindGroup     tGrassBindGroup0;
    plBindGroup     tGrassBindGroup2;
    plMesh          tGrassMesh;
    plDraw          tGrassDraw;
    uint32_t        uGrassConstantBuffer;
    uint32_t        uGrassStorageBuffer;

    // new
    plShader     tShader0;
    plShader     tShader1;
    plBindGroup  tBindGroup0; // global
    plBindGroup  tBindGroup1; // material
    plDraw*      sbtDraws;
    plDrawArea*  sbtDrawAreas;
    plGltf       tGltf;
} plAppData;

typedef struct _plGlobalInfo
{
    plVec4 tAmbientColor;
    plVec4 tCameraPos;
    plMat4 tCameraView;
    plMat4 tCameraViewProj;
} plGlobalInfo;

typedef struct _plMaterialInfo
{
    uint32_t uVertexStride;
    int      _unused[3];
} plMaterialInfo;


void pl_load_grass(plAppData* ptData, uint32_t uRows, uint32_t uColumns, float fSpacing);

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
    ptAppData->tCamera = pl_create_perspective_camera((plVec3){0.0f, 2.0f, 8.5f}, PL_PI_3, ptIOCtx->afMainViewportSize[0] / ptIOCtx->afMainViewportSize[1], 0.1f, 100.0f);

    // pl_ext_load_gltf(&ptAppData->tGraphics, "../data/glTF-Sample-Models-master/2.0/Sponza/glTF/Sponza.gltf", &ptAppData->tGltf);
    // pl_ext_load_gltf(&ptAppData->tGraphics, "../data/glTF-Sample-Models-master/2.0/DamagedHelmet/glTF/DamagedHelmet.gltf", &ptAppData->tGltf);
    // pl_ext_load_gltf(&ptAppData->tGraphics, "../data/glTF-Sample-Models-master/2.0/FlightHelmet/glTF/FlightHelmet.gltf", &ptAppData->tGltf);
    pl_ext_load_gltf(&ptAppData->tGraphics, "./tree.gltf", &ptAppData->tGltf);
    pl_load_grass(ptAppData, 500, 500, 0.25f);

    ptAppData->uGlobalConstantBuffer   = pl_create_constant_buffer(&ptAppData->tGraphics.tResourceManager, sizeof(plGlobalInfo), ptAppData->tGraphics.uFramesInFlight);
    ptAppData->uMaterialConstantBuffer = pl_create_constant_buffer(&ptAppData->tGraphics.tResourceManager, sizeof(plMaterialInfo), ptAppData->tGraphics.uFramesInFlight * 1);

    pl_sb_reserve(ptAppData->sbtDraws, pl_sb_size(ptAppData->tGltf.sbtMeshes) + 1);
    for(uint32_t i = 0; i < pl_sb_size(ptAppData->tGltf.sbtMeshes); i++)
    {
        pl_sb_push(ptAppData->sbtDraws, ((plDraw){
            .ptShader     = &ptAppData->tShader0,
            .ptMesh       = &ptAppData->tGltf.sbtMeshes[i],
            .ptBindGroup1 = &ptAppData->tBindGroup1,
            .ptBindGroup2 = &ptAppData->tGltf.sbtBindGroups[i]
            }));
    }

    pl_sb_push(ptAppData->sbtDrawAreas, ((plDrawArea){
        .ptBindGroup0 = &ptAppData->tBindGroup0,
        .uDrawOffset  = 0,
        .uDrawCount   = pl_sb_size(ptAppData->tGltf.sbtMeshes)
    }));

    pl_sb_push(ptAppData->sbtDrawAreas, ((plDrawArea){
        .ptBindGroup0 = &ptAppData->tGrassBindGroup0,
        .uDrawOffset  = pl_sb_size(ptAppData->sbtDraws),
        .uDrawCount   = 1
    }));

    pl_sb_push(ptAppData->sbtDraws, ((plDraw){
        .ptShader     = &ptAppData->tShader1,
        .ptMesh       = &ptAppData->tGrassMesh,
        .ptBindGroup1 = &ptAppData->tBindGroup1,
        .ptBindGroup2 = &ptAppData->tGrassBindGroup2
        }));

    // create bind group layouts
    static plBufferBinding tGlobalBufferBindings[] = {
        {
            .tType       = PL_BUFFER_BINDING_TYPE_UNIFORM,
            .uSlot       = 0,
            .tStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
        },
        {
            .tType       = PL_BUFFER_BINDING_TYPE_STORAGE,
            .uSlot       = 1,
            .tStageFlags = VK_SHADER_STAGE_VERTEX_BIT
        }
    };

    static plBufferBinding tMaterialBufferBindings[] = {
        {
            .tType       = PL_BUFFER_BINDING_TYPE_UNIFORM,
            .uSlot       = 0,
            .tStageFlags = VK_SHADER_STAGE_VERTEX_BIT
        },
    };

    static plBufferBinding tObjectBufferBindings[] = {
        {
            .tType       = PL_BUFFER_BINDING_TYPE_UNIFORM,
            .uSlot       = 0,
            .tStageFlags = VK_SHADER_STAGE_VERTEX_BIT
        },
    };

    static plTextureBinding tObjectTextureBindings[] = {
        {
            .tType       = PL_TEXTURE_BINDING_TYPE_SAMPLED,
            .uSlot       = 1,
            .tStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        }
    };

    static plBindGroupLayout atGroupLayouts[] = {
        {
            .uBufferCount = 2,
            .aBuffers      = tGlobalBufferBindings,
            .uTextureCount = 0
        },
        {
            .uBufferCount = 1,
            .aBuffers      = tMaterialBufferBindings
        },
        {
            .uBufferCount = 1,
            .aBuffers      = tObjectBufferBindings,
            .uTextureCount = 1,
            .aTextures     = tObjectTextureBindings
        }
    };

    pl_create_bind_group(&ptAppData->tGraphics, &atGroupLayouts[0], &ptAppData->tGrassBindGroup0);
    pl_create_bind_group(&ptAppData->tGraphics, &atGroupLayouts[0], &ptAppData->tBindGroup0);
    pl_create_bind_group(&ptAppData->tGraphics, &atGroupLayouts[1], &ptAppData->tBindGroup1);
    pl_create_bind_group(&ptAppData->tGraphics, &atGroupLayouts[2], NULL);

    // create shaders
    plShaderDesc tShaderDesc = {
        ._tRenderPass                       = ptAppData->tGraphics.tRenderPass,
        .pcPixelShader                      = "phong.frag.spv",
        .pcVertexShader                     = "phong.vert.spv",
        .tMeshFormatFlags0                  = PL_MESH_FORMAT_FLAG_HAS_POSITION,
        .tMeshFormatFlags1                  = PL_MESH_FORMAT_FLAG_HAS_NORMAL | PL_MESH_FORMAT_FLAG_HAS_TEXCOORD_0 | PL_MESH_FORMAT_FLAG_HAS_TANGENT,
        .tGraphicsState.bDepthEnabled       = true,
        .tGraphicsState.bFrontFaceClockWise = false,
        .tGraphicsState.fDepthBias          = 0.0f,
        .tGraphicsState.tBlendMode          = PL_BLEND_MODE_ALPHA,
        .tGraphicsState.tCullMode           = VK_CULL_MODE_BACK_BIT,
        .atBindGroupLayouts                 = atGroupLayouts,
        .uBindGroupLayoutCount              = 3
    };
    pl_create_shader(&ptAppData->tGraphics, &tShaderDesc, &ptAppData->tShader0);
    tShaderDesc.tMeshFormatFlags1 = PL_MESH_FORMAT_FLAG_HAS_NORMAL | PL_MESH_FORMAT_FLAG_HAS_TEXCOORD_0;
    tShaderDesc.tGraphicsState.tCullMode = VK_CULL_MODE_NONE,
    pl_create_shader(&ptAppData->tGraphics, &tShaderDesc, &ptAppData->tShader1);

    // just to reuse the above
    tGlobalBufferBindings[0].tBuffer = ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->uGlobalConstantBuffer];
    tGlobalBufferBindings[0].szSize  = sizeof(plGlobalInfo);
    tGlobalBufferBindings[1].tBuffer = ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->tGltf.uStorageBuffer];
    tGlobalBufferBindings[1].szSize  = ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->tGltf.uStorageBuffer].szSize;

    plBindUpdate tBindUpdate = {
        .uBufferCount = 2,
        .aBuffers      = tGlobalBufferBindings
    };
    pl_update_bind_group(&ptAppData->tGraphics, &ptAppData->tBindGroup0, &tBindUpdate);

    tGlobalBufferBindings[1].tBuffer = ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->uGrassStorageBuffer];
    tGlobalBufferBindings[1].szSize  = ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->uGrassStorageBuffer].szSize;

    pl_update_bind_group(&ptAppData->tGraphics, &ptAppData->tGrassBindGroup0, &tBindUpdate);

    tMaterialBufferBindings[0].tBuffer = ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->uMaterialConstantBuffer];
    tMaterialBufferBindings[0].szSize  = sizeof(plMaterialInfo);

    tBindUpdate.uBufferCount = 1;
    tBindUpdate.aBuffers = tMaterialBufferBindings;
    pl_update_bind_group(&ptAppData->tGraphics, &ptAppData->tBindGroup1, &tBindUpdate);
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
    vkDestroyDescriptorSetLayout(ptAppData->tGraphics.tDevice.tLogicalDevice, ptAppData->tGrassBindGroup2.tLayout._tDescriptorSetLayout, NULL);

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

        // update global constant buffer
        const plGlobalInfo tGlobalInfo = {
            .tAmbientColor = {0.1f, 0.0f, 0.0f, 1.0f},
            .tCameraPos    = {.xyz = ptAppData->tCamera.tPos, .w = 0.0f},
            .tCameraView   = ptAppData->tCamera.tViewMat,
            .tCameraViewProj = pl_mul_mat4(&ptAppData->tCamera.tProjMat, &ptAppData->tCamera.tViewMat)
        };

        const plBuffer* ptGlobalConstBuffer = &ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->uGlobalConstantBuffer];
        const uint32_t uConstantBufferOffset0 = (uint32_t)ptGlobalConstBuffer->szStride * (uint32_t)ptAppData->tGraphics.szCurrentFrameIndex;
        memcpy(&ptGlobalConstBuffer->pucMapping[uConstantBufferOffset0], &tGlobalInfo, sizeof(plGlobalInfo));
        ptAppData->sbtDrawAreas[0].uDynamicBufferOffset0 = uConstantBufferOffset0;
        ptAppData->sbtDrawAreas[1].uDynamicBufferOffset0 = uConstantBufferOffset0;

        // update material constant buffer
        const plMaterialInfo tMaterialInfo = {
            .uVertexStride = 2
        };
        const plBuffer* ptMaterialConstBuffer = &ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->uMaterialConstantBuffer];
        const uint32_t uConstantBufferOffset1 = (uint32_t)ptMaterialConstBuffer->szStride * (uint32_t)ptAppData->tGraphics.szCurrentFrameIndex;
        memcpy(&ptMaterialConstBuffer->pucMapping[uConstantBufferOffset1], &tMaterialInfo, sizeof(plMaterialInfo));

        uint32_t uCurrentDraw = 0;
        {
            const plBuffer* ptObjectConstBuffer = &ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->tGltf.uObjectConstantBuffer];
            const uint32_t uConstantBufferOffset2Base = pl_sb_size(ptAppData->tGltf.sbtMeshes) * (uint32_t)ptObjectConstBuffer->szStride * (uint32_t)ptAppData->tGraphics.szCurrentFrameIndex;
            
            for(uint32_t i = 0; i < pl_sb_size(ptAppData->tGltf.sbtMeshes); i++)
            {
                const uint32_t uConstantBufferOffset2 = uConstantBufferOffset2Base + uCurrentDraw * (uint32_t)ptObjectConstBuffer->szStride;
                ptAppData->sbtDraws[uCurrentDraw].uDynamicBufferOffset1 = uConstantBufferOffset1;
                ptAppData->sbtDraws[uCurrentDraw].uDynamicBufferOffset2 = uConstantBufferOffset2;
                uCurrentDraw++;
            }
        }
        {
            const plBuffer* ptObjectConstBuffer = &ptAppData->tGraphics.tResourceManager.sbtBuffers[ptAppData->uGrassConstantBuffer];
            const uint32_t uConstantBufferOffset2 = (uint32_t)ptObjectConstBuffer->szStride * (uint32_t)ptAppData->tGraphics.szCurrentFrameIndex;
            ptAppData->sbtDraws[uCurrentDraw].uDynamicBufferOffset1 = uConstantBufferOffset1;
            ptAppData->sbtDraws[uCurrentDraw].uDynamicBufferOffset2 = uConstantBufferOffset2;
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

void
pl_load_grass(plAppData* ptData, uint32_t uRows, uint32_t uColumns, float fSpacing)
{

    const uint32_t uGrassObjects = uRows * uColumns;
    const float fHeight = 1.0f;
    
    plVec3* sbtVertexBuffer = NULL;
    uint32_t* sbtIndexBuffer = NULL;
    plVec4* sbtStorageBuffer = NULL;



    pl_sb_resize(sbtVertexBuffer, 12 * uGrassObjects);
    pl_sb_resize(sbtStorageBuffer, 2 * 12 * uGrassObjects);
    pl_sb_resize(sbtIndexBuffer, 6 * 3 * uGrassObjects);

    plVec3 tCenterPoint = {(float)uColumns * fSpacing / 2.0f, 0.0f, -(float)uRows * fSpacing / 2.0f};

    const plVec3 atPositionTemplate[12] = {

        // first quad
        {
            .x = -0.5f,
            .y =  fHeight,
            .z =  0.0f
        },
        {
            .x = 0.5f,
            .y = fHeight,
            .z = 0.0f
        },
        {
            .x = 0.5f,
            .y = 0.0f,
            .z = 0.0f
        },
        {
            .x = -0.5f,
            .y =  0.0f,
            .z =  0.0f
        },

        // second quad
        {
            .x = -0.35f,
            .y =  1.0f,
            .z =  -0.35f
        },
        {
            .x = 0.35f,
            .y = fHeight,
            .z = 0.35f
        },
        {
            .x = 0.35f,
            .y = 0.0f,
            .z = 0.35f
        },
        {
            .x = -0.35f,
            .y =  0.0f,
            .z = -0.35f
        },

        // third quad
        {
            .x = -0.35f,
            .y =  fHeight,
            .z =  0.35f
        },
        {
            .x = 0.35f,
            .y = fHeight,
            .z = -0.35f
        },
        {
            .x = 0.35f,
            .y = 0.0f,
            .z = -0.35f
        },
        {
            .x = -0.35f,
            .y =  0.0f,
            .z =  0.35f
        }
    };

    for(uint32_t uGrassIndex = 0; uGrassIndex < uGrassObjects; uGrassIndex++)
    {
        const uint32_t uCurrentIndex = uGrassIndex * 18;
        const uint32_t uCurrentVertex = uGrassIndex * 12;
        const uint32_t uCurrentStorage = uGrassIndex * 24;

        sbtIndexBuffer[uCurrentIndex + 0] = uCurrentVertex + 0;
        sbtIndexBuffer[uCurrentIndex + 1] = uCurrentVertex + 3;
        sbtIndexBuffer[uCurrentIndex + 2] = uCurrentVertex + 2;
        sbtIndexBuffer[uCurrentIndex + 3] = uCurrentVertex + 0;
        sbtIndexBuffer[uCurrentIndex + 4] = uCurrentVertex + 2;
        sbtIndexBuffer[uCurrentIndex + 5] = uCurrentVertex + 1;

        sbtIndexBuffer[uCurrentIndex + 6] = uCurrentVertex + 4;
        sbtIndexBuffer[uCurrentIndex + 7] = uCurrentVertex + 7;
        sbtIndexBuffer[uCurrentIndex + 8] = uCurrentVertex + 6;
        sbtIndexBuffer[uCurrentIndex + 9] = uCurrentVertex + 4;
        sbtIndexBuffer[uCurrentIndex + 10] = uCurrentVertex + 6;
        sbtIndexBuffer[uCurrentIndex + 11] = uCurrentVertex + 5;

        sbtIndexBuffer[uCurrentIndex + 12] = uCurrentVertex + 8;
        sbtIndexBuffer[uCurrentIndex + 13] = uCurrentVertex + 11;
        sbtIndexBuffer[uCurrentIndex + 14] = uCurrentVertex + 10;
        sbtIndexBuffer[uCurrentIndex + 15] = uCurrentVertex + 8;
        sbtIndexBuffer[uCurrentIndex + 16] = uCurrentVertex + 10;
        sbtIndexBuffer[uCurrentIndex + 17] = uCurrentVertex + 9;

        // normals
        for(uint32_t i = 0; i < 12; i++)
            sbtStorageBuffer[uCurrentStorage + i * 2].y = 1.0f;

        // texture coordinates
        for(uint32_t i = 0; i < 3; i++)
        {
            sbtStorageBuffer[uCurrentStorage + i * 8 + 1].x = 0.0f;
            sbtStorageBuffer[uCurrentStorage + i * 8 + 1].y = 0.0f;
            sbtStorageBuffer[uCurrentStorage + i * 8 + 3].x = 1.0f;
            sbtStorageBuffer[uCurrentStorage + i * 8 + 3].y = 0.0f;
            sbtStorageBuffer[uCurrentStorage + i * 8 + 5].x = 1.0f;
            sbtStorageBuffer[uCurrentStorage + i * 8 + 5].y = 1.0f;
            sbtStorageBuffer[uCurrentStorage + i * 8 + 7].x = 0.0f;
            sbtStorageBuffer[uCurrentStorage + i * 8 + 7].y = 1.0f;
        }
        
        for(uint32_t i = 0; i < 3; i++)
        {
            // normals
            sbtStorageBuffer[uCurrentStorage + i * 8].x = 0.0f;
            sbtStorageBuffer[uCurrentStorage + i * 8].y = 1.0f;
            sbtStorageBuffer[uCurrentStorage + i * 8].z = 0.0f;
            sbtStorageBuffer[uCurrentStorage + i * 8].w = 0.0f;

        }

        memcpy(&sbtVertexBuffer[uCurrentVertex], atPositionTemplate, sizeof(plVec3) * 12);

    }

    uint32_t uCurrentVertex = 0;
    for(uint32_t i = 0; i < uRows; i++)
    {
        for(uint32_t j = 0; j < uColumns; j++)
        {
            const plVec3 tTranslation = pl_add_vec3(tCenterPoint, 
                (plVec3){
                    (float)j * -fSpacing + frandom(fSpacing * 8.0f) - fSpacing * 4.0f, 
                    -4.0f*cosf((float)j * 5.0f / (float)uColumns) + 4.0f*cosf((float)i * 4.0f / (float)uRows), 
                    // frandom(fHeight * 0.5f) - fHeight * 0.25f, 
                    i * fSpacing + frandom(fSpacing * 8.0f) - fSpacing * 4.0f
                    });

            for(uint32_t k = 0; k < 12; k++)
            {
                sbtVertexBuffer[uCurrentVertex + k] = pl_add_vec3(sbtVertexBuffer[uCurrentVertex + k], tTranslation);
            }
            uCurrentVertex += 12;
        }
    }


    ptData->uGrassConstantBuffer = pl_create_constant_buffer(&ptData->tGraphics.tResourceManager, sizeof(plObjectInfo), ptData->tGraphics.uFramesInFlight);

    // create bind group layouts
    plBufferBinding tBufferBindings[] = {
        {
            .tType       = PL_BUFFER_BINDING_TYPE_UNIFORM,
            .uSlot       = 0,
            .tStageFlags = VK_SHADER_STAGE_VERTEX_BIT
        }
    };

    plTextureBinding tTextureBindings[] = {
        {
            .tType       = PL_TEXTURE_BINDING_TYPE_SAMPLED,
            .uSlot       = 1,
            .tStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        }
    };
    plBindGroupLayout tGroupLayout0 = {

        .uBufferCount = 1,
        .aBuffers      = tBufferBindings,

        .uTextureCount = 1,
        .aTextures     = tTextureBindings
    };
    pl_create_bind_group(&ptData->tGraphics, &tGroupLayout0, &ptData->tGrassBindGroup2);

    int texWidth, texHeight, texNumChannels;
    int texForceNumChannels = 4;
    unsigned char* rawBytes = stbi_load("grass.png", &texWidth, &texHeight, &texNumChannels, texForceNumChannels);
    PL_ASSERT(rawBytes);

    const plTextureDesc tTextureDesc = {
        .tDimensions = {.x = (float)texWidth, .y = (float)texHeight, .z = 1.0f},
        .tFormat     = VK_FORMAT_R8G8B8A8_UNORM,
        .tUsage      = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .uLayers     = 1,
        .uMips       = 0, // means all mips
        .tType       = VK_IMAGE_TYPE_2D
    };
    ptData->uGrassTexture  = pl_create_texture(&ptData->tGraphics.tResourceManager, tTextureDesc, sizeof(unsigned char) * texHeight * texHeight * 4, rawBytes);

    // just to reuse the above
    tBufferBindings[0].tBuffer   = ptData->tGraphics.tResourceManager.sbtBuffers[ptData->uGrassConstantBuffer];
    tBufferBindings[0].szSize    = ptData->tGraphics.tResourceManager.sbtBuffers[ptData->uGrassConstantBuffer].szStride;
    tTextureBindings[0].tTexture = ptData->tGraphics.tResourceManager.sbtTextures[ptData->uGrassTexture];
    tTextureBindings[0].tSampler = ptData->tGraphics.tResourceManager.sbtTextures[ptData->uGrassTexture].tSampler;

    plBindUpdate tBindUpdate = {
        .uBufferCount  = 1,
        .aBuffers      = tBufferBindings,
        .uTextureCount = 1,
        .aTextures     = tTextureBindings
    };
    pl_update_bind_group(&ptData->tGraphics, &ptData->tGrassBindGroup2, &tBindUpdate);

    ptData->tGrassMesh.uIndexCount   = (uint32_t)pl_sb_size(sbtIndexBuffer);
    ptData->tGrassMesh.uVertexCount  = (uint32_t)pl_sb_size(sbtVertexBuffer);
    ptData->tGrassMesh.uIndexBuffer  = pl_create_index_buffer(&ptData->tGraphics.tResourceManager, sizeof(uint32_t) * pl_sb_size(sbtIndexBuffer), sbtIndexBuffer);
    ptData->tGrassMesh.uVertexBuffer = pl_create_vertex_buffer(&ptData->tGraphics.tResourceManager, sizeof(plVec3) * pl_sb_size(sbtVertexBuffer), sizeof(plVec3), sbtVertexBuffer);

    ptData->uGrassStorageBuffer = pl_create_storage_buffer(&ptData->tGraphics.tResourceManager, pl_sb_size(sbtStorageBuffer) * sizeof(plVec4), sbtStorageBuffer);


    const plBuffer* ptObjectConstBuffer = &ptData->tGraphics.tResourceManager.sbtBuffers[ptData->uGrassConstantBuffer];

    for(uint32_t j = 0; j < ptData->tGraphics.uFramesInFlight; j++)
    {
        const uint32_t uConstantBufferOffset2Base = (uint32_t)ptObjectConstBuffer->szStride * j;

        const plObjectInfo tObjectInfo = {
            .tModel        = pl_mat4_translate_xyz(0.0f, 0.0f, 0.0f),
            .uVertexOffset = 0
        };
        memcpy(&ptObjectConstBuffer->pucMapping[uConstantBufferOffset2Base], &tObjectInfo, sizeof(plObjectInfo));
    }

    pl_sb_free(sbtVertexBuffer);
    pl_sb_free(sbtStorageBuffer);
    pl_sb_free(sbtIndexBuffer);
}
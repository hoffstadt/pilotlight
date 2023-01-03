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
#include "pl_registry.h"
#include "pl_ext.h"
#include "pl_ui.h"
#include "pl_renderer.h"

// extensions
#include "pl_draw_extension.h"
#include "pl_stl_ext.h"
#include "stb_image.h"

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plAppData
{
    plGraphics          tGraphics;
    plDrawContext       tCtx;
    plDrawList          drawlist;
    plDrawLayer*        fgDrawLayer;
    plDrawLayer*        bgDrawLayer;
    plFontAtlas         fontAtlas;
    plProfileContext    tProfileCtx;
    plLogContext        tLogCtx;
    plMemoryContext     tMemoryCtx;
    plDataRegistry      tDataRegistryCtx;
    plExtensionRegistry tExtensionRegistryCtx;
    plCamera            tCamera;
    plUiContext         tUiContext;

    // extension apis
    plDrawExtension* ptDrawExtApi;

    // renderer
    plRenderer      tRenderer;
    plAssetRegistry tAssetRegistry;

    // materials
    uint32_t uGrassMaterial;
    uint32_t uStlMaterial;

    // stl
    plBindGroup     tStlBindGroup2;
    plMesh          tStlMesh;
    plDraw          tStlDraw;
    uint32_t        uStlConstantBuffer;

    // grass
    uint32_t        uGrassTexture;
    plBindGroup     tGrassBindGroup2;
    plMesh          tGrassMesh;
    plDraw          tGrassDraw;
    uint32_t        uGrassConstantBuffer;

} plAppData;

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
    pl_register_data("draw", &tPNewData->tCtx);

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
    pl_initialize_draw_context_vulkan(&ptAppData->tCtx, ptAppData->tGraphics.tDevice.tPhysicalDevice, ptAppData->tGraphics.tSwapchain.uImageCount, ptAppData->tGraphics.tDevice.tLogicalDevice);
    pl_register_drawlist(&ptAppData->tCtx, &ptAppData->drawlist);
    pl_setup_drawlist_vulkan(&ptAppData->drawlist, ptAppData->tGraphics.tRenderPass, ptAppData->tGraphics.tSwapchain.tMsaaSamples);
    ptAppData->bgDrawLayer = pl_request_draw_layer(&ptAppData->drawlist, "Background Layer");
    ptAppData->fgDrawLayer = pl_request_draw_layer(&ptAppData->drawlist, "Foreground Layer");

    // create font atlas
    pl_add_default_font(&ptAppData->fontAtlas);
    pl_build_font_atlas(&ptAppData->tCtx, &ptAppData->fontAtlas);

    // ui
    pl_ui_setup_context(&ptAppData->tCtx, &ptAppData->tUiContext);
    pl_setup_drawlist_vulkan(ptAppData->tUiContext.ptDrawlist, ptAppData->tGraphics.tRenderPass, ptAppData->tGraphics.tSwapchain.tMsaaSamples);
    ptAppData->tUiContext.ptFont = &ptAppData->fontAtlas.sbFonts[0];

    // renderer
    pl_setup_asset_registry(&ptAppData->tGraphics, &ptAppData->tAssetRegistry);
    pl_setup_renderer(&ptAppData->tGraphics, &ptAppData->tAssetRegistry, &ptAppData->tRenderer);

    // camera
    ptAppData->tCamera = pl_create_perspective_camera((plVec3){0.0f, 2.0f, 8.5f}, PL_PI_3, ptIOCtx->afMainViewportSize[0] / ptIOCtx->afMainViewportSize[1], 0.01f, 400.0f);

    // load grass
    pl_load_grass(ptAppData, 50, 50, 0.25f);
   
    // load stl
    const plStlOptions tStlOptions = {
        .bIncludeColor   = true,
        .bIncludeNormals = true,
        .tColor = { .r = 1.0f, .a = 1.0f}
    };
    plStlInfo tStlInfo = {0};
    uint32_t uFileSize = 0u;
    pl_read_file("../data/pilotlight-assets-master/meshes/monkey.stl", &uFileSize, NULL, "rb");
    char* acFileData = pl_alloc(uFileSize);
    memset(acFileData, 0, uFileSize);
    pl_read_file("../data/pilotlight-assets-master/meshes/monkey.stl", &uFileSize, acFileData, "rb");
    pl_load_stl(acFileData, uFileSize, tStlOptions, NULL, NULL, NULL, &tStlInfo);
    float* afVertexBuffer0    = pl_alloc(sizeof(float) * tStlInfo.szVertexStream0Size);
    float* afVertexBuffer1    = pl_alloc(sizeof(float) * tStlInfo.szVertexStream1Size);
    memset(afVertexBuffer1, 0, sizeof(float) * tStlInfo.szVertexStream1Size);
    uint32_t* auIndexBuffer = pl_alloc(sizeof(uint32_t) * tStlInfo.szIndexBufferSize);
    pl_load_stl(acFileData, uFileSize, tStlOptions, afVertexBuffer0, afVertexBuffer1, auIndexBuffer, &tStlInfo);

    ptAppData->uStlConstantBuffer = pl_create_constant_buffer(&ptAppData->tGraphics.tResourceManager, sizeof(plObjectInfo), ptAppData->tGraphics.uFramesInFlight);

    const plMat4 tStlRotation = pl_mat4_rotate_xyz(PL_PI, 0.0f, 1.0f, 0.0f);
    const plMat4 tStlTranslation = pl_mat4_translate_xyz(0.0f, 2.0f, 0.0f);
    const plMat4 tStlTransform = pl_mul_mat4(&tStlTranslation, &tStlRotation);
    for(uint32_t i = 0; i < ptAppData->tGraphics.uFramesInFlight; i++)
    {
        plObjectInfo* ptObjectInfo = pl_get_constant_buffer_data_ex(&ptAppData->tGraphics.tResourceManager, ptAppData->uStlConstantBuffer, (size_t)i, 0);
        ptObjectInfo->uVertexOffset = pl_sb_size(ptAppData->tRenderer.sbfStorageBuffer) / 4;
        ptObjectInfo->tModel = tStlTransform;
    }
    
    // create bind group layouts
    plBindGroupLayout tStlGroupLayout = {

        .uBufferCount = 1,
        .aBuffers      = {
            {
                .tType       = PL_BUFFER_BINDING_TYPE_UNIFORM,
                .uSlot       = 0,
                .tStageFlags = VK_SHADER_STAGE_VERTEX_BIT
            }
        },

        .uTextureCount = 1,
        .aTextures     = {
            {
                .tType       = PL_TEXTURE_BINDING_TYPE_SAMPLED,
                .uSlot       = 1,
                .tStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            }  
        }
    };

    pl_create_bind_group(&ptAppData->tGraphics, &tStlGroupLayout, &ptAppData->tStlBindGroup2, "stl bind group");
    pl_update_bind_group(&ptAppData->tGraphics, &ptAppData->tStlBindGroup2, 1, &ptAppData->uStlConstantBuffer, 1, &ptAppData->uGrassTexture);
    ptAppData->tStlMesh.uIndexCount   = (uint32_t)tStlInfo.szIndexBufferSize;
    ptAppData->tStlMesh.uVertexCount  = (uint32_t)tStlInfo.szVertexStream0Size / 3;
    ptAppData->tStlMesh.uIndexBuffer  = pl_create_index_buffer(&ptAppData->tGraphics.tResourceManager, sizeof(uint32_t) * tStlInfo.szIndexBufferSize, auIndexBuffer);
    ptAppData->tStlMesh.uVertexBuffer = pl_create_vertex_buffer(&ptAppData->tGraphics.tResourceManager, tStlInfo.szVertexStream0Size * sizeof(float), sizeof(plVec3), afVertexBuffer0);
    const uint32_t uPrevIndex = pl_sb_add_n(ptAppData->tRenderer.sbfStorageBuffer, (uint32_t)tStlInfo.szVertexStream1Size);
    memcpy(&ptAppData->tRenderer.sbfStorageBuffer[uPrevIndex], afVertexBuffer1, tStlInfo.szVertexStream1Size * sizeof(float));
    pl_free(acFileData);
    pl_free(afVertexBuffer0);
    pl_free(afVertexBuffer1);
    pl_free(auIndexBuffer);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~materials~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // grass material
    ptAppData->uGrassMaterial = pl_create_material(&ptAppData->tAssetRegistry, "grass");
    plMaterial* ptGrassMaterial = &ptAppData->tAssetRegistry.sbtMaterials[ptAppData->uGrassMaterial];
    ptGrassMaterial->bDoubleSided = true;
    ptGrassMaterial->uAlbedoMap = ptAppData->uGrassTexture;
    ptGrassMaterial->ulVertexStreamMask0 = PL_MESH_FORMAT_FLAG_HAS_POSITION;
    ptGrassMaterial->ulVertexStreamMask1 = PL_MESH_FORMAT_FLAG_HAS_NORMAL | PL_MESH_FORMAT_FLAG_HAS_TEXCOORD_0;

    // stl material
    ptAppData->uStlMaterial = pl_create_material(&ptAppData->tAssetRegistry, "stl");
    plMaterial* ptStlMaterial = &ptAppData->tAssetRegistry.sbtMaterials[ptAppData->uStlMaterial];
    ptStlMaterial->ulVertexStreamMask0 = PL_MESH_FORMAT_FLAG_HAS_POSITION;
    ptStlMaterial->ulVertexStreamMask1 = PL_MESH_FORMAT_FLAG_HAS_NORMAL | PL_MESH_FORMAT_FLAG_HAS_COLOR_0;

    // finalize
    pl_finalize_material(&ptAppData->tRenderer, ptAppData->uGrassMaterial);
    pl_finalize_material(&ptAppData->tRenderer, ptAppData->uStlMaterial);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_shutdown(plAppData* ptAppData)
{
    vkDeviceWaitIdle(ptAppData->tGraphics.tDevice.tLogicalDevice);
    pl_cleanup_font_atlas(&ptAppData->fontAtlas);
    pl_cleanup_draw_context(&ptAppData->tCtx);
    pl_ui_cleanup_context(&ptAppData->tUiContext);
    pl_cleanup_renderer(&ptAppData->tRenderer);
    pl_cleanup_asset_registry(&ptAppData->tAssetRegistry);
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
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~frame setup~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    pl_begin_profile_frame(ptAppData->tCtx.frameCount);
    plIOContext* ptIOCtx = pl_get_io_context();
    pl_handle_extension_reloads();
    pl_new_io_frame();
    pl_new_draw_frame(&ptAppData->tCtx);
    pl_ui_new_frame(&ptAppData->tUiContext);
    pl_process_cleanup_queue(&ptAppData->tGraphics.tResourceManager, 1);

    plFrameContext* ptCurrentFrame = pl_get_frame_resources(&ptAppData->tGraphics);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~begin frame~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if(pl_begin_frame(&ptAppData->tGraphics))
    {
        pl_begin_recording(&ptAppData->tGraphics);

        pl_begin_main_pass(&ptAppData->tGraphics);

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~input handling~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        static const float fCameraTravelSpeed = 8.0f;
        if(pl_is_key_pressed(PL_KEY_W, true)) pl_camera_translate(&ptAppData->tCamera,  0.0f,  0.0f,  fCameraTravelSpeed * ptIOCtx->fDeltaTime);
        if(pl_is_key_pressed(PL_KEY_S, true)) pl_camera_translate(&ptAppData->tCamera,  0.0f,  0.0f, -fCameraTravelSpeed* ptIOCtx->fDeltaTime);
        if(pl_is_key_pressed(PL_KEY_A, true)) pl_camera_translate(&ptAppData->tCamera, -fCameraTravelSpeed * ptIOCtx->fDeltaTime,  0.0f,  0.0f);
        if(pl_is_key_pressed(PL_KEY_D, true)) pl_camera_translate(&ptAppData->tCamera,  fCameraTravelSpeed * ptIOCtx->fDeltaTime,  0.0f,  0.0f);
        if(pl_is_key_pressed(PL_KEY_F, true)) pl_camera_translate(&ptAppData->tCamera,  0.0f, -fCameraTravelSpeed * ptIOCtx->fDeltaTime,  0.0f);
        if(pl_is_key_pressed(PL_KEY_R, true)) pl_camera_translate(&ptAppData->tCamera,  0.0f,  fCameraTravelSpeed * ptIOCtx->fDeltaTime,  0.0f);

        if(pl_is_mouse_dragging(PL_MOUSE_BUTTON_LEFT, -0.0f))
        {
            const plVec2 tMouseDelta = pl_get_mouse_drag_delta(PL_MOUSE_BUTTON_LEFT, -0.0f);
            pl_camera_rotate(&ptAppData->tCamera,  -tMouseDelta.y * 0.1f * ptIOCtx->fDeltaTime,  -tMouseDelta.x * 0.1f * ptIOCtx->fDeltaTime);
            pl_reset_mouse_drag_delta(PL_MOUSE_BUTTON_LEFT);
        }

        //~~~~~~~~~~~~~~~~~~~~~~~~~~update constant buffers~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        // update global constant buffer
        plGlobalInfo* ptGlobalInfo = pl_get_constant_buffer_data(&ptAppData->tGraphics.tResourceManager, ptAppData->tRenderer.uGlobalConstantBuffer, 0);
        ptGlobalInfo->tAmbientColor   = (plVec4){0.1f, 0.0f, 0.0f, 1.0f};
        ptGlobalInfo->tCameraPos      = (plVec4){.xyz = ptAppData->tCamera.tPos, .w = 0.0f};
        ptGlobalInfo->tCameraView     = ptAppData->tCamera.tViewMat;
        ptGlobalInfo->tCameraViewProj = pl_mul_mat4(&ptAppData->tCamera.tProjMat, &ptAppData->tCamera.tViewMat);

        pl_renderer_begin_frame(&ptAppData->tRenderer);

        const uint32_t uDrawOffset = pl_sb_size(ptAppData->tRenderer.sbtDraws);

        pl_renderer_submit_meshes(&ptAppData->tRenderer, &ptAppData->tGrassMesh, &ptAppData->uGrassMaterial, &ptAppData->tGrassBindGroup2, ptAppData->uGrassConstantBuffer, 1);
        pl_renderer_submit_meshes(&ptAppData->tRenderer, &ptAppData->tStlMesh, &ptAppData->uStlMaterial, &ptAppData->tStlBindGroup2, ptAppData->uStlConstantBuffer, 1);

        pl_sb_push(ptAppData->tRenderer.sbtDrawAreas, ((plDrawArea){
            .ptBindGroup0 = &ptAppData->tRenderer.tGlobalBindGroup,
            .uDrawOffset  = uDrawOffset,
            .uDrawCount   = pl_sb_size(ptAppData->tRenderer.sbtDraws),
            .uDynamicBufferOffset0 = pl_get_constant_buffer_offset(&ptAppData->tGraphics.tResourceManager, ptAppData->tRenderer.uGlobalConstantBuffer, 0)
        }));

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~submit draws~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        pl_draw_areas(&ptAppData->tGraphics, pl_sb_size(ptAppData->tRenderer.sbtDrawAreas), ptAppData->tRenderer.sbtDrawAreas, ptAppData->tRenderer.sbtDraws);

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~drawing api~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        ptAppData->ptDrawExtApi->pl_add_text(ptAppData->fgDrawLayer, &ptAppData->fontAtlas.sbFonts[0], 13.0f, (plVec2){100.0f, 100.0f}, (plVec4){1.0f, 1.0f, 0.0f, 1.0f}, "Drawn from extension!");

        // draw profiling info

        pl_begin_profile_sample("Draw Profiling Info");
        char cPProfileValue[64] = {0};
        for(uint32_t i = 0u; i < pl_sb_size(ptAppData->tProfileCtx.ptLastFrame->sbtSamples); i++)
        {
            plProfileSample* tPSample = &ptAppData->tProfileCtx.ptLastFrame->sbtSamples[i];
            pl_add_text(ptAppData->fgDrawLayer, &ptAppData->fontAtlas.sbFonts[0], 13.0f, (plVec2){10.0f + (float)tPSample->uDepth * 15.0f, 50.0f + (float)i * 15.0f}, (plVec4){1.0f, 1.0f, 1.0f, 1.0f}, tPSample->pcName, 0.0f);
            plVec2 sampleTextSize = pl_calculate_text_size(&ptAppData->fontAtlas.sbFonts[0], 13.0f, tPSample->pcName, 0.0f);
            pl_sprintf(cPProfileValue, ": %0.5f", tPSample->dDuration);
            pl_add_text(ptAppData->fgDrawLayer, &ptAppData->fontAtlas.sbFonts[0], 13.0f, (plVec2){sampleTextSize.x + 15.0f + (float)tPSample->uDepth * 15.0f, 50.0f + (float)i * 15.0f}, (plVec4){1.0f, 1.0f, 1.0f, 1.0f}, cPProfileValue, 0.0f);
        }
        pl_end_profile_sample();

        // ui

        static bool bOpen = true;

        if(pl_ui_begin_window("Pilot Light", NULL, false))
        {
            pl_ui_text("%.6f ms", ptIOCtx->fDeltaTime);
            pl_ui_checkbox("Camera Info", &bOpen);

            static bool bMaterialsOpen = false;
            static bool bOpenValues[512] = {0};
            if(pl_ui_collapsing_header("Materials", &bMaterialsOpen))
            {
                for(uint32_t i = 0; i < pl_sb_size(ptAppData->tAssetRegistry.sbtMaterials); i++)
                {
                    plMaterial* ptMaterial = &ptAppData->tAssetRegistry.sbtMaterials[i];

                    static bool bOpen0 = false;
                    if(pl_ui_tree_node(ptMaterial->acName, &bOpenValues[i]))
                    {
                        pl_ui_text("Double Sided: %s", ptMaterial->bDoubleSided ? "true" : "false");
                        pl_ui_text("Alpha cutoff: %0.1f", ptMaterial->fAlphaCutoff);
                        ptMaterial->uShader      == UINT32_MAX ? pl_ui_text("Shader: None")       : pl_ui_text("Shader: %u", ptMaterial->uShader);
                        ptMaterial->uAlbedoMap   == UINT32_MAX ? pl_ui_text("Albedo Map: None")   : pl_ui_text("Albedo Map: %u", ptMaterial->uAlbedoMap);
                        ptMaterial->uNormalMap   == UINT32_MAX ? pl_ui_text("Normal Map: None")   : pl_ui_text("Normal Map: %u", ptMaterial->uNormalMap);
                        ptMaterial->uSpecularMap == UINT32_MAX ? pl_ui_text("Specular Map: None") : pl_ui_text("Specular Map: %u", ptMaterial->uSpecularMap);
                        pl_ui_tree_pop();
                    }
                }
            }

        }
        pl_ui_end_window();

        if(bOpen)
        {
            if(pl_ui_begin_window("Camera Info", &bOpen, true))
            {
                pl_ui_text("Pos: %.3f, %.3f, %.3f", ptAppData->tCamera.tPos.x, ptAppData->tCamera.tPos.y, ptAppData->tCamera.tPos.z);
                pl_ui_text("Pitch: %.3f, Yaw: %.3f, Roll:%.3f", ptAppData->tCamera.fPitch, ptAppData->tCamera.fYaw, ptAppData->tCamera.fRoll);
                pl_ui_text("Up: %.3f, %.3f, %.3f", ptAppData->tCamera._tUpVec.x, ptAppData->tCamera._tUpVec.y, ptAppData->tCamera._tUpVec.z);
                pl_ui_text("Forward: %.3f, %.3f, %.3f", ptAppData->tCamera._tForwardVec.x, ptAppData->tCamera._tForwardVec.y, ptAppData->tCamera._tForwardVec.z);
                pl_ui_text("Right: %.3f, %.3f, %.3f", ptAppData->tCamera._tRightVec.x, ptAppData->tCamera._tRightVec.y, ptAppData->tCamera._tRightVec.z);  
            }
            pl_ui_end_window();
        }

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~submit draws~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        // submit draw layers
        pl_begin_profile_sample("Submit draw layers");
        pl_submit_draw_layer(ptAppData->bgDrawLayer);
        pl_submit_draw_layer(ptAppData->fgDrawLayer);
        pl_end_profile_sample();

        pl_ui_render();

        // submit draw lists
        pl_submit_drawlist_vulkan(&ptAppData->drawlist, (float)ptIOCtx->afMainViewportSize[0], (float)ptIOCtx->afMainViewportSize[1], ptCurrentFrame->tCmdBuf, (uint32_t)ptAppData->tGraphics.szCurrentFrameIndex);
        pl_submit_drawlist_vulkan(ptAppData->tUiContext.ptDrawlist, (float)ptIOCtx->afMainViewportSize[0], (float)ptIOCtx->afMainViewportSize[1], ptCurrentFrame->tCmdBuf, (uint32_t)ptAppData->tGraphics.szCurrentFrameIndex);

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~end frame~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        pl_end_main_pass(&ptAppData->tGraphics);
        pl_end_recording(&ptAppData->tGraphics);
        pl_end_frame(&ptAppData->tGraphics);
    }
    pl_end_io_frame();
    pl_ui_end_frame();
    pl_end_profile_frame();
}

static inline float frandom(float fMax){ return (float)rand()/(float)(RAND_MAX/fMax);}

void
pl_load_grass(plAppData* ptData, uint32_t uRows, uint32_t uColumns, float fSpacing)
{

    // NOTE: this is quick and dirty. Alot of work needed. Mostly just for testing now.

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
                    0.0f, 
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
    plBindGroupLayout tGroupLayout0 = {

        .uBufferCount = 1,
        .aBuffers      = {
            {
                .tType       = PL_BUFFER_BINDING_TYPE_UNIFORM,
                .uSlot       = 0,
                .tStageFlags = VK_SHADER_STAGE_VERTEX_BIT
            }
        },

        .uTextureCount = 1,
        .aTextures     = {
            {
                .tType       = PL_TEXTURE_BINDING_TYPE_SAMPLED,
                .uSlot       = 1,
                .tStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            }  
        }
    };
    pl_create_bind_group(&ptData->tGraphics, &tGroupLayout0, &ptData->tGrassBindGroup2, "grass bind group");

    int texWidth, texHeight, texNumChannels;
    int texForceNumChannels = 4;
    unsigned char* rawBytes = stbi_load("../data/pilotlight-assets-master/images/grass.png", &texWidth, &texHeight, &texNumChannels, texForceNumChannels);
    PL_ASSERT(rawBytes);

    const plTextureDesc tTextureDesc = {
        .tDimensions = {.x = (float)texWidth, .y = (float)texHeight, .z = 1.0f},
        .tFormat     = VK_FORMAT_R8G8B8A8_UNORM,
        .tUsage      = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .uLayers     = 1,
        .uMips       = 0, // means all mips
        .tType       = VK_IMAGE_TYPE_2D,
        .tViewType   = VK_IMAGE_VIEW_TYPE_2D
    };
    ptData->uGrassTexture  = pl_create_texture(&ptData->tGraphics.tResourceManager, tTextureDesc, sizeof(unsigned char) * texHeight * texHeight * 4, rawBytes);
    pl_update_bind_group(&ptData->tGraphics, &ptData->tGrassBindGroup2, 1, &ptData->uGrassConstantBuffer, 1, &ptData->uGrassTexture);

    ptData->tGrassMesh.uIndexCount   = (uint32_t)pl_sb_size(sbtIndexBuffer);
    ptData->tGrassMesh.uVertexCount  = (uint32_t)pl_sb_size(sbtVertexBuffer);
    ptData->tGrassMesh.uIndexBuffer  = pl_create_index_buffer(&ptData->tGraphics.tResourceManager, sizeof(uint32_t) * pl_sb_size(sbtIndexBuffer), sbtIndexBuffer);
    ptData->tGrassMesh.uVertexBuffer = pl_create_vertex_buffer(&ptData->tGraphics.tResourceManager, sizeof(plVec3) * pl_sb_size(sbtVertexBuffer), sizeof(plVec3), sbtVertexBuffer);

    const uint32_t uStorageOffset = pl_sb_size(ptData->tRenderer.sbfStorageBuffer) / 4;
    pl_sb_reserve(ptData->tRenderer.sbfStorageBuffer, pl_sb_size(ptData->tRenderer.sbfStorageBuffer) + pl_sb_size(sbtStorageBuffer) * 4);
    for(uint32_t i = 0; i < pl_sb_size(sbtStorageBuffer); i++)
    {
        pl_sb_push(ptData->tRenderer.sbfStorageBuffer, sbtStorageBuffer[i].x);
        pl_sb_push(ptData->tRenderer.sbfStorageBuffer, sbtStorageBuffer[i].y);
        pl_sb_push(ptData->tRenderer.sbfStorageBuffer, sbtStorageBuffer[i].z);
        pl_sb_push(ptData->tRenderer.sbfStorageBuffer, sbtStorageBuffer[i].w);
    }

    const plBuffer* ptObjectConstBuffer = &ptData->tGraphics.tResourceManager.sbtBuffers[ptData->uGrassConstantBuffer];

    for(uint32_t j = 0; j < ptData->tGraphics.uFramesInFlight; j++)
    {
        const uint32_t uConstantBufferOffset2Base = (uint32_t)ptObjectConstBuffer->szStride * j;

        plObjectInfo* ptObjectInfo = pl_get_constant_buffer_data_ex(&ptData->tGraphics.tResourceManager, ptData->uGrassConstantBuffer, (size_t)j, 0);
        ptObjectInfo->tModel = pl_mat4_translate_xyz(0.0f, 0.0f, 0.0f);
        ptObjectInfo->uVertexOffset = uStorageOffset;
    }

    pl_sb_free(sbtVertexBuffer);
    pl_sb_free(sbtStorageBuffer);
    pl_sb_free(sbtIndexBuffer);
}
#include "pl_renderer.h"
#include "pl_graphics_vulkan.h"
#include "pl_ds.h"

void
pl_setup_asset_registry(plGraphics* ptGraphics, plAssetRegistry* ptRegistryOut)
{
    ptRegistryOut->ptGraphics = ptGraphics;
}

void
pl_cleanup_asset_registry(plAssetRegistry* ptRegistry)
{
    for(uint32_t i = 0; i < pl_sb_size(ptRegistry->sbtMaterials); i++)
    {
        plMaterial* ptMaterial = &ptRegistry->sbtMaterials[i];
        pl_submit_buffer_for_deletion(&ptRegistry->ptGraphics->tResourceManager, ptMaterial->uMaterialConstantBuffer);
    }
    pl_sb_free(ptRegistry->sbtMaterials);
}

uint32_t
pl_create_material(plAssetRegistry* ptRegistry, const char* pcName)
{
    const uint32_t uIndex = pl_sb_size(ptRegistry->sbtMaterials);

    plMaterial tMaterial = {
        .uIndex                 = uIndex,
        .uShader                = UINT32_MAX,
        .uAlbedoMap             = UINT32_MAX,
        .uNormalMap             = UINT32_MAX,
        .uSpecularMap           = UINT32_MAX,
        .tAlbedo                = { 0.45f, 0.45f, 0.85f, 1.0f },
        .fAlphaCutoff           = 0.1f,
        .bDoubleSided           = false,
        .acName                 = "unnamed material"
    };
    if(pcName)
        strncpy(tMaterial.acName, pcName, PL_MATERIAL_MAX_NAME_LENGTH);

    pl_sb_push(ptRegistry->sbtMaterials, tMaterial);
    return uIndex;
}

void
pl_setup_renderer(plGraphics* ptGraphics, plAssetRegistry* ptRegistry, plRenderer* ptRendererOut)
{
    ptRendererOut->ptGraphics = ptGraphics;
    ptRendererOut->ptAssetRegistry = ptRegistry;
    ptRendererOut->uGlobalStorageBuffer = UINT32_MAX;

    plBindGroupLayout tGlobalGroupLayout =
    {
        .uBufferCount = 2,
        .uTextureCount = 0,
        .aBuffers = {
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
        }
    };

    // create constant buffer
    ptRendererOut->uGlobalConstantBuffer = pl_create_constant_buffer(&ptRegistry->ptGraphics->tResourceManager, sizeof(plGlobalInfo), ptRegistry->ptGraphics->uFramesInFlight);

    // create & update global bind group
    pl_create_bind_group(ptRendererOut->ptGraphics, &tGlobalGroupLayout, &ptRendererOut->tGlobalBindGroup, "global bind group");
}

void
pl_cleanup_renderer(plRenderer* ptRenderer)
{
    pl_sb_free(ptRenderer->sbfStorageBuffer);
    pl_sb_free(ptRenderer->sbtDraws);
    pl_sb_free(ptRenderer->sbtDrawAreas);
    pl_submit_buffer_for_deletion(&ptRenderer->ptGraphics->tResourceManager, ptRenderer->uGlobalStorageBuffer);
    pl_submit_buffer_for_deletion(&ptRenderer->ptGraphics->tResourceManager, ptRenderer->uGlobalConstantBuffer);
}

void
pl_finalize_material(plRenderer* ptRenderer, uint32_t uMaterial)
{

    plMaterial* ptMaterial = &ptRenderer->ptAssetRegistry->sbtMaterials[uMaterial];

    // create shaders
    plShaderDesc tShaderDesc = {
        ._tRenderPass                       = ptRenderer->ptGraphics->tRenderPass,
        .pcPixelShader                      = "phong.frag.spv",
        .pcVertexShader                     = "vertex_shader.vert.spv",
        .tGraphicsState.ulVertexStreamMask0 = ptMaterial->ulVertexStreamMask0,
        .tGraphicsState.ulVertexStreamMask1 = ptMaterial->ulVertexStreamMask1,
        .tGraphicsState.ulDepthMode         = PL_DEPTH_MODE_LESS,
        .tGraphicsState.ulBlendMode         = PL_BLEND_MODE_ALPHA,
        .tGraphicsState.ulCullMode          = ptMaterial->bDoubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT,
        .tGraphicsState.ulDepthWriteEnabled = VK_TRUE,
        .uBindGroupLayoutCount              = 3,
        .atBindGroupLayouts                 = {
            {
                .uBufferCount = 2,
                .uTextureCount = 0,
                .aBuffers = {
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
                },
            },
            {
                .uBufferCount = 1,
                .aBuffers      = {
                    {
                        .tType       = PL_BUFFER_BINDING_TYPE_UNIFORM,
                        .uSlot       = 0,
                        .tStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT 
                    }
                }
            },
            {
                .uBufferCount  = 1,
                .uTextureCount = 1,
                .aBuffers      = {
                    {
                        .tType       = PL_BUFFER_BINDING_TYPE_UNIFORM,
                        .uSlot       = 0,
                        .tStageFlags = VK_SHADER_STAGE_VERTEX_BIT
                    }
                },
                .aTextures     = {
                    {
                        .tType       = PL_TEXTURE_BINDING_TYPE_SAMPLED,
                        .uSlot       = 1,
                        .tStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
                    } 
                }
            }
        },
        
    };
    
    pl_create_bind_group(ptRenderer->ptGraphics, &tShaderDesc.atBindGroupLayouts[0], NULL, "nullg material");
    pl_create_bind_group(ptRenderer->ptGraphics, &tShaderDesc.atBindGroupLayouts[1], &ptMaterial->tMaterialBindGroup, ptMaterial->acName);
    pl_create_bind_group(ptRenderer->ptGraphics, &tShaderDesc.atBindGroupLayouts[2], NULL, "null material");
    ptMaterial->uShader = pl_create_shader(&ptRenderer->ptGraphics->tResourceManager, &tShaderDesc);

    // create constant buffers
    ptMaterial->uMaterialConstantBuffer = pl_create_constant_buffer(&ptRenderer->ptGraphics->tResourceManager, sizeof(plMaterialInfo), ptRenderer->ptGraphics->uFramesInFlight);

    // update bind group
    pl_update_bind_group(ptRenderer->ptGraphics, &ptMaterial->tMaterialBindGroup, 1, &ptMaterial->uMaterialConstantBuffer, 0, NULL);

    for(uint32_t i = 0; i < ptRenderer->ptGraphics->uFramesInFlight; i++)
    {
        plMaterialInfo* ptMaterialInfo = pl_get_constant_buffer_data_ex(&ptRenderer->ptGraphics->tResourceManager, ptMaterial->uMaterialConstantBuffer, i, 0);
        ptMaterialInfo->bHasColorTexture = ptMaterial->uAlbedoMap == UINT32_MAX ? VK_FALSE : VK_TRUE;
    }
}

void
pl_renderer_begin_frame(plRenderer* ptRenderer)
{

    pl_sb_reset(ptRenderer->sbtDraws);
    pl_sb_reset(ptRenderer->sbtDrawAreas);

    if(pl_sb_size(ptRenderer->sbfStorageBuffer) > 0)
    {
        if(ptRenderer->uGlobalStorageBuffer != UINT32_MAX)
        {
            pl_submit_buffer_for_deletion(&ptRenderer->ptGraphics->tResourceManager, ptRenderer->uGlobalStorageBuffer);
        }
        ptRenderer->uGlobalStorageBuffer = pl_create_storage_buffer(&ptRenderer->ptGraphics->tResourceManager, pl_sb_size(ptRenderer->sbfStorageBuffer) * sizeof(float), ptRenderer->sbfStorageBuffer);
        pl_sb_reset(ptRenderer->sbfStorageBuffer);

        uint32_t atBuffers0[] = {ptRenderer->uGlobalConstantBuffer, ptRenderer->uGlobalStorageBuffer};
        pl_update_bind_group(ptRenderer->ptGraphics, &ptRenderer->tGlobalBindGroup, 2, atBuffers0, 0, NULL);
    }
}

void
pl_renderer_submit_meshes(plRenderer* ptRenderer, plMesh* ptMeshes, uint32_t* puMaterials, plBindGroup* ptBindGroup, uint32_t uConstantBuffer, uint32_t uMeshCount)
{
    
    pl_sb_reserve(ptRenderer->sbtDraws, pl_sb_size(ptRenderer->sbtDraws) + uMeshCount);
    for(uint32_t i = 0; i < uMeshCount; i++)
    {
        plMaterial* ptMaterial = &ptRenderer->ptAssetRegistry->sbtMaterials[puMaterials[i]];
        pl_sb_push(ptRenderer->sbtDraws, ((plDraw){
            .uShader      = ptMaterial->uShader,
            .ptMesh       = &ptMeshes[i],
            .ptBindGroup1 = &ptMaterial->tMaterialBindGroup,
            .ptBindGroup2 = &ptBindGroup[i],
            .uDynamicBufferOffset1 = pl_get_constant_buffer_offset(&ptRenderer->ptGraphics->tResourceManager, ptMaterial->uMaterialConstantBuffer, 0),
            .uDynamicBufferOffset2 = pl_get_constant_buffer_offset(&ptRenderer->ptGraphics->tResourceManager, uConstantBuffer, i)
            }));
    }
}
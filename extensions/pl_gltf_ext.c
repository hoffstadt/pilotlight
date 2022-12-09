#include "pl_gltf_ext.h"
#include "pl_ds.h"
#include "pl_math.h"
#include "stb_image.h"
#include "pl_graphics_vulkan.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

bool
pl_ext_load_gltf(plGraphics* ptGraphics, const char* pcPath, plGltf* ptGltfOut)
{
    size_t szTotalOffset = 0;

    float* sbStorageBuffer = NULL;

    cgltf_options tGltfOptions = {0};
    cgltf_data* ptGltfData = NULL;

    cgltf_result tGltfResult = cgltf_parse_file(&tGltfOptions, pcPath, &ptGltfData);

    if(tGltfResult != cgltf_result_success)
        return false;

    tGltfResult = cgltf_load_buffers(&tGltfOptions, ptGltfData, pcPath);

    if(tGltfResult != cgltf_result_success)
        return false;

    ptGltfOut->uObjectConstantBuffer = pl_create_constant_buffer(&ptGraphics->tResourceManager, sizeof(plObjectInfo), ptGraphics->uFramesInFlight * 100);
    // ptGltfOut->uObjectConstantBuffer = pl_create_constant_buffer(&ptGraphics->tResourceManager, sizeof(plObjectInfo), ptGraphics->uFramesInFlight * ptGltfData->meshes_count);

    // for(size_t szMeshIndex = 0; szMeshIndex < ptGltfData->meshes_count; szMeshIndex++)
    for(size_t szMeshIndex = 0; szMeshIndex < 100; szMeshIndex++)
    {
        // const cgltf_mesh* ptMesh = &ptGltfData->meshes[szMeshIndex];
        const cgltf_mesh* ptMesh = &ptGltfData->meshes[szMeshIndex % 2];

        pl_sb_reserve(ptGltfOut->sbtMeshes, pl_sb_size(ptGltfOut->sbtMeshes) + ptMesh->primitives_count);
        for(size_t szPrimitiveIndex = 0; szPrimitiveIndex < ptMesh->primitives_count; szPrimitiveIndex++)
        {
            const cgltf_primitive* ptPrimitive = &ptMesh->primitives[szPrimitiveIndex];
            const size_t szVertexCount = ptPrimitive->attributes[0].data->count;
            uint32_t uAttributeComponents = 0u;
            uint32_t uExtraAttributeComponents = 0u;

            unsigned char* pucPosBufferStart     = NULL;
            unsigned char* pucNormalBufferStart  = NULL;
            unsigned char* pucTextBufferStart    = NULL;
            unsigned char* pucTangentBufferStart = NULL;
            unsigned char* pucColorBufferStart   = NULL;
            unsigned char* pucJointsBufferStart  = NULL;
            unsigned char* pucWeightsBufferStart = NULL;

            size_t szPosBufferStride     = 0;
            size_t szNormalBufferStride  = 0;
            size_t szTextBufferStride    = 0;
            size_t szTangentBufferStride = 0;
            size_t szColorBufferStride   = 0;
            size_t szJointsBufferStride  = 0;
            size_t szWeightsBufferStride = 0;

            for(size_t szAttributeIndex = 0; szAttributeIndex < ptPrimitive->attributes_count; szAttributeIndex++)
            {
                const cgltf_attribute* ptAttribute = &ptPrimitive->attributes[szAttributeIndex];
                const cgltf_buffer* ptBuffer = ptAttribute->data->buffer_view->buffer;
                const size_t szStride = ptAttribute->data->buffer_view->stride;

                unsigned char* pucBufferStart = &((unsigned char*)ptBuffer->data)[ptAttribute->data->buffer_view->offset + ptAttribute->data->offset];
                switch(ptAttribute->type)
                {
                    case cgltf_attribute_type_position: pucPosBufferStart     = pucBufferStart; uAttributeComponents += 3;      szPosBufferStride     = szStride == 0 ? sizeof(float) * 3 : szStride; break;
	                case cgltf_attribute_type_normal:   pucNormalBufferStart  = pucBufferStart; uExtraAttributeComponents += 4; szNormalBufferStride  = szStride == 0 ? sizeof(float) * 3 : szStride; break;
	                case cgltf_attribute_type_tangent:  pucTangentBufferStart = pucBufferStart; uExtraAttributeComponents += 4; szTangentBufferStride = szStride == 0 ? sizeof(float) * 4 : szStride; break;
	                case cgltf_attribute_type_texcoord: pucTextBufferStart    = pucBufferStart; uExtraAttributeComponents += 4; szTextBufferStride    = szStride == 0 ? sizeof(float) * 2 : szStride; break;
	                case cgltf_attribute_type_color:    pucColorBufferStart   = pucBufferStart; uExtraAttributeComponents += 4; szColorBufferStride   = szStride == 0 ? sizeof(float) * 4 : szStride; break;
	                case cgltf_attribute_type_joints:   pucJointsBufferStart  = pucBufferStart; uExtraAttributeComponents += 4; szJointsBufferStride  = szStride == 0 ? sizeof(float) * 4 : szStride; break;
	                case cgltf_attribute_type_weights:  pucWeightsBufferStart = pucBufferStart; uExtraAttributeComponents += 4; szWeightsBufferStride = szStride == 0 ? sizeof(float) * 4 : szStride; break;
                    default:
                        PL_ASSERT(false && "unknown gltf attribute type");
                        break;
                }
            }

            // allocate CPU buffers
            float* pfVertexBuffer      = malloc(sizeof(float) * uAttributeComponents * szVertexCount);
            uint32_t* puIndexBuffer    = malloc(sizeof(uint32_t) * ptPrimitive->indices->count);

            // initialize to 0
            memset(pfVertexBuffer, 0, sizeof(float) * uAttributeComponents * szVertexCount);
            memset(puIndexBuffer, 0, sizeof(uint32_t) * ptPrimitive->indices->count);

            const uint32_t uStartPoint = pl_sb_size(sbStorageBuffer);
            pl_sb_add_n(sbStorageBuffer, uExtraAttributeComponents * (uint32_t)szVertexCount);
            uint32_t uCurrentAttributeOffset = 0;

            if(szPosBufferStride)
            {
                for(size_t i = 0; i < szVertexCount; i++)
                {
                    const float x = *(float*)&pucPosBufferStart[i * szPosBufferStride];
                    const float y = ((float*)&pucPosBufferStart[i * szPosBufferStride])[1];
                    const float z = ((float*)&pucPosBufferStart[i * szPosBufferStride])[2];

                    pfVertexBuffer[i * 3]   = x;
                    pfVertexBuffer[i * 3 + 1] = y;
                    pfVertexBuffer[i * 3 + 2] = z;
                }
            }

            // normals
            if(pucNormalBufferStart)
            {
                for(size_t i = 0; i < szVertexCount; i++)
                {

                    const float nx = *(float*)&pucNormalBufferStart[i * szNormalBufferStride];
                    const float ny = ((float*)&pucNormalBufferStart[i * szNormalBufferStride])[1];
                    const float nz = ((float*)&pucNormalBufferStart[i * szNormalBufferStride])[2];

                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset]     = nx;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 1] = ny;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 2] = nz;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 3] = 0.0f;
                }
                uCurrentAttributeOffset += 4;
            }

            // tangents
            if(pucTangentBufferStart)
            {
                for(size_t i = 0; i < szVertexCount; i++)
                {

                    const float tx = *(float*)&pucTangentBufferStart[i * szTangentBufferStride];
                    const float ty = ((float*)&pucTangentBufferStart[i * szTangentBufferStride])[1];
                    const float tz = ((float*)&pucTangentBufferStart[i * szTangentBufferStride])[2]; 
                    const float tw = ((float*)&pucTangentBufferStart[i * szTangentBufferStride])[3]; 

                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset]     = tx;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 1] = ty;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 2] = tz;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 3] = tw;
                }
                uCurrentAttributeOffset += 4;
            }

            // texture coordinates
            if(pucTextBufferStart)
            {
                for(size_t i = 0; i < szVertexCount; i++)
                {

                    const float u = *(float*)&pucTextBufferStart[i * szTextBufferStride];
                    const float v = ((float*)&pucTextBufferStart[i * szTextBufferStride])[1];

                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset]     = u;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 1] = v;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 2] = 0.0f;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 3] = 0.0f;
                }
                uCurrentAttributeOffset += 4;
            }

            // colors
            if(pucColorBufferStart)
            {
                for(size_t i = 0; i < szVertexCount; i++)
                {

                    const float r = *(float*)&pucColorBufferStart[i * szColorBufferStride];
                    const float g = ((float*)&pucColorBufferStart[i * szColorBufferStride])[1];
                    const float b = ((float*)&pucColorBufferStart[i * szColorBufferStride])[2]; 
                    const float a = ((float*)&pucColorBufferStart[i * szColorBufferStride])[3]; 

                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset]     = r;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 1] = g;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 2] = b;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 3] = a;
                }
                uCurrentAttributeOffset += 4;
            }

            // joints
            if(pucJointsBufferStart)
            {
                for(size_t i = 0; i < szVertexCount; i++)
                {

                    const float x = *(float*)&pucJointsBufferStart[i * szJointsBufferStride];
                    const float y = ((float*)&pucJointsBufferStart[i * szJointsBufferStride])[1];
                    const float z = ((float*)&pucJointsBufferStart[i * szJointsBufferStride])[2]; 
                    const float w = ((float*)&pucJointsBufferStart[i * szJointsBufferStride])[3]; 

                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset]     = x;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 1] = y;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 2] = z;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 3] = w;
                }
                uCurrentAttributeOffset += 4;
            }

            // joints
            if(pucWeightsBufferStart)
            {
                for(size_t i = 0; i < szVertexCount; i++)
                {

                    const float x = *(float*)&pucWeightsBufferStart[i * szJointsBufferStride];
                    const float y = ((float*)&pucWeightsBufferStart[i * szJointsBufferStride])[1];
                    const float z = ((float*)&pucWeightsBufferStart[i * szJointsBufferStride])[2]; 
                    const float w = ((float*)&pucWeightsBufferStart[i * szJointsBufferStride])[3]; 

                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset]     = x;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 1] = y;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 2] = z;
                    sbStorageBuffer[uStartPoint + i * uExtraAttributeComponents + uCurrentAttributeOffset + 3] = w;
                }
                uCurrentAttributeOffset += 4;
            }

            pl_sb_push(ptGltfOut->sbuVertexOffsets, (uint32_t)szTotalOffset);
            szTotalOffset += (size_t)uExtraAttributeComponents/4 * szVertexCount;

            // index buffer
            unsigned char* pucIdexBufferStart = &((unsigned char*)ptPrimitive->indices->buffer_view->buffer->data)[ptPrimitive->indices->buffer_view->offset + ptPrimitive->indices->offset];
            if(ptPrimitive->indices->buffer_view->stride == 0)
            {
                for(uint32_t i = 0; i < ptPrimitive->indices->count; i++)
                {
                    unsigned short index = *(unsigned short*)&pucIdexBufferStart[i * sizeof(unsigned short)];
                    puIndexBuffer[i] = (uint32_t)index;
                }
            }
            else
            {
                for(uint32_t i = 0; i < ptPrimitive->indices->count; i++)
                {
                    unsigned short index = *(unsigned short*)&pucIdexBufferStart[i * ptPrimitive->indices->buffer_view->stride];
                    puIndexBuffer[i] = (uint32_t)index;
                }
            }

            const plMesh tMesh = {
                .uIndexCount   = (uint32_t)ptPrimitive->indices->count,
                .uVertexCount  = (uint32_t)szVertexCount,
                .uIndexBuffer  = pl_create_index_buffer(&ptGraphics->tResourceManager, sizeof(uint32_t) * (uint32_t)ptPrimitive->indices->count, puIndexBuffer),
                .uVertexBuffer = pl_create_vertex_buffer(&ptGraphics->tResourceManager, sizeof(float) * szVertexCount * uAttributeComponents, sizeof(float) * uAttributeComponents, pfVertexBuffer)   
            };
            pl_sb_push(ptGltfOut->sbtMeshes, tMesh);
            // pl_sb_push(ptGltfOut->sbtTransforms, pl_identity_mat4());
            float fScale = frandom(15.0f) - 7.5f + 15.0f;
            const plMat4 tScale = pl_mat4_scale_xyz(fScale, fScale, fScale);
            if(szMeshIndex % 2 == 0)
            {
                float fX = frandom(75.0f) - 33.0f;
                // float fY = frandom(6.0f) - 3.0f;
                float fY = 0.0f;
                float fZ = frandom(75.0f) - 33.0f;
                const plMat4 tTranslate = pl_mat4_translate_xyz(fX, fY, fZ);
                pl_sb_push(ptGltfOut->sbtTransforms, pl_mul_mat4(&tTranslate, &tScale));
                pl_sb_push(ptGltfOut->sbtTransforms, pl_mul_mat4(&tTranslate, &tScale)); 
            }

            char filepath[2048] = {0};
            pl_sprintf(filepath, "%s%s", "./", ptPrimitive->material->pbr_metallic_roughness.base_color_texture.texture->image->uri);
            // pl_sprintf(filepath, "%s%s", "../data/glTF-Sample-Models-master/2.0/FlightHelmet/glTF/", ptPrimitive->material->pbr_metallic_roughness.base_color_texture.texture->image->uri);
            // pl_sprintf(filepath, "%s%s", "../data/glTF-Sample-Models-master/2.0/DamagedHelmet/glTF/", ptPrimitive->material->pbr_metallic_roughness.base_color_texture.texture->image->uri);
            // pl_sprintf(filepath, "%s%s", "../data/glTF-Sample-Models-master/2.0/Sponza/glTF/", ptPrimitive->material->pbr_metallic_roughness.base_color_texture.texture->image->uri);

            int texWidth, texHeight, texNumChannels;
            int texForceNumChannels = 4;
            unsigned char* rawBytes = stbi_load(filepath, &texWidth, &texHeight, &texNumChannels, texForceNumChannels);
            PL_ASSERT(rawBytes);

            const plTextureDesc tTextureDesc = {
                .tDimensions = {.x = (float)texWidth, .y = (float)texHeight, .z = 1.0f},
                .tFormat     = VK_FORMAT_R8G8B8A8_UNORM,
                .tUsage      = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .uLayers     = 1,
                .uMips       = 0, // means all mips
                .tType       = VK_IMAGE_TYPE_2D
            };
            uint32_t uColorTexture  = pl_create_texture(&ptGraphics->tResourceManager, tTextureDesc, sizeof(unsigned char) * texHeight * texHeight * 4, rawBytes);

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
            pl_sb_push(ptGltfOut->sbtBindGroups, (plBindGroup){0});
            pl_create_bind_group(ptGraphics, &tGroupLayout0, &pl_sb_last(ptGltfOut->sbtBindGroups));

            // just to reuse the above
            tBufferBindings[0].tBuffer   = ptGraphics->tResourceManager.sbtBuffers[ptGltfOut->uObjectConstantBuffer];
            tBufferBindings[0].szSize    = ptGraphics->tResourceManager.sbtBuffers[ptGltfOut->uObjectConstantBuffer].szStride;
            tTextureBindings[0].tTexture = ptGraphics->tResourceManager.sbtTextures[uColorTexture];
            tTextureBindings[0].tSampler = ptGraphics->tResourceManager.sbtTextures[uColorTexture].tSampler;

            plBindUpdate tBindUpdate = {
                .uBufferCount  = 1,
                .aBuffers      = tBufferBindings,
                .uTextureCount = 1,
                .aTextures     = tTextureBindings
            };
            pl_update_bind_group(ptGraphics, &pl_sb_last(ptGltfOut->sbtBindGroups), &tBindUpdate);
        }
    }

    const plBuffer* ptObjectConstBuffer = &ptGraphics->tResourceManager.sbtBuffers[ptGltfOut->uObjectConstantBuffer];

    for(uint32_t j = 0; j < ptGraphics->uFramesInFlight; j++)
    {
        const uint32_t uConstantBufferOffset2Base = pl_sb_size(ptGltfOut->sbtMeshes) * (uint32_t)ptObjectConstBuffer->szStride * j;
        for(uint32_t i = 0; i < pl_sb_size(ptGltfOut->sbtMeshes); i++)
        {
            const plObjectInfo tObjectInfo = {
                .tModel        = ptGltfOut->sbtTransforms[i],
                .uVertexOffset = ptGltfOut->sbuVertexOffsets[i]
            };
            memcpy(&ptObjectConstBuffer->pucMapping[uConstantBufferOffset2Base + ptObjectConstBuffer->szStride * i], &tObjectInfo, sizeof(plObjectInfo));
      }
    }


    ptGltfOut->uStorageBuffer = pl_create_storage_buffer(&ptGraphics->tResourceManager, pl_sb_size(sbStorageBuffer) * sizeof(float), sbStorageBuffer);
    pl_sb_free(sbStorageBuffer);
    return true;
}

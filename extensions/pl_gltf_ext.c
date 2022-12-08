#include "pl_gltf_ext.h"
#include "pl_ds.h"
#include "stb_image.h"
#include "pl_graphics_vulkan.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

bool
pl_ext_load_gltf(plGraphics* ptGraphics, const char* pcPath, plGltf* ptGltfOut)
{
    cgltf_options tGltfOptions = {0};
    cgltf_data* ptGltfData = NULL;

    cgltf_result tGltfResult = cgltf_parse_file(&tGltfOptions, pcPath, &ptGltfData);

    if(tGltfResult != cgltf_result_success)
        return false;

    tGltfResult = cgltf_load_buffers(&tGltfOptions, ptGltfData, pcPath);

    if(tGltfResult != cgltf_result_success)
        return false;

    for(size_t szMeshIndex = 0; szMeshIndex < ptGltfData->meshes_count; szMeshIndex++)
    {
        const cgltf_mesh* ptMesh = &ptGltfData->meshes[szMeshIndex];

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
	                // case cgltf_attribute_type_color:  
	                // case cgltf_attribute_type_joints: 
	                // case cgltf_attribute_type_weights:
                    default:
                        PL_ASSERT(false && "unknown gltf attribute type");
                        break;
                }
            }

            // allocate CPU buffers
            float* pfVertexBuffer      = malloc(sizeof(float) * uAttributeComponents * szVertexCount);
            float* pfVertexExtraBuffer = malloc(sizeof(float) * uExtraAttributeComponents * szVertexCount);
            uint32_t* puIndexBuffer    = malloc(sizeof(uint32_t) * ptPrimitive->indices->count);

            // initialize to 0
            memset(pfVertexBuffer, 0, sizeof(float) * uAttributeComponents * szVertexCount);
            memset(pfVertexExtraBuffer, 0, sizeof(float) * uExtraAttributeComponents * szVertexCount);
            memset(puIndexBuffer, 0, sizeof(uint32_t) * ptPrimitive->indices->count);


            if(szPosBufferStride && pucNormalBufferStart && pucTextBufferStart && pucTangentBufferStart)
            {
                pl_sb_push(ptGltfOut->sbtHasTangent, true);
                for(size_t i = 0; i < szVertexCount; i++)
                {
                    const float x = *(float*)&pucPosBufferStart[i * szPosBufferStride];
                    const float y = ((float*)&pucPosBufferStart[i * szPosBufferStride])[1];
                    const float z = ((float*)&pucPosBufferStart[i * szPosBufferStride])[2];

                    const float nx = *(float*)&pucNormalBufferStart[i * szNormalBufferStride];
                    const float ny = ((float*)&pucNormalBufferStart[i * szNormalBufferStride])[1];
                    const float nz = ((float*)&pucNormalBufferStart[i * szNormalBufferStride])[2];

                    const float tx = *(float*)&pucTangentBufferStart[i * szTangentBufferStride];
                    const float ty = ((float*)&pucTangentBufferStart[i * szTangentBufferStride])[1];
                    const float tz = ((float*)&pucTangentBufferStart[i * szTangentBufferStride])[2]; 

                    const float u = *(float*)&pucTextBufferStart[i * szTextBufferStride];
                    const float v = ((float*)&pucTextBufferStart[i * szTextBufferStride])[1];

                    // position
                    pfVertexBuffer[i * 3]     = x;
                    pfVertexBuffer[i * 3 + 1] = y;
                    pfVertexBuffer[i * 3 + 2] = z;

                    // normals
                    pfVertexExtraBuffer[i * uExtraAttributeComponents]     = nx;
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 1] = ny;
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 2] = nz;

                    // tangents
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 4] = tx;
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 5] = ty;
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 6] = tz;

                    // texture coords
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 8] = u;
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 9] = v;
                }
            }

            else if(szPosBufferStride && pucNormalBufferStart && pucTextBufferStart)
            {
                pl_sb_push(ptGltfOut->sbtHasTangent, false);
                for(size_t i = 0; i < szVertexCount; i++)
                {
                    const float x = *(float*)&pucPosBufferStart[i * szPosBufferStride];
                    const float y = ((float*)&pucPosBufferStart[i * szPosBufferStride])[1];
                    const float z = ((float*)&pucPosBufferStart[i * szPosBufferStride])[2];

                    const float nx = *(float*)&pucNormalBufferStart[i * szNormalBufferStride];
                    const float ny = ((float*)&pucNormalBufferStart[i * szNormalBufferStride])[1];
                    const float nz = ((float*)&pucNormalBufferStart[i * szNormalBufferStride])[2];

                    const float u = *(float*)&pucTextBufferStart[i * szTextBufferStride];
                    const float v = ((float*)&pucTextBufferStart[i * szTextBufferStride])[1];

                    // position
                    pfVertexBuffer[i * 3]   = x;
                    pfVertexBuffer[i * 3 + 1] = y;
                    pfVertexBuffer[i * 3 + 2] = z;

                    // normals
                    pfVertexExtraBuffer[i * uExtraAttributeComponents]     = nx;
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 1] = ny;
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 2] = nz;

                    // texture coords
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 4] = u;
                    pfVertexExtraBuffer[i * uExtraAttributeComponents + 5] = v;
                }
            }

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
                .uIndexCount = (uint32_t)ptPrimitive->indices->count,
                .uVertexCount = (uint32_t)szVertexCount,
                .uIndexBuffer = pl_create_index_buffer(&ptGraphics->tResourceManager, sizeof(uint32_t) * (uint32_t)ptPrimitive->indices->count, puIndexBuffer),
                .uVertexBuffers = {
                    pl_create_vertex_buffer(&ptGraphics->tResourceManager, sizeof(float) * szVertexCount * uAttributeComponents, sizeof(float) * uAttributeComponents, pfVertexBuffer),
                    pl_create_storage_buffer(&ptGraphics->tResourceManager, sizeof(float) * szVertexCount * uExtraAttributeComponents, pfVertexExtraBuffer)
                }       
            };
            pl_sb_push(ptGltfOut->sbtMeshes, tMesh);

            char filepath[2048] = {0};
            pl_sprintf(filepath, "%s%s", "../data/glTF-Sample-Models-master/2.0/FlightHelmet/glTF/", ptPrimitive->material->pbr_metallic_roughness.base_color_texture.texture->image->uri);
            // pl_sprintf(filepath, "%s%s", "../../mvImporter/data/glTF-Sample-Models/2.0/DamagedHelmet/glTF/", ptPrimitive->material->pbr_metallic_roughness.base_color_texture.texture->image->uri);
            // pl_sprintf(filepath, "%s%s", "../../mvImporter/data/glTF-Sample-Models/2.0/Sponza/glTF/", ptPrimitive->material->pbr_metallic_roughness.base_color_texture.texture->image->uri);

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
                    .tType       = PL_BUFFER_BINDING_TYPE_STORAGE,
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
            tBufferBindings[0].tBuffer   = ptGraphics->tResourceManager.sbtBuffers[tMesh.uVertexBuffers[1]];
            tBufferBindings[0].szSize    = ptGraphics->tResourceManager.sbtBuffers[tMesh.uVertexBuffers[1]].szSize;
            tTextureBindings[0].tTexture = ptGraphics->tResourceManager.sbtTextures[uColorTexture];
            tTextureBindings[0].tSampler = ptGraphics->tResourceManager.sbtTextures[uColorTexture].tSampler;

            plBindUpdate tBindUpdate = {
                .uBufferCount = 1,
                .aBuffers      = tBufferBindings,

                .uTextureCount = 1,
                .aTextures     = tTextureBindings
            };
            pl_update_bind_group(ptGraphics, &pl_sb_last(ptGltfOut->sbtBindGroups), &tBindUpdate);
        }
    }

    return true;
}

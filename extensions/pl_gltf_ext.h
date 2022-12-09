/*
   pl_gltf_extension.h
*/

/*
Index of this file:
// [SECTION] header mess
// [SECTION] includes
// [SECTION] forward declarations & basic types
// [SECTION] public api
*/

//-----------------------------------------------------------------------------
// [SECTION] header mess
//-----------------------------------------------------------------------------

#ifndef PL_GLTF_EXTENSION_H
#define PL_GLTF_EXTENSION_H

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include "pl_math.inc"

//-----------------------------------------------------------------------------
// [SECTION] forward declarations & basic types
//-----------------------------------------------------------------------------

typedef struct _plMesh plMesh;
typedef struct _plGraphics plGraphics;
typedef struct _plBindGroup plBindGroup;

typedef struct _plGltf
{
    plMesh*      sbtMeshes;
    plBindGroup* sbtBindGroups;
    plMat4*      sbtTransforms;
    uint32_t*    sbuVertexOffsets;
    uint32_t     uObjectConstantBuffer;
    uint32_t     uStorageBuffer;  
} plGltf;

typedef struct _plObjectInfo
{
    plMat4   tModel;
    uint32_t uVertexOffset;
    int      _unused[3];
} plObjectInfo;

//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

static inline float frandom(float fMax){ return (float)rand()/(float)(RAND_MAX/fMax);}
bool pl_ext_load_gltf(plGraphics* ptGraphics, const char* pcPath, plGltf* ptGltfOut);

#endif // PL_GLTF_EXTENSION_H
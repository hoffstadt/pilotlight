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
    bool*        sbtHasTangent;
} plGltf;

//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

bool pl_ext_load_gltf(plGraphics* ptGraphics, const char* pcPath, plGltf* ptGltfOut);

#endif // PL_GLTF_EXTENSION_H
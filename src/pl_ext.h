/*
   pl_ext.h, v0.1 (WIP)
   * no dependencies
   * simple
   Do this:
        #define PL_EXT_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.
   // i.e. it should look like this:
   #include ...
   #include ...
   #include ...
   #define PL_EXT_IMPLEMENTATION
   #include "pl_ext.h"
*/

/*
Index of this file:
// [SECTION] defines
// [SECTION] includes
// [SECTION] forward declarations & basic types
// [SECTION] global context
// [SECTION] public api
// [SECTION] enums
// [SECTION] structs
// [SECTION] c file
*/

//-----------------------------------------------------------------------------
// [SECTION] defines
//-----------------------------------------------------------------------------

#ifndef PL_EXT_H
#define PL_EXT_H

#ifndef PL_DECLARE_STRUCT
#define PL_DECLARE_STRUCT(name) typedef struct _ ## name  name
#endif

#ifndef PL_EXTENSION_PATH_MAX_LENGTH
    #define PL_EXTENSION_PATH_MAX_LENGTH 1024
#endif

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdbool.h> // bool
#include <stdint.h>  // uint*_t

//-----------------------------------------------------------------------------
// [SECTION] forward declarations & basic types
//-----------------------------------------------------------------------------

// forward declarations
PL_DECLARE_STRUCT(plExtensionInfo);
PL_DECLARE_STRUCT(plExtension);
PL_DECLARE_STRUCT(plApi);
PL_DECLARE_STRUCT(plExtensionRegistry);


//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

// registry
void                 pl_initialize_extension_registry(plExtensionRegistry* ptRegistry);
void                 pl_cleanup_extension_registry   (void);
void                 pl_set_extension_registry       (plExtensionRegistry* ptRegistry);
plExtensionRegistry* pl_get_extension_registry       (void);

// extensions
void                 pl_load_extension  (plExtension* ptExtension);
void                 pl_unload_extension(plExtension* ptExtension);
void*                pl_get_api         (plExtension* ptExtension, const char* pcApiName);

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plApi
{
    const char* pcName;
    void*       pApi;
} plApi;

typedef struct _plExtension
{
    const char*  pcExtensionName;
    const char*  pcLibName;
    const char*  pcTransName;
    const char*  pcLockName;
    const char*  pcLoadFunc;
    const char*  pcUnloadFunc;
    plApi*       atApis;
    uint32_t     uApiCount;
} plExtension;

typedef struct _plExtensionRegistry
{
    plExtensionInfo*  sbtExtensions;
    plExtensionInfo** sbtHotExtensions;
} plExtensionRegistry;

#endif // PL_EXT_H

//-----------------------------------------------------------------------------
// [SECTION] c file
//-----------------------------------------------------------------------------

/*
Index of this file:
// [SECTION] includes
*/

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include "pilotlight.h"
#include "pl_ds.h"
#include "pl_os.h"

#ifdef PL_EXT_IMPLEMENTATION

#ifndef PL_ASSERT
    #include <assert.h>
    #define PL_ASSERT(x) assert((x))
#endif

//-----------------------------------------------------------------------------
// [SECTION] global context
//-----------------------------------------------------------------------------

plExtensionRegistry* gptExtRegistry = NULL;

//-----------------------------------------------------------------------------
// [SECTION] internal & opaque structs
//-----------------------------------------------------------------------------

typedef struct _plExtensionInfo
{
    const char*     pcName;
    plSharedLibrary tLibrary;

    void (*pl_load)   (plExtensionRegistry* ptRegistry, plExtension* ptExtension, bool bReload);
    void (*pl_unload) (plExtensionRegistry* ptRegistry, plExtension* ptExtension);
} plExtensionInfo;

//-----------------------------------------------------------------------------
// [SECTION] public api implementation
//-----------------------------------------------------------------------------

void
pl_initialize_extension_registry(plExtensionRegistry* ptRegistry)
{
    gptExtRegistry = ptRegistry;
}

void
pl_cleanup_extension_registry(void)
{

}

void
pl_set_extension_registry(plExtensionRegistry* ptRegistry)
{
    gptExtRegistry = ptRegistry;
}

plExtensionRegistry*
pl_get_extension_registry(void)
{
    return gptExtRegistry;
}

void
pl_load_extension(plExtension* ptExtension)
{
    PL_ASSERT(gptExtRegistry && "global extension registry not set");

    // check if extension exists already
    for(uint32_t i = 0; i < pl_sb_size(gptExtRegistry->sbtExtensions); i++)
    {
        if(strcmp(ptExtension->pcExtensionName, gptExtRegistry->sbtExtensions[i].pcName) == 0)
            return;
    }

    plExtensionInfo tExtensionInfo = {0};
    tExtensionInfo.pcName = ptExtension->pcExtensionName;

    if(pl_load_library(&tExtensionInfo.tLibrary, ptExtension->pcLibName, ptExtension->pcTransName, ptExtension->pcLockName))
    {
        #ifdef _WIN32
            tExtensionInfo.pl_load   = (void (__cdecl *)(plExtensionRegistry*, plExtension*, bool)) pl_load_library_function(&tExtensionInfo.tLibrary, ptExtension->pcLoadFunc);
            tExtensionInfo.pl_unload = (void (__cdecl *)(plExtensionRegistry*, plExtension*))       pl_load_library_function(&tExtensionInfo.tLibrary, ptExtension->pcUnloadFunc);
        #else // linux
            tExtensionInfo.pl_load   = (void (__attribute__(()) *)(plExtensionRegistry*, plExtension*, bool)) pl_load_library_function(&tExtensionInfo.tLibrary, ptExtension->pcLoadFunc);
            tExtensionInfo.pl_unload = (void (__attribute__(()) *)(plExtensionRegistry*, plExtension*))       pl_load_library_function(&tExtensionInfo.tLibrary, ptExtension->pcUnloadFunc);
        #endif

        PL_ASSERT(tExtensionInfo.pl_load);
        PL_ASSERT(tExtensionInfo.pl_unload);

        tExtensionInfo.pl_load(gptExtRegistry, ptExtension, false);
        pl_sb_push(gptExtRegistry->sbtExtensions, tExtensionInfo);
    }
    else
    {
        PL_ASSERT(false && "extension not loaded");
    }
}

void
pl_unload_extension(plExtension* ptExtension)
{
    PL_ASSERT(gptExtRegistry && "global extension registry not set");

    for(uint32_t i = 0; i < pl_sb_size(gptExtRegistry->sbtExtensions); i++)
    {
        if(strcmp(ptExtension->pcExtensionName, gptExtRegistry->sbtExtensions[i].pcName) == 0)
        {
            gptExtRegistry->sbtExtensions[i].pl_unload(gptExtRegistry, ptExtension);
            pl_sb_del_swap(gptExtRegistry->sbtExtensions, i);
            return;
        }
    }

    PL_ASSERT(false && "extension not found");
}

void*
pl_get_api(plExtension* ptExtension, const char* pcApiName)
{
    for(uint32_t i = 0; i < ptExtension->uApiCount; i++)
    {
        if(strcmp(ptExtension->atApis[i].pcName, pcApiName) == 0)
        {
            return ptExtension->atApis[i].pApi;
        }
    }
    PL_ASSERT(false && "api not found");
    return NULL;
}


#endif // PL_EXT_IMPLEMENTATION
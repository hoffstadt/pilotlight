/*
   pl.h
     * settings & common functions
*/

/*
Index of this file:
// [SECTION] includes
// [SECTION] settings
// [SECTION] helper macros
// [SECTION] misc
*/

#ifndef PL_H
#define PL_H

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdint.h>  // uint32_t
#include <stdbool.h> // bool

//-----------------------------------------------------------------------------
// [SECTION] settings
//-----------------------------------------------------------------------------

#define PL_MAX_FRAMES_IN_FLIGHT 2
#define PL_MAX_NAME_LENGTH 1024
#define PL_USE_STB_SPRINTF

//-----------------------------------------------------------------------------
// [SECTION] helper macros
//-----------------------------------------------------------------------------

#define PL_DECLARE_STRUCT(name) typedef struct name ##_t name

#ifndef PL_ASSERT
#include <assert.h>
#define PL_ASSERT(x) assert(x)
#endif

#if defined(_MSC_VER) //  Microsoft 
    #define PL_EXPORT extern "C" __declspec(dllexport)
#elif defined(__GNUC__) //  GCC
    #define PL_EXPORT __attribute__((visibility("default")))
#else //  do nothing and hope for the best?
    #define PL_EXPORT
    #pragma warning Unknown dynamic link import/export semantics.
#endif

//-----------------------------------------------------------------------------
// [SECTION] misc
//-----------------------------------------------------------------------------

#ifdef _WIN32
#define PL_VULKAN_BACKEND
#elif defined(__APPLE__)
#define PL_METAL_BACKEND
#else // linux
#define PL_VULKAN_BACKEND
#endif

#ifdef PL_USE_STB_SPRINTF
#include "stb_sprintf.h"
#endif

#ifndef pl_sprintf
#ifdef PL_USE_STB_SPRINTF
    #define pl_sprintf stbsp_sprintf
    #define pl_vsprintf stbsp_vsprintf
#else
    #define pl_sprintf sprintf
    #define pl_vsprintf vsprintf
#endif
#endif

#endif // PL_H
#ifndef PL_DRAW_EXTENSION_H
#define PL_DRAW_EXTENSION_H

#include "pl_ext.h"

typedef struct _plDrawLayer plDrawLayer;
typedef struct _plFont plFont;
typedef union _plVec2 plVec2;;
typedef union _plVec4 plVec4;

typedef struct _plDrawExtension
{
    void (*pl_add_text)(plDrawLayer* layer, plFont* font, float size, plVec2 p, plVec4 color, const char* text);
} plDrawExtension;

static inline void
pl_get_draw_extension_info(plExtension* ptExtension)
{
    ptExtension->pcExtensionName = "pl_draw_extension";
    ptExtension->pcTransName = "./pl_draw_extension_";
    ptExtension->pcLockName = "./pl_draw_extension_lock.tmp";
    ptExtension->pcLoadFunc = "pl_load_draw_extension";
    ptExtension->pcUnloadFunc = "pl_unload_draw_extension";
    ptExtension->uApiCount = 1;

    #ifdef _WIN32
        ptExtension->pcLibName = "./pl_draw_extension.dll";
    #else
        ptExtension->pcLibName = "./pl_draw_extension.so";
    #endif
}

#endif // PL_DRAW_EXTENSION_H
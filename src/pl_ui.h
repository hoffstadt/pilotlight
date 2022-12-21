/*
   pl_ui, v0.1 (WIP)
*/

/*
Index of this file:
// [SECTION] header mess
// [SECTION] includes
// [SECTION] public api
// [SECTION] c file
*/

//-----------------------------------------------------------------------------
// [SECTION] header mess
//-----------------------------------------------------------------------------

#ifndef PL_UI_H
#define PL_UI_H

//-----------------------------------------------------------------------------
// [SECTION] defines
//-----------------------------------------------------------------------------

#ifndef PL_DECLARE_STRUCT
    #define PL_DECLARE_STRUCT(name) typedef struct _ ## name  name
#endif

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdbool.h> // bool
#include <stdint.h>  // uint*_t
#include "pl_math.inc"

#include "pl_draw.h" // temporary

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

// basic types
PL_DECLARE_STRUCT(plUiContext);
PL_DECLARE_STRUCT(plUiStyle);
PL_DECLARE_STRUCT(plUiWindow);
PL_DECLARE_STRUCT(plUiPrevItemData);
PL_DECLARE_STRUCT(plUiNextWindowData);

// enums
typedef int plUiNextWindowFlags;

//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

void pl_ui_new_frame(plUiContext* ptCtx);
void pl_ui_end_frame(void);
void pl_ui_render(void);

// windows
bool pl_ui_begin_window(const char* pcName, bool* pbOpen, bool bAutoSize);
void pl_ui_end_window(void);

// window utils
void pl_ui_set_next_window_pos (plVec2 tPos);
void pl_ui_set_next_window_size(plVec2 tSize);

// widgets
bool pl_ui_button      (const char* pcText);
bool pl_ui_checkbox    (const char* pcText, bool* pbValue);
void pl_ui_text        (const char* pcFmt, ...);
void pl_ui_text_v      (const char* pcFmt, va_list args);
void pl_ui_progress_bar(float fFraction, plVec2 tSize, const char* pcOverlay);

// trees
bool pl_ui_collapsing_header(const char* pcText, bool* pbOpenState);
bool pl_ui_tree_node        (const char* pcText, bool* pbOpenState);
void pl_ui_tree_pop         (void);

// layout
void pl_ui_same_line(float fOffsetFromStart, float fSpacing);
void pl_ui_align_text(void);

// state query
bool pl_ui_was_last_item_hovered(void);

// styling
void pl_ui_set_dark_theme(plUiContext* ptCtx);

//-----------------------------------------------------------------------------
// [SECTION] enums
//-----------------------------------------------------------------------------

enum _plUiNextWindowFlags
{
    PL_NEXT_WINDOW_DATA_FLAGS_NONE          = 0,
    PL_NEXT_WINDOW_DATA_FLAGS_HAS_POS       = 1 << 0,
    PL_NEXT_WINDOW_DATA_FLAGS_HAS_SIZE      = 1 << 1,
    PL_NEXT_WINDOW_DATA_FLAGS_HAS_COLLAPSED = 1 << 2,   
};

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plUiStyle
{
    // style
    float  fTitlePadding;
    float  fFontSize;
    float  fIndentSize;
    float  fWindowHorizontalPadding;
    float  fWindowVerticalPadding;
    plVec2 tItemSpacing;
    plVec2 tInnerSpacing;
    plVec2 tFramePadding;


    // colors
    plVec4 tTitleActiveCol;
    plVec4 tTitleBgCol;
    plVec4 tWindowBgColor;
    plVec4 tButtonCol;
    plVec4 tButtonHoveredCol;
    plVec4 tButtonActiveCol;
    plVec4 tTextCol;
    plVec4 tProgressBarCol;
    plVec4 tCheckmarkCol;
    plVec4 tFrameBgCol;
    plVec4 tFrameBgHoveredCol;
    plVec4 tFrameBgActiveCol;
    plVec4 tHeaderCol;
    plVec4 tHeaderHoveredCol;
    plVec4 tHeaderActiveCol;
} plUiStyle;

typedef struct _plUiNextWindowData
{
    plUiNextWindowFlags tFlags;
    plVec2              tPos;
    plVec2              tSize;
    bool                bCollapsed;
} plUiNextWindowData;

typedef struct _plUiPrevItemData
{
    bool bHovered;
} plUiPrevItemData;

typedef struct _plUiWindow
{
    uint32_t    uId;
    const char* pcName;
    plVec2      tPos;
    plVec2      tContentPos;
    plVec2      tContentMaxSize;
    plVec2      tSize;
    plVec2      tFullSize;
    plVec2      tCursorPos;
    plVec2      tCursorPrevLine;
    float       fTextVerticalOffset;
    uint32_t    uTreeDepth;

    // state
    bool        bHovered;
    bool        bActive;
    bool        bDragging;
    bool        bResizing;
    bool        bAutoSize;
    bool        bCollapsed;

    uint64_t    ulFrameActivated;
    uint64_t    ulFrameHovered;
    
    plDrawLayer* ptBgLayer;
    plDrawLayer* ptFgLayer;
} plUiWindow;

typedef struct _plUiContext
{

    plUiStyle tStyle;

    // prev/next state
    plUiNextWindowData tNextWindowData;
    plUiPrevItemData   tPrevItemData;

    // state
    uint32_t uToggledId;
    uint32_t uNextToggleId;
    uint32_t uHoveredId;
    uint32_t uNextHoveredId;
    uint32_t uActiveId;
    uint32_t uNextActiveId;
    uint32_t uActiveWindowId;
    uint32_t uNextActiveWindowId;
    uint32_t uHoveredWindowId;
    uint32_t uNextHoveredWindowId;

    // windows
    plUiWindow*  sbtWindows;
    plUiWindow** sbtFocusedWindows;
    plUiWindow** sbtSortingWindows;
    plUiWindow*  ptCurrentWindow;
    plUiWindow*  ptHoveredWindow;
    plUiWindow*  ptMovingWindow;
    plUiWindow*  ptActiveWindow;
    plUiWindow*  ptFocusedWindow;

    plDrawList tDrawlist;
    plFont*    ptFont;
} plUiContext;

#endif // PL_UI_H

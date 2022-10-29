/*
   pl_io.h, v0.1 (WIP)
   * no dependencies
   * simple
   Do this:
        #define PL_IO_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.
   // i.e. it should look like this:
   #include ...
   #include ...
   #include ...
   #define PL_IO_IMPLEMENTATION
   #include "pl_io.h"
*/

/*
Index of this file:
// [SECTION] defines
// [SECTION] includes
// [SECTION] forward declarations & basic types
// [SECTION] global context
// [SECTION] public api
// [SECTION] structs
// [SECTION] enums
// [SECTION] c file
*/

#ifndef PL_IO_H
#define PL_IO_H

//-----------------------------------------------------------------------------
// [SECTION] defines
//-----------------------------------------------------------------------------

#ifndef PL_DECLARE_STRUCT
#define PL_DECLARE_STRUCT(name) typedef struct name ##_t name
#endif

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdbool.h> // bool
#include <stdint.h>  // uint*_t
#include "pl_math.h"

//-----------------------------------------------------------------------------
// [SECTION] forward declarations & basic types
//-----------------------------------------------------------------------------

// forward declarations
PL_DECLARE_STRUCT(plIOContext);
PL_DECLARE_STRUCT(plInputEvent);

// enums
typedef int plKey;
typedef int plMouseButton;

//-----------------------------------------------------------------------------
// [SECTION] global context
//-----------------------------------------------------------------------------

extern plIOContext* gptIOContext;

//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

// context
void         pl_initialize_io_context(plIOContext* ptCtx);
void         pl_cleanup_io_context   (void);
void         pl_set_io_context       (plIOContext* ptCtx);
plIOContext* pl_get_io_context       (void);
void         pl_new_io_frame         (void);

// mouse input
bool         pl_is_mouse_down          (plMouseButton tButton);
bool         pl_is_mouse_clicked       (plMouseButton tButton, bool bRepeat);
bool         pl_is_mouse_released      (plMouseButton tButton);
bool         pl_is_mouse_double_clicked(plMouseButton tButton);
bool         pl_is_mouse_hovering_rect (plVec2 tMin, plVec2 tMax);
bool         pl_is_mouse_dragging      (plMouseButton tButton, float fThreshold);
void         pl_reset_mouse_drag_delta (plMouseButton tButton);
plVec2       pl_get_mouse_drag_delta   (plMouseButton tButton, float fThreshold);
plVec2       pl_get_mouse_pos          (void);
bool         pl_is_mouse_pos_valid     (plVec2 tPos);

// input functions
void         pl_add_key_event         (plKey tKey, bool bDown);
void         pl_add_mouse_pos_event   (float fX, float fY);
void         pl_add_mouse_button_event(int iButton, bool bDown);
void         pl_add_mouse_wheel_event (float fX, float fY);

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct plIOContext_t
{
    double   dTime;
    float    fDeltaTime;
    float    afMainViewportSize[2];
    uint64_t ulFrameCount;


    // settings
    float  fMouseDragThreshold;      // default 6.0f
    float  fMouseDoubleClickTime;    // default 0.3f seconds
    float  fMouseDoubleClickMaxDist; // default 6.0f
    float  fKeyRepeatDelay;          // default 0.275f
    float  fKeyRepeatRate;           // default 0.050f
    void*  pUserData;
    void*  pBackendPlatformData;
    void*  pBackendRendererData;

    // [INTERNAL]
    plInputEvent* _sbtInputEvents;

    // main input state
    plVec2 _tMousePos;
    bool   _abMouseDown[5];

    // other state
    plVec2   _tLastValidMousePos;
    plVec2   _tMouseDelta;
    plVec2   _tMousePosPrev;              // previous mouse position
    plVec2   _atMouseClickedPos[5];       // position when clicked
    double   _adMouseClickedTime[5];      // time of last click
    bool     _abMouseClicked[5];          // mouse went from !down to down
    uint32_t _auMouseClickedCount[5];     // 
    uint32_t _auMouseClickedLastCount[5]; // 
    bool     _abMouseReleased[5];         // mouse went from down to !down
    float    _afMouseDownDuration[5];     // duration mouse button has been down (0.0f == just clicked)
    float    _afMouseDownDurationPrev[5]; // previous duration of mouse button down
    float    _afMouseDragMaxDistSqr[5];   // squared max distance mouse traveled from clicked position

} plIOContext;

//-----------------------------------------------------------------------------
// [SECTION] enums
//-----------------------------------------------------------------------------

enum plMouseButton_
{
    PL_MOUSE_BUTTON_LEFT   = 0,
    PL_MOUSE_BUTTON_RIGHT  = 1,
    PL_MOUSE_BUTTON_MIDDLE = 2,
    PL_MOUSE_BUTTON_COUNT  = 5    
};

enum plKey_
{
    PL_KEY_NONE = 0,
    PL_KEY_TAB,
    PL_KEY_LEFT_ARROW,
    PL_KEY_RIGHT_ARROW,
    PL_KEY_UP_ARROW,
    PL_KEY_DOWN_ARROW,
    PL_KEY_PAGE_UP,
    PL_KEY_PAGE_DOWN,
    PL_KEY_HOME,
    PL_KEY_END,
    PL_KEY_INSERT,
    PL_KEY_DELETE,
    PL_KEY_BACKSPACE,
    PL_KEY_SPACE,
    PL_KEY_ENTER,
    PL_KEY_ESCAPE,
    PL_KEY_LEFT_CTRL,
    PL_KEY_LEFT_SHIFT,
    PL_KEY_LEFT_ALT,
    PL_KEY_LEFT_SUPER,
    PL_KEY_RIGHT_CTRL,
    PL_KEY_RIGHT_SHIFT,
    PL_KEY_RIGHT_ALT,
    PL_KEY_RIGHT_SUPER,
    PL_KEY_MENU,
    PL_KEY_0,
    PL_KEY_1,
    PL_KEY_2,
    PL_KEY_3,
    PL_KEY_4,
    PL_KEY_5,
    PL_KEY_6,
    PL_KEY_7,
    PL_KEY_8,
    PL_KEY_9,
    PL_KEY_A,
    PL_KEY_B,
    PL_KEY_C,
    PL_KEY_D,
    PL_KEY_E,
    PL_KEY_F,
    PL_KEY_G,
    PL_KEY_H,
    PL_KEY_I,
    PL_KEY_J,
    PL_KEY_K,
    PL_KEY_L,
    PL_KEY_M,
    PL_KEY_N,
    PL_KEY_O,
    PL_KEY_P,
    PL_KEY_Q,
    PL_KEY_R,
    PL_KEY_S,
    PL_KEY_T,
    PL_KEY_U,
    PL_KEY_V,
    PL_KEY_W,
    PL_KEY_X,
    PL_KEY_Y,
    PL_KEY_Z,
    PL_KEY_F1,
    PL_KEY_F2,
    PL_KEY_F3,
    PL_KEY_F4,
    PL_KEY_F5,
    PL_KEY_F6,
    PL_KEY_F7,
    PL_KEY_F8,
    PL_KEY_F9,
    PL_KEY_F10,
    PL_KEY_F11,
    PL_KEY_F12,
    PL_KEY_F13,
    PL_KEY_F14,
    PL_KEY_F15,
    PL_KEY_F16,
    PL_KEY_F17,
    PL_KEY_F18,
    PL_KEY_F19,
    PL_KEY_F20,
    PL_KEY_F21,
    PL_KEY_F22,
    PL_KEY_F23,
    PL_KEY_F24,
    PL_KEY_APOSTROPHE,    // '
    PL_KEY_COMMA,         // ,
    PL_KEY_MINUS,         // -
    PL_KEY_PERIOD,        // .
    PL_KEY_SLASH,         // /
    PL_KEY_SEMICOLON,     // ;
    PL_KEY_EQUAL,         // =
    PL_KEY_LEFT_BRACKET,  // [
    PL_KEY_BACKSLASH,     // \ (this text inhibit multiline comment caused by backslash)
    PL_KEY_RIGHT_BRACKET, // ]
    PL_KEY_GRAVE_ACCENT,  // `
    PL_KEY_CAPS_LOCK,
    PL_KEY_SCROLL_LOCK,
    PL_KEY_NUM_LOCK,
    PL_KEY_PRINT_SCREEN,
    PL_KEY_PAUSE,
    PL_KEY_KEYPAD_0,
    PL_KEY_KEYPAD_1,
    PL_KEY_KEYPAD_2,
    PL_KEY_KEYPAD_3,
    PL_KEY_KEYPAD_4,
    PL_KEY_KEYPAD_5,
    PL_KEY_KEYPAD_6,
    PL_KEY_KEYPAD_7,
    PL_KEY_KEYPAD_8,
    PL_KEY_KEYPAD_9,
    PL_KEY_KEYPAD_DECIMAL,
    PL_KEY_KEYPAD_DIVIDE,
    PL_KEY_KEYPAD_MULTIPLY,
    PL_KEY_KEYPAD_SUBTRACT,
    PL_KEY_KEYPAD_ADD,
    PL_KEY_KEYPAD_ENTER,
    PL_KEY_KEYPAD_EQUAL,
    PL_KEY_MOD_CTRL,
    PL_KEY_MOD_SHIFT,
    PL_KEY_MOD_ALT,
    PL_KEY_MOD_SUPER,
    
    PL_KEY_COUNT // no valid plKey_ is ever greater than this value
};

#endif // PL_IO_H

//-----------------------------------------------------------------------------
// [SECTION] c file
//-----------------------------------------------------------------------------

/*
Index of this file:
// [SECTION] header mess
// [SECTION] includes
// [SECTION] forward declarations & basic types
// [SECTION] internal & opaque structs
// [SECTION] global context
// [SECTION] internal api
// [SECTION] public api implementations
// [SECTION] internal api implementations
*/

//-----------------------------------------------------------------------------
// [SECTION] header mess
//-----------------------------------------------------------------------------

#ifdef PL_IO_IMPLEMENTATION

#ifndef PL_ASSERT
#include <assert.h>
#define PL_ASSERT(x) assert(x)
#endif

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <string.h> // memset
#include <float.h>  // FLT_MAX
#include "pl_ds.h"  // stretchy buffers

//-----------------------------------------------------------------------------
// [SECTION] forward declarations & basic types
//-----------------------------------------------------------------------------

typedef enum
{
    PL_INPUT_EVENT_TYPE_NONE = 0,
    PL_INPUT_EVENT_TYPE_MOUSE_POS,
    PL_INPUT_EVENT_TYPE_MOUSE_WHEEL,
    PL_INPUT_EVENT_TYPE_MOUSE_BUTTON,
    PL_INPUT_EVENT_TYPE_KEY,
    
    PL_INPUT_EVENT_TYPE_COUNT
} plInputEventType;

typedef enum
{
    PL_INPUT_EVENT_SOURCE_NONE = 0,
    PL_INPUT_EVENT_SOURCE_MOUSE,
    PL_INPUT_EVENT_SOURCE_KEYBOARD,
    
    PL_INPUT_EVENT_SOURCE_COUNT
} plInputEventSource;

//-----------------------------------------------------------------------------
// [SECTION] internal & opaque structs
//-----------------------------------------------------------------------------

typedef struct plInputEvent_t
{
    plInputEventType   tType;
    plInputEventSource tSource;

    union
    {
        struct // mouse pos event
        {
            float fPosX;
            float fPosY;
        };

        struct // mouse wheel event
        {
            float fWheelX;
            float fWheelY;
        };
        
        struct // mouse button event
        {
            int  iButton;
            bool bMouseDown;
        };

        struct // key event
        {
            plKey tKey;
            bool  bKeyDown;
        };
        
    };

} plInputEvent;

//-----------------------------------------------------------------------------
// [SECTION] global context
//-----------------------------------------------------------------------------

plIOContext* gptIOContext = NULL;

//-----------------------------------------------------------------------------
// [SECTION] internal api 
//-----------------------------------------------------------------------------

#define PL_IO_VEC2_LENGTH_SQR(vec) (((vec).x * (vec).x) + ((vec).y * (vec).y))
#define PL_IO_VEC2_SUBTRACT(v1, v2) (plVec2){ (v1).x - (v2).x, (v1).y - (v2).y}
#define PL_IO_MAX(x, y) (x) > (y) ? (x) : (y)

static void pl__update_events(void);
static void pl__update_mouse_inputs(void);
static int  pl__calc_typematic_repeat_amount(float fT0, float fT1, float fRepeatDelay, float fRepeatRate);

//-----------------------------------------------------------------------------
// [SECTION] public api implementation
//-----------------------------------------------------------------------------

void
pl_initialize_io_context(plIOContext* ptCtx)
{
    memset(ptCtx, 0, sizeof(plIOContext));
    gptIOContext = ptCtx;
    gptIOContext->fMouseDoubleClickTime    = 0.3f;
    gptIOContext->fMouseDoubleClickMaxDist = 6.0f;
    gptIOContext->fMouseDragThreshold      = 6.0f;
    gptIOContext->fKeyRepeatDelay          = 0.275f;
    gptIOContext->fKeyRepeatRate           = 0.050f;
}

void
pl_cleanup_io_context(void)
{
    pl_sb_free(gptIOContext->_sbtInputEvents);
    gptIOContext = NULL;
}

void
pl_set_io_context(plIOContext* ptCtx)
{
    gptIOContext = ptCtx;
}

plIOContext*
pl_get_io_context(void)
{
    return gptIOContext;
}

void
pl_new_io_frame(void)
{
    plIOContext* ptIO = gptIOContext;

    ptIO->dTime += (double)ptIO->fDeltaTime;
    ptIO->ulFrameCount++;

    pl__update_events();
    pl__update_mouse_inputs();
}

void
pl_add_key_event(plKey tKey, bool bDown)
{
    plInputEvent tEvent = {
        .tType    = PL_INPUT_EVENT_TYPE_KEY,
        .tSource  = PL_INPUT_EVENT_SOURCE_KEYBOARD,
        .tKey     = tKey,
        .bKeyDown = bDown
    };
    pl_sb_push(gptIOContext->_sbtInputEvents, tEvent);
}

void
pl_add_mouse_pos_event(float fX, float fY)
{
    plInputEvent tEvent = {
        .tType    = PL_INPUT_EVENT_TYPE_MOUSE_POS,
        .tSource  = PL_INPUT_EVENT_SOURCE_MOUSE,
        .fPosX    = fX,
        .fPosY    = fY
    };
    pl_sb_push(gptIOContext->_sbtInputEvents, tEvent);
}

void
pl_add_mouse_button_event(int iButton, bool bDown)
{
    plInputEvent tEvent = {
        .tType      = PL_INPUT_EVENT_TYPE_MOUSE_BUTTON,
        .tSource    = PL_INPUT_EVENT_SOURCE_MOUSE,
        .iButton    = iButton,
        .bMouseDown = bDown
    };
    pl_sb_push(gptIOContext->_sbtInputEvents, tEvent);
}

void
pl_add_mouse_wheel_event(float fX, float fY)
{
    plInputEvent tEvent = {
        .tType    = PL_INPUT_EVENT_TYPE_MOUSE_WHEEL,
        .tSource  = PL_INPUT_EVENT_SOURCE_MOUSE,
        .fWheelX  = fX,
        .fWheelY  = fY
    };
    pl_sb_push(gptIOContext->_sbtInputEvents, tEvent);
}

bool
pl_is_mouse_down(plMouseButton tButton)
{
    return gptIOContext->_abMouseDown[tButton];
}

bool
pl_is_mouse_clicked(plMouseButton tButton, bool bRepeat)
{
    plIOContext* ptIO = gptIOContext;
    if(!ptIO->_abMouseDown[tButton])
        return false;
    const float fT = ptIO->_afMouseDownDuration[tButton];
    if(fT == 0.0f)
        return true;
    if(bRepeat && fT > ptIO->fKeyRepeatDelay)
        return pl__calc_typematic_repeat_amount(fT - ptIO->fDeltaTime, fT, ptIO->fKeyRepeatDelay, ptIO->fKeyRepeatRate) > 0;
    return false;
}

bool
pl_is_mouse_released(plMouseButton tButton)
{
    return gptIOContext->_abMouseReleased[tButton];
}

bool
pl_is_mouse_double_clicked(plMouseButton tButton)
{
    return gptIOContext->_auMouseClickedCount[tButton] == 2;
}

bool
pl_is_mouse_dragging(plMouseButton tButton, float fThreshold)
{
    plIOContext* ptIO = gptIOContext;
    if(!ptIO->_abMouseDown[tButton])
        return false;
    if(fThreshold < 0.0f)
        fThreshold = ptIO->fMouseDragThreshold;
    return ptIO->_afMouseDragMaxDistSqr[tButton] >= fThreshold * fThreshold;
}

void
pl_reset_mouse_drag_delta(plMouseButton tButton)
{
    gptIOContext->_atMouseClickedPos[tButton] = gptIOContext->_tMousePos;
}

plVec2
pl_get_mouse_drag_delta(plMouseButton tButton, float fThreshold)
{
    plIOContext* ptIO = gptIOContext;
    if(fThreshold < 0.0f)
        fThreshold = ptIO->fMouseDragThreshold;
    if(ptIO->_abMouseDown[tButton] || ptIO->_abMouseReleased[tButton])
    {
        if(ptIO->_afMouseDragMaxDistSqr[tButton] >= fThreshold * fThreshold)
        {
            if(pl_is_mouse_pos_valid(ptIO->_tMousePos) && pl_is_mouse_pos_valid(ptIO->_atMouseClickedPos[tButton]))
                return PL_IO_VEC2_SUBTRACT(ptIO->_tMousePos, ptIO->_atMouseClickedPos[tButton]);
        }
    }
    return (plVec2){0};
}

plVec2
pl_get_mouse_pos(void)
{
    return gptIOContext->_tMousePos;
}

bool
pl_is_mouse_pos_valid(plVec2 tPos)
{
    return tPos.x >= -FLT_MAX && tPos.y >= -FLT_MAX;
}

//-----------------------------------------------------------------------------
// [SECTION] internal api implementation
//-----------------------------------------------------------------------------

static void
pl__update_events(void)
{
    plIOContext* ptIO = gptIOContext;

    for(uint32_t i = 0; i < pl_sb_size(ptIO->_sbtInputEvents); i++)
    {
        plInputEvent* ptEvent = &ptIO->_sbtInputEvents[i];

        switch(ptEvent->tType)
        {
            case PL_INPUT_EVENT_TYPE_MOUSE_POS:
            {
                ptIO->_tMousePos.x = ptEvent->fPosX;
                ptIO->_tMousePos.y = ptEvent->fPosY;
                break;
            }

            case PL_INPUT_EVENT_TYPE_MOUSE_WHEEL:
            {
                break;
            }

            case PL_INPUT_EVENT_TYPE_MOUSE_BUTTON:
            {
                PL_ASSERT(ptEvent->iButton >= 0 && ptEvent->iButton < PL_MOUSE_BUTTON_COUNT);
                ptIO->_abMouseDown[ptEvent->iButton] = ptEvent->bMouseDown;
                break;
            }

            case PL_INPUT_EVENT_TYPE_KEY:
            {
                break;
            }

            default:
            {
                PL_ASSERT(false && "unknown input event type");
                break;
            }
        }
    }

    pl_sb_reset(ptIO->_sbtInputEvents);
}

static void
pl__update_mouse_inputs(void)
{
    plIOContext* ptIO = gptIOContext;

    if(pl_is_mouse_pos_valid(ptIO->_tMousePos))
    {
        ptIO->_tMousePos.x = floorf(ptIO->_tMousePos.x);
        ptIO->_tMousePos.y = floorf(ptIO->_tMousePos.y);
        ptIO->_tLastValidMousePos = ptIO->_tMousePos;
    }

    if(pl_is_mouse_pos_valid(ptIO->_tMousePos) && pl_is_mouse_pos_valid(ptIO->_tMousePosPrev))
        ptIO->_tMouseDelta = PL_IO_VEC2_SUBTRACT(ptIO->_tMousePos, ptIO->_tMousePosPrev);
    else
    {
        ptIO->_tMouseDelta.x = 0.0f;
        ptIO->_tMouseDelta.y = 0.0f;
    }

    ptIO->_tMousePosPrev = ptIO->_tMousePos;

    for(uint32_t i = 0; i < PL_MOUSE_BUTTON_COUNT; i++)
    {
        ptIO->_abMouseClicked[i] = ptIO->_abMouseDown[i] && ptIO->_afMouseDownDuration[i] < 0.0f;
        ptIO->_auMouseClickedCount[i] = 0;
        ptIO->_abMouseReleased[i] = !ptIO->_abMouseDown[i] && ptIO->_afMouseDownDuration[i] >= 0.0f;
        ptIO->_afMouseDownDurationPrev[i] = ptIO->_afMouseDownDuration[i];
        ptIO->_afMouseDownDuration[i] = ptIO->_abMouseDown[i] ? (ptIO->_afMouseDownDuration[i] < 0.0f ? 0.0f : ptIO->_afMouseDownDuration[i] + ptIO->fDeltaTime) : -1.0f;

        if(ptIO->_abMouseClicked[i])
        {

            bool bIsRepeatedClick = false;
            if((float)(ptIO->dTime - ptIO->_adMouseClickedTime[i]) < ptIO->fMouseDoubleClickTime)
            {
                plVec2 tDeltaFromClickPos = pl_is_mouse_pos_valid(ptIO->_tMousePos) ? PL_IO_VEC2_SUBTRACT(ptIO->_tMousePos, ptIO->_atMouseClickedPos[i]) : (plVec2){0};

                if(PL_IO_VEC2_LENGTH_SQR(tDeltaFromClickPos) < ptIO->fMouseDoubleClickMaxDist * ptIO->fMouseDoubleClickMaxDist)
                    bIsRepeatedClick = true;
            }

            if(bIsRepeatedClick)
                ptIO->_auMouseClickedLastCount[i]++;
            else
                ptIO->_auMouseClickedLastCount[i] = 1;

            ptIO->_adMouseClickedTime[i] = ptIO->dTime;
            ptIO->_atMouseClickedPos[i] = ptIO->_tMousePos;
            ptIO->_afMouseDragMaxDistSqr[i] = 0.0f;
            ptIO->_auMouseClickedCount[i] = ptIO->_auMouseClickedLastCount[i];
        }
        else if(ptIO->_abMouseDown[i])
        {
            float fDeltaSqrClickPos = pl_is_mouse_pos_valid(ptIO->_tMousePos) ? PL_IO_VEC2_LENGTH_SQR(PL_IO_VEC2_SUBTRACT(ptIO->_tMousePos, ptIO->_atMouseClickedPos[i])) : 0.0f;
            ptIO->_afMouseDragMaxDistSqr[i] = PL_IO_MAX(fDeltaSqrClickPos, ptIO->_afMouseDragMaxDistSqr[i]);
        }


    }
}

static int
pl__calc_typematic_repeat_amount(float fT0, float fT1, float fRepeatDelay, float fRepeatRate)
{
    if(fT1 == 0.0f)
        return 1;
    if(fT0 >= fT1)
        return 0;
    if(fRepeatRate <= 0.0f)
        return (fT0 < fRepeatDelay) && (fT1 >= fRepeatDelay);
    
    const int iCountT0 = (fT0 < fRepeatDelay) ? -1 : (int)((fT0 - fRepeatDelay) / fRepeatRate);
    const int iCountT1 = (fT1 < fRepeatDelay) ? -1 : (int)((fT1 - fRepeatDelay) / fRepeatRate);
    const int iCount = iCountT1 - iCountT0;
    return iCount;
}

#endif // PL_IO_IMPLEMENTATION
/*
   win32_pl.c
*/

/*
Index of this file:
// [SECTION] header mess
// [SECTION] defines
// [SECTION] includes
// [SECTION] forward declarations
// [SECTION] globals
// [SECTION] entry point
// [SECTION] windows procedure
// [SECTION] implementations
// [SECTION] unity build
*/

//-----------------------------------------------------------------------------
// [SECTION] header mess
//-----------------------------------------------------------------------------

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "ws2_32.lib") // winsock2 library

#ifdef _DEBUG
#pragma comment(lib, "ucrtd.lib")
#else
#pragma comment(lib, "ucrt.lib")
#endif

//-----------------------------------------------------------------------------
// [SECTION] defines
//-----------------------------------------------------------------------------

#define PL_VK_KEYPAD_ENTER (VK_RETURN + 256)
#define WIN32_LEAN_AND_MEAN

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include "pilotlight.h" // data registry, api registry, extension registry
#include "pl_io.h"      // io context
#include "pl_os.h"      // os apis
#include "pl_ds.h"      // hashmap

#include <float.h>      // FLT_MAX
#include <stdlib.h>     // exit
#include <stdio.h>      // printf
#include <wchar.h>      // mbsrtowcs, wcsrtombs
#include <winsock2.h>   // sockets
#include <windows.h>
#include <windowsx.h>   // GET_X_LPARAM(), GET_Y_LPARAM()

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

// internal
LRESULT CALLBACK pl__windows_procedure(HWND tHwnd, UINT tMsg, WPARAM tWParam, LPARAM tLParam);
void             pl__convert_to_wide_string(const char* narrowValue, wchar_t* wideValue);
void             pl__render_frame(void);
void             pl_update_mouse_cursor(void);
inline bool      pl__is_vk_down           (int iVk)         { return (GetKeyState(iVk) & 0x8000) != 0;}
plKey            pl__virtual_key_to_pl_key(WPARAM tWParam);

// os services
void  pl__read_file            (const char* pcFile, unsigned* puSize, char* pcBuffer, const char* pcMode);
void  pl__copy_file            (const char* pcSource, const char* pcDestination, unsigned* puSize, char* pcBuffer);
void  pl__create_udp_socket    (plSocket* ptSocketOut, bool bNonBlocking);
void  pl__bind_udp_socket      (plSocket* ptSocket, int iPort);
bool  pl__send_udp_data        (plSocket* ptFromSocket, const char* pcDestIP, int iDestPort, void* pData, size_t szSize);
bool  pl__get_udp_data         (plSocket* ptSocket, void* pData, size_t szSize);
bool  pl__has_library_changed  (plSharedLibrary* ptLibrary);
bool  pl__load_library         (plSharedLibrary* ptLibrary, const char* pcName, const char* pcTransitionalName, const char* pcLockFile);
void  pl__reload_library       (plSharedLibrary* ptLibrary);
void* pl__load_library_function(plSharedLibrary* ptLibrary, const char* pcName);
int   pl__sleep                (uint32_t millisec);

static inline FILETIME
pl__get_last_write_time(const char* pcFilename)
{
    FILETIME tLastWriteTime = {0};
    
    WIN32_FILE_ATTRIBUTE_DATA tData = {0};
    if(GetFileAttributesExA(pcFilename, GetFileExInfoStandard, &tData))
        tLastWriteTime = tData.ftLastWriteTime;
    
    return tLastWriteTime;
}

typedef struct _plWin32SharedLibrary
{
    HMODULE   tHandle;
    FILETIME  tLastWriteTime;
} plWin32SharedLibrary;

//-----------------------------------------------------------------------------
// [SECTION] globals
//-----------------------------------------------------------------------------

// apis
static plDataRegistryApiI*      gptDataRegistry = NULL;
static plApiRegistryApiI*       gptApiRegistry = NULL;
static plExtensionRegistryApiI* gptExtensionRegistry = NULL;

// win32 stuff
static HWND  gtHandle = NULL;
static plSharedLibrary gtAppLibrary = {0};
static void* gpUserData = NULL;
static bool  gbRunning = true;
static bool  gbFirstRun = true;
static bool  gbEnableVirtualTerminalProcessing = true;
static INT64 ilTime = 0;
static INT64 ilTicksPerSecond = 0;
static HWND  tMouseHandle = NULL;
static bool  bMouseTracked = true;

// memory tracking
static plMemoryContext gtMemoryContext = {0};
static plHashMap gtMemoryHashMap = {0};

// app function pointers
static void* (*pl_app_load)    (plApiRegistryApiI* ptApiRegistry, void* userData);
static void  (*pl_app_shutdown)(void* userData);
static void  (*pl_app_resize)  (void* userData);
static void  (*pl_app_update)  (void* userData);

//-----------------------------------------------------------------------------
// [SECTION] entry point
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{

    for(int i = 1; i < argc; i++)
    {
        
        if(strcmp(argv[i], "--disable_vt") == 0)
        {
            gbEnableVirtualTerminalProcessing = false;
        }
    }

    // initialize winsock
    WSADATA tWsaData = {0};
    if(WSAStartup(MAKEWORD(2, 2), &tWsaData) != 0)
    {
        printf("Failed to start winsock with error code: %d\n", WSAGetLastError());
        return -1;
    }

    // load apis
    gtMemoryContext.ptHashMap = &gtMemoryHashMap;
    gptApiRegistry = pl_load_core_apis();

    static plLibraryApiI tApi3 = {
        .has_changed   = pl__has_library_changed,
        .load          = pl__load_library,
        .load_function = pl__load_library_function,
        .reload        = pl__reload_library
    };

    static plFileApiI tApi4 = {
        .copy = pl__copy_file,
        .read = pl__read_file
    };
    
    static plUdpApiI tApi5 = {
        .create_socket = pl__create_udp_socket,
        .bind_socket   = pl__bind_udp_socket,  
        .get_data      = pl__get_udp_data,
        .send_data     = pl__send_udp_data
    };

    static plOsServicesApiI tApi6 = {
        .sleep     = pl__sleep
    };

    gptApiRegistry->add(PL_API_LIBRARY, &tApi3);
    gptApiRegistry->add(PL_API_FILE, &tApi4);
    gptApiRegistry->add(PL_API_UDP, &tApi5);
    gptApiRegistry->add(PL_API_OS_SERVICES, &tApi6);

    gptDataRegistry      = gptApiRegistry->first(PL_API_DATA_REGISTRY);
    gptExtensionRegistry = gptApiRegistry->first(PL_API_EXTENSION_REGISTRY);
    
    // setup & retrieve io context 
    plIOContext* ptIOCtx = pl_get_io_context();
    gptDataRegistry->set_data(PL_CONTEXT_IO_NAME, ptIOCtx);
    gptDataRegistry->set_data("memory", &gtMemoryContext);

    // register window class
    const WNDCLASSEXW tWc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = pl__windows_procedure,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = GetModuleHandle(NULL),
        .hIcon = NULL,
        .hCursor = NULL,
        .hbrBackground = NULL,
        .lpszMenuName = NULL,
        .lpszClassName = L"Pilot Light (win32)",
        .hIconSm = NULL
    };
    RegisterClassExW(&tWc);

    // calculate window size based on desired client region size
    RECT tWr = 
    {
        .left = 0,
        .right = 500 + tWr.left,
        .top = 0,
        .bottom = 500 + tWr.top
    };
    AdjustWindowRect(&tWr, WS_OVERLAPPEDWINDOW, FALSE);

    wchar_t awWideTitle[1024];
    pl__convert_to_wide_string("Pilot Light (win32/vulkan)", awWideTitle);

    // create window & get handle
    gtHandle = CreateWindowExW(
        0,
        tWc.lpszClassName,
        awWideTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME,
        0, 0, tWr.right - tWr.left, tWr.bottom - tWr.top,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL // user data
    );
    QueryPerformanceFrequency((LARGE_INTEGER*)&ilTicksPerSecond);
    QueryPerformanceCounter((LARGE_INTEGER*)&ilTime);
    // tMouseHandle = gtHandle;
    ptIOCtx->pBackendPlatformData = &gtHandle;
    
    // setup console
    DWORD tCurrentMode = 0;
    DWORD tOriginalMode = 0;
    HANDLE tStdOutHandle = NULL;

    if(gbEnableVirtualTerminalProcessing)
    {
        tStdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if(tStdOutHandle == INVALID_HANDLE_VALUE)
            exit(GetLastError());
        if(!GetConsoleMode(tStdOutHandle, &tCurrentMode))
            exit(GetLastError());
        tOriginalMode = tCurrentMode;
        tCurrentMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING; // enable ANSI escape codes
        if(!SetConsoleMode(tStdOutHandle, tCurrentMode))
            exit(GetLastError());
    }

    // load library
    plLibraryApiI* ptLibraryApi = gptApiRegistry->first(PL_API_LIBRARY);
    if(ptLibraryApi->load(&gtAppLibrary, "./app.dll", "./app_", "./lock.tmp"))
    {
        pl_app_load     = (void* (__cdecl  *)(plApiRegistryApiI*, void*)) ptLibraryApi->load_function(&gtAppLibrary, "pl_app_load");
        pl_app_shutdown = (void  (__cdecl  *)(void*)) ptLibraryApi->load_function(&gtAppLibrary, "pl_app_shutdown");
        pl_app_resize   = (void  (__cdecl  *)(void*)) ptLibraryApi->load_function(&gtAppLibrary, "pl_app_resize");
        pl_app_update   = (void  (__cdecl  *)(void*)) ptLibraryApi->load_function(&gtAppLibrary, "pl_app_update");
        gpUserData = pl_app_load(gptApiRegistry, NULL);
    }

    // show window
    ShowWindow(gtHandle, SW_SHOWDEFAULT);

    // main loop
    while (gbRunning)
    {

        // while queue has messages, remove and dispatch them (but do not block on empty queue)
        MSG tMsg = {0};
        while (PeekMessage(&tMsg, NULL, 0, 0, PM_REMOVE))
        {
            // check for quit because peekmessage does not signal this via return val
            if (tMsg.message == WM_QUIT)
            {
                gbRunning = false;
                break;
            }
            // TranslateMessage will post auxilliary WM_CHAR messages from key msgs
            TranslateMessage(&tMsg);
            DispatchMessage(&tMsg);
        }

        pl_update_mouse_cursor();

        // reload library
        if(ptLibraryApi->has_changed(&gtAppLibrary))
        {
            ptLibraryApi->reload(&gtAppLibrary);
            pl_app_load     = (void* (__cdecl  *)(plApiRegistryApiI*, void*)) ptLibraryApi->load_function(&gtAppLibrary, "pl_app_load");
            pl_app_shutdown = (void  (__cdecl  *)(void*)) ptLibraryApi->load_function(&gtAppLibrary, "pl_app_shutdown");
            pl_app_resize   = (void  (__cdecl  *)(void*)) ptLibraryApi->load_function(&gtAppLibrary, "pl_app_resize");
            pl_app_update   = (void  (__cdecl  *)(void*)) ptLibraryApi->load_function(&gtAppLibrary, "pl_app_update");
            gpUserData = pl_app_load(gptApiRegistry, gpUserData);
        }

        // render a frame
        if(gbRunning)
            pl__render_frame();
    }

    // app cleanup
    pl_app_shutdown(gpUserData);

    // cleanup win32 stuff
    UnregisterClassW(tWc.lpszClassName, GetModuleHandle(NULL));
    DestroyWindow(gtHandle);
    gtHandle = NULL;

    // cleanup io context
    gptExtensionRegistry->unload_all(gptApiRegistry);
    pl_unload_core_apis();

    // return console to original mode
    if(gbEnableVirtualTerminalProcessing)
    {
        if(!SetConsoleMode(tStdOutHandle, tOriginalMode))
            exit(GetLastError());
    }

    // cleanup winsock
    WSACleanup();

    uint32_t uMemoryLeakCount = 0;
    for(uint32_t i = 0; i < pl_sb_size(gtMemoryContext.sbtAllocations); i++)
    {
        if(gtMemoryContext.sbtAllocations[i].pAddress != NULL)
        {
            printf("Unfreed memory from line %i in file '%s'.\n", gtMemoryContext.sbtAllocations[i].iLine, gtMemoryContext.sbtAllocations[i].pcFile);
            uMemoryLeakCount++;
        }
    }
        
    assert(uMemoryLeakCount == gtMemoryContext.szActiveAllocations);
    if(uMemoryLeakCount > 0)
        printf("%u unfreed allocations.\n", uMemoryLeakCount);
}

//-----------------------------------------------------------------------------
// [SECTION] windows procedure
//-----------------------------------------------------------------------------

LRESULT CALLBACK 
pl__windows_procedure(HWND tHwnd, UINT tMsg, WPARAM tWParam, LPARAM tLParam)
{
    plIOContext* ptIOCtx = pl_get_io_context();
    static UINT_PTR puIDEvent = 0;
    switch (tMsg)
    {

        case WM_SYSCOMMAND:
        {
            if(tWParam == SC_MINIMIZE)     ptIOCtx->bViewportMinimized = true;
            else if(tWParam == SC_RESTORE) ptIOCtx->bViewportMinimized = false;
            break;
        }

        case WM_SIZE:
        case WM_SIZING:
        {
            if (tWParam != SIZE_MINIMIZED)
            {
                // client window size
                RECT tCRect;
                int iCWidth = 0;
                int iCHeight = 0;
                if (GetClientRect(tHwnd, &tCRect))
                {
                    iCWidth = tCRect.right - tCRect.left;
                    iCHeight = tCRect.bottom - tCRect.top;
                }

                if(iCWidth > 0 && iCHeight > 0)
                    ptIOCtx->bViewportMinimized = false;
                else
                    ptIOCtx->bViewportMinimized = true;

                if(ptIOCtx->afMainViewportSize[0] != (float)iCWidth || ptIOCtx->afMainViewportSize[1] != (float)iCHeight)
                    ptIOCtx->bViewportSizeChanged = true;

                ptIOCtx->afMainViewportSize[0] = (float)iCWidth;
                ptIOCtx->afMainViewportSize[1] = (float)iCHeight;    

                if(ptIOCtx->bViewportSizeChanged && !ptIOCtx->bViewportMinimized && !gbFirstRun)
                {
                    pl_app_resize(gpUserData);
                    gbFirstRun = false;
                }
                gbFirstRun = false;

                // send paint message
                InvalidateRect(tHwnd, NULL, TRUE);
            }
            break;
        }

        case WM_CHAR:
            if (IsWindowUnicode(tHwnd))
            {
                // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
                if (tWParam > 0 && tWParam < 0x10000)
                    pl_add_text_event_utf16((uint16_t)tWParam);
            }
            else
            {
                wchar_t wch = 0;
                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)&tWParam, 1, &wch, 1);
                pl_add_text_event(wch);
            }
            break;

        case WM_SETCURSOR:
            // required to restore cursor when transitioning from e.g resize borders to client area.
            if (LOWORD(tLParam) == HTCLIENT)
            {
                ptIOCtx->tNextCursor = PL_MOUSE_CURSOR_ARROW;
                ptIOCtx->tCurrentCursor = PL_MOUSE_CURSOR_NONE;
                pl_update_mouse_cursor();
            }
            break;

        case WM_MOVE:
        case WM_MOVING:
        {
            pl__render_frame();
            break;
        }

        case WM_PAINT:
        {
            pl__render_frame();

            // must be called for the OS to do its thing
            PAINTSTRUCT tPaint;
            HDC tDeviceContext = BeginPaint(tHwnd, &tPaint);  
            EndPaint(tHwnd, &tPaint); 
            break;
        }

        case WM_MOUSEMOVE:
        {
            tMouseHandle = tHwnd;
            if(!bMouseTracked)
            {
                TRACKMOUSEEVENT tTme = { sizeof(tTme), TME_LEAVE, tHwnd, 0 };
                TrackMouseEvent(&tTme);
                bMouseTracked = true;        
            }
            POINT tMousePos = { (LONG)GET_X_LPARAM(tLParam), (LONG)GET_Y_LPARAM(tLParam) };
            pl_add_mouse_pos_event((float)tMousePos.x, (float)tMousePos.y);
            break;
        }
        case WM_MOUSELEAVE:
        {
            if(tHwnd == tMouseHandle)
            {
                tMouseHandle = NULL;
                pl_add_mouse_pos_event(-FLT_MAX, -FLT_MAX);
            }
            bMouseTracked = false;
            break;
        }

        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        {
            int iButton = 0;
            if (tMsg == WM_LBUTTONDOWN || tMsg == WM_LBUTTONDBLCLK) { iButton = 0; }
            if (tMsg == WM_RBUTTONDOWN || tMsg == WM_RBUTTONDBLCLK) { iButton = 1; }
            if (tMsg == WM_MBUTTONDOWN || tMsg == WM_MBUTTONDBLCLK) { iButton = 2; }
            if (tMsg == WM_XBUTTONDOWN || tMsg == WM_XBUTTONDBLCLK) { iButton = (GET_XBUTTON_WPARAM(tWParam) == XBUTTON1) ? 3 : 4; }
            if(ptIOCtx->_iMouseButtonsDown == 0 && GetCapture() == NULL)
                SetCapture(tHwnd);
            ptIOCtx->_iMouseButtonsDown |= 1 << iButton;
            pl_add_mouse_button_event(iButton, true);
            break;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            int iButton = 0;
            if (tMsg == WM_LBUTTONUP) { iButton = 0; }
            if (tMsg == WM_RBUTTONUP) { iButton = 1; }
            if (tMsg == WM_MBUTTONUP) { iButton = 2; }
            if (tMsg == WM_XBUTTONUP) { iButton = (GET_XBUTTON_WPARAM(tWParam) == XBUTTON1) ? 3 : 4; }
            ptIOCtx->_iMouseButtonsDown &= ~(1 << iButton);
            if(ptIOCtx->_iMouseButtonsDown == 0 && GetCapture() == tHwnd)
                ReleaseCapture();
            pl_add_mouse_button_event(iButton, false);
            break;
        }

        case WM_MOUSEWHEEL:
        {
            pl_add_mouse_wheel_event(0.0f, (float)GET_WHEEL_DELTA_WPARAM(tWParam) / (float)WHEEL_DELTA);
            break;
        }

        case WM_MOUSEHWHEEL:
        {
            pl_add_mouse_wheel_event((float)GET_WHEEL_DELTA_WPARAM(tWParam) / (float)WHEEL_DELTA, 0.0f);
            break;
        }

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            const bool bKeyDown = (tMsg == WM_KEYDOWN || tMsg == WM_SYSKEYDOWN);
            if (tWParam < 256)
            {

                // Submit modifiers
                pl_add_key_event(PL_KEY_MOD_CTRL,  pl__is_vk_down(VK_CONTROL));
                pl_add_key_event(PL_KEY_MOD_SHIFT, pl__is_vk_down(VK_SHIFT));
                pl_add_key_event(PL_KEY_MOD_ALT,   pl__is_vk_down(VK_MENU));
                pl_add_key_event(PL_KEY_MOD_SUPER, pl__is_vk_down(VK_APPS));

                // obtain virtual key code
                int iVk = (int)tWParam;
                if ((tWParam == VK_RETURN) && (HIWORD(tLParam) & KF_EXTENDED))
                {
                    iVk = PL_VK_KEYPAD_ENTER;
                }

                // submit key event
                const plKey tKey = pl__virtual_key_to_pl_key(iVk);

                if (tKey != PL_KEY_NONE)
                {
                    pl_add_key_event(tKey, bKeyDown);
                }

                // Submit individual left/right modifier events
                if (iVk == VK_SHIFT)
                {
                    if (pl__is_vk_down(VK_LSHIFT) == bKeyDown) pl_add_key_event(PL_KEY_LEFT_SHIFT, bKeyDown);
                    if (pl__is_vk_down(VK_RSHIFT) == bKeyDown) pl_add_key_event(PL_KEY_RIGHT_SHIFT, bKeyDown);
                }
                else if (iVk == VK_CONTROL)
                {
                    if (pl__is_vk_down(VK_LCONTROL) == bKeyDown) pl_add_key_event(PL_KEY_LEFT_CTRL, bKeyDown);
                    if (pl__is_vk_down(VK_RCONTROL) == bKeyDown) pl_add_key_event(PL_KEY_RIGHT_CTRL, bKeyDown);
                }
                else if (iVk == VK_MENU)
                {
                    if (pl__is_vk_down(VK_LMENU) == bKeyDown) pl_add_key_event(PL_KEY_LEFT_ALT, bKeyDown);
                    if (pl__is_vk_down(VK_RMENU) == bKeyDown) pl_add_key_event(PL_KEY_RIGHT_ALT, bKeyDown);
                }
            }
            break;
        }

        case WM_CLOSE:
        {
            PostQuitMessage(0);
            break;
        }

        case WM_ENTERSIZEMOVE:
        {
            // DefWindowProc below will block until mouse is released or moved.
            // Timer events can still be caught so here we add a timer so we
            // can continue rendering when catching the WM_TIMER event.
            // Timer is killed in the WM_EXITSIZEMOVE case below.
            puIDEvent = SetTimer(NULL, puIDEvent, USER_TIMER_MINIMUM , NULL);
            SetTimer(tHwnd, puIDEvent, USER_TIMER_MINIMUM , NULL);
            break;
        }

        case WM_EXITSIZEMOVE:
        {
            KillTimer(tHwnd, puIDEvent);
            break;
        }

        case WM_TIMER:
        {
            if(tWParam == puIDEvent)
                pl__render_frame();
            break;
        }
    }
    return DefWindowProcW(tHwnd, tMsg, tWParam, tLParam);
}

//-----------------------------------------------------------------------------
// [SECTION] implementations
//-----------------------------------------------------------------------------

void
pl__convert_to_wide_string(const char* pcNarrowValue, wchar_t* pwWideValue)
{
    mbstate_t tState;
    memset(&tState, 0, sizeof(tState));
    size_t szLen = 1 + mbsrtowcs(NULL, &pcNarrowValue, 0, &tState);
    mbsrtowcs(pwWideValue, &pcNarrowValue, szLen, &tState);
}

void
pl__render_frame(void)
{
    plIOContext* ptIOCtx = pl_get_io_context();

    // setup time step
    INT64 ilCurrentTime = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&ilCurrentTime);
    ptIOCtx->fDeltaTime = (float)(ilCurrentTime - ilTime) / ilTicksPerSecond;
    ilTime = ilCurrentTime;
    if(!ptIOCtx->bViewportMinimized)
    {
        pl_app_update(gpUserData);
        gptExtensionRegistry->reload(gptApiRegistry);
    }
}

void
pl_update_mouse_cursor(void)
{
    plIOContext* ptIOCtx = pl_get_io_context();

    // updating mouse cursor
    if(ptIOCtx->tCurrentCursor != PL_MOUSE_CURSOR_ARROW && ptIOCtx->tNextCursor == PL_MOUSE_CURSOR_ARROW)
        ptIOCtx->bCursorChanged = true;

    if(ptIOCtx->bCursorChanged && ptIOCtx->tNextCursor != ptIOCtx->tCurrentCursor)
    {
        ptIOCtx->tCurrentCursor = ptIOCtx->tNextCursor;
        LPTSTR tWin32Cursor = IDC_ARROW;
        switch (ptIOCtx->tNextCursor)
        {
            case PL_MOUSE_CURSOR_ARROW:       tWin32Cursor = IDC_ARROW; break;
            case PL_MOUSE_CURSOR_TEXT_INPUT:  tWin32Cursor = IDC_IBEAM; break;
            case PL_MOUSE_CURSOR_RESIZE_ALL:  tWin32Cursor = IDC_SIZEALL; break;
            case PL_MOUSE_CURSOR_RESIZE_EW:   tWin32Cursor = IDC_SIZEWE; break;
            case PL_MOUSE_CURSOR_RESIZE_NS:   tWin32Cursor = IDC_SIZENS; break;
            case PL_MOUSE_CURSOR_RESIZE_NESW: tWin32Cursor = IDC_SIZENESW; break;
            case PL_MOUSE_CURSOR_RESIZE_NWSE: tWin32Cursor = IDC_SIZENWSE; break;
            case PL_MOUSE_CURSOR_HAND:        tWin32Cursor = IDC_HAND; break;
            case PL_MOUSE_CURSOR_NOT_ALLOWED: tWin32Cursor = IDC_NO; break;
        }
        SetCursor(LoadCursor(NULL, tWin32Cursor));    
    }
    ptIOCtx->tNextCursor = PL_MOUSE_CURSOR_ARROW;
    ptIOCtx->bCursorChanged = false;
}

plKey
pl__virtual_key_to_pl_key(WPARAM tWParam)
{
    switch (tWParam)
    {
        case VK_TAB:             return PL_KEY_TAB;
        case VK_LEFT:            return PL_KEY_LEFT_ARROW;
        case VK_RIGHT:           return PL_KEY_RIGHT_ARROW;
        case VK_UP:              return PL_KEY_UP_ARROW;
        case VK_DOWN:            return PL_KEY_DOWN_ARROW;
        case VK_PRIOR:           return PL_KEY_PAGE_UP;
        case VK_NEXT:            return PL_KEY_PAGE_DOWN;
        case VK_HOME:            return PL_KEY_HOME;
        case VK_END:             return PL_KEY_END;
        case VK_INSERT:          return PL_KEY_INSERT;
        case VK_DELETE:          return PL_KEY_DELETE;
        case VK_BACK:            return PL_KEY_BACKSPACE;
        case VK_SPACE:           return PL_KEY_SPACE;
        case VK_RETURN:          return PL_KEY_ENTER;
        case VK_ESCAPE:          return PL_KEY_ESCAPE;
        case VK_OEM_7:           return PL_KEY_APOSTROPHE;
        case VK_OEM_COMMA:       return PL_KEY_COMMA;
        case VK_OEM_MINUS:       return PL_KEY_MINUS;
        case VK_OEM_PERIOD:      return PL_KEY_PERIOD;
        case VK_OEM_2:           return PL_KEY_SLASH;
        case VK_OEM_1:           return PL_KEY_SEMICOLON;
        case VK_OEM_PLUS:        return PL_KEY_EQUAL;
        case VK_OEM_4:           return PL_KEY_LEFT_BRACKET;
        case VK_OEM_5:           return PL_KEY_BACKSLASH;
        case VK_OEM_6:           return PL_KEY_RIGHT_BRACKET;
        case VK_OEM_3:           return PL_KEY_GRAVE_ACCENT;
        case VK_CAPITAL:         return PL_KEY_CAPS_LOCK;
        case VK_SCROLL:          return PL_KEY_SCROLL_LOCK;
        case VK_NUMLOCK:         return PL_KEY_NUM_LOCK;
        case VK_SNAPSHOT:        return PL_KEY_PRINT_SCREEN;
        case VK_PAUSE:           return PL_KEY_PAUSE;
        case VK_NUMPAD0:         return PL_KEY_KEYPAD_0;
        case VK_NUMPAD1:         return PL_KEY_KEYPAD_1;
        case VK_NUMPAD2:         return PL_KEY_KEYPAD_2;
        case VK_NUMPAD3:         return PL_KEY_KEYPAD_3;
        case VK_NUMPAD4:         return PL_KEY_KEYPAD_4;
        case VK_NUMPAD5:         return PL_KEY_KEYPAD_5;
        case VK_NUMPAD6:         return PL_KEY_KEYPAD_6;
        case VK_NUMPAD7:         return PL_KEY_KEYPAD_7;
        case VK_NUMPAD8:         return PL_KEY_KEYPAD_8;
        case VK_NUMPAD9:         return PL_KEY_KEYPAD_9;
        case VK_DECIMAL:         return PL_KEY_KEYPAD_DECIMAL;
        case VK_DIVIDE:          return PL_KEY_KEYPAD_DIVIDE;
        case VK_MULTIPLY:        return PL_KEY_KEYPAD_MULTIPLY;
        case VK_SUBTRACT:        return PL_KEY_KEYPAD_SUBTRACT;
        case VK_ADD:             return PL_KEY_KEYPAD_ADD;
        case PL_VK_KEYPAD_ENTER: return PL_KEY_KEYPAD_ENTER;
        case VK_LSHIFT:          return PL_KEY_LEFT_SHIFT;
        case VK_LCONTROL:        return PL_KEY_LEFT_CTRL;
        case VK_LMENU:           return PL_KEY_LEFT_ALT;
        case VK_LWIN:            return PL_KEY_LEFT_SUPER;
        case VK_RSHIFT:          return PL_KEY_RIGHT_SHIFT;
        case VK_RCONTROL:        return PL_KEY_RIGHT_CTRL;
        case VK_RMENU:           return PL_KEY_RIGHT_ALT;
        case VK_RWIN:            return PL_KEY_RIGHT_SUPER;
        case VK_APPS:            return PL_KEY_MENU;
        case '0':                return PL_KEY_0;
        case '1':                return PL_KEY_1;
        case '2':                return PL_KEY_2;
        case '3':                return PL_KEY_3;
        case '4':                return PL_KEY_4;
        case '5':                return PL_KEY_5;
        case '6':                return PL_KEY_6;
        case '7':                return PL_KEY_7;
        case '8':                return PL_KEY_8;
        case '9':                return PL_KEY_9;
        case 'A':                return PL_KEY_A;
        case 'B':                return PL_KEY_B;
        case 'C':                return PL_KEY_C;
        case 'D':                return PL_KEY_D;
        case 'E':                return PL_KEY_E;
        case 'F':                return PL_KEY_F;
        case 'G':                return PL_KEY_G;
        case 'H':                return PL_KEY_H;
        case 'I':                return PL_KEY_I;
        case 'J':                return PL_KEY_J;
        case 'K':                return PL_KEY_K;
        case 'L':                return PL_KEY_L;
        case 'M':                return PL_KEY_M;
        case 'N':                return PL_KEY_N;
        case 'O':                return PL_KEY_O;
        case 'P':                return PL_KEY_P;
        case 'Q':                return PL_KEY_Q;
        case 'R':                return PL_KEY_R;
        case 'S':                return PL_KEY_S;
        case 'T':                return PL_KEY_T;
        case 'U':                return PL_KEY_U;
        case 'V':                return PL_KEY_V;
        case 'W':                return PL_KEY_W;
        case 'X':                return PL_KEY_X;
        case 'Y':                return PL_KEY_Y;
        case 'Z':                return PL_KEY_Z;
        case VK_F1:              return PL_KEY_F1;
        case VK_F2:              return PL_KEY_F2;
        case VK_F3:              return PL_KEY_F3;
        case VK_F4:              return PL_KEY_F4;
        case VK_F5:              return PL_KEY_F5;
        case VK_F6:              return PL_KEY_F6;
        case VK_F7:              return PL_KEY_F7;
        case VK_F8:              return PL_KEY_F8;
        case VK_F9:              return PL_KEY_F9;
        case VK_F10:             return PL_KEY_F10;
        case VK_F11:             return PL_KEY_F11;
        case VK_F12:             return PL_KEY_F12;
        default:                 return PL_KEY_NONE;
    }   
}

void
pl__read_file(const char* pcFile, unsigned* puSizeIn, char* pcBuffer, const char* pcMode)
{
    PL_ASSERT(puSizeIn);

    FILE* ptDataFile = fopen(pcFile, pcMode);
    unsigned uSize = 0u;

    if (ptDataFile == NULL)
    {
        PL_ASSERT(false && "File not found.");
        *puSizeIn = 0u;
        return;
    }

    // obtain file size
    fseek(ptDataFile, 0, SEEK_END);
    uSize = ftell(ptDataFile);
    fseek(ptDataFile, 0, SEEK_SET);

    if(pcBuffer == NULL)
    {
        *puSizeIn = uSize;
        fclose(ptDataFile);
        return;
    }

    // copy the file into the buffer:
    size_t szResult = fread(pcBuffer, sizeof(char), uSize, ptDataFile);
    if (szResult != uSize)
    {
        if (feof(ptDataFile))
            printf("Error reading test.bin: unexpected end of file\n");
        else if (ferror(ptDataFile)) {
            perror("Error reading test.bin");
        }
        PL_ASSERT(false && "File not read.");
    }

    fclose(ptDataFile);
}

void
pl__copy_file(const char* pcSource, const char* pcDestination, unsigned* puSize, char* pcBuffer)
{
    CopyFile(pcSource, pcDestination, FALSE);
}

void
pl__create_udp_socket(plSocket* ptSocketOut, bool bNonBlocking)
{

    UINT_PTR tWin32Socket = 0;

    // create socket
    if((tWin32Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d\n", WSAGetLastError());
        PL_ASSERT(false && "Could not create socket");
    }

    // enable non-blocking
    if(bNonBlocking)
    {
        u_long uMode = 1;
        ioctlsocket(tWin32Socket, FIONBIO, &uMode);
    }

    ptSocketOut->_pPlatformData = (void*)tWin32Socket;
}

void
pl__bind_udp_socket(plSocket* ptSocket, int iPort)
{
    ptSocket->iPort = iPort;
    PL_ASSERT(ptSocket->_pPlatformData && "Socket not created yet");
    UINT_PTR tWin32Socket = (UINT_PTR)ptSocket->_pPlatformData;
    
    // prepare sockaddr_in struct
    struct sockaddr_in tServer = {
        .sin_family      = AF_INET,
        .sin_port        = htons((u_short)iPort),
        .sin_addr.s_addr = INADDR_ANY
    };

    // bind socket
    if(bind(tWin32Socket, (struct sockaddr* )&tServer, sizeof(tServer)) == SOCKET_ERROR)
    {
        printf("Bind socket failed with error code : %d\n", WSAGetLastError());
        PL_ASSERT(false && "Socket error");
    }
}

bool
pl__send_udp_data(plSocket* ptFromSocket, const char* pcDestIP, int iDestPort, void* pData, size_t szSize)
{
    PL_ASSERT(ptFromSocket->_pPlatformData && "Socket not created yet");
    UINT_PTR tWin32Socket = (UINT_PTR)ptFromSocket->_pPlatformData;

    struct sockaddr_in tDestSocket = {
        .sin_family           = AF_INET,
        .sin_port             = htons((u_short)iDestPort),
        .sin_addr.S_un.S_addr = inet_addr(pcDestIP)
    };
    static const size_t szLen = sizeof(tDestSocket);

    // send
    if(sendto(tWin32Socket, (const char*)pData, (int)szSize, 0, (struct sockaddr*)&tDestSocket, (int)szLen) == SOCKET_ERROR)
    {
        printf("sendto() failed with error code : %d\n", WSAGetLastError());
        PL_ASSERT(false && "Socket error");
        return false;
    }

    return true;
}

bool
pl__get_udp_data(plSocket* ptSocket, void* pData, size_t szSize)
{
    PL_ASSERT(ptSocket->_pPlatformData && "Socket not created yet");
    UINT_PTR tWin32Socket = (UINT_PTR)ptSocket->_pPlatformData;

    struct sockaddr_in tSiOther = {0};
    static int iSLen = (int)sizeof(tSiOther);
    memset(pData, 0, szSize);
    int iRecvLen = recvfrom(tWin32Socket, (char*)pData, (int)szSize, 0, (struct sockaddr*)&tSiOther, &iSLen);

    if(iRecvLen == SOCKET_ERROR)
    {
        const int iLastError = WSAGetLastError();
        if(iLastError != WSAEWOULDBLOCK)
        {
            printf("recvfrom() failed with error code : %d\n", WSAGetLastError());
            PL_ASSERT(false && "Socket error");
            return false;
        }
    }

    return iRecvLen > 0;
}

bool
pl__has_library_changed(plSharedLibrary* ptLibrary)
{
    FILETIME newWriteTime = pl__get_last_write_time(ptLibrary->acPath);
    plWin32SharedLibrary* win32Library = ptLibrary->_pPlatformData;
    return CompareFileTime(&newWriteTime, &win32Library->tLastWriteTime) != 0;
}

bool
pl__load_library(plSharedLibrary* ptLibrary, const char* pcName, const char* pcTransitionalName, const char* pcLockFile)
{
    if(ptLibrary->acPath[0] == 0)             strncpy(ptLibrary->acPath, pcName, PL_MAX_PATH_LENGTH);
    if(ptLibrary->acTransitionalName[0] == 0) strncpy(ptLibrary->acTransitionalName, pcTransitionalName, PL_MAX_PATH_LENGTH);
    if(ptLibrary->acLockFile[0] == 0)         strncpy(ptLibrary->acLockFile, pcLockFile, PL_MAX_PATH_LENGTH);
    ptLibrary->bValid = false;

    if(ptLibrary->_pPlatformData == NULL)
        ptLibrary->_pPlatformData = malloc(sizeof(plWin32SharedLibrary));
    plWin32SharedLibrary* ptWin32Library = ptLibrary->_pPlatformData;

    WIN32_FILE_ATTRIBUTE_DATA tIgnored;
    if(!GetFileAttributesExA(ptLibrary->acLockFile, GetFileExInfoStandard, &tIgnored))  // lock file gone
    {
        char acTemporaryName[2024] = {0};
        ptWin32Library->tLastWriteTime = pl__get_last_write_time(ptLibrary->acPath);
        
        stbsp_sprintf(acTemporaryName, "%s%u%s", ptLibrary->acTransitionalName, ptLibrary->uTempIndex, ".dll");
        if(++ptLibrary->uTempIndex >= 1024)
        {
            ptLibrary->uTempIndex = 0;
        }
        pl__copy_file(ptLibrary->acPath, acTemporaryName, NULL, NULL);

        ptWin32Library->tHandle = NULL;
        ptWin32Library->tHandle = LoadLibraryA(acTemporaryName);
        if(ptWin32Library->tHandle)
            ptLibrary->bValid = true;
    }

    return ptLibrary->bValid;
}

void
pl__reload_library(plSharedLibrary* ptLibrary)
{
    ptLibrary->bValid = false;
    for(uint32_t i = 0; i < 100; i++)
    {
        if(pl__load_library(ptLibrary, ptLibrary->acPath, ptLibrary->acTransitionalName, ptLibrary->acLockFile))
            break;
        pl__sleep(100);
    }
}

void*
pl__load_library_function(plSharedLibrary* ptLibrary, const char* name)
{
    PL_ASSERT(ptLibrary->bValid && "Library not valid");
    void* pLoadedFunction = NULL;
    if(ptLibrary->bValid)
    {
        plWin32SharedLibrary* ptWin32Library = ptLibrary->_pPlatformData;
        pLoadedFunction = (void*)GetProcAddress(ptWin32Library->tHandle, name);
        if(pLoadedFunction == NULL)
            printf("Failed to load function with error code: %d\n", WSAGetLastError());
    }
    return pLoadedFunction;
}

int
pl__sleep(uint32_t uMillisec)
{
    Sleep((long)uMillisec);
    return 0;
}

//-----------------------------------------------------------------------------
// [SECTION] unity build
//-----------------------------------------------------------------------------

#include "pilotlight_exe.c"

/*=============================================================================
    win_main.c
 =============================================================================*/

#include <windows.h>
#include <gl/gl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "win_main.h"

#include "../text.c"
#include "../game.c"

/*-----------------------------------------------------------------------------
    WinMain
    Application entry point for Windows.
 ----------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR szCmdLine, int iCmdShow)
{
    /* Register the window class. */
    WNDCLASS wndclass;
    static TCHAR szAppName[] = TEXT("Breakout");

    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szAppName;

    if (!RegisterClass(&wndclass)) {
        MessageBox(NULL, TEXT("Failed to register the window class."),
            szAppName, MB_ICONERROR);
        return 0;
    }

    /* Increase the timer resolution for more accurate timers. */
    TIMECAPS tc;
    UINT timerResolution;

    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
        MessageBox(NULL, TEXT("Could not obtain timer device resolution."),
            szAppName, MB_ICONERROR);
        return 0;
    }

    timerResolution = min(max(tc.wPeriodMin, TARGET_TIMER_RESOLUTION_MS), tc.wPeriodMax);
    timeBeginPeriod(timerResolution);

    /* Initialize the performance timer */
    LARGE_INTEGER ticksStart, ticksCurrent;
    int64_t ticksElapsed;
    LARGE_INTEGER ticksPerSecond;
    QueryPerformanceFrequency(&ticksPerSecond);
    float msElapsed = 0.0f;
    float msPerUpdate = (float)MS_PER_SECOND / (float)UPDATES_PER_SECOND;
    float msAccumulator = 0.0f;

    /* Allocate game memory */
    struct game_memory *gameMemory;
    gameMemory = VirtualAlloc(NULL, sizeof(struct game_memory), MEM_COMMIT, PAGE_READWRITE);
    struct game_state *gameState;
    gameState = VirtualAlloc(NULL, sizeof(struct game_state), MEM_COMMIT, PAGE_READWRITE);
    GameInit(gameState);
    enum graphicsAPIType graphicsAPI = opengl;
    gameMemory->gameState = gameState;
    gameMemory->graphicsAPI = &graphicsAPI;

    /* Create the window. */
    HWND hwnd;
    MSG msg;
    RECT rect = {0, 0, QVGA_WIDTH, QVGA_HEIGHT};

    AdjustWindowRect(&rect, WS_CAPTION, FALSE);

    hwnd = CreateWindow(
        szAppName,
        TEXT("Breakout"),
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        hInstance,
        gameMemory);
    
    if (hwnd == NULL) {
        MessageBox(NULL, TEXT("Failed to create the window."),
            szAppName, MB_ICONERROR);
        return 0;
    }

    /* Get a handle to the window device context. */
    HDC hdc = GetDC(hwnd);

    /* Initialize OpenGL. */
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);
    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);
    wgl_swap_interval_ext *wglSwapInterval = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
    if(wglSwapInterval)
        wglSwapInterval(1);

    /* Create a Windows frame buffer. */
    HBITMAP frameBmp;
    int bitmapMemorySize = QVGA_WIDTH * QVGA_HEIGHT * BYTES_PER_PIXEL;
    void *bitmapMemory = VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    BITMAPINFO bitmapInfo;
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = QVGA_WIDTH;
    bitmapInfo.bmiHeader.biHeight = QVGA_HEIGHT;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = 0;
    bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biClrUsed = 0;
    bitmapInfo.bmiHeader.biClrImportant = 0;

    frameBmp = CreateDIBSection(
        hdc,
        &bitmapInfo,
        DIB_RGB_COLORS,
        &bitmapMemory,
        NULL,
        0);

    /* Release the handle to the window device context. */
    ReleaseDC(hwnd, hdc);
  
    /* Display the window. */
    ShowWindow(hwnd, iCmdShow);

    /* Start the timer. */
    QueryPerformanceCounter(&ticksStart);

    /* Game loop. */
    for (;;) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (msg.message == WM_QUIT) {
            break;
        }

        msAccumulator += msElapsed;

        while (msAccumulator >= msPerUpdate) {
            GameUpdate(msPerUpdate, gameState);
            msAccumulator -= msPerUpdate;
        }

        //render check for missed?
        struct bitmap_buffer gameBitmapBuffer;
        gameBitmapBuffer.memory = bitmapMemory;
        gameBitmapBuffer.memorySize = bitmapMemorySize;
        gameBitmapBuffer.width = QVGA_WIDTH;
        gameBitmapBuffer.height = QVGA_HEIGHT;
        gameBitmapBuffer.pitch = QVGA_WIDTH * BYTES_PER_PIXEL;
        GameRender(gameState, &gameBitmapBuffer);

        msElapsed = ComputeMsElapsed(&ticksStart, &ticksCurrent, &ticksElapsed, &ticksPerSecond);
        char buffer[256];
        sprintf(buffer, "%.2fms/f\n", msElapsed);
        OutputDebugString(buffer);

        if (graphicsAPI == opengl)
            BlitFrameOpenGL(hwnd, bitmapMemory);
        else if (graphicsAPI == software) {
            while (msElapsed < msPerUpdate) {
                DWORD msSleep = (DWORD)(msPerUpdate - msElapsed);
                Sleep(msSleep);
                msElapsed = ComputeMsElapsed(&ticksStart, &ticksCurrent, &ticksElapsed, &ticksPerSecond);
            }
            BlitFrameGDI(hwnd, frameBmp);
        }

        msElapsed = ComputeMsElapsed(&ticksStart, &ticksCurrent, &ticksElapsed, &ticksPerSecond);
        ticksStart = ticksCurrent;

        sprintf(buffer, "%.2fms/f, %.2ff/s\n", msElapsed, (float)ticksPerSecond.QuadPart / ticksElapsed);
        OutputDebugString(buffer);
    }

    /* Reset the system timer to the default resolution. */
    timeEndPeriod(timerResolution);

    /* Clean up resources. */
    DeleteObject(frameBmp);
    VirtualFree(gameState, 0, MEM_RELEASE);
    VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    VirtualFree(gameMemory, 0, MEM_RELEASE);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);

    return msg.wParam;
}

/*-----------------------------------------------------------------------------
    WndProc
    Processes messages sent to the window by the operating system.
 ----------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct game_memory *gameMemory;

    switch(message) {
    case WM_CREATE:
        CREATESTRUCT *createStruct;
        createStruct = (CREATESTRUCT *)lParam;
        gameMemory = createStruct->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)gameMemory);
        break;
    case WM_KEYDOWN:
    case WM_KEYUP:
        gameMemory = (struct game_memory *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        struct game_state *gameState = gameMemory->gameState;
        enum graphicsAPIType *graphicsAPI = gameMemory->graphicsAPI;
        uint32_t virtualKeyCode = wParam;
        uint32_t keyState = lParam;
        bool keyWasDown = ((keyState & KEY_PREVIOUS_STATE) != 0);
        bool keyIsDown = ((keyState & KEY_TRANSITION_STATE) == 0);
        if (!(keyIsDown && keyWasDown)) {
            switch(virtualKeyCode){
            case VK_LEFT:
                GameKeyboardUpdate(gameState, GAME_KEY_LEFT, keyIsDown);
                break;
            case VK_RIGHT:
                GameKeyboardUpdate(gameState, GAME_KEY_RIGHT, keyIsDown);
                break;
            case VK_F2:
                if (!keyIsDown)
                    *graphicsAPI = opengl;
                break;
            case VK_F3:
                if (!keyIsDown)
                    *graphicsAPI = software;
                break;
            case VK_ESCAPE:
                GameKeyboardUpdate(gameState, GAME_KEY_ESCAPE, keyIsDown);
                break;
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

/*-----------------------------------------------------------------------------
    BlitFrameOpenGL
    Transfers a bitmap to the display via OpenGL.
 ----------------------------------------------------------------------------*/
void BlitFrameOpenGL(HWND hwnd, void *bitmapMemory)
{
    HDC windowHDC = GetDC(hwnd);

    glViewport(0, 0, QVGA_WIDTH, QVGA_HEIGHT);

    GLuint textureHandle;
    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        QVGA_WIDTH,
        QVGA_HEIGHT,
        0,
        GL_BGRA_EXT,
        GL_UNSIGNED_BYTE,
        bitmapMemory);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glBegin(GL_TRIANGLES);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, -1.0f);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1.0f, -1.0f);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, -1.0f);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);

    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);

    glEnd();

    SwapBuffers(windowHDC);

    glDeleteTextures(1, &textureHandle);
    ReleaseDC(hwnd, windowHDC);

    return;
}

/*-----------------------------------------------------------------------------
    BlitFrameGDI
    Transfers a bitmap to the display via GDI.
 ----------------------------------------------------------------------------*/
void BlitFrameGDI(HWND hwnd, HBITMAP frameBmp)
{
    HDC windowHDC = GetDC(hwnd);
    HDC backbufferHDC = CreateCompatibleDC(windowHDC);
    HBITMAP oldBmp = SelectObject(backbufferHDC, frameBmp);

    BitBlt(
        windowHDC,
        0,
        0,
        QVGA_WIDTH,
        QVGA_HEIGHT,
        backbufferHDC,
        0,
        0,
        SRCCOPY);

    ReleaseDC(hwnd, windowHDC);
    SelectObject(backbufferHDC, oldBmp);
    DeleteDC(backbufferHDC);

    return;
}

/*-----------------------------------------------------------------------------
    ComputeMsElapsed
    Returns the amount of time elapsed since ticksStart.
 ----------------------------------------------------------------------------*/
float ComputeMsElapsed(LARGE_INTEGER *ticksStart, LARGE_INTEGER *ticksCurrent,
    int64_t *ticksElapsed, LARGE_INTEGER *ticksPerSecond)
{
    QueryPerformanceCounter(ticksCurrent);
    *ticksElapsed = ticksCurrent->QuadPart - ticksStart->QuadPart;

    return (float)(*ticksElapsed * MS_PER_SECOND) / (float)ticksPerSecond->QuadPart;
}
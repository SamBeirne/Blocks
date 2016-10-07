/*=============================================================================
    win_main.h
 =============================================================================*/

#ifndef WIN_MAIN_H
#define WIN_MAIN_H

#define KEY_PREVIOUS_STATE 0x40000000L
#define KEY_TRANSITION_STATE 0x80000000L
#define QVGA_WIDTH 320
#define QVGA_HEIGHT 240
#define BYTES_PER_PIXEL 4
#define TARGET_TIMER_RESOLUTION_MS 1
#define UPDATES_PER_SECOND 60
#define MS_PER_SECOND 1000

typedef BOOL WINAPI wgl_swap_interval_ext (int interval);

enum graphicsAPIType {
    opengl,
    software
};

struct game_memory {
    struct game_state *gameState;
    enum graphicsAPIType *graphicsAPI;
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void BlitFrameOpenGL(HWND, void *);
void BlitFrameGDI(HWND, HBITMAP);
float ComputeMsElapsed(LARGE_INTEGER *, LARGE_INTEGER *, int64_t *, LARGE_INTEGER *);

#endif /* WIN_MAIN_H */
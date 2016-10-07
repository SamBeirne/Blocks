//=============================================================================
//  mac_main.h
//=============================================================================

#ifndef MAC_MAIN_H
#define MAC_MAIN_H

#include "../game.h"

#define QVGA_WIDTH 320.0f
#define QVGA_HEIGHT 240.0f
#define BYTES_PER_PIXEL 4
#define BITS_PER_PIXEL_COMPONENT 8
#define BITMAP_SIZE (int)QVGA_WIDTH * (int)QVGA_HEIGHT * BYTES_PER_PIXEL
#define MS_PER_UPDATE 1000.0f / 60.0f
#define TIMER_INTERVAL 0.01666

@interface AppDelegate : NSObject <NSApplicationDelegate>
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication;
@end

@interface WindowView : NSView {
NSTimer *timer;
uint64_t timeStartAbsolute;
uint64_t timeEndAbsolute;
uint64_t timeElapsedAbsolute;
uint64_t timeElapsedNanoseconds;
float timeElapsedMilliseconds;
float timeAccumulatorMilliseconds;
struct game_state gameState;
struct bitmap_buffer gameBitmapBuffer;
}

- (instancetype)initWithFrame:(NSRect)frameRect;
- (BOOL)acceptsFirstResponder;
- (void)keyDown:(NSEvent *)event;
- (void)gameLoop:(NSTimer *)timer;
- (void)drawRect:(NSRect)rect;
- (void)dealloc;
@end

#endif // MAC_MAIN_H
//=============================================================================
//  mac_main.m
//=============================================================================

#import <Cocoa/Cocoa.h>
#import "mac_main.h"
#include <mach/mach_time.h>
#include "../text.c"
#include "../game.c"

//-----------------------------------------------------------------------------
//  main
//  Application entry point for macOS.
//-----------------------------------------------------------------------------
int main(int argc, const char *argv[])
{
	@autoreleasepool {

	[NSApplication sharedApplication];

	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	AppDelegate *appDelegate = [[AppDelegate alloc] autorelease];
	[NSApp setDelegate:appDelegate];

	id menuBar = [[NSMenu new] autorelease];
	id appMenuItem = [[NSMenuItem new] autorelease];
	[menuBar addItem:appMenuItem];
	[NSApp setMainMenu:menuBar];

	id appMenu = [[NSMenu new] autorelease];
	id appName = [[NSProcessInfo processInfo] processName];
	id quitTitle = [@"Quit " stringByAppendingString:appName];
	id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle
	                                       action:@selector(terminate:)
	                                       keyEquivalent:@"q"] autorelease];
	[appMenu addItem:quitMenuItem];
	[appMenuItem setSubmenu:appMenu];

	NSRect screenRect = [[NSScreen mainScreen] frame];
	NSRect windowRect = NSMakeRect((screenRect.size.width - QVGA_WIDTH) * 0.5f,
		                           (screenRect.size.height - QVGA_HEIGHT) * 0.5f,
		                           QVGA_WIDTH,
		                           QVGA_HEIGHT);
	NSUInteger windowStyle = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
	NSWindow *window = [[NSWindow alloc] initWithContentRect:windowRect
		                                 styleMask:windowStyle
		                                 backing:NSBackingStoreBuffered
		                                 defer:NO];
	[window setTitle:appName];
    WindowView *windowView = [[[WindowView alloc] initWithFrame:windowRect] autorelease];
    [window setContentView:windowView];
	[window autorelease];

	[window makeKeyAndOrderFront:nil];
	[NSApp run];

	[NSApp release];
    } // @autoreleasepool

	return 0;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//  AppDelegate
//  Delegate for the main application.
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@implementation AppDelegate

//-----------------------------------------------------------------------------
//  applicationShouldTerminateAfterLastWindowClosed
//  It doesn't make sense for the application to keep running after the
//  window is closed, so terminate the application.
//-----------------------------------------------------------------------------
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
	return YES;
}

@end // @implementation AppDelegate

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//  WindowView
//  Delegate for the main view where the game runs.
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@implementation WindowView

//-----------------------------------------------------------------------------
//  dealloc
//  Free memory.
//-----------------------------------------------------------------------------
- (void)dealloc
{
    free(gameBitmapBuffer.memory);
    [super dealloc];
}

//-----------------------------------------------------------------------------
//  initWithFrame
//  Initialize the NSView object.
//-----------------------------------------------------------------------------
- (instancetype)initWithFrame:(NSRect)frameRect
{
    if(self = [super initWithFrame:frameRect]) {
        timer = [NSTimer scheduledTimerWithTimeInterval:TIMER_INTERVAL
                         target:self
                         selector:@selector(gameLoop:)
                         userInfo:nil
                         repeats:YES];
        timeStartAbsolute = mach_absolute_time();
        timeAccumulatorMilliseconds = 0.0f;
        GameInit(&gameState);
        gameBitmapBuffer.memory = malloc(BITMAP_SIZE);
        gameBitmapBuffer.memorySize = BITMAP_SIZE;
        gameBitmapBuffer.width = (int)QVGA_WIDTH;
        gameBitmapBuffer.height = (int)QVGA_HEIGHT;
        gameBitmapBuffer.pitch = (int)QVGA_WIDTH * (int)BYTES_PER_PIXEL;
    }

    return self;
}

//-----------------------------------------------------------------------------
//  acceptsFirstResponder
//  Make this the first object that responds to events.
//-----------------------------------------------------------------------------
- (BOOL)acceptsFirstResponder
{
    return YES;
}

//-----------------------------------------------------------------------------
//  keyDown
//  Handle keyboard events.
//-----------------------------------------------------------------------------
- (void)keyDown:(NSEvent *)event
{
    if ([event modifierFlags] & NSNumericPadKeyMask) {
        NSString *keyArrow = [event charactersIgnoringModifiers];
        unichar keyChar = 0;
        if ([keyArrow length] == 0)
            return;
        if ([keyArrow length] == 1) {
            keyChar = [keyArrow characterAtIndex:0];
            if (keyChar == NSLeftArrowFunctionKey) {
                GameKeyboardUpdate(&gameState, GAME_KEY_LEFT, true);
                return;
            }
            else if (keyChar == NSRightArrowFunctionKey) {
                GameKeyboardUpdate(&gameState, GAME_KEY_RIGHT, true);
                return;
            }
        }
    }
    switch([event keyCode]) {
    case 53: // esc
        if (![event isARepeat])
            GameKeyboardUpdate(&gameState, GAME_KEY_ESCAPE, true);
        return;
    }

    [super keyDown:event];
}

//-----------------------------------------------------------------------------
//  keyUp
//  Handle keyboard events.
//-----------------------------------------------------------------------------
- (void)keyUp:(NSEvent *)event
{
    if ([event modifierFlags] & NSNumericPadKeyMask) {
        NSString *keyArrow = [event charactersIgnoringModifiers];
        unichar keyChar = 0;
        if ([keyArrow length] == 0)
            return;
        if ([keyArrow length] == 1) {
            keyChar = [keyArrow characterAtIndex:0];
            if (keyChar == NSLeftArrowFunctionKey) {
                GameKeyboardUpdate(&gameState, GAME_KEY_LEFT, false);
                return;
            }
            else if (keyChar == NSRightArrowFunctionKey) {
                GameKeyboardUpdate(&gameState, GAME_KEY_RIGHT, false);
                return;
            }
        }
    }
    switch([event keyCode]) {
    case 53: // esc
        GameKeyboardUpdate(&gameState, GAME_KEY_ESCAPE, false);
        return;
    }

    [super keyDown:event];
}

//-----------------------------------------------------------------------------
//  gameLoop
//  Update the game state and render. This function is called periodically
//  by a timer that starts when the NSView is created.
//-----------------------------------------------------------------------------
- (void)gameLoop:(NSTimer *)timer
{
    static mach_timebase_info_data_t timebaseInfo;
    if (timebaseInfo.denom == 0)
        (void) mach_timebase_info(&timebaseInfo);
    timeEndAbsolute = mach_absolute_time();
    timeElapsedAbsolute = timeEndAbsolute - timeStartAbsolute;
    timeElapsedNanoseconds = timeElapsedAbsolute * timebaseInfo.numer / timebaseInfo.denom;
    timeElapsedMilliseconds = (float) timeElapsedNanoseconds / NSEC_PER_MSEC;
    timeAccumulatorMilliseconds += timeElapsedMilliseconds;

    while (timeAccumulatorMilliseconds >= MS_PER_UPDATE) {
        GameUpdate(MS_PER_UPDATE, &gameState);
        timeAccumulatorMilliseconds -= MS_PER_UPDATE;
    }

    GameRender(&gameState, &gameBitmapBuffer);

    [self setNeedsDisplay:YES];

    timeStartAbsolute = timeEndAbsolute;
}

//-----------------------------------------------------------------------------
//  drawRect
//  Draw the screen. Triggerd by setNeedsDisplay.
//-----------------------------------------------------------------------------
- (void)drawRect:(NSRect)rect
{
    NSRect bounds = [self bounds];
    NSBitmapImageRep *imageRep = [[[NSBitmapImageRep alloc] 
                                  initWithBitmapDataPlanes:(void *)&gameBitmapBuffer.memory
                                  pixelsWide:gameBitmapBuffer.width
                                  pixelsHigh:gameBitmapBuffer.height
                                  bitsPerSample:BITS_PER_PIXEL_COMPONENT
                                  samplesPerPixel:BYTES_PER_PIXEL
                                  hasAlpha:YES
                                  isPlanar:NO
                                  colorSpaceName:NSCalibratedRGBColorSpace
                                  bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
                                  bytesPerRow:(BYTES_PER_PIXEL * gameBitmapBuffer.width)
                                  bitsPerPixel:(BITS_PER_PIXEL_COMPONENT * BYTES_PER_PIXEL)]
                                  autorelease];
    NSImage *image = [[[NSImage alloc] initWithSize:NSMakeSize(QVGA_WIDTH, QVGA_HEIGHT)] autorelease];
    [image addRepresentation:imageRep];
    // The image needs to be flipped. The game draws the frame bottom up, but Cocoa draws it top down.
    NSAffineTransform *windowTransform = [NSAffineTransform transform];
    [windowTransform translateXBy:0.0 yBy:bounds.size.height];
    [windowTransform scaleXBy:1.0 yBy:-1.0];
    [windowTransform concat];
    [image drawInRect:bounds
           fromRect:NSZeroRect
           operation:NSCompositeCopy
           fraction:1.0];
}

@end // @implementation WindowView
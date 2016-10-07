/*=============================================================================
    game.h
 =============================================================================*/

#ifndef GAME_H
#define GAME_H

#include "text.h"

#define PI 3.14159265359

#define MS_PER_SECOND 1000

#define NUM_KEYS 3
#define GAME_KEY_LEFT 0
#define GAME_KEY_RIGHT 1
#define GAME_KEY_ESCAPE 2

#define BALL_INIT_X 0.0f
#define BALL_INIT_Y 115.0f
#define BALL_INIT_ANGLE_DEGREES 330.0
#define BALL_INIT_ANGLE_RADIANS (BALL_INIT_ANGLE_DEGREES * (PI/180.0))
#define BALL_ANGLE_DEGREES_REFLECT_PADDLE_MIN 45.0
#define BALL_ANGLE_DEGREES_REFLECT_PADDLE_MAX 135.0
#define BALL_WIDTH 8
#define BALL_HEIGHT 8
#define BALL_SPEED_PIXELS_PER_SECOND 90.0f

#define PADDLE_INIT_X 128.0f
#define PADDLE_INIT_Y 16.0f
#define PADDLE_WIDTH 64
#define PADDLE_HEIGHT 8
#define PADDLE_DEAD_ZONE_RADIUS 16
#define PADDLE_DEAD_ZONE_CENTER ((PADDLE_WIDTH + BALL_WIDTH) / 2)
#define PADDLE_DEAD_ZONE_LEFT (PADDLE_DEAD_ZONE_CENTER - PADDLE_DEAD_ZONE_RADIUS)
#define PADDLE_DEAD_ZONE_RIGHT (PADDLE_DEAD_ZONE_CENTER + PADDLE_DEAD_ZONE_RADIUS)
#define PADDLE_SPEED_PIXELS_PER_SECOND 90.0f

#define BRICK_WIDTH 16
#define BRICK_HEIGHT 8
#define BRICK_ROWS 7
#define BRICK_COLUMNS 20
#define BRICK_POSITION_Y_FIRST_COLUMN 140.0f

#define LIVES_INIT 3
#define LIVES_X 0
#define LIVES_Y 231

#define SCORE_POINTS_PER_BRICK 1
#define SCORE_DIGITS 3
#define SCORE_MAX 999
#define SCORE_X 295
#define SCORE_Y 231

#define COUNTDOWN_TIME 3.5f
#define COUNTDOWN_LABEL_X 124
#define COUNTDOWN_LABEL_Y 82
#define COUNTDOWN_NUM_X 156
#define COUNTDOWN_NUM_Y 66

#ifdef __APPLE__
    #define COLOR_BLACK 0xFF000000
    #define COLOR_WHITE 0xFFFFFFFF
    #define COLOR_RED 0xFF0000FF
    #define COLOR_ORANGE 0xFF00A5FF
    #define COLOR_YELLOW 0xFF00FFFF
    #define COLOR_GREEN 0xFF00FF00
    #define COLOR_BLUE 0xFFFF0000
    #define COLOR_INDIGO 0xFF82004B
    #define COLOR_VIOLET 0xFFC9388D
#elif _WIN32
    #define COLOR_WHITE 0x00FFFFFF
    #define COLOR_RED 0x00FF0000
    #define COLOR_ORANGE 0x00FFA500
    #define COLOR_YELLOW 0x00FFFF00
    #define COLOR_GREEN 0x0000FF00
    #define COLOR_BLUE 0x000000FF
    #define COLOR_INDIGO 0x004B0082
    #define COLOR_VIOLET 0x008D38C9
#else
    #error "Unknown compiler."
#endif

#define DegreesToRadians(degrees) (degrees * ((PI/180.0)))

enum brick_colors {
    RED,
    ORANGE,
    YELLOW,
    GREEN,
    BLUE,
    INDIGO,
    VIOLET
};

struct vector_2d {
    float x;
    float y;
};

struct rectangle {
    struct vector_2d position;
    int width;
    int height;
};

struct bitmap_buffer {
    void *memory;
    int memorySize;
    int width;
    int height;
    int pitch;
};

struct paddle_vars {
    struct rectangle rect;
    int color;
};

struct ball_vars {
    struct rectangle rect;
    struct vector_2d velocity;
    int color;
};

struct brick_vars {
    struct rectangle rect;
    int color;
    bool broken;
};

struct impact_state {
    struct rectangle rectOverlap;
    float overlapPosRight;
    float overlapPosTop;
    bool impactLeftRight;
    bool impactTopBottom;
    bool impactLeft;
    bool impactRight;
    bool impactTop;
    bool impactBottom;
    bool ballMovingLeft;
    bool ballMovingRight;
    bool ballMovingUp;
    bool ballMovingDown;
};

struct game_state {
    bool paused;
    bool pausedUser;
    float countdown;
    bool keyboard[NUM_KEYS];
    struct paddle_vars paddle;
    struct ball_vars ball;
    struct brick_vars bricks[BRICK_ROWS][BRICK_COLUMNS];
    int lives;
    int score;
    struct text_cursor cursor;
};

void GameInit(struct game_state *);
void BallSetVelocity(struct game_state *, double);
void BallInit(struct game_state *);
void GameUpdate(float, struct game_state *);
void GameRender(struct game_state *, struct bitmap_buffer *);
void GameKeyboardUpdate(struct game_state *, int, bool);
void DrawRectangle(struct rectangle, uint32_t, struct bitmap_buffer *);
bool DetectCollisionRectangle(struct rectangle, struct rectangle);
void CalculateImpactState(struct impact_state *, struct game_state *, struct rectangle, struct rectangle);
void BallBouncePaddle(struct game_state *, struct rectangle, struct rectangle);
void BallBounceBrick(struct game_state *, struct rectangle, struct rectangle);
float CalcMin(float, float);
float CalcMax(float, float);
float ClampMin(float, float);
float ClampMax(float, float);

#endif /* GAME_H */
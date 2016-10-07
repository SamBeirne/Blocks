/*=============================================================================
    game.c
 =============================================================================*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "game.h"
#include "text.h"

/*-----------------------------------------------------------------------------
    GameInit
    Initialize the game state.
 ----------------------------------------------------------------------------*/
void GameInit(struct game_state *gameState)
{
    gameState->paused = true;
    gameState->pausedUser = false;
    gameState->countdown = COUNTDOWN_TIME;

    gameState->paddle.rect.position.x = PADDLE_INIT_X;
    gameState->paddle.rect.position.y = PADDLE_INIT_Y;
    gameState->paddle.rect.width = PADDLE_WIDTH;
    gameState->paddle.rect.height = PADDLE_HEIGHT;
    gameState->paddle.color = COLOR_WHITE;

    BallInit(gameState);

    float brickPositionY = BRICK_POSITION_Y_FIRST_COLUMN;
    float brickPositionX;
    enum brick_colors color;
    for (int brickRow = 0; brickRow < BRICK_ROWS; brickRow++) {
        brickPositionX = 0.0f;
        color = brickRow;
        for (int brickColumn = 0; brickColumn < BRICK_COLUMNS; brickColumn++) {
            gameState->bricks[brickRow][brickColumn].rect.position.x = brickPositionX;
            gameState->bricks[brickRow][brickColumn].rect.position.y = brickPositionY;
            gameState->bricks[brickRow][brickColumn].rect.width = BRICK_WIDTH;
            gameState->bricks[brickRow][brickColumn].rect.height = BRICK_HEIGHT;
            switch (color) {
            case RED:
                gameState->bricks[brickRow][brickColumn].color = COLOR_RED;
                break;
            case ORANGE:
                gameState->bricks[brickRow][brickColumn].color = COLOR_ORANGE;
                break;
            case YELLOW:
                gameState->bricks[brickRow][brickColumn].color = COLOR_YELLOW;
                break;
            case GREEN:
                gameState->bricks[brickRow][brickColumn].color = COLOR_GREEN;
                break;
            case BLUE:
                gameState->bricks[brickRow][brickColumn].color = COLOR_BLUE;
                break;
            case INDIGO:
                gameState->bricks[brickRow][brickColumn].color = COLOR_INDIGO;
                break;
            case VIOLET:
                gameState->bricks[brickRow][brickColumn].color = COLOR_VIOLET;
                break;
            }
            gameState->bricks[brickRow][brickColumn].broken = false;
            brickPositionX += (float)BRICK_WIDTH;
        }
        brickPositionY += (float)BRICK_HEIGHT;
    }

    gameState->lives = LIVES_INIT;
    gameState->score = 0;

    gameState->cursor.x = 0;
    gameState->cursor.y = 0;
    gameState->cursor.size = FONT_SIZE;

    return;
}

/*-----------------------------------------------------------------------------
    BallSetVelocity
    Sets the x/y velocity of the ball based on an angle.
 ----------------------------------------------------------------------------*/
void BallSetVelocity(struct game_state *gameState, double angle)
{
    gameState->ball.velocity.x = BALL_SPEED_PIXELS_PER_SECOND * (float)cos(angle);
    gameState->ball.velocity.y = BALL_SPEED_PIXELS_PER_SECOND * (float)sin(angle);
    return;
}

/*-----------------------------------------------------------------------------
    BallInit
    Initialize the ball.
 ----------------------------------------------------------------------------*/
void BallInit(struct game_state *gameState)
{
    gameState->ball.rect.position.x = BALL_INIT_X;
    gameState->ball.rect.position.y = BALL_INIT_Y;
    gameState->ball.rect.width = BALL_WIDTH;
    gameState->ball.rect.height = BALL_HEIGHT;
    BallSetVelocity(gameState, BALL_INIT_ANGLE_RADIANS);
    gameState->ball.color = COLOR_WHITE;

    return;
}

/*-----------------------------------------------------------------------------
    GameUpdate
    Update the game state based on the time elapsed since the last update.
 ----------------------------------------------------------------------------*/
void GameUpdate(float deltaTimeMs, struct game_state *gameState)
{
    if (gameState->pausedUser)
        return;

    float secondElapsed = (deltaTimeMs / (float)MS_PER_SECOND);

    if (gameState->paused) {
        if (gameState->countdown >= 0.0f) {
            gameState->countdown -= secondElapsed;
            gameState->countdown = ClampMin(gameState->countdown, 0.0f);
        }
        if (gameState->countdown == 0.0f)
            gameState->paused = false;

        return;
    }

    /* Update paddle. */
    if (gameState->keyboard[GAME_KEY_LEFT] && !(gameState->keyboard[GAME_KEY_RIGHT])) {
        gameState->paddle.rect.position.x -= PADDLE_SPEED_PIXELS_PER_SECOND * secondElapsed;
        gameState->paddle.rect.position.x = ClampMin(gameState->paddle.rect.position.x, 0.0f);
    }
    else if (gameState->keyboard[GAME_KEY_RIGHT] && !(gameState->keyboard[GAME_KEY_LEFT])) {
        gameState->paddle.rect.position.x += PADDLE_SPEED_PIXELS_PER_SECOND * secondElapsed;
        gameState->paddle.rect.position.x = ClampMax(gameState->paddle.rect.position.x, (QVGA_WIDTH - PADDLE_WIDTH));
    }

    /* Update ball. */
    gameState->ball.rect.position.x += gameState->ball.velocity.x * secondElapsed;
    gameState->ball.rect.position.x = ClampMin(gameState->ball.rect.position.x, 0.0f);
    gameState->ball.rect.position.x = ClampMax(gameState->ball.rect.position.x, (QVGA_WIDTH - BALL_WIDTH));
    
    gameState->ball.rect.position.y += gameState->ball.velocity.y * secondElapsed;
    gameState->ball.rect.position.y = ClampMin(gameState->ball.rect.position.y, 0.0f);
    gameState->ball.rect.position.y = ClampMax(gameState->ball.rect.position.y, (QVGA_HEIGHT - BALL_HEIGHT));

    if (gameState->ball.rect.position.y == 0.0f) {
        if (gameState->lives > 0) {
            gameState->lives -= 1;
            BallInit(gameState);
            gameState->paused = true;
            gameState->countdown = COUNTDOWN_TIME;
        }
        else
            GameInit(gameState);

        return;
    }

    /* Check for collisions. */
    struct rectangle rectBall;
    struct rectangle rectPaddle;
    struct rectangle rectBrick;

    rectBall = gameState->ball.rect;
    rectPaddle = gameState->paddle.rect;

    /* Walls. */
    if (rectBall.position.x == 0.0f || rectBall.position.x == (QVGA_WIDTH - BALL_WIDTH))
        gameState->ball.velocity.x *= -1;
    if (rectBall.position.y == 0.0f || rectBall.position.y == (QVGA_HEIGHT - BALL_WIDTH))
        gameState->ball.velocity.y *= -1;

    /* Paddle. */
    if (DetectCollisionRectangle(rectBall, rectPaddle))
        BallBouncePaddle(gameState, rectBall, rectPaddle);

    /* Bricks. */
    for (int brickRow = 0; brickRow < BRICK_ROWS; brickRow++) {
        for (int brickColumn = 0; brickColumn < BRICK_COLUMNS; brickColumn++) {
            rectBrick = gameState->bricks[brickRow][brickColumn].rect;
            if (!gameState->bricks[brickRow][brickColumn].broken) {
                if (DetectCollisionRectangle(rectBall, rectBrick)) {
                    gameState->bricks[brickRow][brickColumn].broken = true;
                    if (gameState->score < SCORE_MAX)
                        gameState->score += SCORE_POINTS_PER_BRICK;
                    BallBounceBrick(gameState, rectBall, rectBrick);
                }
            }
        }
    }

    return;
}

/*-----------------------------------------------------------------------------
    GameRender
    Render the current game state to a bitmap buffer.
 ----------------------------------------------------------------------------*/
void GameRender(struct game_state *gameState, struct bitmap_buffer *bitmapBuffer)
{
    /* Clear bitmap to black. */
    #ifdef __APPLE__
        const uint32_t black = COLOR_BLACK;
        const void *pattern;
        pattern = &black;
        memset_pattern4(bitmapBuffer->memory, pattern, bitmapBuffer->memorySize);
    #else
        memset(bitmapBuffer->memory, 0x00, bitmapBuffer->memorySize);
    #endif

    /* Draw paddle. */
    DrawRectangle(
        gameState->paddle.rect,
        gameState->paddle.color,
        bitmapBuffer);

    /* Draw ball. */
    DrawRectangle(
        gameState->ball.rect,
        gameState->ball.color,
        bitmapBuffer);

    /* Draw bricks. */
    for (int brickRow = 0; brickRow < BRICK_ROWS; brickRow++) {
        for (int brickColumn = 0; brickColumn < BRICK_COLUMNS; brickColumn++) {
            if (gameState->bricks[brickRow][brickColumn].broken == false) {
                DrawRectangle(
                    gameState->bricks[brickRow][brickColumn].rect,
                    gameState->bricks[brickRow][brickColumn].color,
                    bitmapBuffer);
            }
        }
    }

    /* Draw countdown. */
    if (gameState->countdown > 0.0f) {
        gameState->cursor.x = COUNTDOWN_LABEL_X;
        gameState->cursor.y = COUNTDOWN_LABEL_Y;
        DrawString("GET READY", &gameState->cursor, COLOR_WHITE, bitmapBuffer);

        gameState->cursor.x = COUNTDOWN_NUM_X;
        gameState->cursor.y = COUNTDOWN_NUM_Y;
        DrawDigit((int)gameState->countdown, &gameState->cursor, COLOR_WHITE, bitmapBuffer);
    }

    /* Draw lives. */
    gameState->cursor.x = LIVES_X;
    gameState->cursor.y = LIVES_Y;
    DrawDigit(gameState->lives, &gameState->cursor, COLOR_WHITE, bitmapBuffer);

    /* Draw score. */
    gameState->cursor.x = SCORE_X;
    gameState->cursor.y = SCORE_Y;
    DrawNumber(gameState->score, SCORE_DIGITS, &gameState->cursor, COLOR_WHITE, bitmapBuffer);

    return;
}

/*-----------------------------------------------------------------------------
    GameKeyboardUpdate
    Update the keyboard state when a key is pressed/released.
 ----------------------------------------------------------------------------*/
void GameKeyboardUpdate(struct game_state *gameState, int key, bool keyIsDown)
{
    if (key == GAME_KEY_ESCAPE && keyIsDown)
        gameState->pausedUser = !gameState->pausedUser;

    gameState->keyboard[key] = keyIsDown;

    return;
}

/*-----------------------------------------------------------------------------
    DrawRectangle
    Draw a solid rectangle in a bitmap buffer.
 ----------------------------------------------------------------------------*/
void DrawRectangle(struct rectangle rect, uint32_t color, struct bitmap_buffer *bitmapBuffer)
{
    uint8_t *row = bitmapBuffer->memory;
    row += bitmapBuffer->pitch * (int)rect.position.y;
    for (int rectY = 0; rectY < rect.height; rectY++) {
        uint32_t *pixel = (uint32_t *)row;
        pixel += (int)rect.position.x;
        for (int rectX = 0; rectX < rect.width; rectX++) {
            *pixel = color;
            pixel++;
        }
        row += bitmapBuffer->pitch;
    }

    return;
}

/*-----------------------------------------------------------------------------
    DetectCollisionRectangle
    Check for a collision of two rectangles.
 ----------------------------------------------------------------------------*/
bool DetectCollisionRectangle(struct rectangle rect1, struct rectangle rect2)
{
    if (rect1.position.x + rect1.width < rect2.position.x) return false;
    if (rect1.position.x > rect2.position.x + rect2.width) return false;
    if (rect1.position.y + rect1.height < rect2.position.y) return false;
    if (rect1.position.y > rect2.position.y + rect2.height) return false;
    return true;
}

/*-----------------------------------------------------------------------------
    CalculateImpactState
    Calculate the state of the impact between two rectangles, the ball and
    another object, so the ball can later be reflected properly.
 ----------------------------------------------------------------------------*/
void CalculateImpactState(struct impact_state *impact, struct game_state *gameState, struct rectangle rectA, struct rectangle rectB)
{
    impact->rectOverlap.position.x = CalcMax(rectA.position.x, rectB.position.x);
    impact->rectOverlap.position.y = CalcMax(rectA.position.y, rectB.position.y);
    impact->overlapPosRight = CalcMin(rectA.position.x + rectA.width, rectB.position.x + rectB.width);
    impact->overlapPosTop = CalcMin(rectA.position.y + rectA.height, rectB.position.y + rectB.height);
    impact->rectOverlap.width = impact->overlapPosRight - impact->rectOverlap.position.x;
    impact->rectOverlap.height = impact->overlapPosTop - impact->rectOverlap.position.y;

    impact->impactLeftRight = impact->rectOverlap.height > impact->rectOverlap.width;
    impact->impactTopBottom = impact->rectOverlap.width >= impact->rectOverlap.height;

    impact->impactLeft = impact->impactLeftRight && rectB.position.x == impact->rectOverlap.position.x;
    impact->impactRight = impact->impactLeftRight && rectA.position.x == impact->rectOverlap.position.x;
    impact->impactTop = impact->impactTopBottom && rectA.position.y == impact->rectOverlap.position.y;
    impact->impactBottom = impact->impactTopBottom && rectB.position.y == impact->rectOverlap.position.y;

    impact->ballMovingLeft = gameState->ball.velocity.x < 0;
    impact->ballMovingRight = gameState->ball.velocity.x > 0;
    impact->ballMovingUp = gameState->ball.velocity.y > 0;
    impact->ballMovingDown = gameState->ball.velocity.y < 0;

    return;
}

/*-----------------------------------------------------------------------------
    BallBouncePaddle
    Reflect the ball off the paddle.
 ----------------------------------------------------------------------------*/
void BallBouncePaddle(struct game_state *gameState, struct rectangle rectBall, struct rectangle rectPaddle)
{
    struct impact_state impact;
    float ballPosRelativePaddle, ballRatioPaddle, ballNewAngle;
    int deadZoneMin, deadZoneMax;

    CalculateImpactState(&impact, gameState, rectBall, rectPaddle);

    if (impact.impactTop && impact.ballMovingDown) {
        /* The game is too easy if the ball can be reflected straight
           vertically off the center of the paddle, so add a dead zone in the
           center of the paddle making the ball bounce off at an angle */
        ballPosRelativePaddle = (PADDLE_WIDTH + BALL_WIDTH) - (rectBall.position.x - (rectPaddle.position.x - BALL_WIDTH));
        if (ballPosRelativePaddle > PADDLE_DEAD_ZONE_LEFT && ballPosRelativePaddle < PADDLE_DEAD_ZONE_RIGHT) {
            if (ballPosRelativePaddle < PADDLE_DEAD_ZONE_CENTER)
                ballPosRelativePaddle -= PADDLE_DEAD_ZONE_RADIUS;
            else
                ballPosRelativePaddle += PADDLE_DEAD_ZONE_RADIUS;
        }
        ballRatioPaddle = ballPosRelativePaddle / (PADDLE_WIDTH + BALL_WIDTH);
        ballNewAngle = ballRatioPaddle * (BALL_ANGLE_DEGREES_REFLECT_PADDLE_MAX - BALL_ANGLE_DEGREES_REFLECT_PADDLE_MIN) + BALL_ANGLE_DEGREES_REFLECT_PADDLE_MIN;
        BallSetVelocity(gameState, DegreesToRadians(ballNewAngle));
    }
    else if (impact.impactLeft && impact.ballMovingRight)
        gameState->ball.velocity.x *= -1;
    else if (impact.impactRight && impact.ballMovingLeft)
        gameState->ball.velocity.x *= -1;

    return;
}

/*-----------------------------------------------------------------------------
    BallBounceBrick
    Reflect the ball off a brick.
 ----------------------------------------------------------------------------*/
void BallBounceBrick(struct game_state *gameState, struct rectangle rectBall, struct rectangle rectBrick)
{
    struct impact_state impact;

    CalculateImpactState(&impact, gameState, rectBall, rectBrick);

    if (impact.impactLeft && impact.ballMovingRight)
        gameState->ball.velocity.x *= -1;
    else if (impact.impactRight && impact.ballMovingLeft)
        gameState->ball.velocity.x *= -1;
    else if (impact.impactTop && impact.ballMovingDown)
        gameState->ball.velocity.y *= -1;
    else if (impact.impactBottom && impact.ballMovingUp)
        gameState->ball.velocity.y *= -1;

    return;
}

/*-----------------------------------------------------------------------------
    CalcMin
    Return the minimum of two floats.
 ----------------------------------------------------------------------------*/
float CalcMin(float valA, float valB)
{
    return (valA < valB) ? valA : valB;
}

/*-----------------------------------------------------------------------------
    CalcMax
    Return the maximum of two floats.
 ----------------------------------------------------------------------------*/
float CalcMax(float valA, float valB)
{
    return (valA > valB) ? valA : valB;
}

/*-----------------------------------------------------------------------------
    ClampMin
    Clamp a float to a minimum value.
 ----------------------------------------------------------------------------*/
float ClampMin(float val, float min)
{
    if (val < min)
        return min;
    else
        return val;
}

/*-----------------------------------------------------------------------------
    ClampMax
    Clamp a float to a maximum value.
 ----------------------------------------------------------------------------*/
float ClampMax(float val, float max)
{
    if (val > max)
        return max;
    else
        return val;
}
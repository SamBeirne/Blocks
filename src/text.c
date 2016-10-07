/*=============================================================================
    text.c
 =============================================================================*/

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "game.h"
#include "text.h"

/*-----------------------------------------------------------------------------
    DrawGlyph
    Draw a glyph to a bitmap buffer.
 ----------------------------------------------------------------------------*/
void DrawGlyph(char font[][FONT_SIZE], unsigned int glyph, struct text_cursor *cursor, uint32_t color, struct bitmap_buffer *bitmapBuffer)
{
    bool fillPixel;

    uint8_t *row = bitmapBuffer->memory;
    row += bitmapBuffer->pitch * cursor->y;
    for (int glyphY = 0; glyphY < FONT_SIZE; glyphY++) {
        uint32_t *pixel = (uint32_t *)row;
        pixel += cursor->x;
        for (int glyphX = 0; glyphX < FONT_SIZE; glyphX++) {
            fillPixel = font[glyph][glyphY] & (1 << (FONT_SIZE - glyphX));
            if (fillPixel)
                *pixel = color;
            pixel++;
        }
        row += bitmapBuffer->pitch;
    }

    cursor->x += cursor->size;

    return;
}

/*-----------------------------------------------------------------------------
    DrawCharacter
    Draw a single character to a bitmap buffer.
 ----------------------------------------------------------------------------*/
void DrawCharacter(unsigned int character, struct text_cursor *cursor, uint32_t color, struct bitmap_buffer *bitmapBuffer)
{
    character -= ASCII_OFFSET;

    if (character < NUM_OF_LETTERS)
        DrawGlyph(font_letters, character, cursor, color, bitmapBuffer);

    return;
}

/*-----------------------------------------------------------------------------
    DrawString
    Draw a string to a bitmap buffer.
 ----------------------------------------------------------------------------*/
void DrawString(char *string, struct text_cursor *cursor, uint32_t color, struct bitmap_buffer *bitmapBuffer)
{
    char c;

    while(*string != '\0') {
        c = *string;
        if (c == ' ')
            cursor->x += cursor->size;
        else
            DrawCharacter((unsigned int)c, cursor, color, bitmapBuffer);
        string++;
    }

    return;
}

/*-----------------------------------------------------------------------------
    DrawDigit
    Draw a single digit to a bitmap buffer.
 ----------------------------------------------------------------------------*/
void DrawDigit(unsigned int digit, struct text_cursor *cursor, uint32_t color, struct bitmap_buffer *bitmapBuffer)
{
    if (digit < NUM_OF_NUMBERS)
        DrawGlyph(font_numbers, digit, cursor, color, bitmapBuffer);

    return;
}

/*-----------------------------------------------------------------------------
    DrawNumber
    Draw a number to a bitmap buffer.
 ----------------------------------------------------------------------------*/
void DrawNumber(int number, int digits, struct text_cursor *cursor, uint32_t color, struct bitmap_buffer *bitmapBuffer)
{
    int digit;
    int padding = pow(10, digits - 1);

    while (padding > 0 && number / padding == 0) {
        DrawDigit(0, cursor, color, bitmapBuffer);
        padding /= 10;
        digits -= 1;
    }

    number = ReverseNumber(number);

    while (number > 0) {
        digit = number % 10;
        DrawDigit(digit, cursor, color, bitmapBuffer);
        number /= 10;
        digits -= 1;
    }

    if (digits > 0)
        DrawDigit(0, cursor, color, bitmapBuffer);

    return;
}

/*-----------------------------------------------------------------------------
    ReverseNumber
    Reverse the digits in a number (so it can be printed from left to right).
 ----------------------------------------------------------------------------*/
int ReverseNumber(int number)
{
    int reverse = 0;

    while (number != 0) {
        reverse *= 10;
        reverse += number % 10;
        number /= 10;
        ReverseNumber(number);
    }

    return reverse;
}
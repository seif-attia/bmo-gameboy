#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "controller.h"

#define DINO_SCREEN_WIDTH 160
#define DINO_SCREEN_HEIGHT 128

#define GROUND_Y 116
#define DINO_X_POS 20
#define GRAVITY 1
#define JUMP_IMPULSE -9

#define DINO_W 28     // Width of Finn riding Jake-Dino sprite
#define DINO_H 26     // Height of Finn riding Jake-Dino sprite

typedef enum
{
    GAME_STATE_START,     // Waiting at start screen for player to press START
    GAME_STATE_RUNNING,   // Active gameplay in progress
    GAME_STATE_GAME_OVER, // Player collided with obstacle, displaying loss panel
    GAME_STATE_EXIT       // Player pressed BACK to quit back to main menu
} GameState;

typedef struct
{
    int16_t x, y;         // Current screen coordinates
    int16_t old_x, old_y; // Previous frame coordinates (used for dirty rect clearing)
    int8_t vy;            // Current vertical velocity
    bool is_jumping;      // Flag preventing double-jumping while in mid-air
    uint8_t anim_frame;   // Animation run frame toggle (0 or 1)
} PlayerDino;

typedef struct
{
    int16_t x, y;          // Current screen coordinates
    int16_t old_x, old_y;  // Previous frame coordinates (used for dirty rect clearing)
    uint8_t w, h;          // Dynamic obstacle dimensions (varies by enemy type)
    const uint8_t *bitmap; // Pointer to 1-bit PROGMEM bitmap
    bool active;           // Obstacle active flag
} Obstacle;

void play_dino(void);
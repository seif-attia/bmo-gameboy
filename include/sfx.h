#ifndef SFX_H
#define SFX_H

#include <avr/pgmspace.h>

// Game Over & Victory
extern const char sfx_gameover_retro[] PROGMEM;
extern const char sfx_gameover_fast[] PROGMEM;
extern const char sfx_gameover_mario[] PROGMEM;
extern const char sfx_gameover_1[] PROGMEM;
extern const char sfx_gameover_2[] PROGMEM;
extern const char sfx_gameover_3[] PROGMEM;

// Snake
extern const char sfx_snake_eat[] PROGMEM;

// Flappy Bird & Dino
extern const char sfx_flappy_score[] PROGMEM;
extern const char sfx_dino_jump[] PROGMEM;

// Pong
extern const char sfx_pong_hit[] PROGMEM;
extern const char sfx_pong_wall[] PROGMEM;
extern const char sfx_pong_score[] PROGMEM;

// Tic-Tac-Toe
extern const char sfx_tictactoe_x[] PROGMEM;
extern const char sfx_tictactoe_o[] PROGMEM;
extern const char sfx_tictactoe_win[] PROGMEM;

#endif // SFX_H
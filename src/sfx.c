#include "sfx.h"

// Game Over & Victory
const char sfx_gameover_retro[] PROGMEM = "GameOver:d=16,o=5,b=160:g,f,e,4d";
const char sfx_gameover_fast[] PROGMEM = "Die:d=32,o=5,b=240:b,a,g,f,e,d,c";
const char sfx_gameover_mario[] PROGMEM = "better_gameover:d=4,o=5,b=150:8c6,8p,8g,8p,8e.,8a,8b,8a,8g#.,8a#.,8g#.,16g.,16f.,1g";

// Snake
const char sfx_snake_eat[] PROGMEM = "Eat:d=16,o=6,b=260:c,g";

// Flappy Bird & Dino
const char sfx_flappy_score[] PROGMEM = "Score:d=16,o=6,b=240:e,8g";
const char sfx_dino_jump[] PROGMEM = "Jump:d=32,o=6,b=300:c,e,g,c7";

// Pong
const char sfx_pong_hit[] PROGMEM = "Hit:d=32,o=4,b=300:a";
const char sfx_pong_wall[] PROGMEM = "Wall:d=32,o=4,b=300:e";
const char sfx_pong_score[] PROGMEM = "Point:d=16,o=6,b=240:g,c7";

// Tic-Tac-Toe
const char sfx_tictactoe_x[] PROGMEM = "PlaceX:d=32,o=5,b=280:e";
const char sfx_tictactoe_o[] PROGMEM = "PlaceO:d=32,o=5,b=280:b";
const char sfx_tictactoe_win[] PROGMEM = "TTTWin:d=16,o=5,b=200:c,e,8g";
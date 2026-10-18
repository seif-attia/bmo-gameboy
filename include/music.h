#ifndef MUSIC_H
#define MUSIC_H

#include <avr/io.h>
#include <stdbool.h>

#define tonePin_Arduino PD3 // D3 on arduino nano

/**
 * @brief Starts playing an RTTTL song on loop continuously.
 * @param p Pointer to the RTTTL string (can be stored in RAM or PROGMEM).
 */
void start_song(const char *p);

/**
 * @brief Plays an RTTTL song or sound effect once and stops automatically at the end.
 * @param p Pointer to the RTTTL string (can be stored in RAM or PROGMEM).
 */
void play_once(const char *p);

/**
 * @brief Alias for play_once(), intended for one-shot in-game sound effects.
 * @param p Pointer to the RTTTL string (can be stored in RAM or PROGMEM).
 */
void play_sfx(const char *p);

/**
 * @brief Stops any currently playing music or sound effect immediately.
 */
void stop_song(void);

/**
 * @brief Checks if a song or sound effect is currently playing.
 * @return true if audio is active, false if idle.
 */
bool is_music_playing(void);

/**
 * @brief Non-blocking state machine update function.
 *        Must be called continuously inside loop() or a timer interrupt.
 */
void update_music(void);

#endif
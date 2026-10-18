#include "music.h"
#include "tone.h"
#include "timer.h"
#include <stdbool.h>
#include <string.h>
#include <avr/pgmspace.h>

// Musical pitch table across Octaves 3 through 7 (C3 to B7)
static const uint16_t note_pitches[] PROGMEM = {
    // Octave 3: C3 to B3
    131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
    // Octave 4: C4 to B4
    262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
    // Octave 5: C5 to B5
    523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
    // Octave 6: C6 to B6
    1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
    // Octave 7: C7 to B7
    2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951};

#define is_digit_char(n) ((n) >= '0' && (n) <= '9')

// Helper function to safely read characters from RAM or PROGMEM
static inline char read_char(const char *ptr)
{
#if defined(__AVR__)
    return pgm_read_byte(ptr);
#else
    return *ptr;
#endif
}

// --- State Machine Variables ---
static const char *current_p = NULL;
static uint16_t bpm = 63;
static uint8_t default_oct = 6;
static uint8_t default_dur = 4;
static uint32_t beatEvery = 0;

static bool is_playing = false;
static bool is_note_sounding = false;
static uint32_t note_start_time = 0;
static uint32_t current_phase_duration = 0;
static uint32_t next_pause_duration = 0;

static const char *original_song_ptr = NULL;
static bool loop_song = true;

static void play_internal(const char *p, bool loop)
{
    if (!p)
        return;

    original_song_ptr = p;
    loop_song = loop;
    noTone(tonePin_Arduino);

    is_playing = true;
    is_note_sounding = false;
    current_phase_duration = 0; // Trigger first note immediately in update_music
    note_start_time = millis();

    // Default RTTTL values
    bpm = 63;
    default_oct = 6;
    default_dur = 4;

    current_p = p;

    // 1. Skip Name Section
    while (read_char(current_p) != ':' && read_char(current_p) != '\0')
        current_p++;
    if (read_char(current_p) == ':')
        current_p++;

    // 2. Parse Defaults Section (d=X, o=Y, b=Z)
    while (read_char(current_p) != ':' && read_char(current_p) != '\0')
    {
        while (read_char(current_p) == ' ')
            current_p++;

        char c = read_char(current_p);
        if (c == 'd')
        {
            current_p++;
            if (read_char(current_p) == '=')
                current_p++;
            int num = 0;
            while (is_digit_char(read_char(current_p)))
            {
                num = (num * 10) + (read_char(current_p++) - '0');
            }
            if (num > 0)
                default_dur = num;
        }
        else if (c == 'o')
        {
            current_p++;
            if (read_char(current_p) == '=')
                current_p++;
            int num = 0;
            while (is_digit_char(read_char(current_p)))
            {
                num = (num * 10) + (read_char(current_p++) - '0');
            }
            if (num > 0)
                default_oct = num;
        }
        else if (c == 'b')
        {
            current_p++;
            if (read_char(current_p) == '=')
                current_p++;
            int num = 0;
            while (is_digit_char(read_char(current_p)))
            {
                num = (num * 10) + (read_char(current_p++) - '0');
            }
            if (num > 0)
                bpm = num;
        }
        else
        {
            current_p++;
        }

        while (read_char(current_p) == ' ')
            current_p++;

        if (read_char(current_p) == ',')
            current_p++;
    }

    if (read_char(current_p) == ':')
        current_p++;

    if (bpm == 0)
        bpm = 63;

    beatEvery = 60000UL / (uint32_t)bpm;
}

void start_song(const char *p)
{
    play_internal(p, true);
}

void play_once(const char *p)
{
    play_internal(p, false);
}

void play_sfx(const char *p)
{
    play_internal(p, false);
}

void stop_song(void)
{
    is_playing = false;
    is_note_sounding = false;
    current_p = NULL;
    original_song_ptr = NULL;
    noTone(tonePin_Arduino);
}

bool is_music_playing(void)
{
    return is_playing;
}

void update_music(void)
{
    if (!is_playing || !current_p)
        return;

    uint32_t current_time = millis();

    if (current_time - note_start_time >= current_phase_duration)
    {
        // ---------------------------------------------------------
        // STATE 1: Just finished sounding a note. Silence & wait staccato gap.
        // ---------------------------------------------------------
        if (is_note_sounding)
        {
            noTone(tonePin_Arduino);
            is_note_sounding = false;
            current_phase_duration = next_pause_duration;
            note_start_time = current_time;
            return;
        }

        // ---------------------------------------------------------
        // STATE 2: Finished pause gap or rest. Parse next note.
        // ---------------------------------------------------------

        // Skip leading spaces or commas
        while (read_char(current_p) == ' ' || read_char(current_p) == ',')
            current_p++;

        // End of song reached
        if (read_char(current_p) == '\0')
        {
            if (loop_song && original_song_ptr)
            {
                play_internal(original_song_ptr, true);
            }
            else
            {
                stop_song();
            }
            return;
        }

        // Parse Duration
        int num = 0;
        while (is_digit_char(read_char(current_p)))
        {
            num = (num * 10) + (read_char(current_p++) - '0');
        }
        int noteDuration = (num == 0) ? default_dur : num;
        if (noteDuration == 0)
            noteDuration = 4;

        // Parse Note Letter
        char noteChar = read_char(current_p);
        if (noteChar >= 'A' && noteChar <= 'Z')
            noteChar += 32; // Force lowercase
        if (noteChar == 'h')
            noteChar = 'b'; // Bluejay standard: maps 'h' to 'b'
        if (noteChar != '\0')
            current_p++;

        // Parse Sharps
        bool sharp = false;
        if (read_char(current_p) == '#')
        {
            sharp = true;
            current_p++;
        }

        // Parse Dots
        int dots = 0;
        while (read_char(current_p) == '.')
        {
            dots++;
            current_p++;
        }

        // Parse Octave
        int octave = default_oct;
        if (is_digit_char(read_char(current_p)))
        {
            octave = read_char(current_p++) - '0';
        }

        // Dots after octave
        while (read_char(current_p) == '.')
        {
            dots++;
            current_p++;
        }

        // Skip trailing spaces or commas
        while (read_char(current_p) == ' ' || read_char(current_p) == ',')
            current_p++;

        // Calculate note and gap durations
        uint32_t durationMs = (beatEvery * 4UL) / (uint32_t)noteDuration;
        if (dots == 1)
            durationMs = (durationMs * 3UL) / 2UL;
        else if (dots == 2)
            durationMs = (durationMs * 7UL) / 4UL;
        else if (dots == 3)
            durationMs = (durationMs * 15UL) / 8UL;
        else if (dots >= 4)
            durationMs = (durationMs * 31UL) / 16UL;

        if (noteChar == 'p')
        {
            // Rest / Pause
            noTone(tonePin_Arduino);
            is_note_sounding = false;
            current_phase_duration = durationMs;
            note_start_time = current_time;
        }
        else if (noteChar >= 'a' && noteChar <= 'g')
        {
            int noteIndex = 0;
            switch (noteChar)
            {
            case 'c':
                noteIndex = 0;
                break;
            case 'd':
                noteIndex = 2;
                break;
            case 'e':
                noteIndex = 4;
                break;
            case 'f':
                noteIndex = 5;
                break;
            case 'g':
                noteIndex = 7;
                break;
            case 'a':
                noteIndex = 9;
                break;
            case 'b':
                noteIndex = 11;
                break;
            default:
                noteIndex = 0;
                break;
            }

            if (sharp)
                noteIndex++;

            if (noteIndex >= 12)
            {
                noteIndex -= 12;
                octave++;
            }

            if (octave < 3)
                octave = 3;
            if (octave > 7)
                octave = 7;

            uint8_t tableIndex = ((octave - 3) * 12) + noteIndex;
            uint16_t noteFreq = pgm_read_word(&note_pitches[tableIndex]);

            // Staccato timing: ~77% sound duration, ~23% pause
            uint32_t toneDuration = (durationMs * 10UL) / 13UL;
            next_pause_duration = durationMs - toneDuration;

            tone(tonePin_Arduino, noteFreq, 0);

            is_note_sounding = true;
            current_phase_duration = toneDuration;
            note_start_time = current_time;
        }
        else
        {
            // Unrecognized character: skip safely
            is_note_sounding = false;
            current_phase_duration = 0;
            note_start_time = current_time;
        }
    }
}

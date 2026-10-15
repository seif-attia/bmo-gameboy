#include "controller.h"

static uint8_t previous_states = 0b00000000;

void controller_init(void)
{
    // Sets pins DDR direction (0 as input)
    CLR_BIT(CONTROLLER_DDR, RIGHT);
    CLR_BIT(CONTROLLER_DDR, LEFT);
    CLR_BIT(CONTROLLER_DDR, UP);
    CLR_BIT(CONTROLLER_DDR, DOWN);
    CLR_BIT(CONTROLLER_DDR, START);
    CLR_BIT(CONTROLLER_DDR, BACK);

    // Set the Pins as HIGH to enable the internal Pull-ups
    SET_BIT(CONTROLLER_PORT, RIGHT);
    SET_BIT(CONTROLLER_PORT, LEFT);
    SET_BIT(CONTROLLER_PORT, UP);
    SET_BIT(CONTROLLER_PORT, DOWN);
    SET_BIT(CONTROLLER_PORT, START);
    SET_BIT(CONTROLLER_PORT, BACK);
}

uint8_t get_key(uint8_t key)
{
    // Active-LOW pin reads 0 when pressed, invert so 1 = pressed
    uint8_t is_pressed = !GET_BIT(CONTROLLER_PIN, key);
    uint8_t was_pressed = GET_BIT(previous_states, key);

    // Initial press detected (1 now, 0 before)
    if (is_pressed && !was_pressed)
    {
        SET_BIT(previous_states, key); // Mark as pressed
        return 1;
    }

    // Button released (0 now)
    if (!is_pressed)
    {
        CLR_BIT(previous_states, key); // Reset state for next press
    }

    return 0; // Returns 0 if holding continuously or if not pressed
}
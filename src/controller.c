#include "controller.h"
#include "timer.h"
#include "IRDriver.h"
#include <util/delay.h>

static uint8_t previous_states = 0b00000000;
static uint32_t current_ir_signal = 0;
static uint32_t last_poll_ms = 0;

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

    // Initialize IR receiver pin
    ir_driver_init(0);
}

uint32_t get_key_signal(void)
{
    static uint32_t last_received_hex = 0;
    static uint8_t button_locked = 0;

    uint32_t current_hex = ir_driver_read_hex();

    // If no signal is received (button released/idle), reset the lock
    if (current_hex == 0)
    {
        button_locked = 0;
        last_received_hex = 0;
        return 0;
    }

    // If the same button is held down, ignore subsequent triggers until released
    if (current_hex == last_received_hex && button_locked)
    {
        return 0;
    }

    // New button press detected
    last_received_hex = current_hex;
    button_locked = 1; // Lock until the driver returns 0 (released)

    // Check which button code was received and return its identifier
    switch (current_hex)
    {
        case UP_BUTTON:
            return UP_BUTTON;
        case DOWN_BUTTON:
            return DOWN_BUTTON;
        case LEFT_BUTTON:
            return LEFT_BUTTON;
        case RIGHT_BUTTON:
            return RIGHT_BUTTON;
        case RETURN_BUTTON:
            return RETURN_BUTTON;
        case START_BUTTON:
            return START_BUTTON;
        default:
            button_locked = 0; // Unknown code, unlock
            return 0;          // No matching button
    }
}

static void poll_inputs_if_needed(void)
{
    uint32_t now = millis();
    if (now != last_poll_ms)
    {
        last_poll_ms = now;
        current_ir_signal = get_key_signal();
    }
}

uint8_t get_key(uint8_t key)
{
    poll_inputs_if_needed();

    // Active-LOW pin reads 0 when pressed, invert so 1 = pressed
    uint8_t is_pressed = !GET_BIT(CONTROLLER_PIN, key);

    // Also check corresponding IR remote button
    switch (key)
    {
        case UP:
            if (current_ir_signal == UP_BUTTON) is_pressed = 1;
            break;
        case DOWN:
            if (current_ir_signal == DOWN_BUTTON) is_pressed = 1;
            break;
        case LEFT:
            if (current_ir_signal == LEFT_BUTTON) is_pressed = 1;
            break;
        case RIGHT:
            if (current_ir_signal == RIGHT_BUTTON) is_pressed = 1;
            break;
        case START:
            if (current_ir_signal == START_BUTTON) is_pressed = 1;
            break;
        case BACK:
            if (current_ir_signal == RETURN_BUTTON) is_pressed = 1;
            break;
    }

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
#pragma once

#include <avr/io.h>
#include <stddef.h>

/**
 * @brief Initilaizes all the TIMER0 Registers and sets up global interrupts
 *
 * @note Add it before the while(1) loop and before calling any timer function
 */
void timer_init();

/**
 * @return uint32_t Returns time since system start in ms
 */
uint32_t millis();

/**
 * @brief Schedules a one-shot (non-repeating) callback without arguments.
 *
 * Executes the specified callback function once after the delay time has elapsed.
 *
 * @param delay_ms Delay time in milliseconds before execution.
 * @param cb Pointer to the callback function taking no arguments.
 *
 * @return int8_t Assigned timer slot index (0 to MAX_TIMERS - 1), or -1 if all slots are full.
 *
 * @note Pass the returned slot ID to timer_cancel() to stop execution early.
 * @note Use timer_set_timeout_args() if you need to pass data into the callback.
 */
int8_t timer_set_timeout(uint32_t delay_ms, void (*cb)(void));

/**
 * @brief Schedules a one-shot (non-repeating) callback with a custom argument.
 *
 * Executes the specified callback function once after the delay time has elapsed,
 * passing the provided pointer or value into the callback.
 *
 * @param delay_ms Delay time in milliseconds before execution.
 * @param cb Pointer to the callback function accepting a void pointer argument.
 * @param arg Pointer or cast value passed directly into the callback parameter.
 *
 * @return int8_t Assigned timer slot index (0 to MAX_TIMERS - 1), or -1 if all slots are full.
 *
 * @note Cast raw values using (void *) when passing, then cast back inside the callback.
 * @note Ensure heap/stack pointers remain valid in memory until the callback executes.
 */
int8_t timer_set_timeout_arg(uint32_t delay_ms, void (*cb)(void *arg), void *arg);

/**
 * @brief Schedules a recurring (repeating) callback without arguments.
 *
 * Repeatedly executes the specified callback function at a fixed millisecond interval
 * until canceled.
 *
 * @param delay_ms Interval period in milliseconds between executions.
 * @param cb Pointer to the callback function taking no arguments.
 *
 * @return int8_t Assigned timer slot index (0 to MAX_TIMERS - 1), or -1 if all slots are full.
 *
 * @note Pass the returned slot ID to timer_cancel() to stop recurring execution.
 * @note Use timer_set_interval_arg() if you need to pass data into the callback.
 */
int8_t timer_set_interval(uint32_t delay_ms, void (*cb)(void));

/**
 * @brief Schedules a recurring (repeating) callback with a custom argument.
 *
 * @details Repeatedly executes the specified callback function at a fixed millisecond interval,
 * passing the provided pointer or value into the callback on each cycle.
 *
 * @param delay_ms Interval period in milliseconds between executions.
 * @param cb Pointer to the callback function accepting a void pointer argument.
 * @param arg Pointer or cast value passed directly into the callback parameter.
 *
 * @return int8_t Assigned timer slot index (0 to MAX_TIMERS - 1), or -1 if all slots are full.
 *
 * @note Cast raw values using (void *) when passing, then cast back inside the callback.
 * @note Pass the returned slot ID to timer_cancel() to stop recurring execution.
 */
int8_t timer_set_interval_arg(uint32_t delay_ms, void (*cb)(void *arg), void *arg);

/**
 * @brief Processes and dispatches all active timer tasks.
 *
 * Compares the current system time against the target expiration time for each
 * registered task. Executes the callback function for expired tasks, updates
 * target times for repeating intervals, and deactivates completed one-shot timers.
 *
 * @note MUST be called repeatedly inside your main program loop (while(1)).
 * @note Callbacks are executed in the main execution thread context (non-blocking ISR).
 */
void timer_update();

/**
 * @brief Pass the timer_id parameter to cancel the set timer
 *
 * @param timer_id
 */
void timer_cancel(uint8_t timer_id);

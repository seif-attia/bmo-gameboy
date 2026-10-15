#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>
#include "timer.h"

// Change to the max desired timer number
#define MAX_TIMERS 4

typedef struct
{
    uint32_t targetTime;
    uint32_t interval;
    union
    {
        void (*simple)(void);
        void (*with_arg)(void *arg);
    } cb;
    void *arg;
    uint8_t has_arg; // 0 = simple, 1 = with arg
    uint8_t repeat;
    uint8_t active;

} TimerTask;

static volatile uint32_t system_millis = 0;
static TimerTask timerTasks[MAX_TIMERS];

/*
    Interrupt for TIMER0 Output Compare Match A

    Which fires whenever the timer0's counter value (TCNT0) equals the value of the output compare register A (OCR0A)

*/
ISR(TIMER0_COMPA_vect)
{
    system_millis++;
}

void timer_init()
{
    // Set CTC mode (Clear timer on compare)
    TCCR0A = (1 << WGM01);

    // Set prescaler to 64 -> 16Mhz / 64 = 250kHz
    TCCR0B = (1 << CS01) | (1 << CS00);

    // Set compare match value to 1ms: (250kHz * 0.01s) - 1 = 249
    OCR0A = 249;

    // Enable Output Compare Match A Interrupt
    TIMSK0 = (1 << OCIE0A);

    // Enable global interrupts
    sei();
}

uint32_t millis()
{
    uint32_t ms;
    uint8_t sreg = SREG; // Save interrupt state
    cli();               // Disable Interrupts to prevent read corruptions while outputting time
    ms = system_millis;
    SREG = sreg; // Load interrupt state back
    return ms;
}

uint32_t micros()
{
    uint32_t ms;
    uint8_t t;
    uint8_t sreg = SREG;
    cli();
    ms = system_millis;
    t = TCNT0;
    if ((TIFR0 & (1 << OCF0A)) && (t < 249))
    {
        ms++;
    }
    SREG = sreg;
    return (ms * 1000UL) + ((uint32_t)t * 4UL);
}

int8_t timer_set_timeout(uint32_t delay_ms, void (*cb)(void))
{
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (!timerTasks[i].active)
        {
            timerTasks[i].targetTime = millis() + delay_ms;
            timerTasks[i].interval = delay_ms;
            timerTasks[i].cb.simple = cb;
            timerTasks[i].arg = NULL;
            timerTasks[i].has_arg = 0;
            timerTasks[i].repeat = 0;
            timerTasks[i].active = 1;
            return i;
        }
    }

    return -1;
}

int8_t timer_set_timeout_arg(uint32_t delay_ms, void (*cb)(void *arg), void *arg)
{
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (!timerTasks[i].active)
        {
            timerTasks[i].targetTime = millis() + delay_ms;
            timerTasks[i].interval = delay_ms;
            timerTasks[i].cb.with_arg = cb;
            timerTasks[i].arg = arg;
            timerTasks[i].has_arg = 1;
            timerTasks[i].repeat = 0;
            timerTasks[i].active = 1;
            return i;
        }
    }

    return -1;
}

int8_t timer_set_interval(uint32_t delay_ms, void (*cb)(void))
{
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (!timerTasks[i].active)
        {
            timerTasks[i].targetTime = millis() + delay_ms;
            timerTasks[i].interval = delay_ms;
            timerTasks[i].cb.simple = cb;
            timerTasks[i].arg = NULL;
            timerTasks[i].has_arg = 0;
            timerTasks[i].repeat = 1;
            timerTasks[i].active = 1;
            return i;
        }
    }

    return -1;
}

int8_t timer_set_interval_arg(uint32_t delay_ms, void (*cb)(void *arg), void *arg)
{
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (!timerTasks[i].active)
        {
            timerTasks[i].targetTime = millis() + delay_ms;
            timerTasks[i].interval = delay_ms;
            timerTasks[i].cb.with_arg = cb;
            timerTasks[i].arg = arg;
            timerTasks[i].has_arg = 1;
            timerTasks[i].repeat = 1;
            timerTasks[i].active = 1;
            return i;
        }
    }

    return -1;
}

void timer_cancel(uint8_t timer_id)
{
    if (timer_id < MAX_TIMERS)
        timerTasks[timer_id].active = 0;
}

void timer_update()
{
    uint32_t current_ms = millis();

    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (timerTasks[i].active && (current_ms >= timerTasks[i].targetTime))
        {
            if (timerTasks[i].has_arg)
            {
                if (timerTasks[i].cb.with_arg != NULL)
                    timerTasks[i].cb.with_arg(timerTasks[i].arg);
            }
            else
            {
                if (timerTasks[i].cb.simple != NULL)
                    timerTasks[i].cb.simple();
            }

            if (timerTasks[i].repeat)
                timerTasks[i].targetTime = current_ms + timerTasks[i].interval;
            else
                timerTasks[i].active = 0;
        }
    }
}

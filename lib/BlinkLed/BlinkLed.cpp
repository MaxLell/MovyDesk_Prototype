#include "BlinkLed.h"
#include <Arduino.h>

// ###########################################################################
// # Module state
// ###########################################################################
static struct
{
    u8 pin;
    bool led_state;
    bool initialized;
} s_ctx = {0};

// ###########################################################################
// # Public API
// ###########################################################################
void blinkled_init(u8 pin)
{
    s_ctx.pin = pin;
    s_ctx.led_state = false;
    s_ctx.initialized = true;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void blinkled_enable(void)
{
    if (!s_ctx.initialized)
    {
        return;
    }

    s_ctx.led_state = true;
    digitalWrite(s_ctx.pin, HIGH);
}

void blinkled_disable(void)
{
    if (!s_ctx.initialized)
    {
        return;
    }

    s_ctx.led_state = false;
    digitalWrite(s_ctx.pin, LOW);
}

void blinkled_toggle(void)
{
    if (!s_ctx.initialized)
    {
        return;
    }

    s_ctx.led_state = !s_ctx.led_state;
    digitalWrite(s_ctx.pin, s_ctx.led_state ? HIGH : LOW);
}

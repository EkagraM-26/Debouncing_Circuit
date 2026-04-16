/**
 * button_debounce.ino — Arduino example using all three debounce algorithms.
 *
 * Wiring:
 *   Pin 2 → one leg of tactile switch → GND
 *   (Internal pull-up enabled, so idle = HIGH, pressed = LOW)
 *
 * Open Serial Monitor at 115200 baud to see output.
 *
 * Copy debounce.h and debounce.c into this sketch's folder,
 * or place them in Arduino/libraries/debounce/src/
 */

#include "debounce.h"

#define BUTTON_PIN 2

/* One instance of each algorithm — pick whichever suits your project */
TimerDebounce        timer_btn;
IntegratorDebounce   integ_btn;
StateMachineDebounce sm_btn;

/* Press counters to compare algorithms */
uint32_t timer_count = 0;
uint32_t integ_count = 0;
uint32_t sm_count    = 0;

void setup()
{
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    /* Read current pin state so algorithms start correct */
    PinState initial = (digitalRead(BUTTON_PIN) == LOW) ? PIN_LOW : PIN_HIGH;
    uint32_t now = millis();

    timer_debounce_init(&timer_btn, initial, now);
    integrator_debounce_init(&integ_btn, initial);
    sm_debounce_init(&sm_btn, now);

    Serial.println("=== Debounce Demo ===");
    Serial.println("Press the button and watch the Serial Monitor.");
    Serial.println("All three algorithms run in parallel.");
    Serial.println("---------------------");
}

void loop()
{
    uint32_t now = millis();
    PinState raw  = (digitalRead(BUTTON_PIN) == LOW) ? PIN_LOW : PIN_HIGH;

    /* ── Update all three algorithms ── */
    timer_debounce_update(&timer_btn, raw, now);
    integrator_debounce_update(&integ_btn, raw);
    sm_debounce_update(&sm_btn, raw, now);

    /* ── Algorithm 1: Timer-based ── */
    if (timer_debounce_event(&timer_btn)) {
        PinState s = timer_debounce_state(&timer_btn);
        if (s == PIN_LOW) {          /* Active-low: LOW means pressed */
            timer_count++;
            Serial.print("[TIMER]  PRESS   count=");
            Serial.println(timer_count);
        } else {
            Serial.println("[TIMER]  RELEASE");
        }
    }

    /* ── Algorithm 2: Integrator ── */
    if (integrator_debounce_event(&integ_btn)) {
        PinState s = integrator_debounce_state(&integ_btn);
        if (s == PIN_LOW) {
            integ_count++;
            Serial.print("[INTEG]  PRESS   count=");
            Serial.println(integ_count);
        } else {
            Serial.println("[INTEG]  RELEASE");
        }
    }

    /* ── Algorithm 3: State machine (also detects long press) ── */
    ButtonEvent ev = sm_debounce_get_event(&sm_btn);
    switch (ev) {
        case BTN_EVENT_PRESS:
            sm_count++;
            Serial.print("[SM]     PRESS   count=");
            Serial.println(sm_count);
            break;
        case BTN_EVENT_HOLD:
            Serial.println("[SM]     HELD (long press!)");
            break;
        case BTN_EVENT_RELEASE:
            Serial.println("[SM]     RELEASE");
            break;
        default:
            break;
    }

    /* Poll every 1 ms — faster than bounce duration (~0.3 ms spikes) */
    delay(1);
}

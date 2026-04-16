/**
 * debounce.c — Implementation of all three debounce algorithms.
 * See debounce.h for full documentation.
 */

#include "debounce.h"

/* ══════════════════════════════════════════════════════════════
 * ALGORITHM 1 — Timer-based
 * ══════════════════════════════════════════════════════════════ */

void timer_debounce_init(TimerDebounce *db, PinState initial, uint32_t now_ms)
{
    db->stable_state    = initial;
    db->raw_state       = initial;
    db->last_change_ms  = now_ms;
    db->event_pending   = false;
}

void timer_debounce_update(TimerDebounce *db, PinState raw, uint32_t now_ms)
{
    /* New transition detected */
    if (raw != db->raw_state) {
        db->raw_state      = raw;
        db->last_change_ms = now_ms;
        return;   /* Start the lockout window */
    }

    /* Raw has been stable — has the window elapsed? */
    uint32_t elapsed = now_ms - db->last_change_ms;
    if (elapsed >= DEBOUNCE_TIMER_MS) {
        if (raw != db->stable_state) {
            db->stable_state  = raw;
            db->event_pending = true;
        }
    }
}

bool timer_debounce_event(TimerDebounce *db)
{
    if (db->event_pending) {
        db->event_pending = false;
        return true;
    }
    return false;
}

PinState timer_debounce_state(const TimerDebounce *db)
{
    return db->stable_state;
}


/* ══════════════════════════════════════════════════════════════
 * ALGORITHM 2 — Integrator
 * ══════════════════════════════════════════════════════════════ */

void integrator_debounce_init(IntegratorDebounce *db, PinState initial)
{
    db->stable_state  = initial;
    db->integrator    = (initial == PIN_HIGH) ? DEBOUNCE_INTEGRATOR_MAX : 0;
    db->event_pending = false;
}

void integrator_debounce_update(IntegratorDebounce *db, PinState raw)
{
    if (raw == PIN_HIGH) {
        if (db->integrator < DEBOUNCE_INTEGRATOR_MAX)
            db->integrator++;
    } else {
        if (db->integrator > 0)
            db->integrator--;
    }

    if (db->integrator == DEBOUNCE_INTEGRATOR_MAX && db->stable_state == PIN_LOW) {
        db->stable_state  = PIN_HIGH;
        db->event_pending = true;
    } else if (db->integrator == 0 && db->stable_state == PIN_HIGH) {
        db->stable_state  = PIN_LOW;
        db->event_pending = true;
    }
}

bool integrator_debounce_event(IntegratorDebounce *db)
{
    if (db->event_pending) {
        db->event_pending = false;
        return true;
    }
    return false;
}

PinState integrator_debounce_state(const IntegratorDebounce *db)
{
    return db->stable_state;
}


/* ══════════════════════════════════════════════════════════════
 * ALGORITHM 3 — State machine
 * ══════════════════════════════════════════════════════════════ */

void sm_debounce_init(StateMachineDebounce *db, uint32_t now_ms)
{
    db->state           = BTN_IDLE;
    db->last_event      = BTN_EVENT_NONE;
    db->press_time_ms   = 0;
    db->debounce_time_ms = now_ms;
    db->raw_prev        = PIN_HIGH;   /* Assume pull-up: idle = HIGH */
}

void sm_debounce_update(StateMachineDebounce *db, PinState raw, uint32_t now_ms)
{
    db->last_event = BTN_EVENT_NONE;

    switch (db->state) {

    case BTN_IDLE:
        /* Active-low button: press pulls pin LOW */
        if (raw == PIN_LOW) {
            db->debounce_time_ms = now_ms;
            db->state = BTN_PRESSED;
        }
        break;

    case BTN_PRESSED:
        if (raw == PIN_HIGH) {
            /* Glitch: went back HIGH before window closed */
            db->state = BTN_IDLE;
        } else if ((now_ms - db->debounce_time_ms) >= DEBOUNCE_TIMER_MS) {
            /* Confirmed press */
            db->press_time_ms = now_ms;
            db->last_event    = BTN_EVENT_PRESS;
            db->state         = BTN_HELD;
        }
        break;

    case BTN_HELD:
        if (raw == PIN_HIGH) {
            /* Released */
            db->debounce_time_ms = now_ms;
            db->state = BTN_RELEASED;
        } else if ((now_ms - db->press_time_ms) >= DEBOUNCE_HELD_MS) {
            /* Long press */
            db->last_event    = BTN_EVENT_HOLD;
            db->press_time_ms = now_ms;   /* Reset so HOLD fires once */
        }
        break;

    case BTN_RELEASED:
        if (raw == PIN_LOW) {
            /* Bounce after release — restart */
            db->debounce_time_ms = now_ms;
            db->state = BTN_PRESSED;
        } else if ((now_ms - db->debounce_time_ms) >= DEBOUNCE_TIMER_MS) {
            db->last_event = BTN_EVENT_RELEASE;
            db->state      = BTN_IDLE;
        }
        break;
    }

    db->raw_prev = raw;
}

ButtonEvent sm_debounce_get_event(StateMachineDebounce *db)
{
    ButtonEvent e = db->last_event;
    db->last_event = BTN_EVENT_NONE;
    return e;
}

ButtonState sm_debounce_get_state(const StateMachineDebounce *db)
{
    return db->state;
}

/**
 * debounce.h — Portable software debounce library
 *
 * Supports three algorithms:
 *   1. Timer-based  — ignore all transitions for N ms after first edge
 *   2. Integrator   — increment/decrement counter; confirm only when saturated
 *   3. State machine— tracks IDLE → PRESSED → HELD → RELEASED states
 *
 * Platform-agnostic: supply your own get_time_ms() function.
 * Works on Arduino, ESP32, STM32, bare-metal, or desktop (for testing).
 *
 * Author : <your-name>
 * License: MIT
 */

#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>
#include <stdbool.h>

/* ── Configuration ─────────────────────────────────────────── */
#define DEBOUNCE_TIMER_MS       20U   /* Timer-based: lockout window in ms     */
#define DEBOUNCE_INTEGRATOR_MAX 10U   /* Integrator: samples needed to confirm */
#define DEBOUNCE_HELD_MS        500U  /* State-machine: ms before "held" fires  */


/* ── Raw pin state ──────────────────────────────────────────── */
typedef enum {
    PIN_LOW  = 0,
    PIN_HIGH = 1
} PinState;


/* ══════════════════════════════════════════════════════════════
 * ALGORITHM 1 — Timer-based debounce
 *
 * How it works:
 *   On the FIRST transition, latch the new state and record the timestamp.
 *   Ignore ALL further changes until DEBOUNCE_TIMER_MS has elapsed.
 *   After the window, the button is re-armed.
 * ══════════════════════════════════════════════════════════════ */
typedef struct {
    PinState stable_state;       /* Last confirmed stable level       */
    PinState raw_state;          /* Most-recent raw reading           */
    uint32_t last_change_ms;     /* Timestamp of last edge            */
    bool     event_pending;      /* True if a new event is ready      */
} TimerDebounce;

/**
 * Initialise a TimerDebounce instance.
 * @param db          Pointer to instance
 * @param initial     Initial pin state (read before calling this)
 * @param now_ms      Current time in milliseconds
 */
void timer_debounce_init(TimerDebounce *db, PinState initial, uint32_t now_ms);

/**
 * Feed a new raw sample.  Call this from your polling loop or ISR.
 * @param db      Pointer to instance
 * @param raw     Current raw pin reading
 * @param now_ms  Current time in milliseconds
 */
void timer_debounce_update(TimerDebounce *db, PinState raw, uint32_t now_ms);

/**
 * Check if a new stable event is available and consume it.
 * @return true once per confirmed transition, false otherwise
 */
bool timer_debounce_event(TimerDebounce *db);

/** Return the current stable (debounced) state. */
PinState timer_debounce_state(const TimerDebounce *db);


/* ══════════════════════════════════════════════════════════════
 * ALGORITHM 2 — Integrator debounce
 *
 * How it works:
 *   A counter starts at DEBOUNCE_INTEGRATOR_MAX/2.
 *   Every sample: raw HIGH → counter++, raw LOW → counter--.
 *   Counter clamps at [0, MAX].
 *   Output flips to HIGH only when counter hits MAX, LOW only when it hits 0.
 *   Requires DEBOUNCE_INTEGRATOR_MAX consecutive same-level samples to confirm.
 * ══════════════════════════════════════════════════════════════ */
typedef struct {
    PinState stable_state;
    uint8_t  integrator;
    bool     event_pending;
} IntegratorDebounce;

void     integrator_debounce_init(IntegratorDebounce *db, PinState initial);
void     integrator_debounce_update(IntegratorDebounce *db, PinState raw);
bool     integrator_debounce_event(IntegratorDebounce *db);
PinState integrator_debounce_state(const IntegratorDebounce *db);


/* ══════════════════════════════════════════════════════════════
 * ALGORITHM 3 — State-machine debounce
 *
 * How it works:
 *   Tracks four states: IDLE, PRESSED, HELD, RELEASED.
 *   Combines timer-based filtering with long-press detection.
 *   Emits distinct events for press, hold, and release.
 * ══════════════════════════════════════════════════════════════ */
typedef enum {
    BTN_IDLE     = 0,
    BTN_PRESSED  = 1,
    BTN_HELD     = 2,
    BTN_RELEASED = 3
} ButtonState;

typedef enum {
    BTN_EVENT_NONE     = 0,
    BTN_EVENT_PRESS    = 1,   /* Short press confirmed                */
    BTN_EVENT_HOLD     = 2,   /* Button held longer than DEBOUNCE_HELD_MS */
    BTN_EVENT_RELEASE  = 3    /* Button released                      */
} ButtonEvent;

typedef struct {
    ButtonState state;
    ButtonEvent last_event;
    uint32_t    press_time_ms;
    uint32_t    debounce_time_ms;
    PinState    raw_prev;
} StateMachineDebounce;

void        sm_debounce_init(StateMachineDebounce *db, uint32_t now_ms);
void        sm_debounce_update(StateMachineDebounce *db, PinState raw, uint32_t now_ms);
ButtonEvent sm_debounce_get_event(StateMachineDebounce *db);
ButtonState sm_debounce_get_state(const StateMachineDebounce *db);


#endif /* DEBOUNCE_H */

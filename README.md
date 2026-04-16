# Switch Debounce Library

A portable, well-documented C library implementing **three debounce algorithms** for mechanical switch inputs in embedded systems. Includes Arduino/ESP32 examples and a Python simulation that generates comparison waveforms.

![Debounce Comparison](simulation/debounce_comparison.png)

---

## The Problem

When a mechanical switch closes, its contacts physically bounce against each other for 1–10 ms. A microcontroller running at MHz speeds sees this as dozens of rapid HIGH/LOW transitions — i.e., many false button presses from a single physical click.

```
Raw signal seen by MCU:
       ____  _ _  __________________________
  ____|    || | ||
  (idle)  (bounce!) (settled)
```

This library cleans that up entirely in software.

---

## Project Structure

```
debounce-lib/
├── src/
│   ├── debounce.h          ← Header: all types, constants, function prototypes
│   └── debounce.c          ← Implementation of all three algorithms
├── examples/
│   ├── arduino/
│   │   └── button_debounce.ino      ← Arduino Uno/Nano example
│   └── esp32/
│       └── button_debounce_esp32.ino ← ESP32 + FreeRTOS example
├── simulation/
│   └── simulate_debounce.py         ← Python waveform generator & visualiser
└── README.md
```

---

## Algorithms

### 1. Timer-Based Debounce
**File:** `src/debounce.c` → `timer_debounce_*`

When the first edge is detected, a lockout window starts (`DEBOUNCE_TIMER_MS`, default 20 ms). Any transitions during the window are ignored. After the window, if the signal is still at the new level, it is confirmed as a valid transition.

- ✅ Simple and low overhead
- ✅ Deterministic latency
- ⚠️ Misses a press if it's shorter than the window

```c
TimerDebounce btn;
timer_debounce_init(&btn, PIN_HIGH, millis());

// In your loop (call every 1ms):
timer_debounce_update(&btn, read_pin(), millis());
if (timer_debounce_event(&btn)) {
    // confirmed transition!
}
```

---

### 2. Integrator Debounce
**File:** `src/debounce.c` → `integrator_debounce_*`

A saturating counter is incremented on every HIGH sample and decremented on every LOW sample. The output only changes state when the counter hits its maximum (all HIGH) or zero (all LOW). Requires `DEBOUNCE_INTEGRATOR_MAX` consecutive same-level samples to confirm a transition.

- ✅ No time dependency — works on any polling rate
- ✅ Naturally filters short spikes
- ✅ Used in Jack Ganssle's famous debounce guide

```c
IntegratorDebounce btn;
integrator_debounce_init(&btn, PIN_HIGH);

// In your loop (call at fixed interval):
integrator_debounce_update(&btn, read_pin());
if (integrator_debounce_event(&btn)) {
    // confirmed transition!
}
```

---

### 3. State Machine Debounce
**File:** `src/debounce.c` → `sm_debounce_*`

Tracks four states: `IDLE → PRESSED → HELD → RELEASED`. Combines timer-based filtering with long-press detection. Emits distinct events (`BTN_EVENT_PRESS`, `BTN_EVENT_HOLD`, `BTN_EVENT_RELEASE`).

- ✅ Detects short press AND long press
- ✅ Production-quality pattern (used in real firmware)
- ✅ Easiest to integrate with UI/UX logic

```c
StateMachineDebounce btn;
sm_debounce_init(&btn, millis());

// In your loop (call every 1ms):
sm_debounce_update(&btn, read_pin(), millis());
switch (sm_debounce_get_event(&btn)) {
    case BTN_EVENT_PRESS:   /* single tap */   break;
    case BTN_EVENT_HOLD:    /* long press  */  break;
    case BTN_EVENT_RELEASE: /* finger lifted */ break;
}
```

---

## Simulation (Python)

Run the simulation to visualise all three algorithms on a synthetic bouncing signal:

```bash
pip install matplotlib numpy
cd simulation
python simulate_debounce.py
```

This generates `debounce_comparison.png` — a 4-panel plot showing:
- Raw bouncing signal
- Timer-based output
- Integrator output
- State-machine output with annotated PRESS/HOLD/RELEASE events

**Example output:**
```
Raw: 5 apparent presses | Timer: 1 | Integrator: 1 | State machine: 1
```

---

## Getting Started on Arduino

1. Copy `src/debounce.h` and `src/debounce.c` into your sketch folder.
2. Open `examples/arduino/button_debounce.ino`.
3. Wire a tactile switch between **Pin 2** and **GND**.
4. Upload and open Serial Monitor at **115200 baud**.
5. Press the button — you should see exactly **one** `PRESS` per physical press.

---

## Getting Started on ESP32

1. Copy the `src/` files alongside the sketch.
2. Open `examples/esp32/button_debounce_esp32.ino` in Arduino IDE (with ESP32 board package installed).
3. Wire a switch between **GPIO 15** and **GND**.
4. Upload and open Serial Monitor at **115200 baud**.

The ESP32 example uses a **FreeRTOS task** for the button polling and a **queue** to deliver events to the main loop — a production-grade pattern.

---

## Configuration

All tuning constants are in `src/debounce.h`:

| Constant | Default | Description |
|---|---|---|
| `DEBOUNCE_TIMER_MS` | 20 | Lockout window for timer algorithm (ms) |
| `DEBOUNCE_INTEGRATOR_MAX` | 10 | Samples needed for integrator to confirm |
| `DEBOUNCE_HELD_MS` | 500 | Long-press threshold for state machine (ms) |

---

## Porting to Other Platforms

The library has **no platform dependencies**. The only requirement is a `uint32_t` millisecond counter. Replace `millis()` in the examples with your platform's equivalent:

| Platform | Time function |
|---|---|
| Arduino / ESP32 | `millis()` |
| STM32 HAL | `HAL_GetTick()` |
| FreeRTOS | `xTaskGetTickCount() * portTICK_PERIOD_MS` |
| Linux (test) | `clock_gettime(CLOCK_MONOTONIC)` |

---

## Tools Used

| Tool | Purpose |
|---|---|
| [Wokwi](https://wokwi.com) | Online Arduino/ESP32 simulator — test without hardware |
| [Arduino IDE](https://www.arduino.cc/en/software) | Upload to physical boards |
| [PlatformIO](https://platformio.org) | VS Code extension for professional embedded workflow |
| Python + Matplotlib | Waveform simulation and visualisation |
| [TinkerCAD Circuits](https://www.tinkercad.com) | Circuit schematic and breadboard layout |

---

## References

- Jack Ganssle, *[A Guide to Debouncing](https://www.ganssle.com/debouncing.htm)* — the definitive resource
- Elliot Williams, *[Debounce your Switches](https://hackaday.com/2015/12/09/embed-with-elliot-debounce-your-noisy-buttons-part-i/)* — Hackaday two-part series
- *The Art of Electronics*, Horowitz & Hill — Chapter 9

---

## License

MIT — free to use in personal and commercial projects.

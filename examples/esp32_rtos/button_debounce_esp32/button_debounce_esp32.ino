/**
 * button_debounce_esp32.ino — ESP32 debounce with FreeRTOS & queue.
 *
 * Uses the state-machine debounce algorithm.
 * A dedicated task reads the pin; events are sent to the main loop via queue.
 * This pattern is production-quality for real firmware projects.
 *
 * Wiring: GPIO 15 → switch → GND (internal pull-up enabled)
 */

#include "debounce.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define BUTTON_GPIO     15
#define POLL_INTERVAL   1     /* ms */

static QueueHandle_t btn_event_queue;
static StateMachineDebounce sm_btn;

/* ── Button polling task (runs on core 0) ── */
void buttonTask(void *pvParameters)
{
    (void)pvParameters;

    pinMode(BUTTON_GPIO, INPUT_PULLUP);
    sm_debounce_init(&sm_btn, millis());

    for (;;) {
        PinState raw = (digitalRead(BUTTON_GPIO) == LOW) ? PIN_LOW : PIN_HIGH;
        sm_debounce_update(&sm_btn, raw, millis());

        ButtonEvent ev = sm_debounce_get_event(&sm_btn);
        if (ev != BTN_EVENT_NONE) {
            /* Non-blocking send — drop if queue is full */
            xQueueSend(btn_event_queue, &ev, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL));
    }
}

void setup()
{
    Serial.begin(115200);
    btn_event_queue = xQueueCreate(8, sizeof(ButtonEvent));

    /* Pin the button task to core 0, leave core 1 for main loop */
    xTaskCreatePinnedToCore(
        buttonTask, "btn_task",
        2048, NULL, 2,
        NULL, 0
    );

    Serial.println("ESP32 Debounce Demo — FreeRTOS edition");
    Serial.println("Press the button to see events.");
}

void loop()
{
    ButtonEvent ev;

    /* Block up to 100 ms waiting for an event */
    if (xQueueReceive(btn_event_queue, &ev, pdMS_TO_TICKS(100)) == pdTRUE) {
        switch (ev) {
            case BTN_EVENT_PRESS:
                Serial.println("PRESS");
                /* → add your application logic here */
                break;
            case BTN_EVENT_HOLD:
                Serial.println("HELD — long press detected!");
                break;
            case BTN_EVENT_RELEASE:
                Serial.println("RELEASE");
                break;
            default:
                break;
        }
    }
}

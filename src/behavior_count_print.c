/*
 * &count_print — types the current scroll-counter value as decimal digits.
 *
 * iPhone usage: focus a text field (Notes, Messages compose, etc.) before
 * pressing the encoder; the digits land in the focused input.
 *
 * Implementation: convert uint32 → up to 10 ASCII digits, then queue each
 * digit as a key_press behavior via zmk_behavior_queue_add — the same path
 * &macro uses internally. Press + release per digit, with a small gap so
 * the host doesn't coalesce them.
 */

#define DT_DRV_COMPAT zmk_behavior_count_print

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <dt-bindings/zmk/keys.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>

#include "scroll_counter.h"

LOG_MODULE_DECLARE(scroll_counter, CONFIG_ZMK_LOG_LEVEL);

/* HID usage codes for digits 0..9 — N0=0x27, N1..N9=0x1E..0x26 */
static const uint32_t digit_keycodes[10] = {
    N0, N1, N2, N3, N4, N5, N6, N7, N8, N9,
};

#define KEY_PRESS_DEV "key_press"
#define INTER_KEY_MS 12

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);

    uint32_t v = scroll_counter_get();

    /* Render decimal digits, MSB first. uint32_t max = 10 digits. */
    char buf[11];
    int n = 0;
    if (v == 0) {
        buf[n++] = 0;
    } else {
        char tmp[10];
        int t = 0;
        while (v > 0 && t < (int)sizeof(tmp)) {
            tmp[t++] = (char)(v % 10);
            v /= 10;
        }
        while (t > 0) {
            buf[n++] = tmp[--t];
        }
    }

    LOG_INF("printing count, %d digit(s)", n);

    struct zmk_behavior_binding press = {
        .behavior_dev = KEY_PRESS_DEV,
    };
    for (int i = 0; i < n; i++) {
        press.param1 = digit_keycodes[(int)buf[i]];
        zmk_behavior_queue_add(event.position, press, true,  INTER_KEY_MS);
        zmk_behavior_queue_add(event.position, press, false, INTER_KEY_MS);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_count_print_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

static int behavior_count_print_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

#define KP_INST(n)                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_count_print_init, NULL, NULL, NULL,    \
                           POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,   \
                           &behavior_count_print_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

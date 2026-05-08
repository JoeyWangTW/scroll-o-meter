/*
 * &count_reset — zeroes the scroll counter and persists.
 */

#define DT_DRV_COMPAT zmk_behavior_count_reset

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>

#include "scroll_counter.h"

LOG_MODULE_DECLARE(scroll_counter, CONFIG_ZMK_LOG_LEVEL);

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    scroll_counter_reset();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_count_reset_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

static int behavior_count_reset_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

#define KP_INST(n)                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_count_reset_init, NULL, NULL, NULL,    \
                           POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,   \
                           &behavior_count_reset_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

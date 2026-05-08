/*
 * scroll-meter: persisted detent counter.
 *
 * Subscribes to ZMK's sensor_event so we don't have to fork &enc_scroll —
 * the keymap keeps using the standard sensor-rotate behavior, and we just
 * observe each rotation tick and bump a uint32_t.
 *
 * Persistence uses Zephyr's settings subsystem (NVS backend). Writes are
 * batched: every Nth increment + every reset hits flash. With N=25, even
 * a heavy 6,000-detent day is 240 writes, well under typical NVS wear.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <stdint.h>

#include <zmk/event_manager.h>
#include <zmk/events/sensor_event.h>

#include "scroll_counter.h"

LOG_MODULE_REGISTER(scroll_counter, CONFIG_ZMK_LOG_LEVEL);

#define SETTINGS_KEY "scroll_meter/count"

static atomic_t total = ATOMIC_INIT(0);
static atomic_t since_persist = ATOMIC_INIT(0);

static struct k_work persist_work;

uint32_t scroll_counter_get(void) {
    return (uint32_t)atomic_get(&total);
}

static void persist_now(struct k_work *w) {
    ARG_UNUSED(w);
    uint32_t v = (uint32_t)atomic_get(&total);
    int rc = settings_save_one(SETTINGS_KEY, &v, sizeof(v));
    if (rc) {
        LOG_WRN("settings_save_one(%s)=%d", SETTINGS_KEY, rc);
    } else {
        atomic_set(&since_persist, 0);
        LOG_DBG("persisted count=%u", v);
    }
}

void scroll_counter_add(uint32_t delta) {
    if (!delta) return;
    atomic_add(&total, (atomic_val_t)delta);
    atomic_val_t n = atomic_add(&since_persist, (atomic_val_t)delta) + (atomic_val_t)delta;
#if CONFIG_ZMK_SCROLL_COUNTER_PERSIST_EVERY > 0
    if (n >= CONFIG_ZMK_SCROLL_COUNTER_PERSIST_EVERY) {
        k_work_submit(&persist_work);
    }
#endif
}

void scroll_counter_reset(void) {
    atomic_set(&total, 0);
    atomic_set(&since_persist, 0);
    k_work_submit(&persist_work);
    LOG_INF("counter reset");
}

/* settings load callback */
static int counter_set(const char *name, size_t len, settings_read_cb read_cb,
                      void *cb_arg) {
    if (strcmp(name, "count") != 0) {
        return -ENOENT;
    }
    if (len != sizeof(uint32_t)) {
        return -EINVAL;
    }
    uint32_t v = 0;
    int rc = read_cb(cb_arg, &v, sizeof(v));
    if (rc < 0) return rc;
    atomic_set(&total, (atomic_val_t)v);
    LOG_INF("loaded count=%u", v);
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(scroll_meter, "scroll_meter", NULL,
                              counter_set, NULL, NULL);

/* sensor_event listener */
static int sensor_listener(const zmk_event_t *eh) {
    const struct zmk_sensor_event *ev = as_zmk_sensor_event(eh);
    if (!ev) return ZMK_EV_EVENT_BUBBLE;
    /* Each sensor_event represents one detent of rotation regardless of
       direction; magnitude-only counting is what we want for "how much
       did I scroll". If a future sensor type emits multi-tick events,
       we'd need to read ev->channel_data — for EC11 it's 1-per-detent. */
    scroll_counter_add(1);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(scroll_counter_listener, sensor_listener);
ZMK_SUBSCRIPTION(scroll_counter_listener, zmk_sensor_event);

static int scroll_counter_init(void) {
    k_work_init(&persist_work, persist_now);
    /* settings_subsys_init() is called by ZMK at boot; we just register
       above via SETTINGS_STATIC_HANDLER_DEFINE and rely on settings_load()
       happening during ZMK boot. */
    return 0;
}

SYS_INIT(scroll_counter_init, APPLICATION, 80);

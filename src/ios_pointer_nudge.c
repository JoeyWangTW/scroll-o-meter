/*
 * scroll-meter: wake iOS AssistiveTouch pointer/scroll routing on
 * BLE reconnect, without forcing the user to toggle AT off and on.
 *
 * Walks the AT cursor to mid-screen via a chain of small per-axis
 * relative motions, then fires a self-cancelling scroll wake. Single
 * large diagonal motions appear to get partially eaten by iOS gesture
 * recognition (Y advances but X doesn't), so we step instead.
 *
 * Verbatim from agent-keyboard's iphone-scroll-encoder branch.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <stdint.h>

LOG_MODULE_REGISTER(scroll_meter_ios_nudge, CONFIG_ZMK_LOG_LEVEL);

struct zmk_hid_mouse_report_body {
    uint8_t buttons;
    int16_t d_x;
    int16_t d_y;
    int16_t d_scroll_y;
    int16_t d_scroll_x;
} __packed;

extern int zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *body);

static void send_report(int16_t dx, int16_t dy, int16_t scroll_y) {
    struct zmk_hid_mouse_report_body r = {
        .d_x = dx, .d_y = dy, .d_scroll_y = scroll_y,
    };
    int ret = zmk_hog_send_mouse_report(&r);
    if (ret < 0) {
        LOG_DBG("nudge report failed: %d", ret);
    }
}

struct nudge_step {
    int16_t dx;
    int16_t dy;
    int16_t sy;
    uint16_t next_delay_ms;
};

/* Land the AT cursor at screen-center on every reconnect, regardless
   of where it was sleeping.

   iOS HID is relative-motion only. Trick: send a large negative X+Y
   to park at top-left (iOS clamps at edges), then walk known positive
   steps toward center. Empirically iOS scales AT pointer deltas at
   roughly 2x — a +195 logical-step landed near right edge, not center,
   so values are halved here vs my first pass. Final scroll +1/-1 is
   the AT pointer-routing wake. */
static const struct nudge_step steps[] = {
    /* Park: massive negative diagonal — iOS clamps to (0,0). */
    { -2000, -2000,  0,  200 },
    /* Walk to ~center: empirically iOS's AT cursor scales HID deltas
       ~4x on this iPhone, so halving again from previous attempt. */
    {   15,    0,   0,   80 },
    {   15,    0,   0,   80 },
    {   20,    0,   0,  150 },
    {    0,  100,   0,  200 },
    /* Scroll wake, self-cancelling. */
    {    0,    0,   1,  120 },
    {    0,    0,  -1,  120 },
    {    0,    0,   0,    0 },
};

static size_t step_index;
static struct k_work_delayable step_work;

static void run_step(struct k_work *w) {
    ARG_UNUSED(w);
    if (step_index >= ARRAY_SIZE(steps)) {
        return;
    }
    const struct nudge_step *s = &steps[step_index++];
    send_report(s->dx, s->dy, s->sy);
    if (step_index < ARRAY_SIZE(steps) && s->next_delay_ms > 0) {
        k_work_reschedule(&step_work, K_MSEC(s->next_delay_ms));
    }
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err) {
    ARG_UNUSED(conn);
    if (err || level < BT_SECURITY_L2) {
        return;
    }
    step_index = 0;
    /* Match agent-keyboard's iphone-scroll-encoder branch exactly:
       500ms after security_changed. The Kconfig default of 1200ms
       was a leftover from the initial nudge commit; later commits
       in agent-keyboard hardcoded 500ms but never updated the
       Kconfig. iOS's HID-input CCC subscription window appears to
       close before 1.2s, so the report would be dropped. */
    k_work_reschedule(&step_work, K_MSEC(500));
}

BT_CONN_CB_DEFINE(scroll_meter_ios_nudge_cb) = {
    .security_changed = security_changed,
};

static int scroll_meter_ios_nudge_init(void) {
    k_work_init_delayable(&step_work, run_step);
    return 0;
}

SYS_INIT(scroll_meter_ios_nudge_init, APPLICATION, 90);

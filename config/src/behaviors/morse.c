#include <zephyr/kernel.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/hid.h>
#include <zmk/keys.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>

#define DOT_THRESHOLD_MS 200
#define LETTER_TIMEOUT_MS 600
#define MAX_MORSE_LEN 8

static int64_t press_time;
static char morse_buffer[MAX_MORSE_LEN + 1];
static int morse_len = 0;

static struct k_work_delayable letter_work;

/* Morse lookup table */
struct morse_entry {
    const char *pattern;
    uint32_t keycode;
};

static const struct morse_entry morse_table[] = {
    {".-", HID_USAGE_KEY_KEYBOARD_A},
    {"-...", HID_USAGE_KEY_KEYBOARD_B},
    {"-.-.", HID_USAGE_KEY_KEYBOARD_C},
    {"-..", HID_USAGE_KEY_KEYBOARD_D},
    {".", HID_USAGE_KEY_KEYBOARD_E},
    {"..-.", HID_USAGE_KEY_KEYBOARD_F},
    {"--.", HID_USAGE_KEY_KEYBOARD_G},
    {"....", HID_USAGE_KEY_KEYBOARD_H},
    {"..", HID_USAGE_KEY_KEYBOARD_I},
    {".---", HID_USAGE_KEY_KEYBOARD_J},
    {"-.-", HID_USAGE_KEY_KEYBOARD_K},
    {".-..", HID_USAGE_KEY_KEYBOARD_L},
    {"--", HID_USAGE_KEY_KEYBOARD_M},
    {"-.", HID_USAGE_KEY_KEYBOARD_N},
    {"---", HID_USAGE_KEY_KEYBOARD_O},
    {".--.", HID_USAGE_KEY_KEYBOARD_P},
    {"--.-", HID_USAGE_KEY_KEYBOARD_Q},
    {".-.", HID_USAGE_KEY_KEYBOARD_R},
    {"...", HID_USAGE_KEY_KEYBOARD_S},
    {"-", HID_USAGE_KEY_KEYBOARD_T},
    {"..-", HID_USAGE_KEY_KEYBOARD_U},
    {"...-", HID_USAGE_KEY_KEYBOARD_V},
    {".--", HID_USAGE_KEY_KEYBOARD_W},
    {"-..-", HID_USAGE_KEY_KEYBOARD_X},
    {"-.--", HID_USAGE_KEY_KEYBOARD_Y},
    {"--..", HID_USAGE_KEY_KEYBOARD_Z},
};

static void send_key(uint32_t keycode) {
    zmk_hid_keyboard_press(keycode);
    zmk_hid_keyboard_release(keycode);
}

/* Convert morse buffer to key */
static void letter_timeout_handler(struct k_work *work) {
    morse_buffer[morse_len] = '\0';

    for (int i = 0; i < ARRAY_SIZE(morse_table); i++) {
        if (strcmp(morse_buffer, morse_table[i].pattern) == 0) {
            send_key(morse_table[i].keycode);
            break;
        }
    }

    morse_len = 0;
}

static int morse_press(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    press_time = k_uptime_get();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int morse_release(struct zmk_behavior_binding *binding,
                         struct zmk_behavior_binding_event event) {
    int64_t duration = k_uptime_get() - press_time;

    if (morse_len < MAX_MORSE_LEN) {
        if (duration < DOT_THRESHOLD_MS) {
            morse_buffer[morse_len++] = '.';
        } else {
            morse_buffer[morse_len++] = '-';
        }
    }

    /* Restart letter timeout */
    k_work_reschedule(&letter_work, K_MSEC(LETTER_TIMEOUT_MS));

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api morse_driver_api = {
    .press = morse_press,
    .release = morse_release,
};

static int morse_init(const struct device *dev) {
    ARG_UNUSED(dev);
    k_work_init_delayable(&letter_work, letter_timeout_handler);
    return 0;
}

DEVICE_DT_INST_DEFINE(0,
                      morse_init,
                      NULL,
                      NULL,
                      NULL,
                      APPLICATION,
                      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                      &morse_driver_api);


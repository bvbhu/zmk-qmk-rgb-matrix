/* Copyright 2017 Jason Williams
 * Copyright 2017 Jack Humbert
 * Copyright 2018 Yiancar
 * Copyright (c) 2026 bvbhu
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "rgb_matrix.h"
#include "rgb_matrix_mode_select.h"
#include "utils.h"

#include <zephyr/logging/log.h>
#include <zmk/activity.h>
#include <zmk/usb.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
static struct k_spinlock rgb_last_hit_lock;
#endif
#ifdef RGB_MATRIX_FRAMEBUFFER_EFFECTS
static struct k_spinlock rgb_framebuffer_lock;
#endif

#ifndef RGB_MATRIX_CENTER
const led_point_t k_rgb_matrix_center = {112, 32};
#else
const led_point_t k_rgb_matrix_center = RGB_MATRIX_CENTER;
#endif

__attribute__((weak)) rgb_t rgb_matrix_hsv_to_rgb(hsv_t hsv) {
    return hsv_to_rgb(hsv);
}

// Generic effect runners
#include "rgb_matrix_runners.inc"

// ------------------------------------------
// -----Begin rgb effect includes macros-----
#define RGB_MATRIX_EFFECT(name)
#define RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#include "rgb_matrix_effects.inc"

#undef RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#undef RGB_MATRIX_EFFECT
// -----End rgb effect includes macros-------
// ------------------------------------------

// globals
rgb_config_t rgb_matrix_config; // TODO: would like to prefix this with g_ for global consistancy, do this in another pr
uint32_t     g_rgb_timer;
#ifdef RGB_MATRIX_FRAMEBUFFER_EFFECTS
uint8_t g_rgb_frame_buffer[MATRIX_ROWS][MATRIX_COLS] = {{0}};
#endif // RGB_MATRIX_FRAMEBUFFER_EFFECTS
#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
last_hit_t g_last_hit_tracker;
#endif // RGB_MATRIX_KEYREACTIVE_ENABLED

// internals
static uint8_t         rgb_last_enable    = UINT8_MAX;
static uint8_t         rgb_last_effect    = UINT8_MAX;
static effect_params_t rgb_effect_params  = {0, LED_FLAG_ALL, false};
rgb_task_states        rgb_task_state     = SYNCING;

// double buffers
static uint32_t rgb_timer_buffer;
#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
static last_hit_t last_hit_buffer;
#endif // RGB_MATRIX_KEYREACTIVE_ENABLED

// split rgb matrix
#if defined(RGB_MATRIX_SPLIT)
const uint8_t k_rgb_matrix_split[2] = RGB_MATRIX_SPLIT;
#endif
void eeconfig_update_rgb_matrix_default(void) {
    rgb_matrix_config.enable = RGB_MATRIX_DEFAULT_ON;
    rgb_matrix_config.mode   = rgb_matrix_default_mode_from_name();
    rgb_matrix_config.hsv    = (hsv_t){RGB_MATRIX_DEFAULT_HUE, RGB_MATRIX_DEFAULT_SAT, RGB_MATRIX_DEFAULT_VAL};
    rgb_matrix_config.speed  = RGB_MATRIX_DEFAULT_SPD;
    rgb_matrix_config.flags  = RGB_MATRIX_DEFAULT_FLAGS;
}

__attribute__((weak)) uint8_t rgb_matrix_map_row_column_to_led_kb(uint8_t row, uint8_t column, uint8_t *led_i) {
    return 0;
}

/* row/col → LED 列表。*/
uint8_t rgb_matrix_map_row_column_to_led(uint8_t row, uint8_t column, uint8_t *led_i) {
    if (row >= MATRIX_ROWS || column >= MATRIX_COLS) {
        return 0;
    }

    uint8_t led_count = rgb_matrix_map_row_column_to_led_kb(row, column, led_i);
    if (led_count > LED_HITS_TO_REMEMBER) {
        led_count = LED_HITS_TO_REMEMBER;
    }
    if (led_count >= LED_HITS_TO_REMEMBER) {
        return led_count;
    }

    uint8_t led_index = g_led_config.matrix_co[row][column];
    if (led_index != NO_LED) {
        led_i[led_count] = led_index;
        led_count++;
    }
    return led_count;
}
__attribute__((weak)) int rgb_matrix_led_index(int index) {
#if defined(RGB_MATRIX_SPLIT)
    if (!is_keyboard_left() && index >= k_rgb_matrix_split[0]) {
        return index - k_rgb_matrix_split[0];
    }
#endif
    return index;
}

void rgb_matrix_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    const int led_index = rgb_matrix_led_index(index);
    if (led_index < 0 || (uint8_t)led_index >= RGB_MATRIX_LOCAL_LED_COUNT) {
        return;
    }

    rgb_matrix_driver.set_color(led_index, red, green, blue);
}

void rgb_matrix_set_color_all(uint8_t red, uint8_t green, uint8_t blue) {
#if defined(RGB_MATRIX_SPLIT)
    uint8_t split[2]    = RGB_MATRIX_SPLIT;
    uint8_t side_min    = is_keyboard_left() ? 0 : split[0];
    uint8_t side_max    = is_keyboard_left() ? split[0] : RGB_MATRIX_LED_COUNT;
    for (uint8_t i = side_min; i < side_max; i++) {
        rgb_matrix_set_color(i, red, green, blue);
    }
#else
    rgb_matrix_driver.set_color_all(red, green, blue);
#endif
}

// QMK 兼容：按键事件入口，更新 last_hit_buffer (Key Reactive) 与 Framebuffer (热图)
#if defined(RGB_MATRIX_KEYREACTIVE_ENABLED) || (defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP))
void rgb_matrix_handle_key_event(uint8_t row, uint8_t col, bool pressed) {
#    ifndef RGB_MATRIX_SPLIT
    if (!is_keyboard_master()) return;
#    endif
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) return;
#    ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
    uint8_t led[LED_HITS_TO_REMEMBER];
    uint8_t led_count = 0;
#        if defined(RGB_MATRIX_KEYRELEASES)
    if (!pressed)
#        elif defined(RGB_MATRIX_KEYPRESSES)
    if (pressed)
#        endif // defined(RGB_MATRIX_KEYRELEASES)
    {
        led_count = rgb_matrix_map_row_column_to_led(row, col, led);
    }


    uint8_t valid_count = 0;
    for (uint8_t i = 0; i < led_count; i++) {
        if (led[i] < RGB_MATRIX_LED_COUNT) led[valid_count++] = led[i];
    }
    led_count = valid_count;

    k_spinlock_key_t lock_key = k_spin_lock(&rgb_last_hit_lock);
    if (last_hit_buffer.count + led_count > LED_HITS_TO_REMEMBER) {
        uint8_t drop = last_hit_buffer.count + led_count - LED_HITS_TO_REMEMBER;
        uint8_t keep = last_hit_buffer.count - drop;
        memmove(&last_hit_buffer.x[0], &last_hit_buffer.x[drop], keep);
        memmove(&last_hit_buffer.y[0], &last_hit_buffer.y[drop], keep);
        memmove(&last_hit_buffer.tick[0], &last_hit_buffer.tick[drop], keep * 2); // 16 bit
        memmove(&last_hit_buffer.index[0], &last_hit_buffer.index[drop], keep);
        last_hit_buffer.count = keep;
    }

    for (uint8_t i = 0; i < led_count; i++) {
        uint8_t index                = last_hit_buffer.count;
        last_hit_buffer.x[index]     = g_led_config.point[led[i]].x;
        last_hit_buffer.y[index]     = g_led_config.point[led[i]].y;
        last_hit_buffer.index[index] = led[i];
        last_hit_buffer.tick[index]  = 0;
        last_hit_buffer.count++;
    }
    k_spin_unlock(&rgb_last_hit_lock, lock_key);
#    endif // RGB_MATRIX_KEYREACTIVE_ENABLED
#    if defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP)
#        if defined(RGB_MATRIX_KEYRELEASES)
    if (!pressed)
#        else
    if (pressed)
#        endif // defined(RGB_MATRIX_KEYRELEASES)
    {
        if (rgb_matrix_config.mode == RGB_MATRIX_TYPING_HEATMAP) {
            process_rgb_matrix_typing_heatmap(row, col);
        }
    }
#    endif // defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP)
}
#endif

void rgb_matrix_test(void) {
    // Mask out bits 4 and 5
    // Increase the factor to make the test animation slower (and reduce to make it faster)
    uint8_t factor = 10;
    switch ((g_rgb_timer & (0b11 << factor)) >> factor) {
        case 0: {
            rgb_matrix_set_color_all(20, 0, 0);
            break;
        }
        case 1: {
            rgb_matrix_set_color_all(0, 20, 0);
            break;
        }
        case 2: {
            rgb_matrix_set_color_all(0, 0, 20);
            break;
        }
        case 3: {
            rgb_matrix_set_color_all(20, 20, 20);
            break;
        }
    }
}

static bool rgb_matrix_none(effect_params_t *params) {
    if (!params->init) {
        return false;
    }

    rgb_matrix_set_color_all(0, 0, 0);
    return false;
}

static void rgb_task_timers(void) {
#if defined(RGB_MATRIX_KEYREACTIVE_ENABLED)
    uint32_t deltaTime = sync_timer_elapsed32(rgb_timer_buffer);
#endif // defined(RGB_MATRIX_KEYREACTIVE_ENABLED)
    rgb_timer_buffer = sync_timer_read32();

    // Update double buffer last hit timers
#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
    k_spinlock_key_t lock_key = k_spin_lock(&rgb_last_hit_lock);
    if (last_hit_buffer.count > LED_HITS_TO_REMEMBER) {
        last_hit_buffer.count = LED_HITS_TO_REMEMBER;
    }
    uint8_t count = last_hit_buffer.count;
    for (uint8_t i = 0; i < count;) {
        if (UINT16_MAX - deltaTime < last_hit_buffer.tick[i]) {
            // Entry expired: shift remaining entries down, decrement count
            count--;
            last_hit_buffer.count = count;
            uint8_t tail = count - i;
            if (tail > 0) {
                memmove(&last_hit_buffer.x[i], &last_hit_buffer.x[i + 1], tail);
                memmove(&last_hit_buffer.y[i], &last_hit_buffer.y[i + 1], tail);
                memmove(&last_hit_buffer.tick[i], &last_hit_buffer.tick[i + 1], tail * 2);
                memmove(&last_hit_buffer.index[i], &last_hit_buffer.index[i + 1], tail);
            }
            // Don't increment i; the entry shifted into i needs checking
        } else {
            last_hit_buffer.tick[i] += deltaTime;
            i++;
        }
    }
    k_spin_unlock(&rgb_last_hit_lock, lock_key);
#endif // RGB_MATRIX_KEYREACTIVE_ENABLED
}

static void rgb_task_sync(void) {
    // next task
    if (sync_timer_elapsed32(g_rgb_timer) >= RGB_MATRIX_LED_FLUSH_LIMIT) rgb_task_state = STARTING;
}

static void rgb_task_start(void) {
    // reset iter
    rgb_effect_params.iter = 0;

    // update double buffers
    g_rgb_timer = rgb_timer_buffer;
#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
    k_spinlock_key_t lock_key = k_spin_lock(&rgb_last_hit_lock);
    g_last_hit_tracker        = last_hit_buffer;
    k_spin_unlock(&rgb_last_hit_lock, lock_key);
#endif // RGB_MATRIX_KEYREACTIVE_ENABLED

    // next task
    rgb_task_state = RENDERING;
}

static void rgb_task_render(uint8_t effect) {
    bool rendering         = false;
    rgb_effect_params.init = (effect != rgb_last_effect) || (rgb_matrix_config.enable != rgb_last_enable);
    if (rgb_effect_params.init) {
        rgb_matrix_set_color_all(0, 0, 0);
    }
    if (rgb_effect_params.flags != rgb_matrix_config.flags) {
        rgb_effect_params.flags = rgb_matrix_config.flags;
        rgb_matrix_set_color_all(0, 0, 0);
    }

    // each effect can opt to do calculations
    // and/or request PWM buffer updates.
    switch (effect) {
        case RGB_MATRIX_NONE:
            rendering = rgb_matrix_none(&rgb_effect_params);
            break;

// ---------------------------------------------
// -----Begin rgb effect switch case macros-----
#define RGB_MATRIX_EFFECT(name, ...)          \
    case RGB_MATRIX_##name:                   \
        rendering = name(&rgb_effect_params); \
        break;
#include "rgb_matrix_effects.inc"
#undef RGB_MATRIX_EFFECT

            // -----End rgb effect switch case macros-------
            // ---------------------------------------------

        // Factory default magic value
        case UINT8_MAX: {
            rgb_matrix_test();
            rgb_task_state = FLUSHING;
        }
            return;
    }

    rgb_effect_params.iter++;

    // next task
    if (!rendering) {
        rgb_task_state = FLUSHING;
        if (!rgb_effect_params.init && effect == RGB_MATRIX_NONE) {
            // We only need to flush once if we are RGB_MATRIX_NONE
            rgb_task_state = SYNCING;
        }
    }
}

static void rgb_task_flush(uint8_t effect) {
    // update last trackers after the first full render so we can init over several frames
    rgb_last_effect = effect;
    rgb_last_enable = rgb_matrix_config.enable;

    // update pwm buffers
    rgb_matrix_driver.flush();

    // next task
    rgb_task_state = SYNCING;
}

void rgb_matrix_task(void) {
    rgb_task_timers();

    // Ideally we would also stop sending zeros to the LED driver PWM buffers
    // while suspended and just do a software shutdown. This is a cheap hack for now.
    bool        rgb_idle_off  = false;
    static bool prev_idle_off = false;
#if CONFIG_ZMK_IDLE_TIMEOUT > 0
    /* 有线模式下是否自动关闭 RGB：
     * 定义 RGB_MATRIX_KEEP_ON_WIRED 后，有线（USB）模式不自动关闭，
     * 无线（BLE）模式仍按空闲超时关闭；不定义则有线/无线均按空闲超时关闭。 */
#    if defined(RGB_MATRIX_KEEP_ON_WIRED)
    bool keep_on_wired = zmk_usb_is_powered();
#    else
    bool keep_on_wired = false;
#    endif
    if (!keep_on_wired && zmk_activity_get_state() == ZMK_ACTIVITY_IDLE) {
        rgb_idle_off = true;
    }
#endif

    /* 空闲状态变化时强制重启状态机，确保灯效正确初始化，
     * 避免从空闲切换回活跃时因状态机停留在 SYNCING 导致灯效卡死。 */
    if (rgb_idle_off != prev_idle_off) {
        rgb_task_state = STARTING;
    }
    prev_idle_off = rgb_idle_off;

    uint8_t effect = rgb_matrix_config.enable && !rgb_idle_off ? rgb_matrix_config.mode : RGB_MATRIX_NONE;

    switch (rgb_task_state) {
        case STARTING:
            rgb_task_start();
            break;
        case RENDERING:
            rgb_task_render(effect);
            if (effect) {
                if (rgb_task_state == FLUSHING) { // ensure we only draw basic indicators once rendering is finished
                    rgb_matrix_indicators();
                }
                rgb_matrix_indicators_advanced(&rgb_effect_params);
            }
            break;
        case FLUSHING:
            rgb_task_flush(effect);
            break;
        case SYNCING:
            rgb_task_sync();
            break;
    }
}

__attribute__((weak)) bool rgb_matrix_indicators_modules(void) {
    return true;
}

void rgb_matrix_indicators(void) {
    rgb_matrix_indicators_modules();
    rgb_matrix_indicators_kb();
}

__attribute__((weak)) bool rgb_matrix_indicators_kb(void) {
    return rgb_matrix_indicators_user();
}

__attribute__((weak)) bool rgb_matrix_indicators_user(void) {
    return true;
}

struct rgb_matrix_limits_t rgb_matrix_get_limits(uint8_t iter) {
    struct rgb_matrix_limits_t limits = {0};

    // 当前侧（split）的 LED 范围
    uint8_t side_min = 0;
    uint8_t side_max = RGB_MATRIX_LED_COUNT;
#if defined(RGB_MATRIX_SPLIT)
    if (is_keyboard_left() && (side_max > k_rgb_matrix_split[0])) side_max = k_rgb_matrix_split[0];
    if (!(is_keyboard_left()) && (side_min < k_rgb_matrix_split[0])) side_min = k_rgb_matrix_split[0];
#endif

    uint16_t process_limit = RGB_MATRIX_LED_PROCESS_LIMIT;
    uint16_t side_count    = side_max - side_min;
    if (process_limit >= side_count) {
        // LED 数不超过单帧处理上限：一帧处理全部
        limits.led_min_index = side_min;
        limits.led_max_index = side_max;
    } else {
        // 按 iter 切块：每帧最多处理 process_limit 个 LED
        uint16_t start = (uint16_t)side_min + (uint16_t)iter * process_limit;
        if (start >= side_max) {
            // 已越过末尾：空块，check_finished_leds 会判定渲染完成
            limits.led_min_index = side_max;
            limits.led_max_index = side_max;
        } else {
            limits.led_min_index = (uint8_t)start;
            limits.led_max_index = (uint8_t)(start + process_limit);
            if (limits.led_max_index > side_max) limits.led_max_index = side_max;
        }
    }
    return limits;
}

void rgb_matrix_indicators_advanced(effect_params_t *params) {
    /* special handling is needed for "params->iter", since it's already been incremented.
     * Could move the invocations to rgb_task_render, but then it's missing a few checks
     * and not sure which would be better. Otherwise, this should be called from
     * rgb_task_render, right before the iter++ line.
     */
    RGB_MATRIX_USE_LIMITS_ITER(min, max, params->iter - 1);
    rgb_matrix_indicators_advanced_user(min, max);
}

__attribute__((weak)) bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    return true;
}

void rgb_matrix_init(void) {
    rgb_matrix_driver.init();

#ifdef RGB_MATRIX_KEYREACTIVE_ENABLED
    g_last_hit_tracker.count = 0;
    for (uint8_t i = 0; i < LED_HITS_TO_REMEMBER; ++i) {
        g_last_hit_tracker.tick[i] = UINT16_MAX;
    }

    last_hit_buffer.count = 0;
    for (uint8_t i = 0; i < LED_HITS_TO_REMEMBER; ++i) {
        last_hit_buffer.tick[i] = UINT16_MAX;
    }
#endif // RGB_MATRIX_KEYREACTIVE_ENABLED
}

/* 灯效控制 API （函数名 QMK 相同，但使用zmk的保存机制且默认保存）*/

void rgb_matrix_toggle(void) {
    rgb_matrix_config.enable ^= 1;
    rgb_task_state = STARTING;
    rgb_matrix_settings_save();
}

void rgb_matrix_enable(void) {
    if (!rgb_matrix_config.enable) rgb_task_state = STARTING;
    rgb_matrix_config.enable = 1;
    rgb_matrix_settings_save();
}

void rgb_matrix_disable(void) {
    if (rgb_matrix_config.enable) rgb_task_state = STARTING;
    rgb_matrix_config.enable = 0;
    rgb_matrix_settings_save();
}

uint8_t rgb_matrix_is_enabled(void) {
    return rgb_matrix_config.enable;
}

void rgb_matrix_mode(uint8_t mode) {
    if (!rgb_matrix_config.enable) {
        return;
    }
    if (mode < 1) {
        mode = 1;
    } else if (mode >= RGB_MATRIX_EFFECT_MAX) {
        mode = RGB_MATRIX_EFFECT_MAX - 1;
    }
    rgb_matrix_config.mode = mode;
    rgb_task_state = STARTING;
    rgb_matrix_settings_save();
}

uint8_t rgb_matrix_get_mode(void) {
    return rgb_matrix_config.mode;
}

void rgb_matrix_step(void) {
    uint8_t mode = rgb_matrix_config.mode + 1;
    rgb_matrix_mode((mode < RGB_MATRIX_EFFECT_MAX) ? mode : 1);
}

void rgb_matrix_step_reverse(void) {
    uint8_t mode = rgb_matrix_config.mode - 1;
    rgb_matrix_mode((mode < 1) ? RGB_MATRIX_EFFECT_MAX - 1 : mode);
}
void rgb_matrix_sethsv(uint16_t hue, uint8_t sat, uint8_t val) {
    if (!rgb_matrix_config.enable) {
        return;
    }
    rgb_matrix_config.hsv.h = (uint8_t)hue;
    rgb_matrix_config.hsv.s = sat;
    rgb_matrix_config.hsv.v = (val > RGB_MATRIX_MAXIMUM_BRIGHTNESS) ? RGB_MATRIX_MAXIMUM_BRIGHTNESS : val;
    rgb_matrix_settings_save();
}

hsv_t rgb_matrix_get_hsv(void) {
    return rgb_matrix_config.hsv;
}
uint8_t rgb_matrix_get_hue(void) {
    return rgb_matrix_config.hsv.h;
}
uint8_t rgb_matrix_get_sat(void) {
    return rgb_matrix_config.hsv.s;
}
uint8_t rgb_matrix_get_val(void) {
    return rgb_matrix_config.hsv.v;
}

void rgb_matrix_increase_hue(void) {
    rgb_matrix_sethsv(rgb_matrix_config.hsv.h + RGB_MATRIX_HUE_STEP, rgb_matrix_config.hsv.s, rgb_matrix_config.hsv.v);
}
void rgb_matrix_decrease_hue(void) {
    rgb_matrix_sethsv(rgb_matrix_config.hsv.h - RGB_MATRIX_HUE_STEP, rgb_matrix_config.hsv.s, rgb_matrix_config.hsv.v);
}
void rgb_matrix_increase_sat(void) {
    rgb_matrix_sethsv(rgb_matrix_config.hsv.h, qadd8(rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP), rgb_matrix_config.hsv.v);
}
void rgb_matrix_decrease_sat(void) {
    rgb_matrix_sethsv(rgb_matrix_config.hsv.h, qsub8(rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP), rgb_matrix_config.hsv.v);
}
void rgb_matrix_increase_val(void) {
    rgb_matrix_sethsv(rgb_matrix_config.hsv.h, rgb_matrix_config.hsv.s, qadd8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP));
}
void rgb_matrix_decrease_val(void) {
    rgb_matrix_sethsv(rgb_matrix_config.hsv.h, rgb_matrix_config.hsv.s, qsub8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP));
}
void rgb_matrix_set_speed(uint8_t speed) {
    rgb_matrix_config.speed = speed;
    rgb_matrix_settings_save();
}

uint8_t rgb_matrix_get_speed(void) {
    return rgb_matrix_config.speed;
}

void rgb_matrix_increase_speed(void) {
    rgb_matrix_set_speed(qadd8(rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP));
}
void rgb_matrix_decrease_speed(void) {
    rgb_matrix_set_speed(qsub8(rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP));
}

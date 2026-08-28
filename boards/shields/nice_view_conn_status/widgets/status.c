/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>

#include "status.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define PROFILE_RADIUS 16

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool profiles_connected[NICEVIEW_PROFILE_COUNT];
    bool profiles_bonded[NICEVIEW_PROFILE_COUNT];
};

struct layer_status_state {
    zmk_keymap_layer_index_t index;
    const char *label;
};

static void draw_profile(lv_obj_t *canvas, const struct conn_status_state *state, int index,
                         int x, int y) {
    lv_draw_arc_dsc_t connected_dsc;
    init_arc_dsc(&connected_dsc, LVGL_FOREGROUND, 3);
    lv_draw_arc_dsc_t bonded_dsc;
    init_arc_dsc(&bonded_dsc, LVGL_FOREGROUND, 2);
    lv_draw_arc_dsc_t selected_dsc;
    init_arc_dsc(&selected_dsc, LVGL_FOREGROUND, PROFILE_RADIUS);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);
    lv_draw_label_dsc_t selected_label_dsc;
    init_label_dsc(&selected_label_dsc, LVGL_BACKGROUND, &lv_font_montserrat_18,
                   LV_TEXT_ALIGN_CENTER);

    bool selected = index == state->active_profile_index;

    if (state->profiles_connected[index]) {
        canvas_draw_arc(canvas, x, y, PROFILE_RADIUS, 0, 360, &connected_dsc);
    } else if (state->profiles_bonded[index]) {
        for (int segment = 0; segment < 8; segment++) {
            int start = segment * 45 + 5;
            canvas_draw_arc(canvas, x, y, PROFILE_RADIUS, start, start + 24, &bonded_dsc);
        }
    }

    if (selected) {
        canvas_draw_arc(canvas, x, y, PROFILE_RADIUS, 0, 360, &selected_dsc);
    }

    char label[2];
    snprintf(label, sizeof(label), "%d", index + 1);
    canvas_draw_text(canvas, x - 9, y - 11, 18,
                     selected ? &selected_label_dsc : &label_dsc, label);
}

static void draw_top(lv_obj_t *widget, const struct conn_status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    lv_draw_label_dsc_t output_dsc;
    init_label_dsc(&output_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16,
                   LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t foreground_dsc;
    init_rect_dsc(&foreground_dsc, LVGL_FOREGROUND);

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
    draw_battery(canvas, state);

    char output_text[10] = {};
    switch (state->selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        strcat(output_text, LV_SYMBOL_USB);
        break;
    case ZMK_TRANSPORT_BLE:
        if (!state->active_profile_bonded) {
            strcat(output_text, LV_SYMBOL_SETTINGS);
        } else if (state->active_profile_connected) {
            strcat(output_text, LV_SYMBOL_WIFI);
        } else {
            strcat(output_text, LV_SYMBOL_CLOSE);
        }
        break;
    case ZMK_TRANSPORT_NONE:
        break;
    }

    canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &output_dsc, output_text);
    canvas_draw_rect(canvas, 2, 23, 64, 1, &foreground_dsc);
    draw_profile(canvas, state, 0, 17, 46);
    draw_profile(canvas, state, 1, 51, 46);
    rotate_canvas(canvas);
}

static void draw_middle(lv_obj_t *widget, const struct conn_status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);
    lv_draw_rect_dsc_t foreground_dsc;
    init_rect_dsc(&foreground_dsc, LVGL_FOREGROUND);

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
    draw_profile(canvas, state, 2, 34, 14);
    draw_profile(canvas, state, 3, 17, 50);
    draw_profile(canvas, state, 4, 51, 50);
    canvas_draw_rect(canvas, 2, 67, 64, 1, &foreground_dsc);
    rotate_canvas(canvas);
}

static void draw_bottom(lv_obj_t *widget, const struct conn_status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 2);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_14,
                   LV_TEXT_ALIGN_CENTER);

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    if (state->layer_label == NULL || strlen(state->layer_label) == 0) {
        char text[10] = {};
        snprintf(text, sizeof(text), "LAYER %u", state->layer_index);
        canvas_draw_text(canvas, 0, 5, CANVAS_SIZE, &label_dsc, text);
    } else {
        canvas_draw_text(canvas, 0, 5, CANVAS_SIZE, &label_dsc, state->layer_label);
    }

    rotate_canvas(canvas);
}

static void set_battery_status(struct zmk_widget_conn_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif
    widget->state.battery = state.level;
    draw_top(widget->obj, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_conn_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *event = as_zmk_battery_state_changed(eh);

    return (struct battery_status_state){
        .level = event != NULL ? event->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_conn_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)
ZMK_SUBSCRIPTION(widget_conn_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_conn_battery_status, zmk_usb_conn_state_changed);
#endif

static void set_output_status(struct zmk_widget_conn_status *widget,
                              const struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_index = state->active_profile_index;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;
    for (int i = 0; i < NICEVIEW_PROFILE_COUNT; i++) {
        widget->state.profiles_connected[i] = state->profiles_connected[i];
        widget->state.profiles_bonded[i] = state->profiles_bonded[i];
    }

    draw_top(widget->obj, &widget->state);
    draw_middle(widget->obj, &widget->state);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_conn_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *eh) {
    struct output_status_state state = {
        .selected_endpoint = zmk_endpoint_get_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };

    for (int i = 0; i < MIN(NICEVIEW_PROFILE_COUNT, ZMK_BLE_PROFILE_COUNT); i++) {
        state.profiles_connected[i] = zmk_ble_profile_is_connected(i);
        state.profiles_bonded[i] = !zmk_ble_profile_is_open(i);
    }

    return state;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_conn_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_conn_output_status, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_conn_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_conn_output_status, zmk_ble_active_profile_changed);
#endif

static void set_layer_status(struct zmk_widget_conn_status *widget,
                             struct layer_status_state state) {
    widget->state.layer_index = state.index;
    widget->state.layer_label = state.label;
    draw_bottom(widget->obj, &widget->state);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_conn_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){
        .index = index,
        .label = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index)),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_conn_layer_status, struct layer_status_state,
                            layer_status_update_cb, layer_status_get_state)
ZMK_SUBSCRIPTION(widget_conn_layer_status, zmk_layer_state_changed);

int zmk_widget_conn_status_init(struct zmk_widget_conn_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);

    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);

    lv_obj_t *middle = lv_canvas_create(widget->obj);
    lv_obj_align(middle, LV_ALIGN_TOP_LEFT, 24, 0);
    lv_canvas_set_buffer(middle, widget->cbuf2, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);

    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_TOP_LEFT, -44, 0);
    lv_canvas_set_buffer(bottom, widget->cbuf3, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);

    sys_slist_append(&widgets, &widget->node);
    widget_conn_battery_status_init();
    widget_conn_output_status_init();
    widget_conn_layer_status_init();

    return 0;
}

lv_obj_t *zmk_widget_conn_status_obj(struct zmk_widget_conn_status *widget) {
    return widget->obj;
}

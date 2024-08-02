/*
 * Copyright (c) 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_LVGL_LVGL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_LVGL_LVGL_H_

/* Predefined keys to control the focused object.
 * Values taken from enum _lv_key_t in lv_group.h
 */
#define LV_KEY_UP        17
#define LV_KEY_DOWN      18
#define LV_KEY_RIGHT     19
#define LV_KEY_LEFT      20
#define LV_KEY_ESC       27
#define LV_KEY_DEL       127
#define LV_KEY_BACKSPACE 8
#define LV_KEY_ENTER     10
#define LV_KEY_NEXT      9
#define LV_KEY_PREV      11
#define LV_KEY_HOME      2
#define LV_KEY_END       3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_LVGL_LVGL_H_ */

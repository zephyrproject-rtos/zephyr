/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018,2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Human Interface Device (HID) common definitions header
 *
 * Header follows Device Class Definition for Human Interface Devices (HID)
 * Version 1.11 document (HID1_11-1.pdf).
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_HID_CODES_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_HID_CODES_H_

/* zephyr-keep-sorted-start */
#define HID_KEY_0          39  /**< 0 key */
#define HID_KEY_1          30  /**< 1 key */
#define HID_KEY_2          31  /**< 2 key */
#define HID_KEY_3          32  /**< 3 key */
#define HID_KEY_4          33  /**< 4 key */
#define HID_KEY_5          34  /**< 5 key */
#define HID_KEY_6          35  /**< 6 key */
#define HID_KEY_7          36  /**< 7 key */
#define HID_KEY_8          37  /**< 8 key */
#define HID_KEY_9          38  /**< 9 key */
#define HID_KEY_A          4   /**< A key */
#define HID_KEY_APOSTROPHE 52  /**< Apostrophe key */
#define HID_KEY_B          5   /**< B key */
#define HID_KEY_BACKSLASH  49  /**< Backslash key */
#define HID_KEY_BACKSPACE  42  /**< Backspace key */
#define HID_KEY_C          6   /**< C key */
#define HID_KEY_CAPSLOCK   57  /**< Caps Lock key */
#define HID_KEY_COMMA      54  /**< Comma key */
#define HID_KEY_D          7   /**< D key */
#define HID_KEY_DELETE     76  /**< Delete key */
#define HID_KEY_DOT        55  /**< Dot key */
#define HID_KEY_DOWN       81  /**< Down arrow key */
#define HID_KEY_E          8   /**< E key */
#define HID_KEY_END        77  /**< End key */
#define HID_KEY_ENTER      40  /**< Enter key */
#define HID_KEY_EQUAL      46  /**< Equal key */
#define HID_KEY_ESC        41  /**< Escape key */
#define HID_KEY_F          9   /**< F key */
#define HID_KEY_F1         58  /**< F1 key */
#define HID_KEY_F10        67  /**< F10 key */
#define HID_KEY_F11        68  /**< F11 key */
#define HID_KEY_F12        69  /**< F12 key */
#define HID_KEY_F2         59  /**< F2 key */
#define HID_KEY_F3         60  /**< F3 key */
#define HID_KEY_F4         61  /**< F4 key */
#define HID_KEY_F5         62  /**< F5 key */
#define HID_KEY_F6         63  /**< F6 key */
#define HID_KEY_F7         64  /**< F7 key */
#define HID_KEY_F8         65  /**< F8 key */
#define HID_KEY_F9         66  /**< F9 key */
#define HID_KEY_G          10  /**< G key */
#define HID_KEY_GRAVE      53  /**< Grave accent key */
#define HID_KEY_H          11  /**< H key */
#define HID_KEY_HASH       50  /**< Non-US # and ~ key */
#define HID_KEY_HOME       74  /**< Home key */
#define HID_KEY_I          12  /**< I key */
#define HID_KEY_INSERT     73  /**< Insert key */
#define HID_KEY_J          13  /**< J key */
#define HID_KEY_K          14  /**< K key */
#define HID_KEY_KPASTERISK 85  /**< Numpad asterisk key */
#define HID_KEY_KPDOT      99  /**< Numpad dot key */
#define HID_KEY_KPENTER    88  /**< Numpad enter key */
#define HID_KEY_KPMINUS    86  /**< Numpad minus key */
#define HID_KEY_KPPLUS     87  /**< Numpad plus key */
#define HID_KEY_KPSLASH    84  /**< Numpad slash key */
#define HID_KEY_KP_0       98  /**< Numpad 0 key */
#define HID_KEY_KP_1       89  /**< Numpad 1 key */
#define HID_KEY_KP_2       90  /**< Numpad 2 key */
#define HID_KEY_KP_3       91  /**< Numpad 3 key */
#define HID_KEY_KP_4       92  /**< Numpad 4 key */
#define HID_KEY_KP_5       93  /**< Numpad 5 key */
#define HID_KEY_KP_6       94  /**< Numpad 6 key */
#define HID_KEY_KP_7       95  /**< Numpad 7 key */
#define HID_KEY_KP_8       96  /**< Numpad 8 key */
#define HID_KEY_KP_9       97  /**< Numpad 9 key */
#define HID_KEY_L          15  /**< L key */
#define HID_KEY_LEFT       80  /**< Left arrow key */
#define HID_KEY_LEFTBRACE  47  /**< Left brace key */
#define HID_KEY_M          16  /**< M key */
#define HID_KEY_MENU       101 /**< Menu key */
#define HID_KEY_MINUS      45  /**< Minus key */
#define HID_KEY_N          17  /**< N key */
#define HID_KEY_NONE       0   /**< Unmapped / no key */
#define HID_KEY_NUMLOCK    83  /**< Num Lock key */
#define HID_KEY_O          18  /**< O key */
#define HID_KEY_P          19  /**< P key */
#define HID_KEY_PAGEDOWN   78  /**< Page Down key */
#define HID_KEY_PAGEUP     75  /**< Page Up key */
#define HID_KEY_PAUSE      72  /**< Pause key */
#define HID_KEY_Q          20  /**< Q key */
#define HID_KEY_R          21  /**< R key */
#define HID_KEY_RIGHT      79  /**< Right arrow key */
#define HID_KEY_RIGHTBRACE 48  /**< Right brace key */
#define HID_KEY_S          22  /**< S key */
#define HID_KEY_SCROLLLOCK 71  /**< Scroll Lock key */
#define HID_KEY_SEMICOLON  51  /**< Semicolon key */
#define HID_KEY_SLASH      56  /**< Slash key */
#define HID_KEY_SPACE      44  /**< Space key */
#define HID_KEY_SYSRQ      70  /**< Print Screen / SysRq key */
#define HID_KEY_T          23  /**< T key */
#define HID_KEY_TAB        43  /**< Tab key */
#define HID_KEY_U          24  /**< U key */
#define HID_KEY_UP         82  /**< Up arrow key */
#define HID_KEY_V          25  /**< V key */
#define HID_KEY_W          26  /**< W key */
#define HID_KEY_X          27  /**< X key */
#define HID_KEY_Y          28  /**< Y key */
#define HID_KEY_Z          29  /**< Z key */
/* zephyr-keep-sorted-stop */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_HID_CODES_H_ */

/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Input event codes, for codes available in Linux, use the same values as in
 * https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_INPUT_EVENT_CODES_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_INPUT_EVENT_CODES_H_

/**
 * @defgroup input_events Input Event Definitions
 * @ingroup input_interface
 * @{
 */


/**
 * @name Input event types.
 * @anchor INPUT_EV_CODES
 * @{
 */
#define INPUT_EV_KEY 0x01
#define INPUT_EV_REL 0x02
#define INPUT_EV_ABS 0x03
#define INPUT_EV_MSC 0x04
#define INPUT_EV_VENDOR_START 0xf0
#define INPUT_EV_VENDOR_STOP 0xff
/** @} */

/**
 * @name Input event KEY codes.
 * @anchor INPUT_KEY_CODES
 * @{
 */
#define INPUT_KEY_0 11
#define INPUT_KEY_1 2
#define INPUT_KEY_2 3
#define INPUT_KEY_3 4
#define INPUT_KEY_4 5
#define INPUT_KEY_5 6
#define INPUT_KEY_6 7
#define INPUT_KEY_7 8
#define INPUT_KEY_8 9
#define INPUT_KEY_9 10
#define INPUT_KEY_A 30
#define INPUT_KEY_B 48
#define INPUT_KEY_C 46
#define INPUT_KEY_D 32
#define INPUT_KEY_E 18
#define INPUT_KEY_F 33
#define INPUT_KEY_G 34
#define INPUT_KEY_H 35
#define INPUT_KEY_I 23
#define INPUT_KEY_J 36
#define INPUT_KEY_K 37
#define INPUT_KEY_L 38
#define INPUT_KEY_M 50
#define INPUT_KEY_N 49
#define INPUT_KEY_O 24
#define INPUT_KEY_P 25
#define INPUT_KEY_Q 16
#define INPUT_KEY_R 19
#define INPUT_KEY_S 31
#define INPUT_KEY_T 20
#define INPUT_KEY_U 22
#define INPUT_KEY_V 47
#define INPUT_KEY_VOLUMEDOWN 114
#define INPUT_KEY_VOLUMEUP 115
#define INPUT_KEY_W 17
#define INPUT_KEY_X 45
#define INPUT_KEY_Y 21
#define INPUT_KEY_Z 44
/** @} */

/**
 * @name Input event BTN codes.
 * @anchor INPUT_BTN_CODES
 * @{
 */
#define INPUT_BTN_DPAD_DOWN 0x221
#define INPUT_BTN_DPAD_LEFT 0x222
#define INPUT_BTN_DPAD_RIGHT 0x223
#define INPUT_BTN_DPAD_UP 0x220
#define INPUT_BTN_EAST 0x131
#define INPUT_BTN_LEFT 0x110
#define INPUT_BTN_MIDDLE 0x112
#define INPUT_BTN_MODE 0x13c
#define INPUT_BTN_NORTH 0x133
#define INPUT_BTN_RIGHT 0x111
#define INPUT_BTN_SELECT 0x13a
#define INPUT_BTN_SOUTH 0x130
#define INPUT_BTN_START 0x13b
#define INPUT_BTN_THUMBL 0x13d
#define INPUT_BTN_THUMBR 0x13e
#define INPUT_BTN_TL 0x136
#define INPUT_BTN_TL2 0x138
#define INPUT_BTN_TOUCH 0x14a
#define INPUT_BTN_TR 0x137
#define INPUT_BTN_TR2 0x139
#define INPUT_BTN_WEST 0x134
/** @} */

/**
 * @name Input event ABS codes.
 * @anchor INPUT_ABS_CODES
 * @{
 */
#define INPUT_ABS_RX 0x03
#define INPUT_ABS_RY 0x04
#define INPUT_ABS_RZ 0x05
#define INPUT_ABS_X 0x00
#define INPUT_ABS_Y 0x01
#define INPUT_ABS_Z 0x02
/** @} */

/**
 * @name Input event REL codes.
 * @anchor INPUT_REL_CODES
 * @{
 */
#define INPUT_REL_RX 0x03
#define INPUT_REL_RY 0x04
#define INPUT_REL_RZ 0x05
#define INPUT_REL_X 0x00
#define INPUT_REL_Y 0x01
#define INPUT_REL_Z 0x02
/** @} */

/**
 * @name Input event MSC codes.
 * @anchor INPUT_MSC_CODES
 * @{
 */
#define INPUT_MSC_SCAN 0x04
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_INPUT_EVENT_CODES_H_ */

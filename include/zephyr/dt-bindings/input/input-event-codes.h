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
#define INPUT_EV_KEY 0x01               /**< Key event */
#define INPUT_EV_REL 0x02               /**< Relative coordinate event */
#define INPUT_EV_ABS 0x03               /**< Absolute coordinate event */
#define INPUT_EV_MSC 0x04               /**< Miscellaneous event */
#define INPUT_EV_VENDOR_START 0xf0      /**< Vendor specific event start */
#define INPUT_EV_VENDOR_STOP 0xff       /**< Vendor specific event stop */
/** @} */

/**
 * @name Input event KEY codes.
 * @anchor INPUT_KEY_CODES
 * @{
 */
#define INPUT_KEY_0 11                  /**< 0 Key */
#define INPUT_KEY_1 2                   /**< 1 Key */
#define INPUT_KEY_2 3                   /**< 2 Key */
#define INPUT_KEY_3 4                   /**< 3 Key */
#define INPUT_KEY_4 5                   /**< 4 Key */
#define INPUT_KEY_5 6                   /**< 5 Key */
#define INPUT_KEY_6 7                   /**< 6 Key */
#define INPUT_KEY_7 8                   /**< 7 Key */
#define INPUT_KEY_8 9                   /**< 8 Key */
#define INPUT_KEY_9 10                  /**< 9 Key */
#define INPUT_KEY_A 30                  /**< A Key */
#define INPUT_KEY_B 48                  /**< B Key */
#define INPUT_KEY_C 46                  /**< C Key */
#define INPUT_KEY_D 32                  /**< D Key */
#define INPUT_KEY_E 18                  /**< E Key */
#define INPUT_KEY_F 33                  /**< F Key */
#define INPUT_KEY_G 34                  /**< G Key */
#define INPUT_KEY_H 35                  /**< H Key */
#define INPUT_KEY_I 23                  /**< I Key */
#define INPUT_KEY_J 36                  /**< J Key */
#define INPUT_KEY_K 37                  /**< K Key */
#define INPUT_KEY_L 38                  /**< L Key */
#define INPUT_KEY_M 50                  /**< M Key */
#define INPUT_KEY_N 49                  /**< N Key */
#define INPUT_KEY_O 24                  /**< O Key */
#define INPUT_KEY_P 25                  /**< P Key */
#define INPUT_KEY_Q 16                  /**< Q Key */
#define INPUT_KEY_R 19                  /**< R Key */
#define INPUT_KEY_S 31                  /**< S Key */
#define INPUT_KEY_T 20                  /**< T Key */
#define INPUT_KEY_U 22                  /**< U Key */
#define INPUT_KEY_V 47                  /**< V Key */
#define INPUT_KEY_VOLUMEDOWN 114        /**< Volume Down Key */
#define INPUT_KEY_VOLUMEUP 115          /**< Volume Up Key */
#define INPUT_KEY_W 17                  /**< W Key */
#define INPUT_KEY_X 45                  /**< X Key */
#define INPUT_KEY_Y 21                  /**< Y Key */
#define INPUT_KEY_Z 44                  /**< Z Key */
/** @} */


/**
 * @name Input event BTN codes.
 * @anchor INPUT_BTN_CODES
 * @{
 */
#define INPUT_BTN_DPAD_DOWN 0x221       /**< Directional pad Down */
#define INPUT_BTN_DPAD_LEFT 0x222       /**< Directional pad Left */
#define INPUT_BTN_DPAD_RIGHT 0x223      /**< Directional pad Right */
#define INPUT_BTN_DPAD_UP 0x220         /**< Directional pad Up */
#define INPUT_BTN_EAST 0x131            /**< East button */
#define INPUT_BTN_LEFT 0x110            /**< Left button */
#define INPUT_BTN_MIDDLE 0x112          /**< Middle button */
#define INPUT_BTN_MODE 0x13c            /**< Mode button */
#define INPUT_BTN_NORTH 0x133           /**< North button */
#define INPUT_BTN_RIGHT 0x111           /**< Right button */
#define INPUT_BTN_SELECT 0x13a          /**< Select button */
#define INPUT_BTN_SOUTH 0x130           /**< South button */
#define INPUT_BTN_START 0x13b           /**< Start button */
#define INPUT_BTN_THUMBL 0x13d          /**< Left thumbstick button */
#define INPUT_BTN_THUMBR 0x13e          /**< Right thumbstick button */
#define INPUT_BTN_TL 0x136              /**< Left trigger (L1) */
#define INPUT_BTN_TL2 0x138             /**< Left trigger 2 (L2) */
#define INPUT_BTN_TOUCH 0x14a           /**< Touchscreen touch */
#define INPUT_BTN_TR 0x137              /**< Right trigger (R1) */
#define INPUT_BTN_TR2 0x139             /**< Right trigger 2 (R2) */
#define INPUT_BTN_WEST 0x134            /**< West button */
/** @} */

/**
 * @name Input event ABS codes.
 * @anchor INPUT_ABS_CODES
 * @{
 */
#define INPUT_ABS_RX 0x03               /**< Absolute rotation around X axis */
#define INPUT_ABS_RY 0x04               /**< Absolute rotation around Y axis */
#define INPUT_ABS_RZ 0x05               /**< Absolute rotation around Z axis */
#define INPUT_ABS_X 0x00                /**< Absolute X coordinate */
#define INPUT_ABS_Y 0x01                /**< Absolute Y coordinate */
#define INPUT_ABS_Z 0x02                /**< Absolute Z coordinate */
/** @} */

/**
 * @name Input event REL codes.
 * @anchor INPUT_REL_CODES
 * @{
 */
#define INPUT_REL_RX 0x03               /**< Relative rotation around X axis */
#define INPUT_REL_RY 0x04               /**< Relative rotation around Y axis */
#define INPUT_REL_RZ 0x05               /**< Relative rotation around Z axis */
#define INPUT_REL_X 0x00                /**< Relative X coordinate */
#define INPUT_REL_Y 0x01                /**< Relative Y coordinate */
#define INPUT_REL_Z 0x02                /**< Relative Z coordinate */
/** @} */

/**
 * @name Input event MSC codes.
 * @anchor INPUT_MSC_CODES
 * @{
 */
#define INPUT_MSC_SCAN 0x04             /**< Scan code */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INPUT_INPUT_EVENT_CODES_H_ */

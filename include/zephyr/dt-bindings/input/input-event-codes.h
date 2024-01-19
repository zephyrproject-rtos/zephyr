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
#define INPUT_KEY_RESERVED 0            /**< Reserved, do not use */

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
#define INPUT_KEY_APOSTROPHE 40         /**< Apostrophe Key */
#define INPUT_KEY_B 48                  /**< B Key */
#define INPUT_KEY_BACK 158              /**< Back Key */
#define INPUT_KEY_BACKSLASH 43          /**< Backslash Key */
#define INPUT_KEY_BACKSPACE 14          /**< Backspace Key */
#define INPUT_KEY_BLUETOOTH 237         /**< Bluetooth Key */
#define INPUT_KEY_BRIGHTNESSDOWN 224    /**< Brightness Up Key */
#define INPUT_KEY_BRIGHTNESSUP 225      /**< Brightneess Down Key */
#define INPUT_KEY_C 46                  /**< C Key */
#define INPUT_KEY_CAPSLOCK 58           /**< Caps Lock Key */
#define INPUT_KEY_COFFEE 152            /**< Screen Saver Key */
#define INPUT_KEY_COMMA 51              /**< Comma Key */
#define INPUT_KEY_COMPOSE 127           /**< Compose Key */
#define INPUT_KEY_CONNECT 218           /**< Connect Key */
#define INPUT_KEY_D 32                  /**< D Key */
#define INPUT_KEY_DELETE 111            /**< Delete Key */
#define INPUT_KEY_DOT 52                /**< Dot Key */
#define INPUT_KEY_DOWN 108              /**< Down Key */
#define INPUT_KEY_E 18                  /**< E Key */
#define INPUT_KEY_END 107               /**< End Key */
#define INPUT_KEY_ENTER 28              /**< Enter Key */
#define INPUT_KEY_EQUAL 13              /**< Equal Key */
#define INPUT_KEY_ESC 1                 /**< Escape Key */
#define INPUT_KEY_F 33                  /**< F Key */
#define INPUT_KEY_F1 59                 /**< F1 Key */
#define INPUT_KEY_F10 68                /**< F10 Key */
#define INPUT_KEY_F11 87                /**< F11 Key */
#define INPUT_KEY_F12 88                /**< F12 Key */
#define INPUT_KEY_F13 183               /**< F13 Key */
#define INPUT_KEY_F14 184               /**< F14 Key */
#define INPUT_KEY_F15 185               /**< F15 Key */
#define INPUT_KEY_F16 186               /**< F16 Key */
#define INPUT_KEY_F17 187               /**< F17 Key */
#define INPUT_KEY_F18 188               /**< F18 Key */
#define INPUT_KEY_F19 189               /**< F19 Key */
#define INPUT_KEY_F2 60                 /**< F2 Key */
#define INPUT_KEY_F20 190               /**< F20 Key */
#define INPUT_KEY_F21 191               /**< F21 Key */
#define INPUT_KEY_F22 192               /**< F22 Key */
#define INPUT_KEY_F23 193               /**< F23 Key */
#define INPUT_KEY_F24 194               /**< F24 Key */
#define INPUT_KEY_F3 61                 /**< F3 Key */
#define INPUT_KEY_F4 62                 /**< F4 Key */
#define INPUT_KEY_F5 63                 /**< F5 Key */
#define INPUT_KEY_F6 64                 /**< F6 Key */
#define INPUT_KEY_F7 65                 /**< F7 Key */
#define INPUT_KEY_F8 66                 /**< F8 Key */
#define INPUT_KEY_F9 67                 /**< F9 Key */
#define INPUT_KEY_FASTFORWARD 208       /**< Fast Forward Key */
#define INPUT_KEY_FORWARD 159           /**< Forward Key */
#define INPUT_KEY_G 34                  /**< G Key */
#define INPUT_KEY_GRAVE 41              /**< Grave (backtick) Key */
#define INPUT_KEY_H 35                  /**< H Key */
#define INPUT_KEY_HOME 102              /**< Home Key */
#define INPUT_KEY_I 23                  /**< I Key */
#define INPUT_KEY_INSERT 110            /**< Insert Key */
#define INPUT_KEY_J 36                  /**< J Key */
#define INPUT_KEY_K 37                  /**< K Key */
#define INPUT_KEY_KP0 82                /**< Keypad 0 Key */
#define INPUT_KEY_KP1 79                /**< Keypad 1 Key */
#define INPUT_KEY_KP2 80                /**< Keypad 2 Key */
#define INPUT_KEY_KP3 81                /**< Keypad 3 Key */
#define INPUT_KEY_KP4 75                /**< Keypad 4 Key */
#define INPUT_KEY_KP5 76                /**< Keypad 5 Key */
#define INPUT_KEY_KP6 77                /**< Keypad 6 Key */
#define INPUT_KEY_KP7 71                /**< Keypad 7 Key */
#define INPUT_KEY_KP8 72                /**< Keypad 8 Key */
#define INPUT_KEY_KP9 73                /**< Keypad 9 Key */
#define INPUT_KEY_KPASTERISK 55         /**< Keypad Asterisk Key */
#define INPUT_KEY_KPCOMMA 121           /**< Keypad Comma Key */
#define INPUT_KEY_KPDOT 83              /**< Keypad Dot Key */
#define INPUT_KEY_KPENTER 96            /**< Keypad Enter Key */
#define INPUT_KEY_KPEQUAL 117           /**< Keypad Equal Key */
#define INPUT_KEY_KPMINUS 74            /**< Keypad Minus Key */
#define INPUT_KEY_KPPLUS 78             /**< Keypad Plus Key */
#define INPUT_KEY_KPPLUSMINUS 118       /**< Keypad Plus Key */
#define INPUT_KEY_KPSLASH 98            /**< Keypad Slash Key */
#define INPUT_KEY_L 38                  /**< L Key */
#define INPUT_KEY_LEFT 105              /**< Left Key */
#define INPUT_KEY_LEFTALT 56            /**< Left Alt Key */
#define INPUT_KEY_LEFTBRACE 26          /**< Left Brace Key */
#define INPUT_KEY_LEFTCTRL 29           /**< Left Ctrl Key */
#define INPUT_KEY_LEFTMETA 125          /**< Left Meta Key */
#define INPUT_KEY_LEFTSHIFT 42          /**< Left Shift Key */
#define INPUT_KEY_M 50                  /**< M Key */
#define INPUT_KEY_MENU 139              /**< Menu Key */
#define INPUT_KEY_MINUS 12              /**< Minus Key */
#define INPUT_KEY_MUTE 113              /**< Mute Key */
#define INPUT_KEY_N 49                  /**< N Key */
#define INPUT_KEY_NUMLOCK 69            /**< Num Lock Key */
#define INPUT_KEY_O 24                  /**< O Key */
#define INPUT_KEY_P 25                  /**< P Key */
#define INPUT_KEY_PAGEDOWN 109          /**< Page Down Key */
#define INPUT_KEY_PAGEUP 104            /**< Page UpKey */
#define INPUT_KEY_PAUSE 119             /**< Pause Key */
#define INPUT_KEY_PLAY 207              /**< Play Key */
#define INPUT_KEY_POWER 116             /**< Power Key */
#define INPUT_KEY_PRINT 210             /**< Print Key */
#define INPUT_KEY_Q 16                  /**< Q Key */
#define INPUT_KEY_R 19                  /**< R Key */
#define INPUT_KEY_RIGHT 106             /**< Right Key */
#define INPUT_KEY_RIGHTALT 100          /**< Right Alt Key */
#define INPUT_KEY_RIGHTBRACE 27         /**< Right Brace Key */
#define INPUT_KEY_RIGHTCTRL 97          /**< Right Ctrl Key */
#define INPUT_KEY_RIGHTMETA 126         /**< Right Meta Key */
#define INPUT_KEY_RIGHTSHIFT 54         /**< Right Shift Key */
#define INPUT_KEY_S 31                  /**< S Key */
#define INPUT_KEY_SCALE 120             /**< Scale Key */
#define INPUT_KEY_SCROLLLOCK 70         /**< Scroll Lock Key */
#define INPUT_KEY_SEMICOLON 39          /**< Semicolon Key */
#define INPUT_KEY_SLASH 53              /**< Slash Key */
#define INPUT_KEY_SLEEP 142             /**< System Sleep Key */
#define INPUT_KEY_SPACE 57              /**< Space Key */
#define INPUT_KEY_SYSRQ 99              /**< SysReq Key */
#define INPUT_KEY_T 20                  /**< T Key */
#define INPUT_KEY_TAB 15                /**< Tab Key*/
#define INPUT_KEY_U 22                  /**< U Key */
#define INPUT_KEY_UP 103                /**< Up Key */
#define INPUT_KEY_UWB 239               /**< Ultra-Wideband Key */
#define INPUT_KEY_V 47                  /**< V Key */
#define INPUT_KEY_VOLUMEDOWN 114        /**< Volume Down Key */
#define INPUT_KEY_VOLUMEUP 115          /**< Volume Up Key */
#define INPUT_KEY_W 17                  /**< W Key */
#define INPUT_KEY_WAKEUP 143            /**< System Wake Up Key */
#define INPUT_KEY_WLAN 238              /**< Wireless LAN Key */
#define INPUT_KEY_X 45                  /**< X Key */
#define INPUT_KEY_Y 21                  /**< Y Key */
#define INPUT_KEY_Z 44                  /**< Z Key */
/** @} */


/**
 * @name Input event BTN codes.
 * @anchor INPUT_BTN_CODES
 * @{
 */
#define INPUT_BTN_0 0x100               /**< 0 button */
#define INPUT_BTN_1 0x101               /**< 1 button */
#define INPUT_BTN_2 0x102               /**< 2 button */
#define INPUT_BTN_3 0x103               /**< 3 button */
#define INPUT_BTN_4 0x104               /**< 4 button */
#define INPUT_BTN_5 0x105               /**< 5 button */
#define INPUT_BTN_6 0x106               /**< 6 button */
#define INPUT_BTN_7 0x107               /**< 7 button */
#define INPUT_BTN_8 0x108               /**< 8 button */
#define INPUT_BTN_9 0x109               /**< 9 button */
#define INPUT_BTN_A BTN_SOUTH           /**< A button */
#define INPUT_BTN_B BTN_EAST            /**< B button */
#define INPUT_BTN_C 0x132               /**< C button */
#define INPUT_BTN_DPAD_DOWN 0x221       /**< Directional pad Down */
#define INPUT_BTN_DPAD_LEFT 0x222       /**< Directional pad Left */
#define INPUT_BTN_DPAD_RIGHT 0x223      /**< Directional pad Right */
#define INPUT_BTN_DPAD_UP 0x220         /**< Directional pad Up */
#define INPUT_BTN_EAST 0x131            /**< East button */
#define INPUT_BTN_GEAR_DOWN 0x150       /**< Gear Up button */
#define INPUT_BTN_GEAR_UP 0x151         /**< Gear Down button */
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
#define INPUT_BTN_X BTN_NORTH           /**< X button */
#define INPUT_BTN_Y BTN_WEST            /**< Y button */
#define INPUT_BTN_Z 0x135               /**< Z button */
/** @} */

/**
 * @name Input event ABS codes.
 * @anchor INPUT_ABS_CODES
 * @{
 */
#define INPUT_ABS_BRAKE 0x0a            /**< Absolute brake position */
#define INPUT_ABS_GAS 0x09              /**< Absolute gas position */
#define INPUT_ABS_RUDDER 0x07           /**< Absolute rudder position */
#define INPUT_ABS_RX 0x03               /**< Absolute rotation around X axis */
#define INPUT_ABS_RY 0x04               /**< Absolute rotation around Y axis */
#define INPUT_ABS_RZ 0x05               /**< Absolute rotation around Z axis */
#define INPUT_ABS_THROTTLE 0x06         /**< Absolute throttle position */
#define INPUT_ABS_WHEEL 0x08            /**< Absolute wheel position */
#define INPUT_ABS_X 0x00                /**< Absolute X coordinate */
#define INPUT_ABS_Y 0x01                /**< Absolute Y coordinate */
#define INPUT_ABS_Z 0x02                /**< Absolute Z coordinate */
/** @} */

/**
 * @name Input event REL codes.
 * @anchor INPUT_REL_CODES
 * @{
 */
#define INPUT_REL_DIAL 0x07             /**< Relative dial coordinate */
#define INPUT_REL_HWHEEL 0x06           /**< Relative horizontal wheel coordinate */
#define INPUT_REL_MISC 0x09             /**< Relative misc coordinate */
#define INPUT_REL_RX 0x03               /**< Relative rotation around X axis */
#define INPUT_REL_RY 0x04               /**< Relative rotation around Y axis */
#define INPUT_REL_RZ 0x05               /**< Relative rotation around Z axis */
#define INPUT_REL_WHEEL 0x08            /**< Relative wheel coordinate */
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

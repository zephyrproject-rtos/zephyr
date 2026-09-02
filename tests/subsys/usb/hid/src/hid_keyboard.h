/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HID_KEYBOARD_H_INCLUDED
#define HID_KEYBOARD_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

int hid_keyboard_register(void);
uint8_t hid_keyboard_get_report_value(void);

/**
 * @brief Configure the result the emulated keyboard's Get Report handler should give
 *
 * @param error 0 for a successful (dummy) report, a negative errno value to make the
 *              device reject (stall) the next Get Report requests
 */
void hid_keyboard_set_get_report_error(int error);

/**
 * @brief Configure whether the emulated keyboard advertises Set Idle support
 *
 * @param supported false to make the device reject (stall) Set Idle requests
 */
void hid_keyboard_set_idle_supported(bool supported);

#endif /* HID_KEYBOARD_H_INCLUDED */

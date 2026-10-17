/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FAKE_DISPLAY_H_
#define FAKE_DISPLAY_H_

#include <stdint.h>

/* Number of display_write() calls the fake display has received. */
uint32_t fake_display_writes(void);

#endif /* FAKE_DISPLAY_H_ */

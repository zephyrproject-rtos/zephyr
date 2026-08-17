/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_RTC_RTC_EMUL_NATIVE_H
#define DRIVERS_RTC_RTC_EMUL_NATIVE_H

#include <stdint.h>

int rtc_emul_native_gettime(int64_t *sec, int64_t *nsec);

#endif /* DRIVERS_RTC_RTC_EMUL_NATIVE_H */

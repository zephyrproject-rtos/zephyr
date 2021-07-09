/*
 * Copyright (c) 2020 Endian Technologies AB
 * Copyright (c) 2020 Laird Connectivity
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GSM_PPP_H_
#define GSM_PPP_H_

#ifdef CONFIG_NEWLIB_LIBC
#include <time.h>
#endif

#define GSM_MODEM_DEVICE_NAME "modem_gsm"

/** @cond INTERNAL_HIDDEN */
struct device;
void gsm_ppp_start(const struct device *dev);
void gsm_ppp_stop(const struct device *dev);
#ifdef CONFIG_NEWLIB_LIBC
/**
 * @brief Get the local time from the modem's real time clock.
 *
 * @param ts time structure
 * @param offset The amount the local time is offset from GMT/UTC in seconds.
 *
 * @param 0 if successful
 */
int32_t gsm_ppp_get_local_time(const struct device *dev,
			       struct tm *ts,
			       int32_t *offset);
#endif
/** @endcond */

#endif /* GSM_PPP_H_ */

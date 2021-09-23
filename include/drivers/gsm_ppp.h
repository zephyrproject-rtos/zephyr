/*
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GSM_PPP_H_
#define GSM_PPP_H_

#include <device.h>

#define GSM_MODEM_DEVICE_NAME "modem_gsm"

struct modem_context;

/**
 * @brief Application specific modem setup code.
 *
 * This function is a NOP by default, but weakly linked so that it can be
 * explicitly implemented by the application if they chose, but gets optimized
 * out if they don't (similar to how we do with main()). This is called after
 * the general modem setup, just before the PDP context is initialized. Typical
 * use cases for this would be to disable GSM or customize the bandmasks
 * according to regional differences.
 *
 * @param context pointer to modem info
 * @param sem_response semaphore to be passed to modem_cmd_send
 */
void gsm_ppp_application_setup(struct modem_context *context,
			       struct k_sem *sem_response);

/**
 * @brief RTC setup.
 *
 * This function is a NOP by default, but weakly linked so that it can be
 * explicitly implemented by the application if they chose, but gets optimized
 * out if they don't (similar to how we do with main()). This is called right
 * before starting PPP, and gets the current time from the network. This
 * allows the application to set the RTC.
 *
 * @param year last 2 digits of the current year
 * @param mon the current month
 * @param day the current day
 * @param hour the current hour in 24-hour format
 * @param min the current minute
 * @param sec the current second
 * @param tz timezone offset in quarters of an hour
 */
void gsm_ppp_clock_set(int year, int mon, int day, int hour, int min,
                       int sec, int tz);

/** @cond INTERNAL_HIDDEN */
void gsm_ppp_start(const struct device *device);
void gsm_ppp_stop(const struct device *device);

/** @endcond */


#endif /* GSM_PPP_H_ */

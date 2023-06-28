/*
 * Copyright (c) 2023 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MODEM_MODEM_SMS_H_
#define ZEPHYR_DRIVERS_MODEM_MODEM_SMS_H_

#include <zephyr/drivers/modem/sms.h>

/**
 * @brief Notify all registered callbacks of a received SMS message
 *
 * @param dev Device pointer of modem which received the SMS message
 * @param sms Received SMS message
 * @param csms_ref CSMS Reference number (if available, value is set to -1 if not)
 * @param csms_idx CSMS Index number (if available, value is set to 0 if not)
 * @param csms_tot CSMS Total segment count (if available, value is set to 1 if not)
 *
 */
void notify_sms_recv(const struct device *dev, struct sms_in *sms, int csms_ref, int csms_idx,
		       int csms_tot);

#endif /* ZEPHYR_DRIVERS_MODEM_MODEM_SMS_H_ */

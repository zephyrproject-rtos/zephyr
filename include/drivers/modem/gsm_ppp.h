/*
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_GSM_PPP_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_GSM_PPP_H_

/** @cond INTERNAL_HIDDEN */
struct device;
typedef void (*gsm_modem_power_cb)(const struct device *, void *);

void gsm_ppp_start(const struct device *dev);
void gsm_ppp_stop(const struct device *dev);
/** @endcond */

/**
 * @brief Register functions callbacks for power modem on/off.
 *
 * @param dev: gsm modem device
 * @param modem_on: callback function to
 *		execute during gsm ppp configuring.
 * @param modem_off: callback function to
 *		execute during gsm ppp stopping.
 * @param user_data: user specified data
 *
 * @retval None.
 */
void gsm_ppp_register_modem_power_callback(const struct device *dev,
					   gsm_modem_power_cb modem_on,
					   gsm_modem_power_cb modem_off,
					   void *user_data);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_GSM_PPP_H_ */

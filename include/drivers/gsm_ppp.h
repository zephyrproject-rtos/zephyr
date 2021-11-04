/*
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GSM_PPP_H_
#define GSM_PPP_H_

/** @cond INTERNAL_HIDDEN */
struct device;
typedef void (*gsm_modem_power_cb)(const struct device *, void *);

/**
 * @brief  Start the GSM PPP modem.
 *
 * @param  dev: Modem device struct
 *
 * @retval 0 if success, errno.h values otherwise.
 */
int gsm_ppp_start(const struct device *dev);

/**
 * @brief  Stop the GSM PPP modem.
 *
 * @param  dev: Modem device struct
 *
 * @retval 0 if success, errno.h values otherwise.
 */
int gsm_ppp_stop(const struct device *dev);
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

struct modem_context;

/**
 * @brief Weakly linked hook for augmenting modem setup.
 *
 * This will get called just before PDP context creation, but after initial
 * setup.
 *
 * @param ctx: Todo.
 * @param sem: Todo.
 *
 * @retval Todo.
 */
int gsm_ppp_setup_hook(struct modem_context *ctx, struct k_sem *sem);

/**
 * @brief Weakly linked hook for augmenting modem setup.
 *
 * This will get called after network connection has been established, just
 * before activating PPP.
 *
 * @param ctx: Todo.
 * @param sem: Todo.
 *
 * @retval Todo.
 */
int gsm_ppp_pre_connect_hook(struct modem_context *ctx, struct k_sem *sem);

#endif /* GSM_PPP_H_ */

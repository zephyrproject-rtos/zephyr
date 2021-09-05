/*
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GSM_PPP_H
#define GSM_PPP_H

#include <device.h>
#include "modem_context.h"

#define GSM_PPP_HAS_PWR_SRC DT_INST_NODE_HAS_PROP(0, power_src_gpios)
#define GSM_PPP_HAS_PWR_KEY DT_INST_NODE_HAS_PROP(0, power_key_gpios)

#if GSM_PPP_HAS_PWR_SRC || GSM_PPP_HAS_PWR_KEY
/* pin settings */
enum mdm_control_pins {
#if GSM_PPP_HAS_PWR_SRC
	GSM_PPP_MDM_PWR_SRC,
#endif
#if GSM_PPP_HAS_PWR_KEY
	GSM_PPP_MDM_PWR_KEY,
#endif
};

/* Modem pins - Power, Reset & others. */
static struct modem_pin modem_pins[] = {
#if GSM_PPP_HAS_PWR_SRC
	/* MDM_POWER SUPPLY */
	MODEM_PIN(DT_INST_GPIO_LABEL(0, power_src_gpios), DT_INST_GPIO_PIN(0, power_src_gpios),
		  DT_INST_GPIO_FLAGS(0, power_src_gpios) | GPIO_OUTPUT_LOW),
#endif

#if GSM_PPP_HAS_PWR_KEY
	/* MDM_POWER_KEY */
	MODEM_PIN(DT_INST_GPIO_LABEL(0, power_key_gpios), DT_INST_GPIO_PIN(0, power_key_gpios),
		  DT_INST_GPIO_FLAGS(0, power_key_gpios) | GPIO_OUTPUT_LOW),
#endif
};
#endif

/**
 * @brief  Disable power source of the modem.
 *
 * @param  ctx: modem_context struct
 *
 * @retval None.
 */
static inline void gsm_ppp_disable_power_source(struct modem_context *ctx)
{
#if GSM_PPP_HAS_PWR_SRC
	modem_pin_write(ctx, GSM_PPP_MDM_PWR_SRC, 0);
#endif
}

/**
 * @brief  Enable power source of the modem.
 *
 * @param  ctx: modem_context struct
 *
 * @retval None.
 */
static inline void gsm_ppp_enable_power_source(struct modem_context *ctx)
{
#if GSM_PPP_HAS_PWR_SRC
	modem_pin_write(ctx, GSM_PPP_MDM_PWR_SRC, 1);
#endif
}

#if GSM_PPP_HAS_PWR_KEY
static inline void gsm_ppp_press_power_key(struct modem_context *ctx, const k_timeout_t dur)
{
	modem_pin_write(ctx, GSM_PPP_MDM_PWR_KEY, 1);
	k_sleep(dur);
	modem_pin_write(ctx, GSM_PPP_MDM_PWR_KEY, 0);
}
#endif

/**
 * @brief  Operations required to turn the modem on.
 *
 * @param  ctx: modem_context struct
 *
 * @retval None.
 */
static inline void gsm_ppp_power_on_ops(struct modem_context *ctx)
{
#if DT_INST_NODE_HAS_PROP(0, power_key_on_ms)
	gsm_ppp_press_power_key(ctx, K_MSEC(DT_INST_PROP(0, power_key_on_ms)));
#endif
}

/**
 * @brief  Operations required to turn the modem off.
 *
 * @param  ctx: modem_context struct
 *
 * @retval None.
 */
static inline void gsm_ppp_power_off_ops(struct modem_context *ctx)
{
#if DT_INST_NODE_HAS_PROP(0, power_key_off_ms)
	gsm_ppp_press_power_key(ctx, K_MSEC(DT_INST_PROP(0, power_key_off_ms)));
#endif
}

#endif /* GSM_PPP_H */

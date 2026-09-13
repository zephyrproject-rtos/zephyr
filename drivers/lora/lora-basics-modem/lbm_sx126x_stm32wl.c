/*
 * Copyright (c) 2021 Fabio Baltieri
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Board glue for the sub-GHz radio inside an STM32WL, where the SX126x core
 * has no pins of its own. Reset comes from the reset controller, busy from a
 * power register flag and the radio interrupt from the NVIC.
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stm32_ll_exti.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>

#include "lbm_sx126x_core.h"

#define DT_DRV_COMPAT st_stm32wl_subghz_radio

LOG_MODULE_DECLARE(lbm_driver, CONFIG_LORA_LOG_LEVEL);

/* The radio interrupt shares EXTI line 44 with the busy signal. */
#define STM32WL_RADIO_EXTI_LINE LL_EXTI_LINE_44

static void stm32wl_reset(const struct device *dev)
{
	struct lbm_sx126x_data *data = dev->data;

	LL_RCC_RF_EnableReset();
	k_sleep(K_MSEC(20));
	LL_RCC_RF_DisableReset();
	k_sleep(K_MSEC(10));

	/* The core comes out of reset asleep, so the next access has to wake it. */
	data->asleep = true;
}

static bool stm32wl_is_busy(const struct device *dev)
{
	ARG_UNUSED(dev);

	return LL_PWR_IsActiveFlag_RFBUSYS() != 0;
}

static void stm32wl_dio1_irq_set(const struct device *dev, bool enable)
{
	ARG_UNUSED(dev);

	if (!enable) {
		irq_disable(DT_INST_IRQN(0));
		return;
	}

	/*
	 * The handler leaves the line masked but pending, so clear it here.
	 * Enabling it while still pending re-enters the handler at once, and
	 * the bus traffic that follows wakes a radio that meant to sleep.
	 */
	k_irq_clear_pending(DT_INST_IRQN(0));
	irq_enable(DT_INST_IRQN(0));
}

static void stm32wl_radio_isr(const struct device *dev)
{
	struct lbm_sx126x_data *data = dev->data;

	/* Level triggered, so keep it masked until the work has read the cause. */
	irq_disable(DT_INST_IRQN(0));

	k_work_schedule(&data->lbm_common.op_done_work, K_NO_WAIT);
}

static int stm32wl_variant_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), stm32wl_radio_isr,
		    DEVICE_DT_INST_GET(0), 0);
	LL_EXTI_EnableIT_32_63(STM32WL_RADIO_EXTI_LINE);

	lbm_driver_dio1_irq_enable(dev);

	return 0;
}

/* Amplifier settings from ST AN5457, chapter 5.1.2. */
#define STM32WL_LP_PWR_MIN -17
#define STM32WL_HP_PWR_MIN -9

#define SX126X_PA_OUTPUT_RFO_LP 0
#define SX126X_PA_OUTPUT_RFO_HP 1

struct stm32wl_driver_config {
	struct lbm_sx126x_config common;
	/* The part carries two amplifiers where a discrete radio has one */
	uint8_t pa_output;
	int8_t rfo_lp_max_power;
	int8_t rfo_hp_max_power;
};

static void stm32wl_get_tx_cfg(const struct device *dev, int16_t power,
			       ral_sx126x_bsp_tx_cfg_output_params_t *output_params)
{
	const struct stm32wl_driver_config *config = dev->config;

	if (config->pa_output == SX126X_PA_OUTPUT_RFO_LP) {
		int8_t max_power = config->rfo_lp_max_power;

		power = MIN(power, max_power);
		output_params->pa_cfg.device_sel = 0x01;
		output_params->pa_cfg.hp_max = 0x00;
		if (max_power == 15) {
			output_params->pa_cfg.pa_duty_cycle = 0x06;
			power = 14 - (max_power - power);
		} else if (max_power == 10) {
			output_params->pa_cfg.pa_duty_cycle = 0x01;
			power = 13 - (max_power - power);
		} else {
			output_params->pa_cfg.pa_duty_cycle = 0x04;
			power = 14 - (max_power - power);
		}
		power = MAX(power, STM32WL_LP_PWR_MIN);
	} else {
		int8_t max_power = config->rfo_hp_max_power;

		power = MIN(power, max_power);
		output_params->pa_cfg.device_sel = 0x00;
		if (max_power == 20) {
			output_params->pa_cfg.pa_duty_cycle = 0x03;
			output_params->pa_cfg.hp_max = 0x05;
			power = 22 - (max_power - power);
		} else if (max_power == 17) {
			output_params->pa_cfg.pa_duty_cycle = 0x02;
			output_params->pa_cfg.hp_max = 0x03;
			power = 22 - (max_power - power);
		} else if (max_power == 14) {
			output_params->pa_cfg.pa_duty_cycle = 0x02;
			output_params->pa_cfg.hp_max = 0x02;
			power = 14 - (max_power - power);
		} else {
			output_params->pa_cfg.pa_duty_cycle = 0x04;
			output_params->pa_cfg.hp_max = 0x07;
			power = 22 - (max_power - power);
		}
		power = MAX(power, STM32WL_HP_PWR_MIN);
	}

	output_params->chip_output_pwr_in_dbm_configured = power;
	output_params->chip_output_pwr_in_dbm_expected = power;
}

static void stm32wl_get_ocp(const struct device *dev, uint8_t *ocp_in_step_of_2_5_ma)
{
	const struct stm32wl_driver_config *config = dev->config;

	/* 60 mA on the low-power output, 140 mA on the high-power one. */
	*ocp_in_step_of_2_5_ma = config->pa_output == SX126X_PA_OUTPUT_RFO_LP ? 0x18 : 0x38;
}

static const struct lbm_sx126x_ops stm32wl_ops = {
	.variant_init = stm32wl_variant_init,
	.reset = stm32wl_reset,
	.is_busy = stm32wl_is_busy,
	.get_tx_cfg = stm32wl_get_tx_cfg,
	.get_ocp = stm32wl_get_ocp,
	.dio1_irq_set = stm32wl_dio1_irq_set,
};

#define STM32WL_DEFINE(node_id)                                                                    \
	static const struct stm32wl_driver_config config_##node_id = {                             \
		.common = {LBM_SX126X_CONFIG_COMMON(node_id), .ops = &stm32wl_ops},                \
		.pa_output = DT_ENUM_IDX(node_id, power_amplifier_output),                         \
		.rfo_lp_max_power = DT_PROP(node_id, rfo_lp_max_power),                            \
		.rfo_hp_max_power = DT_PROP(node_id, rfo_hp_max_power),                            \
	};                                                                                         \
	static struct lbm_sx126x_data data_##node_id;                                              \
	DEVICE_DT_DEFINE(node_id, lbm_sx126x_init, NULL, &data_##node_id, &config_##node_id,       \
			 POST_KERNEL, CONFIG_LORA_INIT_PRIORITY, &lbm_lora_api)

DT_FOREACH_STATUS_OKAY(st_stm32wl_subghz_radio, STM32WL_DEFINE);

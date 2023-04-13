/*
 * Copyright (c) 2021 Fabio Baltieri
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "sx126x_common.h"

#include <stm32wlxx_ll_exti.h>
#include <stm32wlxx_ll_pwr.h>
#include <stm32wlxx_ll_rcc.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_DECLARE(sx126x, CONFIG_LORA_LOG_LEVEL);

static const enum {
	RFO_LP,
	RFO_HP,
} pa_output = DT_INST_STRING_UPPER_TOKEN(0, power_amplifier_output);

void sx126x_reset(struct sx126x_data *dev_data)
{
	LL_RCC_RF_EnableReset();
	k_sleep(K_MSEC(20));
	LL_RCC_RF_DisableReset();
	k_sleep(K_MSEC(10));
}

bool sx126x_is_busy(struct sx126x_data *dev_data)
{
	return LL_PWR_IsActiveFlag_RFBUSYS();
}

uint32_t sx126x_get_dio1_pin_state(struct sx126x_data *dev_data)
{
	return 0;
}

void sx126x_dio1_irq_enable(struct sx126x_data *dev_data)
{
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
	irq_enable(DT_INST_IRQN(0));
}

void sx126x_dio1_irq_disable(struct sx126x_data *dev_data)
{
	irq_disable(DT_INST_IRQN(0));
}

void sx126x_set_tx_params(int8_t power, RadioRampTimes_t ramp_time)
{
	uint8_t buf[2];

	if (pa_output == RFO_LP) {
		const int8_t max_power = DT_INST_PROP(0, rfo_lp_max_power);

		if (power > max_power) {
			power = max_power;
		}
		if (max_power == 15) {
			SX126xSetPaConfig(0x07, 0x00, 0x01, 0x01);
			power = 14 - (max_power - power);
		} else if (max_power == 10) {
			SX126xSetPaConfig(0x01, 0x00, 0x01, 0x01);
			power = 13 - (max_power - power);
		} else { /* default +14 dBm */
			SX126xSetPaConfig(0x04, 0x00, 0x01, 0x01);
			power = 14 - (max_power - power);
		}
		if (power < -17) {
			power = -17;
		}

		/* PA overcurrent protection limit 60 mA */
		SX126xWriteRegister(REG_OCP, 0x18);
	} else { /* RFO_HP */
		/* Better Resistance of the RFO High Power Tx to Antenna
		 * Mismatch, see STM32WL Erratasheet
		 */
		SX126xWriteRegister(REG_TX_CLAMP_CFG,
				    SX126xReadRegister(REG_TX_CLAMP_CFG)
					| (0x0F << 1));

		const int8_t max_power = DT_INST_PROP(0, rfo_hp_max_power);

		if (power > max_power) {
			power = max_power;
		}
		if (max_power == 20) {
			SX126xSetPaConfig(0x03, 0x05, 0x00, 0x01);
			power = 22 - (max_power - power);
		} else if (max_power == 17) {
			SX126xSetPaConfig(0x02, 0x03, 0x00, 0x01);
			power = 22 - (max_power - power);
		} else if (max_power == 14) {
			SX126xSetPaConfig(0x02, 0x02, 0x00, 0x01);
			power = 14 - (max_power - power);
		} else { /* default +22 dBm */
			SX126xSetPaConfig(0x04, 0x07, 0x00, 0x01);
			power = 22 - (max_power - power);
		}
		if (power < -9) {
			power = -9;
		}

		/* PA overcurrent protection limit 140 mA */
		SX126xWriteRegister(REG_OCP, 0x38);
	}

	buf[0] = power;
	buf[1] = (uint8_t)ramp_time;
	SX126xWriteCommand(RADIO_SET_TXPARAMS, buf, 2);
}

static void radio_isr(const struct device *dev)
{
	struct sx126x_data *dev_data = dev->data;

	irq_disable(DT_INST_IRQN(0));
	k_work_submit(&dev_data->dio1_irq_work);
}

int sx126x_variant_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    radio_isr, DEVICE_DT_INST_GET(0), 0);
	LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_44);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>

#include <stm32_ll_exti.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>

#include "sx126x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx126x_hal_stm32wl, CONFIG_LORA_LOG_LEVEL);

#define SX126X_RESET_PULSE_MS       20
#define SX126X_RESET_WAIT_MS        10
#define SX126X_BUSY_DEFAULT_TIMEOUT 1000

#define SX126X_OCP_60MA             0x18
#define SX126X_OCP_140MA            0x38

#define DT_DRV_COMPAT st_stm32wl_subghz_radio

static inline struct sx126x_hal_data *get_hal_data(const struct device *dev)
{
	struct sx126x_data *data = dev->data;

	return &data->hal;
}

static int sx126x_hal_wakeup(const struct device *dev)
{
	const struct sx126x_hal_config *config = dev->config;
	uint8_t tx_buf[2] = { SX126X_CMD_GET_STATUS, 0x00 };

	struct spi_buf tx_spi_buf = {
		.buf = tx_buf,
		.len = sizeof(tx_buf),
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_spi_buf,
		.count = 1,
	};

	return spi_write_dt(&config->spi, &tx_set);
}

int sx126x_hal_reset(const struct device *dev)
{
	int ret;

	/* Use RCC to reset the radio subsystem */
	LL_RCC_RF_EnableReset();
	k_msleep(SX126X_RESET_PULSE_MS);

	LL_RCC_RF_DisableReset();
	k_msleep(SX126X_RESET_WAIT_MS);

	/*
	 * After RCC reset, the radio is in sleep mode. Send a wakeup command
	 * to allow the busy flag to be properly checked.
	 */
	ret = sx126x_hal_wakeup(dev);
	if (ret < 0) {
		LOG_ERR("Wakeup failed: %d", ret);
		return ret;
	}

	/* Wait for chip to be ready */
	ret = sx126x_hal_wait_busy(dev, SX126X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Reset complete");
	return 0;
}

bool sx126x_hal_is_busy(const struct device *dev)
{
	ARG_UNUSED(dev);

	return LL_PWR_IsActiveFlag_RFBUSYS() != 0;
}

static void radio_isr(const struct device *dev)
{
	struct sx126x_hal_data *data = get_hal_data(dev);

	irq_disable(DT_INST_IRQN(0));

	if (data->dio1_callback != NULL) {
		data->dio1_callback(data->dev);
	}
}

int sx126x_hal_set_dio1_callback(const struct device *dev,
				 void (*callback)(const struct device *dev))
{
	struct sx126x_hal_data *data = get_hal_data(dev);

	data->dio1_callback = callback;

	if (callback != NULL) {
		NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
		irq_enable(DT_INST_IRQN(0));
	} else {
		irq_disable(DT_INST_IRQN(0));
	}

	return 0;
}

void sx126x_hal_dio1_irq_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

	irq_enable(DT_INST_IRQN(0));
}

static int sx126x_hal_set_pa_config(const struct device *dev, uint8_t pa_duty_cycle,
				    uint8_t hp_max, uint8_t device_sel, uint8_t pa_lut)
{
	uint8_t buf[4] = { pa_duty_cycle, hp_max, device_sel, pa_lut };

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_PA_CONFIG, buf, 4);
}

int sx126x_hal_configure_tx_params(const struct device *dev, int8_t power,
				   uint32_t frequency, uint8_t ramp_time)
{
	const struct sx126x_hal_config *config = dev->config;
	int8_t tx_power = power;
	uint8_t ocp_value;
	int ret;

	ARG_UNUSED(frequency);

	if (config->pa_output == SX126X_PA_OUTPUT_RFO_LP) {
		/* RFO_LP (Low Power) output */
		const int8_t max_power = config->rfo_lp_max_power;

		if (tx_power > max_power) {
			tx_power = max_power;
		}

		if (max_power == 15) {
			ret = sx126x_hal_set_pa_config(dev, 0x07, 0x00, 0x01, 0x01);
			tx_power = 14 - (max_power - tx_power);
		} else if (max_power == 10) {
			ret = sx126x_hal_set_pa_config(dev, 0x01, 0x00, 0x01, 0x01);
			tx_power = 13 - (max_power - tx_power);
		} else {
			/* Default: +14 dBm */
			ret = sx126x_hal_set_pa_config(dev, 0x04, 0x00, 0x01, 0x01);
			tx_power = 14 - (max_power - tx_power);
		}

		if (ret < 0) {
			return ret;
		}

		if (tx_power < -17) {
			tx_power = -17;
		}

		/* PA overcurrent protection limit 60 mA for RFO_LP */
		ocp_value = SX126X_OCP_60MA;
	} else {
		/* RFO_HP (High Power) output */
		const int8_t max_power = config->rfo_hp_max_power;
		uint8_t reg_val;

		if (tx_power > max_power) {
			tx_power = max_power;
		}

		ret = sx126x_hal_read_regs(dev, SX126X_REG_TX_CLAMP_CFG, &reg_val, 1);
		if (ret < 0) {
			return ret;
		}
		reg_val |= (0x0F << 1);
		ret = sx126x_hal_write_regs(dev, SX126X_REG_TX_CLAMP_CFG, &reg_val, 1);
		if (ret < 0) {
			return ret;
		}

		if (max_power == 20) {
			ret = sx126x_hal_set_pa_config(dev, 0x03, 0x05, 0x00, 0x01);
			tx_power = 22 - (max_power - tx_power);
		} else if (max_power == 17) {
			ret = sx126x_hal_set_pa_config(dev, 0x02, 0x03, 0x00, 0x01);
			tx_power = 22 - (max_power - tx_power);
		} else if (max_power == 14) {
			ret = sx126x_hal_set_pa_config(dev, 0x02, 0x02, 0x00, 0x01);
			tx_power = 14 - (max_power - tx_power);
		} else {
			/* Default: +22 dBm */
			ret = sx126x_hal_set_pa_config(dev, 0x04, 0x07, 0x00, 0x01);
			tx_power = 22 - (max_power - tx_power);
		}

		if (ret < 0) {
			return ret;
		}

		if (tx_power < -9) {
			tx_power = -9;
		}

		/* PA overcurrent protection limit 140 mA for RFO_HP */
		ocp_value = SX126X_OCP_140MA;
	}

	/* Set OCP value */
	ret = sx126x_hal_write_regs(dev, SX126X_REG_OCP, &ocp_value, 1);
	if (ret < 0) {
		return ret;
	}

	/* Set TX params */
	uint8_t buf[2] = { (uint8_t)tx_power, ramp_time };

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_TX_PARAMS, buf, 2);
}

int sx126x_hal_init(const struct device *dev)
{
	const struct sx126x_hal_config *config = dev->config;
	struct sx126x_hal_data *data = get_hal_data(dev);
	int ret;

	data->dev = dev;
	data->dio1_callback = NULL;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	/* Setup radio IRQ (EXTI line 44) */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    radio_isr, DEVICE_DT_INST_GET(0), 0);
	LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_44);

	/* Configure optional GPIOs */
	ret = sx126x_hal_configure_gpio(&config->antenna_enable, GPIO_OUTPUT_INACTIVE,
					"antenna enable");
	if (ret < 0) {
		return ret;
	}

	ret = sx126x_hal_configure_gpio(&config->tx_enable, GPIO_OUTPUT_INACTIVE,
					"TX enable");
	if (ret < 0) {
		return ret;
	}

	ret = sx126x_hal_configure_gpio(&config->rx_enable, GPIO_OUTPUT_INACTIVE,
					"RX enable");
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("STM32WL HAL initialized");
	return 0;
}

/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/byteorder.h>

#include "sx126x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx126x, CONFIG_LORA_LOG_LEVEL);

static uint8_t bandwidth_to_reg(enum lora_signal_bandwidth bw)
{
	switch (bw) {
	case BW_7_KHZ:
		return SX126X_LORA_BW_7_8;
	case BW_10_KHZ:
		return SX126X_LORA_BW_10_4;
	case BW_15_KHZ:
		return SX126X_LORA_BW_15_6;
	case BW_20_KHZ:
		return SX126X_LORA_BW_20_8;
	case BW_31_KHZ:
		return SX126X_LORA_BW_31_25;
	case BW_41_KHZ:
		return SX126X_LORA_BW_41_7;
	case BW_62_KHZ:
		return SX126X_LORA_BW_62_5;
	case BW_125_KHZ:
		return SX126X_LORA_BW_125;
	case BW_250_KHZ:
		return SX126X_LORA_BW_250;
	case BW_500_KHZ:
		return SX126X_LORA_BW_500;
	default:
		return SX126X_LORA_BW_125;
	}
}

static uint32_t bandwidth_to_hz(enum lora_signal_bandwidth bw)
{
	switch (bw) {
	case BW_7_KHZ:
		return 7810;
	case BW_10_KHZ:
		return 10420;
	case BW_15_KHZ:
		return 15630;
	case BW_20_KHZ:
		return 20830;
	case BW_31_KHZ:
		return 31250;
	case BW_41_KHZ:
		return 41670;
	case BW_62_KHZ:
		return 62500;
	case BW_125_KHZ:
		return 125000;
	case BW_250_KHZ:
		return 250000;
	case BW_500_KHZ:
		return 500000;
	default:
		return 125000;
	}
}

static bool should_enable_ldro(enum lora_datarate sf, enum lora_signal_bandwidth bw,
			       const struct sx126x_hal_config *config)
{
	if (config->force_ldro) {
		return true;
	}

	uint32_t bw_hz = bandwidth_to_hz(bw);
	/* Symbol time = 2^SF / BW (in seconds) */
	/* 16.38 ms = 16380 us */
	/* 2^SF / BW > 0.01638 => 2^SF * 1000000 / BW > 16380 */
	uint32_t symbol_time_us = ((1 << sf) * 1000000UL) / bw_hz;

	return symbol_time_us > 16380;
}

static int sx126x_set_standby(const struct device *dev, uint8_t mode)
{
	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_STANDBY, &mode, 1);
}

static int sx126x_set_regulator_mode(const struct device *dev, uint8_t mode)
{
	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_REGULATOR_MODE, &mode, 1);
}

static int sx126x_set_buffer_base_address(const struct device *dev,
					  uint8_t tx_base, uint8_t rx_base)
{
	uint8_t buf[2] = { tx_base, rx_base };

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_BUFFER_BASE_ADDRESS, buf, 2);
}

static int sx126x_set_packet_type(const struct device *dev, uint8_t type)
{
	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_PACKET_TYPE, &type, 1);
}

static int sx126x_set_dio_irq_params(const struct device *dev,
				     uint16_t irq_mask, uint16_t dio1_mask,
				     uint16_t dio2_mask, uint16_t dio3_mask)
{
	uint8_t buf[8];

	sys_put_be16(irq_mask, &buf[0]);
	sys_put_be16(dio1_mask, &buf[2]);
	sys_put_be16(dio2_mask, &buf[4]);
	sys_put_be16(dio3_mask, &buf[6]);

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_DIO_IRQ_PARAMS, buf, 8);
}

static int sx126x_clear_irq_status(const struct device *dev, uint16_t mask)
{
	uint8_t buf[2];

	sys_put_be16(mask, buf);
	return sx126x_hal_write_cmd(dev, SX126X_CMD_CLR_IRQ_STATUS, buf, 2);
}

static int sx126x_get_irq_status(const struct device *dev, uint16_t *status)
{
	uint8_t buf[2];
	int ret;

	ret = sx126x_hal_read_cmd(dev, SX126X_CMD_GET_IRQ_STATUS, buf, 2);
	if (ret == 0) {
		*status = ((uint16_t)buf[0] << 8) | buf[1];
	}

	return ret;
}

static int sx126x_set_dio2_as_rf_switch(const struct device *dev, bool enable)
{
	uint8_t val = enable;

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_DIO2_AS_RF_SWITCH, &val, 1);
}

static int sx126x_set_dio3_as_tcxo_ctrl(const struct device *dev,
					uint8_t voltage, uint32_t timeout_ms)
{
	/* Timeout in units of 15.625 us */
	uint32_t timeout = SX126X_MS_TO_TIMEOUT(timeout_ms);
	uint8_t buf[4];

	buf[0] = voltage;
	sys_put_be24(timeout, &buf[1]);

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, buf, 4);
}

static int sx126x_calibrate(const struct device *dev, uint8_t mask)
{
	return sx126x_hal_write_cmd(dev, SX126X_CMD_CALIBRATE, &mask, 1);
}

static int sx126x_calibrate_image(const struct device *dev, uint32_t freq)
{
	uint8_t buf[2];

	if (freq > 900000000) {
		buf[0] = 0xE1;
		buf[1] = 0xE9;
	} else if (freq > 850000000) {
		buf[0] = 0xD7;
		buf[1] = 0xDB;
	} else if (freq > 770000000) {
		buf[0] = 0xC1;
		buf[1] = 0xC5;
	} else if (freq > 460000000) {
		buf[0] = 0x75;
		buf[1] = 0x81;
	} else {
		buf[0] = 0x6B;
		buf[1] = 0x6F;
	}

	return sx126x_hal_write_cmd(dev, SX126X_CMD_CALIBRATE_IMAGE, buf, 2);
}

static int sx126x_set_rf_frequency(const struct device *dev, uint32_t freq)
{
	uint32_t freq_reg = SX126X_FREQ_TO_REG(freq);
	uint8_t buf[4];

	sys_put_be32(freq_reg, buf);

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_RF_FREQUENCY, buf, 4);
}

static int sx126x_set_pa_config(const struct device *dev, uint8_t pa_duty_cycle,
				uint8_t hp_max, uint8_t device_sel, uint8_t pa_lut)
{
	uint8_t buf[4] = { pa_duty_cycle, hp_max, device_sel, pa_lut };

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_PA_CONFIG, buf, 4);
}

static int sx126x_configure_pa_and_tx_params(const struct device *dev,
					     int8_t power, uint32_t frequency,
					     uint8_t ramp_time)
{
	const struct sx126x_hal_config *config = dev->config;
	uint8_t pa_duty_cycle;
	int8_t tx_power;
	int ret;

	if (config->is_sx1261) {
		/*
		 * SX1261: Low power PA, up to +15 dBm
		 * For +15 dBm at >400 MHz, use higher paDutyCycle.
		 * For lower power, use lower paDutyCycle for efficiency.
		 */
		pa_duty_cycle = (power >= SX1261_MAX_POWER && frequency >= 400000000)
				? SX1261_PA_DUTY_CYCLE_HIGH
				: SX1261_PA_DUTY_CYCLE_LOW;
		ret = sx126x_set_pa_config(dev, pa_duty_cycle, SX1261_HP_MAX,
					   SX126X_DEVICE_SEL_SX1261,
					   SX126X_PA_LUT);
		if (ret < 0) {
			return ret;
		}
		tx_power = CLAMP(power, SX1261_MIN_POWER, SX1261_MAX_POWER_TX_PARAM);
	} else {
		/* SX1262: High power PA, up to +22 dBm */
		ret = sx126x_set_pa_config(dev, SX1262_PA_DUTY_CYCLE,
					   SX1262_HP_MAX,
					   SX126X_DEVICE_SEL_SX1262,
					   SX126X_PA_LUT);
		if (ret < 0) {
			return ret;
		}
		tx_power = CLAMP(power, SX1262_MIN_POWER, SX1262_MAX_POWER);
	}

	uint8_t buf[2] = { (uint8_t)tx_power, ramp_time };

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_TX_PARAMS, buf, 2);
}

static int sx126x_set_modulation_params(const struct device *dev,
					uint8_t sf, uint8_t bw, uint8_t cr,
					bool ldro)
{
	uint8_t buf[4] = { sf, bw, cr, ldro };

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_MODULATION_PARAMS, buf, 4);
}

static int sx126x_set_packet_params(const struct device *dev,
				    uint16_t preamble_len, uint8_t header_type,
				    uint8_t payload_len, uint8_t crc_mode,
				    uint8_t invert_iq)
{
	uint8_t buf[6];

	sys_put_be16(preamble_len, &buf[0]);
	buf[2] = header_type;
	buf[3] = payload_len;
	buf[4] = crc_mode;
	buf[5] = invert_iq;

	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_PACKET_PARAMS, buf, 6);
}

static int sx126x_set_sync_word(const struct device *dev, bool public_network)
{
	uint16_t sync_word = public_network ?
			     SX126X_LORA_SYNC_WORD_PUBLIC :
			     SX126X_LORA_SYNC_WORD_PRIVATE;
	uint8_t buf[2];

	sys_put_be16(sync_word, buf);
	return sx126x_hal_write_regs(dev, SX126X_REG_LORA_SYNC_WORD_MSB, buf, 2);
}

static int sx126x_set_rx_gain(const struct device *dev, bool boosted)
{
	uint8_t val = boosted ? SX126X_RX_GAIN_BOOSTED : SX126X_RX_GAIN_POWER_SAVING;

	return sx126x_hal_write_regs(dev, SX126X_REG_RX_GAIN, &val, 1);
}

static int sx126x_set_tx(const struct device *dev, uint32_t timeout_ms)
{
	uint32_t timeout = SX126X_MS_TO_TIMEOUT(timeout_ms);
	uint8_t buf[3];

	sys_put_be24(timeout, buf);
	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_TX, buf, 3);
}

static int sx126x_set_rx(const struct device *dev, uint32_t timeout_ms)
{
	uint32_t timeout;

	if (timeout_ms == 0) {
		timeout = SX126X_RX_TIMEOUT_CONTINUOUS;
	} else {
		timeout = SX126X_MS_TO_TIMEOUT(timeout_ms);
	}

	uint8_t buf[3];

	sys_put_be24(timeout, buf);
	return sx126x_hal_write_cmd(dev, SX126X_CMD_SET_RX, buf, 3);
}

static int sx126x_get_rx_buffer_status(const struct device *dev,
				       uint8_t *payload_len, uint8_t *offset)
{
	uint8_t buf[2];
	int ret;

	ret = sx126x_hal_read_cmd(dev, SX126X_CMD_GET_RX_BUFFER_STATUS, buf, 2);
	if (ret == 0) {
		*payload_len = buf[0];
		*offset = buf[1];
	}

	return ret;
}

static int sx126x_get_packet_status(const struct device *dev,
				    int16_t *rssi, int8_t *snr)
{
	uint8_t buf[3];
	int ret;

	ret = sx126x_hal_read_cmd(dev, SX126X_CMD_GET_PACKET_STATUS, buf, 2);
	if (ret == 0) {
		/* RSSI is -value/2 dBm */
		*rssi = -((int16_t)buf[0] >> 1);
		/* SNR is value/4 dB (signed) */
		*snr = ((int8_t)buf[1]) >> 2;
	}

	return ret;
}

static int sx126x_chip_init(const struct device *dev)
{
	const struct sx126x_hal_config *config = dev->config;
	int ret;

	/* Hardware reset */
	ret = sx126x_hal_reset(dev);
	if (ret < 0) {
		LOG_ERR("Reset failed: %d", ret);
		return ret;
	}

	/* Set standby mode */
	ret = sx126x_set_standby(dev, SX126X_STANDBY_RC);
	if (ret < 0) {
		LOG_ERR("Set standby failed: %d", ret);
		return ret;
	}

	/* Configure TCXO if enabled */
	if (config->dio3_tcxo_enable) {
		ret = sx126x_set_dio3_as_tcxo_ctrl(dev, config->dio3_tcxo_voltage,
						   config->tcxo_startup_delay_ms);
		if (ret < 0) {
			LOG_ERR("Set TCXO failed: %d", ret);
			return ret;
		}

		/* Run full calibration after TCXO setup */
		ret = sx126x_calibrate(dev, SX126X_CALIBRATE_ALL);
		if (ret < 0) {
			LOG_ERR("Calibration failed: %d", ret);
			return ret;
		}
	}

	/* Configure DIO2 as RF switch if enabled */
	if (config->dio2_tx_enable) {
		ret = sx126x_set_dio2_as_rf_switch(dev, true);
		if (ret < 0) {
			LOG_ERR("Set DIO2 RF switch failed: %d", ret);
			return ret;
		}
	}

	/* Set regulator mode */
	ret = sx126x_set_regulator_mode(dev, config->regulator_ldo ?
					SX126X_REGULATOR_LDO : SX126X_REGULATOR_DCDC);
	if (ret < 0) {
		LOG_ERR("Set regulator failed: %d", ret);
		return ret;
	}

	/* Set buffer base addresses */
	ret = sx126x_set_buffer_base_address(dev, 0x00, 0x00);
	if (ret < 0) {
		LOG_ERR("Set buffer base failed: %d", ret);
		return ret;
	}

	/* Set packet type to LoRa */
	ret = sx126x_set_packet_type(dev, SX126X_PACKET_TYPE_LORA);
	if (ret < 0) {
		LOG_ERR("Set packet type failed: %d", ret);
		return ret;
	}

	/* Configure IRQs on DIO1: TX done, RX done, timeout */
	uint16_t irq_mask = SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE |
			    SX126X_IRQ_RX_TX_TIMEOUT | SX126X_IRQ_CRC_ERR;
	ret = sx126x_set_dio_irq_params(dev, irq_mask, irq_mask, 0, 0);
	if (ret < 0) {
		LOG_ERR("Set IRQ params failed: %d", ret);
		return ret;
	}

	/* Clear any pending IRQs */
	ret = sx126x_clear_irq_status(dev, SX126X_IRQ_ALL);
	if (ret < 0) {
		LOG_ERR("Clear IRQ failed: %d", ret);
		return ret;
	}

	LOG_INF("SX126x initialized");
	return 0;
}

static void sx126x_dio1_callback(const struct device *dev)
{
	struct sx126x_data *data = dev->data;

	k_work_submit(&data->irq_work);
}

static void sx126x_set_rf_path(const struct device *dev, bool enable, bool tx)
{
	const struct sx126x_hal_config *config = dev->config;

	sx126x_hal_set_antenna_enable(dev, enable);
	if (!config->dio2_tx_enable) {
		sx126x_hal_set_rf_switch(dev, enable && tx);
	}
}

static void sx126x_handle_irq_tx_done(const struct device *dev)
{
	struct sx126x_data *data = dev->data;
	struct sx126x_tx_result result = { .status = 0 };

	LOG_DBG("TX done");
	atomic_set(&data->state, SX126X_STATE_IDLE);
	sx126x_set_rf_path(dev, false, false);

	if (data->tx_async_signal != NULL) {
		k_poll_signal_raise(data->tx_async_signal, 0);
		data->tx_async_signal = NULL;
	}
	k_msgq_put(&data->tx_msgq, &result, K_NO_WAIT);
}

static void sx126x_handle_irq_rx_done(const struct device *dev, uint16_t irq_status)
{
	struct sx126x_data *data = dev->data;
	struct sx126x_rx_result result = { 0 };
	uint8_t payload_len = 0, offset = 0;
	int ret;

	/* Get received packet info */
	ret = sx126x_get_rx_buffer_status(dev, &payload_len, &offset);
	if (ret < 0) {
		LOG_ERR("Failed to get RX buffer status");
		result.status = ret;
	} else {
		/* Get signal quality */
		sx126x_get_packet_status(dev, &result.rssi, &result.snr);

		/* Check for CRC error */
		if (irq_status & SX126X_IRQ_CRC_ERR) {
			LOG_WRN("CRC error");
			result.status = -EIO;
		} else {
			/* Read payload into shared buffer */
			result.len = MIN(payload_len, sizeof(data->rx_buf));
			ret = sx126x_hal_read_buffer(dev, offset,
						     data->rx_buf, result.len);
			if (ret < 0) {
				LOG_ERR("Failed to read RX buffer");
				result.status = ret;
			} else {
				result.status = result.len;
				LOG_DBG("RX done: %d bytes, RSSI=%d, SNR=%d",
					result.len, result.rssi, result.snr);
			}
		}
	}

	/* Handle async callback or signal sync receiver */
	if (data->rx_cb != NULL) {
		/* Async mode - call callback and restart RX */
		data->rx_cb(dev, data->rx_buf, result.len,
			    result.rssi, result.snr,
			    data->rx_cb_user_data);
		/* Restart RX for continuous reception */
		sx126x_set_rx(dev, 0);
	} else {
		/* Sync mode */
		atomic_set(&data->state, SX126X_STATE_IDLE);
		sx126x_set_rf_path(dev, false, false);
		k_msgq_put(&data->rx_msgq, &result, K_NO_WAIT);
	}
}

static void sx126x_handle_irq_timeout(const struct device *dev)
{
	struct sx126x_data *data = dev->data;

	LOG_DBG("Timeout");
	atomic_set(&data->state, SX126X_STATE_IDLE);
	sx126x_set_rf_path(dev, false, false);

	if (data->tx_async_signal != NULL) {
		struct sx126x_tx_result result = { .status = -ETIMEDOUT };

		k_poll_signal_raise(data->tx_async_signal, -ETIMEDOUT);
		data->tx_async_signal = NULL;
		k_msgq_put(&data->tx_msgq, &result, K_NO_WAIT);
	} else if (data->rx_cb == NULL) {
		/* Sync RX timeout */
		struct sx126x_rx_result result = { .status = -EAGAIN };

		k_msgq_put(&data->rx_msgq, &result, K_NO_WAIT);
	}
}

static void sx126x_irq_work_handler(struct k_work *work)
{
	struct sx126x_data *data = CONTAINER_OF(work, struct sx126x_data, irq_work);
	const struct device *dev = data->dev;
	uint16_t irq_status = 0;
	int ret;

	ret = sx126x_get_irq_status(dev, &irq_status);
	if (ret < 0) {
		LOG_ERR("Failed to get IRQ status");
		return;
	}

	LOG_DBG("IRQ status: 0x%04x", irq_status);

	/* Clear handled IRQs */
	sx126x_clear_irq_status(dev, irq_status);

	if (irq_status & SX126X_IRQ_TX_DONE) {
		sx126x_handle_irq_tx_done(dev);
	}

	if (irq_status & SX126X_IRQ_RX_DONE) {
		sx126x_handle_irq_rx_done(dev, irq_status);
	}

	if (irq_status & SX126X_IRQ_RX_TX_TIMEOUT) {
		sx126x_handle_irq_timeout(dev);
	}
}

static int sx126x_lora_config(const struct device *dev,
			      struct lora_modem_config *config)
{
	struct sx126x_data *data = dev->data;
	const struct sx126x_hal_config *hal_config = dev->config;
	bool ldro;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Store configuration */
	memcpy(&data->config, config, sizeof(*config));

	/* Run image calibration for frequency band */
	ret = sx126x_calibrate_image(dev, config->frequency);
	if (ret < 0) {
		goto out;
	}

	/* Set RF frequency */
	ret = sx126x_set_rf_frequency(dev, config->frequency);
	if (ret < 0) {
		goto out;
	}

	/* Configure PA and TX power based on chip variant and frequency */
	ret = sx126x_configure_pa_and_tx_params(dev, config->tx_power,
						config->frequency,
						SX126X_RAMP_200_US);
	if (ret < 0) {
		goto out;
	}

	/* Set modulation parameters */
	ldro = should_enable_ldro(config->datarate, config->bandwidth, hal_config);
	ret = sx126x_set_modulation_params(dev,
					   config->datarate,
					   bandwidth_to_reg(config->bandwidth),
					   config->coding_rate,
					   ldro);
	if (ret < 0) {
		goto out;
	}

	/* Set sync word */
	ret = sx126x_set_sync_word(dev, config->public_network);
	if (ret < 0) {
		goto out;
	}

	/* Set RX gain */
	ret = sx126x_set_rx_gain(dev, hal_config->rx_boosted);
	if (ret < 0) {
		goto out;
	}

	data->config_valid = true;
	LOG_DBG("Config: freq=%u, SF=%d, BW=%d, CR=%d, power=%d",
		config->frequency, config->datarate, config->bandwidth,
		config->coding_rate, config->tx_power);

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int sx126x_lora_send_async(const struct device *dev,
				  uint8_t *data_buf, uint32_t data_len,
				  struct k_poll_signal *async)
{
	struct sx126x_data *data = dev->data;
	int ret;

	if (!data->config_valid) {
		LOG_ERR("Not configured");
		return -EINVAL;
	}

	if (data_len > SX126X_MAX_PAYLOAD_LEN) {
		LOG_ERR("Payload too long: %u", data_len);
		return -EINVAL;
	}

	if (!atomic_cas(&data->state, SX126X_STATE_IDLE, SX126X_STATE_TX)) {
		LOG_ERR("Busy");
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->tx_async_signal = async;
	k_msgq_purge(&data->tx_msgq);

	/* Set packet parameters */
	ret = sx126x_set_packet_params(dev,
				       data->config.preamble_len,
				       SX126X_LORA_HEADER_EXPLICIT,
				       data_len,
				       data->config.packet_crc_disable ?
				       SX126X_LORA_CRC_OFF : SX126X_LORA_CRC_ON,
				       data->config.iq_inverted ?
				       SX126X_LORA_IQ_INVERTED : SX126X_LORA_IQ_STANDARD);
	if (ret < 0) {
		goto out_error;
	}

	/* Write payload to buffer */
	ret = sx126x_hal_write_buffer(dev, 0x00, data_buf, data_len);
	if (ret < 0) {
		goto out_error;
	}

	/* Enable antenna and set TX path */
	sx126x_set_rf_path(dev, true, true);

	/* Start transmission with 10 second timeout */
	ret = sx126x_set_tx(dev, 10000);
	if (ret < 0) {
		goto out_error;
	}

	k_mutex_unlock(&data->lock);
	return 0;

out_error:
	data->tx_async_signal = NULL;
	k_mutex_unlock(&data->lock);
	atomic_set(&data->state, SX126X_STATE_IDLE);
	return ret;
}

static int sx126x_lora_send(const struct device *dev,
			    uint8_t *data_buf, uint32_t data_len)
{
	struct sx126x_data *data = dev->data;
	struct sx126x_tx_result result;
	int ret;

	ret = sx126x_lora_send_async(dev, data_buf, data_len, NULL);
	if (ret < 0) {
		return ret;
	}

	/* Wait for TX completion */
	ret = k_msgq_get(&data->tx_msgq, &result, K_SECONDS(15));
	if (ret < 0) {
		LOG_ERR("TX timeout");
		atomic_set(&data->state, SX126X_STATE_IDLE);
		return -ETIMEDOUT;
	}

	return result.status;
}

static int sx126x_lora_recv(const struct device *dev, uint8_t *data_buf,
			    uint8_t size, k_timeout_t timeout,
			    int16_t *rssi, int8_t *snr)
{
	struct sx126x_data *data = dev->data;
	struct sx126x_rx_result result;
	uint32_t timeout_ms;
	int ret;

	if (!data->config_valid) {
		LOG_ERR("Not configured");
		return -EINVAL;
	}

	if (!atomic_cas(&data->state, SX126X_STATE_IDLE, SX126X_STATE_RX)) {
		LOG_ERR("Busy");
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->rx_cb = NULL;
	k_msgq_purge(&data->rx_msgq);

	/* Set packet parameters for variable length reception */
	ret = sx126x_set_packet_params(dev,
				       data->config.preamble_len,
				       SX126X_LORA_HEADER_EXPLICIT,
				       SX126X_MAX_PAYLOAD_LEN,
				       data->config.packet_crc_disable ?
				       SX126X_LORA_CRC_OFF : SX126X_LORA_CRC_ON,
				       data->config.iq_inverted ?
				       SX126X_LORA_IQ_INVERTED : SX126X_LORA_IQ_STANDARD);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		atomic_set(&data->state, SX126X_STATE_IDLE);
		return ret;
	}

	/* Enable antenna and set RX path */
	sx126x_set_rf_path(dev, true, false);

	/* Start reception (0 = continuous for K_FOREVER) */
	timeout_ms = K_TIMEOUT_EQ(timeout, K_FOREVER)
		     ? 0 : k_ticks_to_ms_ceil32(timeout.ticks);
	ret = sx126x_set_rx(dev, timeout_ms);
	if (ret < 0) {
		sx126x_set_rf_path(dev, false, false);
		k_mutex_unlock(&data->lock);
		atomic_set(&data->state, SX126X_STATE_IDLE);
		return ret;
	}

	k_mutex_unlock(&data->lock);

	/* Wait for RX completion */
	ret = k_msgq_get(&data->rx_msgq, &result, timeout);
	if (ret < 0) {
		LOG_DBG("RX timeout");
		atomic_set(&data->state, SX126X_STATE_IDLE);
		sx126x_set_standby(dev, SX126X_STANDBY_RC);
		sx126x_set_rf_path(dev, false, false);
		return -EAGAIN;
	}

	/* Copy received data from shared buffer */
	if (result.status > 0) {
		int copy_len = MIN(result.status, size);

		memcpy(data_buf, data->rx_buf, copy_len);
		if (rssi != NULL) {
			*rssi = result.rssi;
		}
		if (snr != NULL) {
			*snr = result.snr;
		}
		return copy_len;
	}

	return result.status;
}

static int sx126x_lora_recv_async(const struct device *dev,
				  lora_recv_cb cb, void *user_data)
{
	struct sx126x_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (cb == NULL) {
		/* Stop async reception */
		data->rx_cb = NULL;
		data->rx_cb_user_data = NULL;
		if (atomic_cas(&data->state, SX126X_STATE_RX, SX126X_STATE_IDLE)) {
			sx126x_set_standby(dev, SX126X_STANDBY_RC);
			sx126x_set_rf_path(dev, false, false);
		}
		k_mutex_unlock(&data->lock);
		return 0;
	}

	if (!data->config_valid) {
		LOG_ERR("Not configured");
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	if (!atomic_cas(&data->state, SX126X_STATE_IDLE, SX126X_STATE_RX)) {
		LOG_ERR("Busy");
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	data->rx_cb = cb;
	data->rx_cb_user_data = user_data;

	/* Set packet parameters */
	ret = sx126x_set_packet_params(dev,
				       data->config.preamble_len,
				       SX126X_LORA_HEADER_EXPLICIT,
				       SX126X_MAX_PAYLOAD_LEN,
				       data->config.packet_crc_disable ?
				       SX126X_LORA_CRC_OFF : SX126X_LORA_CRC_ON,
				       data->config.iq_inverted ?
				       SX126X_LORA_IQ_INVERTED : SX126X_LORA_IQ_STANDARD);
	if (ret < 0) {
		data->rx_cb = NULL;
		k_mutex_unlock(&data->lock);
		atomic_set(&data->state, SX126X_STATE_IDLE);
		return ret;
	}

	/* Enable antenna and set RX path */
	sx126x_set_rf_path(dev, true, false);

	/* Start continuous reception */
	ret = sx126x_set_rx(dev, 0);
	if (ret < 0) {
		data->rx_cb = NULL;
		sx126x_set_rf_path(dev, false, false);
		k_mutex_unlock(&data->lock);
		atomic_set(&data->state, SX126X_STATE_IDLE);
		return ret;
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

static uint32_t sx126x_lora_airtime(const struct device *dev, uint32_t data_len)
{
	struct sx126x_data *data = dev->data;
	uint32_t t_preamble_us, t_payload_us, t_sym_us, n_payload, bw_hz;
	uint8_t sf, cr;
	int32_t tmp;
	bool de, crc;

	if (!data->config_valid) {
		return 0;
	}

	/* Calculate symbol time in microseconds */
	bw_hz = bandwidth_to_hz(data->config.bandwidth);
	sf = data->config.datarate;

	/* Symbol time = 2^SF / BW (seconds) */
	/* In microseconds: (2^SF * 1000000) / BW */
	t_sym_us = ((1UL << sf) * 1000000UL) / bw_hz;

	/* Preamble time (4.25 extra symbols) */
	t_preamble_us = (data->config.preamble_len + 4) * t_sym_us +
			(t_sym_us / 4);

	/* Payload symbol count calculation (from LoRa modem designer's guide) */
	de = should_enable_ldro(sf, data->config.bandwidth, dev->config);
	crc = !data->config.packet_crc_disable;
	cr = data->config.coding_rate;

	/* ih (implicit header) = false for explicit header mode */
	tmp = 8 * data_len - 4 * sf + 28 + 16 * crc;
	if (tmp < 0) {
		tmp = 0;
	}

	n_payload = 8 + (((tmp + 4 * (sf - 2 * de) - 1) /
			  (4 * (sf - 2 * de))) * (cr + 4));
	t_payload_us = n_payload * t_sym_us;

	/* Total airtime in milliseconds */
	return (t_preamble_us + t_payload_us + 500) / 1000;
}

static int sx126x_lora_test_cw(const struct device *dev, uint32_t frequency,
			       int8_t tx_power, uint16_t duration)
{
	struct sx126x_data *data = dev->data;
	int ret;

	if (atomic_get(&data->state) != SX126X_STATE_IDLE) {
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Set frequency */
	ret = sx126x_set_rf_frequency(dev, frequency);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/* Set PA config and TX power */
	ret = sx126x_configure_pa_and_tx_params(dev, tx_power, frequency,
						SX126X_RAMP_200_US);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/* Enable antenna and TX path */
	sx126x_set_rf_path(dev, true, true);

	/* Start CW transmission */
	ret = sx126x_hal_write_cmd(dev, SX126X_CMD_SET_TX_CONTINUOUS_WAVE, NULL, 0);
	if (ret < 0) {
		sx126x_set_rf_path(dev, false, false);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	k_mutex_unlock(&data->lock);

	/* Wait for duration */
	k_sleep(K_SECONDS(duration));

	/* Stop CW */
	k_mutex_lock(&data->lock, K_FOREVER);
	sx126x_set_standby(dev, SX126X_STANDBY_RC);
	sx126x_set_rf_path(dev, false, false);
	k_mutex_unlock(&data->lock);

	return 0;
}

static const struct lora_driver_api sx126x_lora_api = {
	.config = sx126x_lora_config,
	.send = sx126x_lora_send,
	.send_async = sx126x_lora_send_async,
	.recv = sx126x_lora_recv,
	.recv_async = sx126x_lora_recv_async,
	.airtime = sx126x_lora_airtime,
	.test_cw = sx126x_lora_test_cw,
};

static int sx126x_init(const struct device *dev)
{
	struct sx126x_data *data = dev->data;
	int ret;

	/* Initialize data structures */
	k_mutex_init(&data->lock);
	k_msgq_init(&data->tx_msgq, (char *)&data->tx_result,
		    sizeof(struct sx126x_tx_result), 1);
	k_msgq_init(&data->rx_msgq, (char *)&data->rx_result,
		    sizeof(struct sx126x_rx_result), 1);
	k_work_init(&data->irq_work, sx126x_irq_work_handler);
	data->dev = dev;
	atomic_set(&data->state, SX126X_STATE_IDLE);
	data->config_valid = false;

	/* Initialize HAL */
	ret = sx126x_hal_init(dev);
	if (ret < 0) {
		LOG_ERR("HAL init failed: %d", ret);
		return ret;
	}

	/* Setup DIO1 interrupt callback */
	ret = sx126x_hal_set_dio1_callback(dev, sx126x_dio1_callback);
	if (ret < 0) {
		LOG_ERR("DIO1 callback setup failed: %d", ret);
		return ret;
	}

	/* Initialize chip */
	ret = sx126x_chip_init(dev);
	if (ret < 0) {
		LOG_ERR("Chip init failed: %d", ret);
		return ret;
	}

	return 0;
}

#define SX126X_INIT(inst, is_1261)						\
	static struct sx126x_data sx126x_data_##inst;				\
										\
	static const struct sx126x_hal_config sx126x_config_##inst = {		\
		.spi = SPI_DT_SPEC_INST_GET(inst,				\
					    SPI_WORD_SET(8) | SPI_TRANSFER_MSB), \
		.reset = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),		\
		.busy = GPIO_DT_SPEC_INST_GET(inst, busy_gpios),		\
		.dio1 = GPIO_DT_SPEC_INST_GET(inst, dio1_gpios),		\
		.antenna_enable = GPIO_DT_SPEC_INST_GET_OR(inst,		\
							   antenna_enable_gpios, \
							   {0}),		\
		.tx_enable = GPIO_DT_SPEC_INST_GET_OR(inst, tx_enable_gpios,	\
						      {0}),			\
		.rx_enable = GPIO_DT_SPEC_INST_GET_OR(inst, rx_enable_gpios,	\
						      {0}),			\
		.is_sx1261 = is_1261,						\
		.dio2_tx_enable = DT_INST_PROP(inst, dio2_tx_enable),		\
		.dio3_tcxo_enable = DT_INST_NODE_HAS_PROP(inst, dio3_tcxo_voltage), \
		.dio3_tcxo_voltage = DT_INST_PROP_OR(inst, dio3_tcxo_voltage, 0), \
		.tcxo_startup_delay_ms = DT_INST_PROP_OR(inst,			\
						tcxo_power_startup_delay_ms, 10), \
		.rx_boosted = DT_INST_PROP(inst, rx_boosted),			\
		.regulator_ldo = DT_INST_PROP(inst, regulator_ldo),		\
		.force_ldro = DT_INST_PROP(inst, force_ldro),			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, sx126x_init, NULL,				\
			      &sx126x_data_##inst, &sx126x_config_##inst,	\
			      POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,		\
			      &sx126x_lora_api);

#define DT_DRV_COMPAT semtech_sx1262
DT_INST_FOREACH_STATUS_OKAY_VARGS(SX126X_INIT, false)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT semtech_sx1261
DT_INST_FOREACH_STATUS_OKAY_VARGS(SX126X_INIT, true)

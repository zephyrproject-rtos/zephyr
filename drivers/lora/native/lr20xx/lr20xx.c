/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/byteorder.h>

#include "lr20xx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lr20xx, CONFIG_LORA_LOG_LEVEL);

#define LR20XX_REST_STATE \
	(IS_ENABLED(CONFIG_LORA_LR20XX_NATIVE_SLEEP) \
	 ? LR20XX_STATE_SLEEP : LR20XX_STATE_IDLE)

/* LR20xx does not restrict bandwidths to bands */
static int bandwidth_to_reg(enum lora_signal_bandwidth bw, uint8_t *reg)
{
	switch (bw) {
	case BW_31_KHZ:
		*reg = LR20XX_LORA_BW_31_25;
		return 0;
	case BW_41_KHZ:
		*reg = LR20XX_LORA_BW_41_67;
		return 0;
	case BW_62_KHZ:
		*reg = LR20XX_LORA_BW_62_5;
		return 0;
	case BW_83_KHZ:
		*reg = LR20XX_LORA_BW_83_34;
		return 0;
	case BW_100_KHZ:
		*reg = LR20XX_LORA_BW_101_5625;
		return 0;
	case BW_125_KHZ:
		*reg = LR20XX_LORA_BW_125;
		return 0;
	case BW_200_KHZ:
		*reg = LR20XX_LORA_BW_203_125;
		return 0;
	case BW_250_KHZ:
		*reg = LR20XX_LORA_BW_250;
		return 0;
	case BW_400_KHZ:
		*reg = LR20XX_LORA_BW_406_25;
		return 0;
	case BW_500_KHZ:
		*reg = LR20XX_LORA_BW_500;
		return 0;
	case BW_800_KHZ:
		*reg = LR20XX_LORA_BW_812_5;
		return 0;
	case BW_1000_KHZ:
		*reg = LR20XX_LORA_BW_1000;
		return 0;
	default:
		return -EINVAL;
	}
}

static int bandwidth_to_hz(enum lora_signal_bandwidth bw, uint32_t *bw_hz)
{
	switch (bw) {
	case BW_7_KHZ:
		*bw_hz = 7810;
		return 0;
	case BW_10_KHZ:
		*bw_hz = 10420;
		return 0;
	case BW_15_KHZ:
		*bw_hz = 15630;
		return 0;
	case BW_20_KHZ:
		*bw_hz = 20830;
		return 0;
	case BW_31_KHZ:
		*bw_hz = 31250;
		return 0;
	case BW_41_KHZ:
		*bw_hz = 41670;
		return 0;
	case BW_62_KHZ:
		*bw_hz = 62500;
		return 0;
	case BW_83_KHZ:
		*bw_hz = 83340;
		return 0;
	case BW_100_KHZ:
		*bw_hz = 101562;
		return 0;
	case BW_125_KHZ:
		*bw_hz = 125000;
		return 0;
	case BW_200_KHZ:
		*bw_hz = 203125;
		return 0;
	case BW_250_KHZ:
		*bw_hz = 250000;
		return 0;
	case BW_400_KHZ:
		*bw_hz = 406250;
		return 0;
	case BW_500_KHZ:
		*bw_hz = 500000;
		return 0;
	case BW_800_KHZ:
		*bw_hz = 812500;
		return 0;
	case BW_1000_KHZ:
		*bw_hz = 1000000;
		return 0;
	case BW_1600_KHZ:
		*bw_hz = 1625000;
		return 0;
	default:
		return -EINVAL;
	}
}

static bool should_enable_ldro(enum lora_datarate sf, enum lora_signal_bandwidth bw,
			       const struct lr20xx_config *config)
{
	uint32_t bw_hz;
	int ret;

	if (config->force_ldro) {
		return true;
	}

	ret = bandwidth_to_hz(bw, &bw_hz);
	__ASSERT_NO_MSG(ret == 0);
	/* Symbol time = 2^SF / BW (in seconds) */
	/* 16.38 ms = 16380 us */
	/* 2^SF / BW > 0.01638 => 2^SF * 1000000 / BW > 16380 */
	uint32_t symbol_time_us = ((1 << sf) * 1000000UL) / bw_hz;

	return symbol_time_us > 16380;
}

static uint32_t lr20xx_lora_airtime(const struct device *dev, uint32_t data_len)
{
	struct lr20xx_data *data = dev->data;
	uint32_t t_preamble_us, t_payload_us, t_sym_us, n_payload, bw_hz;
	uint8_t sf, cr, hdr_len;
	int32_t tmp;
	bool de, crc;
	int ret;

	if (!data->config_valid) {
		return 0;
	}

	/* Calculate symbol time in microseconds */
	ret = bandwidth_to_hz(data->config.bandwidth, &bw_hz);
	__ASSERT_NO_MSG(ret == 0);
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
	hdr_len = data->config.explicit_header_disable ? 0 : 20;

	tmp = 8 * data_len - 4 * sf + 8 + hdr_len + 16 * crc;
	if (tmp < 0) {
		tmp = 0;
	}

	n_payload = 8 + (((tmp + 4 * (sf - 2 * de) - 1) /
			  (4 * (sf - 2 * de))) * (cr + 4));
	t_payload_us = n_payload * t_sym_us;

	/* Total airtime in milliseconds */
	return (t_preamble_us + t_payload_us + 500) / 1000;
}

static int lr20xx_validate_config(const struct device *dev, const struct lora_modem_config *config)
{
	const struct lr20xx_config *cfg = dev->config;
	uint8_t bw_reg;

	if (config->frequency > LR20XX_SUBGHZ_THRESHOLD && cfg->variant == LR20XX_LR2012) {
		LOG_ERR("Frequency too high for LR2012");
		return -EINVAL;
	}

	if (bandwidth_to_reg(config->bandwidth, &bw_reg) < 0) {
		LOG_ERR("Unsupported bandwidth: %d kHz", config->bandwidth);
		return -EINVAL;
	}

	return 0;
}

static int lr20xx_set_standby(const struct device *dev, uint8_t mode)
{
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_STANDBY, &mode, 1);
}

static int lr20xx_set_regulator_mode(const struct device *dev, uint8_t mode)
{
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_REGULATOR_MODE, &mode, 1);
}

static int lr20xx_set_packet_type(const struct device *dev, uint8_t type)
{
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_PACKET_TYPE, &type, 1);
}

static int lr20xx_set_dio_irq_params(const struct device *dev, uint8_t dio, uint32_t irq_mask)
{
	uint8_t buf[LR20XX_IRQ_SIZE + 1];

	buf[0] = dio;
	sys_put_be32(irq_mask, &buf[1]);

	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_DIO_IRQ, buf, LR20XX_IRQ_SIZE + 1);
}

static int lr20xx_clear_irq_status(const struct device *dev, uint32_t mask)
{
	uint8_t buf[LR20XX_IRQ_SIZE];

	sys_put_be32(mask, buf);
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_CLEAR_IRQ, buf, LR20XX_IRQ_SIZE);
}

static int lr20xx_clear_errors(const struct device *dev)
{
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_CLEAR_ERRORS, NULL, 0);
}

static int lr20xx_get_errors(const struct device *dev, uint16_t *errors)
{
	uint8_t buf[2];
	int ret;

	ret = lr20xx_hal_read_cmd(dev, LR20XX_CMD_GET_ERRORS, buf, 2);
	if (ret != 0) {
		return ret;
	}

	*errors = (uint16_t)buf[0] << 8 | (uint16_t)buf[1];

	return ret;
}

static int lr20xx_configure_tcxo(const struct device *dev, enum lr20xx_tcxo_voltages voltage,
				 uint32_t timeout_ms)
{
	uint32_t timeout = timeout_ms * 32000U;
	uint8_t buf[5];

	buf[0] = voltage;
	sys_put_be32(timeout, &buf[1]);

	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_TCXO_MODE, buf, 5);
}

static int lr20xx_calibrate(const struct device *dev, uint8_t mask)
{
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_CALIBRATE, &mask, 1);
}

/* When no arguments are provided to this command, it calibrates for the currently configured
 * frequency and RX path.
 */
static int lr20xx_calibrate_frontend(const struct device *dev)
{
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_CALIBRATE_FRONTEND, NULL, 0);
}

static int lr20xx_set_rf_frequency(const struct device *dev, uint32_t freq)
{
	uint8_t buf[4];

	sys_put_be32(freq, buf);

	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_RF_FREQUENCY, buf, 4);
}

static int lr20xx_config_lora_modulation(const struct device *dev,
					 uint8_t sf, uint8_t bw, uint8_t cr,
					 bool ldro)
{
	uint8_t buf[2] = { bw | (sf << 4U), (ldro ? 1U : 0) | (cr << 4U) };

	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_CONFIG_LORA_MODULATION, buf, 2);
}

static int lr20xx_config_lora_packet(const struct device *dev,
				     uint16_t preamble_len, uint8_t payload_len,
				     bool implicit_header, bool crc, bool invert_iq)
{
	uint8_t buf[4];

	sys_put_be16(preamble_len, &buf[0]);
	buf[2] = payload_len;
	buf[3] = (invert_iq ? BIT(0U) : 0) | (crc ? BIT(1U) : 0) | (implicit_header ? BIT(2U) : 0);

	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_CONFIG_LORA_PACKET, buf, 4);
}

static int lr20xx_set_sync_word(const struct device *dev, const struct lora_modem_config *config)
{
	uint8_t sync_word;

	if (config->sync_word != 0) {
		sync_word = config->sync_word;
	} else {
		sync_word = config->public_network ? LR20XX_LORA_SYNC_WORD_PUBLIC
						   : LR20XX_LORA_SYNC_WORD_PRIVATE;
	}

	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_LORA_SYNCWORD, &sync_word, 1);
}

static int lr20xx_set_tx(const struct device *dev, uint32_t timeout_ms)
{
	uint32_t timeout = LR20XX_MS_TO_TIMEOUT(timeout_ms);
	uint8_t buf[3];

	sys_put_be24(timeout, buf);
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_TX, buf, 3);
}

static int lr20xx_set_rx_duty_cycle(const struct device *dev,
				    uint32_t rx_period, uint32_t sleep_rx_period)
{
	uint8_t buf[7];

	sys_put_be24(rx_period, &buf[0]);
	sys_put_be24(sleep_rx_period, &buf[3]);
	/* Mode = RX mode, not CAD mode */
	buf[6] = 0;

	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_RX_DUTY_CYCLE, buf, 7);
}

static int lr20xx_set_rx(const struct device *dev, uint32_t timeout)
{
	uint8_t buf[3];

	sys_put_be24(timeout, buf);
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_RX, buf, 3);
}

static int lr20xx_get_lora_packet_status(const struct device *dev, uint8_t *payload_len,
					 int16_t *rssi, int8_t *snr, int32_t *frequency_offset)
{
	uint8_t buf[9];
	int ret;

	ret = lr20xx_hal_read_cmd(dev, LR20XX_CMD_GET_LORA_PACKET_STATUS, buf, 9);
	if (ret == 0) {
		/* rssi_signal_pkt to match other drivers, RSSI is -value/2 dBm */
		*rssi = -((int16_t)buf[4]);
		/* SNR is value/4 dB (signed) */
		*snr = ((int8_t)buf[2]) >> 2;
		*payload_len = buf[1];
		*frequency_offset = sign_extend(sys_get_be24(&buf[6]), 23);
	}

	return ret;
}

static int lr20xx_set_lf_clk(const struct device *dev, enum lr20xx_lf_clock clk)
{
	uint8_t buf = clk;

	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_CONFIG_LF_CLOCK, &buf, 1);
}

static int lr20xx_configure_rf_switches(const struct device *dev)
{
	const struct lr20xx_config *config = dev->config;
	uint8_t buf[2];
	int ret;
	size_t off;

	for (size_t i = LR20XX_DIO_START; i <= LR20XX_DIO_END; i++) {
		off = i - LR20XX_DIO_START;
		if (config->dio_functions[off] == LR20XX_DIO_FUNCTION_RFSW) {
			buf[0] = i;
			buf[1] = 0;
			if ((config->rfsw.standby & BIT(off)) != 0) {
				buf[1] |= LR20XX_DIO_RFSW_STANDBY;
			}
			if ((config->rfsw.rx_lf & BIT(off)) != 0) {
				buf[1] |= LR20XX_DIO_RFSW_RX_LF;
			}
			if ((config->rfsw.tx_lf & BIT(off)) != 0) {
				buf[1] |= LR20XX_DIO_RFSW_TX_LF;
			}
			if ((config->rfsw.rx_hf & BIT(off)) != 0) {
				buf[1] |= LR20XX_DIO_RFSW_RX_HF;
			}
			if ((config->rfsw.tx_hf & BIT(off)) != 0) {
				buf[1] |= LR20XX_DIO_RFSW_TX_HF;
			}

			ret = lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_DIO_RFSW, buf, 2);
			if (ret != 0) {
				LOG_ERR("Failed to configure RF switch DIO %d: %d", i, ret);
				return ret;
			}
		}
	}

	return 0;
}

static int lr20xx_configure_dios(const struct device *dev)
{
	const struct lr20xx_config *config = dev->config;
	uint8_t buf[2];
	int ret;
	size_t off;

	for (size_t i = LR20XX_DIO_START; i <= LR20XX_DIO_END; i++) {
		off = i - LR20XX_DIO_START;
		buf[0] = i;
		buf[1] = (config->dio_functions[off] << 4U) | config->dio_sleep_pulls[off];

		ret = lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_DIO_FUNCTION, buf, 2);
		if (ret != 0) {
			LOG_ERR("Failed to configure DIO %d: %d", i, ret);
			return ret;
		}
	}

	return 0;
}

static int lr20xx_clear_rx_fifo(const struct device *dev)
{
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_CLEAR_FIFO_RX, NULL, 0);
}

static int lr20xx_clear_tx_fifo(const struct device *dev)
{
	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_CLEAR_FIFO_TX, NULL, 0);
}

static int lr20xx_chip_init(const struct device *dev)
{
	const struct lr20xx_config *config = dev->config;
	uint8_t version_buf[2];
	uint16_t errors;
	int ret;

	/* Hardware reset */
	ret = lr20xx_hal_reset(dev);
	if (ret < 0) {
		LOG_ERR("Reset failed: %d", ret);
		return ret;
	}

	/* Set standby mode */
	ret = lr20xx_set_standby(dev, LR20XX_STANDBY_RC);
	if (ret < 0) {
		LOG_ERR("Set standby failed: %d", ret);
		return ret;
	}

	ret = lr20xx_clear_errors(dev);
	if (ret != 0) {
		LOG_ERR("Failed clearing errors: %d", ret);
		return ret;
	}

	ret = lr20xx_hal_read_cmd(dev, LR20XX_CMD_GET_VERSION, version_buf, 2);
	if (ret < 0) {
		LOG_ERR("Fetch version failed: %d", ret);
		return ret;
	}

	if ((version_buf[0] != LR20XX_VERSION_LR2021 && config->variant == LR20XX_LR2021)
	    || (version_buf[0] != LR20XX_VERSION_LR20X2 && config->variant != LR20XX_LR2021)) {
		LOG_ERR("Major FW version (%x) doesn't match variant configured", version_buf[0]);
		return -ENODEV;
	}

	/* Set regulator mode */
	ret = lr20xx_set_regulator_mode(dev, config->use_simo ?
					LR20XX_REGULATOR_SIMO_NORMAL : LR20XX_REGULATOR_LDO);
	if (ret < 0) {
		LOG_ERR("Set regulator failed: %d", ret);
		return ret;
	}

	ret = lr20xx_configure_dios(dev);
	if (ret < 0) {
		return ret;
	}

	ret = lr20xx_configure_rf_switches(dev);
	if (ret < 0) {
		return ret;
	}

	ret = lr20xx_set_lf_clk(dev, config->lf_clk);
	if (ret < 0) {
		LOG_ERR("Configuring LF clock failed: %d", ret);
		return ret;
	}

	ret = lr20xx_get_errors(dev, &errors);
	if (ret != 0) {
		LOG_ERR("Failed fetching errors: %d", ret);
		return ret;
	}

	if (errors != 0) {
		LOG_ERR("There were errors initializing: %x", errors);
		return -EIO;
	}

	/* Configure TCXO if enabled */
	if (config->tcxo_voltage < LR20XX_NO_TCXO) {
		ret = lr20xx_configure_tcxo(dev, config->tcxo_voltage,
					    config->tcxo_startup_delay_ms);
		if (ret < 0) {
			LOG_ERR("Set TCXO failed: %d", ret);
			return ret;
		}

		ret = lr20xx_clear_errors(dev);
		if (ret != 0) {
			LOG_ERR("Failed clearing errors: %d", ret);
			return ret;
		}

		ret = lr20xx_calibrate(dev, LR20XX_CALIBRATE_ALL);
		if (ret < 0) {
			LOG_ERR("Calibration failed: %d", ret);
			return ret;
		}

		ret = lr20xx_get_errors(dev, &errors);
		if (ret != 0) {
			LOG_ERR("Failed fetching errors: %d", ret);
			return ret;
		}

		if (errors != 0) {
			LOG_ERR("There were errors calibrating: %x", errors);
			return -EIO;
		}
	}

	/* Set packet type to LoRa */
	ret = lr20xx_set_packet_type(dev, LR20XX_PACKET_TYPE_LORA);
	if (ret < 0) {
		LOG_ERR("Set packet type failed: %d", ret);
		return ret;
	}

	/* Configure IRQs on IRQ1: TX done, RX done, timeout */
	uint32_t irq_mask = LR20XX_IRQ_TX_DONE | LR20XX_IRQ_RX_DONE |
			    LR20XX_IRQ_TIMEOUT | LR20XX_IRQ_CRC_ERR;
	ret = lr20xx_set_dio_irq_params(dev, config->irq_dio, irq_mask);
	if (ret < 0) {
		LOG_ERR("Set IRQ params failed: %d", ret);
		return ret;
	}

	/* Clear any pending IRQs */
	ret = lr20xx_clear_irq_status(dev, LR20XX_IRQ_ALL);
	if (ret < 0) {
		LOG_ERR("Clear IRQ failed: %d", ret);
		return ret;
	}

#if CONFIG_LORA_LOG_LEVEL >= LOG_LEVEL_INF
	const char *lr20x_str;

	switch (config->variant) {
	case LR20XX_LR2012:
		lr20x_str = "LR2012";
		break;
	case LR20XX_LR2022:
		lr20x_str = "LR2022";
		break;
	case LR20XX_LR2021:
		lr20x_str = "LR2021";
		break;
	default:
		lr20x_str = "Unknown LR20xx";
	}
	LOG_INF("%s initialized with FW version %x%02x", lr20x_str, version_buf[0], version_buf[1]);
#endif
	return 0;
}

static void lr20xx_irq_callback(const struct device *dev)
{
	struct lr20xx_data *data = dev->data;

	k_work_submit(&data->irq_work);
}

static int lr20xx_set_sleep(const struct device *dev)
{
	struct lr20xx_data *data = dev->data;
	uint8_t cfg[5] = { LR20XX_SLEEP_WARM_START, 0x00, 0x00, 0x00, 0x00};
	int ret;

	if (!IS_ENABLED(CONFIG_LORA_LR20XX_NATIVE_SLEEP)) {
		atomic_set(&data->state, LR20XX_STATE_IDLE);
		return 0;
	}

	if (atomic_get(&data->state) == LR20XX_STATE_SLEEP) {
		return 0;
	}

	lr20xx_hal_set_irq_callback(dev, NULL);

	ret = lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_SLEEP, cfg, sizeof(cfg));
	if (ret == 0) {
		atomic_set(&data->state, LR20XX_STATE_SLEEP);
	}

	return ret;
}

static int lr20xx_ensure_ready(const struct device *dev)
{
	int ret;

	/* Re-enable interrupt */
	ret = lr20xx_hal_set_irq_callback(dev, lr20xx_irq_callback);
	if (ret < 0) {
		return ret;
	}

	if (!IS_ENABLED(CONFIG_LORA_LR20XX_NATIVE_SLEEP)) {
		return 0;
	}

	ret = lr20xx_hal_wakeup(dev);
	if (ret != 0) {
		return ret;
	}

	return ret;
}

static void lr20xx_handle_irq_tx_done(const struct device *dev)
{
	struct lr20xx_data *data = dev->data;
	struct lr20xx_tx_result result = { .status = 0 };

	LOG_DBG("TX done");
	lr20xx_set_sleep(dev);

	if (data->tx_async_signal != NULL) {
		k_poll_signal_raise(data->tx_async_signal, 0);
		data->tx_async_signal = NULL;
	}
	k_msgq_put(&data->tx_msgq, &result, K_NO_WAIT);
}

static void lr20xx_rx_restart(const struct device *dev)
{
	struct lr20xx_data *data = dev->data;

	if (atomic_get(&data->state) == LR20XX_STATE_RX_DUTY_CYCLE) {
		int ret;

		ret = lr20xx_set_rx_duty_cycle(dev,
					       data->duty_cycle.rx_period,
					       data->duty_cycle.sleep_rx_period);
		if (ret < 0) {
			data->rx_cb = NULL;
			data->rx_cb_user_data = NULL;
			atomic_set(&data->state, LR20XX_STATE_IDLE);
			lr20xx_set_sleep(dev);
		}
	} else {
		/* Not expected to do anything in normal function, acts as safety */
		lr20xx_set_rx(dev, LR20XX_RX_TIMEOUT_CONTINUOUS);
	}
}

static void lr20xx_handle_irq_rx_done(const struct device *dev, uint32_t irq_status)
{
	struct lr20xx_data *data = dev->data;
	struct lr20xx_rx_result result = { 0 };
	uint8_t payload_len = 0;
	int32_t frequency_offset = 0;
	uint32_t bandwidth;
	int ret;


	ret = lr20xx_get_lora_packet_status(dev, &payload_len, &result.rssi, &result.snr,
					    &frequency_offset);
	if (ret < 0) {
		LOG_ERR("Failed to get RX buffer status");
		result.status = ret;
		goto out;
	}

	(void)bandwidth_to_hz(data->config.bandwidth, &bandwidth);
	if (abs(frequency_offset) > (bandwidth >> 3U)) {
		LOG_WRN_RATELIMIT(
			"Packet received with over 12.5 percents of frequency deviation (%d)",
			frequency_offset);
	}

	/* Always read payload to empty the FIFO */
	result.len = MIN(payload_len, sizeof(data->rx_buf));
	ret = lr20xx_hal_read_fifo(dev, data->rx_buf, result.len);
	if (ret < 0) {
		result.status = ret;
		goto out;
	}

	if (irq_status & LR20XX_IRQ_CRC_ERR) {
		LOG_WRN("CRC error");
		result.status = -EIO;
		goto out;
	}

	result.status = result.len;
	LOG_DBG("RX done: %d bytes, RSSI=%d, SNR=%d, Frequency offset=%d",
		result.len, result.rssi, result.snr, frequency_offset);

out:
	if (data->rx_cb == NULL) {
		lr20xx_set_sleep(dev);
		k_msgq_put(&data->rx_msgq, &result, K_NO_WAIT);
		return;
	}

	/* Async mode: deliver valid packets, drop errors silently */
	if (result.status > 0) {
		data->rx_cb(dev, data->rx_buf, result.len,
			    result.rssi, result.snr,
			    data->rx_cb_user_data);
	}

	/* Restart reception unless the callback stopped it */
	if (data->rx_cb != NULL) {
		lr20xx_rx_restart(dev);
	}
}

static void lr20xx_handle_irq_timeout(const struct device *dev)
{
	struct lr20xx_data *data = dev->data;
	enum lr20xx_state state = atomic_get(&data->state);

	LOG_DBG("Timeout");

	if (state == LR20XX_STATE_RX_DUTY_CYCLE) {
		/*
		 * Preamble was detected but full packet reception failed.
		 * The radio fell back to STDBY_RC — restart the duty cycle.
		 */
		lr20xx_rx_restart(dev);
		return;
	}

	lr20xx_set_sleep(dev);

	if (state == LR20XX_STATE_TX) {
		struct lr20xx_tx_result result = { .status = -ETIMEDOUT };

		if (data->tx_async_signal != NULL) {
			k_poll_signal_raise(data->tx_async_signal, -ETIMEDOUT);
			data->tx_async_signal = NULL;
		}
		k_msgq_put(&data->tx_msgq, &result, K_NO_WAIT);
	} else if (data->rx_cb == NULL) {
		/* Sync RX timeout */
		struct lr20xx_rx_result result = { .status = -EAGAIN };

		k_msgq_put(&data->rx_msgq, &result, K_NO_WAIT);
	}
}

static void lr20xx_irq_work_handler(struct k_work *work)
{
	struct lr20xx_data *data = CONTAINER_OF(work, struct lr20xx_data, irq_work);
	const struct device *dev = data->dev;
	uint32_t irq_status = 0;
	uint8_t buf[4];
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = lr20xx_hal_read_cmd(dev, LR20XX_CMD_GET_AND_CLEAR_IRQ, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to get IRQ status");
		k_mutex_unlock(&data->lock);
		return;
	}

	irq_status = sys_get_be32(buf);

	LOG_DBG("IRQ status: 0x%08x", irq_status);

	if (irq_status & LR20XX_IRQ_TX_DONE) {
		lr20xx_handle_irq_tx_done(dev);
	}

	if (irq_status & LR20XX_IRQ_RX_DONE) {
		lr20xx_handle_irq_rx_done(dev, irq_status);
	}

	if (irq_status & LR20XX_IRQ_TIMEOUT) {
		lr20xx_handle_irq_timeout(dev);
	}

	/* Re-enable the interrupt for the next event (unless sleeping) */
	if (atomic_get(&data->state) != LR20XX_REST_STATE) {
		lr20xx_hal_irq_enable(dev);
	}

	k_mutex_unlock(&data->lock);
}

static int lr20xx_configure_tx_params(const struct device *dev, int8_t power,
				      uint32_t frequency, uint8_t ramp_time)
{
	int8_t tx_power;
	uint8_t buf[3];
	int ret;

	if (frequency > LR20XX_SUBGHZ_THRESHOLD) {
		tx_power = CLAMP(power, (int8_t)LR20XX_MIN_POWER_HF, LR20XX_MAX_POWER_HF);
		buf[0] = LR20XX_PA_HF;
		buf[1] = (LR20XX_LF_PA_DUTY_CYCLE_HF << 4U) | LR20XX_LF_PA_SLICES_HF;
		buf[2] = LR20XX_HF_PA_DUTY_CYCLE_HF;
	} else {
		tx_power = CLAMP(power, (int8_t)LR20XX_MIN_POWER_LF, LR20XX_MAX_POWER_LF);
		buf[0] = LR20XX_PA_LF;
		buf[1] = (LR20XX_LF_PA_DUTY_CYCLE_LF << 4U) | LR20XX_LF_PA_SLICES_LF;
		buf[2] = LR20XX_HF_PA_DUTY_CYCLE_LF;
	}
	ret = lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_PA_CONFIG, buf, 3);
	if (ret < 0) {
		return ret;
	}

	buf[0] = (uint8_t)LR20XX_VALUE_POWER_FROM_DBM(tx_power);
	buf[1] = ramp_time;

	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_CONFIG_TX, buf, 2);
}

static int lr20xx_set_rx_path(const struct device *dev, bool hf, bool rx_boosted)
{
	uint8_t buf[2] = { hf ? 1U : 0, rx_boosted ? 7U : (hf ? 4U : 0U) };

	return lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_RX_PATH, buf, 2);
}

static int lr20xx_lora_config(const struct device *dev,
			      const struct lora_modem_config *config)
{
	struct lr20xx_data *data = dev->data;
	const struct lr20xx_config *cfg = dev->config;
	uint8_t bw_reg;
	uint16_t errors;
	bool ldro;
	int ret;

	ret = lr20xx_validate_config(dev, config);
	if (ret < 0) {
		return ret;
	}

	if (!atomic_cas(&data->state, LR20XX_REST_STATE, LR20XX_STATE_IDLE)) {
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = lr20xx_ensure_ready(dev);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		atomic_set(&data->state, LR20XX_REST_STATE);
		return ret;
	}

	/* Store configuration */
	memcpy(&data->config, config, sizeof(*config));
	data->config_valid = false;

	/* Set modulation parameters */
	ret = bandwidth_to_reg(config->bandwidth, &bw_reg);
	__ASSERT_NO_MSG(ret == 0);
	ldro = should_enable_ldro(config->datarate, config->bandwidth, cfg);
	ret = lr20xx_config_lora_modulation(dev,
					    config->datarate,
					    bw_reg,
					    config->coding_rate,
					    ldro);
	if (ret < 0) {
		goto out;
	}

	/* Set RF frequency */
	ret = lr20xx_set_rf_frequency(dev, config->frequency);
	if (ret < 0) {
		goto out;
	}

	ret = lr20xx_set_rx_path(dev, config->frequency > LR20XX_SUBGHZ_THRESHOLD, cfg->rx_boosted);
	if (ret < 0) {
		goto out;
	}

	ret = lr20xx_clear_errors(dev);
	if (ret < 0) {
		goto out;
	}

	/* Run partial Calibration (required with a change of over 50 MHz or temperature changes) */
	ret = lr20xx_calibrate(dev, LR20XX_CALIBRATE_CHANGES);
	if (ret < 0) {
		goto out;
	}

	ret = lr20xx_get_errors(dev, &errors);
	if (ret != 0) {
		goto out;
	}

	if (errors != 0) {
		ret = -EIO;
		goto out;
	}

	/* Run image calibration after setting frequency and RX Path */
	ret = lr20xx_calibrate_frontend(dev);
	if (ret < 0) {
		goto out;
	}

	/* Configure PA and TX power based on chip variant and frequency */
	ret = lr20xx_configure_tx_params(dev, config->tx_power,
					 config->frequency,
					 LR20XX_RAMP_64_US);
	if (ret < 0) {
		goto out;
	}

	/* Set sync word */
	ret = lr20xx_set_sync_word(dev, config);
	if (ret < 0) {
		goto out;
	}

	data->config_valid = true;
	LOG_DBG("Config: freq=%u, SF=%d, BW=%d, CR=%d, power=%d",
		config->frequency, config->datarate, config->bandwidth,
		config->coding_rate, config->tx_power);

out:
	lr20xx_set_sleep(dev);
	k_mutex_unlock(&data->lock);
	return ret;
}

static int lr20xx_lora_send_async(const struct device *dev,
				  uint8_t *data_buf, uint32_t data_len,
				  struct k_poll_signal *async)
{
	struct lr20xx_data *data = dev->data;
	int ret;

	if (!data->config_valid) {
		LOG_ERR("Not configured");
		return -EINVAL;
	}

	if (data_len > LR20XX_LORA_MAX_PAYLOAD_LENGTH) {
		LOG_ERR("Payload too long: %u", data_len);
		return -EINVAL;
	}

	if (!atomic_cas(&data->state, LR20XX_REST_STATE, LR20XX_STATE_TX)) {
		LOG_ERR("Busy");
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = lr20xx_ensure_ready(dev);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		atomic_set(&data->state, LR20XX_REST_STATE);
		return ret;
	}

	data->tx_async_signal = async;
	k_msgq_purge(&data->tx_msgq);

	ret = lr20xx_clear_tx_fifo(dev);
	if (ret < 0) {
		goto out_error;
	}

	if (data->config.explicit_header_disable &&
		data_len != data->config.implicit_packet_length) {
		ret = -EINVAL;
		goto out_error;
	}

	/* Set packet parameters */
	ret = lr20xx_config_lora_packet(dev,
					data->config.preamble_len,
					data_len,
					data->config.explicit_header_disable,
					!data->config.packet_crc_disable,
					data->config.iq_inverted);
	if (ret < 0) {
		goto out_error;
	}

	/* Write payload to buffer */
	ret = lr20xx_hal_write_fifo(dev, data_buf, data_len);
	if (ret < 0) {
		goto out_error;
	}

	ret = lr20xx_set_tx(dev, lr20xx_lora_airtime(dev, data_len) + LR20XX_AIRTIME_MARGIN_MS);
	if (ret < 0) {
		goto out_error;
	}

	k_mutex_unlock(&data->lock);
	return 0;

out_error:
	data->tx_async_signal = NULL;
	lr20xx_set_sleep(dev);
	k_mutex_unlock(&data->lock);
	return ret;
}

static int lr20xx_lora_send(const struct device *dev,
			    uint8_t *data_buf, uint32_t data_len)
{
	struct lr20xx_data *data = dev->data;
	struct lr20xx_tx_result result;
	uint32_t airtime_ms;
	int ret;

	ret = lr20xx_lora_send_async(dev, data_buf, data_len, NULL);
	if (ret < 0) {
		return ret;
	}

	airtime_ms = lr20xx_lora_airtime(dev, data_len) + LR20XX_AIRTIME_MARGIN_MS;

	/* Wait for TX completion */
	ret = k_msgq_get(&data->tx_msgq, &result, K_MSEC(airtime_ms));
	if (ret < 0) {
		k_mutex_lock(&data->lock, K_FOREVER);
		/* Check result hasn't appeared in the meantime */
		if (k_msgq_get(&data->tx_msgq, &result, K_NO_WAIT) != 0) {
			LOG_ERR("TX timeout");
			if (atomic_get(&data->state) == LR20XX_STATE_TX) {
				/* Chip is still transmitting, abort first */
				lr20xx_set_standby(dev, LR20XX_STANDBY_RC);
				lr20xx_set_sleep(dev);
			}
			result.status = -ETIMEDOUT;
		}
		k_mutex_unlock(&data->lock);
	}

	return result.status;
}

static int lr20xx_lora_recv(const struct device *dev, uint8_t *data_buf,
			    uint8_t size, k_timeout_t timeout,
			    int16_t *rssi, int8_t *snr)
{
	struct lr20xx_data *data = dev->data;
	struct lr20xx_rx_result result;
	uint32_t timeout_val, timeout_ms;
	int ret;

	if (!data->config_valid) {
		LOG_ERR("Not configured");
		return -EINVAL;
	}

	if (!atomic_cas(&data->state, LR20XX_REST_STATE, LR20XX_STATE_RX)) {
		LOG_ERR("Busy");
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = lr20xx_ensure_ready(dev);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		atomic_set(&data->state, LR20XX_REST_STATE);
		return ret;
	}

	data->rx_cb = NULL;
	k_msgq_purge(&data->rx_msgq);

	ret = lr20xx_clear_rx_fifo(dev);
	if (ret < 0) {
		lr20xx_set_sleep(dev);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/* Set packet parameters for variable length reception */
	ret = lr20xx_config_lora_packet(dev,
		data->config.preamble_len,
		data->config.explicit_header_disable ?
		data->config.implicit_packet_length : LR20XX_LORA_ANY_PAYLOAD_LENGTH,
		data->config.explicit_header_disable,
		!data->config.packet_crc_disable,
		data->config.iq_inverted);
	if (ret < 0) {
		lr20xx_set_sleep(dev);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/* Tweak timeout to avoid race between host and modem timeouts */
	timeout_ms = k_ticks_to_ms_ceil32(timeout.ticks);
	timeout_val = K_TIMEOUT_EQ(timeout, K_FOREVER)
		     ? LR20XX_RX_TIMEOUT_SINGLE
		     : LR20XX_MS_TO_TIMEOUT(timeout_ms);
	ret = lr20xx_set_rx(dev, timeout_val);
	if (ret < 0) {
		lr20xx_set_sleep(dev);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	k_mutex_unlock(&data->lock);


	timeout = K_TIMEOUT_EQ(timeout, K_FOREVER) ? K_FOREVER
			: K_MSEC(timeout_ms + LR20XX_RX_MARGIN_MS(timeout_ms));
	/* Wait for RX completion */
	ret = k_msgq_get(&data->rx_msgq, &result, timeout);
	if (ret < 0) {
		LOG_DBG("RX timeout");
		k_mutex_lock(&data->lock, K_FOREVER);
		/* Check result hasn't appeared in the meantime */
		if (k_msgq_get(&data->rx_msgq, &result, K_NO_WAIT) != 0) {
			if (atomic_get(&data->state) == LR20XX_STATE_RX) {
				/* Chip is still receiving, abort first */
				lr20xx_set_standby(dev, LR20XX_STANDBY_RC);
				lr20xx_set_sleep(dev);
			}
			result.status = -EAGAIN;
		}
		k_mutex_unlock(&data->lock);
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

static int lr20xx_lora_recv_async(const struct device *dev,
				  lora_recv_cb cb, void *user_data)
{
	struct lr20xx_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (cb == NULL) {
		/* Stop async reception */
		data->rx_cb = NULL;
		data->rx_cb_user_data = NULL;
		if (atomic_cas(&data->state, LR20XX_STATE_RX, LR20XX_STATE_IDLE)) {
			lr20xx_set_standby(dev, LR20XX_STANDBY_RC);
			lr20xx_set_sleep(dev);
		}
		k_mutex_unlock(&data->lock);
		return 0;
	}

	if (!data->config_valid) {
		LOG_ERR("Not configured");
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	if (!atomic_cas(&data->state, LR20XX_REST_STATE, LR20XX_STATE_RX)) {
		LOG_ERR("Busy");
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	ret = lr20xx_ensure_ready(dev);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		atomic_set(&data->state, LR20XX_REST_STATE);
		return ret;
	}

	ret = lr20xx_clear_rx_fifo(dev);
	if (ret < 0) {
		lr20xx_set_sleep(dev);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	data->rx_cb = cb;
	data->rx_cb_user_data = user_data;

	/* Set packet parameters */
	ret = lr20xx_config_lora_packet(dev,
		data->config.preamble_len,
		data->config.explicit_header_disable ?
		data->config.implicit_packet_length : LR20XX_LORA_ANY_PAYLOAD_LENGTH,
		data->config.explicit_header_disable,
		!data->config.packet_crc_disable,
		data->config.iq_inverted);
	if (ret < 0) {
		data->rx_cb = NULL;
		lr20xx_set_sleep(dev);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/* Start continuous reception */
	ret = lr20xx_set_rx(dev, LR20XX_RX_TIMEOUT_CONTINUOUS);
	if (ret < 0) {
		data->rx_cb = NULL;
		lr20xx_set_sleep(dev);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

static void lr20xx_duty_cycle_stop(const struct device *dev)
{
	/*
	 * During duty cycle the BUSY pin stays asserted even in the
	 * sleep phase. Wake the radio via a raw NSS edge (bypasses
	 * BUSY check) so the standby command can go through
	 * immediately.
	 */
	if (lr20xx_hal_is_busy(dev)) {
		lr20xx_hal_wakeup(dev);
	}
	lr20xx_set_standby(dev, LR20XX_STANDBY_RC);
	lr20xx_set_sleep(dev);
}

static int lr20xx_duty_cycle_start(const struct device *dev,
				   k_timeout_t rx_period,
				   k_timeout_t sleep_period)
{
	struct lr20xx_data *data = dev->data;
	int ret;

	data->duty_cycle.rx_period = LR20XX_MS_TO_TIMEOUT(
		k_ticks_to_ms_ceil32(rx_period.ticks));
	data->duty_cycle.sleep_rx_period = LR20XX_MS_TO_TIMEOUT(
		k_ticks_to_ms_ceil32(sleep_period.ticks) + k_ticks_to_ms_ceil32(rx_period.ticks));

	ret = lr20xx_clear_rx_fifo(dev);
	if (ret < 0) {
		goto out_error;
	}

	ret = lr20xx_config_lora_packet(dev,
		data->config.preamble_len,
		data->config.explicit_header_disable ?
		data->config.implicit_packet_length : LR20XX_LORA_ANY_PAYLOAD_LENGTH,
		data->config.explicit_header_disable,
		!data->config.packet_crc_disable,
		data->config.iq_inverted);
	if (ret < 0) {
		goto out_error;
	}

	ret = lr20xx_set_rx_duty_cycle(dev, data->duty_cycle.rx_period,
				       data->duty_cycle.sleep_rx_period);
	if (ret < 0) {
		goto out_error;
	}

	k_mutex_unlock(&data->lock);
	return 0;

out_error:
	data->rx_cb = NULL;
	atomic_set(&data->state, LR20XX_STATE_IDLE);
	lr20xx_set_sleep(dev);
	k_mutex_unlock(&data->lock);
	return ret;
}

static int lr20xx_duty_cycle_enter(const struct device *dev,
				   lora_recv_cb cb, void *user_data)
{
	struct lr20xx_data *data = dev->data;
	int ret;

	if (!data->config_valid) {
		LOG_ERR("Not configured");
		return -EINVAL;
	}

	if (!atomic_cas(&data->state, LR20XX_REST_STATE,
			LR20XX_STATE_RX_DUTY_CYCLE)) {
		LOG_ERR("Busy");
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = lr20xx_ensure_ready(dev);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		atomic_set(&data->state, LR20XX_REST_STATE);
		return ret;
	}

	data->rx_cb = cb;
	data->rx_cb_user_data = user_data;

	return 0;
}

static int lr20xx_lora_recv_duty_cycle(const struct device *dev,
				       k_timeout_t rx_period,
				       k_timeout_t sleep_period,
				       uint8_t *data_buf, uint8_t size,
				       k_timeout_t timeout,
				       int16_t *rssi, int8_t *snr)
{
	struct lr20xx_data *data = dev->data;
	struct lr20xx_rx_result result;
	int ret;

	ret = lr20xx_duty_cycle_enter(dev, NULL, NULL);
	if (ret < 0) {
		return ret;
	}

	k_msgq_purge(&data->rx_msgq);

	ret = lr20xx_duty_cycle_start(dev, rx_period, sleep_period);
	if (ret < 0) {
		return ret;
	}

	/* Block until a packet arrives or timeout */
	ret = k_msgq_get(&data->rx_msgq, &result, timeout);

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Check result hasn't appeared in the meantime */
	if (ret < 0) {
		ret = k_msgq_get(&data->rx_msgq, &result, K_NO_WAIT);
	}

	if (atomic_get(&data->state) == LR20XX_STATE_RX_DUTY_CYCLE) {
		lr20xx_duty_cycle_stop(dev);
	}

	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		LOG_DBG("RX duty cycle timeout");
		return -EAGAIN;
	}

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

static int lr20xx_lora_recv_duty_cycle_async(const struct device *dev,
					     k_timeout_t rx_period,
					     k_timeout_t sleep_period,
					     lora_recv_cb cb, void *user_data)
{
	struct lr20xx_data *data = dev->data;
	int ret;

	if (cb == NULL) {
		k_mutex_lock(&data->lock, K_FOREVER);
		data->rx_cb = NULL;
		data->rx_cb_user_data = NULL;
		if (atomic_cas(&data->state, LR20XX_STATE_RX_DUTY_CYCLE,
			       LR20XX_STATE_IDLE)) {
			lr20xx_duty_cycle_stop(dev);
		}
		k_mutex_unlock(&data->lock);
		return 0;
	}

	ret = lr20xx_duty_cycle_enter(dev, cb, user_data);
	if (ret < 0) {
		return ret;
	}

	return lr20xx_duty_cycle_start(dev, rx_period, sleep_period);
}

static int lr20xx_lora_test_cw(const struct device *dev, uint32_t frequency,
			       int8_t tx_power, uint16_t duration)
{
	const struct lr20xx_config *cfg = dev->config;
	struct lr20xx_data *data = dev->data;
	uint8_t buf = LR20XX_TX_TEST_CONTINUOUS_WAVE;
	int ret;

	if (frequency > LR20XX_SUBGHZ_THRESHOLD && cfg->variant == LR20XX_LR2012) {
		LOG_ERR("Frequency too high for LR2012");
		return -EINVAL;
	}

	if (!atomic_cas(&data->state, LR20XX_REST_STATE, LR20XX_STATE_TX)) {
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = lr20xx_ensure_ready(dev);
	if (ret < 0) {
		goto out_unlock;
	}

	/* Set frequency */
	ret = lr20xx_set_rf_frequency(dev, frequency);
	if (ret < 0) {
		goto out_sleep;
	}

	/* Set PA config and TX power */
	ret = lr20xx_configure_tx_params(dev, tx_power, frequency, LR20XX_RAMP_64_US);
	if (ret < 0) {
		goto out_sleep;
	}

	/* Start CW transmission */
	ret = lr20xx_hal_write_cmd(dev, LR20XX_CMD_SET_TX_TEST, &buf, 1);
	if (ret < 0) {
		goto out_sleep;
	}

	k_mutex_unlock(&data->lock);

	/* Wait for duration */
	k_sleep(K_SECONDS(duration));

	/* Stop CW */
	k_mutex_lock(&data->lock, K_FOREVER);
	lr20xx_set_standby(dev, LR20XX_STANDBY_RC);

out_sleep:
	data->config_valid = false;
	lr20xx_set_sleep(dev);
out_unlock:
	k_mutex_unlock(&data->lock);
	atomic_set(&data->state, LR20XX_REST_STATE);

	return ret;
}

static DEVICE_API(lora, lr20xx_lora_api) = {
	.config = lr20xx_lora_config,
	.send = lr20xx_lora_send,
	.send_async = lr20xx_lora_send_async,
	.recv = lr20xx_lora_recv,
	.recv_async = lr20xx_lora_recv_async,
	.recv_duty_cycle = lr20xx_lora_recv_duty_cycle,
	.recv_duty_cycle_async = lr20xx_lora_recv_duty_cycle_async,
	.airtime = lr20xx_lora_airtime,
	.test_cw = lr20xx_lora_test_cw,
};

static int lr20xx_init(const struct device *dev)
{
	struct lr20xx_data *data = dev->data;
	int ret;

	/* Initialize data structures */
	k_mutex_init(&data->lock);
	k_msgq_init(&data->tx_msgq, (char *)&data->tx_result,
		    sizeof(struct lr20xx_tx_result), 1);
	k_msgq_init(&data->rx_msgq, (char *)&data->rx_result,
		    sizeof(struct lr20xx_rx_result), 1);
	k_work_init(&data->irq_work, lr20xx_irq_work_handler);
	data->dev = dev;
	atomic_set(&data->state, LR20XX_STATE_IDLE);
	data->config_valid = false;

	/* Initialize HAL */
	ret = lr20xx_hal_init(dev);
	if (ret < 0) {
		LOG_ERR("HAL init failed: %d", ret);
		return ret;
	}

	/* Initialize chip */
	ret = lr20xx_chip_init(dev);
	if (ret < 0) {
		LOG_ERR("Chip init failed: %d", ret);
		return ret;
	}

	/*
	 * Place the radio into sleep mode upon boot.
	 * The required lora_config call before transmission or reception
	 * will wake the radio. It is automatically placed back into sleep
	 * mode upon TX or RX completion.
	 */
	if (IS_ENABLED(CONFIG_LORA_LR20XX_NATIVE_SLEEP)) {
		ret = lr20xx_set_sleep(dev);
		if (ret < 0) {
			LOG_ERR("Initial sleep failed: %d", ret);
			return ret;
		}
	}

	return 0;
}

#define LR20XX_GET_IRQ_DIO_IMPL(node_id, _prop, idx)						\
	IF_ENABLED(IS_EQ(DT_ENUM_IDX_BY_IDX(node_id, _prop, idx), LR20XX_DIO_FUNCTION_IRQ_PREP),\
		   (idx))

#define LR20XX_GET_IRQ_DIO(node_id)								\
	DT_FOREACH_PROP_ELEM(node_id, dio_functions, LR20XX_GET_IRQ_DIO_IMPL)

#define LR20XX_CHECK_IRQ_DIO_IMPL(node_id, _prop, idx)						\
	+ IS_EQ(DT_ENUM_IDX_BY_IDX(node_id, _prop, idx), LR20XX_DIO_FUNCTION_IRQ_PREP)

#define LR20XX_CHECK_IRQ_DIO(node_id)								\
	0 DT_FOREACH_PROP_ELEM(node_id, dio_functions, LR20XX_CHECK_IRQ_DIO_IMPL)

#define LR20XX_GET_ENUM_DIOS(node_id, _prop)							\
	{											\
		DT_ENUM_IDX_BY_IDX(node_id, _prop, 0),						\
		DT_ENUM_IDX_BY_IDX(node_id, _prop, 1),						\
		DT_ENUM_IDX_BY_IDX(node_id, _prop, 2),						\
		DT_ENUM_IDX_BY_IDX(node_id, _prop, 3),						\
		DT_ENUM_IDX_BY_IDX(node_id, _prop, 4),						\
		DT_ENUM_IDX_BY_IDX(node_id, _prop, 5),						\
		DT_ENUM_IDX_BY_IDX(node_id, _prop, 6),						\
	}

#define LR20XX_INIT(node_id, _variant)								\
	BUILD_ASSERT(LR20XX_CHECK_IRQ_DIO(node_id) == 1, "Must have exactly one IRQ DIO");	\
	BUILD_ASSERT(DT_ENUM_IDX_BY_IDX(node_id, dio_sleep_pulls, 0) ==				\
		LR20XX_DIO_SLEEP_PULL_UP, "DIO5 only supports UP sleep pull");			\
	static struct lr20xx_data lr20xx_data_##node_id;					\
												\
	static const struct lr20xx_config lr20xx_config_##node_id = {				\
		.variant = _variant,								\
		.spi = SPI_DT_SPEC_GET(node_id,							\
					    SPI_WORD_SET(8) | SPI_TRANSFER_MSB),		\
		.spi_wake = SPI_DT_SPEC_GET(node_id,						\
			SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_HOLD_ON_CS | SPI_LOCK_ON),	\
		.reset = GPIO_DT_SPEC_GET(node_id, reset_gpios),				\
		.busy = GPIO_DT_SPEC_GET(node_id, busy_gpios),					\
		.irq = GPIO_DT_SPEC_GET(node_id, irq_gpios),					\
		.dio_functions = LR20XX_GET_ENUM_DIOS(node_id, dio_functions),			\
		.dio_sleep_pulls = LR20XX_GET_ENUM_DIOS(node_id, dio_sleep_pulls),		\
		.tcxo_voltage = DT_ENUM_IDX(node_id, tcxo_voltage),				\
		.lf_clk = DT_ENUM_IDX(node_id, lf_clk),						\
		.tcxo_startup_delay_ms = DT_PROP(node_id, tcxo_power_startup_delay_ms),		\
		.rfsw.standby = DT_PROP(node_id, rfsw_standby),					\
		.rfsw.rx_lf = DT_PROP(node_id, rfsw_rx_lf),					\
		.rfsw.tx_lf = DT_PROP(node_id, rfsw_tx_lf),					\
		.rfsw.rx_hf = DT_PROP(node_id, rfsw_rx_hf),					\
		.rfsw.tx_hf = DT_PROP(node_id, rfsw_tx_hf),					\
		.irq_dio = LR20XX_GET_IRQ_DIO(node_id) + LR20XX_DIO_START,			\
		.use_simo = DT_PROP(node_id, use_simo),						\
		.force_ldro = DT_PROP(node_id, force_ldro),					\
		.rx_boosted = DT_PROP(node_id, rx_boosted),					\
	};											\
												\
	DEVICE_DT_DEFINE(node_id, lr20xx_init, NULL,						\
			      &lr20xx_data_##node_id, &lr20xx_config_##node_id,			\
			      POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,				\
			      &lr20xx_lora_api);

DT_FOREACH_STATUS_OKAY_VARGS(semtech_lr2021, LR20XX_INIT, LR20XX_LR2021)
DT_FOREACH_STATUS_OKAY_VARGS(semtech_lr2022, LR20XX_INIT, LR20XX_LR2022)
DT_FOREACH_STATUS_OKAY_VARGS(semtech_lr2012, LR20XX_INIT, LR20XX_LR2012)

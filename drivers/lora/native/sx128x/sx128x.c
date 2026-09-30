/*
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Native driver for the Semtech SX1280/SX1281 2.4 GHz transceivers, with
 * the LoRa modem and, under CONFIG_LORA_GFSK, the GFSK one.
 *
 * No sleep-mode support yet: the chip stays in STDBY_RC between operations.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/byteorder.h>

#include "sx128x.h"
#include "sx128x_gfsk.h"
#include "sx12xx_lora.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx128x, CONFIG_LORA_LOG_LEVEL);

#define SX128X_REST_STATE SX128X_STATE_STANDBY

/* SetTx/SetRx PeriodBase: use the 1 ms unit throughout for simplicity. */
#define SX128X_PERIOD_BASE_1MS      0x02
#define SX128X_PERIOD_BASE_4MS      0x03
/*
 * The radio abandons a transmission that takes twice its time on air plus
 * this; the host waits the same margin again before assuming the interrupt
 * was lost.
 */
#define SX128X_TX_TIMEOUT_MARGIN_MS 100
#define SX128X_RX_CONTINUOUS        0xFFFF
#define SX128X_IRQ_POLL_INTERVAL    K_MSEC(1)

/*
 * A corrupted LoRa header raises HeaderError without RxDone or RxTimeout and
 * leaves the receiver stuck (datasheet errata 16.2), so it is handled like a
 * CRC error.
 */
#define SX128X_IRQ_RX_ERROR (SX128X_IRQ_CRC_ERROR | SX128X_IRQ_HEADER_ERROR)

/*
 * One mask for transmit and receive: TX_DONE cannot fire while receiving,
 * and keeping the mask unchanged lets sx128x_set_dio_irq_params() skip the
 * SPI command on every RX/TX turnaround.
 */
#define SX128X_IRQ_TXRX                                                                            \
	(SX128X_IRQ_TX_DONE | SX128X_IRQ_RX_DONE | SX128X_IRQ_RX_TX_TIMEOUT | SX128X_IRQ_RX_ERROR)

/*
 * Draining a reception is several SPI transactions (interrupt status, clear,
 * buffer status, payload read), and in continuous receive they have to finish
 * before the next packet lands at the same buffer offset - which with GFSK at
 * 2 Mb/s is a few hundred microseconds away. The system workqueue is shared
 * with everything else in the application, so it can be arbitrarily late; use
 * a dedicated cooperative thread instead, so the only things that can delay a
 * reception are interrupts and higher-priority threads.
 */
static K_THREAD_STACK_DEFINE(sx128x_irq_stack, CONFIG_LORA_SX128X_NATIVE_IRQ_THREAD_STACK_SIZE);
static struct k_work_q sx128x_irq_workq;
static bool sx128x_irq_workq_started;

/* SX128X SetTxParams power field is an offset from -18 dBm. */
#define SX128X_TX_POWER_MIN_DBM -18
#define SX128X_TX_POWER_MAX_DBM 13

static int bandwidth_to_reg(enum lora_signal_bandwidth bw, uint8_t *reg)
{
	/*
	 * The SX128X's 2.4 GHz LoRa mode only supports these four
	 * bandwidths. lora.h's enum values already anticipate this (the
	 * *_KHZ names are nominal; the SX128X's real bandwidths are
	 * 203.125/406.25/812.5/1625 kHz).
	 */
	switch (bw) {
	case BW_200_KHZ:
		*reg = SX128X_LORA_BW_203;
		return 0;
	case BW_400_KHZ:
		*reg = SX128X_LORA_BW_406;
		return 0;
	case BW_800_KHZ:
		*reg = SX128X_LORA_BW_812;
		return 0;
	case BW_1600_KHZ:
		*reg = SX128X_LORA_BW_1625;
		return 0;
	default:
		return -EINVAL;
	}
}

static int validate_config(const struct lora_modem_config *config)
{
	uint8_t bw_reg;

	if (config == NULL) {
		return -EINVAL;
	}

	if (bandwidth_to_reg(config->bandwidth, &bw_reg) < 0) {
		LOG_ERR("Unsupported bandwidth for SX128X: %d", config->bandwidth);
		return -EINVAL;
	}

	if (config->datarate < SF_5 || config->datarate > SF_12) {
		LOG_ERR("Unsupported spreading factor: %d", config->datarate);
		return -EINVAL;
	}
	if (config->frequency < SX128X_FREQ_MIN_HZ || config->frequency > SX128X_FREQ_MAX_HZ ||
	    config->coding_rate < CR_4_5 || config->coding_rate > CR_4_8 ||
	    config->preamble_len == 0 || config->cad.mode < LORA_CAD_MODE_NONE ||
	    config->cad.mode > LORA_CAD_MODE_LBT) {
		return -EINVAL;
	}
	if (config->cad.detection_minimum != 0) {
		return -ENOTSUP;
	}
	switch ((int)config->cad.symbol_num) {
	case 0:
	case LORA_CAD_SYMB_1:
	case LORA_CAD_SYMB_2:
	case LORA_CAD_SYMB_4:
	case LORA_CAD_SYMB_8:
	case LORA_CAD_SYMB_16:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static uint8_t encode_preamble_len(uint16_t symbols)
{
	uint8_t exponent = 0;
	uint32_t mant = symbols ? symbols : 1;

	while (mant > 15 && exponent < 15) {
		mant = (mant + 1) / 2;
		exponent++;
	}

	return (uint8_t)((exponent << 4) | MIN(mant, 15));
}

static int sx128x_set_standby(const struct device *dev, uint8_t mode)
{
	const struct sx128x_hal_config *config = dev->config;
	struct sx128x_data *data = dev->data;
	int ret;

	ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_STANDBY, &mode, 1);

	/*
	 * Whatever was running may have latched an interrupt no one handles.
	 * The global IRQ mask always equals the DIO mask, so a latched flag
	 * holds the DIO line high: with the line low there is nothing to
	 * clear, and the next operation can skip ClearIrqStatus.
	 */
	if (config->irq.port == NULL || gpio_pin_get_dt(&config->irq) != 0) {
		data->irq_clean = false;
	}

	return ret;
}

static int sx128x_set_regulator_mode(const struct device *dev, uint8_t mode)
{
	return sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_REGULATOR_MODE, &mode, 1);
}

static int sx128x_set_buffer_base_address(const struct device *dev, uint8_t tx_base,
					  uint8_t rx_base)
{
	uint8_t buf[2] = {tx_base, rx_base};

	return sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_BUFFER_BASE_ADDRESS, buf, 2);
}

int sx128x_set_packet_type(const struct device *dev, uint8_t type)
{
	return sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_PACKET_TYPE, &type, 1);
}

static int sx128x_set_dio_irq_params(const struct device *dev, uint16_t irq_mask)
{
	const struct sx128x_hal_config *config = dev->config;
	struct sx128x_data *data = dev->data;
	uint8_t buf[8] = {0};
	int ret;

	if (data->irq_mask_valid && data->irq_mask == irq_mask) {
		return 0;
	}

	sys_put_be16(irq_mask, &buf[0]);
	if (config->irq.port != NULL) {
		/* SetDioIrqParams: global mask, then DIO1/2/3 masks. */
		sys_put_be16(irq_mask, &buf[2 * config->irq_dio]);
	}

	ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_DIO_IRQ_PARAMS, buf, 8);
	data->irq_mask = irq_mask;
	data->irq_mask_valid = ret == 0;

	return ret;
}

static int sx128x_clear_irq_status(const struct device *dev, uint16_t mask)
{
	uint8_t buf[2];

	sys_put_be16(mask, buf);
	return sx12xx_hal_write_cmd(dev, SX128X_CMD_CLEAR_IRQ_STATUS, buf, 2);
}

/*
 * The interrupt handler clears every flag it reads, so after an operation
 * that ran to completion nothing is latched and the next one can skip this
 * command. Anything left over would hold the DIO line high and swallow the
 * edge of the next interrupt.
 */
static int sx128x_clear_stale_irqs(const struct device *dev)
{
	struct sx128x_data *data = dev->data;
	int ret;

	if (data->irq_clean) {
		return 0;
	}

	ret = sx128x_clear_irq_status(dev, SX128X_IRQ_ALL);
	data->irq_clean = ret == 0;

	return ret;
}

static int sx128x_get_irq_status(const struct device *dev, uint16_t *status)
{
	uint8_t buf[2];
	int ret;

	ret = sx12xx_hal_read_cmd(dev, SX128X_CMD_GET_IRQ_STATUS, buf, 2);
	if (ret == 0) {
		*status = sys_get_be16(buf);
	}

	return ret;
}

static int sx128x_set_rf_frequency(const struct device *dev, uint32_t freq_hz)
{
	uint32_t freq_reg =
		(uint32_t)(((uint64_t)freq_hz << SX128X_FREQ_STEP_SHIFT) / SX128X_XTAL_FREQ_HZ);
	uint8_t buf[3];

	sys_put_be24(freq_reg, buf);
	return sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_RF_FREQUENCY, buf, 3);
}

static int sx128x_set_modulation_params(const struct device *dev, uint8_t sf, uint8_t bw,
					uint8_t cr)
{
	uint8_t buf[3] = {sf, bw, cr};

	return sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_MODULATION_PARAMS, buf, 3);
}

static int sx128x_set_packet_params(const struct device *dev, uint16_t preamble_symbols,
				    uint8_t header_type, uint8_t payload_len, uint8_t crc_mode,
				    uint8_t iq)
{
	uint8_t buf[7] = {
		encode_preamble_len(preamble_symbols),
		header_type,
		payload_len,
		crc_mode,
		iq,
		0x00,
		0x00,
	};

	return sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_PACKET_PARAMS, buf, 7);
}

static int sx128x_set_tx_params(const struct device *dev, int8_t power_dbm, uint8_t ramp)
{
	const struct sx128x_hal_config *hal = dev->config;
	int8_t power = power_dbm;
	uint8_t buf[2];

	if (hal->tx_power_max_valid && power > hal->tx_power_max_dbm) {
		LOG_WRN("Requested TX power %d dBm exceeds board limit %d dBm "
			"(external PA/FEM) - clamping",
			power_dbm, hal->tx_power_max_dbm);
		power = hal->tx_power_max_dbm;
	}

	power = CLAMP(power, SX128X_TX_POWER_MIN_DBM, SX128X_TX_POWER_MAX_DBM);

	buf[0] = (uint8_t)(power - SX128X_TX_POWER_MIN_DBM);
	buf[1] = ramp;
	return sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_TX_PARAMS, buf, 2);
}

/*
 * SetTx and SetRx take a 16-bit count of a period base. Use 1 ms steps while
 * they reach, 4 ms steps up to about 262 s, and beyond that no timeout in
 * the radio at all: the host's own wait then ends the operation. A count of
 * 0 means no timeout, and for SetRx 0xFFFF means continuous receive.
 */
static void sx128x_encode_timeout(uint32_t timeout_ms, uint8_t buf[3])
{
	uint32_t count;

	if (timeout_ms < SX128X_RX_CONTINUOUS) {
		buf[0] = SX128X_PERIOD_BASE_1MS;
		count = timeout_ms;
	} else if (DIV_ROUND_UP(timeout_ms, 4U) < SX128X_RX_CONTINUOUS) {
		buf[0] = SX128X_PERIOD_BASE_4MS;
		count = DIV_ROUND_UP(timeout_ms, 4U);
	} else {
		buf[0] = SX128X_PERIOD_BASE_1MS;
		count = 0;
	}

	sys_put_be16((uint16_t)count, &buf[1]);
}

static int sx128x_set_tx(const struct device *dev, uint32_t timeout_ms)
{
	const struct sx128x_hal_config *config = dev->config;
	struct sx128x_data *data = dev->data;
	uint8_t buf[3];
	int ret;

	sx128x_encode_timeout(timeout_ms, buf);
	ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_TX, buf, 3);
	if (ret == 0 && config->irq.port == NULL) {
		k_work_reschedule(&data->irq_poll_work, SX128X_IRQ_POLL_INTERVAL);
	}
	return ret;
}

/* timeout_ms of 0 receives continuously */
static int sx128x_set_rx(const struct device *dev, uint32_t timeout_ms)
{
	const struct sx128x_hal_config *config = dev->config;
	struct sx128x_data *data = dev->data;
	uint8_t buf[3];
	int ret;

	sx128x_encode_timeout(timeout_ms, buf);
	if (sys_get_be16(&buf[1]) == 0) {
		sys_put_be16(SX128X_RX_CONTINUOUS, &buf[1]);
	}
	ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_RX, buf, 3);
	if (ret == 0 && config->irq.port == NULL) {
		k_work_reschedule(&data->irq_poll_work, SX128X_IRQ_POLL_INTERVAL);
	}
	return ret;
}

static int sx128x_get_rx_buffer_status(const struct device *dev, uint8_t *payload_len,
				       uint8_t *offset)
{
	uint8_t buf[2];
	int ret;

	ret = sx12xx_hal_read_cmd(dev, SX128X_CMD_GET_RX_BUFFER_STATUS, buf, 2);
	if (ret == 0) {
		*payload_len = buf[0];
		*offset = buf[1];
	}

	return ret;
}

static int sx128x_get_packet_status(const struct device *dev, int16_t *rssi, int8_t *snr)
{
	struct sx128x_data *data = dev->data;
	uint8_t buf[5];
	int ret;

	ret = sx12xx_hal_read_cmd(dev, SX128X_CMD_GET_PACKET_STATUS, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	/* rssiSync is -value/2 dBm. LoRa puts it first and follows it with
	 * the SNR in quarter dB; GFSK has an RFU byte first and no SNR
	 * (datasheet Table 11-65).
	 */
	if (IS_ENABLED(CONFIG_LORA_GFSK) && data->gfsk) {
		*rssi = -((int16_t)buf[1]) / 2;
		*snr = 0;
	} else {
		*rssi = -((int16_t)buf[0]) / 2;
		*snr = ((int8_t)buf[1]) / 4;
	}

	return 0;
}

static void sx128x_set_rf_path(const struct device *dev, bool enable, bool tx)
{
	sx12xx_hal_set_rf_switch(dev, enable, tx);
}

static int sx128x_chip_init(const struct device *dev)
{
	const struct sx128x_hal_config *hal = dev->config;
	int ret;
	uint16_t irq_mask;

	ret = sx128x_hal_reset(dev);
	if (ret < 0) {
		if (hal->reset.port == NULL) {
			LOG_ERR("Power-on startup failed: BUSY did not go low (%d)", ret);
		} else {
			LOG_ERR("Reset failed: %d", ret);
		}
		return ret;
	}

	ret = sx128x_set_standby(dev, SX128X_STDBY_RC);
	if (ret < 0) {
		LOG_ERR("Set standby failed: %d", ret);
		return ret;
	}

	ret = sx128x_set_regulator_mode(dev, hal->regulator_ldo ? SX128X_REGULATOR_LDO
								: SX128X_REGULATOR_DCDC);
	if (ret < 0) {
		LOG_ERR("Set regulator failed: %d", ret);
		return ret;
	}

	ret = sx128x_set_buffer_base_address(dev, 0x00, 0x00);
	if (ret < 0) {
		LOG_ERR("Set buffer base failed: %d", ret);
		return ret;
	}

	ret = sx128x_set_packet_type(dev, SX128X_PACKET_TYPE_LORA);
	if (ret < 0) {
		LOG_ERR("Set packet type failed: %d", ret);
		return ret;
	}

	irq_mask = SX128X_IRQ_TXRX;
	ret = sx128x_set_dio_irq_params(dev, irq_mask);
	if (ret < 0) {
		LOG_ERR("Set IRQ params failed: %d", ret);
		return ret;
	}

	ret = sx128x_clear_stale_irqs(dev);
	if (ret < 0) {
		LOG_ERR("Clear IRQ failed: %d", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_LORA_SX128X_NATIVE_AUTO_FS)) {
		uint8_t enable = 1;

		ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_AUTO_FS, &enable, 1);
		if (ret < 0) {
			LOG_ERR("Set AutoFS failed: %d", ret);
			return ret;
		}
	}

	/*
	 * Read the packet type back as a proof of SPI communication: MISO stuck
	 * low or high reads as 0x00 or 0xFF, neither of them LoRa
	 */
	uint8_t packet_type = 0xFF;

	ret = sx12xx_hal_read_cmd(dev, SX128X_CMD_GET_PACKET_TYPE, &packet_type, 1);
	if (ret < 0 || packet_type != SX128X_PACKET_TYPE_LORA) {
		LOG_ERR("Packet type readback failed: ret=%d type=0x%02x", ret, packet_type);
		return ret < 0 ? ret : -EIO;
	}

	LOG_INF("SX128X initialized");
	return 0;
}

static void sx128x_irq_callback(const struct device *dev)
{
	struct sx128x_data *data = dev->data;

	k_work_submit_to_queue(data->irq_workq, &data->irq_work);
}

static int sx128x_set_tx_base(const struct device *dev, uint8_t base);
static int sx128x_set_modem_packet_params(const struct device *dev, uint8_t payload_len);

static uint32_t sx128x_lora_airtime(const struct device *dev, uint32_t data_len);

static uint32_t sx128x_tx_timeout_ms(const struct device *dev, uint8_t data_len)
{
	return 2U * sx128x_lora_airtime(dev, data_len) + SX128X_TX_TIMEOUT_MARGIN_MS;
}

static void sx128x_tx_complete(const struct device *dev, struct k_poll_signal *async, int status)
{
	struct sx128x_data *data = dev->data;
	struct sx128x_tx_result result = {.status = status};

	if (async != NULL) {
		k_poll_signal_raise(async, status);
	} else {
		k_msgq_put(&data->tx_msgq, &result, K_NO_WAIT);
	}
}

/* Send the queued frame right behind the one that just finished */
static int sx128x_tx_next(const struct device *dev)
{
	struct sx128x_data *data = dev->data;
	int ret = 0;

	if (!data->tx_next.in_chip) {
		ret = sx128x_set_tx_base(dev, data->tx_next.base);
		if (ret == 0) {
			ret = sx12xx_hal_write_buffer(dev, data->tx_next.base, data->tx_next.buf,
						      data->tx_next.len);
		}
	}
	if (ret == 0) {
		ret = sx128x_set_modem_packet_params(dev, data->tx_next.len);
	}
	if (ret == 0) {
		ret = sx128x_set_tx(dev, sx128x_tx_timeout_ms(dev, data->tx_next.len));
	}

	data->tx_signal = data->tx_next.signal;
	data->tx_len = data->tx_next.len;
	data->tx_next.pending = false;

	return ret;
}

static void sx128x_handle_irq_tx_done(const struct device *dev)
{
	struct sx128x_data *data = dev->data;
	struct k_poll_signal *done;
	int ret;

	LOG_DBG("TX done");
	k_mutex_lock(&data->lock, K_FOREVER);
	done = data->tx_signal;
	data->tx_signal = NULL;

	if (data->tx_next.pending) {
		ret = sx128x_tx_next(dev);
		if (ret < 0) {
			LOG_ERR("Queued frame not sent: %d", ret);
			sx128x_tx_complete(dev, data->tx_signal, ret);
			data->tx_signal = NULL;
			(void)sx128x_set_standby(dev, SX128X_STDBY_RC);
			sx128x_set_rf_path(dev, false, false);
			atomic_set(&data->state, SX128X_REST_STATE);
		}
	} else {
		sx128x_set_rf_path(dev, false, false);
		atomic_set(&data->state, SX128X_REST_STATE);
	}

	k_mutex_unlock(&data->lock);
	sx128x_tx_complete(dev, done, 0);
}

static void sx128x_handle_irq_rx_done(const struct device *dev, uint16_t irq_status)
{
	struct sx128x_data *data = dev->data;
	struct sx128x_rx_result result = {0};
	uint8_t payload_len = 0, offset = 0;
	int ret;

	ret = sx128x_get_rx_buffer_status(dev, &payload_len, &offset);
	if (ret < 0) {
		LOG_ERR("Failed to get RX buffer status");
		result.status = ret;
		goto out;
	}

	if (irq_status & SX128X_IRQ_RX_ERROR) {
		LOG_DBG("RX error, IRQ 0x%04x", irq_status);
		result.status = -EIO;
		goto out;
	}

	/*
	 * Payload before packet status: in continuous receive the next packet
	 * lands at the same buffer offset, so the read has to start before its
	 * first byte does.
	 */
	result.len = MIN(payload_len, sizeof(result.payload));
	ret = sx12xx_hal_read_buffer(dev, offset, result.payload, result.len);
	if (ret < 0) {
		LOG_ERR("Failed to read RX buffer");
		result.status = ret;
		goto out;
	}

	(void)sx128x_get_packet_status(dev, &result.rssi, &result.snr);

	result.status = result.len;
	LOG_DBG("RX done: %d bytes, RSSI=%d, SNR=%d", result.len, result.rssi, result.snr);

out:
	if (data->recv_cb == NULL) {
		/* Blocking receive may be followed immediately by a reply. */
		ret = sx128x_set_standby(dev, SX128X_STDBY_RC);
		if (ret < 0) {
			LOG_ERR("Failed to stop RX after packet: %d", ret);
			result.status = ret;
		}
		sx128x_set_rf_path(dev, false, false);
		atomic_set(&data->state, SX128X_REST_STATE);
	}
	if (data->recv_cb != NULL) {
		if (result.status > 0) {
			data->recv_cb(dev, result.payload, result.len, result.rssi, result.snr,
				      data->recv_user_data);
		}
		if (data->rx_duty_cycle && data->recv_cb != NULL) {
			ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_RX_DUTY_CYCLE,
						   data->rx_duty_params, 5);
		} else if ((irq_status & SX128X_IRQ_HEADER_ERROR) && data->recv_cb != NULL) {
			/* Continuous receive does not resume by itself */
			ret = sx128x_set_rx(dev, 0);
		} else {
			ret = 0;
		}
		if (ret < 0 && data->recv_cb != NULL) {
			lora_recv_cb cb = data->recv_cb;
			void *user_data = data->recv_user_data;

			data->recv_cb = NULL;
			data->rx_duty_cycle = false;
			atomic_set(&data->state, SX128X_REST_STATE);
			cb(dev, NULL, 0, 0, 0, user_data);
		}
	} else if (k_msgq_put(&data->rx_msgq, &result, K_NO_WAIT) < 0) {
		LOG_DBG("RX queue full, dropping packet");
	}
}

static void sx128x_handle_irq_timeout(const struct device *dev)
{
	struct sx128x_data *data = dev->data;

	LOG_DBG("Timeout");
	sx128x_set_rf_path(dev, false, false);

	if (atomic_get(&data->state) == SX128X_STATE_TX) {
		struct k_poll_signal *failed;
		struct k_poll_signal *dropped;
		bool had_next;

		/* Same lock as lora_send_async(), so no frame can be queued
		 * behind a transmission that has just been given up on
		 */
		k_mutex_lock(&data->lock, K_FOREVER);
		failed = data->tx_signal;
		dropped = data->tx_next.signal;
		had_next = data->tx_next.pending;
		data->tx_signal = NULL;
		data->tx_next.pending = false;
		atomic_set(&data->state, SX128X_REST_STATE);
		k_mutex_unlock(&data->lock);

		sx128x_tx_complete(dev, failed, -ETIMEDOUT);
		if (had_next) {
			sx128x_tx_complete(dev, dropped, -ECANCELED);
		}
	} else if (atomic_get(&data->state) == SX128X_STATE_RX) {
		struct sx128x_rx_result result = {.status = -EAGAIN};

		atomic_set(&data->state, SX128X_REST_STATE);
		if (data->recv_cb != NULL) {
			lora_recv_cb cb = data->recv_cb;
			void *user_data = data->recv_user_data;

			data->recv_cb = NULL;
			cb(dev, NULL, 0, 0, 0, user_data);
		} else {
			k_msgq_put(&data->rx_msgq, &result, K_NO_WAIT);
		}
	}
}

static void sx128x_irq_work_handler(struct k_work *work)
{
	struct sx128x_data *data = CONTAINER_OF(work, struct sx128x_data, irq_work);
	const struct device *dev = data->dev;
	uint16_t irq_status = 0;
	int ret;

	ret = sx128x_get_irq_status(dev, &irq_status);
	if (ret < 0) {
		LOG_ERR("Failed to get IRQ status");
		return;
	}

	LOG_DBG("IRQ status: 0x%04x", irq_status);
	if (irq_status == 0) {
		return;
	}
	data->irq_clean = sx128x_clear_irq_status(dev, irq_status) == 0;
	if ((irq_status & SX128X_IRQ_CAD_DONE) && atomic_get(&data->state) == SX128X_STATE_CAD) {
		data->cad_detected = (irq_status & SX128X_IRQ_CAD_DETECTED) != 0;
		sx128x_set_rf_path(dev, false, false);
		atomic_set(&data->state, SX128X_REST_STATE);
		k_sem_give(&data->cad_done);
		if (data->cad_cb != NULL) {
			lora_cad_cb cb = data->cad_cb;
			void *user_data = data->cad_user_data;

			data->cad_cb = NULL;
			cb(dev, data->cad_detected, user_data);
		}
	}

	if ((irq_status & SX128X_IRQ_TX_DONE) && atomic_get(&data->state) == SX128X_STATE_TX) {
		sx128x_handle_irq_tx_done(dev);
	}

	if ((irq_status & (SX128X_IRQ_RX_DONE | SX128X_IRQ_HEADER_ERROR)) &&
	    atomic_get(&data->state) == SX128X_STATE_RX) {
		sx128x_handle_irq_rx_done(dev, irq_status);
	}

	if (irq_status & SX128X_IRQ_RX_TX_TIMEOUT) {
		sx128x_handle_irq_timeout(dev);
	}

	/*
	 * The IRQ line is edge triggered: a flag the radio raised after the
	 * status was read keeps it active, and no further edge comes. Without
	 * another pass the radio would stay deaf until a reconfiguration.
	 */
	if (sx128x_hal_irq_active(dev) > 0) {
		data->irq_clean = false;
		k_work_submit_to_queue(data->irq_workq, &data->irq_work);
	}
}

static void sx128x_irq_poll_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sx128x_data *data = CONTAINER_OF(dwork, struct sx128x_data, irq_poll_work);

	if (atomic_get(&data->state) == SX128X_STATE_STANDBY) {
		return;
	}

	sx128x_irq_work_handler(&data->irq_work);
	if (atomic_get(&data->state) != SX128X_STATE_STANDBY) {
		k_work_reschedule(&data->irq_poll_work, SX128X_IRQ_POLL_INTERVAL);
	}
}

/*
 * Take the radio for a configuration change. On success the lock is held and
 * the previous configuration is no longer valid; sx128x_config_end() releases
 * both, whether or not the new one was applied.
 */
int sx128x_config_begin(const struct device *dev)
{
	struct sx128x_data *data = dev->data;

	if (!atomic_cas(&data->state, SX128X_REST_STATE, SX128X_STATE_CONFIG)) {
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->config_valid = false;
	data->packet_params_valid = false;

	return 0;
}

void sx128x_config_end(const struct device *dev)
{
	struct sx128x_data *data = dev->data;

	k_mutex_unlock(&data->lock);
	atomic_set(&data->state, SX128X_REST_STATE);
}

int sx128x_config_carrier(const struct device *dev, uint32_t frequency, int8_t tx_power)
{
	int ret;

	ret = sx128x_set_rf_frequency(dev, frequency);
	if (ret < 0) {
		return ret;
	}

	return sx128x_set_tx_params(dev, tx_power, SX128X_TX_RAMP_20_US);
}

static int sx128x_set_lora_sync_word(const struct device *dev,
				     const struct lora_modem_config *config)
{
	uint16_t sync_word;
	uint8_t buf[2];

	if (config->sync_word != 0) {
		/* Same nibble layout as the SX126x, and as the other backends */
		sync_word = 0x0404 | ((config->sync_word & 0xF0) << 8) |
			    ((config->sync_word & 0x0F) << 4);
	} else {
		sync_word = config->public_network ? SX128X_LORA_SYNC_WORD_PUBLIC
						   : SX128X_LORA_SYNC_WORD_PRIVATE;
	}

	sys_put_be16(sync_word, buf);
	return sx12xx_hal_write_regs(dev, SX128X_REG_LORA_SYNC_WORD, buf, 2);
}

static int sx128x_lora_config(const struct device *dev, const struct lora_modem_config *config)
{
	struct sx128x_data *data = dev->data;
	uint8_t bw_reg = 0;
	uint8_t sf_config;
	uint8_t freq_error = 0x01;
	int ret;

	ret = validate_config(config);
	if (ret < 0) {
		return ret;
	}

	ret = sx128x_config_begin(dev);
	if (ret < 0) {
		return ret;
	}

	/* Every configuration sets its packet type, since a GFSK one may
	 * have changed it since the last LoRa one.
	 */
	ret = sx128x_set_packet_type(dev, SX128X_PACKET_TYPE_LORA);
	if (ret < 0) {
		goto out;
	}

	ret = sx128x_config_carrier(dev, config->frequency, config->tx_power);
	if (ret < 0) {
		goto out;
	}

	ret = bandwidth_to_reg(config->bandwidth, &bw_reg);
	__ASSERT_NO_MSG(ret == 0);

	/* SF register value is SF << 4 (matches SX128X_LORA_SF5..SX128X_LORA_SF12) */
	ret = sx128x_set_modulation_params(dev, (uint8_t)(config->datarate << 4), bw_reg,
					   (uint8_t)config->coding_rate);
	if (ret < 0) {
		goto out;
	}

	/* SF-dependent detection settings required after SetModulationParams. */
	sf_config = config->datarate <= SF_6 ? 0x1E : config->datarate <= SF_8 ? 0x37 : 0x32;
	ret = sx12xx_hal_write_regs(dev, SX128X_REG_LORA_SF_CONFIG, &sf_config, 1);
	if (ret < 0) {
		goto out;
	}
	ret = sx12xx_hal_write_regs(dev, SX128X_REG_LORA_FREQ_ERROR, &freq_error, 1);
	if (ret < 0) {
		goto out;
	}
	ret = sx128x_set_lora_sync_word(dev, config);
	if (ret < 0) {
		goto out;
	}

	memcpy(&data->config, config, sizeof(*config));
	data->gfsk = false;
	data->config_valid = true;
	LOG_DBG("Config: freq=%u, SF=%d, BW=%d, CR=%d, power=%d", config->frequency,
		config->datarate, config->bandwidth, config->coding_rate, config->tx_power);

out:
	sx128x_config_end(dev);
	return ret;
}

static int sx128x_set_lora_packet_params(const struct device *dev, uint8_t payload_len)
{
	struct sx128x_data *data = dev->data;

	return sx128x_set_packet_params(
		dev, data->config.preamble_len, SX128X_LORA_HEADER_EXPLICIT, payload_len,
		data->config.packet_crc_disable ? SX128X_LORA_CRC_OFF : SX128X_LORA_CRC_ON,
		data->config.iq_inverted ? SX128X_LORA_IQ_INVERTED : SX128X_LORA_IQ_STD);
}

/* Packet parameters for whichever modem the radio was last set up for */
static int sx128x_set_modem_packet_params(const struct device *dev, uint8_t payload_len)
{
	struct sx128x_data *data = dev->data;
	int ret;

	if (data->packet_params_valid && data->packet_params_len == payload_len) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_LORA_GFSK) && data->gfsk) {
		ret = sx128x_set_gfsk_packet_params(dev, &data->gfsk_config, payload_len);
	} else {
		ret = sx128x_set_lora_packet_params(dev, payload_len);
	}

	data->packet_params_len = payload_len;
	data->packet_params_valid = ret == 0;

	return ret;
}

static int sx128x_lora_cad(const struct device *dev, k_timeout_t timeout);

static k_timeout_t sx128x_cad_timeout(const struct lora_modem_config *cfg)
{
	uint32_t symbols = cfg->cad.symbol_num ? cfg->cad.symbol_num : 2;
	uint32_t duration_ms =
		DIV_ROUND_UP(symbols * (1U << cfg->datarate), (uint32_t)cfg->bandwidth);

	return K_MSEC(duration_ms + 20);
}

static int sx128x_set_tx_base(const struct device *dev, uint8_t base)
{
	struct sx128x_data *data = dev->data;
	int ret;

	if (data->tx_base == base) {
		return 0;
	}

	/* Receptions always start at offset 0 */
	ret = sx128x_set_buffer_base_address(dev, base, 0x00);
	if (ret == 0) {
		data->tx_base = base;
	}

	return ret;
}

/* Put a frame on air from an idle radio. Called with the lock held. */
static int sx128x_tx_start(const struct device *dev, uint8_t *data_buf, uint8_t data_len)
{
	int ret;

	ret = sx128x_set_dio_irq_params(dev, SX128X_IRQ_TXRX);
	if (ret == 0) {
		ret = sx128x_clear_stale_irqs(dev);
	}
	if (ret == 0) {
		ret = sx128x_set_modem_packet_params(dev, data_len);
	}
	if (ret == 0) {
		ret = sx128x_set_tx_base(dev, 0x00);
	}
	if (ret == 0) {
		ret = sx12xx_hal_write_buffer(dev, 0x00, data_buf, data_len);
	}
	if (ret == 0) {
		sx128x_set_rf_path(dev, true, true);
		ret = sx128x_set_tx(dev, sx128x_tx_timeout_ms(dev, data_len));
	}

	return ret;
}

/*
 * Take a frame while another one is on air. If both fit in half of the chip
 * buffer, write this one to the other half now, while the first is still
 * being sent; otherwise keep a copy until the first one is out. Called with
 * the lock held.
 */
static int sx128x_tx_queue(const struct device *dev, uint8_t *data_buf, uint8_t data_len,
			   struct k_poll_signal *async)
{
	struct sx128x_data *data = dev->data;
	int ret = 0;

	data->tx_next.in_chip =
		data->tx_len <= SX128X_TX_HALF_LEN && data_len <= SX128X_TX_HALF_LEN;
	if (data->tx_next.in_chip) {
		data->tx_next.base = data->tx_base == 0x00 ? SX128X_TX_HALF_LEN : 0x00;
		ret = sx12xx_hal_write_buffer(dev, data->tx_next.base, data_buf, data_len);
		if (ret == 0) {
			/* The radio loads its TX pointer from the base address
			 * when it enters TX (datasheet 8.3), so moving the base
			 * does not disturb the frame on air. Packet parameters
			 * are different: the frame on air stops at the length
			 * they hold, so they wait for it to finish.
			 */
			ret = sx128x_set_tx_base(dev, data->tx_next.base);
		}
	} else {
		data->tx_next.base = 0x00;
		memcpy(data->tx_next.buf, data_buf, data_len);
	}

	if (ret == 0) {
		data->tx_next.len = data_len;
		data->tx_next.signal = async;
		data->tx_next.pending = true;
	}

	return ret;
}

static int sx128x_lora_start_send(const struct device *dev, uint8_t *data_buf, uint32_t data_len,
				  struct k_poll_signal *async, bool may_queue)
{
	struct sx128x_data *data = dev->data;
	int ret;

	if (!data->config_valid) {
		LOG_ERR("Not configured");
		return -EINVAL;
	}
	if (data_len > SX128X_MAX_PAYLOAD_LEN) {
		LOG_ERR("Payload too long: %u", data_len);
		return -EINVAL;
	}
	if (IS_ENABLED(CONFIG_LORA_GFSK) && data->gfsk && data->gfsk_config.fixed_len &&
	    data_len != data->gfsk_config.payload_len) {
		LOG_ERR("Fixed-length GFSK takes %u bytes, got %u", data->gfsk_config.payload_len,
			data_len);
		return -EINVAL;
	}
	if (!data->gfsk && data->config.cad.mode == LORA_CAD_MODE_LBT) {
		/* Listening first makes no sense behind a frame on air */
		may_queue = false;
		ret = sx128x_lora_cad(dev, sx128x_cad_timeout(&data->config));
		if (ret != 0) {
			if (ret > 0 && async != NULL) {
				k_poll_signal_raise(async, -EBUSY);
				return 0;
			}
			return ret > 0 ? -EBUSY : ret;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (atomic_cas(&data->state, SX128X_REST_STATE, SX128X_STATE_TX)) {
		k_msgq_purge(&data->tx_msgq);
		data->tx_signal = async;
		data->tx_len = (uint8_t)data_len;
		ret = sx128x_tx_start(dev, data_buf, (uint8_t)data_len);
		if (ret < 0) {
			data->tx_signal = NULL;
			sx128x_set_rf_path(dev, false, false);
			atomic_set(&data->state, SX128X_REST_STATE);
		}
	} else if (may_queue && atomic_get(&data->state) == SX128X_STATE_TX &&
		   !data->tx_next.pending) {
		ret = sx128x_tx_queue(dev, data_buf, (uint8_t)data_len, async);
	} else {
		ret = -EBUSY;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int sx128x_lora_send(const struct device *dev, uint8_t *data_buf, uint32_t data_len)
{
	struct sx128x_data *data = dev->data;
	struct sx128x_tx_result result;
	int ret = sx128x_lora_start_send(dev, data_buf, data_len, NULL, false);

	if (ret < 0) {
		return ret;
	}

	ret = k_msgq_get(
		&data->tx_msgq, &result,
		K_MSEC(sx128x_tx_timeout_ms(dev, (uint8_t)data_len) + SX128X_TX_TIMEOUT_MARGIN_MS));
	if (ret < 0) {
		struct k_poll_signal *dropped;
		bool had_next;

		LOG_ERR("TX timeout");
		/* Give up on the queue too, under the lock it is filled with */
		k_mutex_lock(&data->lock, K_FOREVER);
		dropped = data->tx_next.signal;
		had_next = data->tx_next.pending;
		data->tx_next.pending = false;
		data->tx_signal = NULL;
		(void)sx128x_set_standby(dev, SX128X_STDBY_RC);
		sx128x_set_rf_path(dev, false, false);
		atomic_set(&data->state, SX128X_REST_STATE);
		k_mutex_unlock(&data->lock);

		if (had_next) {
			sx128x_tx_complete(dev, dropped, -ECANCELED);
		}
		return -ETIMEDOUT;
	}

	return result.status;
}

static int sx128x_lora_send_async(const struct device *dev, uint8_t *data_buf, uint32_t data_len,
				  struct k_poll_signal *async)
{
	return sx128x_lora_start_send(dev, data_buf, data_len, async, true);
}

static int sx128x_lora_cad_start(const struct device *dev, lora_cad_cb cb, void *user_data)
{
	struct sx128x_data *data = dev->data;
	uint8_t symbols = data->config.cad.symbol_num ? data->config.cad.symbol_num : 2;
	uint8_t cad_param = (uint8_t)(__builtin_ctz(symbols) << 5);
	uint8_t peak = data->config.cad.detection_peak;
	int ret;

	if (!data->config_valid) {
		return -EINVAL;
	}
	if (data->gfsk) {
		/* Channel activity detection looks for a LoRa preamble */
		return -ENOTSUP;
	}
	if (!atomic_cas(&data->state, SX128X_REST_STATE, SX128X_STATE_CAD)) {
		return -EBUSY;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	k_sem_reset(&data->cad_done);
	data->cad_cb = cb;
	data->cad_user_data = user_data;
	ret = sx128x_clear_stale_irqs(dev);
	if (ret < 0) {
		goto fail;
	}
	ret = sx128x_set_dio_irq_params(dev, SX128X_IRQ_CAD_DONE | SX128X_IRQ_CAD_DETECTED);
	if (ret < 0) {
		goto fail;
	}
	ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_CAD_PARAMS, &cad_param, 1);
	if (ret < 0) {
		goto fail;
	}
	if (peak != 0) {
		ret = sx12xx_hal_write_regs(dev, SX128X_REG_LORA_CAD_PEAK, &peak, 1);
		if (ret < 0) {
			goto fail;
		}
	}
	sx128x_set_rf_path(dev, true, false);
	ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_CAD, NULL, 0);
	if (ret == 0) {
		const struct sx128x_hal_config *hal = dev->config;

		if (hal->irq.port == NULL) {
			k_work_reschedule(&data->irq_poll_work, SX128X_IRQ_POLL_INTERVAL);
		}
		k_mutex_unlock(&data->lock);
		return 0;
	}
fail:
	data->cad_cb = NULL;
	sx128x_set_rf_path(dev, false, false);
	atomic_set(&data->state, SX128X_REST_STATE);
	k_mutex_unlock(&data->lock);
	return ret;
}

static int sx128x_lora_cad(const struct device *dev, k_timeout_t timeout)
{
	struct sx128x_data *data = dev->data;
	int ret = sx128x_lora_cad_start(dev, NULL, NULL);

	if (ret < 0) {
		return ret;
	}
	ret = k_sem_take(&data->cad_done, timeout);
	if (ret < 0) {
		k_mutex_lock(&data->lock, K_FOREVER);
		sx128x_set_standby(dev, SX128X_STDBY_RC);
		sx128x_set_rf_path(dev, false, false);
		atomic_set(&data->state, SX128X_REST_STATE);
		k_mutex_unlock(&data->lock);
		return -ETIMEDOUT;
	}
	return data->cad_detected ? 1 : 0;
}

static int sx128x_lora_cad_async(const struct device *dev, lora_cad_cb cb, void *user_data)
{
	struct sx128x_data *data = dev->data;

	if (cb == NULL) {
		if (atomic_get(&data->state) != SX128X_STATE_CAD || data->cad_cb == NULL) {
			return 0;
		}
		k_mutex_lock(&data->lock, K_FOREVER);
		data->cad_cb = NULL;
		sx128x_set_standby(dev, SX128X_STDBY_RC);
		sx128x_set_rf_path(dev, false, false);
		atomic_set(&data->state, SX128X_REST_STATE);
		k_mutex_unlock(&data->lock);
		return 0;
	}
	return sx128x_lora_cad_start(dev, cb, user_data);
}

static int sx128x_lora_recv(const struct device *dev, uint8_t *data_buf, uint8_t size,
			    k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	struct sx128x_data *data = dev->data;
	struct sx128x_rx_result result;
	uint32_t timeout_ms;
	int ret;

	if (!data->config_valid) {
		LOG_ERR("Not configured");
		return -EINVAL;
	}
	if (!data->gfsk && data->config.cad.mode == LORA_CAD_MODE_RX) {
		ret = sx128x_lora_cad(dev, sx128x_cad_timeout(&data->config));
		if (ret <= 0) {
			return ret;
		}
	}

	if (!atomic_cas(&data->state, SX128X_REST_STATE, SX128X_STATE_RX)) {
		LOG_ERR("Busy");
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	k_msgq_purge(&data->rx_msgq);
	ret = sx128x_set_dio_irq_params(dev, SX128X_IRQ_TXRX);
	if (ret < 0) {
		atomic_set(&data->state, SX128X_REST_STATE);
		k_mutex_unlock(&data->lock);
		return ret;
	}
	ret = sx128x_clear_stale_irqs(dev);
	if (ret < 0) {
		atomic_set(&data->state, SX128X_REST_STATE);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = sx128x_set_modem_packet_params(dev, SX128X_MAX_PAYLOAD_LEN);
	if (ret < 0) {
		atomic_set(&data->state, SX128X_REST_STATE);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	sx128x_set_rf_path(dev, true, false);

	timeout_ms = K_TIMEOUT_EQ(timeout, K_FOREVER) ? 0 : k_ticks_to_ms_ceil32(timeout.ticks);
	ret = sx128x_set_rx(dev, timeout_ms);
	if (ret < 0) {
		sx128x_set_rf_path(dev, false, false);
		atomic_set(&data->state, SX128X_REST_STATE);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	k_mutex_unlock(&data->lock);

	ret = k_msgq_get(&data->rx_msgq, &result, timeout);
	if (ret < 0) {
		LOG_DBG("RX timeout");
		sx128x_set_standby(dev, SX128X_STDBY_RC);
		sx128x_set_rf_path(dev, false, false);
		atomic_set(&data->state, SX128X_REST_STATE);
		return -EAGAIN;
	}

	if (result.status > 0) {
		int copy_len = MIN(result.status, size);

		memcpy(data_buf, result.payload, copy_len);
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

static int sx128x_lora_recv_async(const struct device *dev, lora_recv_cb cb, void *user_data)
{
	struct sx128x_data *data = dev->data;
	int ret;

	if (cb == NULL) {
		if (data->recv_cb == NULL || atomic_get(&data->state) != SX128X_STATE_RX) {
			return 0;
		}
		bool duty_cycle = data->rx_duty_cycle;
		uint8_t disable = 0;

		k_mutex_lock(&data->lock, K_FOREVER);
		if (duty_cycle) {
			(void)sx12xx_hal_wakeup(dev);
		}
		data->recv_cb = NULL;
		data->rx_duty_cycle = false;
		ret = sx128x_set_standby(dev, SX128X_STDBY_RC);
		if (duty_cycle) {
			(void)sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_LONG_PREAMBLE, &disable, 1);
		}
		sx128x_set_rf_path(dev, false, false);
		atomic_set(&data->state, SX128X_REST_STATE);
		k_mutex_unlock(&data->lock);
		return ret;
	}
	if (!data->config_valid) {
		return -EINVAL;
	}
	if (!atomic_cas(&data->state, SX128X_REST_STATE, SX128X_STATE_RX)) {
		return -EBUSY;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = sx128x_set_dio_irq_params(dev, SX128X_IRQ_TXRX);
	if (ret < 0) {
		goto fail;
	}
	ret = sx128x_clear_stale_irqs(dev);
	if (ret < 0) {
		goto fail;
	}
	ret = sx128x_set_modem_packet_params(dev, SX128X_MAX_PAYLOAD_LEN);
	if (ret < 0) {
		goto fail;
	}
	data->recv_cb = cb;
	data->recv_user_data = user_data;
	data->rx_duty_cycle = false;
	sx128x_set_rf_path(dev, true, false);
	ret = sx128x_set_rx(dev, 0);
	if (ret == 0) {
		k_mutex_unlock(&data->lock);
		return 0;
	}
fail:
	data->recv_cb = NULL;
	sx128x_set_rf_path(dev, false, false);
	atomic_set(&data->state, SX128X_REST_STATE);
	k_mutex_unlock(&data->lock);
	return ret;
}

static int sx128x_lora_recv_duty_cycle_async(const struct device *dev, k_timeout_t rx_period,
					     k_timeout_t sleep_period, lora_recv_cb cb,
					     void *user_data)
{
	struct sx128x_data *data = dev->data;
	uint32_t rx_ms, sleep_ms;
	uint8_t enable = 1;
	int ret;

	if (cb == NULL) {
		return sx128x_lora_recv_async(dev, NULL, NULL);
	}
	if (!data->config_valid || K_TIMEOUT_EQ(rx_period, K_FOREVER) ||
	    K_TIMEOUT_EQ(sleep_period, K_FOREVER)) {
		return -EINVAL;
	}
	if (data->gfsk) {
		/* Waking on a long preamble is a LoRa feature */
		return -ENOTSUP;
	}
	const struct sx128x_hal_config *hal = dev->config;

	if (hal->irq.port == NULL) {
		return -ENOTSUP;
	}
	rx_ms = k_ticks_to_ms_ceil32(rx_period.ticks);
	sleep_ms = k_ticks_to_ms_ceil32(sleep_period.ticks);
	if (rx_ms == 0 || sleep_ms == 0 || rx_ms > 262140 || sleep_ms > 262140) {
		return -EINVAL;
	}
	if (!atomic_cas(&data->state, SX128X_REST_STATE, SX128X_STATE_RX)) {
		return -EBUSY;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = sx128x_set_dio_irq_params(dev, SX128X_IRQ_RX_DONE | SX128X_IRQ_RX_ERROR);
	if (ret < 0) {
		goto fail;
	}
	ret = sx128x_clear_stale_irqs(dev);
	if (ret < 0) {
		goto fail;
	}
	ret = sx128x_set_modem_packet_params(dev, SX128X_MAX_PAYLOAD_LEN);
	if (ret < 0) {
		goto fail;
	}
	ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_LONG_PREAMBLE, &enable, 1);
	if (ret < 0) {
		goto fail;
	}
	/* PeriodBase 1 ms below 65.535 s, 4 ms above it. */
	uint32_t step = MAX(rx_ms, sleep_ms) > UINT16_MAX ? 4 : 1;

	data->rx_duty_params[0] = step == 4 ? SX128X_PERIOD_BASE_4MS : SX128X_PERIOD_BASE_1MS;
	sys_put_be16(DIV_ROUND_UP(rx_ms, step), &data->rx_duty_params[1]);
	sys_put_be16(DIV_ROUND_UP(sleep_ms, step), &data->rx_duty_params[3]);
	data->recv_cb = cb;
	data->recv_user_data = user_data;
	data->rx_duty_cycle = true;
	sx128x_set_rf_path(dev, true, false);
	ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_RX_DUTY_CYCLE, data->rx_duty_params, 5);
	if (ret == 0) {
		k_mutex_unlock(&data->lock);
		return 0;
	}
fail:
	data->recv_cb = NULL;
	data->rx_duty_cycle = false;
	sx128x_set_rf_path(dev, false, false);
	atomic_set(&data->state, SX128X_REST_STATE);
	k_mutex_unlock(&data->lock);
	return ret;
}

static uint32_t sx128x_lora_airtime(const struct device *dev, uint32_t data_len)
{
	struct sx128x_data *data = dev->data;
	uint32_t bw_hz, preamble;

	if (!data->config_valid) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_LORA_GFSK) && data->gfsk) {
		return sx12xx_gfsk_airtime_ms(&data->gfsk_config, data_len,
					      SX128X_GFSK_HEADER_BITS);
	}

	switch (data->config.bandwidth) {
	case BW_200_KHZ:
		bw_hz = 203125;
		break;
	case BW_400_KHZ:
		bw_hz = 406250;
		break;
	case BW_800_KHZ:
		bw_hz = 812500;
		break;
	case BW_1600_KHZ:
		bw_hz = 1625000;
		break;
	default:
		return 0;
	}

	/* The radio sends the preamble it can encode, which may be longer */
	preamble = encode_preamble_len(data->config.preamble_len);
	preamble = (preamble & 0x0F) << (preamble >> 4);

	/* No low data rate optimization on the SX128x */
	return sx12xx_lora_airtime_ms(bw_hz, data->config.datarate, data->config.coding_rate,
				      preamble, false, !data->config.packet_crc_disable, data_len);
}

static int sx128x_init(const struct device *dev)
{
	struct sx128x_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->cad_done, 0, 1);
	k_msgq_init(&data->tx_msgq, (char *)&data->tx_result, sizeof(struct sx128x_tx_result), 1);
	k_msgq_init(&data->rx_msgq, (char *)&data->rx_result, sizeof(struct sx128x_rx_result), 1);
	k_work_init(&data->irq_work, sx128x_irq_work_handler);
	k_work_init_delayable(&data->irq_poll_work, sx128x_irq_poll_work_handler);

	/* One queue serves every instance; starting it twice would leak a
	 * thread, so only the first instance to initialize starts it.
	 */
	if (!sx128x_irq_workq_started) {
		struct k_work_queue_config cfg = {
			.name = "sx128x_irq",
			.no_yield = true,
		};

		k_work_queue_start(&sx128x_irq_workq, sx128x_irq_stack,
				   K_THREAD_STACK_SIZEOF(sx128x_irq_stack),
				   K_PRIO_COOP(CONFIG_LORA_SX128X_NATIVE_IRQ_THREAD_PRIO), &cfg);
		sx128x_irq_workq_started = true;
	}
	data->irq_workq = &sx128x_irq_workq;
	data->dev = dev;
	atomic_set(&data->state, SX128X_STATE_STANDBY);
	data->config_valid = false;

	ret = sx128x_hal_init(dev);
	if (ret < 0) {
		LOG_ERR("HAL init failed: %d", ret);
		return ret;
	}

	ret = sx128x_hal_set_irq_callback(dev, sx128x_irq_callback);
	if (ret < 0) {
		LOG_ERR("IRQ pin callback setup failed: %d", ret);
		return ret;
	}

	ret = sx128x_chip_init(dev);
	if (ret < 0) {
		LOG_ERR("Chip init failed: %d", ret);
		return ret;
	}

	return 0;
}

static int sx128x_lora_test_cw(const struct device *dev, uint32_t frequency, int8_t tx_power,
			       uint16_t duration)
{
	struct sx128x_data *data = dev->data;
	int ret;

	if (!atomic_cas(&data->state, SX128X_REST_STATE, SX128X_STATE_TX)) {
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = sx128x_set_rf_frequency(dev, frequency);
	if (ret < 0) {
		goto out;
	}

	/* tx_power is still clamped against tx-power-max-dbm inside here */
	ret = sx128x_set_tx_params(dev, tx_power, SX128X_TX_RAMP_20_US);
	if (ret < 0) {
		goto out;
	}

	sx128x_set_rf_path(dev, true, true);

	ret = sx12xx_hal_write_cmd(dev, SX128X_CMD_SET_TX_CONTINUOUS_WAVE, NULL, 0);
	if (ret < 0) {
		sx128x_set_rf_path(dev, false, false);
		goto out;
	}

	k_mutex_unlock(&data->lock);

	k_sleep(K_SECONDS(duration));

	k_mutex_lock(&data->lock, K_FOREVER);
	sx128x_set_standby(dev, SX128X_STDBY_RC);
	sx128x_set_rf_path(dev, false, false);
	ret = 0;

out:
	k_mutex_unlock(&data->lock);
	atomic_set(&data->state, SX128X_REST_STATE);
	return ret;
}

static int sx128x_lora_rssi(const struct device *dev, int16_t *rssi)
{
	uint8_t raw;
	int ret;

	ret = sx12xx_hal_read_cmd(dev, SX128X_CMD_GET_RSSI_INST, &raw, 1);
	if (ret == 0) {
		/* RSSI is -value/2 dBm */
		*rssi = -((int16_t)raw) / 2;
	}

	return ret;
}

static DEVICE_API(lora, sx128x_lora_api) = {
	.config = sx128x_lora_config,
#ifdef CONFIG_LORA_GFSK
	.config_gfsk = sx128x_lora_config_gfsk,
#endif
	.send = sx128x_lora_send,
	.send_async = sx128x_lora_send_async,
	.recv = sx128x_lora_recv,
	.recv_async = sx128x_lora_recv_async,
	.cad = sx128x_lora_cad,
	.cad_async = sx128x_lora_cad_async,
	.rssi = sx128x_lora_rssi,
	.recv_duty_cycle_async = sx128x_lora_recv_duty_cycle_async,
	.airtime = sx128x_lora_airtime,
	.test_cw = sx128x_lora_test_cw,
};

static const struct sx12xx_hal_opcodes sx128x_opcodes = {
	.write_register = SX128X_CMD_WRITE_REGISTER,
	.read_register = SX128X_CMD_READ_REGISTER,
	.write_buffer = SX128X_CMD_WRITE_BUFFER,
	.read_buffer = SX128X_CMD_READ_BUFFER,
};

BUILD_ASSERT(offsetof(struct sx128x_hal_config, common) == 0);
BUILD_ASSERT(offsetof(struct sx128x_data, hal.common) == 0);

/* Radio IRQs go to the lowest-numbered DIO wired to the host */
#define SX128X_IRQ_DIO(inst)                                                                       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, dio1_gpios), (1),                                  \
		    (COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, dio2_gpios), (2),                     \
				 (COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, dio3_gpios), (3), (0))))))

#define SX128X_IRQ_GPIO(inst)                                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, dio1_gpios),                                       \
		    (GPIO_DT_SPEC_INST_GET(inst, dio1_gpios)),                                     \
		    (COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, dio2_gpios),                          \
				 (GPIO_DT_SPEC_INST_GET(inst, dio2_gpios)),                        \
				 (GPIO_DT_SPEC_INST_GET_OR(inst, dio3_gpios, {0})))))

#define SX128X_HAL_COMMON_CONFIG(inst)                                                             \
	{                                                                                          \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB),             \
		.opcodes = &sx128x_opcodes,                                                        \
		.is_busy = sx128x_hal_is_busy,                                                     \
		.tx_enable = GPIO_DT_SPEC_INST_GET_OR(inst, tx_enable_gpios, {0}),                 \
		.rx_enable = GPIO_DT_SPEC_INST_GET_OR(inst, rx_enable_gpios, {0}),                 \
	}

#define SX128X_INIT(inst, model)                                                                   \
	static struct sx128x_data sx128x_data_##model##_##inst;                                    \
                                                                                                   \
	static const struct sx128x_hal_config sx128x_config_##model##_##inst = {                   \
		.common = SX128X_HAL_COMMON_CONFIG(inst),                                          \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
		.busy = GPIO_DT_SPEC_INST_GET(inst, busy_gpios),                                   \
		.irq = SX128X_IRQ_GPIO(inst),                                                      \
		.irq_dio = SX128X_IRQ_DIO(inst),                                                   \
		.tx_power_max_valid = DT_INST_NODE_HAS_PROP(inst, tx_power_max_dbm),               \
		.tx_power_max_dbm = (int8_t)DT_INST_PROP_OR(inst, tx_power_max_dbm, 0),            \
		.regulator_ldo = DT_INST_PROP(inst, regulator_ldo),                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, sx128x_init, NULL, &sx128x_data_##model##_##inst,              \
			      &sx128x_config_##model##_##inst, POST_KERNEL,                        \
			      CONFIG_LORA_INIT_PRIORITY, &sx128x_lora_api);

#define DT_DRV_COMPAT semtech_sx1280
DT_INST_FOREACH_STATUS_OKAY_VARGS(SX128X_INIT, sx1280)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT semtech_sx1281
DT_INST_FOREACH_STATUS_OKAY_VARGS(SX128X_INIT, sx1281)

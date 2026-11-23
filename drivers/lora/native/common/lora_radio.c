/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>

#include "lora_radio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_radio, CONFIG_LORA_LOG_LEVEL);

/* A caller blocked on an operation, and where its result goes */
struct lora_radio_waiter {
	struct k_sem done;
	int status;
	uint8_t *buf;
	uint8_t size;
	int16_t *rssi;
	int8_t *snr;
	/* Waiting on a reception without limit, which a packet does not end */
	bool continuous;
};

/* Claim the radio for an operation and wake it up */
static int lora_radio_op_begin(const struct device *dev, enum lora_radio_op op)
{
	const struct lora_radio_config *config = dev->config;
	struct lora_radio_data *data = dev->data;
	uint32_t stale;
	int ret;

	if (data->op != LORA_RADIO_OP_NONE) {
		return -EBUSY;
	}

	ret = config->ops->wake(dev);
	if (ret < 0) {
		return ret;
	}

	/* Interrupts an earlier operation left behind are not this one's */
	if (data->irq_stale) {
		ret = config->ops->irq_get(dev, &stale);
		if (ret < 0) {
			config->ops->sleep(dev);
			return ret;
		}

		data->irq_stale = false;
	}

	data->op = op;

	return 0;
}

/* Give the radio back and tell whoever is waiting how the operation went */
static void lora_radio_op_end(const struct device *dev, int status)
{
	const struct lora_radio_config *config = dev->config;
	struct lora_radio_data *data = dev->data;
	struct lora_radio_waiter *waiter = data->waiter;
	struct k_poll_signal *async = data->tx_async_signal;

	data->waiter = NULL;
	data->tx_async_signal = NULL;
	data->rx_cb = NULL;
	data->rx_cb_user_data = NULL;

	config->ops->sleep(dev);
	data->op = LORA_RADIO_OP_NONE;
	data->ops_ended++;

	if (async != NULL) {
		k_poll_signal_raise(async, status);
	}

	if (waiter != NULL) {
		waiter->status = status;
		k_sem_give(&waiter->done);
	}
}

/* End an operation the radio is still busy with */
static void lora_radio_op_abort(const struct device *dev, int status)
{
	const struct lora_radio_config *config = dev->config;
	struct lora_radio_data *data = dev->data;
	uint32_t stale;

	config->ops->stop(dev, data->op);
	/* What it raised meanwhile must not be taken for the next operation's */
	data->irq_stale = (config->ops->irq_get(dev, &stale) < 0);
	lora_radio_op_end(dev, status);
}

static void lora_radio_register_waiter(struct lora_radio_data *data,
				       struct lora_radio_waiter *waiter)
{
	if (waiter != NULL) {
		k_sem_init(&waiter->done, 0, 1);
	}

	data->waiter = waiter;
}

/* Wait for the operation to end, stopping it when it outlasts the timeout */
static int lora_radio_wait(const struct device *dev, struct lora_radio_waiter *waiter,
			   k_timeout_t timeout)
{
	struct lora_radio_data *data = dev->data;

	if (k_sem_take(&waiter->done, timeout) != 0) {
		k_mutex_lock(&data->lock, K_FOREVER);

		/* Still registered: the interrupt work has not ended it meanwhile */
		if (data->waiter == waiter) {
			lora_radio_op_abort(dev,
					    data->op == LORA_RADIO_OP_TX ? -ETIMEDOUT : -EAGAIN);
		}

		k_mutex_unlock(&data->lock);
	}

	return waiter->status;
}

/* (Re)start listening the way the operation in flight wants it */
static int lora_radio_rx_arm(const struct device *dev, uint32_t timeout_ms)
{
	const struct lora_radio_config *config = dev->config;
	struct lora_radio_data *data = dev->data;

	if (data->op == LORA_RADIO_OP_RX_DUTY_CYCLE) {
		return config->ops->rx_duty_cycle_start(dev, data->duty_cycle.rx_ms,
							data->duty_cycle.sleep_ms);
	}

	return config->ops->rx_start(dev, timeout_ms);
}

static void lora_radio_rx_restart(const struct device *dev)
{
	int ret;

	ret = lora_radio_rx_arm(dev, 0);
	if (ret < 0) {
		LOG_ERR("Failed to restart reception: %d", ret);
		/* A continuous reception is still running */
		lora_radio_op_abort(dev, ret);
	}
}

static void lora_radio_rx_done(const struct device *dev, uint32_t irq_status)
{
	const struct lora_radio_config *config = dev->config;
	struct lora_radio_data *data = dev->data;
	struct lora_radio_waiter *waiter = data->waiter;
	lora_recv_cb cb = data->rx_cb;
	void *user_data = data->rx_cb_user_data;
	uint32_t ended = data->ops_ended;
	int16_t rssi = 0;
	int8_t snr = 0;
	int len;

	len = config->ops->rx_read(dev, irq_status, data->rx_buf, sizeof(data->rx_buf),
				   &rssi, &snr);

	if (cb == NULL) {
		if (len > 0 && waiter != NULL) {
			len = MIN(len, waiter->size);
			memcpy(waiter->buf, data->rx_buf, len);
			if (waiter->rssi != NULL) {
				*waiter->rssi = rssi;
			}
			if (waiter->snr != NULL) {
				*waiter->snr = snr;
			}
		}

		if (waiter != NULL && waiter->continuous) {
			lora_radio_op_abort(dev, len);
		} else {
			lora_radio_op_end(dev, len);
		}
		return;
	}

	/* Async mode: deliver valid packets, drop errors silently */
	if (len > 0) {
		/* Unlocked: the callback may call back into the driver */
		k_mutex_unlock(&data->lock);
		cb(dev, data->rx_buf, len, rssi, snr, user_data);
		k_mutex_lock(&data->lock, K_FOREVER);
	}

	/* Restart the reception unless the callback ended it, even to start another */
	if (data->ops_ended == ended) {
		lora_radio_rx_restart(dev);
	}
}

static void lora_radio_irq_work_handler(struct k_work *work)
{
	struct lora_radio_data *data = CONTAINER_OF(work, struct lora_radio_data, irq_work);
	const struct device *dev = data->dev;
	const struct lora_radio_config *config = dev->config;
	const struct lora_radio_ops *ops = config->ops;
	uint32_t irq_status = 0;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Asleep with nothing in flight, the radio cannot be asked */
	if (data->op == LORA_RADIO_OP_NONE && ops->sleeps) {
		goto unlock;
	}

	ret = ops->irq_get(dev, &irq_status);
	if (ret < 0) {
		LOG_ERR("Failed to get IRQ status: %d", ret);
		/* How the operation went is unknown. CW is ended by its own caller. */
		if (data->op != LORA_RADIO_OP_NONE && data->op != LORA_RADIO_OP_CW) {
			lora_radio_op_abort(dev, ret);
		} else {
			data->irq_stale = true;
		}
		goto unlock;
	}

	switch (data->op) {
	case LORA_RADIO_OP_TX:
		if ((irq_status & ops->irq_tx_done) != 0) {
			lora_radio_op_end(dev, 0);
		} else if ((irq_status & ops->irq_timeout) != 0) {
			lora_radio_op_end(dev, -ETIMEDOUT);
		}
		break;
	case LORA_RADIO_OP_RX:
		if ((irq_status & ops->irq_rx_done) != 0) {
			lora_radio_rx_done(dev, irq_status);
		} else if ((irq_status & ops->irq_timeout) != 0) {
			lora_radio_op_end(dev, -EAGAIN);
		}
		break;
	case LORA_RADIO_OP_RX_DUTY_CYCLE:
		if ((irq_status & ops->irq_rx_done) != 0) {
			lora_radio_rx_done(dev, irq_status);
		} else if ((irq_status & ops->irq_timeout) != 0) {
			/*
			 * Preamble was detected but full packet reception failed.
			 * The radio fell back to standby, restart the duty cycle.
			 */
			lora_radio_rx_restart(dev);
		}
		break;
	default:
		break;
	}

	/* Re-enable the interrupt for the next event */
	if (data->op != LORA_RADIO_OP_NONE) {
		ops->irq_rearm(dev);
	}

unlock:
	k_mutex_unlock(&data->lock);
}

void lora_radio_irq(const struct device *dev)
{
	struct lora_radio_data *data = dev->data;

	k_work_submit(&data->irq_work);
}

void lora_radio_init(const struct device *dev)
{
	struct lora_radio_data *data = dev->data;

	k_mutex_init(&data->lock);
	k_work_init(&data->irq_work, lora_radio_irq_work_handler);
	data->dev = dev;
	data->op = LORA_RADIO_OP_NONE;
	data->configured = false;
}

int lora_radio_configure(const struct device *dev, lora_radio_config_fn fn, const void *cfg)
{
	struct lora_radio_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = lora_radio_op_begin(dev, LORA_RADIO_OP_CONFIG);
	if (ret < 0) {
		goto unlock;
	}

	ret = fn(dev, cfg);
	/* A configuration that failed may have left the radio half set up */
	data->configured = (ret >= 0);

	lora_radio_op_end(dev, 0);

unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int lora_radio_tx_start(const struct device *dev,
			       uint8_t *data_buf, uint32_t data_len,
			       struct k_poll_signal *async, struct lora_radio_waiter *waiter)
{
	const struct lora_radio_config *config = dev->config;
	struct lora_radio_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Checked under the lock: a configuration in flight is over by now */
	if (!data->configured) {
		LOG_ERR("Not configured");
		ret = -EINVAL;
		goto unlock;
	}

	if (data_len > LORA_RADIO_MAX_PAYLOAD_LEN) {
		LOG_ERR("Payload too long: %u", data_len);
		ret = -EINVAL;
		goto unlock;
	}

	if (config->ops->tx_check != NULL) {
		ret = config->ops->tx_check(dev, data_len);
		if (ret < 0) {
			goto unlock;
		}
	}

	ret = lora_radio_op_begin(dev, LORA_RADIO_OP_TX);
	if (ret < 0) {
		if (ret == -EBUSY) {
			LOG_ERR("Busy");
		}
		goto unlock;
	}

	ret = config->ops->tx_start(dev, data_buf, data_len);
	if (ret < 0) {
		lora_radio_op_end(dev, ret);
		goto unlock;
	}

	data->tx_async_signal = async;
	lora_radio_register_waiter(data, waiter);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

int lora_radio_send_async(const struct device *dev,
			  uint8_t *data_buf, uint32_t data_len,
			  struct k_poll_signal *async)
{
	return lora_radio_tx_start(dev, data_buf, data_len, async, NULL);
}

int lora_radio_send(const struct device *dev,
		    uint8_t *data_buf, uint32_t data_len)
{
	struct lora_radio_waiter waiter = {0};
	int ret;

	ret = lora_radio_tx_start(dev, data_buf, data_len, NULL, &waiter);
	if (ret < 0) {
		return ret;
	}

	ret = lora_radio_wait(dev, &waiter, K_SECONDS(15));
	if (ret == -ETIMEDOUT) {
		LOG_ERR("TX timeout");
	}

	return ret;
}

/*
 * rx_ms is how long the radio listens: until it gives up on a plain
 * reception, 0 for no limit, or in each window of a duty-cycled one.
 */
static int lora_radio_rx_start(const struct device *dev, enum lora_radio_op op,
			       uint32_t rx_ms, uint32_t sleep_ms,
			       lora_recv_cb cb, void *user_data,
			       struct lora_radio_waiter *waiter)
{
	const struct lora_radio_config *config = dev->config;
	struct lora_radio_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->configured) {
		LOG_ERR("Not configured");
		ret = -EINVAL;
		goto unlock;
	}

	ret = lora_radio_op_begin(dev, op);
	if (ret < 0) {
		if (ret == -EBUSY) {
			LOG_ERR("Busy");
		}
		goto unlock;
	}

	if (op == LORA_RADIO_OP_RX_DUTY_CYCLE) {
		data->duty_cycle.rx_ms = rx_ms;
		data->duty_cycle.sleep_ms = sleep_ms;
	}

	ret = config->ops->rx_prepare(dev);
	if (ret == 0) {
		ret = lora_radio_rx_arm(dev, rx_ms);
	}

	if (ret < 0) {
		lora_radio_op_end(dev, ret);
		goto unlock;
	}

	data->rx_cb = cb;
	data->rx_cb_user_data = user_data;
	lora_radio_register_waiter(data, waiter);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/* Stop a reception of the given kind, if one is in flight */
static int lora_radio_rx_stop(const struct device *dev, enum lora_radio_op op)
{
	struct lora_radio_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->op == op) {
		lora_radio_op_abort(dev, -EAGAIN);
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

int lora_radio_recv(const struct device *dev, uint8_t *data_buf,
		    uint8_t size, k_timeout_t timeout,
		    int16_t *rssi, int8_t *snr)
{
	struct lora_radio_waiter waiter = {
		.buf = data_buf,
		.size = size,
		.rssi = rssi,
		.snr = snr,
	};
	uint32_t timeout_ms;
	int ret;

	/* 0 = continuous for K_FOREVER */
	timeout_ms = K_TIMEOUT_EQ(timeout, K_FOREVER)
		     ? 0 : k_ticks_to_ms_ceil32(timeout.ticks);
	waiter.continuous = (timeout_ms == 0);
	ret = lora_radio_rx_start(dev, LORA_RADIO_OP_RX, timeout_ms, 0, NULL, NULL, &waiter);
	if (ret < 0) {
		return ret;
	}

	return lora_radio_wait(dev, &waiter, timeout);
}

int lora_radio_recv_async(const struct device *dev,
			  lora_recv_cb cb, void *user_data)
{
	if (cb == NULL) {
		return lora_radio_rx_stop(dev, LORA_RADIO_OP_RX);
	}

	return lora_radio_rx_start(dev, LORA_RADIO_OP_RX, 0, 0, cb, user_data, NULL);
}

int lora_radio_recv_duty_cycle(const struct device *dev,
			       k_timeout_t rx_period,
			       k_timeout_t sleep_period,
			       uint8_t *data_buf, uint8_t size,
			       k_timeout_t timeout,
			       int16_t *rssi, int8_t *snr)
{
	struct lora_radio_waiter waiter = {
		.buf = data_buf,
		.size = size,
		.rssi = rssi,
		.snr = snr,
	};
	int ret;

	ret = lora_radio_rx_start(dev, LORA_RADIO_OP_RX_DUTY_CYCLE,
				  k_ticks_to_ms_ceil32(rx_period.ticks),
				  k_ticks_to_ms_ceil32(sleep_period.ticks), NULL, NULL, &waiter);
	if (ret < 0) {
		return ret;
	}

	return lora_radio_wait(dev, &waiter, timeout);
}

int lora_radio_recv_duty_cycle_async(const struct device *dev,
				     k_timeout_t rx_period,
				     k_timeout_t sleep_period,
				     lora_recv_cb cb, void *user_data)
{
	if (cb == NULL) {
		return lora_radio_rx_stop(dev, LORA_RADIO_OP_RX_DUTY_CYCLE);
	}

	return lora_radio_rx_start(dev, LORA_RADIO_OP_RX_DUTY_CYCLE,
				   k_ticks_to_ms_ceil32(rx_period.ticks),
				   k_ticks_to_ms_ceil32(sleep_period.ticks), cb, user_data, NULL);
}

int lora_radio_test_cw(const struct device *dev, uint32_t frequency,
		       int8_t tx_power, uint16_t duration)
{
	const struct lora_radio_config *config = dev->config;
	struct lora_radio_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = lora_radio_op_begin(dev, LORA_RADIO_OP_CW);
	if (ret < 0) {
		goto unlock;
	}

	ret = config->ops->cw_start(dev, frequency, tx_power);
	if (ret < 0) {
		lora_radio_op_end(dev, ret);
		goto unlock;
	}

	k_mutex_unlock(&data->lock);

	/* Wait for duration */
	k_sleep(K_SECONDS(duration));

	/* Stop CW */
	k_mutex_lock(&data->lock, K_FOREVER);
	lora_radio_op_abort(dev, ret);

unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

int lora_radio_rssi(const struct device *dev, int16_t *rssi)
{
	const struct lora_radio_config *config = dev->config;
	struct lora_radio_data *data = dev->data;
	int ret;

	if (config->ops->rssi == NULL) {
		return -ENOSYS;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Not in a duty cycle: the radio is asleep for most of it */
	if (data->op == LORA_RADIO_OP_RX) {
		ret = config->ops->rssi(dev, rssi);
	} else {
		ret = -EBUSY;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

uint32_t lora_radio_airtime(const struct device *dev, uint32_t data_len)
{
	const struct lora_radio_config *config = dev->config;
	struct lora_radio_data *data = dev->data;
	uint32_t airtime = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->configured) {
		airtime = config->ops->airtime(dev, data_len);
	}

	k_mutex_unlock(&data->lock);

	return airtime;
}

uint32_t lora_radio_lora_airtime_ms(uint32_t bw_hz, uint8_t sf, uint8_t cr,
				    uint32_t preamble_len, bool ldro, bool explicit_hdr,
				    bool crc, uint32_t payload_len)
{
	uint64_t t_sym_us, t_preamble_us, t_payload_us;
	uint32_t n_payload;
	int32_t tmp;
	int32_t de = ldro ? 1 : 0;

	/* Symbol time = 2^SF / BW */
	t_sym_us = ((uint64_t)BIT(sf) * USEC_PER_SEC) / bw_hz;

	/* Preamble time (4.25 extra symbols) */
	t_preamble_us = (uint64_t)(preamble_len + 4U) * t_sym_us + (t_sym_us / 4U);

	/* Payload symbol count */
	tmp = 8 * (int32_t)payload_len - 4 * sf + 8 + (explicit_hdr ? 20 : 0) + (crc ? 16 : 0);
	if (tmp < 0) {
		tmp = 0;
	}

	n_payload = 8U + (uint32_t)(((tmp + 4 * (sf - 2 * de) - 1) / (4 * (sf - 2 * de))) *
				    (cr + 4));
	t_payload_us = n_payload * t_sym_us;

	return (uint32_t)((t_preamble_us + t_payload_us + 500U) / USEC_PER_MSEC);
}

uint32_t lora_radio_gfsk_airtime_ms(const struct lora_modem_config_gfsk *cfg,
				    uint32_t data_len, uint32_t header_bits)
{
	uint32_t payload = cfg->fixed_len ? cfg->payload_len : data_len;
	uint64_t bits;

	bits = (uint64_t)(cfg->preamble_len + cfg->sync_word_len + payload +
			  (cfg->packet_crc_disable ? 0U : 2U)) *
	       BITS_PER_BYTE;
	if (!cfg->fixed_len) {
		bits += header_bits;
	}

	return (uint32_t)DIV_ROUND_UP(bits * MSEC_PER_SEC, cfg->bitrate);
}

/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_COMMON_LORA_RADIO_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_COMMON_LORA_RADIO_H_

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>

#define LORA_RADIO_MAX_PAYLOAD_LEN 255

/*
 * Operation state machine. The state is what the radio is claimed for, and
 * the functions are named without their lora_radio_ prefix.
 *
 *               config_begin()                    +---------------+
 *           +------------------------------------>|               |
 *           |                                     |    CONFIG     |
 *           |<------------------------------------|               |
 *           |   config_end()                      +---------------+
 *           |
 *           |   send(), send_async()              +---------------+
 *           +------------------------------------>|               |
 *           |                                     |      TX       |
 *           |<------------------------------------|               |
 *           |   TX done, timeout                  +---------------+
 *           |
 *           |   test_cw()                         +---------------+
 * +------+  +------------------------------------>|               |
 * | NONE |--+                                     |      CW       |
 * +------+  |<------------------------------------|               |
 *           |   duration over                     +---------------+
 *           |
 *           |   recv(), recv_async(cb)            +---------------+
 *           +------------------------------------>|               |<--+ recv_async():
 *           |                                     |      RX       |   | after each
 *           |<------------------------------------|               |---+ packet
 *           |   recv(): packet, timeout           +---------------+
 *           |   recv_async(NULL)
 *           |
 *           |   recv_duty_cycle(),
 *           |   recv_duty_cycle_async(cb)         +---------------+
 *           +------------------------------------>|               |<--+ after each packet
 *           |                                     | RX_DUTY_CYCLE |   | (async) and each
 *           |<------------------------------------|               |---+ reception the
 *               recv_duty_cycle(): packet,        +---------------+     radio gave up on
 *               timeout
 *               recv_duty_cycle_async(NULL)
 *
 * The radio is woken when an operation claims it and put to rest when the
 * operation ends. An operation the radio refuses to start, or a duty cycle it
 * fails to resume, ends at once. A call that finds the radio claimed returns
 * -EBUSY, except during a configuration: that one holds the lock, so the call
 * waits for it and is checked against what it set.
 */

/* What the radio is claimed for */
enum lora_radio_op {
	LORA_RADIO_OP_NONE = 0,
	LORA_RADIO_OP_CONFIG,
	LORA_RADIO_OP_TX,
	LORA_RADIO_OP_CW,
	LORA_RADIO_OP_RX,
	LORA_RADIO_OP_RX_DUTY_CYCLE,
};

/*
 * What the operation handling needs from a radio. Everything is called with
 * the lock held and, apart from tx_check(), with the radio claimed.
 */
struct lora_radio_ops {
	/* Interrupt status bits, as irq_get() reports them */
	uint32_t irq_tx_done;
	uint32_t irq_rx_done;
	uint32_t irq_timeout;

	/* The radio sleeps between operations and takes no command then */
	bool sleeps;

	/* Get the radio ready to take commands */
	int (*wake)(const struct device *dev);
	/* Leave the radio at rest once an operation is over */
	int (*sleep)(const struct device *dev);
	/* Take the radio out of the operation it is busy with */
	int (*stop)(const struct device *dev, enum lora_radio_op op);

	/* Read and clear the pending interrupts */
	int (*irq_get)(const struct device *dev, uint32_t *irq);
	/* Let the next interrupt through */
	void (*irq_rearm)(const struct device *dev);

	/* Optional. Whether the radio can send len bytes as it is configured. */
	int (*tx_check)(const struct device *dev, uint32_t len);
	int (*tx_start)(const struct device *dev, const uint8_t *buf, uint32_t len);

	/* Set the radio up for the receptions that follow */
	int (*rx_prepare)(const struct device *dev);
	/* Listen for timeout_ms, without limit when 0 */
	int (*rx_start)(const struct device *dev, uint32_t timeout_ms);
	/* Listen for rx_ms, sleep for sleep_ms, and over again */
	int (*rx_duty_cycle_start)(const struct device *dev, uint32_t rx_ms, uint32_t sleep_ms);
	/* Read the packet behind an RX done interrupt. Returns its length. */
	int (*rx_read)(const struct device *dev, uint32_t irq, uint8_t *buf, uint8_t size,
		       int16_t *rssi, int8_t *snr);

	/* Emit an unmodulated carrier */
	int (*cw_start)(const struct device *dev, uint32_t frequency, int8_t tx_power);
};

/* Constant part of a radio. Must be the first member of the driver config. */
struct lora_radio_config {
	const struct lora_radio_ops *ops;
};

struct lora_radio_waiter;

/* Operation state of a radio. Must be the first member of the driver data. */
struct lora_radio_data {
	const struct device *dev;

	/* Guards the operation state and every exchange with the radio */
	struct k_mutex lock;
	enum lora_radio_op op;
	bool configured;

	struct lora_radio_waiter *waiter;

	/* RX data buffer */
	uint8_t rx_buf[LORA_RADIO_MAX_PAYLOAD_LEN];

	/* Async RX callback */
	lora_recv_cb rx_cb;
	void *rx_cb_user_data;

	/* RX duty cycle periods */
	struct {
		uint32_t rx_ms;
		uint32_t sleep_ms;
	} duty_cycle;

	/* Async TX signal */
	struct k_poll_signal *tx_async_signal;

	/* Deferred work for interrupt handling */
	struct k_work irq_work;
};

void lora_radio_init(const struct device *dev);

/* To be called, from any context, when the radio raises its interrupt */
void lora_radio_irq(const struct device *dev);

/*
 * Claim the radio to configure it. On success the radio is awake and the lock
 * held until lora_radio_config_end(), which takes how the configuration went.
 */
int lora_radio_config_begin(const struct device *dev);
void lora_radio_config_end(const struct device *dev, int status);

/* Implementations of the LoRa driver API */
int lora_radio_send(const struct device *dev, uint8_t *data_buf, uint32_t data_len);
int lora_radio_send_async(const struct device *dev, uint8_t *data_buf, uint32_t data_len,
			  struct k_poll_signal *async);
int lora_radio_recv(const struct device *dev, uint8_t *data_buf, uint8_t size,
		    k_timeout_t timeout, int16_t *rssi, int8_t *snr);
int lora_radio_recv_async(const struct device *dev, lora_recv_cb cb, void *user_data);
int lora_radio_recv_duty_cycle(const struct device *dev, k_timeout_t rx_period,
			       k_timeout_t sleep_period, uint8_t *data_buf, uint8_t size,
			       k_timeout_t timeout, int16_t *rssi, int8_t *snr);
int lora_radio_recv_duty_cycle_async(const struct device *dev, k_timeout_t rx_period,
				     k_timeout_t sleep_period, lora_recv_cb cb, void *user_data);
int lora_radio_test_cw(const struct device *dev, uint32_t frequency, int8_t tx_power,
		       uint16_t duration);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_COMMON_LORA_RADIO_H_ */

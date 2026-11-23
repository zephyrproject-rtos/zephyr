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
 * The radio is woken when an operation claims it and put to rest when the
 * operation ends. An operation the radio refuses to start, or a reception it
 * fails to resume, ends at once. A call that finds the radio claimed returns
 * -EBUSY, except during a configuration: that one holds the lock for as long
 * as it runs, so the call waits for it and is checked against what it set.
 */

/* What the radio is claimed for */
enum lora_radio_op {
	LORA_RADIO_OP_NONE = 0,
	LORA_RADIO_OP_CONFIG,
	LORA_RADIO_OP_TX,
	LORA_RADIO_OP_CAD,
	LORA_RADIO_OP_CW,
	LORA_RADIO_OP_RX,
	LORA_RADIO_OP_RX_DUTY_CYCLE,
};

/*
 * What the operation handling needs from a radio. Everything is called with
 * the lock held. The radio is claimed and awake by then, except that wake()
 * is what wakes it for the claim, tx_check() and airtime() come without the
 * claim and must not talk to the radio, and a radio that does not sleep gets
 * irq_get() with nothing in flight too.
 */
struct lora_radio_ops {
	/* Interrupt status bits, as irq_get() reports them */
	uint32_t irq_tx_done;
	uint32_t irq_rx_done;
	uint32_t irq_timeout;
	uint32_t irq_cad_done;
	uint32_t irq_cad_detected;

	/* The radio sleeps between operations and takes no command then */
	bool sleeps;
	/*
	 * An asynchronous reception goes on by itself after a packet, so
	 * rx_start() is only called again after a failed one
	 */
	bool rx_continuous;
	/*
	 * Restart an asynchronous reception before handing the packet to the
	 * callback rather than after, so the radio is deaf for less time
	 */
	bool rx_rearm_before_callback;

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

	/*
	 * Optional. Take the next frame while a transmission is in flight,
	 * and send it from the TX done interrupt with tx_continue().
	 */
	int (*tx_enqueue)(const struct device *dev, const uint8_t *buf, uint32_t len);
	int (*tx_continue)(const struct device *dev);

	/* Optional. Listen once for a LoRa preamble. */
	int (*cad_start)(const struct device *dev);

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

	/* Time on air, in ms, of len bytes as the radio is configured */
	uint32_t (*airtime)(const struct device *dev, uint32_t len);

	/* Optional. Signal level as it is now, asked for during a plain reception only. */
	int (*rssi)(const struct device *dev, int16_t *rssi);
};

/* Constant part of a radio. Must be the first member of the driver config. */
struct lora_radio_config {
	const struct lora_radio_ops *ops;
};

struct lora_radio_waiter;

/* Operation state of a radio. Must be the first member of the driver data. */
struct lora_radio_data {
	const struct device *dev;

	/* Guards the operation state and the operations' exchanges with the radio */
	struct k_mutex lock;
	enum lora_radio_op op;
	/* How many operations have ended, to tell one from the next */
	uint32_t ops_ended;
	bool configured;
	/* Interrupts of an ended operation are still to be cleared */
	bool irq_stale;

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
	/* Signal of the frame queued behind the one on air */
	struct k_poll_signal *queued_tx_signal;

	/* Async CAD callback */
	lora_cad_cb cad_cb;
	void *cad_cb_user_data;

	/* Deferred work for interrupt handling */
	struct k_work irq_work;
	/* Queue the work runs on, the system one when NULL */
	struct k_work_q *irq_workq;
};

void lora_radio_init(const struct device *dev);

/* To be called, from any context, when the radio raises its interrupt */
void lora_radio_irq(const struct device *dev);

/* Applies a configuration to a radio that is claimed, awake and locked */
typedef int (*lora_radio_config_fn)(const struct device *dev, const void *cfg);

/*
 * Claim the radio, wake it, run fn() against it and let it rest again. The
 * lock is held throughout, so a call that arrives meanwhile waits for the
 * configuration instead of being turned away.
 */
int lora_radio_configure(const struct device *dev, lora_radio_config_fn fn, const void *cfg);

/* Implementations of the LoRa driver API */
int lora_radio_send(const struct device *dev, uint8_t *data_buf, uint32_t data_len);
int lora_radio_send_async(const struct device *dev, uint8_t *data_buf, uint32_t data_len,
			  struct k_poll_signal *async);
int lora_radio_cad(const struct device *dev, k_timeout_t timeout);
int lora_radio_cad_async(const struct device *dev, lora_cad_cb cb, void *user_data);
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
int lora_radio_rssi(const struct device *dev, int16_t *rssi);
uint32_t lora_radio_airtime(const struct device *dev, uint32_t data_len);

/**
 * @brief Time on air of a LoRa packet
 *
 * From the Semtech LoRa modem designer's guide.
 *
 * @param bw_hz        Bandwidth in Hz
 * @param sf           Spreading factor
 * @param cr           Coding rate, 1 to 4 for 4/5 to 4/8
 * @param preamble_len Preamble length in symbols
 * @param ldro         Low data rate optimization on
 * @param explicit_hdr Explicit header on
 * @param crc          Payload CRC on
 * @param payload_len  Payload length in bytes
 *
 * @return Time on air in milliseconds, rounded to the nearest
 */
uint32_t lora_radio_lora_airtime_ms(uint32_t bw_hz, uint8_t sf, uint8_t cr,
				    uint32_t preamble_len, bool ldro, bool explicit_hdr,
				    bool crc, uint32_t payload_len);

/**
 * @brief Time on air of a GFSK packet
 *
 * Preamble, sync word, the length header unless the length is fixed, the
 * payload and the CRC, all at the configured bit rate.
 *
 * @param cfg         GFSK configuration
 * @param data_len    Payload length in bytes, when the length is not fixed
 * @param header_bits Length of the header of a variable-length packet
 *
 * @return Time on air in milliseconds, rounded up
 */
uint32_t lora_radio_gfsk_airtime_ms(const struct lora_modem_config_gfsk *cfg,
				    uint32_t data_len, uint32_t header_bits);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_COMMON_LORA_RADIO_H_ */

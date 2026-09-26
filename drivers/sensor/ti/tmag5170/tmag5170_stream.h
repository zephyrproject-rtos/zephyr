/*
 * Copyright (c) 2026 Swarovski Optik AG & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TI_TMAG5170_TMAG5170_STREAM_H_
#define ZEPHYR_DRIVERS_SENSOR_TI_TMAG5170_TMAG5170_STREAM_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>

#include "tmag5170_bus.h"

#define TMAG5170_STREAM_TX_FRAMES_COUNT 5
#define TMAG5170_STREAM_FRAME_LEN       TMAG5170_SPI_BUFFER_LEN

enum tmag5170_stream_state {
	TMAG5170_STREAM_OFF = 0,
	TMAG5170_STREAM_ON = 1,
	TMAG5170_STREAM_BUSY = 2,
};

struct tmag5170_stream_data {
	struct gpio_callback gpio_cb;
	const struct device *dev;
	/** Protects iodev_sqe against concurrent access from the GPIO ISR and
	 * from tmag5170_stream_disable(); the interrupt itself is always
	 * disabled before iodev_sqe is cleared, see tmag5170_stream_result().
	 */
	struct k_spinlock lock;
	struct rtio_iodev_sqe *iodev_sqe;
	atomic_t state;
	uint64_t timestamp;
	uint8_t tx_frames[TMAG5170_STREAM_TX_FRAMES_COUNT][TMAG5170_STREAM_FRAME_LEN];

	/* Diagnostic counters, updated at all error sites but never read back
	 * by driver logic. Purely for observability.
	 */
	uint32_t crc_errors;
	uint32_t nomem_errors;
	uint32_t sqe_errors;
	uint32_t spurious_irqs;
	uint32_t submissions;
	uint32_t completions;
	/** Non-recoverable bus/transport errors reported via the CQE result */
	uint32_t bus_errors;
};

int tmag5170_stream_init(const struct device *dev);
void tmag5170_stream_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Disable the streaming submission of a device.
 *
 * Disables the data-ready interrupt, discards the driver's own reference to
 * the streaming submission and finalizes it with -ECANCELED if no transfer
 * was in flight for it (an in-flight transfer finalizes it itself once it
 * completes). Safe to call from application code, e.g. before/after
 * rtio_sqe_cancel(), to leave the driver in a consistent, restartable state.
 *
 * @param dev TMAG5170 device
 */
void tmag5170_stream_disable(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_TI_TMAG5170_TMAG5170_STREAM_H_ */

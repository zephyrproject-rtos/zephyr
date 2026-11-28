/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_RC_RC_INTERNAL_H_
#define ZEPHYR_SUBSYS_RC_RC_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc_protocol.h>

#define RC_FLAG_OVERFLOW     0
#define RC_FLAG_IDLE         1
#define RC_FLAG_CORE_TIMEOUT 2
#define RC_FLAG_TIMEOUT_AUTO 3

static inline struct rc_dev_data *rc_data(const struct device *dev)
{
	return dev->data;
}

int rc_protocol_init(const struct device *dev, const uint16_t *protocols, size_t num_protocols);

void rc_protocol_reset_all(const struct device *dev);

void rc_protocol_decode(const struct device *dev, uint32_t word);

uint32_t rc_protocol_min_timeout_us(const struct device *dev);

int rc_rx_timeout_update(const struct device *dev);

int rc_rx_submit(struct k_work *work);

int rc_rx_reschedule(struct k_work_delayable *dwork, k_timeout_t delay);

bool rc_rx_is_current(void);

void rc_event_report(const struct device *dev, const struct rc_event *evt);

void rc_sync_dispatch(void);

void rc_callback_call(const struct device *dev, const struct rc_event *evt);

/* Complete with an error on rc_rx */
void rc_sqe_done(struct rtio_iodev_sqe *iodev_sqe);

void rc_tx_dispatch(void);

bool rc_raw_has_readers(void);

void rc_raw_report(const struct device *dev, const uint32_t *words, size_t count);

void rc_key_init(const struct device *dev);

void rc_key_frame(const struct device *dev, const struct rc_protocol *p,
		  const struct rc_decode_result *res);

void rc_raw_init(const struct device *dev, bool core_timeout);

#endif /* ZEPHYR_SUBSYS_RC_RC_INTERNAL_H_ */

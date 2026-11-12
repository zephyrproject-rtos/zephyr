/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHAN_METRICS_H_
#define CHAN_METRICS_H_

struct net_buf_simple;

/**
 * @brief Start the channel metrics histogram printing.
 *
 * Call before the vendor-specific events are delegated to chan_metrics_vnd_evt().
 */
void chan_metrics_init(void);

/**
 * @brief Handle a vendor-specific event.
 *
 * @param buf Vendor-specific event, starting with the subevent code.
 *
 * @retval true The event is a Channel Metrics event and was handled
 * @retval false The event is not a Channel Metrics event
 */
bool chan_metrics_vnd_evt(struct net_buf_simple *buf);

#endif /* CHAN_METRICS_H_ */

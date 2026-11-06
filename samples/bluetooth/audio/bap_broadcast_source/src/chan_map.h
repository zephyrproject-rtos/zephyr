/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHAN_MAP_H_
#define CHAN_MAP_H_

struct net_buf_simple;

/**
 * @brief Start the channel metrics based channel map update.
 *
 * Call after bt_enable().
 */
void chan_map_init(void);

/**
 * @brief Collect a Channel Metrics vendor-specific event.
 *
 * @param buf Vendor-specific event, starting with the subevent code.
 *
 * @retval true The event is a Channel Metrics event and was collected
 * @retval false The event is not a Channel Metrics event
 */
bool chan_map_vnd_evt(struct net_buf_simple *buf);

#endif /* CHAN_MAP_H_ */

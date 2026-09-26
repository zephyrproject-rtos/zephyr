/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESP154_SNIFFER_H_
#define ESP154_SNIFFER_H_

#include <stdint.h>

struct sniffer_stream_stats {
	uint32_t rx_enqueued;
	uint32_t rx_dropped;
	uint32_t tx_records;
	uint32_t tx_bytes;
	uint32_t q_high_wm;
};

int sniffer_start(void);
int sniffer_stop(void);
int sniffer_set_channel(uint16_t channel);
uint16_t sniffer_get_channel(void);

void sniffer_stream_get_stats(struct sniffer_stream_stats *stats);
void sniffer_stream_reset_stats(void);
uint32_t sniffer_stream_queue_used(void);
uint32_t sniffer_stream_queue_capacity(void);

#endif /* ESP154_SNIFFER_H_ */

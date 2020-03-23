/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_OPENTHREAD_H_
#define ZEPHYR_INCLUDE_NET_OPENTHREAD_H_

#include <kernel.h>

#include <net/net_if.h>

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pkt_list_elem {
	struct net_pkt *pkt;
};

struct openthread_context {
	otInstance *instance;
	struct net_if *iface;
	u16_t pkt_list_in_idx;
	u16_t pkt_list_out_idx;
	u8_t pkt_list_full;
	struct pkt_list_elem pkt_list[CONFIG_OPENTHREAD_PKT_LIST_SIZE];
};

k_tid_t openthread_thread_id_get(void);

enum openthread_signal_id {
	OPENTHREAD_SIGNAL_PENDING,
	OPENTHREAD_SIGNAL_BLOCKING,
	OPENTHREAD_SIGNALS
};

/**
 * @brief Sets openthread signal with given value
 *
 * @param signal_id     Identifier of openthread signal
 * @param signal_value  Value of signal to set
 *
 */
void ot_signal_set(enum openthread_signal_id signal_id,
							int signal_value);
/**
 * @brief Wait for specified openthread signal.
 *
 * Result is stored as the mask (argument n is n'th bit). Bit is set if string
 * format specifier was found.
 *
 * @param signal_start_id  Identifier of first openthread signal
 * @param num_events       Number of signals
 * @param timeout          Timeut in us
 *
 */
int ot_signal_poll(enum openthread_signal_id signal_start_id,
					int num_events, s32_t timeout);

/**
 * @brief Check if set, reads, and clears specified openthread signal
 *
 *
 * @param signal_id     Identifier openthread signal to read
 * @param signal_value  Value of obtained signal
 *
 * @retval              True if signal is valid
 *
 */
bool ot_signal_check_clear(enum openthread_signal_id signal_id,
						int *signal_value);

#define OPENTHREAD_L2_CTX_TYPE struct openthread_context

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_OPENTHREAD_H_ */

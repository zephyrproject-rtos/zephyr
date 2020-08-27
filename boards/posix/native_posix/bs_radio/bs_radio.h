/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BS_RADIO_H
#define _BS_RADIO_H

#include <stdint.h>
#include <stdbool.h>
#include "bs_radio_argparse.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct bs_radio_args bs_radio_args;

enum bs_radio_event_types {
	/** On reception success */
	BS_RADIO_EVENT_RX_DONE,

	/** On reception failure */
	BS_RADIO_EVENT_RX_FAILED,

	/** On transmission success */
	BS_RADIO_EVENT_TX_DONE,

	/** On transmittion failure */
	BS_RADIO_EVENT_TX_FAILED,

	/** On CCA success */
	BS_RADIO_EVENT_CCA_DONE,

	/** On CCA failure */
	BS_RADIO_EVENT_CCA_FAILED,

	/** On energy measurement success */
	BS_RADIO_EVENT_RSSI_DONE,

	/** On energy measurement failure */
	BS_RADIO_EVENT_RSSI_FAILED
};

struct bs_radio_event_data {
	enum bs_radio_event_types type;
	union {
		struct {
			/** Received data starts at psdu+1, psdu[0] holds the
			 * len of packet
			 */
			uint8_t *psdu;
			int8_t rssi;
			uint32_t timestamp;
		} rx_done;

		struct {
			uint16_t rssi;
		} energy_done;
	};
};

typedef void (*bs_radio_event_cb_t)(struct bs_radio_event_data *);

/* HW Models API */
void bs_radio_init(void);
void bs_radio_triggered(void);
void bs_radio_deinit(void);

/* User API */
void bs_radio_start(bs_radio_event_cb_t event_cb);
void bs_radio_stop(void);

int bs_radio_tx(uint8_t *data, bool cca);
int bs_radio_rssi(uint64_t duration_us);

int bs_radio_cca(void);
int bs_radio_channel_set(uint16_t channel);
uint16_t bs_radio_channel_get(void);
int bs_radio_tx_power_set(int8_t power_dBm);
int8_t bs_radio_tx_power_get(void);

void bs_radio_get_mac(uint8_t *mac);

#ifdef __cplusplus
}
#endif

#endif /* _BS_RADIO_H */

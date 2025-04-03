/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPENTHREAD_RCP_H
#define OPENTHREAD_RCP_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/net/ieee802154_radio.h>

#include "hdlc_coder.h"
#include "spinel_drv.h"

typedef void (*openthread_rcp_reception)(const struct spinel_frame_data *frame, const void *ctx);

struct openthread_rcp_buffer {
	uint8_t *data;
	size_t data_size;
};

struct openthread_rcp_data {
	const struct device *uart;
	struct k_work work;
	uint8_t tx_data[CONFIG_TELINK_W91_OT_RCP_BUFFER_SIZE];
	struct openthread_rcp_buffer tx_buffer;
	struct hdlc_coder hdlc;
	struct openthread_rcp_buffer spinel_rx_buffer;
	struct openthread_rcp_buffer
		spinel_msgq_buffer[CONFIG_TELINK_W91_OT_SPINEL_RX_BUFFER_COUNT];
	struct k_msgq spinel_msgq;
	struct spinel_drv_data spinel_drv;
	openthread_rcp_reception reception;
	const void *ctx;
};

int openthread_rcp_init(struct openthread_rcp_data *ot_rcp, const struct device *uart);
void openthread_rcp_reception_set(struct openthread_rcp_data *ot_rcp,
				  openthread_rcp_reception reception, const void *ctx);
int openthread_rcp_deinit(struct openthread_rcp_data *ot_rcp);
int openthread_rcp_reset(struct openthread_rcp_data *ot_rcp);
int openthread_rcp_ieee_eui64(struct openthread_rcp_data *ot_rcp, uint8_t ieee_eui64[8]);
int openthread_rcp_capabilities(struct openthread_rcp_data *ot_rcp,
				enum ieee802154_hw_caps *radio_caps);
int openthread_rcp_enable_src_match(struct openthread_rcp_data *ot_rcp, bool enable);
int openthread_rcp_ack_fpb(struct openthread_rcp_data *ot_rcp, uint16_t addr, bool enable);
int openthread_rcp_ack_fpb_ext(struct openthread_rcp_data *ot_rcp, uint8_t addr[8], bool enable);
int openthread_rcp_ack_fpb_clear(struct openthread_rcp_data *ot_rcp);
int openthread_rcp_mac_frame_counter(struct openthread_rcp_data *ot_rcp, uint32_t frame_counter,
				     bool set_if_larger);
int openthread_rcp_panid(struct openthread_rcp_data *ot_rcp, uint16_t pan_id);
int openthread_rcp_short_addr(struct openthread_rcp_data *ot_rcp, uint16_t addr);
int openthread_rcp_ext_addr(struct openthread_rcp_data *ot_rcp, uint8_t addr[8]);
int openthread_rcp_tx_power(struct openthread_rcp_data *ot_rcp, int8_t pwr_dbm);
int openthread_rcp_enable(struct openthread_rcp_data *ot_rcp, bool enable);
int openthread_rcp_receive_enable(struct openthread_rcp_data *ot_rcp, bool enable);
int openthread_rcp_channel(struct openthread_rcp_data *ot_rcp, uint8_t channel);
int openthread_rcp_transmit(struct openthread_rcp_data *ot_rcp, struct spinel_frame_data *frame);
int openthread_rcp_link_metrics(struct openthread_rcp_data *ot_rcp, uint16_t short_addr,
				const uint8_t ext_addr[8], struct spinel_link_metrics link_metrics);

#endif /* OPENTHREAD_RCP_H */

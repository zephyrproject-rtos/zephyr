/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SPINEL_DRV_H
#define SPINEL_DRV_H

#include <zephyr/kernel.h>
#include <zephyr/net/ieee802154_radio.h>

#include "spinel.h"

struct spinel_frame_data {
	uint8_t *data;
	size_t data_length;

	bool time_enabled;
	uint32_t time_base;
	uint32_t time_offset;

	union {
		struct {
			uint8_t channel;

			bool header_updated;
			bool security_processed;
			bool is_ret;
			bool csma_ca_enabled;
		} tx;
		struct {
			int8_t rssi;
			uint8_t lqi;

			bool frame_pending;
		} rx;
	};
};

struct spinel_link_metrics {
	bool pdu_count: 1;
	bool lqi: 1;
	bool link_margin: 1;
	bool rssi: 1;
};

#define SPINEL_MAC_KEY_SIZE 16

struct spinel_mac_keys {
	uint8_t key_mode;
	uint8_t key_id;
	uint8_t prev_key[SPINEL_MAC_KEY_SIZE];
	uint8_t curr_key[SPINEL_MAC_KEY_SIZE];
	uint8_t next_key[SPINEL_MAC_KEY_SIZE];
};

struct spinel_t_id {
	uint8_t act_id;
	uint32_t props[SPINEL_MAX_NUMB_TID];
};

struct spinel_drv_data {
	uint8_t inst;
	struct spinel_t_id t_id;
};

void spinel_drv_init(struct spinel_drv_data *spinel_drv, uint8_t inst);
int spinel_drv_send_reset(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb, const void *ctx,
			  uint8_t type);
bool spinel_drv_check_reset(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			    uint16_t data_size);
int spinel_drv_send_get_ieee_eui64(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				   const void *ctx);
bool spinel_drv_check_get_ieee_eui64(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				     uint16_t data_size, uint8_t ieee_eui64[8]);
int spinel_drv_send_get_capabilities(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				     const void *ctx);
bool spinel_drv_check_get_capabilities(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				       uint16_t data_size, enum ieee802154_hw_caps *radio_caps);
int spinel_drv_send_enable_src_match(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				     const void *ctx, bool enable);
bool spinel_drv_check_enable_src_match(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				       uint16_t data_size, bool enable);
int spinel_drv_send_ack_fpb(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb, const void *ctx,
			    uint16_t addr, bool enable);
bool spinel_drv_check_ack_fpb(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			      uint16_t data_size, uint16_t addr, bool enable);
int spinel_drv_send_ack_fpb_ext(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				const void *ctx, uint8_t addr[8], bool enable);
bool spinel_drv_check_ack_fpb_ext(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				  uint16_t data_size, uint8_t addr[8], bool enable);
int spinel_drv_send_ack_fpb_clear(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				  const void *ctx);
bool spinel_drv_check_ack_fpb_clear(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				    uint16_t data_size);
int spinel_drv_send_ack_fpb_ext_clear(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				      const void *ctx);
bool spinel_drv_check_ack_fpb_ext_clear(struct spinel_drv_data *spinel_drv, const uint8_t *data,
					uint16_t data_size);
int spinel_drv_send_mac_frame_counter(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				      const void *ctx, uint32_t frame_counter, bool set_if_larger);
bool spinel_drv_check_mac_frame_counter(struct spinel_drv_data *spinel_drv, const uint8_t *data,
					uint16_t data_size, uint32_t frame_counter);
int spinel_drv_send_panid(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb, const void *ctx,
			  uint16_t pan_id);
bool spinel_drv_check_panid(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			    uint16_t data_size, uint16_t pan_id);
int spinel_drv_send_short_addr(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
			       const void *ctx, uint16_t addr);
bool spinel_drv_check_short_addr(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				 uint16_t data_size, uint16_t addr);
int spinel_drv_send_ext_addr(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
			     const void *ctx, uint8_t addr[8]);
bool spinel_drv_check_ext_addr(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			       uint16_t data_size, uint8_t addr[8]);
int spinel_drv_send_tx_power(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
			     const void *ctx, int8_t pwr_dbm);
bool spinel_drv_check_tx_power(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			       uint16_t data_size, int8_t pwr_dbm);
int spinel_drv_send_rcp_enable(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
			       const void *ctx, bool enable);
bool spinel_drv_check_rcp_enable(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				 uint16_t data_size, bool enable);
int spinel_drv_send_receive_enable(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				   const void *ctx, bool enable);
bool spinel_drv_check_receive_enable(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				     uint16_t data_size, bool enable);
int spinel_drv_send_channel(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb, const void *ctx,
			    uint8_t channel);
bool spinel_drv_check_channel(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			      uint16_t data_size, uint8_t channel);
int spinel_drv_send_transmit_frame(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				   const void *ctx, const struct spinel_frame_data *frame);
bool spinel_drv_check_transmit_frame(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				     uint16_t data_size, struct spinel_frame_data *frame);
bool spinel_drv_check_receive_frame(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				    uint16_t data_size, struct spinel_frame_data *frame);
int spinel_drv_send_link_metrics(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				 const void *ctx, uint16_t short_addr, const uint8_t ext_addr[8],
				 struct spinel_link_metrics link_metrics);
bool spinel_drv_check_link_metrics(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				   uint16_t data_size);
int spinel_drv_send_mac_keys(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
			     const void *ctx, const struct spinel_mac_keys *keys);
bool spinel_drv_check_mac_keys(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			       uint16_t data_size);

#endif /* SPINEL_DRV_H */

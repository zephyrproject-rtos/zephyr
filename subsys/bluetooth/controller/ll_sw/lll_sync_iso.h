/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct lll_sync_iso_stream {
	uint8_t big_handle;
	uint8_t bis_index;
	struct ll_iso_rx_test_mode *test_mode;
	struct ll_iso_datapath *dp;
};

struct lll_sync_iso_data_chan {
	uint16_t prn_s;
	uint16_t remap_idx;
};

struct lll_sync_iso_data_chan_interleaved {
	uint16_t prn_s;
	uint16_t remap_idx;
	uint16_t id;
};

struct lll_sync_iso {
	struct lll_hdr hdr;

	uint8_t seed_access_addr[4];
	uint8_t base_crc_init[2];

	uint16_t iso_interval;

	uint16_t latency_prepare;
	uint16_t latency_event;
	union {
		struct lll_sync_iso_data_chan data_chan;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED)
		struct lll_sync_iso_data_chan_interleaved
			interleaved_data_chan[BT_CTLR_SYNC_ISO_STREAM_MAX];
#endif /* CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED */
	};
	uint8_t  next_chan_use;

	uint64_t payload_count:39;
	uint64_t framing:1;
	uint64_t enc:1;
	uint64_t ctrl:1;
	uint64_t cssn_curr:3;
	uint64_t cssn_next:3;

	uint8_t data_chan_map[PDU_CHANNEL_MAP_SIZE];
	uint8_t data_chan_count:6;
	uint8_t num_bis:5;
	uint8_t bn:3;
	uint8_t nse:5;
	uint8_t phy:3;

	uint32_t sub_interval:20;
	uint32_t max_pdu:8;
	uint32_t pto:4;

	uint32_t bis_spacing:20;
	uint32_t max_sdu:8;
	uint32_t irc:4;

	uint32_t sdu_interval:20;
	uint32_t irc_curr:4;
	uint32_t ptc_curr:4;
	uint32_t ptc:4;

	uint8_t bn_curr:3;
	uint8_t bis_curr:5;

	uint8_t stream_curr:5;
	uint8_t establish_events:3;

	/* Encryption */
	uint8_t giv[8];
	struct ccm ccm_rx;

	uint8_t chm_chan_map[PDU_CHANNEL_MAP_SIZE];
	uint8_t chm_chan_count:6;

	uint8_t is_lll_resume:1;

	uint8_t term_reason;

	uint16_t ctrl_instant;

	uint8_t stream_count;
	uint16_t stream_handle[BT_CTLR_SYNC_ISO_STREAM_MAX];

	struct node_rx_pdu *payload[BT_CTLR_SYNC_ISO_STREAM_MAX]
				   [PDU_BIG_PAYLOAD_COUNT_MAX];
	uint8_t payload_count_max;
	uint8_t payload_tail;

	uint32_t window_widening_periodic_us;
	uint32_t window_widening_max_us;
	uint32_t window_widening_prepare_us;
	uint32_t window_widening_event_us;
	uint32_t window_size_event_us;
};

int lll_sync_iso_init(void);
int lll_sync_iso_reset(void);
void lll_sync_iso_create_prepare(void *param);
void lll_sync_iso_prepare(void *param);
void lll_sync_iso_flush(uint8_t handle, struct lll_sync_iso *lll);

extern uint8_t ull_sync_iso_lll_index_get(struct lll_sync_iso *lll);
extern struct lll_sync_iso_stream *ull_sync_iso_lll_stream_get(uint16_t handle);
extern void ll_iso_rx_put(memq_link_t *link, void *rx);
extern void ll_rx_sched(void);

/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Macro to convert time in us to connection interval units */
#define RADIO_CONN_EVENTS(x, y) ((u16_t)(((x) + (y) - 1) / (y)))

/*
 * Macros to return correct Data Channel PDU time
 * Note: formula is valid for 1M, 2M and Coded S8
 * see BT spec Version 5.1 Vol 6. Part B, chapters 2.1 and 2.2
 * for packet formats and thus lengths
 *
 * Payload overhead size is the Data Channel PDU Header + the MIC
 */
#define PAYLOAD_OVERHEAD_SIZE (2 + 4)

#define PHY_1M BIT(0)
#define PHY_2M BIT(1)
#define PHY_CODED BIT(2)
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#define CODED_PHY_PREAMBLE_TIME_US (80)
#define CODED_PHY_ACCESS_ADDRESS_TIME_US (256)
#define CODED_PHY_CI_TIME_US (16)
#define CODED_PHY_TERM1_TIME_US (24)
#define CODED_PHY_CRC_SIZE (24)
#define CODED_PHY_TERM2_SIZE (3)

#define FEC_BLOCK1_TIME_US (CODED_PHY_ACCESS_ADDRESS_TIME_US + \
			    CODED_PHY_CI_TIME_US + \
			    CODED_PHY_TERM1_TIME_US)
#define FEC_BLOCK2_TIME_US(octets) ((((PAYLOAD_OVERHEAD_SIZE + \
				       (octets)) * 8) + \
				     CODED_PHY_CRC_SIZE + \
				     CODED_PHY_TERM2_SIZE) * 8)

#define PKT_US(octets, phy) (((phy) & PHY_CODED) ?		   \
			     (CODED_PHY_PREAMBLE_TIME_US + \
			      FEC_BLOCK1_TIME_US + \
			      FEC_BLOCK2_TIME_US(octets)) : \
			     (((PREAMBLE_SIZE(phy) + \
				ACCESS_ADDR_SIZE + \
				PAYLOAD_OVERHEAD_SIZE + \
				(octets) + \
				CRC_SIZE) * 8) / \
			      BIT(((phy) & 0x03) >> 1)))
#else /* !CONFIG_BT_CTLR_PHY_CODED */
#define PKT_US(octets, phy) ((((PREAMBLE_SIZE(phy)) +	\
			       ACCESS_ADDR_SIZE + \
			       PAYLOAD_OVERHEAD_SIZE + \
			       (octets) + \
			       CRC_SIZE) * 8) / \
			      BIT(((phy) & 0x03) >> 1))
#endif /* !CONFIG_BT_CTLR_PHY_CODED */

struct ll_conn *ll_conn_acquire(void);
void ll_conn_release(struct ll_conn *conn);
u16_t ll_conn_handle_get(struct ll_conn *conn);
struct ll_conn *ll_conn_get(u16_t handle);
struct ll_conn *ll_connected_get(u16_t handle);
int ull_conn_init(void);
int ull_conn_reset(void);
u8_t ull_conn_chan_map_cpy(u8_t *chan_map);
void ull_conn_chan_map_set(u8_t *chan_map);
u16_t ull_conn_default_tx_octets_get(void);
u16_t ull_conn_default_tx_time_get(void);
u8_t ull_conn_default_phy_tx_get(void);
u8_t ull_conn_default_phy_rx_get(void);
void ull_conn_setup(memq_link_t *link, struct node_rx_hdr *rx);
int ull_conn_rx(memq_link_t *link, struct node_rx_pdu **rx);
int ull_conn_llcp(struct ll_conn *conn, u32_t ticks_at_expire, u16_t lazy);
void ull_conn_done(struct node_rx_event_done *done);
void ull_conn_tx_demux(u8_t count);
void ull_conn_tx_lll_enqueue(struct ll_conn *conn, u8_t count);
void ull_conn_link_tx_release(void *link);
u8_t ull_conn_ack_last_idx_get(void);
memq_link_t *ull_conn_ack_peek(u8_t *ack_last, u16_t *handle,
			       struct node_tx **tx);
memq_link_t *ull_conn_ack_by_last_peek(u8_t last, u16_t *handle,
				       struct node_tx **tx);
void *ull_conn_ack_dequeue(void);
struct ll_conn *ull_conn_tx_ack(u16_t handle, memq_link_t *link,
				struct node_tx *tx);
u8_t ull_conn_llcp_req(void *conn);

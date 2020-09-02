/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * copied from ull_conn_internal.h
 * some macros will need to be moved to other, more logical places
 */

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

#else /* !CONFIG_BT_CTLR_PHY_CODED */
#endif /* !CONFIG_BT_CTLR_PHY_CODED */

#define ULL_HANDLE_NOT_CONNECTED 0xFFFF

struct ull_cp_conn *ll_conn_acquire(void);
void ll_conn_release(struct ull_cp_conn *conn);
uint16_t ll_conn_handle_get(struct ull_cp_conn *conn);
struct ull_cp_conn *ll_conn_get(uint16_t handle);
struct ull_cp_conn *ll_connected_get(uint16_t handle);
uint16_t ull_conn_init(void);
int ull_conn_reset(void);
uint8_t ull_conn_chan_map_cpy(uint8_t *chan_map);
void ull_conn_chan_map_set(uint8_t const *const chan_map);
uint16_t ull_conn_default_tx_octets_get(void);
uint16_t ull_conn_default_tx_time_get(void);
void ull_conn_default_tx_octets_set(uint16_t tx_octets);
void ull_conn_default_tx_time_set(uint16_t tx_time);
uint8_t ull_conn_default_phy_tx_get(void);
uint8_t ull_conn_default_phy_rx_get(void);
void ull_conn_default_phy_tx_set(uint8_t tx);
void ull_conn_default_phy_rx_set(uint8_t rx);
void ull_conn_setup(memq_link_t *link, struct node_rx_hdr *rx);
int ull_conn_rx(memq_link_t *link, struct node_rx_pdu **rx);
int ull_conn_llcp(struct ull_cp_conn *conn, uint32_t ticks_at_expire, uint16_t lazy);
void ull_conn_done(struct node_rx_event_done *done);
void ull_conn_tx_demux(uint8_t count);
void ull_conn_tx_lll_enqueue(struct ull_cp_conn *conn, uint8_t count);
void ull_conn_link_tx_release(void *link);
uint8_t ull_conn_ack_last_idx_get(void);
memq_link_t *ull_conn_ack_peek(uint8_t *ack_last, uint16_t *handle,
			       struct node_tx **tx);
memq_link_t *ull_conn_ack_by_last_peek(uint8_t last, uint16_t *handle,
				       struct node_tx **tx);
void *ull_conn_ack_dequeue(void);
struct ull_cp_conn *ull_conn_tx_ack(uint16_t handle, memq_link_t *link,
				struct node_tx *tx);
uint8_t ull_conn_llcp_req(void *conn);

void *ull_conn_tx_mem_acquire(void);
void ull_conn_tx_mem_release(void *tx);
uint8_t ull_conn_mfifo_get_tx(void *lll_tx);
void ull_conn_mfifo_enqueue_tx(uint8_t idx);

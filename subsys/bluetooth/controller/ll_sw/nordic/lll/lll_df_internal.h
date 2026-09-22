/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Forward declaration to avoid unnecessary includes. */
struct lll_adv_sync;
struct lll_sync;

/* Enables CTE transmission according to provided configuration */
void lll_df_cte_tx_enable(struct lll_adv_sync *lll_sync, const struct pdu_adv *pdu,
			  uint32_t *cte_len_us);

/* Allocate memory for new DF sync configuration. It will always return the same
 * pointer until buffer is swapped by lll_df_sync_cfg_latest_get operation.
 */
struct lll_df_sync_cfg *lll_df_sync_cfg_alloc(struct lll_df_sync *df_cfg,
					      uint8_t *idx);
/* Returns pointer to last allocated DF sync configuration. If it is called before
 * lll_df_sync_cfg_alloc it will return pointer to memory that was recently
 * enqueued.
 */
static inline struct lll_df_sync_cfg *lll_df_sync_cfg_peek(struct lll_df_sync *df_cfg)
{
	return &df_cfg->cfg[df_cfg->last];
}

/* Enqueue new DF sync configuration data. */
static inline void lll_df_sync_cfg_enqueue(struct lll_df_sync *df_cfg, uint8_t idx)
{
	df_cfg->last = idx;
}

/* Get latest DF sync configuration data. Latest configuration data are the one
 * that were enqueued by last lll_df_sync_cfg_enqueue call.
 */
struct lll_df_sync_cfg *lll_df_sync_cfg_latest_get(struct lll_df_sync *df_cfg,
						   uint8_t *is_modified);
/* Get current DF sync configuration data. Current configuration data
 * are the one that are available after last buffer swap done by call
 * lll_df_sync_cfg_latest_get.
 */
static inline struct lll_df_sync_cfg *lll_df_sync_cfg_curr_get(struct lll_df_sync *df_cfg)
{
	return &df_cfg->cfg[df_cfg->first];
}

/* Return information if DF sync configuration data were modified since last
 * call to lll_df_sync_cfg_latest_get.
 */
static inline uint8_t lll_df_sync_cfg_is_modified(struct lll_df_sync *df_cfg)
{
	return df_cfg->first != df_cfg->last;
}

/* Enables CTE reception according to provided configuration */
int lll_df_conf_cte_rx_enable(uint8_t slot_duration, uint8_t ant_num, const uint8_t *ant_ids,
			      uint8_t chan_idx, bool cte_info_in_s1, uint8_t phy);

/* Function prepares memory for Host notification about insufficient resources to sample all CTE
 * in a given periodic synchronization event.
 */
int lll_df_iq_report_no_resources_prepare(struct lll_sync *sync);

/* Configure CTE transmission */
void lll_df_cte_tx_configure(uint8_t cte_type, uint8_t cte_length, uint8_t num_ant_ids,
			     const uint8_t *ant_ids);

/* Disables CTE transmission */
void lll_df_cte_tx_disable(void);

/* Enabled parsing of a PDU for CTEInfo */
void lll_df_conf_cte_info_parsing_enable(void);

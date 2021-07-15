/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr.h>
#include <sys/util.h>
#include <bluetooth/hci.h>

#include "hal/cpu.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_sync.h"
#include "lll_df.h"
#include "lll/lll_df_internal.h"

#include "ull_scan_types.h"
#include "ull_sync_types.h"
#include "ull_sync_internal.h"
#include "ull_adv_types.h"
#include "ull_df_types.h"
#include "ull_df_internal.h"

#include "ull_adv_internal.h"
#include "ull_internal.h"

#include "ll.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_CTLR_DF_DEBUG_ENABLE)
#define LOG_MODULE_NAME bt_ctlr_ull_df
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)

#define CTE_LEN_MAX_US 160U

#define IQ_REPORT_HEADER_SIZE      (offsetof(struct node_rx_iq_report, pdu))
#define IQ_REPORT_STRUCT_OVERHEAD  (IQ_REPORT_HEADER_SIZE)
#define IQ_SAMPLE_SIZE (sizeof(struct iq_sample))

#define IQ_REPORT_RX_NODE_POOL_ELEMENT_SIZE              \
	MROUND(IQ_REPORT_STRUCT_OVERHEAD + (IQ_SAMPLE_TOTAL_CNT * IQ_SAMPLE_SIZE))
#define IQ_REPORT_POOL_SIZE (IQ_REPORT_RX_NODE_POOL_ELEMENT_SIZE * IQ_REPORT_CNT)

/* Memory pool to store IQ reports data */
static struct {
	void *free;
	uint8_t pool[IQ_REPORT_POOL_SIZE];
} mem_iq_report;

/* FIFO to store free IQ report norde_rx objects. */
static MFIFO_DEFINE(iq_report_free, sizeof(void *), IQ_REPORT_CNT);

/* Number of available instance of linked list to be used for node_rx_iq_reports. */
static uint8_t mem_link_iq_report_quota_pdu;
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

/* ToDo:
 * - Add release of df_adv_cfg when adv_sync is released.
 *   Open question, should df_adv_cfg be released when Adv. CTE is disabled?
 *   If yes that would mean, end user must always run ll_df_set_cl_cte_tx_params
 *   before consecutive Adv CTE enable.
 */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
static struct lll_df_adv_cfg lll_df_adv_cfg_pool[CONFIG_BT_CTLR_ADV_AUX_SET];
static void *df_adv_cfg_free;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

/* @brief Function performs common steps for initialization and reset
 * of Direction Finding ULL module.
 *
 * @return      Zero in case of success, other value in case of failure.
 */
static int init_reset(void);

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
/* @brief Function acquires memory for DF advertising configuration.
 *
 * The memory is acquired from private @ref lll_df_adv_cfg_pool memory store.
 *
 * @return Pointer to lll_df_adv_cfg or NULL if there is no more free memory.
 */
static struct lll_df_adv_cfg *df_adv_cfg_acquire(void);
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

/* @brief Function performs ULL Direction Finding initialization
 *
 * @return      Zero in case of success, other value in case of failure.
 */
int ull_df_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

/* @brief Function performs ULL Direction Finding reset
 *
 * @return      Zero in case of success, other value in case of failure.
 */
int ull_df_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

static int init_reset(void)
{
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	/* Initialize advertising DF memory configuration pool. */
	mem_init(lll_df_adv_cfg_pool, sizeof(struct lll_df_adv_cfg),
		 sizeof(lll_df_adv_cfg_pool) / sizeof(struct lll_df_adv_cfg),
		 &df_adv_cfg_free);
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	/* Re-initialize the free IQ report mfifo */
	MFIFO_INIT(iq_report_free);

	/* Initialize IQ report memory pool. */
	mem_init(mem_iq_report.pool, (IQ_REPORT_RX_NODE_POOL_ELEMENT_SIZE),
		 sizeof(mem_iq_report.pool) / (IQ_REPORT_RX_NODE_POOL_ELEMENT_SIZE),
		 &mem_iq_report.free);

	/* Allocate free IQ report node rx */
	mem_link_iq_report_quota_pdu = IQ_REPORT_CNT;
	ull_df_rx_iq_report_alloc(UINT8_MAX);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
	return 0;
}

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
/* @brief Function sets CTE transmission parameters for periodic advertising.
 *
 * @param[in]adv_handle                 Handle of advertising set.
 * @param[in]cte_len                    Length of CTE in 8us units.
 * @param[in]cte_type                   Type of CTE to be used for transmission.
 * @param[in]cte_count                  Number of CTE that should be transmitted
 *                                      during each periodic advertising
 *                                      interval.
 * @param[in]num_ant_ids                Number of antenna IDs in switching
 *                                      pattern. May be zero if CTE type is
 *                                      AoA.
 * @param[in]ant_ids                    Array of antenna IDs in a switching
 *                                      pattern. May be NULL if CTE type is AoA.
 *
 * @return Status of command completion.
 */
uint8_t ll_df_set_cl_cte_tx_params(uint8_t adv_handle, uint8_t cte_len,
				   uint8_t cte_type, uint8_t cte_count,
				   uint8_t num_ant_ids, uint8_t *ant_ids)
{
	struct lll_df_adv_cfg *cfg;
	struct ll_adv_set *adv;

	/* Get the advertising set instance */
	adv = ull_adv_is_created_get(adv_handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	if (cte_len < BT_HCI_LE_CTE_LEN_MIN ||
	    cte_len > BT_HCI_LE_CTE_LEN_MAX) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	/* ToDo: Check if there is a limit of per. adv. pdu that may be
	 * sent. This affects number of CTE that may be requested.
	 */
	if (cte_count < BT_HCI_LE_CTE_COUNT_MIN ||
	    cte_count > BT_HCI_LE_CTE_COUNT_MAX) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	if (!(IS_ENABLED(CONFIG_BT_CTLR_DF_ADV_CTE_TX) &&
	      ((cte_type == BT_HCI_LE_AOA_CTE) ||
		(IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX) &&
		 ((cte_type == BT_HCI_LE_AOD_CTE_2US) ||
		  (IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_1US) &&
		   cte_type == BT_HCI_LE_AOD_CTE_1US)))))) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	if ((cte_type == BT_HCI_LE_AOD_CTE_1US ||
	     cte_type == BT_HCI_LE_AOD_CTE_2US) &&
	    (num_ant_ids < LLL_DF_MIN_ANT_PATTERN_LEN ||
	     num_ant_ids > BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN ||
	     !ant_ids)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	if (!adv->df_cfg) {
		adv->df_cfg = df_adv_cfg_acquire();
	}

	cfg = adv->df_cfg;

	if (cfg->is_enabled) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	cfg->cte_count = cte_count;
	cfg->cte_length = cte_len;
	cfg->cte_type = cte_type;

	if (cte_type == BT_HCI_LE_AOD_CTE_1US ||
	    cte_type == BT_HCI_LE_AOD_CTE_2US) {
		/* Note:
		 * Are we going to check antenna identifiers if they are valid?
		 * BT 5.2 Core spec. Vol. 4 Part E Section 7.8.80 says
		 * that not all controller may be able to do that.
		 */
		memcpy(cfg->ant_ids, ant_ids, num_ant_ids);
		cfg->ant_sw_len = num_ant_ids;
	} else {
		cfg->ant_sw_len = 0;
	}

	return BT_HCI_ERR_SUCCESS;
}

/* @brief Function enables or disables CTE TX for periodic advertising.
 *
 * @param[in] handle                    Advertising set handle.
 * @param[in] cte_enable                Enable or disable CTE TX
 *
 * @return Status of command completion.
 */
uint8_t ll_df_set_cl_cte_tx_enable(uint8_t adv_handle, uint8_t cte_enable)
{
	void *extra_data_prev, *extra_data;
	struct pdu_adv *pdu_prev, *pdu;
	struct lll_adv_sync *lll_sync;
	struct lll_df_adv_cfg *df_cfg;
	struct ll_adv_sync_set *sync;
	struct ll_adv_set *adv;
	uint8_t err, ter_idx;

	/* Get the advertising set instance */
	adv = ull_adv_is_created_get(adv_handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll_sync = adv->lll.sync;
	/* If there is no sync in advertising set, then the HCI_LE_Set_-
	 * Periodic_Advertising_Parameters command was not issued before.
	 */
	if (!lll_sync) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	sync = HDR_LLL2ULL(lll_sync);

	/* If df_cfg is NULL, then the HCI_LE_Set_Connectionless_CTE_Transmit_-
	 * Parameters command was not issued before.
	 */
	df_cfg = adv->df_cfg;
	if (!df_cfg) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (adv->lll.phy_s == PHY_CODED) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (!cte_enable) {
		if (!df_cfg->is_enabled) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_NEVER, &pdu_prev,
					     &pdu, &extra_data_prev, &extra_data, &ter_idx);
		if (err) {
			return err;
		}

		if (extra_data) {
			ull_adv_sync_extra_data_set_clear(extra_data_prev,
							  extra_data, 0,
							  ULL_ADV_PDU_HDR_FIELD_CTE_INFO,
							  NULL);
		}

		err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu, 0,
						 ULL_ADV_PDU_HDR_FIELD_CTE_INFO,
						 NULL);
		if (err) {
			return err;
		}

		df_cfg->is_enabled = 0U;
	} else {
		struct pdu_cte_info cte_info;
		void *hdr_data;

		if (df_cfg->is_enabled) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		cte_info.type = df_cfg->cte_type;
		cte_info.time = df_cfg->cte_length;
		hdr_data = (uint8_t *)&cte_info;

		err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_ALWAYS, &pdu_prev,
					     &pdu, &extra_data_prev, &extra_data, &ter_idx);
		if (err) {
			return err;
		}

		if (extra_data) {
			ull_adv_sync_extra_data_set_clear(extra_data_prev, extra_data,
							  ULL_ADV_PDU_HDR_FIELD_CTE_INFO, 0,
							  df_cfg);
		}

		err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
						 ULL_ADV_PDU_HDR_FIELD_CTE_INFO, 0, hdr_data);
		if (err) {
			return err;
		}

		df_cfg->is_enabled = 1U;
	}

	if (sync->is_started) {
		err = ull_adv_sync_time_update(sync, pdu);
		if (err) {
			return err;
		}
	}

	lll_adv_sync_data_enqueue(adv->lll.sync, ter_idx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

/* @brief Function sets CTE transmission parameters for a connection.
 *
 * @param[in]handle                     Connection handle.
 * @param[in]cte_types                  Bitfield holding information about
 *                                      allowed CTE types.
 * @param[in]switch_pattern_len         Number of antenna ids in switch pattern.
 * @param[in]ant_id                     Array of antenna identifiers.
 *
 * @return Status of command completion.
 */
uint8_t ll_df_set_conn_cte_tx_params(uint16_t handle, uint8_t cte_types,
				     uint8_t switch_pattern_len,
				     uint8_t *ant_id)
{
	if (cte_types & BT_HCI_LE_AOD_CTE_RSP_1US ||
	    cte_types & BT_HCI_LE_AOD_CTE_RSP_2US) {

		if (!IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX)) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		if (switch_pattern_len < BT_HCI_LE_SWITCH_PATTERN_LEN_MIN ||
		    switch_pattern_len > BT_HCI_LE_SWITCH_PATTERN_LEN_MAX ||
		    !ant_id) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}
	}

	return BT_HCI_ERR_CMD_DISALLOWED;
}

/* @brief Function provides information about Direction Finding
 *        antennas switching and sampling related settings.
 *
 * @param[out]switch_sample_rates       Pointer to store available antennas
 *                                      switch-sampling configurations.
 * @param[out]num_ant                   Pointer to store number of available
 *                                      antennas.
 * @param[out]max_switch_pattern_len    Pointer to store maximum number of
 *                                      antennas ids in switch pattern.
 * @param[out]max_cte_len               Pointer to store maximum length of CTE
 *                                      in [8us] units.
 */
void ll_df_read_ant_inf(uint8_t *switch_sample_rates,
			uint8_t *num_ant,
			uint8_t *max_switch_pattern_len,
			uint8_t *max_cte_len)
{
	*switch_sample_rates = 0;
	if (IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX) &&
	    IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_1US)) {
		*switch_sample_rates |= DF_AOD_1US_TX;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_DF_CTE_RX) &&
	    IS_ENABLED(CONFIG_BT_CTLR_DF_CTE_RX_SAMPLE_1US)) {
		*switch_sample_rates |= DF_AOD_1US_RX;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX) &&
	    IS_ENABLED(CONFIG_BT_CTLR_DF_CTE_RX_SAMPLE_1US)) {
		*switch_sample_rates |= DF_AOA_1US;
	}

	*max_switch_pattern_len = BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN;
	*num_ant = lll_df_ant_num_get();
	*max_cte_len = LLL_DF_MAX_CTE_LEN;
}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
/* @brief Function sets IQ sampling enabled or disabled.
 *
 * Set IQ sampling enable for received PDUs that has attached CTE.
 *
 * @param[in]handle                     Connection handle.
 * @param[in]sampling_enable            Enable or disable CTE RX
 * @param[in]slot_durations             Switching and samplig slot durations for
 *                                      AoA mode.
 * @param[in]max_cte_count              Maximum number of sampled CTEs in single
 *                                      periodic advertising event.
 * @param[in]switch_pattern_len         Number of antenna ids in switch pattern.
 * @param[in]ant_ids                    Array of antenna identifiers.
 *
 * @return Status of command completion.
 *
 * @Note This function may put TX thread into wait state. This may lead to a
 *       situation that ll_sync_set instnace is relased (RX thread has higher
 *       priority than TX thread). l_sync_set instance may not be accessed after
 *       call to ull_sync_slot_update.
 *       This is related with possible race condition with RX thread handling
 *       periodic sync lost event.
 */
uint8_t ll_df_set_cl_iq_sampling_enable(uint16_t handle,
					uint8_t sampling_enable,
					uint8_t slot_durations,
					uint8_t max_cte_count,
					uint8_t switch_pattern_len,
					uint8_t *ant_ids)
{
	struct lll_df_sync_cfg *cfg, *cfg_prev;
	uint32_t slot_minus_us = 0;
	uint32_t slot_plus_us = 0;
	struct ll_sync_set *sync;
	struct lll_sync *lll;
	uint8_t cfg_idx;

	/* After this call and before ull_sync_slot_update the function may not
	 * call any kernel API that may put the thread into wait state. It may
	 * cause race condition with RX thread and lead to use of released memory.
	 */
	sync = ull_sync_is_enabled_get(handle);
	if (!sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll = &sync->lll;

	/* CTE is not supported for CODED Phy */
	if (lll->phy == PHY_CODED) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	cfg_prev = lll_df_sync_cfg_curr_get(&lll->df_cfg);
	cfg = lll_df_sync_cfg_alloc(&lll->df_cfg, &cfg_idx);

	if (!sampling_enable) {
		if (!cfg_prev->is_enabled) {
			/* Disable already disabled CTE Rx */
			return BT_HCI_ERR_SUCCESS;
		}
		slot_minus_us = CTE_LEN_MAX_US;
		cfg->is_enabled = 0U;
	} else {
		/* Enable of already enabled CTE updates AoA configuration */
		if (!((IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_1US) &&
		       slot_durations == BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US) ||
		      slot_durations == BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US)) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		/* max_cte_count equal to 0x0 has special meaning - sample and
		 * report continuously until there are CTEs received.
		 */
		if (max_cte_count > BT_HCI_LE_SAMPLE_CTE_COUNT_MAX) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		if (switch_pattern_len < BT_HCI_LE_SWITCH_PATTERN_LEN_MIN ||
		    switch_pattern_len > BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN ||
		    !ant_ids) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		cfg->slot_durations = slot_durations;
		cfg->max_cte_count = max_cte_count;
		memcpy(cfg->ant_ids, ant_ids, switch_pattern_len);
		cfg->ant_sw_len = switch_pattern_len;

		cfg->is_enabled = 1U;

		if (!cfg_prev->is_enabled) {
			/* Extend sync event by maximum CTE duration.
			 * CTE duration denepnds on transmitter configuration
			 * so it is unknown for receiver upfront.
			 */
			slot_plus_us = BT_HCI_LE_CTE_LEN_MAX;
		}
	}

	lll_df_sync_cfg_enqueue(&lll->df_cfg, cfg_idx);

	if (slot_plus_us || slot_minus_us) {
		int err;
		/* Update of sync slot may fail due to race condition.
		 * If periodic sync is lost, the ticker event will be stopped.
		 * The stop operation may preempt call to this functions.
		 * So update may be called after that. Accept this failure
		 * (-ENOENT) gracefully.
		 * Periodic sync lost event also disables the CTE sampling.
		 */
		err = ull_sync_slot_update(sync, 0, CTE_LEN_MAX_US);
		LL_ASSERT(err == 0 || err == -ENOENT);
	}

	return 0;
}

void *ull_df_iq_report_alloc_peek(uint8_t count)
{
	if (count > MFIFO_AVAIL_COUNT_GET(iq_report_free)) {
		return NULL;
	}

	return MFIFO_DEQUEUE_PEEK(iq_report_free);
}

void *ull_df_iq_report_alloc_peek_iter(uint8_t *idx)
{
	return *(void **)MFIFO_DEQUEUE_ITER_GET(iq_report_free, idx);
}

void *ull_df_iq_report_alloc(void)
{
	return MFIFO_DEQUEUE(iq_report_free);
}

void ull_df_iq_report_mem_release(struct node_rx_hdr *rx)
{
	mem_release(rx, &mem_iq_report.free);
}

void ull_iq_report_link_inc_quota(int8_t delta)
{
	LL_ASSERT(delta <= 0 || mem_link_iq_report_quota_pdu < IQ_REPORT_CNT);
	mem_link_iq_report_quota_pdu += delta;
}

void ull_df_rx_iq_report_alloc(uint8_t max)
{
	uint8_t idx;

	if (max > mem_link_iq_report_quota_pdu) {
		max = mem_link_iq_report_quota_pdu;
	}

	while ((max--) && MFIFO_ENQUEUE_IDX_GET(iq_report_free, &idx)) {
		memq_link_t *link;
		struct node_rx_hdr *rx;

		link = ll_rx_link_alloc();
		if (!link) {
			return;
		}

		rx = mem_acquire(&mem_iq_report.free);
		if (!rx) {
			ll_rx_link_release(link);
			return;
		}

		rx->link = link;

		MFIFO_BY_IDX_ENQUEUE(iq_report_free, idx, rx);

		ull_iq_report_link_inc_quota(-1);
	}
}

void ull_df_sync_cfg_init(struct lll_df_sync *cfg)
{
	memset(&cfg->cfg[0], 0, DOUBLE_BUFFER_SIZE * sizeof(cfg->cfg[0]));
	cfg->first = 0U;
	cfg->last = 0U;
}

uint8_t ull_df_sync_cfg_is_disabled_or_requested_to_disable(struct lll_df_sync *df_cfg)
{
	struct lll_df_sync_cfg *cfg;

	/* If new CTE sampling configuration was enqueued, get reference to
	 * latest congiruation without swapping buffers. Buffer should be
	 * swapped only at the beginning of the radio event.
	 *
	 * We may not get here if CTE sampling is not enabled in current
	 * configuration.
	 */
	if (lll_df_sync_cfg_is_modified(df_cfg)) {
		cfg = lll_df_sync_cfg_peek(df_cfg);
	} else {
		cfg = lll_df_sync_cfg_curr_get(df_cfg);
	}

	return !cfg->is_enabled;
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
/* @brief Function releases unused memory for DF advertising configuration.
 *
 * The memory is released to private @ref lll_df_adv_cfg_pool memory store.
 *
 * @param[in] df_adv_cfg        Pointer to lll_df_adv_cfg memory to be released.
 */
void ull_df_adv_cfg_release(struct lll_df_adv_cfg *df_adv_cfg)
{
	mem_release(df_adv_cfg, &df_adv_cfg_free);
}

static struct lll_df_adv_cfg *df_adv_cfg_acquire(void)
{
	struct lll_df_adv_cfg *df_adv_cfg;

	df_adv_cfg = mem_acquire(&df_adv_cfg_free);
	if (!df_adv_cfg) {
		return NULL;
	}

	df_adv_cfg->is_enabled = 0U;

	return df_adv_cfg;
}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

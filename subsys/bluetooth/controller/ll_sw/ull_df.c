/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/hci_types.h>

#include "hal/cpu.h"
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"
#include "lll_df.h"
#include "lll/lll_df_internal.h"

#include "isoal.h"
#include "ull_scan_types.h"
#include "ull_sync_types.h"
#include "ull_adv_types.h"
#include "ull_tx_queue.h"
#include "ull_conn_types.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_df_types.h"
#include "ull_llcp.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_sync_internal.h"
#include "ull_conn_internal.h"
#include "ull_df_internal.h"

#include "ll.h"

#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX) || \
	defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)

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

/* FIFO to store free IQ report norde_rx objects for LLL to ULL handover. */
static MFIFO_DEFINE(iq_report_free, sizeof(void *), IQ_REPORT_CNT);

/* Number of available instance of linked list to be used for node_rx_iq_reports. */
static uint8_t mem_link_iq_report_quota_pdu;

#if defined(CONFIG_BT_CTLR_DF_DEBUG_ENABLE)
/* Debug variable to store information about current number of allocated node_rx_iq_report.
 * It supports verification if there is a resource leak.
 * The variable may not be used when multiple
 * advertising syncs are enabled. Checks may fail because CTE reception may be enabled/disabled
 * in different moments, hence there may be allocated reports when it is expected not to.
 */
COND_CODE_1(CONFIG_BT_PER_ADV_SYNC_MAX, (static uint32_t iq_report_alloc_count;), (EMPTY))
#define IF_SINGLE_ADV_SYNC_SET(code) COND_CODE_1(CONFIG_BT_PER_ADV_SYNC_MAX, (code), (EMPTY))
#endif /* CONFIG_BT_CTLR_DF_DEBUG_ENABLE */
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX || CONFIG_BT_CTLR_DF_CONN_CTE_RX*/

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
/* Make sure the configuration follows BT Core 5.3. Vol 4 Part E section 7.8.82 about
 * max CTE count sampled in periodic advertising chain.
 */
BUILD_ASSERT(CONFIG_BT_CTLR_DF_PER_SCAN_CTE_NUM_MAX <= BT_HCI_LE_SAMPLE_CTE_COUNT_MAX,
	     "Max advertising CTE count exceed BT_HCI_LE_SAMPLE_CTE_COUNT_MAX");
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

/* ToDo:
 * - Add release of df_adv_cfg when adv_sync is released.
 *   Open question, should df_adv_cfg be released when Adv. CTE is disabled?
 *   If yes that would mean, end user must always run ll_df_set_cl_cte_tx_params
 *   before consecutive Adv CTE enable.
 */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
/* Make sure the configuration follows BT Core 5.3. Vol 4 Part E section 7.8.80 about
 * max CTE count in a periodic advertising chain.
 */
BUILD_ASSERT(CONFIG_BT_CTLR_DF_PER_ADV_CTE_NUM_MAX <= BT_HCI_LE_CTE_COUNT_MAX,
	     "Max advertising CTE count exceed BT_HCI_LE_CTE_COUNT_MAX");

static struct lll_df_adv_cfg lll_df_adv_cfg_pool[CONFIG_BT_CTLR_ADV_AUX_SET];
static void *df_adv_cfg_free;
static uint8_t cte_info_clear(struct ll_adv_set *adv, struct lll_df_adv_cfg *df_cfg,
			      uint8_t *ter_idx, struct pdu_adv **first_pdu);
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

static uint8_t cte_info_set(struct ll_adv_set *adv, struct lll_df_adv_cfg *df_cfg, uint8_t *ter_idx,
			    struct pdu_adv **first_pdu);
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

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	struct ll_adv_set *adv;
	uint8_t handle;

	/* Get the advertising set instance */
	for (handle = 0U; handle < BT_CTLR_ADV_SET; handle++) {
		adv = ull_adv_is_created_get(handle);
		if (!adv) {
			continue;
		}

		adv->df_cfg = NULL;
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

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

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX) || \
	defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
	/* Re-initialize the free IQ report mfifo */
	MFIFO_INIT(iq_report_free);

	/* Initialize IQ report memory pool. */
	mem_init(mem_iq_report.pool, (IQ_REPORT_RX_NODE_POOL_ELEMENT_SIZE),
		 sizeof(mem_iq_report.pool) / (IQ_REPORT_RX_NODE_POOL_ELEMENT_SIZE),
		 &mem_iq_report.free);

	/* Allocate free IQ report node rx */
	mem_link_iq_report_quota_pdu = IQ_REPORT_CNT;
	ull_df_rx_iq_report_alloc(UINT8_MAX);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX || CONFIG_BT_CTLR_DF_CONN_CTE_RX */
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

	/* Max number of CTE in a single periodic advertising event is limited
	 * by configuration. It shall not be greater than BT_HCI_LE_CTE_COUNT_MAX.
	 */
	if (cte_count < BT_HCI_LE_CTE_COUNT_MIN ||
	    cte_count > CONFIG_BT_CTLR_DF_PER_ADV_CTE_NUM_MAX) {
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

	if ((cte_type == BT_HCI_LE_AOD_CTE_1US || cte_type == BT_HCI_LE_AOD_CTE_2US) &&
	    (num_ant_ids < BT_HCI_LE_CTE_LEN_MIN ||
	     num_ant_ids > BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN || !ant_ids)) {
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
	struct lll_adv_sync *lll_sync;
	struct lll_df_adv_cfg *df_cfg;
	struct ll_adv_sync_set *sync;
	struct ll_adv_set *adv;
	uint8_t err, ter_idx;
	struct pdu_adv *pdu;

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

		err = cte_info_clear(adv, df_cfg, &ter_idx, &pdu);
		if (err) {
			return err;
		}

		df_cfg->is_enabled = 0U;
	} else {
		if (df_cfg->is_enabled) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		err = cte_info_set(adv, df_cfg, &ter_idx, &pdu);
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

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
/* @brief Function sets IQ sampling enabled or disabled.
 *
 * Set IQ sampling enable for received PDUs that has attached CTE.
 *
 * @param[in]handle                     Connection handle.
 * @param[in]sampling_enable            Enable or disable CTE RX
 * @param[in]slot_durations             Switching and sampling slot durations for
 *                                      AoA mode.
 * @param[in]max_cte_count              Maximum number of sampled CTEs in single
 *                                      periodic advertising event.
 * @param[in]switch_pattern_len         Number of antenna ids in switch pattern.
 * @param[in]ant_ids                    Array of antenna identifiers.
 *
 * @return Status of command completion.
 *
 * @Note This function may put TX thread into wait state. This may lead to a
 *       situation that ll_sync_set instance is released (RX thread has higher
 *       priority than TX thread). ll_sync_set instance may not be accessed after
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

#if defined(CONFIG_BT_CTLR_DF_DEBUG_ENABLE)
		/* When CTE is enabled there should be no iq report allocated */
		IF_SINGLE_ADV_SYNC_SET(LL_ASSERT(iq_report_alloc_count == 0));
#endif /* CONFIG_BT_CTLR_DF_DEBUG_ENABLE */

		/* Enable of already enabled CTE updates AoA configuration */

		/* According to Core 5.3 Vol 4, Part E, section 7.8.82 slot_durations,
		 * switch_pattern_len and ant_ids are used only for AoA and do not affect
		 * reception of AoD CTE. If AoA is not supported relax command validation
		 * to improve interoperability with different Host implementations.
		 */
		if (IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)) {
			if (!((IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_1US) &&
			       slot_durations == BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US) ||
			      slot_durations == BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US)) {
				return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
			}

			if (switch_pattern_len < BT_HCI_LE_SWITCH_PATTERN_LEN_MIN ||
			    switch_pattern_len > BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN || !ant_ids) {
				return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
			}

			(void)memcpy(cfg->ant_ids, ant_ids, switch_pattern_len);
		}
		cfg->slot_durations = slot_durations;
		cfg->ant_sw_len = switch_pattern_len;

		/* max_cte_count equal to 0x0 has special meaning - sample and
		 * report continuously until there are CTEs received.
		 */
		if (max_cte_count > CONFIG_BT_CTLR_DF_PER_SCAN_CTE_NUM_MAX) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}
		cfg->max_cte_count = max_cte_count;

		cfg->is_enabled = 1U;

		if (!cfg_prev->is_enabled) {
			/* Extend sync event by maximum CTE duration.
			 * CTE duration depends on transmitter configuration
			 * so it is unknown for receiver upfront.
			 * BT_HCI_LE_CTE_LEN_MAX is in 8us units.
			 */
			slot_plus_us = BT_HCI_LE_CTE_LEN_MAX * 8U;
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
		err = ull_sync_slot_update(sync, slot_plus_us, slot_minus_us);
		LL_ASSERT(err == 0 || err == -ENOENT);
	}

	return 0;
}

void ull_df_sync_cfg_init(struct lll_df_sync *df_cfg)
{
	(void)memset(&df_cfg->cfg, 0, sizeof(df_cfg->cfg));
	df_cfg->first = 0U;
	df_cfg->last = 0U;
}

bool ull_df_sync_cfg_is_not_enabled(struct lll_df_sync *df_cfg)
{
	struct lll_df_sync_cfg *cfg;

	/* If new CTE sampling configuration was enqueued, get reference to
	 * latest configuration without swapping buffers. Buffer should be
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

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX) || \
	defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
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
#if defined(CONFIG_BT_CTLR_DF_DEBUG_ENABLE)
	IF_SINGLE_ADV_SYNC_SET(iq_report_alloc_count++);
#endif /* CONFIG_BT_CTLR_DF_DEBUG_ENABLE */

	return MFIFO_DEQUEUE(iq_report_free);
}

void ull_df_iq_report_mem_release(struct node_rx_pdu *rx)
{
#if defined(CONFIG_BT_CTLR_DF_DEBUG_ENABLE)
	IF_SINGLE_ADV_SYNC_SET(iq_report_alloc_count--);
#endif /* CONFIG_BT_CTLR_DF_DEBUG_ENABLE */

	mem_release(rx, &mem_iq_report.free);
}

void ull_iq_report_link_inc_quota(int8_t delta)
{
	LL_ASSERT(delta <= 0 || mem_link_iq_report_quota_pdu < (IQ_REPORT_CNT));

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
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX || CONFIG_BT_CTLR_DF_CONN_CTE_RX */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
bool ull_df_conn_cfg_is_not_enabled(struct lll_df_conn_rx_cfg *rx_cfg)
{
	struct lll_df_conn_rx_params *rx_params;

	/* If new CTE sampling configuration was enqueued, get reference to
	 * latest configuration without swapping buffers. Buffer should be
	 * swapped only at the beginning of the radio event.
	 *
	 * We may not get here if CTE sampling is not enabled in current
	 * configuration.
	 */
	if (dbuf_is_modified(&rx_cfg->hdr)) {
		rx_params = dbuf_peek(&rx_cfg->hdr);
	} else {
		rx_params = dbuf_curr_get(&rx_cfg->hdr);
	}

	return !rx_params->is_enabled;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

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

/*
 * @brief Function sets content of cte_info field for periodic advertising
 *
 * @param adv            Pointer to periodic advertising set.
 * @param df_cfg         Pointer to direction finding configuration
 * @param[out] ter_idx   Pointer used to return index of allocated or updated PDU.
 *                       Index is required for scheduling the PDU for transmission in LLL.
 * @param[out] first_pdu Pointer to return address of first PDU in a periodic advertising chain
 *
 * @return Zero in case of success, other value in case of failure.
 */
static uint8_t cte_info_set(struct ll_adv_set *adv, struct lll_df_adv_cfg *df_cfg, uint8_t *ter_idx,
			    struct pdu_adv **first_pdu)
{
	struct pdu_adv *pdu_prev, *pdu;
	struct lll_adv_sync *lll_sync;
	struct pdu_cte_info cte_info;
	void *extra_data;
	uint8_t err;

	lll_sync = adv->lll.sync;

	cte_info.type = df_cfg->cte_type;
	cte_info.time = df_cfg->cte_length;
	cte_info.rfu = 0U;

	/* Note: ULL_ADV_PDU_EXTRA_DATA_ALLOC_ALWAYS is just information that extra_data
	 * is required in case of this ull_adv_sync_pdu_alloc.
	 */
	extra_data = NULL;
	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_ALWAYS, &pdu_prev, &pdu,
				     NULL, &extra_data, ter_idx);
	if (err != BT_HCI_ERR_SUCCESS) {
		return err;
	}

	if (extra_data) {
		ull_adv_sync_extra_data_set_clear(NULL, extra_data, ULL_ADV_PDU_HDR_FIELD_CTE_INFO,
						  0, df_cfg);
	}

#if (CONFIG_BT_CTLR_DF_PER_ADV_CTE_NUM_MAX > 1)
	err = ull_adv_sync_add_cteinfo(lll_sync, pdu_prev, pdu, &cte_info, df_cfg->cte_count);
#else
	err = ull_adv_sync_add_cteinfo(lll_sync, pdu_prev, pdu, &cte_info, 1U);
#endif

	if (err != BT_HCI_ERR_SUCCESS) {
		return err;
	}

	*first_pdu = pdu;

	return BT_HCI_ERR_SUCCESS;
}

/*
 * @brief Function removes content of cte_info field from periodic advertising PDUs.
 *
 * @param adv            Pointer to periodic advertising set.
 * @param df_cfg         Pointer to direction finding configuration
 * @param[out] ter_idx   Pointer used to return index of allocated or updated PDU.
 *                       Index is required for scheduling the PDU for transmission in LLL.
 * @param[out] first_pdu Pointer to return address of first PDU in a chain
 *
 * @return Zero in case of success, other value in case of failure.
 */
static uint8_t cte_info_clear(struct ll_adv_set *adv, struct lll_df_adv_cfg *df_cfg,
			      uint8_t *ter_idx, struct pdu_adv **first_pdu)
{
	void *extra_data_prev, *extra_data;
	struct pdu_adv *pdu_prev, *pdu;
	struct lll_adv_sync *lll_sync;
	uint8_t err;

	lll_sync = adv->lll.sync;

	/* NOTE: ULL_ADV_PDU_EXTRA_DATA_ALLOC_NEVER is just information that extra_data
	 * should be removed in case of this call ull_adv_sync_pdu_alloc.
	 */
	extra_data_prev = NULL;
	extra_data = NULL;
	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_NEVER, &pdu_prev, &pdu,
				     &extra_data_prev, &extra_data, ter_idx);
	if (err != BT_HCI_ERR_SUCCESS) {
		return err;
	}

	if (extra_data_prev && extra_data) {
		ull_adv_sync_extra_data_set_clear(extra_data_prev, extra_data, 0,
						  ULL_ADV_PDU_HDR_FIELD_CTE_INFO, NULL);
	}

	*first_pdu = pdu;

	err = ull_adv_sync_remove_cteinfo(lll_sync, pdu_prev, pdu);

	if (err != BT_HCI_ERR_SUCCESS) {
		/* TODO: return here leaves periodic advertising chain in an inconsistent state.
		 * Add graceful return or assert.
		 */
		return err;
	}

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX)
/* @brief Function sets CTE transmission parameters for a connection.
 *
 * @param handle             Connection handle.
 * @param cte_types          Bitfield holding information about
 *                           allowed CTE types.
 * @param switch_pattern_len Number of antenna ids in switch pattern.
 * @param ant_id             Array of antenna identifiers.
 *
 * @return Status of command completion.
 */
uint8_t ll_df_set_conn_cte_tx_params(uint16_t handle, uint8_t cte_types, uint8_t switch_pattern_len,
				     const uint8_t *ant_ids)
{
	struct lll_df_conn_tx_cfg *df_tx_cfg;
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	df_tx_cfg = &conn->lll.df_tx_cfg;

	if (df_tx_cfg->cte_rsp_en) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Bits other than representing AoA, AoD 1us, AoD 2us are RFU */
	if (cte_types == 0U ||
	    ((cte_types & (~(uint8_t)(BT_HCI_LE_AOA_CTE_RSP | BT_HCI_LE_AOD_CTE_RSP_1US |
				      BT_HCI_LE_AOD_CTE_RSP_2US))) != 0U)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	if (!IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX)) {
		if (cte_types & BT_HCI_LE_AOD_CTE_RSP_2US) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		if ((cte_types & BT_HCI_LE_AOD_CTE_RSP_1US) &&
		    !IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_1US)) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}
	}

	/* Check antenna switching pattern only whether CTE TX in AoD mode is allowed */
	if (((cte_types & BT_HCI_LE_AOD_CTE_RSP_1US) || (cte_types & BT_HCI_LE_AOD_CTE_RSP_2US)) &&
	    (switch_pattern_len < BT_HCI_LE_SWITCH_PATTERN_LEN_MIN ||
	     switch_pattern_len > BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN || !ant_ids)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	(void)memcpy(df_tx_cfg->ant_ids, ant_ids, switch_pattern_len);
	df_tx_cfg->ant_sw_len = switch_pattern_len;

	df_tx_cfg->cte_types_allowed = cte_types;
	df_tx_cfg->is_initialized = 1U;

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_TX */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
/**
 * @brief Function sets CTE reception parameters for a connection.
 *
 * @note: The CTE may not be send/received with PHY CODED. The BT Core 5.3 specification does not
 *        mention special handling of CTE receive and sampling while the functionality is enabled
 *        for a connection that currently uses PHY CODED. Enable of CTE receive for a PHY CODED
 *        will introduce complications for TISF maintenance by software switch. To avoid that
 *        the lower link layer will enable the functionality when connection uses PHY UNCODED only.
 *
 * @param handle             Connection handle.
 * @param sampling_enable    Enable or disable CTE RX. When the parameter is set to false,
 *                           @p slot_durations, @p switch_pattern_len and @ant_ids are ignored.
 * @param slot_durations     Switching and sampling slot durations for AoA mode.
 * @param switch_pattern_len Number of antenna ids in switch pattern.
 * @param ant_ids            Array of antenna identifiers.
 *
 * @return HCI status of command completion.
 */
uint8_t ll_df_set_conn_cte_rx_params(uint16_t handle, uint8_t sampling_enable,
				     uint8_t slot_durations, uint8_t switch_pattern_len,
				     const uint8_t *ant_ids)
{
	struct lll_df_conn_rx_params *params_rx;
	struct dbuf_hdr *params_buf_hdr;
	struct lll_df_conn_rx_cfg *cfg_rx;
	struct ll_conn *conn;
	uint8_t params_idx;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	cfg_rx = &conn->lll.df_rx_cfg;
	/* This is an information for HCI_LE_Connection_CTE_Request_Enable that
	 * HCI_LE_Set_Connection_CTE_Receive_Parameters was called at least once.
	 */
	cfg_rx->is_initialized = 1U;
	params_buf_hdr = &cfg_rx->hdr;

	params_rx = dbuf_alloc(params_buf_hdr, &params_idx);

	if (!sampling_enable) {
		params_rx->is_enabled = false;
	} else {
		/* According to Core 5.3 Vol 4, Part E, section 7.8.83 slot_durations,
		 * switch_pattern_len and ant_ids are used only for AoA and do not affect
		 * reception of AoD CTE. If AoA is not supported relax command validation
		 * to improve interoperability with different Host implementations.
		 */
		if (IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)) {
			if (!((IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_1US) &&
			       slot_durations == BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US) ||
			      slot_durations == BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US)) {
				return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
			}

			if (switch_pattern_len < BT_HCI_LE_SWITCH_PATTERN_LEN_MIN ||
			    switch_pattern_len > BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN || !ant_ids) {
				return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
			}
		}

		params_rx->is_enabled = true;
		params_rx->slot_durations = slot_durations;
		(void)memcpy(params_rx->ant_ids, ant_ids, switch_pattern_len);
		params_rx->ant_sw_len = switch_pattern_len;
	}

	dbuf_enqueue(params_buf_hdr, params_idx);

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
static void df_conn_cte_req_disable(void *param)
{
	k_sem_give(param);
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
/* @brief Function enables or disables CTE request control procedure for a connection.
 *
 * The procedure may be enabled in two modes:
 * - single-shot, it is automatically disabled when the occurrence finishes.
 * - periodic, it is executed periodically until disabled, connection is lost or PHY is changed
 *   to the one that does not support CTE.
 *
 * @param handle               Connection handle.
 * @param enable               Enable or disable CTE request. When the parameter is set to false
 *                             @p cte_request_interval, @requested_cte_length and
 *                             @p requested_cte_type are ignored.
 * @param cte_request_interval Value zero enables single-shot mode. Other values enable periodic
 *                             mode. In periodic mode, the value is a number of connection envets
 *                             the procedure is executed. The value may not be lower than
 *                             connection peer latency.
 * @param requested_cte_length Minimum value of CTE length requested from peer.
 * @param requested_cte_type   Type of CTE requested from peer.
 *
 * @return HCI Status of command completion.
 */
uint8_t ll_df_set_conn_cte_req_enable(uint16_t handle, uint8_t enable,
				      uint16_t cte_request_interval, uint8_t requested_cte_length,
				      uint8_t requested_cte_type)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (!enable) {
		ull_cp_cte_req_set_disable(conn);

		return BT_HCI_ERR_SUCCESS;
	}

	if (!conn->lll.df_rx_cfg.is_initialized) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (conn->llcp.cte_req.is_enabled) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

#if defined(CONFIG_BT_CTLR_PHY)
	/* CTE request may be enabled only in case the receiver PHY is not CODED */
	if (conn->lll.phy_rx == PHY_CODED) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_CTLR_PHY */

	if (cte_request_interval != 0 && cte_request_interval < conn->lll.latency) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (requested_cte_length < BT_HCI_LE_CTE_LEN_MIN ||
	    requested_cte_length > BT_HCI_LE_CTE_LEN_MAX) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	if (requested_cte_type != BT_HCI_LE_AOA_CTE &&
	    requested_cte_type != BT_HCI_LE_AOD_CTE_1US &&
	    requested_cte_type != BT_HCI_LE_AOD_CTE_2US) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	conn->llcp.cte_req.is_enabled = 1U;
	conn->llcp.cte_req.req_interval = cte_request_interval;
	conn->llcp.cte_req.cte_type = requested_cte_type;
	conn->llcp.cte_req.min_cte_len = requested_cte_length;

	return ull_cp_cte_req(conn, requested_cte_length, requested_cte_type);
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
/**
 * @brief Function enables or disables CTE response control procedure for a connection.
 *
 * @param handle Connection handle.
 * @param enable Enable or disable CTE response.
 *
 * @return HCI Status of command completion.
 */
uint8_t ll_df_set_conn_cte_rsp_enable(uint16_t handle, uint8_t enable)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (enable) {
		if (!conn->lll.df_tx_cfg.is_initialized) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

#if defined(CONFIG_BT_CTLR_PHY)
		/* CTE may not be send over CODED PHY */
		if (conn->lll.phy_tx == PHY_CODED) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
#endif /* CONFIG_BT_CTLR_PHY */
		conn->lll.df_tx_cfg.cte_rsp_en = 1U;

		ull_cp_cte_rsp_enable(conn, enable, LLL_DF_MAX_CTE_LEN,
				conn->lll.df_tx_cfg.cte_types_allowed);
	} else {
		conn->lll.df_tx_cfg.cte_rsp_en = false;

		if (conn->llcp.cte_rsp.is_active) {
			struct k_sem sem;

			k_sem_init(&sem, 0U, 1U);
			conn->llcp.cte_rsp.disable_param = &sem;
			conn->llcp.cte_rsp.disable_cb = df_conn_cte_req_disable;

			if (!conn->llcp.cte_rsp.is_active) {
				k_sem_take(&sem, K_FOREVER);
			}
		}
	}

	return BT_HCI_ERR_SUCCESS;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

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
void ll_df_read_ant_inf(uint8_t *switch_sample_rates, uint8_t *num_ant,
			uint8_t *max_switch_pattern_len, uint8_t *max_cte_len)
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

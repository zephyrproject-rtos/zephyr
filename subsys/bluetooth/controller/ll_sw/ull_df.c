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

#include "pdu.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll/lll_df_types.h"
#include "lll_df.h"

#include "ull_adv_types.h"
#include "ull_df.h"

#include "ull_adv_internal.h"

#include "ll.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_df
#include "common/log.h"
#include "hal/debug.h"

/* ToDo:
 * - Add release of df_adv_cfg when adv_sync is released.
 *   Open question, should df_adv_cfg be released when Adv. CTE is disabled?
 *   If yes that would mean, end user must always run ll_df_set_cl_cte_tx_params
 *   before consecutive Adv CTE enable.
 */

static struct lll_df_adv_cfg lll_df_adv_cfg_pool[CONFIG_BT_CTLR_ADV_AUX_SET];
static void *df_adv_cfg_free;

/* @brief Function performs common steps for initialization and reset
 * of Direction Finding ULL module.
 *
 * @return      Zero in case of success, other value in case of failure.
 */
static int init_reset(void);

/* @brief Function acquires memory for DF advertising configuration.
 *
 * The memory is acquired from private @ref lll_df_adv_cfg_pool memory store.
 *
 * @return Pointer to lll_df_adv_cfg or NULL if there is no more free memory.
 */
static struct lll_df_adv_cfg *df_adv_cfg_acquire(void);

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
	/* Initialize advertising DF memory configuration pool. */
	mem_init(lll_df_adv_cfg_pool, sizeof(struct lll_df_adv_cfg),
		 sizeof(lll_df_adv_cfg_pool) / sizeof(struct lll_df_adv_cfg),
		 &df_adv_cfg_free);

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

	sync = (void *)HDR_LLL2EVT(lll_sync);

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

		err = ull_adv_sync_pdu_set_clear(adv, 0,
						 ULL_ADV_PDU_HDR_FIELD_CTE_INFO,
						 NULL, &ter_idx);
		if (err) {
			return err;
		}

		if (sync->is_started) {
			/* If CTE is disabled when advertising is pending,
			 * decrease advertising event length
			 */
			ull_adv_sync_update(sync, 0, df_cfg->cte_length);
			/* ToDo decrease number of chain PDUs in pending
			 * advertising if there are added empty chain PDUs
			 * to sent requested number of CTEs in a chain
			 */
		}

		df_cfg->is_enabled = 0U;
	} else {
		struct pdu_cte_info cte_info;
		struct adv_pdu_field_data pdu_data;

		if (df_cfg->is_enabled) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		cte_info.type = df_cfg->cte_type;
		cte_info.time = df_cfg->cte_length;
		pdu_data.field_data = (uint8_t *)&cte_info;
		pdu_data.extra_data = df_cfg;
		err = ull_adv_sync_pdu_set_clear(adv,
						 ULL_ADV_PDU_HDR_FIELD_CTE_INFO,
						 0, &pdu_data, &ter_idx);
		if (err) {
			return err;
		}

		if (sync->is_started) {
			/* If CTE is enabled when advertising is pending,
			 * increase advertising event length
			 */
			ull_adv_sync_update(sync, df_cfg->cte_length, 0);
			/* ToDo increase number of chain PDUs in pending
			 * advertising if requested more CTEs than available
			 * PDU with advertising data.
			 */
		}

		df_cfg->is_enabled = 1U;
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

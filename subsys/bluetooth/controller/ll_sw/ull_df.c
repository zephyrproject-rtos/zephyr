/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr.h>
#include <sys/util.h>
#include <bluetooth/hci.h>

#include "hal/debug.h"
#include "hal/cpu.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"

#include "pdu.h"
#include "ll.h"
#include "lll.h"

#include "lll_adv.h"
#include "ull_adv_types.h"
#include "ull_adv_internal.h"

#include "ull_df.h"
#include "lll_df.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_df
#include "common/log.h"

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
static struct lll_df_adv_cfg *ull_df_adv_cfg_acquire(void);

/* @brief Function releases memory for DF advertising configuration.
 *
 * @param[in] df_adv_cfg        Pointer to memory to release
 */
static void ull_df_adv_cfg_release(struct ll_adv_aux_set *df_adv_cfg);

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
	struct ll_adv_set *adv;
	struct lll_adv_sync *sync;
	struct lll_df_adv_cfg *cfg;

	/* Get the advertising set instance */
	adv = ull_adv_is_created_get(adv_handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	sync = adv->lll.sync;
	if (!sync) {
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
	     num_ant_ids > CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	if (!sync->df_cfg) {
		sync->df_cfg = ull_df_adv_cfg_acquire();
	}

	cfg = sync->df_cfg;

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
 *        antennae switching and sampling related settings.
 *
 * @param[out]switch_sample_rates       Pointer to store available antennae
 *                                      switch-sampling configurations.
 * @param[out]num_ant                   Pointer to store number of available
 *                                      antennae.
 * @param[out]max_switch_pattern_len    Pointer to store maximum number of
 *                                      antennae switch patterns.
 * @param[out]max_cte_len               Pointer to store maximum length of CTE
 *                                      in [8us] units.
 */
void ll_df_read_ant_inf(uint8_t *switch_sample_rates,
			uint8_t *num_ant,
			uint8_t *max_switch_pattern_len,
			uint8_t *max_cte_len)
{
	/* Currently filled with data that inform about
	 * lack of antenna support for Direction Finding
	 */
	*switch_sample_rates = 0;
	*num_ant = 0;
	*max_switch_pattern_len = 0;
	*max_cte_len = 0;
}

static struct lll_df_adv_cfg *ull_df_adv_cfg_acquire(void)
{
	struct lll_df_adv_cfg *df_adv_cfg;

	df_adv_cfg = mem_acquire(&df_adv_cfg_free);
	if (!df_adv_cfg) {
		return NULL;
	}

	df_adv_cfg->is_enabled = false;

	return df_adv_cfg;
}

static void ull_df_adv_cfg_release(struct ll_adv_aux_set *df_adv_cfg)
{
	mem_release(df_adv_cfg, &df_adv_cfg_free);
}

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stddef.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <host/hci_core.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "hal/ccm.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_adv_sync.h"
#include "lll/lll_df_types.h"

#include "ull_adv_types.h"
#include "ull_adv_internal.h"

#include "ll.h"
#include "common.h"

#define PDU_PAULOAD_BUFF_SIZE 100
#define TEST_CTE_LENGTH 20

/* Controller code uses static function sync_acquire to get adv. sync.
 * For test purposes it is used as global variable to avoid complete
 * creation of advertising set.
 */
static struct ll_adv_sync_set g_sync_set;
static struct lll_df_adv_cfg g_df_cfg;

static void common_pdu_adv_data_set(struct pdu_adv *pdu, const uint8_t *data, uint8_t len);

/*
 * @brief Helper function to create advertising set.
 *
 * The function creates advertising set to an extent required to test adding CTE to periodic
 * advertising chains. The function returns handle to advertising set that may be used
 * in calls to ULL functions related with advertising.
 *
 * @param hci_handle equivalent of a handle received from HCI command.
 *
 * @return Handle to created advertising set.
 */
struct ll_adv_set *common_create_adv_set(uint8_t hci_handle)
{
	struct lll_adv_sync *lll_sync;
	struct ll_adv_set *adv_set;
	uint8_t handle;
	uint8_t err;

	zassert_true(hci_handle < BT_HCI_LE_ADV_HANDLE_MAX,
		     "Advertising set handle: %" PRIu8 " exceeds maximum handles value %" PRIu8,
		     hci_handle, BT_HCI_LE_ADV_HANDLE_MAX);
	err = ll_adv_set_by_hci_handle_get_or_new(hci_handle, &handle);
	zassert_equal(err, 0, "Unexpected error while create new advertising set, err: %" PRIu8,
		      err);

	adv_set = ull_adv_set_get(handle);
	zassert_not_null(adv_set, 0, "Unexpectedly advertising set is NULL");
	/* Note: there is single lll_adv_sync instance. If created more than one advertising set
	 * all will have reference to the same lll_adv_sync instance.
	 */
	lll_sync = &g_sync_set.lll;
	adv_set->lll.sync = &g_sync_set.lll;
	lll_hdr_init(&adv_set->lll, adv_set);
	g_sync_set.lll.adv = &adv_set->lll;
	lll_hdr_init(lll_sync, &g_sync_set);

	err = lll_adv_init();
	zassert_equal(err, 0, "Unexpected error while initialization advertising set, err: %d",
		      err);

	lll_hdr_init(lll_sync, &g_sync_set);

	lll_adv_data_reset(&lll_sync->data);
	err = lll_adv_data_init(&lll_sync->data);
	zassert_equal(err, 0,
		      "Unexpected error while initialization advertising data init, err: %d", err);

	adv_set->is_created = 1U;

	return adv_set;
}

/*
 * @brief Release advertising set.
 *
 * @param adv_set Pointer to advertising set to be released.
 */
void common_release_adv_set(struct ll_adv_set *adv_set)
{
	struct ll_adv_sync_set *sync;

	if (adv_set->lll.sync) {
		sync = HDR_LLL2ULL(adv_set->lll.sync);
		if (sync) {
			sync->is_started = 0U;
		}

		lll_adv_data_reset(&sync->lll.data);
	}
	adv_set->lll.sync = NULL;
	if (adv_set->df_cfg->is_enabled) {
		adv_set->df_cfg->is_enabled = 0U;
	}
	adv_set->df_cfg = NULL;
	adv_set->is_created = 0U;
}

/*
 * @brief Helper function that creates periodic advertising chain.
 *
 * The function creates periodic advertising chain with provided number of PDUs @p pdu_count.
 * The created chain is enqueued in provided advertising set. Number of requested PDUs includes
 * head of a chain AUX_SYNC_IND.
 * Each created PDU will hold payload data in following pattern:
 * "test%d test%d test%d", where '%d' is substituted by PDU index.
 *
 * @param adv_set   Pointer to advertising set to enqueue created chain.
 * @param pdu_count Requested number of PDUs in a chain.
 */
void common_create_per_adv_chain(struct ll_adv_set *adv_set, uint8_t pdu_count)
{
	uint8_t hdr_data[ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_AUX_PTR_PTR_SIZE];
	struct pdu_adv *pdu_prev, *pdu, *pdu_new;
	char pdu_buff[PDU_PAULOAD_BUFF_SIZE];
	void *extra_data_prev, *extra_data;
	struct lll_adv_sync *lll_sync;
	bool adi_in_sync_ind;
	uint8_t err, pdu_idx;

	lll_sync = adv_set->lll.sync;
	pdu = lll_adv_sync_data_peek(lll_sync, NULL);
	ull_adv_sync_pdu_init(pdu, 0U, 0U, 0U, NULL);

	err = ull_adv_sync_pdu_alloc(adv_set, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST, &pdu_prev,
				     &pdu, &extra_data_prev, &extra_data, &pdu_idx);
	zassert_equal(err, 0, "Unexpected error while PDU allocation, err: %d", err);

	if (extra_data) {
		ull_adv_sync_extra_data_set_clear(extra_data_prev, extra_data, 0, 0, NULL);
	}

	/* Create AUX_SYNC_IND PDU as a head of chain */
	err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
					 (pdu_count > 1 ? ULL_ADV_PDU_HDR_FIELD_AUX_PTR :
								ULL_ADV_PDU_HDR_FIELD_NONE),
					 ULL_ADV_PDU_HDR_FIELD_NONE, hdr_data);
	zassert_equal(err, 0, "Unexpected error during initialization of extended PDU, err: %d",
		      err);

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT)) {
		adi_in_sync_ind = ull_adv_sync_pdu_had_adi(pdu);
	}

	/* Add some AD for testing */
	snprintf(pdu_buff, ARRAY_SIZE(pdu_buff), "test%" PRIu8 " test%" PRIu8 " test%" PRIu8 "", 0,
		 0, 0);
	common_pdu_adv_data_set(pdu, pdu_buff, strlen(pdu_buff));
	/* Create AUX_CHAIN_IND PDUs. Start from 1, AUX_SYNC_IND is first PDU. */
	for (uint8_t idx = 1; idx < pdu_count; ++idx) {
		snprintf(pdu_buff, ARRAY_SIZE(pdu_buff),
			 "test%" PRIu8 " test%" PRIu8 " test%" PRIu8 "", idx, idx, idx);
		/* Allocate new PDU */
		pdu_new = lll_adv_pdu_alloc_pdu_adv();
		zassert_not_null(pdu_new, "Cannot allocate new PDU.");
		/* Initialize new empty PDU. Last AUX_CHAIN_IND may not include AuxPtr. */
		if (idx < pdu_count - 1) {
			if (IS_ENABLED(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT) &&
			    adi_in_sync_ind) {
				ull_adv_sync_pdu_init(pdu_new,
						ULL_ADV_PDU_HDR_FIELD_AUX_PTR |
						ULL_ADV_PDU_HDR_FIELD_ADI,
						lll_sync->adv->phy_s,
						lll_sync->adv->phy_flags, NULL);
			} else {
				ull_adv_sync_pdu_init(pdu_new,
						ULL_ADV_PDU_HDR_FIELD_AUX_PTR,
						lll_sync->adv->phy_s,
						lll_sync->adv->phy_flags, NULL);
			}
		} else {
			if (IS_ENABLED(CONFIG_BT_CTLR_ADV_PERIODIC_ADI_SUPPORT) &&
			    adi_in_sync_ind) {
				ull_adv_sync_pdu_init(pdu_new,
						ULL_ADV_PDU_HDR_FIELD_ADI,
						0U, 0U, NULL);
			} else {
				ull_adv_sync_pdu_init(pdu_new,
						ULL_ADV_PDU_HDR_FIELD_NONE,
						0U, 0U, NULL);
			}
		}
		/* Add some AD for testing */
		common_pdu_adv_data_set(pdu_new, pdu_buff, strlen(pdu_buff));
		/* Link to previous PDU */
		lll_adv_pdu_linked_append(pdu_new, pdu);
		pdu = pdu_new;
	}

	lll_adv_sync_data_enqueue(lll_sync, pdu_idx);
}

/*
 * @brief Helper function to release periodic advertising chain that was enqueued for
 * advertising set.
 *
 * @param adv_set Pointer to advertising set to release a PDUs chain.
 */
void common_release_per_adv_chain(struct ll_adv_set *adv_set)
{
	struct lll_adv_sync *lll_sync;
	struct pdu_adv *pdu;

	lll_sync = adv_set->lll.sync;
	pdu = lll_adv_sync_data_peek(lll_sync, NULL);
	if (pdu != NULL) {
		lll_adv_pdu_linked_release_all(pdu);
	}

	pdu = (void *)lll_sync->data.pdu[lll_sync->data.first];
	if (pdu != NULL) {
		lll_adv_pdu_linked_release_all(pdu);
	}
}

/*
 * @brief Helper function that validates content of periodic advertising PDU.
 *
 * The function verifies if content of periodic advertising PDU as as expected. The function
 * verifies two types of PDUs: AUX_SYNC_IND and AUX_CHAIN_IND. AUX_CHAIN_IND is validated
 * as if its superior PDU is AUX_SYNC_IND only.
 *
 * Expected fields in extended advertising header are provided by @p exp_ext_hdr_flags.
 *
 * Note: The function expects that there is no ACAD data in the PDU.
 *
 * @param pdu Pointer to PDU to be verified.
 * @param type Type of the PDU.
 * @param exp_ext_hdr_flags Bitfield with expected extended header flags.
 */
void common_validate_per_adv_pdu(struct pdu_adv *pdu, enum test_pdu_ext_adv_type type,
				 uint16_t exp_ext_hdr_flags)
{
	struct pdu_adv_com_ext_adv *com_hdr;
	struct pdu_adv_ext_hdr *ext_hdr;
	uint8_t ext_hdr_len;
	uint8_t *dptr;

	if (pdu->len > 1) {
		com_hdr = &pdu->adv_ext_ind;
		/* Non-connectable and Non-scannable adv mode */
		zassert_equal(com_hdr->adv_mode, 0U,
			      "Unexpected mode of extended advertising PDU: %" PRIu8,
			      com_hdr->adv_mode);

		ext_hdr = &com_hdr->ext_hdr;
		dptr = ext_hdr->data;

		if (com_hdr->ext_hdr_len > 0) {
			zassert_false(ext_hdr->adv_addr,
				      "Unexpected AdvA field in extended advertising header");
			zassert_false(ext_hdr->tgt_addr,
				      "Unexpected TargetA field in extended advertising header");
			if (exp_ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_CTE_INFO) {
				zassert_true(
					ext_hdr->cte_info,
					"Missing expected CteInfo field in extended advertising header");
				dptr += sizeof(struct pdu_cte_info);
			} else {
				zassert_false(
					ext_hdr->cte_info,
					"Unexpected CteInfo field in extended advertising header");
			}
			if (exp_ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_ADI) {
				zassert_true(
					ext_hdr->adi,
					"Missing expected ADI field in extended advertising header");
				dptr += sizeof(struct pdu_adv_adi);
			} else {
				zassert_false(
					ext_hdr->adi,
					"Unexpected ADI field in extended advertising header");
			}
			if (exp_ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_AUX_PTR) {
				zassert_true(
					ext_hdr->aux_ptr,
					"Missing expected AuxPtr field in extended advertising header");
				dptr += sizeof(struct pdu_adv_aux_ptr);
			} else {
				zassert_false(
					ext_hdr->aux_ptr,
					"Unexpected AuxPtr field in extended advertising header");
			}
			zassert_false(ext_hdr->sync_info,
				      "Unexpected SyncInfo field in extended advertising header");
			if (exp_ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_TX_POWER) {
				zassert_true(
					ext_hdr->tx_pwr,
					"Missing expected CteInfo field in extended advertising header");
				dptr += sizeof(uint8_t);
			} else {
				zassert_false(
					ext_hdr->tx_pwr,
					"Unexpected CteInfo field in extended advertising header");
			}

			/* Calculate extended header len, ACAD should not be available.
			 * ull_adv_aux_hdr_len_calc returns ext hdr length without it.
			 */
			ext_hdr_len = ull_adv_aux_hdr_len_calc(com_hdr, &dptr);
			zassert_equal(com_hdr->ext_hdr_len,
				      ext_hdr_len - PDU_AC_EXT_HEADER_SIZE_MIN,
				      "Extended header length: %" PRIu8
				      " different than expected %" PRIu8,
				      ext_hdr_len, com_hdr->ext_hdr_len);

			if (exp_ext_hdr_flags & ULL_ADV_PDU_HDR_FIELD_AD_DATA) {
				zassert_true((pdu->len - ext_hdr_len) > 0,
					     "Missing expected advertising data in PDU");
			} else {
				zassert_equal(pdu->len - ext_hdr_len, 0,
					      "Unexpected advertising data in PDU");
			}
		} else {
			zassert_equal(exp_ext_hdr_flags, ULL_ADV_PDU_HDR_FIELD_AD_DATA,
				      "Unexpected extended header flags: %" PRIu16,
				      exp_ext_hdr_flags);
		}
	}
}

/*
 * @brief Helper function to prepare CTE configuration for a given advertising set.
 *
 * Note: There is a single instance of CTE configuration. In case there is a need
 * to use multiple advertising sets at once, all will use the same CTE configuration.
 *
 * @param adv       Pointer to advertising set
 * @param cte_count Requested number of CTEs to be send
 */
void common_prepare_df_cfg(struct ll_adv_set *adv, uint8_t cte_count)
{
	/* Prepare CTE configuration */
	g_df_cfg.cte_count = cte_count;
	g_df_cfg.cte_length = TEST_CTE_LENGTH;
	g_df_cfg.cte_type = BT_HCI_LE_AOD_CTE_2US;

	adv->df_cfg = &g_df_cfg;
}

/*
 * @brief Helper function that validates correctness of periodic advertising chain.
 *
 * The function expects that all periodic advertising chain PDUs will have advertising data.
 *
 * @param adv       Pointer to advertising set
 * @param pdu_count Number of PDUs in a chain
 */
void common_validate_per_adv_chain(struct ll_adv_set *adv, uint8_t pdu_count)
{
	uint16_t ext_hdr_flags;
	struct pdu_adv *pdu;

	pdu = lll_adv_sync_data_peek(adv->lll.sync, NULL);

	/* Validate AUX_SYNC_IND */
	if (pdu_count > 1) {
		ext_hdr_flags = ULL_ADV_PDU_HDR_FIELD_AUX_PTR | ULL_ADV_PDU_HDR_FIELD_AD_DATA;
	} else {
		ext_hdr_flags = ULL_ADV_PDU_HDR_FIELD_AD_DATA;
	}

	common_validate_per_adv_pdu(pdu, TEST_PDU_EXT_ADV_SYNC_IND, ext_hdr_flags);
	pdu = lll_adv_pdu_linked_next_get(pdu);
	if (pdu_count > 1) {
		zassert_not_null(pdu, "Expected PDU in periodic advertising chain is NULL");
	} else {
		zassert_is_null(pdu, "Unexpected PDU in a single PDU periodic advertising chain");
	}
	/* Validate AUX_CHAIN_IND PDUs in a periodic advertising chain. Start from 1, because
	 * first PDU is AUX_SYNC_IND.
	 */
	for (uint8_t idx = 1; idx < pdu_count; ++idx) {
		if (idx != (pdu_count - 1)) {
			ext_hdr_flags =
				ULL_ADV_PDU_HDR_FIELD_AUX_PTR | ULL_ADV_PDU_HDR_FIELD_AD_DATA;
		} else {
			ext_hdr_flags = ULL_ADV_PDU_HDR_FIELD_AD_DATA;
		}

		common_validate_per_adv_pdu(pdu, TEST_PDU_EXT_ADV_CHAIN_IND, ext_hdr_flags);
		pdu = lll_adv_pdu_linked_next_get(pdu);
		if (idx != (pdu_count - 1)) {
			zassert_not_null(pdu, "Expected PDU in periodic advertising chain is NULL");
		} else {
			zassert_is_null(pdu, "Unexpected PDU at end of periodic advertising chain");
		}
	}
}

/*
 * @brief Helper function that validates correctness of periodic advertising chain including CTE
 *
 * The number of PDUs including advertising data or CTE is provided by appropriate function
 * arguments. PUDs including CTE are always first #N PDUs. The same rule applies to PDUs including
 * advertising data. So maximum number of PDUs in a chain is maximum value from pair @p cte_count
 * and @p ad_data_count.
 *
 * @param adv               Pointer to advertising set
 * @param cte_count         Number of PDUs including CTE
 * @param ad_data_pdu_count Number of PDUs including advertising data
 */
void common_validate_chain_with_cte(struct ll_adv_set *adv, uint8_t cte_count,
				    uint8_t ad_data_pdu_count)
{
	uint16_t ext_hdr_flags;
	struct pdu_adv *pdu;
	uint8_t pdu_count;

	pdu = lll_adv_sync_data_peek(adv->lll.sync, NULL);
	if (cte_count > 1) {
		ext_hdr_flags = ULL_ADV_PDU_HDR_FIELD_AUX_PTR | ULL_ADV_PDU_HDR_FIELD_CTE_INFO;

	} else {
		ext_hdr_flags = ULL_ADV_PDU_HDR_FIELD_CTE_INFO;
	}
	if (ad_data_pdu_count > 0) {
		ext_hdr_flags |= ULL_ADV_PDU_HDR_FIELD_AD_DATA;
	}

	common_validate_per_adv_pdu(pdu, TEST_PDU_EXT_ADV_SYNC_IND, ext_hdr_flags);

	pdu_count = MAX(cte_count, ad_data_pdu_count);

	pdu = lll_adv_pdu_linked_next_get(pdu);
	if (pdu_count > 1) {
		zassert_not_null(pdu, "Expected PDU in periodic advertising chain is NULL");
	} else {
		zassert_is_null(pdu, "Unexpected PDU in a single PDU periodic advertising chain");
	}

	for (uint8_t idx = 1; idx < pdu_count; ++idx) {
		if (idx < pdu_count - 1) {
			ext_hdr_flags = ULL_ADV_PDU_HDR_FIELD_AUX_PTR;
		} else {
			ext_hdr_flags = 0U;
		}
		if (idx < cte_count) {
			ext_hdr_flags |= ULL_ADV_PDU_HDR_FIELD_CTE_INFO;
		}
		if (idx < ad_data_pdu_count) {
			ext_hdr_flags |= ULL_ADV_PDU_HDR_FIELD_AD_DATA;
		}

		common_validate_per_adv_pdu(pdu, TEST_PDU_EXT_ADV_CHAIN_IND, ext_hdr_flags);

		pdu = lll_adv_pdu_linked_next_get(pdu);
		if (idx < (pdu_count - 1)) {
			zassert_not_null(pdu, "Expected PDU in periodic advertising chain is NULL");
		} else {
			zassert_is_null(pdu, "Unexpected PDU at end of periodic advertising chain");
		}
	}
}

/*
 * @brief Helper function to cleanup after test case end.
 *
 * @param adv               Pointer to advertising set
 */
void common_teardown(struct ll_adv_set *adv)
{
	common_release_per_adv_chain(adv);
	common_release_adv_set(adv);
	lll_adv_init();
}
/*
 * @brief Helper function to add payload data to extended advertising PDU.
 *
 * @param pdu Pointer to extended advertising PDU.
 * @param data Pointer to data to be added.
 * @param len  Length of the data.
 */
static void common_pdu_adv_data_set(struct pdu_adv *pdu, const uint8_t *data, uint8_t len)
{
	struct pdu_adv_com_ext_adv *com_hdr;
	uint8_t len_max;
	uint8_t *dptr;

	com_hdr = &pdu->adv_ext_ind;

	dptr = &com_hdr->ext_hdr_adv_data[com_hdr->ext_hdr_len];

	len_max = PDU_AC_PAYLOAD_SIZE_MAX - (dptr - pdu->payload);
	zassert_false(len > len_max,
		      "Provided data length exceeds maximum supported payload length: %" PRIu8,
		      len_max);

	memcpy(dptr, data, len);
	dptr += len;

	pdu->len = dptr - pdu->payload;
}

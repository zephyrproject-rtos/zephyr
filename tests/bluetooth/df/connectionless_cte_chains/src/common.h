/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum test_pdu_ext_adv_type {
	TEST_PDU_EXT_ADV_SYNC_IND,
	TEST_PDU_EXT_ADV_CHAIN_IND,
};

struct ll_adv_set *common_create_adv_set(uint8_t hci_handle);
void common_release_adv_set(struct ll_adv_set *adv_set);
void common_create_per_adv_chain(struct ll_adv_set *adv_set, uint8_t pdu_count);
void common_validate_per_adv_pdu(struct pdu_adv *pdu, enum test_pdu_ext_adv_type type,
				 uint16_t exp_ext_hrd_flags);
void common_release_per_adv_chain(struct ll_adv_set *adv_set);
void common_prepare_df_cfg(struct ll_adv_set *adv, uint8_t cte_count);
void common_validate_per_adv_chain(struct ll_adv_set *adv, uint8_t pdu_count);
void common_validate_chain_with_cte(struct ll_adv_set *adv, uint8_t cte_count,
				    uint8_t ad_data_pdu_count);
void common_teardown(struct ll_adv_set *adv);

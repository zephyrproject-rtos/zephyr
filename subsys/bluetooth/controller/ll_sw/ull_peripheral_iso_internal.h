/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Helper functions to initialize and reset ull_peripheral_iso module */
int ull_peripheral_iso_init(void);
int ull_peripheral_iso_reset(void);

uint8_t ull_peripheral_iso_acquire(struct ll_conn *acl,
				   struct pdu_data_llctrl_cis_req *req,
				   uint16_t *cis_handle);
uint8_t ull_peripheral_iso_setup(struct pdu_data_llctrl_cis_ind *ind,
				 uint8_t cig_id,
				 uint16_t cis_handle);
void ull_peripheral_iso_start(struct ll_conn *acl, uint32_t ticks_at_expire,
			      uint16_t cis_handle);

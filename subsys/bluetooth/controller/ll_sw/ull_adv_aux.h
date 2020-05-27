/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

uint8_t ll_adv_aux_random_addr_set(uint8_t handle, uint8_t *addr);
uint8_t *ll_adv_aux_random_addr_get(uint8_t handle, uint8_t *addr);
uint8_t ll_adv_aux_ad_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref, uint8_t len,
			    uint8_t *data);
uint8_t ll_adv_aux_sr_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref, uint8_t len,
			    uint8_t *data);
uint16_t ll_adv_aux_max_data_length_get(void);
uint8_t ll_adv_aux_set_count_get(void);
uint8_t ll_adv_aux_set_remove(uint8_t handle);
uint8_t ll_adv_aux_set_clear(void);

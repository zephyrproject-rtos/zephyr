/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

u8_t ll_adv_aux_random_addr_set(u8_t handle, u8_t *addr);
u8_t *ll_adv_aux_random_addr_get(u8_t handle, u8_t *addr);
u8_t ll_adv_aux_ad_data_set(u8_t handle, u8_t op, u8_t frag_pref, u8_t len,
			    u8_t *data);
u8_t ll_adv_aux_sr_data_set(u8_t handle, u8_t op, u8_t frag_pref, u8_t len,
			    u8_t *data);
u16_t ll_adv_aux_max_data_length_get(void);
u8_t ll_adv_aux_set_count_get(void);
u8_t ll_adv_aux_set_remove(u8_t handle);
u8_t ll_adv_aux_set_clear(void);

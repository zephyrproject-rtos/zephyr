/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Helper functions to initialize and reset ull_central_iso module */
int ull_central_iso_init(void);
int ull_central_iso_reset(void);

void ull_central_iso_start(struct ll_conn *acl, uint32_t ticks_at_expire, uint16_t cis_handle);
uint16_t ull_central_iso_cis_offset_get(struct ll_conn_iso_stream *cis, uint32_t *cis_offset_min,
					uint32_t *cis_offset_max);
uint8_t ull_central_iso_setup(uint16_t cis_handle,
			      uint32_t *cig_sync_delay,
			      uint32_t *cis_sync_delay,
			      uint32_t *cis_offset_min,
			      uint32_t *cis_offset_max,
			      uint16_t *conn_event_count,
			      uint8_t  *access_addr);

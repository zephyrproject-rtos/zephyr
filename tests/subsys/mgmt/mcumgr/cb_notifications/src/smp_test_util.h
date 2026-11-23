/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SMP_TEST_UTIL_
#define H_SMP_TEST_UTIL_

#include <zephyr/ztest.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zcbor_common.h>
#include <smp_internal.h>

/* SMP header function for generating MCUmgr command header with sequence number set to 1 */
void smp_make_hdr(struct smp_hdr *rsp_hdr, size_t len);

/* Function for creating an os_mgmt echo command */
bool create_mcumgr_format_packet(zcbor_state_t *zse, uint8_t *buffer, uint8_t *output_buffer,
				 uint16_t *buffer_size);


#endif

/*
 * Copyright (c) 2017-2023 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FCB_TEST_H
#define _FCB_TEST_H

#include <stdio.h>
#include <string.h>
#include <zephyr/ztest.h>

#include <zephyr/fs/fcb.h>
#include "fcb_priv.h"
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TEST_FCB_FLASH_AREA		slot1_partition
#define TEST_FCB_FLASH_AREA_ID		FIXED_PARTITION_ID(TEST_FCB_FLASH_AREA)

extern struct fcb test_fcb;
#if defined(CONFIG_FCB_ALLOW_FIXED_ENDMARKER)
extern struct fcb test_fcb_crc_disabled;
#endif

extern struct flash_sector test_fcb_sector[];

extern uint8_t fcb_test_erase_value;

struct append_arg {
	int *elem_cnts;
};

void test_fcb_wipe(void);
int fcb_test_empty_walk_cb(struct fcb_entry_ctx *entry_ctx, void *arg);
uint8_t fcb_test_append_data(int msg_len, int off);
int fcb_test_data_walk_cb(struct fcb_entry_ctx *entry_ctx, void *arg);
int fcb_test_cnt_elems_cb(struct fcb_entry_ctx *entry_ctx, void *arg);

#ifdef __cplusplus
}
#endif
#endif /* _FCB_TEST_H */

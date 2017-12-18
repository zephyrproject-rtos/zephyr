/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FCB_TEST_H
#define _FCB_TEST_H

#include <stdio.h>
#include <string.h>
#include <ztest.h>

#include "fcb.h"
#include "fcb_priv.h"

#ifdef __cplusplus
#extern "C" {
#endif

extern struct fcb test_fcb;

extern struct flash_area test_fcb_area[];

struct append_arg {
	int *elem_cnts;
};

void fcb_test_wipe(void);
int fcb_test_empty_walk_cb(struct fcb_entry *loc, void *arg);
uint8_t fcb_test_append_data(int msg_len, int off);
int fcb_test_data_walk_cb(struct fcb_entry *loc, void *arg);
int fcb_test_cnt_elems_cb(struct fcb_entry *loc, void *arg);

#ifdef __cplusplus
}
#endif
#endif /* _FCB_TEST_H */

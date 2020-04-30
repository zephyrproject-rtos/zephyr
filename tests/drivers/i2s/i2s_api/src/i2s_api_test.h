/*
 * Copyright (c) 2017 comsuisse AG
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef I2S_API_TEST_H
#define I2S_API_TEST_H

#include <kernel.h>

void test_i2s_tx_transfer_configure_0(void);
void test_i2s_rx_transfer_configure_0(void);
void test_i2s_transfer_short(void);
void test_i2s_transfer_long(void);
void test_i2s_rx_sync_start(void);
void test_i2s_rx_empty_timeout(void);
void test_i2s_transfer_restart(void);
void test_i2s_transfer_rx_overrun(void);
void test_i2s_transfer_tx_underrun(void);

void test_i2s_tx_transfer_configure_1(void);
void test_i2s_rx_transfer_configure_1(void);
void test_i2s_state_not_ready_neg(void);
void test_i2s_state_ready_neg(void);
void test_i2s_state_running_neg(void);
void test_i2s_state_stopping_neg(void);
void test_i2s_state_error_neg(void);

extern struct k_mem_slab rx_0_mem_slab;
extern struct k_mem_slab tx_0_mem_slab;
extern struct k_mem_slab rx_1_mem_slab;
extern struct k_mem_slab tx_1_mem_slab;

#define SAMPLE_NO	32
#define TIMEOUT		2000
#define FRAME_CLK_FREQ	8000

extern int16_t data_l[SAMPLE_NO];
extern int16_t data_r[SAMPLE_NO];

#define I2S_DEV_NAME "I2S_0"
#define BLOCK_SIZE (2 * sizeof(data_l))

int rx_block_read_slab(const struct device *dev_i2s, int att,
		       struct k_mem_slab *slab);
int tx_block_write_slab(const struct device *dev_i2s, int att, int err,
			struct k_mem_slab *slab);

void fill_buf_const(int16_t *tx_block, int16_t val_l, int16_t val_r);
int verify_buf_const(int16_t *rx_block, int16_t val_l, int16_t val_r);

#endif

/*
 * Copyright (c) 2017 comsuisse AG
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef I2S_API_TEST_H
#define I2S_API_TEST_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>

extern struct k_mem_slab rx_mem_slab;
extern struct k_mem_slab tx_mem_slab;

#define SAMPLE_NO	32
#define TIMEOUT		2000
#define FRAME_CLK_FREQ	8000

extern int16_t data_l[SAMPLE_NO];
extern int16_t data_r[SAMPLE_NO];

extern const struct device *dev_i2s_rx;
extern const struct device *dev_i2s_tx;
extern const struct device *dev_i2s;
extern bool dir_both_supported;

#define I2S_DEV_NODE_RX DT_ALIAS(i2s_node0)
#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
#define I2S_DEV_NODE_TX DT_ALIAS(i2s_node1)
#else
#define I2S_DEV_NODE_TX DT_ALIAS(i2s_node0)
#endif
#define BLOCK_SIZE (2 * sizeof(data_l))

#define NUM_RX_BLOCKS	4
#define NUM_TX_BLOCKS	4

int tx_block_write(const struct device *dev_i2s, int att, int err);
int rx_block_read(const struct device *dev_i2s, int att);

void fill_buf_const(int16_t *tx_block, int16_t val_l, int16_t val_r);
int verify_buf_const(int16_t *rx_block, int16_t val_l, int16_t val_r);

int configure_stream(const struct device *dev_i2s, enum i2s_dir dir);

#endif

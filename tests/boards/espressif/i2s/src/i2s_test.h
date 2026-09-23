/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef I2S_TEST_H_
#define I2S_TEST_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>

#define VAL_L 11
#define VAL_R 22

extern const struct device *dev_i2s_rx;
extern const struct device *dev_i2s_tx;
extern const struct device *dev_i2s;

extern struct k_mem_slab rx_mem_slab;
extern struct k_mem_slab tx_mem_slab;

int configure_stream(const struct device *i2s_dev, enum i2s_dir dir);
int tx_block_write(const struct device *i2s_dev, int16_t val_l, int16_t val_r, int err);
int rx_block_read(const struct device *i2s_dev, int16_t val_l, int16_t val_r);
void verify_buf_new_stream(void);
void i2s_test_recover(const struct device *dev);

#endif /* I2S_TEST_H_ */

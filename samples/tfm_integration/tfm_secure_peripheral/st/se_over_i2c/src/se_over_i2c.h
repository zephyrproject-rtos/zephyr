/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ST_TFM_SE_PARTITION_SE_PARTITION_H
#define ST_TFM_SE_PARTITION_SE_PARTITION_H

#include <psa/error.h>
#include <stdint.h>

struct i2c_probe_result {
	uint8_t hal_status;
	uint8_t isr_lo;
	uint8_t isr_hi;
};

psa_status_t se_i2c_probe(struct i2c_probe_result *out);
psa_status_t se_i2c_echo(const uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf, size_t rx_len);

#endif /* ST_TFM_SE_PARTITION_SE_PARTITION_H */

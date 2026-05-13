/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2S_I2S_NATIVE_SIM_BOTTOM_H_
#define ZEPHYR_DRIVERS_I2S_I2S_NATIVE_SIM_BOTTOM_H_

#ifdef __cplusplus
extern "C" {
#endif

int ns_i2s_open_rx_file_bottom(const char *pathname);
int ns_i2s_open_tx_file_bottom(const char *pathname);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2S_I2S_NATIVE_SIM_BOTTOM_H_ */

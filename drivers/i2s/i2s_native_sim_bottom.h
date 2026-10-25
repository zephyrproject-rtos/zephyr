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

int ns_i2s_open_bottom(const char *pathname, int rx, unsigned int rate, unsigned int frame_bytes,
		       unsigned int word_size);
long ns_i2s_write_bottom(int idx, const void *buf, long len);
long ns_i2s_read_bottom(int idx, void *buf, long len);
void ns_i2s_close_bottom(int idx);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2S_I2S_NATIVE_SIM_BOTTOM_H_ */

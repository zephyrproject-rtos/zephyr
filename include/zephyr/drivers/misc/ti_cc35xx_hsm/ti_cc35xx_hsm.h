/*
 * Copyright (c) 2026 Conclusive Engineering Sp. z o.o.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEVICE_TI_CC35XX_HSM_H_
#define DEVICE_TI_CC35XX_HSM_H_

#include <zephyr/device.h>

#include <zephyr/crypto/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HSM_AES_BLOCK_SIZE 16U

struct hsm_ti_cc35xx_driver_session {
	union {
		uint8_t key[HSM_AES_BLOCK_SIZE];
		uint8_t ctr[HSM_AES_BLOCK_SIZE];
	};
	uint8_t *nonce;
};

__subsystem struct hsm_ti_cc35xx_driver_api {
	int (*get_entropy)(const struct device *dev, uint8_t *buf, uint16_t len);
	int (*do_crypto)(const struct device *dev, enum cipher_algo algo, enum cipher_mode mode,
			 enum cipher_op op_type, struct cipher_ctx *ctx, void *pkt);
	int (*get_hw_caps)(const struct device *dev);
};

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_TI_CC35XX_HSM_H_ */

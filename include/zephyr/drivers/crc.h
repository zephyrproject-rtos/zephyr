/*
 * Copyright (c) 2024 Brill Power Ltd.
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CRC public API header file.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CRC_H
#define ZEPHYR_INCLUDE_DRIVERS_CRC_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CRC driver APIs
 * @defgroup crc_interface CRC driver APIs
 * @ingroup io_interfaces
 * @{
 */
#define CRC_FLAG_REVERSE_INPUT     BIT(0)
#define CRC_FLAG_REVERSE_OUTPUT    BIT(1)
#define CRC_FLAG_NO_REVERSE_INPUT  BIT(2)
#define CRC_FLAG_NO_REVERSE_OUTPUT BIT(3)

#define CRC4_INIT_VAL        0x0
#define CRC4_TI_INIT_VAL     0x0
#define CRC7_BE_INIT_VAL     0x0
#define CRC8_INIT_VAL        0x0
#define CRC8_CCITT_INIT_VAL  0xFF
#define CRC8_ROHC_INIT_VAL   0xFF
#define CRC16_INIT_VAL       0x0
#define CRC16_ANSI_INIT_VAL  0x0
#define CRC16_CCITT_INIT_VAL 0x0000
#define CRC16_ITU_T_INIT_VAL 0x0000
#define CRC24_PGP_INIT_VALUE 0x00B704CEU
#define CRC32_IEEE_INIT_VAL  0xFFFFFFFFU
#define CRC32_C_INIT_VAL     0xFFFFFFFFU

#define CRC32C_XOR_OUT 0xFFFFFFFFUL

/* Define polynomial for CRC */
#define CRC4_POLY          0x3
#define CRC4_REFLECT_POLY  0xC
#define CRC7_BE_POLY       0x09
#define CRC8_POLY          0x07
#define CRC8_REFLECT_POLY  0xE0
#define CRC16_POLY         0x8005
#define CRC16_REFLECT_POLY 0xA001
#define CRC16_CCITT_POLY   0x1021
#define CRC24_PGP_POLY     0x01864CFBU
#define CRC32_IEEE_POLY    0x04C11DB7U
#define CRC32C_POLY        0x1EDC6F41U
#define CRC32K_4_2_POLY    0x93A409EBU

/**
 * @brief CRC state enumeration
 */

enum crc_state {
	CRC_STATE_IDLE = 0,
	CRC_STATE_IN_PROGRESS
};

typedef uint32_t crc_init_val_t;
typedef uint32_t crc_poly_t;
typedef uint32_t crc_result_t;

/**
 * @brief CRC context structure
 */

struct crc_ctx {
	enum crc_type type;
	enum crc_state state;
	uint32_t reversed;
	crc_poly_t polynomial;
	crc_init_val_t seed;
	crc_result_t result;
};

typedef int (*crc_api_begin)(const struct device *dev, struct crc_ctx *ctx);
typedef int (*crc_api_update)(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			      size_t bufsize);
typedef int (*crc_api_finish)(const struct device *dev, struct crc_ctx *ctx);

/**
 * @brief CRC driver API structure
 */

__subsystem struct crc_driver_api {
	crc_api_begin begin;
	crc_api_update update;
	crc_api_finish finish;
};

/**
 * @brief  Configure CRC unit for calculation
 * @param dev Pointer to the device structure
 * @param ctx Pointer to the CRC context structure
 * @retval 0 if successful, negative errno code on failure
 */
__syscall int crc_begin(const struct device *dev, struct crc_ctx *ctx);
static inline int z_impl_crc_begin(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	return api->begin(dev, ctx);
}

/**
 * @brief Perform CRC calculation on the provided data buffer and retrieve
 *			result
 * @param dev Pointer to the device structure
 * @param ctx Pointer to the CRC context structure
 * @param buffer Pointer to input data buffer
 * @param bufsize Number of bytes in *buffer
 * @retval 0 if successful, negative errno code on failure
 */
__syscall int crc_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			 size_t bufsize);
static inline int z_impl_crc_update(const struct device *dev, struct crc_ctx *ctx,
				    const void *buffer, size_t bufsize)
{
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	return api->update(dev, ctx, buffer, bufsize);
}

/**
 * @brief  Finalize CRC calculation
 * @param dev Pointer to the device structure
 * @param ctx Pointer to the CRC context structure
 * @retval 0 if successful, negative errno code on failure
 */
__syscall int crc_finish(const struct device *dev, struct crc_ctx *ctx);
static inline int z_impl_crc_finish(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	return api->finish(dev, ctx);
}

static inline int crc_verify(struct crc_ctx *ctx, crc_result_t expected)
{
	if (ctx->state == CRC_STATE_IN_PROGRESS) {
		return -EBUSY;
	}

	crc_result_t res = ctx->result;

	if (expected != res) {
		return -EINVAL;
	}
	return 0;
}
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/crc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CRC_H */

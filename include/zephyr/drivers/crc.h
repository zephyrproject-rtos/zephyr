/*
 * Copyright (c) 2024 Brill Power Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CRC_H
#define ZEPHYR_INCLUDE_DRIVERS_CRC_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CRC driver APIs
 * @defgroup crc_interface CRC driver APIs
 * @ingroup io_interfaces
 * @{
 */

#define CRC_FLAG_REVERSE_INPUT  BIT(0)
#define CRC_FLAG_REVERSE_OUTPUT BIT(1)

#define CRC4_INIT_VAL        0x0
#define CRC4_TI_INIT_VAL     0x0
#define CRC7_BE_INIT_VAL     0x0
#define CRC8_INIT_VAL        0x0
#define CRC8_CCITT_INIT_VAL  0x00
#define CRC16_INIT_VAL       0x0
#define CRC16_ANSI_INIT_VAL  0x0
#define CRC16_CCITT_INIT_VAL 0x0000
#define CRC16_ITU_T_INIT_VAL 0x0000
#define CRC24_PGP_INIT_VAL   0x0
#define CRC32_C_INIT_VAL     0x00000000
#define CRC32_IEEE_INIT_VAL  0xFFFFFFFFU

#define CRC8_CCITT_POLY 0x07
#define CRC32_IEEE_POLY 0x04C11DB7U

/**
 * @brief CRC algorithm enumeration
 */
enum crc_type_hw {
	CRC4_HW,
	CRC4_TI_HW,
	CRC7_BE_HW,
	CRC8_HW,
	CRC8_CCITT_HW,
	CRC16_HW,
	CRC16_ANSI_HW,
	CRC16_CCITT_HW,
	CRC16_ITU_T_HW,
	CRC24_PGP_HW,
	CRC32_C_HW,
	CRC32_IEEE_HW,
};

/**
 * @brief CRC state enumeration
 */
enum crc_state {
	CRC_STATE_IDLE,
	CRC_STATE_IN_PROGRESS
};

typedef uint64_t crc_init_val_t;
typedef uint64_t crc_polynomial_t;
typedef uint64_t crc_result_t;

/**
 * @brief CRC context structure
 */
struct crc_ctx {
	enum crc_type_hw type;        /**< Type of CRC algorithm */
	enum crc_state state;         /**< State of CRC calculation */
	crc_init_val_t initial_value; /**< Initial value for CRC calculation */
	uint32_t flags;               /**< Flags for CRC calculation */
	crc_polynomial_t polynomial;  /**< Polynomial used in CRC calculation */
	crc_result_t result;          /**< Store the CRC result */
};

typedef int (*crc_api_begin)(const struct device *dev, struct crc_ctx *ctx);
typedef int (*crc_api_update)(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			      size_t bufsize);
typedef int (*crc_api_finish)(const struct device *dev, struct crc_ctx *ctx);

/**
 * @brief CRC driver API structure
 */
__subsystem struct crc_driver_api {
	crc_api_begin crc_begin;
	crc_api_update crc_update;
	crc_api_finish crc_finish;
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

	return api->crc_begin(dev, ctx);
}

/**
 * @brief Perform CRC calculation on the provided data buffer and retrieve result
 * @param dev Pointer to the device structure
 * @param ctx Pointer to the CRC context structure
 * @param buffer Pointer to array to be CRCd
 * @param bufsize Number of bytes in *buffer
 * @retval 0 if successful, negative errno code on failure
 */
__syscall int crc_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			 size_t bufsize);
static inline int z_impl_crc_update(const struct device *dev, struct crc_ctx *ctx,
				    const void *buffer, size_t bufsize)
{
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	return api->crc_update(dev, ctx, buffer, bufsize);
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

	return api->crc_finish(dev, ctx);
}

/**
 * @brief Verify CRC calculation
 * @param ctx Pointer to the CRC context structure
 * @param expected Expected CRC result
 * @retval 0 if successful, negative errno code on failure
 */
static inline int crc_verify(struct crc_ctx *ctx, crc_result_t expected)
{
	if (ctx->state == CRC_STATE_IN_PROGRESS) {
		return -EBUSY;
	}

	crc_result_t res = ctx->result;

	if (res != expected) {
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

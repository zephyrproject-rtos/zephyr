/*
 * Copyright (c) 2024 Brill Power Ltd.
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CRC public API header file.
 * @ingroup crc_interface
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
 * @brief Interfaces for Cyclic Redundancy Check (CRC) devices.
 * @defgroup crc_interface CRC driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name CRC input/output string flags
 * @anchor CRC_FLAGS
 *
 * @{
 */

/**
 * @brief Reverse input string
 */
#define CRC_FLAG_REVERSE_INPUT BIT(0)

/**
 * @brief Reverse output string
 */
#define CRC_FLAG_REVERSE_OUTPUT BIT(1)

/** @} */

/**
 * @cond INTERNAL_HIDDEN
 * Internally seed values for CRC algorithms are defined.
 * These values are used to initialize the CRC calculation.
 */

/** CRC4 initial value */
#define CRC4_INIT_VAL 0x0

/** CRC4_TI initial value */
#define CRC4_TI_INIT_VAL 0x0

/** CRC7_BE initial value */
#define CRC7_BE_INIT_VAL 0x0

/** CRC8 initial value */
#define CRC8_INIT_VAL 0x0

/** CRC8_CCITT initial value */
#define CRC8_CCITT_INIT_VAL 0xFF

/** CRC8_ROHC initial value */
#define CRC8_ROHC_INIT_VAL 0xFF

/** CRC16 initial value */
#define CRC16_INIT_VAL 0x0

/** CRC16_ANSI initial value */
#define CRC16_ANSI_INIT_VAL 0x0

/** CRC16_CCITT initial value */
#define CRC16_CCITT_INIT_VAL 0x0000

/** CRC16_ITU_T initial value */
#define CRC16_ITU_T_INIT_VAL 0x0000

/** CRC24_PGP initial value */
#define CRC24_PGP_INIT_VALUE 0x00B704CEU

/** CRC32_C initial value */
#define CRC32_IEEE_INIT_VAL 0xFFFFFFFFU

/** CRC32_K_4_2 initial value */
#define CRC32_C_INIT_VAL 0xFFFFFFFFU

/** @endcond */

/**
 * @brief CRC state enumeration
 */

enum crc_state {
	/** CRC device is in IDLE state. */
	CRC_STATE_IDLE = 0,
	/** CRC calculation is in-progress. */
	CRC_STATE_IN_PROGRESS
};

/**
 * @brief Provides a type to hold CRC initial seed value
 */
typedef uint32_t crc_init_val_t;

/**
 * @brief Provides a type to hold CRC polynomial value.
 * See @a CRC_POLYNOMIAL for predefined polynomial values.
 */
typedef uint32_t crc_poly_t;

/**
 * @brief Provides a type to hold CRC result value
 */
typedef uint32_t crc_result_t;

/**
 * @brief CRC context structure
 *
 * This structure holds the state of the CRC calculation, including
 * the type of CRC being calculated, the current state, the polynomial,
 * the initial seed value, and the result of the CRC calculation.
 */

struct crc_ctx {
	/** CRC calculation type */
	enum crc_type type;
	/** Current CRC device state */
	enum crc_state state;
	/** CRC input/output reverse flags */
	uint32_t reversed;
	/** CRC polynomial */
	crc_poly_t polynomial;
	/** CRC initial seed value */
	crc_init_val_t seed;
	/** CRC result */
	crc_result_t result;
};

/**
 * @brief Callback API upon CRC calculation begin
 * See @a crc_begin() for argument description
 */
typedef int (*crc_api_begin)(const struct device *dev, struct crc_ctx *ctx);

/**
 * @brief Callback API upon CRC calculation stream update
 * See @a crc_update() for argument description
 */
typedef int (*crc_api_update)(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			      size_t bufsize);

/**
 * @brief Callback API upon CRC calculation finish
 * See @a crc_finish() for argument description
 */
typedef int (*crc_api_finish)(const struct device *dev, struct crc_ctx *ctx);

__subsystem struct crc_driver_api {
	crc_api_begin begin;
	crc_api_update update;
	crc_api_finish finish;
};

/**
 * @brief  Configure CRC unit for calculation
 *
 * @param dev Pointer to the device structure
 * @param ctx Pointer to the CRC context structure
 *
 * @retval 0 if successful
 * @retval -ENOSYS if function is not implemented
 * @retval errno code on failure
 */
__syscall int crc_begin(const struct device *dev, struct crc_ctx *ctx);

static inline int z_impl_crc_begin(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	if (api->begin == NULL) {
		return -ENOSYS;
	}

	return api->begin(dev, ctx);
}

/**
 * @brief Perform CRC calculation on the provided data buffer and retrieve result
 *
 * @param dev Pointer to the device structure
 * @param ctx Pointer to the CRC context structure
 * @param buffer Pointer to input data buffer
 * @param bufsize Number of bytes in *buffer
 *
 * @retval 0 if successful
 * @retval -ENOSYS if function is not implemented
 * @retval errno code on failure
 */
__syscall int crc_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			 size_t bufsize);

static inline int z_impl_crc_update(const struct device *dev, struct crc_ctx *ctx,
				    const void *buffer, size_t bufsize)
{
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	if (api->update == NULL) {
		return -ENOSYS;
	}

	return api->update(dev, ctx, buffer, bufsize);
}

/**
 * @brief  Finalize CRC calculation
 *
 * @param dev Pointer to the device structure
 * @param ctx Pointer to the CRC context structure
 *
 * @retval 0 if successful
 * @retval -ENOSYS if function is not implemented
 * @retval errno code on failure
 */
__syscall int crc_finish(const struct device *dev, struct crc_ctx *ctx);

static inline int z_impl_crc_finish(const struct device *dev, struct crc_ctx *ctx)
{
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	if (api->finish == NULL) {
		return -ENOSYS;
	}

	return api->finish(dev, ctx);
}

/**
 * @brief Verify CRC result
 *
 * @param ctx Pointer to the CRC context structure
 * @param expected Expected CRC result
 *
 * @retval 0 if successful
 * @retval -EBUSY if CRC calculation is not completed
 * @retval -EPERM if CRC verification failed
 */
static inline int crc_verify(struct crc_ctx *ctx, crc_result_t expected)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	if (ctx->state == CRC_STATE_IN_PROGRESS) {
		return -EBUSY;
	}

	if (expected != ctx->result) {
		return -EPERM;
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

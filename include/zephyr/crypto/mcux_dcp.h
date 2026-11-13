/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Opaque crypto key handles for NXP DCP driver.
 * @addtogroup crypto_opaque Opaque key handles
 * @ingroup crypto
 */

#ifndef ZEPHYR_INCLUDE_CRYPTO_MCUX_DCP_H_
#define ZEPHYR_INCLUDE_CRYPTO_MCUX_DCP_H_

#include <zephyr/sys/util.h>

/** Reverse the bytes within each 4-byte word of an opaque key. */
#define CRYPTO_MCUX_DCP_SWAP_KEY_BYTES	BIT(0)
/** Reverse the 4-byte words of an opaque key. */
#define CRYPTO_MCUX_DCP_SWAP_KEY_WORDS	BIT(1)

/**
 * @brief DCP slot identifier for an opaque key
 *
 * DCP slots for non-opaque keys are used internally by the driver and cannot be
 * explicitly referenced.
 */
enum crypto_mcux_dcp_slot {
	/** kDCP_OtpKey in MCUXpresso SDK; exact key source is chip-specific. */
	CRYPTO_MCUX_DCP_OTP_KEY = 1,
	/** kDCP_OtpUniqueKey in MCUXpresso SDK; exact key source is chip-specific. */
	CRYPTO_MCUX_DCP_OTP_UNIQUE_KEY = 2,
};

/**
 * @brief NXP DCP opaque key descriptor
 *
 * When using CRYPTO_MCUX_DCP with CAP_OPAQUE_KEY_HNDL, point
 * cipher_ctx.key.handle to an instance of this struct to tell the hardware
 * which opaque key to use.
 */
struct crypto_mcux_dcp_opaque_key {
	/** Selection of opaque key for DCP to use. Note that the specific key
	 * used often also depends on mux bits in IOMUXC_GPR registers, which
	 * are outside the scope of this driver.
	 */
	enum crypto_mcux_dcp_slot slot;

	/** Bitmask of CRYPTO_MCUX_DCP_SWAP_* values, indicating how the
	 * hardware should transform the selected opaque key before using it.
	 */
	int swap;
};

#endif /* ZEPHYR_INCLUDE_CRYPTO_MCUX_DCP_H_ */

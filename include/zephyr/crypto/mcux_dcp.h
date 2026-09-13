/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CRYPTO_MCUX_DCP_H_
#define ZEPHYR_INCLUDE_CRYPTO_MCUX_DCP_H_

#include <zephyr/sys/util.h>

#define CRYPTO_MCUX_DCP_SWAP_KEY_BYTES	BIT(0)
#define CRYPTO_MCUX_DCP_SWAP_KEY_WORDS	BIT(1)

enum crypto_mcux_dcp_slot {
	CRYPTO_MCUX_DCP_OTP_KEY = 1,
	CRYPTO_MCUX_DCP_OTP_UNIQUE_KEY = 2,
};

/** NXP DCP opaque key descriptor
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

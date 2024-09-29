/*
 * Copyright (c) 2024 Rapid Silicon.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Crypto Engine's OTP (One Time Programmable) Memory APIs.
 * 		  
 *
 * This file contains the Crypto Engine's OTP memory APIs
 * for reading, writing, zeroizing and locking the OTP 
 * memory contents.
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#ifndef ZEPHYR_INCLUDE_CRYPTO_OTP_MEM_H_
#define ZEPHYR_INCLUDE_CRYPTO_OTP_MEM_H_

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/toolchain.h>

/**
 * @brief Crypto OTP Mem APIs
 * @defgroup crypto OTP Mem
 * @since 1.7
 * @version 1.0.0
 * @ingroup os_services
 * @{
 */

/* ctx.flags values. Not all drivers support all flags.
 * A user app can query the supported hw / driver
 * capabilities via provided API (crypto_query_hwcaps()), and choose a
 * supported config during the session setup.
 */
#define CAP_READ_OTP			BIT(0)
#define CAP_WRITE_OTP			BIT(1)

#define CAP_LOCK_OTP			BIT(2)

/** Whether to permanently zeroiz the OTP */
#define CAP_ZEROIZ_OTP			BIT(3)

/**
 * These denotes if the output is conveyed
 * by the op function returning, or it is conveyed by an async notification
 */
#define CAP_SYNC_OPS			BIT(5)
#define CAP_ASYNC_OPS			BIT(6)

/* More flags to be added as necessary */

// OTP lock types
enum crypto_otp_lock {
  CRYPTO_OTP_RW = 0x0,  // Read-Write
  CRYPTO_OTP_RO = 0x3,  // Read-Only
  CRYPTO_OTP_NA = 0xF,  // No-Access
};

/** @brief Crypto OTP Memory driver API definition. */
__subsystem struct otp_driver_api {
	/* Get the driver capability flags for OTP Memory operations */
	int (*query_hw_caps)(const struct device *dev);

	/**
	 * otp_info API returns the total number of slots 
	 * and number of bytes per slot.
	 */
	int (*otp_info)(
					const struct device *dev, 
					uint16_t *totalSlots, 
					uint16_t *bytesPerSlot
				   );

	/**
	 * otp_read API returns the number of bytes
	 * from the selected OTP Slot. Use otp_info
	 * to get total number of slots and bytes per slot.
	 */
	int (*otp_read)(
					const struct device *dev, 
					uint16_t otp_slot, 
					uint8_t *data, 
					uint32_t len
				   );

	/**
	 * otp_write API writes the number of bytes
	 * to the selected OTP Slot. Use otp_info
	 * to get total number of slots and bytes per slot.
	 */
	int (*otp_write)(
					 const struct device *dev, 
					 uint16_t otp_slot, 
					 uint8_t *data, 
					 uint32_t len
				    );

	/**
	 * otp_zeroize API zeroize the selected OTP Slot
	 * Permanently. Use otp_info to get total number 
	 * of slots and bytes per slot.
	 */
	int (*otp_zeroize)(
					   const struct device *dev, 
					   uint16_t otp_slot
				      );

	/**
	 * otp_lock API locks the selected OTP slot as per
	 * the crypto_otp_lock. Use otp_info to get total 
	 * number of slots and bytes per slot.
	 */
	int (*otp_lock)(
					const struct device *dev, 
					uint16_t otp_slot,
					enum crypto_otp_lock lock
				   );

	/**
	 * otp_get_rwlck API gets the selected OTP Slots's
	 * lock status.
	 */
	int (*otp_get_rwlck)(
					const struct device *dev, 
					uint16_t otp_slot,
					enum crypto_otp_lock *lock
				   );
};

/* Following are the public API a user app may call. */

/**
 * @brief Query the crypto otp driver capabilities
 *
 * This API is used by the app to query the capabilities supported by the
 * crypto otp memory driver. Based on this the app can specify a subset of 
 * the supported options to be honored for a session during cipher_begin_session().
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return bitmask of supported options.
 */
__syscall int crypto_query_hwcaps(const struct device *dev);

static inline int z_impl_crypto_query_hwcaps(const struct device *dev)
{
	struct otp_driver_api *api;
	int tmp;

	api = (struct otp_driver_api *) dev->api;

	tmp = api->query_hw_caps(dev);

	__ASSERT((tmp & (CAP_READ_OTP | CAP_WRITE_OTP)) != 0,
		 "Driver should support at least Read or Write to the OTP Memory");

	__ASSERT((tmp & (CAP_SYNC_OPS | CAP_ASYNC_OPS)) != 0,
	     "Driver should support at least Synch or Asynch operation");

	return tmp;

}

#endif /* ZEPHYR_INCLUDE_CRYPTO_OTP_MEM_H_ */

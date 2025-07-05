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
#include <zephyr/internal/syscall_handler.h>

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
	int (*otp_hw_caps)(const struct device *dev);

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
	 * otp_set_lock API locks the selected OTP slot as per
	 * the crypto_otp_lock. Use otp_info to get total 
	 * number of slots and bytes per slot.
	 */
	int (*otp_set_lock)(
					const struct device *dev, 
					uint16_t otp_slot,
                    uint16_t len,
					enum crypto_otp_lock lock
				   );

	/**
	 * otp_get_lock API gets the selected OTP Slots's
	 * lock status.
	 */
	int (*otp_get_lock)(
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

static inline int otp_query_hw_aps(const struct device *dev)
{
	struct otp_driver_api *api;
	int tmp;

	api = (struct otp_driver_api *) dev->api;

	if (api->otp_hw_caps == NULL) {
		return -ENOSYS;
	}

	tmp = api->otp_hw_caps(dev);

	__ASSERT((tmp & (CAP_READ_OTP | CAP_WRITE_OTP)) != 0,
		 "Driver should support at least Read or Write to the OTP Memory");

	__ASSERT((tmp & (CAP_SYNC_OPS | CAP_ASYNC_OPS)) != 0,
	     "Driver should support at least Synch or Asynch operation");

	return tmp;

}

/**
 * @brief Query the crypto otp information.
 *
 * This API is used by the app to query the otp memory information.
 * It provides the total number of slots available in the otp and the
 * number of bytes per slot.
 *
 * @param[in]  dev Pointer to the device structure for the driver instance.
 * @param[out] totalSlots total number of available otp slots.
 * @param[out] bytesPerSlot number of bytes per slot.
 *
 * @return error code.
 */	
static inline int otp_info(
							const struct device *dev,
							uint16_t *totalSlots,
							uint16_t *bytesPerSlot
						  )
{
	struct otp_driver_api *api;

	api = (struct otp_driver_api *) dev->api;

	if (api->otp_info == NULL) {
		return -ENOSYS;
	}

	return api->otp_info(dev, totalSlots, bytesPerSlot);
}

/**
 * @brief Read the crypto otp slot
 *
 * This API is used to read a certain number of bytes from an otp slot.
 *
 * @param       dev Pointer to the device structure for the driver instance.
 * @param[in]   otp_slot the slot number of the otp to read from.
 * @param[out]  data The bytes returned.
 * @param[in]   len The number of bytes to read.
 *
 * @return error code.
 */	
static inline int otp_read(
							const struct device *dev, 
							uint16_t otp_slot, 
							uint8_t *data, 
							uint32_t len
						  )
{
	struct otp_driver_api *api;

	api = (struct otp_driver_api *) dev->api;

	if (api->otp_read == NULL) {
		return -ENOSYS;
	}

	return api->otp_read(dev, otp_slot, data, len);
}

/**
 * @brief Write to the crypto otp slot
 *
 * This API is used to write a certain number of bytes to an otp slot.
 *
 * @param dev   Pointer to the device structure for the driver instance.
 * @param[in]   otp_slot the slot number of the otp to write to.
 * @param[out]  data The bytes written to the selected otp slot.
 * @param[in]   len The number of bytes to read.
 *
 * @return error code.
 */	
static inline int otp_write(
								const struct device *dev, 
								uint16_t otp_slot, 
								uint8_t *data, 
								uint32_t len
						   )
{
	struct otp_driver_api *api;

	api = (struct otp_driver_api *) dev->api;

	if (api->otp_write == NULL) {
		return -ENOSYS;
	}

	return api->otp_write(dev, otp_slot, data, len);
}						  

/**
 * @brief Zeroize the crypto otp slot
 *
 * This API is used to zeroize a an otp slot.
 *
 * @param dev   Pointer to the device structure for the driver instance.
 * @param[in]   otp_slot the slot number to zeroize.
 *
 * @return error code.
 */	
static inline int otp_zeoirze(
								const struct device *dev,
								uint16_t otp_slot
							 )	
{
	struct otp_driver_api *api;

	api = (struct otp_driver_api *) dev->api;

	if (api->otp_zeroize == NULL) {
		return -ENOSYS;
	}

	return api->otp_zeroize(dev, otp_slot);
}														

/**
 * @brief Set the crypto otp slot a particular lock value. The lock
 * value can be referenced from crypto_otp_lock enumeration.
 *
 * This API is used to set a lock value to an otp slot.
 *
 * @param dev   Pointer to the device structure for the driver instance.
 * @param[in]   otp_slot the slot number to lock.
 * @param[in]   lock The lock value to assign to an otp slot.
 *
 * @return error code.
 */	
static inline int otp_set_lock(
								const struct device *dev,
								uint16_t otp_slot,
                                uint16_t len,
								enum crypto_otp_lock lock
							 )
{
	struct otp_driver_api *api;

	api = (struct otp_driver_api *) dev->api;

	if (api->otp_set_lock == NULL) {
		return -ENOSYS;
	}

	return api->otp_set_lock(dev, otp_slot, len, lock);
}								

/**
 * @brief Get the lock value of a particular crypto otp slot. The lock
 * value can be referenced from crypto_otp_lock enumeration.
 *
 * This API is used to get a lock value of an otp slot.
 *
 * @param dev   Pointer to the device structure for the driver instance.
 * @param[in]   otp_slot the slot number to lock.
 * @param[out]  lock The current lock value of an otp slot.
 *
 * @return error code.
 */	
static inline int otp_get_lock(
								const struct device *dev,
								uint16_t otp_slot,
								enum crypto_otp_lock *lock
							  )
{
	struct otp_driver_api *api;

	api = (struct otp_driver_api *) dev->api;

	if (api->otp_get_lock == NULL) {
		return -ENOSYS;
	}

	return api->otp_get_lock(dev, otp_slot, lock);
}

#endif /* ZEPHYR_INCLUDE_CRYPTO_OTP_MEM_H_ */

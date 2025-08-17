/*
 * Copyright (c), 2025 tinyvision.ai Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_SPI_SC18IS606_H_
#define ZEPHYR_DRIVERS_I2C_SPI_SC18IS606_H_



#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Claim the the SC18IS606 bridge
 *
 * @warning After calling this routine, the device cannot be used by any other thread
 * until the calling bridge releases it with the counterpart function of this.
 *
 * @param  dev SC18IS606  device
 * @retval 0 Device is claimed
 * @retval -EBUSY The device cannnot be claimed
 */
int nxp_sc18is606_claim(const struct device *dev);

/* @brief Release the SC18IS606 bridge
 *
 * @warning this routine can only be called once a device has been locked
 *
 * @param dev SC18IS606 bridge
 *
 * @retval 0  Device is released
 * @retval -EPERM The current thread has not claimed this so cannot release it
 * @retval -EINVAL The device has no locks on it.
 */
int nxp_sc18is606_release(const struct  device *dev);

/* @brief Transfer data using I2C to or from the bridge
 *
 * This routine implements the synchronization between the spi controller and gpio cntroller
 *
 * @param dev SC18IS606 bridge
 * @param tx_data Data to be sent out
 * @param tx_len Tx Data length
 * @param rx_data Container to recieve data
 * @param rx_let  size of expected receipt
 * @param function_id function id to be used, use NULL if none is needed ( for register writes)
 *
 * @retval 0 Tranfer success
 * @retvak -EAGAIN device did not complete and needs another turn
 */
int nxp_sc18is606_transfer(const struct device *dev,
		       const uint8_t *tx_data, uint8_t tx_len,
		       uint8_t *rx_data, uint8_t rx_len);


#ifdef __cplusplus
}
#endif

#endif

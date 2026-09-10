/* ieee802154_rf2xx_iface.h - ATMEL RF2XX transceiver interface */

/*
 * Copyright (c) 2019-2020 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_RF2XX_IFACE_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_RF2XX_IFACE_H_

/**
 * @brief Resets the TRX radio
 *
 * @param[in] dev   Transceiver device instance
 */
void rf2xx_iface_phy_rst(const struct device *dev);

/**
 * @brief Start TX transmission
 *
 * @param[in] dev   Transceiver device instance
 */
void rf2xx_iface_phy_tx_start(const struct device *dev);

/**
 * @brief Reads current value from a transceiver register
 *
 * This function reads the current value from a transceiver register.
 *
 * @param[in] dev   Transceiver device instance
 * @param[in] addr  Specifies the address of the trx register
 * from which the data shall be read
 *
 * @return value of the register read
 */
uint8_t rf2xx_iface_reg_read(const struct device *dev,
			     uint8_t addr);

/**
 * @brief Writes data into a transceiver register
 *
 * This function writes a value into transceiver register.
 *
 * @param[in] dev   Transceiver device instance
 * @param[in] addr  Address of the trx register
 * @param[in] data  Data to be written to trx register
 *
 */
void rf2xx_iface_reg_write(const struct device *dev,
			   uint8_t addr,
			   uint8_t data);

/**
 * @brief Subregister read
 *
 * @param[in] dev   Transceiver device instance
 * @param[in] addr  offset of the register
 * @param[in] mask  bit mask of the subregister
 * @param[in] pos   bit position of the subregister
 *
 * @return  value of the read bit(s)
 */
uint8_t rf2xx_iface_bit_read(const struct device *dev,
			     uint8_t addr,
			     uint8_t mask,
			     uint8_t pos);

/**
 * @brief Subregister write
 *
 * @param[in]   dev       Transceiver device instance
 * @param[in]   reg_addr  Offset of the register
 * @param[in]   mask      Bit mask of the subregister
 * @param[in]   pos       Bit position of the subregister
 * @param[out]  new_value Data, which is muxed into the register
 */
void rf2xx_iface_bit_write(const struct device *dev,
			   uint8_t reg_addr,
			   uint8_t mask,
			   uint8_t pos,
			   uint8_t new_value);

/**
 * @brief Reads frame buffer of the transceiver
 *
 * This function reads the frame buffer of the transceiver.
 *
 * @param[in]   dev     Transceiver device instance
 * @param[out]  data    Pointer to the location to store frame
 * @param[in]   length  Number of bytes to be read from the frame
 */
void rf2xx_iface_frame_read(const struct device *dev,
			    uint8_t *data,
			    uint8_t length);

/**
 * @brief Writes data into frame buffer of the transceiver
 *
 * This function writes data into the frame buffer of the transceiver
 *
 * @param[in] dev    Transceiver device instance
 * @param[in] data   Pointer to data to be written into frame buffer
 * @param[in] length Number of bytes to be written into frame buffer
 */
void rf2xx_iface_frame_write(const struct device *dev,
			     uint8_t *data,
			     uint8_t length);

/**
 * @brief Reads sram data from the transceiver
 *
 * This function reads the sram data of the transceiver.
 *
 * @param[in]   dev     Transceiver device instance
 * @param[in]   address Start address to be read
 * @param[out]  data    Pointer to the location to store data
 * @param[in]   length  Number of bytes to be read from the sram space
 */
void rf2xx_iface_sram_read(const struct device *dev,
			    uint8_t address,
			    uint8_t *data,
			    uint8_t length);

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_RF2XX_IFACE_H_ */

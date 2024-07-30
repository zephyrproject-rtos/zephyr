/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public 1-Wire Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_W1_H_
#define ZEPHYR_INCLUDE_DRIVERS_W1_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 1-Wire Interface
 * @defgroup w1_interface 1-Wire Interface
 * @since 3.2
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

/**  @cond INTERNAL_HIDDEN */

/*
 * Count the number of slaves expected on the bus.
 * This can be used to decide if the bus has a multidrop topology or
 * only a single slave is present.
 * There is a comma after each ordinal (including the last)
 * Hence FOR_EACH adds "+1" once too often which has to be subtracted in the end.
 */
#define F1(x) 1
#define W1_SLAVE_COUNT(node_id) \
		(FOR_EACH(F1, (+), DT_SUPPORTS_DEP_ORDS(node_id)) - 1)
#define W1_INST_SLAVE_COUNT(inst)  \
		(W1_SLAVE_COUNT(DT_DRV_INST(inst)))

/** @endcond */

/**
 * @brief Defines the 1-Wire master settings types, which are runtime configurable.
 */
enum w1_settings_type {
	/** Overdrive speed is enabled in case a value of 1 is passed and
	 * disabled passing 0.
	 */
	W1_SETTING_SPEED,
	/**
	 * The strong pullup resistor is activated immediately after the next
	 * written data block by passing a value of 1, and deactivated passing 0.
	 */
	W1_SETTING_STRONG_PULLUP,

	/**
	 * Number of different settings types.
	 */
	W1_SETINGS_TYPE_COUNT,
};

/**  @cond INTERNAL_HIDDEN */

/** Configuration common to all 1-Wire master implementations. */
struct w1_master_config {
	/* Number of connected slaves */
	uint16_t slave_count;
};

/** Data common to all 1-Wire master implementations. */
struct w1_master_data {
	/* The mutex used by w1_lock_bus and w1_unlock_bus methods */
	struct k_mutex bus_lock;
};

typedef int (*w1_reset_bus_t)(const struct device *dev);
typedef int (*w1_read_bit_t)(const struct device *dev);
typedef int (*w1_write_bit_t)(const struct device *dev, bool bit);
typedef int (*w1_read_byte_t)(const struct device *dev);
typedef int (*w1_write_byte_t)(const struct device *dev, const uint8_t byte);
typedef int (*w1_read_block_t)(const struct device *dev, uint8_t *buffer,
			       size_t len);
typedef int (*w1_write_block_t)(const struct device *dev, const uint8_t *buffer,
				size_t len);
typedef size_t (*w1_get_slave_count_t)(const struct device *dev);
typedef int (*w1_configure_t)(const struct device *dev,
			      enum w1_settings_type type, uint32_t value);
typedef int (*w1_change_bus_lock_t)(const struct device *dev, bool lock);

__subsystem struct w1_driver_api {
	w1_reset_bus_t reset_bus;
	w1_read_bit_t read_bit;
	w1_write_bit_t write_bit;
	w1_read_byte_t read_byte;
	w1_write_byte_t write_byte;
	w1_read_block_t read_block;
	w1_write_block_t write_block;
	w1_configure_t configure;
	w1_change_bus_lock_t change_bus_lock;
};
/** @endcond */

/** @cond INTERNAL_HIDDEN */
__syscall int w1_change_bus_lock(const struct device *dev, bool lock);

static inline int z_impl_w1_change_bus_lock(const struct device *dev, bool lock)
{
	struct w1_master_data *ctrl_data = (struct w1_master_data *)dev->data;
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	if (api->change_bus_lock) {
		return api->change_bus_lock(dev, lock);
	}

	if (lock) {
		return k_mutex_lock(&ctrl_data->bus_lock, K_FOREVER);
	} else {
		return k_mutex_unlock(&ctrl_data->bus_lock);
	}
}
/** @endcond */

/**
 * @brief Lock the 1-wire bus to prevent simultaneous access.
 *
 * This routine locks the bus to prevent simultaneous access from different
 * threads. The calling thread waits until the bus becomes available.
 * A thread is permitted to lock a mutex it has already locked.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval        0 If successful.
 * @retval -errno Negative error code on error.
 */
static inline int w1_lock_bus(const struct device *dev)
{
	return w1_change_bus_lock(dev, true);
}

/**
 * @brief Unlock the 1-wire bus.
 *
 * This routine unlocks the bus to permit access to bus line.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code on error.
 */
static inline int w1_unlock_bus(const struct device *dev)
{
	return w1_change_bus_lock(dev, false);
}

/**
 * @brief 1-Wire data link layer
 * @defgroup w1_data_link 1-Wire data link layer
 * @ingroup w1_interface
 * @{
 */

/**
 * @brief Reset the 1-Wire bus to prepare slaves for communication.
 *
 * This routine resets all 1-Wire bus slaves such that they are
 * ready to receive a command.
 * Connected slaves answer with a presence pulse once they are ready
 * to receive data.
 *
 * In case the driver supports both standard speed and overdrive speed,
 * the reset routine takes care of sendig either a short or a long reset pulse
 * depending on the current state. The speed can be changed using
 * @a w1_configure().
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0      If no slaves answer with a present pulse.
 * @retval 1      If at least one slave answers with a present pulse.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_reset_bus(const struct device *dev);

static inline int z_impl_w1_reset_bus(const struct device *dev)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->reset_bus(dev);
}

/**
 * @brief Read a single bit from the 1-Wire bus.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval rx_bit The read bit value on success.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_read_bit(const struct device *dev);

static inline int z_impl_w1_read_bit(const struct device *dev)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->read_bit(dev);
}

/**
 * @brief Write a single bit to the 1-Wire bus.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param bit     Transmitting bit value 1 or 0.
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_write_bit(const struct device *dev, const bool bit);

static inline int z_impl_w1_write_bit(const struct device *dev, bool bit)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->write_bit(dev, bit);
}

/**
 * @brief Read a single byte from the 1-Wire bus.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval rx_byte The read byte value on success.
 * @retval -errno  Negative error code on error.
 */
__syscall int w1_read_byte(const struct device *dev);

static inline int z_impl_w1_read_byte(const struct device *dev)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->read_byte(dev);
}

/**
 * @brief Write a single byte to the 1-Wire bus.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param byte    Transmitting byte.
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_write_byte(const struct device *dev, uint8_t byte);

static inline int z_impl_w1_write_byte(const struct device *dev, uint8_t byte)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->write_byte(dev, byte);
}

/**
 * @brief Read a block of data from the 1-Wire bus.
 *
 * @param[in] dev     Pointer to the device structure for the driver instance.
 * @param[out] buffer Pointer to receive buffer.
 * @param len         Length of receiving buffer (in bytes).
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_read_block(const struct device *dev, uint8_t *buffer, size_t len);

/**
 * @brief Write a block of data from the 1-Wire bus.
 *
 * @param[in] dev    Pointer to the device structure for the driver instance.
 * @param[in] buffer Pointer to transmitting buffer.
 * @param len        Length of transmitting buffer (in bytes).
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_write_block(const struct device *dev,
			     const uint8_t *buffer, size_t len);

/**
 * @brief Get the number of slaves on the bus.
 *
 * @param[in] dev  Pointer to the device structure for the driver instance.
 *
 * @retval slave_count  Positive Number of connected 1-Wire slaves on success.
 * @retval -errno       Negative error code on error.
 */
__syscall size_t w1_get_slave_count(const struct device *dev);

static inline size_t z_impl_w1_get_slave_count(const struct device *dev)
{
	const struct w1_master_config *ctrl_cfg =
		(const struct w1_master_config *)dev->config;

	return ctrl_cfg->slave_count;
}

/**
 * @brief Configure parameters of the 1-Wire master.
 *
 * Allowed configuration parameters are defined in enum w1_settings_type,
 * but master devices may not support all types.
 *
 * @param[in] dev  Pointer to the device structure for the driver instance.
 * @param type     Enum specifying the setting type.
 * @param value    The new value for the passed settings type.
 *
 * @retval 0        If successful.
 * @retval -ENOTSUP The master doesn't support the configuration of the supplied type.
 * @retval -EIO     General input / output error, failed to configure master devices.
 */
__syscall int w1_configure(const struct device *dev,
			   enum w1_settings_type type, uint32_t value);

static inline int z_impl_w1_configure(const struct device *dev,
				      enum w1_settings_type type, uint32_t value)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->configure(dev, type, value);
}

/**
 * @}
 */

/**
 * @brief 1-Wire network layer
 * @defgroup w1_network 1-Wire network layer
 * @ingroup w1_interface
 * @{
 */

/**
 * @name 1-Wire ROM Commands
 * @{
 */

/**
 * This command allows the bus master to read the slave devices without
 * providing their ROM code.
 */
#define W1_CMD_SKIP_ROM			0xCC

/**
 * This command allows the bus master to address a specific slave device by
 * providing its ROM code.
 */
#define W1_CMD_MATCH_ROM		0x55

/**
 * This command allows the bus master to resume a previous read out from where
 * it left off.
 */
#define W1_CMD_RESUME			0xA5

/**
 * This command allows the bus master to read the ROM code from a single slave
 * device.
 * This command should be used when there is only a single slave device on the
 * bus.
 */
#define W1_CMD_READ_ROM			0x33

/**
 * This command allows the bus master to discover the addresses (i.e., ROM
 * codes) of all slave devices on the bus.
 */
#define W1_CMD_SEARCH_ROM		0xF0

/**
 * This command allows the bus master to identify which devices have experienced
 * an alarm condition.
 */
#define W1_CMD_SEARCH_ALARM		0xEC

/**
 * This command allows the bus master to address all devices on the bus and then
 * switch them to overdrive speed.
 */
#define W1_CMD_OVERDRIVE_SKIP_ROM	0x3C

/**
 * This command allows the bus master to address a specific device and switch it
 * to overdrive speed.
 */
#define W1_CMD_OVERDRIVE_MATCH_ROM	0x69

/** @} */

/**
 * @name CRC Defines
 * @{
 */

/** Seed value used to calculate the 1-Wire 8-bit crc. */
#define W1_CRC8_SEED		0x00
/** Polynomial used to calculate the 1-Wire 8-bit crc. */
#define W1_CRC8_POLYNOMIAL	0x8C
/** Seed value used to calculate the 1-Wire 16-bit crc. */
#define W1_CRC16_SEED		0x0000
/** Polynomial used to calculate the 1-Wire 16-bit crc. */
#define W1_CRC16_POLYNOMIAL	0xa001

/** @} */

/** This flag can be passed to searches in order to not filter on family ID. */
#define W1_SEARCH_ALL_FAMILIES		0x00

/** Initialize all w1_rom struct members to zero. */
#define W1_ROM_INIT_ZERO					\
	{							\
		.family = 0, .serial = { 0 }, .crc = 0,		\
	}

/**
 * @brief w1_rom struct.
 */
struct w1_rom {
	/** @brief The 1-Wire family code identifying the slave device type.
	 *
	 * An incomplete list of family codes is available at:
	 * https://www.maximintegrated.com/en/app-notes/index.mvp/id/155
	 * others are documented in the respective device data sheet.
	 */
	uint8_t family;
	/** The serial together with the family code composes the unique 56-bit id */
	uint8_t serial[6];
	/** 8-bit checksum of the 56-bit unique id. */
	uint8_t crc;
};

/**
 * @brief Node specific 1-wire configuration struct.
 *
 * This struct is passed to network functions, such that they can configure
 * the bus to address the specific slave using the selected speed.
 */
struct w1_slave_config {
	/** Unique 1-Wire ROM. */
	struct w1_rom rom;
	/** overdrive speed is used if set to 1. */
	uint32_t overdrive : 1;
	/** @cond INTERNAL_HIDDEN */
	uint32_t res : 31;
	/** @endcond */
};

/**
 * @brief Define the application callback handler function signature
 *        for searches.
 *
 * @param rom found The ROM of the found slave.
 * @param user_data User data provided to the w1_search_bus() call.
 */
typedef void (*w1_search_callback_t)(struct w1_rom rom, void *user_data);

/**
 * @brief Read Peripheral 64-bit ROM.
 *
 * This procedure allows the 1-Wire bus master to read the peripherals’
 * 64-bit ROM without using the Search ROM procedure.
 * This command can be used as long as not more than a single peripheral is
 * connected to the bus.
 * Otherwise data collisions occur and a faulty ROM is read.
 *
 * @param[in] dev  Pointer to the device structure for the driver instance.
 * @param[out] rom Pointer to the ROM structure.
 *
 * @retval 0       If successful.
 * @retval -ENODEV In case no slave responds to reset.
 * @retval -errno  Other negative error code in case of invalid crc and
 *         communication errors.
 */
int w1_read_rom(const struct device *dev, struct w1_rom *rom);

/**
 * @brief Select a specific slave by broadcasting a selected ROM.
 *
 * This routine allows the 1-Wire bus master to select a slave
 * identified by its unique ROM, such that the next command will target only
 * this single selected slave.
 *
 * This command is only necessary in multidrop environments, otherwise the
 * Skip ROM command can be issued.
 * Once a slave has been selected, to reduce the communication overhead, the
 * resume command can be used instead of this command to communicate with the
 * selected slave.
 *
 * @param[in] dev    Pointer to the device structure for the driver instance.
 * @param[in] config Pointer to the slave specific 1-Wire config.
 *
 * @retval 0       If successful.
 * @retval -ENODEV In case no slave responds to reset.
 * @retval -errno  Other negative error code on error.
 */
int w1_match_rom(const struct device *dev, const struct w1_slave_config *config);

/**
 * @brief Select the slave last addressed with a Match ROM or Search ROM command.
 *
 * This routine allows the 1-Wire bus master to re-select a slave
 * device that was already addressed using a Match ROM or Search ROM command.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 *
 * @retval 0       If successful.
 * @retval -ENODEV In case no slave responds to reset.
 * @retval -errno  Other negative error code on error.
 */
int w1_resume_command(const struct device *dev);

/**
 * @brief Select all slaves regardless of ROM.
 *
 * This routine sets up the bus slaves to receive a command.
 * It is usually used when there is only one peripheral on the bus
 * to avoid the overhead of the Match ROM command.
 * But it can also be used to concurrently write to all slave devices.
 *
 * @param[in] dev    Pointer to the device structure for the driver instance.
 * @param[in] config Pointer to the slave specific 1-Wire config.
 *
 * @retval 0       If successful.
 * @retval -ENODEV In case no slave responds to reset.
 * @retval -errno  Other negative error code on error.
 */
int w1_skip_rom(const struct device *dev, const struct w1_slave_config *config);

/**
 * @brief In single drop configurations use Skip Select command, otherwise use
 *        Match ROM command.
 *
 * @param[in] dev    Pointer to the device structure for the driver instance.
 * @param[in] config Pointer to the slave specific 1-Wire config.
 *
 * @retval 0       If successful.
 * @retval -ENODEV In case no slave responds to reset.
 * @retval -errno  Other negative error code on error.
 */
int w1_reset_select(const struct device *dev, const struct w1_slave_config *config);

/**
 * @brief Write then read data from the 1-Wire slave with matching ROM.
 *
 * This routine uses w1_reset_select to select the given ROM.
 * Then writes given data and reads the response back from the slave.
 *
 * @param[in] dev       Pointer to the device structure for the driver instance.
 * @param[in] config    Pointer to the slave specific 1-Wire config.
 * @param[in] write_buf Pointer to the data to be written.
 * @param write_len     Number of bytes to write.
 * @param[out] read_buf Pointer to storage for read data.
 * @param read_len      Number of bytes to read.
 *
 * @retval 0       If successful.
 * @retval -ENODEV In case no slave responds to reset.
 * @retval -errno  Other negative error code on error.
 */
int w1_write_read(const struct device *dev, const struct w1_slave_config *config,
		  const uint8_t *write_buf, size_t write_len,
		  uint8_t *read_buf, size_t read_len);

/**
 * @brief Search 1-wire slaves on the bus.
 *
 * This function searches slaves on the 1-wire bus, with the possibility
 * to search either all slaves or only slaves that have an active alarm state.
 * If a callback is passed, the callback is called for each found slave.
 *
 * The algorithm mostly follows the suggestions of
 * https://pdfserv.maximintegrated.com/en/an/AN187.pdf
 *
 * Note: Filtering on families is not supported.
 *
 * @param[in] dev       Pointer to the device structure for the driver instance.
 * @param command       Can either be W1_SEARCH_ALARM or W1_SEARCH_ROM.
 * @param family        W1_SEARCH_ALL_FAMILIES searcheas all families,
 *                      filtering on a specific family is not yet supported.
 * @param callback      Application callback handler function to be called
 *                      for each found slave.
 * @param[in] user_data User data to pass to the application callback handler
 *                      function.
 *
 * @retval slave_count  Number of slaves found.
 * @retval -errno       Negative error code on error.
 */
__syscall int w1_search_bus(const struct device *dev, uint8_t command,
			    uint8_t family, w1_search_callback_t callback,
			    void *user_data);

/**
 * @brief Search for 1-Wire slave on bus.
 *
 * This routine can discover unknown slaves on the bus by scanning for the
 * unique 64-bit registration number.
 *
 * @param[in] dev       Pointer to the device structure for the driver instance.
 * @param callback      Application callback handler function to be called
 *                      for each found slave.
 * @param[in] user_data User data to pass to the application callback handler
 *                      function.
 *
 * @retval slave_count  Number of slaves found.
 * @retval -errno       Negative error code on error.
 */
static inline int w1_search_rom(const struct device *dev,
				w1_search_callback_t callback, void *user_data)
{
	return w1_search_bus(dev, W1_CMD_SEARCH_ROM, W1_SEARCH_ALL_FAMILIES,
			     callback, user_data);
}

/**
 * @brief Search for 1-Wire slaves with an active alarm.
 *
 * This routine searches 1-Wire slaves on the bus, which currently have
 * an active alarm.
 *
 * @param[in] dev       Pointer to the device structure for the driver instance.
 * @param callback      Application callback handler function to be called
 *                      for each found slave.
 * @param[in] user_data User data to pass to the application callback handler
 *                      function.
 *
 * @retval slave_count  Number of slaves found.
 * @retval -errno       Negative error code on error.
 */
static inline int w1_search_alarm(const struct device *dev,
				  w1_search_callback_t callback, void *user_data)
{
	return w1_search_bus(dev, W1_CMD_SEARCH_ALARM, W1_SEARCH_ALL_FAMILIES,
			     callback, user_data);
}

/**
 * @brief Function to convert a w1_rom struct to an uint64_t.
 *
 * @param[in] rom Pointer to the ROM struct.
 *
 * @retval rom64 The ROM converted to an unsigned integer in  endianness.
 */
static inline uint64_t w1_rom_to_uint64(const struct w1_rom *rom)
{
	return sys_get_be64((uint8_t *)rom);
}

/**
 * @brief Function to write an uint64_t to struct w1_rom pointer.
 *
 * @param rom64    Unsigned 64 bit integer representing the ROM in host endianness.
 * @param[out] rom The ROM struct pointer.
 */
static inline void w1_uint64_to_rom(const uint64_t rom64, struct w1_rom *rom)
{
	sys_put_be64(rom64, (uint8_t *)rom);
}

/**
 * @brief Compute CRC-8 chacksum as defined in the 1-Wire specification.
 *
 * The 1-Wire of CRC 8 variant is using 0x31 as its polynomial with the initial
 * value set to 0x00.
 * This CRC is used to check the correctness of the unique 56-bit ROM.
 *
 * @param[in] src Input bytes for the computation.
 * @param len     Length of the input in bytes.
 *
 * @retval crc The computed CRC8 value.
 */
static inline uint8_t w1_crc8(const uint8_t *src, size_t len)
{
	return crc8(src, len, W1_CRC8_POLYNOMIAL, W1_CRC8_SEED, true);
}

/**
 * @brief Compute 1-Wire variant of CRC 16
 *
 * The 16-bit 1-Wire crc variant is using the reflected polynomial function
 * X^16 + X^15 * + X^2 + 1 with the initial value set to 0x0000.
 * See also APPLICATION NOTE 27:
 * "UNDERSTANDING AND USING CYCLIC REDUNDANCY CHECKS WITH MAXIM 1-WIRE AND IBUTTON PRODUCTS"
 * https://www.maximintegrated.com/en/design/technical-documents/app-notes/2/27.html
 *
 * @param seed    Init value for the CRC, it is usually set to 0x0000.
 * @param[in] src Input bytes for the computation.
 * @param len     Length of the input in bytes.
 *
 * @retval crc The computed CRC16 value.
 */
static inline uint16_t w1_crc16(const uint16_t seed, const uint8_t *src,
				const size_t len)
{
	return crc16_reflect(W1_CRC16_POLYNOMIAL, seed, src, len);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#include <zephyr/syscalls/w1.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_W1_H_ */

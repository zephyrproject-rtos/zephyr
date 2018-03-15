/**
 * @file
 *
 * @brief Public APIs for the 1-Wire drivers.
 */

/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DRIVERS_W1_H
#define __DRIVERS_W1_H

/**
 * @brief 1-Wire Interface
 * @defgroup w1_interface 1-Wire Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <device.h>

/*
 * The following #defines are used to configure the 1-Wire controller.
 */
#define W1_SEARCH		0xF0
#define W1_ALARM_SEARCH		0xEC
#define W1_CONVERT_TEMP		0x44
#define W1_SKIP_ROM		0xCC
#define W1_COPY_SCRATCHPAD	0x48
#define W1_WRITE_SCRATCHPAD	0x4E
#define W1_READ_SCRATCHPAD	0xBE
#define W1_READ_ROM		0x33
#define W1_READ_PSUPPLY		0xB4
#define W1_MATCH_ROM		0x55
#define W1_RESUME_CMD		0xA5

#define W1_UUID_INIT(value...) (w1_reg_num *){ value }

union __deprecated dev_config {
	u32_t raw;
};

/**
 * Some family codes here
 * https://www.maximintegrated.com/en/app-notes/index.mvp/id/155
 */
typedef struct {
	u8_t family;    /* identifies the type of device */
	u8_t id[6];     /* along with family is the unique device id */
	u8_t crc;       /* checksum of the other bytes */
} w1_reg_num;

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
typedef bool (*w1_reset_bus_t)(struct device *);
typedef bool (*w1_read_bit_t)(struct device *);
typedef void (*w1_write_bit_t)(struct device *, bool);
typedef u8_t (*w1_read_byte_t)(struct device *);
typedef void (*w1_write_byte_t)(struct device *, const u8_t);
typedef void (*w1_read_block_t)(struct device *, u8_t *, u32_t);
typedef void (*w1_write_block_t)(struct device *, const void *, u32_t);
typedef void (*w1_lock_bus_t)(struct device *);
typedef void (*w1_unlock_bus_t)(struct device *);
typedef int (*w1_wait_for_t)(struct device *, u8_t, u32_t);
typedef u8_t (*w1_calc_crc8_t)(struct device *, const void *, u32_t);

struct w1_driver_api {
	w1_reset_bus_t reset_bus;
	w1_read_bit_t read_bit;
	w1_write_bit_t write_bit;
	w1_read_byte_t read_byte;
	w1_write_byte_t write_byte;
	w1_read_block_t read_block;
	w1_write_block_t write_block;
	w1_lock_bus_t lock_bus;
	w1_unlock_bus_t unlock_bus;
	w1_wait_for_t wait_for;
	w1_calc_crc8_t calc_crc8;
};
/**
 * @endcond
 */

/**
 * @brief Reset the 1-Wire bus slave devices and ready them for a command
 *
 * This routine Reset the 1-Wire bus slave devices and ready them for a command
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If there are no devices on bus.
 * @retval 1 If at least one device present on bus.
 */
static inline bool w1_reset_bus(struct device *dev)
{
	const struct w1_driver_api *api = dev->driver_api;

	return api->reset_bus(dev);
}

/**
 * @brief Send a command to bus
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param command Command byte.
 */
static inline void w1_send_command(struct device *dev, u8_t command)
{
	const struct w1_driver_api *api = dev->driver_api;

	api->write_byte(dev, command);
}

/**
 * @brief Wait for data line changed to required state
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param state State to wait for.
 * @param timeout How long to wait for required state.
 *
 * @retval 0 If catched requires state.
 * @retval 1- If timeout expired.
 */
static inline int w1_wait_for(struct device *dev, bool state, u32_t timeout)
{
	const struct w1_driver_api *api = dev->driver_api;

	return api->wait_for(dev, state, timeout);
}

/**
 * @brief Read single bit value from bus line
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval Current bit value
 */
static inline bool w1_read_bit(struct device *dev)
{
	const struct w1_driver_api *api = dev->driver_api;

	return api->read_bit(dev);
}

/**
 * @brief Write single bit to bus
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param bit Output bit value 1 or 0.
 */
static inline void w1_write_bit(struct device *dev, bool bit)
{
	const struct w1_driver_api *api = dev->driver_api;

	api->write_bit(dev, bit);
}

/**
 * @brief Read byte value from bus
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval Read byte value
 */
static inline u8_t w1_read_byte(struct device *dev)
{
	const struct w1_driver_api *api = dev->driver_api;

	return api->read_byte(dev);
}

/**
 * @brief Write single byte to bus
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param byte Output byte.
 */
static inline void w1_write_byte(struct device *dev, u8_t byte)
{
	const struct w1_driver_api *api = dev->driver_api;

	api->write_byte(dev, byte);
}

/**
 * @brief Read block of data
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buffer Pointer to transmitting buffer.
 * @param length Length of transmitting buffer (in bytes).
 */
static inline void w1_write_block(struct device *dev, const void *buffer,
	unsigned int length)
{
	const struct w1_driver_api *api = dev->driver_api;

	api->write_block(dev, buffer, length);
}

/**
 * @brief Read block of data
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buffer Pointer to receive buffer.
 * @param length Length of receiving buffer (in bytes).
 */
static inline void w1_read_block(struct device *dev, void *buffer,
	unsigned int length)
{
	const struct w1_driver_api *api = dev->driver_api;

	api->read_block(dev, buffer, length);
}

/**
 * @brief Lock the bus to prevent sumultaneous access from different threads
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
static inline void w1_lock_bus(struct device *dev)
{
	const struct w1_driver_api *api = dev->driver_api;

	api->lock_bus(dev);
}

/**
 * @brief Unlock the bus to permit access to bus line
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
static inline void w1_unlock_bus(struct device *dev)
{
	const struct w1_driver_api *api = dev->driver_api;

	api->unlock_bus(dev);
}

/**
 * @brief Read the slave’s 64-bit ROM ID
 *
 * This command can only be used when there is one slave on the bus.
 * It allows the bus master to read the slave’s 64-bit ROM code without
 * using the Search ROM procedure.
 *
 * If this command is used when there is more than one slave present on the
 * bus, a data collision will occur when all the slaves attempt to respond at
 * the same time.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg_num Pointer to the ROM structure.
 */
static inline void w1_read_rom(struct device *dev, w1_reg_num *reg_num)
{
	w1_lock_bus(dev);
	w1_send_command(dev, W1_READ_ROM);
	w1_read_block(dev, reg_num, 8);
	w1_unlock_bus(dev);
}

/**
 * @brief Selects a specific device by broadcasting a selected ROM ID
 *
 * This command can only be used when there is several slaves on the bus.
 * It allows the bus master to select slave device identified by it's UUID and
 * so the next commantd will be targeted only to this selected device.
 *
 * The Overdrive Match is similar but it also switches the device to the
 * Overdrive communication speed. The Resume Command is used to reselect the
 * last device that was selected. This is a shortcut command when repeatedly
 * accessing the same device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg_num Pointer to the ROM structure.
 */
static inline void w1_match_rom(struct device *dev, const w1_reg_num *reg_num)
{
	w1_lock_bus(dev);
	w1_reset_bus(dev);
	w1_send_command(dev, W1_MATCH_ROM);
	w1_write_block(dev, reg_num, 8);
	w1_unlock_bus(dev);
}

/**
 * @brief Command is used to select all devices regardless of ROM ID
 *
 * This command can only be used when there is one slave on the bus.
 * This could be used to gang program memory devices provided there is
 * sufficient energy. The Overdrive Skip command is similar but it not only
 * selects all devices it also puts those devices at the Overdrive
 * communication rate. This is most often used to move all capable 1-Wire
 * devices to Overdrive speed. After the devices are communicating in
 * Overdrive, the ROM IDs can be discovered using the conventional Search
 * ROM sequence.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
static inline void w1_skip_rom(struct device *dev)
{
	w1_lock_bus(dev);
	w1_reset_bus(dev);
	w1_send_command(dev, W1_SKIP_ROM);
	w1_unlock_bus(dev);
}

/**
 * @brief Calculate CRC of data
 *
 * This command can only be used when there is several slaves on the bus.
 * It allows the bus master to select slave device identified by it's UUID and
 * so the next commantd will be targeted only to this selected device.
 *
 * @param data Pointer to the data structure to calculate CRC for.
 * @param len Length of data.
 *
 * @retval CRC of data buffer
 */
static inline u8_t w1_calc_crc8(void *data, int len)
{
	// u8_t crc = 0;
  //
	// while (len--) {
	// 	crc = w1_crc8_table[crc ^ *(u8_t *)data++];
	// }
  //
	// return crc;

  const struct w1_driver_api *api = dev->driver_api;

  return api->calc_crc8(dev, data, len);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __DRIVERS_W1_H */

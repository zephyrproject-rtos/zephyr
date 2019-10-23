/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Public APIs for QSPI drivers and applications
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_QSPI_H_
#define ZEPHYR_INCLUDE_DRIVERS_QSPI_H_

/**
 * @defgroup qspi_interface QSPI Interface
 * @ingroup io_interfaces
 * @brief QSPI Interface
 *
 * The QSPI API provides support for the standard flash
 * memory devices
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following #defines are used to configure the QSPI controller.
 */

/**
 * @brief No of data lines that are used for the transfer.
 * Some flash memories supports one, two or four lines.
 */
#define QSPI_DATA_LINES_SINGLE                  0x00
#define QSPI_DATA_LINES_DOUBLE                  0x01
#define QSPI_DATA_LINES_QUAD                    0x02

/**
 * @brief QSPI mode of the transmission
 * For detailed description see mode in qspi_config
 */
#define QSPI_MODE_0                             0x00
#define QSPI_MODE_1                             0x01
#define QSPI_MODE_2                             0x02
#define QSPI_MODE_3                             0x03

/**
 * @brief QSPI Address configuration
 * Length of the address field in bits.
 * Typical flash chips support 24bit address mode
 */
#define QSPI_ADDRESS_MODE_8BIT                  0x00
#define QSPI_ADDRESS_MODE_16BIT                 0x01
#define QSPI_ADDRESS_MODE_24BIT                 0x02
#define QSPI_ADDRESS_MODE_32BIT                 0x03

/**
 * @brief QSPI buffer structure
 * Structure used both for TX and RX purposes.
 *
 * @param buf is a valid pointer to a data buffer.
 * Can not be NULL.
 * @param len is the length of the data to be handled.
 * If no data to transmit/receive - pass 0.
 */
struct qspi_buf {
	u8_t *buf;
	size_t len;
};


/**
 * @brief QSPI command structure
 * Structure used for custom command usage.
 *
 * @param op_code is a command value (i.e 0x9F - get Jedec ID)
 * @param tx_buf structure used for TX purposes. Can be NULL if not used.
 * @param rx_buf structure used for RX purposes. Can be NULL if not used.
 */
struct qspi_cmd {
	u8_t op_code;
	const struct qspi_buf *tx_buf;
	const struct qspi_buf *rx_buf;
};


/**
 * @brief QSPI controller configuration structure
 */
struct qspi_config {
	/* Chip Select pin used to select memory */
	u16_t cs_pin;

	/* Frequency of the QSPI bus in [Hz]*/
	u32_t frequency;

	/* Polarity, phase, Mode
	 * 0x00: Mode 0: Data are captured on the clock rising edge and
	 *			data are sampled on a leading edge.
	 *			Base level of clock is 0.(CPOL=0, CPHA=0).
	 * 0x01: Mode 1: Data are captured on the clock rising edge and
	 *			data are sampled on a trailing edge.
	 *			Base level of clock is 0.(CPOL=0, CPHA=1).
	 * 0x02: Mode 2: Data are captured on the clock falling edge and
	 *			data are sampled on a leading edge.
	 *			Base level of clock is 1.(CPOL=1, CPHA=0).
	 * 0x03: Mode 3: Data are captured on the clock falling edge and
	 *			data are sampled on a trailing edge.
	 *			Base level of clock is 1.(CPOL=1, CPHA=1).
	 */
	u8_t mode : 2;

	/* Defines how many lines will be used for read/write operation
	 * 0x00: One data line
	 * 0x01: Two data lines
	 * 0x02: Four data lines
	 */
	u8_t data_lines : 2;

	/* Defines how many bits are used for address (8/16/24/32)
	 * 0x00: 8 bit address field
	 * 0x01: 16 bit address field
	 * 0x02: 24 bit address field
	 * 0x03: 32 bit address field
	 */
	u8_t address : 2;

	/* Specifies the Chip Select High Time.
	 * No of clock cycles when CS must remain high between commands.
	 * Note: Refer to the chip manufacturer to check what
	 * clock source is used to generate cs_high_time. In most cases
	 * it is system clock, but sometimes it is independent
	 * QSPI clock with prescaler.
	 *
	 *                            |<-CS_HIGH_TIME->|
	 *      .-.                   .----------------.
	 *  CS -' '-------------------'                '----------
	 *          .-. .-. .-. .-. .-.                  .-. .-. .-.
	 * SCK -----' '-' '-' '-' '-' '------------------' '-' '-' '
	 *        .---.---.---.---.---.                .---.---.---.
	 * DATA   |MSB|   |...|   |LSB|                |MSB|...|LSB|
	 *    ----'---'---'---'---'---'----------------'---'---'---'
	 */
	u8_t cs_high_time;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
/**
 * @typedef qspi_api_configure
 * @brief Callback API for configuring QSPI driver
 * See qspi_configure() for argument descriptions
 */
typedef int (*qspi_api_configure)(struct device *dev,
				const struct qspi_config *config);

/**
 * @typedef qspi_api_write
 * @brief Callback API for write operations
 * See qspi_write() for argument descriptions
 */
typedef int (*qspi_api_write)(struct device *dev,
				const struct qspi_buf *tx_buf,
				u32_t address);

/**
 * @typedef qspi_api_read
 * @brief Callback API for read operations
 * See qspi_read() for argument descriptions
 */
typedef int (*qspi_api_read)(struct device *dev,
				const struct qspi_buf *rx_buf,
				u32_t address);

/**
 * @typedef qspi_api_send_cmd
 * @brief Callback API for sending custom command
 * See qspi_send_cmd() for argument descriptions
 */
typedef int (*qspi_api_send_cmd)(struct device *dev,
				const struct qspi_cmd *cmd);

/**
 * @typedef qspi_api_erase
 * @brief Callback API for erasing memory
 * See qspi_erase() for argument descriptions
 */
typedef int (*qspi_api_erase)(struct device *dev,
				u32_t start_address,
				u32_t length);


/**
 * @typedef qspi_api_write_async
 * @brief Callback API for asynchronous write operation
 * See qspi_write_async() for argument descriptions
 */
typedef int (*qspi_api_write_async)(struct device *dev,
				const struct qspi_buf *tx_buf,
				u32_t address,
				struct k_poll_signal *async);

/**
 * @typedef qspi_api_read_async
 * @brief Callback API for asynchronous read operation
 * See qspi_read_async() for argument descriptions
 */
typedef int (*qspi_api_read_async)(struct device *dev,
				const struct qspi_buf *rx_buf,
				u32_t address,
				struct k_poll_signal *async);

/**
 * @typedef qspi_api_send_cmd_async
 * @brief Callback API for asynchronous custom command send operation
 * See qspi_send_cmd_async() for argument descriptions
 */
typedef int (*qspi_api_send_cmd_async)(struct device *dev,
				const struct qspi_cmd *cmd,
				struct k_poll_signal *async);

/**
 * @typedef qspi_api_erase_async
 * @brief Callback API for asynchronous erase operation
 * See qspi_erase_async() for argument descriptions
 */
typedef int (*qspi_api_erase_async)(struct device *dev,
				u32_t start_address,
				u32_t length,
				struct k_poll_signal *async);

/**
 * @brief QSPI driver API
 * This is the mandatory API any QSPI driver needs to expose.
 */
struct qspi_driver_api {
	qspi_api_configure configure;
	qspi_api_write write;
	qspi_api_read read;
	qspi_api_send_cmd send_cmd;
	qspi_api_erase erase;
#ifdef CONFIG_QSPI_ASYNC
	qspi_api_write_async write_async;
	qspi_api_read_async read_async;
	qspi_api_send_cmd_async send_cmd_async;
	qspi_api_erase_async erase_async;
#endif /* CONFIG_QSPI_ASYNC */
};
/**
 * @endcond
 */


/**
 * @brief Configures QSPI driver to desired memory.
 *
 * Note: This function is synchronous.
 * If user want to configure QSPI flash memory for usage with quad data lines,
 * one must set Quad Enble (QE) bit in Status Register. To do so use
 * qspi_send_cmd function and Write Status Register (WRSR) command.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to the configuration structure.
 * See qspi_config structure for detailed description
 *
 * @retval  0 in case of success
 * @retval	-ENXIO No such device or address
 * @retval	-EINVAL invalid input parameter
 * @retval	-EBUSY device busy
 */
__syscall int qspi_configure(struct device *dev,
				const struct qspi_config *config);

static inline int z_impl_qspi_configure(struct device *dev,
				const struct qspi_config *config)
{
	const struct qspi_driver_api *api =
		(const struct qspi_driver_api *)dev->driver_api;

	return api->configure(dev, config);
}


/**
 * @brief Writes desired amount of data to the external flash memory.
 *
 * Note: This function is synchronous.
 * Driver that realizes write functionality has to:
 *  1. Check if memory is ready for operation - using RDSR
 *	   (read status register) command
 *	2. Enable write operation - using WREN (write enable) command
 *	3. Send data to write followed by desired write command (depends
 *	   on how many data lines are used) and address.
 *	For full list of supported commands refer to the
 *	manufacturers datasheet.
 *	If driver is configured to use four data lines one must
 *	enable quad transfer for the memory.
 *	Refer to qspi_configure description.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param tx_buf Buffer for transmit purpose.
 *			For detailed description please refer to the qspi_buf
 * @param address Address where the data will be written.
 *
 * @retval  0 in case of success
 * @retval	-ENXIO No such device or address
 * @retval	-EINVAL invalid input parameter
 * @retval	-EBUSY device busy
 */
__syscall int qspi_write(struct device *dev,
				const struct qspi_buf *tx_buf, u32_t address);

static inline int z_impl_qspi_write(struct device *dev,
				const struct qspi_buf *tx_buf, u32_t address)
{
	const struct qspi_driver_api *api =
		(const struct qspi_driver_api *)dev->driver_api;

	return api->write(dev, tx_buf, address);
}


/**
 * @brief Read desired amount of data from the external flash memory.
 *
 * Note: This function is synchronous.
 * Driver that realizes read functionality has to:
 *  1. Check if memory is ready for operation - using RDSR
 *	   (read status register) command
 *	2. Send desired read command (depends
 *	   on how many data lines are used) and address.
 *
 *	For full list of supported commands refer to the
 *	manufacturers datasheet.
 *	If driver is configured to use four data lines one must
 *	enable quad transfer for the memory.
 *	Refer to qspi_configure description.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param rx_buf Buffer for receive purpose.
			For detailed description please refer to the qspi_buf
 * @param address Address from the data will be read.
 *
 * @retval  0 in case of success
 * @retval	-ENXIO No such device or address
 * @retval	-EINVAL invalid input parameter
 * @retval	-EBUSY device busy
 */
__syscall int qspi_read(struct device *dev,
				const struct qspi_buf *rx_buf, u32_t address);

static inline int z_impl_qspi_read(struct device *dev,
				const struct qspi_buf *rx_buf, u32_t address)
{
	const struct qspi_driver_api *api =
		(const struct qspi_driver_api *)dev->driver_api;

	return api->read(dev, rx_buf, address);
}


/**
 * @brief Function for sending operation code, sending data
 * and receiving data from the memory device.
 *
 * Note: This function is synchronous.
 * For command issuing driver uses only one data line.
 * This is universal function, so user can send
 * i.e read/write/erase command using this command.
 * If user would like to send command with address and
 * payload (i.e write command), one must
 * fill op_code(i.e PP - Page Program - 0x02)
 * and tx_buff (address and data).
 * For command structure refer to the memory manufacturers datasheet.
 *
 * Driver that realizes custom command functionality has to:
 *  1. Check if memory is ready for operation - using RDSR
 *	   (read status register) command
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param cmd Command structure.
 * For detailed description please refer to the qspi_cmd
 *
 * @retval  0 in case of success
 * @retval	-ENXIO No such device or address
 * @retval	-EINVAL invalid input parameter
 * @retval	-EBUSY device busy
 */
__syscall int qspi_send_cmd(struct device *dev, const struct qspi_cmd *cmd);

static inline int z_impl_qspi_send_cmd(struct device *dev,
				const struct qspi_cmd *cmd)
{
	const struct qspi_driver_api *api =
		(const struct qspi_driver_api *)dev->driver_api;

	return api->send_cmd(dev, cmd);
}

/**
 * @brief Erases desired amount of flash memory.
 *  It is possible to erase memory area with granularity of :
 *  - 4KB (the least amount of memory to erase)
 *  - 32kB (if chip supports this function - defined in DTS)
 *  - 64KB
 *  - whole chip.
 *	One can also erase memory that is multiple or sum of the
 * listed above values. i.e user can erase memory
 * area of 100 kB (64kB + 32kB + 4 kB).
 *
 * Note: This function is synchronous.
 * Driver that realizes erase functionality has to:
 *  1. Check if memory is ready for operation - using RDSR
 *	   (read status register) command
 *	2. Enable write operation - using WREN (write enable) command
 *	3. Send desired erase command (i.e SE - sector erase). Some erase
 *		commands require address (i.e Sector Erase, Block Erase)
 *		and one not (Chip Erase).
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param start_address Memory address to start erasing.
 *		Address has to be sector aligned. (i.e it is impossible
 *		to erase memory area that begins at 0x800, since it is
 *		not sector aligned memory region)
 *		If chip erase is performed, address field is ommited.
 * @param length Size of data to erase.
 * It could be:
 *		- any size, but sector aligned
 *		- block (32kB) aligned (if 32kB block has to be erased)
 *		- block (64kB) aligned (if 64kB block has to be erased)
 *		- whole chip - pass 0XFFFFFFFF

 * @retval  0 in case of success
 * @retval	-ENXIO No such device or address
 * @retval	-EINVAL invalid input parameter
 * @retval	-EBUSY device busy
 */
__syscall int qspi_erase(struct device *dev, u32_t start_address, u32_t length);

static inline int z_impl_qspi_erase(struct device *dev,
				u32_t start_address, u32_t length)
{
	const struct qspi_driver_api *api =
		(const struct qspi_driver_api *)dev->driver_api;

	return api->erase(dev, start_address, length);
}



#ifdef CONFIG_QSPI_ASYNC
/**
 * @brief Writes desired amount of data to the external
 * flash memory in asynchronous mode.
 *
 * Note: This function is asynchronous.
 * Driver that realizes write functionality has to:
 *  1. Check if memory is ready for operation - using RDSR
 *	   (read status register) command
 *	2. Enable write operation - using WREN (write enable) command
 *	3. Send data to write followed by desired write command (depends
 *	   on how many data lines are used) and address.
 *	For full list of supported commands refer to the
 *	manufacturers datasheet.
 *	If driver is configured to use four data lines one must
 *	enable quad transfer for the memory.
 *	Refer to qspi_configure description.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param tx_buf Buffer for transmit purpose.
 *			For detailed description please refer to the qspi_buf
 * @param address Address where the data will be written.
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval  0 in case of success
 * @retval	-ENXIO No such device or address
 * @retval	-EINVAL invalid input parameter
 * @retval	-EBUSY device busy
 */
__syscall int qspi_write_async(struct device *dev,
				const struct qspi_buf *tx_buf,
				u32_t address, struct k_poll_signal *async);

static inline int z_impl_qspi_write_async(struct device *dev,
				const struct qspi_buf *tx_buf, u32_t address,
				struct k_poll_signal *async)
{
	const struct qspi_driver_api *api =
		(const struct qspi_driver_api *)dev->driver_api;

	return api->write_async(dev, tx_buf, address, async);
}


/**
 * @brief Read desired amount of data from the external flash memory
 * in asynchronous mode.
 *
 * Note: This function is asynchronous.
 * Driver that realizes read functionality has to:
 *  1. Check if memory is ready for operation - using RDSR
 *	   (read status register) command
 *	2. Send desired read command (depends
 *	   on how many data lines are used) and address.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param rx_buf Buffer for receive purpose. Consists of the fields:
 *			For detailed description please refer to the qspi_buf
 * @param address Address from the data will be read.
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval  0 in case of success
 * @retval	-ENXIO No such device or address
 * @retval	-EINVAL invalid input parameter
 * @retval	-EBUSY device busylength
 */
__syscall int qspi_read_async(struct device *dev, const struct qspi_buf *rx_buf,
				u32_t address, struct k_poll_signal *async);

static inline int z_impl_qspi_read_async(struct device *dev,
				const struct qspi_buf *rx_buf, u32_t address,
				struct k_poll_signal *async)
{
	const struct qspi_driver_api *api =
		(const struct qspi_driver_api *)dev->driver_api;

	return api->read_async(dev, rx_buf, address, async);
}


/**
 * @brief Function for sending operation code, sending data
 * and receiving data from the memory device.
 *
 * Note: This function is asynchronous.
 * For command issuing driver uses only one data line.
 * This is universal function, so user can send
 * i.e read/write/erase command using this command.
 * If user would like to send command with address and payload
 * (i.e write command), one must
 * fill op_code(i.e PP - Page Program - 0x02) and tx_buff (address and data).
 * For command structure refer to the memory manufacturers datasheet.
 *
 * Driver that realizes custom command functionality has to:
 *  1. Check if memory is ready for operation - using RDSR
 *	   (read status register) command
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param cmd Command structure.
 * For detailed description please refer to the qspi_cmd
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval  0 in case of success
 * @retval	-ENXIO No such device or address
 * @retval	-EINVAL invalid input parameter
 * @retval	-EBUSY device busy
 */
__syscall int qspi_send_cmd_async(struct device *dev,
				const struct qspi_cmd *cmd,
				struct k_poll_signal *async);

static inline int z_impl_qspi_send_cmd_async(struct device *dev,
				const struct qspi_cmd *cmd,
				struct k_poll_signal *async)
{
	const struct qspi_driver_api *api =
		(const struct qspi_driver_api *)dev->driver_api;

	return api->send_cmd_async(dev, cmd, async);
}


/**
 * @brief Erases desired amount of flash memory.
 *  It is possible to erase memory area with granularity of :
 *  - 4kB (the least amount of memory to erase)
 *  - 32kB (if chip supports this function)
 *  - 64kB
 *  - whole chip.
 *	One can also erase memory that is multiple
 *	or sum of the listed above values. i.e user can erase
 *	memory area of 100 kB (64kB + 32kB + 4 kB).
 *
 * Note: This function is asynchronous.
 * Driver that realizes erase functionality has to:
 *  1. Check if memory is ready for operation - using RDSR
 *	   (read status register) command
 *	2. Enable write operation - using WREN (write enable) command
 *	3. Send desired erase command (i.e SE - sectore erase). Some erase
 *		commands require address (i.e Sector Erase, Block Erase)
 *		and one not (Chip Erase).
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param start_address Memory address to start erasing.
 *		Address has to be sector aligned. (i.e it is impossible
 *		to erase memory area that begins at 0x800,
 *		since it is not sector aligned memory region)
 *		If chip erase is performed, address field is ommited.
 * @param length Size of data to erase.
 *		It could be:
 *		- sector aligned (if sector has to be erased)
 *		- block (32kB) aligned (if 32kB block has to be erased)
 *		- block (64kB) aligned (if 64kB block has to be erased)
 *		- whole chip - 0XFFFFFFFF
 *		- custom area, but sector aligned
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval  0 in case of success
 * @retval	-ENXIO No such device or address
 * @retval	-EINVAL invalid input parameter
 * @retval	-EBUSY device busy
 */
__syscall int qspi_erase_async(struct device *dev, u32_t start_address,
				u32_t length, struct k_poll_signal *async);

static inline int z_impl_qspi_erase_async(struct device *dev,
				u32_t start_address, u32_t length,
				struct k_poll_signal *async)
{
	const struct qspi_driver_api *api =
		(const struct qspi_driver_api *)dev->driver_api;

	return api->erase_async(dev, start_address, length, async);
}

#endif /* CONFIG_QSPI_ASYNC */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/qspi.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_QSPI_H_ */

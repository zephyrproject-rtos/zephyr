/*
 * Copyright (c) 2026 FIRST RF
 * SPDX-License-Identifier: Apache-2.0
 *
 * Lattice MachXO2 sysCONFIG transport helpers.
 *
 * These helpers are shared with the MachXO2 UFM flash driver. Transport
 * functions assume the caller holds the lock returned by
 * fpga_machxo2_lock() for the duration of a command sequence.
 */

#ifndef ZEPHYR_DRIVERS_FPGA_FPGA_MACHXO2_H_
#define ZEPHYR_DRIVERS_FPGA_FPGA_MACHXO2_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/** MachXO2 configuration and UFM page size, in bytes. */
#define FPGA_MACHXO2_PAGE_SIZE 16U

/* sysCONFIG commands used by the UFM driver. */
#define FPGA_MACHXO2_CMD_ISC_ENABLE_X      0x74U
#define FPGA_MACHXO2_CMD_ISC_DISABLE       0x26U
#define FPGA_MACHXO2_CMD_ISC_NOOP          0xFFU
#define FPGA_MACHXO2_CMD_LSC_INIT_ADDR_UFM 0x47U
#define FPGA_MACHXO2_CMD_LSC_WRITE_ADDRESS 0xB4U
#define FPGA_MACHXO2_CMD_LSC_ERASE_TAG     0xCBU
#define FPGA_MACHXO2_CMD_LSC_PROG_TAG      0xC9U
#define FPGA_MACHXO2_CMD_LSC_READ_UFM      0xCAU
#define FPGA_MACHXO2_CMD_LSC_READ_STATUS   0x3CU

/* Status register bits shared with the UFM driver. */
#define FPGA_MACHXO2_STATUS_FAIL     BIT(13)
#define FPGA_MACHXO2_STATUS_ERR_POS  23U
#define FPGA_MACHXO2_STATUS_ERR_MASK 0x7U

/* UFM command operands. */
#define FPGA_MACHXO2_ISC_ENABLE_X_UFM   0x080000U
#define FPGA_MACHXO2_UFM_ADDRESS_MARKER 0x40000000U
#define FPGA_MACHXO2_UFM_READ_OPERAND   0x100000U
#define FPGA_MACHXO2_UFM_PROG_OPERAND   0x000001U

/**
 * @brief Lock the MachXO2 transport.
 *
 * @param dev MachXO2 FPGA device.
 * @param timeout Lock timeout.
 *
 * @retval 0 on success.
 * @retval -EBUSY if the lock could not be taken within @p timeout.
 */
int fpga_machxo2_lock(const struct device *dev, k_timeout_t timeout);

/**
 * @brief Release the MachXO2 transport lock.
 *
 * @param dev MachXO2 FPGA device.
 */
void fpga_machxo2_unlock(const struct device *dev);

/**
 * @brief Write a sysCONFIG command.
 *
 * @param dev MachXO2 FPGA device.
 * @param opcode Command opcode.
 * @param operand Three-byte command operand.
 *
 * @retval 0 on success.
 * @retval -errno on SPI failure.
 */
int fpga_machxo2_write_cmd(const struct device *dev, uint8_t opcode, uint32_t operand);

/**
 * @brief Write a sysCONFIG command followed by data.
 *
 * @param dev MachXO2 FPGA device.
 * @param opcode Command opcode.
 * @param operand Three-byte command operand.
 * @param data Data sent after the command.
 * @param len Data length in bytes.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p data is NULL and @p len is not zero.
 * @retval -errno on SPI failure.
 */
int fpga_machxo2_write_cmd_data(const struct device *dev, uint8_t opcode, uint32_t operand,
				const void *data, size_t len);

/**
 * @brief Read data following a sysCONFIG command.
 *
 * @param dev MachXO2 FPGA device.
 * @param opcode Command opcode.
 * @param operand Three-byte command operand.
 * @param data Destination buffer.
 * @param len Number of bytes to read.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p data is NULL or @p len is zero.
 * @retval -errno on SPI failure.
 */
int fpga_machxo2_read_cmd(const struct device *dev, uint8_t opcode, uint32_t operand, void *data,
			  size_t len);

/**
 * @brief Poll the status register until BUSY clears.
 *
 * @param dev MachXO2 FPGA device.
 * @param timeout_ms Overall timeout in milliseconds.
 *
 * @retval 0 when the device is no longer busy.
 * @retval -ETIMEDOUT if BUSY did not clear in time.
 * @retval -errno on SPI failure.
 */
int fpga_machxo2_wait_not_busy(const struct device *dev, uint32_t timeout_ms);

#endif /* ZEPHYR_DRIVERS_FPGA_FPGA_MACHXO2_H_ */

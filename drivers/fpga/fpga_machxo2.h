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

/* sysCONFIG command opcodes (Lattice FPGA-TN-02155, table 9.3). */
#define FPGA_MACHXO2_CMD_IDCODE_PUB           0xE0U /* Read Device ID */
#define FPGA_MACHXO2_CMD_ISC_ENABLE_X         0x74U /* Enable configuration, transparent mode */
#define FPGA_MACHXO2_CMD_ISC_ENABLE           0xC6U /* Enable configuration, offline mode */
#define FPGA_MACHXO2_CMD_LSC_CHECK_BUSY       0xF0U
#define FPGA_MACHXO2_CMD_LSC_READ_STATUS      0x3CU
#define FPGA_MACHXO2_CMD_ISC_ERASE            0x0EU /* Erase memory space: sram, config, feature row, or ufm */
#define FPGA_MACHXO2_CMD_LSC_ERASE_TAG        0xCBU /* Erase UFM only */
#define FPGA_MACHXO2_CMD_LSC_INIT_ADDRESS     0x46U /* Reset page pointer to start of config flash */
#define FPGA_MACHXO2_CMD_LSC_WRITE_ADDRESS    0xB4U /* Set page address pointer */
#define FPGA_MACHXO2_CMD_LSC_PROG_INC_RNV     0x70U /* Program one flash page, config or UFM */
#define FPGA_MACHXO2_CMD_LSC_INIT_ADDR_UFM    0x47U /* Reset page pointer to start of UFM */
#define FPGA_MACHXO2_CMD_LSC_PROG_TAG         0xC9U /* Program one UFM page */
#define FPGA_MACHXO2_CMD_ISC_PROG_USERCODE    0xC2U
#define FPGA_MACHXO2_CMD_ISC_READ_USERCODE    0xC0U
#define FPGA_MACHXO2_CMD_LSC_PROG_FEATURE     0xE4U
#define FPGA_MACHXO2_CMD_LSC_READ_FEATURE     0xE7U
#define FPGA_MACHXO2_CMD_LSC_PROG_FEABITS     0xF8U
#define FPGA_MACHXO2_CMD_LSC_READ_FEABITS     0xFBU
#define FPGA_MACHXO2_CMD_LSC_READ_INCR_NV     0x73U /* Read config flash pages */
#define FPGA_MACHXO2_CMD_LSC_READ_UFM         0xCAU
#define FPGA_MACHXO2_CMD_ISC_PROGRAM_DONE     0x5EU /* Program DONE  */
#define FPGA_MACHXO2_CMD_LSC_PROG_OTP         0xF9U
#define FPGA_MACHXO2_CMD_LSC_READ_OTP         0xFAU
#define FPGA_MACHXO2_CMD_ISC_DISABLE          0x26U /* Disable configuration interface */
#define FPGA_MACHXO2_CMD_ISC_NOOP             0xFFU
#define FPGA_MACHXO2_CMD_LSC_REFRESH          0x79U /* Refresh, reconfigure from flash */
#define FPGA_MACHXO2_CMD_ISC_PROGRAM_SECURITY 0xCEU /* Program SECURITY bit */
#define FPGA_MACHXO2_CMD_ISC_PROGRAM_SECPLUS  0xCFU /* Program SECURITY PLUS bit */
#define FPGA_MACHXO2_CMD_UIDCODE_PUB          0x19U /* Read TraceID code */

/* ISC_ERASE operand: bitwise OR of the memory spaces to erase. */
#define FPGA_MACHXO2_OP_ISC_ERASE_SRAM    {0x01, 0x00, 0x00}
#define FPGA_MACHXO2_OP_ISC_ERASE_FEATURE {0x02, 0x00, 0x00}
#define FPGA_MACHXO2_OP_ISC_ERASE_CONFIG  {0x04, 0x00, 0x00}
#define FPGA_MACHXO2_OP_ISC_ERASE_UFM     {0x08, 0x00, 0x00}
#define FPGA_MACHXO2_OP_NONE			  {0x00, 0x00, 0x00}
#define FPGA_MACHXO2_OP_ISE_ENABLE		  {0x08, 0x00, 0x00}

/* Status register bits. */
#define FPGA_MACHXO2_STATUS_DONE BIT(8)
#define FPGA_MACHXO2_STATUS_BUSY BIT(12)
#define FPGA_MACHXO2_STATUS_FAIL BIT(13)

/** MachXO2 configuration and UFM page size, in bytes. */
#define FPGA_MACHXO2_PAGE_SIZE 16U


struct fpga_machxo2_cmd {
	uint8_t cmd;
	uint8_t op[3];
	size_t op_len;
};


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
 * @brief Transact a sysCONFIG command with optional data.
 *
 * Sends the four-byte command (opcode plus three operand bytes) and
 * simultaneously clocks data in or out. Set @p tx_data and @p rx_data to NULL
 * when the command carries no payload in that direction.
 *
 * @param dev MachXO2 FPGA device.
 * @param opcode Command opcode.
 * @param op Three-byte or Two-byte command operand.
 * @param op_len Length of @p op in bytes.
 * @param tx_data Data sent after the command, or NULL.
 * @param tx_len Length of @p tx_data in bytes.
 * @param rx_data Destination buffer for data read after the command, or NULL.
 * @param rx_len Number of bytes to read into @p rx_data.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p tx_data is NULL and @p tx_len is not zero.
 * @retval -EINVAL if @p rx_data is NULL and @p rx_len is not zero.
 * @retval -errno on SPI failure.
 */
int fpga_machxo2_transceive(const struct device *dev, struct fpga_machxo2_cmd *cmd,  const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data,
			    size_t rx_len);

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
int machxo2_wait_not_busy(const struct device *dev, uint32_t timeout_ms);

#endif /* ZEPHYR_DRIVERS_FPGA_FPGA_MACHXO2_H_ */

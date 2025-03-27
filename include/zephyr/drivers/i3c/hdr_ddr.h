/*
 * Copyright 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_HDR_DDR_H_
#define ZEPHYR_INCLUDE_DRIVERS_I3C_HDR_DDR_H_

/**
 * @brief I3C HDR DDR API
 * @defgroup i3c_hdr_ddr I3C HDR DDR API
 * @ingroup i3c_interface
 * @{
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/i3c.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write a set amount of data to an I3C target device with HDR DDR.
 *
 * This routine writes a set amount of data synchronously.
 *
 * @param target I3C target device descriptor.
 * @param cmd 7-bit command code
 * @param buf Memory pool from which the data is transferred.
 * @param num_bytes Number of bytes to write.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_hdr_ddr_write(struct i3c_device_desc *target, uint8_t cmd, uint8_t *buf,
				    uint32_t num_bytes)
{
	struct i3c_msg msg;

	msg.buf = buf;
	msg.len = num_bytes;
	msg.flags = I3C_MSG_WRITE | I3C_MSG_STOP | I3C_MSG_HDR;
	msg.hdr_mode = I3C_MSG_HDR_DDR;
	msg.hdr_cmd_code = cmd;

	return i3c_transfer(target, &msg, 1);
}

/**
 * @brief Read a set amount of data from an I3C target device with HDR DDR.
 *
 * This routine reads a set amount of data synchronously.
 *
 * @param target I3C target device descriptor.
 * @param cmd 7-bit command code
 * @param buf Memory pool that stores the retrieved data.
 * @param num_bytes Number of bytes to read.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_hdr_ddr_read(struct i3c_device_desc *target, uint8_t cmd, uint8_t *buf,
				   uint32_t num_bytes)
{
	struct i3c_msg msg;

	msg.buf = buf;
	msg.len = num_bytes;
	msg.flags = I3C_MSG_READ | I3C_MSG_STOP | I3C_MSG_HDR;
	msg.hdr_mode = I3C_MSG_HDR_DDR;
	msg.hdr_cmd_code = cmd;

	return i3c_transfer(target, &msg, 1);
}

/**
 * @brief Write then read data from an I3C target device with HDR DDR.
 *
 * This supports the common operation "this is what I want", "now give
 * it to me" transaction pair through a combined write-then-read bus
 * transaction.
 *
 * @param target I3C target device descriptor.
 * @param write_buf Pointer to the data to be written
 * @param num_write Number of bytes to write
 * @param read_cmd 7-bit command code for read
 * @param read_buf Pointer to storage for read data
 * @param num_read Number of bytes to read
 * @param write_cmd 7-bit command code for write
 *
 * @retval 0 if successful
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_hdr_ddr_write_read(struct i3c_device_desc *target, const void *write_buf,
					 size_t num_write, uint8_t read_cmd, void *read_buf,
					 size_t num_read, uint8_t write_cmd)
{
	struct i3c_msg msg[2];

	msg[0].buf = (uint8_t *)write_buf;
	msg[0].len = num_write;
	msg[0].flags = I3C_MSG_WRITE | I3C_MSG_HDR;
	msg[0].hdr_mode = I3C_MSG_HDR_DDR;
	msg[0].hdr_cmd_code = write_cmd;

	msg[1].buf = (uint8_t *)read_buf;
	msg[1].len = num_read;
	msg[1].flags = I3C_MSG_RESTART | I3C_MSG_READ | I3C_MSG_HDR | I3C_MSG_STOP;
	msg[1].hdr_mode = I3C_MSG_HDR_DDR;
	msg[1].hdr_cmd_code = read_cmd;

	return i3c_transfer(target, msg, 2);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_HDR_DDR_H_ */

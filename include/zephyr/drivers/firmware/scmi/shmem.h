/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCMI SHMEM API
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_SHMEM_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_SHMEM_H_

#include <zephyr/device.h>
#include <zephyr/arch/cpu.h>
#include <errno.h>

#define SCMI_SHMEM_CHAN_STATUS_BUSY_BIT BIT(0)
#define SCMI_SHMEM_CHAN_FLAG_IRQ_BIT BIT(0)

#define SCMI_SHMEM_CHAN_MSG_HDR_OFFSET	0x18

struct scmi_shmem_layout {
	volatile uint32_t res0;
	volatile uint32_t chan_status;
	volatile uint32_t res1[2];
	volatile uint32_t chan_flags;
	volatile uint32_t len;
	volatile uint32_t msg_hdr;
};

struct scmi_message;

/**
 * @brief Write a message in the SHMEM area
 *
 * @param shmem pointer to shmem device
 * @param msg message to write
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_shmem_write_message(const struct device *shmem,
			     struct scmi_message *msg);

/**
 * @brief Read a message from a SHMEM area
 *
 * @param shmem pointer to shmem device
 * @param msg message to write the data into
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_shmem_read_message(const struct device *shmem,
			    struct scmi_message *msg);

/**
 * @brief Update the channel flags
 *
 * @param shmem pointer to shmem device
 * @param mask value to negate and bitwise-and the old
 * channel flags value
 * @param val value to bitwise and with the mask and
 * bitwise-or with the masked old value
 */
void scmi_shmem_update_flags(const struct device *shmem,
			     uint32_t mask, uint32_t val);

/**
 * @brief Read a channel's status
 *
 * @param shmem pointer to shmem device
 */
uint32_t scmi_shmem_channel_status(const struct device *shmem);

/**
 * @brief Process vendor specific features when writing message
 *
 * @param layout layout of the message
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_shmem_vendor_write_message(struct scmi_shmem_layout *layout);

/**
 * @brief Process vendor specific features when reading message
 *
 * @param layout layout of the message
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_shmem_vendor_read_message(const struct scmi_shmem_layout *layout);

/**
 * @brief Read a message head from a SHMEM area
 *
 * @param shmem pointer to shmem device
 * @param msg message to write the data into
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_shmem_read_hdr(const struct device *shmem, struct scmi_message *msg);
#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_SHMEM_H_ */

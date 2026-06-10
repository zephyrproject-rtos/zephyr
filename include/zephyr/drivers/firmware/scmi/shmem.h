/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_shmem
 * @brief Header file for the SCMI Shared Memory.
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_SHMEM_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_SHMEM_H_

#include <zephyr/device.h>
#include <zephyr/arch/cpu.h>
#include <errno.h>

/**
 * @brief Shared memory transport definitions for SCMI
 * @defgroup scmi_shmem Shared Memory
 * @ingroup scmi_transport
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * Channel status: set while the channel is busy (a message has been written
 * by the agent and not yet consumed/replied to by the platform).
 */
#define SCMI_SHMEM_CHAN_STATUS_BUSY_BIT BIT(0)
/**
 * Channel flag: when set, the platform should raise an interrupt on
 * completion instead of relying on the agent polling the status.
 */
#define SCMI_SHMEM_CHAN_FLAG_IRQ_BIT BIT(0)

/**
 * @brief In-memory layout of an SCMI shared memory channel
 *
 * Mirrors the SCMI specification's shared memory area. Only meant to be used
 * by the SCMI core and by transport/vendor drivers operating on the raw shared
 * memory region.
 */
struct scmi_shmem_layout {
	/** Reserved (implementation defined) */
	volatile uint32_t res0;
	/** Channel status (see @ref SCMI_SHMEM_CHAN_STATUS_BUSY_BIT) */
	volatile uint32_t chan_status;
	/** Reserved (implementation defined) */
	volatile uint32_t res1[2];
	/** Channel flags (see @ref SCMI_SHMEM_CHAN_FLAG_IRQ_BIT) */
	volatile uint32_t chan_flags;
	/** Length in bytes of @ref msg_hdr plus the payload that follows it */
	volatile uint32_t len;
	/** Message header (see @ref SCMI_MESSAGE_HDR_MAKE) */
	volatile uint32_t msg_hdr;
};

/** @endcond */

struct scmi_message;

/**
 * @brief Write a message in the SHMEM area
 *
 * @param shmem Pointer to shmem device
 * @param msg Message to write
 * @param use_polling true if polling should be used, false otherwise
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_shmem_write_message(const struct device *shmem,
			     struct scmi_message *msg,
			     bool use_polling);

/**
 * @brief Read a message from a SHMEM area
 *
 * @param shmem Pointer to shmem device
 * @param msg Message to write the data into
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_shmem_read_message(const struct device *shmem,
			    struct scmi_message *msg);

/**
 * @brief Update the channel flags
 *
 * @param shmem Pointer to shmem device
 * @param mask Value to negate and bitwise-and the old channel flags value
 * @param val Value to bitwise and with the mask and bitwise-or with the masked old value
 */
void scmi_shmem_update_flags(const struct device *shmem,
			     uint32_t mask, uint32_t val);

/**
 * @brief Read a channel's status
 *
 * @param shmem Pointer to shmem device
 */
uint32_t scmi_shmem_channel_status(const struct device *shmem);

/**
 * @brief Process vendor specific features when writing message
 *
 * @param layout Layout of the message
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_shmem_vendor_write_message(struct scmi_shmem_layout *layout);

/**
 * @brief Process vendor specific features when reading message
 *
 * @param layout Layout of the message
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_shmem_vendor_read_message(const struct scmi_shmem_layout *layout);

/**
 * @}
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_SHMEM_H_ */

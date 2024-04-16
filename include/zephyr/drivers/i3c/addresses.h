/*
 * Copyright 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_ADDRESSES_H_
#define ZEPHYR_INCLUDE_DRIVERS_I3C_ADDRESSES_H_

/**
 * @brief I3C Address-related Helper Code
 * @defgroup i3c_addresses I3C Address-related Helper Code
 * @ingroup i3c_interface
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Broadcast Address on I3C bus. */
#define I3C_BROADCAST_ADDR			0x7E

/** Maximum value of device addresses. */
#define I3C_MAX_ADDR				0x7F

struct i3c_dev_list;

/**
 * Enum to indicate whether an address is reserved, has I2C/I3C device attached,
 * or no device attached.
 */
enum i3c_addr_slot_status {
	/** Address has not device attached. */
	I3C_ADDR_SLOT_STATUS_FREE = 0U,

	/** Address is reserved. */
	I3C_ADDR_SLOT_STATUS_RSVD,

	/** Address is associated with an I3C device. */
	I3C_ADDR_SLOT_STATUS_I3C_DEV,

	/** Address is associated with an I2C device. */
	I3C_ADDR_SLOT_STATUS_I2C_DEV,

	/** Bit masks used to filter status bits. */
	I3C_ADDR_SLOT_STATUS_MASK = 0x03U,
};

/**
 * @brief Structure to keep track of addresses on I3C bus.
 */
struct i3c_addr_slots {
	/* 2 bits per slot */
	unsigned long slots[((I3C_MAX_ADDR + 1) * 2) / BITS_PER_LONG];
};

/**
 * @brief Initialize the I3C address slots struct.
 *
 * This clears out the assigned address bits, and set the reserved
 * address bits according to the I3C specification.
 *
 * @param dev Pointer to controller device driver instance.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if duplicate addresses.
 */
int i3c_addr_slots_init(const struct device *dev);

/**
 * @brief Set the address status of a device.
 *
 * @param slots Pointer to the address slots structure.
 * @param dev_addr Device address.
 * @param status New status for the address @p dev_addr.
 */
void i3c_addr_slots_set(struct i3c_addr_slots *slots,
			uint8_t dev_addr,
			enum i3c_addr_slot_status status);

/**
 * @brief Get the address status of a device.
 *
 * @param slots Pointer to the address slots structure.
 * @param dev_addr Device address.
 *
 * @return Address status for the address @p dev_addr.
 */
enum i3c_addr_slot_status i3c_addr_slots_status(struct i3c_addr_slots *slots,
						uint8_t dev_addr);

/**
 * @brief Check if the address is free.
 *
 * @param slots Pointer to the address slots structure.
 * @param dev_addr Device address.
 *
 * @retval true if address is free.
 * @retval false if address is not free.
 */
bool i3c_addr_slots_is_free(struct i3c_addr_slots *slots,
			    uint8_t dev_addr);

/**
 * @brief Find the next free address.
 *
 * This can be used to find the next free address that can be
 * assigned to a new device.
 *
 * @param slots Pointer to the address slots structure.
 *
 * @return The next free address, or 0 if none found.
 */
uint8_t i3c_addr_slots_next_free_find(struct i3c_addr_slots *slots);

/**
 * @brief Mark the address as free (not used) in device list.
 *
 * @param addr_slots Pointer to the address slots struct.
 * @param addr Device address.
 */
static inline void i3c_addr_slots_mark_free(struct i3c_addr_slots *addr_slots,
					    uint8_t addr)
{
	i3c_addr_slots_set(addr_slots, addr,
			   I3C_ADDR_SLOT_STATUS_FREE);
}

/**
 * @brief Mark the address as reserved in device list.
 *
 * @param addr_slots Pointer to the address slots struct.
 * @param addr Device address.
 */
static inline void i3c_addr_slots_mark_rsvd(struct i3c_addr_slots *addr_slots,
					    uint8_t addr)
{
	i3c_addr_slots_set(addr_slots, addr,
			   I3C_ADDR_SLOT_STATUS_RSVD);
}

/**
 * @brief Mark the address as I3C device in device list.
 *
 * @param addr_slots Pointer to the address slots struct.
 * @param addr Device address.
 */
static inline void i3c_addr_slots_mark_i3c(struct i3c_addr_slots *addr_slots,
					   uint8_t addr)
{
	i3c_addr_slots_set(addr_slots, addr,
			   I3C_ADDR_SLOT_STATUS_I3C_DEV);
}

/**
 * @brief Mark the address as I2C device in device list.
 *
 * @param addr_slots Pointer to the address slots struct.
 * @param addr Device address.
 */
static inline void i3c_addr_slots_mark_i2c(struct i3c_addr_slots *addr_slots,
					   uint8_t addr)
{
	i3c_addr_slots_set(addr_slots, addr,
			   I3C_ADDR_SLOT_STATUS_I2C_DEV);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_ADDRESSES_H_ */

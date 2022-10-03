/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

#include <zephyr/drivers/i3c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c, CONFIG_I3C_LOG_LEVEL);

void i3c_dump_msgs(const char *name, const struct i3c_msg *msgs,
		   uint8_t num_msgs, struct i3c_device_desc *target)
{
	LOG_DBG("I3C msg: %s, addr=%x", name, target->dynamic_addr);
	for (unsigned int i = 0; i < num_msgs; i++) {
		const struct i3c_msg *msg = &msgs[i];

		LOG_DBG("   %c len=%02x: ",
			msg->flags & I3C_MSG_READ ? 'R' : 'W', msg->len);
		if (!(msg->flags & I3C_MSG_READ)) {
			LOG_HEXDUMP_DBG(msg->buf, msg->len, "contents:");
		}
	}
}

void i3c_addr_slots_set(struct i3c_addr_slots *slots,
			uint8_t dev_addr,
			enum i3c_addr_slot_status status)
{
	int bitpos;
	int idx;

	__ASSERT_NO_MSG(slots != NULL);

	if (dev_addr > I3C_MAX_ADDR) {
		/* Invalid address. Do nothing. */
		return;
	}

	bitpos = dev_addr * 2;
	idx = bitpos / BITS_PER_LONG;

	slots->slots[idx] &= ~((unsigned long)I3C_ADDR_SLOT_STATUS_MASK <<
			       (bitpos % BITS_PER_LONG));
	slots->slots[idx] |= status << (bitpos % BITS_PER_LONG);
}

enum i3c_addr_slot_status
i3c_addr_slots_status(struct i3c_addr_slots *slots,
		      uint8_t dev_addr)
{
	unsigned long status;
	int bitpos;
	int idx;

	__ASSERT_NO_MSG(slots != NULL);

	if (dev_addr > I3C_MAX_ADDR) {
		/* Invalid address.
		 * Simply says it's reserved so it will not be
		 * used for anything.
		 */
		return I3C_ADDR_SLOT_STATUS_RSVD;
	}

	bitpos = dev_addr * 2;
	idx = bitpos / BITS_PER_LONG;

	status = slots->slots[idx] >> (bitpos % BITS_PER_LONG);
	status &= I3C_ADDR_SLOT_STATUS_MASK;

	return status;
}


int i3c_addr_slots_init(struct i3c_addr_slots *slots,
			const struct i3c_dev_list *dev_list)
{
	int i, ret = 0;
	struct i3c_device_desc *i3c_dev;
	struct i3c_i2c_device_desc *i2c_dev;

	__ASSERT_NO_MSG(slots != NULL);

	(void)memset(slots, 0, sizeof(*slots));

	for (i = 0; i <= 7; i++) {
		/* Addresses 0 to 7 are reserved */
		i3c_addr_slots_set(slots, i, I3C_ADDR_SLOT_STATUS_RSVD);

		/*
		 * Addresses within a single bit error of broadcast address
		 * are also reserved.
		 */
		i3c_addr_slots_set(slots, I3C_BROADCAST_ADDR ^ BIT(i),
				   I3C_ADDR_SLOT_STATUS_RSVD);

	}

	/* The broadcast address is reserved */
	i3c_addr_slots_set(slots, I3C_BROADCAST_ADDR,
			   I3C_ADDR_SLOT_STATUS_RSVD);

	/*
	 * If there is a static address for the I3C devices, check
	 * if this address is free, and there is no other devices of
	 * the same (pre-assigned) address on the bus.
	 */
	for (i = 0; i < dev_list->num_i3c; i++) {
		i3c_dev = &dev_list->i3c[i];
		if (i3c_dev->static_addr != 0U) {
			if (i3c_addr_slots_is_free(slots, i3c_dev->static_addr)) {
				/*
				 * Mark address slot as I3C device for now to
				 * detect address collisons. This marking may be
				 * released during address assignment.
				 */
				i3c_addr_slots_mark_i3c(slots, i3c_dev->static_addr);
			} else {
				/* Address slot is not free */
				ret = -EINVAL;
				goto out;
			}
		}
	}

	/*
	 * Mark all I2C addresses.
	 */
	for (i = 0; i < dev_list->num_i2c; i++) {
		i2c_dev = &dev_list->i2c[i];
		if (i3c_addr_slots_is_free(slots, i2c_dev->addr)) {
			i3c_addr_slots_mark_i2c(slots, i2c_dev->addr);
		} else {
			/* Address slot is not free */
			ret = -EINVAL;
			goto out;
		}
	}

out:
	return ret;
}

bool i3c_addr_slots_is_free(struct i3c_addr_slots *slots,
			    uint8_t dev_addr)
{
	enum i3c_addr_slot_status status;

	__ASSERT_NO_MSG(slots != NULL);

	status = i3c_addr_slots_status(slots, dev_addr);

	return (status == I3C_ADDR_SLOT_STATUS_FREE);
}

uint8_t i3c_addr_slots_next_free_find(struct i3c_addr_slots *slots)
{
	uint8_t addr;
	enum i3c_addr_slot_status status;

	/* Addresses 0 to 7 are reserved. So start at 8. */
	for (addr = 8; addr < I3C_MAX_ADDR; addr++) {
		status = i3c_addr_slots_status(slots, addr);
		if (status == I3C_ADDR_SLOT_STATUS_FREE) {
			return addr;
		}
	}

	return 0;
}

struct i3c_device_desc *i3c_dev_list_find(const struct i3c_dev_list *dev_list,
					  const struct i3c_device_id *id)
{
	int i;
	struct i3c_device_desc *ret = NULL;

	__ASSERT_NO_MSG(dev_list != NULL);

	for (i = 0; i < dev_list->num_i3c; i++) {
		struct i3c_device_desc *desc = &dev_list->i3c[i];

		if (desc->pid == id->pid) {
			ret = desc;
			break;
		}
	}

	return ret;
}

struct i3c_device_desc *i3c_dev_list_i3c_addr_find(const struct i3c_dev_list *dev_list,
						   uint8_t addr)
{
	int i;
	struct i3c_device_desc *ret = NULL;

	__ASSERT_NO_MSG(dev_list != NULL);

	for (i = 0; i < dev_list->num_i3c; i++) {
		struct i3c_device_desc *desc = &dev_list->i3c[i];

		if (desc->dynamic_addr == addr) {
			ret = desc;
			break;
		}
	}

	return ret;
}

struct i3c_i2c_device_desc *i3c_dev_list_i2c_addr_find(const struct i3c_dev_list *dev_list,
						       uint16_t addr)
{
	int i;
	struct i3c_i2c_device_desc *ret = NULL;

	__ASSERT_NO_MSG(dev_list != NULL);

	for (i = 0; i < dev_list->num_i2c; i++) {
		struct i3c_i2c_device_desc *desc = &dev_list->i2c[i];

		if (desc->addr == addr) {
			ret = desc;
			break;
		}
	}

	return ret;
}

int i3c_dev_list_daa_addr_helper(struct i3c_addr_slots *addr_slots,
				 const struct i3c_dev_list *dev_list,
				 uint64_t pid, bool must_match,
				 bool assigned_okay,
				 struct i3c_device_desc **target,
				 uint8_t *addr)
{
	struct i3c_device_desc *desc;
	const uint16_t vendor_id = (uint16_t)(pid >> 32);
	const uint32_t part_no = (uint32_t)(pid & 0xFFFFFFFFU);
	uint8_t dyn_addr = 0;
	int ret = 0;
	const struct i3c_device_id i3c_id = I3C_DEVICE_ID(pid);

	desc = i3c_dev_list_find(dev_list, &i3c_id);
	if (must_match && (desc == NULL)) {
		/*
		 * No device descriptor matching incoming PID and
		 * that we want an exact match.
		 */
		ret = -ENODEV;

		LOG_DBG("PID 0x%04x%08x is not in registered device list",
			vendor_id, part_no);

		goto out;
	}

	if (desc->dynamic_addr != 0U) {
		if (assigned_okay) {
			/* Return the already assigned address if desired so. */
			dyn_addr = desc->dynamic_addr;
			goto out;
		} else {
			/*
			 * Bail If target already has an assigned address.
			 * This is probably due to having the same PIDs for multiple targets
			 * in the device tree.
			 */
			LOG_ERR("PID 0x%04x%08x already has "
				"dynamic address (0x%02x) assigned",
				vendor_id, part_no, desc->dynamic_addr);
			ret = -EINVAL;
			goto err;
		}
	}

	/*
	 * Use the desired dynamic address as the new dynamic address
	 * if the slot is free.
	 */
	if (desc->init_dynamic_addr != 0U) {
		if (i3c_addr_slots_is_free(addr_slots,
					   desc->init_dynamic_addr)) {
			dyn_addr = desc->init_dynamic_addr;
			goto out;
		}
	}

	/*
	 * Find the next available address.
	 */
	dyn_addr = i3c_addr_slots_next_free_find(addr_slots);

	if (dyn_addr == 0U) {
		/* No free addresses available */
		LOG_DBG("No more free addresses available.");
		ret = -ENOSPC;
	}

out:
	*addr = dyn_addr;
	*target = desc;

err:
	return ret;
}

int i3c_device_basic_info_get(struct i3c_device_desc *target)
{
	int ret;
	uint8_t tmp_bcr;

	struct i3c_ccc_getbcr bcr = {0};
	struct i3c_ccc_getdcr dcr = {0};
	struct i3c_ccc_mrl mrl = {0};
	struct i3c_ccc_mwl mwl = {0};

	/*
	 * Since some CCC functions requires BCR to function
	 * correctly, we save the BCR here and update the BCR
	 * in the descriptor. If any following operations fails,
	 * we can restore the BCR.
	 */
	tmp_bcr = target->bcr;

	/* GETBCR */
	ret = i3c_ccc_do_getbcr(target, &bcr);
	if (ret != 0) {
		goto out;
	}

	target->bcr = bcr.bcr;

	/* GETDCR */
	ret = i3c_ccc_do_getdcr(target, &dcr);
	if (ret != 0) {
		goto out;
	}

	/* GETMRL */
	ret = i3c_ccc_do_getmrl(target, &mrl);
	if (ret != 0) {
		goto out;
	}

	/* GETMWL */
	ret = i3c_ccc_do_getmwl(target, &mwl);
	if (ret != 0) {
		goto out;
	}

	target->dcr = dcr.dcr;
	target->data_length.mrl = mrl.len;
	target->data_length.mwl = mwl.len;
	target->data_length.max_ibi = mrl.ibi_len;

out:
	if (ret != 0) {
		/* Restore BCR is any CCC fails. */
		target->bcr = tmp_bcr;
	}
	return ret;
}

/**
 * @brief Do SETDASA to set static address as dynamic address.
 *
 * @param dev Pointer to the device driver instance.
 * @param[out] True if DAA is still needed. False if all registered
 *             devices have static addresses.
 *
 * @retval 0 if successful.
 */
static int i3c_bus_setdasa(const struct device *dev,
			   const struct i3c_dev_list *dev_list,
			   bool *need_daa)
{
	int i, ret;

	*need_daa = false;

	/* Loop through the registered I3C devices */
	for (i = 0; i < dev_list->num_i3c; i++) {
		struct i3c_device_desc *desc = &dev_list->i3c[i];

		/*
		 * A device without static address => need to do
		 * dynamic address assignment.
		 */
		if (desc->static_addr == 0U) {
			*need_daa = true;
			continue;
		}

		/*
		 * If there is a desired dynamic address and it is
		 * not the same as the static address, wait till
		 * ENTDAA to do address assignment as this is
		 * no longer SETDASA.
		 */
		if ((desc->init_dynamic_addr != 0U) &&
		    (desc->init_dynamic_addr != desc->static_addr)) {
			*need_daa = true;
			continue;
		}

		LOG_DBG("SETDASA for 0x%x", desc->static_addr);

		ret = i3c_ccc_do_setdasa(desc);
		if (ret == 0) {
			desc->dynamic_addr = desc->static_addr;
		} else {
			LOG_ERR("SETDASA error on address 0x%x (%d)",
				desc->static_addr, ret);
			continue;
		}
	}

	return 0;
}

int i3c_bus_init(const struct device *dev, const struct i3c_dev_list *dev_list)
{
	int i, ret = 0;
	bool need_daa = true;
	struct i3c_ccc_events i3c_events;

	/*
	 * Reset all connected targets. Also reset dynamic
	 * addresses for all devices as we have no idea what
	 * dynamic addresses the connected devices have
	 * (e.g. assigned during previous power cycle).
	 *
	 * Note that we ignore error for both RSTACT and RSTDAA
	 * as there may not be any connected devices responding
	 * to these CCCs.
	 */
	if (i3c_ccc_do_rstact_all(dev, I3C_CCC_RSTACT_RESET_WHOLE_TARGET) != 0) {
		/*
		 * Reset Whole Target support is not required so
		 * if there is any NACK, we want to at least reset
		 * the I3C peripheral of targets.
		 */
		LOG_DBG("Broadcast RSTACT (whole target) was NACK.");

		if (i3c_ccc_do_rstact_all(dev, I3C_CCC_RSTACT_PERIPHERAL_ONLY) != 0) {
			LOG_DBG("Broadcast RSTACT (peripehral) was NACK.");
		}
	}

	if (i3c_ccc_do_rstdaa_all(dev) != 0) {
		LOG_DBG("Broadcast RSTDAA was NACK.");
	}

	/*
	 * Disable all events from targets to avoid them
	 * interfering with bus initialization,
	 * especially during DAA.
	 */
	i3c_events.events = I3C_CCC_EVT_ALL;
	ret = i3c_ccc_do_events_all_set(dev, false, &i3c_events);
	if (ret != 0) {
		LOG_DBG("Broadcast DISEC was NACK.");
	}

	/*
	 * Set static addresses as dynamic addresses.
	 */
	ret = i3c_bus_setdasa(dev, dev_list, &need_daa);
	if (ret != 0) {
		goto err_out;
	}

	/*
	 * Perform Dynamic Address Assignment if needed.
	 */
	if (need_daa) {
		ret = i3c_do_daa(dev);
		if (ret != 0) {
			/*
			 * Spec says to try once more
			 * if DAA fails the first time.
			 */
			ret = i3c_do_daa(dev);
			if (ret != 0) {
				/*
				 * Failure to finish dynamic address assignment
				 * is not the end of world... hopefully.
				 * Continue on so the devices already have
				 * addresses can still function.
				 */
				LOG_ERR("DAA was not successful.");
			}
		}
	}

	/*
	 * Loop through the registered I3C devices to retrieve
	 * basic target information.
	 */
	for (i = 0; i < dev_list->num_i3c; i++) {
		struct i3c_device_desc *desc = &dev_list->i3c[i];

		if (desc->dynamic_addr == 0U) {
			continue;
		}

		ret = i3c_device_basic_info_get(desc);
		if (ret != 0) {
			LOG_ERR("Error getting basic device info for 0x%02x",
				desc->static_addr);
		} else {
			LOG_DBG("Target 0x%02x, BCR 0x%02x, DCR 0x%02x, MRL %d, MWL %d, IBI %d",
				desc->dynamic_addr, desc->bcr, desc->dcr,
				desc->data_length.mrl, desc->data_length.mwl,
				desc->data_length.max_ibi);
		}
	}

	/*
	 * Only re-enable Hot-Join from targets.
	 * Target interrupts will be enabled when IBI is enabled.
	 * And transferring controller role is not supported so not need to
	 * enable the event.
	 */
	i3c_events.events = I3C_CCC_EVT_HJ;
	ret = i3c_ccc_do_events_all_set(dev, true, &i3c_events);
	if (ret != 0) {
		LOG_DBG("Broadcast ENEC was NACK.");
	}

err_out:
	return ret;
}

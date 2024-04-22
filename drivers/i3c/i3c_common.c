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


int i3c_addr_slots_init(const struct device *dev)
{
	struct i3c_driver_data *data =
		(struct i3c_driver_data *)dev->data;
	const struct i3c_driver_config *config =
		(const struct i3c_driver_config *)dev->config;
	int i, ret = 0;
	struct i3c_device_desc *i3c_dev;
	struct i3c_i2c_device_desc *i2c_dev;

	__ASSERT_NO_MSG(dev != NULL);

	(void)memset(&data->attached_dev.addr_slots, 0, sizeof(data->attached_dev.addr_slots));
	sys_slist_init(&data->attached_dev.devices.i3c);
	sys_slist_init(&data->attached_dev.devices.i2c);

	for (i = 0; i <= 7; i++) {
		/* Addresses 0 to 7 are reserved */
		i3c_addr_slots_set(&data->attached_dev.addr_slots, i, I3C_ADDR_SLOT_STATUS_RSVD);

		/*
		 * Addresses within a single bit error of broadcast address
		 * are also reserved.
		 */
		i3c_addr_slots_set(&data->attached_dev.addr_slots, I3C_BROADCAST_ADDR ^ BIT(i),
				   I3C_ADDR_SLOT_STATUS_RSVD);

	}

	/* The broadcast address is reserved */
	i3c_addr_slots_set(&data->attached_dev.addr_slots, I3C_BROADCAST_ADDR,
			   I3C_ADDR_SLOT_STATUS_RSVD);

	/*
	 * Mark all I2C addresses first.
	 */
	for (i = 0; i < config->dev_list.num_i2c; i++) {
		i2c_dev = &config->dev_list.i2c[i];
		ret = i3c_attach_i2c_device(i2c_dev);
		if (ret != 0) {
			/* Address slot is not free */
			ret = -EINVAL;
			goto out;
		}
	}

	/*
	 * If there is a static address for the I3C devices, check
	 * if this address is free, and there is no other devices of
	 * the same (pre-assigned) address on the bus.
	 */
	for (i = 0; i < config->dev_list.num_i3c; i++) {
		i3c_dev = &config->dev_list.i3c[i];
		ret = i3c_attach_i3c_device(i3c_dev);
		if (ret != 0) {
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

uint8_t i3c_addr_slots_next_free_find(struct i3c_addr_slots *slots, uint8_t start_addr)
{
	uint8_t addr;
	enum i3c_addr_slot_status status;

	/* Addresses 0 to 7 are reserved. So start at 8. */
	for (addr = MAX(start_addr, 8); addr < I3C_MAX_ADDR; addr++) {
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

	/* this only searches known I3C PIDs */
	for (i = 0; i < dev_list->num_i3c; i++) {
		struct i3c_device_desc *desc = &dev_list->i3c[i];

		if (desc->pid == id->pid) {
			ret = desc;
			break;
		}
	}

	return ret;
}

struct i3c_device_desc *i3c_dev_list_i3c_addr_find(struct i3c_dev_attached_list *dev_list,
						   uint8_t addr)
{
	sys_snode_t *node;
	struct i3c_device_desc *ret = NULL;

	__ASSERT_NO_MSG(dev_list != NULL);

	SYS_SLIST_FOR_EACH_NODE(&dev_list->devices.i3c, node) {
		struct i3c_device_desc *desc = (void *)node;

		if (desc->dynamic_addr == addr) {
			ret = desc;
			break;
		}
	}

	return ret;
}

struct i3c_i2c_device_desc *i3c_dev_list_i2c_addr_find(struct i3c_dev_attached_list *dev_list,
							   uint16_t addr)
{
	sys_snode_t *node;
	struct i3c_i2c_device_desc *ret = NULL;

	__ASSERT_NO_MSG(dev_list != NULL);

	SYS_SLIST_FOR_EACH_NODE(&dev_list->devices.i2c, node) {
		struct i3c_i2c_device_desc *desc = (void *)node;

		if (desc->addr == addr) {
			ret = desc;
			break;
		}
	}

	return ret;
}

int i3c_determine_default_addr(struct i3c_device_desc *target, uint8_t *addr)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)target->bus->data;

	/* If dynamic addr is set, then it assumed that it was assigned by a primary controller */
	if (target->dynamic_addr == 0) {
		/* It is assumed that SETDASA or ENTDAA will be run after this */
		if (target->init_dynamic_addr != 0U) {
			/* initial dynamic address is requested */
			if (target->static_addr == 0) {
				/* SA is set to 0, so DA will be set with ENTDAA */
				if (i3c_addr_slots_is_free(&data->attached_dev.addr_slots,
							   target->init_dynamic_addr)) {
					/* Set DA during ENTDAA */
					*addr = target->init_dynamic_addr;
				} else {
					/* address is not free, get the next one */
					*addr = i3c_addr_slots_next_free_find(
						&data->attached_dev.addr_slots, 0);
				}
			} else {
				/* Use the init dynamic address as it's DA, but the RR will need to
				 * be first set with it's SA to run SETDASA, the RR address will
				 * need be updated after SETDASA with the request dynamic address
				 */
				if (i3c_addr_slots_is_free(&data->attached_dev.addr_slots,
							   target->static_addr)) {
					*addr = target->static_addr;
				} else {
					/* static address has already been taken */
					return -EINVAL;
				}
			}
		} else {
			/* no init dynamic address is requested */
			if (target->static_addr != 0) {
				if (i3c_addr_slots_is_free(&data->attached_dev.addr_slots,
							   target->static_addr)) {
					/* static exists, set DA with same SA during SETDASA*/
					*addr = target->static_addr;
				} else {
					/* static address has already been taken */
					return -EINVAL;
				}
			} else {
				/* pick a DA to use */
				*addr = i3c_addr_slots_next_free_find(
					&data->attached_dev.addr_slots, 0);
			}
		}
	} else {
		*addr = target->dynamic_addr;
	}

	return 0;
}

int i3c_attach_i3c_device(struct i3c_device_desc *target)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)target->bus->data;
	const struct i3c_driver_api *api = (const struct i3c_driver_api *)target->bus->api;
	sys_snode_t *node;
	uint8_t addr = 0;
	int status = 0;

	/* check to see if the device has already been attached */
	if (!sys_slist_is_empty(&data->attached_dev.devices.i3c)) {
		SYS_SLIST_FOR_EACH_NODE(&data->attached_dev.devices.i3c, node) {
			if (node == &target->node) {
				return -EINVAL;
			}
		}
	}

	status = i3c_determine_default_addr(target, &addr);
	if (status != 0) {
		return status;
	}

	sys_slist_append(&data->attached_dev.devices.i3c, &target->node);

	if (api->attach_i3c_device != NULL) {
		status = api->attach_i3c_device(target->bus, target, addr);
	}

	i3c_addr_slots_mark_i3c(&data->attached_dev.addr_slots, addr);

	return status;
}

int i3c_reattach_i3c_device(struct i3c_device_desc *target, uint8_t old_dyn_addr)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)target->bus->data;
	const struct i3c_driver_api *api = (const struct i3c_driver_api *)target->bus->api;
	int status = 0;

	if (!i3c_addr_slots_is_free(&data->attached_dev.addr_slots, target->dynamic_addr)) {
		return -EINVAL;
	}

	if (api->reattach_i3c_device != NULL) {
		status = api->reattach_i3c_device(target->bus, target, old_dyn_addr);
	}

	if (old_dyn_addr) {
		/* mark the old address as free */
		i3c_addr_slots_mark_free(&data->attached_dev.addr_slots, old_dyn_addr);
	}

	i3c_addr_slots_mark_i3c(&data->attached_dev.addr_slots, target->dynamic_addr);

	return status;
}

int i3c_detach_i3c_device(struct i3c_device_desc *target)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)target->bus->data;
	const struct i3c_driver_api *api = (const struct i3c_driver_api *)target->bus->api;
	int status = 0;

	if (!sys_slist_is_empty(&data->attached_dev.devices.i3c)) {
		if (!sys_slist_find_and_remove(&data->attached_dev.devices.i3c, &target->node)) {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	if (api->detach_i3c_device != NULL) {
		status = api->detach_i3c_device(target->bus, target);
	}

	i3c_addr_slots_mark_free(&data->attached_dev.addr_slots,
				 target->dynamic_addr ? target->dynamic_addr : target->static_addr);

	return status;
}

int i3c_attach_i2c_device(struct i3c_i2c_device_desc *target)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)target->bus->data;
	const struct i3c_driver_api *api = (const struct i3c_driver_api *)target->bus->api;
	sys_snode_t *node;
	int status = 0;

	/* check to see if the device has already been attached */
	if (!sys_slist_is_empty(&data->attached_dev.devices.i2c)) {
		SYS_SLIST_FOR_EACH_NODE(&data->attached_dev.devices.i2c, node) {
			if (node == &target->node) {
				return -EINVAL;
			}
		}
	}

	if (!i3c_addr_slots_is_free(&data->attached_dev.addr_slots, target->addr)) {
		return -EINVAL;
	}

	sys_slist_append(&data->attached_dev.devices.i2c, &target->node);

	if (api->attach_i2c_device != NULL) {
		status = api->attach_i2c_device(target->bus, target);
	}

	i3c_addr_slots_mark_i2c(&data->attached_dev.addr_slots, target->addr);

	return status;
}

int i3c_detach_i2c_device(struct i3c_i2c_device_desc *target)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)target->bus->data;
	const struct i3c_driver_api *api = (const struct i3c_driver_api *)target->bus->api;
	int status = 0;

	if (!sys_slist_is_empty(&data->attached_dev.devices.i2c)) {
		if (!sys_slist_find_and_remove(&data->attached_dev.devices.i2c, &target->node)) {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	if (api->detach_i2c_device != NULL) {
		status = api->detach_i2c_device(target->bus, target);
	}

	i3c_addr_slots_mark_free(&data->attached_dev.addr_slots, target->addr);

	return status;
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

	if (desc != NULL && desc->dynamic_addr != 0U) {
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
	if (desc != NULL && desc->init_dynamic_addr != 0U) {
		if (i3c_addr_slots_is_free(addr_slots, desc->init_dynamic_addr)) {
			dyn_addr = desc->init_dynamic_addr;
			goto out;
		}
	}

	/*
	 * Find the next available address.
	 */
	dyn_addr = i3c_addr_slots_next_free_find(addr_slots, 0);

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
	if (i3c_ccc_do_getmrl(target, &mrl) != 0) {
		/* GETMRL may be optionally supported if no settable limit */
		LOG_DBG("No settable limit for GETMRL");
	}

	/* GETMWL */
	if (i3c_ccc_do_getmwl(target, &mwl) != 0) {
		/* GETMWL may be optionally supported if no settable limit */
		LOG_DBG("No settable limit for GETMWL");
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

		LOG_DBG("SETDASA for 0x%x", desc->static_addr);

		ret = i3c_ccc_do_setdasa(desc);
		if (ret == 0) {
			desc->dynamic_addr = (desc->init_dynamic_addr ? desc->init_dynamic_addr
								      : desc->static_addr);
			if (desc->dynamic_addr != desc->static_addr) {
				if (i3c_reattach_i3c_device(desc, desc->static_addr) != 0) {
					LOG_ERR("Failed to reattach %s (%d)", desc->dev->name, ret);
				}
			}
		} else {
			/* SETDASA failed, detach it from the controller */
			if (i3c_detach_i3c_device(desc) != 0) {
				LOG_ERR("Failed to detach %s (%d)", desc->dev->name, ret);
			}
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

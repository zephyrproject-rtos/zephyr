/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/i3c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c, CONFIG_I3C_LOG_LEVEL);

void i3c_dump_msgs(const char *name, const struct i3c_msg *msgs, uint8_t num_msgs,
		   struct i3c_device_desc *target)
{
	LOG_DBG("I3C msg: %s, addr=%x", name, target->dynamic_addr);
	for (unsigned int i = 0; i < num_msgs; i++) {
		const struct i3c_msg *msg = &msgs[i];

		LOG_DBG("   %c len=%02x: ", msg->flags & I3C_MSG_READ ? 'R' : 'W', msg->len);
		if (!(msg->flags & I3C_MSG_READ)) {
			LOG_HEXDUMP_DBG(msg->buf, msg->len, "contents:");
		}
	}
}

void i3c_addr_slots_set(struct i3c_addr_slots *slots, uint8_t dev_addr,
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
	bitpos %= BITS_PER_LONG;

	slots->slots[idx] &= ~((unsigned long)I3C_ADDR_SLOT_STATUS_MASK << bitpos);
	slots->slots[idx] |= status << bitpos;
}

enum i3c_addr_slot_status i3c_addr_slots_status(struct i3c_addr_slots *slots, uint8_t dev_addr)
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
	bitpos %= BITS_PER_LONG;

	status = slots->slots[idx] >> bitpos;
	status &= I3C_ADDR_SLOT_STATUS_MASK;

	return status;
}

int i3c_addr_slots_init(const struct device *dev)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)dev->data;
	const struct i3c_driver_config *config = (const struct i3c_driver_config *)dev->config;
	int i, ret = 0;
	struct i3c_device_desc *i3c_dev;
	struct i3c_i2c_device_desc *i2c_dev;

	__ASSERT_NO_MSG(dev != NULL);

	(void)memset(&data->attached_dev.addr_slots, 0, sizeof(data->attached_dev.addr_slots));
	sys_slist_init(&data->attached_dev.devices.i3c);
	sys_slist_init(&data->attached_dev.devices.i2c);

	/* Address restrictions (ref 5.1.2.2.5, Specification for I3C v1.1.1) */
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

bool i3c_addr_slots_is_free(struct i3c_addr_slots *slots, uint8_t dev_addr)
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

struct i3c_device_desc *i3c_dev_list_i3c_addr_find(const struct device *dev, uint8_t addr)
{
	struct i3c_device_desc *ret = NULL;
	struct i3c_device_desc *desc;

	__ASSERT_NO_MSG(dev != NULL);

	I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
		if (desc->dynamic_addr == addr) {
			ret = desc;
			break;
		}
	}

	return ret;
}

struct i3c_device_desc *i3c_dev_list_i3c_static_addr_find(const struct device *dev, uint8_t addr)
{
	struct i3c_device_desc *ret = NULL;
	struct i3c_device_desc *desc;

	__ASSERT_NO_MSG(dev != NULL);

	I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
		if (desc->static_addr == addr) {
			ret = desc;
			break;
		}
	}

	return ret;
}

struct i3c_i2c_device_desc *i3c_dev_list_i2c_addr_find(const struct device *dev, uint16_t addr)
{
	struct i3c_i2c_device_desc *ret = NULL;
	struct i3c_i2c_device_desc *desc;

	__ASSERT_NO_MSG(dev != NULL);

	I3C_BUS_FOR_EACH_I2CDEV(dev, desc) {
		if (desc->addr == addr) {
			ret = desc;
			break;
		}
	}

	return ret;
}

int i3c_attach_i3c_device(struct i3c_device_desc *target)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)target->bus->data;
	const struct i3c_driver_api *api = (const struct i3c_driver_api *)target->bus->api;
	uint8_t addr = 0;
	int status = 0;
	struct i3c_device_desc *i3c_desc;

	/* check to see if the device has already been attached */
	I3C_BUS_FOR_EACH_I3CDEV(target->bus, i3c_desc) {
		if (i3c_desc == target) {
			return -EINVAL;
		}
	}

	addr = target->dynamic_addr ? target->dynamic_addr : target->static_addr;

	/*
	 * If it has a dynamic addr already assigned or a static address, check that it is free
	 */
	if (addr) {
		if (!i3c_addr_slots_is_free(&data->attached_dev.addr_slots, addr)) {
			return -EINVAL;
		}
	}

	sys_slist_append(&data->attached_dev.devices.i3c, &target->node);

	if (api->attach_i3c_device != NULL) {
		status = api->attach_i3c_device(target->bus, target);
	}

	if (addr) {
		i3c_addr_slots_mark_i3c(&data->attached_dev.addr_slots, addr);
	}

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

	/* if it was from allocated memory, free it */
	if (i3c_device_desc_in_pool(target)) {
		i3c_device_desc_free(target);
	}

	return status;
}

int i3c_attach_i2c_device(struct i3c_i2c_device_desc *target)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)target->bus->data;
	const struct i3c_driver_api *api = (const struct i3c_driver_api *)target->bus->api;
	int status = 0;
	struct i3c_i2c_device_desc *i3c_i2c_desc;

	/* check to see if the device has already been attached */
	I3C_BUS_FOR_EACH_I2CDEV(target->bus, i3c_i2c_desc) {
		if (i3c_i2c_desc == target) {
			return -EINVAL;
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

	/* if it was from allocated memory, free it */
	if (i3c_i2c_device_desc_in_pool(target)) {
		i3c_i2c_device_desc_free(target);
	}

	return status;
}

int i3c_sec_get_basic_info(const struct device *dev, uint8_t dynamic_addr, uint8_t static_addr,
			   uint8_t bcr, uint8_t dcr)
{
	struct i3c_ccc_getpid getpid;
	struct i3c_device_desc temp_desc;
	struct i3c_device_desc *desc;
	struct i3c_device_id id;
	const struct i3c_driver_config *config = dev->config;
	int ret;

	temp_desc.bus = dev;
	temp_desc.dynamic_addr = dynamic_addr;
	temp_desc.bcr = bcr;
	temp_desc.dcr = dcr;
	/* attach it first with a temporary value so we can at least get the pid */
	ret = i3c_attach_i3c_device(&temp_desc);
	if (ret != 0) {
		return ret;
	}

	/* First try to look up if this is a known device in the list by PID */
	ret = i3c_ccc_do_getpid(&temp_desc, &getpid);
	if (ret != 0) {
		return ret;
	}

	id.pid = sys_get_be48(getpid.pid);

	/* try to see if we already have a device statically allocated */
	desc = i3c_dev_list_find(&config->dev_list, &id);
	if (!desc) {
		/* device was not found so allocate a descriptor */
		desc = i3c_device_desc_alloc();
		if (!desc) {
			return -ENOMEM;
		}
		desc->pid = id.pid;
		temp_desc.static_addr = (uint16_t)static_addr;
	}
	desc->dynamic_addr = dynamic_addr;
	desc->bcr = bcr;
	desc->dcr = dcr;

	/* Detach that temporary device */
	ret = i3c_detach_i3c_device(&temp_desc);
	if (ret != 0) {
		return ret;
	}
	ret = i3c_attach_i3c_device(desc);
	if (ret != 0) {
		return ret;
	}

	/* Skip reading BCR and DCR as they came from DEFTGTS */
	ret = i3c_device_adv_info_get(desc);

	return ret;
}

int i3c_sec_i2c_attach(const struct device *dev, uint8_t static_addr, uint8_t lvr)
{
	struct i3c_i2c_device_desc *i2c_desc;
	int ret;

	/* try to see if we already have a device statically allocated */
	i2c_desc = i3c_dev_list_i2c_addr_find(dev, (uint16_t)static_addr);
	if (!i2c_desc) {
		/* device was not found so allocate a descriptor */
		i2c_desc = i3c_i2c_device_desc_alloc();
		if (!i2c_desc) {
			return -ENOMEM;
		}
		*(const struct device **)&i2c_desc->bus = dev;
		i2c_desc->addr = (uint16_t)static_addr;
		i2c_desc->lvr = lvr;
	}

	ret = i3c_attach_i2c_device(i2c_desc);
	return ret;
}

static void i3c_sec_bus_reset(const struct device *dev)
{
	struct i3c_device_desc *i3c_desc;
	struct i3c_i2c_device_desc *i3c_i2c_desc;

	I3C_BUS_FOR_EACH_I3CDEV(dev, i3c_desc) {
		i3c_detach_i3c_device(i3c_desc);
	}

	I3C_BUS_FOR_EACH_I2CDEV(dev, i3c_i2c_desc) {
		i3c_detach_i2c_device(i3c_i2c_desc);
	}
}
#ifdef CONFIG_I3C_USE_IBI
/* call this from a workq after the interrupt from a controller */
void i3c_sec_handoffed(struct k_work *work)
{
	struct i3c_ibi_work *ibi_node = CONTAINER_OF(work, struct i3c_ibi_work, work);
	const struct device *dev = ibi_node->controller;
	struct i3c_driver_data *data = (struct i3c_driver_data *)dev->data;
	struct i3c_ccc_deftgts *deftgts = data->deftgts;
	struct i3c_config_target config_target;
	uint8_t n, cur_dyn_addr;
	int ret;

	if (!deftgts) {
		LOG_ERR("Did not receive DEFTGTS before Handoff");
		return;
	}

	if (!data->deftgts_refreshed) {
		LOG_DBG("Already processed DEFTGTS from previous handoff");
		return;
	}

	/* Forget all devices as another controller made changes */
	i3c_sec_bus_reset(dev);

	/*
	 * Retrieve the active controller information
	 */
	ret = i3c_config_get_target(dev, &config_target);
	if (ret != 0) {
		LOG_ERR("Failed to retrieve active controller info");
		return;
	}

	cur_dyn_addr = config_target.dynamic_addr;

	/* Attach the previous AC */
	ret = i3c_sec_get_basic_info(
		dev, deftgts->active_controller.addr, deftgts->active_controller.static_addr,
		deftgts->active_controller.bcr, deftgts->active_controller.dcr);

	/* Attach all Targets */
	for (n = 0; n < deftgts->count; n++) {
		if (deftgts->targets[n].addr != 0) {
			/* Must be an I3C device and skip itself */
			if (deftgts->targets[n].addr != cur_dyn_addr) {
				ret = i3c_sec_get_basic_info(dev, deftgts->targets[n].addr,
							     deftgts->targets[n].static_addr,
							     deftgts->targets[n].bcr,
							     deftgts->targets[n].dcr);
			}
		} else {
			/* Must be an I2C device */
			ret = i3c_sec_i2c_attach(dev, deftgts->targets[n].static_addr,
						 deftgts->targets[n].lvr);
		}
	}

	/* Set false, so the next handoff doesn't retrigger regathering info */
	data->deftgts_refreshed = false;
}
#endif

int i3c_dev_list_daa_addr_helper(struct i3c_addr_slots *addr_slots,
				 const struct i3c_dev_list *dev_list, uint64_t pid, bool must_match,
				 bool assigned_okay, struct i3c_device_desc **target, uint8_t *addr)
{
	struct i3c_device_desc *desc;
	const uint16_t vendor_id = (uint16_t)(pid >> 32);
	const uint32_t part_no = (uint32_t)(pid & 0xFFFFFFFFU);
	uint8_t dyn_addr = 0;
	int ret = 0;
	const struct i3c_device_id i3c_id = I3C_DEVICE_ID(pid);

	desc = i3c_dev_list_find(dev_list, &i3c_id);
	/* If a device was not found, try to allocate a descriptor */
	if (desc == NULL) {
		desc = i3c_device_desc_alloc();
	}
	if (must_match && (desc == NULL)) {
		/*
		 * No device descriptor matching incoming PID and
		 * that we want an exact match.
		 */
		ret = -ENODEV;

		LOG_DBG("PID 0x%04x%08x is not in registered device list", vendor_id, part_no);

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

uint8_t i3c_odd_parity(uint8_t p)
{
	p ^= p >> 4;
	p &= 0xf;
	return (0x9669 >> p) & 1;
}

int i3c_device_controller_handoff(const struct i3c_device_desc *target, bool requested)
{
	int ret;
	union i3c_ccc_getstatus status = {0};
	struct i3c_ccc_events i3c_events;
	struct i3c_ccc_address handoff_address;

	/*
	 * If the Active Controller intends to pass the Controller Role to a selected Secondary
	 * Controller that did not send a Controller Role Request, then the Active Controller should
	 * verify that the selected Secondary Controller is active and ready to respond to
	 * additional commands
	 */
	if (!requested) {
		ret = i3c_ccc_do_getstatus_fmt1(target, &status);
		if (ret != 0) {
			return ret;
		}

		if (I3C_CCC_GETSTATUS_ACTIVITY_MODE(status.fmt1.status) ==
		    I3C_CCC_GETSTATUS_ACTIVITY_MODE_NCH) {
			return -EBUSY;
		}
	}

	/*
	 * The Active Controller needs to disable Hot-Joins, Target Interrupt Requests, and other
	 * Bus events that could interfere with the Handoff, then it sends the appropriate
	 * Broadcast to disable those events before the Handoff. Once the Handoff is complete, the
	 * new Active Controller should re-enable events that are disabled in this step.
	 */
	i3c_events.events = I3C_CCC_EVT_ALL;
	ret = i3c_ccc_do_events_all_set(target->bus, false, &i3c_events);
	if (ret != 0) {
		return ret;
	}

	/** TODO: reconfigure MLANE if needed */

	/*
	 * If the Active Controller knows that the selected Secondary Controller must be put into a
	 * different Activity State before Handoff, then the Active Controller shall send the
	 * appropriate Broadcast or Direct CCCs to put the Bus (or selected Devices) into a
	 * different Activity State
	 */
	if (target->crhdly1 & I3C_CCC_GETMXDS_CRDHLY1_SET_BUS_ACT_STATE) {
		ret = i3c_ccc_do_entas(
			target, I3C_CCC_GETMXDS_CRDHLY1_CTRL_HANDOFF_ACT_STATE(target->crhdly1));
		if (ret != 0) {
			return ret;
		}
	}

	if ((target->getcaps.getcap3 & I3C_CCC_GETCAPS3_GETSTATUS_DEFINING_BYTE_SUPPORT) &&
	    (target->crcaps.crcaps2 & I3C_CCC_GETCAPS_CRCAPS2_DEEP_SLEEP_CAPABLE)) {
		ret = i3c_ccc_do_getstatus_fmt2(target, &status, GETSTATUS_FORMAT_2_PRECR);
		if (ret != 0) {
			return ret;
		}

		/*
		 * If the Active Controller determines that the indicated Secondary Controller has
		 * been in a “deep sleep” state and may need to be re-synchronized with the most
		 * current list of I3C Targets and Group Addresses, then the Active Controller
		 * should send CCC DEFTGTS and DEFGRPA
		 */
		if (status.fmt2.precr & I3C_CCC_GETSTATUS_PRECR_DEEP_SLEEP_DETECTED) {
			ret = i3c_bus_deftgts(target->bus);
			if (ret != 0) {
				return ret;
			}
			/* TODO: broadcast DEFGRPA when group address support comes */

			/* Check CRCAPS if the device needs additional time to process */
			if (target->crcaps.crcaps2 &
			    I3C_CCC_GETCAPS_CRCAPS2_DELAYED_CONTROLLER_HANDOFF) {
				/*
				 * Afterwards, the Active Controller should poll the Secondary
				 * Controller to ensure that it has successfully processed this data
				 * and indicates that it is ready to accept the Controller Role
				 */
				do {
					ret = i3c_ccc_do_getstatus_fmt2(target, &status,
									GETSTATUS_FORMAT_2_PRECR);
					if (ret != 0) {
						return ret;
					}
				} while (!(status.fmt2.precr &
					   I3C_CCC_GETSTATUS_PRECR_HANDOFF_DELAY_NACK));
			}
		}
	}

	/*
	 * After the Active Controller has prepared for Handoff, the Active Controller shall
	 * then issue a GETACCCR CCC
	 */
	ret = i3c_ccc_do_getacccr(target, &handoff_address);
	if (ret != 0) {
		return ret;
	}

	/* Verify Odd Parity and Correct Dynamic Address Reply */
	if ((i3c_odd_parity(handoff_address.addr >> 1) != (handoff_address.addr & BIT(0))) ||
	    (handoff_address.addr >> 1 != target->dynamic_addr)) {
		return -EIO;
	}

	return ret;
}

int i3c_device_basic_info_get(struct i3c_device_desc *target)
{
	int ret;
	struct i3c_ccc_getbcr bcr = {0};
	struct i3c_ccc_getdcr dcr = {0};

	/* GETBCR */
	ret = i3c_ccc_do_getbcr(target, &bcr);
	if (ret != 0) {
		return ret;
	}

	/* GETDCR */
	ret = i3c_ccc_do_getdcr(target, &dcr);
	if (ret != 0) {
		return ret;
	}

	target->bcr = bcr.bcr;
	target->dcr = dcr.dcr;

	return 0;
}

int i3c_device_adv_info_get(struct i3c_device_desc *target)
{
	struct i3c_ccc_mrl mrl = {0};
	struct i3c_ccc_mwl mwl = {0};
	union i3c_ccc_getcaps caps = {0};
	union i3c_ccc_getmxds mxds = {0};
	int ret;

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

	/* GETCAPS */
	ret = i3c_ccc_do_getcaps_fmt1(target, &caps);
	/*
	 * GETCAPS (GETHDRCAP) is required to be supported for I3C v1.0 targets that support HDR
	 * modes and required if the Target's I3C version is v1.1 or later, but which the version it
	 * supports it can't be known ahead of time. So if the BCR bit for Advanced capabilities is
	 * set, then it is expected for GETCAPS to always be supported. Otherwise, then it's a I3C
	 * v1.0 device without any HDR modes so do not treat as an error if no valid response.
	 */
	if ((ret != 0) && (target->bcr & I3C_BCR_ADV_CAPABILITIES)) {
		return ret;
	} else {
		ret = 0;
	}

	/* CRCAPS */
	if ((target->getcaps.getcap3 & I3C_CCC_GETCAPS3_GETCAPS_DEFINING_BYTE_SUPPORT) &&
	    (i3c_device_is_controller_capable(target))) {
		ret = i3c_ccc_do_getcaps_fmt2(target, &caps, GETCAPS_FORMAT_2_CRCAPS);
		if (ret != 0) {
			return ret;
		}
	}

	/* GETMXDS */
	if (target->bcr & I3C_BCR_MAX_DATA_SPEED_LIMIT) {
		ret = i3c_ccc_do_getmxds_fmt2(target, &mxds);
		if (ret != 0) {
			return ret;
		}

		/* Get CRHDLY if supported */
		if ((target->data_speed.maxwr & I3C_CCC_GETMXDS_MAXWR_DEFINING_BYTE_SUPPORT) &&
		    (i3c_device_is_controller_capable(target))) {
			ret = i3c_ccc_do_getmxds_fmt3(target, &mxds, GETMXDS_FORMAT_3_CRHDLY);
			if (ret != 0) {
				return ret;
			}

			target->crhdly1 = mxds.fmt3.crhdly1;
		}
	}

	target->data_length.mrl = mrl.len;
	target->data_length.mwl = mwl.len;
	target->data_length.max_ibi = mrl.ibi_len;

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
static int i3c_bus_setdasa(const struct device *dev, const struct i3c_dev_list *dev_list,
			   bool *need_daa, bool *need_aasa)
{
	int i, ret;

	*need_daa = false;
	*need_aasa = false;

	/* Loop through the registered I3C devices */
	for (i = 0; i < dev_list->num_i3c; i++) {
		struct i3c_device_desc *desc = &dev_list->i3c[i];
		struct i3c_driver_data *bus_data = (struct i3c_driver_data *)dev->data;
		struct i3c_ccc_address dyn_addr;

		/*
		 * A device without static address => need to do
		 * dynamic address assignment.
		 */
		if (desc->static_addr == 0U) {
			*need_daa = true;
			continue;
		}

		/*
		 * A device that supports SETAASA and will use the same dynamic
		 * address as its static address if a different dynamic address
		 * is not requested
		 */
		if ((desc->supports_setaasa) && ((desc->init_dynamic_addr == 0) ||
						 desc->init_dynamic_addr == desc->static_addr)) {
			*need_aasa = true;
			continue;
		}

		LOG_DBG("SETDASA for 0x%x", desc->static_addr);

		/*
		 * check that initial dynamic address is free before setting it
		 * if configured
		 */
		if ((desc->init_dynamic_addr != 0) &&
		    (desc->init_dynamic_addr != desc->static_addr)) {
			if (!i3c_addr_slots_is_free(&bus_data->attached_dev.addr_slots,
						    desc->init_dynamic_addr)) {
				if (i3c_detach_i3c_device(desc) != 0) {
					LOG_ERR("Failed to detach %s", desc->dev->name);
				}
				continue;
			}
		}

		/*
		 * Note that the 7-bit address needs to start at bit 1
		 * (aka left-justified). So shift left by 1;
		 */
		dyn_addr.addr =
			(desc->init_dynamic_addr ? desc->init_dynamic_addr : desc->static_addr)
			<< 1;

		ret = i3c_ccc_do_setdasa(desc, dyn_addr);
		if (ret == 0) {
			desc->dynamic_addr = dyn_addr.addr >> 1;
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
			LOG_ERR("SETDASA error on address 0x%x (%d)", desc->static_addr, ret);
		}
	}

	return 0;
}

bool i3c_bus_has_sec_controller(const struct device *dev)
{
	struct i3c_device_desc *i3c_desc;

	I3C_BUS_FOR_EACH_I3CDEV(dev, i3c_desc) {
		if (i3c_device_is_controller_capable(i3c_desc)) {
			return true;
		}
	}

	return false;
}

int i3c_bus_deftgts(const struct device *dev)
{
	struct i3c_driver_data *data = (struct i3c_driver_data *)dev->data;
	struct i3c_config_target config_target;
	struct i3c_ccc_deftgts *deftgts;
	struct i3c_device_desc *i3c_desc;
	struct i3c_i2c_device_desc *i3c_i2c_desc;
	int ret;
	uint8_t n = 0;
	size_t num_of_targets = sys_slist_len(&data->attached_dev.devices.i3c) +
				sys_slist_len(&data->attached_dev.devices.i2c);
	size_t data_len = sizeof(uint8_t) + sizeof(struct i3c_ccc_deftgts_active_controller) +
			  (num_of_targets * sizeof(struct i3c_ccc_deftgts_target));

	/*
	 * Retrieve the active controller information
	 */
	ret = i3c_config_get(dev, I3C_CONFIG_TARGET, &config_target);
	if (ret != 0) {
		LOG_ERR("Failed to retrieve active controller info");
		return ret;
	}

	/* Allocate memory for the struct with enough space for the targets */
	deftgts = malloc(data_len);
	if (!deftgts) {
		return -ENOMEM;
	}

	/*
	 * Write the total number of I3C and I2C targets to the payload
	 */
	deftgts->count = num_of_targets;

	/*
	 * Add the active controller information to the payload
	 */
	deftgts->active_controller.addr = config_target.dynamic_addr << 1;
	deftgts->active_controller.dcr = config_target.dcr;
	deftgts->active_controller.bcr = config_target.bcr;
	deftgts->active_controller.static_addr = I3C_BROADCAST_ADDR << 1;

	/*
	 * Loop through each attached I3C device and add it to the payload
	 */
	I3C_BUS_FOR_EACH_I3CDEV(dev, i3c_desc) {
		deftgts->targets[n].addr = i3c_desc->dynamic_addr << 1;
		deftgts->targets[n].dcr = i3c_desc->dcr;
		deftgts->targets[n].bcr = i3c_desc->bcr;
		deftgts->targets[n].static_addr = i3c_desc->static_addr << 1;
		n++;
	}

	/*
	 * Loop through each attached I2C device and add it to the payload
	 */
	I3C_BUS_FOR_EACH_I2CDEV(dev, i3c_i2c_desc) {
		deftgts->targets[n].addr = 0;
		deftgts->targets[n].lvr = i3c_i2c_desc->lvr;
		deftgts->targets[n].bcr = 0;
		deftgts->targets[n].static_addr = (uint8_t)(i3c_i2c_desc->addr << 1);
		n++;
	}

	/* TODO: add support for Group Addr in DEFTGTS when that comes */

	ret = i3c_ccc_do_deftgts_all(dev, deftgts);

	free(deftgts);

	return ret;
}

int i3c_bus_init(const struct device *dev, const struct i3c_dev_list *dev_list)
{
	int i, ret = 0;
	bool need_daa = true;
	bool need_aasa = true;
	struct i3c_ccc_events i3c_events;

#ifdef CONFIG_I3C_INIT_RSTACT
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
#endif

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
	ret = i3c_bus_setdasa(dev, dev_list, &need_daa, &need_aasa);
	if (ret != 0) {
		goto err_out;
	}

	/*
	 * Perform Set All Addresses to Static Address if possible.
	 */
	if (need_aasa) {
		ret = i3c_ccc_do_setaasa_all(dev);
		if (ret != 0) {
			LOG_ERR("failed to perform setaasa");
		} else {
			for (i = 0; i < dev_list->num_i3c; i++) {
				struct i3c_device_desc *desc = &dev_list->i3c[i];
				/*
				 * Only set for devices that support SETAASA and do not
				 * request a different dynamic address than its SA
				 */
				if ((desc->supports_setaasa) && (desc->static_addr != 0) &&
				    ((desc->init_dynamic_addr == 0) ||
				     desc->init_dynamic_addr == desc->static_addr)) {
					desc->dynamic_addr = desc->static_addr;
				}
			}
		}
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

		/*
		 * If static address is 0, then it is assumed that BCR
		 * and DCR were already read through ENTDAA
		 */
		ret = (desc->static_addr == 0) ? i3c_device_adv_info_get(desc)
					       : i3c_device_info_get(desc);
		if (ret != 0) {
			LOG_ERR("Error getting device info for 0x%02x", desc->static_addr);
		} else {
			LOG_DBG("Target 0x%02x, BCR 0x%02x, DCR 0x%02x, MRL %d, MWL %d, IBI %d",
				desc->dynamic_addr, desc->bcr, desc->dcr, desc->data_length.mrl,
				desc->data_length.mwl, desc->data_length.max_ibi);
		}
	}

	if (i3c_bus_has_sec_controller(dev)) {
		ret = i3c_bus_deftgts(dev);
		if (ret != 0) {
			LOG_ERR("Error sending DEFTGTS");
		}
	}

	/*
	 * Only re-enable Hot-Join from targets.
	 * Target interrupts will be enabled when IBI is enabled.
	 */
	i3c_events.events = I3C_CCC_EVT_HJ;
	ret = i3c_ccc_do_events_all_set(dev, true, &i3c_events);
	if (ret != 0) {
		LOG_DBG("Broadcast ENEC was NACK.");
	}

err_out:
	return ret;
}

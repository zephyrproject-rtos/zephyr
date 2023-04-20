/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/drivers/i3c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(i3c, CONFIG_I3C_LOG_LEVEL);

int i3c_ccc_do_getbcr(struct i3c_device_desc *target,
		      struct i3c_ccc_getbcr *bcr)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);
	__ASSERT_NO_MSG(bcr != NULL);

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = &bcr->bcr;
	ccc_tgt_payload.data_len = sizeof(bcr->bcr);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_GETBCR;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_getdcr(struct i3c_device_desc *target,
		      struct i3c_ccc_getdcr *dcr)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);
	__ASSERT_NO_MSG(dcr != NULL);

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = &dcr->dcr;
	ccc_tgt_payload.data_len = sizeof(dcr->dcr);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_GETDCR;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_getpid(struct i3c_device_desc *target,
		      struct i3c_ccc_getpid *pid)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);
	__ASSERT_NO_MSG(pid != NULL);

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = &pid->pid[0];
	ccc_tgt_payload.data_len = sizeof(pid->pid);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_GETPID;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_rstact_all(const struct device *controller,
			  enum i3c_ccc_rstact_defining_byte action)
{
	struct i3c_ccc_payload ccc_payload;
	uint8_t def_byte;

	__ASSERT_NO_MSG(controller != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_RSTACT(true);

	def_byte = (uint8_t)action;
	ccc_payload.ccc.data = &def_byte;
	ccc_payload.ccc.data_len = 1U;

	return i3c_do_ccc(controller, &ccc_payload);
}

int i3c_ccc_do_rstdaa_all(const struct device *controller)
{
	struct i3c_ccc_payload ccc_payload;

	__ASSERT_NO_MSG(controller != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_RSTDAA;

	return i3c_do_ccc(controller, &ccc_payload);
}

int i3c_ccc_do_setdasa(const struct i3c_device_desc *target)
{
	struct i3c_driver_data *bus_data = (struct i3c_driver_data *)target->bus->data;
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	uint8_t dyn_addr;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);

	if ((target->static_addr == 0U) || (target->dynamic_addr != 0U)) {
		return -EINVAL;
	}

	/*
	 * Note that the 7-bit address needs to start at bit 1
	 * (aka left-justified). So shift left by 1;
	 */
	dyn_addr = (target->init_dynamic_addr ?
				target->init_dynamic_addr : target->static_addr) << 1;

	/* check that initial dynamic address is free before setting it */
	if ((target->init_dynamic_addr != 0) &&
		(target->init_dynamic_addr != target->static_addr)) {
		if (!i3c_addr_slots_is_free(&bus_data->attached_dev.addr_slots, dyn_addr >> 1)) {
			return -EINVAL;
		}
	}

	ccc_tgt_payload.addr = target->static_addr;
	ccc_tgt_payload.rnw = 0;
	ccc_tgt_payload.data = &dyn_addr;
	ccc_tgt_payload.data_len = 1;

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_SETDASA;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_setnewda(const struct i3c_device_desc *target, struct i3c_ccc_address new_da)
{
	struct i3c_driver_data *bus_data = (struct i3c_driver_data *)target->bus->data;
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	uint8_t new_dyn_addr;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);

	if (target->dynamic_addr == 0U) {
		return -EINVAL;
	}

	/*
	 * Note that the 7-bit address needs to start at bit 1
	 * (aka left-justified). So shift left by 1;
	 */
	new_dyn_addr = new_da.addr << 1;

	/* check that initial dynamic address is free before setting it */
	if (target->dynamic_addr != new_da.addr) {
		if (!i3c_addr_slots_is_free(&bus_data->attached_dev.addr_slots, new_da.addr)) {
			return -EINVAL;
		}
	}

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 0;
	ccc_tgt_payload.data = &new_dyn_addr;
	ccc_tgt_payload.data_len = 1;

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_SETNEWDA;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_events_all_set(const struct device *controller,
			      bool enable, struct i3c_ccc_events *events)
{
	struct i3c_ccc_payload ccc_payload;

	__ASSERT_NO_MSG(controller != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));

	ccc_payload.ccc.id = enable ? I3C_CCC_ENEC(true) : I3C_CCC_DISEC(true);

	ccc_payload.ccc.data = &events->events;
	ccc_payload.ccc.data_len = sizeof(events->events);

	return i3c_do_ccc(controller, &ccc_payload);
}

int i3c_ccc_do_events_set(struct i3c_device_desc *target,
			  bool enable, struct i3c_ccc_events *events)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);

	if (target->dynamic_addr == 0U) {
		return -EINVAL;
	}

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 0;
	ccc_tgt_payload.data = &events->events;
	ccc_tgt_payload.data_len = sizeof(events->events);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = enable ? I3C_CCC_ENEC(false) : I3C_CCC_DISEC(false);
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_setmwl_all(const struct device *controller,
			  const struct i3c_ccc_mwl *mwl)
{
	struct i3c_ccc_payload ccc_payload;
	uint8_t data[2];

	__ASSERT_NO_MSG(controller != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));

	ccc_payload.ccc.id = I3C_CCC_SETMWL(true);

	ccc_payload.ccc.data = &data[0];
	ccc_payload.ccc.data_len = sizeof(data);

	/* The actual data is MSB first. So order the data. */
	data[0] = (uint8_t)((mwl->len & 0xFF00U) >> 8);
	data[1] = (uint8_t)(mwl->len & 0xFFU);

	return i3c_do_ccc(controller, &ccc_payload);
}

int i3c_ccc_do_setmwl(const struct i3c_device_desc *target,
		      const struct i3c_ccc_mwl *mwl)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	uint8_t data[2];

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 0;
	ccc_tgt_payload.data = &data[0];
	ccc_tgt_payload.data_len = sizeof(data);

	ccc_payload.ccc.id = I3C_CCC_SETMWL(false);
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	/* The actual length is MSB first. So order the data. */
	data[0] = (uint8_t)((mwl->len & 0xFF00U) >> 8);
	data[1] = (uint8_t)(mwl->len & 0xFFU);

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_getmwl(const struct i3c_device_desc *target,
		      struct i3c_ccc_mwl *mwl)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	uint8_t data[2];
	int ret;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);
	__ASSERT_NO_MSG(mwl != NULL);

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = &data[0];
	ccc_tgt_payload.data_len = sizeof(data);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_GETMWL;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	ret = i3c_do_ccc(target->bus, &ccc_payload);

	if (ret == 0) {
		/* The actual length is MSB first. So order the data. */
		mwl->len = (data[0] << 8) | data[1];
	}

	return ret;
}

int i3c_ccc_do_setmrl_all(const struct device *controller,
			  const struct i3c_ccc_mrl *mrl,
			  bool has_ibi_size)
{
	struct i3c_ccc_payload ccc_payload;
	uint8_t data[3];

	__ASSERT_NO_MSG(controller != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));

	ccc_payload.ccc.id = I3C_CCC_SETMRL(true);

	ccc_payload.ccc.data = &data[0];
	ccc_payload.ccc.data_len = has_ibi_size ? 3 : 2;

	/* The actual length is MSB first. So order the data. */
	data[0] = (uint8_t)((mrl->len & 0xFF00U) >> 8);
	data[1] = (uint8_t)(mrl->len & 0xFFU);

	if (has_ibi_size) {
		data[2] = mrl->ibi_len;
	}

	return i3c_do_ccc(controller, &ccc_payload);
}

int i3c_ccc_do_setmrl(const struct i3c_device_desc *target,
		      const struct i3c_ccc_mrl *mrl)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	uint8_t data[3];

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 0;
	ccc_tgt_payload.data = &data[0];

	ccc_payload.ccc.id = I3C_CCC_SETMRL(false);
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	/* The actual length is MSB first. So order the data. */
	data[0] = (uint8_t)((mrl->len & 0xFF00U) >> 8);
	data[1] = (uint8_t)(mrl->len & 0xFFU);

	if ((target->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE)
	    == I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE) {
		ccc_tgt_payload.data_len = 3;

		data[2] = mrl->ibi_len;
	} else {
		ccc_tgt_payload.data_len = 2;
	}

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_getmrl(const struct i3c_device_desc *target,
		      struct i3c_ccc_mrl *mrl)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	uint8_t data[3];
	bool has_ibi_sz;
	int ret;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);
	__ASSERT_NO_MSG(mrl != NULL);

	has_ibi_sz = (target->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE)
		     == I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE;

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = &data[0];
	ccc_tgt_payload.data_len = has_ibi_sz ? 3 : 2;

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_GETMRL;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	ret = i3c_do_ccc(target->bus, &ccc_payload);

	if (ret == 0) {
		/* The actual length is MSB first. So order the data. */
		mrl->len = (data[0] << 8) | data[1];

		if (has_ibi_sz) {
			mrl->ibi_len = data[2];
		}
	}

	return ret;
}

int i3c_ccc_do_getstatus(const struct i3c_device_desc *target,
			 union i3c_ccc_getstatus *status,
			 enum i3c_ccc_getstatus_fmt fmt,
			 enum i3c_ccc_getstatus_defbyte defbyte)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	uint8_t defining_byte;
	uint8_t data[2];
	int ret;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);
	__ASSERT_NO_MSG(status != NULL);

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = &data[0];

	if (fmt == GETSTATUS_FORMAT_1) {
		ccc_tgt_payload.data_len = 2;
	} else if (fmt == GETSTATUS_FORMAT_2) {
		switch (defbyte) {
		case GETSTATUS_FORMAT_2_TGTSTAT:
			__fallthrough;
		case GETSTATUS_FORMAT_2_PRECR:
			ccc_tgt_payload.data_len = 2;
			break;
		default:
			ret = -EINVAL;
			goto out;
		}
	} else {
		ret = -EINVAL;
		goto out;
	}

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_GETSTATUS;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	if (fmt == GETSTATUS_FORMAT_2) {
		defining_byte = (uint8_t)defbyte;

		ccc_payload.ccc.data = &defining_byte;
		ccc_payload.ccc.data_len = 1;
	}

	ret = i3c_do_ccc(target->bus, &ccc_payload);

	if (ret == 0) {
		/* Received data is MSB first. So order the data. */
		if (fmt == GETSTATUS_FORMAT_1) {
			status->fmt1.status = (data[0] << 8) | data[1];
		} else if (fmt == GETSTATUS_FORMAT_2) {
			switch (defbyte) {
			case GETSTATUS_FORMAT_2_TGTSTAT:
				__fallthrough;
			case GETSTATUS_FORMAT_2_PRECR:
				status->fmt2.raw_u16 = (data[0] << 8) | data[1];
				break;
			default:
				break;
			}
		}
	}

out:
	return ret;
}

/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/byteorder.h>
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

int i3c_ccc_do_rstact(const struct i3c_device_desc *target,
			  enum i3c_ccc_rstact_defining_byte action,
			  bool get,
			  uint8_t *data)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	uint8_t def_byte;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));

	ccc_tgt_payload.addr = target->dynamic_addr;
	if (get) {
		__ASSERT_NO_MSG(data != NULL);

		ccc_tgt_payload.rnw = 1;
		ccc_tgt_payload.data = data;
		ccc_tgt_payload.data_len = sizeof(*data);
	} else {
		ccc_tgt_payload.rnw = 0;
	}

	ccc_payload.ccc.id = I3C_CCC_RSTACT(false);
	def_byte = (uint8_t)action;
	ccc_payload.ccc.data = &def_byte;
	ccc_payload.ccc.data_len = 1U;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_rstdaa(struct i3c_device_desc *target)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data_len = 0;

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_RSTDAA_DC;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_rstdaa_all(const struct device *controller)
{
	struct i3c_ccc_payload ccc_payload;

	__ASSERT_NO_MSG(controller != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_RSTDAA;

	return i3c_do_ccc(controller, &ccc_payload);
}

int i3c_ccc_do_setdasa(const struct i3c_device_desc *target, struct i3c_ccc_address da)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);

	if ((target->static_addr == 0U) || (target->dynamic_addr != 0U)) {
		return -EINVAL;
	}

	ccc_tgt_payload.addr = target->static_addr;
	ccc_tgt_payload.rnw = 0;
	ccc_tgt_payload.data = (uint8_t *)&da.addr;
	ccc_tgt_payload.data_len = 1;

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_SETDASA;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_setnewda(const struct i3c_device_desc *target, struct i3c_ccc_address new_da)
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
	ccc_tgt_payload.data = (uint8_t *)&new_da.addr;
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

int i3c_ccc_do_entas(const struct i3c_device_desc *target, uint8_t as)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);
	__ASSERT_NO_MSG(as <= 3);

	memset(&ccc_payload, 0, sizeof(ccc_payload));

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 0;
	ccc_tgt_payload.data_len = 0;

	ccc_payload.ccc.id = I3C_CCC_ENTAS(as, false);
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_entas_all(const struct device *controller, uint8_t as)
{
	struct i3c_ccc_payload ccc_payload;

	__ASSERT_NO_MSG(controller != NULL);
	__ASSERT_NO_MSG(as <= 3);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_ENTAS(as, true);

	return i3c_do_ccc(controller, &ccc_payload);
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
	sys_put_be16(mwl->len, data);

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
	sys_put_be16(mwl->len, data);

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
		mwl->len = sys_get_be16(data);
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
	sys_put_be16(mrl->len, data);

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
	sys_put_be16(mrl->len, data);

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
		mrl->len = sys_get_be16(data);

		if (has_ibi_sz) {
			mrl->ibi_len = data[2];
		}
	}

	return ret;
}

int i3c_ccc_do_enttm(const struct device *controller,
			  enum i3c_ccc_enttm_defbyte defbyte)
{
	struct i3c_ccc_payload ccc_payload;
	uint8_t def_byte;

	__ASSERT_NO_MSG(controller != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_ENTTM;

	def_byte = (uint8_t)defbyte;
	ccc_payload.ccc.data = &def_byte;
	ccc_payload.ccc.data_len = 1U;

	return i3c_do_ccc(controller, &ccc_payload);
}

int i3c_ccc_do_deftgts_all(const struct device *controller,
			   struct i3c_ccc_deftgts *deftgts)
{
	struct i3c_ccc_payload ccc_payload;

	__ASSERT_NO_MSG(controller != NULL);
	__ASSERT_NO_MSG(deftgts != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_DEFTGTS;

	ccc_payload.ccc.data = (uint8_t *)deftgts;
	ccc_payload.ccc.data_len = sizeof(uint8_t) +
				   sizeof(struct i3c_ccc_deftgts_active_controller) +
				   (deftgts->count * sizeof(struct i3c_ccc_deftgts_target));

	return i3c_do_ccc(controller, &ccc_payload);
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
			status->fmt1.status = sys_get_be16(data);
		} else if (fmt == GETSTATUS_FORMAT_2) {
			switch (defbyte) {
			case GETSTATUS_FORMAT_2_TGTSTAT:
				__fallthrough;
			case GETSTATUS_FORMAT_2_PRECR:
				status->fmt2.raw_u16 = sys_get_be16(data);
				break;
			default:
				break;
			}
		}
	}

out:
	return ret;
}

int i3c_ccc_do_getcaps(const struct i3c_device_desc *target,
			 union i3c_ccc_getcaps *caps,
			 enum i3c_ccc_getcaps_fmt fmt,
			 enum i3c_ccc_getcaps_defbyte defbyte)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	uint8_t defining_byte;
	uint8_t data[4];
	uint8_t len;
	int ret;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);
	__ASSERT_NO_MSG(caps != NULL);

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = &data[0];

	if (fmt == GETCAPS_FORMAT_1) {
		/* Could be 1-4 Data Bytes Returned */
		ccc_tgt_payload.data_len = 4;
	} else if (fmt == GETCAPS_FORMAT_2) {
		switch (defbyte) {
		case GETCAPS_FORMAT_2_CRCAPS:
			__fallthrough;
		case GETCAPS_FORMAT_2_VTCAPS:
			/* Could be 1-2 Data Bytes Returned*/
			ccc_tgt_payload.data_len = 2;
			break;
		case GETCAPS_FORMAT_2_TGTCAPS:
			__fallthrough;
		case GETCAPS_FORMAT_2_TESTPAT:
			/* Could be 1-4 Data Bytes Returned */
			ccc_tgt_payload.data_len = 4;
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
	ccc_payload.ccc.id = I3C_CCC_GETCAPS;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	if (fmt == GETCAPS_FORMAT_2) {
		defining_byte = (uint8_t)defbyte;

		ccc_payload.ccc.data = &defining_byte;
		ccc_payload.ccc.data_len = 1;
	}

	ret = i3c_do_ccc(target->bus, &ccc_payload);

	if (ret == 0) {
		/* GETCAPS will return a variable length */
		len = ccc_tgt_payload.num_xfer;

		if (fmt == GETCAPS_FORMAT_1) {
			memcpy(caps->fmt1.getcaps, data, len);
			/* for values not received, assume default (1'b0) */
			memset(&caps->fmt1.getcaps[len], 0, sizeof(caps->fmt1.getcaps) - len);
		} else if (fmt == GETCAPS_FORMAT_2) {
			switch (defbyte) {
			case GETCAPS_FORMAT_2_CRCAPS:
				memcpy(caps->fmt2.crcaps, data, len);
				/* for values not received, assume default (1'b0) */
				memset(&caps->fmt2.crcaps[len], 0, sizeof(caps->fmt2.crcaps) - len);
				break;
			case GETCAPS_FORMAT_2_VTCAPS:
				memcpy(caps->fmt2.vtcaps, data, len);
				/* for values not received, assume default (1'b0) */
				memset(&caps->fmt2.vtcaps[len], 0, sizeof(caps->fmt2.vtcaps) - len);
				break;
			case GETCAPS_FORMAT_2_TGTCAPS:
				memcpy(caps->fmt2.tgtcaps, data, len);
				/* for values not received, assume default (1'b0) */
				memset(&caps->fmt2.tgtcaps[len], 0,
				       sizeof(caps->fmt2.tgtcaps) - len);
				break;
			case GETCAPS_FORMAT_2_TESTPAT:
				/* should always be 4 data bytes */
				caps->fmt2.testpat = sys_get_be32(data);
				break;
			default:
				break;
			}
		}
	}

out:
	return ret;
}

int i3c_ccc_do_setvendor(const struct i3c_device_desc *target,
			 uint8_t id,
			 uint8_t *payload,
			 size_t len)
{
	struct i3c_ccc_payload ccc_payload;

	__ASSERT_NO_MSG(target != NULL);

	/* CCC must be between 0xE0 and 0xFE, the total range of this is 0x1E */
	if (id > 0x1E) {
		return -EINVAL;
	}

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_VENDOR(false, id);
	ccc_payload.ccc.data = payload;
	ccc_payload.ccc.data_len = len;

	return i3c_do_ccc(target->bus, &ccc_payload);
}

int i3c_ccc_do_getvendor(const struct i3c_device_desc *target,
			 uint8_t id,
			 uint8_t *payload,
			 size_t len,
			 size_t *num_xfer)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	int ret;

	__ASSERT_NO_MSG(target != NULL);

	/* CCC must be between 0xE0 and 0xFE, the total range of this is 0x1E */
	if (id > 0x1E) {
		return -EINVAL;
	}

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = payload;
	ccc_tgt_payload.data_len = len;

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_VENDOR(false, id);
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	ret = i3c_do_ccc(target->bus, &ccc_payload);

	if (ret == 0) {
		*num_xfer = ccc_tgt_payload.num_xfer;
	}

	return ret;
}

int i3c_ccc_do_getvendor_defbyte(const struct i3c_device_desc *target,
				 uint8_t id,
				 uint8_t defbyte,
				 uint8_t *payload,
				 size_t len,
				 size_t *num_xfer)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	int ret;

	__ASSERT_NO_MSG(target != NULL);

	/* CCC must be between 0xE0 and 0xFE, the total range of this is 0x1E */
	if (id > 0x1E) {
		return -EINVAL;
	}

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = payload;
	ccc_tgt_payload.data_len = len;

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_VENDOR(false, id);
	ccc_payload.ccc.data = &defbyte;
	ccc_payload.ccc.data_len = 1;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	ret = i3c_do_ccc(target->bus, &ccc_payload);

	if (ret == 0) {
		*num_xfer = ccc_tgt_payload.num_xfer;
	}

	return ret;
}

int i3c_ccc_do_setvendor_all(const struct device *controller,
				 uint8_t id,
				 uint8_t *payload,
				 size_t len)
{
	struct i3c_ccc_payload ccc_payload;

	__ASSERT_NO_MSG(controller != NULL);

	/* CCC must be between 0x61 and 0x7F, the total range of this is 0x1E */
	if (id > 0x1E) {
		return -EINVAL;
	}

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_VENDOR(true, id);
	ccc_payload.ccc.data = payload;
	ccc_payload.ccc.data_len = len;

	return i3c_do_ccc(controller, &ccc_payload);
}

int i3c_ccc_do_setaasa_all(const struct device *controller)
{
	struct i3c_ccc_payload ccc_payload;

	__ASSERT_NO_MSG(controller != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_SETAASA;

	return i3c_do_ccc(controller, &ccc_payload);
}

int i3c_ccc_do_getmxds(const struct i3c_device_desc *target,
			 union i3c_ccc_getmxds *mxds,
			 enum i3c_ccc_getmxds_fmt fmt,
			 enum i3c_ccc_getmxds_defbyte defbyte)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	uint8_t defining_byte;
	uint8_t data[5];
	uint8_t len;
	int ret;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(target->bus != NULL);
	__ASSERT_NO_MSG(mxds != NULL);

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = &data[0];

	if ((fmt == GETMXDS_FORMAT_1) || (fmt == GETMXDS_FORMAT_2)) {
		/* Could be 2 or 5 Data Bytes Returned */
		ccc_tgt_payload.data_len = sizeof(((union i3c_ccc_getmxds *)0)->fmt2);
	} else if (fmt == GETMXDS_FORMAT_3) {
		switch (defbyte) {
		case GETMXDS_FORMAT_3_WRRDTURN:
			/* Could be 2 or 5 Data Bytes Returned */
			ccc_tgt_payload.data_len =
				sizeof(((union i3c_ccc_getmxds *)0)->fmt3.wrrdturn);
			break;
		case GETMXDS_FORMAT_3_CRHDLY:
			/* Only 1 Byte returned */
			ccc_tgt_payload.data_len =
				sizeof(((union i3c_ccc_getmxds *)0)->fmt3.crhdly1);
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
	ccc_payload.ccc.id = I3C_CCC_GETMXDS;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	if (fmt == GETMXDS_FORMAT_3) {
		defining_byte = (uint8_t)defbyte;

		ccc_payload.ccc.data = &defining_byte;
		ccc_payload.ccc.data_len = 1;
	}

	ret = i3c_do_ccc(target->bus, &ccc_payload);

	if (ret == 0) {
		/* GETMXDS will return a variable length */
		len = ccc_tgt_payload.num_xfer;

		if ((fmt == GETMXDS_FORMAT_1) || (fmt == GETMXDS_FORMAT_2)) {
			if (len == SIZEOF_FIELD(union i3c_ccc_getmxds, fmt1)) {
				mxds->fmt1.maxwr = data[0];
				mxds->fmt1.maxrd = data[1];
				/* It is unknown wither format 1 or format 2 is returned ahead of
				 * time
				 */
				memset(&mxds->fmt2.maxrdturn, 0, sizeof(mxds->fmt2.maxrdturn));
			} else if (len == SIZEOF_FIELD(union i3c_ccc_getmxds, fmt2)) {
				mxds->fmt2.maxwr = data[0];
				mxds->fmt2.maxrd = data[1];
				memcpy(&mxds->fmt2.maxrdturn, &data[2],
				       sizeof(mxds->fmt2.maxrdturn));
			}
		} else if (fmt == GETMXDS_FORMAT_3) {
			switch (defbyte) {
			case GETMXDS_FORMAT_3_WRRDTURN:
				memcpy(mxds->fmt3.wrrdturn, data, len);
				/* for values not received, assume default (1'b0) */
				memset(&mxds->fmt3.wrrdturn[len], 0,
				       sizeof(mxds->fmt3.wrrdturn) - len);
				break;
			case GETMXDS_FORMAT_3_CRHDLY:
				mxds->fmt3.crhdly1 = data[0];
				break;
			default:
				break;
			}
		}
	}

out:
	return ret;
}

int i3c_ccc_do_setbuscon(const struct device *controller,
				uint8_t *context, uint16_t length)
{
	struct i3c_ccc_payload ccc_payload;

	__ASSERT_NO_MSG(controller != NULL);
	__ASSERT_NO_MSG(context != NULL);

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_SETBUSCON;

	ccc_payload.ccc.data = context;
	ccc_payload.ccc.data_len = length;

	return i3c_do_ccc(controller, &ccc_payload);
}

int i3c_ccc_do_getacccr(const struct i3c_device_desc *target,
			 struct i3c_ccc_address *handoff_address)
{
	struct i3c_ccc_payload ccc_payload;
	struct i3c_ccc_target_payload ccc_tgt_payload;
	int ret;

	__ASSERT_NO_MSG(target != NULL);
	__ASSERT_NO_MSG(handoff_address != NULL);

	ccc_tgt_payload.addr = target->dynamic_addr;
	ccc_tgt_payload.rnw = 1;
	ccc_tgt_payload.data = &handoff_address->addr;
	ccc_tgt_payload.data_len = 1;

	memset(&ccc_payload, 0, sizeof(ccc_payload));
	ccc_payload.ccc.id = I3C_CCC_GETACCCR;
	ccc_payload.targets.payloads = &ccc_tgt_payload;
	ccc_payload.targets.num_targets = 1;

	ret = i3c_do_ccc(target->bus, &ccc_payload);

	return ret;
}

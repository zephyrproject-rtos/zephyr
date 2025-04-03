/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spinel_drv.h"

#include <openthread/platform/radio.h>

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spinel_drv);

#define PROP_OFFSET(t_id) ((t_id) - 1)

#define FRAME_FC0_OFFSET 0
#define FRAME_SN_OFFSET  2

#define FRAME_TYPE_MASK    0x07
#define FRAME_ACK_REQ_MASK 0x20
#define FRAME_TYPE_ACK     0x02

#define SPINEL_DRV_DEFAULT_MAX_FRAME_RETRIES 0
#define SPINEL_DRV_DEFAULT_MAX_CSMA_BACKOFFS 4

static enum ieee802154_hw_caps spinel_drv_get_hw_caps(otRadioCaps caps)
{
	enum ieee802154_hw_caps radio_caps = 0;

	if (caps & OT_RADIO_CAPS_ENERGY_SCAN) {
		radio_caps |= IEEE802154_HW_ENERGY_SCAN;
	}

	if (caps & OT_RADIO_CAPS_CSMA_BACKOFF) {
		radio_caps |= IEEE802154_HW_CSMA;
	}

	if (caps & OT_RADIO_CAPS_ACK_TIMEOUT) {
		radio_caps |= IEEE802154_HW_TX_RX_ACK;
	}

	if (caps & OT_RADIO_CAPS_SLEEP_TO_TX) {
		radio_caps |= IEEE802154_HW_SLEEP_TO_TX;
	}

	if (caps & OT_RADIO_CAPS_TRANSMIT_SEC) {
		radio_caps |= IEEE802154_HW_TX_SEC;
	}

	if (caps & OT_RADIO_CAPS_TRANSMIT_TIMING) {
		radio_caps |= IEEE802154_HW_TXTIME;
	}

	if (caps & OT_RADIO_CAPS_RECEIVE_TIMING) {
		radio_caps |= IEEE802154_HW_RXTIME;
	}

	if (caps & OT_RADIO_CAPS_RX_ON_WHEN_IDLE) {
		radio_caps |= IEEE802154_RX_ON_WHEN_IDLE;
	}

	if (caps & OT_RADIO_CAPS_TRANSMIT_RETRIES) {
		radio_caps |= IEEE802154_HW_RETRANSMISSION;
	}

	return radio_caps;
}

static int spinel_drv_set_t_id(struct spinel_drv_data *spinel_drv, uint32_t prop)
{
	uint8_t next_t_id;

	do {
		next_t_id = SPINEL_GET_NEXT_TID(spinel_drv->t_id.act_id);

		if (spinel_drv->t_id.act_id == next_t_id) {
			/* All TIDs are used */
			return -ENOMEM;
		}
	} while (spinel_drv->t_id.props[PROP_OFFSET(next_t_id)] != SPINEL_PROP_UNDEFINED);

	spinel_drv->t_id.props[PROP_OFFSET(next_t_id)] = prop;
	spinel_drv->t_id.act_id = next_t_id;

	return 0;
}

static int spinel_drv_send_cmd(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
			       const void *ctx, uint32_t cmd, uint32_t prop, const char *fmt, ...)
{
	int ret;
	va_list args;

	va_start(args, fmt);

	do {
		ret = spinel_drv_set_t_id(spinel_drv, prop);
		if (ret < 0) {
			LOG_ERR("Failed set next Transaction ID (inst = %u, err = %d)",
				spinel_drv->inst, ret);
			break;
		}

		ret = spinel_datatype_pack(tx_cb, ctx, SPINEL_DATATYPE_COMMAND_PROP_S,
					   SPINEL_HEADER_FLAG |
						   SPINEL_HEADER_IID(spinel_drv->inst) |
						   spinel_drv->t_id.act_id,
					   cmd, prop);

		if (ret < 0) {
			LOG_ERR("Failed pack spinel header (inst = %u, err = %d)", spinel_drv->inst,
				ret);
			break;
		}

		if (fmt) {
			int ret_vpack = spinel_datatype_vpack(tx_cb, ctx, fmt, &args);

			if (ret_vpack < 0) {
				ret = ret_vpack;
				LOG_ERR("Failed pack spinel data (inst = %u, err = %d)",
					spinel_drv->inst, ret);
				break;
			}

			ret += ret_vpack;
		}
	} while (0);

	va_end(args);
	return ret;
}

static int spinel_drv_get_cmd(struct spinel_drv_data *spinel_drv, uint32_t cmd, uint32_t prop,
			      const uint8_t *in_data, uint16_t in_data_size,
			      const uint8_t **p_out_data, uint16_t *p_out_data_size)
{
	int ret;
	uint8_t header;
	uint32_t unpacked_cmd;
	uint32_t unpacked_prop;

	do {
		ret = spinel_datatype_unpack(in_data, in_data_size, false,
					     SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_DATA_S,
					     &header, &unpacked_cmd, &unpacked_prop, p_out_data,
					     p_out_data_size);

		if (ret < 0) {
			LOG_ERR("Failed unpack spinel header and data (inst = %u, err = %d)",
				spinel_drv->inst, ret);
			break;
		}

		if (spinel_drv->inst != SPINEL_HEADER_GET_IID(header)) {
			/* Skip this frame */
			ret = -EPERM;
			break;
		}

		uint8_t t_id = SPINEL_HEADER_GET_TID(header);

		if (unpacked_cmd == cmd && unpacked_prop == prop) {
			if (t_id) {
				uint32_t *p_saved_prop = &spinel_drv->t_id.props[PROP_OFFSET(t_id)];

				if (!(cmd == SPINEL_CMD_PROP_VALUE_IS &&
				      prop == SPINEL_PROP_LAST_STATUS &&
				      *p_saved_prop != SPINEL_PROP_UNDEFINED) &&
				    (prop != *p_saved_prop)) {
					ret = -EIO;
					LOG_ERR("Unexpected Transaction ID (inst = %u)",
						spinel_drv->inst);
					break;
				}

				*p_saved_prop = SPINEL_PROP_UNDEFINED;
			}

			ret = 0;
			break;
		} else if (unpacked_cmd == SPINEL_CMD_PROP_VALUE_IS &&
			   unpacked_prop == SPINEL_PROP_LAST_STATUS && t_id &&
			   prop == spinel_drv->t_id.props[PROP_OFFSET(t_id)] &&
			   prop != SPINEL_PROP_STREAM_RAW) {
			uint32_t status;

			spinel_drv->t_id.props[PROP_OFFSET(t_id)] = SPINEL_PROP_UNDEFINED;

			ret = spinel_datatype_unpack(*p_out_data, *p_out_data_size, true,
						     SPINEL_DATATYPE_UINT_PACKED_S, &status);

			if (ret < 0) {
				LOG_ERR("Failed get result (inst = %u, err = %d)", spinel_drv->inst,
					ret);
				break;
			}

			if (status != SPINEL_STATUS_OK) {
				LOG_ERR("Incorrect response status (inst = %u, status = %u)",
					spinel_drv->inst, status);
				ret = -EIO;
				break;
			}
		}

		/* Skip this frame */
		ret = -EPERM;
	} while (0);

	if (ret < 0) {
		*p_out_data = NULL;
		*p_out_data_size = 0;
	}

	return ret;
}

static int spinel_drv_get_radio_frame(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				      uint16_t data_size, bool copy_buff,
				      struct spinel_frame_data *frame)
{
	int8_t noise_floor;
	uint16_t flags;
	uint8_t channel;
	uint64_t timestamp;
	uint32_t err;

	frame->data_length = OT_RADIO_FRAME_MAX_SIZE;

	int ret = spinel_datatype_unpack(
		data, data_size, copy_buff,
		SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_INT8_S SPINEL_DATATYPE_INT8_S
			SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S
				SPINEL_DATATYPE_UINT64_S SPINEL_DATATYPE_UINT_PACKED_S,
		copy_buff ? frame->data : (uint8_t *)&frame->data, &frame->data_length,
		&frame->rx.rssi, &noise_floor, &flags, &channel, &frame->rx.lqi, &timestamp, &err);

	if (ret < 0) {
		LOG_ERR("Failed get radio frame (inst = %u, err = %d)", spinel_drv->inst, ret);
		return ret;
	}

	if (err) {
		LOG_ERR("Get incorrect error value in get_radio_frame (inst = %u, err = %d)",
			spinel_drv->inst, err);
		return -EIO;
	}

	frame->rx.frame_pending = flags & SPINEL_MD_FLAG_ACKED_FP;

	return ret;
}

void spinel_drv_init(struct spinel_drv_data *spinel_drv, uint8_t inst)
{
	spinel_drv->inst = inst;
	spinel_drv->t_id.act_id = 0;

	for (size_t i = 0; i < SPINEL_MAX_NUMB_TID; i++) {
		spinel_drv->t_id.props[i] = SPINEL_PROP_UNDEFINED;
	}
}

int spinel_drv_send_reset(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb, const void *ctx,
			  uint8_t type)
{
	int ret = spinel_datatype_pack(
		tx_cb, ctx, SPINEL_DATATYPE_COMMAND_S SPINEL_DATATYPE_UINT8_S,
		SPINEL_HEADER_FLAG | SPINEL_HEADER_IID(spinel_drv->inst), SPINEL_CMD_RESET, type);

	if (ret < 0) {
		LOG_ERR("Failed pack spinel reset command (inst = %u, err = %d)", spinel_drv->inst,
			ret);
	}

	return ret;
}

bool spinel_drv_check_reset(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			    uint16_t data_size)
{
	uint8_t header;
	uint32_t cmd;
	uint32_t prop;
	uint32_t status;

	int ret =
		spinel_datatype_unpack(data, data_size, false,
				       SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT_PACKED_S,
				       &header, &cmd, &prop, &status);

	if (ret < 0) {
		LOG_ERR("Failed unpack spinel header and data (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	if (spinel_drv->inst != SPINEL_HEADER_GET_IID(header) || cmd != SPINEL_CMD_PROP_VALUE_IS ||
	    prop != SPINEL_PROP_LAST_STATUS) {
		return false;
	}

	if (status != SPINEL_STATUS_RESET_POWER_ON) {
		LOG_ERR("Incorrect reset status (inst = %u, status = %u)", spinel_drv->inst,
			status);
		return false;
	}

	return true;
}

int spinel_drv_send_get_ieee_eui64(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				   const void *ctx)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_GET,
				      SPINEL_PROP_HWADDR, NULL);

	if (ret < 0) {
		LOG_ERR("Failed send get_ieee_eui64 (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_get_ieee_eui64(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				     uint16_t data_size, uint8_t ieee_eui64[8])
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_HWADDR, data,
				     data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(struct spinel_eui64)) {
		LOG_ERR("Failed check get_ieee_eui64 (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_EUI64_S,
				     ieee_eui64);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of get_ieee_eui64 (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	return true;
}

int spinel_drv_send_get_capabilities(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				     const void *ctx)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_GET,
				      SPINEL_PROP_RADIO_CAPS, NULL);

	if (ret < 0) {
		LOG_ERR("Failed send get_capabilities (inst = %u, err = %d)", spinel_drv->inst,
			ret);
	}

	return ret;
}

bool spinel_drv_check_get_capabilities(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				       uint16_t data_size, enum ieee802154_hw_caps *radio_caps)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	uint32_t get_caps;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_RADIO_CAPS,
				     data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || !param_size) {
		LOG_ERR("Failed check get_capabilities (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_UINT_PACKED_S,
				     &get_caps);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of get_capabilities (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	*radio_caps = spinel_drv_get_hw_caps(get_caps);

	return true;
}

int spinel_drv_send_enable_src_match(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				     const void *ctx, bool enable)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_MAC_SRC_MATCH_ENABLED, SPINEL_DATATYPE_BOOL_S,
				      enable);

	if (ret < 0) {
		LOG_ERR("Failed send enable_src_match (inst = %u, err = %d)", spinel_drv->inst,
			ret);
	}

	return ret;
}

bool spinel_drv_check_enable_src_match(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				       uint16_t data_size, bool enable)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	bool get_enable;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS,
				     SPINEL_PROP_MAC_SRC_MATCH_ENABLED, data, data_size,
				     &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(bool)) {
		LOG_ERR("Failed check enable_src_match (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_BOOL_S,
				     &get_enable);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of enable_src_match (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	if (get_enable != enable) {
		LOG_ERR("Get incorrect parameter in response to enable_src_match (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_ack_fpb(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb, const void *ctx,
			    uint16_t addr, bool enable)
{
	int ret;

	if (enable) {
		ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_INSERT,
					  SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES,
					  SPINEL_DATATYPE_UINT16_S, addr);
	} else {
		ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_REMOVE,
					  SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES,
					  SPINEL_DATATYPE_UINT16_S, addr);
	}

	if (ret < 0) {
		LOG_ERR("Failed send ack_fpb (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_ack_fpb(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			      uint16_t data_size, uint16_t addr, bool enable)
{
	int ret;
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	uint16_t get_addr;

	if (enable) {
		ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_INSERTED,
					 SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, data, data_size,
					 &param_data, &param_size);
	} else {
		ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_REMOVED,
					 SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, data, data_size,
					 &param_data, &param_size);
	}

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(uint16_t)) {
		LOG_ERR("Failed check ack_fpb (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_UINT16_S,
				     &get_addr);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of ack_fpb (inst = %u, err = %d)", spinel_drv->inst,
			ret);
		return false;
	}

	if (get_addr != addr) {
		LOG_ERR("Get incorrect address in response to ack_fpb (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_ack_fpb_ext(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				const void *ctx, uint8_t addr[8], bool enable)
{
	int ret;

	if (enable) {
		ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_INSERT,
					  SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES,
					  SPINEL_DATATYPE_EUI64_S, addr);
	} else {
		ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_REMOVE,
					  SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES,
					  SPINEL_DATATYPE_EUI64_S, addr);
	}

	if (ret < 0) {
		LOG_ERR("Failed send ack_fpb_ext (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_ack_fpb_ext(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				  uint16_t data_size, uint8_t addr[8], bool enable)
{
	int ret;
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	uint8_t(*p_get_addr)[8];

	if (enable) {
		ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_INSERTED,
					 SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, data,
					 data_size, &param_data, &param_size);
	} else {
		ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_REMOVED,
					 SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, data,
					 data_size, &param_data, &param_size);
	}

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(*p_get_addr)) {
		LOG_ERR("Failed check ack_fpb (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, false, SPINEL_DATATYPE_EUI64_S,
				     &p_get_addr);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of ack_fpb (inst = %u, err = %d)", spinel_drv->inst,
			ret);
		return false;
	}

	if (!p_get_addr || !addr) {
		ret = -EINVAL;
		LOG_ERR("NULL pointer in response ack_fpb (inst = %u, addr = %p, p_get_addr = %p)",
			spinel_drv->inst, addr, p_get_addr);
		return false;
	}

	if (memcmp(p_get_addr, addr, sizeof(*p_get_addr))) {
		LOG_ERR("Get incorrect address in response to ack_fpb (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_ack_fpb_clear(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				  const void *ctx)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, NULL);

	if (ret < 0) {
		LOG_ERR("Failed send ack_fpb_clear (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}
bool spinel_drv_check_ack_fpb_clear(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				    uint16_t data_size)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS,
				     SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, data, data_size,
				     &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0) {
		LOG_ERR("Failed check ack_fpb_ext_clear (inst = %u, err = %d)", spinel_drv->inst,
			ret);
		return false;
	}

	return true;
}

int spinel_drv_send_ack_fpb_ext_clear(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				      const void *ctx)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, NULL);

	if (ret < 0) {
		LOG_ERR("Failed send ack_fpb_clear (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}
bool spinel_drv_check_ack_fpb_ext_clear(struct spinel_drv_data *spinel_drv, const uint8_t *data,
					uint16_t data_size)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS,
				     SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, data, data_size,
				     &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0) {
		LOG_ERR("Failed check ack_fpb_ext_clear (inst = %u, err = %d)", spinel_drv->inst,
			ret);
		return false;
	}

	return true;
}

int spinel_drv_send_mac_frame_counter(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				      const void *ctx, uint32_t frame_counter, bool set_if_larger)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_RCP_MAC_FRAME_COUNTER,
				      SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_BOOL_S,
				      frame_counter, set_if_larger);

	if (ret < 0) {
		LOG_ERR("Failed send mac_frame_counter (inst = %u, err = %d)", spinel_drv->inst,
			ret);
	}

	return ret;
}

bool spinel_drv_check_mac_frame_counter(struct spinel_drv_data *spinel_drv, const uint8_t *data,
					uint16_t data_size, uint32_t frame_counter)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	uint32_t get_frame_counter;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS,
				     SPINEL_PROP_RCP_MAC_FRAME_COUNTER, data, data_size,
				     &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(uint32_t)) {
		LOG_ERR("Failed check mac_frame_counter (inst = %u, err = %d, data = %p, size = "
			"%u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_UINT32_S,
				     &get_frame_counter);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of mac_frame_counter (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	if (get_frame_counter != frame_counter) {
		LOG_ERR("Get incorrect frame counter in response to mac_frame_counter (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_panid(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb, const void *ctx,
			  uint16_t pan_id)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_MAC_15_4_PANID, SPINEL_DATATYPE_UINT16_S, pan_id);

	if (ret < 0) {
		LOG_ERR("Failed send panid (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_panid(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			    uint16_t data_size, uint16_t pan_id)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	uint16_t get_pan_id;

	int ret =
		spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_MAC_15_4_PANID,
				   data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(uint16_t)) {
		LOG_ERR("Failed check panid (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_UINT16_S,
				     &get_pan_id);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of panid (inst = %u, err = %d)", spinel_drv->inst,
			ret);
		return false;
	}

	if (get_pan_id != pan_id) {
		LOG_ERR("Get incorrect pan id in response to panid (inst = %u)", spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_short_addr(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
			       const void *ctx, uint16_t addr)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_MAC_15_4_SADDR, SPINEL_DATATYPE_UINT16_S, addr);

	if (ret < 0) {
		LOG_ERR("Failed send short_addr (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_short_addr(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				 uint16_t data_size, uint16_t addr)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	uint16_t get_addr;

	int ret =
		spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_MAC_15_4_SADDR,
				   data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(uint16_t)) {
		LOG_ERR("Failed check short_addr (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_UINT16_S,
				     &get_addr);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of short_addr (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	if (get_addr != addr) {
		LOG_ERR("Get incorrect address in response to short_addr (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_ext_addr(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
			     const void *ctx, uint8_t addr[8])
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_MAC_15_4_LADDR, SPINEL_DATATYPE_EUI64_S, addr);

	if (ret < 0) {
		LOG_ERR("Failed send ext_addr (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_ext_addr(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			       uint16_t data_size, uint8_t addr[8])
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	uint8_t(*p_get_addr)[8];

	int ret =
		spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_MAC_15_4_LADDR,
				   data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(*p_get_addr)) {
		LOG_ERR("Failed check ext_addr (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, false, SPINEL_DATATYPE_EUI64_S,
				     &p_get_addr);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of ext_addr (inst = %u, err = %d)", spinel_drv->inst,
			ret);
		return false;
	}

	if (!p_get_addr || !addr) {
		ret = -EINVAL;
		LOG_ERR("NULL pointer in response ext_addr (inst = %u, addr = %p, p_get_addr = %p)",
			spinel_drv->inst, addr, p_get_addr);
		return false;
	}

	if (memcmp(p_get_addr, addr, sizeof(*p_get_addr))) {
		LOG_ERR("Get incorrect address in response to ext_addr (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_tx_power(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
			     const void *ctx, int8_t pwr_dbm)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, pwr_dbm);

	if (ret < 0) {
		LOG_ERR("Failed send tx_power (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_tx_power(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			       uint16_t data_size, int8_t pwr_dbm)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	int8_t get_pwr_dbm;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_PHY_TX_POWER,
				     data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(int8_t)) {
		LOG_ERR("Failed check tx_power (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_INT8_S,
				     &get_pwr_dbm);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of tx_power (inst = %u, err = %d)", spinel_drv->inst,
			ret);
		return false;
	}

	if (get_pwr_dbm != pwr_dbm) {
		LOG_ERR("Get incorrect tx power in response to tx_power (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_rcp_enable(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
			       const void *ctx, bool enable)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_PHY_ENABLED, SPINEL_DATATYPE_BOOL_S, enable);

	if (ret < 0) {
		LOG_ERR("Failed send rcp_enable (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_rcp_enable(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				 uint16_t data_size, bool enable)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	bool get_enable;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_PHY_ENABLED,
				     data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(bool)) {
		LOG_ERR("Failed check rcp_enable (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_BOOL_S,
				     &get_enable);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of rcp_enable (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	if (get_enable != enable) {
		LOG_ERR("Get incorrect state in response to rcp_enable (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_receive_enable(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				   const void *ctx, bool enable)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S,
				      enable);

	if (ret < 0) {
		LOG_ERR("Failed send receive_enable (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_receive_enable(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				     uint16_t data_size, bool enable)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	bool get_enable;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS,
				     SPINEL_PROP_MAC_RAW_STREAM_ENABLED, data, data_size,
				     &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(bool)) {
		LOG_ERR("Failed check receive_enable (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_BOOL_S,
				     &get_enable);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of receive_enable (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	if (get_enable != enable) {
		LOG_ERR("Get incorrect state in response to receive_enable (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_channel(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb, const void *ctx,
			    uint8_t channel)
{
	int ret = spinel_drv_send_cmd(spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET,
				      SPINEL_PROP_PHY_CHAN, SPINEL_DATATYPE_UINT8_S, channel);

	if (ret < 0) {
		LOG_ERR("Failed send channel (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_channel(struct spinel_drv_data *spinel_drv, const uint8_t *data,
			      uint16_t data_size, uint8_t channel)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;
	uint8_t get_channel;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_PHY_CHAN,
				     data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || param_size != sizeof(uint8_t)) {
		LOG_ERR("Failed check channel (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_datatype_unpack(param_data, param_size, true, SPINEL_DATATYPE_UINT8_S,
				     &get_channel);

	if (ret < 0) {
		LOG_ERR("Failed get parameters of channel (inst = %u, err = %d)", spinel_drv->inst,
			ret);
		return false;
	}

	if (get_channel != channel) {
		LOG_ERR("Get incorrect channel in response to send channel (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

int spinel_drv_send_transmit_frame(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				   const void *ctx, const struct spinel_frame_data *frame)
{
	int ret = spinel_drv_send_cmd(
		spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_STREAM_RAW,
		SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_UINT8_S
			SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S
				SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S
					SPINEL_DATATYPE_UINT32_S SPINEL_DATATYPE_UINT32_S
						SPINEL_DATATYPE_UINT8_S,
		frame->data, frame->data_length, frame->tx.channel,
		SPINEL_DRV_DEFAULT_MAX_CSMA_BACKOFFS, SPINEL_DRV_DEFAULT_MAX_FRAME_RETRIES,
		frame->tx.csma_ca_enabled, frame->tx.header_updated, frame->tx.is_ret,
		frame->tx.security_processed, frame->time_offset, frame->time_base,
		frame->tx.channel);

	if (ret < 0) {
		LOG_ERR("Failed send transmit_frame (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_transmit_frame(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				     uint16_t data_size, struct spinel_frame_data *frame)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_LAST_STATUS,
				     data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (!(frame->data[FRAME_FC0_OFFSET] & FRAME_ACK_REQ_MASK)) {
		memset(frame, 0, sizeof(struct spinel_frame_data));
		return true;
	} else if (ret < 0 || !param_data || !param_size) {
		LOG_ERR("Failed check transmit_frame (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	uint32_t status;
	bool frame_pending;
	bool header_updated;

	ret = spinel_datatype_unpack(
		param_data, param_size, true,
		SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_BOOL_S SPINEL_DATATYPE_BOOL_S,
		&status, &frame_pending, &header_updated);

	if (ret < 0) {
		LOG_ERR("Failed get first parameters of transmit_frame (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	if (status != SPINEL_STATUS_OK) {
		LOG_ERR("Incorrect response status of transmit_frame (inst = %u, status = %u)",
			spinel_drv->inst, status);
		return false;
	}

	uint8_t tx_seq_numb = frame->data[FRAME_SN_OFFSET];

	param_data += ret;
	param_size -= (uint16_t)ret;

	ret = spinel_drv_get_radio_frame(spinel_drv, param_data, param_size, true, frame);

	if (ret < 0 || !frame->data_length) {
		LOG_ERR("Failed get frame in transmit_frame (inst = %u, get_len/err = %d, frame "
			"size = %u)",
			spinel_drv->inst, ret, frame->data_length);
		return false;
	}

	uint8_t rx_frame_control_lo = frame->data[FRAME_FC0_OFFSET];
	uint8_t rx_seq_numb = frame->data[FRAME_SN_OFFSET];

	if ((rx_frame_control_lo & FRAME_TYPE_MASK) != FRAME_TYPE_ACK) {
		LOG_ERR("Get incorrect frame type in response to send transmit_frame (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	if (rx_seq_numb != tx_seq_numb) {
		LOG_ERR("Get incorrect sequence number in response send transmit_frame (inst = %u)",
			spinel_drv->inst);
		return false;
	}

	return true;
}

bool spinel_drv_check_receive_frame(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				    uint16_t data_size, struct spinel_frame_data *frame)
{
	const uint8_t *param_data = NULL;
	uint16_t param_size = 0;

	int ret = spinel_drv_get_cmd(spinel_drv, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_STREAM_RAW,
				     data, data_size, &param_data, &param_size);

	if (ret == -EPERM) {
		/* Skip this frame */
		return false;
	} else if (ret < 0 || !param_data || !param_size) {
		LOG_ERR("Failed check receive_frame (inst = %u, err = %d, data = %p, size = %u)",
			spinel_drv->inst, ret, param_data, param_size);
		return false;
	}

	ret = spinel_drv_get_radio_frame(spinel_drv, param_data, param_size, false, frame);

	if (ret < 0 || !frame->data_length) {
		LOG_ERR("Failed get frame in receive_frame (inst = %u, get_len/err = %d, size = "
			"%u)",
			spinel_drv->inst, ret, frame->data_length);
		return false;
	}

	return true;
}

int spinel_drv_send_link_metrics(struct spinel_drv_data *spinel_drv, spinel_tx_cb tx_cb,
				 const void *ctx, uint16_t short_addr, const uint8_t ext_addr[8],
				 struct spinel_link_metrics link_metrics)
{
	uint8_t flags = 0;

	if (link_metrics.pdu_count) {
		flags |= SPINEL_THREAD_LINK_METRIC_PDU_COUNT;
	}
	if (link_metrics.lqi) {
		flags |= SPINEL_THREAD_LINK_METRIC_LQI;
	}
	if (link_metrics.link_margin) {
		flags |= SPINEL_THREAD_LINK_METRIC_LINK_MARGIN;
	}
	if (link_metrics.rssi) {
		flags |= SPINEL_THREAD_LINK_METRIC_RSSI;
	}
	int ret = spinel_drv_send_cmd(
		spinel_drv, tx_cb, ctx, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_RCP_ENH_ACK_PROBING,
		SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_EUI64_S SPINEL_DATATYPE_UINT8_S,
		short_addr, ext_addr, flags);

	if (ret < 0) {
		LOG_ERR("Failed send link metrics (inst = %u, err = %d)", spinel_drv->inst, ret);
	}

	return ret;
}

bool spinel_drv_check_link_metrics(struct spinel_drv_data *spinel_drv, const uint8_t *data,
				   uint16_t data_size)
{
	uint8_t header;
	uint32_t cmd;
	uint32_t prop;
	uint32_t status;

	int ret =
		spinel_datatype_unpack(data, data_size, false,
				       SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT_PACKED_S,
				       &header, &cmd, &prop, &status);

	if (ret < 0) {
		LOG_ERR("Failed unpack spinel header and data (inst = %u, err = %d)",
			spinel_drv->inst, ret);
		return false;
	}

	if (spinel_drv->inst != SPINEL_HEADER_GET_IID(header) || cmd != SPINEL_CMD_PROP_VALUE_IS ||
	    prop != SPINEL_PROP_LAST_STATUS) {
		return false;
	}

	if (status != SPINEL_STATUS_OK) {
		LOG_ERR("Incorrect link metrics status (inst = %u, status = %u)", spinel_drv->inst,
			status);
		return false;
	}

	return true;
}

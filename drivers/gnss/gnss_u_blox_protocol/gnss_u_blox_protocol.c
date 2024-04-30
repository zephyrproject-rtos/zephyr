/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gnss_u_blox_protocol.h"

const uint32_t ubx_baudrate[UBX_BAUDRATE_COUNT] = {
	4800,
	9600,
	19200,
	38400,
	57600,
	115200,
	230400,
	460800,
	921600,
};

static inline int ubx_validate_payload_size_ack(uint8_t msg_id, uint16_t payload_size)
{
	switch (msg_id) {
	case UBX_ACK_ACK:
		return payload_size == UBX_CFG_ACK_PAYLOAD_SZ ? 0 : -1;
	case UBX_ACK_NAK:
		return payload_size == UBX_CFG_NAK_PAYLOAD_SZ ? 0 : -1;
	default:
		return -1;
	}
}

static inline int ubx_validate_payload_size_cfg(uint8_t msg_id, uint16_t payload_size)
{
	switch (msg_id) {
	case UBX_CFG_RATE:
		return payload_size == UBX_CFG_RATE_PAYLOAD_SZ ? 0 : -1;
	case UBX_CFG_PRT:
		return (payload_size == UBX_CFG_PRT_POLL_PAYLOAD_SZ ||
		       payload_size == UBX_CFG_PRT_SET_PAYLOAD_SZ) ? 0 : -1;
	case UBX_CFG_RST:
		return payload_size == UBX_CFG_RST_PAYLOAD_SZ ? 0 : -1;
	case UBX_CFG_NAV5:
		return payload_size == UBX_CFG_NAV5_PAYLOAD_SZ ? 0 : -1;
	case UBX_CFG_GNSS:
		return ((payload_size - UBX_CFG_GNSS_PAYLOAD_INIT_SZ) %
		       UBX_CFG_GNSS_PAYLOAD_CFG_BLK_SZ == 0) ? 0 : -1;
	case UBX_CFG_MSG:
		return payload_size == UBX_CFG_MSG_PAYLOAD_SZ ? 0 : -1;
	default:
		return -1;
	}
}

static inline int ubx_validate_payload_size(uint8_t msg_cls, uint8_t msg_id, uint16_t payload_size)
{
	if (payload_size == 0) {
		return 0;
	}

	if (payload_size > UBX_PAYLOAD_SZ_MAX) {
		return -1;
	}

	switch (msg_cls) {
	case UBX_CLASS_ACK:
		return ubx_validate_payload_size_ack(msg_id, payload_size);
	case UBX_CLASS_CFG:
		return ubx_validate_payload_size_cfg(msg_id, payload_size);
	default:
		return -1;
	}
}

int ubx_create_and_validate_frame(uint8_t *ubx_frame, uint16_t ubx_frame_size, uint8_t msg_cls,
				  uint8_t msg_id, const void *payload, uint16_t payload_size)
{
	if (ubx_validate_payload_size(msg_cls, msg_id, payload_size)) {
		return -1;
	}

	return modem_ubx_create_frame(ubx_frame, ubx_frame_size, msg_cls, msg_id, payload,
				      payload_size);
}

void ubx_cfg_ack_payload_default(struct ubx_cfg_ack_payload *payload)
{
	payload->message_class = UBX_CLASS_CFG;
	payload->message_id = UBX_CFG_PRT;
}

void ubx_cfg_rate_payload_default(struct ubx_cfg_rate_payload *payload)
{
	payload->meas_rate_ms = 1000;
	payload->nav_rate = 1;
	payload->time_ref = UBX_CFG_RATE_TIME_REF_UTC;
}

void ubx_cfg_prt_poll_payload_default(struct ubx_cfg_prt_poll_payload *payload)
{
	payload->port_id = UBX_PORT_NUMBER_UART;
}

void ubx_cfg_prt_set_payload_default(struct ubx_cfg_prt_set_payload *payload)
{
	payload->port_id = UBX_PORT_NUMBER_UART;
	payload->reserved0 = UBX_CFG_PRT_RESERVED0;
	payload->tx_ready_pin_conf = UBX_CFG_PRT_TX_READY_PIN_CONF_POL_HIGH;
	payload->port_mode = UBX_CFG_PRT_PORT_MODE_CHAR_LEN_8 | UBX_CFG_PRT_PORT_MODE_PARITY_NONE |
			     UBX_CFG_PRT_PORT_MODE_STOP_BITS_1;
	payload->baudrate = ubx_baudrate[3];
	payload->in_proto_mask = UBX_CFG_PRT_IN_PROTO_UBX | UBX_CFG_PRT_IN_PROTO_NMEA |
				 UBX_CFG_PRT_IN_PROTO_RTCM;
	payload->out_proto_mask = UBX_CFG_PRT_OUT_PROTO_UBX | UBX_CFG_PRT_OUT_PROTO_NMEA |
				  UBX_CFG_PRT_OUT_PROTO_RTCM3;
	payload->flags = UBX_CFG_PRT_FLAGS_DEFAULT;
	payload->reserved1 = UBX_CFG_PRT_RESERVED1;
}

void ubx_cfg_rst_payload_default(struct ubx_cfg_rst_payload *payload)
{
	payload->nav_bbr_mask = UBX_CFG_RST_NAV_BBR_MASK_HOT_START;
	payload->reset_mode = UBX_CFG_RST_RESET_MODE_CONTROLLED_SOFT_RESET;
	payload->reserved0 = UBX_CFG_RST_RESERVED0;
}

void ubx_cfg_nav5_payload_default(struct ubx_cfg_nav5_payload *payload)
{
	payload->mask = UBX_CFG_NAV5_MASK_ALL;
	payload->dyn_model = UBX_DYN_MODEL_PORTABLE;

	payload->fix_mode = UBX_FIX_AUTO_FIX;

	payload->fixed_alt = UBX_CFG_NAV5_FIXED_ALT_DEFAULT;
	payload->fixed_alt_var = UBX_CFG_NAV5_FIXED_ALT_VAR_DEFAULT;

	payload->min_elev = UBX_CFG_NAV5_MIN_ELEV_DEFAULT;
	payload->dr_limit = UBX_CFG_NAV5_DR_LIMIT_DEFAULT;

	payload->p_dop = UBX_CFG_NAV5_P_DOP_DEFAULT;
	payload->t_dop = UBX_CFG_NAV5_T_DOP_DEFAULT;
	payload->p_acc = UBX_CFG_NAV5_P_ACC_DEFAULT;
	payload->t_acc = UBX_CFG_NAV5_T_ACC_DEFAULT;

	payload->static_hold_threshold = UBX_CFG_NAV5_STATIC_HOLD_THRESHOLD_DEFAULT;
	payload->dgnss_timeout = UBX_CFG_NAV5_DGNSS_TIMEOUT_DEFAULT;
	payload->cno_threshold_num_svs = UBX_CFG_NAV5_CNO_THRESHOLD_NUM_SVS_DEFAULT;
	payload->cno_threshold = UBX_CFG_NAV5_CNO_THRESHOLD_DEFAULT;

	payload->reserved0 = UBX_CFG_NAV5_RESERVED0;

	payload->static_hold_dist_threshold = UBX_CFG_NAV5_STATIC_HOLD_DIST_THRESHOLD;
	payload->utc_standard = UBX_CFG_NAV5_UTC_STANDARD_DEFAULT;
}

static struct ubx_cfg_gnss_payload_config_block ubx_cfg_gnss_payload_config_block_default = {
	.gnss_id = UBX_GNSS_ID_GPS,
	.num_res_trk_ch = 0x00,
	.max_num_trk_ch = 0x00,
	.reserved0 = UBX_CFG_GNSS_RESERVED0,
	.flags = UBX_CFG_GNSS_FLAG_ENABLE | UBX_CFG_GNSS_FLAG_SGN_CNF_GPS_L1C_A,
};

void ubx_cfg_gnss_payload_default(struct ubx_cfg_gnss_payload *payload)
{
	payload->msg_ver = UBX_CFG_GNSS_MSG_VER;
	payload->num_trk_ch_hw = UBX_CFG_GNSS_NUM_TRK_CH_HW_DEFAULT;
	payload->num_trk_ch_use = UBX_CFG_GNSS_NUM_TRK_CH_USE_DEFAULT;

	for (int i = 0; i < payload->num_config_blocks; ++i) {
		payload->config_blocks[i] = ubx_cfg_gnss_payload_config_block_default;
	}
}

void ubx_cfg_msg_payload_default(struct ubx_cfg_msg_payload *payload)
{
	payload->message_class = UBX_CLASS_NMEA;
	payload->message_id = UBX_NMEA_GGA;
	payload->rate = UBX_CFG_MSG_RATE_DEFAULT;
}

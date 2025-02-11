/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/modem/ubx.h>
#include "gnss_u_blox_protocol_defines.h"

#ifndef ZEPHYR_U_BLOX_PROTOCOL_
#define ZEPHYR_U_BLOX_PROTOCOL_

#define UBX_BAUDRATE_COUNT			9

/* When a configuration frame is sent, the device requires some delay to reflect the changes. */
/* TODO: check what is the precise waiting time for each message. */
#define UBX_CFG_RST_WAIT_MS			6000
#define UBX_CFG_GNSS_WAIT_MS			6000
#define UBX_CFG_NAV5_WAIT_MS			6000

extern const uint32_t ubx_baudrate[UBX_BAUDRATE_COUNT];

#define UBX_FRM_GET_PAYLOAD_SZ		0
#define UBX_CFG_ACK_PAYLOAD_SZ		2
#define UBX_CFG_NAK_PAYLOAD_SZ		2
#define UBX_CFG_RATE_PAYLOAD_SZ		6
#define UBX_CFG_PRT_POLL_PAYLOAD_SZ	1
#define UBX_CFG_PRT_POLL_FRM_SZ		(UBX_FRM_SZ_WO_PAYLOAD + UBX_CFG_PRT_POLL_PAYLOAD_SZ)
#define UBX_CFG_PRT_SET_PAYLOAD_SZ	20
#define UBX_CFG_PRT_SET_FRM_SZ		(UBX_FRM_SZ_WO_PAYLOAD + UBX_CFG_PRT_SET_PAYLOAD_SZ)
#define UBX_CFG_RST_PAYLOAD_SZ		4
#define UBX_CFG_RST_FRM_SZ		(UBX_FRM_SZ_WO_PAYLOAD + UBX_CFG_RST_PAYLOAD_SZ)
#define UBX_CFG_NAV5_PAYLOAD_SZ		36
#define UBX_CFG_NAV5_FRM_SZ		(UBX_FRM_SZ_WO_PAYLOAD + UBX_CFG_NAV5_PAYLOAD_SZ)
#define UBX_CFG_MSG_PAYLOAD_SZ		3
#define UBX_CFG_MSG_FRM_SZ		(UBX_FRM_SZ_WO_PAYLOAD + UBX_CFG_MSG_PAYLOAD_SZ)
#define UBX_CFG_GNSS_PAYLOAD_INIT_SZ	4
#define UBX_CFG_GNSS_PAYLOAD_CFG_BLK_SZ	8
#define UBX_CFG_GNSS_PAYLOAD_SZ(n)	\
	(UBX_CFG_GNSS_PAYLOAD_INIT_SZ + UBX_CFG_GNSS_PAYLOAD_CFG_BLK_SZ * n)
#define UBX_CFG_GNSS_FRM_SZ(n)		(UBX_FRM_SZ_WO_PAYLOAD + UBX_CFG_GNSS_PAYLOAD_SZ(n))


int ubx_create_and_validate_frame(uint8_t *ubx_frame, uint16_t ubx_frame_size, uint8_t msg_cls,
				  uint8_t msg_id, const void *payload, uint16_t payload_size);

struct ubx_cfg_ack_payload {
	uint8_t message_class;
	uint8_t message_id;
};

void ubx_cfg_ack_payload_default(struct ubx_cfg_ack_payload *payload);

#define UBX_CFG_RATE_TIME_REF_UTC	0	/* Align measurements to UTC time. */
#define UBX_CFG_RATE_TIME_REF_GPS	1	/* Align measurements to GPS time. */
#define UBX_CFG_RATE_TIME_REF_GLO	2	/* Align measurements to GLONASS time. */
#define UBX_CFG_RATE_TIME_REF_BDS	3	/* Align measurements to BeiDou time. */
#define UBX_CFG_RATE_TIME_REF_GAL	4	/* Align measurements to Galileo time. */

struct ubx_cfg_rate_payload {
	uint16_t meas_rate_ms;
	uint16_t nav_rate;
	uint16_t time_ref;
};

void ubx_cfg_rate_payload_default(struct ubx_cfg_rate_payload *payload);

struct ubx_cfg_prt_poll_payload {
	uint8_t port_id;
};

void ubx_cfg_prt_poll_payload_default(struct ubx_cfg_prt_poll_payload *payload);

#define UBX_CFG_PRT_IN_PROTO_UBX			BIT(0)
#define UBX_CFG_PRT_IN_PROTO_NMEA			BIT(1)
#define UBX_CFG_PRT_IN_PROTO_RTCM			BIT(2)
#define UBX_CFG_PRT_IN_PROTO_RTCM3			BIT(5)
#define UBX_CFG_PRT_OUT_PROTO_UBX			BIT(0)
#define UBX_CFG_PRT_OUT_PROTO_NMEA			BIT(1)
#define UBX_CFG_PRT_OUT_PROTO_RTCM3			BIT(5)

#define UBX_CFG_PRT_PORT_MODE_CHAR_LEN_5		0U
#define UBX_CFG_PRT_PORT_MODE_CHAR_LEN_6		BIT(6)
#define UBX_CFG_PRT_PORT_MODE_CHAR_LEN_7		BIT(7)
#define UBX_CFG_PRT_PORT_MODE_CHAR_LEN_8		(BIT(6) | BIT(7))

#define UBX_CFG_PRT_PORT_MODE_PARITY_EVEN		0U
#define UBX_CFG_PRT_PORT_MODE_PARITY_ODD		BIT(9)
#define UBX_CFG_PRT_PORT_MODE_PARITY_NONE		BIT(11)

#define UBX_CFG_PRT_PORT_MODE_STOP_BITS_1		0U
#define UBX_CFG_PRT_PORT_MODE_STOP_BITS_1_HALF		BIT(12)
#define UBX_CFG_PRT_PORT_MODE_STOP_BITS_2		BIT(13)
#define UBX_CFG_PRT_PORT_MODE_STOP_BITS_HALF		(BIT(12) | BIT(13))

#define UBX_CFG_PRT_RESERVED0				0x00
#define UBX_CFG_PRT_TX_READY_PIN_CONF_DEFAULT		0x0000
#define UBX_CFG_PRT_TX_READY_PIN_CONF_EN		BIT(0)
#define UBX_CFG_PRT_TX_READY_PIN_CONF_POL_LOW		BIT(1)
#define UBX_CFG_PRT_TX_READY_PIN_CONF_POL_HIGH		0U
#define UBX_CFG_PRT_RESERVED1				0x00
#define UBX_CFG_PRT_FLAGS_DEFAULT			0x0000
#define UBX_CFG_PRT_FLAGS_EXTENDED_TX_TIMEOUT		BIT(0)

struct ubx_cfg_prt_set_payload {
	uint8_t port_id;
	uint8_t reserved0;
	uint16_t tx_ready_pin_conf;
	uint32_t port_mode;
	uint32_t baudrate;
	uint16_t in_proto_mask;
	uint16_t out_proto_mask;
	uint16_t flags;
	uint8_t reserved1;
};

void ubx_cfg_prt_set_payload_default(struct ubx_cfg_prt_set_payload *payload);

#define UBX_CFG_RST_NAV_BBR_MASK_HOT_START				0x0000
#define UBX_CFG_RST_NAV_BBR_MASK_WARM_START				0x0001
#define UBX_CFG_RST_NAV_BBR_MASK_COLD_START				0xFFFF

#define UBX_CFG_RST_RESET_MODE_HARD_RESET				0x00
#define UBX_CFG_RST_RESET_MODE_CONTROLLED_SOFT_RESET			0x01
#define UBX_CFG_RST_RESET_MODE_CONTROLLED_SOFT_RESET_GNSS_ONLY		0x02
#define UBX_CFG_RST_RESET_MODE_HARD_RESET_AFTER_SHUTDOWN		0x04
#define UBX_CFG_RST_RESET_MODE_CONTROLLED_GNSS_STOP			0x08
#define UBX_CFG_RST_RESET_MODE_CONTROLLED_GNSS_START			0x09

#define UBX_CFG_RST_RESERVED0						0x00

struct ubx_cfg_rst_payload {
	uint16_t nav_bbr_mask;
	uint8_t reset_mode;
	uint8_t reserved0;
};

void ubx_cfg_rst_payload_default(struct ubx_cfg_rst_payload *payload);

#define UBX_CFG_NAV5_MASK_ALL				0x05FF
#define UBX_CFG_NAV5_FIX_MODE_DEFAULT			UBX_FIX_AUTO_FIX
#define UBX_CFG_NAV5_FIXED_ALT_DEFAULT			0
#define UBX_CFG_NAV5_FIXED_ALT_VAR_DEFAULT		1U
#define UBX_CFG_NAV5_MIN_ELEV_DEFAULT			5
#define UBX_CFG_NAV5_DR_LIMIT_DEFAULT			3U
#define UBX_CFG_NAV5_P_DOP_DEFAULT			100U
#define UBX_CFG_NAV5_T_DOP_DEFAULT			100U
#define UBX_CFG_NAV5_P_ACC_DEFAULT			100U
#define UBX_CFG_NAV5_T_ACC_DEFAULT			350U
#define UBX_CFG_NAV5_STATIC_HOLD_THRESHOLD_DEFAULT	0U
#define UBX_CFG_NAV5_DGNSS_TIMEOUT_DEFAULT		60U
#define UBX_CFG_NAV5_CNO_THRESHOLD_NUM_SVS_DEFAULT	0U
#define UBX_CFG_NAV5_CNO_THRESHOLD_DEFAULT		0U
#define UBX_CFG_NAV5_RESERVED0				0U
#define UBX_CFG_NAV5_STATIC_HOLD_DIST_THRESHOLD		0U
#define UBX_CFG_NAV5_UTC_STANDARD_DEFAULT		UBX_UTC_AUTOUTC

struct ubx_cfg_nav5_payload {
	uint16_t mask;
	uint8_t dyn_model;

	uint8_t fix_mode;

	int32_t fixed_alt;
	uint32_t fixed_alt_var;

	int8_t min_elev;
	uint8_t dr_limit;

	uint16_t p_dop;
	uint16_t t_dop;
	uint16_t p_acc;
	uint16_t t_acc;

	uint8_t static_hold_threshold;
	uint8_t dgnss_timeout;
	uint8_t cno_threshold_num_svs;
	uint8_t cno_threshold;

	uint16_t reserved0;

	uint16_t static_hold_dist_threshold;
	uint8_t utc_standard;
};

void ubx_cfg_nav5_payload_default(struct ubx_cfg_nav5_payload *payload);

#define UBX_CFG_GNSS_MSG_VER			0x00
#define UBX_CFG_GNSS_NUM_TRK_CH_HW_DEFAULT	0x31
#define UBX_CFG_GNSS_NUM_TRK_CH_USE_DEFAULT	0x31

#define UBX_CFG_GNSS_RESERVED0			0x00
#define UBX_CFG_GNSS_FLAG_ENABLE		BIT(0)
#define UBX_CFG_GNSS_FLAG_DISABLE		0U
#define UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT		16
/* When gnss_id is 0 (GPS) */
#define UBX_CFG_GNSS_FLAG_SGN_CNF_GPS_L1C_A	(0x01 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
#define UBX_CFG_GNSS_FLAG_SGN_CNF_GPS_L2C	(0x10 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
#define UBX_CFG_GNSS_FLAG_SGN_CNF_GPS_L5	(0x20 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
/* When gnss_id is 1 (SBAS) */
#define UBX_CFG_GNSS_FLAG_SGN_CNF_SBAS_L1C_A	(0x01 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
/* When gnss_id is 2 (Galileo) */
#define UBX_CFG_GNSS_FLAG_SGN_CNF_GALILEO_E1	(0x01 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
#define UBX_CFG_GNSS_FLAG_SGN_CNF_GALILEO_E5A	(0x10 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
#define UBX_CFG_GNSS_FLAG_SGN_CNF_GALILEO_E5B	(0x20 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
/* When gnss_id is 3 (BeiDou) */
#define UBX_CFG_GNSS_FLAG_SGN_CNF_BEIDOU_B1I	(0x01 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
#define UBX_CFG_GNSS_FLAG_SGN_CNF_BEIDOU_B2I	(0x10 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
#define UBX_CFG_GNSS_FLAG_SGN_CNF_BEIDOU_B2A	(0x80 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
/* When gnss_id is 4 (IMES) */
#define UBX_CFG_GNSS_FLAG_SGN_CNF_IMES_L1	(0x01 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
/* When gnss_id is 5 (QZSS) */
#define UBX_CFG_GNSS_FLAG_SGN_CNF_QZSS_L1C_A	(0x01 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
#define UBX_CFG_GNSS_FLAG_SGN_CNF_QZSS_L1S	(0x04 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
#define UBX_CFG_GNSS_FLAG_SGN_CNF_QZSS_L2C	(0x10 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
#define UBX_CFG_GNSS_FLAG_SGN_CNF_QZSS_L5	(0x20 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
/* When gnss_id is 6 (GLONASS) */
#define UBX_CFG_GNSS_FLAG_SGN_CNF_GLONASS_L1	(0x01 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)
#define UBX_CFG_GNSS_FLAG_SGN_CNF_GLONASS_L2	(0x10 << UBX_CFG_GNSS_FLAG_SGN_CNF_SHIFT)

struct ubx_cfg_gnss_payload_config_block {
	uint8_t gnss_id;
	uint8_t num_res_trk_ch;
	uint8_t max_num_trk_ch;
	uint8_t reserved0;
	uint32_t flags;
};

struct ubx_cfg_gnss_payload {
	uint8_t msg_ver;
	uint8_t num_trk_ch_hw;
	uint8_t num_trk_ch_use;
	uint8_t num_config_blocks;
	struct ubx_cfg_gnss_payload_config_block config_blocks[];
};

void ubx_cfg_gnss_payload_default(struct ubx_cfg_gnss_payload *payload);

#define UBX_CFG_MSG_RATE_DEFAULT			1

struct ubx_cfg_msg_payload {
	uint8_t message_class;
	uint8_t message_id;
	uint8_t rate;
};

void ubx_cfg_msg_payload_default(struct ubx_cfg_msg_payload *payload);

#endif /* ZEPHYR_U_BLOX_PROTOCOL_ */

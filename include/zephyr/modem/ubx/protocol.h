/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODEM_UBX_PROTOCOL_
#define ZEPHYR_MODEM_UBX_PROTOCOL_

#include <stdint.h>
#include <zephyr/modem/ubx/checksum.h>

#define UBX_FRM_HEADER_SZ			6
#define UBX_FRM_FOOTER_SZ			2
#define UBX_FRM_SZ_WITHOUT_PAYLOAD		(UBX_FRM_HEADER_SZ + UBX_FRM_FOOTER_SZ)
#define UBX_FRM_SZ(payload_size)		(payload_size + UBX_FRM_SZ_WITHOUT_PAYLOAD)

#define UBX_PREAMBLE_SYNC_CHAR_1		0xB5
#define UBX_PREAMBLE_SYNC_CHAR_2		0x62

#define UBX_FRM_PREAMBLE_SYNC_CHAR_1_IDX	0
#define UBX_FRM_PREAMBLE_SYNC_CHAR_2_IDX	1
#define UBX_FRM_MSG_CLASS_IDX			2

#define UBX_PAYLOAD_SZ_MAX			512
#define UBX_FRM_SZ_MAX				UBX_FRM_SZ(UBX_PAYLOAD_SZ_MAX)

struct ubx_frame {
	uint8_t preamble_sync_char_1;
	uint8_t preamble_sync_char_2;
	uint8_t class;
	uint8_t id;
	uint16_t payload_size;
	uint8_t payload_and_checksum[];
};

struct ubx_frame_match {
	uint8_t class;
	uint8_t id;
	struct {
		uint8_t *buf;
		uint16_t len;
	} payload;
};

enum ubx_class_id {
	UBX_CLASS_ID_NAV = 0x01,  /* Navigation Results Messages */
	UBX_CLASS_ID_RXM = 0x02,  /* Receiver Manager Messages */
	UBX_CLASS_ID_INF = 0x04,  /* Information Messages */
	UBX_CLASS_ID_ACK = 0x05,  /* Ack/Nak Messages */
	UBX_CLASS_ID_CFG = 0x06,  /* Configuration Input Messages */
	UBX_CLASS_ID_UPD = 0x09,  /* Firmware Update Messages */
	UBX_CLASS_ID_MON = 0x0A,  /* Monitoring Messages */
	UBX_CLASS_ID_TIM = 0x0D,  /* Timing Messages */
	UBX_CLASS_ID_MGA = 0x13,  /* Multiple GNSS Assistance Messages */
	UBX_CLASS_ID_LOG = 0x21,  /* Logging Messages */
	UBX_CLASS_ID_SEC = 0x27,  /* Security Feature Messages */
	UBX_CLASS_ID_NMEA_STD = 0xF0, /* Note: Only used to configure message rate */
	UBX_CLASS_ID_NMEA_PUBX = 0xF1, /* Note: Only used to configure message rate */
};

enum ubx_msg_id_nav {
	UBX_MSG_ID_NAV_PVT = 0x07,
	UBX_MSG_ID_NAV_SAT = 0x35,
};

enum ubx_nav_fix_type {
	UBX_NAV_FIX_TYPE_NO_FIX = 0,
	UBX_NAV_FIX_TYPE_DR = 1,
	UBX_NAV_FIX_TYPE_2D = 2,
	UBX_NAV_FIX_TYPE_3D = 3,
	UBX_NAV_FIX_TYPE_GNSS_DR_COMBINED = 4,
	UBX_NAV_FIX_TYPE_TIME_ONLY = 5,
};

struct ubx_nav_pvt {
	struct {
		uint32_t itow;
		uint16_t year;
		uint8_t month;
		uint8_t day;
		uint8_t hour;
		uint8_t minute;
		uint8_t second;
		struct {
			uint8_t date : 1;
			uint8_t time : 1;
			uint8_t time_fully_resolved : 1;
			uint8_t mag : 1;
			uint8_t reserved : 4;
		} __packed valid;
		uint32_t tacc;
		int32_t nano;
	} __packed time;
	uint8_t fix_type; /** See ubx_nav_fix_type */
	struct {
		uint8_t gnss_fix_ok : 1;
		uint8_t diff_soln : 1;
		uint8_t psm_state : 3;
		uint8_t head_vehicle_valid : 1;
		uint8_t carr_soln : 2;
	} __packed flags;
	struct {
		uint8_t reserved : 5;
		uint8_t confirmed_avai : 1;
		uint8_t confirmed_date : 1;
		uint8_t confirmed_time : 1;
	} __packed flags2;
	struct {
		uint8_t num_sv;
		int32_t longitude; /* Longitude. Degrees. scaling: 1e-7 */
		int32_t latitude; /* Latitude. Degrees. scaling: 1e-7 */
		int32_t height; /* Height above ellipsoid. mm */
		int32_t hmsl; /* Height above mean sea level. mm */
		uint32_t horiz_acc; /* Horizontal accuracy estimate. mm */
		uint32_t vert_acc; /* Vertical accuracy estimate. mm */
		int32_t vel_north; /* NED north velocity. mm/s */
		int32_t vel_east; /* NED east velocity. mm/s */
		int32_t vel_down; /* NED down velocity. mm/s */
		int32_t ground_speed; /* Ground Speed (2D). mm/s */
		int32_t head_motion; /* Heading of Motion (2D). Degrees. scaling: 1e-5 */
		uint32_t speed_acc; /* Speed accuracy estimated. mm/s */
		uint32_t head_acc; /** Heading accuracy estimate (both motion and vehicle).
				     *  Degrees. scaling: 1e-5.
				     */
		uint16_t pdop; /* scaling: 1e-2 */
		struct {
			uint8_t invalid_lon_height_hmsl : 1;
			uint8_t last_correction_age : 4;
			uint16_t reserved : 11;
		} __packed flags3;
		uint32_t reserved;
		int32_t head_vehicle; /* Heading of vehicle (2D). Degrees. Valid if
				       * flags.head_vehicle_valid is set.
				       */
		int16_t mag_decl; /* Magnetic declination. Degrees. */
		uint16_t magacc; /* Magnetic declination accuracy. Degrees. scaling: 1e-2 */
	} __packed nav;
} __packed;

enum ubx_nav_sat_health {
	UBX_NAV_SAT_HEALTH_UNKNOWN = 0,
	UBX_NAV_SAT_HEALTH_HEALTHY = 1,
	UBX_NAV_SAT_HEALTH_UNHEALTHY = 2,
};

enum ubx_gnss_id {
	UBX_GNSS_ID_GPS = 0,
	UBX_GNSS_ID_SBAS = 1,
	UBX_GNSS_ID_GALILEO = 2,
	UBX_GNSS_ID_BEIDOU = 3,
	UBX_GNSS_ID_QZSS = 5,
	UBX_GNSS_ID_GLONASS = 6,
};

struct ubx_nav_sat {
	uint32_t itow;
	uint8_t version; /* Message version. */
	uint8_t num_sv;
	uint16_t reserved1;
	struct ubx_nav_sat_info {
		uint8_t gnss_id; /* See ubx_gnss_id */
		uint8_t sv_id;
		uint8_t cno; /* Carrier-to-noise ratio. dBHz */
		int8_t elevation; /* Elevation (range: +/- 90). Degrees */
		int16_t azimuth; /* Azimuth (range: 0 - 360). Degrees */
		int16_t pseu_res; /* Pseudorange Residual. Meters */
		struct {
			uint8_t quality : 3;
			uint8_t sv_used : 1;
			uint8_t health : 2; /* See ubx_nav_sat_health */
			uint8_t diff_corr : 1;
			uint8_t smoothed : 1;
			uint8_t orbit_src : 3;
			uint8_t eph_avail : 1;
			uint8_t alm_avail : 1;
			uint8_t ano_avail : 1;
			uint8_t aop_avail : 1;
			uint8_t reserved1 : 1;
			uint8_t sbas_corr_used : 1;
			uint8_t rtcm_corr_used : 1;
			uint8_t slas_corr_used : 1;
			uint8_t spartn_corr_used : 1;
			uint8_t pr_corr_used : 1;
			uint8_t cr_corr_used : 1;
			uint8_t do_corr_used : 1;
			uint8_t clas_corr_used : 1;
			uint8_t reserved : 8;
		} __packed flags;
	} sat[];
};

enum ubx_msg_id_ack {
	UBX_MSG_ID_ACK = 0x01,
	UBX_MSG_ID_NAK = 0x00
};

enum ubx_msg_id_cfg {
	UBX_MSG_ID_CFG_PRT = 0x00,
	UBX_MSG_ID_CFG_MSG = 0x01,
	UBX_MSG_ID_CFG_RST = 0x04,
	UBX_MSG_ID_CFG_RATE = 0x08,
	UBX_MSG_ID_CFG_NAV5 = 0x24,
};

enum ubx_msg_id_mon {
	UBX_MSG_ID_MON_VER = 0x04,
	UBX_MSG_ID_MON_GNSS = 0x28,
};

struct ubx_ack {
	uint8_t class;
	uint8_t id;
};

struct gnss_selection {
	uint8_t gps : 1;
	uint8_t glonass : 1;
	uint8_t beidou : 1;
	uint8_t galileo : 1;
	uint8_t reserved : 4;
} __packed;

struct ubx_mon_gnss {
	uint8_t ver;
	struct gnss_selection supported;
	struct gnss_selection default_ena;
	struct gnss_selection enabled;
	uint8_t simultaneous;
	uint8_t reserved1[3];
} __packed;

enum ubx_cfg_port_id {
	UBX_CFG_PORT_ID_DDC = 0,
	UBX_CFG_PORT_ID_UART = 1,
	UBX_CFG_PORT_ID_USB = 2,
	UBX_CFG_PORT_ID_SPI = 3,
};

enum ubx_cfg_char_len {
	UBX_CFG_PRT_PORT_MODE_CHAR_LEN_5 = 0, /* Not supported */
	UBX_CFG_PRT_PORT_MODE_CHAR_LEN_6 = 1, /* Not supported */
	UBX_CFG_PRT_PORT_MODE_CHAR_LEN_7 = 2, /* Supported only with parity */
	UBX_CFG_PRT_PORT_MODE_CHAR_LEN_8 = 3,
};

enum ubx_cfg_parity {
	UBX_CFG_PRT_PORT_MODE_PARITY_EVEN = 0,
	UBX_CFG_PRT_PORT_MODE_PARITY_ODD = 1,
	UBX_CFG_PRT_PORT_MODE_PARITY_NONE = 4,
};

enum ubx_cfg_stop_bits {
	UBX_CFG_PRT_PORT_MODE_STOP_BITS_1 = 0,
	UBX_CFG_PRT_PORT_MODE_STOP_BITS_1_5 = 1,
	UBX_CFG_PRT_PORT_MODE_STOP_BITS_2 = 2,
	UBX_CFG_PRT_PORT_MODE_STOP_BITS_0_5 = 3,
};

struct ubx_cfg_prt {
	uint8_t port_id; /* See ubx_cfg_port_id */
	uint8_t reserved1;
	uint16_t rx_ready_pin;
	struct {
		uint32_t reserved1 : 6;
		uint32_t char_len : 2; /* See ubx_cfg_char_len */
		uint32_t reserved2 : 1;
		uint32_t parity : 3; /* See ubx_cfg_parity */
		uint32_t stop_bits : 2; /* See ubx_cfg_stop_bits */
		uint32_t reserved3 :18;
	} __packed mode;
	uint32_t baudrate;
	struct {
		uint32_t ubx : 1;
		uint32_t nmea : 1;
		uint32_t rtcm : 1;
		uint32_t reserved1 : 2;
		uint32_t rtcm3 : 1;
		uint32_t reserved2 : 10;
	} __packed in_proto_mask;
	struct {
		uint32_t ubx : 1;
		uint32_t nmea : 1;
		uint32_t reserved1 : 3;
		uint32_t rtcm3 : 1;
		uint32_t reserved2 : 10;
	} __packed out_proto_mask;
	uint16_t flags;
	uint16_t reserved2;
};

enum ubx_dyn_model {
	UBX_DYN_MODEL_PORTABLE = 0,
	UBX_DYN_MODEL_STATIONARY = 2,
	UBX_DYN_MODEL_PEDESTRIAN = 3,
	UBX_DYN_MODEL_AUTOMOTIVE = 4,
	UBX_DYN_MODEL_SEA = 5,
	UBX_DYN_MODEL_AIRBORNE_1G = 6,
	UBX_DYN_MODEL_AIRBORNE_2G = 7,
	UBX_DYN_MODEL_AIRBORNE_4G = 8,
	UBX_DYN_MODEL_WRIST = 9,
	UBX_DYN_MODEL_BIKE = 10,
};

enum ubx_fix_mode {
	UBX_FIX_MODE_2D_ONLY = 1,
	UBX_FIX_MODE_3D_ONLY = 2,
	UBX_FIX_MODE_AUTO = 3,
};

enum ubx_utc_standard {
	UBX_UTC_STANDARD_AUTOMATIC = 0,
	UBX_UTC_STANDARD_GPS = 3,
	UBX_UTC_STANDARD_GALILEO = 5,
	UBX_UTC_STANDARD_GLONASS = 6,
	UBX_UTC_STANDARD_BEIDOU = 7,
};

struct ubx_cfg_nav5 {
	struct {
		uint16_t dyn : 1;
		uint16_t min_elev : 1;
		uint16_t fix_mode : 1;
		uint16_t dr_lim : 1;
		uint16_t pos_mask : 1;
		uint16_t time_mask : 1;
		uint16_t static_hold_thresh : 1;
		uint16_t dgps_mask : 1;
		uint16_t cno_threshold : 1;
		uint16_t reserved : 1;
		uint16_t utc : 1;
		uint16_t reserved2 : 5;
	} __packed apply;
	uint8_t dyn_model; /* Dynamic platform model. See ubx_dyn_model */
	uint8_t fix_mode; /* Position fixing mode. See ubx_fix_mode */
	int32_t fixed_alt; /* Fixed altitude for 2D fix mode. Meters */
	uint32_t fixed_alt_var; /* Variance for Fixed altitude in 2D mode. Sq. meters */
	int8_t min_elev; /* Minimum Elevation to use a GNSS satellite in Navigation. Degrees */
	uint8_t dr_limit; /* Reserved */
	uint16_t p_dop; /* Position DOP mask */
	uint16_t t_dop; /* Time DOP mask */
	uint16_t p_acc; /* Position accuracy mask. Meters */
	uint16_t t_acc; /* Time accuracy mask. Meters */
	uint8_t static_hold_thresh; /* Static hold threshold. cm/s */
	uint8_t dgnss_timeout; /* DGNSS timeout. Seconds */
	uint8_t cno_thresh_num_svs; /* Number of satellites required above cno_thresh */
	uint8_t cno_thresh; /* C/N0 threshold for GNSS signals. dbHz */
	uint8_t reserved1[2];
	uint16_t static_hold_max_dist; /* Static hold distance threshold. Meters */
	uint8_t utc_standard; /* UTC standard to be used. See ubx_utc_standard */
	uint8_t reserved2[5];
} __packed;

enum ubx_cfg_rst_start_mode {
	UBX_CFG_RST_HOT_START = 0x0000,
	UBX_CFG_RST_WARM_START = 0x0001,
	UBX_CFG_RST_COLD_START = 0xFFFF,
};

enum ubx_cfg_rst_mode {
	UBX_CFG_RST_MODE_HW = 0x00,
	UBX_CFG_RST_MODE_SW = 0x01,
	UBX_CFG_RST_MODE_GNSS_STOP = 0x08,
	UBX_CFG_RST_MODE_GNSS_START = 0x09,
};

struct ubx_cfg_rst {
	uint16_t nav_bbr_mask;
	uint8_t reset_mode;
	uint8_t reserved;
};

enum ubx_cfg_rate_time_ref {
	UBX_CFG_RATE_TIME_REF_UTC = 0,
	UBX_CFG_RATE_TIME_REF_GPS = 1,
	UBX_CFG_RATE_TIME_REF_GLONASS = 2,
	UBX_CFG_RATE_TIME_REF_BEIDOU = 3,
	UBX_CFG_RATE_TIME_REF_GALILEO = 4,
	UBX_CFG_RATE_TIME_REF_NAVIC = 5,
};

struct ubx_cfg_rate {
	uint16_t meas_rate_ms;
	uint16_t nav_rate;
	uint16_t time_ref;
};

enum ubx_msg_id_nmea_std {
	UBX_MSG_ID_NMEA_STD_DTM = 0x0A,
	UBX_MSG_ID_NMEA_STD_GBQ = 0x44,
	UBX_MSG_ID_NMEA_STD_GBS = 0x09,
	UBX_MSG_ID_NMEA_STD_GGA = 0x00,
	UBX_MSG_ID_NMEA_STD_GLL = 0x01,
	UBX_MSG_ID_NMEA_STD_GLQ = 0x43,
	UBX_MSG_ID_NMEA_STD_GNQ = 0x42,
	UBX_MSG_ID_NMEA_STD_GNS = 0x0D,
	UBX_MSG_ID_NMEA_STD_GPQ = 0x40,
	UBX_MSG_ID_NMEA_STD_GRS = 0x06,
	UBX_MSG_ID_NMEA_STD_GSA = 0x02,
	UBX_MSG_ID_NMEA_STD_GST = 0x07,
	UBX_MSG_ID_NMEA_STD_GSV = 0x03,
	UBX_MSG_ID_NMEA_STD_RMC = 0x04,
	UBX_MSG_ID_NMEA_STD_THS = 0x0E,
	UBX_MSG_ID_NMEA_STD_TXT = 0x41,
	UBX_MSG_ID_NMEA_STD_VLW = 0x0F,
	UBX_MSG_ID_NMEA_STD_VTG = 0x05,
	UBX_MSG_ID_NMEA_STD_ZDA = 0x08,
};

enum ubx_msg_id_nmea_pubx {
	UBX_MSG_ID_NMEA_PUBX_CONFIG = 0x41,
	UBX_MSG_ID_NMEA_PUBX_POSITION = 0x00,
	UBX_MSG_ID_NMEA_PUBX_RATE = 0x40,
	UBX_MSG_ID_NMEA_PUBX_SVSTATUS = 0x03,
	UBX_MSG_ID_NMEA_PUBX_TIME = 0x04,
};

struct ubx_cfg_msg_rate {
	uint8_t class;
	uint8_t id;
	uint8_t rate;
};

struct ubx_mon_ver {
	char sw_ver[30];
	char hw_ver[10];
};

static inline uint16_t ubx_calc_checksum(const struct ubx_frame *frame, size_t len)
{
	uint8_t ck_a = 0;
	uint8_t ck_b = 0;
	const uint8_t *data = (const uint8_t *)frame;

	/** Mismatch in expected and actual length results in an invalid frame */
	if (len != UBX_FRM_SZ(frame->payload_size)) {
		return 0xFFFF;
	}

	for (int i = UBX_FRM_MSG_CLASS_IDX ; i < (UBX_FRM_SZ(frame->payload_size) - 2) ; i++) {
		ck_a = ck_a + data[i];
		ck_b = ck_b + ck_a;
	}

	return ((ck_a & 0xFF) | ((ck_b & 0xFF) << 8));
}

static inline int ubx_frame_encode(uint8_t class, uint8_t id,
				    const uint8_t *payload, size_t payload_len,
				    uint8_t *buf, size_t buf_len)
{
	if (buf_len < UBX_FRM_SZ(payload_len)) {
		return -EINVAL;
	}

	struct ubx_frame *frame = (struct ubx_frame *)buf;

	frame->preamble_sync_char_1 = UBX_PREAMBLE_SYNC_CHAR_1;
	frame->preamble_sync_char_2 = UBX_PREAMBLE_SYNC_CHAR_2;
	frame->class = class;
	frame->id = id;
	frame->payload_size = payload_len;
	memcpy(frame->payload_and_checksum, payload, payload_len);

	uint16_t checksum = ubx_calc_checksum(frame, UBX_FRM_SZ(payload_len));

	frame->payload_and_checksum[payload_len] = checksum & 0xFF;
	frame->payload_and_checksum[payload_len + 1] = (checksum >> 8) & 0xFF;

	return UBX_FRM_SZ(payload_len);
}

#define UBX_FRAME_DEFINE(_name, _frame)								   \
	const static struct ubx_frame _name = _frame

#define UBX_FRAME_ARRAY_DEFINE(_name, ...)							   \
	const struct ubx_frame *_name[] = {__VA_ARGS__};

#define UBX_FRAME_ACK_INITIALIZER(_class_id, _msg_id)						   \
	UBX_FRAME_INITIALIZER_PAYLOAD(UBX_CLASS_ID_ACK, UBX_MSG_ID_ACK, _class_id, _msg_id)

#define UBX_FRAME_NAK_INITIALIZER(_class_id, _msg_id)						   \
	UBX_FRAME_INITIALIZER_PAYLOAD(UBX_CLASS_ID_ACK, UBX_MSG_ID_NAK, _class_id, _msg_id)

#define UBX_FRAME_CFG_RST_INITIALIZER(_start_mode, _reset_mode)					   \
	UBX_FRAME_INITIALIZER_PAYLOAD(UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_RST,			   \
				      (_start_mode & 0xFF), ((_start_mode >> 8) & 0xFF),	   \
				       _reset_mode, 0)

#define UBX_FRAME_CFG_RATE_INITIALIZER(_meas_rate_ms, _nav_rate, _time_ref)			   \
	UBX_FRAME_INITIALIZER_PAYLOAD(UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_RATE,			   \
				      (_meas_rate_ms & 0xFF), ((_meas_rate_ms >> 8) & 0xFF),	   \
				      (_nav_rate & 0xFF), ((_nav_rate >> 8) & 0xFF),		   \
				      (_time_ref & 0xFF), ((_time_ref >> 8) & 0xFF))

#define UBX_FRAME_CFG_MSG_RATE_INITIALIZER(_class_id, _msg_id, _rate)				   \
	UBX_FRAME_INITIALIZER_PAYLOAD(UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_MSG,			   \
				      _class_id, _msg_id, _rate)

#define UBX_FRAME_INITIALIZER_PAYLOAD(_class_id, _msg_id, ...)					   \
	_UBX_FRAME_INITIALIZER_PAYLOAD(_class_id, _msg_id, __VA_ARGS__)

#define _UBX_FRAME_INITIALIZER_PAYLOAD(_class_id, _msg_id, ...)					   \
	{											   \
		.preamble_sync_char_1 = UBX_PREAMBLE_SYNC_CHAR_1,				   \
		.preamble_sync_char_2 = UBX_PREAMBLE_SYNC_CHAR_2,				   \
		.class = _class_id,								   \
		.id = _msg_id,									   \
		.payload_size = (NUM_VA_ARGS(__VA_ARGS__)) & 0xFFFF,				   \
		.payload_and_checksum = {							   \
			__VA_ARGS__,								   \
			UBX_CSUM(_class_id, _msg_id,						   \
				 ((NUM_VA_ARGS(__VA_ARGS__)) & 0xFF),				   \
				 (((NUM_VA_ARGS(__VA_ARGS__)) >> 8) & 0xFF),			   \
				 __VA_ARGS__),							   \
		},										   \
	}

#define UBX_FRAME_GET_INITIALIZER(_class_id, _msg_id)						   \
	{											   \
		.preamble_sync_char_1 = UBX_PREAMBLE_SYNC_CHAR_1,				   \
		.preamble_sync_char_2 = UBX_PREAMBLE_SYNC_CHAR_2,				   \
		.class = _class_id,								   \
		.id = _msg_id,									   \
		.payload_size = 0,								   \
		.payload_and_checksum = {							   \
			UBX_CSUM(_class_id, _msg_id, 0, 0),					   \
		},										   \
	}

#endif /* ZEPHYR_MODEM_UBX_PROTOCOL_ */

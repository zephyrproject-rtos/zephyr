/** @file
 * @brief NEO-M8 gnss module public API header file.
 *
 * Copyright (c) 2022 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GNSS_UBLOX_NEO_M8_H_
#define ZEPHYR_INCLUDE_DRIVERS_GNSS_UBLOX_NEO_M8_H_

#include <zephyr/types.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gnss/ublox_neo_m8_defines.h>

enum gnss_mode {
	Portable = 0,
	Stationary = 2,
	Pedestrian,
	Automotiv,
	Sea,
	Airbone1G,
	Airbone2G,
	Airbone4G,
	Wirst,
	Bike,
	LawnMower,
	KickScooter,
};

enum fix_mode {
	P_2D = 1,
	P_3D,
	AutoFix,
};

enum utc_standard {
	AutoUTC = 0,
	GPS = 3,
	GALILEO = 5,
	GLONASS,
	BEIDOU,
	NAVIC,
};

enum message_id {
	INVALID = -1,
	UNKNOWN,
	MESSAGE_RMC,
	MESSAGE_GGA,
	MESSAGE_GSA,
	MESSAGE_GLL,
	MESSAGE_GST,
	MESSAGE_GSV,
	MESSAGE_VTG,
	MESSAGE_ZDA,
};

enum ack_nack_return_codes {
	NACK = 150,
	ACK = 151,
};

struct neom8_config {
	const struct device *i2c_dev;
	uint16_t i2c_addr;
};

struct neom8_data {
	const struct device *neom8;

	struct k_sem lock;

	uint8_t _index;
	uint8_t _buffer[MAX_NMEA_SIZE];

	struct time {
		uint8_t hour;
		uint8_t min;
		uint8_t sec;
	} time;

	float longitude_min;
	float latitude_min;
	float longitude_deg;
	float latitude_deg;

	float altitude;

	char ind_longitude;
	char ind_latitude;

	uint8_t satellites;
};

typedef int (*neom8_api_fetch_data)(const struct device *dev);
typedef int (*neom8_api_send_ubx)(const struct device *dev, uint8_t class, uint8_t id,
				  uint8_t payload[], uint16_t length);
typedef int (*neom8_api_cfg_nav5)(const struct device *dev, enum gnss_mode g_mode,
				  enum fix_mode f_mode, int32_t fixed_alt, uint32_t fixed_alt_var,
				  int8_t min_elev, uint16_t p_dop, uint16_t t_dop, uint16_t p_acc,
				  uint16_t t_acc, uint8_t static_hold_thresh, uint8_t dgnss_timeout,
				  uint8_t cno_thresh_num_svs, uint8_t cno_thresh,
				  uint16_t static_hold_max_dist, enum utc_standard utc_strd);
typedef int (*neom8_api_cfg_gnss)(const struct device *dev, uint8_t msg_ver, uint8_t num_trk_ch_use,
				  uint8_t num_config_blocks, ...);
typedef int (*neom8_api_cfg_msg)(const struct device *dev, uint8_t msg_id, uint8_t rate);

typedef void (*neom8_api_get_time)(const struct device *dev, struct time *time);
typedef void (*neom8_api_get_latitude)(const struct device *dev, float *latitude);
typedef void (*neom8_api_get_ns)(const struct device *dev, char *ns);
typedef void (*neom8_api_get_longitude)(const struct device *dev, float *longitude);
typedef void (*neom8_api_get_ew)(const struct device *dev, char *ew);
typedef void (*neom8_api_get_altitude)(const struct device *dev, float *altitude);
typedef void (*neom8_api_get_satellites)(const struct device *dev, int *satellites);

__subsystem struct neom8_api {
	neom8_api_fetch_data fetch_data;
	neom8_api_send_ubx send_ubx;
	neom8_api_cfg_nav5 cfg_nav5;
	neom8_api_cfg_gnss cfg_gnss;
	neom8_api_cfg_msg cfg_msg;

	neom8_api_get_time get_time;
	neom8_api_get_latitude get_latitude;
	neom8_api_get_ns get_ns;
	neom8_api_get_longitude get_longitude;
	neom8_api_get_ew get_ew;
	neom8_api_get_altitude get_altitude;
	neom8_api_get_satellites get_satellites;
};

#endif /*ZEPHYR_INCLUDE_DRIVERS_GNSS_UBLOX_NEO_M8_H_*/

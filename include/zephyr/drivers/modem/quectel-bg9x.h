/*
 * Copyright (c) 2022 Sensorfy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_QUECTEL_BG9X_H
#define ZEPHYR_INCLUDE_DRIVERS_QUECTEL_BG9X_H

#ifdef __cplusplus
extern "C" {
#endif

#define BG9X_GNSS_DATA_UTC_LEN      64

struct bg9x_gnss_data {
	/**
	 * UTC in format ddmmyyhhmmss.s
	 */
	float utc[BG9X_GNSS_DATA_UTC_LEN];
	/**
	 * Latitude in 10^-5 degree.
	 */
	int32_t lat;
	/**
	 * Longitude in 10^-5 degree.
	 */
	int32_t lon;
	/**
	 * Altitude in mm.
	 */
	int32_t alt;
	/**
	 * Horizontal dilution of precision in 10^-1.
	 */
	uint16_t hdop;
	/**
	 * Course over ground in 10^-2 degree.
	 */
	uint16_t cog;
	/**
	 * Speed in 10^-1 km/h.
	 */
	uint16_t kmh;
	/**
	 * Number of satellites in use.
	 */
	uint16_t nsat;
};


/**
 * @brief Starts the modem in gnss operation mode.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int mdm_bg9x_start_gnss(void);

/**
 * @brief Query gnss position form the modem.
 *
 * @return 0 on success. If no fix is acquired yet -EAGAIN is returned.
 *         Otherwise <0 is returned.
 */
int mdm_bg9x_query_gnss(struct bg9x_gnss_data *data);

/**
 * @brief Stops the gnss operation mode of the modem.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int mdm_bg9x_stop_gnss(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_QUECTEL_BG9X_H */

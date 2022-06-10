/*
 * Copyright (c) 2022 Abel Sensors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gnss/ublox_neo_m8.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(neom8, CONFIG_NEOM8_LOG_LEVEL);

#define NEO_SERIAL DT_NODELABEL(neom8)

static const struct device *neo_dev;
static struct neom8_api *neo_api;

void main(void)
{
	int rc;
	struct time neotime;
	float lat;
	char ns;
	float lon;
	char ew;
	float alt;
	int sat;
	int i = 0;

	neo_dev = DEVICE_DT_GET(NEO_SERIAL);

	if (!device_is_ready(neo_dev)) {
		LOG_ERR("%s Device not ready", neo_dev->name);
		return;
	}

	neo_api = (struct neom8_api *)neo_dev->api;

	neo_api->cfg_nav5(neo_dev, Stationary, P_2D, 0, 1, 5, 100, 100, 100, 350, 0, 60, 0, 0, 0,
			  AutoUTC);
	neo_api->cfg_gnss(neo_dev, 0, 32, 5, 0, 8, 16, 0, 0x01010001, 1, 1, 3, 0, 0x01010001, 3, 8,
			  16, 0, 0x01010000, 5, 0, 3, 0, 0x01010001, 6, 8, 14, 0, 0x01010001);
	neo_api->cfg_msg(neo_dev, NMEA_DTM, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GBQ, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GBS, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GLL, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GLQ, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GNQ, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GNS, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GPQ, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GRS, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GSA, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GST, 0);
	neo_api->cfg_msg(neo_dev, NMEA_GSV, 0);
	neo_api->cfg_msg(neo_dev, NMEA_RMC, 0);
	neo_api->cfg_msg(neo_dev, NMEA_THS, 0);
	neo_api->cfg_msg(neo_dev, NMEA_TXT, 0);
	neo_api->cfg_msg(neo_dev, NMEA_VLW, 0);
	neo_api->cfg_msg(neo_dev, NMEA_VTG, 0);
	neo_api->cfg_msg(neo_dev, NMEA_ZDA, 0);

	while (1) {
		rc = neo_api->fetch_data(neo_dev);
		if (rc) {
			LOG_ERR("Error %d while reading data", rc);
			return;
		}

		if (i++ == 1000) {
			i = 0;

			neo_api->get_time(neo_dev, &neotime);
			neo_api->get_latitude(neo_dev, &lat);
			neo_api->get_ns(neo_dev, &ns);
			neo_api->get_longitude(neo_dev, &lon);
			neo_api->get_ew(neo_dev, &ew);
			neo_api->get_altitude(neo_dev, &alt);
			neo_api->get_satellites(neo_dev, &sat);

			LOG_INF("Hour: %d\n\r", neotime.hour);
			LOG_INF("Minute: %d\n\r", neotime.min);
			LOG_INF("Second: %d\n\r", neotime.sec);
			LOG_INF("Latitude: %.5f\n\r", lat);
			LOG_INF("North/South: %c\n\r", ns);
			LOG_INF("Longitude: %.5f\n\r", lon);
			LOG_INF("East/West: %c\n\r", ew);
			LOG_INF("Altitude: %.2f\n\r", alt);
			LOG_INF("Satellites: %d\n\r\n\r", sat);
		}
	}
}

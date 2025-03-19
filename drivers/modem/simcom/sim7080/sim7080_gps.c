/*
 * Copyright (C) 2025 Metratec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_sim7080_gps, CONFIG_MODEM_LOG_LEVEL);

#include "sim7080.h"

static struct sim7080_gnss_data gnss_data;

/**
 * Get the next parameter from the gnss phrase.
 *
 * @param src The source string supported on first call.
 * @param delim The delimiter of the parameter list.
 * @param saveptr Pointer for subsequent parses.
 * @return On success a pointer to the parameter. On failure
 *         or end of string NULL is returned.
 *
 * This function is used instead of strtok because strtok would
 * skip empty parameters, which is not desired. The modem may
 * omit parameters which could lead to a incorrect parse.
 */
static char *gnss_get_next_param(char *src, const char *delim, char **saveptr)
{
	char *start, *del;

	if (src) {
		start = src;
	} else {
		start = *saveptr;
	}

	/* Illegal start string. */
	if (!start) {
		return NULL;
	}

	/* End of string reached. */
	if (*start == '\0' || *start == '\r') {
		return NULL;
	}

	del = strstr(start, delim);
	if (!del) {
		return NULL;
	}

	*del = '\0';
	*saveptr = del + 1;

	if (del == start) {
		return NULL;
	}

	return start;
}

static void gnss_skip_param(char **saveptr)
{
	gnss_get_next_param(NULL, ",", saveptr);
}

/**
 * Splits float parameters of the CGNSINF response on '.'
 *
 * @param src Null terminated string containing the float.
 * @param f1 Resulting number part of the float.
 * @param f2 Resulting fraction part of the float.
 * @return 0 if parsing was successful. Otherwise <0 is returned.
 *
 * If the number part of the float is negative f1 and f2 will be
 * negative too.
 */
static int gnss_split_on_dot(const char *src, int32_t *f1, int32_t *f2)
{
	char *dot = strchr(src, '.');

	if (!dot) {
		return -1;
	}

	*dot = '\0';

	*f1 = (int32_t)strtol(src, NULL, 10);
	*f2 = (int32_t)strtol(dot + 1, NULL, 10);

	if (*f1 < 0) {
		*f2 = -*f2;
	}

	return 0;
}

/**
 * Parses cgnsinf response into the gnss_data structure.
 *
 * @param gps_buf Null terminated buffer containing the response.
 * @return 0 on successful parse. Otherwise <0 is returned.
 */
static int parse_cgnsinf(char *gps_buf)
{
	char *saveptr;
	int ret;
	int32_t number, fraction;

	char *run_status = gnss_get_next_param(gps_buf, ",", &saveptr);

	if (run_status == NULL) {
		goto error;
	} else if (*run_status != '1') {
		goto error;
	}

	char *fix_status = gnss_get_next_param(NULL, ",", &saveptr);

	if (fix_status == NULL) {
		goto error;
	} else if (*fix_status != '1') {
		goto error;
	}

	char *utc = gnss_get_next_param(NULL, ",", &saveptr);

	if (utc == NULL) {
		goto error;
	}

	char *lat = gnss_get_next_param(NULL, ",", &saveptr);

	if (lat == NULL) {
		goto error;
	}

	char *lon = gnss_get_next_param(NULL, ",", &saveptr);

	if (lon == NULL) {
		goto error;
	}

	char *alt = gnss_get_next_param(NULL, ",", &saveptr);
	char *speed = gnss_get_next_param(NULL, ",", &saveptr);
	char *course = gnss_get_next_param(NULL, ",", &saveptr);

	/* discard fix mode and reserved*/
	gnss_skip_param(&saveptr);
	gnss_skip_param(&saveptr);

	char *hdop = gnss_get_next_param(NULL, ",", &saveptr);

	if (hdop == NULL) {
		goto error;
	}

	gnss_data.run_status = 1;
	gnss_data.fix_status = 1;

	strncpy(gnss_data.utc, utc, sizeof(gnss_data.utc) - 1);

	ret = gnss_split_on_dot(lat, &number, &fraction);
	if (ret != 0) {
		goto error;
	}
	gnss_data.lat = number * 10000000 + fraction * 10;

	ret = gnss_split_on_dot(lon, &number, &fraction);
	if (ret != 0) {
		goto error;
	}
	gnss_data.lon = number * 10000000 + fraction * 10;

	if (alt) {
		ret = gnss_split_on_dot(alt, &number, &fraction);
		if (ret != 0) {
			goto error;
		}
		gnss_data.alt = number * 1000 + fraction;
	} else {
		gnss_data.alt = 0;
	}

	ret = gnss_split_on_dot(hdop, &number, &fraction);
	if (ret != 0) {
		goto error;
	}
	gnss_data.hdop = number * 100 + fraction * 10;

	if (course) {
		ret = gnss_split_on_dot(course, &number, &fraction);
		if (ret != 0) {
			goto error;
		}
		gnss_data.cog = number * 100 + fraction * 10;
	} else {
		gnss_data.cog = 0;
	}

	if (speed) {
		ret = gnss_split_on_dot(speed, &number, &fraction);
		if (ret != 0) {
			goto error;
		}
		gnss_data.kmh = number * 10 + fraction / 10;
	} else {
		gnss_data.kmh = 0;
	}

	return 0;
error:
	memset(&gnss_data, 0, sizeof(gnss_data));
	return -1;
}

/*
 * Parses the +CGNSINF Gnss response.
 *
 * The CGNSINF command has the following parameters but
 * not all parameters are set by the module:
 *
 *  +CGNSINF: <GNSS run status>,<Fix status>,<UTC date & Time>,
 *            <Latitude>,<Longitude>,<MSL Altitude>,<Speed Over Ground>,
 *            <Course Over Ground>,<Fix Mode>,<Reserved1>,<HDOP>,<PDOP>,
 *            <VDOP>,<Reserved2>,<GNSS Satellites in View>,<Reserved3>,
 *            <HPA>,<VPA>
 *
 */
MODEM_CMD_DEFINE(on_cmd_cgnsinf)
{
	char gps_buf[MDM_GNSS_PARSER_MAX_LEN];
	size_t out_len = net_buf_linearize(gps_buf, sizeof(gps_buf) - 1, data->rx_buf, 0, len);

	gps_buf[out_len] = '\0';
	return parse_cgnsinf(gps_buf);
}

int mdm_sim7080_query_gnss(struct sim7080_gnss_data *data)
{
	int ret;
	struct modem_cmd cmds[] = { MODEM_CMD("+CGNSINF: ", on_cmd_cgnsinf, 0U, NULL) };

	if (sim7080_get_state() != SIM7080_STATE_GNSS) {
		LOG_ERR("GNSS functionality is not enabled!!");
		return -1;
	}

    memset(&gnss_data, 0, sizeof(gnss_data));

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), "AT+CGNSINF",
			     &mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		return ret;
	}

	if (!gnss_data.run_status || !gnss_data.fix_status) {
		return -EAGAIN;
	}

	if (data) {
		memcpy(data, &gnss_data, sizeof(gnss_data));
	}

	return ret;
}

int mdm_sim7080_start_gnss(void)
{
	int ret;

	sim7080_change_state(SIM7080_STATE_INIT);
	k_work_cancel_delayable(&mdata.rssi_query_work);

	ret = modem_autobaud();
	if (ret < 0) {
		LOG_ERR("Failed to start modem!!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT+CGNSCOLD",
			     &mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		return -1;
	}

	sim7080_change_state(SIM7080_STATE_GNSS);
	return 0;
}
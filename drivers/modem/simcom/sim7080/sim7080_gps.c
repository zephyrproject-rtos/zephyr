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

static uint8_t cgnscpy_ret;

MODEM_CMD_DEFINE(on_cmd_cgnscpy)
{
	cgnscpy_ret = (uint8_t)strtoul(argv[0], NULL, 10);
	LOG_INF("CGNSCPY: %u", cgnscpy_ret);
	return 0;
}

static int16_t xtra_diff_h, xtra_duration_h;
static struct tm *xtra_inject;

MODEM_CMD_DEFINE(on_cmd_cgnsxtra)
{
	xtra_diff_h = (int16_t)strtol(argv[0], NULL, 10);
	xtra_duration_h = (int16_t)strtol(argv[1], NULL, 10);
	int ret = sim7080_utils_parse_time(argv[2], argv[3], xtra_inject);

	LOG_INF("XTRA validity: diff=%d, duration=%d, inject=%s,%s",
		xtra_diff_h,
		xtra_duration_h,
		argv[2],
		argv[3]);
	return ret;
}

int mdm_sim7080_query_xtra_validity(int16_t *diff_h, int16_t *duration_h, struct tm *inject)
{
	struct modem_cmd cmds[] = { MODEM_CMD("+CGNSXTRA: ", on_cmd_cgnsxtra, 4U, ",") };
	int ret = -EINVAL;

	if (!diff_h || !duration_h || !inject) {
		goto out;
	}

	xtra_inject = inject;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), "AT+CGNSXTRA",
				 &mdata.sem_response, K_SECONDS(2));
	if (ret != 0) {
		LOG_ERR("Failed to query xtra validity");
		goto out;
	}

	*diff_h = xtra_diff_h;
	*duration_h = xtra_duration_h;

out:
	xtra_inject = NULL;
	return ret;
}

static int sim7080_start_gnss_ext(bool xtra)
{
	int ret = -EALREADY;

	if (sim7080_get_state() == SIM7080_STATE_GNSS) {
		LOG_WRN("Modem already in gnss state");
		goto out;
	} else if (sim7080_get_state() != SIM7080_STATE_IDLE) {
		LOG_WRN("Can only activate gnss from idle state");
		ret = -EINVAL;
		goto out;
	}

	/* Power GNSS unit */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT+CGNSPWR=1",
				 &mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		LOG_ERR("Failed to power on gnss: %d", ret);
		goto out;
	}

	if (xtra == false) {
		goto coldstart;
	}

	struct modem_cmd cmds[] = { MODEM_CMD("+CGNSCPY: ", on_cmd_cgnscpy, 1U, "") };

	cgnscpy_ret = UINT8_MAX;

	/* Copy the xtra file to gnss unit */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), "AT+CGNSCPY",
				 &mdata.sem_response, K_SECONDS(5));
	if (ret < 0) {
		LOG_WRN("Failed to copy xtra file. Performing cold start");
		goto coldstart;
	}

	if (cgnscpy_ret != 0) {
		LOG_WRN("CGNSCPY returned %u. Performing cold start", cgnscpy_ret);
		goto coldstart;
	}

	/* Query the xtra file validity */
	int16_t diff, duration;
	struct tm inject;

	ret = mdm_sim7080_query_xtra_validity(&diff, &duration, &inject);
	if (ret != 0) {
		LOG_WRN("Could not query xtra validity. Performing cold start");
		goto coldstart;
	}

	if (diff < 0) {
		LOG_WRN("XTRA file is not valid. Performing cold start");
		goto coldstart;
	}

	/* Enable xtra functionality */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT+CGNSXTRA=1",
				 &mdata.sem_response, K_SECONDS(5));
	if (ret < 0) {
		LOG_WRN("Failed to enable xtra. Performing cold start");
		goto coldstart;
	}

coldstart:
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT+CGNSCOLD",
				 &mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		LOG_ERR("Failed to start gnss: %d", ret);
		goto out;
	}

	sim7080_change_state(SIM7080_STATE_GNSS);
out:
	return ret;
}

int mdm_sim7080_start_gnss(void)
{
	return sim7080_start_gnss_ext(false);
}

int mdm_sim7080_start_gnss_xtra(void)
{
	return sim7080_start_gnss_ext(true);
}

int mdm_sim7080_stop_gnss(void)
{
	int ret = -EINVAL;

	if (sim7080_get_state() != SIM7080_STATE_GNSS) {
		LOG_WRN("Modem not in gnss state");
		goto out;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT+CGNSPWR=0",
				 &mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		LOG_ERR("Failed to power on gnss: %d", ret);
		goto out;
	}

	sim7080_change_state(SIM7080_STATE_IDLE);
out:
	return ret;
}

int mdm_sim7080_download_xtra(uint8_t server_id, const char *f_name)
{
	char buf[sizeof("AT+HTTPTOFS=\"http://iot#.xtracloud.net/xtra3##_72h.bin\",\"/customer/Xtra3.bin\"")];
	int ret = -ENOTCONN;

	if (sim7080_get_state() != SIM7080_STATE_NETWORKING) {
		LOG_WRN("Need network to download xtra file");
		goto out;
	}

	ret = snprintk(buf, sizeof(buf), "AT+HTTPTOFS=\"http://iot%hhu.xtracloud.net/%s\",\"/customer/Xtra3.bin\"",
		server_id, f_name);
	if (ret < 0) {
		LOG_ERR("Failed to format xtra download");
		goto out;
	}

	mdata.http_status = 0;

	/* Download xtra file */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf,
				 &mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		LOG_ERR("Failed to download xtra file");
		goto out;
	}

	/* Wait for HTTP status code */
	ret = k_sem_take(&mdata.sem_http, K_SECONDS(60));
	if (ret != 0) {
		LOG_ERR("Waiting for http completion failed");
		goto out;
	}

	if (mdata.http_status != 200) {
		LOG_ERR("HTTP request failed with: %u", mdata.http_status);
		ret = -1;
	}

out:
	return ret;
}

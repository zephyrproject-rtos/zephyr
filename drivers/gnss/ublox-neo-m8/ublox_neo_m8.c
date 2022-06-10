/*
 * Copyright (c) 2022 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT u_blox_neom8

#include <ctype.h>
#include <device.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <drivers/gnss/ublox_neo_m8.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(neom8, CONFIG_NEOM8_LOG_LEVEL);

#define TO_LITTLE_ENDIAN(data, b)                                                                  \
	for (int i = 0; i < sizeof(data); i++) {                                                   \
		b[i] = (data >> (i * 8)) & 0xFF;                                                   \
	}

static void neom8_get_time(const struct device *dev, struct time *time)
{
	struct neom8_data *data = dev->data;
	*time = data->time;
}

static void neom8_get_latitude(const struct device *dev, float *latitude)
{
	struct neom8_data *data = dev->data;
	*latitude = data->latitude_deg;
}

static void neom8_get_ns(const struct device *dev, char *ns)
{
	struct neom8_data *data = dev->data;
	*ns = data->ind_latitude;
}

static void neom8_get_longitude(const struct device *dev, float *longitude)
{
	struct neom8_data *data = dev->data;
	*longitude = data->longitude_deg;
}

static void neom8_get_ew(const struct device *dev, char *ew)
{
	struct neom8_data *data = dev->data;
	*ew = data->ind_longitude;
}

static void neom8_get_altitude(const struct device *dev, float *altitude)
{
	struct neom8_data *data = dev->data;
	*altitude = data->altitude;
}

static void neom8_get_satellites(const struct device *dev, int *satellites)
{
	struct neom8_data *data = dev->data;
	*satellites = data->satellites;
}

static int hex2int(uint8_t c)
{
	int a_offset = 10;

	if (c >= '0' && c <= '9') {
		return c - '0';
	}

	if (c >= 'A' && c <= 'F') {
		return c - 'A' + a_offset;
	}

	if (c >= 'a' && c <= 'f') {
		return c - 'a' + a_offset;
	}

	return -EINVAL;
}

static int read_register(const struct device *dev, uint8_t addr, char *buffer)
{
	const struct neom8_config *cfg = dev->config;

	int rc = i2c_write_read(cfg->i2c_dev, cfg->i2c_addr, &addr, sizeof(addr), buffer, 1);

	return rc;
}

static int write_register(const struct device *dev, char *buffer, uint16_t length)
{
	const struct neom8_config *cfg = dev->config;
	uint8_t data[length + 1];
	int rc = 0;

	data[0] = 0xFF;
	memcpy(data + 1, buffer, length);

	rc = i2c_write(cfg->i2c_dev, data, sizeof(data), cfg->i2c_addr);

	return rc;
}

char *strsep(char **stringp, const char *delim)
{
	char *rv = *stringp;

	if (rv) {
		*stringp += strcspn(*stringp, delim);
		if (**stringp) {
			*(*stringp)++ = '\0';
		} else {
			*stringp = NULL;
		}
	}

	return rv;
}

static void neom8_parse_comma_del(char *buffer, char *fields[20])
{
	int i = 0;
	const char delim = ',';
	char *string = buffer;
	char *found;

	while ((found = strsep(&string, &delim))) {
		if (found[0] == '\0') {
			found = "-";
		}
		fields[i++] = found;
	}
}

static int message_check(const char *buffer, bool strict)
{
	uint8_t checksum = 0x00;
	uint8_t upper, lower, expected;

	if (strlen(buffer) > 83) {
		return 0;
	}

	if (*buffer++ != '$') {
		return 0;
	}

	while (*buffer && *buffer != '*' && isprint((uint8_t)*buffer)) {
		checksum ^= *buffer++;
	}

	if (*buffer == '*') {
		buffer++;
		upper = hex2int(*buffer++);

		if (upper == -1) {
			return 0;
		}

		lower = hex2int(*buffer++);

		if (lower == -1) {
			return 0;
		}

		expected = upper << 4 | lower;

		if (checksum != expected) {
			return 0;
		}
	} else if (strict) {
		return 0;
	}

	if (*buffer && strcmp(buffer, "\n") && strcmp(buffer, "\r\n")) {
		return 0;
	}

	return 1;
}

static enum message_id get_message_id(const char *buffer, bool strict)
{
	const char *temp = buffer + 3;

	if (!message_check(buffer, strict)) {
		return INVALID;
	}

	if (!strncmp(temp, "RMC", 3)) {
		return MESSAGE_RMC;
	}

	if (!strncmp(temp, "GGA", 3)) {
		return MESSAGE_GGA;
	}

	if (!strncmp(temp, "GSA", 3)) {
		return MESSAGE_GSA;
	}

	if (!strncmp(temp, "GLL", 3)) {
		return MESSAGE_GLL;
	}

	if (!strncmp(temp, "GST", 3)) {
		return MESSAGE_GST;
	}

	if (!strncmp(temp, "GSV", 3)) {
		return MESSAGE_GSV;
	}

	if (!strncmp(temp, "VTG", 3)) {
		return MESSAGE_VTG;
	}

	if (!strncmp(temp, "ZDA", 3)) {
		return MESSAGE_ZDA;
	}

	return UNKNOWN;
}

static float to_degrees(float deg_min)
{
	float degrees = (int)(deg_min / 100.0);
	float minutes = deg_min - (100.0 * degrees);

	return (degrees + (minutes / 60.0));
}

void neom8_parse_gga(const struct device *dev, char *fields[20])
{
	struct neom8_data *data = dev->data;
	uint8_t hour[3], min[3], sec[3];
	uint8_t degrees_minutes[6], decimal_minutes[6];
	uint32_t wholeNum;
	uint32_t decimal;
	uint8_t meter[4], meter_decimal[2];

	if (strcmp(fields[1], "-")) {
		strncpy(hour, &fields[1][0], 2);
		hour[2] = '\0';
		strncpy(min, &fields[1][2], 2);
		min[2] = '\0';
		strncpy(sec, &fields[1][4], 2);
		sec[2] = '\0';
		data->time.hour = atoi(hour);
		data->time.min = atoi(min);
		data->time.sec = atoi(sec);
	}

	if (strcmp(fields[2], "-")) {
		strncpy(degrees_minutes, &fields[2][0], 4);
		degrees_minutes[4] = '\0';
		strncpy(decimal_minutes, &fields[2][5], 5);
		decimal_minutes[5] = '\0';
		wholeNum = atoi(degrees_minutes);
		decimal = atoi(decimal_minutes);
		data->latitude_min = (float)((float)wholeNum + ((float)decimal / 100000));
		data->latitude_deg = to_degrees(data->latitude_min);
	}

	if (strcmp(fields[3], "-")) {
		strncpy(&data->ind_latitude, fields[3], 1);
	}

	if (strcmp(fields[4], "-")) {
		strncpy(degrees_minutes, &fields[4][0], 5);
		degrees_minutes[5] = '\0';
		strncpy(decimal_minutes, &fields[4][6], 5);
		decimal_minutes[5] = '\0';
		wholeNum = atoi(degrees_minutes);
		decimal = atoi(decimal_minutes);
		data->longitude_min = (float)((float)wholeNum + ((float)decimal / 100000));
		data->longitude_deg = to_degrees(data->longitude_min);
	}

	if (strcmp(fields[5], "-")) {
		strncpy(&data->ind_longitude, fields[5], 1);
	}

	if (strcmp(fields[7], "-")) {
		data->satellites = atoi(fields[7]);
	}

	if (strcmp(fields[9], "-")) {
		strncpy(meter, &fields[9][0], 3);
		meter[3] = '\0';
		strncpy(meter_decimal, &fields[9][4], 1);
		meter_decimal[1] = '\0';
		wholeNum = atoi(meter);
		decimal = atoi(meter_decimal);
		data->altitude = (float)((float)wholeNum + ((float)decimal / 10));
	}
}

static int neom8_parse_data(const struct device *dev)
{
	struct neom8_data *data = dev->data;
	char tmp[MAX_NMEA_SIZE];
	char *fields[20] = { 0 };

	strcpy(tmp, data->_buffer);

	int message_id = get_message_id(tmp, false);

	neom8_parse_comma_del(tmp, fields);

	switch (message_id) {
		case MESSAGE_RMC: {
			break;
		}
		case MESSAGE_GGA: {
			neom8_parse_gga(dev, fields);
			break;
		}
		case MESSAGE_GSA: {
			break;
		}
		case MESSAGE_GLL: {
			break;
		}
		case MESSAGE_GST: {
			break;
		}
		case MESSAGE_GSV: {
			break;
		}
		case MESSAGE_VTG: {
			break;
		}
		case MESSAGE_ZDA: {
			break;
		}
		default: {
			break;
		}
	}

	free(tmp);

	return 0;
}

static int neom8_get_available(const struct device *dev)
{
	uint8_t high_byte, low_byte;
	int rc;

	rc = read_register(dev, NBYTES_HIGH_ADDR, &high_byte);
	if (rc) {
		LOG_ERR("Failed to read number of bytes HIGH from %s", dev->name);
		return -1;
	}

	rc = read_register(dev, NBYTES_LOW_ADDR, &low_byte);
	if (rc) {
		LOG_ERR("Failed to read number of bytes LOW from %s", dev->name);
		return -1;
	}

	if (high_byte == 0xff && low_byte == 0xff) {
		return -1;
	}

	return (high_byte << 8) | low_byte;
}

static int neom8_fetch_data(const struct device *dev)
{
	struct neom8_data *data = dev->data;
	uint16_t n_bytes;
	uint8_t c;
	int rc;

	k_sem_take(&data->lock, K_FOREVER);

	n_bytes = neom8_get_available(dev);

	if (n_bytes > 255) {
		n_bytes = 255;
	}

	while (n_bytes) {
		rc = read_register(dev, DATA_STREAM_ADDR, &c);

		if (rc) {
			LOG_ERR("Failed to read data stream from %s", dev->name);
			goto out;
		}

		if (c == '$') {
			data->_index = 0;
		}

		if (data->_index < ((uint8_t)sizeof(data->_buffer) - 1)) {
			data->_buffer[data->_index++] = c;

			if (c == '\n' && data->_buffer[0] == '$') {
				data->_buffer[data->_index++] = '\0';

				neom8_parse_data(dev);
			}
		}

		n_bytes--;
	}
out:
	k_sem_give(&data->lock);

	return rc;
}

static int neom8_send_ubx(const struct device *dev, uint8_t class, uint8_t id, uint8_t payload[],
			  uint16_t length)
{
	struct neom8_data *data = dev->data;
	uint8_t ckA = 0;
	uint8_t ckB = 0;
	uint8_t c;
	uint8_t response[10];
	int rc;

	const unsigned int cmdLength = 8 + length;
	uint8_t cmd[cmdLength];

	cmd[0] = 0xb5;
	cmd[1] = 0x62;
	cmd[2] = class;
	cmd[3] = id;
	cmd[4] = (length & 0xff);
	cmd[5] = (length >> 8);
	memcpy(&cmd[6], payload, length);

	for (unsigned int i = 2; i < (cmdLength - 2); i++) {
		ckA += cmd[i];
		ckB += ckA;
	}

	cmd[cmdLength - 2] = ckA;
	cmd[cmdLength - 1] = ckB;

	k_sem_take(&data->lock, K_FOREVER);

	rc = write_register(dev, cmd, cmdLength);
	if (rc) {
		LOG_ERR("Failed sending UBX frame to %s", dev->name);
		goto out;
	}

	while (1) {
		rc = read_register(dev, DATA_STREAM_ADDR, &c);

		if (rc) {
			LOG_ERR("Failed to read data stream from %s", dev->name);
			goto out;
		}

		if (c == 0xB5) {
			response[0] = c;

			for (int i = 1; i < 10; i++) {
				rc = read_register(dev, DATA_STREAM_ADDR, &c);

				if (rc) {
					LOG_ERR("Failed to read data stream from %s",
						dev->name);
					goto out;
				}

				response[i] = c;
			}

			break;
		}
	}

	if (response[3] == 0x00) {
		rc = NACK;
	} else if (response[3] == 0x01) {
		rc = ACK;
	}

out:
	k_sem_give(&data->lock);

	return rc;
}

static int neom8_cfg_nav5(const struct device *dev, enum gnss_mode g_mode, enum fix_mode f_mode,
			  int32_t fixed_alt, uint32_t fixed_alt_var, int8_t min_elev,
			  uint16_t p_dop, uint16_t t_dop, uint16_t p_acc, uint16_t t_acc,
			  uint8_t static_hold_thresh, uint8_t dgnss_timeout,
			  uint8_t cno_thresh_num_svs, uint8_t cno_thresh,
			  uint16_t static_hold_max_dist, enum utc_standard utc_strd)
{
	uint8_t payload[MAX_PAYLOAD_SIZE];
	int8_t fixed_alt_le[4] = { 0 };
	uint8_t fixed_alt_var_le[4] = { 0 };
	uint8_t p_dop_le[2] = { 0 };
	uint8_t t_dop_le[2] = { 0 };
	uint8_t p_acc_le[2] = { 0 };
	uint8_t t_acc_le[2] = { 0 };
	uint8_t static_hold_max_dist_le[2] = { 0 };
	int rc;

	TO_LITTLE_ENDIAN(fixed_alt, fixed_alt_le);
	TO_LITTLE_ENDIAN(fixed_alt_var, fixed_alt_var_le);
	TO_LITTLE_ENDIAN(p_dop, p_dop_le);
	TO_LITTLE_ENDIAN(t_dop, t_dop_le);
	TO_LITTLE_ENDIAN(p_acc, p_acc_le);
	TO_LITTLE_ENDIAN(t_acc, t_acc_le);
	TO_LITTLE_ENDIAN(static_hold_max_dist, static_hold_max_dist_le);

	payload[0] = 0xff;
	payload[1] = 0x05;
	payload[2] = g_mode;
	payload[3] = f_mode;
	payload[4] = fixed_alt_le[0];
	payload[5] = fixed_alt_le[1];
	payload[6] = fixed_alt_le[2];
	payload[7] = fixed_alt_le[3];
	payload[8] = fixed_alt_var_le[0];
	payload[9] = fixed_alt_var_le[1];
	payload[10] = fixed_alt_var_le[2];
	payload[11] = fixed_alt_var_le[3];
	payload[12] = min_elev;
	payload[14] = p_dop_le[0];
	payload[15] = p_dop_le[1];
	payload[16] = t_dop_le[0];
	payload[17] = t_dop_le[1];
	payload[18] = p_acc_le[0];
	payload[19] = p_acc_le[1];
	payload[20] = t_acc_le[0];
	payload[21] = t_acc_le[1];
	payload[22] = static_hold_thresh;
	payload[23] = dgnss_timeout;
	payload[24] = cno_thresh_num_svs;
	payload[25] = cno_thresh;
	payload[28] = static_hold_max_dist_le[0];
	payload[29] = static_hold_max_dist_le[1];
	payload[30] = utc_strd;

	rc = neom8_send_ubx(dev, UBX_CLASS_CFG, UBX_CFG_NAV5, payload, 36);
	if (rc == NACK) {
		LOG_ERR("Config NAV5 not acknowledged %s", dev->name);
	} else if (rc == ACK) {
		LOG_INF("Config NAV5 acknowledged %s", dev->name);
	} else if (rc) {
		LOG_ERR("Error %d config NAV5 for %s", rc, dev->name);
	}

	return rc;
}

static int neom8_cfg_gnss(const struct device *dev, uint8_t msg_ver, uint8_t num_trk_ch_use,
			  uint8_t num_config_blocks, ...)
{
	uint8_t payload[MAX_PAYLOAD_SIZE];
	uint8_t flags_le[4] = { 0 };
	uint32_t flags;
	va_list ap;
	int rc;

	va_start(ap, num_config_blocks);

	payload[0] = msg_ver;
	payload[2] = num_trk_ch_use;
	payload[3] = num_config_blocks;

	for (int i = 0; i < num_config_blocks; i++) {
		payload[4 + (8 * i)] = (uint8_t)va_arg(ap, int);
		payload[5 + (8 * i)] = (uint8_t)va_arg(ap, int);
		payload[6 + (8 * i)] = (uint8_t)va_arg(ap, int);
		payload[7 + (8 * i)] = (uint8_t)va_arg(ap, int);

		flags = (uint32_t)va_arg(ap, int);
		TO_LITTLE_ENDIAN(flags, flags_le);
		payload[8 + (8 * i)] = flags_le[0];
		payload[9 + (8 * i)] = flags_le[1];
		payload[10 + (8 * i)] = flags_le[2];
		payload[11 + (8 * i)] = flags_le[3];
	}

	va_end(ap);

	rc = neom8_send_ubx(dev, UBX_CLASS_CFG, UBX_CFG_GNSS, payload,
			    (4 + (8 * num_config_blocks)));
	if (rc == NACK) {
		LOG_ERR("Config GNSS not acknowledged %s", dev->name);
	} else if (rc == ACK) {
		LOG_INF("Config GNSS acknowledged %s", dev->name);
	} else if (rc) {
		LOG_ERR("Error %d config GNSS for %s", rc, dev->name);
	}

	return rc;
}

static int neom8_cfg_msg(const struct device *dev, uint8_t msg_id, uint8_t rate)
{
	uint8_t payload[MAX_PAYLOAD_SIZE];
	int rc;

	payload[0] = UBX_CLASS_NMEA;
	payload[1] = msg_id;
	payload[2] = rate;

	rc = neom8_send_ubx(dev, UBX_CLASS_CFG, UBX_CFG_MSG, payload, 3);
	if (rc == NACK) {
		LOG_ERR("Config MSG not acknowledged %s", dev->name);
	} else if (rc == ACK) {
		LOG_INF("Config MSG acknowledged %s", dev->name);
	} else if (rc) {
		LOG_ERR("Error %d config MSG for %s", rc, dev->name);
	}

	return rc;
}

static int neom8_init(const struct device *dev)
{
	struct neom8_data *data = dev->data;
	const struct neom8_config *cfg = dev->config;
	int rc = 0;

	k_sem_init(&data->lock, 0, 1);

	if (!device_is_ready(cfg->i2c_dev)) {
		LOG_ERR("I2C device %s is not ready", cfg->i2c_dev->name);
		return -ENODEV;
		goto out;
	}

	data->time.hour = 0;
	data->time.min = 0;
	data->time.sec = 0;
	data->longitude_min = 0;
	data->latitude_min = 0;
	data->longitude_deg = 0;
	data->latitude_deg = 0;
	data->altitude = 0;
	data->ind_latitude = 'A';
	data->ind_longitude = 'A';
	data->satellites = 0;
out:
	k_sem_give(&data->lock);

	return rc;
};

static const struct neom8_api neom8_api = {
	.fetch_data = neom8_fetch_data,
	.send_ubx = neom8_send_ubx,
	.cfg_nav5 = neom8_cfg_nav5,
	.cfg_gnss = neom8_cfg_gnss,
	.cfg_msg = neom8_cfg_msg,

	.get_altitude = neom8_get_altitude,
	.get_latitude = neom8_get_latitude,
	.get_ns = neom8_get_ns,
	.get_longitude = neom8_get_longitude,
	.get_ew = neom8_get_ew,
	.get_time = neom8_get_time,
	.get_satellites = neom8_get_satellites,
};

#if CONFIG_NEOM8_INIT_PRIORITY <= CONFIG_I2C_INIT_PRIORITY
#error CONFIG_NEOM8_INIT_PRIORITY must be greater than I2C_INIT_PRIORITY
#endif

#define NEOM8_INIT(n)                                                                              \
	static struct neom8_data neom8_data_##n;                                                   \
                                                                                                   \
	static const struct neom8_config neom8_config_##n = {                                      \
		.i2c_dev = DEVICE_DT_GET(DT_INST_BUS(n)), .i2c_addr = DT_INST_REG_ADDR(n)          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, neom8_init, NULL, &neom8_data_##n, &neom8_config_##n,             \
			      POST_KERNEL, CONFIG_NEOM8_INIT_PRIORITY, &neom8_api);

DT_INST_FOREACH_STATUS_OKAY(NEOM8_INIT);

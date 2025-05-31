/*
 * Copyright (c) 2025 Netfeasa
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "hl78xx.h"

#define ICCID_PREFIX_LEN 7
#define IMSI_PREFIX_LEN  6

LOG_MODULE_REGISTER(hl78xx_utility, CONFIG_MODEM_LOG_LEVEL);

/**
 * @brief  Convert string to long integer, but handle errors
 *
 * @param  s: string with representation of integer number
 * @param  err_value: on error return this value instead
 * @param  desc: name the string being converted
 * @param  func: function where this is called (typically __func__)
 *
 * @retval return integer conversion on success, or err_value on error
 */
int modem_atoi(const char *s, const int err_value, const char *desc, const char *func)
{
	int ret;
	char *endptr;

	ret = (int)strtol(s, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		LOG_ERR("bad %s '%s' in %s", s, desc, func);
		return err_value;
	}

	return ret;
}

bool hl78xx_is_registered(struct hl78xx_data *data)
{
	return (data->status.registration.network_state == HL78XX_REGISTRATION_REGISTERED_HOME) ||
	       (data->status.registration.network_state == HL78XX_REGISTRATION_REGISTERED_ROAMING);
}

#define HASH_MULTIPLIER 37
uint32_t hash32(const char *str, int len)
{
	uint32_t h = 0;

	for (int i = 0; i < len; ++i) {
		h = (h * HASH_MULTIPLIER) + str[i];
	}

	return h;
}

/**
 * Portable memmem() replacement for C99.
 */
const void *c99_memmem(const void *haystack, size_t haystacklen, const void *needle,
		       size_t needlelen)
{
	if (!haystack || !needle || needlelen == 0 || haystacklen < needlelen) {
		return NULL;
	}

	const uint8_t *h = haystack;

	for (size_t i = 0; i <= haystacklen - needlelen; i++) {
		if (memcmp(h + i, needle, needlelen) == 0) {
			return (h + i);
		}
	}

	return NULL;
}

#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID) || defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
int find_apn(const char *profile, const char *associated_number, char *apn_buff, uint8_t prefix_len)
{
	char buffer[512];
	char *saveptr;

	if (prefix_len > strlen(associated_number)) {
		return -1;
	}

	strncpy(buffer, profile, sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';

	char *token = strtok_r(buffer, ",", &saveptr);

	while (token != NULL) {
		char *equal_sign = strchr(token, '=');

		if (equal_sign != NULL) {
			*equal_sign = '\0';
			char *p_apn = token;
			char *associated_number_prefix = equal_sign + 1;

			/*  Trim leading whitespace */
			while (*p_apn == ' ') {
				p_apn++;
			}
			while (*associated_number_prefix == ' ') {
				associated_number_prefix++;
			}

			if (strncmp(associated_number, associated_number_prefix, prefix_len) == 0) {
				strncpy(apn_buff, p_apn, MDM_APN_MAX_LENGTH - 1);
				apn_buff[MDM_APN_MAX_LENGTH - 1] = '\0';
				return 0;
			}
		}

		token = strtok_r(NULL, ",", &saveptr);
	}

	/* No match found, clear apn_buff */
	apn_buff[0] = '\0';
	return -1; /* not found */
}
/* try to detect APN automatically, based on IMSI / ICCID */
int modem_detect_apn(struct hl78xx_data *data, const char *associated_number)
{
	int rc = -1;

	if (associated_number != NULL && strlen(associated_number) >= 5) {
/* extract MMC and MNC from IMSI */
#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
		/*
		 * First 5 digits (e.g. 31026) → often sufficient to identify carrier.
		 * However, in some regions (like the US), MNCs can be 3 digits (e.g. 310260).
		 */
		char mmcmnc[7] = {0}; /* IMSI */
		#define APN_PREFIX_LEN IMSI_PREFIX_LEN
#else
		/* These 7 digits are generally sufficient to identify the SIM provider.
		 */
		char mmcmnc[8] = {0}; /* ICCID */
		#define APN_PREFIX_LEN ICCID_PREFIX_LEN
#endif

		strncpy(mmcmnc, associated_number, sizeof(mmcmnc) - 1);
		mmcmnc[sizeof(mmcmnc) - 1] = '\0';
		/* try to find a matching IMSI/ICCID, and assign the APN */
		rc = find_apn(CONFIG_MODEM_HL78XX_APN_PROFILES, mmcmnc, data->identity.apn,
			      APN_PREFIX_LEN);
		if (rc < 0) {
			LOG_ERR("%d %s APN Parser error %d", __LINE__, __func__, rc);
		}
	}

	if (rc == 0) {
		LOG_INF("Assign APN: \"%s\"", data->identity.apn);
	} else {
		LOG_INF("No assigned APN: \"%d\"", rc);
	}

	return rc;
}
#endif

void set_band_bit(uint8_t *bitmap, uint16_t band_num)
{
	if (band_num < 1 || band_num > 256) {
		return; /* Out of range */
	}

	uint16_t bit_pos = band_num - 1;
	uint16_t byte_index = bit_pos / 8;
	uint8_t bit_index = bit_pos % 8;
	/* Big-endian format: band 1 in byte 31, band 256 in byte 0 */
	bitmap[byte_index] |= (1 << bit_index);
}

static uint8_t hl78xx_generate_band_bitmap(uint8_t *bitmap)
{
	memset(bitmap, 0, MDM_BAND_BITMAP_LEN_BYTES);
	/* Index is reversed: Band 1 is LSB of byte 31, Band 256 is MSB of byte 0 */

#if CONFIG_MODEM_HL78XX_BAND_1
	set_band_bit(bitmap, 1);
#endif
#if CONFIG_MODEM_HL78XX_BAND_2
	set_band_bit(bitmap, 2);
#endif
#if CONFIG_MODEM_HL78XX_BAND_3
	set_band_bit(bitmap, 3);
#endif
#if CONFIG_MODEM_HL78XX_BAND_4
	set_band_bit(bitmap, 4);
#endif
#if CONFIG_MODEM_HL78XX_BAND_5
	set_band_bit(bitmap, 5);
#endif
#if CONFIG_MODEM_HL78XX_BAND_8
	set_band_bit(bitmap, 8);
#endif
#if CONFIG_MODEM_HL78XX_BAND_9
	set_band_bit(bitmap, 9);

#endif
#if CONFIG_MODEM_HL78XX_BAND_10
	set_band_bit(bitmap, 10);

#endif
#if CONFIG_MODEM_HL78XX_BAND_12
	set_band_bit(bitmap, 12);

#endif
#if CONFIG_MODEM_HL78XX_BAND_13
	set_band_bit(bitmap, 13);

#endif
#if CONFIG_MODEM_HL78XX_BAND_17
	set_band_bit(bitmap, 17);

#endif
#if CONFIG_MODEM_HL78XX_BAND_18
	set_band_bit(bitmap, 18);

#endif
#if CONFIG_MODEM_HL78XX_BAND_19
	set_band_bit(bitmap, 19);

#endif
#if CONFIG_MODEM_HL78XX_BAND_20
	set_band_bit(bitmap, 20);

#endif
#if CONFIG_MODEM_HL78XX_BAND_23
	set_band_bit(bitmap, 23);

#endif
#if CONFIG_MODEM_HL78XX_BAND_25
	set_band_bit(bitmap, 25);

#endif
#if CONFIG_MODEM_HL78XX_BAND_26
	set_band_bit(bitmap, 26);

#endif
#if CONFIG_MODEM_HL78XX_BAND_27
	set_band_bit(bitmap, 27);

#endif
#if CONFIG_MODEM_HL78XX_BAND_28
	set_band_bit(bitmap, 28);

#endif
#if CONFIG_MODEM_HL78XX_BAND_31
	set_band_bit(bitmap, 31);

#endif
#if CONFIG_MODEM_HL78XX_BAND_66
	set_band_bit(bitmap, 66);

#endif
#if CONFIG_MODEM_HL78XX_BAND_72
	set_band_bit(bitmap, 72);

#endif
#if CONFIG_MODEM_HL78XX_BAND_73
	set_band_bit(bitmap, 73);

#endif
#if CONFIG_MODEM_HL78XX_BAND_85
	set_band_bit(bitmap, 85);

#endif
#if CONFIG_MODEM_HL78XX_BAND_87
	set_band_bit(bitmap, 87);

#endif
#if CONFIG_MODEM_HL78XX_BAND_88
	set_band_bit(bitmap, 88);

#endif
#if CONFIG_MODEM_HL78XX_BAND_106
	set_band_bit(bitmap, 106);

#endif
#if CONFIG_MODEM_HL78XX_BAND_107
	set_band_bit(bitmap, 107);

#endif
#if CONFIG_MODEM_HL78XX_BAND_255
	set_band_bit(bitmap, 255);

#endif
#if CONFIG_MODEM_HL78XX_BAND_256
	set_band_bit(bitmap, 256);

#endif
	/* Add additional bands similarly... */

	return 0;
}

void hl78xx_bitmap_to_hex_string_trimmed(const uint8_t *bitmap, char *hex_str, size_t hex_str_len)
{

	int started = 0;
	size_t offset = 0;

	for (int i = MDM_BAND_BITMAP_LEN_BYTES - 1; i >= 0; i--) {
		if (!started && bitmap[i] == 0) {
			continue; /*  Skip leading zero bytes */
		}

		started = 1;

		if (offset + 2 >= hex_str_len) {
			break;
		}

		offset += snprintk(&hex_str[offset], hex_str_len - offset, "%02X", bitmap[i]);
	}

	if (!started) {
		strcpy(hex_str, "0");
	}
}

int hl78xx_hex_string_to_bitmap(const char *hex_str, uint8_t *bitmap_out)
{
	if (strlen(hex_str) >= MDM_BAND_HEX_STR_LEN) {
		LOG_ERR("Invalid hex string length: %zu", strlen(hex_str));
		return -EINVAL;
	}

	for (int i = 0; i < MDM_BAND_BITMAP_LEN_BYTES; i++) {
		unsigned int byte_val;

		if (sscanf(&hex_str[i * 2], "%2x", &byte_val) != 1) {
			LOG_ERR("Failed to parse byte at position %d", i);
			return -EINVAL;
		}
		bitmap_out[i] = (uint8_t)byte_val;
	}

	return 0;
}

int hl78xx_get_band_default_config(char *hex_bndcfg, size_t SizeInBytes)
{
	uint8_t bitmap[MDM_BAND_BITMAP_LEN_BYTES];
	char hex_str[MDM_BAND_HEX_STR_LEN] = {0};

	if (SizeInBytes < MDM_BAND_HEX_STR_LEN || hex_bndcfg == NULL) {
		return -EINVAL;
	}
	hl78xx_generate_band_bitmap(bitmap);
	hl78xx_bitmap_to_hex_string_trimmed(bitmap, hex_str, sizeof(hex_str));

	LOG_INF("Default band config: %s", hex_str);

	strncpy(hex_bndcfg, hex_str, MDM_BAND_HEX_STR_LEN);
	return 0;
}

const char *hl78xx_trim_leading_zeros(const char *hex_str)
{
	while (*hex_str == '0' && *(hex_str + 1) != '\0') {
		hex_str++;
	}
	return hex_str;
}

/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * hl78xx_cfg.c
 *
 * Extracted helper implementations for RAT, band and APN configuration to
 * keep the main state-machine TU small and maintainable.
 */
#include "hl78xx.h"
#include "hl78xx_cfg.h"
#include "hl78xx_chat.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(hl78xx_dev);

#define ICCID_PREFIX_LEN            7
#define IMSI_PREFIX_LEN             6
#define MAX_BANDS                   32
#define MDM_APN_FULL_STRING_MAX_LEN 256

int hl78xx_rat_cfg(struct hl78xx_data *data, bool *modem_require_restart,
		   enum hl78xx_cell_rat_mode *rat_request)
{
	int ret = 0;

#if defined(CONFIG_MODEM_HL78XX_AUTORAT)
	/* Check autorat status/configs */
	if (IS_ENABLED(CONFIG_MODEM_HL78XX_AUTORAT_OVER_WRITE_PRL) ||
	    (data->kselacq_data.rat1 == 0 && data->kselacq_data.rat2 == 0 &&
	     data->kselacq_data.rat3 == 0)) {
		char cmd_kselq[] = "AT+KSELACQ=0," CONFIG_MODEM_HL78XX_AUTORAT_PRL_PROFILES;

		ret = modem_dynamic_cmd_send(data, NULL, cmd_kselq, strlen(cmd_kselq),
					     hl78xx_get_ok_match(), 1, false);
		if (ret < 0) {
			goto error;
		} else {
			*modem_require_restart = true;
		}
	}

	*rat_request = HL78XX_RAT_MODE_AUTO;
#else
	char const *cmd_ksrat_query = (const char *)KSRAT_QUERY;
	char const *cmd_kselq_disable = (const char *)DISABLE_RAT_AUTO;
	const char *cmd_set_rat = NULL;
	/* Check if auto rat are disabled */
	if (data->kselacq_data.rat1 != 0 && data->kselacq_data.rat2 != 0 &&
	    data->kselacq_data.rat3 != 0) {
		ret = modem_dynamic_cmd_send(data, NULL, cmd_kselq_disable,
					     strlen(cmd_kselq_disable), hl78xx_get_ok_match(), 1,
					     false);
		if (ret < 0) {
			goto error;
		}
	}
	/* Query current rat */
	ret = modem_dynamic_cmd_send(data, NULL, cmd_ksrat_query, strlen(cmd_ksrat_query),
				     hl78xx_get_ksrat_match(), 1, false);
	if (ret < 0) {
		goto error;
	}

#if !defined(CONFIG_MODEM_HL78XX_RAT_M1) && !defined(CONFIG_MODEM_HL78XX_RAT_NB1) &&               \
	!defined(CONFIG_MODEM_HL78XX_RAT_GSM) && !defined(CONFIG_MODEM_HL78XX_RAT_NBNTN)
#error "No rat has been selected."
#endif

	if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_M1)) {
		cmd_set_rat = (const char *)SET_RAT_M1_CMD_LEGACY;
		*rat_request = HL78XX_RAT_CAT_M1;
	} else if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_NB1)) {
		cmd_set_rat = (const char *)SET_RAT_NB1_CMD_LEGACY;
		*rat_request = HL78XX_RAT_NB1;
	}
#ifdef CONFIG_MODEM_HL78XX_12
	else if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_GSM)) {
		cmd_set_rat = (const char *)SET_RAT_GSM_CMD_LEGACY;
		*rat_request = HL78XX_RAT_GSM;
	}
#ifdef CONFIG_MODEM_HL78XX_12_FW_R6
	else if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_NBNTN)) {
		cmd_set_rat = (const char *)SET_RAT_NBNTN_CMD_LEGACY;
		*rat_request = HL78XX_RAT_NBNTN;
	}
#endif
#endif

	if (cmd_set_rat == NULL || *rat_request == HL78XX_RAT_MODE_NONE) {
		ret = -EINVAL;
		goto error;
	}

	if (*rat_request != data->status.registration.rat_mode) {
		ret = modem_dynamic_cmd_send(data, NULL, cmd_set_rat, strlen(cmd_set_rat),
					     hl78xx_get_ok_match(), 1, false);
		if (ret < 0) {
			goto error;
		} else {
			*modem_require_restart = true;
		}
	}
#endif

error:
	return ret;
}

int hl78xx_band_cfg(struct hl78xx_data *data, bool *modem_require_restart,
		    enum hl78xx_cell_rat_mode rat_config_request)
{
	int ret = 0;
	char bnd_bitmap[MDM_BAND_HEX_STR_LEN] = {0};
	const char *modem_trimmed;
	const char *expected_trimmed;

	if (rat_config_request == HL78XX_RAT_MODE_NONE) {
		return -EINVAL;
	}
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	for (int rat = HL78XX_RAT_CAT_M1; rat <= HL78XX_RAT_NB1; rat++) {
#else
	int rat = rat_config_request;

#endif
		ret = hl78xx_get_band_default_config_for_rat(rat, bnd_bitmap,
							     ARRAY_SIZE(bnd_bitmap));
		if (ret) {
			LOG_ERR("%d %s error get band default config %d", __LINE__, __func__, ret);
			goto error;
		}
		modem_trimmed = hl78xx_trim_leading_zeros(data->status.kbndcfg[rat].bnd_bitmap);
		expected_trimmed = hl78xx_trim_leading_zeros(bnd_bitmap);

		if (strcmp(modem_trimmed, expected_trimmed) != 0) {
			char cmd_bnd[80] = {0};

			snprintf(cmd_bnd, sizeof(cmd_bnd), "AT+KBNDCFG=%d,%s", rat, bnd_bitmap);
			ret = modem_dynamic_cmd_send(data, NULL, cmd_bnd, strlen(cmd_bnd),
						     hl78xx_get_ok_match(), 1, false);
			if (ret < 0) {
				goto error;
			} else {
				*modem_require_restart |= true;
			}
		} else {
			LOG_DBG("The band configs (%s) matched with exist configs (%s) for rat: "
				"[%d]",
				modem_trimmed, expected_trimmed, rat);
		}
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	}
#endif
error:
	return ret;
}

int hl78xx_set_apn_internal(struct hl78xx_data *data, const char *apn, uint16_t size)
{
	int ret = 0;
	char cmd_string[sizeof("AT+KCNXCFG=,\"\",\"\"") + sizeof(uint8_t) +
			MODEM_HL78XX_ADDRESS_FAMILY_FORMAT_LEN + MDM_APN_MAX_LENGTH] = {0};
	int cmd_max_len = sizeof(cmd_string) - 1;
	int apn_size = strlen(apn);

	if (apn == NULL || size >= MDM_APN_MAX_LENGTH) {
		return -EINVAL;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);
	if (strncmp(data->identity.apn, apn, apn_size) != 0) {
		safe_strncpy(data->identity.apn, apn, sizeof(data->identity.apn));
	}
	k_mutex_unlock(&data->api_lock);

	snprintk(cmd_string, cmd_max_len, "AT+CGDCONT=1,\"%s\",\"%s\"", MODEM_HL78XX_ADDRESS_FAMILY,
		 apn);

	ret = modem_dynamic_cmd_send(data, NULL, cmd_string, strlen(cmd_string),
				     hl78xx_get_ok_match(), 1, false);
	if (ret < 0) {
		goto error;
	}
	snprintk(cmd_string, cmd_max_len,
		 "AT+KCNXCFG=1,\"GPRS\",\"%s\",,,\"" MODEM_HL78XX_ADDRESS_FAMILY "\"", apn);
	ret = modem_dynamic_cmd_send(data, NULL, cmd_string, strlen(cmd_string),
				     hl78xx_get_ok_match(), 1, false);
	if (ret < 0) {
		goto error;
	}
	data->status.apn.state = APN_STATE_CONFIGURED;
	return 0;
error:
	LOG_ERR("Set APN to %s, result: %d", apn, ret);
	return ret;
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
	uint16_t bit_pos;
	uint16_t byte_index;
	uint8_t bit_index;

	if (band_num < 1 || band_num > 256) {
		return; /* Out of range */
	}
	/* Calculate byte and bit positions */
	bit_pos = band_num - 1;
	byte_index = bit_pos / 8;
	bit_index = bit_pos % 8;
	/* Big-endian format: band 1 in byte 31, band 256 in byte 0 */
	bitmap[byte_index] |= (1 << bit_index);
}

#ifdef CONFIG_MODEM_HL78XX_CONFIGURE_BANDS
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
#endif /* CONFIG_MODEM_HL78XX_CONFIGURE_BANDS */

#if defined(CONFIG_MODEM_HL78XX_AUTORAT)
/**
 * @brief Parse a comma-separated list of bands from a string.
 *
 * @param band_str The input string containing band numbers.
 * @param bands Output array to store parsed band numbers.
 * @param max_bands Maximum number of bands that can be stored in the output array.
 *
 * @return Number of bands parsed, or negative error code on failure.
 */
static int parse_band_list(const char *band_str, int *bands, size_t max_bands)
{
	char buf[128] = {0};
	char *token;
	char *rest;
	int count = 0;
	int band = 0;

	if (!band_str || !bands || max_bands == 0) {
		return -EINVAL;
	}
	strncpy(buf, band_str, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	rest = buf;
	while ((token = strtok_r(rest, ",", &rest))) {
		band = ATOI(token, -1, "band");
		if (band <= 0) {
			printk("Invalid band number: %s\n", token);
			continue;
		}
		if (count >= max_bands) {
			printk("Too many bands, max is %d\n", (int)max_bands);
			break;
		}
		bands[count++] = band;
	}
	return count;
}
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */

int hl78xx_generate_bitmap_from_config(enum hl78xx_cell_rat_mode rat, uint8_t *bitmap_out)
{
	if (!bitmap_out) {
		return -EINVAL;
	}
	memset(bitmap_out, 0, MDM_BAND_BITMAP_LEN_BYTES);
#if defined(CONFIG_MODEM_HL78XX_AUTORAT)
	/* Auto-RAT: read bands from string configs */
	const char *band_str = NULL;

	switch (rat) {
	case HL78XX_RAT_CAT_M1:
#ifdef CONFIG_MODEM_HL78XX_AUTORAT_M1_BAND_CFG
		band_str = CONFIG_MODEM_HL78XX_AUTORAT_M1_BAND_CFG;
#endif
		break;

	case HL78XX_RAT_NB1:
#ifdef CONFIG_MODEM_HL78XX_AUTORAT_NB_BAND_CFG
		band_str = CONFIG_MODEM_HL78XX_AUTORAT_NB_BAND_CFG;
#endif
		break;

	default:
		return -EINVAL;
	}
	if (band_str) {
		int bands[MAX_BANDS];
		int count = parse_band_list(band_str, bands, MAX_BANDS);

		if (count < 0) {
			return -EINVAL;
		}
		for (int i = 0; i < count; i++) {
			set_band_bit(bitmap_out, bands[i]);
		}
		return 0;
	}
#else
	/* Else: use standalone config */
	return hl78xx_generate_band_bitmap(bitmap_out);
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */
	return -EINVAL;
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

int hl78xx_get_band_default_config_for_rat(enum hl78xx_cell_rat_mode rat, char *hex_bndcfg,
					   size_t size_in_bytes)
{
	uint8_t bitmap[MDM_BAND_BITMAP_LEN_BYTES] = {0};
	char hex_str[MDM_BAND_HEX_STR_LEN] = {0};

	if (size_in_bytes < MDM_BAND_HEX_STR_LEN || hex_bndcfg == NULL) {
		return -EINVAL;
	}
	if (hl78xx_generate_bitmap_from_config(rat, bitmap) != 0) {
		return -EINVAL;
	}
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

static void strip_quotes(char *str)
{
	size_t len = strlen(str);

	if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
		/*  Shift string left by 1 and null-terminate earlier */
		memmove(str, str + 1, len - 2);
		str[len - 2] = '\0';
	}
}

void hl78xx_extract_essential_part_apn(const char *full_apn, char *essential_apn, size_t max_len)
{
	char apn_buf[MDM_APN_FULL_STRING_MAX_LEN] = {0};
	size_t len;
	const char *mnc_ptr;

	if (full_apn == NULL || essential_apn == NULL || max_len == 0) {
		return;
	}
	strncpy(apn_buf, full_apn, sizeof(apn_buf) - 1);
	apn_buf[sizeof(apn_buf) - 1] = '\0';
	/*  Remove surrounding quotes if any */
	strip_quotes(apn_buf);
	mnc_ptr = strstr(apn_buf, ".mnc");
	if (mnc_ptr != NULL) {
		len = mnc_ptr - apn_buf;
		if (len >= max_len) {
			len = max_len - 1;
		}
		strncpy(essential_apn, apn_buf, len);
		essential_apn[len] = '\0';
	} else {
		/* No ".mnc" found, copy entire string */
		strncpy(essential_apn, apn_buf, max_len - 1);
		essential_apn[max_len - 1] = '\0';
	}
}

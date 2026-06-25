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

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* Required for strtok_r() */
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hl78xx.h"
#include "hl78xx_cfg.h"
#include "hl78xx_chat.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/timeutil.h>

LOG_MODULE_DECLARE(hl78xx_dev, CONFIG_MODEM_LOG_LEVEL);

#define ICCID_PREFIX_LEN            7
#define IMSI_PREFIX_LEN             6
#define MAX_BANDS                   32
#define HL78XX_CTZEU_FIELD_MAX_LEN  8U
#define MDM_APN_FULL_STRING_MAX_LEN 256

/* Delay after AT+IPR command for new rate to take effect */
#define BAUDRATE_SWITCH_DELAY_MS 2500

static void set_band_bit(uint8_t *bitmap, uint16_t band_num);

int hl78xx_enable_lte_coverage_urc(struct hl78xx_data *data, bool *modem_require_restart,
				   uint16_t timeout_s)
{
	int ret = 0;
	char cmd[HL78XX_AT_CMD_MAX_LEN] = {0};

	if (timeout_s > 1200) {
		return -EINVAL;
	}

	if (data->status.kcellmeas.timeout == timeout_s) {
		/* No update needed */
		*modem_require_restart |= false;
		return 0;
	}
	if (!IS_ENABLED(CONFIG_HL78XX_GNSS)) {
		/* GNSS disabled: configure +KCELLMEAS with the requested timeout. */
		snprintk(cmd, sizeof(cmd), "AT+KCELLMEAS=1,%d", timeout_s);
	} else {
		snprintk(cmd, sizeof(cmd), "AT+KCELLMEAS=1,0");
	}

	ret = modem_dynamic_cmd_send(data, NULL, cmd, strlen(cmd), hl78xx_get_ok_match(),
				     hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);
	if (ret < 0) {
		LOG_ERR("Failed to enable LTE coverage URC: %d", ret);
	} else {
		data->status.kcellmeas.timeout = timeout_s;
	}
	*modem_require_restart = true;

	return ret;
}

#ifdef CONFIG_MODEM_HL78XX_AUTORAT
static bool hl78xx_prl_rat_is_valid(enum hl78xx_kselacq_rat rat)
{
#ifdef CONFIG_MODEM_HL78XX_12
	return (rat >= HL78XX_KSELACQ_RAT_CLEAR) && (rat <= HL78XX_KSELACQ_RAT_GSM);
#else
	return (rat >= HL78XX_KSELACQ_RAT_CLEAR) && (rat <= HL78XX_KSELACQ_RAT_NB1);
#endif
}

static bool hl78xx_prl_is_clear_request(const struct kselacq_syntax kselacq_rats)
{
	return (kselacq_rats.rat1 == HL78XX_KSELACQ_RAT_CLEAR) &&
	       (kselacq_rats.rat2 == HL78XX_KSELACQ_RAT_CLEAR) &&
	       (kselacq_rats.rat3 == HL78XX_KSELACQ_RAT_CLEAR);
}

static bool hl78xx_prl_write_is_valid(const struct kselacq_syntax kselacq_rats)
{
	if (!hl78xx_prl_rat_is_valid(kselacq_rats.rat1) ||
	    !hl78xx_prl_rat_is_valid(kselacq_rats.rat2) ||
	    !hl78xx_prl_rat_is_valid(kselacq_rats.rat3)) {
		return false;
	}

	if (hl78xx_prl_is_clear_request(kselacq_rats)) {
		return true;
	}

	return (kselacq_rats.rat1 != HL78XX_KSELACQ_RAT_CLEAR) &&
	       (kselacq_rats.rat2 != HL78XX_KSELACQ_RAT_CLEAR) &&
	       (kselacq_rats.rat3 != HL78XX_KSELACQ_RAT_CLEAR);
}

int hl78xx_set_prl_internal(struct hl78xx_data *data, const struct kselacq_syntax kselacq_rats)
{
	int ret;
	char cmd[sizeof("AT+KSELACQ=0,255,255,255")] = {0};

	if (data == NULL) {
		return -EINVAL;
	}

	if (!hl78xx_prl_write_is_valid(kselacq_rats)) {
		return -EINVAL;
	}

	if (hl78xx_prl_is_clear_request(kselacq_rats)) {
		snprintk(cmd, sizeof(cmd), "AT+KSELACQ=0,0");
	} else {
		snprintk(cmd, sizeof(cmd), "AT+KSELACQ=0,%d,%d,%d", kselacq_rats.rat1,
			 kselacq_rats.rat2, kselacq_rats.rat3);
	}

	ret = modem_dynamic_cmd_send(data, NULL, cmd, strlen(cmd), hl78xx_get_ok_match(),
				     hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);
	if (ret < 0) {
		LOG_ERR("Failed to update PRL: %d", ret);
		return ret;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);
	data->kselacq_data.mode = false;
	data->kselacq_data.rat1 = kselacq_rats.rat1;
	data->kselacq_data.rat2 = kselacq_rats.rat2;
	data->kselacq_data.rat3 = kselacq_rats.rat3;
	k_mutex_unlock(&data->api_lock);

	return 0;
}
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */

int hl78xx_rat_cfg(struct hl78xx_data *data, bool *modem_require_restart,
		   enum hl78xx_cell_rat_mode *rat_request)
{
	int ret = 0;
	const struct hl78xx_config *config = data->devices.hl78xx->config;

#if defined(CONFIG_MODEM_HL78XX_AUTORAT)
	/* Check autorat status/configs */
	if (IS_ENABLED(CONFIG_MODEM_HL78XX_AUTORAT_OVER_WRITE_PRL) ||
	    (data->kselacq_data.rat1 == HL78XX_KSELACQ_RAT_CLEAR &&
	     data->kselacq_data.rat2 == HL78XX_KSELACQ_RAT_CLEAR &&
	     data->kselacq_data.rat3 == HL78XX_KSELACQ_RAT_CLEAR)) {
		char cmd_kselq[] = "AT+KSELACQ=0," CONFIG_MODEM_HL78XX_AUTORAT_PRL_PROFILES;

		ret = modem_dynamic_cmd_send(data, NULL, cmd_kselq, strlen(cmd_kselq),
					     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
					     MDM_CMD_TIMEOUT, false);
		if (ret < 0) {
			goto error;
		} else {
			*modem_require_restart = true;
		}
	}

	*rat_request = HL78XX_RAT_MODE_AUTO;

	if (config->variant->cfg_apply_rat_post_select) {
		ret = config->variant->cfg_apply_rat_post_select(
			data, data->status.registration.rat_mode);
		if (ret < 0) {
			goto error;
		}
	}
#else
	char const *cmd_ksrat_query = (const char *)KSRAT_QUERY;
	char const *cmd_kselq_disable = (const char *)DISABLE_RAT_AUTO;
	const char *cmd_set_rat = NULL;
	/* Check if auto rat are disabled */
	if (data->kselacq_data.rat1 != HL78XX_KSELACQ_RAT_CLEAR &&
	    data->kselacq_data.rat2 != HL78XX_KSELACQ_RAT_CLEAR &&
	    data->kselacq_data.rat3 != HL78XX_KSELACQ_RAT_CLEAR) {
		ret = modem_dynamic_cmd_send(data, NULL, cmd_kselq_disable,
					     strlen(cmd_kselq_disable), hl78xx_get_ok_match(),
					     hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);
		if (ret < 0) {
			goto error;
		}
	}
	/* Query current rat */
	ret = modem_dynamic_cmd_send(data, NULL, cmd_ksrat_query, strlen(cmd_ksrat_query),
				     hl78xx_get_ksrat_match(), 1, MDM_CMD_TIMEOUT, false);
	if (ret < 0) {
		goto error;
	}

#if !defined(CONFIG_MODEM_HL78XX_RAT_M1) && !defined(CONFIG_MODEM_HL78XX_RAT_NB1) &&               \
	!defined(CONFIG_MODEM_HL78XX_RAT_GSM) && !defined(CONFIG_MODEM_HL78XX_RAT_NBNTN)
#error "No rat has been selected."
#endif
	if (config->variant->cfg_select_rat) {
		ret = config->variant->cfg_select_rat(data, &cmd_set_rat, rat_request);
		if (ret < 0) {
			goto error;
		}
	}

	if (cmd_set_rat == NULL || *rat_request == HL78XX_RAT_MODE_NONE) {
		ret = -EINVAL;
		goto error;
	}

	if (*rat_request != data->status.registration.rat_mode) {
		ret = modem_dynamic_cmd_send(data, NULL, cmd_set_rat, strlen(cmd_set_rat),
					     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
					     MDM_CMD_TIMEOUT, false);
		if (ret < 0) {
			goto error;
		} else {
			*modem_require_restart = true;
		}
	}

	if (config->variant->cfg_apply_rat_post_select) {
		ret = config->variant->cfg_apply_rat_post_select(data, *rat_request);
		if (ret < 0) {
			goto error;
		}
	}
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */

error:
	return ret;
}

static bool hl78xx_get_runtime_band_override(struct hl78xx_data *data,
					     enum hl78xx_cell_rat_mode rat, uint16_t *band)
{
	hl78xx_runtime_band_provider_t provider;
	void *user_data;
	uint16_t override_band = 0U;
	bool has_override;

	if ((data == NULL) || (band == NULL)) {
		return false;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);
	provider = data->runtime_band.provider;
	user_data = data->runtime_band.provider_user_data;
	k_mutex_unlock(&data->api_lock);

	if (provider == NULL) {
		return false;
	}

	has_override = provider(data->devices.hl78xx, rat, &override_band, user_data);
	if (!has_override) {
		return false;
	}

	if (override_band == 0U) {
		LOG_WRN("Runtime band provider returned empty band for RAT %d", rat);
		return false;
	}

	*band = override_band;
	return true;
}

static int hl78xx_band_bitmap_from_single_band(uint16_t band_number, char *hex_bndcfg,
					       size_t size_in_bytes)
{
	uint8_t bitmap[MDM_BAND_BITMAP_LEN_BYTES] = {0};

	if ((hex_bndcfg == NULL) || (size_in_bytes < MDM_BAND_HEX_STR_LEN)) {
		return -EINVAL;
	}

	if ((band_number == 0U) || (band_number > 256U)) {
		return -EINVAL;
	}

	set_band_bit(bitmap, band_number);
	hl78xx_bitmap_to_hex_string_trimmed(bitmap, hex_bndcfg, size_in_bytes);

	return 0;
}

static int hl78xx_get_expected_band_config_for_rat(struct hl78xx_data *data,
						   enum hl78xx_cell_rat_mode rat, char *hex_bndcfg,
						   size_t size_in_bytes)
{
	uint16_t runtime_band = 0U;
	int ret;

	if ((data == NULL) || (hex_bndcfg == NULL)) {
		return -EINVAL;
	}

	if (hl78xx_get_runtime_band_override(data, rat, &runtime_band)) {
		ret = hl78xx_band_bitmap_from_single_band(runtime_band, hex_bndcfg, size_in_bytes);
		if (ret == 0) {
			LOG_DBG("Using runtime band %u for RAT %d", runtime_band, rat);
		}
		return ret;
	}

	return hl78xx_get_band_default_config_for_rat(rat, hex_bndcfg, size_in_bytes);
}

int hl78xx_band_cfg(struct hl78xx_data *data, bool *modem_require_restart,
		    enum hl78xx_cell_rat_mode rat_config_request)
{
	int ret = 0;
	char bnd_bitmap[MDM_BAND_HEX_STR_LEN] = {0};
	const char *modem_trimmed;
	const char *expected_trimmed;
	const struct hl78xx_config *config = data->devices.hl78xx->config;

	if (rat_config_request == HL78XX_RAT_MODE_NONE) {
		return -EINVAL;
	}
	if (config->variant->cfg_skip_band_for_rat &&
	    config->variant->cfg_skip_band_for_rat(data, rat_config_request)) {
		return 0;
	}
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	for (int rat = HL78XX_RAT_CAT_M1; rat <= HL78XX_RAT_NB1; rat++) {
#else
	int rat = rat_config_request;
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */
		ret = hl78xx_get_expected_band_config_for_rat(data, rat, bnd_bitmap,
							      ARRAY_SIZE(bnd_bitmap));
		if (ret) {
			LOG_ERR("%d %s error get expected band config %d", __LINE__, __func__, ret);
			goto error;
		}
		modem_trimmed =
			hl78xx_trim_leading_zeros(data->status.band.kbndcfg[rat].bnd_bitmap);
		expected_trimmed = hl78xx_trim_leading_zeros(bnd_bitmap);

		if (strcmp(modem_trimmed, expected_trimmed) != 0) {
			char cmd_bnd[80] = {0};

			snprintf(cmd_bnd, sizeof(cmd_bnd), "AT+KBNDCFG=%d,%s", rat, bnd_bitmap);
			ret = modem_dynamic_cmd_send(
				data, NULL, cmd_bnd, strlen(cmd_bnd), hl78xx_get_ok_match(),
				hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);
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
#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN
int hl78xx_rat_ntn_cfg(struct hl78xx_data *data, bool *modem_require_restart,
		       enum hl78xx_cell_rat_mode rat_config_request)
{
	int ret = 0;
	char cmd_kntncfg[HL78XX_AT_CMD_MAX_LEN] = {0};
	char *pos_provider = NULL;
	bool is_dynamic = false;

	if (rat_config_request == HL78XX_RAT_MODE_NONE) {
		return -EINVAL;
	}

#ifdef CONFIG_NTN_POSITION_SOURCE_IGNSS
	pos_provider = "IGNSS";
#else
	pos_provider = "MANUAL";
#endif
#ifdef CONFIG_NTN_MOBILITY_TYPE_STATIC
	is_dynamic = false;
#else
	is_dynamic = true;
#endif
	/* Enable GNSS based positioning for NB-NTN */
	snprintf(cmd_kntncfg, sizeof(cmd_kntncfg), "AT+KNTNCFG=\"POS\",\"%s\",%hhu", pos_provider,
		 is_dynamic);

	ret = modem_dynamic_cmd_send(data, NULL, cmd_kntncfg, strlen(cmd_kntncfg),
				     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				     MDM_CMD_TIMEOUT, false);
	if (ret < 0) {
		goto error;
	}

error:
	return ret;
}
#endif

int hl78xx_set_apn_internal(struct hl78xx_data *data, const char *apn, uint16_t size)
{
	int ret = 0;
	char cmd_string[sizeof("AT+KCNXCFG=,\"\",\"\"") + sizeof(uint8_t) +
			MODEM_HL78XX_ADDRESS_FAMILY_FORMAT_LEN + MDM_APN_MAX_LENGTH] = {0};
	int cmd_max_len = sizeof(cmd_string) - 1;
	int apn_size;

	if (apn == NULL || size >= MDM_APN_MAX_LENGTH) {
		return -EINVAL;
	}

	apn_size = strlen(apn);

	k_mutex_lock(&data->api_lock, K_FOREVER);
	if (strncmp(data->identity.apn, apn, apn_size) != 0) {
		safe_strncpy(data->identity.apn, apn, sizeof(data->identity.apn));
	}
	k_mutex_unlock(&data->api_lock);

	snprintk(cmd_string, cmd_max_len, "AT+CGDCONT=1,\"%s\",\"%s\"", MODEM_HL78XX_ADDRESS_FAMILY,
		 apn);

	ret = modem_dynamic_cmd_send(data, NULL, cmd_string, strlen(cmd_string),
				     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				     MDM_CMD_TIMEOUT, false);
	if (ret < 0) {
		goto error;
	}
#if defined(CONFIG_MODEM_HL78XX_RAT_NBNTN)
	snprintk(cmd_string, cmd_max_len, "AT+KCNXCFG=1,\"GPRS\",\"%s\",,,", apn);
#else
	snprintk(cmd_string, cmd_max_len,
		 "AT+KCNXCFG=1,\"GPRS\",\"%s\",,,\"" MODEM_HL78XX_ADDRESS_FAMILY "\"", apn);
#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */
	ret = modem_dynamic_cmd_send(data, NULL, cmd_string, strlen(cmd_string),
				     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				     MDM_CMD_TIMEOUT, false);
	if (ret < 0) {
		goto error;
	}
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
	ret = modem_dynamic_cmd_send(data, NULL, "AT+WDSS=2,1", strlen("AT+WDSS=2,1"),
				     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				     MDM_CMD_TIMEOUT, false);
	if (ret < 0) {
		goto error;
	}
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
	data->status.apn.state = APN_STATE_CONFIGURED;
	return 0;
error:
	LOG_ERR("Set APN to %s, result: %d", apn, ret);
	return ret;
}

int hl78xx_gsm_pdp_activate(struct hl78xx_data *data)
{
	int ret = 0;
	/* Activate the PDP context, Today only one pdp context is supported */
	const char *cmd_activate_pdp = "AT+CGACT=1,1";
	/* Check if the current RAT is GSM and if the PDP context is not already active */
	if (data->status.registration.rat_mode == HL78XX_RAT_CAT_M1 ||
	    data->status.registration.rat_mode == HL78XX_RAT_NB1 ||
	    data->status.gprs[0].is_active) {
		return 0;
	}

	ret = modem_dynamic_cmd_send(data, NULL, cmd_activate_pdp, strlen(cmd_activate_pdp),
				     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				     MDM_CMD_TIMEOUT, false);
	if (ret < 0) {
		LOG_ERR("GSM PDP activation failed: %d", ret);
		return ret;
	}
	return 0;
}

#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID) || defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
/* Find APN from profile string based on associated number prefix */
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
#endif /* CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID || CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI */
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
int hl78xx_enable_pmc(struct hl78xx_data *data)
{
	const char *turn_on_pmc_cmd = "AT+KSLEEP=1,2,0";

	LOG_DBG("%d Enabling Power Management Config", __LINE__);
	return modem_dynamic_cmd_send(data, NULL, turn_on_pmc_cmd, strlen(turn_on_pmc_cmd),
				      hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				      MDM_CMD_TIMEOUT, false);
}

int hl78xx_psm_settings(struct hl78xx_data *data)
{
	if (data->status.registration.rat_mode != HL78XX_RAT_NB1 &&
	    data->status.registration.rat_mode != HL78XX_RAT_CAT_M1) {
		LOG_DBG("PSM is not supported for RAT mode: %d",
			data->status.registration.rat_mode);
		return 0;
	}
#ifdef CONFIG_MODEM_HL78XX_PSM
	if (data->status.lpm.cpsms.mode == false) {
		const char *turn_on_psm_cmd = "AT+CPSMS=1,,,\"" CONFIG_MODEM_HL78XX_PSM_PERIODIC_TAU
					      "\",\"" CONFIG_MODEM_HL78XX_PSM_ACTIVE_TIME "\"";

		return modem_dynamic_cmd_send(data, NULL, turn_on_psm_cmd, strlen(turn_on_psm_cmd),
					      hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
					      MDM_CMD_TIMEOUT, false);
	}
#else
	if (data->status.lpm.cpsms.mode == true) {
		const char *turn_off_psm_cmd = "AT+CPSMS=0";

		return modem_dynamic_cmd_send(data, NULL, turn_off_psm_cmd,
					      strlen(turn_off_psm_cmd), hl78xx_get_ok_match(),
					      hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);
	}
#endif
	LOG_DBG("PSM is already configured for RAT mode: %d", data->status.registration.rat_mode);
	return 0; /* PSM already disabled */
}

int hl78xx_edrx_settings(struct hl78xx_data *data)
{
	if (data->status.registration.rat_mode != HL78XX_RAT_NB1 &&
	    data->status.registration.rat_mode != HL78XX_RAT_CAT_M1) {
		LOG_DBG("eDRX is not supported for RAT mode: %d",
			data->status.registration.rat_mode);
		return 0;
	}
#ifdef CONFIG_MODEM_HL78XX_EDRX
	if (data->status.lpm.kedrxcfg[data->status.registration.rat_mode].mode ==
		    HL78XX_KEDRX_MODE_DISABLE ||
	    data->status.lpm.kedrxcfg[data->status.registration.rat_mode].mode ==
		    HL78XX_KEDRX_MODE_DISABLE_AND_ERASE_CFG) {
		char turn_on_edrx_cmd[sizeof("AT+KEDRXCFG=1,X,XXXX,XXXX")] = {0};
		uint8_t ack_type = 4;

		ack_type = (data->status.registration.rat_mode == HL78XX_RAT_NB1) ? 5 : ack_type;

		snprintf(turn_on_edrx_cmd, sizeof(turn_on_edrx_cmd), "AT+KEDRXCFG=1,%hhu,%s,%d",
			 ack_type, CONFIG_MODEM_HL78XX_EDRX_VALUE, CONFIG_MODEM_HL78XX_PTW_VALUE);

		return modem_dynamic_cmd_send(data, NULL, turn_on_edrx_cmd,
					      strlen(turn_on_edrx_cmd), hl78xx_get_ok_match(),
					      hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);
	}
#else
	if (data->status.lpm.kedrxcfg[data->status.registration.rat_mode].mode ==
		    HL78XX_KEDRX_MODE_ENABLE ||
	    data->status.lpm.kedrxcfg[data->status.registration.rat_mode].mode ==
		    HL78XX_KEDRX_MODE_ENABLE_W_URC) {
		char *turn_off_edrx_cmd = "AT+KEDRXCFG=0";

		return modem_dynamic_cmd_send(data, NULL, turn_off_edrx_cmd,
					      strlen(turn_off_edrx_cmd), hl78xx_get_ok_match(),
					      hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);
	}
#endif

	LOG_DBG("eDRX is already configured for RAT mode: %d", data->status.registration.rat_mode);
	return 0;
}

void hl78xx_dynamic_cmd_feed_lpm_timers(struct hl78xx_data *data, uint16_t response_timeout)
{
#if defined(CONFIG_MODEM_HL78XX_USE_DELAY_BASED_POWER_DOWN)
	hl78xx_power_down_feed_timer(data, response_timeout);
#endif /* CONFIG_MODEM_HL78XX_USE_DELAY_BASED_POWER_DOWN */

#if defined(CONFIG_MODEM_HL78XX_EDRX)
	if (hl78xx_edrx_idle_is_ignoring_feeding(data) == false) {
		hl78xx_edrx_idle_feed_timer(data, response_timeout);
	} else {
		hl78xx_edrx_idle_allow_feeding(data);
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */
}

#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN

static void hl78xx_power_down_shutdown_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct hl78xx_data *data =
		CONTAINER_OF(dwork, struct hl78xx_data, work.power_down_shutdown_work);

	LOG_DBG("%d: power down shutdown: entering INIT_POWER_OFF", __LINE__);
	data->status.lpm.power_down.previous = data->status.lpm.power_down.current;
	data->status.lpm.power_down.current = POWER_DOWN_EVENT_ENTER;

	hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
	data->status.lpm.power_down.is_power_down_requested = true;
}

static void hl78xx_power_down_work_handler(struct k_work *work_item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work_item);
	struct hl78xx_data *data =
		CONTAINER_OF(dwork, struct hl78xx_data, work.hl78xx_pwr_dwn_work);

	LOG_DBG("%d: Power down work handler called", __LINE__);

	notif_carrier_off(data->devices.hl78xx);

	struct hl78xx_evt pd_evt = {.type = HL78XX_POWER_DOWN_UPDATE};

	hl78xx_power_down_ignore_feeding(data);
	pd_evt.content.power_down_event = POWER_DOWN_EVENT_ENTER;
	event_dispatcher_dispatch(&pd_evt);

	/* Give the application time to perform graceful teardown (e.g. cloud
	 * disconnect).
	 */
	k_work_reschedule(&data->work.power_down_shutdown_work,
			  K_SECONDS(CONFIG_MODEM_HL78XX_POWER_DOWN_CONFIRM_TIMEOUT_S));
}

int hl78xx_power_down_trigger(const struct device *dev, k_timeout_t delay)
{
	struct hl78xx_data *data;

	if ((dev == NULL) || (dev->data == NULL)) {
		return -EINVAL;
	}

	data = dev->data;

	if ((data->status.lpm.power_down.current == POWER_DOWN_EVENT_ENTER) ||
	    data->status.lpm.power_down.is_power_down_requested) {
		LOG_DBG("Ignoring redundant power-down trigger while shutdown is already active %d "
			"%d",
			data->status.lpm.power_down.current,
			data->status.lpm.power_down.is_power_down_requested);
		return 0;
	}

	LOG_INF("Power down trigger requested by application");
	hl78xx_power_down_ignore_feeding(data);
	(void)k_work_reschedule(&data->work.hl78xx_pwr_dwn_work, delay);

	return 0;
}

int hl78xx_power_down_cancel(const struct device *dev)
{
	struct hl78xx_data *data;
	int ret;

	if ((dev == NULL) || (dev->data == NULL)) {
		return -EINVAL;
	}

	data = dev->data;

	if (!hl78xx_is_power_down_scheduled(data)) {
		return -ENOENT;
	}

	ret = hl78xx_cancel_power_down(data);
	if (ret < 0) {
		return ret;
	}

	hl78xx_power_down_allow_feeding(data);
	LOG_INF("Pending power down timer canceled by application");

	return 0;
}

static int hl78xx_power_down_apply_response(struct hl78xx_data *data,
					    enum hl78xx_power_down_response response,
					    uint32_t timeout_s)
{
	if (data == NULL) {
		return -EINVAL;
	}

	if (!k_work_delayable_is_pending(&data->work.power_down_shutdown_work)) {
		return -ENOENT;
	}

	switch (response) {
	case HL78XX_POWER_DOWN_RESPONSE_IMMEDIATE:
		LOG_INF("Power down confirmed by application, proceeding immediately");
		hl78xx_power_down_allow_feeding(data);
		(void)k_work_reschedule(&data->work.power_down_shutdown_work, K_NO_WAIT);
		return 0;

	case HL78XX_POWER_DOWN_RESPONSE_RESCHEDULE:
		if (timeout_s == 0U) {
			return -EINVAL;
		}

		LOG_INF("Power down rescheduled by application: %u s", timeout_s);
		hl78xx_power_down_ignore_feeding(data);
		(void)k_work_reschedule(&data->work.power_down_shutdown_work, K_SECONDS(timeout_s));
		return 0;

	default:
		return -EINVAL;
	}
}

int hl78xx_power_down_respond(const struct device *dev, enum hl78xx_power_down_response response,
			      uint32_t timeout_s)
{
	if ((dev == NULL) || (dev->data == NULL)) {
		return -EINVAL;
	}

	return hl78xx_power_down_apply_response(dev->data, response, timeout_s);
}

/**
 * @brief Confirm that the application has finished its cleanup and the modem
 *        may proceed with the physical shutdown sequence.
 *
 * Call this from the application side (e.g. when CLOUD_EVT_DISCONNECTED is
 * received) to allow the driver to enter MODEM_HL78XX_STATE_INIT_POWER_OFF
 * immediately instead of waiting for the full confirmation timeout.
 *
 * If no power-down shutdown is pending, the call is a no-op.
 *
 * @param dev Pointer to the HL78xx modem device.
 */
void hl78xx_power_down_confirm(const struct device *dev)
{
	(void)hl78xx_power_down_respond(dev, HL78XX_POWER_DOWN_RESPONSE_IMMEDIATE, 0U);
}

int hl78xx_init_power_down(struct hl78xx_data *data)
{
	LOG_DBG("%d Initializing Power Down", __LINE__);
	k_work_init_delayable(&data->work.hl78xx_pwr_dwn_work, hl78xx_power_down_work_handler);
	k_work_init_delayable(&data->work.power_down_shutdown_work, hl78xx_power_down_shutdown_fn);
	data->status.lpm.power_down.current = POWER_DOWN_EVENT_NONE;
	data->status.lpm.power_down.previous = POWER_DOWN_EVENT_NONE;
	data->status.lpm.power_down.is_power_down_requested = false;
	return 0;
}

void hl78xx_power_down_ignore_feeding(struct hl78xx_data *data)
{
	LOG_DBG("Ignoring Power Down timer feeding for the next command");
	data->status.lpm.ignore_power_down_feeding = true;
}

void hl78xx_power_down_allow_feeding(struct hl78xx_data *data)
{
	LOG_DBG("Allowing Power Down timer feeding for the next command");
	data->status.lpm.ignore_power_down_feeding = false;
}

bool hl78xx_power_down_is_ignoring_feeding(struct hl78xx_data *data)
{
	return data->status.lpm.ignore_power_down_feeding;
}

int hl78xx_cancel_power_down(struct hl78xx_data *data)
{
	LOG_DBG("%d Canceling Power Down", __LINE__);
	return k_work_cancel_delayable(&data->work.hl78xx_pwr_dwn_work);
}

int hl78xx_is_power_down_scheduled(struct hl78xx_data *data)
{
	return k_work_delayable_is_pending(&data->work.hl78xx_pwr_dwn_work);
}

int hl78xx_power_down_feed_timer(struct hl78xx_data *data, uint32_t cmd_timeout_s)
{
	const struct hl78xx_config *config = data->devices.hl78xx->config;
#ifdef CONFIG_MODEM_HL78XX_USE_DELAY_BASED_POWER_DOWN
	uint32_t total_timeout_s = CONFIG_MODEM_HL78XX_POWER_DOWN_DELAY + cmd_timeout_s;
#endif /* CONFIG_MODEM_HL78XX_USE_DELAY_BASED_POWER_DOWN */
	if (hl78xx_power_down_is_ignoring_feeding(data)) {
		LOG_DBG("Not feeding timer because feeding is currently ignored");
		return 0;
	}

	if (hl78xx_is_registered(data) == false) {
		LOG_DBG("Not feeding timer because modem is not registered");
		return 0;
	}
	/* If the modem was sleeping, pull WAKE HIGH first to wake it */
	if (data->status.lpm.power_down.current == POWER_DOWN_EVENT_ENTER) {
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
			gpio_pin_set_dt(&config->mdm_gpio_wake, 1);
			LOG_DBG("Set WAKE pin to 1 (power down wakeup)");
		}
	}
#ifdef CONFIG_MODEM_HL78XX_USE_DELAY_BASED_POWER_DOWN
	LOG_DBG("%d Feeding Power Down Timer %d + %d = %d", __LINE__,
		CONFIG_MODEM_HL78XX_POWER_DOWN_DELAY, cmd_timeout_s, total_timeout_s);
	k_work_reschedule(&data->work.hl78xx_pwr_dwn_work, K_SECONDS(total_timeout_s));
#else
	k_work_reschedule(&data->work.hl78xx_pwr_dwn_work,
			  K_SECONDS(CONFIG_MODEM_HL78XX_POWER_DOWN_ACTIVE_TIME));
#endif
	return 0;
}

int hl78xx_power_down_settings(struct hl78xx_data *data)
{
	LOG_DBG("%d Modem Power Down Settings", __LINE__);
	return 0;
}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

#if defined(CONFIG_MODEM_HL78XX_EDRX)
/**
 * eDRX idle-sleep timer
 *
 * Unlike PSM (which sends +PSMEV:1 to signal readiness for sleep), eDRX
 * provides no URC. We use an inactivity timer instead: every AT command
 * resets the timer via hl78xx_edrx_idle_feed_timer(). When the timer
 * expires (no AT activity for MODEM_HL78XX_EDRX_IDLE_TIMEOUT seconds),
 * the WAKE pin is pulled LOW so the modem can enter eDRX low-power sleep.
 *
 * GNSS interaction (idle-eDRX mode per HL78xx GNSS App Note 5.3):
 *   During eDRX idle periods the RF path is free for GNSS. When the LTE
 *   modem wakes for a paging cycle, GNSS automatically shuts off and
 *   restarts when LTE returns to idle. GNSS assistance data can improve
 *   performance. The driver dispatches DEVICE_ASLEEP on timer expiry so
 *   the state machine can process pending GNSS requests the same way it
 *   does for PSM sleep.
 */
static void hl78xx_edrx_idle_work_handler(struct k_work *work_item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work_item);
	struct hl78xx_data *data =
		CONTAINER_OF(dwork, struct hl78xx_data, work.hl78xx_edrx_idle_work);
	const struct hl78xx_config *config = data->devices.hl78xx->config;

	LOG_DBG("eDRX idle timer expired - allowing modem to sleep");

	/* Pull WAKE pin LOW to allow the modem to enter eDRX sleep */
	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
		LOG_DBG("Set WAKE pin to 0 (eDRX idle sleep)");
	}

	data->status.lpm.edrxev.is_requested = true;

	/* Notify the state machine so it can transition to SLEEP (and
	 * optionally start GNSS if a search is pending).
	 */
	if (data->status.state != MODEM_HL78XX_STATE_SLEEP &&
	    data->status.state != MODEM_HL78XX_STATE_IDLE) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_ASLEEP);
	}
}

void hl78xx_edrx_idle_init(struct hl78xx_data *data)
{
	LOG_DBG("Initializing eDRX idle timer");
	data->status.lpm.edrxev.is_requested = false;
	data->status.lpm.edrxev.current = HL78XX_EDRX_EVENT_IDLE_NONE;
	data->status.lpm.edrxev.previous = HL78XX_EDRX_EVENT_IDLE_NONE;

	k_work_init_delayable(&data->work.hl78xx_edrx_idle_work, hl78xx_edrx_idle_work_handler);
}

void hl78xx_edrx_idle_ignore_feeding(struct hl78xx_data *data)
{
	LOG_DBG("Ignoring eDRX idle timer feeding for the next command");
	data->status.lpm.edrxev.ignore_feeding = true;
}

void hl78xx_edrx_idle_allow_feeding(struct hl78xx_data *data)
{
	LOG_DBG("Allowing eDRX idle timer feeding for the next command");
	data->status.lpm.edrxev.ignore_feeding = false;
}

bool hl78xx_edrx_idle_is_ignoring_feeding(struct hl78xx_data *data)
{
	return data->status.lpm.edrxev.ignore_feeding;
}

int hl78xx_edrx_idle_feed_timer(struct hl78xx_data *data, uint32_t cmd_timeout_s)
{
	const struct hl78xx_config *config = data->devices.hl78xx->config;
	uint32_t total_timeout_s = CONFIG_MODEM_HL78XX_EDRX_IDLE_TIMEOUT + cmd_timeout_s;

	if (hl78xx_is_registered(data) == false) {
		LOG_DBG("Not feeding eDRX idle timer because modem is not registered");
		return 0;
	}
	/* If the modem was sleeping, pull WAKE HIGH first to wake it */
	if (data->status.lpm.edrxev.current == HL78XX_EDRX_EVENT_IDLE_ENTER) {
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
			gpio_pin_set_dt(&config->mdm_gpio_wake, 1);
			LOG_DBG("Set WAKE pin to 1 (eDRX wakeup)");
		}
	}

	LOG_DBG("Feeding eDRX idle timer (%u s = %d idle + %u cmd)", total_timeout_s,
		CONFIG_MODEM_HL78XX_EDRX_IDLE_TIMEOUT, cmd_timeout_s);
	k_work_reschedule(&data->work.hl78xx_edrx_idle_work, K_SECONDS(total_timeout_s));
	return 0;
}

int hl78xx_is_edrx_idle_scheduled(struct hl78xx_data *data)
{
	return k_work_delayable_is_pending(&data->work.hl78xx_edrx_idle_work);
}

int hl78xx_edrx_idle_cancel(struct hl78xx_data *data)
{
	LOG_DBG("Canceling eDRX idle timer");
	return k_work_cancel_delayable(&data->work.hl78xx_edrx_idle_work);
}

bool hl78xx_edrx_idle_is_sleeping(struct hl78xx_data *data)
{
	return data->status.lpm.edrxev.current == HL78XX_EDRX_EVENT_IDLE_ENTER;
}

uint32_t hl78xx_edrx_idle_get_remaining_timetosleep(struct hl78xx_data *data)
{
	return k_ticks_to_ms_floor32(
		k_work_delayable_remaining_get(&data->work.hl78xx_edrx_idle_work));
}

#endif /* CONFIG_MODEM_HL78XX_EDRX */

#ifdef CONFIG_MODEM_HL78XX_PSM

void hl78xx_psmev_init(struct hl78xx_data *data)
{
	LOG_DBG("Initializing PSM settings");
	data->status.lpm.psmev.current = HL78XX_PSM_EVENT_NONE;
	data->status.lpm.psmev.previous = HL78XX_PSM_EVENT_NONE;
	data->status.lpm.psmev.is_psm_active = false;
}

#endif /* CONFIG_MODEM_HL78XX_PSM */
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

bool hl78xx_is_rsrp_value_valid(int16_t rsrp)
{
	return (rsrp >= CONFIG_MODEM_MIN_ALLOWED_SIGNAL_STRENGTH);
}

bool hl78xx_is_rsrq_value_valid(int16_t rsrq)
{
	return (rsrq >= CONFIG_MODEM_MIN_ALLOWED_SIGNAL_QUALITY);
}

bool hl78xx_is_sinr_value_valid(int16_t sinr)
{
	return (sinr >= CONFIG_MODEM_MIN_ALLOWED_SINR);
}

bool hl78xx_is_rsrp_valid(struct hl78xx_data *data)
{
	return hl78xx_is_rsrp_value_valid(data->status.signal.rsrp);
}

static void set_band_bit(uint8_t *bitmap, uint16_t band_num)
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

int hl78xx_band_bitmap_to_number(const char *bmp)
{
	if (bmp == NULL || bmp[0] == '\0') {
		return -ENODATA;
	}

	int bmp_len = strlen(bmp);

	for (int i = bmp_len - 1; i >= 0; i--) {
		char c = bmp[i];
		uint8_t nibble;

		if (c >= '0' && c <= '9') {
			nibble = c - '0';
		} else if (c >= 'a' && c <= 'f') {
			nibble = c - 'a' + 10;
		} else if (c >= 'A' && c <= 'F') {
			nibble = c - 'A' + 10;
		} else {
			continue;
		}

		if (nibble != 0) {
			int chars_from_right = bmp_len - 1 - i;
			int bit_in_nibble = find_lsb_set(nibble) - 1;
			int bit_pos = chars_from_right * 4 + bit_in_nibble;

			return bit_pos + 1;
		}
	}

	return -ENODATA;
}

void hl78xx_trim_surrounding_quotes(char *str)
{
	size_t len;

	if (str == NULL) {
		return;
	}

	len = strlen(str);

	if ((len >= 2U) && (str[0] == '"') && (str[len - 1] == '"')) {
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

	if ((full_apn == NULL) || (essential_apn == NULL) || (max_len == 0)) {
		return;
	}
	strncpy(apn_buf, full_apn, sizeof(apn_buf) - 1);
	apn_buf[sizeof(apn_buf) - 1] = '\0';
	/*  Remove surrounding quotes if any */
	hl78xx_trim_surrounding_quotes(apn_buf);
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

int hl78xx_get_uart_config(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = data->devices.hl78xx->config;
	struct uart_config uart_cfg;
	int ret;
	/* Get current UART configuration */
	ret = uart_config_get(config->uart, &uart_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to get UART config: %d", ret);
		return ret;
	}
	data->status.uart.current_baudrate = uart_cfg.baudrate;
	return 0;
}

static bool hl78xx_parse_quoted_hex_field(const char *src, uint32_t *value)
{
	char buf[16];
	char *endptr;
	const char *parse_src = src;
	size_t len;
	unsigned long parsed;

	if ((src == NULL) || (value == NULL)) {
		return false;
	}

	len = strlen(src);
	if ((len >= 2U) && (src[0] == '"') && (src[len - 1U] == '"')) {
		len -= 2U;
		if ((len == 0U) || (len >= sizeof(buf))) {
			return false;
		}
		memcpy(buf, &src[1], len);
		buf[len] = '\0';
		parse_src = buf;
	}

	errno = 0;
	parsed = strtoul(parse_src, &endptr, 16);
	if ((errno != 0) || (endptr == parse_src) || (*endptr != '\0')) {
		return false;
	}

	*value = (uint32_t)parsed;
	return true;
}

static void hl78xx_ctzeu_normalize_value(char *str)
{
	char *start;
	char *end;

	if (str == NULL) {
		return;
	}

	start = str;
	while (isspace((unsigned char)*start)) {
		start++;
	}
	if (start != str) {
		memmove(str, start, strlen(start) + 1U);
	}

	hl78xx_trim_surrounding_quotes(str);

	start = str;
	while (isspace((unsigned char)*start)) {
		start++;
	}
	if (start != str) {
		memmove(str, start, strlen(start) + 1U);
	}

	end = str + strlen(str);
	while ((end > str) && isspace((unsigned char)end[-1])) {
		end--;
	}
	*end = '\0';
}

static int hl78xx_ctzeu_parse_numeric_arg(const char *src, int min_value, int max_value, int *value)
{
	char value_buf[HL78XX_CTZEU_FIELD_MAX_LEN];
	char *endptr;
	long parsed_value;

	if ((src == NULL) || (value == NULL)) {
		return -EINVAL;
	}

	if (strlen(src) >= sizeof(value_buf)) {
		return -ENOSPC;
	}

	strcpy(value_buf, src);
	hl78xx_ctzeu_normalize_value(value_buf);

	parsed_value = strtol(value_buf, &endptr, 10);
	if ((endptr == value_buf) || (*endptr != '\0')) {
		return -EINVAL;
	}

	if ((parsed_value < min_value) || (parsed_value > max_value)) {
		return -EINVAL;
	}

	*value = (int)parsed_value;

	return 0;
}

static int hl78xx_ctzeu_extract_utime_value(char **argv, uint16_t argc, char *value,
					    size_t value_size)
{
	size_t used;

	if ((argv == NULL) || (value == NULL) || (value_size == 0U)) {
		return -EINVAL;
	}

	if (argc < 4U) {
		return -ENODATA;
	}

	used = 0U;
	value[0] = '\0';

	for (uint16_t i = 3U; i < argc; i++) {
		int ret;

		ret = snprintf(value + used, value_size - used, "%s%s", (used == 0U) ? "" : " ",
			       argv[i]);
		if ((ret < 0) || ((size_t)ret >= (value_size - used))) {
			return -ENOSPC;
		}

		used += (size_t)ret;
	}

	hl78xx_ctzeu_normalize_value(value);

	return 0;
}

static int hl78xx_ctzeu_parse_utime_value(const char *utime_value, int64_t *date_time_ms,
					  struct tm *utc_time_out)
{
	struct tm utc_time = {0};
	char value[HL78XX_CTZEU_UTIME_MAX_LEN];
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
	char separator;
	int parsed;

	if ((utime_value == NULL) || (date_time_ms == NULL)) {
		return -EINVAL;
	}

	if (strlen(utime_value) >= sizeof(value)) {
		return -ENOSPC;
	}

	strcpy(value, utime_value);
	hl78xx_ctzeu_normalize_value(value);

	parsed = sscanf(value, "%u/%u/%u%c%u:%u:%u", &year, &month, &day, &separator, &hour,
			&minute, &second);
	if ((parsed != 7) || ((separator != ',') && !isspace((unsigned char)separator))) {
		return -ENODATA;
	}

	utc_time.tm_year = (int)year - 1900;
	utc_time.tm_mon = (int)month - 1;
	utc_time.tm_mday = (int)day;
	utc_time.tm_hour = (int)hour;
	utc_time.tm_min = (int)minute;
	utc_time.tm_sec = (int)second;

	*date_time_ms = (int64_t)timeutil_timegm64(&utc_time) * 1000;
	if (utc_time_out != NULL) {
		*utc_time_out = utc_time;
	}

	return 0;
}

int hl78xx_ctzeu_parse_urc(char **argv, uint16_t argc, struct hl78xx_ctzeu_update *update)
{
	char utime_value[HL78XX_CTZEU_UTIME_MAX_LEN];
	int err;

	if ((argv == NULL) || (update == NULL)) {
		return -EINVAL;
	}

	if (argc < 3U) {
		return -ENODATA;
	}

	memset(update, 0, sizeof(*update));

	err = hl78xx_ctzeu_parse_numeric_arg(argv[1], -48, 56, &update->tz);
	if (err) {
		return err;
	}

	err = hl78xx_ctzeu_parse_numeric_arg(argv[2], 0, 2, &update->dst);
	if (err) {
		return err;
	}

	err = hl78xx_ctzeu_extract_utime_value(argv, argc, utime_value, sizeof(utime_value));
	if (err == -ENODATA) {
		return 0;
	}
	if (err) {
		return err;
	}

	err = hl78xx_ctzeu_parse_utime_value(utime_value, &update->date_time_ms, &update->utc_time);
	if (err) {
		return err;
	}

	update->has_utime = true;

	return 0;
}

static bool hl78xx_copy_cereg_timer_field(const char *src, char *dst, size_t dst_size)
{
	if ((src == NULL) || (dst == NULL) || (dst_size == 0U)) {
		return false;
	}

	if (strlen(src) >= dst_size) {
		return false;
	}

	safe_strncpy(dst, src, dst_size);
	return true;
}

void hl78xx_parse_cereg_info(struct hl78xx_data *data, char **argv, uint16_t argc, bool is_urc)
{
	struct hl78xx_cxreg_status *cxreg;
	int status_idx;
	int tac_idx;
	int cell_idx;
	int act_idx;
	int cause_type_idx;
	int reject_cause_idx;
	int active_time_idx;
	int tau_idx;
	uint32_t value;

	if ((argv == NULL) || (data == NULL) || (argc < 2U)) {
		return;
	}

	cxreg = &data->status.cxreg;
	cxreg->has_tac = false;
	cxreg->has_cell_id = false;
	cxreg->has_rat_mode = false;
	cxreg->has_cause_type = false;
	cxreg->has_reject_cause = false;
	cxreg->has_active_time = false;
	cxreg->active_time[0] = '\0';
	cxreg->has_tau = false;
	cxreg->tau[0] = '\0';

	status_idx = is_urc ? 2 : 1;
	tac_idx = status_idx + 1;
	cell_idx = status_idx + 2;
	act_idx = status_idx + 3;
	cause_type_idx = status_idx + 4;
	reject_cause_idx = status_idx + 5;
	active_time_idx = status_idx + 6;
	tau_idx = status_idx + 7;

	if (argc > status_idx) {
		cxreg->reg_status = ATOI(argv[status_idx], 0, "registration_status");
	}

	if ((argc > tac_idx) && (argv[tac_idx] != NULL) && (argv[tac_idx][0] != '\0') &&
	    hl78xx_parse_quoted_hex_field(argv[tac_idx], &value)) {
		cxreg->has_tac = true;
		cxreg->tac = value;
	}

	if ((argc > cell_idx) && (argv[cell_idx] != NULL) && (argv[cell_idx][0] != '\0') &&
	    hl78xx_parse_quoted_hex_field(argv[cell_idx], &value)) {
		cxreg->has_cell_id = true;
		cxreg->cell_id = value;
	}

	if ((argc > act_idx) && (argv[act_idx] != NULL) && (argv[act_idx][0] != '\0')) {
		cxreg->has_rat_mode = true;
		cxreg->rat_mode = hl78xx_access_tech_to_rat(ATOI(argv[act_idx], -1, "act_value"));
	}

	if ((argc > cause_type_idx) && (argv[cause_type_idx] != NULL) &&
	    (argv[cause_type_idx][0] != '\0')) {
		cxreg->cause_type = ATOI(argv[cause_type_idx], 0, "cereg_cause_type");
		cxreg->has_cause_type = true;
	}

	if ((argc > reject_cause_idx) && (argv[reject_cause_idx] != NULL) &&
	    (argv[reject_cause_idx][0] != '\0')) {
		cxreg->reject_cause = ATOI(argv[reject_cause_idx], 0, "cereg_reject_cause");
		cxreg->has_reject_cause = true;
	}

	if ((argc > active_time_idx) && (argv[active_time_idx] != NULL) &&
	    (argv[active_time_idx][0] != '\0') &&
	    hl78xx_copy_cereg_timer_field(argv[active_time_idx], cxreg->active_time,
					  sizeof(cxreg->active_time))) {
		cxreg->has_active_time = true;
	}

	if ((argc > tau_idx) && (argv[tau_idx] != NULL) && (argv[tau_idx][0] != '\0') &&
	    hl78xx_copy_cereg_timer_field(argv[tau_idx], cxreg->tau, sizeof(cxreg->tau))) {
		cxreg->has_tau = true;
	}
}

bool hl78xx_parse_plmn(const char *operator, uint16_t *mcc, uint16_t *mnc)
{
	char digits[7] = {0};
	size_t len = 0U;
	size_t index;

	if ((operator == NULL) || (mcc == NULL) || (mnc == NULL)) {
		return false;
	}

	while ((*operator != '\0') && (len < (sizeof(digits) - 1U))) {
		if ((*operator >= '0') && (*operator <= '9')) {
			digits[len++] = *operator;
		}
		operator++;
	}

	if ((len != 5U) && (len != 6U)) {
		return false;
	}

	*mcc = 0U;
	for (index = 0U; index < 3U; index++) {
		*mcc = (uint16_t)((*mcc * 10U) + (uint16_t)(digits[index] - '0'));
	}

	*mnc = 0U;
	for (index = 3U; index < len; index++) {
		*mnc = (uint16_t)((*mnc * 10U) + (uint16_t)(digits[index] - '0'));
	}

	LOG_DBG("Parsed PLMN: MCC=%u, MNC=%u", *mcc, *mnc);
	return true;
}

int hl78xx_set_network_operator_format(struct hl78xx_data *data, enum hl78xx_operator_format format)
{
	char cmd[16] = {0};

	snprintk(cmd, sizeof(cmd), "AT+COPS=3,%u", format);
	return modem_dynamic_cmd_send(data, NULL, cmd, strlen(cmd), hl78xx_get_ok_match(),
				      hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);
}

#ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE

int configure_uart_for_auto_baudrate(struct hl78xx_data *data, uint32_t baudrate)
{
	const struct hl78xx_config *config = data->devices.hl78xx->config;
	struct uart_config uart_cfg;
	int ret;

	/* Get current UART configuration */
	ret = uart_config_get(config->uart, &uart_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to get UART config: %d", ret);
		return ret;
	}

	/* Update baud rate */
	uart_cfg.baudrate = baudrate;
	LOG_INF("Trying baud rate: %d", baudrate);
	/* Apply new UART configuration */
	ret = uart_configure(config->uart, &uart_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to set UART baud rate to %d: %d", baudrate, ret);
		return ret;
	}

	/* Give the modem time to stabilize */
	k_sleep(K_MSEC(50));
	return 0;
}

int hl78xx_try_baudrate(struct hl78xx_data *data, uint32_t baudrate)
{
	int ret;
	/* Configure UART to the target baud rate */
	ret = configure_uart_for_auto_baudrate(data, baudrate);
	if (ret < 0) {
		return ret;
	}
	/* Try to send AT command and wait for response */
	const char *test_cmd = "AT";

	ret = modem_dynamic_cmd_send(data, NULL, test_cmd, strlen(test_cmd),
				     hl78xx_get_sockets_allow_matches(),
				     hl78xx_get_sockets_allow_matches_size(),
				     CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT, false);

	if (ret == 0) {
		LOG_INF("Modem responded at %d baud", baudrate);
		data->status.uart.current_baudrate = baudrate;
		return 0;
	}

	return -ENOENT;
}

int hl78xx_detect_current_baudrate(struct hl78xx_data *data)
{
	const char *baudrate_list = CONFIG_MODEM_HL78XX_AUTOBAUD_DETECTION_BAUDRATES;
	char baudrate_str[16];
	const char *ptr = baudrate_list;
	int idx = 0;
	int ret;

	LOG_INF("Starting baud rate detection...");

	while (*ptr != '\0') {
		/* Extract baud rate from string */
		while (*ptr == ' ' || *ptr == ',') {
			ptr++;
		}

		if (*ptr == '\0') {
			break;
		}

		idx = 0;
		while (*ptr >= '0' && *ptr <= '9' && idx < sizeof(baudrate_str) - 1) {
			baudrate_str[idx++] = *ptr++;
		}
		baudrate_str[idx] = '\0';

		if (idx > 0) {
			uint32_t baudrate = (uint32_t)strtol(baudrate_str, NULL, 10);

			LOG_DBG("Trying baud rate: %d", baudrate);
			ret = hl78xx_try_baudrate(data, baudrate);
			if (ret == 0) {
				return 0;
			}
		}
	}

	LOG_ERR("Failed to detect modem baud rate");
	return -ENOENT;
}

int hl78xx_switch_baudrate(struct hl78xx_data *data, uint32_t target_baudrate)
{
	char cmd_buf[32] = {0};
	const struct hl78xx_config *config = data->devices.hl78xx->config;
	struct uart_config uart_cfg;
	int ret;

	if (data->status.uart.current_baudrate == target_baudrate) {
		LOG_INF("Already at target baud rate %d", target_baudrate);
		return 0;
	}

	LOG_INF("Switching baud rate from %d to %d", data->status.uart.current_baudrate,
		target_baudrate);

	/* Send AT+IPR command to set baud rate */
	snprintf(cmd_buf, sizeof(cmd_buf), SET_BAUDRATE_CMD_FMT, target_baudrate);
	ret = modem_dynamic_cmd_send(data, NULL, cmd_buf, strlen(cmd_buf), hl78xx_get_ok_match(),
				     hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);

	if (ret < 0) {
		LOG_ERR("Failed to send baud rate change command: %d", ret);
		return ret;
	}

	/* Wait for new baud rate to take effect (~2s per AT command guide) */
	LOG_DBG("Waiting %d ms for modem to apply new baud rate", BAUDRATE_SWITCH_DELAY_MS);
	k_sleep(K_MSEC(BAUDRATE_SWITCH_DELAY_MS));

	/* Get current UART configuration */
	ret = uart_config_get(config->uart, &uart_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to get UART config: %d", ret);
		return ret;
	}

	/* Update to new baud rate */
	uart_cfg.baudrate = target_baudrate;
	ret = uart_configure(config->uart, &uart_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure new baud rate: %d", ret);
		return ret;
	}

	/* Wait for UART to stabilize */
	k_sleep(K_MSEC(50));

	/* Verify communication at new baud rate */
	const char *test_cmd = "AT";

	ret = modem_dynamic_cmd_send(data, NULL, test_cmd, strlen(test_cmd), hl78xx_get_ok_match(),
				     hl78xx_get_ok_match_size(),
				     CONFIG_MODEM_HL78XX_AUTOBAUD_TIMEOUT, false);

	if (ret < 0) {
		LOG_ERR("Failed to communicate at new baud rate %d", target_baudrate);
		return ret;
	}

#ifdef CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT
	/* Save configuration to non-volatile memory (required per AT command guide) */
	const char *cmd_save = "AT&W";

	ret = modem_dynamic_cmd_send(data, NULL, cmd_save, strlen(cmd_save), hl78xx_get_ok_match(),
				     hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);

	if (ret < 0) {
		LOG_WRN("Failed to save baud rate to NVRAM: %d (will be temporary)", ret);
		/* Continue anyway - baud rate is still active for current session */
	} else {
		LOG_DBG("Baud rate configuration saved to NVRAM");
	}
#endif /* CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT */
	data->status.uart.current_baudrate = target_baudrate;
	LOG_INF("Successfully switched to baud rate %d", target_baudrate);

	return 0;
}
#endif /* CONFIG_MODEM_HL78XX_AUTO_BAUDRATE */

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
/* Convert 8-character binary string to byte (0–255) */
int binary_str_to_byte(const char *bin_str)
{
	if (strlen(bin_str) != 8) {
		return -EINVAL; /*  Invalid input length */
	}

	int value = 0;

	for (int i = 0; i < 8; i++) {
		if (bin_str[i] == '1') {
			value = (value << 1) | 1;
		} else if (bin_str[i] == '0') {
			value = value << 1;
		} else {
			return -EINVAL; /*  Invalid character */
		}
	}

	return value;
}

/* Convert byte to 8-character binary string */
void byte_to_binary_str(uint8_t byte, char *output)
{
	if (output == NULL) {
		return; /*  Invalid output pointer */
	}

	for (int i = 7; i >= 0; i--) {
		output[7 - i] = (byte & (1 << i)) ? '1' : '0';
	}
	output[8] = '\0';
}
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

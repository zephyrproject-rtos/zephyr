/*
 * Copyright (C) 2025 Metratec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* Required for strtok_r() */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_sim7080_meas, CONFIG_MODEM_LOG_LEVEL);

#include "sim7080.h"

/* Common CPSI indices */
#define CPSI_SYS_MODE_IDX 0U
#define CPSI_OP_MODE_IDX 1U
#define CPSI_MCC_MNC_IDX 2U
/* GSM specific CPSI indices */
#define CPSI_GSM_LAC_IDX 3U
#define CPSI_GSM_CID_IDX 4U
#define CPSI_GSM_ARFCN_IDX 5U
#define CPSI_GSM_RX_LVL_IDX 6U
#define CPSI_GSM_TLO_ADJ_IDX 7U
#define CPSI_GSM_C1_C2_IDX 8U
/* LTE specific CPSI indices */
#define CPSI_LTE_TAC_IDX 3U
#define CPSI_LTE_SCI_IDX 4U
#define CPSI_LTE_PCI_IDX 5U
#define CPSI_LTE_BAND_IDX 6U
#define CPSI_LTE_EARFCN_IDX 7U
#define CPSI_LTE_DLBW_IDX 8U
#define CPSI_LTE_ULBW_IDX 9U
#define CPSI_LTE_RSRQ_IDX 10U
#define CPSI_LTE_RSRP_IDX 11U
#define CPSI_LTE_RSSI_IDX 12U
#define CPSI_LTE_RSSNR_IDX 13U

#define CPSI_GSM_ARG_COUNT 9U
#define CPSI_LTE_ARG_COUNT 14U

static struct {
	uint8_t bcs;
	uint8_t bcl;
	uint16_t volt;
} cbc_data;

MODEM_CMD_DEFINE(on_cmd_cbc)
{
	long tmp;

	tmp = strtol(argv[0], NULL, 10);
	if (errno == -ERANGE) {
		return -EBADMSG;
	}
	cbc_data.bcs = (uint8_t)tmp;
	tmp = strtol(argv[1], NULL, 10);
	if (errno == -ERANGE) {
		return -EBADMSG;
	}
	cbc_data.bcl = (uint8_t)tmp;
	tmp = strtol(argv[2], NULL, 10);
	if (errno == -ERANGE) {
		return -EBADMSG;
	}
	cbc_data.volt = (uint16_t)tmp;

	return 0;
}

int mdm_sim7080_get_battery_charge(uint8_t *bcs, uint8_t *bcl, uint16_t *voltage)
{
	int ret;
	struct modem_cmd cmds[] = {MODEM_CMD("+CBC: ", on_cmd_cbc, 3U, ",")};

	if (sim7080_get_state() == SIM7080_STATE_OFF) {
		LOG_ERR("SIM7080 not powered on!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), "AT+CBC",
				 &mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		return ret;
	}

	*bcs = cbc_data.bcs;
	*bcl = cbc_data.bcl;
	*voltage = cbc_data.volt;

	return ret;
}

static const uint8_t *ue_sys_mode_lut[] = {
	"NO SERVICE",
	"GSM",
	"LTE CAT-M1",
	"LTE NB-IOT",
};

static const uint8_t *ue_op_mode_lut[] = {
	"Online",
	"Offline",
	"Factory Test Mode",
	"Reset",
	"Low Power Mode",
};

/**
 * Check lookup table for match.
 * @param s 0 terminated source string.
 * @param lut The lookup table.
 * @param size Size of the lookup table.
 * @return Index in the lookup table or -1 on no match.
 */
static int8_t lut_match(const uint8_t *s, const uint8_t **lut, uint8_t size)
{
	for (uint8_t i = 0; i < size; i++) {
		if (strcmp(s, lut[i]) == 0) {
			return i;
		}
	}

	return -1;
}

static int cpsi_parse_minus(uint8_t *s, uint16_t *a, uint16_t *b)
{
	char *saveptr;
	char *tmp = strtok_r(s, "-", &saveptr);

	if (tmp == NULL) {
		return -1;
	}

	*a = (uint16_t)strtoul(tmp, NULL, 10);

	tmp = strtok_r(NULL, "-", &saveptr);
	if (tmp == NULL) {
		return -1;
	}

	*b = (uint16_t)strtoul(tmp, NULL, 10);

	return 0;
}

static int cpsi_parse_gsm(struct sim7080_ue_sys_info *info, uint8_t **argv, uint16_t argc)
{
	int ret = -EINVAL;

	if (argc != CPSI_GSM_ARG_COUNT) {
		LOG_ERR("Unexpected number of arguments: %u", argc);
		goto out;
	}

	ret = cpsi_parse_minus(argv[CPSI_MCC_MNC_IDX], &info->cell.gsm.mcc, &info->cell.gsm.mcn);
	if (ret != 0) {
		LOG_ERR("Failed to parse MCC/MCN");
		goto out;
	}

	info->cell.gsm.lac = (uint16_t)strtoul(argv[CPSI_GSM_LAC_IDX], NULL, 16);
	info->cell.gsm.cid = (uint16_t)strtoul(argv[CPSI_GSM_CID_IDX], NULL, 10);
	strncpy(info->cell.gsm.arfcn, argv[CPSI_GSM_ARFCN_IDX], sizeof(info->cell.gsm.arfcn) - 1);
	info->cell.gsm.rx_lvl = (int16_t)strtol(argv[CPSI_GSM_CID_IDX], NULL, 10);
	info->cell.gsm.track_lo_adjust = (int16_t)strtol(argv[CPSI_GSM_TLO_ADJ_IDX], NULL, 10);

	ret = cpsi_parse_minus(argv[CPSI_GSM_C1_C2_IDX], &info->cell.gsm.c1, &info->cell.gsm.c2);
	if (ret != 0) {
		LOG_ERR("Failed to parse C1/C2");
		goto out;
	}

out:
	return ret;
}

static int cpsi_parse_lte(struct sim7080_ue_sys_info *info, uint8_t **argv, uint16_t argc)
{
	int ret = -EINVAL;

	if (argc != CPSI_LTE_ARG_COUNT) {
		LOG_ERR("Unexpected number of arguments: %u", argc);
		goto out;
	}

	ret = cpsi_parse_minus(argv[CPSI_MCC_MNC_IDX], &info->cell.lte.mcc, &info->cell.lte.mcn);
	if (ret != 0) {
		LOG_ERR("Failed to parse MCC/MCN");
		goto out;
	}

	info->cell.lte.tac = (uint16_t)strtoul(argv[CPSI_LTE_TAC_IDX], NULL, 16);
	info->cell.lte.sci = (uint32_t)strtoul(argv[CPSI_LTE_SCI_IDX], NULL, 10);
	info->cell.lte.pci = (uint16_t)strtoul(argv[CPSI_LTE_PCI_IDX], NULL, 10);
	strncpy(info->cell.lte.band, argv[CPSI_LTE_BAND_IDX], sizeof(info->cell.lte.band) - 1);
	info->cell.lte.earfcn = (uint16_t)strtoul(argv[CPSI_LTE_EARFCN_IDX], NULL, 10);
	info->cell.lte.dlbw = (uint16_t)strtoul(argv[CPSI_LTE_DLBW_IDX], NULL, 10);
	info->cell.lte.ulbw = (uint16_t)strtoul(argv[CPSI_LTE_ULBW_IDX], NULL, 10);
	info->cell.lte.rsrq = (int16_t)strtol(argv[CPSI_LTE_RSRQ_IDX], NULL, 10);
	info->cell.lte.rsrp = (int16_t)strtol(argv[CPSI_LTE_RSRP_IDX], NULL, 10);
	info->cell.lte.rssi = (int16_t)strtol(argv[CPSI_LTE_RSSI_IDX], NULL, 10);
	info->cell.lte.rssnr = (int16_t)strtol(argv[CPSI_LTE_RSSNR_IDX], NULL, 10);
	info->cell.lte.sinr = 2 * info->cell.lte.rssnr - 20;

out:
	return ret;
}

static struct sim7080_ue_sys_info *ue_sys_info;

MODEM_CMD_DEFINE(on_cmd_cpsi)
{
	int ret = -1;

	memset(ue_sys_info, 0, sizeof(*ue_sys_info));

	if (argc < 2) {
		LOG_ERR("Insufficient number of parameters: %u", argc);
		goto out;
	}

	ret = lut_match(argv[CPSI_SYS_MODE_IDX], ue_sys_mode_lut, ARRAY_SIZE(ue_sys_mode_lut));
	if (ret < 0) {
		LOG_ERR("Illegal sys mode: %s", argv[CPSI_SYS_MODE_IDX]);
		goto out;
	}

	ue_sys_info->sys_mode = (enum sim7080_ue_sys_mode)ret;

	ret = lut_match(argv[CPSI_OP_MODE_IDX], ue_op_mode_lut, ARRAY_SIZE(ue_op_mode_lut));
	if (ret < 0) {
		LOG_ERR("Illegal op mode: %s", argv[CPSI_OP_MODE_IDX]);
		goto out;
	}

	ue_sys_info->op_mode = (enum sim7080_ue_op_mode)ret;

	if (ue_sys_info->sys_mode == SIM7080_UE_SYS_MODE_NO_SERVICE) {
		/* No further information available */
		ret = 0;
	} else if (ue_sys_info->sys_mode == SIM7080_UE_SYS_MODE_GSM) {
		ret = cpsi_parse_gsm(ue_sys_info, argv, argc);
	} else if (ue_sys_info->sys_mode == SIM7080_UE_SYS_MODE_LTE_CAT_M1 ||
			   ue_sys_info->sys_mode == SIM7080_UE_SYS_MODE_LTE_NB_IOT) {
		ret = cpsi_parse_lte(ue_sys_info, argv, argc);
	}

out:
	return ret;
}

int mdm_sim7080_get_ue_sys_info(struct sim7080_ue_sys_info *info)
{
	int ret = -1;
	struct modem_cmd cmds[] = {MODEM_CMD("+CPSI: ", on_cmd_cpsi, 14U, ",")};

	if (sim7080_get_state() == SIM7080_STATE_OFF) {
		LOG_ERR("SIM7080 not powered on!");
		goto out;
	}

	if (info == NULL) {
		ret = -EINVAL;
		goto out;
	}

	ue_sys_info = info;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), "AT+CPSI?",
				 &mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		goto out;
	}

out:
	return ret;
}

static struct tm *local_tm;

MODEM_CMD_DEFINE(on_cmd_cclk)
{
	char *saveptr;
	int ret = -1;

	/* +1 to skip leading " */
	char *date = strtok_r(argv[0] + 1, ",", &saveptr);

	if (date == NULL) {
		LOG_WRN("Failed to parse date");
		goto out;
	}

	char *time_str = strtok_r(NULL, "\"", &saveptr);

	if (time_str == NULL) {
		LOG_WRN("Failed to parse time");
		goto out;
	}

	ret = sim7080_utils_parse_time(date, time_str, local_tm);

out:
	return ret;
}

int mdm_sim7080_get_local_time(struct tm *t)
{
	int ret = -1;
	struct modem_cmd cmds[] = {MODEM_CMD("+CCLK: ", on_cmd_cclk, 1U, ",")};

	if (sim7080_get_state() == SIM7080_STATE_OFF) {
		goto out;
	}

	if (!t) {
		ret = -EINVAL;
		goto out;
	}

	local_tm = t;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), "AT+CCLK?",
				 &mdata.sem_response, K_SECONDS(2));

out:
	local_tm = NULL;
	return ret;
}

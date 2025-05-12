/*
 * Copyright (C) 2025 Metratec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_sim7080_meas, CONFIG_MODEM_LOG_LEVEL);

#include "sim7080.h"

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

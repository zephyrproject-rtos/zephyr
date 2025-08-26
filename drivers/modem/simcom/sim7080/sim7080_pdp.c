/*
 * Copyright (C) 2025 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/net/offloaded_netdev.h>
LOG_MODULE_REGISTER(modem_simcom_sim7080_pdp, CONFIG_MODEM_LOG_LEVEL);

#include "sim7080.h"

static const struct setup_cmd band_setup_cmds[] = {
	#if defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_NB1)
		SETUP_CMD_NOHANDLE("AT+CNMP=38"),
		SETUP_CMD_NOHANDLE("AT+CMNB=2"),
		SETUP_CMD_NOHANDLE("AT+CBANDCFG=\"NB-IOT\"," MDM_LTE_BANDS),
	#endif /* defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_NB1) */
	#if defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_M1)
		SETUP_CMD_NOHANDLE("AT+CNMP=38"),
		SETUP_CMD_NOHANDLE("AT+CMNB=1"),
		SETUP_CMD_NOHANDLE("AT+CBANDCFG=\"CAT-M\"," MDM_LTE_BANDS),
	#endif /* defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_M1) */
	#if defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_GSM)
		SETUP_CMD_NOHANDLE("AT+CNMP=13"),
	#endif /* defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_GSM) */
};

/*
 * Handler for RSSI query.
 *
 * +CSQ: <rssi>,<ber>
 *  rssi: 0,-115dBm; 1,-111dBm; 2...30,-110...-54dBm; 31,-52dBm or greater.
 *        99, ukn
 *  ber: Not used.
 */
MODEM_CMD_DEFINE(on_cmd_csq)
{
	int rssi = atoi(argv[0]);

	if (rssi == 0) {
		mdata.mdm_rssi = -115;
	} else if (rssi == 1) {
		mdata.mdm_rssi = -111;
	} else if (rssi > 1 && rssi < 31) {
		mdata.mdm_rssi = -114 + 2 * rssi;
	} else if (rssi == 31) {
		mdata.mdm_rssi = -52;
	} else {
		mdata.mdm_rssi = -1000;
	}

	LOG_INF("RSSI: %d", mdata.mdm_rssi);
	return 0;
}

/*
 * Queries modem RSSI.
 *
 * If a work queue parameter is provided query work will
 * be scheduled. Otherwise rssi is queried once.
 */
void sim7080_rssi_query_work(struct k_work *work)
{
	struct modem_cmd cmd[] = { MODEM_CMD("+CSQ: ", on_cmd_csq, 2U, ",") };
	static char *send_cmd = "AT+CSQ";
	int ret;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmd, ARRAY_SIZE(cmd), send_cmd,
				 &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+CSQ ret:%d", ret);
	}

	if (work) {
		k_work_reschedule_for_queue(&modem_workq, &mdata.rssi_query_work,
						K_SECONDS(RSSI_TIMEOUT_SECS));
	}
}

/*
 * Parses the non urc C(E)REG and updates registration status.
 */
MODEM_CMD_DEFINE(on_cmd_cereg)
{
	mdata.mdm_registration = atoi(argv[1]);
	LOG_INF("CREG: %u", mdata.mdm_registration);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_cgatt)
{
	int cgatt = atoi(argv[0]);

	if (cgatt) {
		mdata.status_flags |= SIM7080_STATUS_FLAG_ATTACHED;
	} else {
		mdata.status_flags &= ~SIM7080_STATUS_FLAG_ATTACHED;
	}

	LOG_INF("CGATT: %d", cgatt);
	return 0;
}

int sim7080_pdp_activate(void)
{
	int counter;
	int ret = 0;
#if defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_GSM)
	const char *buf = "AT+CREG?";
	struct modem_cmd cmds[] = { MODEM_CMD("+CREG: ", on_cmd_cereg, 2U, ",") };
#else
	const char *buf = "AT+CEREG?";
	struct modem_cmd cmds[] = { MODEM_CMD("+CEREG: ", on_cmd_cereg, 2U, ",") };
#endif /* defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_GSM) */

	/* Set the preferred bands */
	ret = modem_cmd_handler_setup_cmds(&mctx.iface, &mctx.cmd_handler, band_setup_cmds,
					   ARRAY_SIZE(band_setup_cmds), &mdata.sem_response,
					   MDM_REGISTRATION_TIMEOUT);
	if (ret != 0) {
		LOG_ERR("Failed to send band setup commands");
		goto error;
	}

	/* Wait for acceptable rssi values. */
	sim7080_rssi_query_work(NULL);

	counter = 0;
	while (counter++ < MDM_WAIT_FOR_RSSI_COUNT &&
		   (mdata.mdm_rssi >= 0 || mdata.mdm_rssi <= -1000)) {
		k_sleep(MDM_WAIT_FOR_RSSI_DELAY);
		sim7080_rssi_query_work(NULL);
	}

	if (mdata.mdm_rssi >= 0 || mdata.mdm_rssi <= -1000) {
		LOG_ERR("No valid RSSI reached");
		ret = -ENETUNREACH;
		goto error;
	}

	struct modem_cmd cgatt_cmd[] = { MODEM_CMD("+CGATT: ", on_cmd_cgatt, 1U, "") };

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cgatt_cmd,
				 ARRAY_SIZE(cgatt_cmd), "AT+CGATT?", &mdata.sem_response,
				 MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Failed to query cgatt");
		goto error;
	}

	counter = 0;
	while (counter++ < MDM_MAX_CGATT_WAITS &&
		(mdata.status_flags & SIM7080_STATUS_FLAG_ATTACHED) == 0) {
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cgatt_cmd,
					 ARRAY_SIZE(cgatt_cmd), "AT+CGATT?", &mdata.sem_response,
					 MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Failed to query cgatt");
			goto error;
		}

		k_sleep(K_SECONDS(1));
	}

	if ((mdata.status_flags & SIM7080_STATUS_FLAG_CPIN_READY) == 0 ||
		(mdata.status_flags & SIM7080_STATUS_FLAG_ATTACHED) == 0) {
		LOG_ERR("Fatal: Modem is not attached to GPRS network");
		ret = -ENETUNREACH;
		goto error;
	}

	LOG_INF("Waiting for network");

	/* Wait until the module is registered to the network.
	 * Registration will be set by urc.
	 */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), buf,
				 &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Failed to query registration");
		goto error;
	}

	counter = 0;
	while (counter++ < MDM_MAX_CEREG_WAITS && mdata.mdm_registration != 1 &&
		   mdata.mdm_registration != 5) {
		k_sleep(K_SECONDS(1));

		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), buf,
					 &mdata.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Failed to query registration");
			goto error;
		}
	}

	if (mdata.mdm_registration != 1 && mdata.mdm_registration != 5) {
		LOG_WRN("Network registration failed!");
		ret = -ENETUNREACH;
		goto error;
	}

	/* Set dual stack mode (IPv4/IPv6) */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, "AT+CNCFG=0,0",
				&mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Could not configure pdp context!");
		goto error;
	}

	/*
	 * Now activate the pdp context and wait for confirmation.
	 */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, "AT+CNACT=0,1",
				 &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Could not activate PDP context.");
		goto error;
	}

	k_sem_reset(&mdata.pdp_sem);
	ret = k_sem_take(&mdata.pdp_sem, MDM_PDP_TIMEOUT);
	if (ret < 0 || (mdata.status_flags & SIM7080_STATUS_FLAG_PDP_ACTIVE) == 0) {
		LOG_ERR("Failed to activate PDP context.");
		ret = -ENETUNREACH;
		goto error;
	}

	LOG_INF("Network active.");
	sim7080_change_state(SIM7080_STATE_NETWORKING);

	k_work_reschedule_for_queue(&modem_workq, &mdata.rssi_query_work,
					K_SECONDS(RSSI_TIMEOUT_SECS));

error:
	return ret;
}

int sim7080_pdp_deactivate(void)
{
	int ret = -EINVAL;

	if (sim7080_get_state() != SIM7080_STATE_NETWORKING) {
		LOG_WRN("Cannot deactivate pdp context in state: %d", sim7080_get_state());
		goto out;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, "AT+CNACT=0,0",
				 &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Could not deactivate PDP context.");
		goto out;
	}

	k_sem_reset(&mdata.pdp_sem);
	ret = k_sem_take(&mdata.pdp_sem, MDM_PDP_TIMEOUT);
	if (ret < 0 || (mdata.status_flags & SIM7080_STATUS_FLAG_PDP_ACTIVE) != 0) {
		LOG_ERR("PDP response timed out");
		ret = -EIO;
	}

	k_work_cancel_delayable(&mdata.rssi_query_work);

	LOG_INF("PDP context deactivated");
	sim7080_change_state(SIM7080_STATE_IDLE);

out:
	return ret;
}

/*
 * Copyright (C) 2025 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/net/offloaded_netdev.h>
LOG_MODULE_REGISTER(modem_simcom_sim7080_pdp, CONFIG_MODEM_LOG_LEVEL);

#include "sim7080.h"

static const char *const lte_mode_lut[] = {
	"AT+CMNB=2",
	"AT+CMNB=1",
	"AT+CMNB=3",
};

static const char *const lte_band_lut[] = {
	"1",  "2",  "3",  "4",  "5",  "8",  "12", "13", "14", "18",
	"19", "20", "25", "26", "27", "28", "66", "71", "85",
};

/**
 * Configure bands for the selected radio access technology.
 *
 * @param rat Radio access technology. Only SIM7080_RAT_LTE_NB1 and SIM7080_RAT_LTE_M1 are valid.
 * @param bands Band bitmap.
 */
static int configure_bands(enum sim7080_rat rat, uint32_t bands)
{
	const char *band_cmd;
	uint32_t used_bands = 0;

	if (rat == SIM7080_RAT_LTE_NB1) {
		band_cmd = "AT+CBANDCFG=\"NB-IOT\"";
		used_bands = bands & SIM7080_LTE_CHANNEL_MASK_NB1;
	} else if (rat == SIM7080_RAT_LTE_M1) {
		band_cmd = "AT+CBANDCFG=\"CAT-M\"";
		used_bands = bands & SIM7080_LTE_CHANNEL_MASK_M1;
	} else {
		return -EINVAL;
	}

	LOG_DBG("Setting bands: rat=%d, bands=0x%" PRIX32 ", used_bands=0x%" PRIX32, (int)rat,
		bands, used_bands);

	/* Check if there are any bands activated */
	if (used_bands == 0) {
		LOG_ERR("No bands in sanitized channel mask");
		return -EINVAL;
	}

	/* Do not get interrupted here */
	(void)k_sem_take(&mdata.cmd_handler_data.sem_tx_lock, K_FOREVER);

	/* Send the prefix of the band configuration */
	int ret = modem_cmd_send_data_nolock(&mctx.iface, band_cmd, strlen(band_cmd));

	if (ret != 0) {
		LOG_ERR("Failed to send band configuration prefix");
		goto out;
	}

	/* Send list of bands */
	for (uint8_t i = 0; i < SIM7080_NUM_LTE_CHANNELS; i++) {
		if ((used_bands & BIT(i)) == 0) {
			continue;
		}

		ret = modem_cmd_send_data_nolock(&mctx.iface, ",", 1);
		if (ret != 0) {
			LOG_ERR("Failed to send delimiter");
			goto out;
		}

		ret = modem_cmd_send_data_nolock(&mctx.iface, lte_band_lut[i],
						 strlen(lte_band_lut[i]));
		if (ret != 0) {
			LOG_ERR("Failed to send channel");
			goto out;
		}
	}

	k_sem_reset(&mdata.sem_response);

	/* Send the EOL */
	ret = modem_cmd_send_data_nolock(&mctx.iface, "\r", 1);
	if (ret != 0) {
		LOG_ERR("Failed to send eol");
		goto out;
	}

out:
	k_sem_give(&mdata.cmd_handler_data.sem_tx_lock);

	if (ret != 0) {
		LOG_ERR("Setting LTE bands failed: %d", ret);
		return ret;
	}

	return modem_cmd_handler_await(&mdata.cmd_handler_data, &mdata.sem_response,
				       MDM_CMD_TIMEOUT);
}

/**
 * Configure the preferred radio mode and used bands.
 */
static int configure_radio(void)
{
	const char *mode_cmd;

	if (mdata.radio.rat < SIM7080_RAT_LTE_NB1 || mdata.radio.rat > SIM7080_RAT_GSM) {
		return -EINVAL;
	}

	LOG_DBG("rat=%d, bands_nb1=0x%" PRIX32 ", bands_m1=0x%" PRIX32, (int)mdata.radio.rat,
		mdata.radio.lte_bands_nb1, mdata.radio.lte_bands_m1);

	if (mdata.radio.rat == SIM7080_RAT_GSM) {
		mode_cmd = "AT+CNMP=13";
	} else {
		mode_cmd = "AT+CNMP=38";
	}

	/* Configure the preferred radio mode */
	int ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, mode_cmd,
				 &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Could not configure radio mode");
		return ret;
	}

	/* Nothing more to do for GSM */
	if (mdata.radio.rat == SIM7080_RAT_GSM) {
		return 0;
	}

	/* Set the preferred LTE mode */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, lte_mode_lut[mdata.radio.rat],
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Could not configure LTE mode");
		return ret;
	}

	/* Configure the LTE bands */
	if (mdata.radio.rat == SIM7080_RAT_LTE_NB1 || mdata.radio.rat == SIM7080_RAT_LTE_AUTO) {
		ret = configure_bands(SIM7080_RAT_LTE_NB1, mdata.radio.lte_bands_nb1);
		if (ret != 0) {
			LOG_ERR("Failed to configure NB-IoT bands");
			return ret;
		}
	}

	if (mdata.radio.rat == SIM7080_RAT_LTE_M1 || mdata.radio.rat == SIM7080_RAT_LTE_AUTO) {
		ret = configure_bands(SIM7080_RAT_LTE_M1, mdata.radio.lte_bands_m1);
		if (ret != 0) {
			LOG_ERR("Failed to configure CAT-M1 bands");
			return ret;
		}
	}

	return 0;
}

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

	const char *buf = mdata.radio.rat == SIM7080_RAT_GSM ? "AT+CREG?" : "AT+CEREG?";
	struct modem_cmd cmds[] = {
		MODEM_CMD(mdata.radio.rat == SIM7080_RAT_GSM ? "+CREG: " : "+CEREG: ", on_cmd_cereg,
			  2U, ",")};

	/* Set the preferred bands */
	ret = configure_radio();
	if (ret != 0) {
		LOG_ERR("Failed to configure radio");
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

void mdm_sim7080_get_rat(enum sim7080_rat *rat)
{
	if (rat) {
		*rat = mdata.radio.rat;
	}
}

int mdm_sim7080_set_rat(enum sim7080_rat rat)
{
	if (rat < SIM7080_RAT_LTE_NB1 || rat > SIM7080_RAT_GSM) {
		return -EINVAL;
	}

	mdata.radio.rat = rat;

	return 0;
}

void mdm_sim7080_get_lte_bands(uint32_t *nb1, uint32_t *m1)
{
	if (nb1) {
		*nb1 = mdata.radio.lte_bands_nb1;
	}
	if (m1) {
		*m1 = mdata.radio.lte_bands_m1;
	}
}

int mdm_sim7080_set_lte_bands(uint32_t nb1, uint32_t m1)
{
	if (nb1 != 0 && (nb1 & ~SIM7080_LTE_CHANNEL_MASK_NB1) != 0) {
		LOG_ERR("Illegal NB-IoT band selection");
		return -EINVAL;
	}

	if (m1 != 0 && (m1 & ~SIM7080_LTE_CHANNEL_MASK_M1) != 0) {
		LOG_ERR("Illegal CAT-M1 band selection");
		return -EINVAL;
	}

	if (nb1 != 0) {
		mdata.radio.lte_bands_nb1 = nb1;
	}

	if (m1 != 0) {
		mdata.radio.lte_bands_m1 = m1;
	}

	return 0;
}

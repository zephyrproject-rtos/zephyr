/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hl78xx_hl7800.c
 * @brief HL7800 variant-specific operations.
 *
 * Implements the hl78xx_variant_ops callbacks for the HL7800 module.
 * Key HL7800 characteristics vs HL7812:
 *   - PSM: GPIO6 is the sole authoritative sleep/wake indicator
 *     (no +PSMEV URC available).
 *   - Socket contexts are destroyed during PSM/eDRX sleep; modem-side
 *     IDs must be invalidated and re-created on wake.
 *   - KCNXCFG connection profiles are lost after sleep; APN/KCNXCFG
 *     must be restored before data transmission.
 *   - +KSUP on PSM/eDRX exit is informational (AT settings preserved,
 *     modem just needs to re-camp via +KCELLMEAS).
 */

#include "hl78xx.h"
#include "hl78xx_chat.h"
#include "hl78xx_cfg.h"
#ifdef CONFIG_HL78XX_GNSS
#include "hl78xx_gnss.h"
#endif /* CONFIG_HL78XX_GNSS */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_DECLARE(hl78xx_dev, CONFIG_MODEM_LOG_LEVEL);

#define HL7800_GPIO6_DEBOUNCE_MS 0

static void hl78xx_hl7800_on_rrc_status_urc(struct hl78xx_data *data, bool is_idle)
{
	data->status.rrc_idle = is_idle;
	LOG_INF("RRC status: idle=%d", data->status.rrc_idle);
}

static int hl78xx_hl7800_cfg_select_rat(struct hl78xx_data *data, const char **cmd_set_rat,
					enum hl78xx_cell_rat_mode *rat_request)
{
	ARG_UNUSED(data);

	if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_M1)) {
		*cmd_set_rat = (const char *)SET_RAT_M1_CMD_LEGACY;
		*rat_request = HL78XX_RAT_CAT_M1;
		return 0;
	}

	if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_NB1)) {
		*cmd_set_rat = (const char *)SET_RAT_NB1_CMD_LEGACY;
		*rat_request = HL78XX_RAT_NB1;
		return 0;
	}

	*cmd_set_rat = NULL;
	*rat_request = HL78XX_RAT_MODE_NONE;
	return -EINVAL;
}

static int hl78xx_hl7800_cfg_apply_rat_post_select(struct hl78xx_data *data,
						   enum hl78xx_cell_rat_mode rat_request)
{
	ARG_UNUSED(rat_request);
	return hl78xx_run_gsm_dis_lte_en_reg_status_script(data);
}

static bool hl78xx_hl7800_cfg_skip_band_for_rat(struct hl78xx_data *data,
						enum hl78xx_cell_rat_mode rat_request)
{
	ARG_UNUSED(data);
	ARG_UNUSED(rat_request);
	return false;
}

static bool hl78xx_hl7800_carrier_on_gnss_pending(struct hl78xx_data *data)
{
#ifdef CONFIG_HL78XX_GNSS
	LOG_INF("HL7800 GNSS pending - routing through carrier_off/airplane");
	hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_GNSS_MODE_ENTER_REQUESTED);
	return true;
#else
	ARG_UNUSED(data);
	return false;
#endif /* CONFIG_HL78XX_GNSS */
}

static bool hl78xx_hl7800_on_gnss_mode_enter_lpm(struct hl78xx_data *data)
{
#ifdef CONFIG_HL78XX_GNSS
	ARG_UNUSED(data);
	LOG_INF("HL7800 GNSS requested in LPM - will start on PSM/EDRX entry");
	return true;
#else
	ARG_UNUSED(data);
	return false;
#endif /* CONFIG_HL78XX_GNSS */
}

static bool hl78xx_hl7800_cxreg_try_parse_rat_mode(struct hl78xx_data *data, int act_value,
						   enum hl78xx_cell_rat_mode *rat_mode)
{
	ARG_UNUSED(data);

	switch (act_value) {
	case 7:
		*rat_mode = HL78XX_RAT_CAT_M1;
		return true;
	case 9:
		*rat_mode = HL78XX_RAT_NB1;
		return true;
	default:
		*rat_mode = HL78XX_RAT_MODE_NONE;
		return true;
	}
}

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE

/* Defined in hl78xx_sockets.c */
extern void hl78xx_invalidate_socket_contexts(struct hl78xx_data *data);

/* -------------------------------------------------------------------------
 * GPIO6 handler
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7800_gpio6_handler(struct hl78xx_data *data, bool pin_state)
{
	/* HL7800: GPIO6 is the authoritative indicator for sleep/wake transitions.
	 * Unlike HL7812, the HL7800 does not generate +KPSMEV or +KEDRXEV URCs.
	 * GPIO6 LOW = modem entering sleep, GPIO6 HIGH = modem waking up.
	 *
	 * Current policy: this ISR always updates local LPM status and publishes
	 * LPM update events, but does not unconditionally dispatch DEVICE_AWAKE on
	 * GPIO6 HIGH. Wake progression is driven by explicit RESUME flow and
	 * follow-up readiness/registration URCs.
	 */
	struct hl78xx_evt lpm_evt = {0};
	bool dispatch_lpm_evt = false;

	if (!pin_state) {
		/* GPIO6 LOW: modem is entering sleep */
#ifdef CONFIG_MODEM_HL78XX_PSM
		data->status.psmev.previous = data->status.psmev.current;
		data->status.psmev.current = HL78XX_PSM_EVENT_ENTER;
		lpm_evt.type = HL78XX_LTE_PSMEV_UPDATE;
		lpm_evt.content.psm_event = data->status.psmev.current;
		dispatch_lpm_evt = true;

#endif /* CONFIG_MODEM_HL78XX_PSM */

#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
		data->status.power_down.previous = data->status.power_down.current;
		if (data->status.power_down.is_power_down_requested) {
			data->status.power_down.current = POWER_DOWN_EVENT_ENTER;
			if (data->status.power_down.current != data->status.power_down.previous) {
				struct hl78xx_evt pd_evt = {.type = HL78XX_POWER_DOWN_UPDATE};

				pd_evt.content.power_down_event = data->status.power_down.current;
				event_dispatcher_dispatch(&pd_evt);
			}
		}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

#ifdef CONFIG_MODEM_HL78XX_EDRX
		data->status.edrxev.previous = data->status.edrxev.current;
		if (data->status.edrxev.is_edrx_idle_requested) {
			data->status.edrxev.current = HL78XX_EDRX_EVENT_IDLE_ENTER;
		}
		lpm_evt.type = HL78XX_EDRX_IDLE_UPDATE;
		lpm_evt.content.edrx_event = data->status.edrxev.current;
		dispatch_lpm_evt = true;

#endif /* CONFIG_MODEM_HL78XX_EDRX */

		/* Dispatch sleep event to drive state machine into SLEEP */
		if (data->status.state != MODEM_HL78XX_STATE_SLEEP &&
		    data->status.state != MODEM_HL78XX_STATE_IDLE) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_ASLEEP);
		}
	} else {
		/* GPIO6 HIGH: modem is waking up */
#ifdef CONFIG_MODEM_HL78XX_PSM
		data->status.psmev.previous = data->status.psmev.current;
		data->status.psmev.current = HL78XX_PSM_EVENT_EXIT;

		lpm_evt.type = HL78XX_LTE_PSMEV_UPDATE;
		lpm_evt.content.psm_event = data->status.psmev.current;
		dispatch_lpm_evt = true;

#endif /* CONFIG_MODEM_HL78XX_PSM */

#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
		data->status.power_down.previous = data->status.power_down.current;
		if (data->status.power_down.is_power_down_requested) {
			data->status.power_down.current = POWER_DOWN_EVENT_EXIT;
			LOG_DBG("GPIO6 indicates wake, set power down event to EXIT");
			if (data->status.power_down.current != data->status.power_down.previous) {
				struct hl78xx_evt pd_evt = {.type = HL78XX_POWER_DOWN_UPDATE};

				pd_evt.content.power_down_event = data->status.power_down.current;
				event_dispatcher_dispatch(&pd_evt);
			}
		}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

#ifdef CONFIG_MODEM_HL78XX_EDRX
		data->status.edrxev.previous = data->status.edrxev.current;
		data->status.edrxev.current = HL78XX_EDRX_EVENT_IDLE_EXIT;
		if (data->status.edrxev.is_edrx_idle_requested) {
			data->status.edrxev.is_edrx_idle_requested = false;
		}
		lpm_evt.type = HL78XX_EDRX_IDLE_UPDATE;
		lpm_evt.content.edrx_event = data->status.edrxev.current;
		dispatch_lpm_evt = true;

#endif /* CONFIG_MODEM_HL78XX_EDRX */

		/* Intentionally do not dispatch DEVICE_AWAKE from raw GPIO6 HIGH.
		 * Keep wake sequencing centralized in explicit RESUME handling.
		 */
		if (data->status.state == MODEM_HL78XX_STATE_SLEEP) {
			LOG_DBG("GPIO6 HIGH in SLEEP observed; waiting for explicit wake flow");
		}
	}
	if (dispatch_lpm_evt) {
		event_dispatcher_dispatch(&lpm_evt);
	}
}

/* -------------------------------------------------------------------------
 * +KSUP during LPM
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7800_on_ksup_lpm(struct hl78xx_data *data)
{
	/* HL7800 generates +KSUP on PSM/eDRX exit.
	 * this KSUP is informational. AT settings are preserved except KCNXCFG, DNS setup and
	 * sockets, the modem just needs to camp on a cell (+KCELLMEAS) before data transmission.
	 *
	 * GPIO6 is the primary wake indicator.
	 */
#ifdef CONFIG_HL78XX_GNSS
	if (data->status.state == MODEM_HL78XX_STATE_SLEEP) {
		if (hl78xx_gnss_is_pending(data)) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT);
		}
	} else if (data->status.state == MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_AWAKE);
	} else {
#endif /* CONFIG_HL78XX_GNSS */
		if (data->status.state == MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT ||
		    data->status.state == MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT) {
			/* KSUP during RAT_CFG or PMC_CFG state: these states
			 * explicitly send AT+CFUN=4,1 and are waiting for the
			 * modem to reboot.  Dispatch MDM_RESTART so the event
			 * handler transitions to RUN_INIT_SCRIPT.
			 */
			LOG_DBG("KSUP after config restart (state=%d) - "
				"dispatching MDM_RESTART",
				data->status.state);
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_MDM_RESTART);
		} else {
			/* All other states: KSUP is informational.
			 * - RUN_INIT_SCRIPT: init script already handling the
			 *   restart (TIMEOUT in RAT_CFG transitioned here
			 *   before KSUP arrived).
			 * - CARRIER_ON / AWAIT_REGISTERED: PSM/eDRX exit,
			 *   GPIO6 is the primary wake indicator.
			 */
			LOG_DBG("KSUP (state=%d) - informational, "
				"init already running or PSM/eDRX exit",
				data->status.state);
		}
#ifdef CONFIG_HL78XX_GNSS
	}
#endif /* CONFIG_HL78XX_GNSS */
}

/* -------------------------------------------------------------------------
 * Await-registered state enter (LPM)
 * -------------------------------------------------------------------------
 */
static int hl78xx_hl7800_await_registered_enter_lpm(struct hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_EDRX
	if (data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_ENTER ||
	    data->status.edrxev.is_edrx_idle_requested) {
		LOG_INF("eDRX wake: waiting for modem to become responsive");
		hl78xx_start_timer(data, K_SECONDS(3));
		return 0;
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */
#ifdef CONFIG_MODEM_HL78XX_PSM
	/* Wake LTE only on a real PSM wake edge (ENTER -> EXIT).
	 * This avoids sending PING on cold boot where GPIO6 can report NONE -> EXIT.
	 */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS) &&
	    data->status.psmev.previous == HL78XX_PSM_EVENT_ENTER &&
	    data->status.psmev.current == HL78XX_PSM_EVENT_EXIT) {
		modem_dynamic_cmd_send_req(data, &(const struct hl78xx_dynamic_cmd_request){
							 .script_user_callback = NULL,
							 .cmd = (const uint8_t *)WAKE_LTE_LAYER_CMD,
							 .cmd_len = strlen(WAKE_LTE_LAYER_CMD),
							 .response_matches = NULL,
							 .matches_size = 0,
							 .response_timeout = MDM_CMD_TIMEOUT,
							 .user_cmd = false,
						 });
	}
#endif /* CONFIG_MODEM_HL78XX_PSM */
	if (hl78xx_is_registered(data)) {
		LOG_INF("Already registered on entry - proceeding to CARRIER_ON");
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_REGISTERED);
		return 0;
	}
	return 0;
}

static bool hl78xx_hl7800_await_registered_timeout_lpm(struct hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_EDRX
	if (hl78xx_is_registered(data) &&
	    (data->status.edrxev.previous == HL78XX_EDRX_EVENT_IDLE_ENTER ||
	     data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_ENTER)) {
		LOG_INF("eDRX wake settling complete - requesting signal quality check");
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_REGISTERED);
		return true;
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */

	ARG_UNUSED(data);
	return false;
}

/* -------------------------------------------------------------------------
 * Carrier-on enter (LPM)
 * -------------------------------------------------------------------------
 */
static int hl78xx_hl7800_carrier_on_enter_lpm(struct hl78xx_data *data, bool *is_lpm)
{
#ifdef CONFIG_MODEM_HL78XX_PSM
	LOG_DBG("PSMEV previous: %d, current: %d", data->status.psmev.previous,
		data->status.psmev.current);
	/* HL7800 PSM: sockets, PDP context and KCNXCFG are all destroyed
	 * when the modem enters PSM sleep.  Network setup (CGCONTRDP,
	 * DNS, KCNXCFG) is therefore needed on EVERY CARRIER_ON entry:
	 *   - First boot: modem just registered for the first time.
	 *   - PSM wake: contexts lost during sleep.
	 * Always set is_lpm = true (same rationale as POWER_DOWN).
	 */
	*is_lpm = true;
#endif /* CONFIG_MODEM_HL78XX_PSM */

#ifdef CONFIG_MODEM_HL78XX_EDRX
	LOG_DBG("eDRX event previous: %d, current: %d, is_lpm: %d", data->status.edrxev.previous,
		data->status.edrxev.current, *is_lpm);
	/* HL7800 eDRX: ENTER/EXIT or NONE/EXIT means the modem woke from
	 * eDRX idle.  Although the PDP context survives eDRX idle
	 * (CGCONTRDP still returns valid IP/DNS), the KCNXCFG connection
	 * profile is lost.  Without it, KUDPCFG / KTCPCNX / KUDPSND all
	 * fail with CME 910.
	 */
	*is_lpm = *is_lpm || (data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_EXIT);
#endif /* CONFIG_MODEM_HL78XX_EDRX */

	return 0;
}

/* -------------------------------------------------------------------------
 * Carrier-on DNS complete
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7800_carrier_on_dns_complete(struct hl78xx_data *data)
{
	/* HL7800: PSM/eDRX exit handling is now complete.  The PDP context
	 * has been restored and DNS is ready.  Clear the LPM exit state
	 * so that ensure_modem_awake() no longer considers the modem to be
	 * in LPM, then release the socket semaphore.
	 */
#ifdef CONFIG_MODEM_HL78XX_PSM
	data->status.psmev.previous = HL78XX_PSM_EVENT_EXIT;
#endif /* CONFIG_MODEM_HL78XX_PSM */

#ifdef CONFIG_MODEM_HL78XX_EDRX
	data->status.edrxev.previous = HL78XX_EDRX_EVENT_IDLE_EXIT;
#endif /* CONFIG_MODEM_HL78XX_EDRX */

	hl78xx_release_socket_comms(data);
}

/* -------------------------------------------------------------------------
 * PSM deregistration handling
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7800_carrier_on_deregistered_psm(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	/* HL7800: Must pull WAKE LOW to allow modem to enter PSM sleep.
	 * Without this, the modem stays awake and GPIO6 never transitions.
	 * GPIO6 going LOW will then confirm actual sleep entry.
	 */
	LOG_DBG("Network state: %d", data->status.registration.network_state_current);
	if (data->status.registration.network_state_current == CELLULAR_REGISTRATION_UNKNOWN) {
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
			gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
		}
		LOG_DBG("Deregistered - WAKE LOW, awaiting GPIO6 LOW to confirm PSM");
	}
}

/* -------------------------------------------------------------------------
 * Sleep entry
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7800_on_sleep_enter(struct hl78xx_data *data)
{
	/* HL7800 destroys all TCP/UDP socket contexts when entering PSM/eDRX.
	 * Invalidate modem-side socket IDs now so they will be transparently
	 * re-created when the application uses them after wake.
	 */
	hl78xx_invalidate_socket_contexts(data);
}

/* -------------------------------------------------------------------------
 * Sleep -> BUS_OPENED state routing
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7800_sleep_bus_opened_routing(struct hl78xx_data *data)
{
#ifdef CONFIG_HL78XX_GNSS
	if (hl78xx_gnss_is_pending(data)) {
#ifdef CONFIG_MODEM_HL78XX_PSM
		return;
#else
		/* HL7800 eDRX + GNSS: process GNSS immediately */
		LOG_INF("GNSS pending from sleep - transitioning to GNSS init");
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT);
		return;
#endif /* CONFIG_MODEM_HL78XX_PSM */
	}
#endif /* CONFIG_HL78XX_GNSS */

#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	/* POWER_DOWN mode: modem reboots on wake, need full init.
	 * Run init script from scratch (like first power-up).
	 */
	LOG_INF("Power-down wake: running init script");
	hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
#else
	/* HL7800 PSM/eDRX exit: AT settings are preserved, no init
	 * script needed.  Wait for +KCELLMEAS URC (confirms the modem
	 * is camped on a cell and ready for data).  AWAIT_REGISTERED
	 * transitions to CARRIER_ON where KCNXCFG restores PDP/TCP.
	 */
	LOG_INF("HL7800 warm wake: waiting for +KCELLMEAS");
	hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_REGISTERED);
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
}

/* -------------------------------------------------------------------------
 * Socket layer LPM check
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7800_check_lpm_state(struct hl78xx_data *data, bool *in_lpm,
					  bool *early_return)
{
#ifdef CONFIG_MODEM_HL78XX_EDRX
	*in_lpm = *in_lpm || (data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_ENTER) ||
		  (data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_EXIT &&
		   data->status.edrxev.previous == HL78XX_EDRX_EVENT_IDLE_ENTER);

	LOG_DBG("EDRX status: current=%d previous=%d is_edrx_idle_requested=%d",
		data->status.edrxev.current, data->status.edrxev.previous,
		data->status.edrxev.is_edrx_idle_requested);

	/* Not currently in eDRX sleep and no idle sleep requested → awake */
	if (!*in_lpm && !data->status.edrxev.is_edrx_idle_requested) {
		*early_return = true;
		return;
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */
}

/* -------------------------------------------------------------------------
 * Ready-for-data callbacks
 * -------------------------------------------------------------------------
 */
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

/* -------------------------------------------------------------------------
 * Variant ops table
 * -------------------------------------------------------------------------
 */
const struct hl78xx_variant_ops hl78xx_variant_ops_hl7800 = {
	.socket_lpm_recreate_required = true,
	.cfg_select_rat = hl78xx_hl7800_cfg_select_rat,
	.cfg_apply_rat_post_select = hl78xx_hl7800_cfg_apply_rat_post_select,
	.cfg_skip_band_for_rat = hl78xx_hl7800_cfg_skip_band_for_rat,
	.on_kstatev_urc = NULL, /* HL7800 does not use +KSTATEV */
	.on_psmev_urc = NULL,   /* HL7800 does not use +PSMEV */
	.on_rrc_status_urc = hl78xx_hl7800_on_rrc_status_urc,
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	.gpio6_debounce_ms = HL7800_GPIO6_DEBOUNCE_MS,
	.gpio6_handler = hl78xx_hl7800_gpio6_handler,
	.on_ksup_lpm = hl78xx_hl7800_on_ksup_lpm,
	.await_registered_enter_lpm = hl78xx_hl7800_await_registered_enter_lpm,
	.await_registered_timeout_lpm = hl78xx_hl7800_await_registered_timeout_lpm,
	.carrier_on_enter_lpm = hl78xx_hl7800_carrier_on_enter_lpm,
	.carrier_on_dns_complete = hl78xx_hl7800_carrier_on_dns_complete,
	.carrier_on_deregistered_psm = hl78xx_hl7800_carrier_on_deregistered_psm,
	.on_sleep_enter = hl78xx_hl7800_on_sleep_enter,
	.sleep_bus_opened_routing = hl78xx_hl7800_sleep_bus_opened_routing,
	.check_lpm_state = hl78xx_hl7800_check_lpm_state,
	.on_registered_ready = NULL, /* HL7800 readiness gates on carrier_on_dns_complete */
	.on_kcellmeas_ready = NULL,  /* HL7800 readiness gates on carrier_on_dns_complete */
#endif                               /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	.cxreg_try_parse_rat_mode = hl78xx_hl7800_cxreg_try_parse_rat_mode,
	.carrier_on_gnss_pending = hl78xx_hl7800_carrier_on_gnss_pending,
	.on_gnss_mode_enter_lpm = hl78xx_hl7800_on_gnss_mode_enter_lpm,
};

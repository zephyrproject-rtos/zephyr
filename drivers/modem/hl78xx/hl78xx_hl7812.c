/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hl78xx_hl7812.c
 * @brief HL7812 variant-specific operations.
 *
 * Implements the hl78xx_variant_ops callbacks for the HL7812 module.
 * Key HL7812 characteristics vs HL7800:
 *   - PSM: +PSMEV URC provides authoritative sleep/wake indication,
 *     GPIO6 supplements it for eDRX/power-down tracking.
 *   - Socket contexts survive PSM/eDRX sleep (no re-creation needed).
 *   - +KCELLMEAS is the "ready for data" signal that releases the
 *     socket semaphore after wake.
 *   - +KSTATEV provides RAT mode updates (separate from +CEREG).
 */

#include "hl78xx.h"
#include "hl78xx_chat.h"
#ifdef CONFIG_HL78XX_GNSS
#include "hl78xx_gnss.h"
#endif /* CONFIG_HL78XX_GNSS */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_DECLARE(hl78xx_dev, CONFIG_MODEM_LOG_LEVEL);

#define HL7812_GPIO6_DEBOUNCE_MS 300

static void hl78xx_hl7812_on_kstatev_urc(struct hl78xx_data *data, int state_value, int rat_mode)
{
	struct hl78xx_evt event = {.type = HL78XX_LTE_RAT_UPDATE};

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	hl78xx_on_kstatev_parser(data, state_value, rat_mode);
#endif /* CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG */

	if (rat_mode != data->status.registration.rat_mode) {
		data->status.registration.rat_mode = rat_mode;
		event.content.rat_mode = data->status.registration.rat_mode;
		event_dispatcher_dispatch(&event);
	}
}

static void hl78xx_hl7812_on_rrc_status_urc(struct hl78xx_data *data, bool is_idle)
{
	data->status.rrc_idle = is_idle;
	LOG_INF("RRC status: idle=%d", data->status.rrc_idle);
}

static void hl78xx_hl7812_on_psmev_urc(struct hl78xx_data *data, int psmev_value)
{
#if defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE) && defined(CONFIG_MODEM_HL78XX_PSM)
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;
	struct hl78xx_evt event = {.type = HL78XX_LTE_PSMEV_UPDATE};

	data->status.psmev.previous = data->status.psmev.current;
	data->status.psmev.current = psmev_value;
	event.content.psm_event = data->status.psmev.current;

	event_dispatcher_dispatch(&event);

	if (data->status.psmev.current == HL78XX_PSM_EVENT_ENTER) {
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
			gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
			LOG_DBG("Set WAKE pin to 0");
		}
		if (data->status.state != MODEM_HL78XX_STATE_SLEEP &&
		    data->status.state != MODEM_HL78XX_STATE_IDLE) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_ASLEEP);
		}
	} else if (data->status.psmev.current == HL78XX_PSM_EVENT_EXIT) {
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
			gpio_pin_set_dt(&config->mdm_gpio_wake, 1);
			LOG_DBG("Set WAKE pin to 1");
		}
	} else {
		LOG_DBG("Unknown PSM event value: %d", data->status.psmev.current);
	}
#else
	ARG_UNUSED(data);
	ARG_UNUSED(psmev_value);
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE && CONFIG_MODEM_HL78XX_PSM */
}

static int hl78xx_hl7812_cfg_select_rat(struct hl78xx_data *data, const char **cmd_set_rat,
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

#ifdef CONFIG_MODEM_HL78XX_RAT_GSM
	if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_GSM)) {
		*cmd_set_rat = (const char *)SET_RAT_GSM_CMD_LEGACY;
		*rat_request = HL78XX_RAT_GSM;
		return 0;
	}
#endif /* CONFIG_MODEM_HL78XX_RAT_GSM */

#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN
	if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_NBNTN)) {
		*cmd_set_rat = (const char *)SET_RAT_NBNTN_CMD_LEGACY;
		*rat_request = HL78XX_RAT_NBNTN;
		return 0;
	}
#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */

	*cmd_set_rat = NULL;
	*rat_request = HL78XX_RAT_MODE_NONE;
	return -EINVAL;
}

static int hl78xx_hl7812_cfg_apply_rat_post_select(struct hl78xx_data *data,
						   enum hl78xx_cell_rat_mode rat_request)
{
#ifdef CONFIG_MODEM_HL78XX_RAT_GSM
	if (rat_request == HL78XX_RAT_GSM) {
		return hl78xx_run_lte_dis_gsm_en_reg_status_script(data);
	}
#endif /* CONFIG_MODEM_HL78XX_RAT_GSM */

	return hl78xx_run_gsm_dis_lte_en_reg_status_script(data);
}

static bool hl78xx_hl7812_cfg_skip_band_for_rat(struct hl78xx_data *data,
						enum hl78xx_cell_rat_mode rat_request)
{
	ARG_UNUSED(data);

#ifdef CONFIG_MODEM_HL78XX_RAT_GSM
	if (rat_request == HL78XX_RAT_GSM) {
		return true;
	}
#endif /* CONFIG_MODEM_HL78XX_RAT_GSM */

	return false;
}

#ifdef CONFIG_HL78XX_GNSS
static bool hl78xx_hl7812_carrier_on_gnss_pending(struct hl78xx_data *data)
{
	LOG_INF("Processing pending GNSS mode request (queued before modem ready)");
	hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT);
	return true;
}

static bool hl78xx_hl7812_on_gnss_mode_enter_lpm(struct hl78xx_data *data)
{
	ARG_UNUSED(data);
#ifdef CONFIG_MODEM_HL78XX_PSM
#if defined(CONFIG_HL78XX_GNSS_PROCESS_PRE_PSM)
	LOG_INF("GNSS mode requested in LPM - will start on PSM entry (pre-PSM)");
#else
	LOG_INF("GNSS mode requested in LPM - will start on user wakeup (post-PSM)");
#endif
#else
	LOG_INF("GNSS mode requested in LPM - will start on eDRX entry");
#endif /* CONFIG_MODEM_HL78XX_PSM */
	return true;
}
#else
static bool hl78xx_hl7812_carrier_on_gnss_pending(struct hl78xx_data *data)
{
	ARG_UNUSED(data);
	return false;
}

static bool hl78xx_hl7812_on_gnss_mode_enter_lpm(struct hl78xx_data *data)
{
	ARG_UNUSED(data);
	return false;
}
#endif /* CONFIG_HL78XX_GNSS */

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE

/* -------------------------------------------------------------------------
 * GPIO6 handler
 * -------------------------------------------------------------------------
 */
#if defined(CONFIG_MODEM_HL78XX_POWER_DOWN) || defined(CONFIG_MODEM_HL78XX_EDRX)
static void __maybe_unused hl78xx_hl7812_append_pending_event(struct hl78xx_evt *pending_evts,
							      uint8_t *pending_evt_count,
							      enum hl78xx_evt_type evt_type,
							      int evt_value)
{
	pending_evts[*pending_evt_count].type = evt_type;

	ARG_UNUSED(evt_value);

#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	if (evt_type == HL78XX_POWER_DOWN_UPDATE) {
		pending_evts[*pending_evt_count].content.power_down_event = evt_value;
		(*pending_evt_count)++;
		return;
	}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

#ifdef CONFIG_MODEM_HL78XX_EDRX
	if (evt_type == HL78XX_EDRX_IDLE_UPDATE) {
		pending_evts[*pending_evt_count].content.edrx_event = evt_value;
		(*pending_evt_count)++;
		return;
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */

	/* Keep event list compact if type has no payload in this build. */
	pending_evts[*pending_evt_count].type = HL78XX_EVT_TYPE_COUNT;
}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN || CONFIG_MODEM_HL78XX_EDRX */

static void hl78xx_hl7812_gpio6_handle_low(struct hl78xx_data *data,
					   struct hl78xx_evt *pending_evts,
					   uint8_t *pending_evt_count)
{
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	data->status.power_down.previous = data->status.power_down.current;
	if (data->status.power_down.is_power_down_requested) {
		data->status.power_down.current = POWER_DOWN_EVENT_ENTER;
		if (data->status.power_down.current != data->status.power_down.previous) {
			hl78xx_hl7812_append_pending_event(pending_evts, pending_evt_count,
							   HL78XX_POWER_DOWN_UPDATE,
							   data->status.power_down.current);
		}
	}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

#ifdef CONFIG_MODEM_HL78XX_EDRX
	/* HL7812: accept GPIO6 LOW as eDRX ENTER only when idle-sleep was
	 * explicitly requested by the eDRX idle timer.
	 */
	if (data->status.edrxev.is_edrx_idle_requested) {
		if (data->status.edrxev.current != HL78XX_EDRX_EVENT_IDLE_ENTER) {
			data->status.edrxev.previous = data->status.edrxev.current;
			data->status.edrxev.current = HL78XX_EDRX_EVENT_IDLE_ENTER;
			hl78xx_hl7812_append_pending_event(pending_evts, pending_evt_count,
							   HL78XX_EDRX_IDLE_UPDATE,
							   data->status.edrxev.current);
		}
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */

	if (data->status.state != MODEM_HL78XX_STATE_SLEEP &&
	    data->status.state != MODEM_HL78XX_STATE_IDLE) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_ASLEEP);
	}
}

static void hl78xx_hl7812_gpio6_handle_high(struct hl78xx_data *data,
					    struct hl78xx_evt *pending_evts,
					    uint8_t *pending_evt_count)
{
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	data->status.power_down.previous = data->status.power_down.current;
	if (data->status.power_down.is_power_down_requested) {
		data->status.power_down.current = POWER_DOWN_EVENT_EXIT;
		LOG_DBG("GPIO6 indicates wake, set power down event to EXIT");
		if (data->status.power_down.current != data->status.power_down.previous) {
			hl78xx_hl7812_append_pending_event(pending_evts, pending_evt_count,
							   HL78XX_POWER_DOWN_UPDATE,
							   data->status.power_down.current);
		}
	}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

#ifdef CONFIG_MODEM_HL78XX_EDRX
	/* HL7812: accept GPIO6 HIGH as eDRX EXIT only after explicit wake
	 * intent cleared the idle-request marker in RESUME flow.
	 */
	if (!data->status.edrxev.is_edrx_idle_requested) {
		if (data->status.edrxev.current != HL78XX_EDRX_EVENT_IDLE_EXIT) {
			data->status.edrxev.previous = data->status.edrxev.current;
			data->status.edrxev.current = HL78XX_EDRX_EVENT_IDLE_EXIT;
			hl78xx_hl7812_append_pending_event(pending_evts, pending_evt_count,
							   HL78XX_EDRX_IDLE_UPDATE,
							   data->status.edrxev.current);
		}
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */

	/* HL7812 GPIO6 can pulse during deeper low-power transitions.
	 * Do not treat GPIO6 HIGH as an authoritative wake signal.
	 * Wake progression is handled by explicit RESUME flow plus URCs
	 * (+CEREG/+KCELLMEAS and +PSMEV where applicable).
	 */
	if (data->status.state == MODEM_HL78XX_STATE_SLEEP) {
		LOG_DBG("GPIO6 HIGH in SLEEP ignored for wake dispatch on HL7812");
	}
}

static void hl78xx_hl7812_gpio6_handler(struct hl78xx_data *data, bool pin_state)
{
	struct hl78xx_evt pending_evts[2] = {0};
	uint8_t pending_evt_count = 0;

	if (!pin_state) {
		/* GPIO6 LOW: modem entering sleep/idle */
		hl78xx_hl7812_gpio6_handle_low(data, pending_evts, &pending_evt_count);
	} else {
		/* GPIO6 HIGH: modem waking */
		hl78xx_hl7812_gpio6_handle_high(data, pending_evts, &pending_evt_count);
	}

	for (uint8_t i = 0; i < pending_evt_count; i++) {
		event_dispatcher_dispatch(&pending_evts[i]);
	}
}

/* -------------------------------------------------------------------------
 * +KSUP during LPM
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7812_on_ksup_lpm(struct hl78xx_data *data)
{
	/* HL7812 does not use +KSUP for PSM/eDRX wake indication.
	 * A +KSUP with MODULE_READY when already booted is always an
	 * unexpected restart (e.g. firmware crash or watchdog).
	 */
	LOG_DBG("Modem unexpected restart detected");
	hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_MDM_RESTART);
}

/* -------------------------------------------------------------------------
 * Await-registered state enter (LPM)
 * -------------------------------------------------------------------------
 */
static int hl78xx_hl7812_await_registered_enter_lpm(struct hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_PSM
	LOG_DBG("PSM event: previous=%d current=%d", data->status.psmev.previous,
		data->status.psmev.current);

	if (hl78xx_psm_is_active(data) && IS_ENABLED(CONFIG_HL78XX_GNSS)) {
		return modem_dynamic_cmd_send_req(
			data, &(const struct hl78xx_dynamic_cmd_request){
				      .script_user_callback = NULL,
				      .cmd = (const uint8_t *)WAKE_LTE_LAYER_CMD,
				      .cmd_len = strlen(WAKE_LTE_LAYER_CMD),
				      .response_matches = NULL,
				      .matches_size = 0,
				      .response_timeout = MDM_CMD_TIMEOUT,
				      .user_cmd = false,
			      });
	}
	return 0;
#endif /* CONFIG_MODEM_HL78XX_PSM */

#ifdef CONFIG_MODEM_HL78XX_EDRX
	/* HL7812 eDRX: modem stays registered throughout idle cycles
	 * and after GNSS returns to LTE.  Trigger +KCELLMEAS so the
	 * URC handler fires REGISTERED -> CARRIER_ON and releases
	 * the socket semaphore.
	 */
	if (hl78xx_is_registered(data) && !IS_ENABLED(CONFIG_MODEM_HL78XX_PSM)) {
		data->status.edrxev.is_edrx_idle_requested = false;
		return modem_dynamic_cmd_send_req(
			data, &(const struct hl78xx_dynamic_cmd_request){
				      .script_user_callback = NULL,
				      .cmd = (const uint8_t *)CHECK_LTE_COVERAGE_CMD,
				      .cmd_len = strlen(CHECK_LTE_COVERAGE_CMD),
				      .response_matches = hl78xx_get_ok_match(),
				      .matches_size = hl78xx_get_ok_match_size(),
				      .response_timeout = MDM_CMD_TIMEOUT,
				      .user_cmd = false,
			      });
	}
	return 0;
#endif /* CONFIG_MODEM_HL78XX_EDRX */
	return 0;
}

static bool hl78xx_hl7812_await_registered_timeout_lpm(struct hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_EDRX
	/* HL7812: avoid depending on GPIO6-derived eDRX previous/current state.
	 * During wake settling, if we're already registered, proceed.
	 */
	if (hl78xx_is_registered(data)) {
		LOG_INF("eDRX wake settling complete - requesting signal quality check");
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_REGISTERED);
		return true;
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */
	return false;
}

/* -------------------------------------------------------------------------
 * Carrier-on enter (LPM)
 * -------------------------------------------------------------------------
 */
static int hl78xx_hl7812_carrier_on_enter_lpm(struct hl78xx_data *data, bool *is_lpm)
{
#ifdef CONFIG_MODEM_HL78XX_PSM
	LOG_DBG("PSMEV previous: %d, current: %d", data->status.psmev.previous,
		data->status.psmev.current);

	*is_lpm = *is_lpm || ((data->status.psmev.previous == HL78XX_PSM_EVENT_NONE &&
			       data->status.psmev.current == HL78XX_PSM_EVENT_NONE) &&
			      data->status.registration.network_state_previous !=
				      CELLULAR_REGISTRATION_UNKNOWN);

	/* HL7812 PSM: sockets, IP and DNS are all retained after PSM exit.
	 * +KCELLMEAS releases the socket semaphore, so there is no need to
	 * re-run CGCONTRDP or refresh the DNS resolver.
	 *
	 * When +CEREG:5 fires (triggering CARRIER_ON), psmev.current is
	 * still ENTER (set by +PSMEV:1 before sleep) because the +PSMEV:0
	 * URC hasn't arrived yet.  Detect this: if current is ENTER and
	 * we're already in CARRIER_ON, the modem woke from PSM.  Clear the
	 * state and return — data can flow as soon as +KCELLMEAS fires.
	 */
	if (data->status.psmev.current == HL78XX_PSM_EVENT_ENTER) {
		LOG_DBG("HL7812 PSM wake: sockets retained, skipping CGCONTRDP/DNS");
		data->status.psmev.previous = HL78XX_PSM_EVENT_ENTER;
		data->status.psmev.current = HL78XX_PSM_EVENT_EXIT;
		return 1; /* early return from carrier_on_enter */
	}
#endif /* CONFIG_MODEM_HL78XX_PSM */

#ifdef CONFIG_MODEM_HL78XX_EDRX
	/* HL7812 eDRX wake classification:
	 * keep the legacy NONE -> EXIT detection that triggers deferred
	 * LPM restore sequencing in CARRIER_ON (CGCONTRDP/DNS path), but
	 * do not drive these states from GPIO6 edges.
	 */
	LOG_DBG("eDRX event previous: %d, current: %d, is_lpm: %d", data->status.edrxev.previous,
		data->status.edrxev.current, *is_lpm);
	*is_lpm = *is_lpm || ((data->status.edrxev.previous == HL78XX_EDRX_EVENT_IDLE_NONE &&
			       data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_EXIT));
#endif /* CONFIG_MODEM_HL78XX_EDRX */

	return 0;
}

/* -------------------------------------------------------------------------
 * PSM deregistration handling
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7812_carrier_on_deregistered_psm(struct hl78xx_data *data)
{
	/* HL7812: GPIO6 LOW and/or +PSMEV:1 URC will confirm actual PSM
	 * entry.  No GPIO action needed here — just wait for confirmation.
	 */
	LOG_DBG("Deregistered - awaiting GPIO6 LOW to confirm PSM entry");
}

/* -------------------------------------------------------------------------
 * Sleep entry
 * -------------------------------------------------------------------------
 */
/* -------------------------------------------------------------------------
 * Sleep -> BUS_OPENED state routing
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7812_sleep_bus_opened_routing(struct hl78xx_data *data)
{
#ifdef CONFIG_HL78XX_GNSS
	if (hl78xx_gnss_is_pending(data)) {
		/* Process pending GNSS mode entry request */
		LOG_INF("GNSS pending from sleep - transitioning to GNSS init");
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT);
		return;
	}
#endif /* CONFIG_HL78XX_GNSS */

	/* HL7812 PSM/eDRX exit: AT settings preserved, no init script needed.
	 * Wait for +CEREG and +KCELLMEAS URCs to confirm ready for data.
	 */
	hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_REGISTERED);
}

/* -------------------------------------------------------------------------
 * Socket layer LPM check
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7812_check_lpm_state(struct hl78xx_data *data, bool *in_lpm,
					  bool *early_return)
{
#ifdef CONFIG_MODEM_HL78XX_EDRX
	/* HL7812: use explicit eDRX idle request as the LPM indicator,
	 * not GPIO6-derived previous/current transitions.
	 */
	*in_lpm = *in_lpm || data->status.edrxev.is_edrx_idle_requested;

	LOG_DBG("EDRX status: is_edrx_idle_requested=%d current=%d previous=%d",
		data->status.edrxev.is_edrx_idle_requested, data->status.edrxev.current,
		data->status.edrxev.previous);

	if (!*in_lpm) {
		*early_return = true;
		return;
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */
}

/* -------------------------------------------------------------------------
 * Ready-for-data callbacks
 * -------------------------------------------------------------------------
 */
static void hl78xx_hl7812_on_kcellmeas_ready(struct hl78xx_data *data)
{
	/* HL7812: +KCELLMEAS is authoritative "ready for data" signal. */
	hl78xx_release_socket_comms(data);
}

#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

/* -------------------------------------------------------------------------
 * Variant ops table
 * -------------------------------------------------------------------------
 */
const struct hl78xx_variant_ops hl78xx_variant_ops_hl7812 = {
	.socket_lpm_recreate_required = false,
	.cfg_select_rat = hl78xx_hl7812_cfg_select_rat,
	.cfg_apply_rat_post_select = hl78xx_hl7812_cfg_apply_rat_post_select,
	.cfg_skip_band_for_rat = hl78xx_hl7812_cfg_skip_band_for_rat,
	.on_kstatev_urc = hl78xx_hl7812_on_kstatev_urc,
	.on_psmev_urc = hl78xx_hl7812_on_psmev_urc,
	.on_rrc_status_urc = hl78xx_hl7812_on_rrc_status_urc,
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	.gpio6_debounce_ms = HL7812_GPIO6_DEBOUNCE_MS,
	.gpio6_handler = hl78xx_hl7812_gpio6_handler,
	.on_ksup_lpm = hl78xx_hl7812_on_ksup_lpm,
	.await_registered_enter_lpm = hl78xx_hl7812_await_registered_enter_lpm,
	.await_registered_timeout_lpm = hl78xx_hl7812_await_registered_timeout_lpm,
	.carrier_on_enter_lpm = hl78xx_hl7812_carrier_on_enter_lpm,
	.carrier_on_dns_complete = NULL, /* HL7812 releases socket sem via +KCELLMEAS */
	.carrier_on_deregistered_psm = hl78xx_hl7812_carrier_on_deregistered_psm,
	.on_sleep_enter = NULL, /* HL7812 retains socket contexts through sleep */
	.sleep_bus_opened_routing = hl78xx_hl7812_sleep_bus_opened_routing,
	.check_lpm_state = hl78xx_hl7812_check_lpm_state,
	.on_registered_ready = NULL,
	.on_kcellmeas_ready = hl78xx_hl7812_on_kcellmeas_ready,
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	.cxreg_try_parse_rat_mode = NULL,
	.carrier_on_gnss_pending = hl78xx_hl7812_carrier_on_gnss_pending,
	.on_gnss_mode_enter_lpm = hl78xx_hl7812_on_gnss_mode_enter_lpm,
};

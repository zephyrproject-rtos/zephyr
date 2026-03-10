/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_GNSS_H_
#define ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_GNSS_H_

#include <zephyr/types.h>

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>

#include "../../gnss/gnss_nmea0183.h"
#include "../../gnss/gnss_nmea0183_match.h"
#include "../../gnss/gnss_parse.h"

#include "hl78xx_gnss_parsers.h"
#include "hl78xx.h"

/**
 * @brief GNSS search state machine states
 *
 * These states track the GNSS search lifecycle from idle to actively searching.
 * The state machine ensures proper handling of the HL78xx GNSS constraints:
 * - GNSS cannot operate when LTE is active (shared RF path)
 * - GNSS requires CFUN=4 (airplane mode) or PSM/idle-eDRX
 */
enum hl78xx_gnss_search_state {
	/** GNSS is idle, not searching, no pending request */
	HL78XX_GNSS_SEARCH_STATE_IDLE = 0,
	/** Search requested but waiting for airplane mode (CFUN=4) */
	HL78XX_GNSS_SEARCH_STATE_WAITING_FOR_AIRPLANE,
	/** AT+GNSSSTART sent, waiting for +GNSSEV: 1,x response */
	HL78XX_GNSS_SEARCH_STATE_STARTING,
	/** GNSS is actively searching for satellites */
	HL78XX_GNSS_SEARCH_STATE_SEARCHING,
	/** AT+GNSSSTOP sent, waiting for +GNSSEV: 2,x response */
	HL78XX_GNSS_SEARCH_STATE_STOPPING,
	/** GNSS start failed (LTE blocked it) - user should retry when in airplane mode */
	HL78XX_GNSS_SEARCH_STATE_BLOCKED_BY_LTE,
};

/**
 * @brief GNSS search request configuration
 *
 * Configuration structure for initiating a GNSS search. This provides
 * a cleaner API than setting individual parameters before search.
 */
struct hl78xx_gnss_search_config {
	/** NMEA output port configuration (use NMEA_OUTPUT_NONE for GNSSLOC only) */
	enum nmea_output_port output_port;
	/** Search timeout in milliseconds (0 = no timeout) */
	uint32_t timeout_ms;
	/** Automatically start when airplane mode is entered */
	bool auto_start_on_airplane;
};

struct hl78xx_gnss_config {
	const struct device *parent_modem;
	const uint16_t fix_rate_default;
};

struct hl78xx_gnss_data {
	const struct device *dev;
	struct gnss_nmea0183_match_data match_data;
#if defined(CONFIG_GNSS_SATELLITES) && defined(CONFIG_HL78XX_GNSS_SOURCE_NMEA)
	struct gnss_satellite satellites[CONFIG_HL78XX_GNSS_SATELLITES_COUNT];
#endif
#ifdef CONFIG_HL78XX_GNSS_AUX_DATA_PARSER
	/* Auxiliary GNSS data */
	struct hl78xx_gnss_nmea_aux_data aux_data;
#endif /* CONFIG_HL78XX_GNSS_AUX_DATA_PARSER */
	/* Reference to parent modem's chat and pipe */
	struct hl78xx_data *parent_data;

	uint32_t fix_interval_ms;
	uint32_t search_timeout_ms;

	/* GNSS search state machine */
	enum hl78xx_gnss_search_state search_state;

	/* GNSS state tracking (legacy - being replaced by search_state) */
	bool gnss_init_status;
	bool gnss_start_status;

	enum gnss_position_events position_status;

	/* Search configuration */
	struct hl78xx_gnss_search_config search_config;
	enum nmea_output_port output_port;
	uint16_t fix_rate_ms;
	gnss_systems_t enabled_systems;
	bool static_filter_enabled;

	/* Exit to LTE pending flag - set when GNSS mode exit is requested during search */
	bool exit_to_lte_pending;

	/* Enter GNSS mode pending flag - set when GNSS mode is requested before modem is ready */
	bool gnss_mode_enter_pending;

#ifdef CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE
	/* A-GNSS assistance data status - updated by AT+GNSSAD? queries */
	struct hl78xx_agnss_status agnss_status;
#endif /* CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE */

	/* Lock for thread-safe API access */
	struct k_sem lock;
	k_timepoint_t pm_deadline;
};

/**
 * @brief Set the GNSS search state
 */
void gnss_set_search_state(struct hl78xx_gnss_data *gnss, enum hl78xx_gnss_search_state new_state);

/**
 * @brief Get GNSS data structure from modem data
 *
 * Helper function to safely navigate the data structure hierarchy.
 *
 * @param data Modem data structure
 * @return Pointer to GNSS data, or NULL if not available
 */
struct hl78xx_gnss_data *hl78xx_get_gnss_data(struct hl78xx_data *data);

/**
 * @brief Check and clear the pending GNSS mode entry flag
 *
 * Atomically checks if GNSS mode entry was requested before modem was ready,
 * and clears the flag if set.
 *
 * @param data Modem data structure
 * @return true if pending flag was set (and is now cleared), false otherwise
 */
bool hl78xx_gnss_check_and_clear_pending(struct hl78xx_data *data);

/**
 * @brief Check if GNSS mode entry is pending (without clearing)
 *
 * @param data Modem data structure
 * @return true if GNSS mode entry is pending, false otherwise
 */
bool hl78xx_gnss_is_pending(struct hl78xx_data *data);

/**
 * @brief Get the current GNSS search state
 */
enum hl78xx_gnss_search_state hl78xx_gnss_get_search_state(struct hl78xx_gnss_data *gnss);
/**
 * @brief Check if modem is in GNSS mode (state machine)
 */
bool hl78xx_is_in_gnss_mode(struct hl78xx_data *data);
/**
 * @brief Check if GNSS is actively searching
 */
bool hl78xx_gnss_is_searching(struct hl78xx_gnss_data *gnss);

/**
 * @brief Check if a GNSS search is queued (waiting for airplane mode)
 */
bool hl78xx_gnss_search_is_queued(struct hl78xx_gnss_data *gnss);
/**
 * @brief Check if GNSS search is active or pending
 *
 * Returns true if GNSS is in any state other than IDLE, meaning configuration
 * changes should not be allowed.
 */
bool hl78xx_gnss_is_active(struct hl78xx_gnss_data *gnss);
/**
 * @brief Queue a GNSS search request
 *
 * Queues the search and will automatically start when modem enters airplane mode.
 *
 * @return true if search was already queued, false if newly queued
 */
bool hl78xx_gnss_queue_search(struct hl78xx_gnss_data *gnss);

/**
 * @brief Clear the GNSS search queue
 */
bool hl78xx_gnss_clear_search_queue(struct hl78xx_gnss_data *gnss);

/**
 * @brief Start GNSS search with configuration
 *
 * Higher-level API that validates modem state and starts search.
 *
 * @param gnss GNSS data structure
 * @param config Search configuration (NULL uses defaults)
 * @return 0 on success, -EBUSY if LTE is active, -EALREADY if already searching
 */
int hl78xx_gnss_start_search(struct hl78xx_gnss_data *gnss,
			     const struct hl78xx_gnss_search_config *config);

/**
 * @brief Request GNSS search stop
 */
int hl78xx_gnss_stop_search(struct hl78xx_gnss_data *gnss);

/* State machine handlers for modem driver integration */
int hl78xx_on_run_gnss_init_script_state_enter(struct hl78xx_data *data);
int hl78xx_on_run_gnss_init_script_state_leave(struct hl78xx_data *data);
void hl78xx_run_gnss_init_script_event_handler(struct hl78xx_data *data, enum hl78xx_event event);

int hl78xx_on_gnss_search_started_state_enter(struct hl78xx_data *data);
int hl78xx_on_gnss_search_started_state_leave(struct hl78xx_data *data);
void hl78xx_gnss_search_started_event_handler(struct hl78xx_data *data, enum hl78xx_event event);

#endif /* ZEPHYR_DRIVERS_MODEM_HL78XX_HL78XX_GNSS_H_ */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is part of CC33XX
 *
 * Copyright (C) 2023 Texas Instruments
 *
 * Author: Bashar Badir <bashar_badir@ti.com>
 *
 */

#ifndef __CONF_H__
#define __CONF_H__

struct cc33xx_conf_header {
	uint32_t magic;
	uint16_t fw_major_version;
	uint16_t fw_minor_version;
	uint16_t fw_api_version;
	uint16_t fw_build_version;
	uint8_t sp_major_version;
	uint8_t sp_minor_version;
	uint16_t sp_revision_version;
	uint32_t sp_build_version;
	uint32_t checksum;
} __attribute__((__packed__));

#define CC33XX_CONF_MAGIC 0x10e100ca

#define CC33XX_CONF_FW_MAJOR_VERSION 0x0001
#define CC33XX_CONF_FW_MINOR_VERSION 0x0007
#define CC33XX_CONF_FW_API_VERSION   0x0000
#define CC33XX_CONF_FW_BUILD_VERSION 0x0121

#define CC33XX_CONF_SP_MAJOR_VERSION    0x00
#define CC33XX_CONF_SP_MINOR_VERSION    0x00
#define CC33XX_CONF_SP_REVISION_VERSION 0x0000
#define CC33XX_CONF_SP_BUILD_VERSION    0x00000007

#define CC33XX_CONF_MASK 0x0000ffff
#define CC33X_CONF_SIZE  (sizeof(struct cc33xx_conf_file))

enum {
	CONF_HW_BIT_RATE_1MBPS = BIT(1),
	CONF_HW_BIT_RATE_2MBPS = BIT(2),
	CONF_HW_BIT_RATE_5_5MBPS = BIT(3),
	CONF_HW_BIT_RATE_11MBPS = BIT(4),
	CONF_HW_BIT_RATE_6MBPS = BIT(5),
	CONF_HW_BIT_RATE_9MBPS = BIT(6),
	CONF_HW_BIT_RATE_12MBPS = BIT(7),
	CONF_HW_BIT_RATE_18MBPS = BIT(8),
	CONF_HW_BIT_RATE_24MBPS = BIT(9),
	CONF_HW_BIT_RATE_36MBPS = BIT(10),
	CONF_HW_BIT_RATE_48MBPS = BIT(11),
	CONF_HW_BIT_RATE_54MBPS = BIT(12),
	CONF_HW_BIT_RATE_MCS_0 = BIT(13),
	CONF_HW_BIT_RATE_MCS_1 = BIT(14),
	CONF_HW_BIT_RATE_MCS_2 = BIT(15),
	CONF_HW_BIT_RATE_MCS_3 = BIT(16),
	CONF_HW_BIT_RATE_MCS_4 = BIT(17),
	CONF_HW_BIT_RATE_MCS_5 = BIT(18),
	CONF_HW_BIT_RATE_MCS_6 = BIT(19),
	CONF_HW_BIT_RATE_MCS_7 = BIT(20)
};

struct cc33xx_clk_cfg {
	uint32_t n;
	uint32_t m;
	uint32_t p;
	uint32_t q;
	uint8_t swallow;
};

#define CONF_TX_MAX_AC_COUNT 4

struct conf_tx_ac_category {
	/*
	 * The AC class identifier.
	 *
	 * Range: enum conf_tx_ac
	 */
	uint8_t ac;

	/*
	 * The contention window minimum size (in slots) for the access
	 * class.
	 *
	 * Range: uint8_t
	 */
	uint8_t cw_min;

	/*
	 * The contention window maximum size (in slots) for the access
	 * class.
	 *
	 * Range: uint8_t
	 */
	uint16_t cw_max;

	/*
	 * The AIF value (in slots) for the access class.
	 *
	 * Range: uint8_t
	 */
	uint8_t aifsn;

	/*
	 * The TX Op Limit (in microseconds) for the access class.
	 *
	 * Range: uint16_t
	 */
	uint16_t tx_op_limit;

	/*
	 * Is the MU EDCA configured
	 *
	 * Range: uint8_t
	 */
	uint8_t is_mu_edca;

	/*
	 *  The AIFSN value for the corresonding access class
	 *
	 * Range: uint8_t
	 */
	uint8_t mu_edca_aifs;

	/*
	 * The ECWmin and ECWmax value is indicating contention window maximum
	 * size (in slots) for the access
	 *
	 * Range: uint8_t
	 */
	uint8_t mu_edca_ecw_min_max;

	/*
	 * The MU EDCA timer (in microseconds) obtaining an EDCA TXOP
	 * for STA using MU EDCA parameters
	 *
	 * Range: uint8_t
	 */
	uint8_t mu_edca_timer;
} __attribute__((__packed__));

struct conf_tx_tid {
	uint8_t channel_type;
	uint8_t ps_scheme;
} __attribute__((__packed__));

struct conf_tx_settings {
	/*
	 * Configuration for access categories for TX rate control.
	 */
	uint8_t ac_conf_count;

	/*struct conf_tx_ac_category ac_conf[CONF_TX_MAX_AC_COUNT];*/
	struct conf_tx_ac_category ac_conf0;
	struct conf_tx_ac_category ac_conf1;
	struct conf_tx_ac_category ac_conf2;
	struct conf_tx_ac_category ac_conf3;

	/*
	 * AP-mode - allow this number of TX retries to a station before an
	 * event is triggered from FW.
	 * In AP-mode the hlids of unreachable stations are given in the
	 * "sta_tx_retry_exceeded" member in the event mailbox.
	 */
	uint8_t max_tx_retries;

	/*
	 * Configuration for TID parameters.
	 */
	uint8_t tid_conf_count;

	/* struct conf_tx_tid tid_conf[]; */
	struct conf_tx_tid tid_conf0;
	struct conf_tx_tid tid_conf1;
	struct conf_tx_tid tid_conf2;
	struct conf_tx_tid tid_conf3;
	struct conf_tx_tid tid_conf4;
	struct conf_tx_tid tid_conf5;
	struct conf_tx_tid tid_conf6;
	struct conf_tx_tid tid_conf7;

	/*
	 * Max time in msec the FW may delay frame TX-Complete interrupt.
	 *
	 * Range: uint16_t
	 */
	uint16_t tx_compl_timeout;

	/*
	 * The rate used for control messages and scanning on the 2.4GHz band
	 *
	 * Range: CONF_HW_BIT_RATE_* bit mask
	 */
	uint32_t basic_rate;

	/*
	 * The rate used for control messages and scanning on the 5GHz band
	 *
	 * Range: CONF_HW_BIT_RATE_* bit mask
	 */
	uint32_t basic_rate_5;

	/* Time in ms for Tx watchdog timer to expire */
	uint32_t tx_watchdog_timeout;
} __attribute__((__packed__));

#define CONF_MAX_BCN_FILT_IE_COUNT 32

struct conf_bcn_filt_rule {
	/*
	 * IE number to which to associate a rule.
	 *
	 * Range: uint8_t
	 */
	uint8_t ie;

	/*
	 * Rule to associate with the specific ie.
	 *
	 * Range: CONF_BCN_RULE_PASS_ON_*
	 */
	uint8_t rule;

	/*
	 * OUI for the vendor specifie IE (221)
	 */
	uint8_t oui[3];

	/*
	 * Type for the vendor specifie IE (221)
	 */
	uint8_t type;

	/*
	 * Version for the vendor specifie IE (221)
	 */
	uint8_t version[2];
} __attribute__((__packed__));

struct conf_conn_settings {
	/*
	 * Enable or disable the beacon filtering.
	 *
	 * Range: CONF_BCN_FILT_MODE_*
	 */
	uint8_t bcn_filt_mode;

	/*
	 * Configure Beacon filter pass-thru rules.
	 */
	uint8_t bcn_filt_ie_count;

	/*struct conf_bcn_filt_rule bcn_filt_ie[CONF_MAX_BCN_FILT_IE_COUNT];*/
	/* struct conf_bcn_filt_rule bcn_filt_ie[32]; */
	struct conf_bcn_filt_rule bcn_filt_ie0;
	struct conf_bcn_filt_rule bcn_filt_ie1;
	struct conf_bcn_filt_rule bcn_filt_ie2;
	struct conf_bcn_filt_rule bcn_filt_ie3;
	struct conf_bcn_filt_rule bcn_filt_ie4;
	struct conf_bcn_filt_rule bcn_filt_ie5;
	struct conf_bcn_filt_rule bcn_filt_ie6;
	struct conf_bcn_filt_rule bcn_filt_ie7;
	struct conf_bcn_filt_rule bcn_filt_ie8;
	struct conf_bcn_filt_rule bcn_filt_ie9;
	struct conf_bcn_filt_rule bcn_filt_ie10;
	struct conf_bcn_filt_rule bcn_filt_ie11;
	struct conf_bcn_filt_rule bcn_filt_ie12;
	struct conf_bcn_filt_rule bcn_filt_ie13;
	struct conf_bcn_filt_rule bcn_filt_ie14;
	struct conf_bcn_filt_rule bcn_filt_ie15;
	struct conf_bcn_filt_rule bcn_filt_ie16;
	struct conf_bcn_filt_rule bcn_filt_ie17;
	struct conf_bcn_filt_rule bcn_filt_ie18;
	struct conf_bcn_filt_rule bcn_filt_ie19;
	struct conf_bcn_filt_rule bcn_filt_ie20;
	struct conf_bcn_filt_rule bcn_filt_ie21;
	struct conf_bcn_filt_rule bcn_filt_ie22;
	struct conf_bcn_filt_rule bcn_filt_ie23;
	struct conf_bcn_filt_rule bcn_filt_ie24;
	struct conf_bcn_filt_rule bcn_filt_ie25;
	struct conf_bcn_filt_rule bcn_filt_ie26;
	struct conf_bcn_filt_rule bcn_filt_ie27;
	struct conf_bcn_filt_rule bcn_filt_ie28;
	struct conf_bcn_filt_rule bcn_filt_ie29;
	struct conf_bcn_filt_rule bcn_filt_ie30;
	struct conf_bcn_filt_rule bcn_filt_ie31;

	/*
	 * The number of consecutive beacons to lose, before the firmware
	 * becomes out of synch.
	 *
	 * Range: uint32_t
	 */
	uint32_t synch_fail_thold;

	/*
	 * After out-of-synch, the number of TU's to wait without a further
	 * received beacon (or probe response) before issuing the BSS_EVENT_LOSE
	 * event.
	 *
	 * Range: uint32_t
	 */
	uint32_t bss_lose_timeout;

	/*
	 * Specifies the dynamic PS timeout in ms that will be used
	 * by the FW when in AUTO_PS mode
	 */
	uint16_t dynamic_ps_timeout;

	/*
	 * Maximum listen interval supported by the driver in units of beacons.
	 *
	 * Range: uint16_t
	 */
	uint8_t max_listen_interval;

	/*
	 * Default sleep authorization for a new STA interface. This determines
	 * whether we can go to ELP.
	 */
	uint8_t sta_sleep_auth;

	/*
	 * Default RX BA Activity filter configuration
	 */
	uint8_t suspend_rx_ba_activity;
} __attribute__((__packed__));

struct conf_scan_settings {
	/*
	 * The minimum time to wait on each channel for active scans
	 * This value will be used whenever there's a connected interface.
	 *
	 * Range: uint32_t tu/1000
	 */
	uint32_t min_dwell_time_active;

	/*
	 * The maximum time to wait on each channel for active scans
	 * This value will be currently used whenever there's a
	 * connected interface. It shouldn't exceed 30000 (~30ms) to avoid
	 * possible interference of voip traffic going on while scanning.
	 *
	 * Range: uint32_t tu/1000
	 */
	uint32_t max_dwell_time_active;

	/* The minimum time to wait on each channel for active scans
	 * when it's possible to have longer scan dwell times.
	 * Currently this is used whenever we're idle on all interfaces.
	 * Longer dwell times improve detection of networks within a
	 * single scan.
	 *
	 * Range: uint32_t tu/1000
	 */
	uint32_t min_dwell_time_active_long;

	/* The maximum time to wait on each channel for active scans
	 * when it's possible to have longer scan dwell times.
	 * See min_dwell_time_active_long
	 *
	 * Range: uint32_t tu/1000
	 */
	uint32_t max_dwell_time_active_long;

	/* time to wait on the channel for passive scans (in TU/1000) */
	uint32_t dwell_time_passive;

	/* time to wait on the channel for DFS scans (in TU/1000) */
	uint32_t dwell_time_dfs;

	/*
	 * Number of probe requests to transmit on each active scan channel
	 *
	 * Range: uint8_t
	 */
	uint16_t num_probe_reqs;

	/*
	 * Scan trigger (split scan) timeout. The FW will split the scan
	 * operation into slices of the given time and allow the FW to schedule
	 * other tasks in between.
	 *
	 * Range: uint32_t Microsecs
	 */
	uint32_t split_scan_timeout;
} __attribute__((__packed__));

struct conf_sched_scan_settings {
	/*
	 * The base time to wait on the channel for active scans (in TU/1000).
	 * The minimum dwell time is calculated according to this:
	 * min_dwell_time = base + num_of_probes_to_be_sent * delta_per_probe
	 * The maximum dwell time is calculated according to this:
	 * max_dwell_time = min_dwell_time + max_dwell_time_delta
	 */
	uint32_t base_dwell_time;

	/* The delta between the min dwell time and max dwell time for
	 * active scans (in TU/1000s). The max dwell time is used by the FW once
	 * traffic is detected on the channel.
	 */
	uint32_t max_dwell_time_delta;

	/* Delta added to min dwell time per each probe in 2.4 GHz (TU/1000) */
	uint32_t dwell_time_delta_per_probe;

	/* Delta added to min dwell time per each probe in 5 GHz (TU/1000) */
	uint32_t dwell_time_delta_per_probe_5;

	/* time to wait on the channel for passive scans (in TU/1000) */
	uint32_t dwell_time_passive;

	/* time to wait on the channel for DFS scans (in TU/1000) */
	uint32_t dwell_time_dfs;

	/* number of probe requests to send on each channel in active scans */
	uint8_t num_probe_reqs;

	/* RSSI threshold to be used for filtering */
	int8_t rssi_threshold;

	/* SNR threshold to be used for filtering */
	int8_t snr_threshold;

	/*
	 * number of short intervals scheduled scan cycles before
	 * switching to long intervals
	 */
	uint8_t num_short_intervals;

	/* interval between each long scheduled scan cycle (in ms) */
	uint16_t long_interval;
} __attribute__((__packed__));

struct conf_ht_setting {
	uint8_t rx_ba_win_size;

	/* DEFAULT / WIDE / SISO20 */
	uint8_t mode;
} __attribute__((__packed__));

struct conf_fwlog {
	/* Continuous or on-demand */
	uint8_t mode;

	/*
	 * Number of memory blocks dedicated for the FW logger
	 *
	 * Range: 2-16, or 0 to disable the FW logger
	 */
	uint8_t mem_blocks;

	/* Minimum log level threshold */
	uint8_t severity;

	/* Include/exclude timestamps from the log messages */
	uint8_t timestamp;

	/* See enum cc33xx_fwlogger_output */
	uint8_t output;

	/* Regulates the frequency of log messages */
	uint8_t threshold;
} __attribute__((__packed__));

enum cc33xx_ht_mode {
	/* Default - use MIMO, fallback to SISO20 */
	HT_MODE_DEFAULT = 0,

	/* Wide - use SISO40 */
	HT_MODE_WIDE = 1,

	/* Use SISO20 */
	HT_MODE_SISO20 = 2,
};

struct coex_wifi_group_priorities {
	/*
	 * WLAN activity groups priorities
	 *
	 * Range: 0 - 15
	 */
	uint8_t coex_enabled;
	uint8_t wlan_group_core_active_priority;
	uint8_t wlan_group_traffic_priority;
	uint8_t wlan_group_scan_priority;
	uint8_t wlan_group_twt_traffic_priority;
	uint8_t wlan_group_management_sequence_priority;
	uint8_t wlan_group_scan_high_priority;
	uint8_t wlan_group_broadcast_multicast_priority;
	uint8_t wlan_group_beacon_priority;
	uint8_t wlan_group_management_sequence_urgent_priority;
	uint8_t wlan_group_beacon_urgent_priority;
	uint8_t wlan_group_phy_command_priority;
} __attribute__((__packed__));

struct coex_ble_group_priorities {
	/*
	 * BLE command groups priorities
	 *
	 * Range: 0 - 15
	 */
	uint8_t ble_group_test;
	uint8_t ble_group_setup;
	uint8_t ble_group_connected;
	uint8_t ble_group_observer;
	uint8_t ble_group_broadcaster;
	uint8_t ble_group_initiator;
	uint8_t ble_group_urgent_priority;
} __attribute__((__packed__));

struct coex_external_soc_priorities {
	/*
	 * External SoC low and high priorities
	 *
	 * Range: 0 - 15
	 */
	uint8_t low_priority;
	uint8_t high_priority;
} __attribute__((__packed__));

struct conf_coex_configuration {
	/*
	 * Work without Coex HW
	 *
	 * Range: 1 - YES, 0 - NO
	 */
	uint8_t Disable_coex;

	/*
	 * Tie breaker configuration
	 *
	 * Range: 0 - 3 for each tie breaker.
	 *
	 * 0 is lowest and 3 is the highest.
	 */
	uint8_t tie_breaker_ble;
	uint8_t tie_breaker_wifi;
	uint8_t tie_breaker_ext_soc;

	/*
	 * Coex BLE configuration
	 */
	uint8_t ble_enabled;
	uint8_t ble_grant_polarity;
	uint8_t ble_pta_signalling_mode;
	uint8_t ble_tx_bypass_val;
	uint8_t ble_rx_bypass_val;

	/*
	 * Coex WiFi configuration
	 */
	uint8_t wifi_enabled;
	uint8_t wifi_grant_polarity;
	uint8_t wifi_alt_band_input_bypass;
	uint8_t wifi_rx_only_input_bypass;
	uint8_t wifi_alt_band_input_bypass_val;
	uint8_t wifi_rx_only_input_bypass_val;

	/*
	 * External SoC entity enable
	 *
	 * 0 - NO
	 * 1 - YES
	 */
	uint8_t is_Ext_soc_enable;
	/*
	 * External SoC PTA signalling mode
	 *
	 * 00 - Reserved
	 * 01 - 1-wire
	 * 02 - 2-wires
	 * 11 - 3-wires
	 */
	uint8_t ext_soc_pta_signalling_mode;
	/*
	 * External SoC request polarity
	 *
	 * 0 - Active Low (Default)
	 *
	 * 1 - Active High
	 */
	uint8_t ext_soc_request_polarity;
	/*
	 * External SoC priority polarity
	 *
	 * 0 - Active Low (Default)
	 *
	 * 1 - Active High
	 */
	uint8_t ext_soc_priority_polarity;
	/*
	 * External SoC grant polarity
	 *
	 * 0 - Active Low
	 *
	 * 1 - Active High (Default)
	 */
	uint8_t ext_soc_grant_polarity;
	/*
	 * External SoC grant renew bypass
	 *
	 * 0 - SOC needs to renew grant.
	 *
	 * 1 - Bypass SOC grant renew
	 */
	uint8_t ext_soc_grant_renew_bypass;
	/*
	 * External SoC request signal detection mechanism
	 *
	 * 00 - Rise edge detection (L -> H)
	 * 01 - Fall edge detection (H -> L)
	 * 10 - Level detection (active high)
	 * 11 - Level detection (active low)
	 */
	uint8_t ext_soc_request_signal_detection;

	/*
	 * External SoC GPIO pins
	 */
	uint8_t ext_soc_grant_pin;
	uint8_t ext_soc_request_pin;
	uint8_t ext_soc_priority_pin;

	/*
	 * Coex grant delays
	 *
	 * Range: 0 - 200 usec
	 */
	uint8_t wifi_to_ble_grant_delay;
	uint8_t wifi_to_ext_soc_grant_delay;
	uint8_t ble_to_wifi_grant_delay;
	uint8_t ble_to_ext_soc_grant_delay;
	uint8_t ext_soc_to_wifi_grant_delay;
	uint8_t ext_soc_to_ble_grant_delay;

	/*
	 * Coex grant times
	 */
	uint16_t wifi_min_grant_time;
	uint16_t ble_min_grant_time;
	uint16_t ble_max_grant_time;
	uint16_t ext_soc_min_grant_time;
	uint16_t ext_soc_max_grant_time;
	/*
	 * BLE T2 time
	 *
	 * Range: 0 - 50 us
	 */
	uint8_t ble_t2_time;
	/*
	 * External SoC T2 time
	 *
	 * Range: 0 - 50 us
	 */
	uint8_t ext_soc_t2_time;

	struct coex_wifi_group_priorities wifi_group_priorities;
	struct coex_ble_group_priorities ble_group_priorities;
	struct coex_external_soc_priorities external_soc_priorities;

} __attribute__((__packed__));

struct conf_iomux_configuration {
	/*
	 * For any iomux pull value:
	 * 1: Pull up
	 * 2: Pull down
	 * 3: Pull disable
	 * ff: Default value set by HW
	 * ANY other value is invalid
	 */
	uint8_t slow_clock_in_pull_val;
	uint8_t sdio_clk_pull_val;
	uint8_t sdio_cmd_pull_val;
	uint8_t sdio_d0_pull_val;
	uint8_t sdio_d1_pull_val;
	uint8_t sdio_d2_pull_val;
	uint8_t sdio_d3_pull_val;
	uint8_t host_irq_wl_pull_val;
	uint8_t uart1_tx_pull_val;
	uint8_t uart1_rx_pull_val;
	uint8_t uart1_cts_pull_val;
	uint8_t uart1_rts_pull_val;
	uint8_t coex_priority_pull_val;
	uint8_t coex_req_pull_val;
	uint8_t coex_grant_pull_val;
	uint8_t host_irq_ble_pull_val;
	uint8_t fast_clk_req_pull_val;
	uint8_t ant_sel_pull_val;
} __attribute__((__packed__));

struct conf_ant_diversity {
	/*
	 * First beacons after antenna switch.
	 * In this window we asses our satisfaction from the new antenna.
	 */
	uint8_t fast_switching_window;
	/*
	 * Deltas above this threshold between the curiosity score and
	 * the average RSSI will lead to antenna switch.
	 */
	uint8_t rssi_delta_for_switching;
	/*
	 * Used in the first beacons after antenna switch:
	 * Deltas above this threshold between the average RSSI and
	 * the curiosity score will make us switch back the antennas.
	 */
	uint8_t rssi_delta_for_fast_switching;
	/*
	 * Curiosity punishment in beacon timeout after an antenna switch.
	 */
	uint8_t curiosity_punish;
	/*
	 * Curiosity raise in beacon timeout not after an antenna switch.
	 */
	uint8_t curiosity_raise;
	/*
	 * Used for the average RSSI punishment in beacon timeout
	 * not after antenna switch.
	 */
	uint8_t consecutive_missed_beacons_threshold;
	/*
	 * Used in the curiosity metric.
	 */
	uint8_t compensation_log;
	/*
	 * Used in the average RSSI metric.
	 */
	uint8_t log_alpha;
	/*
	 * Curiosity initialization score.
	 */
	int8_t initial_curiosity;
	/*
	 * MR configuration: should the AP follow the STA antenna or use the default antenna.
	 */
	uint8_t ap_follows_sta;
	/*
	 * MR configuration: should the BLE follow the STA antenna or use the default antenna.
	 */
	uint8_t ble_follows_sta;
	/*
	 * The antenna to use when the diversity mechanism is not in charge.
	 */
	uint8_t default_antenna;
	/*
	 * Rssi low limit
	 */
	uint8_t rssi_low_limit;

	/*
	 * Override Coex grant via SW
	 */
	uint8_t wifi_grant_override;
	uint8_t wifi_grant_override_val;
	uint8_t ext_soc_grant_override;
	uint8_t ext_soc_grant_override_val;
	uint8_t ble_grant_override;
	uint8_t ble_grant_override_val;

	/*
	 * Override Coex grant
	 *
	 * 0 - External enable
	 * 1 - BLE only enable
	 * 2 - WiFi only enable
	 */
	uint8_t grant_override;

	/*
	 * SW enable grant (external mask)
	 *
	 * 0 - External mask mode
	 * 1 - BLE only enable
	 * 2 - WiFi only enable
	 */
	uint8_t external_mask;

	/*
	 * Antenna control MUX override
	 */
	uint8_t antenna_control_override;

	/*
	 * SW enable register (sw_antenna_control)
	 */
	uint8_t sw_enable_register;

	/*
	 * SW index
	 * Bit 0 - SW index value
	 * Bit 1 - SW index override
	 */
	uint8_t sw_index;

	/*
	 * GPIO antenna index (0-5)
	 */
	uint8_t gpio_antenna_index_0;
	uint8_t gpio_antenna_index_1;
	uint8_t gpio_antenna_index_2;
	uint8_t gpio_antenna_index_3;
	uint8_t gpio_antenna_index_4;
	uint8_t gpio_antenna_index_5;

	/*
	 * Antenna selection bits
	 */
	uint8_t ant_sel_0;
	uint8_t ant_sel_1;
	uint8_t ant_sel_2;
	uint8_t ant_sel_3;
} __attribute__((__packed__));

struct conf_thermal_thresholds {
	int16_t high_threshold_24G;
	int16_t low_threshold_24G;
	int16_t high_threshold_5G;
	int16_t low_threshold_5G;
	int16_t high_sample_threshold;
	uint8_t enable_value;
} __attribute__((__packed__));

struct conf_limit105c_params {
	uint8_t enable_value;
	uint8_t maxMCS;
	uint8_t maxOFDM;
} __attribute__((__packed__));

struct conf_gpadc_params {
	uint8_t overrideEn;
	uint8_t disable_measuring;
	uint8_t pmcioSlop;
	uint16_t pmcioIntercept;
	uint8_t rfcioSlop;
	uint16_t rfcioIntercept;
	uint32_t measurePeriodUsec;
	uint8_t dcSlopExponent;
} __attribute__((__packed__));

struct cc33xx_core_conf {
	uint8_t enable_5ghz;
	uint8_t enable_ble;
	uint8_t enable_at_test_debug;     /* only for at-test chips, debug mode (ignoring disable
					   * efuses)
					   */
	uint8_t disable_beamforming_fftp; /* for PG version 2.0 */
	uint32_t BleUartBaudrate;
	uint8_t enable_FlowCtrl;
	int8_t ble_default_tx_power;
	uint8_t listen_interval;
	uint8_t wake_up_event;
	uint8_t suspend_listen_interval;
	uint8_t suspend_wake_up_event;
	uint8_t per_channel_power_limit[520]; /* per channel power limitations */
	uint32_t internalSlowclk_wakeupEarlier;
	uint32_t internalSlowclk_OpenWindowLonger;
	uint32_t externalSlowclk_wakeupEarlier;
	uint32_t externalSlowclk_OpenWindowLonger;
	uint32_t slowclk_sampledCycles;
	uint32_t mx_slowclk_source;
	struct conf_coex_configuration coex_configuration;
	/* Prevent HW recovery. FW will remain stuck. */
	uint8_t no_recovery;
	uint8_t disable_logger;
	uint8_t mixed_mode_support;
	uint8_t sramLdo_voltageTrimming;
	uint32_t xtal_SettlingTime_usec;
	uint8_t max_rx_ampdu_len;
	uint32_t country_code;
	struct conf_ant_diversity ant_diversity;
	struct conf_iomux_configuration iomux_configuration;
	struct conf_thermal_thresholds thermal_thresholds;
	struct conf_limit105c_params limit105c_params;
	struct conf_gpadc_params gpadc_params;
} __attribute__((__packed__));

struct cc33xx_mac_conf {
	uint8_t ps_mode;
	uint8_t ps_scheme;
	uint8_t he_enable;
	uint8_t ApMaxNumStations;
	uint8_t fw_defrag;
	uint16_t rx_memblks_override;
	uint8_t rts_mode; /* 0: dynamic rts, 1: always on, 2: always off */
} __attribute__((__packed__));

struct cc33xx_phy_conf {
	uint8_t insertion_loss_2_4GHz[2];
	uint8_t insertion_loss_5GHz[2];
	uint8_t reserved_0[2];
	uint8_t ant_gain_2_4GHz[2];
	uint8_t ant_gain_5GHz[2];
	uint8_t reserved_1[2];
	uint8_t ble_ch_lim_1M[40];
	uint8_t ble_ch_lim_2M[40];
	uint8_t one_time_calibration_only;
	uint8_t is_diplexer_present;
	uint8_t num_of_antennas;
	uint8_t reg_domain;
	uint16_t calib_period;
	int8_t tx_psat_compensation_2_4GHz;
	int8_t tx_psat_compensation_5GHz;
	uint8_t Is85cDevice;
	uint32_t gpio_data[4];
	int16_t xtalCorrAbove95C;
} __attribute__((__packed__));

struct cc33xx_host_conf {
	struct conf_tx_settings tx;
	struct conf_conn_settings conn;
	struct conf_scan_settings scan;
	struct conf_sched_scan_settings sched_scan;
	struct conf_ht_setting ht;
	struct conf_fwlog fwlog;
} __attribute__((__packed__));

struct cc33xx_crc_conf {
	uint32_t userChecksum;
} __attribute__((__packed__));

struct cc33xx_extra_conf {
	/*
	 * Used to distinguish between "product" types, like CC33xxMOD TI,
	 * or other that will come later. Needed to apply special settings,
	 * like RF params etc.
	 */
	uint16_t Product_type;
	int8_t tx_psat_compensation_6GHz; /* Missing in psat reserved caused misalignment */
} __attribute__((__packed__));

struct cc33xx_conf_file {
	struct cc33xx_conf_header header;
	struct cc33xx_phy_conf phy;
	struct cc33xx_mac_conf mac;
	struct cc33xx_core_conf core;
	struct cc33xx_host_conf host_conf;
	struct cc33xx_crc_conf crc_conf;
	struct cc33xx_extra_conf extra_conf;
} __attribute__((__packed__));

#endif /* __CONF_H__ */

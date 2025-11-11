/**
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Structures and related enumerations used in Coexistence.
 */

#ifndef __COEX_STRUCT_H__
#define __COEX_STRUCT_H__

#include <stdint.h>
#include <stdbool.h>

/* Max size of message buffer (exchanged between host and MAC). This is in "bytes" */
#define MAX_MESSAGE_BUF_SIZE 320
/* Number of elements in coex_ch_configuration other than configbuf[] */
#define NUM_ELEMENTS_EXCL_CONFIGBUF 4
/* Each configuration value is of type uint32_t */
#define MAX_NUM_CONFIG_VALUES ((MAX_MESSAGE_BUF_SIZE-\
	(NUM_ELEMENTS_EXCL_CONFIGBUF*sizeof(uint32_t)))>>2)
/* Number of elements in coex_sr_traffic_info other than sr_traffic_info[] */
#define NUM_ELEMENTS_EXCL_SRINFOBUF 1
/* Each SR Traffic Info is of type uint32_t */
#define MAX_SR_TRAFFIC_BUF_SIZE 32

enum {
	/** Used two different values for AGGREGATION module because offset from base is
	 *  beyond supported message buffer size for  WAIT_STATE_1_TIME register
	 */
	COEX_HARDWARE = 1,
	MAC_CTRL,
	MAC_CTRL_AGG_WAIT_STATE_1_TIME,
	MAC_CTRL_AGG,
	MAC_CTRL_DEAGG,
	WLAN_CTRL,
};

/* IDs of different messages posted from Coexistence Driver to Coexistence Manager */
enum {
	/* To insturct Coexistence Manager to collect and post SR traffic information */
	COLLECT_SR_TRAFFIC_INFO = 1,
	/* To insturct Coexistence Manager to allocate a priority window to SR */
	ALLOCATE_PTI_WINDOW,
	/* To do configuration of hardware related to coexistence */
	HW_CONFIGURATION,
	/* To start allocating periodic priority windows to Wi-Fi and SR */
	ALLOCATE_PPW,
	/* To start allocating virtual priority windows to Wi-Fi */
	ALLOCATE_VPW,
	/* To configure CM SW parameters */
	SW_CONFIGURATION,
	/* To control sheliak side switch */
	UPDATE_SWITCH_CONFIG
};

/* ID(s) of different messages posted from Coexistence Manager to Coexistence Driver */
enum {
	/* To post SR traffic information */
	SR_TRAFFIC_INFO = 1
};

/**
 * struct coex_collect_sr_traffic_info - Message from CD to CM  to request SR traffic info.
 * @message_id: Indicates message ID. This is to be set to COLLECT_SR_TRAFFIC_INFO.
 * @num_sets_requested: Indicates the number of sets of duration and periodicity to be collected.
 *
 * Message from CD to CM  to request SR traffic information.
 */
struct coex_collect_sr_traffic_info {
	uint32_t message_id;
	uint32_t num_sets_requested;
};

/**
 * struct coex_ch_configuration -Message from CD to CM  to configure CH.
 * @message_id: Indicates message ID. This is to be set to HW_CONFIGURATION.
 * @num_reg_to_config: Indicates the number of registers to be configured.
 * @hw_to_config: Indicates the hardware block that is to be configured.
 * @hw_block_base_addr: Base address of the hardware block to be configured.
 * @configbuf: Configuration buffer that holds packed offset and configuration value.
 *
 * Message from CD to CM  to configure CH
 */
struct coex_ch_configuration {
	uint32_t message_id;
	uint32_t num_reg_to_config;
	uint32_t hw_to_config;
	uint32_t hw_block_base_addr;
	uint32_t configbuf[MAX_NUM_CONFIG_VALUES];
};

/**
 * struct coex_allocate_pti_window - Message to CM to request a priority window.
 * @message_id: Indicates message ID. This is to be set to ALLOCATE_PTI_WINDOW.
 * @device_req_window: Indicates device requesting a priority window.
 * @window_start_or_end: Indicates if request is posted to START or END a priority window.
 * @imp_of_request: Indicates importance of activity for which the window is requested.
 * @can_be_deferred: activity of Wi-Fi/SR, for which window is requested can be deferred or not.
 *
 * Message to CM to request a priority window
 */
struct coex_allocate_pti_window {
	uint32_t message_id;
	uint32_t device_req_window;
	uint32_t window_start_or_end;
	uint32_t imp_of_request;
	uint32_t can_be_deferred;
};

/**
 * struct coex_allocate_ppw - Message from CD to CM  to allocate Periodic Priority Windows.
 * @message_id: Indicates message ID. This is to be set to ALLOCATE_PPW.
 * @start_or_stop: Indiates start or stop allocation of PPWs.
 * @first_pti_window: Indicates first priority window in the series of PPWs.
 * @ps_mechanism: Indicates recommended powersave mechanism for Wi-Fi's downlink.
 * @wifi_window_duration: Indicates duration of Wi-Fi priority window.
 * @sr_window_duration: Indicates duration of SR priority window.
 *
 * Message from CD to CM  to allocate Periodic Priority Windows.
 */
struct coex_allocate_ppw {
	uint32_t message_id;
	uint32_t start_or_stop;
	uint32_t first_pti_window;
	uint32_t ps_mechanism;
	uint32_t wifi_window_duration;
	uint32_t sr_window_duration;
};

/**
 * struct coex_allocate_vpw - Message from CD to CM  to allocate Virtual Priority Windows.
 * @message_id: Indicates message ID. This is to be set to ALLOCATE_VPW.
 * @start_or_stop: Indicates start or stop allocation of VPWs.
 * @wifi_window_duration: Indicates duration of Wi-Fi virtual priority window.
 * @ps_mechanism: Indicates recommended powersave mechanism for Wi-Fi's downlink.
 *
 * Message from CD to CM  to allocate Virtual Priority Windows.
 */
struct coex_allocate_vpw {
	uint32_t message_id;
	uint32_t start_or_stop;
	uint32_t wifi_window_duration;
	uint32_t ps_mechanism;
};

/**
 * struct coex_config_cm_params - Message from CD to CM  to configure CM parameters
 * @message_id: Indicates message ID. This is to be set to SW_CONFIGURATION.
 * @first_isr_trigger_period: microseconds . used to trigger the ISR mechanism.
 * @sr_window_poll_periodicity_vpw: microseconds. This is used to poll through SR window.
 *  that comes after Wi-Fi window ends and next SR activity starts, in the case of VPWs.
 * @lead_time_from_end_of_wlan_win: microseconds. Lead time from the end of Wi-Fi window.
 *  (to inform  AP that Wi-Fi is entering powersave) in the case of PPW and VPW generation.
 * @sr_window_poll_count_threshold: This is equal to "Wi-Fi contention timeout.
 *  threshold"/sr_window_poll_periodicity_vpw.
 *
 * Message from CD to CM  to configure CM parameters.
 */
struct coex_config_cm_params {
	uint32_t message_id;
	uint32_t first_isr_trigger_period;
	uint32_t sr_window_poll_periodicity_vpw;
	uint32_t lead_time_from_end_of_wlan_win;
	uint32_t sr_window_poll_count_threshold;
};

/**
 * struct coex_sr_traffic_info - Message from CM to CD to post SR traffic information.
 * @message_id: Indicates message ID. This is to be set to SR_TRAFFIC_INFO.
 * @sr_traffic_info: Traffic information buffer.
 *
 * Message from CM to CD to post SR traffic inforamtion
 */
struct coex_sr_traffic_info {
	uint32_t message_id;
	uint32_t sr_traffic_info[MAX_SR_TRAFFIC_BUF_SIZE];
};

#endif /* __COEX_STRUCT_H__ */

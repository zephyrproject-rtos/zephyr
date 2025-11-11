/*
 * Copyright (c) 2025, Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the TISCI driver
 *
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_TISCI_H_
#define INCLUDE_ZEPHYR_DRIVERS_TISCI_H_

#include <zephyr/device.h>

#define MAILBOX_MBOX_SIZE 60

/**
 * @struct tisci_version_info
 * @brief version information structure
 * @param abi_major:	Major ABI version. Change here implies risk of backward
 *		compatibility break.
 * @param abi_minor:	Minor ABI version. Change here implies new feature addition,
 *		or compatible change in ABI.
 * @param firmware_revision:	Firmware revision (not usually used).
 * @param firmware_description: Firmware description (not usually used).
 */
struct tisci_version_info {
	uint8_t abi_major;
	uint8_t abi_minor;
	uint16_t firmware_revision;
	char firmware_description[32];
};

/**
 * @struct tisci_msg_fwl_region_cfg
 * @brief Request and Response for firewalls settings
 *
 * @param fwl_id:		Firewall ID in question
 * @param region:		Region or channel number to set config info
 *			This field is unused in case of a simple firewall  and must be initialized
 *			to zero.  In case of a region based firewall, this field indicates the
 *			region in question. (index starting from 0) In case of a channel based
 *			firewall, this field indicates the channel in question (index starting
 *			from 0)
 * @param n_permission_regs:	Number of permission registers to set
 * @param control:		Contents of the firewall CONTROL register to set
 * @param permissions:	Contents of the firewall PERMISSION register to set
 * @param start_address:	Contents of the firewall START_ADDRESS register to set
 * @param end_address:	Contents of the firewall END_ADDRESS register to set
 */
struct tisci_msg_fwl_region {
	uint16_t fwl_id;
	uint16_t region;
	uint32_t n_permission_regs;
	uint32_t control;
	uint32_t permissions[3];
	uint64_t start_address;
	uint64_t end_address;
};

/**
 * @brief Request and Response for firewall owner change
 * @struct tisci_msg_fwl_owner
 * @param fwl_id:		Firewall ID in question
 * @param region:		Region or channel number to set config info
 *			This field is unused in case of a simple firewall  and must be initialized
 *			to zero.  In case of a region based firewall, this field indicates the
 *			region in question. (index starting from 0) In case of a channel based
 *			firewall, this field indicates the channel in question (index starting
 *			from 0)
 * @param n_permission_regs:	Number of permission registers <= 3
 * @param control:		Control register value for this region
 * @param owner_index:	New owner index to change to. Owner indexes are setup in DMSC firmware boot
 *configuration data
 * @param owner_privid:	New owner priv-id, used to lookup owner_index is not known, must be set to
 *zero otherwise
 * @param owner_permission_bits: New owner permission bits
 */
struct tisci_msg_fwl_owner {
	uint16_t fwl_id;
	uint16_t region;
	uint8_t owner_index;
	uint8_t owner_privid;
	uint16_t owner_permission_bits;
};

/**
 * Configures a Navigator Subsystem UDMAP transmit channel
 *
 * Configures a Navigator Subsystem UDMAP transmit channel registers.
 * See tisci_msg_rm_udmap_tx_ch_cfg_req
 */
struct tisci_msg_rm_udmap_tx_ch_cfg {
	uint32_t valid_params;
#define TISCI_MSG_VALUE_RM_UDMAP_CH_TX_FILT_EINFO_VALID    BIT(9)
#define TISCI_MSG_VALUE_RM_UDMAP_CH_TX_FILT_PSWORDS_VALID  BIT(10)
#define TISCI_MSG_VALUE_RM_UDMAP_CH_TX_SUPR_TDPKT_VALID    BIT(11)
#define TISCI_MSG_VALUE_RM_UDMAP_CH_TX_CREDIT_COUNT_VALID  BIT(12)
#define TISCI_MSG_VALUE_RM_UDMAP_CH_TX_FDEPTH_VALID        BIT(13)
#define TISCI_MSG_VALUE_RM_UDMAP_CH_TX_TDTYPE_VALID        BIT(15)
#define TISCI_MSG_VALUE_RM_UDMAP_CH_EXTENDED_CH_TYPE_VALID BIT(16)
	uint16_t nav_id;
	uint16_t index;
	uint8_t tx_pause_on_err;
	uint8_t tx_filt_einfo;
	uint8_t tx_filt_pswords;
	uint8_t tx_atype;
	uint8_t tx_chan_type;
	uint8_t tx_supr_tdpkt;
	uint16_t tx_fetch_size;
	uint8_t tx_credit_count;
	uint16_t txcq_qnum;
	uint8_t tx_priority;
	uint8_t tx_qos;
	uint8_t tx_orderid;
	uint16_t fdepth;
	uint8_t tx_sched_priority;
	uint8_t tx_burst_size;
	uint8_t tx_tdtype;
	uint8_t extended_ch_type;
};

/**
 * Configures a Navigator Subsystem UDMAP receive channel
 *
 * Configures a Navigator Subsystem UDMAP receive channel registers.
 * See tisci_msg_rm_udmap_rx_ch_cfg_req
 */
struct tisci_msg_rm_udmap_rx_ch_cfg {
	uint32_t valid_params;
#define TISCI_MSG_VALUE_RM_UDMAP_CH_RX_FLOWID_START_VALID BIT(9)
#define TISCI_MSG_VALUE_RM_UDMAP_CH_RX_FLOWID_CNT_VALID   BIT(10)
#define TISCI_MSG_VALUE_RM_UDMAP_CH_RX_IGNORE_SHORT_VALID BIT(11)
#define TISCI_MSG_VALUE_RM_UDMAP_CH_RX_IGNORE_LONG_VALID  BIT(12)
	uint16_t nav_id;
	uint16_t index;
	uint16_t rx_fetch_size;
	uint16_t rxcq_qnum;
	uint8_t rx_priority;
	uint8_t rx_qos;
	uint8_t rx_orderid;
	uint8_t rx_sched_priority;
	uint16_t flowid_start;
	uint16_t flowid_cnt;
	uint8_t rx_pause_on_err;
	uint8_t rx_atype;
	uint8_t rx_chan_type;
	uint8_t rx_ignore_short;
	uint8_t rx_ignore_long;
	uint8_t rx_burst_size;
};

#define TISCI_MSG_VALUE_RM_DST_ID_VALID                (1u << 0u)
#define TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID          (1u << 1u)
#define TISCI_MSG_VALUE_RM_IA_ID_VALID                 (1u << 2u)
#define TISCI_MSG_VALUE_RM_VINT_VALID                  (1u << 3u)
#define TISCI_MSG_VALUE_RM_GLOBAL_EVENT_VALID          (1u << 4u)
#define TISCI_MSG_VALUE_RM_VINT_STATUS_BIT_INDEX_VALID (1u << 5u)

/**
 * @brief Request to set up an interrupt route.
 *
 * Configures peripherals within the interrupt subsystem according to the
 * valid configuration provided.
 *
 * @param valid_params         Bitfield defining validity of interrupt route set parameters.
 *                             Each bit corresponds to a field's validity.
 * @param src_id               ID of interrupt source peripheral.
 * @param src_index            Interrupt source index within source peripheral.
 * @param dst_id               SoC IR device ID (valid if TISCI_MSG_VALUE_RM_DST_ID_VALID is set).
 * @param dst_host_irq         SoC IR output index (valid if TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID
 * is set).
 * @param ia_id                Device ID of interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_IA_ID_VALID is set).
 * @param vint                 Virtual interrupt number (valid if TISCI_MSG_VALUE_RM_VINT_VALID is
 * set).
 * @param global_event         Global event mapped to interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_GLOBAL_EVENT_VALID is set).
 * @param vint_status_bit_index Virtual interrupt status bit (valid if
 * TISCI_MSG_VALUE_RM_VINT_STATUS_BIT_INDEX_VALID is set).
 * @param secondary_host       Secondary host value (valid if
 * TISCI_MSG_VALUE_RM_SECONDARY_HOST_VALID is set).
 */
struct tisci_irq_set_req {
	uint32_t valid_params;
	uint16_t src_id;
	uint16_t src_index;
	uint16_t dst_id;
	uint16_t dst_host_irq;
	uint16_t ia_id;
	uint16_t vint;
	uint16_t global_event;
	uint8_t vint_status_bit_index;
	uint8_t secondary_host;
};

/**
 * @brief Request to release interrupt peripheral resources.
 *
 * Releases interrupt peripheral resources according to the valid configuration provided.
 *
 * @param valid_params         Bitfield defining validity of interrupt route release parameters.
 *                             Each bit corresponds to a field's validity.
 * @param src_id               ID of interrupt source peripheral.
 * @param src_index            Interrupt source index within source peripheral.
 * @param dst_id               SoC IR device ID (valid if TISCI_MSG_VALUE_RM_DST_ID_VALID is set).
 * @param dst_host_irq         SoC IR output index (valid if TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID
 * is set).
 * @param ia_id                Device ID of interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_IA_ID_VALID is set).
 * @param vint                 Virtual interrupt number (valid if TISCI_MSG_VALUE_RM_VINT_VALID is
 * set).
 * @param global_event         Global event mapped to interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_GLOBAL_EVENT_VALID is set).
 * @param vint_status_bit_index Virtual interrupt status bit (valid if
 * TISCI_MSG_VALUE_RM_VINT_STATUS_BIT_INDEX_VALID is set).
 * @param secondary_host       Secondary host value (valid if
 * TISCI_MSG_VALUE_RM_SECONDARY_HOST_VALID is set).
 */

struct tisci_irq_release_req {
	uint32_t valid_params;
	uint16_t src_id;
	uint16_t src_index;
	uint16_t dst_id;
	uint16_t dst_host_irq;
	uint16_t ia_id;
	uint16_t vint;
	uint16_t global_event;
	uint8_t vint_status_bit_index;
	uint8_t secondary_host;
};

/* Version/Revision Functions */

/**
 * @brief Get the revision information of the TI SCI firmware
 *
 * Queries the TI SCI firmware for its version and revision information.
 * The retrieved information is stored in the provided @p ver structure.
 *
 * @param dev Pointer to the TI SCI device
 * @param ver Pointer to a structure where the firmware version information will be stored
 *
 * @return 0 if successful, or a negative error code on failure
 */
int tisci_cmd_get_revision(const struct device *dev, struct tisci_version_info *ver);

/* Clock Management Functions */

/**
 * @brief Get the state of a clock
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param programmed_state Pointer to store the requested state of the clock
 * @param current_state Pointer to store the current state of the clock
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_get_clock_state(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
			      uint8_t *programmed_state, uint8_t *current_state);

/**
 * @brief Set the state of a clock
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param flags Header flags as needed
 * @param state State to request for the clock
 *
 * @return 0 if successful, or an error code
 */
int tisci_set_clock_state(const struct device *dev, uint32_t dev_id, uint8_t clk_id, uint32_t flags,
			  uint8_t state);

/**
 * @brief Check if the clock is ON
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param req_state Pointer to store whether the clock is managed and enabled
 * @param curr_state Pointer to store whether the clock is ready for operation
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_clk_is_on(const struct device *dev, uint32_t dev_id, uint8_t clk_id, bool *req_state,
			bool *curr_state);

/**
 * @brief Check if the clock is OFF
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param req_state Pointer to store whether the clock is managed and disabled
 * @param curr_state Pointer to store whether the clock is NOT ready for operation
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_clk_is_off(const struct device *dev, uint32_t dev_id, uint8_t clk_id, bool *req_state,
			 bool *curr_state);

/**
 * @brief Check if the clock is being auto-managed
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param req_state Pointer to store whether the clock is auto-managed
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_clk_is_auto(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
			  bool *req_state);

/**
 * @brief Get the current frequency of a clock
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param freq Pointer to store the current frequency in Hz
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_clk_get_freq(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
			   uint64_t *freq);

/**
 * @brief Set a frequency for a clock
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param min_freq Minimum allowable frequency in Hz
 * @param target_freq Target clock frequency in Hz
 * @param max_freq Maximum allowable frequency in Hz
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_clk_set_freq(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
			   uint64_t min_freq, uint64_t target_freq, uint64_t max_freq);

/**
 * @brief Get a matching frequency for a clock
 *
 * Finds a frequency that matches the requested range for a clock.
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param min_freq Minimum allowable frequency in Hz
 * @param target_freq Target clock frequency in Hz
 * @param max_freq Maximum allowable frequency in Hz
 * @param match_freq Pointer to store the matched frequency in Hz
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_clk_get_match_freq(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
				 uint64_t min_freq, uint64_t target_freq, uint64_t max_freq,
				 uint64_t *match_freq);

/**
 * @brief Set the parent clock for a clock
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param parent_id Identifier of the parent clock to set
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_clk_set_parent(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
			     uint8_t parent_id);

/**
 * @brief Get the parent clock for a clock
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param parent_id Pointer to store the identifier of the parent clock
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_clk_get_parent(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
			     uint8_t *parent_id);

/**
 * @brief Get the number of parent clocks for a clock
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param num_parents Pointer to store the number of parent clocks
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_clk_get_num_parents(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
				  uint8_t *num_parents);

/**
 * @brief Get control of a clock from TI SCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 * @param needs_ssc 'true' if Spread Spectrum clock is desired, else 'false'
 * @param can_change_freq 'true' if frequency change is desired, else 'false'
 * @param enable_input_term 'true' if input termination is desired, else 'false'
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_get_clock(const struct device *dev, uint32_t dev_id, uint8_t clk_id, bool needs_ssc,
			bool can_change_freq, bool enable_input_term);

/**
 * @brief Idle a clock that is under control of TI SCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_idle_clock(const struct device *dev, uint32_t dev_id, uint8_t clk_id);

/**
 * @brief Release a clock from control back to TI SCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_put_clock(const struct device *dev, uint32_t dev_id, uint8_t clk_id);

/* Device Management Functions */

/**
 * @brief Set the state of a device
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param flags Flags to set for the device
 * @param state State to move the device to:
 *              - 0: Device is off
 *              - 1: Device is on
 *              - 2: Device is in retention
 *              - 3: Device is in reset
 *
 * @return 0 if successful, or an error code
 */
int tisci_set_device_state(const struct device *dev, uint32_t dev_id, uint32_t flags,
			   uint8_t state);

/**
 * @brief Set the state of a device without waiting for a response
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param flags Flags to set for the device
 * @param state State to move the device to:
 *              - 0: Device is off
 *              - 1: Device is on
 *              - 2: Device is in retention
 *              - 3: Device is in reset
 *
 * @return 0 if successful, or an error code
 */
int tisci_set_device_state_no_wait(const struct device *dev, uint32_t dev_id, uint32_t flags,
				   uint8_t state);

/**
 * @brief Get the state of a device
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clcnt Pointer to store the Context Loss Count
 * @param resets Pointer to store the reset count
 * @param p_state Pointer to store the programmed state
 * @param c_state Pointer to store the current state
 *
 * @return 0 if successful, or an error code
 */
int tisci_get_device_state(const struct device *dev, uint32_t dev_id, uint32_t *clcnt,
			   uint32_t *resets, uint8_t *p_state, uint8_t *c_state);

/**
 * @brief Request exclusive access to a device managed by TISCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_get_device(const struct device *dev, uint32_t dev_id);
int tisci_cmd_get_device_exclusive(const struct device *dev, uint32_t dev_id);

/**
 * @brief Command to idle a device managed by TISCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_idle_device(const struct device *dev, uint32_t dev_id);
int tisci_cmd_idle_device_exclusive(const struct device *dev, uint32_t dev_id);

/**
 * @brief Command to release a device managed by TISCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_put_device(const struct device *dev, uint32_t dev_id);

/**
 * @brief Check if a device ID is valid
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 *
 * @return 0 if the device ID is valid, or an error code
 */
int tisci_cmd_dev_is_valid(const struct device *dev, uint32_t dev_id);

/**
 * @brief Get the context loss counter for a device
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param count Pointer to store the context loss counter
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_dev_get_clcnt(const struct device *dev, uint32_t dev_id, uint32_t *count);

/**
 * @brief Check if the device is requested to be idle
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param r_state Pointer to store the result (true if requested to be idle)
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_dev_is_idle(const struct device *dev, uint32_t dev_id, bool *r_state);

/**
 * @brief Check if the device is requested to be stopped
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param r_state Pointer to store the result (true if requested to be stopped)
 * @param curr_state Pointer to store the result (true if currently stopped)
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_dev_is_stop(const struct device *dev, uint32_t dev_id, bool *r_state,
			  bool *curr_state);

/**
 * @brief Check if the device is requested to be ON
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param r_state Pointer to store the result (true if requested to be ON)
 * @param curr_state Pointer to store the result (true if currently ON and active)
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_dev_is_on(const struct device *dev, uint32_t dev_id, bool *r_state, bool *curr_state);

/**
 * @brief Check if the device is currently transitioning
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param curr_state Pointer to store the result (true if currently transitioning)
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_dev_is_trans(const struct device *dev, uint32_t dev_id, bool *curr_state);

/**
 * @brief Set resets for a device managed by TISCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param reset_state Device-specific reset bit field
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_set_device_resets(const struct device *dev, uint32_t dev_id, uint32_t reset_state);

/**
 * @brief Get reset state for a device managed by TISCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param reset_state Pointer to store the reset state
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_get_device_resets(const struct device *dev, uint32_t dev_id, uint32_t *reset_state);

/* Resource Management Functions */

/**
 * @brief Get a range of resources assigned to a host
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id TISCI device ID
 * @param subtype Resource assignment subtype being requested
 * @param s_host Host processor ID to which the resources are allocated
 * @param range_start Pointer to store the start index of the resource range
 * @param range_num Pointer to store the number of resources in the range
 *
 * @return 0 if successful, or an error code
 */
int tisci_get_resource_range(const struct device *dev, uint32_t dev_id, uint8_t subtype,
			     uint8_t s_host, uint16_t *range_start, uint16_t *range_num);

/**
 * @brief Get a range of resources assigned to the host
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id TISCI device ID
 * @param subtype Resource assignment subtype being requested
 * @param range_start Pointer to store the start index of the resource range
 * @param range_num Pointer to store the number of resources in the range
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_get_resource_range(const struct device *dev, uint32_t dev_id, uint8_t subtype,
				 uint16_t *range_start, uint16_t *range_num);

/**
 * @brief Get a range of resources assigned to a specified host
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id TISCI device ID
 * @param subtype Resource assignment subtype being requested
 * @param s_host Host processor ID to which the resources are allocated
 * @param range_start Pointer to store the start index of the resource range
 * @param range_num Pointer to store the number of resources in the range
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_get_resource_range_from_shost(const struct device *dev, uint32_t dev_id,
					    uint8_t subtype, uint8_t s_host, uint16_t *range_start,
					    uint16_t *range_num);

/* Processor Management Functions */

/**
 * @brief Command to request a physical processor control
 *
 * @param dev Pointer to the TI SCI device
 * @param proc_id Processor ID this request is for
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_proc_request(const struct device *dev, uint8_t proc_id);

/**
 * @brief Command to release a physical processor control
 *
 * @param dev Pointer to the TI SCI device
 * @param proc_id Processor ID this request is for
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_proc_release(const struct device *dev, uint8_t proc_id);

/**
 * @brief Command to handover a physical processor control to a host
 *        in the processor's access control list
 *
 * @param dev Pointer to the TI SCI device
 * @param proc_id Processor ID this request is for
 * @param host_id Host ID to get the control of the processor
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_proc_handover(const struct device *dev, uint8_t proc_id, uint8_t host_id);

/**
 * @brief Command to set the processor boot configuration flags
 *
 * @param dev Pointer to the TI SCI device
 * @param proc_id Processor ID this request is for
 * @param bootvector Boot vector address
 * @param config_flags_set Configuration flags to be set
 * @param config_flags_clear Configuration flags to be cleared
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_set_proc_boot_cfg(const struct device *dev, uint8_t proc_id, uint64_t bootvector,
				uint32_t config_flags_set, uint32_t config_flags_clear);

/**
 * @brief Command to set the processor boot control flags
 *
 * @param dev Pointer to the TI SCI device
 * @param proc_id Processor ID this request is for
 * @param control_flags_set Control flags to be set
 * @param control_flags_clear Control flags to be cleared
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_set_proc_boot_ctrl(const struct device *dev, uint8_t proc_id,
				 uint32_t control_flags_set, uint32_t control_flags_clear);

/**
 * @brief Command to authenticate and load the image, then set the processor configuration flags
 *
 * @param dev Pointer to the TI SCI device
 * @param image_addr Pointer to the memory address of the payload image and certificate
 * @param image_size Pointer to the size of the image after authentication
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_proc_auth_boot_image(const struct device *dev, uint64_t *image_addr,
				   uint32_t *image_size);

/**
 * @brief Command to get the processor boot status
 *
 * @param dev Pointer to the TI SCI device
 * @param proc_id Processor ID this request is for
 * @param bv Pointer to store the boot vector
 * @param cfg_flags Pointer to store the configuration flags
 * @param ctrl_flags Pointer to store the control flags
 * @param sts_flags Pointer to store the status flags
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_get_proc_boot_status(const struct device *dev, uint8_t proc_id, uint64_t *bv,
				   uint32_t *cfg_flags, uint32_t *ctrl_flags, uint32_t *sts_flags);

/**
 * @brief Helper function to wait for a processor boot status without requesting or waiting for a
 * response
 *
 * @param dev Pointer to the TI SCI device
 * @param proc_id Processor ID this request is for
 * @param num_wait_iterations Total number of iterations to check before timeout
 * @param num_match_iterations Number of consecutive matches required to confirm status
 * @param delay_per_iteration_us Delay in microseconds between each status check
 * @param delay_before_iterations_us Delay in microseconds before the first status check
 * @param status_flags_1_set_all_wait Flags that must all be set to 1
 * @param status_flags_1_set_any_wait Flags where at least one must be set to 1
 * @param status_flags_1_clr_all_wait Flags that must all be cleared to 0
 * @param status_flags_1_clr_any_wait Flags where at least one must be cleared to 0
 *
 * @return 0 if successful, or an error code
 */
int tisci_proc_wait_boot_status_no_wait(const struct device *dev, uint8_t proc_id,
					uint8_t num_wait_iterations, uint8_t num_match_iterations,
					uint8_t delay_per_iteration_us,
					uint8_t delay_before_iterations_us,
					uint32_t status_flags_1_set_all_wait,
					uint32_t status_flags_1_set_any_wait,
					uint32_t status_flags_1_clr_all_wait,
					uint32_t status_flags_1_clr_any_wait);

/**
 * @brief Command to shutdown a core without requesting or waiting for a response
 *
 * @param dev Pointer to the TI SCI device
 * @param proc_id Processor ID this request is for
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_proc_shutdown_no_wait(const struct device *dev, uint8_t proc_id);

/* Board Configuration Functions */

/**
 * @brief Set board configuration using a specified message type
 *
 * Sends a board configuration message to the TI SCI firmware with configuration
 * data from a specified memory location.
 *
 * @param dev Pointer to the TI SCI device
 * @param msg_type TISCI message type for board configuration
 * @param addr Physical address of board configuration data
 * @param size Size of board configuration data in bytes
 *
 * @return 0 if successful, or an error code
 */
int cmd_set_board_config_using_msg(const struct device *dev, uint16_t msg_type, uint64_t addr,
				   uint32_t size);

/* Ring Configuration Function */

/**
 * @brief Configure a RA ring
 *
 * @param dev Pointer to the TI SCI device
 * @param valid_params Bitfield defining validity of ring configuration parameters
 * @param nav_id Device ID of Navigator Subsystem from which the ring is allocated
 * @param index Ring index
 * @param addr_lo The ring base address low 32 bits
 * @param addr_hi The ring base address high 32 bits
 * @param count Number of ring elements
 * @param mode The mode of the ring
 * @param size The ring element size
 * @param order_id Specifies the ring's bus order ID
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_ring_config(const struct device *dev, uint32_t valid_params, uint16_t nav_id,
			  uint16_t index, uint32_t addr_lo, uint32_t addr_hi, uint32_t count,
			  uint8_t mode, uint8_t size, uint8_t order_id);

/* System Control Functions */

/**
 * @brief Request a system reset
 *
 * Commands the TI SCI firmware to perform a system reset.
 *
 * @param dev Pointer to the TI SCI device
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_sys_reset(const struct device *dev);

/* Memory Management Functions */

/**
 * @brief Query the available MSMC memory range
 *
 * Queries the TI SCI firmware for the currently available MSMC (Multi-Standard
 * Shared Memory Controller) memory range.
 *
 * @param dev Pointer to the TI SCI device
 * @param msmc_start Pointer to store the MSMC start address
 * @param msmc_end Pointer to store the MSMC end address
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_query_msmc(const struct device *dev, uint64_t *msmc_start, uint64_t *msmc_end);

/* Firewall Management Functions */

/**
 * @brief Configure a firewall region
 *
 * Sets up a firewall region with the specified configuration parameters
 * including permissions, addresses, and control settings.
 *
 * @param dev Pointer to the TI SCI device
 * @param region Pointer to the firewall region configuration parameters
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_set_fwl_region(const struct device *dev, const struct tisci_msg_fwl_region *region);

/* INCLUDE_ZEPHYR_DRIVERS_TISCI_H_ */

/* Firewall Management Functions */
/* ... previous firewall functions ... */

/**
 * @brief Get firewall region configuration
 *
 * Retrieves the configuration of a firewall region including permissions,
 * addresses, and control settings.
 *
 * @param dev Pointer to the TI SCI device
 * @param region Pointer to store the firewall region configuration.
 *              The fwl_id, region, and n_permission_regs fields must be
 *              set before calling this function.
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_get_fwl_region(const struct device *dev, struct tisci_msg_fwl_region *region);

/* Firewall Management Functions */
/* ... previous firewall functions ... */

/**
 * @brief Change firewall region owner
 *
 * Changes the ownership of a firewall region and retrieves updated
 * ownership information.
 *
 * @param dev Pointer to the TI SCI device
 * @param owner Pointer to firewall owner configuration.
 *             On input: contains fwl_id, region, and owner_index
 *             On output: contains updated ownership information
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_change_fwl_owner(const struct device *dev, struct tisci_msg_fwl_owner *owner);

/* UDMAP Management Functions */

/**
 * @brief Configure a UDMAP transmit channel
 *
 * Configures the non-real-time registers of a Navigator Subsystem UDMAP
 * transmit channel.
 *
 * @param dev Pointer to the TI SCI device
 * @param params Pointer to the transmit channel configuration parameters
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_rm_udmap_tx_ch_cfg(const struct device *dev,
				 const struct tisci_msg_rm_udmap_tx_ch_cfg *params);

/**
 * @brief Configure a UDMAP receive channel
 *
 * Configures the non-real-time registers of a Navigator Subsystem UDMAP
 * receive channel.
 *
 * @param dev Pointer to the TI SCI device
 * @param params Pointer to the receive channel configuration parameters
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_rm_udmap_rx_ch_cfg(const struct device *dev,
				 const struct tisci_msg_rm_udmap_rx_ch_cfg *params);

/* PSI-L Management Functions */

/**
 * @brief Pair PSI-L source thread to destination thread
 *
 * Pairs a PSI-L source thread to a destination thread in the
 * Navigator Subsystem.
 *
 * @param dev Pointer to the TI SCI device
 * @param nav_id Navigator Subsystem device ID
 * @param src_thread Source thread ID
 * @param dst_thread Destination thread ID
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_rm_psil_pair(const struct device *dev, uint32_t nav_id, uint32_t src_thread,
			   uint32_t dst_thread);

/**
 * @brief Unpair PSI-L source thread from destination thread
 *
 * Unpairs a PSI-L source thread from a destination thread in the
 * Navigator Subsystem.
 *
 * @param dev Pointer to the TI SCI device
 * @param nav_id Navigator Subsystem device ID
 * @param src_thread Source thread ID
 * @param dst_thread Destination thread ID
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_rm_psil_unpair(const struct device *dev, uint32_t nav_id, uint32_t src_thread,
			     uint32_t dst_thread);

/**
 * @brief Set a Navigator Subsystem IRQ
 *
 * Sets up an interrupt route in the Navigator Subsystem using the provided request structure.
 *
 * @param dev Pointer to the TI SCI device
 * @param req Pointer to the IRQ set request structure
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_rm_irq_set(const struct device *dev, struct tisci_irq_set_req *req);

/**
 * @brief Release a Navigator Subsystem IRQ
 *
 * Releases an interrupt route in the Navigator Subsystem using the provided request structure.
 *
 * @param dev Pointer to the TI SCI device
 * @param req Pointer to the IRQ release request structure
 *
 * @return 0 if successful, or an error code
 */
int tisci_cmd_rm_irq_release(const struct device *dev, struct tisci_irq_release_req *req);
#endif

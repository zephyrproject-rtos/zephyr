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
#include <zephyr/drivers/firmware/tisci/tisci_protocol.h>

/* Version/Revision Functions */

/**
 * @brief Get the revision of the SCI entity
 *
 * Updates the SCI information in the internal data structure.
 *
 * @param dev Pointer to the TI SCI device
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_get_revision(const struct device *dev);

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
int ti_sci_cmd_get_clock_state(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
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
int ti_sci_set_clock_state(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
			   uint32_t flags, uint8_t state);

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
int ti_sci_cmd_clk_is_on(const struct device *dev, uint32_t dev_id, uint8_t clk_id, bool *req_state,
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
int ti_sci_cmd_clk_is_off(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
			  bool *req_state, bool *curr_state);

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
int ti_sci_cmd_clk_is_auto(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
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
int ti_sci_cmd_clk_get_freq(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
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
int ti_sci_cmd_clk_set_freq(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
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
int ti_sci_cmd_clk_get_match_freq(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
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
int ti_sci_cmd_clk_set_parent(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
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
int ti_sci_cmd_clk_get_parent(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
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
int ti_sci_cmd_clk_get_num_parents(const struct device *dev, uint32_t dev_id, uint8_t clk_id,
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
int ti_sci_cmd_get_clock(const struct device *dev, uint32_t dev_id, uint8_t clk_id, bool needs_ssc,
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
int ti_sci_cmd_idle_clock(const struct device *dev, uint32_t dev_id, uint8_t clk_id);

/**
 * @brief Release a clock from control back to TI SCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param clk_id Clock identifier for the device for this request
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_put_clock(const struct device *dev, uint32_t dev_id, uint8_t clk_id);

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
int ti_sci_set_device_state(const struct device *dev, uint32_t dev_id, uint32_t flags,
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
int ti_sci_set_device_state_no_wait(const struct device *dev, uint32_t dev_id, uint32_t flags,
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
int ti_sci_get_device_state(const struct device *dev, uint32_t dev_id, uint32_t *clcnt,
			    uint32_t *resets, uint8_t *p_state, uint8_t *c_state);

/**
 * @brief Request exclusive access to a device managed by TISCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_get_device(const struct device *dev, uint32_t dev_id);
int ti_sci_cmd_get_device_exclusive(const struct device *dev, uint32_t dev_id);

/**
 * @brief Command to idle a device managed by TISCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_idle_device(const struct device *dev, uint32_t dev_id);
int ti_sci_cmd_idle_device_exclusive(const struct device *dev, uint32_t dev_id);

/**
 * @brief Command to release a device managed by TISCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_put_device(const struct device *dev, uint32_t dev_id);

/**
 * @brief Check if a device ID is valid
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 *
 * @return 0 if the device ID is valid, or an error code
 */
int ti_sci_cmd_dev_is_valid(const struct device *dev, uint32_t dev_id);

/**
 * @brief Get the context loss counter for a device
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param count Pointer to store the context loss counter
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_dev_get_clcnt(const struct device *dev, uint32_t dev_id, uint32_t *count);

/**
 * @brief Check if the device is requested to be idle
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param r_state Pointer to store the result (true if requested to be idle)
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_dev_is_idle(const struct device *dev, uint32_t dev_id, bool *r_state);

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
int ti_sci_cmd_dev_is_stop(const struct device *dev, uint32_t dev_id, bool *r_state,
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
int ti_sci_cmd_dev_is_on(const struct device *dev, uint32_t dev_id, bool *r_state,
			 bool *curr_state);

/**
 * @brief Check if the device is currently transitioning
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param curr_state Pointer to store the result (true if currently transitioning)
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_dev_is_trans(const struct device *dev, uint32_t dev_id, bool *curr_state);

/**
 * @brief Set resets for a device managed by TISCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param reset_state Device-specific reset bit field
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_set_device_resets(const struct device *dev, uint32_t dev_id, uint32_t reset_state);

/**
 * @brief Get reset state for a device managed by TISCI
 *
 * @param dev Pointer to the TI SCI device
 * @param dev_id Device identifier for this request
 * @param reset_state Pointer to store the reset state
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_get_device_resets(const struct device *dev, uint32_t dev_id, uint32_t *reset_state);

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
int ti_sci_get_resource_range(const struct device *dev, uint32_t dev_id, uint8_t subtype,
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
int ti_sci_cmd_get_resource_range(const struct device *dev, uint32_t dev_id, uint8_t subtype,
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
int ti_sci_cmd_get_resource_range_from_shost(const struct device *dev, uint32_t dev_id,
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
int ti_sci_cmd_proc_request(const struct device *dev, uint8_t proc_id);

/**
 * @brief Command to release a physical processor control
 *
 * @param dev Pointer to the TI SCI device
 * @param proc_id Processor ID this request is for
 *
 * @return 0 if successful, or an error code
 */
int ti_sci_cmd_proc_release(const struct device *dev, uint8_t proc_id);

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
int ti_sci_cmd_proc_handover(const struct device *dev, uint8_t proc_id, uint8_t host_id);

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
int ti_sci_cmd_set_proc_boot_cfg(const struct device *dev, uint8_t proc_id, uint64_t bootvector,
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
int ti_sci_cmd_set_proc_boot_ctrl(const struct device *dev, uint8_t proc_id,
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
int ti_sci_cmd_proc_auth_boot_image(const struct device *dev, uint64_t *image_addr,
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
int ti_sci_cmd_get_proc_boot_status(const struct device *dev, uint8_t proc_id, uint64_t *bv,
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
int ti_sci_proc_wait_boot_status_no_wait(const struct device *dev, uint8_t proc_id,
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
int ti_sci_cmd_proc_shutdown_no_wait(const struct device *dev, uint8_t proc_id);

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
int ti_sci_cmd_ring_config(const struct device *dev, uint32_t valid_params, uint16_t nav_id,
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
int ti_sci_cmd_sys_reset(const struct device *dev);

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
int ti_sci_cmd_query_msmc(const struct device *dev, uint64_t *msmc_start, uint64_t *msmc_end);

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
int ti_sci_cmd_set_fwl_region(const struct device *dev, const struct ti_sci_msg_fwl_region *region);

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
int ti_sci_cmd_get_fwl_region(const struct device *dev, struct ti_sci_msg_fwl_region *region);

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
int ti_sci_cmd_get_fwl_region(const struct device *dev, struct ti_sci_msg_fwl_region *region);

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
int ti_sci_cmd_get_fwl_region(const struct device *dev, struct ti_sci_msg_fwl_region *region);

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
int ti_sci_cmd_change_fwl_owner(const struct device *dev, struct ti_sci_msg_fwl_owner *owner);

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
int ti_sci_cmd_rm_udmap_tx_ch_cfg(const struct device *dev,
				  const struct ti_sci_msg_rm_udmap_tx_ch_cfg *params);

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
int ti_sci_cmd_rm_udmap_rx_ch_cfg(const struct device *dev,
				  const struct ti_sci_msg_rm_udmap_rx_ch_cfg *params);

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
int ti_sci_cmd_rm_psil_pair(const struct device *dev, uint32_t nav_id, uint32_t src_thread,
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
int ti_sci_cmd_rm_psil_unpair(const struct device *dev, uint32_t nav_id, uint32_t src_thread,
			      uint32_t dst_thread);

#endif

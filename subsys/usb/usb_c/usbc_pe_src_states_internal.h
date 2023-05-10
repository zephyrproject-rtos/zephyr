/*
 * Copyright (c) 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_PE_SRC_STATES_INTERNAL_H_
#define ZEPHYR_SUBSYS_USBC_PE_SRC_STATES_INTERNAL_H_

/**
 * @brief Init the PE Source State machine
 */
void pe_src_init(const struct device *dev);

/**
 * @brief Handle Source-specific DPM requests
 */
bool source_dpm_requests(const struct device *dev);

/**
 * @brief PE_SRC_Startup State
 */
void pe_src_startup_entry(void *obj);
void pe_src_startup_run(void *obj);
void pe_src_startup_exit(void *obj);

/**
 * @brief PE_SRC_Discovery State
 */
void pe_src_discovery_entry(void *obj);
void pe_src_discovery_run(void *obj);
void pe_src_discovery_exit(void *obj);

/**
 * @brief PE_SRC_Send_Capabilities State
 */
void pe_src_send_capabilities_entry(void *obj);
void pe_src_send_capabilities_run(void *obj);
void pe_src_send_capabilities_exit(void *obj);

/**
 * @brief PE_SRC_Negotiate_Capability State
 */
void pe_src_negotiate_capability_entry(void *obj);

/**
 * @brief PE_SRC_Transition_Supply State
 */
void pe_src_transition_supply_entry(void *obj);
void pe_src_transition_supply_run(void *obj);
void pe_src_transition_supply_exit(void *obj);

/**
 * @brief PE_SRC_Ready State
 */
void pe_src_ready_entry(void *obj);
void pe_src_ready_run(void *obj);
void pe_src_ready_exit(void *obj);

/**
 * @brief PE_SRC_Transition_To_Default State
 */
void pe_src_transition_to_default_entry(void *obj);
void pe_src_transition_to_default_run(void *obj);
void pe_src_transition_to_default_exit(void *obj);

/**
 * @brief PE_SRC_Hard_Reset State
 */
void pe_src_hard_reset_entry(void *obj);
void pe_src_hard_reset_run(void *obj);
void pe_src_hard_reset_exit(void *obj);

/**
 * @brief PE_SRC_Capability_Response State
 */
void pe_src_capability_response_entry(void *obj);
void pe_src_capability_response_run(void *obj);
void pe_src_capability_response_exit(void *obj);

/**
 * @brief PE_SRC_Wait_New_Capabilities State
 */
void pe_src_wait_new_capabilities_entry(void *obj);
void pe_src_wait_new_capabilities_run(void *obj);
void pe_src_wait_new_capabilities_exit(void *obj);

/**
 * @brief PE_SRC_Hard_Reset_Parent State
 *
 *	  Parent state of PE_SRC_Hard_Reset and
 *	  PE_SRC_Hard_Reset_Received States
 */
void pe_src_hard_reset_parent_entry(void *obj);
void pe_src_hard_reset_parent_run(void *obj);
void pe_src_hard_reset_parent_exit(void *obj);

#endif /* ZEPHYR_SUBSYS_USBC_PE_SRC_STATES_INTERNAL_H_ */

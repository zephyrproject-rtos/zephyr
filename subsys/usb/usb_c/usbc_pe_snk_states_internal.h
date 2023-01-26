/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_PE_SNK_STATES_INTERNAL_H_
#define ZEPHYR_SUBSYS_USBC_PE_SNK_STATES_INTERNAL_H_

/**
 * @brief Init the PE Sink State machine
 */
void pe_snk_init(const struct device *dev);

/**
 * @brief Handle Sink-specific DPM requests
 */
bool sink_dpm_requests(const struct device *dev);

/**
 * @brief PE_SNK_Startup Entry State
 */
void pe_snk_startup_entry(void *obj);
void pe_snk_startup_run(void *obj);
void pe_snk_discovery_entry(void *obj);

/**
 * @brief PE_SNK_Discovery Run State
 */
void pe_snk_discovery_run(void *obj);

/**
 * @brief PE_SNK_Wait_For_Capabilities
 */
void pe_snk_wait_for_capabilities_entry(void *obj);
void pe_snk_wait_for_capabilities_run(void *obj);
void pe_snk_wait_for_capabilities_exit(void *obj);

/**
 * @brief PE_SNK_Evaluate_Capability Entry State
 */
void pe_snk_evaluate_capability_entry(void *obj);

/**
 * @brief PE_SNK_Select_Capability
 */
void pe_snk_select_capability_entry(void *obj);
void pe_snk_select_capability_run(void *obj);
void pe_snk_select_capability_exit(void *obj);

/**
 * @brief PE_SNK_Transition_Sink Entry State
 */
void pe_snk_transition_sink_entry(void *obj);
void pe_snk_transition_sink_run(void *obj);
void pe_snk_transition_sink_exit(void *obj);

/**
 * @brief PE_SNK_Ready Entry State
 */
void pe_snk_ready_entry(void *obj);
void pe_snk_ready_run(void *obj);

/**
 * @brief PE_SNK_Hard_Reset
 */
void pe_snk_hard_reset_entry(void *obj);
void pe_snk_hard_reset_run(void *obj);

/**
 * @brief PE_SNK_Transition_to_default Entry State
 */
void pe_snk_transition_to_default_entry(void *obj);
void pe_snk_transition_to_default_run(void *obj);

/**
 * @brief PE_SNK_Get_Source_Cap Entry State
 */
void pe_snk_get_source_cap_entry(void *obj);
void pe_snk_get_source_cap_run(void *obj);
void pe_snk_get_source_cap_exit(void *obj);

/**
 * @brief PE_SNK_Give_Sink_Cap Entry state
 */
void pe_snk_give_sink_cap_entry(void *obj);
void pe_snk_give_sink_cap_run(void *obj);

#endif /* ZEPHYR_SUBSYS_USBC_PE_SNK_STATES_INTERNAL_H_ */

/*
 * Copyright (c) 2024 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_ARM_SCMI_BASE_H_
#define DRIVERS_ARM_SCMI_BASE_H_

#define SCMI_PROTOCOL_BASE_REV_MAJOR 0x2

/**
 * @brief SCMI base protocol revision information
 */
struct scmi_revision_info {
	/** Major ABI version. */
	uint16_t major_ver;
	/** Minor ABI version. */
	uint16_t minor_ver;
#if defined(CONFIG_ARM_SCMI_BASE_EXT_REV) || defined(__DOXYGEN__)
	/** Number of protocols that are implemented, excluding the base protocol. */
	uint8_t num_protocols;
	/** Number of agents in the system. */
	uint8_t num_agents;
	/** A vendor-specific implementation version. */
	uint32_t impl_ver;
	/** A vendor identifier. */
	char vendor_id[SCMI_SHORT_NAME_MAX_SIZE];
	/** A sub-vendor identifier. */
	char sub_vendor_id[SCMI_SHORT_NAME_MAX_SIZE];
#endif /* CONFIG_ARM_SCMI_BASE_EXT_REV) || __DOXYGEN__ */
};

/**
 * @brief SCMI base protocol agent info
 *
 * @agent_id: SCMI agent id.
 * @name: SCMI agent name.
 */

#define SCMI_BASE_AGENT_ID_OWN 0xFFFFFFFF

struct scmi_agent_info {
	uint32_t agent_id;
	char name[SCMI_SHORT_NAME_MAX_SIZE];
};

/**
 * @brief SCMI base protocol get revision information.
 *
 * @param rev pointer on revision information struct scmi_revision_info.
 * @retval 0 If successful.
 */
int scmi_base_get_revision_info(struct scmi_revision_info *rev);

/**
 * @brief SCMI base protocol discover the name of an agent and agent id.
 *
 * @param agent_id SCMI agent id. The platform will return caller SCMI agent id
 *                 if set to SCMI_BASE_AGENT_ID_OWN.
 * @param agent_inf pointer on SCMI agent information struct scmi_agent_info.
 * @retval 0 If successful.
 */
int scmi_base_discover_agent(uint32_t agent_id, struct scmi_agent_info *agent_inf);

/**
 * @brief SCMI base protocol set an agent permissions to access devices.
 *
 * @param agent_id SCMI agent id.
 * @param device_id SCMI device id.
 * @param allow If set to true, allow agent access to the device.
 * @retval 0 If successful.
 */
int scmi_base_device_permission(uint32_t agent_id, uint32_t device_id, bool allow);

/**
 * @brief SCMI base protocol reset platform resource settings that were configured by an agent.
 *
 * @param agent_id SCMI agent id.
 * @param reset_perm If set to true, reset all access permission settings of the agent.
 * @retval 0 If successful.
 */
int scmi_base_reset_agent_cfg(uint32_t agent_id, bool reset_perm);

#endif /* DRIVERS_ARM_SCMI_BASE_H_ */

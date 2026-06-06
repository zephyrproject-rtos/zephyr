/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_reset
 * @brief Header file for the SCMI Reset Protocol.
 */

#ifndef DRIVERS_ARM_SCMI_RESET_H_
#define DRIVERS_ARM_SCMI_RESET_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

/**
 * @brief Reset domain management via SCMI
 * @defgroup scmi_reset Reset domain management Protocol
 * @ingroup scmi_protocols
 * @{
 */

/* PROTOCOL_ATTRIBUTES */
/** @brief Mask for the number of reset domains in the PROTOCOL_ATTRIBUTES report */
#define SCMI_RESET_ATTR_NUM_DOMAINS           GENMASK(15, 0)
/** @brief Extract the number of reset domains from the protocol attributes */
#define SCMI_RESET_ATTR_GET_NUM_DOMAINS(attr) FIELD_GET(SCMI_RESET_ATTR_NUM_DOMAINS, (attr))

/* RESET Domain Attributes Flags */
/** @brief Autonomous Reset flag: the reset must be performed autonomously by the platform */
#define SCMI_RESET_AUTONOMOUS         BIT(0)
/** @brief Explicit assert/deassert flag: explicitly assert reset signal */
#define SCMI_RESET_EXPLICIT_ASSERT    BIT(1)
/** @brief Asynchronous reset flag: the reset must complete asynchronously */
#define SCMI_RESET_ASYNCHRONOUS_RESET BIT(2)

/** @brief Architectural Cold Reset type */
#define SCMI_RESET_ARCH_COLD_RESET 0

/** @brief Supported version of the SCMI Reset protocol */
#define SCMI_RESET_PROTOCOL_SUPPORTED_VERSION 0x30001

/**
 * @brief SCMI Reset domain attributes
 *
 */
struct scmi_reset_domain_attr {
	/** Maximum time (in microseconds) required for the reset to take effect */
	uint32_t latency;
	/** Reset domain name */
	char name[SCMI_SHORT_NAME_MAX_SIZE];
	/** Asynchronous reset support */
	bool is_async_sup:1;
	/** Reset notifications support */
	bool is_notifications_sup:1;
	/** Reset latency is valid */
	bool is_latency_valid:1;
};

/**
 * @brief SCMI reset domain protocol get attributes of the reset domain
 *
 * @param proto pointer to SCMI clock protocol data
 * @param id ID of the Reset domain for which the query is done
 * @param dom_attr pointer to return Reset domain @p id attributes
 *
 * @return 0 on success, negative errno on failure
 */
int scmi_reset_domain_get_attr(struct scmi_protocol *proto, uint32_t id,
			       struct scmi_reset_domain_attr *dom_attr);

/**
 * @brief SCMI reset domain protocol reset domain
 *
 * @param proto pointer to SCMI clock protocol data
 * @param id ID of the Reset domain for which the query is done
 * @param flags the additional conditions and requirements specific to the request
 * @param reset_state the reset state being requested
 *
 * @return 0 on success, negative errno on failure
 */
int scmi_reset_domain(struct scmi_protocol *proto, uint32_t id, uint32_t flags,
		      uint32_t reset_state);

/**
 * @}
 */

#endif /* DRIVERS_ARM_SCMI_RESET_H_ */

/*
 * Copyright (c) 2024 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_ARM_SCMI_RESET_H_
#define DRIVERS_ARM_SCMI_RESET_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

#define SCMI_PROTOCOL_RESET_REV_MAJOR 0x1

/**
 * @brief SCMI Reset domain attributes
 *
 */
struct scmi_reset_domain_attr {
	/** Maximum time (in microseconds) required for the reset to take effect */
	uint32_t latency;
	/** reset domain name */
	char name[SCMI_MAX_STR_SIZE];
	/** Asynchronous reset support */
	bool is_async_sup:1;
	/** Reset notifications support */
	bool is_notifications_sup:1;
	/** Reset latency is valid */
	bool is_latency_valid:1;
};

/**
 * @brief SCMI reset domain protocol get protocol attributes
 *
 * @param proto pointer to SCMI clock protocol data
 * @param num_domains pointer to return Number of reset domains
 *
 * @retval 0 if successful
 * @retval negative errno on failure
 */
int scmi_reset_get_attr(struct scmi_protocol *proto, uint16_t *num_domains);

/**
 * @brief SCMI reset domain protocol get attributes of the reset domain
 *
 * @param proto pointer to SCMI clock protocol data
 * @param id ID of the Reset domain for which the query is done
 * @param dom_attr pointer to return Reset domain @p id attributes
 *
 * @retval 0 if successful
 * @retval negative errno on failure
 */
int scmi_reset_domain_get_attr(struct scmi_protocol *proto, uint32_t id,
			       struct scmi_reset_domain_attr *dom_attr);

/**
 * @brief SCMI reset domain protocol assert reset signal for the reset domain
 *
 * @param proto pointer to SCMI clock protocol data
 * @param id ID of the Reset domain for which the query is done
 *
 * @retval 0 if successful
 * @retval negative errno on failure
 */
int scmi_reset_domain_assert(struct scmi_protocol *proto, uint32_t id);

/**
 * @brief SCMI reset domain protocol de-assert reset signal for the reset domain
 *
 * @param proto pointer to SCMI clock protocol data
 * @param id ID of the Reset domain for which the query is done
 *
 * @retval 0 if successful
 * @retval negative errno on failure
 */
int scmi_reset_domain_deassert(struct scmi_protocol *proto, uint32_t id);

/**
 * @brief SCMI reset domain protocol toggle reset signal for the reset domain
 *
 * In terms of SCMI, perform Autonomous Reset action.
 *
 * @param proto pointer to SCMI clock protocol data
 * @param id ID of the Reset domain for which the query is done
 *
 * @retval 0 if successful
 * @retval negative errno on failure
 */
int scmi_reset_domain_toggle(struct scmi_protocol *proto, uint32_t id);

#endif /* DRIVERS_ARM_SCMI_RESET_H_ */

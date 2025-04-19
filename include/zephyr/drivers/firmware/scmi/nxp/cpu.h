/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCMI power domain protocol helpers
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CPU_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CPU_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>
#if __has_include("scmi_cpu_soc.h")
#include <scmi_cpu_soc.h>
#endif

#define SCMI_CPU_SLEEP_FLAG_IRQ_MUX 0x1U

#define SCMI_PROTOCOL_CPU_DOMAIN 130

/**
 * @struct scmi_cpu_sleep_mode_config
 *
 * @brief Describes the parameters for the CPU_STATE_SET
 * command
 */
struct scmi_cpu_sleep_mode_config {
	uint32_t cpu_id;
	uint32_t flags;
	uint32_t sleep_mode;
};

/**
 * @brief CPU domain protocol command message IDs
 */
enum scmi_cpu_domain_message {
	SCMI_CPU_DOMAIN_MSG_PROTOCOL_VERSION = 0x0,
	SCMI_CPU_DOMAIN_MSG_PROTOCOL_ATTRIBUTES = 0x1,
	SCMI_CPU_DOMAIN_MSG_PROTOCOL_MESSAGE_ATTRIBUTES = 0x2,
	SCMI_CPU_DOMAIN_MSG_CPU_DOMAIN_ATTRIBUTES = 0x3,
	SCMI_CPU_DOMAIN_MSG_CPU_START = 0x4,
	SCMI_CPU_DOMAIN_MSG_CPU_STOP = 0x5,
	SCMI_CPU_DOMAIN_MSG_CPU_RESET_VECTOR_SET = 0x6,
	SCMI_CPU_DOMAIN_MSG_CPU_SLEEP_MODE_SET = 0x7,
	SCMI_CPU_DOMAIN_MSG_CPU_IRQ_WAKE_SET = 0x8,
	SCMI_CPU_DOMAIN_MSG_CPU_NON_IRQ_WAKE_SET = 0x9,
	SCMI_CPU_DOMAIN_MSG_CPU_PD_LPM_CONFIG_SET = 0xA,
	SCMI_CPU_DOMAIN_MSG_CPU_PER_LPM_CONFIG_SET = 0xB,
	SCMI_CPU_DOMAIN_MSG_CPU_INFO_GET = 0xC,
	SCMI_CPU_DOMAIN_MSG_NEGOTIATE_PROTOCOL_VERSION = 0x10,
};

/**
 * @brief Send the CPU_SLEEP_MODE_SET command and get its reply
 *
 * @param cfg pointer to structure containing configuration
 * to be set
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_cpu_sleep_mode_set(struct scmi_cpu_sleep_mode_config *cfg);

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CPU_H_ */

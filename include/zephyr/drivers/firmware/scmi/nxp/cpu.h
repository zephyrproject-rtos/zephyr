/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_nxp_cpu
 * @brief Header file for the NXP SCMI CPU Domain Protocol.
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CPU_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CPU_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>
#if __has_include("scmi_cpu_soc.h")
#include <scmi_cpu_soc.h>
#endif

/**
 * @brief NXP vendor-specific CPU domain management via SCMI
 * @defgroup scmi_nxp_cpu NXP CPU Domain Protocol
 * @ingroup scmi_protocols
 * @{
 */

/** CPU sleep flag: wake-up is routed through the IRQ mux */
#define SCMI_NXP_CPU_SLEEP_FLAG_IRQ_MUX 0x1U

/** Protocol ID of the NXP CPU domain protocol */
#define SCMI_PROTOCOL_NXP_CPU_DOMAIN 130

/** Maximum number of power-domain LPM configurations per request */
#define SCMI_NXP_CPU_MAX_PDCONFIGS_T 7U

/** Maximum number of IRQ wake mask words per request */
#define SCMI_NXP_CPU_IRQ_WAKE_NUM	22U

/** CPU vector flag: Boot address (cold boot/reset) */
#define SCMI_NXP_CPU_VEC_FLAGS_BOOT    BIT(29)

/** CPU vector flag: Start address (warm start) */
#define SCMI_NXP_CPU_VEC_FLAGS_START   BIT(30)

/** CPU vector flag: Resume address (exit from suspend) */
#define SCMI_NXP_CPU_VEC_FLAGS_RESUME  BIT(31)

/** Version of the NXP SCMI CPU domain protocol supported by this driver */
#define SCMI_NXP_CPU_PROTOCOL_SUPPORTED_VERSION	0x10000

/**
 * @struct scmi_nxp_cpu_sleep_mode_config
 *
 * @brief Describes the parameters for the CPU_STATE_SET
 * command
 */
struct scmi_nxp_cpu_sleep_mode_config {
	/** ID of the CPU to configure */
	uint32_t cpu_id;
	/** Sleep flags (see SCMI_NXP_CPU_SLEEP_FLAG_*) */
	uint32_t flags;
	/** sleep mode to enter */
	uint32_t sleep_mode;
};

/**
 * @struct scmi_pd_lpm_settings
 *
 * @brief Low power mode setting for a single power domain
 */
struct scmi_pd_lpm_settings {
	/** ID of the power domain */
	uint32_t domain_id;
	/** Low power mode setting for the domain */
	uint32_t lpm_setting;
	/** Retention mask applied in low power mode */
	uint32_t ret_mask;
};

/**
 * @struct scmi_nxp_cpu_pd_lpm_config
 *
 * @brief Describes CPU power domain low power mode setting
 */
struct scmi_nxp_cpu_pd_lpm_config {
	/** ID of the CPU to configure */
	uint32_t cpu_id;
	/** Number of valid entries in @ref cfgs */
	uint32_t num_cfg;
	/** Per power-domain low power mode settings */
	struct scmi_pd_lpm_settings cfgs[SCMI_NXP_CPU_MAX_PDCONFIGS_T];
};

/**
 * @struct scmi_nxp_cpu_irq_mask_config
 *
 * @brief Describes the parameters for the CPU_IRQ_WAKE_SET command
 */
struct scmi_nxp_cpu_irq_mask_config {
	/** ID of the CPU to configure */
	uint32_t cpu_id;
	/** Index of the first mask word to write */
	uint32_t mask_idx;
	/** Number of valid entries in @ref mask */
	uint32_t num_mask;
	/** IRQ wake-up mask words */
	uint32_t mask[SCMI_NXP_CPU_IRQ_WAKE_NUM];
};

/**
 * @struct scmi_nxp_cpu_vector_config
 *
 * @brief Describes the parameters for the CPU_RESET_VECTOR_SET command
 */
struct scmi_nxp_cpu_vector_config {
	/** ID of the CPU to configure */
	uint32_t cpu_id;
	/** Vector flags (see SCMI_NXP_CPU_VEC_FLAGS_*) */
	uint32_t flags;
	/** Lower 32 bits of the reset vector address */
	uint32_t vector_low;
	/** Upper 32 bits of the reset vector address */
	uint32_t vector_high;
};

/**
 * @struct scmi_nxp_cpu_info
 *
 * @brief Describes the parameters for the CPU_INFO_GET command
 */
struct scmi_nxp_cpu_info {
	/** Current run mode of the CPU */
	uint32_t run_mode;
	/** Current sleep mode of the CPU */
	uint32_t sleep_mode;
	/** Lower 32 bits of the CPU reset vector address */
	uint32_t vector_low;
	/** Upper 32 bits of the CPU reset vector address */
	uint32_t vector_high;
};

/**
 * @brief Send the CPU_SLEEP_MODE_SET command and get its reply
 *
 * @param cfg pointer to structure containing configuration
 * to be set
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_nxp_cpu_sleep_mode_set(struct scmi_nxp_cpu_sleep_mode_config *cfg);

/**
 * @brief Send the SCMI_NXP_CPU_DOMAIN_MSG_CPU_PD_LPM_CONFIG_SET command and get its reply
 *
 * @param cfg pointer to structure containing configuration
 * to be set
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_nxp_cpu_pd_lpm_set(struct scmi_nxp_cpu_pd_lpm_config *cfg);

/**
 * @brief Send the CPU_IRQ_WAKE_SET command and get its reply
 *
 * @param cfg pointer to structure containing configuration to be set
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_nxp_cpu_set_irq_mask(struct scmi_nxp_cpu_irq_mask_config *cfg);

/**
 * @brief Send the CPU_RESET_VECTOR_SET command and get its reply
 *
 * @param cfg pointer to structure containing configuration to be set
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_nxp_cpu_reset_vector(struct scmi_nxp_cpu_vector_config *cfg);

/**
 * @brief Send the CPU_INFO_GET command and get its reply
 *
 * @param cpu_id CPU ID to query (input)
 * @param cfg pointer to structure to receive CPU information (output)
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_nxp_cpu_info_get(uint32_t cpu_id, struct scmi_nxp_cpu_info *cfg);

/**
 * @}
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CPU_H_ */

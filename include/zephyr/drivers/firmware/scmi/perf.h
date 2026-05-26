/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_perf
 * @brief Header file for the SCMI Performance Domain Protocol.
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PERF_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PERF_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

/**
 * @brief Performance domain management via SCMI
 * @defgroup scmi_perf Performance Domain Protocol
 * @ingroup scmi_protocols
 * @{
 */

/** SCMI Performance Domain protocol supported version */
#define SCMI_PERF_PROTOCOL_SUPPORTED_VERSION		0x40000

/** Maximum number of OPPs that can be cached per performance domain */
#define SCMI_PERF_MAX_OPPS				16

/**
 * @name SCMI performance domain attribute flags
 * @{
 */

/** Domain supports PERFORMANCE_LIMITS_SET command */
#define SCMI_PERF_ATTR_SET_LIMITS(f)		((f) & BIT(31))
/** Domain supports PERFORMANCE_LEVEL_SET command */
#define SCMI_PERF_ATTR_SET_LEVEL(f)		((f) & BIT(30))
/** Domain supports performance limit change notifications */
#define SCMI_PERF_ATTR_LIMIT_NOTIFY(f)		((f) & BIT(29))
/** Domain supports performance level change notifications */
#define SCMI_PERF_ATTR_LEVEL_NOTIFY(f)		((f) & BIT(28))
/** Domain supports fast channels */
#define SCMI_PERF_ATTR_FASTCHANNELS(f)		((f) & BIT(27))
/** Domain supports extended names (v3+) */
#define SCMI_PERF_ATTR_EXTENDED_NAME(f)		((f) & BIT(26))
/** Domain uses level indexing mode (v4+) */
#define SCMI_PERF_ATTR_LEVEL_INDEXING(f)	((f) & BIT(25))

/** @} */

/** Rate limit field mask (bits 19:0 of rate_limit field) */
#define SCMI_PERF_RATE_LIMIT_MASK		GENMASK(19, 0)

/**
 * @struct scmi_perf_opp
 *
 * @brief Describes one Operating Performance Point (OPP) as returned
 * by the PERFORMANCE_DESCRIBE_LEVELS command.
 */
struct scmi_perf_opp {
	/** Performance level value. */
	uint32_t perf_val_khz;
	/** Power cost in mW */
	uint32_t power_mw;
	/** Worst-case transition latency in microseconds */
	uint16_t transition_latency_us;
	/** Indicative frequency in kHz */
	uint32_t indicative_freq_khz;
};

/**
 * @struct scmi_perf_domain_attrs
 *
 * @brief Describes the attributes of a performance domain as returned
 * by the PERFORMANCE_DOMAIN_ATTRIBUTES command.
 */
struct scmi_perf_domain_attrs {
	/** Domain attribute flags */
	uint32_t flags;
	/** Minimum interval in microseconds between successive PERFORMANCE_LEVEL_SET calls */
	uint32_t rate_limit_us;
	/** Frequency in kHz at the sustained performance level */
	uint32_t sustained_freq_khz;
	/** Performance level value at the sustained operating point */
	uint32_t sustained_perf_level;
	/** Short ASCII name of the domain (NUL-terminated) */
	uint8_t name[SCMI_SHORT_NAME_MAX_SIZE];
};

/**
 * @struct scmi_perf_limits
 *
 * @brief Describes the current performance limits of a domain as returned
 * by the PERFORMANCE_LIMITS_GET command.
 */
struct scmi_perf_limits {
	/** Maximum allowed performance level */
	uint32_t max_level;
	/** Minimum allowed performance level */
	uint32_t min_level;
};

/**
 * @brief Query the number of performance domains
 *
 * Sends the PROTOCOL_ATTRIBUTES command and extracts the domain count
 * from bits[15:0] of the attributes field.
 *
 * @param num_domains pointer to be set to the number of performance domains
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_perf_num_domains_get(uint32_t *num_domains);

/**
 * @brief Query the attributes of a performance domain
 *
 * Sends the PERFORMANCE_DOMAIN_ATTRIBUTES command and returns the
 * domain's capabilities, rate limit, sustained frequency and name.
 *
 * @param domain_id ID of the performance domain to query
 * @param attrs pointer to structure to be filled with domain attributes
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_perf_domain_attributes_get(uint32_t domain_id,
				    struct scmi_perf_domain_attrs *attrs);

/**
 * @brief Enumerate the OPPs of a performance domain
 *
 * Sends one or more PERFORMANCE_DESCRIBE_LEVELS commands.
 *
 * @param domain_id ID of the performance domain to query
 * @param opps pointer to array of @ref scmi_perf_opp to be filled,
 *             or NULL to query the count only
 * @param max_opps maximum number of OPPs the array can hold
 * @param num_opps pointer to be set to the actual number of OPPs returned
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_perf_describe_levels(uint32_t domain_id,
			      struct scmi_perf_opp *opps,
			      uint32_t max_opps,
			      uint32_t *num_opps);

/**
 * @brief Set the performance level of a domain
 *
 * @param domain_id ID of the performance domain
 * @param perf_level_khz desired frequency in kHz (e.g. 800000 for 800 MHz)
 *
 * @retval 0 if successful
 * @retval -ENOENT if @p perf_level_khz is not in the OPP table
 * @retval negative errno if failure
 */
int scmi_perf_level_set(uint32_t domain_id, uint32_t perf_level_khz);

/**
 * @brief Get the current performance level of a domain
 *
 * @param domain_id ID of the performance domain
 * @param perf_level_khz pointer to be set to the current frequency in kHz
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_perf_level_get(uint32_t domain_id, uint32_t *perf_level_khz);

/**
 * @brief Get the current performance limits of a domain
 *
 * Sends the PERFORMANCE_LIMITS_GET command and returns the current
 * minimum and maximum allowed performance levels.
 *
 * @param domain_id ID of the performance domain
 * @param limits pointer to structure to be filled with current limits
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_perf_limits_get(uint32_t domain_id, struct scmi_perf_limits *limits);

/**
 * @}
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PERF_H_ */

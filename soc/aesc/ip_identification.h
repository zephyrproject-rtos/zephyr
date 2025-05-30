/** @file
 *  @brief IP Identification API.
 *
 *  Copyright (c) 2025 Aesc Silicon
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/sys/util.h>

#ifndef INCLUDE_DRIVERS_AESC_IP_IDENTIFICATION_H_
#define INCLUDE_DRIVERS_AESC_IP_IDENTIFICATION_H_
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Each Aesc Silicon IP core is equipped with a so-called IP identification
 * table at the beginning of each register map. This table helps to identify
 * the underlying hardware.
 *
 * IP identification table for API v0:
 *
 * +---------+---------+---------+---------+
 * | 31 - 24 | 23 - 16 | 15 -  8 |  7 -  0 |
 * +=========+=========+=========+=========+
 * |     API |  Length |                ID |    0x00 (header)
 * +---------+---------+-------------------+
 * |   Major |   Minor |             Patch |    0x04 (version)
 * +---------+---------+-------------------+
 *
 * header.api_version: Version of this IP identification table.
 * header.length: Total length of the IP identification table.
 *                Important to relocate the register map.
 * header.id: ID of this IP core.
 * version: Defines the version of this IP core with major.minor.patchlevel.
 */

struct aesc_ip_id_table {
	uint32_t header;
	uint32_t version;
};

#define CONV_ADDR(addr) ((struct aesc_ip_id_table *)addr)

#define HEADER_API_MASK			GENMASK(31, 24)
#define HEADER_LENGTH_MASK		GENMASK(23, 16)
#define HEADER_ID_MASK			GENMASK(15, 0)
#define VERSION_MAJOR_MASK		GENMASK(31, 24)
#define VERSION_MINOR_MASK		GENMASK(23, 16)
#define VERSION_PATCH_MASK		GENMASK(15, 0)

static inline unsigned int ip_id_get_major_version(volatile uintptr_t *addr)
{
	const volatile struct aesc_ip_id_table *table = CONV_ADDR(addr);

	return FIELD_GET(VERSION_MAJOR_MASK, table->version);
}

static inline unsigned int ip_id_get_minor_version(volatile uintptr_t *addr)
{
	const volatile struct aesc_ip_id_table *table = CONV_ADDR(addr);

	return FIELD_GET(VERSION_MINOR_MASK, table->version);
}

static inline unsigned int ip_id_get_patchlevel(volatile uintptr_t *addr)
{
	const volatile struct aesc_ip_id_table *table = CONV_ADDR(addr);

	return FIELD_GET(VERSION_PATCH_MASK, table->version);
}

static inline unsigned int ip_id_get_api_version(volatile uintptr_t *addr)
{
	const volatile struct aesc_ip_id_table *table = CONV_ADDR(addr);

	return FIELD_GET(HEADER_API_MASK, table->header);
}

static inline unsigned int ip_id_get_header_length(volatile uintptr_t *addr)
{
	const volatile struct aesc_ip_id_table *table = CONV_ADDR(addr);

	return FIELD_GET(HEADER_LENGTH_MASK, table->header);
}

static inline unsigned int ip_id_get_id(volatile uintptr_t *addr)
{
	const volatile struct aesc_ip_id_table *table = CONV_ADDR(addr);

	return FIELD_GET(HEADER_ID_MASK, table->header);
}

static inline uintptr_t ip_id_relocate_driver(volatile uintptr_t *addr)
{
	return (uintptr_t)addr + ip_id_get_header_length(addr);
}

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_DRIVERS_AESC_IP_IDENTIFICATION_H_ */

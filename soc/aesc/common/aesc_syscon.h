/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_AESC_AESC_SYSCON
#define ZEPHYR_SOC_AESC_AESC_SYSCON

/* Syscon registers (after 8-byte IP Identification table) */
#define SYSCON_IDENTITY		0x08
#define SYSCON_SILICON_REV	0x0c
#define SYSCON_BUILD_DATE	0x10
#define SYSCON_REF_CLOCK	0x14
#define SYSCON_FEATURE_INFO	0x18
#define SYSCON_FEATURES		0x1c

#endif /* ZEPHYR_SOC_AESC_AESC_SYSCON */

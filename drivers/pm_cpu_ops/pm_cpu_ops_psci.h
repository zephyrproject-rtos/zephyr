/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PSCI_PSCI_H_
#define ZEPHYR_DRIVERS_PSCI_PSCI_H_

#include <zephyr/drivers/pm_cpu_ops/psci.h>

#ifdef CONFIG_64BIT
#define PSCI_FN_NATIVE(version, name) PSCI_##version##_FN64_##name
#else
#define PSCI_FN_NATIVE(version, name) PSCI_##version##_FN_##name
#endif

/* PSCI v0.2 interface */
#define PSCI_0_2_FN_BASE                  0x84000000
#define PSCI_0_2_FN(n)                    (PSCI_0_2_FN_BASE + (n))
#define PSCI_0_2_64BIT                    0x40000000
#define PSCI_0_2_FN64_BASE                (PSCI_0_2_FN_BASE + PSCI_0_2_64BIT)
#define PSCI_0_2_FN64(n)                  (PSCI_0_2_FN64_BASE + (n))
#define PSCI_0_2_FN_PSCI_VERSION          PSCI_0_2_FN(0)
#define PSCI_0_2_FN_CPU_SUSPEND           PSCI_0_2_FN(1)
#define PSCI_0_2_FN_CPU_OFF               PSCI_0_2_FN(2)
#define PSCI_0_2_FN_CPU_ON                PSCI_0_2_FN(3)
#define PSCI_0_2_FN_AFFINITY_INFO         PSCI_0_2_FN(4)
#define PSCI_0_2_FN_MIGRATE               PSCI_0_2_FN(5)
#define PSCI_0_2_FN_MIGRATE_INFO_TYPE     PSCI_0_2_FN(6)
#define PSCI_0_2_FN_MIGRATE_INFO_UP_CPU   PSCI_0_2_FN(7)
#define PSCI_0_2_FN_SYSTEM_OFF            PSCI_0_2_FN(8)
#define PSCI_0_2_FN_SYSTEM_RESET          PSCI_0_2_FN(9)
#define PSCI_0_2_FN64_CPU_SUSPEND         PSCI_0_2_FN64(1)
#define PSCI_0_2_FN64_CPU_ON              PSCI_0_2_FN64(3)
#define PSCI_0_2_FN64_AFFINITY_INFO       PSCI_0_2_FN64(4)
#define PSCI_0_2_FN64_MIGRATE             PSCI_0_2_FN64(5)
#define PSCI_0_2_FN64_MIGRATE_INFO_UP_CPU PSCI_0_2_FN64(7)
#define PSCI_0_2_FN64_SYSTEM_RESET        PSCI_0_2_FN(9)

/* PSCI v1.0 interface */
#define PSCI_1_0_FN_BASE                  (0x84000000U)
#define PSCI_1_0_64BIT                    (0x40000000U)
#define PSCI_1_0_FN64_BASE                (PSCI_1_0_FN_BASE + PSCI_1_0_64BIT)
#define PSCI_1_0_FN(n)                    (PSCI_1_0_FN_BASE + (n))
#define PSCI_1_0_FN64(n)                  (PSCI_1_0_FN64_BASE + (n))
#define PSCI_1_0_FN_PSCI_VERSION          PSCI_1_0_FN(0)
#define PSCI_1_0_FN_CPU_SUSPEND           PSCI_1_0_FN(1)
#define PSCI_1_0_FN_CPU_OFF               PSCI_1_0_FN(2)
#define PSCI_1_0_FN_CPU_ON                PSCI_1_0_FN(3)
#define PSCI_1_0_FN_AFFINITY_INFO         PSCI_1_0_FN(4)
#define PSCI_1_0_FN_MIGRATE               PSCI_1_0_FN(5)
#define PSCI_1_0_FN_MIGRATE_INFO_TYPE     PSCI_1_0_FN(6)
#define PSCI_1_0_FN_MIGRATE_INFO_UP_CPU   PSCI_1_0_FN(7)
#define PSCI_1_0_FN_SYSTEM_OFF            PSCI_1_0_FN(8)
#define PSCI_1_0_FN_SYSTEM_RESET          PSCI_1_0_FN(9)
#define PSCI_1_0_FN_PSCI_FEATURES         PSCI_1_0_FN(10)
#define PSCI_1_0_FN64_CPU_SUSPEND         PSCI_1_0_FN64(1)
#define PSCI_1_0_FN64_CPU_ON              PSCI_1_0_FN64(3)
#define PSCI_1_0_FN64_AFFINITY_INFO       PSCI_1_0_FN64(4)
#define PSCI_1_0_FN64_MIGRATE             PSCI_1_0_FN64(5)
#define PSCI_1_0_FN64_MIGRATE_INFO_UP_CPU PSCI_1_0_FN64(7)
/* PSCI function ID is same for both 32 and 64 bit.*/
#define PSCI_1_0_FN64_SYSTEM_RESET        PSCI_1_0_FN(9)
#define PSCI_1_0_FN64_PSCI_FEATURES       PSCI_1_0_FN(10)

/* PSCI v1.1 interface. */
#define PSCI_1_1_FN_BASE                  (0x84000000U)
#define PSCI_1_1_64BIT                    (0x40000000U)
#define PSCI_1_1_FN64_BASE                (PSCI_1_1_FN_BASE + PSCI_1_1_64BIT)
#define PSCI_1_1_FN(n)                    (PSCI_1_1_FN_BASE + (n))
#define PSCI_1_1_FN64(n)                  (PSCI_1_1_FN64_BASE + (n))
#define PSCI_1_1_FN_PSCI_VERSION          PSCI_1_1_FN(0)
#define PSCI_1_1_FN_CPU_SUSPEND           PSCI_1_1_FN(1)
#define PSCI_1_1_FN_CPU_OFF               PSCI_1_1_FN(2)
#define PSCI_1_1_FN_CPU_ON                PSCI_1_1_FN(3)
#define PSCI_1_1_FN_AFFINITY_INFO         PSCI_1_1_FN(4)
#define PSCI_1_1_FN_MIGRATE               PSCI_1_1_FN(5)
#define PSCI_1_1_FN_MIGRATE_INFO_TYPE     PSCI_1_1_FN(6)
#define PSCI_1_1_FN_MIGRATE_INFO_UP_CPU   PSCI_1_1_FN(7)
#define PSCI_1_1_FN_SYSTEM_OFF            PSCI_1_1_FN(8)
#define PSCI_1_1_FN_SYSTEM_RESET          PSCI_1_1_FN(9)
#define PSCI_1_1_FN_PSCI_FEATURES         PSCI_1_1_FN(10)
#define PSCI_1_1_FN_SYSTEM_RESET2         PSCI_1_1_FN(18)
#define PSCI_1_1_FN64_CPU_SUSPEND         PSCI_1_1_FN64(1)
#define PSCI_1_1_FN64_CPU_ON              PSCI_1_1_FN64(3)
#define PSCI_1_1_FN64_AFFINITY_INFO       PSCI_1_1_FN64(4)
#define PSCI_1_1_FN64_MIGRATE             PSCI_1_1_FN64(5)
#define PSCI_1_1_FN64_MIGRATE_INFO_UP_CPU PSCI_1_1_FN64(7)
/* PSCI function ID is same for both 32 and 64 bit.*/
#define PSCI_1_1_FN64_SYSTEM_RESET        PSCI_1_1_FN(9)
#define PSCI_1_1_FN64_PSCI_FEATURES       PSCI_1_1_FN(10)
#define PSCI_1_1_FN64_SYSTEM_RESET2       PSCI_1_1_FN64(18)

/* PSCI return values (inclusive of all PSCI versions) */
#define PSCI_RET_SUCCESS          0
#define PSCI_RET_NOT_SUPPORTED    -1
#define PSCI_RET_INVALID_PARAMS   -2
#define PSCI_RET_DENIED           -3
#define PSCI_RET_ALREADY_ON       -4
#define PSCI_RET_ON_PENDING       -5
#define PSCI_RET_INTERNAL_FAILURE -6
#define PSCI_RET_NOT_PRESENT      -7
#define PSCI_RET_DISABLED         -8
#define PSCI_RET_INVALID_ADDRESS  -9

typedef unsigned long (psci_fn)(unsigned long, unsigned long,
				unsigned long, unsigned long);

struct psci_data_t {
	enum arm_smccc_conduit conduit;
	psci_fn *invoke_psci_fn;
	uint32_t ver;
};

/* PSCI configuration data. */
struct psci_config_t {
	const char *method;
};

#endif /* ZEPHYR_DRIVERS_PSCI_PSCI_H_ */

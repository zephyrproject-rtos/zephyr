/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv8-A AArch64 PMUv3 — architectural events and register bits
 *
 * Portable applications should include <zephyr/pmu.h> for the @c pmu_*() API.
 * This header defines @c pmu_info_t, @c pmu_evt_t (Arm architectural PMU event
 * numbers in the @c 0x00-0x1F range), and @c PMCR_EL0 / @c PMUSERENR_EL0 bits.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_PMUV3_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_PMUV3_H_

#include <zephyr/sys/util.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PMU identification for the current logical CPU (after @ref pmu_init).
 */
struct pmu_info {
	/** PMU implementer code (Arm PMU identification register encoding). */
	uint32_t implementer;
	/** PMU part number / ID code (Arm PMU identification register encoding). */
	uint32_t idcode;
};

/**
 * @brief Typedef for @ref pmu_info.
 */
typedef struct pmu_info pmu_info_t;

/**
 * @brief Arm architectural PMU event encoding (PMUv3, 0x00–0x1F).
 *
 * Names and values follow the Arm Architecture Reference Manual.
 */
typedef enum {
	PMU_EVT_SW_INCR = 0x00,          /*!< Software increment */
	PMU_EVT_L1I_CACHE_REFILL = 0x01, /*!< L1 instruction cache refill */
	PMU_EVT_L1I_TLB_REFILL = 0x02,   /*!< L1 instruction TLB refill */
	PMU_EVT_L1D_CACHE_REFILL = 0x03, /*!< L1 data cache refill */
	PMU_EVT_L1D_CACHE = 0x04,        /*!< L1 data cache access */
	PMU_EVT_L1D_TLB_REFILL = 0x05,   /*!< L1 data TLB refill */
	PMU_EVT_INST_RETIRED = 0x08,     /*!< Instruction architecturally executed */
	PMU_EVT_EXC_TAKEN = 0x09,        /*!< Exception taken */
	PMU_EVT_EXC_RETURN = 0x0A,   /*!< Instruction architecturally executed (exception return) */
	PMU_EVT_BR_MIS_PRED = 0x10,  /*!< Mispredicted or not-predicted branch */
	PMU_EVT_CPU_CYCLES = 0x11,   /*!< Cycle */
	PMU_EVT_BR_PRED = 0x12,      /*!< Predictable branch */
	PMU_EVT_MEM_ACCESS = 0x13,   /*!< Data memory access */
	PMU_EVT_L1I_CACHE = 0x14,    /*!< L1 instruction cache access */
	PMU_EVT_L1D_CACHE_WB = 0x15, /*!< L1 data cache write-back */
	PMU_EVT_L2D_CACHE = 0x16,    /*!< L2 data cache access */
	PMU_EVT_L2D_CACHE_REFILL = 0x17,   /*!< L2 data cache refill */
	PMU_EVT_L2D_CACHE_WB = 0x18,       /*!< L2 data cache write-back */
	PMU_EVT_BUS_ACCESS = 0x19,         /*!< Bus access */
	PMU_EVT_MEMORY_ERROR = 0x1A,       /*!< Local memory error */
	PMU_EVT_INST_SPEC = 0x1B,          /*!< Instruction speculatively executed */
	PMU_EVT_TTBR_WRITE_RETIRED = 0x1C, /*!< Instruction architecturally executed (TTBR write) */
	PMU_EVT_BUS_CYCLES = 0x1D,         /*!< Bus cycle */
} pmu_evt_t;

/** @brief PMCR_EL0: enable all counters. */
#define PMCR_E  BIT(0)
/** @brief PMCR_EL0: reset all event counters. */
#define PMCR_P  BIT(1)
/** @brief PMCR_EL0: cycle counter reset. */
#define PMCR_C  BIT(2)
/** @brief PMCR_EL0: clock divider (1 = every 64 cycles). */
#define PMCR_D  BIT(3)
/** @brief PMCR_EL0: export enable. */
#define PMCR_X  BIT(4)
/** @brief PMCR_EL0: disable cycle counter in EL2. */
#define PMCR_DP BIT(5)
/**
 * @brief PMCR_EL0: long cycle counter (64-bit).
 *
 * When set, determines when unsigned overflow of the cycle counter is recorded
 * in PMOVSCLR_EL0.C.
 */
#define PMCR_LC BIT(6)
/**
 * @brief PMCR_EL0: long event counter enable.
 *
 * When set, determines when unsigned overflow of each event counter is recorded
 * in PMOVSCLR_EL0.P[n].
 */
#define PMCR_LP BIT(7)

/** @brief PMCR_EL0 bit position for event counter count field. */
#define PMCR_N_SHIFT      11
/** @brief PMCR_EL0 mask for number of implemented event counters. */
#define PMCR_N_MASK       0x1F
/** @brief PMCR_EL0 bit position for ID code field. */
#define PMCR_IDCODE_SHIFT 16
/** @brief PMCR_EL0 mask for ID code field. */
#define PMCR_IDCODE_MASK  0xFF
/** @brief PMCR_EL0 bit position for implementer code field. */
#define PMCR_IMP_SHIFT    24
/** @brief PMCR_EL0 mask for implementer code field. */
#define PMCR_IMP_MASK     0xFF

/** @brief PMUSERENR_EL0: user-mode PMU access enable. */
#define PMUSERENR_EN BIT(0)
/** @brief PMUSERENR_EL0: software increment at EL0. */
#define PMUSERENR_SW BIT(1)
/** @brief PMUSERENR_EL0: cycle counter read at EL0. */
#define PMUSERENR_CR BIT(2)
/** @brief PMUSERENR_EL0: event counter read at EL0. */
#define PMUSERENR_ER BIT(3)

/** @cond INTERNAL_HIDDEN */

int arch_pmu_init(void);
uint32_t arch_pmu_num_counters(void);
void arch_pmu_get_info(pmu_info_t *info);
int arch_pmu_counter_config(uint32_t counter, pmu_evt_t event);
void arch_pmu_counter_enable(uint32_t counter);
void arch_pmu_counter_disable(uint32_t counter);
void arch_pmu_counter_enable_all(void);
void arch_pmu_counter_disable_all(void);
uint64_t arch_pmu_counter_read(uint32_t counter);
void arch_pmu_counter_reset(uint32_t counter);
void arch_pmu_counter_reset_all(void);
uint64_t arch_pmu_cycle_count(void);
void arch_pmu_cycle_reset(void);
void arch_pmu_start(void);
void arch_pmu_stop(void);
bool arch_pmu_counter_overflow(uint32_t counter);
void arch_pmu_counter_clear_overflow(uint32_t counter);

#define pmu_init                   arch_pmu_init
#define pmu_num_counters           arch_pmu_num_counters
#define pmu_get_info               arch_pmu_get_info
#define pmu_counter_config         arch_pmu_counter_config
#define pmu_counter_enable         arch_pmu_counter_enable
#define pmu_counter_disable        arch_pmu_counter_disable
#define pmu_counter_enable_all     arch_pmu_counter_enable_all
#define pmu_counter_disable_all    arch_pmu_counter_disable_all
#define pmu_counter_read           arch_pmu_counter_read
#define pmu_counter_reset          arch_pmu_counter_reset
#define pmu_counter_reset_all      arch_pmu_counter_reset_all
#define pmu_cycle_count            arch_pmu_cycle_count
#define pmu_cycle_reset            arch_pmu_cycle_reset
#define pmu_start                  arch_pmu_start
#define pmu_stop                   arch_pmu_stop
#define pmu_counter_overflow       arch_pmu_counter_overflow
#define pmu_counter_clear_overflow arch_pmu_counter_clear_overflow

/** @endcond */

/**
 * @name Diagnostic helpers (not portable @c pmu_*() primitives)
 * @{
 */

/** @brief Calibrated CPU MHz from @ref pmu_init (PMU cycle counter vs system timer). */
uint32_t arch_pmu_cpu_freq_mhz(void);

/** @brief Human-readable name for @a event, or @c "UNKNOWN". */
const char *arch_pmu_event_name(uint32_t event);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_PMUV3_H_ */

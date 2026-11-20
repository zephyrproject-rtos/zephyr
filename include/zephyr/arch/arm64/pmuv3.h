/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv8-A AArch64 PMUv3 — architectural events and register bits
 *
 * Portable applications should include <zephyr/arch/perf_counter.h> for the
 * @c perf_counter_*() API.
 * This header defines @c perf_counter_info_t, @c perf_counter_evt_t (Arm architectural PMU event
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
 * @brief PMU identification for the current logical CPU (after @ref perf_counter_init).
 */
struct perf_counter_info {
	/** PMU implementer code (Arm PMU identification register encoding). */
	uint32_t implementer;
	/** PMU part number / ID code (Arm PMU identification register encoding). */
	uint32_t idcode;
};

/**
 * @brief Typedef for @ref perf_counter_info.
 */
typedef struct perf_counter_info perf_counter_info_t;

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
} perf_counter_evt_t;

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

int arch_perf_counter_init(void);
uint32_t arch_perf_counter_num_counters(void);
void arch_perf_counter_get_info(perf_counter_info_t *info);
int arch_perf_counter_config(uint32_t counter, perf_counter_evt_t event);
void arch_perf_counter_enable(uint32_t counter);
void arch_perf_counter_disable(uint32_t counter);
void arch_perf_counter_enable_all(void);
void arch_perf_counter_disable_all(void);
uint64_t arch_perf_counter_read(uint32_t counter);
void arch_perf_counter_reset(uint32_t counter);
void arch_perf_counter_reset_all(void);
uint64_t arch_perf_counter_cycle_count(void);
void arch_perf_counter_cycle_reset(void);
void arch_perf_counter_start(void);
void arch_perf_counter_stop(void);
bool arch_perf_counter_overflow(uint32_t counter);
void arch_perf_counter_clear_overflow(uint32_t counter);

#define perf_counter_init           arch_perf_counter_init
#define perf_counter_num_counters   arch_perf_counter_num_counters
#define perf_counter_get_info       arch_perf_counter_get_info
#define perf_counter_config         arch_perf_counter_config
#define perf_counter_enable         arch_perf_counter_enable
#define perf_counter_disable        arch_perf_counter_disable
#define perf_counter_enable_all     arch_perf_counter_enable_all
#define perf_counter_disable_all    arch_perf_counter_disable_all
#define perf_counter_read           arch_perf_counter_read
#define perf_counter_reset          arch_perf_counter_reset
#define perf_counter_reset_all      arch_perf_counter_reset_all
#define perf_counter_cycle_count    arch_perf_counter_cycle_count
#define perf_counter_cycle_reset    arch_perf_counter_cycle_reset
#define perf_counter_start          arch_perf_counter_start
#define perf_counter_stop           arch_perf_counter_stop
#define perf_counter_overflow       arch_perf_counter_overflow
#define perf_counter_clear_overflow arch_perf_counter_clear_overflow

/** @endcond */

/**
 * @name Diagnostic helpers (not portable @c perf_counter_*() primitives)
 * @{
 */

/** @brief Calibrated CPU MHz from @ref perf_counter_init (PMU cycle counter vs system timer). */
uint32_t arch_perf_counter_cpu_freq_mhz(void);

/** @brief Human-readable name for @a event, or @c "UNKNOWN". */
const char *arch_perf_counter_event_name(uint32_t event);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_PMUV3_H_ */

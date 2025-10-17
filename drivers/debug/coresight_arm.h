/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORESIGHT_ARM_H_
#define CORESIGHT_ARM_H_

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <zephyr/sys/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Generic ARM CoreSight Hardware Abstraction Layer
 *
 * This HAL provides generic register definitions and utility functions for ARM CoreSight
 * peripherals. Platform-specific drivers should provide base addresses and use these
 * generic definitions for register access.
 */

/* Common CoreSight unlock key as defined by ARM architecture */
#define CORESIGHT_UNLOCK_KEY (0xC5ACCE55UL)

/* CoreSight register offsets */

/* Common CoreSight peripheral register offsets (found at the end of all CoreSight peripherals) */
#define CORESIGHT_CLAIMSET_OFFSET (0xFA0UL) /* Claim Tag Set Register */
#define CORESIGHT_CLAIMCLR_OFFSET (0xFA4UL) /* Claim Tag Clear Register */
#define CORESIGHT_LAR_OFFSET      (0xFB0UL) /* Lock Access Register */
#define CORESIGHT_LSR_OFFSET      (0xFB4UL) /* Lock Status Register */

/* ATB Funnel register offsets */
#define ATBFUNNEL_CTRLREG_OFFSET (0x000UL) /* Control Register */

/* ATB Replicator register offsets */
#define ATBREPLICATOR_IDFILTER0_OFFSET (0x000UL) /* ID Filter Register 0 */
#define ATBREPLICATOR_IDFILTER1_OFFSET (0x004UL) /* ID Filter Register 1 */

/* ETR (Embedded Trace Router/TMC-ETR) register offsets */
#define ETR_RSZ_OFFSET   (0x004UL) /* RAM Size Register */
#define ETR_RWP_OFFSET   (0x018UL) /* RAM Write Pointer Register */
#define ETR_CTL_OFFSET   (0x020UL) /* Control Register */
#define ETR_MODE_OFFSET  (0x028UL) /* Mode Register */
#define ETR_DBALO_OFFSET (0x118UL) /* Data Buffer Address Low Register */
#define ETR_DBAHI_OFFSET (0x11CUL) /* Data Buffer Address High Register */
#define ETR_FFCR_OFFSET  (0x304UL) /* Formatter and Flush Control Register */

/* STM (System Trace Macrocell) register offsets */
#define STM_STMHEER_OFFSET    (0xD00UL) /* Hardware Event Enable Register */
#define STM_STMHEMCR_OFFSET   (0xD64UL) /* Hardware Event Master Control Register */
#define STM_STMSPER_OFFSET    (0xE00UL) /* Stimulus Port Enable Register */
#define STM_STMTCSR_OFFSET    (0xE80UL) /* Trace Control and Status Register */
#define STM_STMTSFREQR_OFFSET (0xE8CUL) /* Timestamp Frequency Register */
#define STM_STMSYNCR_OFFSET   (0xE90UL) /* Synchronization Control Register */
#define STM_STMAUXCR_OFFSET   (0xE94UL) /* Auxiliary Control Register */

/* TPIU (Trace Port Interface Unit) register offsets */
#define TPIU_CSPSR_OFFSET (0x004UL) /* Current Parallel Port Size Register */
#define TPIU_FFCR_OFFSET  (0x304UL) /* Formatter and Flush Control Register */
#define TPIU_FSCR_OFFSET  (0x308UL) /* Formatter Synchronization Counter Register */

/* CTI (Cross Trigger Interface) register offsets */
#define CTI_CTICONTROL_OFFSET (0x000UL) /* CTI Control Register */
#define CTI_CTIOUTEN0_OFFSET  (0x0A0UL) /* CTI Trigger Output Enable Register 0 */
#define CTI_CTIGATE_OFFSET    (0x140UL) /* CTI Channel Gate Enable Register */

/* TSGEN (Timestamp Generator) register offsets */
#define TSGEN_CNTCR_OFFSET   (0x000UL) /* Counter Control Register */
#define TSGEN_CNTFID0_OFFSET (0x020UL) /* Counter Frequency ID Register 0 */

/* Lock Status Register (LSR) bit fields */
#define CORESIGHT_LSR_LOCKED_Pos  (1UL)
#define CORESIGHT_LSR_LOCKED_Msk  (0x1UL << CORESIGHT_LSR_LOCKED_Pos)
#define CORESIGHT_LSR_PRESENT_Pos (0UL)
#define CORESIGHT_LSR_PRESENT_Msk (0x1UL << CORESIGHT_LSR_PRESENT_Pos)

/* STM Trace Control and Status Register (STMTCSR) bit fields */
#define STM_STMTCSR_EN_Pos      (0UL)
#define STM_STMTCSR_EN_Msk      (0x1UL << STM_STMTCSR_EN_Pos)
#define STM_STMTCSR_TSEN_Pos    (1UL)
#define STM_STMTCSR_TSEN_Msk    (0x1UL << STM_STMTCSR_TSEN_Pos)
#define STM_STMTCSR_TRACEID_Pos (16UL)
#define STM_STMTCSR_TRACEID_Msk (0x7FUL << STM_STMTCSR_TRACEID_Pos)

/* STM Hardware Event Master Control Register (STMHEMCR) bit fields */
#define STM_STMHEMCR_EN_Pos (0UL)
#define STM_STMHEMCR_EN_Msk (0x1UL << STM_STMHEMCR_EN_Pos)

/* STM Auxiliary Control Register (STMAUXCR) bit fields */
#define STM_STMAUXCR_FIFOAF_Pos (0UL)
#define STM_STMAUXCR_FIFOAF_Msk (0x1UL << STM_STMAUXCR_FIFOAF_Pos)

/* CTI Control Register (CTICONTROL) bit fields */
#define CTI_CTICONTROL_GLBEN_Pos (0UL)
#define CTI_CTICONTROL_GLBEN_Msk (0x1UL << CTI_CTICONTROL_GLBEN_Pos)

/* TPIU Formatter and Flush Control Register (FFCR) bit fields */
#define TPIU_FFCR_ENFCONT_Pos (1UL)
#define TPIU_FFCR_ENFCONT_Msk (0x1UL << TPIU_FFCR_ENFCONT_Pos)
#define TPIU_FFCR_FONFLIN_Pos (4UL)
#define TPIU_FFCR_FONFLIN_Msk (0x1UL << TPIU_FFCR_FONFLIN_Pos)
#define TPIU_FFCR_ENFTC_Pos   (0UL)
#define TPIU_FFCR_ENFTC_Msk   (0x1UL << TPIU_FFCR_ENFTC_Pos)

/* ETR Mode Register bit fields */
#define ETR_MODE_MODE_Pos         (0UL)
#define ETR_MODE_MODE_Msk         (0x3UL << ETR_MODE_MODE_Pos)
#define ETR_MODE_MODE_CIRCULARBUF (0UL) /* Circular Buffer mode */
#define ETR_MODE_MODE_SWFIFO1     (1UL) /* Software FIFO mode */
#define ETR_MODE_MODE_HWFIFO      (2UL) /* Hardware FIFO mode */
#define ETR_MODE_MODE_SWFIFO2     (3UL) /* Software FIFO mode */

/* ETR Control Register bit fields */
#define ETR_CTL_TRACECAPTEN_Pos (0UL)
#define ETR_CTL_TRACECAPTEN_Msk (0x1UL << ETR_CTL_TRACECAPTEN_Pos)

/* ETR Formatter and Flush Control Register (FFCR) bit fields */
#define ETR_FFCR_ENFT_Pos (0UL)
#define ETR_FFCR_ENFT_Msk (0x1UL << ETR_FFCR_ENFT_Pos)
#define ETR_FFCR_ENTI_Pos (1UL)
#define ETR_FFCR_ENTI_Msk (0x1UL << ETR_FFCR_ENTI_Pos)

/* ATB Funnel Control Register bit fields */
#define ATBFUNNEL_CTRLREG_ENS0_Pos (0UL)
#define ATBFUNNEL_CTRLREG_ENS0_Msk (0x1UL << ATBFUNNEL_CTRLREG_ENS0_Pos)
#define ATBFUNNEL_CTRLREG_ENS1_Pos (1UL)
#define ATBFUNNEL_CTRLREG_ENS1_Msk (0x1UL << ATBFUNNEL_CTRLREG_ENS1_Pos)
#define ATBFUNNEL_CTRLREG_ENS2_Pos (2UL)
#define ATBFUNNEL_CTRLREG_ENS2_Msk (0x1UL << ATBFUNNEL_CTRLREG_ENS2_Pos)
#define ATBFUNNEL_CTRLREG_ENS3_Pos (3UL)
#define ATBFUNNEL_CTRLREG_ENS3_Msk (0x1UL << ATBFUNNEL_CTRLREG_ENS3_Pos)
#define ATBFUNNEL_CTRLREG_ENS4_Pos (4UL)
#define ATBFUNNEL_CTRLREG_ENS4_Msk (0x1UL << ATBFUNNEL_CTRLREG_ENS4_Pos)
#define ATBFUNNEL_CTRLREG_ENS5_Pos (5UL)
#define ATBFUNNEL_CTRLREG_ENS5_Msk (0x1UL << ATBFUNNEL_CTRLREG_ENS5_Pos)
#define ATBFUNNEL_CTRLREG_ENS6_Pos (6UL)
#define ATBFUNNEL_CTRLREG_ENS6_Msk (0x1UL << ATBFUNNEL_CTRLREG_ENS6_Pos)
#define ATBFUNNEL_CTRLREG_ENS7_Pos (7UL)
#define ATBFUNNEL_CTRLREG_ENS7_Msk (0x1UL << ATBFUNNEL_CTRLREG_ENS7_Pos)
#define ATBFUNNEL_CTRLREG_HT_Pos   (8UL)
#define ATBFUNNEL_CTRLREG_HT_Msk   (0xFUL << ATBFUNNEL_CTRLREG_HT_Pos)

/* TSGEN Counter Control Register bit fields */
#define TSGEN_CNTCR_EN_Pos (0UL)
#define TSGEN_CNTCR_EN_Msk (0x1UL << TSGEN_CNTCR_EN_Pos)

/**
 * @brief Check if a CoreSight peripheral is locked
 *
 * @param base_addr Base address of CoreSight peripheral
 * @return true if peripheral is locked, false otherwise
 */
static inline bool coresight_is_locked(mem_addr_t base_addr)
{
	uint32_t lsr = *(volatile uint32_t *)(base_addr + CORESIGHT_LSR_OFFSET);

	return (lsr & CORESIGHT_LSR_LOCKED_Msk) != 0;
}

/**
 * @brief Unlock a CoreSight peripheral
 *
 * @param base_addr Base address of CoreSight peripheral
 * @retval 0 on success
 * @retval -EIO if unlock operation failed
 */
static inline int coresight_unlock(mem_addr_t base_addr)
{
	*(volatile uint32_t *)(base_addr + CORESIGHT_LAR_OFFSET) = CORESIGHT_UNLOCK_KEY;

	if (coresight_is_locked(base_addr)) {
		return -EIO;
	}

	return 0;
}

/**
 * @brief Lock a CoreSight peripheral
 *
 * @param base_addr Base address of CoreSight peripheral
 * @retval 0 on success
 * @retval -EIO if lock operation failed
 */
static inline int coresight_lock(mem_addr_t base_addr)
{
	/* Write any value other than unlock key to Lock Access Register to lock */
	*(volatile uint32_t *)(base_addr + CORESIGHT_LAR_OFFSET) = 0x00000000;

	if (!coresight_is_locked(base_addr)) {
		return -EIO;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* CORESIGHT_ARM_H_ */

/*******************************************************************************
 * @file  rsi_d_cache.h
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

/* Includes files */

#include "stdint.h"

#ifndef RSI_D_CACHE_H
#define RSI_D_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 ***************************  Defines / Macros  ********************************
 ******************************************************************************/

/* Cache line size */
#define DCACHE_LINE_SIZE 32

/* Memory address of the Data Cache registers */
#define M4SS_DCACHE_BASE_ADDR (0x44040000)

/* Enables the data cache */
#define DCACHE_CTRL_ENABLE   (0X1)

/* Sets the data cache to write-through mode */
#define DCACHE_CTRL_FORCE_WT (0x2)

/* Indicates if the data cache is enabled */
#define DCACHE_MAINT_STATUS_CACHE_ENABLED  (0x1)

/* Indicates if a cache enable/disable operation is ongoing */
#define DCACHE_MAINT_STATUS_ONGOING_EN_DIS (0x2)

/* Indicates if a cache maintenance operation is ongoing */
#define DCACHE_MAINT_STATUS_ONGOING_MAINT  (0x4)

/* Indicates if a power-related cache maintenance operation is ongoing */
#define DCACHE_MAINT_STATUS_ONGOING_PWR_MAINT (0x8)

/* Indicates if all data in the cache is consistent with memory */
#define DCACHE_MAINT_STATUS_CACHE_IS_CLEAN (0x100)

/* Initiates a clean operation for all cache lines */
#define DCACHE_MAINT_CTRL_ALL_TRIG_CLEAN      (0x1)

/* Initiates an invalidate operation for all cache lines */
#define DCACHE_MAINT_CTRL_ALL_TRIG_INVALIDATE (0x2)

/* Clears all pending data cache secure interrupts */
#define DCACHE_SECIRQSCLR_CLEAR_ALL (0xFF)

/* Enables the data cache statistics counter */
#define DCACHE_SECSTATCTRL_ENABLE_COUNTER (0x1)

/* Resets the data cache statistics counter to zero */
#define DCACHE_SECSTATCTRL_RESET_COUNTER  (0x2)

/* Indicates if the data cache statistics counters are saturated (reached maximum value) */
#define DCACHE_SECIRQSTAT_NSECURE_CNT_SAT (0x40)

/* Initiates a clean operation for a specific cache line */
#define DCACHE_MAINT_CTRL_LINES_TRIG_CLEAN      (0x0)

/* Initiates a invalidate operation for a specific cache line */
#define DCACHE_MAINT_CTRL_LINES_TRIG_INVALIDATE (0x1)

/* Mask to isolate the address of the cache line in a maintenance control register */
#define DCACHE_MAINT_CTRL_LINES_LOWER_ADDRESS_MASK (0x1F)

/*******************************************************************************
 ******************************   Structure    ********************************
 ******************************************************************************/

typedef struct {
	volatile uint32_t HWPRMS;
	volatile uint32_t RESV_1[3];
	volatile uint32_t CTRL;
	volatile uint32_t NSEC_ACCESS;
	volatile uint32_t RESV_2[2];
	volatile uint32_t MAINT_CTRL_ALL;
	volatile uint32_t MAINT_CTRL_LINES;
	volatile uint32_t MAINT_STATUS;
	volatile uint32_t RESV_3[53];
	volatile uint32_t SECIRQSTAT;
	volatile uint32_t SECIRQSCLR;
	volatile uint32_t SECIRQEN;
	volatile uint32_t SECIRQINFO1;
	volatile uint32_t SECIRQINFO2;
	volatile uint32_t RESV_4[11];
	volatile uint32_t NSECIRQSTAT;
	volatile uint32_t NSECIRQSCLR;
	volatile uint32_t NSECIRQEN;
	volatile uint32_t NSECIRQINFO1;
	volatile uint32_t NSECIRQINFO2;
	volatile uint32_t RESV_5[107];
	volatile uint32_t SECHIT;
	volatile uint32_t SECMISS;
	volatile uint32_t SECSTATCTRL;
	volatile uint32_t DUMMY;
	volatile uint32_t NSECHIT;
	volatile uint32_t NSECMISS;
	volatile uint32_t NSECSTATCTRL;
	volatile uint32_t RESV_6[185];
	volatile uint32_t PMSVR0;
	volatile uint32_t PMSVR1;
	volatile uint32_t PMSVR2;
	volatile uint32_t PMSVR3;
	volatile uint32_t RESV_7[28];
	volatile uint32_t PMSSSR;
	volatile uint32_t RESV_8[27];
	volatile uint32_t PMSSCR;
	volatile uint32_t PMSSRR;
	volatile uint32_t RESV_9[566];
	volatile uint32_t PIDR4;
	volatile uint32_t PIDR5;
	volatile uint32_t PIDR6;
	volatile uint32_t PIDR7;
	volatile uint32_t PIDR0;
	volatile uint32_t PIDR1;
	volatile uint32_t PIDR2;
	volatile uint32_t PIDR3;
	volatile uint32_t CIDR0;
	volatile uint32_t CIDR1;
	volatile uint32_t CIDR2;
	volatile uint32_t CIDR3;
} DCache_Reg_Type;

/*******************************************************************************
 ******************************   Prototypes    ********************************
 ******************************************************************************/

void rsi_d_cache_enable(void);
void rsi_d_cache_disable(void);
void rsi_d_cache_invalidate_all(void);
void rsi_d_cache_clean_up_all(void);
void rsi_d_cache_invalidate_address(uint32_t address);
void rsi_d_cache_clean_up_address(uint32_t address);
void rsi_d_cache_disable_stats(void);
void rsi_d_cache_enable_stats(void);
void rsi_d_cache_clear_stats(void);
int rsi_d_cache_get_stats(int *hit_count, int *miss_count);

#ifdef __cplusplus
}
#endif

#endif /* RSI_D_CACHE_H */

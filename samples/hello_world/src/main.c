/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <stdint.h>

#include <xtensa/config/core.h>
#include <xtensa/hal.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(hello_world);

#define CONFIG_TRACE 1

/* SRAM window 0 FW "registers" */
#define SRAM_REG_ROM_STATUS                     0x0
#define SRAM_REG_FW_STATUS                      0x4
#define SRAM_REG_FW_TRACEP                      0x8
#define SRAM_REG_FW_IPC_RECEIVED_COUNT          0xc
#define SRAM_REG_FW_IPC_PROCESSED_COUNT         0x10
#define SRAM_REG_FW_END                         0x14

#define HP_SRAM_BASE            0xBE000000
#define HEAP_HP_BUFFER_BASE     HP_SRAM_BASE
#define HEAP_HP_BUFFER_SIZE     0x8000
#define SRAM_WND_BASE           (HEAP_HP_BUFFER_BASE + HEAP_HP_BUFFER_SIZE)
#define SRAM_SW_REG_BASE        (SRAM_INBOX_BASE + SRAM_INBOX_SIZE)
#define MAILBOX_SW_REG_BASE     SRAM_SW_REG_BASE

#define SRAM_TRACE_BASE         SRAM_WND_BASE
#if CONFIG_TRACE
#define SRAM_TRACE_SIZE         0x2000
#else
#define SRAM_TRACE_SIZE         0
#endif
#define SRAM_DEBUG_BASE         (SRAM_TRACE_BASE + SRAM_TRACE_SIZE)
#define SRAM_DEBUG_SIZE         0x800
#define SRAM_EXCEPT_BASE        (SRAM_DEBUG_BASE + SRAM_DEBUG_SIZE)
#define SRAM_EXCEPT_SIZE        0x800
#define SRAM_STREAM_BASE        (SRAM_EXCEPT_BASE + SRAM_EXCEPT_SIZE)
#define SRAM_STREAM_SIZE        0x1000
#define SRAM_INBOX_BASE         (SRAM_STREAM_BASE + SRAM_STREAM_SIZE)
#define SRAM_INBOX_SIZE         0x2000

#define DCACHE_LINE_SIZE        XCHAL_DCACHE_LINESIZE

static inline void dcache_writeback_region(void *addr, size_t size)
{
#if XCHAL_DCACHE_SIZE > 0
	xthal_dcache_region_writeback(addr, size);
#endif
}

static inline void mailbox_sw_reg_write(size_t offset, uint32_t src)
{
	*((volatile uint32_t*)(MAILBOX_SW_REG_BASE + offset)) = src;
	dcache_writeback_region((void *)(MAILBOX_SW_REG_BASE + offset),
                                sizeof(src));
}

/* Platform defined trace code */
#define platform_trace_point(__x) \
        mailbox_sw_reg_write(SRAM_REG_FW_TRACEP, (__x))


#define SHIM_BASE             0x00001000

static inline uint32_t shim_read(uint32_t reg)
{
        return *((volatile uint32_t*)(SHIM_BASE + reg));
}

static inline void shim_write(uint32_t reg, uint32_t val)
{
        *((volatile uint32_t*)(SHIM_BASE + reg)) = val;
}

void main(void)
{
	LOG_INF("Hello World!");

	mailbox_sw_reg_write(SRAM_REG_ROM_STATUS, 0xabbac0fe);
#if 0
	platform_trace_point(0xabbac0ffe);
#endif
}

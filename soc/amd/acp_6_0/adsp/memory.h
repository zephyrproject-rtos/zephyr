/*
 * Copyright(c) 2022 AMD
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author:      Basavaraj Hiregoudar <basavaraj.hiregoudar@amd.com>
 * DineshKumar Kalva <dineshkumar.kalva@amd.com>
 */
#ifndef ZEPHYR_SOC_AMD_ADSP_MEMORY_H_
#define ZEPHYR_SOC_AMD_ADSP_MEMORY_H_

#define PLATFORM_CORE_COUNT      1
#define PLATFORM_PRIMARY_CORE_ID 0

#define IRAM_BASE 0x7F000000
#define IRAM_SIZE 0x60000

#define IRAM_RESERVE_HEADER_SPACE 0x400

#define MEM_RESET_TEXT_SIZE             0x400
#define MEM_RESET_LIT_SIZE              0x8
#define XCHAL_RESET_VECTOR_PADDR_IRAM   0x7F000000
#define XCHAL_WINDOW_VECTORS_PADDR_IRAM 0x7F000400

#define XCHAL_VECBASE_RESET_PADDR_IRAM (IRAM_BASE + IRAM_RESERVE_HEADER_SPACE)

#define MEM_VECBASE_LIT_SIZE 0x178
#define MEM_WIN_TEXT_SIZE    0x178

/* Vector and literal sizes - not in core-isa.h */
#define MEM_VECT_LIT_SIZE  0x7
#define MEM_VECT_TEXT_SIZE 0x37

#define XCHAL_INTLEVEL2_VECTOR_PADDR_IRAM (XCHAL_VECBASE_RESET_PADDR_IRAM + 0x180)

#define XCHAL_INTLEVEL3_VECTOR_PADDR_IRAM (XCHAL_VECBASE_RESET_PADDR_IRAM + 0x1C0)

#define XCHAL_INTLEVEL4_VECTOR_PADDR_IRAM (XCHAL_VECBASE_RESET_PADDR_IRAM + 0x200)

#define XCHAL_INTLEVEL5_VECTOR_PADDR_IRAM (XCHAL_VECBASE_RESET_PADDR_IRAM + 0x240)

#define XCHAL_INTLEVEL6_VECTOR_PADDR_IRAM (XCHAL_VECBASE_RESET_PADDR_IRAM + 0x280)

#define XCHAL_INTLEVEL7_VECTOR_PADDR_IRAM (XCHAL_VECBASE_RESET_PADDR_IRAM + 0x2C0)

#define XCHAL_KERNEL_VECTOR_PADDR_IRAM (XCHAL_VECBASE_RESET_PADDR_IRAM + 0x300)

#define XCHAL_USER_VECTOR_PADDR_IRAM (XCHAL_VECBASE_RESET_PADDR_IRAM + 0x340)

#define XCHAL_DOUBLEEXC_VECTOR_PADDR_IRAM (XCHAL_VECBASE_RESET_PADDR_IRAM + 0x3C0)

/* Location for the intList section which is later used to construct the
 * Interrupt Descriptor Table (IDT). This is a bogus address as this
 * section will be stripped off in the final image.
 */
#define IDT_BASE               (IRAM_BASE + IRAM_SIZE)
/* size of the Interrupt Descriptor Table (IDT) */
#define IDT_SIZE               0x2000
/* physical DSP addresses */
#define IRAM_BASE              0x7F000000
#define IRAM_SIZE              0x60000    /* 384K */
#define SRAM0_BASE             0x9FF00000 /* Scratch mem */
#define SRAM1_BASE             0x60006000
#define SRAM1_SIZE             0x80000 /* 256K Data Mem */
#define DRAM0_BASE             0xE0000000
#define DRAM0_SIZE             0x20000 /* 128K ,to use for heap mem */
#define DMA0_BASE              PU_REGISTER_BASE
#define DMA0_SIZE              0x4
#define PU_REGISTER_BASE       (0x9FD00000 - 0x01240000)
#define ACP_I2S_RX_RINGBUFADDR 0x1242000
/* DAI DMA register base address */
#define DAI_BASE               (PU_REGISTER_BASE + ACP_I2S_RX_RINGBUFADDR)
#define DAI_BASE_REM           (PU_REGISTER_BASE + ACP_P1_I2S_RX_RINGBUFADDR)
#define DAI_SIZE               0x4
#define BT_TX_FIFO_OFFST       (ACP_P1_BT_TX_FIFOADDR - ACP_P1_I2S_RX_RINGBUFADDR)
#define BT_RX_FIFO_OFFST       (ACP_P1_BT_RX_FIFOADDR - ACP_P1_I2S_RX_RINGBUFADDR)
#define HS_TX_FIFO_OFFST       (ACP_P1_HS_TX_FIFOADDR - ACP_P1_I2S_RX_RINGBUFADDR)
#define HS_RX_FIFO_OFFST       (ACP_P1_HS_TX_FIFOADDR - ACP_P1_I2S_RX_RINGBUFADDR)
#define UUID_ENTRY_ELF_BASE    0x1FFFA000
#define UUID_ENTRY_ELF_SIZE                                                                        \
	0x6000 /* Log buffer  base need to be updated properly, these are used in linker scripts   \
		*/
#define LOG_ENTRY_ELF_BASE    0x20000000
#define LOG_ENTRY_ELF_SIZE    0x2000000
#define EXT_MANIFEST_ELF_BASE (LOG_ENTRY_ELF_BASE + LOG_ENTRY_ELF_SIZE)
#define EXT_MANIFEST_ELF_SIZE 0x2000000 /* Stack configuration */
#define SOF_STACK_SIZE        0x1000
#define SOF_STACK_TOTAL_SIZE  SOF_STACK_SIZE
#define SOF_STACK_END         (DRAM0_BASE + DRAM0_SIZE)
#define SOF_STACK_BASE        (SOF_STACK_END + SOF_STACK_SIZE) /* Mailbox configuration */
#define SRAM_OUTBOX_BASE      SRAM0_BASE
#define SRAM_OUTBOX_SIZE      0x400
#define SRAM_OUTBOX_OFFSET    0
#define SRAM_INBOX_BASE       (SRAM_OUTBOX_BASE + SRAM_OUTBOX_SIZE)
#define SRAM_INBOX_SIZE       0x400
#define SRAM_INBOX_OFFSET     SRAM_OUTBOX_SIZE
#define SRAM_DEBUG_BASE       (SRAM_INBOX_BASE + SRAM_INBOX_SIZE)
#define SRAM_DEBUG_SIZE       0x400
#define SRAM_DEBUG_OFFSET     (SRAM_INBOX_OFFSET + SRAM_INBOX_SIZE)
#define SRAM_EXCEPT_BASE      (SRAM_DEBUG_BASE + SRAM_DEBUG_SIZE)
#define SRAM_EXCEPT_SIZE      0x400
#define SRAM_EXCEPT_OFFSET    (SRAM_DEBUG_OFFSET + SRAM_DEBUG_SIZE)
#define SRAM_STREAM_BASE      (SRAM_EXCEPT_BASE + SRAM_EXCEPT_SIZE)
#define SRAM_STREAM_SIZE      0x400
#define SRAM_STREAM_OFFSET    (SRAM_EXCEPT_OFFSET + SRAM_EXCEPT_SIZE)
#define SRAM_TRACE_BASE       (SRAM_STREAM_BASE + SRAM_STREAM_SIZE)
#define SRAM_TRACE_SIZE       0x400
#define SRAM_TRACE_OFFSET     (SRAM_STREAM_OFFSET + SRAM_STREAM_SIZE)
#define SOF_MAILBOX_SIZE                                                                           \
	(SRAM_INBOX_SIZE + SRAM_OUTBOX_SIZE + SRAM_DEBUG_SIZE + SRAM_EXCEPT_SIZE +                 \
	 SRAM_STREAM_SIZE + SRAM_TRACE_SIZE)
/* Heap section sizes for module pool */
#define HEAP_RT_COUNT8        0
#define HEAP_RT_COUNT16       48
#define HEAP_RT_COUNT32       48
#define HEAP_RT_COUNT64       32
#define HEAP_RT_COUNT128      60
#define HEAP_RT_COUNT256      32
#define HEAP_RT_COUNT512      4
#define HEAP_RT_COUNT1024     12
#define HEAP_RT_COUNT2048     12
/* Heap section sizes for system runtime heap */
#define HEAP_SYS_RT_COUNT64   64
#define HEAP_SYS_RT_COUNT512  20 /*rembrandt-arch*/
#define HEAP_SYS_RT_COUNT1024 6
/* Heap configuration */
#define HEAP_SYSTEM_BASE      DRAM0_BASE /* SRAM1_BASE */
#define HEAP_SYSTEM_SIZE      0xE000
#define HEAP_SYSTEM_0_BASE    HEAP_SYSTEM_BASE
#define HEAP_SYS_RUNTIME_BASE (HEAP_SYSTEM_BASE + HEAP_SYSTEM_SIZE)
#define HEAP_SYS_RUNTIME_SIZE                                                                      \
	(HEAP_SYS_RT_COUNT64 * 64 + HEAP_SYS_RT_COUNT512 * 512 + HEAP_SYS_RT_COUNT1024 * 1024)
#define HEAP_RUNTIME_BASE (HEAP_SYS_RUNTIME_BASE + HEAP_SYS_RUNTIME_SIZE)
#define HEAP_RUNTIME_SIZE                                                                          \
	(HEAP_RT_COUNT8 * 8 + HEAP_RT_COUNT16 * 16 + HEAP_RT_COUNT32 * 32 + HEAP_RT_COUNT64 * 64 + \
	 HEAP_RT_COUNT128 * 128 + HEAP_RT_COUNT256 * 256 + HEAP_RT_COUNT512 * 512 +                \
	 HEAP_RT_COUNT1024 * 1024 + HEAP_RT_COUNT2048 * 2048)
#define HEAP_BUFFER_BASE                 (HEAP_RUNTIME_BASE + HEAP_RUNTIME_SIZE)
#define HEAP_BUFFER_SIZE                 (0xF000)
#define HEAP_BUFFER_BLOCK_SIZE           0x180
#define HEAP_BUFFER_COUNT                (HEAP_BUFFER_SIZE / HEAP_BUFFER_BLOCK_SIZE)
#define PLATFORM_HEAP_SYSTEM             1
#define PLATFORM_HEAP_SYSTEM_RUNTIME     1
#define PLATFORM_HEAP_RUNTIME            1
#define PLATFORM_HEAP_BUFFER             1
/* Vector and literal sizes - not in core-isa.h */
#define SOF_MEM_VECT_LIT_SIZE            0x7
#define SOF_MEM_VECT_TEXT_SIZE           0x37
#define SOF_MEM_VECT_SIZE                (SOF_MEM_VECT_TEXT_SIZE + SOF_MEM_VECT_LIT_SIZE)
#define SOF_MEM_RESET_TEXT_SIZE          0x400
#define SOF_MEM_RESET_LIT_SIZE           0x8
#define SOF_MEM_VECBASE_LIT_SIZE         0x178
#define SOF_MEM_WIN_TEXT_SIZE            0x178
#define SOF_MEM_RO_SIZE                  0x8
#define uncache_to_cache(address)        address
#define cache_to_uncache(address)        address
#define is_uncached(address)             0
#define HEAP_BUF_ALIGNMENT               PLATFORM_DCACHE_ALIGN
/* brief EDF task's default stack size in bytes */
#define PLATFORM_TASK_DEFAULT_STACK_SIZE 3072
#endif /* ZEPHYR_SOC_AMD_ADSP_MEMORY_H_ */

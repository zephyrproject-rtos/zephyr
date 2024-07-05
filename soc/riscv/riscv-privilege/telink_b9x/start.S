/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NDS_MCACHE_CTL               0x7ca
#define NDS_MMISC_CTL                0x7d0

#include <zephyr/toolchain.h>

	.option push
	.option norelax

/* exports */
GTEXT(entry)

SECTION_FUNC(init, init)
	.org 0x0
entry:

	j start

	.org 0x20
	.word ('T' << 24 | 'L' << 16 | 'N' << 8 | 'K')

#if CONFIG_SOC_RISCV_TELINK_B91
	.org 0x26
#if defined(CONFIG_TELINK_B9X_2_WIRE_SPI_ENABLE) && CONFIG_TELINK_B9X_2_WIRE_SPI_ENABLE
	.short (0x173b)                 #/ 2 - wire SPI interface
#else
	.short (0x65eb)                 #/ 4 - wire SPI interface
#endif

#elif CONFIG_SOC_RISCV_TELINK_B92
	.org 0x24
#if defined(CONFIG_TELINK_B9X_2_WIRE_SPI_ENABLE) && CONFIG_TELINK_B9X_2_WIRE_SPI_ENABLE
	.word (0x3b0097a9)              #/ 2 - wire SPI interface
#else
	.word (0x3b0097a9)              #/ 4 - wire SPI interface set in soc_b9x_check_flash
#endif

#elif CONFIG_SOC_RISCV_TELINK_B95
	.org 0x24
#if defined(CONFIG_TELINK_B9X_2_WIRE_SPI_ENABLE) && CONFIG_TELINK_B9X_2_WIRE_SPI_ENABLE
	.word (0x030020A8)              #/ 2 - wire SPI interface
#else
	.word (0x030020A8)              #/ 4 - wire SPI interface set in soc_b9x_check_flash
#endif

#endif

	.align 2

start:

#if defined(CONFIG_TELINK_B91_USB_SWI_DISABLE) && CONFIG_TELINK_B91_USB_SWI_DISABLE
	/* USB SWI disable: 0x80100c01 <- 0x40 */
	lui    t0, 0x80100
	addi   t0, t0, 0x700
	li     t1, 0x40
	sb     t1, 0x501(t0)
#endif

	/* Disable vector mode */
	csrci  0x7d0, 2
	/* Disable vector feture enable address */
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
	lui t0 ,0xe4000
#elif CONFIG_SOC_RISCV_TELINK_B95
	lui t0 ,0xc4000
#endif
	li  t1 ,0x00
	sw  t1 ,0x0(t0)

	/* Enable I/D-Cache */
	csrr   t0, NDS_MCACHE_CTL
	ori    t0, t0, 1                #/ I-Cache
	ori    t0, t0, 2                #/ D-Cache
	csrw   NDS_MCACHE_CTL, t0
	fence.i

	/* Enable misaligned access and non-blocking load */
	li     t0, (1 << 8) | (1 << 6)
	csrs   NDS_MMISC_CTL, t0

#ifdef CONFIG_TELINK_B9X_MATTER_RETENTION_LAYOUT
_RAMCODE_BLE_INIT:
	la     t1, _RAMCODE_BLE_LMA_START
	la     t2, _RAMCODE_BLE_VMA_START
	la     t3, _RAMCODE_BLE_VMA_END
_RAMCODE_BLE_INIT_BEGIN:
	bleu   t3, t2, _RAM_DLM_INIT
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RAMCODE_BLE_INIT_BEGIN

_RAM_DLM_INIT:
	la     t1, _RAM_DLM_LMA_START
	la     t2, _RAM_DLM_VMA_START
	la     t3, _RAM_DLM_VMA_END
_RAM_DLM_BEGIN:
	bleu   t3, t2, _RETENTION_RESET_INIT
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RAM_DLM_BEGIN
#endif /* CONFIG_TELINK_B9X_MATTER_RETENTION_LAYOUT */

_RETENTION_RESET_INIT:
	la     t1, _RETENTION_RESET_LMA_START
	la     t2, _RETENTION_RESET_VMA_START
	la     t3, _RETENTION_RESET_VMA_END
_RETENTION_RESET_BEGIN:
	bleu   t3, t2, _ZERO_AES
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RETENTION_RESET_BEGIN

_ZERO_AES:
	lui    t0, 0
	la     t2, _AES_DATA_VMA_START
	la     t3, _AES_DATA_VMA_END
_ZERO_AES_BEGIN:
	bleu   t3, t2, _RETENTION_DATA_INIT
	sw     t0, 0(t2)
	addi   t2, t2, 4
	j      _ZERO_AES_BEGIN

_RETENTION_DATA_INIT:
	la     t1, _RETENTION_DATA_LMA_START
	la     t2, _RETENTION_DATA_VMA_START
	la     t3, _RETENTION_DATA_VMA_END
_RETENTION_DATA_INIT_BEGIN:
	bleu   t3, t2, _RAMCODE_RET_INIT
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RETENTION_DATA_INIT_BEGIN

_RAMCODE_RET_INIT:
	la     t1, _RAMCODE_RET_LMA_START
	la     t2, _RAMCODE_RET_VMA_START
	la     t3, _RAMCODE_RET_VMA_END
_RAMCODE_RET_INIT_BEGIN:
	bleu   t3, t2, _RAMCODE_INIT
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RAMCODE_RET_INIT_BEGIN

_RAMCODE_INIT:
	la     t1, _RAMCODE_LMA_START
	la     t2, _RAMCODE_VMA_START
	la     t3, _RAMCODE_VMA_END
_RAMCODE_INIT_BEGIN:
	bleu   t3, t2, _START
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RAMCODE_INIT_BEGIN

_START:
	j      __start


#if CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION

/* exports */
GTEXT(retention_entry)

SECTION_FUNC(retention_reset, retention_reset)
	.org 0x0
retention_entry:

	j retention_start

	.org 0x22

retention_start:

#if defined(CONFIG_TELINK_B91_USB_SWI_DISABLE) && CONFIG_TELINK_B91_USB_SWI_DISABLE
	/* USB SWI disable: 0x80100c01 <- 0x40 */
	lui    t0, 0x80100
	addi   t0, t0, 0x700
	li     t1, 0x40
	sb     t1, 0x501(t0)
#endif

	/* Enable I/D-Cache */
	csrr   t0, NDS_MCACHE_CTL
	ori    t0, t0, 1                #/ I-Cache
	ori    t0, t0, 2                #/ D-Cache
	csrw   NDS_MCACHE_CTL, t0
	fence.i

	/* Enable misaligned access and non-blocking load */
	li     t0, (1 << 8) | (1 << 6)
	csrs   NDS_MMISC_CTL, t0

	/* flash wakeup */
#if CONFIG_SOC_RISCV_TELINK_B91
_WAKEUP_FLASH:
	lui    t0, 0x80140
	li     t1, 0xff
	li     t2, 0x0
	li     t3, 0xab
	sb     t1, 0x329(t0)            #/ mspi ie enable   : 0x140329 : 0x1f
	sb     t2, 0x101(t0)            #/ cs_low           : 0x140101 : 0x00
	sb     t3, 0x100(t0)            #/ wakeup_cmd       : 0x140100 : 0xab
_MSPI_WAIT:
	lui    t0, 0x80140
	lb     t2, 0x102 (t0)           #/ read reg_mspi_status FLD_MSPI_BUSY(bit0)
	li     t3, 0x1
	li     t4, 0x10
	beq    t3, t2, _MSPI_WAIT
	sb     t4, 0x101(t0)            #/ cs_high           : 0x140101 : 0x10
	/* efuse load need delay about 18us */
	li     t0, 0
	li     t1, 226
_WAIT_EFUSE_LOAD_FINISH:
	addi   t0, t0, 1
	bgeu   t1, t0, _WAIT_EFUSE_LOAD_FINISH

_MULTI_ADDRESS_BEGIN:
	lui    t0, 0x80140
	la     t1, g_pm_multi_addr
	lw     t2, 0(t1)
	sw     t2, 0x104(t0)            #/ g_pm_multi_addr->0x80140104
#elif CONFIG_SOC_RISCV_TELINK_B92 || CONFIG_SOC_RISCV_TELINK_B95
	/* we are not in ISR so use ISR stack to restore registers */
	la     sp, z_interrupt_stacks
	li     t0, CONFIG_ISR_STACK_SIZE
	add    sp, sp, t0
	la     t0, pm_retention_register_recover
	jalr   t0

	lui    t0, 0xa3fff
	addi   t0, t0, 0x790
	li     t1, 0x0e
	sb     t1, 0x78c(t0)            #/ xip_stop          :0xa3ffff1c = 0x0e
_MSPI_WAIT_1:
	lui     t0, 0xa3fff
	addi    t0, t0, 0x790
	lb      t2, 0x798(t0)
	li      t3, 0x80
	beq     t3, t2, _MSPI_WAIT_1    #/ read reg_mspi_status FLD_MSPI_BUSY(0xA3FFFF28[bit7])
	lui     t0, 0xa3fff
	addi    t0, t0, 0x790
	li      t1, 0x06
	sb      t1, 0x78c(t0)           #/ xip_disable       :0xa3ffff1c = 0x06
	lui     t0, 0x80140
	li      t1, 0x3f
	sb      t1, 0x331(t0)           #/ mspi ie enable    :0x80140331 = 0x3f
	lui     t0, 0xa3fff
	addi    t0, t0, 0x790
	li      t2, 0x80
	sb      t2, 0x778(t0)           #/ mspi_ctrl1        :0xa3ffff08 = 0x80
	li      t3, 0xab
	sb      t3, 0x774(t0)           #/ wakeup_cmd        :0xa3ffff04 = 0xab
_MSPI_WAIT_2:
	lui     t0, 0xa3fff
	addi    t0, t0, 0x790
	lb      t2, 0x798(t0)
	li      t3, 0x80
	beq     t3, t2, _MSPI_WAIT_2    #/ read reg_mspi_status FLD_MSPI_BUSY(0xa3ffff28[bit7])
	lui     t0, 0xa3fff
	addi    t0, t0, 0x790
	li      t1, 0x0a
	sb      t1, 0x78c(t0)           #/ xip_enable        :0xa3ffff1c = 0x0a
	/* flash wakeup need delay about 20us, efuse load need delay about 0us */
	li      t0, 0
	li      t1, 320                 #/ 25us
_WAIT_EFUSE_LOAD_AND_FLASH_WAKEUP_FINISH:
	addi    t0, t0, 1
	bgeu    t1, t0, _WAIT_EFUSE_LOAD_AND_FLASH_WAKEUP_FINISH

_MULTI_ADDRESS_BEGIN:
	lui     t0, 0xa3fff
	addi    t0, t0, 0x790
	la      t1, g_pm_mspi_cfg
	lw      t2, 0(t1)
	sw      t2, 0x790(t0)           #/ g_pm_mspi_cfg->0xa3ffff20
#endif /* flash wakeup chip dependent */

#ifdef CONFIG_SOC_SERIES_RISCV_TELINK_B9X_NON_RETENTION_RAM_CODE
_IRAMCODE_INIT:
	la     t1, _RAMCODE_LMA_START
	la     t2, _RAMCODE_VMA_START
	la     t3, _RAMCODE_VMA_END
_IRAMCODE_INIT_BEGIN:
	bleu   t3, t2, _RESTORE
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _IRAMCODE_INIT_BEGIN
#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_B9X_NON_RETENTION_RAM_CODE */

_RESTORE:
	j      tl_context_restore

#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION */

	.option pop

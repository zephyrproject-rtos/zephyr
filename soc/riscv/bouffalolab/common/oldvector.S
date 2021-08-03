/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <arch/riscv/csr.h>

#define __metal_chicken_bit 0

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(__irq_wrapper)

SECTION_FUNC(exception, ivt)
	.option norvc

	/* Interrupts */
	j __irq_wrapper	/* IRQ 0                                    */
	j __irq_wrapper	/* IRQ 1                                    */
	j __irq_wrapper	/* IRQ 2                                    */
	j __irq_wrapper	/* IRQ 3           - clic_msip              */
	j __irq_wrapper	/* IRQ 4                                    */
	j __irq_wrapper	/* IRQ 5                                    */
	j __irq_wrapper	/* IRQ 6                                    */
	j __irq_wrapper	/* IRQ 7           - clic_mtimer            */
	j __irq_wrapper	/* IRQ 8           - ?                      */
	j __irq_wrapper	/* IRQ 9           - ?                      */
	j __irq_wrapper	/* IRQ 10          - ?                      */
	j __irq_wrapper	/* IRQ 11          - clic_mext              */
	j __irq_wrapper	/* IRQ 12          - clic_csoft             */
	j __irq_wrapper	/* IRQ 13          - ?                      */
	j __irq_wrapper	/* IRQ 14          - Debug interrupt        */
	j __irq_wrapper	/* IRQ 15                                   */

	/* CLIC Local Interrupts */
	j __irq_wrapper	/* IRQ 16 + 0      - BMX_ERR                */
	j __irq_wrapper	/* IRQ 16 + 1      - BMX_TO                 */
	j __irq_wrapper	/* IRQ 16 + 2      - L1C_BMX_ERR            */
	j __irq_wrapper	/* IRQ 16 + 3      - L1C_BMX_TO             */
	j __irq_wrapper	/* IRQ 16 + 4      - SEC_BMX_ERR            */
	j __irq_wrapper	/* IRQ 16 + 5      - RF_TOP_INT0            */
	j __irq_wrapper	/* IRQ 16 + 6      - RF_TOP_INT1            */
	j __irq_wrapper	/* IRQ 16 + 7      - SDIO                   */
	j __irq_wrapper	/* IRQ 16 + 8      - DMA_BMX_ERR            */
	j __irq_wrapper	/* IRQ 16 + 9      - SEC_GMAC               */
	j __irq_wrapper	/* IRQ 16 + 10     - SEC_CDET               */
	j __irq_wrapper	/* IRQ 16 + 11     - SEC_PKA                */
	j __irq_wrapper	/* IRQ 16 + 12     - SEC_TRNG               */
	j __irq_wrapper	/* IRQ 16 + 13     - SEC_AES                */
	j __irq_wrapper	/* IRQ 16 + 14     - SEC_SHA                */
	j __irq_wrapper	/* IRQ 16 + 15     - DMA_ALL                */
	j __irq_wrapper	/* IRQ 16 + 16                              */
	j __irq_wrapper	/* IRQ 16 + 17                              */
	j __irq_wrapper	/* IRQ 16 + 18                              */
	j __irq_wrapper	/* IRQ 16 + 19     - IRTX                   */
	j __irq_wrapper	/* IRQ 16 + 20     - IRRX                   */
	j __irq_wrapper	/* IRQ 16 + 21                              */
	j __irq_wrapper	/* IRQ 16 + 22                              */
	j __irq_wrapper	/* IRQ 16 + 23     - SF_CTRL                */
	j __irq_wrapper	/* IRQ 16 + 24                              */
	j __irq_wrapper	/* IRQ 16 + 25     - GPADC_DMA              */
	j __irq_wrapper	/* IRQ 16 + 26     - EFUSE                  */
	j __irq_wrapper	/* IRQ 16 + 27     - SPI                    */
	j __irq_wrapper	/* IRQ 16 + 28                              */
	j __irq_wrapper	/* IRQ 16 + 29     - UART0                  */
	j __irq_wrapper	/* IRQ 16 + 30     - UART1                  */
	j __irq_wrapper	/* IRQ 16 + 31                              */
	j __irq_wrapper	/* IRQ 16 + 32     - I2C                    */
	j __irq_wrapper	/* IRQ 16 + 33                              */
	j __irq_wrapper	/* IRQ 16 + 34     - PWM                    */
	j __irq_wrapper	/* IRQ 16 + 35                              */
	j __irq_wrapper	/* IRQ 16 + 36     - TIMER_CH0              */
	j __irq_wrapper	/* IRQ 16 + 37     - TIMER_CH1              */
	j __irq_wrapper	/* IRQ 16 + 38     - TIMER_WDT              */
	j __irq_wrapper	/* IRQ 16 + 39                              */
	j __irq_wrapper	/* IRQ 16 + 40                              */
	j __irq_wrapper	/* IRQ 16 + 41                              */
	j __irq_wrapper	/* IRQ 16 + 42                              */
	j __irq_wrapper	/* IRQ 16 + 43                              */
	j __irq_wrapper	/* IRQ 16 + 44     - GPIO_INT0              */
	j __irq_wrapper	/* IRQ 16 + 45                              */
	j __irq_wrapper	/* IRQ 16 + 46                              */
	j __irq_wrapper	/* IRQ 16 + 47                              */
	j __irq_wrapper	/* IRQ 16 + 48                              */
	j __irq_wrapper	/* IRQ 16 + 49                              */
	j __irq_wrapper	/* IRQ 16 + 50     - PDS_WAKEUP             */
	j __irq_wrapper	/* IRQ 16 + 51     - HBN_OUT0               */
	j __irq_wrapper	/* IRQ 16 + 52     - HBN_OUT1               */
	j __irq_wrapper	/* IRQ 16 + 53     - BOR                    */
	j __irq_wrapper	/* IRQ 16 + 54     - WIFI                   */
	j __irq_wrapper	/* IRQ 16 + 55     - BZ_PHY                 */
	j __irq_wrapper	/* IRQ 16 + 56     - BLE                    */
	j __irq_wrapper	/* IRQ 16 + 57     - MAC_TXRX_TIMER         */
	j __irq_wrapper	/* IRQ 16 + 58     - MAC_TXRX_MISC          */
	j __irq_wrapper	/* IRQ 16 + 59     - MAC_RX_TRG             */
	j __irq_wrapper	/* IRQ 16 + 60     - MAC_TX_TRG             */
	j __irq_wrapper	/* IRQ 16 + 61     - MAC_GEN                */
	j __irq_wrapper	/* IRQ 16 + 62     - MAC_PORT               */
	j __irq_wrapper	/* IRQ 16 + 63     - WIFI_IPC               */

SECTION_FUNC(vectors, __start)
	.cfi_startproc

	.option norvc

	/* Inform the debugger that there is nowhere to backtrace */
	.cfi_undefined ra

	/* Disable interrupts */
	li t0, MSTATUS_MIE
	csrc mstatus, t0

	/* The absolute first thing that must happen is configuring the global
	 * pointer register, which must be done with relaxation disabled
	 * because it's not valid to obtain the address of any symbol without
	 * GP configured.  The C environment might go ahead and do this again,
	 * but that's safe as it's a fixed register. */
	.option push
	.option norelax
		la gp, __data_global_pointer
	.option pop

	/* Set mtvec (Machine Trap-Vector Base-Address Register) */
	la t0, __irq_wrapper

	/* enable CLIC Vectored mode */
	/* ori t0, t0, 3 */
	/* enable CLINT Direct mode */
	ori t0, t0, 0
	csrw mtvec, t0

	/* enable chicken bit if core is bullet series*/
	la t0, __metal_chicken_bit
	beqz t0, 1f
	csrwi 0x7C1, 0
1:

	/* Check for an initialization routine and call it if one exists,
	 * otherwise just skip over the call entirely.   Note that
	 * __metal_initialize isn't actually a full C function, as it doesn't
	 * end up with the .bss or .data segments having been initialized.
	 * This is done to avoid putting a burden on systems that can be
	 * initialized without having a C environment set up.*/
	call SystemInit

	/* start load code to itcm like.*/
	call start_load

	/* Jump to __initialize */
	tail __initialize

	.cfi_endproc

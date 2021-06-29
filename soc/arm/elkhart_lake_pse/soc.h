/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BOARD__H_
#define _BOARD__H_

#ifdef __cplusplus
extern "C" {
#endif

/* default system clock */

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(12)

/* IRQs */

#define IRQ_GPIO_PORTA 0
#define IRQ_GPIO_PORTB 1
#define IRQ_GPIO_PORTC 2
#define IRQ_GPIO_PORTD 3
#define IRQ_GPIO_PORTE 4
#define IRQ_SSI0 7
#define IRQ_I2C0 8
#define IRQ_PWM_FAULT 9
#define IRQ_PWM_GEN0 10
#define IRQ_PWM_GEN1 11
#define IRQ_PWM_GEN2 12
#define IRQ_QEI0 13
#define IRQ_ADC0_SEQ0 14
#define IRQ_ADC0_SEQ1 15
#define IRQ_ADC0_SEQ2 16
#define IRQ_ADC0_SEQ3 17
#define IRQ_WDOG0 18
#define IRQ_TIMER0A 19
#define IRQ_TIMER0B 20
#define IRQ_TIMER1A 21
#define IRQ_TIMER1B 22
#define IRQ_TIMER2A 23
#define IRQ_TIMER2B 24
#define IRQ_ANALOG_COMP0 25
#define IRQ_ANALOG_COMP1 26
#define IRQ_RESERVED0 27
#define IRQ_SYS_CONTROL 28
#define IRQ_FLASH_MEM_CTRL 29
#define IRQ_GPIO_PORTF 30
#define IRQ_GPIO_PORTG 31
#define IRQ_RESERVED1 32
#define IRQ_RESERVED2 34
#define IRQ_TIMER3A 35
#define IRQ_TIMER3B 36
#define IRQ_I2C1 37
#define IRQ_QEI1 38
#define IRQ_RESERVED3 39
#define IRQ_RESERVED4 40
#define IRQ_RESERVED5 41
#define IRQ_ETH 42
#define IRQ_HIBERNATION 43

#ifndef _ASMLANGUAGE

typedef enum IRQn {
	NonMaskableInt_IRQn     = -14,  /*  2 Non Maskable Interrupt          */
	HardFault_IRQn          = -13,  /*  3 HardFault Interrupt             */
	MemoryManagement_IRQn   = -12,  /*  4 Memory Management Interrupt     */
	BusFault_IRQn           = -11,  /*  5 Bus Fault Interrupt             */
	UsageFault_IRQn         = -10,  /*  6 Usage Fault Interrupt           */
	SVCall_IRQn             = -5,   /* 11 SV Call Interrupt               */
	DebugMonitor_IRQn       = -4,   /* 12 Debug Monitor Interrupt         */
	PendSV_IRQn             = -2,   /* 14 Pend SV Interrupt               */
	SysTick_IRQn            = -1,   /* 15 System Tick Interrupt           */
} IRQn_Type;
#endif
#define __MPU_PRESENT 1
#ifndef _ASMLANGUAGE
#define __FPU_PRESENT 1
#define __NVIC_PRIO_BITS 3              /* Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig  1       /* Set to 1 if different SysTick Config is used */

#define __FPU_DP 1U                     /* double precision FPU */

#ifndef CONFIG_BOARD_SIMICS             /* No cache support on Simics */
#if (!CONFIG_CACHE_DISABLE)
#define __ICACHE_PRESENT 1U
#define __DCACHE_PRESENT 1U
#endif  /* CONFIG_CACHE_DISABLE */
#endif  /* CONFIG_BOARD_SIMICS */

#define __DTCM_PRESENT 1U
#include <devicetree.h>

/* Start Address of ICCM, DCCM and L2SRAM Memories */
#define ICCM_MEM_START_ADDR    (0x0000000)
#define DCCM_MEM_START_ADDR    (0x20000000)

/* uart configuration settings */
#if defined(CONFIG_UART_STELLARIS)

#define UART_STELLARIS_CLK_FREQ SYSCLK_DEFAULT_IOSC_HZ

#endif /* CONFIG_UART_STELLARIS */

/* Misc Config Base Address */
#define MISC_ISH_MISC_CONFIG_BASE_ADDR  0x40700000

/* SRAM Region for address protection */
#define MISC_SRAM_PROTECTION_BASE(n) \
	(MISC_ISH_MISC_CONFIG_BASE_ADDR + 0x400 + 8 * ((n) - 1))
#define MISC_SRAM_PROTECTION_SIZE(n) \
	(MISC_ISH_MISC_CONFIG_BASE_ADDR + 0x404 + 8 * ((n) - 1))
#define MISC_SRAM_SIZE_PRG_OFF (3)
#define MISC_SRAM_SIZE_PRG_MASK (0x1F)
#define MISC_SRAM_SIZE_PRG_UNIT ((uint32_t)(0x400))

/* prg_size = (((2^(prg - 1) * (2^10)),  prg_size should be equal
 * or just larger than size
 */
static inline uint32_t MISC_SRAM_SIZE_TO_PRG_SIZE(uint32_t size)
{
	uint32_t prg;

	if (!size) {
		return 0;
	} else if (size <= MISC_SRAM_SIZE_PRG_UNIT) {
		size = 1;
	} else {
		size = (size + (MISC_SRAM_SIZE_PRG_UNIT - 1)) /
		       MISC_SRAM_SIZE_PRG_UNIT;
	}

	for (prg = 1; prg < MISC_SRAM_SIZE_PRG_MASK; prg++) {
		if (((uint32_t)0x1 << (prg - 1)) >= size) {
			break;
		}
	}

	return (prg << MISC_SRAM_SIZE_PRG_OFF);
}

#define MISC_SRAM_PROTECTION_REGION_1 (1)
#define MISC_SRAM_PROTECTION_REGION_2 (2)
#define MISC_SRAM_PROTECTION_REGION_3 (3)
#define MISC_SRAM_PROTECTION_REGION_4 (4)

/* ATT Config Base Address */
#define ATT_ISH_ATT_CONFIG_BASE_ADDR    0x40E00000

/* ATT Register */
#define ATT_MMIO_BASE(n) \
	(ATT_ISH_ATT_CONFIG_BASE_ADDR  + 0x10 * (n))
#define ATT_MMIO_LIMIT(n) \
	(ATT_ISH_ATT_CONFIG_BASE_ADDR  + 0x10 * (n) + 0x4)
#define ATT_OCP_XLATE_OFFSET(n)	\
	(ATT_ISH_ATT_CONFIG_BASE_ADDR  + 0x10 * (n) + 0x8)
#define ATT_OCP_XLATE_MASK(n) \
	(ATT_ISH_ATT_CONFIG_BASE_ADDR  + 0x10 * (n) + 0xC)
#define ATT_MMIO_BASE_VALID     BIT(0)
#define ATT_ADDR_ALIGNED        (4096)

#define ATT_ENTRY_0_LH2OSE_IPC (0)
#define ATT_ENTRY_1_DASHBOARD  (1)
#define ATT_ENTRY_2_SEC_REG    (2)
#define ATT_ENTRY_3_PEER_IPC   (3)
#define ATT_ENTRY_4_L2SRAM     (4)
#define ATT_ENTRY_5_GBE0       (5)
#define ATT_ENTRY_6_L2SRAM     (6)
#define ATT_ENTRY_7_GBE1       (7)
#define ATT_ENTRY_8_L2SRAM     (8)

/* gbe configuration settings */
#if defined(CONFIG_ETH_DWC_EQOS)

#define ETH_DWC_EQOS_MAX_TX_QUEUES      8
#define ETH_DWC_EQOS_MAX_RX_QUEUES      8
#define ETH_DWC_EQOS_MAX_TX_FIFOSZ      32768
#define ETH_DWC_EQOS_MAX_RX_FIFOSZ      32768
#define ETH_DWC_EQOS_SYS_CLOCK          (sedi_pm_get_hbw_clock())

/* gbe instance 0 */
#define ETH_DWC_EQOS_0_BASE_ADDR        0x50200000
#define ETH_DWC_EQOS_0_NETPROX_IRQ      55
#define ETH_DWC_EQOS_0_IRQ              59
#define ETH_DWC_EQOS_0_TX0_IRQ          60
#define ETH_DWC_EQOS_0_TX1_IRQ          61
#define ETH_DWC_EQOS_0_TX2_IRQ          62
#define ETH_DWC_EQOS_0_TX3_IRQ          63
#define ETH_DWC_EQOS_0_TX4_IRQ          64
#define ETH_DWC_EQOS_0_TX5_IRQ          65
#define ETH_DWC_EQOS_0_TX6_IRQ          66
#define ETH_DWC_EQOS_0_TX7_IRQ          67
#define ETH_DWC_EQOS_0_RX0_IRQ          68
#define ETH_DWC_EQOS_0_RX1_IRQ          69
#define ETH_DWC_EQOS_0_RX2_IRQ          70
#define ETH_DWC_EQOS_0_RX3_IRQ          71
#define ETH_DWC_EQOS_0_RX4_IRQ          72
#define ETH_DWC_EQOS_0_RX5_IRQ          73
#define ETH_DWC_EQOS_0_RX6_IRQ          74
#define ETH_DWC_EQOS_0_RX7_IRQ          75
#define ETH_DWC_EQOS_0_PHY_ADDR         1
#define ETH_DWC_EQOS_0_PHY_IFACE        PHY_INTERFACE_RGMII
#define ETH_DWC_EQOS_0_USE_XPCS         0
#define ETH_DWC_EQOS_0_CSR_CLOCK_RANGE  MAC_MDIO_CSR_CLOCK_150_250MHZ
#define ETH_DWC_EQOS_0_PTP_CLOCK_RATE   200000000

/* gbe instance 1 */
#define ETH_DWC_EQOS_1_BASE_ADDR        0x50220000
#define ETH_DWC_EQOS_1_NETPROX_IRQ      76
#define ETH_DWC_EQOS_1_IRQ              80
#define ETH_DWC_EQOS_1_TX0_IRQ          81
#define ETH_DWC_EQOS_1_TX1_IRQ          82
#define ETH_DWC_EQOS_1_TX2_IRQ          83
#define ETH_DWC_EQOS_1_TX3_IRQ          84
#define ETH_DWC_EQOS_1_TX4_IRQ          85
#define ETH_DWC_EQOS_1_TX5_IRQ          86
#define ETH_DWC_EQOS_1_TX6_IRQ          87
#define ETH_DWC_EQOS_1_TX7_IRQ          88
#define ETH_DWC_EQOS_1_RX0_IRQ          89
#define ETH_DWC_EQOS_1_RX1_IRQ          90
#define ETH_DWC_EQOS_1_RX2_IRQ          91
#define ETH_DWC_EQOS_1_RX3_IRQ          92
#define ETH_DWC_EQOS_1_RX4_IRQ          93
#define ETH_DWC_EQOS_1_RX5_IRQ          94
#define ETH_DWC_EQOS_1_RX6_IRQ          95
#define ETH_DWC_EQOS_1_RX7_IRQ          96
#define ETH_DWC_EQOS_1_PHY_ADDR         1
#define ETH_DWC_EQOS_1_PHY_IFACE        PHY_INTERFACE_RGMII
#define ETH_DWC_EQOS_1_USE_XPCS         0
#define ETH_DWC_EQOS_1_CSR_CLOCK_RANGE  MAC_MDIO_CSR_CLOCK_150_250MHZ
#define ETH_DWC_EQOS_1_PTP_CLOCK_RATE   200000000

#endif  /* CONFIG_ETH_DWC_EQOS */

#endif  /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _BOARD__H_ */

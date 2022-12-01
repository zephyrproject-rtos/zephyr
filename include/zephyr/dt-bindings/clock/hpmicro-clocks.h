/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_HPMICRO_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_HPMICRO_CLOCKS_H_

/**
 * @brief The cell div of clock-cell is not operated
 *
 */
#define HPM_CLOCK_CELL_DIV_NONE		(0xffff)
#define HPM_CLK_SRC_DEFAULT			(0xffff)

/**
 * @brief Hpmicro clock src
 *
 */
#define HPM_CLK_SRC_OSC24M			(0U)
#define HPM_CLK_SRC_PLL0_CLK0		(1U)
#define HPM_CLK_SRC_PLL1_CLK0		(2U)
#define HPM_CLK_SRC_PLL1_CLK1		(3U)
#define HPM_CLK_SRC_PLL2_CLK0		(4U)
#define HPM_CLK_SRC_PLL2_CLK1		(5U)
#define HPM_CLK_SRC_PLL3_CLK0		(6U)
#define HPM_CLK_SRC_PLL4_CLK0		(7U)
#define HPM_CLK_SRC_OSC32K			(8U)
#define HPM_CLK_ADC_SRC_AHB0		(9U)
#define HPM_CLK_ADC_SRC_ANA0		(10U)
#define HPM_CLK_ADC_SRC_ANA1		(11U)
#define HPM_CLK_ADC_SRC_ANA2		(12U)
#define HPM_CLK_I2S_SRC_AHB0		(13U)
#define HPM_CLK_I2S_SRC_AUD0		(14U)
#define HPM_CLK_I2S_SRC_AUD1		(15U)
#define HPM_CLK_I2S_SRC_AUD2		(16U)

/**
 * @brief Hpmicro clock name
 *
 */
#define	HPM_CLOCK_CPU0				(0U)
#define	HPM_CLOCK_CPU1				(1U)
#define	HPM_CLOCK_MCHTMR0			(2U)
#define	HPM_CLOCK_MCHTMR1			(3U)
#define	HPM_CLOCK_AXI0				(4U)
#define	HPM_CLOCK_AXI1				(5U)
#define	HPM_CLOCK_AXI2				(6U)
#define	HPM_CLOCK_AHB				(7U)
#define	HPM_CLOCK_DRAM				(8U)
#define	HPM_CLOCK_XPI0				(9U)
#define	HPM_CLOCK_XPI1				(10U)
#define	HPM_CLOCK_GPTMR0			(11U)
#define	HPM_CLOCK_GPTMR1			(12U)
#define	HPM_CLOCK_GPTMR2			(13U)
#define	HPM_CLOCK_GPTMR3			(14U)
#define	HPM_CLOCK_GPTMR4			(15U)
#define	HPM_CLOCK_GPTMR5			(16U)
#define	HPM_CLOCK_GPTMR6			(17U)
#define	HPM_CLOCK_GPTMR7			(18U)
#define	HPM_CLOCK_UART0				(19U)
#define	HPM_CLOCK_UART1				(20U)
#define	HPM_CLOCK_UART2				(21U)
#define	HPM_CLOCK_UART3				(22U)
#define	HPM_CLOCK_UART4				(23U)
#define	HPM_CLOCK_UART5				(24U)
#define	HPM_CLOCK_UART6				(25U)
#define	HPM_CLOCK_UART7				(26U)
#define	HPM_CLOCK_UART8				(27U)
#define	HPM_CLOCK_UART9				(28U)
#define	HPM_CLOCK_UART10			(29U)
#define	HPM_CLOCK_UART11			(30U)
#define	HPM_CLOCK_UART12			(31U)
#define	HPM_CLOCK_UART13			(32U)
#define	HPM_CLOCK_UART14			(33U)
#define	HPM_CLOCK_UART15			(34U)
#define	HPM_CLOCK_I2C0				(35U)
#define	HPM_CLOCK_I2C1				(36U)
#define	HPM_CLOCK_I2C2				(37U)
#define	HPM_CLOCK_I2C3				(38U)
#define	HPM_CLOCK_SPI0				(39U)
#define	HPM_CLOCK_SPI1				(40U)
#define	HPM_CLOCK_SPI2				(41U)
#define	HPM_CLOCK_SPI3				(42U)
#define	HPM_CLOCK_CAN0				(43U)
#define	HPM_CLOCK_CAN1				(44U)
#define	HPM_CLOCK_CAN2				(45U)
#define	HPM_CLOCK_CAN3				(46U)
#define	HPM_CLOCK_DISPLAY			(47U)
#define	HPM_CLOCK_SDXC0				(48U)
#define	HPM_CLOCK_SDXC1				(49U)
#define	HPM_CLOCK_CAMERA0			(50U)
#define	HPM_CLOCK_CAMERA1			(51U)
#define	HPM_CLOCK_NTMR0				(52U)
#define	HPM_CLOCK_NTMR1				(53U)

#define	HPM_CLOCK_PTPC				(54U)
#define	HPM_CLOCK_REF0				(55U)
#define	HPM_CLOCK_REF1				(56U)
#define	HPM_CLOCK_WATCHDOG0			(57U)
#define	HPM_CLOCK_WATCHDOG1			(58U)
#define	HPM_CLOCK_WATCHDOG2			(59U)
#define	HPM_CLOCK_WATCHDOG3			(60U)
#define	HPM_CLOCK_PUART				(61U)
#define	HPM_CLOCK_PWDG				(62U)
#define	HPM_CLOCK_ETH0				(63U)
#define	HPM_CLOCK_ETH1				(64U)
#define	HPM_CLOCK_PTP0				(65U)
#define	HPM_CLOCK_PTP1				(66U)
#define	HPM_CLOCK_SDP				(67U)
#define	HPM_CLOCK_XDMA				(68U)
#define	HPM_CLOCK_ROM				(69U)
#define	HPM_CLOCK_RAM0				(70U)
#define	HPM_CLOCK_RAM1				(71U)
#define	HPM_CLOCK_USB0				(72U)
#define	HPM_CLOCK_USB1				(73U)
#define	HPM_CLOCK_JPEG				(74U)
#define	HPM_CLOCK_PDMA				(75U)
#define	HPM_CLOCK_KMAN				(76U)
#define	HPM_CLOCK_GPIO				(77U)
#define	HPM_CLOCK_MBX0				(78U)
#define	HPM_CLOCK_MBX1				(78U)
#define	HPM_CLOCK_HDMA				(79U)
#define	HPM_CLOCK_RNG				(80U)
#define	HPM_CLOCK_MOT0				(81U)
#define	HPM_CLOCK_MOT1				(82U)
#define	HPM_CLOCK_MOT2				(83U)
#define	HPM_CLOCK_MOT3				(84U)
#define	HPM_CLOCK_ACMP				(85U)
#define	HPM_CLOCK_PDM				(86U)
#define	HPM_CLOCK_DAO				(87U)
#define	HPM_CLOCK_MSYN				(88U)
#define	HPM_CLOCK_LMM0				(89U)
#define	HPM_CLOCK_LMM1				(90U)

/* For ADC, there are 2-stage clock source and divider configuration */
#define	HPM_CLOCK_ANA0				(91U)
#define	HPM_CLOCK_ANA1				(92U)
#define	HPM_CLOCK_ANA2				(93U)
#define	HPM_CLOCK_ADC0				(94U)
#define	HPM_CLOCK_ADC1				(95U)
#define	HPM_CLOCK_ADC2				(96U)
#define	HPM_CLOCK_ADC3				(97U)

/* For I2S, there are 2-stage clock source and divider configuration */
#define	HPM_CLOCK_AUD0				(98U)
#define	HPM_CLOCK_AUD1				(99U)
#define	HPM_CLOCK_AUD2				(100U)
#define	HPM_CLOCK_I2S0				(101U)
#define	HPM_CLOCK_I2S1				(102U)
#define	HPM_CLOCK_I2S2				(103U)
#define	HPM_CLOCK_I2S3				(104U)

/* Clock sources */
#define	HPM_CLOCK_OSC0CLK0			(105U)
#define	HPM_CLOCK_PLL0CLK0			(106U)
#define	HPM_CLOCK_PLL1CLK0			(107U)
#define	HPM_CLOCK_PLL1CLK1			(108U)
#define	HPM_CLOCK_PLL2CLK0			(109U)
#define	HPM_CLOCK_PLL2CLK1			(110U)
#define	HPM_CLOCK_PLL3CLK0			(111U)
#define	HPM_CLOCK_PLL4CLK0			(112U)

#endif

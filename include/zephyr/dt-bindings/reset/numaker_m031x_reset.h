/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_NUMAKER_M031X_RESET_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_NUMAKER_M031X_RESET_H

/**
 * @file
 * @brief Reset module IDs for Nuvoton M031X
 * @ingroup reset_controller_nuvoton_m031x
 */

/**
 * @defgroup reset_controller_nuvoton_m031x Nuvoton NuMaker controller Devicetree helpers
 * @ingroup reset_controller_interface
 *
 * @details Devicetree macos/defines for Nuvoton NuMaker devices,
 * for use with the <tt>nuvoton,numaker-rst</tt> binding.
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @name Reset module IDs
 * @brief Register bit positions and reset IDs for Nuvoton M031X
 * @{
 */

/* Beginning of M031 BSP sys_reg.h reset module copy */

#define SYS_IPRST0_CHIPRST_Pos           0
#define SYS_IPRST0_CPURST_Pos            1
#define SYS_IPRST0_PDMARST_Pos           2
#define SYS_IPRST0_EBIRST_Pos            3
#define SYS_IPRST0_HDIVRST_Pos           4
#define SYS_IPRST0_CRCRST_Pos            7
#define SYS_IPRST1_GPIORST_Pos           1
#define SYS_IPRST1_TMR0RST_Pos           2
#define SYS_IPRST1_TMR1RST_Pos           3
#define SYS_IPRST1_TMR2RST_Pos           4
#define SYS_IPRST1_TMR3RST_Pos           5
#define SYS_IPRST1_ACMP01RST_Pos         7
#define SYS_IPRST1_I2C0RST_Pos           8
#define SYS_IPRST1_I2C1RST_Pos           9
#define SYS_IPRST1_QSPI0RST_Pos          12
#define SYS_IPRST1_SPI0RST_Pos           13
#define SYS_IPRST1_UART0RST_Pos          16
#define SYS_IPRST1_UART1RST_Pos          17
#define SYS_IPRST1_UART2RST_Pos          18
#define SYS_IPRST1_UART3RST_Pos          19
#define SYS_IPRST1_UART4RST_Pos          20
#define SYS_IPRST1_UART5RST_Pos          21
#define SYS_IPRST1_UART6RST_Pos          22
#define SYS_IPRST1_UART7RST_Pos          23
#define SYS_IPRST1_USBDRST_Pos           27
#define SYS_IPRST1_ADCRST_Pos            28
#define SYS_IPRST2_USCI0RST_Pos          8
#define SYS_IPRST2_USCI1RST_Pos          9
#define SYS_IPRST2_PWM0RST_Pos           16
#define SYS_IPRST2_PWM1RST_Pos           17
#define SYS_IPRST2_BPWM0RST_Pos          18
#define SYS_IPRST2_BPWM1RST_Pos          19

/* End of M031 BSP sys_reg.h reset module copy */

/* Beginning of M031 BSP sys.h reset module copy */

/*---------------------------------------------------------------------
 *  Module Reset Control Resister constant definitions.
 *---------------------------------------------------------------------
 */

#define NUMAKER_PDMA_RST    ((0x0 << 24) | SYS_IPRST0_PDMARST_Pos)
#define NUMAKER_EBI_RST     ((0x0 << 24) | SYS_IPRST0_EBIRST_Pos)
#define NUMAKER_HDIV_RST    ((0x0 << 24) | SYS_IPRST0_HDIVRST_Pos)
#define NUMAKER_CRC_RST     ((0x0 << 24) | SYS_IPRST0_CRCRST_Pos)
#define NUMAKER_GPIO_RST    ((0x4 << 24) | SYS_IPRST1_GPIORST_Pos)
#define NUMAKER_TMR0_RST    ((0x4 << 24) | SYS_IPRST1_TMR0RST_Pos)
#define NUMAKER_TMR1_RST    ((0x4 << 24) | SYS_IPRST1_TMR1RST_Pos)
#define NUMAKER_TMR2_RST    ((0x4 << 24) | SYS_IPRST1_TMR2RST_Pos)
#define NUMAKER_TMR3_RST    ((0x4 << 24) | SYS_IPRST1_TMR3RST_Pos)
#define NUMAKER_ACMP01_RST  ((0x4 << 24) | SYS_IPRST1_ACMP01RST_Pos)
#define NUMAKER_I2C0_RST    ((0x4 << 24) | SYS_IPRST1_I2C0RST_Pos)
#define NUMAKER_I2C1_RST    ((0x4 << 24) | SYS_IPRST1_I2C1RST_Pos)
#define NUMAKER_QSPI0_RST   ((0x4 << 24) | SYS_IPRST1_QSPI0RST_Pos)
#define NUMAKER_SPI0_RST    ((0x4 << 24) | SYS_IPRST1_SPI0RST_Pos)
#define NUMAKER_UART0_RST   ((0x4 << 24) | SYS_IPRST1_UART0RST_Pos)
#define NUMAKER_UART1_RST   ((0x4 << 24) | SYS_IPRST1_UART1RST_Pos)
#define NUMAKER_UART2_RST   ((0x4 << 24) | SYS_IPRST1_UART2RST_Pos)
#define NUMAKER_UART3_RST   ((0x4 << 24) | SYS_IPRST1_UART3RST_Pos)
#define NUMAKER_UART4_RST   ((0x4 << 24) | SYS_IPRST1_UART4RST_Pos)
#define NUMAKER_UART5_RST   ((0x4 << 24) | SYS_IPRST1_UART5RST_Pos)
#define NUMAKER_UART6_RST   ((0x4 << 24) | SYS_IPRST1_UART6RST_Pos)
#define NUMAKER_UART7_RST   ((0x4 << 24) | SYS_IPRST1_UART7RST_Pos)
#define NUMAKER_USBD_RST    ((0x4 << 24) | SYS_IPRST1_USBDRST_Pos)
#define NUMAKER_ADC_RST     ((0x4 << 24) | SYS_IPRST1_ADCRST_Pos)
#define NUMAKER_USCI0_RST   ((0x8 << 24) | SYS_IPRST2_USCI0RST_Pos)
#define NUMAKER_USCI1_RST   ((0x8 << 24) | SYS_IPRST2_USCI1RST_Pos)
#define NUMAKER_PWM0_RST    ((0x8 << 24) | SYS_IPRST2_PWM0RST_Pos)
#define NUMAKER_PWM1_RST    ((0x8 << 24) | SYS_IPRST2_PWM1RST_Pos)
#define NUMAKER_BPWM0_RST   ((0x8 << 24) | SYS_IPRST2_BPWM0RST_Pos)
#define NUMAKER_BPWM1_RST   ((0x8 << 24) | SYS_IPRST2_BPWM1RST_Pos)

/* End of M031 BSP sys.h reset module copy */

/** @} */

/** @endcond INTERNAL_HIDDEN */

/** @} */

#endif

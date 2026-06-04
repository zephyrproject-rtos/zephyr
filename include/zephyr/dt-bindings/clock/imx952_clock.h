/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock tree definitions for NXP i.MX952 SoC
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX952_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX952_CLOCK_H_

/**
 * @defgroup imx952_clock_sources NXP i.MX952 Clock Sources
 * @brief Root clock sources for i.MX952 SoC
 * @ingroup devicetree-clocks
 * @{
 */

/** External clock source */
#define IMX952_CLK_EXT                  0
/** 32kHz clock source */
#define IMX952_CLK_32K                  1
/** 24MHz clock source */
#define IMX952_CLK_24M                  2
/** Free-running oscillator clock */
#define IMX952_CLK_FRO                  3
/** System PLL1 VCO */
#define IMX952_CLK_SYSPLL1_VCO          4
/** System PLL1 PFD0 ungated */
#define IMX952_CLK_SYSPLL1_PFD0_UNGATED 5
/** System PLL1 PFD0 */
#define IMX952_CLK_SYSPLL1_PFD0         6
/** System PLL1 PFD0 divided by 2 */
#define IMX952_CLK_SYSPLL1_PFD0_DIV2    7
/** System PLL1 PFD1 ungated */
#define IMX952_CLK_SYSPLL1_PFD1_UNGATED 8
/** System PLL1 PFD1 */
#define IMX952_CLK_SYSPLL1_PFD1         9
/** System PLL1 PFD1 divided by 2 */
#define IMX952_CLK_SYSPLL1_PFD1_DIV2    10
/** System PLL1 PFD2 ungated */
#define IMX952_CLK_SYSPLL1_PFD2_UNGATED 11
/** System PLL1 PFD2 */
#define IMX952_CLK_SYSPLL1_PFD2         12
/** System PLL1 PFD2 divided by 2 */
#define IMX952_CLK_SYSPLL1_PFD2_DIV2    13
/** Audio PLL1 VCO */
#define IMX952_CLK_AUDIOPLL1_VCO        14
/** Audio PLL1 output */
#define IMX952_CLK_AUDIOPLL1            15
/** Audio PLL2 VCO */
#define IMX952_CLK_AUDIOPLL2_VCO        16
/** Audio PLL2 output */
#define IMX952_CLK_AUDIOPLL2            17
/** Video PLL1 VCO */
#define IMX952_CLK_VIDEOPLL1_VCO        18
/** Video PLL1 output */
#define IMX952_CLK_VIDEOPLL1            19
/** Reserved src clock 20 */
#define IMX952_CLK_SRC_RESERVED20       20
/** System PLL1 PFD3 ungated */
#define IMX952_CLK_SYSPLL1_PFD3_UNGATED 21
/** System PLL1 PFD3 */
#define IMX952_CLK_SYSPLL1_PFD3         22
/** System PLL1 PFD3 divided by 2 */
#define IMX952_CLK_SYSPLL1_PFD3_DIV2    23
/** ARM PLL VCO */
#define IMX952_CLK_ARMPLL_VCO           24
/** ARM PLL PFD0 ungated */
#define IMX952_CLK_ARMPLL_PFD0_UNGATED  25
/** ARM PLL PFD0 */
#define IMX952_CLK_ARMPLL_PFD0          26
/** ARM PLL PFD1 ungated */
#define IMX952_CLK_ARMPLL_PFD1_UNGATED  27
/** ARM PLL PFD1 */
#define IMX952_CLK_ARMPLL_PFD1          28
/** ARM PLL PFD2 ungated */
#define IMX952_CLK_ARMPLL_PFD2_UNGATED  29
/** ARM PLL PFD2 */
#define IMX952_CLK_ARMPLL_PFD2          30
/** ARM PLL PFD3 ungated */
#define IMX952_CLK_ARMPLL_PFD3_UNGATED  31
/** ARM PLL PFD3 */
#define IMX952_CLK_ARMPLL_PFD3          32
/** DRAM PLL VCO */
#define IMX952_CLK_DRAMPLL_VCO          33
/** DRAM PLL output */
#define IMX952_CLK_DRAMPLL              34
/** HSIO PLL VCO */
#define IMX952_CLK_HSIOPLL_VCO          35
/** HSIO PLL output */
#define IMX952_CLK_HSIOPLL              36
/** LDB PLL VCO */
#define IMX952_CLK_LDBPLL_VCO           37
/** LDB PLL output */
#define IMX952_CLK_LDBPLL               38
/** External clock source 1 */
#define IMX952_CLK_EXT1                 39
/** External clock source 2 */
#define IMX952_CLK_EXT2                 40

/** @} */

/** Total number of clock sources */
#define IMX952_CCM_NUM_CLK_SRC 41

/**
 * @defgroup imx952_peripheral_clocks Peripheral Clocks (41-163)
 * Peripheral clock definitions based on MIMX9529 fsl_clock.h
 * @{
 */

/** ADC clock */
#define IMX952_CLK_ADC              (IMX952_CCM_NUM_CLK_SRC + 0)
/** Reserved peripheral clock 1 */
#define IMX952_CLK_RESERVED1        (IMX952_CCM_NUM_CLK_SRC + 1)
/** AON bus clock */
#define IMX952_CLK_BUSAON           (IMX952_CCM_NUM_CLK_SRC + 2)
/** CAN1 clock */
#define IMX952_CLK_CAN1             (IMX952_CCM_NUM_CLK_SRC + 3)
/** Reserved peripheral clock 4 */
#define IMX952_CLK_RESERVED4        (IMX952_CCM_NUM_CLK_SRC + 4)
/** I3C1 slow clock */
#define IMX952_CLK_I3C1SLOW         (IMX952_CCM_NUM_CLK_SRC + 5)
/** LPI2C1 clock */
#define IMX952_CLK_LPI2C1           (IMX952_CCM_NUM_CLK_SRC + 6)
/** LPI2C2 clock */
#define IMX952_CLK_LPI2C2           (IMX952_CCM_NUM_CLK_SRC + 7)
/** LPSPI1 clock */
#define IMX952_CLK_LPSPI1           (IMX952_CCM_NUM_CLK_SRC + 8)
/** LPSPI2 clock */
#define IMX952_CLK_LPSPI2           (IMX952_CCM_NUM_CLK_SRC + 9)
/** LPTMR1 clock */
#define IMX952_CLK_LPTMR1           (IMX952_CCM_NUM_CLK_SRC + 10)
/** LPUART1 clock */
#define IMX952_CLK_LPUART1          (IMX952_CCM_NUM_CLK_SRC + 11)
/** LPUART2 clock */
#define IMX952_CLK_LPUART2          (IMX952_CCM_NUM_CLK_SRC + 12)
/** Cortex-M33 core clock */
#define IMX952_CLK_M33              (IMX952_CCM_NUM_CLK_SRC + 13)
/** Cortex-M33 SysTick clock */
#define IMX952_CLK_M33SYSTICK       (IMX952_CCM_NUM_CLK_SRC + 14)
/** Reserved peripheral clock 15 */
#define IMX952_CLK_RESERVED15       (IMX952_CCM_NUM_CLK_SRC + 15)
/** PDM clock */
#define IMX952_CLK_PDM              (IMX952_CCM_NUM_CLK_SRC + 16)
/** SAI1 clock */
#define IMX952_CLK_SAI1             (IMX952_CCM_NUM_CLK_SRC + 17)
/** Reserved peripheral clock 18 */
#define IMX952_CLK_RESERVED18       (IMX952_CCM_NUM_CLK_SRC + 18)
/** TPM2 clock */
#define IMX952_CLK_TPM2             (IMX952_CCM_NUM_CLK_SRC + 19)
/** Reserved peripheral clock 20 */
#define IMX952_CLK_RESERVED20       (IMX952_CCM_NUM_CLK_SRC + 20)
/** Camera APB clock */
#define IMX952_CLK_CAMAPB           (IMX952_CCM_NUM_CLK_SRC + 21)
/** Camera AXI clock */
#define IMX952_CLK_CAMAXI           (IMX952_CCM_NUM_CLK_SRC + 22)
/** Camera CM0 clock */
#define IMX952_CLK_CAMCM0           (IMX952_CCM_NUM_CLK_SRC + 23)
/** Camera ISI clock */
#define IMX952_CLK_CAMISI           (IMX952_CCM_NUM_CLK_SRC + 24)
/** Camera PHY configuration clock */
#define IMX952_CLK_CAMPHYCFG        (IMX952_CCM_NUM_CLK_SRC + 25)
/** MIPI PHY PLL bypass clock */
#define IMX952_CLK_MIPIPHYPLLBYPASS (IMX952_CCM_NUM_CLK_SRC + 26)
/** Reserved peripheral clock 27 */
#define IMX952_CLK_RESERVED27       (IMX952_CCM_NUM_CLK_SRC + 27)
/** MIPI test byte clock */
#define IMX952_CLK_MIPITESTBYTE     (IMX952_CCM_NUM_CLK_SRC + 28)
/** Cortex-A55 core clock */
#define IMX952_CLK_A55              (IMX952_CCM_NUM_CLK_SRC + 29)
/** Cortex-A55 MTR bus clock */
#define IMX952_CLK_A55MTRBUS        (IMX952_CCM_NUM_CLK_SRC + 30)
/** Cortex-A55 peripheral clock */
#define IMX952_CLK_A55PERIPH        (IMX952_CCM_NUM_CLK_SRC + 31)
/** DRAM alternate clock */
#define IMX952_CLK_DRAMALT          (IMX952_CCM_NUM_CLK_SRC + 32)
/** DRAM APB clock */
#define IMX952_CLK_DRAMAPB          (IMX952_CCM_NUM_CLK_SRC + 33)
/** Display APB clock */
#define IMX952_CLK_DISPAPB          (IMX952_CCM_NUM_CLK_SRC + 34)
/** Display AXI clock */
#define IMX952_CLK_DISPAXI          (IMX952_CCM_NUM_CLK_SRC + 35)
/** Display LPSPI clock */
#define IMX952_CLK_DISPLPSPI        (IMX952_CCM_NUM_CLK_SRC + 36)
/** Display OCRAM clock */
#define IMX952_CLK_DISPOCRAM        (IMX952_CCM_NUM_CLK_SRC + 37)
/** Display PHY configuration clock */
#define IMX952_CLK_DISPHYCFG        (IMX952_CCM_NUM_CLK_SRC + 38)
/** Display 1 pixel clock */
#define IMX952_CLK_DISP1PIX         (IMX952_CCM_NUM_CLK_SRC + 39)
/** Display CD PHY APB clock */
#define IMX952_CLK_DISPCDPHYAPB     (IMX952_CCM_NUM_CLK_SRC + 40)
/** Reserved peripheral clock 41 */
#define IMX952_CLK_RESERVED41       (IMX952_CCM_NUM_CLK_SRC + 41)
/** GPU APB clock */
#define IMX952_CLK_GPUAPB           (IMX952_CCM_NUM_CLK_SRC + 42)
/** GPU core clock */
#define IMX952_CLK_GPU              (IMX952_CCM_NUM_CLK_SRC + 43)
/** HSIO AC scan 480MHz clock */
#define IMX952_CLK_HSIOACSCAN480M   (IMX952_CCM_NUM_CLK_SRC + 44)
/** HSIO AC scan 80MHz clock */
#define IMX952_CLK_HSIOACSCAN80M    (IMX952_CCM_NUM_CLK_SRC + 45)
/** HSIO bus clock */
#define IMX952_CLK_HSIO             (IMX952_CCM_NUM_CLK_SRC + 46)
/** HSIO PCIe auxiliary clock */
#define IMX952_CLK_HSIOPCIEAUX      (IMX952_CCM_NUM_CLK_SRC + 47)
/** HSIO PCIe test 160MHz clock */
#define IMX952_CLK_HSIOPCIETEST160M (IMX952_CCM_NUM_CLK_SRC + 48)
/** HSIO PCIe test 400MHz clock */
#define IMX952_CLK_HSIOPCIETEST400M (IMX952_CCM_NUM_CLK_SRC + 49)
/** HSIO PCIe test 500MHz clock */
#define IMX952_CLK_HSIOPCIETEST500M (IMX952_CCM_NUM_CLK_SRC + 50)
/** HSIO USB test 50MHz clock */
#define IMX952_CLK_HSIOUSBTEST50M   (IMX952_CCM_NUM_CLK_SRC + 51)
/** HSIO USB test 60MHz clock */
#define IMX952_CLK_HSIOUSBTEST60M   (IMX952_CCM_NUM_CLK_SRC + 52)
/** M7 bus clock */
#define IMX952_CLK_BUSM7            (IMX952_CCM_NUM_CLK_SRC + 53)
/** Cortex-M7 core clock */
#define IMX952_CLK_M7               (IMX952_CCM_NUM_CLK_SRC + 54)
/** Cortex-M7 SysTick clock */
#define IMX952_CLK_M7SYSTICK        (IMX952_CCM_NUM_CLK_SRC + 55)
/** Network mix bus clock */
#define IMX952_CLK_BUSNETCMIX       (IMX952_CCM_NUM_CLK_SRC + 56)
/** Ethernet clock */
#define IMX952_CLK_ENET             (IMX952_CCM_NUM_CLK_SRC + 57)
/** Ethernet PHY test 200MHz clock */
#define IMX952_CLK_ENETPHYTEST200M  (IMX952_CCM_NUM_CLK_SRC + 58)
/** Ethernet PHY test 500MHz clock */
#define IMX952_CLK_ENETPHYTEST500M  (IMX952_CCM_NUM_CLK_SRC + 59)
/** Ethernet PHY test 667MHz clock */
#define IMX952_CLK_ENETPHYTEST667M  (IMX952_CCM_NUM_CLK_SRC + 60)
/** Ethernet reference clock */
#define IMX952_CLK_ENETREF          (IMX952_CCM_NUM_CLK_SRC + 61)
/** Ethernet timer 1 clock */
#define IMX952_CLK_ENETTIMER1       (IMX952_CCM_NUM_CLK_SRC + 62)
/** Reserved peripheral clock 63 */
#define IMX952_CLK_RESERVED63       (IMX952_CCM_NUM_CLK_SRC + 63)
/** SAI2 clock */
#define IMX952_CLK_SAI2             (IMX952_CCM_NUM_CLK_SRC + 64)
/** NOC APB clock */
#define IMX952_CLK_NOCAPB           (IMX952_CCM_NUM_CLK_SRC + 65)
/** Network-on-chip clock */
#define IMX952_CLK_NOC              (IMX952_CCM_NUM_CLK_SRC + 66)
/** NPU APB clock */
#define IMX952_CLK_NPUAPB           (IMX952_CCM_NUM_CLK_SRC + 67)
/** Neural processing unit clock */
#define IMX952_CLK_NPU              (IMX952_CCM_NUM_CLK_SRC + 68)
/** CCM clock output 1 */
#define IMX952_CLK_CCMCKO1          (IMX952_CCM_NUM_CLK_SRC + 69)
/** CCM clock output 2 */
#define IMX952_CLK_CCMCKO2          (IMX952_CCM_NUM_CLK_SRC + 70)
/** CCM clock output 3 */
#define IMX952_CLK_CCMCKO3          (IMX952_CCM_NUM_CLK_SRC + 71)
/** CCM clock output 4 */
#define IMX952_CLK_CCMCKO4          (IMX952_CCM_NUM_CLK_SRC + 72)
/** VPU APB clock */
#define IMX952_CLK_VPUAPB           (IMX952_CCM_NUM_CLK_SRC + 73)
/** Video processing unit clock */
#define IMX952_CLK_VPU              (IMX952_CCM_NUM_CLK_SRC + 74)
/** Reserved peripheral clock 75 */
#define IMX952_CLK_RESERVED75       (IMX952_CCM_NUM_CLK_SRC + 75)
/** Reserved peripheral clock 76 */
#define IMX952_CLK_RESERVED76       (IMX952_CCM_NUM_CLK_SRC + 76)
/** Audio transceiver clock */
#define IMX952_CLK_AUDIOXCVR        (IMX952_CCM_NUM_CLK_SRC + 77)
/** Wakeup bus clock */
#define IMX952_CLK_BUSWAKEUP        (IMX952_CCM_NUM_CLK_SRC + 78)
/** CAN2 clock */
#define IMX952_CLK_CAN2             (IMX952_CCM_NUM_CLK_SRC + 79)
/** CAN3 clock */
#define IMX952_CLK_CAN3             (IMX952_CCM_NUM_CLK_SRC + 80)
/** CAN4 clock */
#define IMX952_CLK_CAN4             (IMX952_CCM_NUM_CLK_SRC + 81)
/** CAN5 clock */
#define IMX952_CLK_CAN5             (IMX952_CCM_NUM_CLK_SRC + 82)
/** FlexIO1 clock */
#define IMX952_CLK_FLEXIO1          (IMX952_CCM_NUM_CLK_SRC + 83)
/** FlexIO2 clock */
#define IMX952_CLK_FLEXIO2          (IMX952_CCM_NUM_CLK_SRC + 84)
/** eXecute-in-place SPI 1 clock */
#define IMX952_CLK_XSPI1            (IMX952_CCM_NUM_CLK_SRC + 85)
/** Reserved peripheral clock 86 */
#define IMX952_CLK_RESERVED86       (IMX952_CCM_NUM_CLK_SRC + 86)
/** I3C2 slow clock */
#define IMX952_CLK_I3C2SLOW         (IMX952_CCM_NUM_CLK_SRC + 87)
/** LPI2C3 clock */
#define IMX952_CLK_LPI2C3           (IMX952_CCM_NUM_CLK_SRC + 88)
/** LPI2C4 clock */
#define IMX952_CLK_LPI2C4           (IMX952_CCM_NUM_CLK_SRC + 89)
/** LPI2C5 clock */
#define IMX952_CLK_LPI2C5           (IMX952_CCM_NUM_CLK_SRC + 90)
/** LPI2C6 clock */
#define IMX952_CLK_LPI2C6           (IMX952_CCM_NUM_CLK_SRC + 91)
/** LPI2C7 clock */
#define IMX952_CLK_LPI2C7           (IMX952_CCM_NUM_CLK_SRC + 92)
/** LPI2C8 clock */
#define IMX952_CLK_LPI2C8           (IMX952_CCM_NUM_CLK_SRC + 93)
/** LPSPI3 clock */
#define IMX952_CLK_LPSPI3           (IMX952_CCM_NUM_CLK_SRC + 94)
/** LPSPI4 clock */
#define IMX952_CLK_LPSPI4           (IMX952_CCM_NUM_CLK_SRC + 95)
/** LPSPI5 clock */
#define IMX952_CLK_LPSPI5           (IMX952_CCM_NUM_CLK_SRC + 96)
/** LPSPI6 clock */
#define IMX952_CLK_LPSPI6           (IMX952_CCM_NUM_CLK_SRC + 97)
/** LPSPI7 clock */
#define IMX952_CLK_LPSPI7           (IMX952_CCM_NUM_CLK_SRC + 98)
/** LPSPI8 clock */
#define IMX952_CLK_LPSPI8           (IMX952_CCM_NUM_CLK_SRC + 99)
/** LPTMR2 clock */
#define IMX952_CLK_LPTMR2           (IMX952_CCM_NUM_CLK_SRC + 100)
/** LPUART3 clock */
#define IMX952_CLK_LPUART3          (IMX952_CCM_NUM_CLK_SRC + 101)
/** LPUART4 clock */
#define IMX952_CLK_LPUART4          (IMX952_CCM_NUM_CLK_SRC + 102)
/** LPUART5 clock */
#define IMX952_CLK_LPUART5          (IMX952_CCM_NUM_CLK_SRC + 103)
/** LPUART6 clock */
#define IMX952_CLK_LPUART6          (IMX952_CCM_NUM_CLK_SRC + 104)
/** LPUART7 clock */
#define IMX952_CLK_LPUART7          (IMX952_CCM_NUM_CLK_SRC + 105)
/** LPUART8 clock */
#define IMX952_CLK_LPUART8          (IMX952_CCM_NUM_CLK_SRC + 106)
/** SAI3 clock */
#define IMX952_CLK_SAI3             (IMX952_CCM_NUM_CLK_SRC + 107)
/** SAI4 clock */
#define IMX952_CLK_SAI4             (IMX952_CCM_NUM_CLK_SRC + 108)
/** SAI5 clock */
#define IMX952_CLK_SAI5             (IMX952_CCM_NUM_CLK_SRC + 109)
/** S/PDIF clock */
#define IMX952_CLK_SPDIF            (IMX952_CCM_NUM_CLK_SRC + 110)
/** SWO trace clock */
#define IMX952_CLK_SWOTRACE         (IMX952_CCM_NUM_CLK_SRC + 111)
/** TPM4 clock */
#define IMX952_CLK_TPM4             (IMX952_CCM_NUM_CLK_SRC + 112)
/** TPM5 clock */
#define IMX952_CLK_TPM5             (IMX952_CCM_NUM_CLK_SRC + 113)
/** TPM6 clock */
#define IMX952_CLK_TPM6             (IMX952_CCM_NUM_CLK_SRC + 114)
/** Timestamp timer 2 clock */
#define IMX952_CLK_TSTMR2           (IMX952_CCM_NUM_CLK_SRC + 115)
/** USB PHY burn-in clock */
#define IMX952_CLK_USBPHYBURUNIN    (IMX952_CCM_NUM_CLK_SRC + 116)
/** uSDHC1 clock */
#define IMX952_CLK_USDHC1           (IMX952_CCM_NUM_CLK_SRC + 117)
/** uSDHC2 clock */
#define IMX952_CLK_USDHC2           (IMX952_CCM_NUM_CLK_SRC + 118)
/** uSDHC3 clock */
#define IMX952_CLK_USDHC3           (IMX952_CCM_NUM_CLK_SRC + 119)
/** V2X packet engine clock */
#define IMX952_CLK_V2XPK            (IMX952_CCM_NUM_CLK_SRC + 120)
/** Wakeup AXI clock */
#define IMX952_CLK_WAKEUPAXI        (IMX952_CCM_NUM_CLK_SRC + 121)
/** XPI slave root clock */
#define IMX952_CLK_XSPISLVROOT      (IMX952_CCM_NUM_CLK_SRC + 122)
/** Audio Mixer 1 clock */
#define IMX952_CLK_AUDMIX1          (IMX952_CCM_NUM_CLK_SRC + 123)
/** Asynchronous Sample Rate Converter 1 clock */
#define IMX952_CLK_ASRC1            (IMX952_CCM_NUM_CLK_SRC + 124)
/** Asynchronous Sample Rate Converter 2 clock */
#define IMX952_CLK_ASRC2            (IMX952_CCM_NUM_CLK_SRC + 125)
/** General Purpose Timer 2 clock */
#define IMX952_CLK_GPT2             (IMX952_CCM_NUM_CLK_SRC + 126)
/** General Purpose Timer 3 clock */
#define IMX952_CLK_GPT3             (IMX952_CCM_NUM_CLK_SRC + 127)
/** General Purpose Timer 4 clock */
#define IMX952_CLK_GPT4             (IMX952_CCM_NUM_CLK_SRC + 128)
/** General Purpose Timer 5 clock */
#define IMX952_CLK_GPT5             (IMX952_CCM_NUM_CLK_SRC + 129)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX952_CLOCK_H_ */

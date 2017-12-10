/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __RDC_DEFS_IMX7D__
#define __RDC_DEFS_IMX7D__

/*!
 * @addtogroup rdc_def_imx7d
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief RDC master assignment. */
enum _rdc_mda
{
    rdcMdaA7          = 0U,  /*!< ARM Cortex-A7 RDC Master. */
    rdcMdaM4          = 1U,  /*!< ARM Cortex-M4 RDC Master. */
    rdcMdaPcie        = 2U,  /*!< PCIe RDC Master. */
    rdcMdaCsi         = 3U,  /*!< CSI RDC Master. */
    rdcMdaEpdc        = 4U,  /*!< EPDC RDC Master. */
    rdcMdaLcdif       = 5U,  /*!< LCDIF RDC Master. */
    rdcMdaDisplayPort = 6U,  /*!< DISPLAY PORT RDC Master. */
    rdcMdaPxp         = 7U,  /*!< PXP RDC Master. */
    rdcMdaCoresight   = 8U,  /*!< CORESIGHT RDC Master. */
    rdcMdaDap         = 9U,  /*!< DAP RDC Master. */
    rdcMdaCaam        = 10U, /*!< CAAM RDC Master. */
    rdcMdaSdmaPeriph  = 11U, /*!< SDMA PERIPHERAL RDC Master. */
    rdcMdaSdmaBurst   = 12U, /*!< SDMA BURST RDC Master. */
    rdcMdaApbhdma     = 13U, /*!< APBH DMA RDC Master. */
    rdcMdaRawnand     = 14U, /*!< RAW NAND RDC Master. */
    rdcMdaUsdhc1      = 15U, /*!< USDHC1 RDC Master. */
    rdcMdaUsdhc2      = 16U, /*!< USDHC2 RDC Master. */
    rdcMdaUsdhc3      = 17U, /*!< USDHC3 RDC Master. */
    rdcMdaNc1         = 18U, /*!< NC1 RDC Master. */
    rdcMdaUsb         = 19U, /*!< USB RDC Master. */
    rdcMdaNc2         = 20U, /*!< NC2 RDC Master. */
    rdcMdaTest        = 21U, /*!< TEST RDC Master. */
    rdcMdaEnet1Tx     = 22U, /*!< Ethernet1 Tx RDC Master. */
    rdcMdaEnet1Rx     = 23U, /*!< Ethernet1 Rx RDC Master. */
    rdcMdaEnet2Tx     = 24U, /*!< Ethernet2 Tx RDC Master. */
    rdcMdaEnet2Rx     = 25U, /*!< Ethernet2 Rx RDC Master. */
    rdcMdaSdmaPort    = 26U, /*!< SDMA PORT RDC Master. */
};

/*! @brief RDC peripheral assignment. */
enum _rdc_pdap
{
    rdcPdapGpio1                = 0U,   /*!< GPIO1 RDC Peripheral. */
    rdcPdapGpio2                = 1U,   /*!< GPIO2 RDC Peripheral. */
    rdcPdapGpio3                = 2U,   /*!< GPIO3 RDC Peripheral. */
    rdcPdapGpio4                = 3U,   /*!< GPIO4 RDC Peripheral. */
    rdcPdapGpio5                = 4U,   /*!< GPIO5 RDC Peripheral. */
    rdcPdapGpio6                = 5U,   /*!< GPIO6 RDC Peripheral. */
    rdcPdapGpio7                = 6U,   /*!< GPIO7 RDC Peripheral. */
    rdcPdapIomuxcLpsrGpr        = 7U,   /*!< IOMXUC LPSR GPR RDC Peripheral. */
    rdcPdapWdog1                = 8U,   /*!< WDOG1 RDC Peripheral. */
    rdcPdapWdog2                = 9U,   /*!< WDOG2 RDC Peripheral. */
    rdcPdapWdog3                = 10U,  /*!< WDOG3 RDC Peripheral. */
    rdcPdapWdog4                = 11U,  /*!< WDOG4 RDC Peripheral. */
    rdcPdapIomuxcLpsr           = 12U,  /*!< IOMUXC LPSR RDC Peripheral. */
    rdcPdapGpt1                 = 13U,  /*!< GPT1 RDC Peripheral. */
    rdcPdapGpt2                 = 14U,  /*!< GPT2 RDC Peripheral. */
    rdcPdapGpt3                 = 15U,  /*!< GPT3 RDC Peripheral. */
    rdcPdapGpt4                 = 16U,  /*!< GPT4 RDC Peripheral. */
    rdcPdapRomcp                = 17U,  /*!< ROMCP RDC Peripheral. */
    rdcPdapKpp                  = 18U,  /*!< KPP RDC Peripheral. */
    rdcPdapIomuxc               = 19U,  /*!< IOMUXC RDC Peripheral. */
    rdcPdapIomuxcGpr            = 20U,  /*!< IOMUXC GPR RDC Peripheral. */
    rdcPdapOcotpCtrl            = 21U,  /*!< OCOTP CTRL RDC Peripheral. */
    rdcPdapAnatopDig            = 22U,  /*!< ANATOPDIG RDC Peripheral. */
    rdcPdapSnvs                 = 23U,  /*!< SNVS RDC Peripheral. */
    rdcPdapCcm                  = 24U,  /*!< CCM RDC Peripheral. */
    rdcPdapSrc                  = 25U,  /*!< SRC RDC Peripheral. */
    rdcPdapGpc                  = 26U,  /*!< GPC RDC Peripheral. */
    rdcPdapSemaphore1           = 27U,  /*!< SEMAPHORE1 RDC Peripheral. */
    rdcPdapSemaphore2           = 28U,  /*!< SEMAPHORE2 RDC Peripheral. */
    rdcPdapRdc                  = 29U,  /*!< RDC RDC Peripheral. */
    rdcPdapCsu                  = 30U,  /*!< CSU RDC Peripheral. */
    rdcPdapReserved1            = 31U,  /*!< Reserved1 RDC Peripheral. */
    rdcPdapReserved2            = 32U,  /*!< Reserved2 RDC Peripheral. */
    rdcPdapAdc1                 = 33U,  /*!< ADC1 RDC Peripheral. */
    rdcPdapAdc2                 = 34U,  /*!< ADC2 RDC Peripheral. */
    rdcPdapEcspi4               = 35U,  /*!< ECSPI4 RDC Peripheral. */
    rdcPdapFlexTimer1           = 36U,  /*!< FTM1 RDC Peripheral. */
    rdcPdapFlexTimer2           = 37U,  /*!< FTM2 RDC Peripheral. */
    rdcPdapPwm1                 = 38U,  /*!< PWM1 RDC Peripheral. */
    rdcPdapPwm2                 = 39U,  /*!< PWM2 RDC Peripheral. */
    rdcPdapPwm3                 = 40U,  /*!< PWM3 RDC Peripheral. */
    rdcPdapPwm4                 = 41U,  /*!< PWM4 RDC Peripheral. */
    rdcPdapSystemCounterRead    = 42U,  /*!< System Counter Read RDC Peripheral. */
    rdcPdapSystemCounterCompare = 43U,  /*!< System Counter Compare RDC Peripheral. */
    rdcPdapSystemCounterControl = 44U,  /*!< System Counter Control RDC Peripheral. */
    rdcPdapPcie                 = 45U,  /*!< PCIE RDC Peripheral. */
    rdcPdapReserved3            = 46U,  /*!< Reserved3 RDC Peripheral. */
    rdcPdapEpdc                 = 47U,  /*!< EPDC RDC Peripheral. */
    rdcPdapPxp                  = 48U,  /*!< PXP RDC Peripheral. */
    rdcPdapCsi                  = 49U,  /*!< CSI RDC Peripheral. */
    rdcPdapReserved4            = 50U,  /*!< Reserved4 RDC Peripheral. */
    rdcPdapLcdif                = 51U,  /*!< LCDIF RDC Peripheral. */
    rdcPdapReserved5            = 52U,  /*!< Reserved5 RDC Peripheral. */
    rdcPdapMipiCsi              = 53U,  /*!< MIPI CSI RDC Peripheral. */
    rdcPdapMipiDsi              = 54U,  /*!< MIPI DSI RDC Peripheral. */
    rdcPdapReserved6            = 55U,  /*!< Reserved6 RDC Peripheral. */
    rdcPdapTzasc                = 56U,  /*!< TZASC RDC Peripheral. */
    rdcPdapDdrPhy               = 57U,  /*!< DDR PHY RDC Peripheral. */
    rdcPdapDdrc                 = 58U,  /*!< DDRC RDC Peripheral. */
    rdcPdapReserved7            = 59U,  /*!< Reserved7 RDC Peripheral. */
    rdcPdapPerfMon1             = 60U,  /*!< PerfMon1 RDC Peripheral. */
    rdcPdapPerfMon2             = 61U,  /*!< PerfMon2 RDC Peripheral. */
    rdcPdapAxi                  = 62U,  /*!< AXI RDC Peripheral. */
    rdcPdapQosc                 = 63U,  /*!< QOSC RDC Peripheral. */
    rdcPdapFlexCan1             = 64U,  /*!< FLEXCAN1 RDC Peripheral. */
    rdcPdapFlexCan2             = 65U,  /*!< FLEXCAN2 RDC Peripheral. */
    rdcPdapI2c1                 = 66U,  /*!< I2C1 RDC Peripheral. */
    rdcPdapI2c2                 = 67U,  /*!< I2C2 RDC Peripheral. */
    rdcPdapI2c3                 = 68U,  /*!< I2C3 RDC Peripheral. */
    rdcPdapI2c4                 = 69U,  /*!< I2C4 RDC Peripheral. */
    rdcPdapUart4                = 70U,  /*!< UART4 RDC Peripheral. */
    rdcPdapUart5                = 71U,  /*!< UART5 RDC Peripheral. */
    rdcPdapUart6                = 72U,  /*!< UART6 RDC Peripheral. */
    rdcPdapUart7                = 73U,  /*!< UART7 RDC Peripheral. */
    rdcPdapMuA                  = 74U,  /*!< MUA RDC Peripheral. */
    rdcPdapMuB                  = 75U,  /*!< MUB RDC Peripheral. */
    rdcPdapSemaphoreHs          = 76U,  /*!< SEMAPHORE HS RDC Peripheral. */
    rdcPdapUsbPl301             = 77U,  /*!< USB PL301 RDC Peripheral. */
    rdcPdapReserved8            = 78U,  /*!< Reserved8 RDC Peripheral. */
    rdcPdapReserved9            = 79U,  /*!< Reserved9 RDC Peripheral. */
    rdcPdapReserved10           = 80U,  /*!< Reserved10 RDC Peripheral. */
    rdcPdapUSB1Otg1             = 81U,  /*!< USB2 OTG1 RDC Peripheral. */
    rdcPdapUSB2Otg2             = 82U,  /*!< USB2 OTG2 RDC Peripheral. */
    rdcPdapUSB3Host             = 83U,  /*!< USB3 HOST RDC Peripheral. */
    rdcPdapUsdhc1               = 84U,  /*!< USDHC1 RDC Peripheral. */
    rdcPdapUsdhc2               = 85U,  /*!< USDHC2 RDC Peripheral. */
    rdcPdapUsdhc3               = 86U,  /*!< USDHC3 RDC Peripheral. */
    rdcPdapReserved11           = 87U,  /*!< Reserved11 RDC Peripheral. */
    rdcPdapReserved12           = 88U,  /*!< Reserved12 RDC Peripheral. */
    rdcPdapSim1                 = 89U,  /*!< SIM1 RDC Peripheral. */
    rdcPdapSim2                 = 90U,  /*!< SIM2 RDC Peripheral. */
    rdcPdapQspi                 = 91U,  /*!< QSPI RDC Peripheral. */
    rdcPdapWeim                 = 92U,  /*!< WEIM RDC Peripheral. */
    rdcPdapSdma                 = 93U,  /*!< SDMA RDC Peripheral. */
    rdcPdapEnet1                = 94U,  /*!< Eneternet1 RDC Peripheral. */
    rdcPdapEnet2                = 95U,  /*!< Eneternet2 RDC Peripheral. */
    rdcPdapReserved13           = 96U,  /*!< Reserved13 RDC Peripheral. */
    rdcPdapReserved14           = 97U,  /*!< Reserved14 RDC Peripheral. */
    rdcPdapEcspi1               = 98U,  /*!< ECSPI1 RDC Peripheral. */
    rdcPdapEcspi2               = 99U,  /*!< ECSPI2 RDC Peripheral. */
    rdcPdapEcspi3               = 100U, /*!< ECSPI3 RDC Peripheral. */
    rdcPdapReserved15           = 101U, /*!< Reserved15 RDC Peripheral. */
    rdcPdapUart1                = 102U, /*!< UART1 RDC Peripheral. */
    rdcPdapReserved16           = 103U, /*!< Reserved16 RDC Peripheral. */
    rdcPdapUart3                = 104U, /*!< UART3 RDC Peripheral. */
    rdcPdapUart2                = 105U, /*!< UART2 RDC Peripheral. */
    rdcPdapSai1                 = 106U, /*!< SAI1 RDC Peripheral. */
    rdcPdapSai2                 = 107U, /*!< SAI2 RDC Peripheral. */
    rdcPdapSai3                 = 108U, /*!< SAI3 RDC Peripheral. */
    rdcPdapReserved17           = 109U, /*!< Reserved17 RDC Peripheral. */
    rdcPdapReserved18           = 110U, /*!< Reserved18 RDC Peripheral. */
    rdcPdapSpba                 = 111U, /*!< SPBA RDC Peripheral. */
    rdcPdapDap                  = 112U, /*!< DAP RDC Peripheral. */
    rdcPdapReserved19           = 113U, /*!< Reserved19 RDC Peripheral. */
    rdcPdapReserved20           = 114U, /*!< Reserved20 RDC Peripheral. */
    rdcPdapReserved21           = 115U, /*!< Reserved21 RDC Peripheral. */
    rdcPdapCaam                 = 116U, /*!< CAAM RDC Peripheral. */
    rdcPdapReserved22           = 117U, /*!< Reserved22 RDC Peripheral. */
};

/*! @brief RDC memory region. */
enum _rdc_mr
{
    rdcMrMmdc          = 0U,  /*!< alignment 4096 */
    rdcMrMmdcLast      = 7U,  /*!< alignment 4096 */
    rdcMrQspi          = 8U,  /*!< alignment 4096 */
    rdcMrQspiLast      = 15U, /*!< alignment 4096 */
    rdcMrWeim          = 16U, /*!< alignment 4096 */
    rdcMrWeimLast      = 23U, /*!< alignment 4096 */
    rdcMrPcie          = 24U, /*!< alignment 4096 */
    rdcMrPcieLast      = 31U, /*!< alignment 4096 */
    rdcMrOcram         = 32U, /*!< alignment 128 */
    rdcMrOcramLast     = 36U, /*!< alignment 128 */
    rdcMrOcramS        = 37U, /*!< alignment 128 */
    rdcMrOcramSLast    = 41U, /*!< alignment 128 */
    rdcMrOcramEpdc     = 42U, /*!< alignment 128 */
    rdcMrOcramEpdcLast = 46U, /*!< alignment 128 */
    rdcMrOcramPxp      = 47U, /*!< alignment 128 */
    rdcMrOcramPxpLast  = 51U, /*!< alignment 128 */
};

#endif /* __RDC_DEFS_IMX7D__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/

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

#ifndef __RDC_DEFS_IMX6SX__
#define __RDC_DEFS_IMX6SX__

/*!
 * @addtogroup rdc_def_imx6sx
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief RDC master assignment. */
enum _rdc_mda
{
    rdcMdaA9L2Cache  = 0U,  /*!< A9 L2 Cache RDC Master. */
    rdcMdaM4         = 1U,  /*!< M4 RDC Master. */
    rdcMdaGpu        = 2U,  /*!< GPU RDC Master. */
    rdcMdaCsi1       = 3U,  /*!< Csi1 RDC Master. */
    rdcMdaCsi2       = 4U,  /*!< Csi2 RDC Master. */
    rdcMdaLcdif1     = 5U,  /*!< Lcdif1 RDC Master. */
    rdcMdaLcdif2     = 6U,  /*!< Lcdif2 RDC Master. */
    rdcMdaPxp        = 7U,  /*!< Pxp RDC Master. */
    rdcMdaPcieCtrl   = 8U,  /*!< Pcie Ctrl RDC Master. */
    rdcMdaDap        = 9U,  /*!< Dap RDC Master. */
    rdcMdaCaam       = 10U, /*!< Caam RDC Master. */
    rdcMdaSdmaPeriph = 11U, /*!< Sdma Periph RDC Master. */
    rdcMdaSdmaBurst  = 12U, /*!< Sdma Burst RDC Master. */
    rdcMdaApbhdma    = 13U, /*!< Apbhdma RDC Master. */
    rdcMdaRawnand    = 14U, /*!< Rawnand RDC Master. */
    rdcMdaUsdhc1     = 15U, /*!< Usdhc1 RDC Master. */
    rdcMdaUsdhc2     = 16U, /*!< Usdhc2 RDC Master. */
    rdcMdaUsdhc3     = 17U, /*!< Usdhc3 RDC Master. */
    rdcMdaUsdhc4     = 18U, /*!< Usdhc4 RDC Master. */
    rdcMdaUsb        = 19U, /*!< USB RDC Master. */
    rdcMdaMlb        = 20U, /*!< MLB RDC Master. */
    rdcMdaTestPort   = 21U, /*!< Test Port RDC Master. */
    rdcMdaEnet1Tx    = 22U, /*!< Enet1 Tx RDC Master. */
    rdcMdaEnet1Rx    = 23U, /*!< Enet1 Rx Master. */
    rdcMdaEnet2Tx    = 24U, /*!< Enet2 Tx RDC Master. */
    rdcMdaEnet2Rx    = 25U, /*!< Enet2 Rx RDC Master. */
    rdcMdaSdmaPort   = 26U, /*!< Sdma Port RDC Master. */
};

/*! @brief RDC peripheral assignment. */
enum _rdc_pdap
{
    rdcPdapPwm1                 = 0U,   /*!< Pwm1 RDC Peripheral. */
    rdcPdapPwm2                 = 1U,   /*!< Pwm2 RDC Peripheral. */
    rdcPdapPwm3                 = 2U,   /*!< Pwm3 RDC Peripheral. */
    rdcPdapPwm4                 = 3U,   /*!< Pwm4 RDC Peripheral. */
    rdcPdapCan1                 = 4U,   /*!< Can1 RDC Peripheral. */
    rdcPdapCan2                 = 5U,   /*!< Can2 RDC Peripheral. */
    rdcPdapGpt                  = 6U,   /*!< Gpt RDC Peripheral. */
    rdcPdapGpio1                = 7U,   /*!< Gpio1 RDC Peripheral. */
    rdcPdapGpio2                = 8U,   /*!< Gpio2 RDC Peripheral. */
    rdcPdapGpio3                = 9U,   /*!< Gpio3 RDC Peripheral. */
    rdcPdapGpio4                = 10U,  /*!< Gpio4 RDC Peripheral. */
    rdcPdapGpio5                = 11U,  /*!< Gpio5 RDC Peripheral. */
    rdcPdapGpio6                = 12U,  /*!< Gpio6 RDC Peripheral. */
    rdcPdapGpio7                = 13U,  /*!< Gpio7 RDC Peripheral. */
    rdcPdapKpp                  = 14U,  /*!< Kpp RDC Peripheral. */
    rdcPdapWdog1                = 15U,  /*!< Wdog1 RDC Peripheral. */
    rdcPdapWdog2                = 16U,  /*!< Wdog2 RDC Peripheral. */
    rdcPdapCcm                  = 17U,  /*!< Ccm RDC Peripheral. */
    rdcPdapAnatopDig            = 18U,  /*!< AnatopDig RDC Peripheral. */
    rdcPdapSnvsHp               = 19U,  /*!< SnvsHp RDC Peripheral. */
    rdcPdapEpit1                = 20U,  /*!< Epit1 RDC Peripheral. */
    rdcPdapEpit2                = 21U,  /*!< Epit2 RDC Peripheral. */
    rdcPdapSrc                  = 22U,  /*!< Src RDC Peripheral. */
    rdcPdapGpc                  = 23U,  /*!< Gpc RDC Peripheral. */
    rdcPdapIomuxc               = 24U,  /*!< Iomuxc RDC Peripheral. */
    rdcPdapIomuxcGpr            = 25U,  /*!< IomuxcGpr RDC Peripheral. */
    rdcPdapCanfdCan1            = 26U,  /*!< Canfd Can1 RDC Peripheral. */
    rdcPdapSdma                 = 27U,  /*!< Sdma RDC Peripheral. */
    rdcPdapCanfdCan2            = 28U,  /*!< Canfd Can2 RDC Peripheral. */
    rdcPdapRdcSema421           = 29U,  /*!< Rdc Sema421 RDC Peripheral. */
    rdcPdapRdcSema422           = 30U,  /*!< Rdc Sema422 RDC Peripheral. */
    rdcPdapRdc                  = 31U,  /*!< Rdc RDC Peripheral. */
    rdcPdapAipsTz1GlobalEnable1 = 32U,  /*!< AipsTz1GlobalEnable1 RDC Peripheral. */
    rdcPdapAipsTz1GlobalEnable2 = 33U,  /*!< AipsTz1GlobalEnable2 RDC Peripheral. */
    rdcPdapUsb02hPl301          = 34U,  /*!< Usb02hPl301 RDC Peripheral. */
    rdcPdapUsb02hUsb            = 35U,  /*!< Usb02hUsb RDC Peripheral. */
    rdcPdapEnet1                = 36U,  /*!< Enet1 RDC Peripheral. */
    rdcPdapMlb2550              = 37U,  /*!< Mlb2550 RDC Peripheral. */
    rdcPdapUsdhc1               = 38U,  /*!< Usdhc1 RDC Peripheral. */
    rdcPdapUsdhc2               = 39U,  /*!< Usdhc2 RDC Peripheral. */
    rdcPdapUsdhc3               = 40U,  /*!< Usdhc3 RDC Peripheral. */
    rdcPdapUsdhc4               = 41U,  /*!< Usdhc4 RDC Peripheral. */
    rdcPdapI2c1                 = 42U,  /*!< I2c1 RDC Peripheral. */
    rdcPdapI2c2                 = 43U,  /*!< I2c2 RDC Peripheral. */
    rdcPdapI2c3                 = 44U,  /*!< I2c3 RDC Peripheral. */
    rdcPdapRomcp                = 45U,  /*!< Romcp RDC Peripheral. */
    rdcPdapMmdc                 = 46U,  /*!< Mmdc RDC Peripheral. */
    rdcPdapEnet2                = 47U,  /*!< Enet2 RDC Peripheral. */
    rdcPdapEim                  = 48U,  /*!< Eim RDC Peripheral. */
    rdcPdapOcotpCtrlWrapper     = 49U,  /*!< OcotpCtrlWrapper RDC Peripheral. */
    rdcPdapCsu                  = 50U,  /*!< Csu RDC Peripheral. */
    rdcPdapPerfmon1             = 51U,  /*!< Perfmon1 RDC Peripheral. */
    rdcPdapPerfmon2             = 52U,  /*!< Perfmon2 RDC Peripheral. */
    rdcPdapAxiMon               = 53U,  /*!< AxiMon RDC Peripheral. */
    rdcPdapTzasc1               = 54U,  /*!< Tzasc1 RDC Peripheral. */
    rdcPdapSai1                 = 55U,  /*!< Sai1 RDC Peripheral. */
    rdcPdapAudmux               = 56U,  /*!< Audmux RDC Peripheral. */
    rdcPdapSai2                 = 57U,  /*!< Sai2 RDC Peripheral. */
    rdcPdapQspi1                = 58U,  /*!< Qspi1 RDC Peripheral. */
    rdcPdapQspi2                = 59U,  /*!< Qspi2 RDC Peripheral. */
    rdcPdapUart2                = 60U,  /*!< Uart2 RDC Peripheral. */
    rdcPdapUart3                = 61U,  /*!< Uart3 RDC Peripheral. */
    rdcPdapUart4                = 62U,  /*!< Uart4 RDC Peripheral. */
    rdcPdapUart5                = 63U,  /*!< Uart5 RDC Peripheral. */
    rdcPdapI2c4                 = 64U,  /*!< I2c4 RDC Peripheral. */
    rdcPdapQosc                 = 65U,  /*!< Qosc RDC Peripheral. */
    rdcPdapCaam                 = 66U,  /*!< Caam RDC Peripheral. */
    rdcPdapDap                  = 67U,  /*!< Dap RDC Peripheral. */
    rdcPdapAdc1                 = 68U,  /*!< Adc1 RDC Peripheral. */
    rdcPdapAdc2                 = 69U,  /*!< Adc2 RDC Peripheral. */
    rdcPdapWdog3                = 70U,  /*!< Wdog3 RDC Peripheral. */
    rdcPdapEcspi5               = 71U,  /*!< Ecspi5 RDC Peripheral. */
    rdcPdapSema4                = 72U,  /*!< Sema4 RDC Peripheral. */
    rdcPdapMuA                  = 73U,  /*!< MuA RDC Peripheral. */
    rdcPdapCanfdCpu             = 74U,  /*!< Canfd Cpu RDC Peripheral. */
    rdcPdapMuB                  = 75U,  /*!< MuB RDC Peripheral. */
    rdcPdapUart6                = 76U,  /*!< Uart6 RDC Peripheral. */
    rdcPdapPwm5                 = 77U,  /*!< Pwm5 RDC Peripheral. */
    rdcPdapPwm6                 = 78U,  /*!< Pwm6 RDC Peripheral. */
    rdcPdapPwm7                 = 79U,  /*!< Pwm7 RDC Peripheral. */
    rdcPdapPwm8                 = 80U,  /*!< Pwm8 RDC Peripheral. */
    rdcPdapAipsTz3GlobalEnable0 = 81U,  /*!< AipsTz3GlobalEnable0 RDC Peripheral. */
    rdcPdapAipsTz3GlobalEnable1 = 82U,  /*!< AipsTz3GlobalEnable1 RDC Peripheral. */
    rdcPdapSpdif                = 84U,  /*!< Spdif RDC Peripheral. */
    rdcPdapEcspi1               = 85U,  /*!< Ecspi1 RDC Peripheral. */
    rdcPdapEcspi2               = 86U,  /*!< Ecspi2 RDC Peripheral. */
    rdcPdapEcspi3               = 87U,  /*!< Ecspi3 RDC Peripheral. */
    rdcPdapEcspi4               = 88U,  /*!< Ecspi4 RDC Peripheral. */
    rdcPdapUart1                = 91U,  /*!< Uart1 RDC Peripheral. */
    rdcPdapEsai                 = 92U,  /*!< Esai RDC Peripheral. */
    rdcPdapSsi1                 = 93U,  /*!< Ssi1 RDC Peripheral. */
    rdcPdapSsi2                 = 94U,  /*!< Ssi2 RDC Peripheral. */
    rdcPdapSsi3                 = 95U,  /*!< Ssi3 RDC Peripheral. */
    rdcPdapAsrc                 = 96U,  /*!< Asrc RDC Peripheral. */
    rdcPdapSpbaMaMegamix        = 98U,  /*!< SpbaMaMegamix RDC Peripheral. */
    rdcPdapGis                  = 99U,  /*!< Gis RDC Peripheral. */
    rdcPdapDcic1                = 100U, /*!< Dcic1 RDC Peripheral. */
    rdcPdapDcic2                = 101U, /*!< Dcic2 RDC Peripheral. */
    rdcPdapCsi1                 = 102U, /*!< Csi1 RDC Peripheral. */
    rdcPdapPxp                  = 103U, /*!< Pxp RDC Peripheral. */
    rdcPdapCsi2                 = 104U, /*!< Csi2 RDC Peripheral. */
    rdcPdapLcdif1               = 105U, /*!< Lcdif1 RDC Peripheral. */
    rdcPdapLcdif2               = 106U, /*!< Lcdif2 RDC Peripheral. */
    rdcPdapVadc                 = 107U, /*!< Vadc RDC Peripheral. */
    rdcPdapVdec                 = 108U, /*!< Vdec RDC Peripheral. */
    rdcPdapSpDisplaymix         = 109U, /*!< SpDisplaymix RDC Peripheral. */
};

/*! @brief RDC memory region */
enum _rdc_mr
{
    rdcMrMmdc        = 0U,  /*!< alignment 4096 */
    rdcMrMmdcLast    = 7U,  /*!< alignment 4096 */
    rdcMrQspi1       = 8U,  /*!< alignment 4096 */
    rdcMrQspi1Last   = 15U, /*!< alignment 4096 */
    rdcMrQspi2       = 16U, /*!< alignment 4096 */
    rdcMrQspi2Last   = 23U, /*!< alignment 4096 */
    rdcMrWeim        = 24U, /*!< alignment 4096 */
    rdcMrWeimLast    = 31U, /*!< alignment 4096 */
    rdcMrPcie        = 32U, /*!< alignment 4096 */
    rdcMrPcieLast    = 39U, /*!< alignment 4096 */
    rdcMrOcram       = 40U, /*!< alignment 128 */
    rdcMrOcramLast   = 44U, /*!< alignment 128 */
    rdcMrOcramS      = 45U, /*!< alignment 128 */
    rdcMrOcramSLast  = 49U, /*!< alignment 128 */
    rdcMrOcramL2     = 50U, /*!< alignment 128 */
    rdcMrOcramL2Last = 54U, /*!< alignment 128 */
};

#endif /* __RDC_DEFS_IMX6SX__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/

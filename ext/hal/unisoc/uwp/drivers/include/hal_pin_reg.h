/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HAL_PIN_REG_H
#define _HAL_PIN_REG_H

#ifdef __cpluscplus
extern "C"
{
#endif

#include <sys/types.h>
#include "hal_pinmap.h"
#include <uwp_hal.h>
#define CTL_BASE_PIN        (BASE_AON_PIN)

#define PIN_PIN_CRTL_REG0 (CTL_BASE_PIN+0x0000)
#define PIN_PIN_CRTL_REG1 (CTL_BASE_PIN+0x0004)
#define PIN_PIN_CRTL_REG2 (CTL_BASE_PIN+0x0008)
#define PIN_PIN_CRTL_REG3 (CTL_BASE_PIN+0x000C)
#define PIN_GPIO0_REG     (CTL_BASE_PIN+0x0010)
#define PIN_GPIO1_REG     (CTL_BASE_PIN+0x0014)
#define PIN_GPIO2_REG     (CTL_BASE_PIN+0x0018)
#define PIN_GPIO3_REG     (CTL_BASE_PIN+0x001C)
#define PIN_ESMD3_REG     (CTL_BASE_PIN+0x0020)
#define PIN_ESMD2_REG     (CTL_BASE_PIN+0x0024)
#define PIN_ESMD1_REG     (CTL_BASE_PIN+0x0028)
#define PIN_ESMD0_REG     (CTL_BASE_PIN+0x002C)
#define PIN_ESMCSN_REG    (CTL_BASE_PIN+0x0030)
#define PIN_ESMSMP_REG    (CTL_BASE_PIN+0x0034)
#define PIN_ESMCLK_REG    (CTL_BASE_PIN+0x0038)
#define PIN_IISDO_REG     (CTL_BASE_PIN+0x003C)
#define PIN_IISCLK_REG    (CTL_BASE_PIN+0x0040)
#define PIN_IISLRCK_REG   (CTL_BASE_PIN+0x0044)
#define PIN_IISDI_REG     (CTL_BASE_PIN+0x0048)
#define PIN_MTMS_REG      (CTL_BASE_PIN+0x004c)
#define PIN_MTCK_REG      (CTL_BASE_PIN+0x0050)
#define PIN_XTLEN_REG     (CTL_BASE_PIN+0x0054)
#define PIN_INT_REG       (CTL_BASE_PIN+0x0058)
#define PIN_PTEST_REG     (CTL_BASE_PIN+0x005C)
#define PIN_CHIP_EN_REG   (CTL_BASE_PIN+0x0060)
#define PIN_RST_N_REG     (CTL_BASE_PIN+0x0064)
#define PIN_WCI_2_RXD_REG    (CTL_BASE_PIN+0x0068)
#define PIN_WCI_2_TXD_REG     (CTL_BASE_PIN+0x006C)
#define PIN_U0TXD_REG     (CTL_BASE_PIN+0x0070)
#define PIN_U0RXD_REG     (CTL_BASE_PIN+0x0074)
#define PIN_U0RTS_REG     (CTL_BASE_PIN+0x0078)
#define PIN_U0CTS_REG     (CTL_BASE_PIN+0x007C)
#define PIN_U3TXD_REG     (CTL_BASE_PIN+0x0080)
#define PIN_U3RXD_REG     (CTL_BASE_PIN+0x0084)
#define PIN_RFCTL0_REG    (CTL_BASE_PIN+0x0088)
#define PIN_RFCTL1_REG    (CTL_BASE_PIN+0x008c)
#define PIN_RFCTL2_REG    (CTL_BASE_PIN+0x0090)
#define PIN_RFCTL3_REG    (CTL_BASE_PIN+0x0094)
#define PIN_RFCTL4_REG    (CTL_BASE_PIN+0x0098)
#define PIN_RFCTL5_REG    (CTL_BASE_PIN+0x009c)
#define PIN_RFCTL6_REG    (CTL_BASE_PIN+0x00A0)
#define PIN_RFCTL7_REG    (CTL_BASE_PIN+0x00A4)
#define PIN_SD_D3_REG     (CTL_BASE_PIN+0x00A8)
#define PIN_SD_D2_REG     (CTL_BASE_PIN+0x00AC)
#define PIN_SD_D1_REG     (CTL_BASE_PIN+0x00B0)
#define PIN_SD_D0_REG     (CTL_BASE_PIN+0x00B4)
#define PIN_SD_CLK_REG    (CTL_BASE_PIN+0x00B8)
#define PIN_SD_CMD_REG    (CTL_BASE_PIN+0x00BC)
#define PIN_GNSS_LNA_EN_REG  (CTL_BASE_PIN+0x00C0)
#define PIN_U1TXD_REG     (CTL_BASE_PIN+0x00C4)
#define PIN_U1RXD_REG     (CTL_BASE_PIN+0x00C8)
#define PIN_U1RTS_REG     (CTL_BASE_PIN+0x00CC)
#define PIN_U1CTS_REG     (CTL_BASE_PIN+0x00D0)
#define PIN_PCIE_CLKREQ_L_REG   (CTL_BASE_PIN+0x00D4)
#define PIN_PCIE_RST_L_REG      (CTL_BASE_PIN+0x00D8)
#define PIN_PCIE_WAKE_L_REG     (CTL_BASE_PIN+0x00DC)

/*pinmap ctrl register Bit field value*/

/*
 * |Reserved[31:16] | Drv btr sel[15:14] | bsr wpus[12] | bsr se[11]
 * | func PU[8] | func PD[7] |func sel[6:4]
 * | PU[3] | PD[2] | input En[1] | output En[0] |
 */
#define PMUX_FPU_EN        (1)
#define PMUX_FPD_EN        (0)

#define BIT_APB_PIN_EB     BIT(18)
#define BIT_18             0x00040000

#ifdef __cpluscplus
}
#endif

#endif	/*_PIN_REG_MARLIN3_H*/
/*end*/

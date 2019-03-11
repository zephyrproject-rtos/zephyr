/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file espi_mem.h
 *MEC1501 eSPI Memory Component definitions
 */
/** @defgroup MEC1501 Peripherals eSPI MEM
 */

#include <stdint.h>
#include <stddef.h>

#ifndef _ESPI_MEM_H
#define _ESPI_MEM_H


/*------------------------------------------------------------------*/

/* =========================================================================*/
/* ================           eSPI Memory Component        ================ */
/* =========================================================================*/

/**
  * @brief ESPI Host interface Memory Component (ESPI_MEM)
  */

typedef struct
{   /*! (@ 0x400F3800) eSPI Memory Compoent registers */
          uint8_t  RSVD1[0x130];
    __IOM uint16_t BAR_LDI_MBX_H0;        /*! (@ 0x0130) Mailbox Logical Device BAR Internal b[15:0] */
    __IOM uint16_t BAR_LDI_MBX_H1;        /*! (@ 0x0132) Mailbox Logical Device BAR Internal b[31:16] */
    __IOM uint16_t BAR_LDI_MBX_H2;        /*! (@ 0x0134) Mailbox Logical Device BAR Internal b[47:32] */
    __IOM uint16_t BAR_LDI_MBX_H3;        /*! (@ 0x0136) Mailbox Logical Device BAR Internal b[63:48] */
    __IOM uint16_t BAR_LDI_MBX_H4;        /*! (@ 0x0138) Mailbox Logical Device BAR Internal b[79:64] */
    __IOM uint16_t BAR_LDI_ACPI_EC0_H0;   /*! (@ 0x013A) ACPI EC0 Logical Device BAR Internal b[15:0] */
    __IOM uint16_t BAR_LDI_ACPI_EC0_H1;   /*! (@ 0x013C) ACPI EC0 Logical Device BAR Internal b[31:16] */
    __IOM uint16_t BAR_LDI_ACPI_EC0_H2;   /*! (@ 0x013E) ACPI EC0 Logical Device BAR Internal b[47:32] */
    __IOM uint16_t BAR_LDI_ACPI_EC0_H3;   /*! (@ 0x0140) ACPI EC0 Logical Device BAR Internal b[63:48] */
    __IOM uint16_t BAR_LDI_ACPI_EC0_H4;   /*! (@ 0x0142) ACPI EC0 Logical Device BAR Internal b[79:64] */
    __IOM uint16_t BAR_LDI_ACPI_EC1_H0;   /*! (@ 0x0144) ACPI EC1 Logical Device BAR Internal b[15:0] */
    __IOM uint16_t BAR_LDI_ACPI_EC1_H1;   /*! (@ 0x0146) ACPI EC1 Logical Device BAR Internal b[31:16] */
    __IOM uint16_t BAR_LDI_ACPI_EC1_H2;   /*! (@ 0x0148) ACPI EC1 Logical Device BAR Internal b[47:32] */
    __IOM uint16_t BAR_LDI_ACPI_EC1_H3;   /*! (@ 0x014A) ACPI EC1 Logical Device BAR Internal b[63:48] */
    __IOM uint16_t BAR_LDI_ACPI_EC1_H4;   /*! (@ 0x014C) ACPI EC1 Logical Device BAR Internal b[79:64] */
    __IOM uint16_t BAR_LDI_ACPI_EC2_H0;   /*! (@ 0x014E) ACPI EC2 Logical Device BAR Internal b[15:0] */
    __IOM uint16_t BAR_LDI_ACPI_EC2_H1;   /*! (@ 0x0150) ACPI EC2 Logical Device BAR Internal b[31:16] */
    __IOM uint16_t BAR_LDI_ACPI_EC2_H2;   /*! (@ 0x0152) ACPI EC2 Logical Device BAR Internal b[47:32] */
    __IOM uint16_t BAR_LDI_ACPI_EC2_H3;   /*! (@ 0x0154) ACPI EC2 Logical Device BAR Internal b[63:48] */
    __IOM uint16_t BAR_LDI_ACPI_EC2_H4;   /*! (@ 0x0156) ACPI EC2 Logical Device BAR Internal b[79:64] */
    __IOM uint16_t BAR_LDI_ACPI_EC3_H0;   /*! (@ 0x0158) ACPI EC3 Logical Device BAR Internal b[15:0] */
    __IOM uint16_t BAR_LDI_ACPI_EC3_H1;   /*! (@ 0x015A) ACPI EC3 Logical Device BAR Internal b[31:16] */
    __IOM uint16_t BAR_LDI_ACPI_EC3_H2;   /*! (@ 0x015C) ACPI EC3 Logical Device BAR Internal b[47:32] */
    __IOM uint16_t BAR_LDI_ACPI_EC3_H3;   /*! (@ 0x015E) ACPI EC3 Logical Device BAR Internal b[63:48] */
    __IOM uint16_t BAR_LDI_ACPI_EC3_H4;   /*! (@ 0x0160) ACPI EC3 Logical Device BAR Internal b[79:64] */
          uint8_t  RSVD2[0x16C - 0x162];
    __IOM uint16_t BAR_LDI_EM0_H0;        /*! (@ 0x016C) EMI0 Logical Device BAR Internal b[15:0] */
    __IOM uint16_t BAR_LDI_EM0_H1;        /*! (@ 0x016E) EMI0 Logical Device BAR Internal b[31:16] */
    __IOM uint16_t BAR_LDI_EM0_H2;        /*! (@ 0x0170) EMI0 Logical Device BAR Internal b[47:32] */
    __IOM uint16_t BAR_LDI_EM0_H3;        /*! (@ 0x0172) EMI0 Logical Device BAR Internal b[63:48] */
    __IOM uint16_t BAR_LDI_EM0_H4;        /*! (@ 0x0174) EMI0 Logical Device BAR Internal b[79:64] */
    __IOM uint16_t BAR_LDI_EM1_H0;        /*! (@ 0x0176) EMI1 Logical Device BAR Internal b[15:0] */
    __IOM uint16_t BAR_LDI_EM1_H1;        /*! (@ 0x0178) EMI1 Logical Device BAR Internal b[31:16] */
    __IOM uint16_t BAR_LDI_EM1_H2;        /*! (@ 0x017A) EMI1 Logical Device BAR Internal b[47:32] */
    __IOM uint16_t BAR_LDI_EM1_H3;        /*! (@ 0x017C) EMI1 Logical Device BAR Internal b[63:48] */
    __IOM uint16_t BAR_LDI_EM1_H4;        /*! (@ 0x017E) EMI1 Logical Device BAR Internal b[79:64] */
          uint8_t  RSVD3[0x1AC - 0x180];
    __IOM uint16_t BAR_SRAM0_H0;        /*! (@ 0x01AC) SRAM0 BAR Internal b[15:0] */
    __IOM uint16_t BAR_SRAM0_H1;        /*! (@ 0x01AE) SRAM0 BAR Internal b[31:16] */
    __IOM uint16_t BAR_SRAM0_H2;        /*! (@ 0x01B0) SRAM0 BAR Internal b[47:32] */
    __IOM uint16_t BAR_SRAM0_H3;        /*! (@ 0x01B2) SRAM0 BAR Internal b[63:48] */
    __IOM uint16_t BAR_SRAM0_H4;        /*! (@ 0x01B4) SRAM0 BAR Internal b[79:64] */
    __IOM uint16_t BAR_SRAM1_H0;        /*! (@ 0x01B6) SRAM1 BAR Internal b[15:0] */
    __IOM uint16_t BAR_SRAM1_H1;        /*! (@ 0x01B8) SRAM1 BAR Internal b[31:16] */
    __IOM uint16_t BAR_SRAM1_H2;        /*! (@ 0x01BA) SRAM1 BAR Internal b[47:32] */
    __IOM uint16_t BAR_SRAM1_H3;        /*! (@ 0x01BC) SRAM1 BAR Internal b[63:48] */
    __IOM uint16_t BAR_SRAM1_H4;        /*! (@ 0x01BE) SRAM1 BAR Internal b[79:64] */
          uint8_t  RSVD4[0x200 - 0x1C0];
    __IOM uint32_t BM_STS;              /*! (@ 0x0200) Bus Master Status */
    __IOM uint32_t BM_IEN;              /*! (@ 0x0204) Bus Master interrupt enable */
    __IOM uint32_t BM_CFG;              /*! (@ 0x0208) Bus Master configuration */
          uint8_t  RSVD5[4];
    __IOM uint32_t BM1_CTRL;            /*! (@ 0x0210) Bus Master 1 control */
    __IOM uint32_t BM1_HOST_ADDR_LSW;   /*! (@ 0x0214) Bus Master 1 host address bits[31:0] */
    __IOM uint32_t BM1_HOST_ADDR_MSW;   /*! (@ 0x0218) Bus Master 1 host address bits[63:32] */
    __IOM uint32_t BM1_EC_ADDR_LSW;     /*! (@ 0x021C) Bus Master 1 EC address bits[31:0] */
    __IOM uint32_t BM1_EC_ADDR_MSW;     /*! (@ 0x0220) Bus Master 1 EC address bits[63:32] */
    __IOM uint32_t BM2_CTRL;            /*! (@ 0x0224) Bus Master 2 control */
    __IOM uint32_t BM2_HOST_ADDR_LSW;   /*! (@ 0x0228) Bus Master 2 host address bits[31:0] */
    __IOM uint32_t BM2_HOST_ADDR_MSW;   /*! (@ 0x022C) Bus Master 2 host address bits[63:32] */
    __IOM uint32_t BM2_EC_ADDR_LSW;     /*! (@ 0x0230) Bus Master 2 EC address bits[31:0] */
    __IOM uint32_t BM2_EC_ADDR_MSW;     /*! (@ 0x0234) Bus Master 2 EC address bits[63:32] */
          uint8_t  RSVD6[0x330 - 0x238];
    __IOM uint16_t BAR_LDH_MBX_H0;        /*! (@ 0x0330) Mailbox Logical Device BAR Host b[15:0] */
    __IOM uint16_t BAR_LDH_MBX_H1;        /*! (@ 0x0332) Mailbox Logical Device BAR Host b[31:16] */
    __IOM uint16_t BAR_LDH_MBX_H2;        /*! (@ 0x0334) Mailbox Logical Device BAR Host b[47:32] */
    __IOM uint16_t BAR_LDH_MBX_H3;        /*! (@ 0x0336) Mailbox Logical Device BAR Host b[63:48] */
    __IOM uint16_t BAR_LDH_MBX_H4;        /*! (@ 0x0338) Mailbox Logical Device BAR Host b[79:64] */
    __IOM uint16_t BAR_LDH_ACPI_EC0_H0;   /*! (@ 0x033A) ACPI EC0 Logical Device BAR Host b[15:0] */
    __IOM uint16_t BAR_LDH_ACPI_EC0_H1;   /*! (@ 0x033C) ACPI EC0 Logical Device BAR Host b[31:16] */
    __IOM uint16_t BAR_LDH_ACPI_EC0_H2;   /*! (@ 0x033E) ACPI EC0 Logical Device BAR Host b[47:32] */
    __IOM uint16_t BAR_LDH_ACPI_EC0_H3;   /*! (@ 0x0340) ACPI EC0 Logical Device BAR Host b[63:48] */
    __IOM uint16_t BAR_LDH_ACPI_EC0_H4;   /*! (@ 0x0342) ACPI EC0 Logical Device BAR Host b[79:64] */
    __IOM uint16_t BAR_LDH_ACPI_EC1_H0;   /*! (@ 0x0344) ACPI EC1 Logical Device BAR Host b[15:0] */
    __IOM uint16_t BAR_LDH_ACPI_EC1_H1;   /*! (@ 0x0346) ACPI EC1 Logical Device BAR Host b[31:16] */
    __IOM uint16_t BAR_LDH_ACPI_EC1_H2;   /*! (@ 0x0348) ACPI EC1 Logical Device BAR Host b[47:32] */
    __IOM uint16_t BAR_LDH_ACPI_EC1_H3;   /*! (@ 0x034A) ACPI EC1 Logical Device BAR Host b[63:48] */
    __IOM uint16_t BAR_LDH_ACPI_EC1_H4;   /*! (@ 0x034C) ACPI EC1 Logical Device BAR Host b[79:64] */
    __IOM uint16_t BAR_LDH_ACPI_EC2_H0;   /*! (@ 0x034E) ACPI EC2 Logical Device BAR Host b[15:0] */
    __IOM uint16_t BAR_LDH_ACPI_EC2_H1;   /*! (@ 0x0350) ACPI EC2 Logical Device BAR Host b[31:16] */
    __IOM uint16_t BAR_LDH_ACPI_EC2_H2;   /*! (@ 0x0352) ACPI EC2 Logical Device BAR Host b[47:32] */
    __IOM uint16_t BAR_LDH_ACPI_EC2_H3;   /*! (@ 0x0354) ACPI EC2 Logical Device BAR Host b[63:48] */
    __IOM uint16_t BAR_LDH_ACPI_EC2_H4;   /*! (@ 0x0356) ACPI EC2 Logical Device BAR Host b[79:64] */
    __IOM uint16_t BAR_LDH_ACPI_EC3_H0;   /*! (@ 0x0358) ACPI EC3 Logical Device BAR Host b[15:0] */
    __IOM uint16_t BAR_LDH_ACPI_EC3_H1;   /*! (@ 0x035A) ACPI EC3 Logical Device BAR Host b[31:16] */
    __IOM uint16_t BAR_LDH_ACPI_EC3_H2;   /*! (@ 0x035C) ACPI EC3 Logical Device BAR Host b[47:32] */
    __IOM uint16_t BAR_LDH_ACPI_EC3_H3;   /*! (@ 0x035E) ACPI EC3 Logical Device BAR Host b[63:48] */
    __IOM uint16_t BAR_LDH_ACPI_EC3_H4;   /*! (@ 0x0360) ACPI EC3 Logical Device BAR Host b[79:64] */
          uint8_t  RSVD7[0x36C - 0x362];
    __IOM uint16_t BAR_LDH_EM0_H0;        /*! (@ 0x036C) EMI0 Logical Device BAR Internal b[15:0] */
    __IOM uint16_t BAR_LDH_EM0_H1;        /*! (@ 0x036E) EMI0 Logical Device BAR Internal b[31:16] */
    __IOM uint16_t BAR_LDH_EM0_H2;        /*! (@ 0x0370) EMI0 Logical Device BAR Internal b[47:32] */
    __IOM uint16_t BAR_LDH_EM0_H3;        /*! (@ 0x0372) EMI0 Logical Device BAR Internal b[63:48] */
    __IOM uint16_t BAR_LDH_EM0_H4;        /*! (@ 0x0374) EMI0 Logical Device BAR Internal b[79:64] */
    __IOM uint16_t BAR_LDH_EM1_H0;        /*! (@ 0x0376) EMI1 Logical Device BAR Internal b[15:0] */
    __IOM uint16_t BAR_LDH_EM1_H1;        /*! (@ 0x0378) EMI1 Logical Device BAR Internal b[31:16] */
    __IOM uint16_t BAR_LDH_EM1_H2;        /*! (@ 0x037A) EMI1 Logical Device BAR Internal b[47:32] */
    __IOM uint16_t BAR_LDH_EM1_H3;        /*! (@ 0x037C) EMI1 Logical Device BAR Internal b[63:48] */
    __IOM uint16_t BAR_LDH_EM1_H4;        /*! (@ 0x037E) EMI1 Logical Device BAR Internal b[79:64] */
          uint8_t  RSVD8[0x3AC - 0x380];
    __IOM uint16_t BAR_SRAM0_HOST_H0;     /*! (@ 0x03AC) SRAM0 BAR Host b[15:0] */
    __IOM uint16_t BAR_SRAM0_HOST_H1;     /*! (@ 0x03AE) SRAM0 BAR Host b[31:16] */
    __IOM uint16_t BAR_SRAM0_HOST_H2;     /*! (@ 0x03B0) SRAM0 BAR Host b[47:32] */
    __IOM uint16_t BAR_SRAM0_HOST_H3;     /*! (@ 0x03B2) SRAM0 BAR Host b[63:48] */
    __IOM uint16_t BAR_SRAM0_HOST_H4;     /*! (@ 0x03B4) SRAM0 BAR Host b[79:64] */
    __IOM uint16_t BAR_SRAM1_HOST_H0;     /*! (@ 0x03B6) SRAM1 BAR Host b[15:0] */
    __IOM uint16_t BAR_SRAM1_HOST_H1;     /*! (@ 0x03B8) SRAM1 BAR Host b[31:16] */
    __IOM uint16_t BAR_SRAM1_HOST_H2;     /*! (@ 0x03BA) SRAM1 BAR Host b[47:32] */
    __IOM uint16_t BAR_SRAM1_HOST_H3;     /*! (@ 0x03BC) SRAM1 BAR Host b[63:48] */
    __IOM uint16_t BAR_SRAM1_HOST_H4;     /*! (@ 0x03BE) SRAM1 BAR Host b[79:64] */
} ESPI_MEM_Type;


#endif /* #ifndef _ESPI_MEM_H */
/* end espi_mem.h */
/**   @}
 */


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

/** @file gpio.h
 *MEC1501 GPIO definitions
 */
/** @defgroup MEC1501 Peripherals GPIO
 */

#ifndef _GPIO_H
#define _GPIO_H

#include <stdint.h>
#include <stddef.h>


#define NUM_GPIO_PORTS      6
#define MAX_NUM_GPIO        (NUM_GPIO_PORTS * 32)

#define GPIO_BASE_ADDR          0x40081000ul
#define GPIO_CTRL2_OFS          0x0500ul

/*
 * !!! MEC15xx data sheets pin numbering is octal !!!
 * n = pin number in octal or the equivalent in decimal or hex
 * Example: GPIO135
 * n = 0135 or n = 0x5D or n = 93
 */
#define GPIO_CTRL_ADDR(n) ((uintptr_t)(GPIO_BASE_ADDR) + ((uintptr_t)(n) << 2))

#define GPIO_CTRL2_ADDR(n) ((uintptr_t)(GPIO_BASE_ADDR + GPIO_CTRL2_OFS) +\
                            ((uintptr_t)(n) << 2))
/*
 * MEC1501H-B0-SZ (144-pin)
 */
#define GPIO_PORT_A_BITMAP 0x7FFFFF9Dul   /* GPIO_0000 - GPIO_0036    GIRQ11 */
#define GPIO_PORT_B_BITMAP 0x0FFFFFFDul   /* GPIO_0040 - GPIO_0076    GIRQ10 */
#define GPIO_PORT_C_BITMAP 0x07FF3CF7ul   /* GPIO_0100 - GPIO_0136    GIRQ09 */
#define GPIO_PORT_D_BITMAP 0x272EFFFFul   /* GPIO_0140 - GPIO_0176    GIRQ08 */
#define GPIO_PORT_E_BITMAP 0x00DE00FFul   /* GPIO_0200 - GPIO_0236    GIRQ12 */
#define GPIO_PORT_F_BITMAP 0x0000397Ful   /* GPIO_0240 - GPIO_0276    GIRQ26 */

#define GPIO_PORT_A_DRVSTR_BITMAP   0x7FFFFF9Dul
#define GPIO_PORT_B_DRVSTR_BITMAP   0x0FFFFFFDul
#define GPIO_PORT_C_DRVSTR_BITMAP   0x07FF3CF7ul
#define GPIO_PORT_D_DRVSTR_BITMAP   0x272EFFFFul
#define GPIO_PORT_E_DRVSTR_BITMAP   0x00DE00FFul
#define GPIO_PORT_F_DRVSTR_BITMAP   0x0000397Ful

/*
 * GPIO Port to ECIA GIRQ mapping
 */
#define GPIO_PORT_A_GIRQ    11u
#define GPIO_PORT_B_GIRQ    10u
#define GPIO_PORT_C_GIRQ    9u
#define GPIO_PORT_D_GIRQ    8u
#define GPIO_PORT_E_GIRQ    12u
#define GPIO_PORT_F_GIRQ    26u

/*
 * GPIO Port GIRQ to NVIC external input
 * GPIO GIRQ's are always aggregated.
 */
#define GPIO_PORT_A_NVIC    3u
#define GPIO_PORT_B_NVIC    2u
#define GPIO_PORT_C_NVIC    1u
#define GPIO_PORT_D_NVIC    0u
#define GPIO_PORT_E_NVIC    4u
#define GPIO_PORT_F_NVIC    17u

/*
 * Control
 */
#define GPIO_CTRL_MASK          0x0101BFFFul
/* bits[15:0] of Control register */
#define GPIO_CTRL_CFG_MASK      0xBFFFul

/* Disable interrupt detect and pad */
#define GPIO_CTRL_DIS_PIN       0x8040ul

#define GPIO_CTRL_DFLT          0x8040ul
#define GPIO_CTRL_DFLT_MASK     0xfffful


#define GPIO000_CTRL_DFLT       0x1040ul
#define GPIO161_CTRL_DFLT       0x1040ul
#define GPIO162_CTRL_DFLT       0x1040ul
#define GPIO163_CTRL_DFLT       0x1040ul
#define GPIO172_CTRL_DFLT       0x1040ul
#define GPIO062_CTRL_DFLT       0x8240ul
#define GPIO170_CTRL_DFLT       0x0041ul    /* Boot-ROM JTAG_STRAP_BS */
#define GPIO116_CTRL_DFLT       0x0041ul
#define GPIO250_CTRL_DFLT       0x1240ul

#define GPIO_PUD_POS            0
#define GPIO_PUD_BLEN           2
#define GPIO_PUD_MASK           (0x03UL << (GPIO_PUD_POS))
#define GPIO_PUD_NONE           0x00
#define GPIO_PUD_PU             0x01
#define GPIO_PUD_PD             0x02
/* Repeater(keeper) mode */
#define GPIO_PUD_RPT            0x03

#define GPIO_PWRG_POS           2
#define GPIO_PWRG_BLEN          2
#define GPIO_PWRG_MASK          (0x03UL << (GPIO_PWRG_POS))
#define GPIO_PWRG_VTR_IO        (0x00UL << (GPIO_PWRG_POS))
#define GPIO_PWRG_VCC_IO        (0x01UL << (GPIO_PWRG_POS))
#define GPIO_PWRG_OFF           (0x02UL << (GPIO_PWRG_POS))
#define GPIO_PWRG_RSVD          (0x03UL << (GPIO_PWRG_POS))

#define GPIO_IDET_POS           4
#define GPIO_IDET_BLEN          4
#define GPIO_IDET_MASK          (0x0FUL << (GPIO_IDET_POS))
#define GPIO_IDET_LVL_LO        (0x00UL << (GPIO_IDET_POS))
#define GPIO_IDET_LVL_HI        (0x01UL << (GPIO_IDET_POS))
#define GPIO_IDET_DIS           (0x04UL << (GPIO_IDET_POS))
#define GPIO_IDET_REDGE         (0x0DUL << (GPIO_IDET_POS))
#define GPIO_IDET_FEDGE         (0x0EUL << (GPIO_IDET_POS))
#define GPIO_IDET_BEDGE         (0x0FUL << (GPIO_IDET_POS))

#define GPIO_BUFT_POS           8
#define GPIO_BUFT_BLEN          1
#define GPIO_BUFT_MASK          (1ul << (GPIO_BUFT_POS))
#define GPIO_BUFT_PUSHPULL      (0x00UL << (GPIO_BUFT_POS))
#define GPIO_BUFT_OPENDRAIN     (0x01UL << (GPIO_BUFT_POS))

#define GPIO_DIR_POS            9
#define GPIO_DIR_BLEN           1
#define GPIO_DIR_MASK           (0x01UL << (GPIO_DIR_POS))
#define GPIO_DIR_INPUT          (0x00UL << (GPIO_DIR_POS))
#define GPIO_DIR_OUTPUT         (0x01UL << (GPIO_DIR_POS))

#define GPIO_ALTOUT_POS         10
#define GPIO_ALTOUT_BLEN        1
#define GPIO_ALTOUT_MASK        (1ul << (GPIO_ALTOUT_POS))
#define GPIO_ALTOUT_DIS         (0x01UL << (GPIO_ALTOUT_POS))
#define GPIO_ALTOUT_EN          (0x00UL << (GPIO_ALTOUT_POS))

#define GPIO_POLARITY_POS       11
#define GPIO_POLARITY_BLEN      1
#define GPIO_POLARITY_MASK      (1ul << (GPIO_POLARITY_POS))
#define GPIO_POLARITY_NON_INV   (0x00UL << (GPIO_POLARITY_POS))
#define GPIO_POLARITY_INV       (0x01UL << (GPIO_POLARITY_POS))

#define GPIO_MUX_POS            12
#define GPIO_MUX_BLEN           2
#define GPIO_MUX_MASK0          0x03ul
#define GPIO_MUX_MASK           (0x0FUL << (GPIO_MUX_POS))
#define GPIO_MUX_GPIO           (0x00UL << (GPIO_MUX_POS))
#define GPIO_MUX_FUNC1          (0x01UL << (GPIO_MUX_POS))
#define GPIO_MUX_FUNC2          (0x02UL << (GPIO_MUX_POS))
#define GPIO_MUX_FUNC3          (0x03UL << (GPIO_MUX_POS))

#define GPIO_PADIN_DIS_POS      15
#define GPIO_PADIN_DIS_BLEN     1
#define GPIO_PADIN_DIS_MASK0    0x01ul
#define GPIO_PADIN_DIS_MASK     (0x01ul << (GPIO_PADIN_DIS_POS))
#define GPIO_PADIN_DIS          (0x01ul << (GPIO_PADIN_DIS_POS))
#define GPIO_PADIN_EN           (0ul << (GPIO_PADIN_DIS_POS))

#define GPIO_OUTPUT_BITPOS      (16)
#define GPIO_OUTPUT_BLEN        (1)
#define GPIO_OUTPUT_MASK        (1ul << (GPIO_OUTPUT_BITPOS))
#define GPIO_OUT_LO             (0x00UL << (GPIO_OUTPUT_BITPOS))
#define GPIO_OUT_HI             (0x01UL << (GPIO_OUTPUT_BITPOS))
#define GP_OUTPUT_0             (0x00UL)    // Byte or Bit-banding usage
#define GP_OUTPUT_1             (0x01UL)

#define GPIO_PADIN_BITPOS       (24)
#define GPIO_PADIN_BLEN         (1)
#define GPIO_PADIN_LOW          (0x00UL << (GPIO_PADIN_BITPOS))
#define GPIO_PADIN_HI           (0x01UL << (GPIO_PADIN_BITPOS))
#define GP_PADIN_LO             (0x00UL)    // Byte or Bit-banding usage
#define GP_PADIN_HI             (0x01UL)

#define GPIO_PIN_LOW            (0UL)
#define GPIO_PIN_HIGH           (1UL)

#define GPIO_DRIVE_OD_HI        (GPIO_BUFT_OPENDRAIN + GPIO_DIR_OUTPUT +\
    GPIO_ALTOUT_EN + GPIO_POLARITY_NON_INV + GPIO_MUX_GPIO + GPIO_OUTPUT_1)

#define GPIO_DRIVE_OD_HI_MASK   (GPIO_BUFT_MASK + GPIO_DIR_MASK +\
    GPIO_ALTOUT_MASK + GPIO_POLARITY_MASK + GPIO_MUX_MASK + GPIO_OUTPUT_MASK)

/*
 * Drive Strength
 * For GPIO pins that implement drive strength each pin
 * has a 32-bit register containing bit fields for
 * slew rate and buffer drive strength
 */
#define GPIO_DRV_STR_OFFSET     (0x0500ul)
#define GPIO_DRV_SLEW_POS    	0ul
#define GPIO_DRV_SLEW_MASK      (1ul << GPIO_DRV_SLEW_POS)
#define GPIO_DRV_SLEW_SLOW      (0ul << GPIO_DRV_SLEW_POS)
#define GPIO_DRV_SLEW_FAST      (1ul << GPIO_DRV_SLEW_POS)
#define GPIO_DRV_STR_POS     	4ul
#define GPIO_DRV_STR_LEN        2ul
#define GPIO_DRV_STR_MASK       (0x03ul << GPIO_DRV_STR_POS)
#define GPIO_DRV_STR_2MA        (0ul << GPIO_DRV_STR_POS)
#define GPIO_DRV_STR_4MA        (1ul << GPIO_DRV_STR_POS)
#define GPIO_DRV_STR_8MA        (2ul << GPIO_DRV_STR_POS)
#define GPIO_DRV_STR_12MA       (3ul << GPIO_DRV_STR_POS)

/*
 * Indices for GPIO Control/Control2 register arrays
 */
#define GPIO_PIN_0000       (0u)    /* Port A at register offset 00h */
#define GPIO_PIN_0001       (1u)
#define GPIO_PIN_0002       (2u)
#define GPIO_PIN_0003       (3u)
#define GPIO_PIN_0004       (4u)
#define GPIO_PIN_0005       (5u)
#define GPIO_PIN_0006       (6u)
#define GPIO_PIN_0007       (7u)

#define GPIO_PIN_0010       (8u)
#define GPIO_PIN_0011       (9u)
#define GPIO_PIN_0012       (10u)
#define GPIO_PIN_0013       (11u)
#define GPIO_PIN_0014       (12u)
#define GPIO_PIN_0015       (13u)
#define GPIO_PIN_0016       (14u)
#define GPIO_PIN_0017       (15u)

#define GPIO_PIN_0020       (16u)
#define GPIO_PIN_0021       (17u)
#define GPIO_PIN_0022       (18u)
#define GPIO_PIN_0023       (19u)
#define GPIO_PIN_0024       (20u)
#define GPIO_PIN_0025       (21u)
#define GPIO_PIN_0026       (22u)
#define GPIO_PIN_0027       (23u)

#define GPIO_PIN_0030       (24u)
#define GPIO_PIN_0031       (25u)
#define GPIO_PIN_0032       (26u)
#define GPIO_PIN_0033       (27u)
#define GPIO_PIN_0034       (28u)
#define GPIO_PIN_0035       (29u)
#define GPIO_PIN_0036       (30u)

#define GPIO_PIN_0040       (32u)   /* Port B at register offset 80h */
#define GPIO_PIN_0041       (33u)
#define GPIO_PIN_0042       (34u)
#define GPIO_PIN_0043       (35u)
#define GPIO_PIN_0044       (36u)
#define GPIO_PIN_0045       (37u)
#define GPIO_PIN_0046       (38u)
#define GPIO_PIN_0047       (39u)

#define GPIO_PIN_0050       (40u)
#define GPIO_PIN_0051       (41u)
#define GPIO_PIN_0052       (42u)
#define GPIO_PIN_0053       (43u)
#define GPIO_PIN_0054       (44u)
#define GPIO_PIN_0055       (45u)
#define GPIO_PIN_0056       (46u)
#define GPIO_PIN_0057       (47u)

#define GPIO_PIN_0060       (48u)
#define GPIO_PIN_0061       (49u)
#define GPIO_PIN_0062       (50u)
#define GPIO_PIN_0063       (51u)
#define GPIO_PIN_0064       (52u)
#define GPIO_PIN_0065       (53u)
#define GPIO_PIN_0066       (54u)
#define GPIO_PIN_0067       (55u)

#define GPIO_PIN_0070       (56u)
#define GPIO_PIN_0071       (57u)
#define GPIO_PIN_0072       (58u)
#define GPIO_PIN_0073       (59u)
#define GPIO_PIN_0074       (60u)
#define GPIO_PIN_0075       (61u)
#define GPIO_PIN_0076       (62u)

#define GPIO_PIN_0100       (64u)   /* Port C at register offset 100h */
#define GPIO_PIN_0101       (65u)
#define GPIO_PIN_0102       (66u)
#define GPIO_PIN_0103       (67u)
#define GPIO_PIN_0104       (68u)
#define GPIO_PIN_0105       (69u)
#define GPIO_PIN_0106       (70u)
#define GPIO_PIN_0107       (71u)

#define GPIO_PIN_0110       (72u)
#define GPIO_PIN_0111       (73u)
#define GPIO_PIN_0112       (74u)
#define GPIO_PIN_0113       (75u)
#define GPIO_PIN_0114       (76u)
#define GPIO_PIN_0115       (77u)
#define GPIO_PIN_0116       (78u)
#define GPIO_PIN_0117       (79u)

#define GPIO_PIN_0120       (80u)
#define GPIO_PIN_0121       (81u)
#define GPIO_PIN_0122       (82u)
#define GPIO_PIN_0123       (83u)
#define GPIO_PIN_0124       (84u)
#define GPIO_PIN_0125       (85u)
#define GPIO_PIN_0126       (86u)
#define GPIO_PIN_0127       (87u)

#define GPIO_PIN_0130       (88u)
#define GPIO_PIN_0131       (89u)
#define GPIO_PIN_0132       (90u)
#define GPIO_PIN_0133       (91u)
#define GPIO_PIN_0134       (92u)
#define GPIO_PIN_0135       (93u)
#define GPIO_PIN_0136       (94u)

#define GPIO_PIN_0140       (96u)   /* Port D at register offset 180h */
#define GPIO_PIN_0141       (97u)
#define GPIO_PIN_0142       (98u)
#define GPIO_PIN_0143       (99u)
#define GPIO_PIN_0144       (100u)
#define GPIO_PIN_0145       (101u)
#define GPIO_PIN_0146       (102u)
#define GPIO_PIN_0147       (103u)

#define GPIO_PIN_0150       (104u)
#define GPIO_PIN_0151       (105u)
#define GPIO_PIN_0152       (106u)
#define GPIO_PIN_0153       (107u)
#define GPIO_PIN_0154       (108u)
#define GPIO_PIN_0155       (109u)
#define GPIO_PIN_0156       (110u)
#define GPIO_PIN_0157       (111u)

#define GPIO_PIN_0160       (112u)
#define GPIO_PIN_0161       (113u)
#define GPIO_PIN_0162       (114u)
#define GPIO_PIN_0163       (115u)
#define GPIO_PIN_0164       (116u)
#define GPIO_PIN_0165       (117u)
#define GPIO_PIN_0166       (118u)
#define GPIO_PIN_0167       (119u)

#define GPIO_PIN_0170       (120u)
#define GPIO_PIN_0171       (121u)
#define GPIO_PIN_0172       (122u)
#define GPIO_PIN_0173       (123u)
#define GPIO_PIN_0174       (124u)
#define GPIO_PIN_0175       (125u)
#define GPIO_PIN_0176       (126u)

#define GPIO_PIN_0200       (128u)  /* Port E at register offset 200h */
#define GPIO_PIN_0201       (129u)
#define GPIO_PIN_0202       (130u)
#define GPIO_PIN_0203       (131u)
#define GPIO_PIN_0204       (132u)
#define GPIO_PIN_0205       (133u)
#define GPIO_PIN_0206       (134u)
#define GPIO_PIN_0207       (135u)

#define GPIO_PIN_0210       (136u)
#define GPIO_PIN_0211       (137u)
#define GPIO_PIN_0212       (138u)
#define GPIO_PIN_0213       (139u)
#define GPIO_PIN_0214       (140u)
#define GPIO_PIN_0215       (141u)
#define GPIO_PIN_0216       (142u)
#define GPIO_PIN_0217       (143u)

#define GPIO_PIN_0220       (144u)
#define GPIO_PIN_0221       (145u)
#define GPIO_PIN_0222       (146u)
#define GPIO_PIN_0223       (147u)
#define GPIO_PIN_0224       (148u)
#define GPIO_PIN_0225       (149u)
#define GPIO_PIN_0226       (150u)
#define GPIO_PIN_0227       (151u)

#define GPIO_PIN_0230       (152u)
#define GPIO_PIN_0231       (153u)
#define GPIO_PIN_0232       (154u)
#define GPIO_PIN_0233       (155u)
#define GPIO_PIN_0234       (156u)
#define GPIO_PIN_0235       (157u)
#define GPIO_PIN_0236       (158u)

#define GPIO_PIN_0240       (160u)  /* Port F at register offset 280h */
#define GPIO_PIN_0241       (161u)
#define GPIO_PIN_0242       (162u)
#define GPIO_PIN_0243       (163u)
#define GPIO_PIN_0244       (164u)
#define GPIO_PIN_0245       (165u)
#define GPIO_PIN_0246       (166u)
#define GPIO_PIN_0247       (167u)

#define GPIO_PIN_0250       (168u)
#define GPIO_PIN_0251       (169u)
#define GPIO_PIN_0252       (170u)
#define GPIO_PIN_0253       (172u)
#define GPIO_PIN_0254       (173u)
#define GPIO_PIN_0255       (174u)

#define GPIO_PIN_MAX        (175u)


/* =========================================================================*/
/* ================            GPIO                        ================ */
/* =========================================================================*/

/**
  * @brief GPIO Control (GPIO)
  */
#define GPIO_CTRL_BEGIN     0
#define GPIO_CTRL_END       0x2C4
#define GPIO_PIN_BEGIN      0x300
#define GPIO_PIN_END        0x318
#define GPIO_POUT_BEGIN     0x380
#define GPIO_POUT_END       0x398
#define GPIO_LOCK_BEGIN     0x3E8
#define GPIO_LOCK_END       0x400
#define GPIO_CTRL2_BEGIN    0x500
#define GPIO_CTRL2_END      0x7B4

#define MAX_GPIO_PIN    ((GPIO_CTRL_END) / 4)
#define MAX_GPIO_BANK   6u
#define GPIO_LOCK5_IDX      0u
#define GPIO_LOCK4_IDX      1u
#define GPIO_LOCK3_IDX      2u
#define GPIO_LOCK2_IDX      3u
#define GPIO_LOCK1_IDX      4u
#define GPIO_LOCK0_IDX      5u
#define GPIO_LOCK_MAX_IDX   6u

typedef union
{
    __IOM uint32_t u32;
    __IOM uint16_t u16[2];
    __IOM uint8_t  u8[4];
} GPIO_CTRL_Type;

typedef union
{
    __IOM uint32_t u32;
} GPIO_PARIN_Type;

typedef union
{
    __IOM uint32_t u32;
} GPIO_PAROUT_Type;

typedef union
{
    __IOM uint32_t u32;
    __IOM uint16_t u16[2];
    __IOM uint8_t  u8[4];
} GPIO_CTRL2_Type;

typedef struct
{
    GPIO_CTRL_Type      CTRL[MAX_GPIO_PIN];     /*!< (@ 0x00000000) GPIO Control register array */
    uint8_t             RSVD1[0x300 - 0x2C4];
    GPIO_PARIN_Type     PARIN[MAX_GPIO_BANK];   /*!< (@ 0x00000300) GPIO Parallel Input */
    uint8_t             RSVD2[0x380 - 0x318];
    GPIO_PAROUT_Type    PAROUT[MAX_GPIO_BANK];  /*!< (@ 0x00000380) GPIO Parallel Output */
    uint8_t             RSVD3[0x3E8 - 0x398];
    __IOM uint32_t      LOCK5;                  /*!< (@ 0x000003E8) GPIO Lock 5 */
    __IOM uint32_t      LOCK4;                  /*!< (@ 0x000003EC) GPIO Lock 4 */
    __IOM uint32_t      LOCK3;                  /*!< (@ 0x000003F0) GPIO Lock 3 */
    __IOM uint32_t      LOCK2;                  /*!< (@ 0x000003F4) GPIO Lock 2 */
    __IOM uint32_t      LOCK1;                  /*!< (@ 0x000003F8) GPIO Lock 1 */
    __IOM uint32_t      LOCK0;                  /*!< (@ 0x000003FC) GPIO Lock 0 */
    uint8_t             RSVD4[0x500 - 0x400];
    GPIO_CTRL2_Type     CTRL2[MAX_GPIO_PIN];    /*!< (@ 0x00000500) GPIO Control 2 register array */
} GPIO_Type;


#endif /* #ifndef _GPIO_H */
/* end gpio.h */
/**   @}
 */

/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NPCM_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NPCM_PINCTRL_H_

/* Pin index follow GPIO group and pin number */
#define NPCM_PINCTRL_PORT_SHIFT     (3)
#define NPCM_PINCTRL_PORT_MASK      (0xF)
#define NPCM_PINCTRL_PORT_OFFSET(N) (((N) & NPCM_PINCTRL_PORT_MASK) << NPCM_PINCTRL_PORT_SHIFT)
#define NPCM_PINCTRL_PIN_MASK       ((1 << NPCM_PINCTRL_PORT_SHIFT) - 1)
#define NPCM_PINCTRL_PIN_OFFSET(N)  ((N) & NPCM_PINCTRL_PIN_MASK)
#define NPCM_PINCTRL_NUM_IDX(port, pin)                                                            \
	(NPCM_PINCTRL_PORT_OFFSET(port) + NPCM_PINCTRL_PIN_OFFSET(pin))

/* Config properties */
#define NPCM_PIN_REG_NULL 0x0
#define NPCM_PIN_CFG_NULL 0xFF

/*
 * Device multi-function pin index and selection.
 *
 * The alt pin is showed in name: cfg0__cfg1__...__cfgN and can set with property pinmux in dts.
 *   For example, selects SMI:      pinmux = <GPIOB0__SMI 1>;
 *                selects GPIOB0:   pinmux = <GPIOB0__SMI 0>;
 *                selects CR_SOUT2: pinmux = <GPIO31__SOUTB__CR_SOUT2__PD2__LED_E 2>;
 */
#define GPIOB0__SMI                                      NPCM_PINCTRL_NUM_IDX(0xB, 0)
#define GPIO00__EMAC_RXD1__CTSB__PWM7                    NPCM_PINCTRL_NUM_IDX(0x0, 0)
#define GPIO01__EMAC_RXD0__DSRB__TA7                     NPCM_PINCTRL_NUM_IDX(0x0, 1)
#define GPIO02__EMAC_REF_CLK__RTSB__PWM8                 NPCM_PINCTRL_NUM_IDX(0x0, 2)
#define GPIO03__EMAC_TXD1__DTRB__TA8                     NPCM_PINCTRL_NUM_IDX(0x0, 3)
#define GPIO04__EMAC_TXD0__SINB__CR_SIN1__PWM1           NPCM_PINCTRL_NUM_IDX(0x0, 4)
#define GPIO05__EMAC_TX_EN__SOUTB__CR_SOUT1__TA1         NPCM_PINCTRL_NUM_IDX(0x0, 5)
#define GPIO06__EMAC_MDI0__DCDB__PWM9                    NPCM_PINCTRL_NUM_IDX(0x0, 6)
#define GPIO07__EMAC_MDC__RIB__TA9                       NPCM_PINCTRL_NUM_IDX(0x0, 7)
#define GPIOB1__CLKRUN                                   NPCM_PINCTRL_NUM_IDX(0xB, 1)
#define GPIOB2__LPCPD__ESPI_RST__LDRQ                    NPCM_PINCTRL_NUM_IDX(0xB, 2)
#define GPIOB3__ESPI_ALERT__SERIRQ                       NPCM_PINCTRL_NUM_IDX(0xB, 3)
#define GPIOB7__PLTRST                                   NPCM_PINCTRL_NUM_IDX(0xB, 7)
#define GPIOB4                                           NPCM_PINCTRL_NUM_IDX(0xB, 4)
#define GPIOB5                                           NPCM_PINCTRL_NUM_IDX(0xB, 5)
#define GPIO10__CTSA                                     NPCM_PINCTRL_NUM_IDX(0x1, 0)
#define GPIO11__SCL2A__DRSA                              NPCM_PINCTRL_NUM_IDX(0x1, 1)
#define GPIO12__RTSA                                     NPCM_PINCTRL_NUM_IDX(0x1, 2)
#define GPIO13__SDA2A__DTRA                              NPCM_PINCTRL_NUM_IDX(0x1, 3)
#define GPIO14__SINA__CR_SIN1                            NPCM_PINCTRL_NUM_IDX(0x1, 4)
#define GPIO15__SOUTA__CR_SOUT1__SOUTA_P80               NPCM_PINCTRL_NUM_IDX(0x1, 5)
#define GPIO16__DCDA__SCL6B                              NPCM_PINCTRL_NUM_IDX(0x1, 6)
#define GPIO17__CLKOUT__RIA__SDA6B                       NPCM_PINCTRL_NUM_IDX(0x1, 7)
#define GPIOC0__CR_SIN3__SINC                            NPCM_PINCTRL_NUM_IDX(0xC, 0)
#define GPIOC1__CR_SOUT3__SOUTC                          NPCM_PINCTRL_NUM_IDX(0xC, 1)
#define GPIOC2__RTSC__SCL7A                              NPCM_PINCTRL_NUM_IDX(0xC, 2)
#define GPIOC3__CTSC__SDA7A                              NPCM_PINCTRL_NUM_IDX(0xC, 3)
#define GPIOC4__CR_SIN4__SIND                            NPCM_PINCTRL_NUM_IDX(0xC, 4)
#define GPIOC5__CR_SOUT4__SOUTD                          NPCM_PINCTRL_NUM_IDX(0xC, 5)
#define GPIOC6__RTSD__SCL8A                              NPCM_PINCTRL_NUM_IDX(0xC, 6)
#define GPIOC7__CTSD__SDA8A                              NPCM_PINCTRL_NUM_IDX(0xC, 7)
#define GPIOE3                                           NPCM_PINCTRL_NUM_IDX(0xE, 3)
#define GPIO20__MHZ_CLKIN__SLCT__YLW_LED                 NPCM_PINCTRL_NUM_IDX(0x2, 0)
#define GPIO21__BACK_CS__I3C1_SCL__DSRB__PE__CIRRX       NPCM_PINCTRL_NUM_IDX(0x2, 1)
#define GPIO22__I3C1_SDA__DTRB__BUSY__P2_DGL             NPCM_PINCTRL_NUM_IDX(0x2, 2)
#define GPIO23__PVT_CS__I3C2_SCL__DCDB__ACK              NPCM_PINCTRL_NUM_IDX(0x2, 3)
#define GPIO24__I3C2_SDA__RIB__PD7__P2_DGH               NPCM_PINCTRL_NUM_IDX(0x2, 4)
#define GPIO25__I3C3_SDA__PD6__LED_A__SDA1B              NPCM_PINCTRL_NUM_IDX(0x2, 5)
#define GPIO26__I3C3_SCL__PD5__LED_B__SCL1B              NPCM_PINCTRL_NUM_IDX(0x2, 6)
#define GPIO27__SMI__PD4__LED_C__BEEP                    NPCM_PINCTRL_NUM_IDX(0x2, 7)
#define GPIO30__SINB__CR_SIN2__PD3__LED_D                NPCM_PINCTRL_NUM_IDX(0x3, 0)
#define GPIO31__SOUTB__CR_SOUT2__PD2__LED_E              NPCM_PINCTRL_NUM_IDX(0x3, 1)
#define GPIO32__RTSB__PD1__LED_F__SCL9A                  NPCM_PINCTRL_NUM_IDX(0x3, 2)
#define GPIO33__CTSB__PD0__LED_G__SDA9A                  NPCM_PINCTRL_NUM_IDX(0x3, 3)
#define GPIO34__SWDIO_1_CR_SOUT1__SLIN__SCL5A            NPCM_PINCTRL_NUM_IDX(0x3, 4)
#define GPIO35__SWCLK_1_CR_SIN1__INIT__P1_DGL__SDA5A     NPCM_PINCTRL_NUM_IDX(0x3, 5)
#define GPIO36__ERR__VSBY_32KHZ_IN__PWM2__SCL4A          NPCM_PINCTRL_NUM_IDX(0x3, 6)
#define GPIO37__AFD__P1_DGH__TA2__SDA4A                  NPCM_PINCTRL_NUM_IDX(0x3, 7)
#define GPIOB6__ECSCI__STB__GRN_LED                      NPCM_PINCTRL_NUM_IDX(0xB, 6)
#define GPIO40__JTAG_TDI__DB_MOSI__MCLK                  NPCM_PINCTRL_NUM_IDX(0x4, 0)
#define GPIO41__SWCLK_0_CR_SIN1__JTAG_TCK__DB_SCK__MDAT  NPCM_PINCTRL_NUM_IDX(0x4, 1)
#define GPIO42__JTAG_TDO__DB_MISO__KCLK                  NPCM_PINCTRL_NUM_IDX(0x4, 2)
#define GPIO43__SWDIO_0_CR_SOUT1__JTAG_TMS__DB_SCE__KDAT NPCM_PINCTRL_NUM_IDX(0x4, 3)
#define GPIO50__PSOUT                                    NPCM_PINCTRL_NUM_IDX(0x5, 0)
#define GPIO51__PSIN                                     NPCM_PINCTRL_NUM_IDX(0x5, 1)
#define GPIO52__PSON                                     NPCM_PINCTRL_NUM_IDX(0x5, 2)
#define GPIO53__SLP_S3                                   NPCM_PINCTRL_NUM_IDX(0x5, 3)
#define GPIO54__PME__ECSCI                               NPCM_PINCTRL_NUM_IDX(0x5, 4)
#define GPIO44__SHD_CS                                   NPCM_PINCTRL_NUM_IDX(0x4, 4)
#define GPIO45__SHD_DIO0                                 NPCM_PINCTRL_NUM_IDX(0x4, 5)
#define GPIO46__SHD_DIO1                                 NPCM_PINCTRL_NUM_IDX(0x4, 6)
#define GPIO47__SHD_SCLK                                 NPCM_PINCTRL_NUM_IDX(0x4, 7)
#define GPIO55__SHD_DIO2__WP_GPIO55                      NPCM_PINCTRL_NUM_IDX(0x5, 5)
#define GPIO56__SHD_DIO3                                 NPCM_PINCTRL_NUM_IDX(0x5, 6)
#define GPIOE1__DPWROK                                   NPCM_PINCTRL_NUM_IDX(0xE, 1)
#define GPIOE0__DEEP_S5__3VSBSW                          NPCM_PINCTRL_NUM_IDX(0xE, 0)
#define GPIO60__RSTOUT2__SCL1A                           NPCM_PINCTRL_NUM_IDX(0x6, 0)
#define GPIO61__RSTOUT1__SDA1A                           NPCM_PINCTRL_NUM_IDX(0x6, 1)
#define GPIO62__RSTOUT0                                  NPCM_PINCTRL_NUM_IDX(0x6, 2)
#define GPIO63__ATXPGD                                   NPCM_PINCTRL_NUM_IDX(0x6, 3)
#define GPIOE2__PWROK0                                   NPCM_PINCTRL_NUM_IDX(0xE, 2)
#define GPIO57                                           NPCM_PINCTRL_NUM_IDX(0x5, 7)
#define GPIO64__RESETCON__DPWROKI__PWM3                  NPCM_PINCTRL_NUM_IDX(0x6, 4)
#define GPIO65__SLP_S5                                   NPCM_PINCTRL_NUM_IDX(0x6, 5)
#define GPIO66__SPIP1_CLK                                NPCM_PINCTRL_NUM_IDX(0x6, 6)
#define GPIO67__SPIP1_MOSI                               NPCM_PINCTRL_NUM_IDX(0x6, 7)
#define GPIO70__SPIP1_DIO1                               NPCM_PINCTRL_NUM_IDX(0x7, 0)
#define GPIO71__SLP_SUS                                  NPCM_PINCTRL_NUM_IDX(0x7, 1)
#define GPIO72__FW_RDY                                   NPCM_PINCTRL_NUM_IDX(0x7, 2)
#define GPIO73__SPIP1_CS                                 NPCM_PINCTRL_NUM_IDX(0x7, 3)
#define GPIOD0__I3C4_SCL__SINE                           NPCM_PINCTRL_NUM_IDX(0xD, 0)
#define GPIOD1__I3C4_SDA__SOUTE                          NPCM_PINCTRL_NUM_IDX(0xD, 1)
#define GPIOD2__RTSE__SCL10A                             NPCM_PINCTRL_NUM_IDX(0xD, 2)
#define GPIOD3__CTSE__SDA10A                             NPCM_PINCTRL_NUM_IDX(0xD, 3)
#define GPIOD4__SINF__SCL11A                             NPCM_PINCTRL_NUM_IDX(0xD, 4)
#define GPIOD5__SOUTF__SDA11A                            NPCM_PINCTRL_NUM_IDX(0xD, 5)
#define GPIOD6__RTSF__SCL12A                             NPCM_PINCTRL_NUM_IDX(0xD, 6)
#define GPIOD7__CTSF__SDA12A                             NPCM_PINCTRL_NUM_IDX(0xD, 7)
#define GPIO74__IOX1_LDSH                                NPCM_PINCTRL_NUM_IDX(0x7, 4)
#define GPIO75__IOX1_DOUT                                NPCM_PINCTRL_NUM_IDX(0x7, 5)
#define GPIO76__WP_GPIO76__SPIP1_DIO2                    NPCM_PINCTRL_NUM_IDX(0x7, 6)
#define GPIO77__SPIP1_DIO3                               NPCM_PINCTRL_NUM_IDX(0x7, 7)
#define GPIO80__KBC_TEST__PCHVSB                         NPCM_PINCTRL_NUM_IDX(0x8, 0)
#define GPIO81__TA3                                      NPCM_PINCTRL_NUM_IDX(0x8, 1)
#define GPIOE4__CASEOPEN                                 NPCM_PINCTRL_NUM_IDX(0xE, 4)
#define GPIOE5__RSMRST                                   NPCM_PINCTRL_NUM_IDX(0xE, 5)
#define GPIOE6__SKTOCC                                   NPCM_PINCTRL_NUM_IDX(0xE, 6)
#define GPIO82__I3C5_SCL__PWM0                           NPCM_PINCTRL_NUM_IDX(0x8, 2)
#define GPIO83__I3C5_SDA__TA0                            NPCM_PINCTRL_NUM_IDX(0x8, 3)
#define GPIO84                                           NPCM_PINCTRL_NUM_IDX(0x8, 4)
#define GPIO85                                           NPCM_PINCTRL_NUM_IDX(0x8, 5)
#define GPIOF0__SCL4B                                    NPCM_PINCTRL_NUM_IDX(0xF, 0)
#define GPIOF1__SDA4B                                    NPCM_PINCTRL_NUM_IDX(0xF, 1)
#define GPIOF2__IOX1_DIN                                 NPCM_PINCTRL_NUM_IDX(0xF, 2)
#define GPIOF3__IOX1_SCLK                                NPCM_PINCTRL_NUM_IDX(0xF, 3)
#define GPIO86__VIN7                                     NPCM_PINCTRL_NUM_IDX(0x8, 6)
#define GPIO87__VIN5                                     NPCM_PINCTRL_NUM_IDX(0x8, 7)
#define GPIO90__THR16_VIN16_TD2P__DSMOUT                 NPCM_PINCTRL_NUM_IDX(0x9, 0)
#define GPIO91__THR15_VIN15_TD1P                         NPCM_PINCTRL_NUM_IDX(0x9, 1)
#define GPIO92__THR14_VIN14_TD0P                         NPCM_PINCTRL_NUM_IDX(0x9, 2)
#define GPIO93__THR1_VIN1_TD3P__DSADC_CLK                NPCM_PINCTRL_NUM_IDX(0x9, 3)
#define GPIO94__VIN2_THR2_TD4P                           NPCM_PINCTRL_NUM_IDX(0x9, 4)
#define GPIO95__ATX5VSB__VIN3                            NPCM_PINCTRL_NUM_IDX(0x9, 5)
#define GPIO96__I3C6_SCL__SCL6A                          NPCM_PINCTRL_NUM_IDX(0x9, 6)
#define GPIO97__PECI__I3C6_SDA__SDA6A                    NPCM_PINCTRL_NUM_IDX(0x9, 7)
#define GPIOA0__PWM4__IOX2_LDSH                          NPCM_PINCTRL_NUM_IDX(0xA, 0)
#define GPIOA1__TA4__IOX2_DOUT                           NPCM_PINCTRL_NUM_IDX(0xA, 1)
#define GPIOA2__SWDIO_2_CR_SOUT1__SDA3A                  NPCM_PINCTRL_NUM_IDX(0xA, 2)
#define GPIOA3__PWM5__IOX2_DIN                           NPCM_PINCTRL_NUM_IDX(0xA, 3)
#define GPIOA4__TA5__IOX2_SCLK                           NPCM_PINCTRL_NUM_IDX(0xA, 4)
#define GPIOA5__PWM6                                     NPCM_PINCTRL_NUM_IDX(0xA, 5)
#define GPIOA6__TA6__USB20                               NPCM_PINCTRL_NUM_IDX(0xA, 6)
#define GPIOA7__SWCLK_2_CR_SIN1__SCL3A                   NPCM_PINCTRL_NUM_IDX(0xA, 7)
#define RES_GPIOE7                                       NPCM_PINCTRL_NUM_IDX(0xE, 7)
#define RES_GPIOF4                                       NPCM_PINCTRL_NUM_IDX(0xF, 4)
#define RES_GPIOF5                                       NPCM_PINCTRL_NUM_IDX(0xF, 5)
#define RES_GPIOF6                                       NPCM_PINCTRL_NUM_IDX(0xF, 6)
#define RES_GPIOF7                                       NPCM_PINCTRL_NUM_IDX(0xF, 7)

/* NPCM pin control offset calculation */
#define NPCM_PINCTRL_SHIFT           (3)
#define NPCM_PINCTRL_MASK            (0xFF)
#define NPCM_PINCTRL_OFFSET(N)       (((N) & NPCM_PINCTRL_MASK) << NPCM_PINCTRL_SHIFT)
#define NPCM_PINCTRL_BIT_MASK        ((1 << NPCM_PINCTRL_SHIFT) - 1)
#define NPCM_PINCTRL_BIT_OFFSET(N)   ((N) & NPCM_PINCTRL_BIT_MASK)
#define NPCM_PINCTRL_IDX(group, bit) (NPCM_PINCTRL_OFFSET(group) + NPCM_PINCTRL_BIT_OFFSET(bit))

/*
 * Device alternate function reg and bit index
 */
/* 0x0B-0x0D: DEVALT10~12 */
#define NPCM_PINCTRL_ALT10_0_CRGPIO_RST_SL    NPCM_PINCTRL_IDX(0x0B, 0)
#define NPCM_PINCTRL_ALT10_1_I3C2_SL          NPCM_PINCTRL_IDX(0x0B, 1)
#define NPCM_PINCTRL_ALT10_2_I3C3_SL          NPCM_PINCTRL_IDX(0x0B, 2)
#define NPCM_PINCTRL_ALT10_3_I3C4_SL          NPCM_PINCTRL_IDX(0x0B, 3)
#define NPCM_PINCTRL_ALT10_4_SMI_SL           NPCM_PINCTRL_IDX(0x0B, 4)
#define NPCM_PINCTRL_ALT10_5_ECSCI_SL         NPCM_PINCTRL_IDX(0x0B, 5)
#define NPCM_PINCTRL_ALT10_6_I3C1_SL          NPCM_PINCTRL_IDX(0x0B, 6)
#define NPCM_PINCTRL_ALT11_1_MIWU_ACPI_EN     NPCM_PINCTRL_IDX(0x0C, 1)
#define NPCM_PINCTRL_ALT11_2_SIB_SYNC_ALL     NPCM_PINCTRL_IDX(0x0C, 2)
#define NPCM_PINCTRL_ALT11_3_SIB_SYNC_ADDRESS NPCM_PINCTRL_IDX(0x0C, 3)

/* 0x10-0x1F: DEVALT0~F */
#define NPCM_PINCTRL_ALT0_1_FIU_PVT_CS_SL    NPCM_PINCTRL_IDX(0x10, 1)
#define NPCM_PINCTRL_ALT0_2_FIU_BACK_CS_SL   NPCM_PINCTRL_IDX(0x10, 2)
#define NPCM_PINCTRL_ALT0_5_CLKRN_SL         NPCM_PINCTRL_IDX(0x10, 5)
#define NPCM_PINCTRL_ALT0_6_LPCPD_SL         NPCM_PINCTRL_IDX(0x10, 6)
#define NPCM_PINCTRL_ALT0_7_SERIRQ_SL        NPCM_PINCTRL_IDX(0x10, 7)
#define NPCM_PINCTRL_ALT1_1_VIN1_SL          NPCM_PINCTRL_IDX(0x11, 1)
#define NPCM_PINCTRL_ALT1_2_VIN2_SL          NPCM_PINCTRL_IDX(0x11, 2)
#define NPCM_PINCTRL_ALT1_3_VIN3_SL          NPCM_PINCTRL_IDX(0x11, 3)
#define NPCM_PINCTRL_ALT1_4_ATX5VVSB_SL      NPCM_PINCTRL_IDX(0x11, 4)
#define NPCM_PINCTRL_ALT1_5_VIN5_SL          NPCM_PINCTRL_IDX(0x11, 5)
#define NPCM_PINCTRL_ALT1_7_VIN7_SL          NPCM_PINCTRL_IDX(0x11, 7)
#define NPCM_PINCTRL_ALT2_0_VIN14_SL         NPCM_PINCTRL_IDX(0x12, 0)
#define NPCM_PINCTRL_ALT2_1_VIN15_SL         NPCM_PINCTRL_IDX(0x12, 1)
#define NPCM_PINCTRL_ALT2_2_VIN16_SL         NPCM_PINCTRL_IDX(0x12, 2)
#define NPCM_PINCTRL_ALT2_3_RES_SL           NPCM_PINCTRL_IDX(0x12, 3)
#define NPCM_PINCTRL_ALT3_0_TA0_SL           NPCM_PINCTRL_IDX(0x13, 0)
#define NPCM_PINCTRL_ALT3_1_TA1_SL           NPCM_PINCTRL_IDX(0x13, 1)
#define NPCM_PINCTRL_ALT3_2_TA2_SL           NPCM_PINCTRL_IDX(0x13, 2)
#define NPCM_PINCTRL_ALT3_3_TA3_SL           NPCM_PINCTRL_IDX(0x13, 3)
#define NPCM_PINCTRL_ALT3_4_TA4_SL           NPCM_PINCTRL_IDX(0x13, 4)
#define NPCM_PINCTRL_ALT3_5_TA5_SL           NPCM_PINCTRL_IDX(0x13, 5)
#define NPCM_PINCTRL_ALT3_6_TA6_SL           NPCM_PINCTRL_IDX(0x13, 6)
#define NPCM_PINCTRL_ALT3_7_TA7_SL           NPCM_PINCTRL_IDX(0x13, 7)
#define NPCM_PINCTRL_ALT4_0_TA8_SL           NPCM_PINCTRL_IDX(0x14, 0)
#define NPCM_PINCTRL_ALT4_1_TA9_SL           NPCM_PINCTRL_IDX(0x14, 1)
#define NPCM_PINCTRL_ALT4_2_PWM0_SL          NPCM_PINCTRL_IDX(0x14, 2)
#define NPCM_PINCTRL_ALT4_3_PWM1_SL          NPCM_PINCTRL_IDX(0x14, 3)
#define NPCM_PINCTRL_ALT4_4_PWM2_SL          NPCM_PINCTRL_IDX(0x14, 4)
#define NPCM_PINCTRL_ALT4_5_PWM3_SL          NPCM_PINCTRL_IDX(0x14, 5)
#define NPCM_PINCTRL_ALT4_6_PWM4_SL          NPCM_PINCTRL_IDX(0x14, 6)
#define NPCM_PINCTRL_ALT4_7_PWM5_SL          NPCM_PINCTRL_IDX(0x14, 7)
#define NPCM_PINCTRL_ALT5_0_PWM6_SL          NPCM_PINCTRL_IDX(0x15, 0)
#define NPCM_PINCTRL_ALT5_1_PWM7_SL          NPCM_PINCTRL_IDX(0x15, 1)
#define NPCM_PINCTRL_ALT5_2_PWM8_SL          NPCM_PINCTRL_IDX(0x15, 2)
#define NPCM_PINCTRL_ALT5_3_PWM9_SL          NPCM_PINCTRL_IDX(0x15, 3)
#define NPCM_PINCTRL_ALT5_4_PECI_EN          NPCM_PINCTRL_IDX(0x15, 4)
#define NPCM_PINCTRL_ALT5_5_ECSCI_SL         NPCM_PINCTRL_IDX(0x15, 5)
#define NPCM_PINCTRL_ALT5_6_SMI_SL           NPCM_PINCTRL_IDX(0x15, 6)
#define NPCM_PINCTRL_ALT6_0_P80_D_INV        NPCM_PINCTRL_IDX(0x16, 0)
#define NPCM_PINCTRL_ALT6_1_BEEP_SEL         NPCM_PINCTRL_IDX(0x16, 1)
#define NPCM_PINCTRL_ALT6_2_PRT_SL           NPCM_PINCTRL_IDX(0x16, 2)
#define NPCM_PINCTRL_ALT6_3_PORT80_SL        NPCM_PINCTRL_IDX(0x16, 3)
#define NPCM_PINCTRL_ALT6_4_P80_G_INV        NPCM_PINCTRL_IDX(0x16, 4)
#define NPCM_PINCTRL_ALT6_5_ESPI_S5S4_SL     NPCM_PINCTRL_IDX(0x16, 5)
#define NPCM_PINCTRL_ALT6_6_ESPI_OUT_SEL     NPCM_PINCTRL_IDX(0x16, 6)
#define NPCM_PINCTRL_ALT6_7_UART_EXT_SEL     NPCM_PINCTRL_IDX(0x16, 7)
#define NPCM_PINCTRL_ALT7_0_GP72_SL          NPCM_PINCTRL_IDX(0x17, 0)
#define NPCM_PINCTRL_ALT7_1_CIRRX_SL         NPCM_PINCTRL_IDX(0x17, 1)
#define NPCM_PINCTRL_ALT7_2_I3C6_SL          NPCM_PINCTRL_IDX(0x17, 2)
#define NPCM_PINCTRL_ALT7_3_GP96_SL          NPCM_PINCTRL_IDX(0x17, 3)
#define NPCM_PINCTRL_ALT7_4_GP97_SL          NPCM_PINCTRL_IDX(0x17, 4)
#define NPCM_PINCTRL_ALT7_5_EXT32KSL         NPCM_PINCTRL_IDX(0x17, 5)
#define NPCM_PINCTRL_ALT7_6_CLKOUT_SL        NPCM_PINCTRL_IDX(0x17, 6)
#define NPCM_PINCTRL_ALT7_7_CLKOM_RTC32K     NPCM_PINCTRL_IDX(0x17, 7)
#define NPCM_PINCTRL_ALT8_0_KB_IRQTOCORE_EN  NPCM_PINCTRL_IDX(0x18, 0)
#define NPCM_PINCTRL_ALT8_1_MS_IRQTOCORE_EN  NPCM_PINCTRL_IDX(0x18, 1)
#define NPCM_PINCTRL_ALT8_2_CIR_IRQTOCORE_EN NPCM_PINCTRL_IDX(0x18, 2)
#define NPCM_PINCTRL_ALT8_3_PRT_IRQTOCORE_EN NPCM_PINCTRL_IDX(0x18, 3)
#define NPCM_PINCTRL_ALT9_0_RTS2_SL          NPCM_PINCTRL_IDX(0x19, 0)
#define NPCM_PINCTRL_ALT9_1_SP2I_SL          NPCM_PINCTRL_IDX(0x19, 1)
#define NPCM_PINCTRL_ALT9_2_SP2O_SL          NPCM_PINCTRL_IDX(0x19, 2)
#define NPCM_PINCTRL_ALT9_3_SP2CTL_SL        NPCM_PINCTRL_IDX(0x19, 3)
#define NPCM_PINCTRL_ALT9_4_CTS2_SL          NPCM_PINCTRL_IDX(0x19, 4)
#define NPCM_PINCTRL_ALT9_5_RI2_SL           NPCM_PINCTRL_IDX(0x19, 5)
#define NPCM_PINCTRL_ALT9_6_DPWORK_SL        NPCM_PINCTRL_IDX(0x19, 6)
#define NPCM_PINCTRL_ALT9_7_S0IX_SL          NPCM_PINCTRL_IDX(0x19, 7)
#define NPCM_PINCTRL_ALTA_0_SMB1A_SL         NPCM_PINCTRL_IDX(0x1A, 0)
#define NPCM_PINCTRL_ALTA_1_SMB3A_SL         NPCM_PINCTRL_IDX(0x1A, 1)
#define NPCM_PINCTRL_ALTA_2_SMB4A_SL         NPCM_PINCTRL_IDX(0x1A, 2)
#define NPCM_PINCTRL_ALTA_3_SMB5A_SL         NPCM_PINCTRL_IDX(0x1A, 3)
#define NPCM_PINCTRL_ALTA_4_I3C5_SL          NPCM_PINCTRL_IDX(0x1A, 4)
#define NPCM_PINCTRL_ALTA_5_SMB1B_SL         NPCM_PINCTRL_IDX(0x1A, 5)
#define NPCM_PINCTRL_ALTA_6_URTO1_SL         NPCM_PINCTRL_IDX(0x1A, 6)
#define NPCM_PINCTRL_ALTA_7_URTO2_SL         NPCM_PINCTRL_IDX(0x1A, 7)
#define NPCM_PINCTRL_ALTB_0_RTS1_SL          NPCM_PINCTRL_IDX(0x1B, 0)
#define NPCM_PINCTRL_ALTB_1_URTI_SL          NPCM_PINCTRL_IDX(0x1B, 1)
#define NPCM_PINCTRL_ALTB_2_SP1I_SL          NPCM_PINCTRL_IDX(0x1B, 2)
#define NPCM_PINCTRL_ALTB_3_SP1O_SL          NPCM_PINCTRL_IDX(0x1B, 3)
#define NPCM_PINCTRL_ALTB_4_SP1CTL_SL        NPCM_PINCTRL_IDX(0x1B, 4)
#define NPCM_PINCTRL_ALTB_5_CTS1_SL          NPCM_PINCTRL_IDX(0x1B, 5)
#define NPCM_PINCTRL_ALTB_6_RI1_SL           NPCM_PINCTRL_IDX(0x1B, 6)
#define NPCM_PINCTRL_ALTB_7_LDRQ_SL          NPCM_PINCTRL_IDX(0x1B, 7)
#define NPCM_PINCTRL_ALTC_1_SPIP1_SL         NPCM_PINCTRL_IDX(0x1C, 1)
#define NPCM_PINCTRL_ALTC_2_SHD_SPI_QUAD     NPCM_PINCTRL_IDX(0x1C, 2)
#define NPCM_PINCTRL_ALTC_3_SHD_SPI          NPCM_PINCTRL_IDX(0x1C, 3)
#define NPCM_PINCTRL_ALTC_4_SPIP_DIO23_GPIO  NPCM_PINCTRL_IDX(0x1C, 4)
#define NPCM_PINCTRL_ALTC_6_URTI1_SL         NPCM_PINCTRL_IDX(0x1C, 6)
#define NPCM_PINCTRL_ALTC_7_SOUTA_P80_SL     NPCM_PINCTRL_IDX(0x1C, 7)
#define NPCM_PINCTRL_ALTD_2_GPIOE0_LV        NPCM_PINCTRL_IDX(0x1D, 2)
#define NPCM_PINCTRL_ALTD_3_GPIOE1_LV        NPCM_PINCTRL_IDX(0x1D, 3)
#define NPCM_PINCTRL_ALTD_4_CASEOPEN_LV      NPCM_PINCTRL_IDX(0x1D, 4)
#define NPCM_PINCTRL_ALTD_5_SKTOCC_LV        NPCM_PINCTRL_IDX(0x1D, 5)
#define NPCM_PINCTRL_ALTD_6_GPIOE2_LV        NPCM_PINCTRL_IDX(0x1D, 6)
#define NPCM_PINCTRL_ALTE_0_KBC_TEST         NPCM_PINCTRL_IDX(0x1E, 0)
#define NPCM_PINCTRL_ALTE_3_KBC_SEL          NPCM_PINCTRL_IDX(0x1E, 3)
#define NPCM_PINCTRL_ALTE_5_SMB2A_SL         NPCM_PINCTRL_IDX(0x1E, 5)
#define NPCM_PINCTRL_ALTE_6_GRN_LED_SEL      NPCM_PINCTRL_IDX(0x1E, 6)
#define NPCM_PINCTRL_ALTE_7_YLW_LED_SEL      NPCM_PINCTRL_IDX(0x1E, 7)
#define NPCM_PINCTRL_ALTF_7_GPIOE5_LV        NPCM_PINCTRL_IDX(0x1F, 7)

/* 0x24: DEVALTCX */
#define NPCM_PINCTRL_ALTCX_7_GPIO_OUT_PULLEN NPCM_PINCTRL_IDX(0x24, 7)

/* 0x2E: DEVALT2E */
#define NPCM_PINCTRL_ALT2E_4_USBDPHY_EN     NPCM_PINCTRL_IDX(0x2E, 4)
#define NPCM_PINCTRL_ALT2E_6_USBDPHY_CLKSEL NPCM_PINCTRL_IDX(0x2E, 6)

/* 0x30-0x34: DEVALT30~34 */
#define NPCM_PINCTRL_ALT30_0_VHIF_PAD_TEST_D   NPCM_PINCTRL_IDX(0x30, 0)
#define NPCM_PINCTRL_ALT30_1_VHIF_PAD_TEST_EN  NPCM_PINCTRL_IDX(0x30, 1)
#define NPCM_PINCTRL_ALT30_2_VSPI_PAD_TEST_D   NPCM_PINCTRL_IDX(0x30, 2)
#define NPCM_PINCTRL_ALT30_3_VSPI_PAD_TEST_EN  NPCM_PINCTRL_IDX(0x30, 3)
#define NPCM_PINCTRL_ALT31_0_FWCTL_CRUART_1    NPCM_PINCTRL_IDX(0x31, 0)
#define NPCM_PINCTRL_ALT31_1_FWCTL_CRUART_1N   NPCM_PINCTRL_IDX(0x31, 1)
#define NPCM_PINCTRL_ALT31_2_FWCTL_CRUART_2    NPCM_PINCTRL_IDX(0x31, 2)
#define NPCM_PINCTRL_ALT31_3_FWCTL_CRUART_2N   NPCM_PINCTRL_IDX(0x31, 3)
#define NPCM_PINCTRL_ALT31_4_URTI2_SL          NPCM_PINCTRL_IDX(0x31, 4)
#define NPCM_PINCTRL_ALT31_5_URTO3_SL          NPCM_PINCTRL_IDX(0x31, 5)
#define NPCM_PINCTRL_ALT31_6_URTI3_SL          NPCM_PINCTRL_IDX(0x31, 6)
#define NPCM_PINCTRL_ALT31_7_URTO4_SL          NPCM_PINCTRL_IDX(0x31, 7)
#define NPCM_PINCTRL_ALT32_0_UARTF_IRQ2CORE_EN NPCM_PINCTRL_IDX(0x32, 0)
#define NPCM_PINCTRL_ALT32_1_UARTE_IRQ2CORE_EN NPCM_PINCTRL_IDX(0x32, 1)
#define NPCM_PINCTRL_ALT32_2_UARTD_IRQ2CORE_EN NPCM_PINCTRL_IDX(0x32, 2)
#define NPCM_PINCTRL_ALT32_3_UARTC_IRQ2CORE_EN NPCM_PINCTRL_IDX(0x32, 3)
#define NPCM_PINCTRL_ALT32_4_UARTB_IRQ2CORE_EN NPCM_PINCTRL_IDX(0x32, 4)
#define NPCM_PINCTRL_ALT32_5_UARTA_IRQ2CORE_EN NPCM_PINCTRL_IDX(0x32, 5)
#define NPCM_PINCTRL_ALT34_6_RES               NPCM_PINCTRL_IDX(0x34, 6)

/* 0x50-0x5F: DEVALT50~5F */
#define NPCM_PINCTRL_ALT50_0_ACPI_HW_SW_CTL_0_0      NPCM_PINCTRL_IDX(0x50, 0)
#define NPCM_PINCTRL_ALT50_1_ACPI_HW_SW_CTL_0_1      NPCM_PINCTRL_IDX(0x50, 1)
#define NPCM_PINCTRL_ALT50_2_ACPI_HW_SW_CTL_0_2      NPCM_PINCTRL_IDX(0x50, 2)
#define NPCM_PINCTRL_ALT50_3_ACPI_HW_SW_CTL_0_3      NPCM_PINCTRL_IDX(0x50, 3)
#define NPCM_PINCTRL_ALT50_5_ACPI_HW_SW_CTL_0_5      NPCM_PINCTRL_IDX(0x50, 5)
#define NPCM_PINCTRL_ALT51_0_ACPI_HW_SW_CTL_1_0      NPCM_PINCTRL_IDX(0x51, 0)
#define NPCM_PINCTRL_ALT51_1_ACPI_HW_SW_CTL_1_1      NPCM_PINCTRL_IDX(0x51, 1)
#define NPCM_PINCTRL_ALT51_2_ACPI_HW_SW_CTL_1_2      NPCM_PINCTRL_IDX(0x51, 2)
#define NPCM_PINCTRL_ALT52_0_ACPI_HW_SW_CTL_2_0      NPCM_PINCTRL_IDX(0x52, 0)
#define NPCM_PINCTRL_ALT52_1_ACPI_HW_SW_CTL_2_1      NPCM_PINCTRL_IDX(0x52, 1)
#define NPCM_PINCTRL_ALT52_2_ACPI_HW_SW_CTL_2_2      NPCM_PINCTRL_IDX(0x52, 2)
#define NPCM_PINCTRL_ALT52_3_ACPI_HW_SW_CTL_2_3      NPCM_PINCTRL_IDX(0x52, 3)
#define NPCM_PINCTRL_ALT53_0_ACPI_HW_SW_VAL_0_0      NPCM_PINCTRL_IDX(0x53, 0)
#define NPCM_PINCTRL_ALT53_1_ACPI_HW_SW_VAL_0_1      NPCM_PINCTRL_IDX(0x53, 1)
#define NPCM_PINCTRL_ALT53_2_ACPI_HW_SW_VAL_0_2      NPCM_PINCTRL_IDX(0x53, 2)
#define NPCM_PINCTRL_ALT53_3_ACPI_HW_SW_VAL_0_3      NPCM_PINCTRL_IDX(0x53, 3)
#define NPCM_PINCTRL_ALT53_5_ACPI_HW_SW_VAL_0_5      NPCM_PINCTRL_IDX(0x53, 5)
#define NPCM_PINCTRL_ALT54_0_ACPI_HW_SW_VAL_1_0      NPCM_PINCTRL_IDX(0x54, 0)
#define NPCM_PINCTRL_ALT54_1_ACPI_HW_SW_VAL_1_1      NPCM_PINCTRL_IDX(0x54, 1)
#define NPCM_PINCTRL_ALT54_2_ACPI_HW_SW_VAL_1_2      NPCM_PINCTRL_IDX(0x54, 2)
#define NPCM_PINCTRL_ALT55_0_ACPI_HW_SW_VAL_2_0      NPCM_PINCTRL_IDX(0x55, 0)
#define NPCM_PINCTRL_ALT55_1_ACPI_HW_SW_VAL_2_1      NPCM_PINCTRL_IDX(0x55, 1)
#define NPCM_PINCTRL_ALT55_2_ACPI_HW_SW_VAL_2_2      NPCM_PINCTRL_IDX(0x55, 2)
#define NPCM_PINCTRL_ALT55_3_ACPI_HW_SW_VAL_2_3      NPCM_PINCTRL_IDX(0x55, 3)
#define NPCM_PINCTRL_ALT59_1_ACPI_HW_OD_SEL_0_1      NPCM_PINCTRL_IDX(0x59, 1)
#define NPCM_PINCTRL_ALT59_2_ACPI_HW_OD_SEL_0_2      NPCM_PINCTRL_IDX(0x59, 2)
#define NPCM_PINCTRL_ALT59_3_ACPI_HW_OD_SEL_0_3      NPCM_PINCTRL_IDX(0x59, 3)
#define NPCM_PINCTRL_ALT59_5_ACPI_HW_OD_SEL_0_5      NPCM_PINCTRL_IDX(0x59, 5)
#define NPCM_PINCTRL_ALT5A_0_ACPI_HW_OD_SEL_1_0      NPCM_PINCTRL_IDX(0x5A, 0)
#define NPCM_PINCTRL_ALT5A_1_ACPI_HW_OD_SEL_1_1      NPCM_PINCTRL_IDX(0x5A, 1)
#define NPCM_PINCTRL_ALT5A_2_ACPI_HW_OD_SEL_1_2      NPCM_PINCTRL_IDX(0x5A, 2)
#define NPCM_PINCTRL_ALT5B_0_ACPI_HW_OD_SEL_2_0      NPCM_PINCTRL_IDX(0x5B, 0)
#define NPCM_PINCTRL_ALT5B_1_ACPI_HW_OD_SEL_2_1      NPCM_PINCTRL_IDX(0x5B, 1)
#define NPCM_PINCTRL_ALT5B_2_ACPI_HW_OD_SEL_2_2      NPCM_PINCTRL_IDX(0x5B, 2)
#define NPCM_PINCTRL_ALT5B_3_ACPI_HW_OD_SEL_2_3      NPCM_PINCTRL_IDX(0x5B, 3)
#define NPCM_PINCTRL_ALT5B_6_ACPI_HW_OD_SEL_2_6      NPCM_PINCTRL_IDX(0x5B, 6)
#define NPCM_PINCTRL_ALT5B_7_ACPI_HW_OD_SEL_2_7      NPCM_PINCTRL_IDX(0x5B, 7)
#define NPCM_PINCTRL_ALT5C_0_ACPI_GPIO51_SEL         NPCM_PINCTRL_IDX(0x5C, 0)
#define NPCM_PINCTRL_ALT5C_1_ACPI_GPIO50_SEL         NPCM_PINCTRL_IDX(0x5C, 1)
#define NPCM_PINCTRL_ALT5C_2_ACPI_GPIO52_SEL         NPCM_PINCTRL_IDX(0x5C, 2)
#define NPCM_PINCTRL_ALT5C_3_ACPI_GPIO54_SEL         NPCM_PINCTRL_IDX(0x5C, 3)
#define NPCM_PINCTRL_ALT5C_4_ACPI_GPIO64_SEL         NPCM_PINCTRL_IDX(0x5C, 4)
#define NPCM_PINCTRL_ALT5C_5_ACPI_GPIO71_SEL         NPCM_PINCTRL_IDX(0x5C, 5)
#define NPCM_PINCTRL_ALT5D_0_ACPI_GPIO62_SEL         NPCM_PINCTRL_IDX(0x5D, 0)
#define NPCM_PINCTRL_ALT5D_1_ACPI_GPIO61_SEL         NPCM_PINCTRL_IDX(0x5D, 1)
#define NPCM_PINCTRL_ALT5D_2_ACPI_GPIO60_SEL         NPCM_PINCTRL_IDX(0x5D, 2)
#define NPCM_PINCTRL_ALT5D_4_ACPI_GPIOE6_SEL         NPCM_PINCTRL_IDX(0x5D, 4)
#define NPCM_PINCTRL_ALT5D_5_ACPI_GPIOE4_SEL         NPCM_PINCTRL_IDX(0x5D, 5)
#define NPCM_PINCTRL_ALT5E_0_ACPI_GPIOE1_SEL         NPCM_PINCTRL_IDX(0x5E, 0)
#define NPCM_PINCTRL_ALT5E_1_ACPI_GPIOE5_SEL         NPCM_PINCTRL_IDX(0x5E, 1)
#define NPCM_PINCTRL_ALT5E_2_ACPI_GPIOE0_DEEP_S5_SEL NPCM_PINCTRL_IDX(0x5E, 2)
#define NPCM_PINCTRL_ALT5E_3_ACPI_GPIOE2_SEL         NPCM_PINCTRL_IDX(0x5E, 3)
#define NPCM_PINCTRL_ALT5E_5_ACPI_GPIOE0_SEL         NPCM_PINCTRL_IDX(0x5E, 5)
#define NPCM_PINCTRL_ALT5F_0_ACPI_GPIO53_SEL         NPCM_PINCTRL_IDX(0x5F, 0)
#define NPCM_PINCTRL_ALT5F_1_ACPI_GPIO65_SEL         NPCM_PINCTRL_IDX(0x5F, 1)
#define NPCM_PINCTRL_ALT5F_2_ACPI_GPIO63_SEL         NPCM_PINCTRL_IDX(0x5F, 2)
#define NPCM_PINCTRL_ALT5F_3_ACPI_GPIO64_SEL         NPCM_PINCTRL_IDX(0x5F, 3)
#define NPCM_PINCTRL_ALT5F_5_ACPI_GPIO53_65_SEL      NPCM_PINCTRL_IDX(0x5F, 5)
#define NPCM_PINCTRL_ALT5F_7_VW473_WK                NPCM_PINCTRL_IDX(0x5F, 7)

/* 0x62: DEVALT62 */
#define NPCM_PINCTRL_ALT62_0_SINC_SL  NPCM_PINCTRL_IDX(0x62, 0)
#define NPCM_PINCTRL_ALT62_1_SOUTC_SL NPCM_PINCTRL_IDX(0x62, 1)
#define NPCM_PINCTRL_ALT62_2_RTSC_SL  NPCM_PINCTRL_IDX(0x62, 2)
#define NPCM_PINCTRL_ALT62_3_CTSC_SL  NPCM_PINCTRL_IDX(0x62, 3)
#define NPCM_PINCTRL_ALT62_4_SIND_SL  NPCM_PINCTRL_IDX(0x62, 4)
#define NPCM_PINCTRL_ALT62_5_SOUTD_SL NPCM_PINCTRL_IDX(0x62, 5)
#define NPCM_PINCTRL_ALT62_6_RTSD_SL  NPCM_PINCTRL_IDX(0x62, 6)
#define NPCM_PINCTRL_ALT62_7_CTSD_SL  NPCM_PINCTRL_IDX(0x62, 7)

/* 0x66-0x6D: DEVALT66~6D */
#define NPCM_PINCTRL_ALT66_0_SINE_SL        NPCM_PINCTRL_IDX(0x66, 0)
#define NPCM_PINCTRL_ALT66_1_SOUTE_SL       NPCM_PINCTRL_IDX(0x66, 1)
#define NPCM_PINCTRL_ALT66_2_RTSE_SL        NPCM_PINCTRL_IDX(0x66, 2)
#define NPCM_PINCTRL_ALT66_3_CTSE_SL        NPCM_PINCTRL_IDX(0x66, 3)
#define NPCM_PINCTRL_ALT66_4_SINF_SL        NPCM_PINCTRL_IDX(0x66, 4)
#define NPCM_PINCTRL_ALT66_5_SOUTF_SL       NPCM_PINCTRL_IDX(0x66, 5)
#define NPCM_PINCTRL_ALT66_6_RTSF_SL        NPCM_PINCTRL_IDX(0x66, 6)
#define NPCM_PINCTRL_ALT66_7_CTSF_SL        NPCM_PINCTRL_IDX(0x66, 7)
#define NPCM_PINCTRL_ALT67_0_FW_RDY_EN      NPCM_PINCTRL_IDX(0x67, 0)
#define NPCM_PINCTRL_ALT67_1_FW_RDY_SEL     NPCM_PINCTRL_IDX(0x67, 1)
#define NPCM_PINCTRL_ALT67_2_FW_RDY_TYPE    NPCM_PINCTRL_IDX(0x67, 2)
#define NPCM_PINCTRL_ALT69_5_USBDPHY_PONRST NPCM_PINCTRL_IDX(0x69, 5)
#define NPCM_PINCTRL_ALT6A_0_SIOX1_SL       NPCM_PINCTRL_IDX(0x6A, 0)
#define NPCM_PINCTRL_ALT6A_1_SIOX2_SL       NPCM_PINCTRL_IDX(0x6A, 1)
#define NPCM_PINCTRL_ALT6A_2_SIOX1_PU_EN    NPCM_PINCTRL_IDX(0x6A, 2)
#define NPCM_PINCTRL_ALT6A_3_SIOX2_PU_EN    NPCM_PINCTRL_IDX(0x6A, 3)
#define NPCM_PINCTRL_ALT6A_4_SIOX1_IDLE_SUP NPCM_PINCTRL_IDX(0x6A, 4)
#define NPCM_PINCTRL_ALT6A_5_SIOX2_IDLE_SUP NPCM_PINCTRL_IDX(0x6A, 5)
#define NPCM_PINCTRL_ALT6B_0_URTB_SIN_SL2   NPCM_PINCTRL_IDX(0x6B, 0)
#define NPCM_PINCTRL_ALT6B_1_URTB_SOUT_SL2  NPCM_PINCTRL_IDX(0x6B, 1)
#define NPCM_PINCTRL_ALT6B_2_URTB_RTS_SL2   NPCM_PINCTRL_IDX(0x6B, 2)
#define NPCM_PINCTRL_ALT6B_3_URTB_CTS_SL2   NPCM_PINCTRL_IDX(0x6B, 3)
#define NPCM_PINCTRL_ALT6B_4_URTB_DSR_SL2   NPCM_PINCTRL_IDX(0x6B, 4)
#define NPCM_PINCTRL_ALT6B_5_URTB_DTR_SL2   NPCM_PINCTRL_IDX(0x6B, 5)
#define NPCM_PINCTRL_ALT6B_6_URTB_DTR_SL2   NPCM_PINCTRL_IDX(0x6B, 6)
#define NPCM_PINCTRL_ALT6B_7_RTB_RI_SL2     NPCM_PINCTRL_IDX(0x6B, 7)
#define NPCM_PINCTRL_ALT6C_0_URTI4_SL       NPCM_PINCTRL_IDX(0x6C, 0)
#define NPCM_PINCTRL_ALT6C_1_URTO5_SL       NPCM_PINCTRL_IDX(0x6C, 1)
#define NPCM_PINCTRL_ALT6D_0_SMB7A_SL       NPCM_PINCTRL_IDX(0x6D, 0)
#define NPCM_PINCTRL_ALT6D_1_SMB8A_SL       NPCM_PINCTRL_IDX(0x6D, 1)
#define NPCM_PINCTRL_ALT6D_2_SMB9A_SL       NPCM_PINCTRL_IDX(0x6D, 2)
#define NPCM_PINCTRL_ALT6D_3_SMB10A_SL      NPCM_PINCTRL_IDX(0x6D, 3)
#define NPCM_PINCTRL_ALT6D_4_SMB11A_SL      NPCM_PINCTRL_IDX(0x6D, 4)
#define NPCM_PINCTRL_ALT6D_5_SMB12A_SL      NPCM_PINCTRL_IDX(0x6D, 5)
#define NPCM_PINCTRL_ALT6D_6_SMB4B_SL       NPCM_PINCTRL_IDX(0x6D, 6)
#define NPCM_PINCTRL_ALT6D_7_SMB6B_SL       NPCM_PINCTRL_IDX(0x6D, 7)

/*
 * Device control index
 */
/* 0x00: DEVCNT */
#define NPCM_PINCTRL_DEVCNT_6_SHD_SPI_TRIS NPCM_PINCTRL_IDX(0x00, 6)
#define NPCM_PINCTRL_DEVCNT_5_JEN_HEN      NPCM_PINCTRL_IDX(0x00, 5)
#define NPCM_PINCTRL_DEVCNT_0_CLKOM        NPCM_PINCTRL_IDX(0x00, 0)

/* 0x03: DEV_CTL2 */
#define NPCM_PINCTRL_DEV_CTL2_7_I3C5_110K     NPCM_PINCTRL_IDX(0x03, 7)
#define NPCM_PINCTRL_DEV_CTL2_6_T0OUT_PLS_INT NPCM_PINCTRL_IDX(0x03, 6)
#define NPCM_PINCTRL_DEV_CTL2_5_PLTRST_VW_SEL NPCM_PINCTRL_IDX(0x03, 5)
#define NPCM_PINCTRL_DEV_CTL2_4_I3C2_110K     NPCM_PINCTRL_IDX(0x03, 4)
#define NPCM_PINCTRL_DEV_CTL2_3_I3C1_110K     NPCM_PINCTRL_IDX(0x03, 3)
#define NPCM_PINCTRL_DEV_CTL2_1_GPRWP         NPCM_PINCTRL_IDX(0x03, 1)
#define NPCM_PINCTRL_DEV_CTL2_0_I3C6_110K     NPCM_PINCTRL_IDX(0x03, 0)

/* 0x04: DEV_CTL3 */
#define NPCM_PINCTRL_DEV_CTL3_7_I3C4_110K NPCM_PINCTRL_IDX(0x04, 7)
#define NPCM_PINCTRL_DEV_CTL3_6_I3C3_110K NPCM_PINCTRL_IDX(0x04, 6)
#define NPCM_PINCTRL_DEV_CTL3_5_KBR_CFG   NPCM_PINCTRL_IDX(0x04, 5)
#define NPCM_PINCTRL_DEV_CTL3_2_WP_GPIO76 NPCM_PINCTRL_IDX(0x04, 2)
#define NPCM_PINCTRL_DEV_CTL3_1_WP_GPIO55 NPCM_PINCTRL_IDX(0x04, 1)
#define NPCM_PINCTRL_DEV_CTL3_0_WP_INT_FL NPCM_PINCTRL_IDX(0x04, 0)

/* 0x06: DEV_CTL4 */
#define NPCM_PINCTRL_DEV_CTL4_5_ESPI_RSTO_EN   NPCM_PINCTRL_IDX(0x06, 5)
#define NPCM_PINCTRL_DEV_CTL4_4_PWROK_WD_EVENT NPCM_PINCTRL_IDX(0x06, 4)

/* 0x4D: EMC_CTL */
#define NPCM_PINCTRL_EMCCTL_3_EMC_PWRDN_EN NPCM_PINCTRL_IDX(0x4D, 3)
#define NPCM_PINCTRL_EMCCTL_2_EMC_FLOWCTRL NPCM_PINCTRL_IDX(0x4D, 2)
#define NPCM_PINCTRL_EMCCTL_1_MDIO_EN      NPCM_PINCTRL_IDX(0x4D, 1)
#define NPCM_PINCTRL_EMCCTL_0_RMII_EN      NPCM_PINCTRL_IDX(0x4D, 0)

/*
 * Device pull-up/pull-down enable index
 */
/* Pull-up/down enable */
#define NPCM_PINCTRL_DEVPU0_0_SMB1A_PUE  NPCM_PINCTRL_IDX(0x28, 0)
#define NPCM_PINCTRL_DEVPU0_1_SMB1B_PUE  NPCM_PINCTRL_IDX(0x28, 1)
#define NPCM_PINCTRL_DEVPU0_2_SMB2A_PUE  NPCM_PINCTRL_IDX(0x28, 2)
#define NPCM_PINCTRL_DEVPU0_4_SMB3A_PUE  NPCM_PINCTRL_IDX(0x28, 4)
#define NPCM_PINCTRL_DEVPU0_6_SMB4A_PUE  NPCM_PINCTRL_IDX(0x28, 6)
#define NPCM_PINCTRL_DEVPU0_7_SMB4B_PUE  NPCM_PINCTRL_IDX(0x28, 7)
#define NPCM_PINCTRL_DEVPU2_0_SMB7A_PUE  NPCM_PINCTRL_IDX(0x73, 0)
#define NPCM_PINCTRL_DEVPU2_1_SMB8A_PUE  NPCM_PINCTRL_IDX(0x73, 1)
#define NPCM_PINCTRL_DEVPU2_2_SMB9A_PUE  NPCM_PINCTRL_IDX(0x73, 2)
#define NPCM_PINCTRL_DEVPU2_3_SMB10A_PUE NPCM_PINCTRL_IDX(0x73, 3)
#define NPCM_PINCTRL_DEVPU2_4_SMB11A_PUE NPCM_PINCTRL_IDX(0x73, 4)
#define NPCM_PINCTRL_DEVPU2_5_SMB12A_PUE NPCM_PINCTRL_IDX(0x73, 5)

#define NPCM_PINCTRL_DEVPD1_0_SMB5A_PUE      NPCM_PINCTRL_IDX(0x29, 0)
#define NPCM_PINCTRL_DEVPD1_1_SMB6A_PUE      NPCM_PINCTRL_IDX(0x29, 1)
#define NPCM_PINCTRL_DEVPD1_2_I3C_PUE        NPCM_PINCTRL_IDX(0x29, 2)
#define NPCM_PINCTRL_DEVPD1_4_SMB6B_PUE      NPCM_PINCTRL_IDX(0x29, 4)
#define NPCM_PINCTRL_DEVPD1_5_SPIM_PDE       NPCM_PINCTRL_IDX(0x29, 5)
#define NPCM_PINCTRL_DEVPD1_6_PVT_SPI_PUD_EN NPCM_PINCTRL_IDX(0x29, 6)
#define NPCM_PINCTRL_DEVPD1_7_SHD_SPI_PUD_EN NPCM_PINCTRL_IDX(0x29, 7)
#define NPCM_PINCTRL_DEVPD3_0_I3C2_PUE       NPCM_PINCTRL_IDX(0x7B, 0)
#define NPCM_PINCTRL_DEVPD3_1_I3C3_PUE       NPCM_PINCTRL_IDX(0x7B, 1)
#define NPCM_PINCTRL_DEVPD3_2_I3C4_PUE       NPCM_PINCTRL_IDX(0x7B, 2)
#define NPCM_PINCTRL_DEVPD3_3_I3C5_PUE       NPCM_PINCTRL_IDX(0x7B, 3)
#define NPCM_PINCTRL_DEVPD3_4_I3C6_PUE       NPCM_PINCTRL_IDX(0x7B, 4)

/*
 * Low-voltage control index
 */
#define NPCM_PINCTRL_LV_CTL0_0_SDA1A_LV NPCM_PINCTRL_IDX(0x2A, 0)
#define NPCM_PINCTRL_LV_CTL0_1_SDA3A_LV NPCM_PINCTRL_IDX(0x2A, 1)
#define NPCM_PINCTRL_LV_CTL0_2_SDA4A_LV NPCM_PINCTRL_IDX(0x2A, 2)
#define NPCM_PINCTRL_LV_CTL0_3_SDA5A_LV NPCM_PINCTRL_IDX(0x2A, 3)
#define NPCM_PINCTRL_LV_CTL0_4_SDA1B_LV NPCM_PINCTRL_IDX(0x2A, 4)
#define NPCM_PINCTRL_LV_CTL0_7_SDA2A_LV NPCM_PINCTRL_IDX(0x2A, 7)

#define NPCM_PINCTRL_LV_CTL1_0_SCL1A_LV  NPCM_PINCTRL_IDX(0x2B, 0)
#define NPCM_PINCTRL_LV_CTL1_1_SCL3A_LV  NPCM_PINCTRL_IDX(0x2B, 1)
#define NPCM_PINCTRL_LV_CTL1_2_SCL4A_LV  NPCM_PINCTRL_IDX(0x2B, 2)
#define NPCM_PINCTRL_LV_CTL1_3_SCL5A_LV  NPCM_PINCTRL_IDX(0x2B, 3)
#define NPCM_PINCTRL_LV_CTL1_4_SCL1B_LV  NPCM_PINCTRL_IDX(0x2B, 4)
#define NPCM_PINCTRL_LV_CTL1_5_SLP_S3_LV NPCM_PINCTRL_IDX(0x2B, 5)
#define NPCM_PINCTRL_LV_CTL1_6_SLP_S5_LV NPCM_PINCTRL_IDX(0x2B, 6)
#define NPCM_PINCTRL_LV_CTL1_7_SCL2A_LV  NPCM_PINCTRL_IDX(0x2B, 7)

#define NPCM_PINCTRL_LV_CTL3_0_SDA7A_LV  NPCM_PINCTRL_IDX(0x2D, 0)
#define NPCM_PINCTRL_LV_CTL3_1_SDA8A_LV  NPCM_PINCTRL_IDX(0x2D, 1)
#define NPCM_PINCTRL_LV_CTL3_2_SDA9A_LV  NPCM_PINCTRL_IDX(0x2D, 2)
#define NPCM_PINCTRL_LV_CTL3_3_SDA10A_LV NPCM_PINCTRL_IDX(0x2D, 3)
#define NPCM_PINCTRL_LV_CTL3_4_SDA11A_LV NPCM_PINCTRL_IDX(0x2D, 4)
#define NPCM_PINCTRL_LV_CTL3_5_SDA12A_LV NPCM_PINCTRL_IDX(0x2D, 5)
#define NPCM_PINCTRL_LV_CTL3_6_SDA4B_LV  NPCM_PINCTRL_IDX(0x2D, 6)
#define NPCM_PINCTRL_LV_CTL3_7_SDA6B_LV  NPCM_PINCTRL_IDX(0x2D, 7)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NPCM_PINCTRL_H_ */

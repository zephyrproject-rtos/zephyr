/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_EVSYS_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_EVSYS_INSTANCE_FIXUP_H_

/* ========== Register definition for EVSYS peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_EVSYS_CTRLA            (0x4100E000) /**< \brief (EVSYS) Control */
#define REG_EVSYS_SWEVT            (0x4100E004) /**< \brief (EVSYS) Software Event */
#define REG_EVSYS_PRICTRL          (0x4100E008) /**< \brief (EVSYS) Priority Control */
#define REG_EVSYS_INTPEND          (0x4100E010) /**< \brief (EVSYS) Channel Pending Interrupt */
#define REG_EVSYS_INTSTATUS        (0x4100E014) /**< \brief (EVSYS) Interrupt Status */
#define REG_EVSYS_BUSYCH           (0x4100E018) /**< \brief (EVSYS) Busy Channels */
#define REG_EVSYS_READYUSR         (0x4100E01C) /**< \brief (EVSYS) Ready Users */
#define REG_EVSYS_CHANNEL0         (0x4100E020) /**< \brief (EVSYS) Channel 0 Control */
#define REG_EVSYS_CHINTENCLR0      (0x4100E024) /**< \brief (EVSYS) Channel 0 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET0      (0x4100E025) /**< \brief (EVSYS) Channel 0 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG0       (0x4100E026) /**< \brief (EVSYS) Channel 0 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS0        (0x4100E027) /**< \brief (EVSYS) Channel 0 Status */
#define REG_EVSYS_CHANNEL1         (0x4100E028) /**< \brief (EVSYS) Channel 1 Control */
#define REG_EVSYS_CHINTENCLR1      (0x4100E02C) /**< \brief (EVSYS) Channel 1 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET1      (0x4100E02D) /**< \brief (EVSYS) Channel 1 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG1       (0x4100E02E) /**< \brief (EVSYS) Channel 1 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS1        (0x4100E02F) /**< \brief (EVSYS) Channel 1 Status */
#define REG_EVSYS_CHANNEL2         (0x4100E030) /**< \brief (EVSYS) Channel 2 Control */
#define REG_EVSYS_CHINTENCLR2      (0x4100E034) /**< \brief (EVSYS) Channel 2 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET2      (0x4100E035) /**< \brief (EVSYS) Channel 2 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG2       (0x4100E036) /**< \brief (EVSYS) Channel 2 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS2        (0x4100E037) /**< \brief (EVSYS) Channel 2 Status */
#define REG_EVSYS_CHANNEL3         (0x4100E038) /**< \brief (EVSYS) Channel 3 Control */
#define REG_EVSYS_CHINTENCLR3      (0x4100E03C) /**< \brief (EVSYS) Channel 3 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET3      (0x4100E03D) /**< \brief (EVSYS) Channel 3 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG3       (0x4100E03E) /**< \brief (EVSYS) Channel 3 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS3        (0x4100E03F) /**< \brief (EVSYS) Channel 3 Status */
#define REG_EVSYS_CHANNEL4         (0x4100E040) /**< \brief (EVSYS) Channel 4 Control */
#define REG_EVSYS_CHINTENCLR4      (0x4100E044) /**< \brief (EVSYS) Channel 4 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET4      (0x4100E045) /**< \brief (EVSYS) Channel 4 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG4       (0x4100E046) /**< \brief (EVSYS) Channel 4 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS4        (0x4100E047) /**< \brief (EVSYS) Channel 4 Status */
#define REG_EVSYS_CHANNEL5         (0x4100E048) /**< \brief (EVSYS) Channel 5 Control */
#define REG_EVSYS_CHINTENCLR5      (0x4100E04C) /**< \brief (EVSYS) Channel 5 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET5      (0x4100E04D) /**< \brief (EVSYS) Channel 5 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG5       (0x4100E04E) /**< \brief (EVSYS) Channel 5 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS5        (0x4100E04F) /**< \brief (EVSYS) Channel 5 Status */
#define REG_EVSYS_CHANNEL6         (0x4100E050) /**< \brief (EVSYS) Channel 6 Control */
#define REG_EVSYS_CHINTENCLR6      (0x4100E054) /**< \brief (EVSYS) Channel 6 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET6      (0x4100E055) /**< \brief (EVSYS) Channel 6 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG6       (0x4100E056) /**< \brief (EVSYS) Channel 6 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS6        (0x4100E057) /**< \brief (EVSYS) Channel 6 Status */
#define REG_EVSYS_CHANNEL7         (0x4100E058) /**< \brief (EVSYS) Channel 7 Control */
#define REG_EVSYS_CHINTENCLR7      (0x4100E05C) /**< \brief (EVSYS) Channel 7 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET7      (0x4100E05D) /**< \brief (EVSYS) Channel 7 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG7       (0x4100E05E) /**< \brief (EVSYS) Channel 7 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS7        (0x4100E05F) /**< \brief (EVSYS) Channel 7 Status */
#define REG_EVSYS_CHANNEL8         (0x4100E060) /**< \brief (EVSYS) Channel 8 Control */
#define REG_EVSYS_CHINTENCLR8      (0x4100E064) /**< \brief (EVSYS) Channel 8 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET8      (0x4100E065) /**< \brief (EVSYS) Channel 8 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG8       (0x4100E066) /**< \brief (EVSYS) Channel 8 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS8        (0x4100E067) /**< \brief (EVSYS) Channel 8 Status */
#define REG_EVSYS_CHANNEL9         (0x4100E068) /**< \brief (EVSYS) Channel 9 Control */
#define REG_EVSYS_CHINTENCLR9      (0x4100E06C) /**< \brief (EVSYS) Channel 9 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET9      (0x4100E06D) /**< \brief (EVSYS) Channel 9 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG9       (0x4100E06E) /**< \brief (EVSYS) Channel 9 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS9        (0x4100E06F) /**< \brief (EVSYS) Channel 9 Status */
#define REG_EVSYS_CHANNEL10        (0x4100E070) /**< \brief (EVSYS) Channel 10 Control */
#define REG_EVSYS_CHINTENCLR10     (0x4100E074) /**< \brief (EVSYS) Channel 10 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET10     (0x4100E075) /**< \brief (EVSYS) Channel 10 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG10      (0x4100E076) /**< \brief (EVSYS) Channel 10 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS10       (0x4100E077) /**< \brief (EVSYS) Channel 10 Status */
#define REG_EVSYS_CHANNEL11        (0x4100E078) /**< \brief (EVSYS) Channel 11 Control */
#define REG_EVSYS_CHINTENCLR11     (0x4100E07C) /**< \brief (EVSYS) Channel 11 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET11     (0x4100E07D) /**< \brief (EVSYS) Channel 11 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG11      (0x4100E07E) /**< \brief (EVSYS) Channel 11 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS11       (0x4100E07F) /**< \brief (EVSYS) Channel 11 Status */
#define REG_EVSYS_CHANNEL12        (0x4100E080) /**< \brief (EVSYS) Channel 12 Control */
#define REG_EVSYS_CHINTENCLR12     (0x4100E084) /**< \brief (EVSYS) Channel 12 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET12     (0x4100E085) /**< \brief (EVSYS) Channel 12 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG12      (0x4100E086) /**< \brief (EVSYS) Channel 12 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS12       (0x4100E087) /**< \brief (EVSYS) Channel 12 Status */
#define REG_EVSYS_CHANNEL13        (0x4100E088) /**< \brief (EVSYS) Channel 13 Control */
#define REG_EVSYS_CHINTENCLR13     (0x4100E08C) /**< \brief (EVSYS) Channel 13 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET13     (0x4100E08D) /**< \brief (EVSYS) Channel 13 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG13      (0x4100E08E) /**< \brief (EVSYS) Channel 13 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS13       (0x4100E08F) /**< \brief (EVSYS) Channel 13 Status */
#define REG_EVSYS_CHANNEL14        (0x4100E090) /**< \brief (EVSYS) Channel 14 Control */
#define REG_EVSYS_CHINTENCLR14     (0x4100E094) /**< \brief (EVSYS) Channel 14 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET14     (0x4100E095) /**< \brief (EVSYS) Channel 14 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG14      (0x4100E096) /**< \brief (EVSYS) Channel 14 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS14       (0x4100E097) /**< \brief (EVSYS) Channel 14 Status */
#define REG_EVSYS_CHANNEL15        (0x4100E098) /**< \brief (EVSYS) Channel 15 Control */
#define REG_EVSYS_CHINTENCLR15     (0x4100E09C) /**< \brief (EVSYS) Channel 15 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET15     (0x4100E09D) /**< \brief (EVSYS) Channel 15 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG15      (0x4100E09E) /**< \brief (EVSYS) Channel 15 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS15       (0x4100E09F) /**< \brief (EVSYS) Channel 15 Status */
#define REG_EVSYS_CHANNEL16        (0x4100E0A0) /**< \brief (EVSYS) Channel 16 Control */
#define REG_EVSYS_CHINTENCLR16     (0x4100E0A4) /**< \brief (EVSYS) Channel 16 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET16     (0x4100E0A5) /**< \brief (EVSYS) Channel 16 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG16      (0x4100E0A6) /**< \brief (EVSYS) Channel 16 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS16       (0x4100E0A7) /**< \brief (EVSYS) Channel 16 Status */
#define REG_EVSYS_CHANNEL17        (0x4100E0A8) /**< \brief (EVSYS) Channel 17 Control */
#define REG_EVSYS_CHINTENCLR17     (0x4100E0AC) /**< \brief (EVSYS) Channel 17 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET17     (0x4100E0AD) /**< \brief (EVSYS) Channel 17 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG17      (0x4100E0AE) /**< \brief (EVSYS) Channel 17 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS17       (0x4100E0AF) /**< \brief (EVSYS) Channel 17 Status */
#define REG_EVSYS_CHANNEL18        (0x4100E0B0) /**< \brief (EVSYS) Channel 18 Control */
#define REG_EVSYS_CHINTENCLR18     (0x4100E0B4) /**< \brief (EVSYS) Channel 18 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET18     (0x4100E0B5) /**< \brief (EVSYS) Channel 18 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG18      (0x4100E0B6) /**< \brief (EVSYS) Channel 18 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS18       (0x4100E0B7) /**< \brief (EVSYS) Channel 18 Status */
#define REG_EVSYS_CHANNEL19        (0x4100E0B8) /**< \brief (EVSYS) Channel 19 Control */
#define REG_EVSYS_CHINTENCLR19     (0x4100E0BC) /**< \brief (EVSYS) Channel 19 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET19     (0x4100E0BD) /**< \brief (EVSYS) Channel 19 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG19      (0x4100E0BE) /**< \brief (EVSYS) Channel 19 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS19       (0x4100E0BF) /**< \brief (EVSYS) Channel 19 Status */
#define REG_EVSYS_CHANNEL20        (0x4100E0C0) /**< \brief (EVSYS) Channel 20 Control */
#define REG_EVSYS_CHINTENCLR20     (0x4100E0C4) /**< \brief (EVSYS) Channel 20 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET20     (0x4100E0C5) /**< \brief (EVSYS) Channel 20 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG20      (0x4100E0C6) /**< \brief (EVSYS) Channel 20 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS20       (0x4100E0C7) /**< \brief (EVSYS) Channel 20 Status */
#define REG_EVSYS_CHANNEL21        (0x4100E0C8) /**< \brief (EVSYS) Channel 21 Control */
#define REG_EVSYS_CHINTENCLR21     (0x4100E0CC) /**< \brief (EVSYS) Channel 21 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET21     (0x4100E0CD) /**< \brief (EVSYS) Channel 21 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG21      (0x4100E0CE) /**< \brief (EVSYS) Channel 21 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS21       (0x4100E0CF) /**< \brief (EVSYS) Channel 21 Status */
#define REG_EVSYS_CHANNEL22        (0x4100E0D0) /**< \brief (EVSYS) Channel 22 Control */
#define REG_EVSYS_CHINTENCLR22     (0x4100E0D4) /**< \brief (EVSYS) Channel 22 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET22     (0x4100E0D5) /**< \brief (EVSYS) Channel 22 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG22      (0x4100E0D6) /**< \brief (EVSYS) Channel 22 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS22       (0x4100E0D7) /**< \brief (EVSYS) Channel 22 Status */
#define REG_EVSYS_CHANNEL23        (0x4100E0D8) /**< \brief (EVSYS) Channel 23 Control */
#define REG_EVSYS_CHINTENCLR23     (0x4100E0DC) /**< \brief (EVSYS) Channel 23 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET23     (0x4100E0DD) /**< \brief (EVSYS) Channel 23 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG23      (0x4100E0DE) /**< \brief (EVSYS) Channel 23 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS23       (0x4100E0DF) /**< \brief (EVSYS) Channel 23 Status */
#define REG_EVSYS_CHANNEL24        (0x4100E0E0) /**< \brief (EVSYS) Channel 24 Control */
#define REG_EVSYS_CHINTENCLR24     (0x4100E0E4) /**< \brief (EVSYS) Channel 24 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET24     (0x4100E0E5) /**< \brief (EVSYS) Channel 24 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG24      (0x4100E0E6) /**< \brief (EVSYS) Channel 24 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS24       (0x4100E0E7) /**< \brief (EVSYS) Channel 24 Status */
#define REG_EVSYS_CHANNEL25        (0x4100E0E8) /**< \brief (EVSYS) Channel 25 Control */
#define REG_EVSYS_CHINTENCLR25     (0x4100E0EC) /**< \brief (EVSYS) Channel 25 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET25     (0x4100E0ED) /**< \brief (EVSYS) Channel 25 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG25      (0x4100E0EE) /**< \brief (EVSYS) Channel 25 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS25       (0x4100E0EF) /**< \brief (EVSYS) Channel 25 Status */
#define REG_EVSYS_CHANNEL26        (0x4100E0F0) /**< \brief (EVSYS) Channel 26 Control */
#define REG_EVSYS_CHINTENCLR26     (0x4100E0F4) /**< \brief (EVSYS) Channel 26 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET26     (0x4100E0F5) /**< \brief (EVSYS) Channel 26 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG26      (0x4100E0F6) /**< \brief (EVSYS) Channel 26 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS26       (0x4100E0F7) /**< \brief (EVSYS) Channel 26 Status */
#define REG_EVSYS_CHANNEL27        (0x4100E0F8) /**< \brief (EVSYS) Channel 27 Control */
#define REG_EVSYS_CHINTENCLR27     (0x4100E0FC) /**< \brief (EVSYS) Channel 27 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET27     (0x4100E0FD) /**< \brief (EVSYS) Channel 27 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG27      (0x4100E0FE) /**< \brief (EVSYS) Channel 27 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS27       (0x4100E0FF) /**< \brief (EVSYS) Channel 27 Status */
#define REG_EVSYS_CHANNEL28        (0x4100E100) /**< \brief (EVSYS) Channel 28 Control */
#define REG_EVSYS_CHINTENCLR28     (0x4100E104) /**< \brief (EVSYS) Channel 28 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET28     (0x4100E105) /**< \brief (EVSYS) Channel 28 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG28      (0x4100E106) /**< \brief (EVSYS) Channel 28 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS28       (0x4100E107) /**< \brief (EVSYS) Channel 28 Status */
#define REG_EVSYS_CHANNEL29        (0x4100E108) /**< \brief (EVSYS) Channel 29 Control */
#define REG_EVSYS_CHINTENCLR29     (0x4100E10C) /**< \brief (EVSYS) Channel 29 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET29     (0x4100E10D) /**< \brief (EVSYS) Channel 29 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG29      (0x4100E10E) /**< \brief (EVSYS) Channel 29 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS29       (0x4100E10F) /**< \brief (EVSYS) Channel 29 Status */
#define REG_EVSYS_CHANNEL30        (0x4100E110) /**< \brief (EVSYS) Channel 30 Control */
#define REG_EVSYS_CHINTENCLR30     (0x4100E114) /**< \brief (EVSYS) Channel 30 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET30     (0x4100E115) /**< \brief (EVSYS) Channel 30 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG30      (0x4100E116) /**< \brief (EVSYS) Channel 30 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS30       (0x4100E117) /**< \brief (EVSYS) Channel 30 Status */
#define REG_EVSYS_CHANNEL31        (0x4100E118) /**< \brief (EVSYS) Channel 31 Control */
#define REG_EVSYS_CHINTENCLR31     (0x4100E11C) /**< \brief (EVSYS) Channel 31 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET31     (0x4100E11D) /**< \brief (EVSYS) Channel 31 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG31      (0x4100E11E) /**< \brief (EVSYS) Channel 31 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS31       (0x4100E11F) /**< \brief (EVSYS) Channel 31 Status */
#define REG_EVSYS_USER0            (0x4100E120) /**< \brief (EVSYS) User Multiplexer 0 */
#define REG_EVSYS_USER1            (0x4100E124) /**< \brief (EVSYS) User Multiplexer 1 */
#define REG_EVSYS_USER2            (0x4100E128) /**< \brief (EVSYS) User Multiplexer 2 */
#define REG_EVSYS_USER3            (0x4100E12C) /**< \brief (EVSYS) User Multiplexer 3 */
#define REG_EVSYS_USER4            (0x4100E130) /**< \brief (EVSYS) User Multiplexer 4 */
#define REG_EVSYS_USER5            (0x4100E134) /**< \brief (EVSYS) User Multiplexer 5 */
#define REG_EVSYS_USER6            (0x4100E138) /**< \brief (EVSYS) User Multiplexer 6 */
#define REG_EVSYS_USER7            (0x4100E13C) /**< \brief (EVSYS) User Multiplexer 7 */
#define REG_EVSYS_USER8            (0x4100E140) /**< \brief (EVSYS) User Multiplexer 8 */
#define REG_EVSYS_USER9            (0x4100E144) /**< \brief (EVSYS) User Multiplexer 9 */
#define REG_EVSYS_USER10           (0x4100E148) /**< \brief (EVSYS) User Multiplexer 10 */
#define REG_EVSYS_USER11           (0x4100E14C) /**< \brief (EVSYS) User Multiplexer 11 */
#define REG_EVSYS_USER12           (0x4100E150) /**< \brief (EVSYS) User Multiplexer 12 */
#define REG_EVSYS_USER13           (0x4100E154) /**< \brief (EVSYS) User Multiplexer 13 */
#define REG_EVSYS_USER14           (0x4100E158) /**< \brief (EVSYS) User Multiplexer 14 */
#define REG_EVSYS_USER15           (0x4100E15C) /**< \brief (EVSYS) User Multiplexer 15 */
#define REG_EVSYS_USER16           (0x4100E160) /**< \brief (EVSYS) User Multiplexer 16 */
#define REG_EVSYS_USER17           (0x4100E164) /**< \brief (EVSYS) User Multiplexer 17 */
#define REG_EVSYS_USER18           (0x4100E168) /**< \brief (EVSYS) User Multiplexer 18 */
#define REG_EVSYS_USER19           (0x4100E16C) /**< \brief (EVSYS) User Multiplexer 19 */
#define REG_EVSYS_USER20           (0x4100E170) /**< \brief (EVSYS) User Multiplexer 20 */
#define REG_EVSYS_USER21           (0x4100E174) /**< \brief (EVSYS) User Multiplexer 21 */
#define REG_EVSYS_USER22           (0x4100E178) /**< \brief (EVSYS) User Multiplexer 22 */
#define REG_EVSYS_USER23           (0x4100E17C) /**< \brief (EVSYS) User Multiplexer 23 */
#define REG_EVSYS_USER24           (0x4100E180) /**< \brief (EVSYS) User Multiplexer 24 */
#define REG_EVSYS_USER25           (0x4100E184) /**< \brief (EVSYS) User Multiplexer 25 */
#define REG_EVSYS_USER26           (0x4100E188) /**< \brief (EVSYS) User Multiplexer 26 */
#define REG_EVSYS_USER27           (0x4100E18C) /**< \brief (EVSYS) User Multiplexer 27 */
#define REG_EVSYS_USER28           (0x4100E190) /**< \brief (EVSYS) User Multiplexer 28 */
#define REG_EVSYS_USER29           (0x4100E194) /**< \brief (EVSYS) User Multiplexer 29 */
#define REG_EVSYS_USER30           (0x4100E198) /**< \brief (EVSYS) User Multiplexer 30 */
#define REG_EVSYS_USER31           (0x4100E19C) /**< \brief (EVSYS) User Multiplexer 31 */
#define REG_EVSYS_USER32           (0x4100E1A0) /**< \brief (EVSYS) User Multiplexer 32 */
#define REG_EVSYS_USER33           (0x4100E1A4) /**< \brief (EVSYS) User Multiplexer 33 */
#define REG_EVSYS_USER34           (0x4100E1A8) /**< \brief (EVSYS) User Multiplexer 34 */
#define REG_EVSYS_USER35           (0x4100E1AC) /**< \brief (EVSYS) User Multiplexer 35 */
#define REG_EVSYS_USER36           (0x4100E1B0) /**< \brief (EVSYS) User Multiplexer 36 */
#define REG_EVSYS_USER37           (0x4100E1B4) /**< \brief (EVSYS) User Multiplexer 37 */
#define REG_EVSYS_USER38           (0x4100E1B8) /**< \brief (EVSYS) User Multiplexer 38 */
#define REG_EVSYS_USER39           (0x4100E1BC) /**< \brief (EVSYS) User Multiplexer 39 */
#define REG_EVSYS_USER40           (0x4100E1C0) /**< \brief (EVSYS) User Multiplexer 40 */
#define REG_EVSYS_USER41           (0x4100E1C4) /**< \brief (EVSYS) User Multiplexer 41 */
#define REG_EVSYS_USER42           (0x4100E1C8) /**< \brief (EVSYS) User Multiplexer 42 */
#define REG_EVSYS_USER43           (0x4100E1CC) /**< \brief (EVSYS) User Multiplexer 43 */
#define REG_EVSYS_USER44           (0x4100E1D0) /**< \brief (EVSYS) User Multiplexer 44 */
#define REG_EVSYS_USER45           (0x4100E1D4) /**< \brief (EVSYS) User Multiplexer 45 */
#define REG_EVSYS_USER46           (0x4100E1D8) /**< \brief (EVSYS) User Multiplexer 46 */
#define REG_EVSYS_USER47           (0x4100E1DC) /**< \brief (EVSYS) User Multiplexer 47 */
#define REG_EVSYS_USER48           (0x4100E1E0) /**< \brief (EVSYS) User Multiplexer 48 */
#define REG_EVSYS_USER49           (0x4100E1E4) /**< \brief (EVSYS) User Multiplexer 49 */
#define REG_EVSYS_USER50           (0x4100E1E8) /**< \brief (EVSYS) User Multiplexer 50 */
#define REG_EVSYS_USER51           (0x4100E1EC) /**< \brief (EVSYS) User Multiplexer 51 */
#define REG_EVSYS_USER52           (0x4100E1F0) /**< \brief (EVSYS) User Multiplexer 52 */
#define REG_EVSYS_USER53           (0x4100E1F4) /**< \brief (EVSYS) User Multiplexer 53 */
#define REG_EVSYS_USER54           (0x4100E1F8) /**< \brief (EVSYS) User Multiplexer 54 */
#define REG_EVSYS_USER55           (0x4100E1FC) /**< \brief (EVSYS) User Multiplexer 55 */
#define REG_EVSYS_USER56           (0x4100E200) /**< \brief (EVSYS) User Multiplexer 56 */
#define REG_EVSYS_USER57           (0x4100E204) /**< \brief (EVSYS) User Multiplexer 57 */
#define REG_EVSYS_USER58           (0x4100E208) /**< \brief (EVSYS) User Multiplexer 58 */
#define REG_EVSYS_USER59           (0x4100E20C) /**< \brief (EVSYS) User Multiplexer 59 */
#define REG_EVSYS_USER60           (0x4100E210) /**< \brief (EVSYS) User Multiplexer 60 */
#define REG_EVSYS_USER61           (0x4100E214) /**< \brief (EVSYS) User Multiplexer 61 */
#define REG_EVSYS_USER62           (0x4100E218) /**< \brief (EVSYS) User Multiplexer 62 */
#define REG_EVSYS_USER63           (0x4100E21C) /**< \brief (EVSYS) User Multiplexer 63 */
#define REG_EVSYS_USER64           (0x4100E220) /**< \brief (EVSYS) User Multiplexer 64 */
#define REG_EVSYS_USER65           (0x4100E224) /**< \brief (EVSYS) User Multiplexer 65 */
#define REG_EVSYS_USER66           (0x4100E228) /**< \brief (EVSYS) User Multiplexer 66 */
#else
#define REG_EVSYS_CTRLA            (*(RwReg8 *)0x4100E000UL) /**< \brief (EVSYS) Control */
#define REG_EVSYS_SWEVT            (*(WoReg  *)0x4100E004UL) /**< \brief (EVSYS) Software Event */
#define REG_EVSYS_PRICTRL          (*(RwReg8 *)0x4100E008UL) /**< \brief (EVSYS) Priority Control */
#define REG_EVSYS_INTPEND          (*(RwReg16*)0x4100E010UL) /**< \brief (EVSYS) Channel Pending Interrupt */
#define REG_EVSYS_INTSTATUS        (*(RoReg  *)0x4100E014UL) /**< \brief (EVSYS) Interrupt Status */
#define REG_EVSYS_BUSYCH           (*(RoReg  *)0x4100E018UL) /**< \brief (EVSYS) Busy Channels */
#define REG_EVSYS_READYUSR         (*(RoReg  *)0x4100E01CUL) /**< \brief (EVSYS) Ready Users */
#define REG_EVSYS_CHANNEL0         (*(RwReg  *)0x4100E020UL) /**< \brief (EVSYS) Channel 0 Control */
#define REG_EVSYS_CHINTENCLR0      (*(RwReg8 *)0x4100E024UL) /**< \brief (EVSYS) Channel 0 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET0      (*(RwReg8 *)0x4100E025UL) /**< \brief (EVSYS) Channel 0 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG0       (*(RwReg8 *)0x4100E026UL) /**< \brief (EVSYS) Channel 0 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS0        (*(RoReg8 *)0x4100E027UL) /**< \brief (EVSYS) Channel 0 Status */
#define REG_EVSYS_CHANNEL1         (*(RwReg  *)0x4100E028UL) /**< \brief (EVSYS) Channel 1 Control */
#define REG_EVSYS_CHINTENCLR1      (*(RwReg8 *)0x4100E02CUL) /**< \brief (EVSYS) Channel 1 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET1      (*(RwReg8 *)0x4100E02DUL) /**< \brief (EVSYS) Channel 1 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG1       (*(RwReg8 *)0x4100E02EUL) /**< \brief (EVSYS) Channel 1 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS1        (*(RoReg8 *)0x4100E02FUL) /**< \brief (EVSYS) Channel 1 Status */
#define REG_EVSYS_CHANNEL2         (*(RwReg  *)0x4100E030UL) /**< \brief (EVSYS) Channel 2 Control */
#define REG_EVSYS_CHINTENCLR2      (*(RwReg8 *)0x4100E034UL) /**< \brief (EVSYS) Channel 2 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET2      (*(RwReg8 *)0x4100E035UL) /**< \brief (EVSYS) Channel 2 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG2       (*(RwReg8 *)0x4100E036UL) /**< \brief (EVSYS) Channel 2 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS2        (*(RoReg8 *)0x4100E037UL) /**< \brief (EVSYS) Channel 2 Status */
#define REG_EVSYS_CHANNEL3         (*(RwReg  *)0x4100E038UL) /**< \brief (EVSYS) Channel 3 Control */
#define REG_EVSYS_CHINTENCLR3      (*(RwReg8 *)0x4100E03CUL) /**< \brief (EVSYS) Channel 3 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET3      (*(RwReg8 *)0x4100E03DUL) /**< \brief (EVSYS) Channel 3 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG3       (*(RwReg8 *)0x4100E03EUL) /**< \brief (EVSYS) Channel 3 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS3        (*(RoReg8 *)0x4100E03FUL) /**< \brief (EVSYS) Channel 3 Status */
#define REG_EVSYS_CHANNEL4         (*(RwReg  *)0x4100E040UL) /**< \brief (EVSYS) Channel 4 Control */
#define REG_EVSYS_CHINTENCLR4      (*(RwReg8 *)0x4100E044UL) /**< \brief (EVSYS) Channel 4 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET4      (*(RwReg8 *)0x4100E045UL) /**< \brief (EVSYS) Channel 4 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG4       (*(RwReg8 *)0x4100E046UL) /**< \brief (EVSYS) Channel 4 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS4        (*(RoReg8 *)0x4100E047UL) /**< \brief (EVSYS) Channel 4 Status */
#define REG_EVSYS_CHANNEL5         (*(RwReg  *)0x4100E048UL) /**< \brief (EVSYS) Channel 5 Control */
#define REG_EVSYS_CHINTENCLR5      (*(RwReg8 *)0x4100E04CUL) /**< \brief (EVSYS) Channel 5 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET5      (*(RwReg8 *)0x4100E04DUL) /**< \brief (EVSYS) Channel 5 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG5       (*(RwReg8 *)0x4100E04EUL) /**< \brief (EVSYS) Channel 5 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS5        (*(RoReg8 *)0x4100E04FUL) /**< \brief (EVSYS) Channel 5 Status */
#define REG_EVSYS_CHANNEL6         (*(RwReg  *)0x4100E050UL) /**< \brief (EVSYS) Channel 6 Control */
#define REG_EVSYS_CHINTENCLR6      (*(RwReg8 *)0x4100E054UL) /**< \brief (EVSYS) Channel 6 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET6      (*(RwReg8 *)0x4100E055UL) /**< \brief (EVSYS) Channel 6 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG6       (*(RwReg8 *)0x4100E056UL) /**< \brief (EVSYS) Channel 6 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS6        (*(RoReg8 *)0x4100E057UL) /**< \brief (EVSYS) Channel 6 Status */
#define REG_EVSYS_CHANNEL7         (*(RwReg  *)0x4100E058UL) /**< \brief (EVSYS) Channel 7 Control */
#define REG_EVSYS_CHINTENCLR7      (*(RwReg8 *)0x4100E05CUL) /**< \brief (EVSYS) Channel 7 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET7      (*(RwReg8 *)0x4100E05DUL) /**< \brief (EVSYS) Channel 7 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG7       (*(RwReg8 *)0x4100E05EUL) /**< \brief (EVSYS) Channel 7 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS7        (*(RoReg8 *)0x4100E05FUL) /**< \brief (EVSYS) Channel 7 Status */
#define REG_EVSYS_CHANNEL8         (*(RwReg  *)0x4100E060UL) /**< \brief (EVSYS) Channel 8 Control */
#define REG_EVSYS_CHINTENCLR8      (*(RwReg8 *)0x4100E064UL) /**< \brief (EVSYS) Channel 8 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET8      (*(RwReg8 *)0x4100E065UL) /**< \brief (EVSYS) Channel 8 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG8       (*(RwReg8 *)0x4100E066UL) /**< \brief (EVSYS) Channel 8 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS8        (*(RoReg8 *)0x4100E067UL) /**< \brief (EVSYS) Channel 8 Status */
#define REG_EVSYS_CHANNEL9         (*(RwReg  *)0x4100E068UL) /**< \brief (EVSYS) Channel 9 Control */
#define REG_EVSYS_CHINTENCLR9      (*(RwReg8 *)0x4100E06CUL) /**< \brief (EVSYS) Channel 9 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET9      (*(RwReg8 *)0x4100E06DUL) /**< \brief (EVSYS) Channel 9 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG9       (*(RwReg8 *)0x4100E06EUL) /**< \brief (EVSYS) Channel 9 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS9        (*(RoReg8 *)0x4100E06FUL) /**< \brief (EVSYS) Channel 9 Status */
#define REG_EVSYS_CHANNEL10        (*(RwReg  *)0x4100E070UL) /**< \brief (EVSYS) Channel 10 Control */
#define REG_EVSYS_CHINTENCLR10     (*(RwReg8 *)0x4100E074UL) /**< \brief (EVSYS) Channel 10 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET10     (*(RwReg8 *)0x4100E075UL) /**< \brief (EVSYS) Channel 10 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG10      (*(RwReg8 *)0x4100E076UL) /**< \brief (EVSYS) Channel 10 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS10       (*(RoReg8 *)0x4100E077UL) /**< \brief (EVSYS) Channel 10 Status */
#define REG_EVSYS_CHANNEL11        (*(RwReg  *)0x4100E078UL) /**< \brief (EVSYS) Channel 11 Control */
#define REG_EVSYS_CHINTENCLR11     (*(RwReg8 *)0x4100E07CUL) /**< \brief (EVSYS) Channel 11 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET11     (*(RwReg8 *)0x4100E07DUL) /**< \brief (EVSYS) Channel 11 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG11      (*(RwReg8 *)0x4100E07EUL) /**< \brief (EVSYS) Channel 11 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS11       (*(RoReg8 *)0x4100E07FUL) /**< \brief (EVSYS) Channel 11 Status */
#define REG_EVSYS_CHANNEL12        (*(RwReg  *)0x4100E080UL) /**< \brief (EVSYS) Channel 12 Control */
#define REG_EVSYS_CHINTENCLR12     (*(RwReg8 *)0x4100E084UL) /**< \brief (EVSYS) Channel 12 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET12     (*(RwReg8 *)0x4100E085UL) /**< \brief (EVSYS) Channel 12 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG12      (*(RwReg8 *)0x4100E086UL) /**< \brief (EVSYS) Channel 12 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS12       (*(RoReg8 *)0x4100E087UL) /**< \brief (EVSYS) Channel 12 Status */
#define REG_EVSYS_CHANNEL13        (*(RwReg  *)0x4100E088UL) /**< \brief (EVSYS) Channel 13 Control */
#define REG_EVSYS_CHINTENCLR13     (*(RwReg8 *)0x4100E08CUL) /**< \brief (EVSYS) Channel 13 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET13     (*(RwReg8 *)0x4100E08DUL) /**< \brief (EVSYS) Channel 13 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG13      (*(RwReg8 *)0x4100E08EUL) /**< \brief (EVSYS) Channel 13 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS13       (*(RoReg8 *)0x4100E08FUL) /**< \brief (EVSYS) Channel 13 Status */
#define REG_EVSYS_CHANNEL14        (*(RwReg  *)0x4100E090UL) /**< \brief (EVSYS) Channel 14 Control */
#define REG_EVSYS_CHINTENCLR14     (*(RwReg8 *)0x4100E094UL) /**< \brief (EVSYS) Channel 14 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET14     (*(RwReg8 *)0x4100E095UL) /**< \brief (EVSYS) Channel 14 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG14      (*(RwReg8 *)0x4100E096UL) /**< \brief (EVSYS) Channel 14 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS14       (*(RoReg8 *)0x4100E097UL) /**< \brief (EVSYS) Channel 14 Status */
#define REG_EVSYS_CHANNEL15        (*(RwReg  *)0x4100E098UL) /**< \brief (EVSYS) Channel 15 Control */
#define REG_EVSYS_CHINTENCLR15     (*(RwReg8 *)0x4100E09CUL) /**< \brief (EVSYS) Channel 15 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET15     (*(RwReg8 *)0x4100E09DUL) /**< \brief (EVSYS) Channel 15 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG15      (*(RwReg8 *)0x4100E09EUL) /**< \brief (EVSYS) Channel 15 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS15       (*(RoReg8 *)0x4100E09FUL) /**< \brief (EVSYS) Channel 15 Status */
#define REG_EVSYS_CHANNEL16        (*(RwReg  *)0x4100E0A0UL) /**< \brief (EVSYS) Channel 16 Control */
#define REG_EVSYS_CHINTENCLR16     (*(RwReg8 *)0x4100E0A4UL) /**< \brief (EVSYS) Channel 16 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET16     (*(RwReg8 *)0x4100E0A5UL) /**< \brief (EVSYS) Channel 16 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG16      (*(RwReg8 *)0x4100E0A6UL) /**< \brief (EVSYS) Channel 16 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS16       (*(RoReg8 *)0x4100E0A7UL) /**< \brief (EVSYS) Channel 16 Status */
#define REG_EVSYS_CHANNEL17        (*(RwReg  *)0x4100E0A8UL) /**< \brief (EVSYS) Channel 17 Control */
#define REG_EVSYS_CHINTENCLR17     (*(RwReg8 *)0x4100E0ACUL) /**< \brief (EVSYS) Channel 17 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET17     (*(RwReg8 *)0x4100E0ADUL) /**< \brief (EVSYS) Channel 17 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG17      (*(RwReg8 *)0x4100E0AEUL) /**< \brief (EVSYS) Channel 17 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS17       (*(RoReg8 *)0x4100E0AFUL) /**< \brief (EVSYS) Channel 17 Status */
#define REG_EVSYS_CHANNEL18        (*(RwReg  *)0x4100E0B0UL) /**< \brief (EVSYS) Channel 18 Control */
#define REG_EVSYS_CHINTENCLR18     (*(RwReg8 *)0x4100E0B4UL) /**< \brief (EVSYS) Channel 18 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET18     (*(RwReg8 *)0x4100E0B5UL) /**< \brief (EVSYS) Channel 18 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG18      (*(RwReg8 *)0x4100E0B6UL) /**< \brief (EVSYS) Channel 18 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS18       (*(RoReg8 *)0x4100E0B7UL) /**< \brief (EVSYS) Channel 18 Status */
#define REG_EVSYS_CHANNEL19        (*(RwReg  *)0x4100E0B8UL) /**< \brief (EVSYS) Channel 19 Control */
#define REG_EVSYS_CHINTENCLR19     (*(RwReg8 *)0x4100E0BCUL) /**< \brief (EVSYS) Channel 19 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET19     (*(RwReg8 *)0x4100E0BDUL) /**< \brief (EVSYS) Channel 19 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG19      (*(RwReg8 *)0x4100E0BEUL) /**< \brief (EVSYS) Channel 19 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS19       (*(RoReg8 *)0x4100E0BFUL) /**< \brief (EVSYS) Channel 19 Status */
#define REG_EVSYS_CHANNEL20        (*(RwReg  *)0x4100E0C0UL) /**< \brief (EVSYS) Channel 20 Control */
#define REG_EVSYS_CHINTENCLR20     (*(RwReg8 *)0x4100E0C4UL) /**< \brief (EVSYS) Channel 20 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET20     (*(RwReg8 *)0x4100E0C5UL) /**< \brief (EVSYS) Channel 20 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG20      (*(RwReg8 *)0x4100E0C6UL) /**< \brief (EVSYS) Channel 20 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS20       (*(RoReg8 *)0x4100E0C7UL) /**< \brief (EVSYS) Channel 20 Status */
#define REG_EVSYS_CHANNEL21        (*(RwReg  *)0x4100E0C8UL) /**< \brief (EVSYS) Channel 21 Control */
#define REG_EVSYS_CHINTENCLR21     (*(RwReg8 *)0x4100E0CCUL) /**< \brief (EVSYS) Channel 21 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET21     (*(RwReg8 *)0x4100E0CDUL) /**< \brief (EVSYS) Channel 21 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG21      (*(RwReg8 *)0x4100E0CEUL) /**< \brief (EVSYS) Channel 21 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS21       (*(RoReg8 *)0x4100E0CFUL) /**< \brief (EVSYS) Channel 21 Status */
#define REG_EVSYS_CHANNEL22        (*(RwReg  *)0x4100E0D0UL) /**< \brief (EVSYS) Channel 22 Control */
#define REG_EVSYS_CHINTENCLR22     (*(RwReg8 *)0x4100E0D4UL) /**< \brief (EVSYS) Channel 22 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET22     (*(RwReg8 *)0x4100E0D5UL) /**< \brief (EVSYS) Channel 22 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG22      (*(RwReg8 *)0x4100E0D6UL) /**< \brief (EVSYS) Channel 22 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS22       (*(RoReg8 *)0x4100E0D7UL) /**< \brief (EVSYS) Channel 22 Status */
#define REG_EVSYS_CHANNEL23        (*(RwReg  *)0x4100E0D8UL) /**< \brief (EVSYS) Channel 23 Control */
#define REG_EVSYS_CHINTENCLR23     (*(RwReg8 *)0x4100E0DCUL) /**< \brief (EVSYS) Channel 23 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET23     (*(RwReg8 *)0x4100E0DDUL) /**< \brief (EVSYS) Channel 23 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG23      (*(RwReg8 *)0x4100E0DEUL) /**< \brief (EVSYS) Channel 23 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS23       (*(RoReg8 *)0x4100E0DFUL) /**< \brief (EVSYS) Channel 23 Status */
#define REG_EVSYS_CHANNEL24        (*(RwReg  *)0x4100E0E0UL) /**< \brief (EVSYS) Channel 24 Control */
#define REG_EVSYS_CHINTENCLR24     (*(RwReg8 *)0x4100E0E4UL) /**< \brief (EVSYS) Channel 24 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET24     (*(RwReg8 *)0x4100E0E5UL) /**< \brief (EVSYS) Channel 24 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG24      (*(RwReg8 *)0x4100E0E6UL) /**< \brief (EVSYS) Channel 24 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS24       (*(RoReg8 *)0x4100E0E7UL) /**< \brief (EVSYS) Channel 24 Status */
#define REG_EVSYS_CHANNEL25        (*(RwReg  *)0x4100E0E8UL) /**< \brief (EVSYS) Channel 25 Control */
#define REG_EVSYS_CHINTENCLR25     (*(RwReg8 *)0x4100E0ECUL) /**< \brief (EVSYS) Channel 25 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET25     (*(RwReg8 *)0x4100E0EDUL) /**< \brief (EVSYS) Channel 25 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG25      (*(RwReg8 *)0x4100E0EEUL) /**< \brief (EVSYS) Channel 25 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS25       (*(RoReg8 *)0x4100E0EFUL) /**< \brief (EVSYS) Channel 25 Status */
#define REG_EVSYS_CHANNEL26        (*(RwReg  *)0x4100E0F0UL) /**< \brief (EVSYS) Channel 26 Control */
#define REG_EVSYS_CHINTENCLR26     (*(RwReg8 *)0x4100E0F4UL) /**< \brief (EVSYS) Channel 26 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET26     (*(RwReg8 *)0x4100E0F5UL) /**< \brief (EVSYS) Channel 26 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG26      (*(RwReg8 *)0x4100E0F6UL) /**< \brief (EVSYS) Channel 26 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS26       (*(RoReg8 *)0x4100E0F7UL) /**< \brief (EVSYS) Channel 26 Status */
#define REG_EVSYS_CHANNEL27        (*(RwReg  *)0x4100E0F8UL) /**< \brief (EVSYS) Channel 27 Control */
#define REG_EVSYS_CHINTENCLR27     (*(RwReg8 *)0x4100E0FCUL) /**< \brief (EVSYS) Channel 27 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET27     (*(RwReg8 *)0x4100E0FDUL) /**< \brief (EVSYS) Channel 27 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG27      (*(RwReg8 *)0x4100E0FEUL) /**< \brief (EVSYS) Channel 27 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS27       (*(RoReg8 *)0x4100E0FFUL) /**< \brief (EVSYS) Channel 27 Status */
#define REG_EVSYS_CHANNEL28        (*(RwReg  *)0x4100E100UL) /**< \brief (EVSYS) Channel 28 Control */
#define REG_EVSYS_CHINTENCLR28     (*(RwReg8 *)0x4100E104UL) /**< \brief (EVSYS) Channel 28 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET28     (*(RwReg8 *)0x4100E105UL) /**< \brief (EVSYS) Channel 28 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG28      (*(RwReg8 *)0x4100E106UL) /**< \brief (EVSYS) Channel 28 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS28       (*(RoReg8 *)0x4100E107UL) /**< \brief (EVSYS) Channel 28 Status */
#define REG_EVSYS_CHANNEL29        (*(RwReg  *)0x4100E108UL) /**< \brief (EVSYS) Channel 29 Control */
#define REG_EVSYS_CHINTENCLR29     (*(RwReg8 *)0x4100E10CUL) /**< \brief (EVSYS) Channel 29 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET29     (*(RwReg8 *)0x4100E10DUL) /**< \brief (EVSYS) Channel 29 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG29      (*(RwReg8 *)0x4100E10EUL) /**< \brief (EVSYS) Channel 29 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS29       (*(RoReg8 *)0x4100E10FUL) /**< \brief (EVSYS) Channel 29 Status */
#define REG_EVSYS_CHANNEL30        (*(RwReg  *)0x4100E110UL) /**< \brief (EVSYS) Channel 30 Control */
#define REG_EVSYS_CHINTENCLR30     (*(RwReg8 *)0x4100E114UL) /**< \brief (EVSYS) Channel 30 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET30     (*(RwReg8 *)0x4100E115UL) /**< \brief (EVSYS) Channel 30 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG30      (*(RwReg8 *)0x4100E116UL) /**< \brief (EVSYS) Channel 30 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS30       (*(RoReg8 *)0x4100E117UL) /**< \brief (EVSYS) Channel 30 Status */
#define REG_EVSYS_CHANNEL31        (*(RwReg  *)0x4100E118UL) /**< \brief (EVSYS) Channel 31 Control */
#define REG_EVSYS_CHINTENCLR31     (*(RwReg8 *)0x4100E11CUL) /**< \brief (EVSYS) Channel 31 Interrupt Enable Clear */
#define REG_EVSYS_CHINTENSET31     (*(RwReg8 *)0x4100E11DUL) /**< \brief (EVSYS) Channel 31 Interrupt Enable Set */
#define REG_EVSYS_CHINTFLAG31      (*(RwReg8 *)0x4100E11EUL) /**< \brief (EVSYS) Channel 31 Interrupt Flag Status and Clear */
#define REG_EVSYS_CHSTATUS31       (*(RoReg8 *)0x4100E11FUL) /**< \brief (EVSYS) Channel 31 Status */
#define REG_EVSYS_USER0            (*(RwReg  *)0x4100E120UL) /**< \brief (EVSYS) User Multiplexer 0 */
#define REG_EVSYS_USER1            (*(RwReg  *)0x4100E124UL) /**< \brief (EVSYS) User Multiplexer 1 */
#define REG_EVSYS_USER2            (*(RwReg  *)0x4100E128UL) /**< \brief (EVSYS) User Multiplexer 2 */
#define REG_EVSYS_USER3            (*(RwReg  *)0x4100E12CUL) /**< \brief (EVSYS) User Multiplexer 3 */
#define REG_EVSYS_USER4            (*(RwReg  *)0x4100E130UL) /**< \brief (EVSYS) User Multiplexer 4 */
#define REG_EVSYS_USER5            (*(RwReg  *)0x4100E134UL) /**< \brief (EVSYS) User Multiplexer 5 */
#define REG_EVSYS_USER6            (*(RwReg  *)0x4100E138UL) /**< \brief (EVSYS) User Multiplexer 6 */
#define REG_EVSYS_USER7            (*(RwReg  *)0x4100E13CUL) /**< \brief (EVSYS) User Multiplexer 7 */
#define REG_EVSYS_USER8            (*(RwReg  *)0x4100E140UL) /**< \brief (EVSYS) User Multiplexer 8 */
#define REG_EVSYS_USER9            (*(RwReg  *)0x4100E144UL) /**< \brief (EVSYS) User Multiplexer 9 */
#define REG_EVSYS_USER10           (*(RwReg  *)0x4100E148UL) /**< \brief (EVSYS) User Multiplexer 10 */
#define REG_EVSYS_USER11           (*(RwReg  *)0x4100E14CUL) /**< \brief (EVSYS) User Multiplexer 11 */
#define REG_EVSYS_USER12           (*(RwReg  *)0x4100E150UL) /**< \brief (EVSYS) User Multiplexer 12 */
#define REG_EVSYS_USER13           (*(RwReg  *)0x4100E154UL) /**< \brief (EVSYS) User Multiplexer 13 */
#define REG_EVSYS_USER14           (*(RwReg  *)0x4100E158UL) /**< \brief (EVSYS) User Multiplexer 14 */
#define REG_EVSYS_USER15           (*(RwReg  *)0x4100E15CUL) /**< \brief (EVSYS) User Multiplexer 15 */
#define REG_EVSYS_USER16           (*(RwReg  *)0x4100E160UL) /**< \brief (EVSYS) User Multiplexer 16 */
#define REG_EVSYS_USER17           (*(RwReg  *)0x4100E164UL) /**< \brief (EVSYS) User Multiplexer 17 */
#define REG_EVSYS_USER18           (*(RwReg  *)0x4100E168UL) /**< \brief (EVSYS) User Multiplexer 18 */
#define REG_EVSYS_USER19           (*(RwReg  *)0x4100E16CUL) /**< \brief (EVSYS) User Multiplexer 19 */
#define REG_EVSYS_USER20           (*(RwReg  *)0x4100E170UL) /**< \brief (EVSYS) User Multiplexer 20 */
#define REG_EVSYS_USER21           (*(RwReg  *)0x4100E174UL) /**< \brief (EVSYS) User Multiplexer 21 */
#define REG_EVSYS_USER22           (*(RwReg  *)0x4100E178UL) /**< \brief (EVSYS) User Multiplexer 22 */
#define REG_EVSYS_USER23           (*(RwReg  *)0x4100E17CUL) /**< \brief (EVSYS) User Multiplexer 23 */
#define REG_EVSYS_USER24           (*(RwReg  *)0x4100E180UL) /**< \brief (EVSYS) User Multiplexer 24 */
#define REG_EVSYS_USER25           (*(RwReg  *)0x4100E184UL) /**< \brief (EVSYS) User Multiplexer 25 */
#define REG_EVSYS_USER26           (*(RwReg  *)0x4100E188UL) /**< \brief (EVSYS) User Multiplexer 26 */
#define REG_EVSYS_USER27           (*(RwReg  *)0x4100E18CUL) /**< \brief (EVSYS) User Multiplexer 27 */
#define REG_EVSYS_USER28           (*(RwReg  *)0x4100E190UL) /**< \brief (EVSYS) User Multiplexer 28 */
#define REG_EVSYS_USER29           (*(RwReg  *)0x4100E194UL) /**< \brief (EVSYS) User Multiplexer 29 */
#define REG_EVSYS_USER30           (*(RwReg  *)0x4100E198UL) /**< \brief (EVSYS) User Multiplexer 30 */
#define REG_EVSYS_USER31           (*(RwReg  *)0x4100E19CUL) /**< \brief (EVSYS) User Multiplexer 31 */
#define REG_EVSYS_USER32           (*(RwReg  *)0x4100E1A0UL) /**< \brief (EVSYS) User Multiplexer 32 */
#define REG_EVSYS_USER33           (*(RwReg  *)0x4100E1A4UL) /**< \brief (EVSYS) User Multiplexer 33 */
#define REG_EVSYS_USER34           (*(RwReg  *)0x4100E1A8UL) /**< \brief (EVSYS) User Multiplexer 34 */
#define REG_EVSYS_USER35           (*(RwReg  *)0x4100E1ACUL) /**< \brief (EVSYS) User Multiplexer 35 */
#define REG_EVSYS_USER36           (*(RwReg  *)0x4100E1B0UL) /**< \brief (EVSYS) User Multiplexer 36 */
#define REG_EVSYS_USER37           (*(RwReg  *)0x4100E1B4UL) /**< \brief (EVSYS) User Multiplexer 37 */
#define REG_EVSYS_USER38           (*(RwReg  *)0x4100E1B8UL) /**< \brief (EVSYS) User Multiplexer 38 */
#define REG_EVSYS_USER39           (*(RwReg  *)0x4100E1BCUL) /**< \brief (EVSYS) User Multiplexer 39 */
#define REG_EVSYS_USER40           (*(RwReg  *)0x4100E1C0UL) /**< \brief (EVSYS) User Multiplexer 40 */
#define REG_EVSYS_USER41           (*(RwReg  *)0x4100E1C4UL) /**< \brief (EVSYS) User Multiplexer 41 */
#define REG_EVSYS_USER42           (*(RwReg  *)0x4100E1C8UL) /**< \brief (EVSYS) User Multiplexer 42 */
#define REG_EVSYS_USER43           (*(RwReg  *)0x4100E1CCUL) /**< \brief (EVSYS) User Multiplexer 43 */
#define REG_EVSYS_USER44           (*(RwReg  *)0x4100E1D0UL) /**< \brief (EVSYS) User Multiplexer 44 */
#define REG_EVSYS_USER45           (*(RwReg  *)0x4100E1D4UL) /**< \brief (EVSYS) User Multiplexer 45 */
#define REG_EVSYS_USER46           (*(RwReg  *)0x4100E1D8UL) /**< \brief (EVSYS) User Multiplexer 46 */
#define REG_EVSYS_USER47           (*(RwReg  *)0x4100E1DCUL) /**< \brief (EVSYS) User Multiplexer 47 */
#define REG_EVSYS_USER48           (*(RwReg  *)0x4100E1E0UL) /**< \brief (EVSYS) User Multiplexer 48 */
#define REG_EVSYS_USER49           (*(RwReg  *)0x4100E1E4UL) /**< \brief (EVSYS) User Multiplexer 49 */
#define REG_EVSYS_USER50           (*(RwReg  *)0x4100E1E8UL) /**< \brief (EVSYS) User Multiplexer 50 */
#define REG_EVSYS_USER51           (*(RwReg  *)0x4100E1ECUL) /**< \brief (EVSYS) User Multiplexer 51 */
#define REG_EVSYS_USER52           (*(RwReg  *)0x4100E1F0UL) /**< \brief (EVSYS) User Multiplexer 52 */
#define REG_EVSYS_USER53           (*(RwReg  *)0x4100E1F4UL) /**< \brief (EVSYS) User Multiplexer 53 */
#define REG_EVSYS_USER54           (*(RwReg  *)0x4100E1F8UL) /**< \brief (EVSYS) User Multiplexer 54 */
#define REG_EVSYS_USER55           (*(RwReg  *)0x4100E1FCUL) /**< \brief (EVSYS) User Multiplexer 55 */
#define REG_EVSYS_USER56           (*(RwReg  *)0x4100E200UL) /**< \brief (EVSYS) User Multiplexer 56 */
#define REG_EVSYS_USER57           (*(RwReg  *)0x4100E204UL) /**< \brief (EVSYS) User Multiplexer 57 */
#define REG_EVSYS_USER58           (*(RwReg  *)0x4100E208UL) /**< \brief (EVSYS) User Multiplexer 58 */
#define REG_EVSYS_USER59           (*(RwReg  *)0x4100E20CUL) /**< \brief (EVSYS) User Multiplexer 59 */
#define REG_EVSYS_USER60           (*(RwReg  *)0x4100E210UL) /**< \brief (EVSYS) User Multiplexer 60 */
#define REG_EVSYS_USER61           (*(RwReg  *)0x4100E214UL) /**< \brief (EVSYS) User Multiplexer 61 */
#define REG_EVSYS_USER62           (*(RwReg  *)0x4100E218UL) /**< \brief (EVSYS) User Multiplexer 62 */
#define REG_EVSYS_USER63           (*(RwReg  *)0x4100E21CUL) /**< \brief (EVSYS) User Multiplexer 63 */
#define REG_EVSYS_USER64           (*(RwReg  *)0x4100E220UL) /**< \brief (EVSYS) User Multiplexer 64 */
#define REG_EVSYS_USER65           (*(RwReg  *)0x4100E224UL) /**< \brief (EVSYS) User Multiplexer 65 */
#define REG_EVSYS_USER66           (*(RwReg  *)0x4100E228UL) /**< \brief (EVSYS) User Multiplexer 66 */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_EVSYS_INSTANCE_FIXUP_H_ */

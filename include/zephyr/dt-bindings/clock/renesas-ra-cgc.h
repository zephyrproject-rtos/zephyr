/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DT_BINDINGS_CLOCK_RENESAS_RA_CGC_H_
#define ZEPHYR_DT_BINDINGS_CLOCK_RENESAS_RA_CGC_H_

#define RA_CLOCK(grp, func, ch) ((grp << 28) | (func << 20) | ch)

#define RA_CLOCK_GROUP(mod) (((mod >> 28) & 0xF) * 4)
#define RA_CLOCK_BIT(mod)   BIT(((mod >> 20) & 0xFF) - ((mod >> 0) & 0xF))

#define RA_CLOCK_DMAC(channel)    RA_CLOCK(0, 22, channel)
#define RA_CLOCK_DTC(channel)     RA_CLOCK(0, 22, channel)
#define RA_CLOCK_CAN(channel)     RA_CLOCK(1, 2, channel)
#define RA_CLOCK_CEC(channel)     RA_CLOCK(1, 3U, channel)
#define RA_CLOCK_I3C(channel)     RA_CLOCK(1, 4U, channel)
#define RA_CLOCK_IRDA(channel)    RA_CLOCK(1, 5U, channel)
#define RA_CLOCK_QSPI(channel)    RA_CLOCK(1, 6U, channel)
#define RA_CLOCK_IIC(channel)     RA_CLOCK(1, 9U, channel)
#define RA_CLOCK_USBFS(channel)   RA_CLOCK(1, 11U, channel)
#define RA_CLOCK_USBHS(channel)   RA_CLOCK(1, 12U, channel)
#define RA_CLOCK_EPTPC(channel)   RA_CLOCK(1, 13U, channel)
#define RA_CLOCK_ETHER(channel)   RA_CLOCK(1, 15U, channel)
#define RA_CLOCK_OSPI(channel)    RA_CLOCK(1, 16U, channel)
#define RA_CLOCK_SPI(channel)     RA_CLOCK(1, 19U, channel)
#define RA_CLOCK_SCI(channel)     RA_CLOCK(1, 31U, channel)
#define RA_CLOCK_CAC(channel)     RA_CLOCK(2, 0U, channel)
#define RA_CLOCK_CRC(channel)     RA_CLOCK(2, 1U, channel)
#define RA_CLOCK_PDC(channel)     RA_CLOCK(2, 2U, channel)
#define RA_CLOCK_CTSU(channel)    RA_CLOCK(2, 3U, channel)
#define RA_CLOCK_SLCDC(channel)   RA_CLOCK(2, 4U, channel)
#define RA_CLOCK_GLCDC(channel)   RA_CLOCK(2, 4U, channel)
#define RA_CLOCK_JPEG(channel)    RA_CLOCK(2, 5U, channel)
#define RA_CLOCK_DRW(channel)     RA_CLOCK(2, 6U, channel)
#define RA_CLOCK_SSI(channel)     RA_CLOCK(2, 8U, channel)
#define RA_CLOCK_SRC(channel)     RA_CLOCK(2, 9U, channel)
#define RA_CLOCK_SDHIMMC(channel) RA_CLOCK(2, 12U, channel)
#define RA_CLOCK_DOC(channel)     RA_CLOCK(2, 13U, channel)
#define RA_CLOCK_ELC(channel)     RA_CLOCK(2, 14U, channel)
#define RA_CLOCK_CEU(channel)     RA_CLOCK(2, 16U, channel)
#define RA_CLOCK_TFU(channel)     RA_CLOCK(2, 20U, channel)
#define RA_CLOCK_IIRFA(channel)   RA_CLOCK(2, 21U, channel)
#define RA_CLOCK_CANFD(channel)   RA_CLOCK(2, 27U, channel)
#define RA_CLOCK_TRNG(channel)    RA_CLOCK(2, 28U, channel)
#define RA_CLOCK_SCE(channel)     RA_CLOCK(2, 31U, channel)
#define RA_CLOCK_AES(channel)     RA_CLOCK(2, 31U, channel)
#define RA_CLOCK_POEG(channel)    RA_CLOCK(3, 14U, channel)
#define RA_CLOCK_ADC(channel)     RA_CLOCK(3, 16U, channel)
#define RA_CLOCK_SDADC(channel)   RA_CLOCK(3, 17U, channel)
#define RA_CLOCK_DAC8(channel)    RA_CLOCK(3, 19U, channel)
#define RA_CLOCK_DAC(channel)     RA_CLOCK(3, 20U, channel)
#define RA_CLOCK_TSN(channel)     RA_CLOCK(3, 22U, channel)
#define RA_CLOCK_ACMPHS(channel)  RA_CLOCK(3, 28U, channel)
#define RA_CLOCK_ACMPLP(channel)  RA_CLOCK(3, 29U, channel)
#define RA_CLOCK_OPAMP(channel)   RA_CLOCK(3, 31U, channel)
#define RA_CLOCK_AGT(channel)     RA_CLOCK(4, 3U, channel)
#define RA_CLOCK_KEY(channel)     RA_CLOCK(4, 4U, channel)
#define RA_CLOCK_ULPT(channel)    RA_CLOCK(4, 9U, channel)
#define RA_CLOCK_GPT(channel)     RA_CLOCK(5, 31U, channel)

#endif /* ZEPHYR_DT_BINDINGS_CLOCK_RENESAS_RA_CGC_H_ */

/*
 * SPDX-FileCopyrightText: Copyright (c) Cirrus Logic 2021
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_regs.h
 * @brief CS47L63 register definitions (internal)
 *
 * Addresses, field masks and shifts are derived from the Apache-2.0 vendor
 * source modules/hal/cirrus-logic/cs47l63/cs47l63_spec.h. Field encodings that
 * the header does not enumerate - the sample-rate code, the ASP1 format code,
 * the SYSCLK source - come from the datasheet, docs/codec/CS47L63_DS1249F2.pdf,
 * and that is the authority for them. The vendor's annotated bring-up script
 * (modules/hal/cirrus-logic/cs47l63/config/wisce_init.txt) names some of the
 * same codes in words, but it is one worked example of a whole register write
 * rather than a specification, and a field lifted out of it does not carry the
 * rest of the write it was consistent with: see SYSCLK_SRC below.
 *
 * Shared by every translation unit in this driver: a unit never defines a
 * register of its own to avoid touching this file.
 *
 * Every register on this part is 32 bits wide at a 32-bit address.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_REGS_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_REGS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* R0 - DEVID: 24-bit device identifier. */
#define CS47L63_DEVID      0x00000000
#define CS47L63_DEVID_MASK 0x00FFFFFF

/* R1 - REVID: silicon and metal/mask revision. */
#define CS47L63_REVID               0x00000004
#define CS47L63_REVID_AREVID_MASK   0x000000F0 /* D7-D4: silicon revision */
#define CS47L63_REVID_MTLREVID_MASK 0x0000000F /* D3-D0: metal/mask revision */

/* R4 - OTPID: OTP trim variant, D3-D0. Selects which trim block the boot
 * sequence has to write - see cs47l63_boot.c.
 */
#define CS47L63_OTPID      0x00000010
#define CS47L63_OTPID_MASK 0x0000000F

/* R12 / R13 - register-region lock. The trim registers are behind these two:
 * writing the unlock pair to each opens the region, writing the lock pair
 * closes it.
 */
#define CS47L63_TEST_KEY_CTRL    0x00000030
#define CS47L63_USER_KEY_CTRL    0x00000034
#define CS47L63_KEY_UNLOCK_CODE0 0x00000055
#define CS47L63_KEY_UNLOCK_CODE1 0x000000AA
#define CS47L63_KEY_LOCK_CODE0   0x000000CC
#define CS47L63_KEY_LOCK_CODE1   0x00000033

/* Trim registers, written only by the OTPID-gated trim block in cs47l63_boot.c.
 * The names come from cs47l63_spec.h; the values do not decompose into anything
 * the field names explain, which is why the block is carried as a unit.
 */
#define CS47L63_MICBIAS_TST_CTRL1 0x00002420
#define CS47L63_MICBIAS_TST_CTRL4 0x00002424
#define CS47L63_HP_OCD_CTRL1      0x000024AC
#define CS47L63_HP_OCD_TEST1      0x000024B4
#define CS47L63_DAC_IF_CONTROL_1  0x00004D68
#define CS47L63_DAC_IF_TEST_1     0x00004D70

/* R770-R773 - GPIO1..4 control. On this board these four pins carry ASP1
 * (I2S): DOUT, DIN, BCLK and FSYNC respectively. GPn_FN (D10-D0) = 0 selects
 * the pin's ASP function; GPn_DIR (D31) is 1 for an input.
 *
 * The pad configuration below the direction bit (pull-ups, drive strength,
 * CMOS output, no debounce) is the vendor bring-up script's value for these
 * four pins, carried whole rather than recomposed field by field.
 */
#define CS47L63_GPIO1_CTRL1      0x00000C08
#define CS47L63_GPIO2_CTRL1      0x00000C0C
#define CS47L63_GPIO3_CTRL1      0x00000C10
#define CS47L63_GPIO4_CTRL1      0x00000C14
/** GPn_DIR, D31: 1 = input. */
#define CS47L63_GP_DIR_INPUT     0x80000000
/** Pad configuration for an ASP1 pin, with GPn_FN = 0 and GPn_DIR clear. */
#define CS47L63_GP_CTRL1_ASP_PAD 0x61000000

/* R5120 - CLOCK32K. */
#define CS47L63_CLOCK32K          0x00001400
#define CS47L63_CLK_32K_EN        0x00000040
#define CS47L63_CLK_32K_SRC_MASK  0x00000003
#define CS47L63_CLK_32K_SRC_MCLK2 0x00000001

/* R5121 - SYSTEM_CLOCK1. */
#define CS47L63_SYSTEM_CLOCK1      0x00001404
/** SYSCLK_FRAC, D15: 0 = SYSCLK is a multiple of 6.144 MHz, 1 = of 5.6448 MHz. */
#define CS47L63_SYSCLK_FRAC        0x00008000
#define CS47L63_SYSCLK_FREQ_MASK   0x00000700
#define CS47L63_SYSCLK_FREQ_SHIFT  8
#define CS47L63_SYSCLK_EN          0x00000040
#define CS47L63_SYSCLK_SRC_MASK    0x0000001F
/* SYSCLK_SRC = FLL1 at its own output frequency. The datasheet's SYSCLK_SRC
 * table (DS1249F2 table 4-48, p. 131) gives two codes for the same loop: 0x0C
 * is FLL1 at 45-50 MHz, 0x04 is FLL1 x 2 at 90-100 MHz. Both are legal, but
 * SYSCLK_FREQ has to name whichever one the mux actually delivers, and this
 * driver derives that code from the FLL's own output (see sysclk_freq_code in
 * cs47l63_clock.c). Pairing SYSCLK_FREQ with 0x04 tells the part it is running
 * at half the rate it really is; the analog output then fills with images
 * almost as loud as the signal.
 *
 * The vendor's bring-up script does use 0x04, and correctly: its single write
 * of SYSTEM_CLOCK1 sets SYSCLK_FREQ to 98.304 MHz in the same word. The two
 * fields are only meaningful together, so 0x04 is the right code for a driver
 * that names the doubled rate and the wrong one for this driver, which does
 * not. The doubled clock is the datasheet's typical choice and buys
 * signal-mixing and processing capacity a single stereo path does not need.
 */
#define CS47L63_SYSCLK_SRC_FLL1    0x0000000C
/** SYSCLK_FREQ codes, in the 6.144 MHz family (SYSCLK_FRAC = 0). */
#define CS47L63_SYSCLK_FREQ_6M144  0
#define CS47L63_SYSCLK_FREQ_12M288 1
#define CS47L63_SYSCLK_FREQ_24M576 2
#define CS47L63_SYSCLK_FREQ_49M152 3
/* Only reachable through SYSCLK_SRC = 0x04; the FLL itself cannot output it,
 * because DS1249F2 section 4.10.7 caps FFLL at 50 MHz.
 */
#define CS47L63_SYSCLK_FREQ_98M304 4

/* R65 - SYSTEM_CLOCK2: the readback twin of SYSTEM_CLOCK1. SYSCLK_STS says
 * whether SYSCLK is actually running, which is not the same question as
 * SYSCLK_EN in SYSTEM_CLOCK1 - that one only says it was asked to.
 */
#define CS47L63_SYSTEM_CLOCK2         0x00001408
#define CS47L63_SYSCLK_FREQ_STS_MASK  0x00000700
#define CS47L63_SYSCLK_FREQ_STS_SHIFT 8
#define CS47L63_SYSCLK_STS            0x00000040
#define CS47L63_SYSCLK_SRC_STS_MASK   0x0000001F

/* R5128 - SAMPLE_RATE1: the first of the part's four global rate slots. Every
 * datapath selects one of the four by index rather than carrying a rate of its
 * own; this driver uses slot 1 exclusively.
 */
#define CS47L63_SAMPLE_RATE1     0x00001420
#define CS47L63_SAMPLE_RATE_MASK 0x0000001F
/** Rate codes for SAMPLE_RATEn. 48 kHz is the code the vendor script states. */
#define CS47L63_SAMPLE_RATE_48K  0x03
#define CS47L63_SAMPLE_RATE_24K  0x02
#define CS47L63_SAMPLE_RATE_16K  0x12

/* R6144-R6149 - FLL1. CONTROL5 and CONTROL6 are not written by this driver. */
#define CS47L63_FLL1_CONTROL1 0x00001C00
#define CS47L63_FLL1_CONTROL2 0x00001C04
#define CS47L63_FLL1_CONTROL3 0x00001C08
#define CS47L63_FLL1_CONTROL4 0x00001C0C

#define CS47L63_FLL1_CTRL_UPD 0x00000004
#define CS47L63_FLL1_HOLD     0x00000002
#define CS47L63_FLL1_EN       0x00000001

#define CS47L63_FLL1_LOCKDET_THR_MASK  0xF0000000
#define CS47L63_FLL1_LOCKDET_THR_SHIFT 28
/** Lock detector enable. With this clear, FLL1_LOCK_STS1 can never rise. */
#define CS47L63_FLL1_LOCKDET           0x08000000
#define CS47L63_FLL1_PHASEDET          0x00400000
/** Reference detector enable. */
#define CS47L63_FLL1_REFDET            0x00200000
#define CS47L63_FLL1_REFCLK_DIV_MASK   0x00030000
#define CS47L63_FLL1_REFCLK_DIV_SHIFT  16
#define CS47L63_FLL1_REFCLK_SRC_MASK   0x0000F000
#define CS47L63_FLL1_REFCLK_SRC_SHIFT  12
#define CS47L63_FLL1_N_MASK            0x000003FF
#define CS47L63_FLL1_N_SHIFT           0

#define CS47L63_FLL1_LAMBDA_SHIFT 16
#define CS47L63_FLL1_THETA_SHIFT  0

#define CS47L63_FLL1_GAIN_MASK                                                                     \
	0xFFFF0000 /* FD_GAIN_COARSE and the fine/PD                                               \
		    * gain fields around it, written                                               \
		    * together as one 16-bit gain word
		    */
#define CS47L63_FLL1_GAIN_SHIFT   16
#define CS47L63_FLL1_HP_MASK      0x00003000
#define CS47L63_FLL1_HP_SHIFT     12
#define CS47L63_FLL1_FB_DIV_MASK  0x000003FF
#define CS47L63_FLL1_FB_DIV_SHIFT 0

/** FLL1_REFCLK_SRC codes. */
#define CS47L63_FLL_SRC_MCLK1 0x0
#define CS47L63_FLL_SRC_MCLK2 0x1

/* R235-R240 - output stage. This part has one output channel, OUT1L, driving a
 * differential headphone load; there is no OUT1R and no OUT2 anywhere in the
 * register map.
 */
#define CS47L63_OUTPUT_ENABLE_1 0x00004804
#define CS47L63_OUTPUT_STATUS_1 0x00004808
#define CS47L63_OUT1L_VOLUME_1  0x00004818
#define CS47L63_OUT1L_EN        0x00000002
#define CS47L63_OUT1L_EN_STS    0x00000002
/** OUT_VU, D9: latch the volume field on write. Write-only strobe. */
#define CS47L63_OUT_VU          0x00000200
#define CS47L63_OUT1L_MUTE      0x00000100
#define CS47L63_OUT1L_VOL_MASK  0x000000FF
/** OUT1L_VOL code for 0 dB, the top of the usable range. */
#define CS47L63_OUT1L_VOL_0DB   0x80

/* R272-R280 - ASP1, the serial audio port wired to the nRF's I2S. */
#define CS47L63_ASP1_ENABLES1      0x00006000
#define CS47L63_ASP1_CONTROL1      0x00006004
#define CS47L63_ASP1_CONTROL2      0x00006008
#define CS47L63_ASP1_DATA_CONTROL1 0x00006030
#define CS47L63_ASP1_DATA_CONTROL5 0x00006040

#define CS47L63_ASP1_RX1_EN 0x00010000
#define CS47L63_ASP1_RX2_EN 0x00020000
#define CS47L63_ASP1_TX1_EN 0x00000001
#define CS47L63_ASP1_TX2_EN 0x00000002

/** ASP1_RATE, D12-D8: which of the four global rate slots this port follows. */
#define CS47L63_ASP1_RATE_MASK             0x00001F00
#define CS47L63_ASP1_RATE_SHIFT            8
#define CS47L63_ASP1_RATE_SEL_SAMPLE_RATE1 0

#define CS47L63_ASP1_RX_WIDTH_MASK  0xFF000000
#define CS47L63_ASP1_RX_WIDTH_SHIFT 24
#define CS47L63_ASP1_TX_WIDTH_MASK  0x00FF0000
#define CS47L63_ASP1_TX_WIDTH_SHIFT 16
#define CS47L63_ASP1_FMT_MASK       0x00000700
#define CS47L63_ASP1_FMT_SHIFT      8
/** ASP1_FMT code for I2S mode, as annotated in the vendor bring-up script. */
#define CS47L63_ASP1_FMT_I2S        0x2
#define CS47L63_ASP1_BCLK_INV       0x00000040
#define CS47L63_ASP1_BCLK_MSTR      0x00000010
#define CS47L63_ASP1_FSYNC_INV      0x00000004
#define CS47L63_ASP1_FSYNC_MSTR     0x00000001

#define CS47L63_ASP1_TX_WL_MASK 0x0000003F
#define CS47L63_ASP1_RX_WL_MASK 0x0000003F

/* R155-R169 - the analog input path. Only IN2 is wired to a connector on the
 * boards this driver runs on: IN1 is the PDM microphone pair, which this
 * driver does not bring up.
 */
#define CS47L63_INPUT_CONTROL  0x00004000
#define CS47L63_IN2L_EN        0x00000008
#define CS47L63_IN2R_EN        0x00000004
#define CS47L63_INPUT_CONTROL3 0x00004014
/** Latches every input volume field written since the last strobe. */
#define CS47L63_IN_VU          0x20000000

#define CS47L63_INPUT2_CONTROL1 0x00004060
#define CS47L63_IN2_OSR_MASK    0x00070000
#define CS47L63_IN2_OSR_SHIFT   16
/** Oversampling the analog input path runs at on the 48 kHz rate family. */
#define CS47L63_IN2_OSR_3M072   5
/** IN2_MODE: 0 selects the analog input, 1 the digital (PDM) one. */
#define CS47L63_IN2_MODE        0x00000001

#define CS47L63_IN2L_CONTROL1        0x00004064
#define CS47L63_IN2L_CONTROL2        0x00004068
#define CS47L63_IN2R_CONTROL1        0x00004084
#define CS47L63_IN2R_CONTROL2        0x00004088
#define CS47L63_IN2_SRC_MASK         0x30000000
#define CS47L63_IN2_SRC_SHIFT        28
/** IN2n_SRC code for a single-ended input; the boards do not wire it
 *  differentially.
 */
#define CS47L63_IN2_SRC_SINGLE_ENDED 1
#define CS47L63_IN2_MUTE             0x10000000
#define CS47L63_IN2_VOL_MASK         0x00FF0000
#define CS47L63_IN2_VOL_SHIFT        16
/** Digital input volume code for 0 dB, on the same 0.5 dB per step scale as
 *  the output volume.
 */
#define CS47L63_IN2_VOL_0DB          0x80
#define CS47L63_IN2_PGA_VOL_MASK     0x000000FE
#define CS47L63_IN2_PGA_VOL_SHIFT    1
/** Analog PGA gain code for 0 dB. */
#define CS47L63_IN2_PGA_VOL_0DB      0x40

/* R315, R319 - the two ASP1 transmit mixers, laid out exactly like the OUT1L
 * mixer slots above.
 */
#define CS47L63_ASP1TX1_INPUT1 0x00008200
#define CS47L63_ASP1TX2_INPUT1 0x00008210

/* R311-R312 - the OUT1L mixer's first two input slots. Each slot names a
 * source and carries its own mix volume.
 */
#define CS47L63_OUT1L_INPUT1       0x00008100
#define CS47L63_OUT1L_INPUT2       0x00008104
#define CS47L63_OUT1LMIX_VOL_MASK  0x00FE0000
#define CS47L63_OUT1LMIX_VOL_SHIFT 17
/** Mixer slot volume code for 0 dB. */
#define CS47L63_OUT1LMIX_VOL_0DB   0x40
#define CS47L63_OUT1L_SRC_MASK     0x000001FF
/** Mixer source codes. 0 is the "no source" encoding that mutes a slot. */
#define CS47L63_MIXER_SRC_NONE     0x000
#define CS47L63_MIXER_SRC_ASP1RX1  0x020
#define CS47L63_MIXER_SRC_ASP1RX2  0x021
#define CS47L63_MIXER_SRC_IN2L     0x012
#define CS47L63_MIXER_SRC_IN2R     0x013

/* R736-R740 - interrupt-1 edge flags. Every EINT bit is write-1-to-clear. */
#define CS47L63_IRQ1_EINT_1 0x00018010
#define CS47L63_IRQ1_EINT_2 0x00018014
#define CS47L63_IRQ1_EINT_6 0x00018024
#define CS47L63_IRQ1_STS_6  0x000180A4

#define CS47L63_OUT1L_SC_EINT1       0x00020000 /* OUT1L short circuit */
#define CS47L63_SYSCLK_ERR_EINT1     0x00000400
#define CS47L63_SYSCLK_FAIL_EINT1    0x00000100
#define CS47L63_BOOT_DONE_EINT1      0x00000008
#define CS47L63_FLL1_REF_LOST_EINT1  0x00000100
#define CS47L63_FLL1_LOCK_FALL_EINT1 0x00000002
#define CS47L63_FLL1_LOCK_RISE_EINT1 0x00000001

/* IRQ1_STS_6 live status bits. FLL1_REF_LOST_STS1 is the part's own answer
 * to whether it currently sees a clock on the selected reference pin; it
 * means nothing unless FLL1_REFDET is set.
 */
#define CS47L63_FLL1_LOCK_STS1     0x00000001
#define CS47L63_FLL1_REF_LOST_STS1 0x00000100

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_REGS_H_ */

/*
 * SPDX-FileCopyrightText: Copyright (c) Cirrus Logic 2021
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63.h
 * @brief CS47L63 register map, instance data and internal interfaces (internal)
 *
 * Addresses and field masks are from the vendor's
 * modules/hal/cirrus-logic/cs47l63/cs47l63_spec.h; other field encodings from DS1249F2.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/audio/cs47l63.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CS47L63_DEVID         0x00000000
#define CS47L63_DEVID_MASK    0x00FFFFFF
#define CS47L63_DEVID_CS47L63 0x00047A63

#define CS47L63_REVID               0x00000004
#define CS47L63_REVID_AREVID_MASK   0x000000F0 /* D7-D4: silicon revision */
#define CS47L63_REVID_MTLREVID_MASK 0x0000000F /* D3-D0: metal/mask revision */

/* OTPID, D3-D0: the OTP trim variant, which selects the trim block apply_trim() writes. */
#define CS47L63_OTPID      0x00000010
#define CS47L63_OTPID_MASK 0x0000000F

/* The trim registers sit behind this lock: the unlock pair written to both opens it. */
#define CS47L63_TEST_KEY_CTRL    0x00000030
#define CS47L63_USER_KEY_CTRL    0x00000034
#define CS47L63_KEY_UNLOCK_CODE0 0x00000055
#define CS47L63_KEY_UNLOCK_CODE1 0x000000AA
#define CS47L63_KEY_LOCK_CODE0   0x000000CC
#define CS47L63_KEY_LOCK_CODE1   0x00000033

/* Trim registers, written only by apply_trim(). */
#define CS47L63_MICBIAS_TST_CTRL1 0x00002420
#define CS47L63_MICBIAS_TST_CTRL4 0x00002424
#define CS47L63_HP_OCD_CTRL1      0x000024AC
#define CS47L63_HP_OCD_TEST1      0x000024B4
#define CS47L63_DAC_IF_CONTROL_1  0x00004D68
#define CS47L63_DAC_IF_TEST_1     0x00004D70

/* MICBIAS1B can be sourced from VDD_A, which needs no VDD_LDO (DS1249F2 section 4.14.2). */
#define CS47L63_MICBIAS_CTRL5 0x00002418
/** MICB1B_SRC, D6: 0 = MICBIAS regulator, 1 = VDD_A. */
#define CS47L63_MICB1B_SRC    0x00000040
#define CS47L63_MICB1B_EN     0x00000010

/* GPIO1..4 carry ASP1 DOUT, DIN, BCLK and FSYNC, GPIO5..8 the same for ASP2.
 * CS47L63_GP_CTRL1_ASP_PAD (GPn_FN = 0, the ASP function) is the vendor's wisce_init.txt
 * value with GPn_DIR cleared.
 */
#define CS47L63_GPIO1_CTRL1      0x00000C08
#define CS47L63_GPIO2_CTRL1      0x00000C0C
#define CS47L63_GPIO3_CTRL1      0x00000C10
#define CS47L63_GPIO4_CTRL1      0x00000C14
#define CS47L63_GPIO_COUNT       12
#define CS47L63_GP_CTRL1_RSVD    0x1EF00800
#define CS47L63_GP_DIR_INPUT     0x80000000
#define CS47L63_GP_CTRL1_ASP_PAD 0x61000000

#define CS47L63_CLOCK32K           0x00001400
#define CS47L63_CLK_32K_EN         0x00000040
#define CS47L63_CLK_32K_SRC_MASK   0x00000003
#define CS47L63_CLK_32K_SRC_MCLK2  0x00000001
/** CLK_32K_SRC = 10: SYSCLK, divided down automatically. */
#define CS47L63_CLK_32K_SRC_SYSCLK 0x00000002

#define CS47L63_SYSTEM_CLOCK1      0x00001404
/** SYSCLK_FRAC, D15: 0 = SYSCLK is a multiple of 6.144 MHz, 1 = of 5.6448 MHz. */
#define CS47L63_SYSCLK_FRAC        0x00008000
#define CS47L63_SYSCLK_FREQ_MASK   0x00000700
#define CS47L63_SYSCLK_FREQ_SHIFT  8
#define CS47L63_SYSCLK_EN          0x00000040
#define CS47L63_SYSCLK_SRC_MASK    0x0000001F
/* SYSCLK_SRC = FLL1 at its own output frequency, not 0x04 (FLL1 x 2): this
 * driver derives SYSCLK_FREQ from the FLL output, and SYSCLK_FREQ must name the
 * rate the mux delivers (DS1249F2 table 4-48, p. 131).
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

/* SYSTEM_CLOCK2: SYSCLK_STS says whether SYSCLK runs; SYSCLK_EN only says it was requested. */
#define CS47L63_SYSTEM_CLOCK2         0x00001408
#define CS47L63_SYSCLK_FREQ_STS_MASK  0x00000700
#define CS47L63_SYSCLK_FREQ_STS_SHIFT 8
#define CS47L63_SYSCLK_STS            0x00000040
#define CS47L63_SYSCLK_SRC_STS_MASK   0x0000001F

/* SAMPLE_RATE1: one of four global rate slots; this driver uses only slot 1. */
#define CS47L63_SAMPLE_RATE1     0x00001420
#define CS47L63_SAMPLE_RATE_MASK 0x0000001F
/** Rate codes for SAMPLE_RATEn. 48 kHz is the code the vendor script states. */
#define CS47L63_SAMPLE_RATE_48K  0x03
#define CS47L63_SAMPLE_RATE_24K  0x02
#define CS47L63_SAMPLE_RATE_16K  0x12

/* FLL1. CONTROL5 and CONTROL6 are not written by this driver. */
#define CS47L63_FLL1_CONTROL1 0x00001C00
#define CS47L63_FLL1_CONTROL2 0x00001C04
#define CS47L63_FLL1_CONTROL3 0x00001C08
#define CS47L63_FLL1_CONTROL4 0x00001C0C

#define CS47L63_FLL1_CTRL_UPD 0x00000004
#define CS47L63_FLL1_HOLD     0x00000002
#define CS47L63_FLL1_EN       0x00000001

#define CS47L63_FLL1_LOCKDET_THR_MASK  0xF0000000
#define CS47L63_FLL1_LOCKDET_THR_SHIFT 28
/** Lock detector enable. With this clear, FLL1_LOCK_STS1 never rises (DS1249F2 4.10.7.8). */
#define CS47L63_FLL1_LOCKDET           0x08000000
#define CS47L63_FLL1_PHASEDET          0x00400000
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

/* One output channel, OUT1L; the register map has no OUT1R or OUT2. */
#define CS47L63_OUTPUT_ENABLE_1 0x00004804
#define CS47L63_OUTPUT_STATUS_1 0x00004808
#define CS47L63_OUT1L_VOLUME_1  0x00004818
#define CS47L63_OUT1L_EN        0x00000002
#define CS47L63_OUT1L_EN_STS    0x00000002
/** OUT_VU, D9: latches the volume and mute fields on write (DS1249F2 section 4.9.4). */
#define CS47L63_OUT_VU          0x00000200
#define CS47L63_OUT1L_MUTE      0x00000100
#define CS47L63_OUT1L_VOL_MASK  0x000000FF
#define CS47L63_OUT1L_VOL_0DB   0x80

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

/* ASP2 fields sit at ASP1's bit positions (DS1249F2 sections 4.7-4.8). */
/** ASP2_ENABLES1 (0x6080) and the rest of the port block, from ASP1's. */
#define CS47L63_ASP2_BLOCK_OFFS  0x00000080
/** ASP2TX1_INPUT1 (0x8300) and ASP2TX2_INPUT1, from the ASP1 transmit mixers. */
#define CS47L63_ASP2_TX_MIX_OFFS 0x00000100
/** Mixer source codes ASP2 RX1/RX2 (0x030/0x031), from ASP1 RX1/RX2. */
#define CS47L63_ASP2_RX_SRC_OFFS 0x010
/** GPIO5..8_CTRL1 (0x0C18..0x0C24), the ASP2 pads, from GPIO1..4_CTRL1. */
#define CS47L63_ASP2_PAD_OFFS    0x00000010

#define CS47L63_INPUT_CONTROL  0x00004000
#define CS47L63_IN2L_EN        0x00000008
#define CS47L63_IN2R_EN        0x00000004
#define CS47L63_IN1L_EN        0x00000002
#define CS47L63_IN1R_EN        0x00000001
#define CS47L63_INPUT_CONTROL3 0x00004014
/** Latches every input volume field written since the last strobe (DS1249F2 section 4.2.7). */
#define CS47L63_IN_VU          0x20000000
/** INnx_VOL code for 0 dB. IN1n_CONTROL2 and IN2n_CONTROL2 share the digital volume
 *  field: D23-D16, 0.5 dB per code (DS1249F2 table 4-5).
 */
#define CS47L63_IN_VOL_0DB     0x80

/* INPUT1_CONTROL1 resets to 0x00050020; bit 5 must stay set (DS1249F2 section 4.2.6),
 * so the register is only ever updated.
 */
#define CS47L63_INPUT1_CONTROL1 0x00004020
#define CS47L63_IN1_OSR_MASK    0x00070000
#define CS47L63_IN1_OSR_SHIFT   16
/** IN1_OSR in digital mode is the IN1_PDMCLK rate: 101 = 3.072 MHz. */
#define CS47L63_IN1_OSR_3M072   5
/** IN1_MODE: 0 selects the analog input, 1 the digital (PDM) one. */
#define CS47L63_IN1_MODE        0x00000001

/* INnx_HPF is D2 of IN1n/IN2n_CONTROL1; IN_HPF_CUT resets to 010, 10 Hz
 * (DS1249F2 table 4-3).
 */
#define CS47L63_IN1L_CONTROL1     0x00004024
#define CS47L63_IN1R_CONTROL1     0x00004044
#define CS47L63_IN_HPF            0x00000004
#define CS47L63_INPUT_HPF_CONTROL 0x00004244

/* IN1n_CONTROL2 has IN2n_CONTROL2's layout. */
#define CS47L63_IN1L_CONTROL2    0x00004028
#define CS47L63_IN1R_CONTROL2    0x00004048
#define CS47L63_IN1_MUTE         0x10000000
#define CS47L63_IN1_VOL_MASK     0x00FF0000
#define CS47L63_IN1_VOL_SHIFT    16
/** Analog PGA gain, D7-D1; not written in digital mode. */
#define CS47L63_IN1_PGA_VOL_MASK 0x000000FE

#define CS47L63_INPUT2_CONTROL1 0x00004060
#define CS47L63_IN2_OSR_MASK    0x00070000
#define CS47L63_IN2_OSR_SHIFT   16
/** IN2_OSR = 101: analog high-performance, standard or low-power mode (DS1249F2 table 4-3). */
#define CS47L63_IN2_OSR_3M072   5
/** IN2_MODE: 0 selects the analog input, 1 the digital (PDM) one. */
#define CS47L63_IN2_MODE        0x00000001

#define CS47L63_IN2L_CONTROL1        0x00004064
#define CS47L63_IN2L_CONTROL2        0x00004068
#define CS47L63_IN2R_CONTROL1        0x00004084
#define CS47L63_IN2R_CONTROL2        0x00004088
#define CS47L63_IN2_SRC_MASK         0x30000000
#define CS47L63_IN2_SRC_SHIFT        28
#define CS47L63_IN2_SRC_SINGLE_ENDED 1
#define CS47L63_IN2_MUTE             0x10000000
#define CS47L63_IN2_VOL_MASK         0x00FF0000
#define CS47L63_IN2_VOL_SHIFT        16
#define CS47L63_IN2_PGA_VOL_MASK     0x000000FE
#define CS47L63_IN2_PGA_VOL_SHIFT    1
#define CS47L63_IN2_PGA_VOL_0DB      0x40

/* The two ASP1 transmit mixers, laid out like the OUT1L mixer slots below. */
#define CS47L63_ASP1TX1_INPUT1 0x00008200
#define CS47L63_ASP1TX2_INPUT1 0x00008210

/* The OUT1L mixer's first two input slots, each a source and its own mix volume. */
#define CS47L63_OUT1L_INPUT1       0x00008100
#define CS47L63_OUT1L_INPUT2       0x00008104
#define CS47L63_OUT1LMIX_VOL_MASK  0x00FE0000
#define CS47L63_OUT1LMIX_VOL_SHIFT 17
#define CS47L63_OUT1LMIX_VOL_0DB   0x40
#define CS47L63_OUT1L_SRC_MASK     0x000001FF
/** Mixer source codes. 0 is the "no source" encoding that mutes a slot. */
#define CS47L63_MIXER_SRC_NONE     0x000
#define CS47L63_MIXER_SRC_ASP1RX1  0x020
#define CS47L63_MIXER_SRC_ASP1RX2  0x021
#define CS47L63_MIXER_SRC_IN1L     0x010
#define CS47L63_MIXER_SRC_IN1R     0x011
#define CS47L63_MIXER_SRC_IN2L     0x012
#define CS47L63_MIXER_SRC_IN2R     0x013

/* Interrupt-1 edge flags; writing 1 clears a bit (DS1249F2 section 4.11). */
#define CS47L63_IRQ1_EINT_1 0x00018010
#define CS47L63_IRQ1_EINT_2 0x00018014
#define CS47L63_IRQ1_EINT_6 0x00018024
#define CS47L63_IRQ1_STS_6  0x000180A4
/* Interrupt-1 mask for the flags in IRQ1_EINT_1; a set bit masks. */
#define CS47L63_IRQ1_MASK_1 0x00018110

#define CS47L63_OUT1L_SC_EINT1       0x00020000 /* OUT1L short circuit */
#define CS47L63_SYSCLK_ERR_EINT1     0x00000400
#define CS47L63_SYSCLK_FAIL_EINT1    0x00000100
/** MICBIAS short circuit; also raised by disabling any MICBIAS1x output (DS1249F2 4.14.2). */
#define CS47L63_MICB_SC_EINT1        0x00000010
#define CS47L63_MICB_SC_MASK1        0x00000010
#define CS47L63_BOOT_DONE_EINT1      0x00000008
#define CS47L63_FLL1_REF_LOST_EINT1  0x00000100
#define CS47L63_FLL1_LOCK_FALL_EINT1 0x00000002
#define CS47L63_FLL1_LOCK_RISE_EINT1 0x00000001

/* IRQ1_STS_6 live status. FLL1_REF_LOST_STS1 means nothing unless FLL1_REFDET is set. */
#define CS47L63_FLL1_LOCK_STS1     0x00000001
#define CS47L63_FLL1_REF_LOST_STS1 0x00000100

#define CS47L63_ASP_COUNT 2

struct cs47l63_port {
	/** cirrus,asp: the serial port, 1 or 2. Selects the ASP register offsets. */
	uint8_t asp;
	/** The first of the two OUT1L mixer inputs this port feeds, 1 or 3. */
	uint8_t out_mix_input;
	/* OUT1L mixer sources, rewritten on every start_output. */
	uint16_t src1;
	uint16_t src2;
	/* Input terminal and transmit mixer sources, kept across a stop. */
	uint16_t in_src1;
	uint16_t in_src2;
	uint8_t in_terminal;
	/** dai_route once configure succeeds; BYPASS from the reset until then. */
	audio_route_t route;
	bool out_started;
	bool in_started;
};

/** The state of the part itself, shared by every port it runs. */
struct cs47l63_chip {
	const struct device *dev;
	/** BIT(asp - 1) for every port configured since the last reset. */
	uint8_t configured;
	/** MCLK1 and frame clock every configured port runs from. */
	uint32_t mclk_freq;
	uint32_t rate;
	/** Per port, index asp - 1: the port a device drives, NULL for none. */
	struct cs47l63_port *ports[CS47L63_ASP_COUNT];
	/** Number of ports with output started. The amplifier is on while this is not 0. */
	uint8_t out_started;
	bool output_running;
	/** Per cs47l63_input terminal: BIT(asp - 1) of every port with input
	 *  started on it. The terminal's front end is up while this is not 0.
	 */
	uint8_t in_users[2];
	/** Per port, index asp - 1: the error callback and the device it reports as. */
	audio_codec_error_callback_t fault_cb[CS47L63_ASP_COUNT];
	const struct device *fault_dev[CS47L63_ASP_COUNT];
	/* Sticky: OR'd by cs47l63_fault_raise(), zeroed only by cs47l63_fault_clear(). */
	uint32_t fault_errors;
	struct k_work_delayable start_check;
	struct k_mutex lock;
};

/** Per-device devicetree-derived configuration. A child sets only chip and port. */
struct cs47l63_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
	/** Optional: only present on a board that wires the codec's GPIO9. */
	struct gpio_dt_spec gpio9_gpio;
	/** cirrus,gpio-defaults: GPIOn_CTRL1 per GPIO from GPIO1; a reserved bit skips it. */
	const uint32_t *gpio_defaults;
	uint8_t gpio_defaults_len;
	struct cs47l63_chip *chip;
	struct cs47l63_port *port;
};

/** Per-instance runtime state: the part-wide levels, set through the parent. */
struct cs47l63_data {
	/* OUT1L_VOL code last requested, kept whether or not it is on the pins. */
	uint8_t vol_code;
	bool output_muted;
	/* Input level and mute last requested; a stop leaves them, a start restores them. */
	uint8_t in_vol_code;
	bool in_muted;
};

bool cs47l63_bus_is_ready(const struct device *dev);

/** Frame: address, a zero padding word, data; each word most significant byte first. */
int cs47l63_bus_write_reg(const struct device *dev, uint32_t addr, uint32_t val);

/** Only the low 31 bits of @p addr are addressable; the top bit marks a read. */
int cs47l63_bus_read_reg(const struct device *dev, uint32_t addr, uint32_t *val);

/** Registers mix fields owned by different units, so field writes must be masked. */
int cs47l63_bus_update_reg(const struct device *dev, uint32_t addr, uint32_t mask, uint32_t val);

int cs47l63_bus_poll_reg(const struct device *dev, uint32_t addr, uint32_t mask, uint32_t expected,
			 uint32_t interval_ms, uint32_t max_polls);

/** Reset, boot-done wait, DEVID check and trim. -ETIMEDOUT without boot-done, -ENODEV
 * for a DEVID other than @ref CS47L63_DEVID_CS47L63.
 */
int cs47l63_boot_bringup(const struct device *dev);

/** The FLL output DS1249F2 section 4.10.7.4 requires for SYSCLK in the 48 kHz family. */
#define CS47L63_FLL_FOUT_HZ 49152000U

struct cs47l63_fll_solution {
	uint8_t refclk_src;
	/** FLL1_REFCLK_DIV: the reference is divided by 2^refclk_div. */
	uint8_t refclk_div;
	uint8_t lockdet_thr;
	/** FLL1_HP: loop performance mode. */
	uint8_t hp;
	/** FLL1_N: the integer part of the multiplier. */
	uint16_t n;
	uint16_t fb_div;
	/** FLL1_LAMBDA and FLL1_THETA: the fractional part, as theta/lambda. */
	uint16_t lambda;
	uint16_t theta;
	/** The 16-bit loop-gain word written to FLL1_CONTROL4. */
	uint16_t gains;
	/** SYSCLK_FREQ code matching @ref CS47L63_FLL_FOUT_HZ. */
	uint8_t sysclk_freq;
};

/** Returns -ENOTSUP for any configuration it cannot reach exactly; nothing is approximated. */
int cs47l63_clock_solve(uint32_t fref_hz, uint32_t fout_hz, uint8_t refclk_src,
			struct cs47l63_fll_solution *out);

/** Covers the FLL lock time and the gap before the caller starts the I2S transfer. */
#define CS47L63_FLL_LOCK_SETTLE_MS 500

/** Does not wait for lock: the reference, MCLK1, runs only while an I2S transfer does. */
int cs47l63_clock_apply(const struct device *dev, const struct cs47l63_fll_solution *sol);

/** FLL1_LOCK_STS1 is live, not latched: read it after @ref CS47L63_FLL_LOCK_SETTLE_MS. */
int cs47l63_clock_locked(const struct device *dev, bool *locked);

struct cs47l63_dai_solution {
	/** SAMPLE_RATEn code for the requested frame rate. */
	uint8_t rate_code;
	/** ASPn_FMT code for the requested DAI type. */
	uint8_t fmt;
	/** Slot length in bit-clock cycles, and word length in bits. */
	uint8_t slot_width;
	uint8_t word_len;
	/** ASPn_ENABLES1 value: which receive slots carry audio. */
	uint32_t enables;
};

/** Returns -ENOTSUP for anything the part cannot express, including a codec clock master. */
int cs47l63_dai_solve(audio_dai_type_t dai_type, const struct i2s_config *i2s,
		      struct cs47l63_dai_solution *out);

/** Also muxes the port's four pads to their ASP function; they reset as GPIOs. */
int cs47l63_dai_apply(const struct device *dev, const struct cs47l63_dai_solution *sol);

/** Loss of SYSCLK or FLL1 lock; the class API has no clock-failure bit. */
#define CS47L63_ERROR_CLOCK BIT(5)

/** OUT1L_EN_STS still low when the deferred start check runs: a dead output stage. */
#define CS47L63_ERROR_OUTPUT BIT(6)

int cs47l63_fault_register_callback(const struct device *dev, audio_codec_error_callback_t cb);

/** Also clears the part's sticky flags, or the next poll re-reports the same fault. */
int cs47l63_fault_clear(const struct device *dev);

/** Polled from start_output and output volume or mute sets, not the interrupt line; each
 * new bit is reported once.
 */
int cs47l63_fault_check(const struct device *dev);

/** Reports a fault found outside the interrupt flags, once per bit like cs47l63_fault_check(). */
void cs47l63_fault_raise(const struct device *dev, uint32_t errors);

/** Called from configure(), so a playback-only device leaves the inputs disabled and muted. */
int cs47l63_in_init(const struct device *dev, struct cs47l63_port *port);

/** Does not wait for INPUT_STATUS. */
int cs47l63_in_start(const struct device *dev);

/** The front end comes down with the last port capturing from it; every step runs despite
 * a bus error.
 */
int cs47l63_in_stop(const struct device *dev);

/** A route set while the input is stopped is applied by the next start. */
int cs47l63_in_route_input(const struct device *dev, audio_channel_t channel, uint32_t input);

/** INnx_VOL envelope in whole dB; the field tops out at +31.5 dB (DS1249F2 table 4-5). */
#define CS47L63_IN_VOLUME_MIN_DB (-64)
#define CS47L63_IN_VOLUME_MAX_DB 31

/** Clamped to the envelope, never wrapped; cached while no port captures. */
int cs47l63_in_set_volume(const struct device *dev, int vol);

int cs47l63_in_set_mute(const struct device *dev, bool mute);

#define CS47L63_VOLUME_MIN_DB (-64)
#define CS47L63_VOLUME_MAX_DB 0

/** Leaves the amplifier disabled and muted: configure() does not start audio. */
int cs47l63_out_init(const struct device *dev, struct cs47l63_port *port);

/** Enables the amplifier muted; @ref cs47l63_out_confirm_start finishes the start. */
int cs47l63_out_start(const struct device *dev);

/** Called from the deferred check: waits for OUT1L_EN_STS, then puts the level on the pins. */
int cs47l63_out_confirm_start(const struct device *dev);

/** Mutes, then disables the amplifier. */
int cs47l63_out_stop(const struct device *dev);

/** Clamped, never wrapped; a level set while stopped is applied at the next start. */
int cs47l63_out_set_volume(const struct device *dev, int vol);

/** Unmuting restores the last level set, including one set while muted. */
int cs47l63_out_set_mute(const struct device *dev, bool mute);

/** ALL mixes both receive slots into OUT1L; FRONT_ or HEADPHONE_LEFT takes slot 1, _RIGHT
 * slot 2. Only the headphone terminal exists.
 */
int cs47l63_out_route_output(const struct device *dev, audio_channel_t channel, uint32_t output);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_H_ */

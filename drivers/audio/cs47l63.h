/*
 * SPDX-FileCopyrightText: Copyright (c) Cirrus Logic 2021
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63.h
 * @brief CS47L63 register map, instance data and internal interfaces (internal)
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
 * Every register on this part is 32 bits wide at a 32-bit address.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

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
 * sequence has to write - see apply_trim() in cs47l63.c.
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

/* Trim registers, written only by the OTPID-gated trim block, apply_trim().
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
 * cs47l63.c). Pairing SYSCLK_FREQ with 0x04 tells the part it is running
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

/* Instance configuration and runtime state. */

/** Per-instance devicetree-derived configuration. */
struct cs47l63_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
	/** Optional: only present on a board that wires the codec's GPIO9. */
	struct gpio_dt_spec gpio9_gpio;
};

/** Per-instance runtime state. */
struct cs47l63_data {
	/* Owned by the output path. vol_code is the OUT1L_VOL code last
	 * requested, held whether or not it is on the pins: output_running
	 * says whether the amplifier is up, output_muted whether the class
	 * API's mute property is holding it silent. Keeping the three apart is
	 * what lets a volume set while stopped survive to the next start, and a
	 * mute/unmute round trip give the level back instead of a default.
	 */
	uint8_t vol_code;
	bool output_running;
	bool output_muted;
	/* The two OUT1L mixer input slots, as mixer source codes. Re-written on
	 * every start_output so a route chosen while stopped takes effect.
	 */
	uint16_t src1;
	uint16_t src2;
	/* Owned by the input path, the same three-way split as the output side:
	 * in_running says whether the analog front end is enabled, in_src1 and
	 * in_src2 are the ASP1 transmit mixer sources, held across a stop so a
	 * route chosen while stopped survives to the next start.
	 */
	uint16_t in_src1;
	uint16_t in_src2;
	bool in_running;
	/* Owned by the fault unit. fault_errors is a bitmask of
	 * audio_codec_error_type plus CS47L63_ERROR_CLOCK and
	 * CS47L63_ERROR_OUTPUT, OR'd into by cs47l63_fault_check() and zeroed
	 * only by cs47l63_fault_clear().
	 * fault_cb is the callback registered through the class API, or NULL.
	 */
	audio_codec_error_callback_t fault_cb;
	uint32_t fault_errors;
	/* Owned by the class API glue. The deferred check armed by start_output - it
	 * reads the FLL lock and completes the amplifier enable, both of which
	 * need a clock that does not exist yet when start_output runs - and the
	 * back-pointer its handler needs to reach the device it belongs to.
	 */
	const struct device *dev;
	struct k_work_delayable start_check;
};

/* 32-bit register access over SPI. The only door to the chip: nothing else in
 * this driver issues an SPI transaction.
 */

/**
 * @brief Check whether the codec's SPI bus is ready.
 *
 * @param dev Codec device
 * @return true when the bus is ready to transfer
 */
bool cs47l63_bus_is_ready(const struct device *dev);

/**
 * @brief Write one 32-bit register.
 *
 * On the wire: four address bytes, most significant first, then four data
 * bytes in the same order.
 *
 * @param dev  Codec device
 * @param addr Register address
 * @param val  Value to write
 * @return 0 on success, the SPI transaction's negative errno on failure
 */
int cs47l63_bus_write_reg(const struct device *dev, uint32_t addr, uint32_t val);

/**
 * @brief Read one 32-bit register.
 *
 * On the wire: four address bytes with the top bit of the first set to mark a
 * read, then four padding bytes the part uses to turn the bus around, then the
 * four data bytes. The padding is consumed here and never reaches @p val -
 * delivering it instead yields data shifted by four bytes, which reads as
 * plausible garbage rather than as an error.
 *
 * @param dev  Codec device
 * @param addr Register address. Only the low 31 bits are addressable; the top
 *             bit is this driver's read marker and is never part of an address.
 * @param[out] val Value read. Left untouched when the transaction fails.
 * @return 0 on success, the SPI transaction's negative errno on failure
 */
int cs47l63_bus_read_reg(const struct device *dev, uint32_t addr, uint32_t *val);

/**
 * @brief Read-modify-write the @p mask bits of one register.
 *
 * Registers on this part mix fields owned by different units, so an unmasked
 * field write silently undoes a neighbour's setting.
 *
 * @param dev  Codec device
 * @param addr Register address
 * @param mask Bits this call owns; every other bit keeps its value
 * @param val  New value for the masked bits, already in position
 * @return 0 on success, negative errno on failure
 */
int cs47l63_bus_update_reg(const struct device *dev, uint32_t addr, uint32_t mask, uint32_t val);

/**
 * @brief Poll one register until every bit of @p mask reaches @p expected.
 *
 * Bounded by construction: it sleeps @p interval_ms between reads and gives up
 * after @p max_polls of them. Used for the three places this part makes the
 * driver wait on silicon - boot-done, FLL lock, and output-stage enable.
 *
 * @param dev         Codec device
 * @param addr        Register address
 * @param mask        Bits to watch
 * @param expected    Value those bits must reach, already in position
 * @param interval_ms Delay between reads, in milliseconds
 * @param max_polls   Number of reads to make before giving up
 * @return 0 once the bits match
 * @return -ETIMEDOUT when they never do
 * @return negative errno from the failing read
 */
int cs47l63_bus_poll_reg(const struct device *dev, uint32_t addr, uint32_t mask, uint32_t expected,
			 uint32_t interval_ms, uint32_t max_polls);

/* Reset, boot-done wait, identification and trim. */

/**
 * @brief Take the part from power-on to a register file that can be trusted.
 *
 * Drives reset, waits for the part to report boot-done, identifies it, and
 * applies the OTP-variant trim block. Nothing analog is enabled here and no
 * datapath register is written: after this returns the caller still has to
 * configure the clock, the serial port and the output.
 *
 * The three ways this can fail are told apart by errno, because at bring-up
 * they call for completely different next moves:
 *
 * @param dev Codec device
 * @retval 0 on success
 * @retval -ETIMEDOUT boot-done never asserted - the part is not running
 * @retval -ENODEV the device ID read back is not a plausible one - usually
 *         wiring, chip select, or SPI mode
 * @retval other negative errno propagated from the failing bus transaction
 */
int cs47l63_boot_bringup(const struct device *dev);

/* FLL1 solver and apply path. cs47l63_clock_solve() is pure computation - no
 * bus access and no @ref device pointer - so it is callable and testable with
 * nothing attached. cs47l63_clock_apply() writes what it produced.
 */

/**
 * @brief FLL1 output this driver locks to, in Hz.
 *
 * A multiple of 6.144 MHz, which is what SYSCLK_FRAC = 0 means, and the value
 * the board's 48 kHz family is built on.
 */
#define CS47L63_FLL_FOUT_HZ 49152000U

/**
 * @brief A complete FLL1 and SYSCLK configuration.
 *
 * cs47l63_clock_solve() never leaves this half-written: on success every field
 * is ready to write unmodified, on error none of them mean anything. There is
 * nothing here for the caller to fix up.
 */
struct cs47l63_fll_solution {
	/** FLL1_REFCLK_SRC code - which pin or net the reference arrives on. */
	uint8_t refclk_src;
	/** FLL1_REFCLK_DIV: the reference is divided by 2^refclk_div. */
	uint8_t refclk_div;
	/** FLL1_LOCKDET_THR. */
	uint8_t lockdet_thr;
	/** FLL1_HP: loop performance mode. */
	uint8_t hp;
	/** FLL1_N: the integer part of the multiplier. */
	uint16_t n;
	/** FLL1_FB_DIV: feedback divider. */
	uint16_t fb_div;
	/** FLL1_LAMBDA and FLL1_THETA: the fractional part, as theta/lambda. */
	uint16_t lambda;
	uint16_t theta;
	/** The 16-bit loop-gain word written to FLL1_CONTROL4. */
	uint16_t gains;
	/** SYSCLK_FREQ code matching @ref CS47L63_FLL_FOUT_HZ. */
	uint8_t sysclk_freq;
};

/**
 * @brief Solve FLL1 for a reference and an output frequency.
 *
 * An unreachable configuration is rejected, never approximated. Locking to a
 * neighbouring frequency yields audio that is slightly fast and that nobody
 * notices, which is worse than a refusal.
 *
 * @param fref_hz Reference frequency at the FLL input, in Hz
 * @param fout_hz Requested FLL output frequency, in Hz
 * @param refclk_src FLL1_REFCLK_SRC code for the pin the reference arrives on
 * @param[out] out Solved terms
 * @retval 0 on success
 * @retval -EINVAL if @p out is NULL or either frequency is zero
 * @retval -ENOTSUP if no set of terms reaches @p fout_hz exactly from
 *         @p fref_hz - including a reference the input divider cannot bring
 *         into range, and an output above the FLL's maximum
 */
int cs47l63_clock_solve(uint32_t fref_hz, uint32_t fout_hz, uint8_t refclk_src,
			struct cs47l63_fll_solution *out);

/**
 * @brief Milliseconds to allow the loop before its lock is worth reading.
 *
 * Generous: the loop needs single-digit milliseconds once its reference runs,
 * and this also has to cover the gap between the codec being told to play and
 * the SoC actually starting the I2S transfer that produces MCLK1.
 */
#define CS47L63_FLL_LOCK_SETTLE_MS 500

/**
 * @brief Write a solved configuration and bring SYSCLK up on it.
 *
 * The vendor order, which this follows exactly: hold the FLL, write the
 * control registers, select the reference source, enable, latch the update,
 * release the hold. The hold is asserted before any other FLL register is
 * written and released only after the update is latched; a bus error partway
 * through therefore leaves the FLL held rather than half-configured and
 * running.
 *
 * This does not wait for lock and does not fail when the loop is unlocked. The
 * reference is the SoC's I2S master clock, which runs only while an I2S
 * transfer does, so at configure time there is nothing for the loop to lock to
 * and the wait could only ever time out. SYSCLK is enabled on the running FLL
 * regardless; it starts for real when the reference appears.
 *
 * @param dev Codec device
 * @param sol Solution from @ref cs47l63_clock_solve
 * @retval 0 on success
 * @retval negative errno propagated from the failing bus transaction
 */
int cs47l63_clock_apply(const struct device *dev, const struct cs47l63_fll_solution *sol);

/**
 * @brief Read whether FLL1 currently reports lock.
 *
 * A live status bit, not a latch: it answers for the moment of the read, so it
 * is only meaningful once the reference has had time to run. See
 * @ref CS47L63_FLL_LOCK_SETTLE_MS.
 *
 * @param dev Codec device
 * @param[out] locked true when FLL1 is locked. Untouched on failure.
 * @retval 0 on success
 * @retval -EINVAL if @p locked is NULL
 * @retval other negative errno propagated from the failing bus transaction
 */
int cs47l63_clock_locked(const struct device *dev, bool *locked);

/* ASP1 serial-port solver and apply path. ASP1 is the port wired to the nRF's
 * I2S. The nRF is always the bit-clock and frame-sync master on this board, so
 * this port is always a slave.
 */

/**
 * @brief A complete ASP1 configuration.
 *
 * cs47l63_dai_solve() never leaves this half-written: on success every field is
 * ready to write unmodified, on error none of them mean anything.
 */
struct cs47l63_dai_solution {
	/** SAMPLE_RATEn code for the requested frame rate. */
	uint8_t rate_code;
	/** ASP1_FMT code for the requested DAI type. */
	uint8_t fmt;
	/** Slot length in bit-clock cycles, and word length in bits. */
	uint8_t slot_width;
	uint8_t word_len;
	/** ASP1_ENABLES1 value: which receive slots carry audio. */
	uint32_t enables;
};

/**
 * @brief Solve the ASP1 configuration for a class-API DAI request.
 *
 * A request for the codec to drive either clock is refused, not silently
 * corrected: two masters on one bus is a hardware fault, and the nRF is
 * already master.
 *
 * @param dai_type Digital interface type (audio_codec_cfg::dai_type)
 * @param i2s      I2S configuration (audio_codec_cfg::dai_cfg.i2s)
 * @param[out] out Solved configuration
 * @retval 0 on success
 * @retval -EINVAL if @p i2s or @p out is NULL
 * @retval -ENOTSUP if the part cannot express the request - an unsupported DAI
 *         type, a word size it has no encoding for, a frame rate with no
 *         rate-field code, a channel count other than one or two, or a request
 *         for the codec to be bit-clock or frame-sync master
 */
int cs47l63_dai_solve(audio_dai_type_t dai_type, const struct i2s_config *i2s,
		      struct cs47l63_dai_solution *out);

/**
 * @brief Write a solved ASP1 configuration.
 *
 * Also muxes the four GPIO pads that carry ASP1 to their serial-port function;
 * they come out of reset as GPIOs, so a port configured without this is
 * correct in every register and still silent on the wire.
 *
 * The global rate slot and the port's rate selector are written from the one
 * decision the solver made, never computed twice.
 *
 * @param dev Codec device
 * @param sol Solution from @ref cs47l63_dai_solve
 * @return 0 on success, negative errno propagated from the failing bus
 *         transaction
 */
int cs47l63_dai_apply(const struct device *dev, const struct cs47l63_dai_solution *sol);

/* Fault decode, sticky fault state and error callback. */

/**
 * @brief Loss of the system clock or of FLL1 lock.
 *
 * Driver-specific, above the class API's own @ref audio_codec_error_type bits.
 * The class enumerates over-current, over-temperature, under- and over-voltage
 * and DC, none of which names a clock failure - and a clock that stopped is the
 * single most common reason this part goes silent, so it is reported rather
 * than folded into a flag that means something else.
 *
 * This part has no thermal-shutdown status bit at all: nothing in the vendor
 * register map reports temperature, so @ref AUDIO_CODEC_ERROR_OVERTEMPERATURE
 * is never raised by this driver.
 */
#define CS47L63_ERROR_CLOCK BIT(5)

/**
 * @brief The headphone amplifier never reported itself enabled.
 *
 * Driver-specific for the same reason as @ref CS47L63_ERROR_CLOCK: the class
 * API has no bit for a stage that simply did not come up. Raised by the
 * deferred check that completes a start, so it means the enable was written,
 * the clock had time to arrive, and OUT1L_EN_STS still reads back low - a dead
 * output rather than a start that was merely early.
 */
#define CS47L63_ERROR_OUTPUT BIT(6)

/**
 * @brief Class API @c register_error_callback.
 *
 * @param dev Codec device
 * @param cb  Callback invoked with a bitmask of @ref audio_codec_error_type
 *            values plus the driver-specific @ref CS47L63_ERROR_CLOCK and
 *            @ref CS47L63_ERROR_OUTPUT, or NULL to unregister
 * @return 0
 */
int cs47l63_fault_register_callback(const struct device *dev, audio_codec_error_callback_t cb);

/**
 * @brief Class API @c clear_errors.
 *
 * Clears the chip's sticky interrupt flags as well as the driver's shadow. A
 * clear that only forgets locally leaves the flags set on the part, so the
 * next poll re-reports the same fault forever.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_fault_clear(const struct device *dev);

/**
 * @brief Read the fault flags, latch anything new, and report it once.
 *
 * The callback fires only for bits this call newly observed, so a fault that
 * is still asserted on the next poll is not reported a second time. With no
 * callback registered the bits are still latched and still cleared correctly,
 * so nothing accumulates on the part.
 *
 * The part does drive an interrupt line, but this driver polls instead: it is
 * called from the class operations that already touch the output stage -
 * @c start_output and an output volume or mute property set. A caller that
 * only configures and streams will not learn about a fault until one of those
 * happens.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_fault_check(const struct device *dev);

/**
 * @brief Latch a fault observed elsewhere in the driver and report it once.
 *
 * The reporting half of @ref cs47l63_fault_check, for a condition that is not
 * one of the part's interrupt flags - the FLL still being unlocked, or the
 * amplifier still not enabled, once the clock has had time to run. Same once-per-fault contract:
 * the callback fires only for bits that were not latched already.
 *
 * @param dev Codec device
 * @param errors Bitmask of @ref audio_codec_error_type values plus
 *               @ref CS47L63_ERROR_CLOCK and @ref CS47L63_ERROR_OUTPUT
 */
void cs47l63_fault_raise(const struct device *dev, uint32_t errors);

/* Analog line input and the ASP1 transmit mixers, the mirror image of the
 * output path. Only the IN2 pair is brought up: it is the one the boards wire
 * to a connector. IN1 is the PDM microphone pair, which needs a bias supply and
 * a clock this driver never configures, so asking for it is refused rather than
 * silently served from IN2.
 */

/** @brief Physical input terminal selected by @ref cs47l63_in_route_input. */
enum cs47l63_input {
	CS47L63_INPUT_LINE = 0, /**< IN2L/IN2R, the analog line pair. */
	CS47L63_INPUT_PDM = 1,  /**< IN1L/IN1R; not brought up by this driver. */
};

/**
 * @brief Put the input stage into its boot state: disabled and muted.
 *
 * Called from configure() alongside @ref cs47l63_out_init, so a device that
 * only ever plays back still leaves the ADC pins quiet instead of at whatever
 * the reset defaults are.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_in_init(const struct device *dev);

/**
 * @brief Class API @c start on @c AUDIO_DAI_DIR_RX: enable the line input.
 *
 * Configures the analog front end, unmutes it at 0 dB, points the two ASP1
 * transmit mixers at the routed sources and enables the transmit slots.
 *
 * Deliberately does not wait for INPUT_STATUS to report the inputs up: the
 * caller starts the I2S transfer that gives FLL1 its reference only after this
 * returns, so at this point SYSCLK does not run and the status can never
 * appear. Waiting here is what @ref cs47l63_out_start had to be split apart to
 * avoid; the input stage needs no second half, because nothing about it has to
 * be applied once it is running.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_in_start(const struct device *dev);

/**
 * @brief Class API @c stop on @c AUDIO_DAI_DIR_RX: mute and disable it again.
 *
 * Leaves the part in the state @ref cs47l63_in_init produced, including the
 * transmit mixer sources, so a stop followed by a start does not depend on
 * whatever the previous route was.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_in_stop(const struct device *dev);

/**
 * @brief Class API @c route_input: pick which pins feed the transmit slots.
 *
 * Takes effect immediately when the input is running and is remembered for the
 * next start when it is not, matching @ref cs47l63_out_route_output.
 *
 * @param dev     Codec device
 * @param channel AUDIO_CHANNEL_ALL (IN2L to transmit slot 1, IN2R to slot 2),
 *                AUDIO_CHANNEL_FRONT_LEFT (IN2L into both slots) or
 *                AUDIO_CHANNEL_FRONT_RIGHT (IN2R into both slots); any other
 *                value is rejected
 * @param input   A @ref cs47l63_input value. Only the line pair is supported
 * @return 0 on success, -ENOTSUP for an unsupported channel or terminal
 *         (nothing written), negative errno on a bus failure
 */
int cs47l63_in_route_input(const struct device *dev, audio_channel_t channel, uint32_t input);

/* Output mixer, headphone amplifier, volume and mute. This part has exactly
 * one output channel, OUT1L, driving a differential headphone load. There is no
 * OUT1R and no OUT2 anywhere in its register map, so "left" and "right" here
 * select which serial-port slot feeds that one channel, not which of two
 * amplifiers is addressed.
 */

/** @brief Physical output terminal selected by @ref cs47l63_out_route_output. */
enum cs47l63_output {
	CS47L63_OUTPUT_HP = 0,   /**< OUTP/OUTN, the differential headphone driver. */
	CS47L63_OUTPUT_LINE = 1, /**< Not present on this part. */
};

/** @brief Volume envelope, in dB, of the OUT1L digital volume control. */
#define CS47L63_VOLUME_MIN_DB (-64)
#define CS47L63_VOLUME_MAX_DB 0

/**
 * @brief Put the output stage into its boot state: muted, routed to nothing.
 *
 * Called from configure() after the clock and the serial port are up. Leaves
 * the amplifier disabled - configure() does not start audio.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_out_init(const struct device *dev);

/**
 * @brief Class API @c start_output: route the mixer and enable the amplifier.
 *
 * Does not repeat configure()'s work: it re-arms the output at the route
 * already chosen. Does not wait for the amplifier and does not put the level
 * on the pins - the amplifier cannot report enabled until SYSCLK runs, and
 * SYSCLK does not run until the caller starts the I2S transfer that gives FLL1
 * its reference, which happens after this returns. The start is completed by
 * @ref cs47l63_out_confirm_start; until then the output is enabled but still
 * muted and @c output_running is false.
 *
 * @param dev Codec device
 * @return 0 on success
 * @retval negative errno from the failing bus transaction
 */
int cs47l63_out_start(const struct device *dev);

/**
 * @brief Complete a start once the clock that the amplifier needs exists.
 *
 * The second half of @ref cs47l63_out_start, called from the deferred check
 * that start_output arms. Waits for OUT1L_EN_STS and, only if it appears,
 * marks the output running and puts the cached level on the pins - leaving the
 * driver in exactly the state a start that could have waited would have.
 *
 * A failure here is a real one: the enable was written, the clock has had time
 * to arrive, and the stage still does not report ready.
 *
 * @param dev Codec device
 * @return 0 on success
 * @retval -ETIMEDOUT if the amplifier never reports enabled
 * @retval other negative errno from the failing bus transaction
 */
int cs47l63_out_confirm_start(const struct device *dev);

/**
 * @brief Class API @c stop_output: mute, then bring the amplifier down.
 *
 * Waits for the shutdown to complete for the same reason the startup is
 * waited for: the next thing a caller does may be to cut the clock.
 *
 * @param dev Codec device
 * @return 0 on success, -ETIMEDOUT if the amplifier never reports disabled,
 *         other negative errno from the failing bus transaction
 */
int cs47l63_out_stop(const struct device *dev);

/**
 * @brief Apply an @ref AUDIO_PROPERTY_OUTPUT_VOLUME request in dB.
 *
 * A value outside @ref CS47L63_VOLUME_MIN_DB to @ref CS47L63_VOLUME_MAX_DB is
 * clamped to the envelope, never wrapped. A volume set while the output is
 * stopped is remembered and applied when it starts, so a caller that sets the
 * level before starting audio gets that level.
 *
 * @param dev Codec device
 * @param vol Requested volume in dB
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_out_set_volume(const struct device *dev, int vol);

/**
 * @brief Class API @c set_property @c AUDIO_PROPERTY_OUTPUT_MUTE.
 *
 * The mute state is held apart from the volume cache, so unmuting returns to
 * the level that was last set - before the mute or during it - rather than to
 * a default or the reset value.
 *
 * @param dev  Codec device
 * @param mute true to silence the output, false to restore the cached level
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_out_set_mute(const struct device *dev, bool mute);

/**
 * @brief Class API @c route_output: feed the headphone channel from ASP1.
 *
 * @param dev     Codec device
 * @param channel AUDIO_CHANNEL_ALL (both receive slots mixed into the one
 *                output channel), AUDIO_CHANNEL_FRONT_LEFT or
 *                AUDIO_CHANNEL_HEADPHONE_LEFT (slot 1 alone),
 *                AUDIO_CHANNEL_FRONT_RIGHT or AUDIO_CHANNEL_HEADPHONE_RIGHT
 *                (slot 2 alone); any other value is rejected
 * @param output  A @ref cs47l63_output value. Only the headphone terminal
 *                exists on this part; the line terminal is refused rather than
 *                quietly routed to the headphone
 * @return 0 on success, -ENOTSUP for an unsupported channel or terminal
 *         (nothing written), negative errno on a bus failure
 */
int cs47l63_out_route_output(const struct device *dev, audio_channel_t channel, uint32_t output);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_H_ */

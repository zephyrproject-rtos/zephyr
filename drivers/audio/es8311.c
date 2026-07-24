/*
 * Copyright (c) 2026 Hsiu-Chi Tsai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Everest Semiconductor ES8311 mono audio codec.
 *
 * Control interface: I2C (7-bit address 0x18 or 0x19, selected by the CE pin).
 * Audio interface: I2S / PCM, with the codec as the clock slave: the SoC drives
 * both the bit clock and the frame clock.
 *
 * The codec takes its internal master clock from the I2S bit clock (register
 * 0x01 bit 7), so no dedicated MCLK line is required. Register programming
 * follows the Everest ES8311 user guide (rev 1.11). The playback and capture
 * paths are validated on hardware.
 */

#define DT_DRV_COMPAT everest_es8311

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(es8311);

/* Register map (the subset this driver touches). */
#define ES8311_REG_RESET        0x00U /* reset / clock state machine */
#define ES8311_REG_CLK_MANAGER  0x01U /* master clock source + clock enables */
#define ES8311_REG_CLK_PRE      0x02U /* DIV_PRE, MULT_PRE */
#define ES8311_REG_ADC_OSR      0x03U /* ADC_FSMODE, ADC_OSR */
#define ES8311_REG_DAC_OSR      0x04U /* DAC_OSR */
#define ES8311_REG_CLK_DIV      0x05U /* DIV_CLKADC, DIV_CLKDAC */
#define ES8311_REG_CLK_BCLK     0x06U /* BCLK_CON, BCLK_INV, DIV_BCLK */
#define ES8311_REG_CLK_LRCK_H   0x07U /* tri-state controls, DIV_LRCK[11:8] */
#define ES8311_REG_CLK_LRCK_L   0x08U /* DIV_LRCK[7:0] */
#define ES8311_REG_SDP_IN       0x09U /* serial data port, DAC path (SDIN) */
#define ES8311_REG_SDP_OUT      0x0AU /* serial data port, ADC path (ASDOUT) */
#define ES8311_REG_PWRUP_AB     0x0BU /* PWRUP_A [7:3], PWRUP_B [3:1] */
#define ES8311_REG_PWRUP_C      0x0CU /* PWRUP_B [0], PWRUP_C [6:0] */
#define ES8311_REG_SYSTEM_0D    0x0DU /* analog power: bias, VREF, VMIDSEL */
#define ES8311_REG_SYSTEM_0E    0x0EU /* ADC power */
#define ES8311_REG_LOW_POWER    0x0FU /* LPDAC, LPPGA, LPPGAOUT, LPVCMMOD, LPADCVRP ... */
#define ES8311_REG_ANALOG_10    0x10U /* SYNCMODE, VMIDLOW, DAC_IBIAS_SW, IBIAS_SW, VX2OFF */
#define ES8311_REG_ANALOG_11    0x11U /* VSEL */
#define ES8311_REG_SYSTEM_12    0x12U /* DAC power */
#define ES8311_REG_SYSTEM_13    0x13U /* output select: line-out or headphone */
#define ES8311_REG_ADC_PGA      0x14U /* LINSEL microphone mux, PGA gain */
#define ES8311_REG_ADC_RAMP     0x15U /* ADC volume ramp rate */
#define ES8311_REG_ADC_SCALE    0x16U /* ADC polarity, ADC_SCALE */
#define ES8311_REG_ADC_VOLUME   0x17U /* ADC digital volume */
#define ES8311_REG_ADC_ALC      0x18U /* ALC_EN, ADC_AUTOMUTE_EN, ALC_WINSIZE */
#define ES8311_REG_ADC_ALC_LVL  0x19U /* ALC maximum and minimum gain */
#define ES8311_REG_ADC_AUTOMUTE 0x1AU /* automute noise gate */
#define ES8311_REG_ADC_HPF1     0x1BU /* ADC high-pass filter, stage 1 */
#define ES8311_REG_ADC_HPF2     0x1CU /* EQ bypass + ADC high-pass filter stage 2 */
#define ES8311_REG_DAC_MUTE     0x31U /* DAC mute */
#define ES8311_REG_DAC_VOLUME   0x32U /* DAC digital volume */
#define ES8311_REG_DAC_OFFSET   0x33U /* DAC DC offset */
#define ES8311_REG_DAC_DRC      0x34U /* DRC_EN, DRC_WINSIZE */
#define ES8311_REG_DAC_DRC_LVL  0x35U /* DRC maximum and minimum level */
#define ES8311_REG_DAC_RAMP_EQ  0x37U /* DAC_RAMPRATE [7:4], DAC equaliser bypass [3] */
#define ES8311_REG_GPIO         0x44U /* ADC2DAC_SEL (bit 7), ADCDAT_SEL [6:4] */
#define ES8311_REG_ADC_GP45     0x45U /* GP control */
#define ES8311_REG_INI          0xFAU /* I2C_RETIME, INI_REG (bit 0) */
#define ES8311_REG_CHIP_ID1     0xFDU /* chip id, high byte */
#define ES8311_REG_CHIP_ID2     0xFEU /* chip id, low byte */

#define ES8311_CHIP_ID1 0x83U
#define ES8311_CHIP_ID2 0x11U

/*
 * 0x00: CSM_ON. The datasheet's RST_DIG is "reset digital EXCEPT control port block",
 * and the control port block is where the registers live -- so no value of 0x00 ever
 * resets the register file. Writing CSM_ON powers the clock state machine up and clears
 * the reset bits; every other register keeps whatever a previous configure(), a previous
 * boot, or a previous firmware left in it.
 *
 * The only register-file reset on this part is 0xFA bit 0, and it is not usable as one:
 * see ES8311_INI_RELEASE.
 *
 * So this driver programs every register its behaviour depends on; es8311_known_state[]
 * below is what makes that true.
 */
#define ES8311_RESET_CSM_ON 0x80U

/*
 * THE REGISTERS THIS DRIVER DEPENDS ON AND MUST NORMALISE.
 *
 * A codec that is never reset inherits its whole register file from whatever ran last:
 * a vendor bootloader, another OS, an earlier firmware, or this driver's own previous
 * route. Two of these are not housekeeping, they are correctness:
 *
 *   0x18 ALC_EN. The datasheet is explicit, under the ADC volume register itself:
 *        "When ALC is on, ADC_VOLUME = MAXGAIN". A stuck ALC bit silently turns 0x17 --
 *        which this driver DOES write, and exposes as AUDIO_PROPERTY_INPUT_VOLUME --
 *        from a volume into a servo-loop ceiling. ALC is a user-flippable switch on
 *        Linux, so it is trivially reachable in the wild.
 *   0x34 DRC_EN. The exact mirror image on the playback side, and the datasheet makes
 *        the same claim under 0x32 (with an obvious copy-paste slip: it says
 *        "ADC_VOLUME" inside the DAC volume register).
 *
 * 0x0C is the other one that is not cosmetic: its power-on default is 0x20, i.e.
 * PWRUP_C = 32, which is outside the field's own documented 0..31 range, and PWRUP_A/B/C
 * time the analog power-up sequence in units that scale with LRCK.
 *
 * Values follow the vendor reference drivers, and Linux's own es8311.c (in tree since
 * v6.10), wherever those write these registers at all. Where none of them do -- 0x0F,
 * 0x19, 0x1A, 0x33, 0x35 -- the value is the power-on default, written anyway so that the
 * dependency is stated rather than inherited.
 *
 * Writing all of this costs nothing measurable on a settled part. Resetting the part to get a
 * known state instead costs about six seconds during which the ADC returns a noise floor of
 * exactly zero -- and a cold power-on does NOT pay that cost, so the reset is a bad trade, not a
 * wash. That is why this driver programs the registers rather than resetting. (The measurements
 * are in the design record.)
 */
#define ES8311_PWRUP_MIN     0x00U /* 0x0B: PWRUP_A = 0, PWRUP_B[3:1] = 0 */
#define ES8311_PWRUP_C_MIN   0x00U /* 0x0C: PWRUP_C = 0. Power-on default is 32. */
#define ES8311_LOW_POWER_OFF 0x00U /* 0x0F: every low-power-mode bit clear */
#define ES8311_ANALOG_10_VAL 0x1FU /* 0x10: differs from the 0x13 default in IBIAS_SW */
#define ES8311_ANALOG_11_VAL 0x7FU /* 0x11: VSEL. The datasheet says "Internal use". */
#define ES8311_ALC_OFF       0x00U /* 0x18: ALC_EN and ADC_AUTOMUTE_EN clear */
#define ES8311_ALC_LVL_DEF   0x00U /* 0x19 */
#define ES8311_AUTOMUTE_OFF  0x00U /* 0x1A */
#define ES8311_DAC_OFFSET_0  0x00U /* 0x33 */
#define ES8311_DRC_OFF       0x00U /* 0x34: DRC_EN clear */
#define ES8311_DRC_LVL_DEF   0x00U /* 0x35 */

/*
 * 0xFA bit 0, INI_REG: "reset registers to default except itself".
 *
 * It is a LEVEL, not a pulse: while set, the register file is HELD at its defaults -- every
 * write is silently discarded and every read returns 0x00. The vendor Linux driver asserts it
 * at shutdown deliberately, to hold the file at defaults across a reboot, so a part can be
 * handed over in that state: stone deaf, every register reading 0x00, yet a driver that only
 * checks return codes sees success. A driver that never writes 0xFA cannot recover such a chip,
 * so clearing it is the first register write this driver makes, in init() and configure().
 *
 * Being the first WRITE is not enough on its own: a held part also answers 0x00 to the identity
 * READ, so es8311_check_id() treats a chip id of 0x0000 as a symptom -- it releases 0xFA and
 * re-reads -- rather than failing identification before the release is ever reached.
 *
 * (Using 0xFA as a register-file reset is a different question, and the answer is no: it reverts
 * 0x0D/0x0E/0x12 to their powered-DOWN defaults, disturbing the analog and costing the same
 * multi-second recharge as the 0x00 block reset.)
 */
#define ES8311_INI_RELEASE 0x00U

/*
 * 0x01: MCLK_SEL (bit 7) takes the internal master clock from BCLK instead of the
 * MCLK pin. MCLK_ON (5) and BCLK_ON (4) enable the clock inputs, and bits [3:0]
 * gate the digital and analog clocks of each converter: CLKADC_ON (3),
 * CLKDAC_ON (2), ANACLKADC_ON (1), ANACLKDAC_ON (0). The unused converter's
 * clocks are gated off, as the ASoC driver does.
 */
#define ES8311_CLK_MGR_BOTH     0xBFU /* both converters clocked */
#define ES8311_CLK_MGR_PLAYBACK 0xB5U /* the ADC clocks gated off */
#define ES8311_CLK_MGR_CAPTURE  0xBAU /* the DAC clocks gated off */

/*
 * Clock tree.
 *
 * The internal master clock is BCLK scaled by DIV_PRE and MULT_PRE (0x02). A
 * standard 16-bit stereo frame carries 32 bit clocks, so BCLK is 32 * Fs, and
 * with DIV_PRE = 1 and MULT_PRE = x8 the internal master clock lands on
 * 256 * Fs. Every divider below is a ratio of that clock, never an absolute
 * frequency, so the same values are correct at every sample rate that preserves
 * the 256fs relationship. Only a 16-bit word does; 24-bit and 32-bit frames
 * carry 48 and 64 bit clocks, which would give 384fs and 512fs, and are
 * rejected in es8311_configure().
 *
 * The user guide requires the ratio of the internal ADC (and DAC) clock to LRCK
 * to be at least 256 and an integral multiple of 16 in single-speed mode. With
 * DIV_CLKADC = DIV_CLKDAC = 1 the ratio is exactly 256 = 16 * 16, and the
 * oversampling rates follow as ratio / 16 = 16.
 */
#define ES8311_CLK_PRE_DIV1_MULT8 0x18U /* 0x02: DIV_PRE = 1, MULT_PRE = x8 */
#define ES8311_ADC_OSR_SINGLE_16  0x10U /* 0x03: single speed, ADC_OSR = 16 */
#define ES8311_DAC_OSR_16         0x10U /* 0x04: DAC_OSR = 16 */
#define ES8311_CLK_DIV_ADC1_DAC1  0x00U /* 0x05: DIV_CLKADC = 1, DIV_CLKDAC = 1 */

/*
 * 0x06 / 0x07 / 0x08 hold the BCLK and LRCK dividers, which the user guide says
 * are inactive in slave mode: the codec detects the master clock to LRCK ratio
 * by itself. They are still written, because two neighbouring fields are not
 * dividers and do matter here: BCLK_CON in 0x06 has to stay clear so the codec
 * does not try to drive the bit clock, and the tri-state controls in 0x07 have
 * to stay clear so the codec actually drives ADCDAT. The divider fields are set
 * to the value the 256fs relationship implies, which would also be correct if
 * the codec were ever used as the clock master.
 */
#define ES8311_BCLK_SLAVE_DIV4 0x03U /* 0x06: BCLK_CON clear, DIV_BCLK = 4 */
#define ES8311_LRCK_DIV_H      0x00U /* 0x07: no tri-state, DIV_LRCK[11:8] = 0 */
#define ES8311_LRCK_DIV_L      0xFFU /* 0x08: DIV_LRCK[7:0], so LRCK = MCLK / 256 */

/*
 * 0x09 and 0x0A, the serial data ports feeding the DAC and coming out of the ADC: word
 * length in bits [4:2] (0b011 selects 16-bit), I2S format in bits [1:0], and a mute bit at
 * bit 6. Both are written whole, never read-modify-written.
 *
 * THE TWO REGISTERS ARE NOT SYMMETRIC AT BIT 7, and that bit is a board policy.
 *
 * 0x09 bit 7 is SDP_IN_SEL: which half of the stereo I2S frame the mono DAC takes its data
 * from. 0 is the left slot and is the reset default; 1 is the right. Mainline Linux exposes
 * it as a control called "Mono DAC Source", with the values Left and Right, and this driver
 * exposes it as the everest,mono-dac-source devicetree property (default "left"). It is
 * written on every configure(), not left to chance: on a part that is never reset, a previous
 * firmware that selected the other slot would otherwise still have it selected.
 *
 * 0x0A has no bit 7 at all: the datasheet's SDP_OUT table starts at bit 6, and neither Linux
 * nor the Espressif drivers name anything above it. Writing it clear is writing the reset
 * value of an undocumented bit, which is the most that can be said for it.
 */
#define ES8311_SDP_I2S_16BIT    0x0CU
#define ES8311_SDP_MUTE         0x40U
#define ES8311_SDP_IN_SEL_RIGHT 0x80U /* 0x09 bit 7: mono DAC takes the RIGHT I2S slot */

/*
 * Power sequencing.
 *
 * 0x0D is partly shared and partly per-converter: PDN_ANA (7), PDN_IBIASGEN (6),
 * PDN_VREF (2) and VMIDSEL [1:0] serve both paths and have to stay up while
 * either one runs, while PDN_ADCBIASGEN (5) and PDN_ADCVERFGEN (4) belong to the
 * ADC and PDN_DACVREFGEN (3) to the DAC. Only the unused converter's own
 * references are dropped.
 *
 * 0x0E powers the ADC down by setting PDN_PGA (6) and PDN_MOD (5) on top of the
 * running value. The vendor's full-idle value of 0xFF is deliberately NOT used
 * here: it also sets LPVREFBUF, putting the internal reference buffer that the
 * DAC output path shares into low-power mode, which would undermine a playback
 * stream that is still running.
 *
 * 0x12 = 0x02 is both PDN_DAC and the register's reset default: the DAC comes out
 * of reset powered down.
 */
#define ES8311_ANALOG_BOTH     0x01U /* 0x0D */
#define ES8311_ANALOG_PLAYBACK 0x31U /* 0x0D: the ADC's bias and reference down */
#define ES8311_ANALOG_CAPTURE  0x09U /* 0x0D: the DAC's reference down */
#define ES8311_ADC_PWR_ON      0x02U /* 0x0E: PGA and modulator powered */
#define ES8311_ADC_PWR_DOWN    0x62U /* 0x0E: PDN_PGA | PDN_MOD, shared refs kept */
#define ES8311_DAC_PWR_ON      0x00U /* 0x12 */
#define ES8311_DAC_PWR_DOWN    0x02U /* 0x12: PDN_DAC */
#define ES8311_OUT_HEADPHONE                                                                       \
	0x10U /* 0x13: HPSW=1, headphone driver (the everest,output-mode default) */
#define ES8311_OUT_LINEOUT 0x00U /* 0x13: HPSW=0, line-level differential drive */
/*
 * 0x37: DAC_RAMPRATE in [7:4] and DAC_EQBYPASS in bit 3. 0x48 is a 0.25 dB / 32-LRCK volume
 * soft ramp (rate 4) with the equaliser bypassed. The ramp is the point: the ADC volume is
 * already ramped at the same rate (0x15 = 0x40), and without this the DAC volume was the odd
 * one out -- every 0x32 change, and the level restored at unmute, landed as a hard step that
 * zippers. The Espressif reference sets 0x48 for the same reason; a driver that writes
 * volume straight to a live DAC (as apply_properties() does) relies on this ramp to make it
 * smooth. bit 3 keeps the equaliser bypassed, as before.
 */
#define ES8311_DAC_RAMP_EQ 0x48U /* 0x37 */

/*
 * 0x31: the DAC has two mute points, DSMMUTE at bit 6 and DEMMUTE at bit 5. The
 * driver asserts both, as do the vendor reference drivers.
 */
#define ES8311_DAC_MUTE_ON  0x60U
#define ES8311_DAC_MUTE_OFF 0x00U

/*
 * Settle delays (ms).
 *
 * These give the power-up writes just above them -- CSM_ON, and the analog power register --
 * a little time to take effect before the next writes lean on them. They do NOT wait on the
 * audio clock: the codec is configured over I2C with no BCLK running (the transport starts it
 * later, see the binding's clock-sequencing note), so nothing here is waiting for the clock
 * state machine to advance. Nor do they cover the ANALOG, which on this part is three 1 uF
 * filtering capacitors on VMID, ADCVREF and DACVREF, takes seconds to charge from cold, and is
 * not specified by the datasheet at all -- mainline Linux's es8311.c says the same in its own
 * reset function ("Specific delay is not documented"). Nothing here can or should wait that
 * out; the driver simply never causes it.
 */
#define ES8311_CSM_SETTLE_MS   10
#define ES8311_PWR_UP_DELAY_MS 10

/*
 * The DAC (0x32) and the ADC (0x17) digital volume registers share one linear
 * layout: 0x00 is the -95.5 dB minimum, 0xBF is 0 dB and 0xFF is the +32 dB
 * maximum, in 0.5 dB steps. The codec API expresses volume in whole dB, so the
 * same conversion serves both, and the reachable range is -95 dB to +32 dB.
 */
#define ES8311_VOL_DB_MAX   32
#define ES8311_VOL_DB_MIN   (-95)
#define ES8311_VOL_0DB_CODE 0xBFU

/*
 * Analog capture front end. The ES8311 has a single fully differential microphone input, so 0x14
 * bit 4 (LINSEL) selects the MIC1P/MIC1N pair, and bits [3:0] set the analog PGA gain: 3 dB per
 * code, 0..30 dB, so code = dB / 3 and 0x0A is the 30 dB maximum. everest,mic-pga-gain-db chooses
 * the gain (default 30 dB, the value both Espressif reference drivers use); ES8311_ADC_PGA_MIC1_0DB
 * is LINSEL set with the gain field zeroed, and es8311_config.pga_reg is that ORed with the code.
 *
 * When capture is not routed, 0x14 is cleared instead: LINSEL = 0 is "no input selection", which
 * disconnects the microphone from the PGA input mux rather than merely leaving it unpowered.
 */
#define ES8311_ADC_PGA_MIC1_0DB     0x10U /* 0x14: LINSEL = 1 (MIC1 diff), PGA gain code 0 */
#define ES8311_ADC_PGA_GAIN_STEP_DB 3U    /* 0x14 bits [3:0]: 3 dB per code, 0..30 dB */
#define ES8311_ADC_MIC_OFF          0x00U /* 0x14: no input selected */
#define ES8311_ADC_RAMP_RATE        0x40U /* 0x15: volume ramp rate */
#define ES8311_ADC_HPF1_VAL         0x0AU /* 0x1B */
#define ES8311_ADC_HPF2_DCBLOCK     0x6AU /* 0x1C: EQ bypass, cancels the DC offset */
#define ES8311_ADC_GP45_DEFAULT     0x00U /* 0x45 */

/*
 * 0x16: ADC_SYNC (bit 5) synchronises the filter counter with LRCK for a standard
 * audio clock, and ADC_SCALE [2:0] is the digital gain scale, whose reset default
 * of 4 is +24 dB. This is the running value.
 */
#define ES8311_ADC_SCALE_24DB 0x24U

/*
 * 0x44 is the GPIO and ADCDAT mux register, not an analog input select.
 * ADCDAT_SEL lives in bits [6:4]; leaving it at 0 puts plain ADC data on both
 * halves of ASDOUT, whereas 5 would inject a digital copy of the DAC output
 * into the ADC stream, which is a digital feedback path and not an analog
 * loopback. Bit 3, like bit 5 of 0x16, is not in the user guide's register
 * table; the vendor reference drivers set it and describe it as improving I2C
 * noise immunity.
 */
#define ES8311_GPIO_ADCDAT_ADC 0x08U

/*
 * The sample rates that keep the 256fs relationship. The user guide gives 8 kHz
 * to 48 kHz as the single-speed range; above that the ADC would need
 * double-speed mode, which this driver does not program.
 */
static const uint32_t es8311_rates[] = {
	8000U, 11025U, 12000U, 16000U, 22050U, 24000U, 32000U, 44100U, 48000U,
};

/*
 * The I2C bus and nothing else.
 *
 * There is deliberately no reset GPIO: the ES8311 has no reset pin. Its twenty pins are the
 * I2C three, MCLK, SCLK, LRCK, ASDOUT, DSDIN, MIC1P/N, OUTP/N, the four supply pairs, and
 * the three analog filtering-capacitor pins (VMID, ADCVREF, DACVREF). There is no signal to
 * wire a reset line to.
 *
 * And no power properties either. supply-gpios and vin-supply are both inherited from
 * power.yaml via i2c-device.yaml, and neither is honoured -- see the binding. A board whose
 * codec sits behind a switch or a regulator must bring that rail up itself, which is the
 * position every other audio codec in the tree takes.
 */
struct es8311_config {
	struct i2c_dt_spec bus;
	uint8_t sdp_in_sel; /* 0x09 bit 7: mono DAC source slot (0 = left, 0x80 = right) */
	uint8_t pga_reg;    /* 0x14: MIC1 differential plus the configured PGA gain */
	uint8_t out_reg;    /* 0x13: headphone or line-out drive mode */
};

struct es8311_data {
	struct k_mutex lock;
	uint8_t dac_volume_code; /* cached 0x32 */
	uint8_t adc_volume_code; /* cached 0x17 */
	/*
	 * output_mute is the AUDIO_PROPERTY_OUTPUT_MUTE the caller last requested;
	 * output_stopped is the start_output()/stop_output() lifecycle. They are
	 * SEPARATE (see the lifecycle comment above es8311_start_output): the DAC is
	 * audible only when the output has NOT been stopped AND the caller has not
	 * muted it -- unmuted exactly when playback && !output_mute && !output_stopped.
	 *
	 * output_stopped defaults TRUE: configure() sets up and powers the path but leaves the
	 * DAC muted, and start_output() is the first unmute -- configure() is not start_output().
	 * The i2s_codec sample does exactly that (configure(), then start_output()). A pending
	 * OUTPUT_MUTE survives a start/stop cycle (start_output() does not clear it and
	 * stop_output() does not forge it), and stop_output() leaves a sticky "stopped" state a
	 * later configure() will not quietly re-open.
	 *
	 * adc_mute is AUDIO_PROPERTY_INPUT_MUTE; the ADC has no start/stop op in the
	 * audio_codec API, so it has no lifecycle flag.
	 */
	bool output_mute;
	bool adc_mute;
	bool output_stopped;
	/*
	 * The route the last configure() programmed. Everything that touches a
	 * converter has to respect it: unmuting a serial port or a DAC that the
	 * current route deliberately powered down would put the microphone or the
	 * speaker back on the bus behind the caller's back.
	 */
	bool playback;
	bool capture;
};

static int es8311_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct es8311_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->bus, reg, val);
}

static int es8311_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct es8311_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->bus, reg, val);
}

/*
 * See the ES8311_PWRUP_MIN block above for why each of these is here. In address order,
 * because the analog power-up timers (0x0B, 0x0C) have to be in place before anything
 * clears the power-down bits in 0x0D and starts the sequence they time.
 */
static const struct es8311_reg_val {
	uint8_t reg;
	uint8_t val;
} es8311_known_state[] = {
	{ES8311_REG_PWRUP_AB, ES8311_PWRUP_MIN},
	{ES8311_REG_PWRUP_C, ES8311_PWRUP_C_MIN},
	{ES8311_REG_LOW_POWER, ES8311_LOW_POWER_OFF},
	{ES8311_REG_ANALOG_10, ES8311_ANALOG_10_VAL},
	{ES8311_REG_ANALOG_11, ES8311_ANALOG_11_VAL},
	{ES8311_REG_ADC_ALC, ES8311_ALC_OFF},
	{ES8311_REG_ADC_ALC_LVL, ES8311_ALC_LVL_DEF},
	{ES8311_REG_ADC_AUTOMUTE, ES8311_AUTOMUTE_OFF},
	{ES8311_REG_DAC_OFFSET, ES8311_DAC_OFFSET_0},
	{ES8311_REG_DAC_DRC, ES8311_DRC_OFF},
	{ES8311_REG_DAC_DRC_LVL, ES8311_DRC_LVL_DEF},
};

static int es8311_write_known_state(const struct device *dev)
{
	for (size_t i = 0U; i < ARRAY_SIZE(es8311_known_state); i++) {
		int ret =
			es8311_reg_write(dev, es8311_known_state[i].reg, es8311_known_state[i].val);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/*
 * BOTH HALVES OF THE PART, SILENT: no sound out, no microphone in, no converter powered.
 *
 * This is where the codec is left when there is nothing safe to say about it -- at the end
 * of init(), and on every path out of a configure() that failed. Both directions, because
 * muting SDP_OUT silences the DIGITAL output and does nothing whatever to the analog front
 * end: a part that was capturing still has its PGA powered and MIC1 wired into it, at
 * whatever gain the last firmware chose.
 *
 * EVERY WRITE IS ATTEMPTED. Giving up on the first failure is the one thing that must not
 * happen here: a single transient error on the serial-port mute would then skip the DAC
 * mute, the DAC power-down, the ADC power-down and the microphone disconnect -- and it would
 * skip them precisely when the bus is already misbehaving, which is the worst possible
 * moment to walk away from a live speaker. The first error is kept and returned; the part is
 * left as safe as the bus allowed, rather than as safe as its first hiccup allowed. And the
 * ORDER is a PRIORITY, because the writes that land on a bus failing partway through are a
 * PREFIX of the list: the most dangerous thing to leave live -- the speaker -- is muted first,
 * the microphone next, and the power-downs, which deepen the silence but are not what stops
 * sound the instant only one write gets through, come last.
 *
 * Releasing 0xFA is not part of this, and the two callers treat a failure of it differently,
 * on purpose. In configure() it is a genuine precondition: if the register file is held, none
 * of these can land at all, so attempting them would be theatre. In init() it is NOT -- by
 * then es8311_check_id() has read 0x8311 back, and a held part answers 0x00 to every read, so
 * INI_REG is already proven clear and the write is housekeeping. init() quiesces even when it
 * fails. See both call sites.
 */
static int es8311_quiesce(const struct device *dev)
{
	static const struct es8311_reg_val quiesce[] = {
		/* Speaker first, then microphone, then the DAC's own data port (redundant once the
		 * DAC is muted, but part of the known-silent state), then the power-downs. See the
		 * PREFIX argument above: this order is what makes a half-completed quiesce safe.
		 */
		{ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON},
		{ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE},
		{ES8311_REG_SDP_IN, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE},
		{ES8311_REG_SYSTEM_12, ES8311_DAC_PWR_DOWN},
		{ES8311_REG_SYSTEM_0E, ES8311_ADC_PWR_DOWN},
		{ES8311_REG_ADC_PGA, ES8311_ADC_MIC_OFF},
	};
	int first_err = 0;

	for (size_t i = 0U; i < ARRAY_SIZE(quiesce); i++) {
		int ret = es8311_reg_write(dev, quiesce[i].reg, quiesce[i].val);

		if (ret < 0 && first_err == 0) {
			first_err = ret;
		}
	}

	return first_err;
}

static bool es8311_rate_supported(uint32_t rate)
{
	for (size_t i = 0; i < ARRAY_SIZE(es8311_rates); i++) {
		if (es8311_rates[i] == rate) {
			return true;
		}
	}

	return false;
}

/*
 * The dB clamp below is the only clamp this needs, and these two assertions are why:
 * with 0DB_CODE = 0xBF the endpoints land exactly on the register's own limits, at
 * 0xBF + 32*2 = 0xFF and 0xBF - 95*2 = 0x01. A second clamp on the code would be
 * unreachable. If anyone retunes these three constants, this fails the build rather
 * than quietly relying on a runtime branch no test can reach.
 *
 * The (int) casts are load-bearing, not decoration. ES8311_VOL_0DB_CODE carries a U
 * suffix, so without them the whole expression is promoted to unsigned, and an
 * unsigned value is >= 0 by construction: the lower-bound assertion would be a
 * tautology that can never fail, whatever anyone sets ES8311_VOL_DB_MIN to.
 */
BUILD_ASSERT((int)ES8311_VOL_0DB_CODE + (ES8311_VOL_DB_MAX * 2) <= 0xFF,
	     "the maximum dB level must not overflow the volume register");
BUILD_ASSERT((int)ES8311_VOL_0DB_CODE + (ES8311_VOL_DB_MIN * 2) >= 0,
	     "the minimum dB level must not underflow the volume register");

/* Convert a dB volume level to a digital volume register code. */
static uint8_t es8311_db_to_code(int db)
{
	if (db > ES8311_VOL_DB_MAX) {
		db = ES8311_VOL_DB_MAX;
	} else if (db < ES8311_VOL_DB_MIN) {
		db = ES8311_VOL_DB_MIN;
	}

	return (uint8_t)((int)ES8311_VOL_0DB_CODE + (db * 2));
}

static int es8311_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct es8311_data *data = dev->data;
	const struct es8311_config *dcfg = dev->config;
	uint32_t rate;
	uint8_t clk_mgr;
	uint8_t analog;
	uint8_t sdp_in;
	uint8_t sdp_out;
	bool playback = false;
	bool capture = false;
	int ret = 0;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_INF("Unsupported DAI type %d", cfg->dai_type);
		return -ENOTSUP;
	}

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK:
		playback = true;
		break;
	case AUDIO_ROUTE_CAPTURE:
		capture = true;
		break;
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		playback = true;
		capture = true;
		break;
	default:
		LOG_INF("Unsupported route %u (playback/capture only)", cfg->dai_route);
		return -ENOTSUP;
	}

	/*
	 * Only the standard (Philips) I2S format with the default clock polarity
	 * and MSB-first ordering is supported: the serial data ports are
	 * programmed for exactly that.
	 */
	if ((cfg->dai_cfg.i2s.format & I2S_FMT_DATA_FORMAT_MASK) != I2S_FMT_DATA_FORMAT_I2S) {
		LOG_INF("Unsupported I2S data format 0x%x (only standard I2S)",
			cfg->dai_cfg.i2s.format & I2S_FMT_DATA_FORMAT_MASK);
		return -ENOTSUP;
	}

	if ((cfg->dai_cfg.i2s.format & I2S_FMT_DATA_ORDER_LSB) != 0U) {
		LOG_INF("LSB-first data ordering not supported");
		return -ENOTSUP;
	}

	if ((cfg->dai_cfg.i2s.format & I2S_FMT_CLK_FORMAT_MASK) != I2S_FMT_CLK_NF_NB) {
		LOG_INF("Unsupported I2S clock format 0x%x (only NF_NB)",
			cfg->dai_cfg.i2s.format & I2S_FMT_CLK_FORMAT_MASK);
		return -ENOTSUP;
	}

	/*
	 * The master clock is derived from BCLK, so the frame has to carry 32 bit
	 * clocks for the clock tree to land on 256fs. Only a 16-bit word does.
	 */
	if (cfg->dai_cfg.i2s.word_size != AUDIO_PCM_WIDTH_16_BITS) {
		LOG_INF("Unsupported word size %u (BCLK-derived MCLK needs a 16-bit word)",
			cfg->dai_cfg.i2s.word_size);
		return -ENOTSUP;
	}

	rate = cfg->dai_cfg.i2s.frame_clk_freq;
	if (!es8311_rate_supported(rate)) {
		LOG_INF("Unsupported sample rate %u", rate);
		return -ENOTSUP;
	}

	/*
	 * audio_codec_cfg.mclk_freq is the frequency of the clock fed to the
	 * codec's MCLK *input* pin. This driver never uses that pin: it takes the
	 * master clock from BCLK (register 0x01 bit 7), so there is no MCLK input
	 * to describe and zero is the only meaningful value. Accepting the
	 * internal 256fs figure here would be reporting a clock that is not on any
	 * pin, and accepting an arbitrary value would silently ignore it.
	 */
	/*
	 * The one I2S option this codec cannot survive.
	 *
	 * I2S_OPT_BIT_CLK_GATED asks the controller to run the bit clock "when sending data
	 * only". On a codec that takes its data clock from BCLK and nothing else, that is a
	 * bit-rate choice. On this one it is not: the ES8311 derives its entire internal
	 * master clock from BCLK, so a gated bit clock stops the codec's clock tree every
	 * time the TX queue runs dry -- including the DAC modulator, which is left frozen on
	 * its last sample, presenting a DC level to the amplifier. That is precisely the
	 * hazard init() exists to clean up after, and it would be re-created on every
	 * underrun, at a moment this driver is not told about and cannot quiesce for.
	 *
	 * Rejected rather than accepted and ignored.
	 *
	 * I2S_OPT_LOOPBACK and I2S_OPT_PINGPONG describe the controller's own RX/TX wiring
	 * and its DMA buffering. Neither reaches the codec.
	 */
	if ((cfg->dai_cfg.i2s.options & I2S_OPT_BIT_CLK_GATED) != 0U) {
		LOG_INF("I2S_OPT_BIT_CLK_GATED is not supported: this codec's master clock is "
			"derived from BCLK, so a gated bit clock stops the whole codec");
		return -ENOTSUP;
	}

	/*
	 * This codec is always the clock TARGET: it never drives BCLK or LRCK (0x06 BCLK_CON is
	 * cleared, the serial port is a slave, and the internal master clock is derived from the
	 * BCLK the I2S controller supplies). So its codec-local role must be TARGET on BOTH clocks.
	 *
	 * The I2S_OPT_*_CLK_{CONTROLLER,TARGET} flags are ENDPOINT-RELATIVE: i2s.h documents them
	 * as "the I2S driver is bit/frame clock controller/target", and "the I2S driver" is
	 * whichever endpoint the config is handed to. samples/drivers/i2s/i2s_codec proves it by
	 * building TWO configs with OPPOSITE role bits -- when the SoC supplies the clock, the host
	 * config is CONTROLLER|CONTROLLER and the codec config passed to audio_codec_configure() is
	 * TARGET|TARGET. In-tree codecs read it the same way (wm8904: FRAME_CLK_TARGET set means
	 * the codec receives BCLK/LRCK). So for this part the correct config is TARGET on both
	 * clocks.
	 *
	 * Accept only TARGET|TARGET. CONTROLLER on either clock would ask this codec to drive a
	 * clock it cannot, leaving the link driven by nobody while configure() returned 0.
	 */
	if ((cfg->dai_cfg.i2s.options & (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) !=
	    (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) {
		LOG_INF("this codec is always the clock target: the bit- and frame-clock roles "
			"must "
			"both be TARGET (the I2S controller drives BCLK and LRCK)");
		return -ENOTSUP;
	}

	if (cfg->mclk_freq != 0U) {
		LOG_INF("mclk_freq must be 0: the master clock comes from BCLK and the "
			"MCLK input is unused");
		return -ENOTSUP;
	}

	LOG_DBG("Configure: rate=%u", rate);

	k_mutex_lock(&data->lock, K_FOREVER);

	/*
	 * From here until the new route is committed, this device has no route. An I2C
	 * failure part-way through the sequence below leaves the hardware half
	 * reprogrammed -- some clocks gated, a converter powered down, an analog
	 * reference moved -- and the old cached route would then be a description of a
	 * chip that no longer exists, which start_output(), stop_output() and
	 * apply_properties() all steer by.
	 *
	 * Clearing it is necessary and it is not sufficient: forgetting a converter is not
	 * the same as stopping one. The error path at the bottom of this function is what
	 * makes it safe rather than merely undescribed.
	 */
	data->playback = false;
	data->capture = false;
	/*
	 * And the output goes STOPPED. configure() is not start_output(): it sets up and powers the
	 * path but leaves the DAC muted, whatever the lifecycle was before -- so "start_output() is
	 * the first unmute" holds on a RECONFIGURE too, not only a fresh init. It also stops a
	 * configure() that FAILED while the output was started from letting the next successful
	 * configure() self-unmute the DAC before the caller has started the output again.
	 * configure() mutes, powers down and reclocks the whole path regardless, so there is no
	 * live-reconfigure continuity to preserve by staying started.
	 */
	data->output_stopped = true;

	/*
	 * Everything route-dependent is decided here, under the lock. Sampling the
	 * cached mute state before taking the mutex would let another thread set an
	 * input mute that this call then overwrites, leaving the cache muted and the
	 * microphone live.
	 *
	 * A configure() never starts from a clean chip, because this driver never resets
	 * the part -- resetting it would cost seconds of settling on the capture path, and
	 * it buys nothing that es8311_write_known_state() does not buy for free. So a
	 * converter a previous route powered up is still up. Every route therefore programs
	 * BOTH directions, powering down the one it does not use rather than leaving it as
	 * it found it.
	 */
	if (playback && capture) {
		clk_mgr = ES8311_CLK_MGR_BOTH;
		analog = ES8311_ANALOG_BOTH;
	} else if (playback) {
		clk_mgr = ES8311_CLK_MGR_PLAYBACK;
		analog = ES8311_ANALOG_PLAYBACK;
	} else {
		clk_mgr = ES8311_CLK_MGR_CAPTURE;
		analog = ES8311_ANALOG_CAPTURE;
	}

	sdp_in = ES8311_SDP_I2S_16BIT | dcfg->sdp_in_sel | (playback ? 0U : ES8311_SDP_MUTE);
	sdp_out = ES8311_SDP_I2S_16BIT | ((capture && !data->adc_mute) ? 0U : ES8311_SDP_MUTE);

	/*
	 * FIRST, unhold the register file.
	 *
	 * If 0xFA bit 0 is set -- and a previous firmware may well have left it set; the
	 * vendor Linux driver does so deliberately at shutdown -- then every write below
	 * this line is silently discarded and every read returns 0x00, while this function
	 * returns 0. Nothing else can be done until it is cleared, so nothing else is
	 * attempted first.
	 */
	ret = es8311_reg_write(dev, ES8311_REG_INI, ES8311_INI_RELEASE);
	if (ret < 0) {
		goto end;
	}

	/*
	 * QUIESCE BEFORE RECLOCKING.
	 *
	 * The clock manager written below gates the clocks of whichever converter the
	 * new route does not carry. Gating the clock of a DAC that is still powered and
	 * still unmuted does not silence it: it freezes its modulator on the last
	 * sample, which is a DC step into the amplifier -- a pop, and on a switch away
	 * from playback the last thing the user hears. The dividers that follow move the
	 * whole clock tree under a converter that is still running.
	 *
	 * So mute both serial ports and the DAC, and power the DAC down, BEFORE
	 * anything touches the clocks. Then the tree can be reprogrammed into silence,
	 * and only what the new route carries comes back up, in order, at the end. This
	 * is what every codec datasheet means by a mute-before-reclock sequence.
	 */
	ret = es8311_reg_write(dev, ES8311_REG_SDP_IN, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_12, ES8311_DAC_PWR_DOWN);
	if (ret < 0) {
		goto end;
	}

	/*
	 * The ADC is NOT powered down here, only muted at its serial port above. It drives no
	 * speaker, so it cannot pop -- and taking its analog down and back up on every configure()
	 * would restart a settling time measured in SECONDS, not milliseconds.
	 *
	 * WHY it costs seconds is MEASURED but not explained: a cold power-on has the ADC alive at
	 * the earliest moment it can be sampled, yet the same part reads a floor of zero for ~2 s
	 * after a register reset. The driver acts on the measurement; the mechanism is not claimed.
	 *
	 * The route logic below still powers the ADC down when the new route does not carry
	 * capture, which is the case that matters.
	 */

	/* Power the clock state machine up. This resets no register: see 0x00 above. */
	ret = es8311_reg_write(dev, ES8311_REG_RESET, ES8311_RESET_CSM_ON);
	if (ret < 0) {
		goto end;
	}
	k_msleep(ES8311_CSM_SETTLE_MS);

	/*
	 * Now put every register this driver depends on into a known state, before the
	 * clock tree below and before anything clears the power-down bits in 0x0D. The
	 * codec is never reset, so without this the driver would be inheriting an ALC, a
	 * DRC, a low-power mode or a set of analog power-up timers from whatever ran last
	 * -- and two of those quietly redefine registers it does write.
	 */
	ret = es8311_write_known_state(dev);
	if (ret < 0) {
		goto end;
	}

	ret = es8311_reg_write(dev, ES8311_REG_CLK_MANAGER, clk_mgr);
	if (ret < 0) {
		goto end;
	}

	ret = es8311_reg_write(dev, ES8311_REG_CLK_PRE, ES8311_CLK_PRE_DIV1_MULT8);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_ADC_OSR, ES8311_ADC_OSR_SINGLE_16);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_DAC_OSR, ES8311_DAC_OSR_16);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_CLK_DIV, ES8311_CLK_DIV_ADC1_DAC1);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_CLK_BCLK, ES8311_BCLK_SLAVE_DIV4);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_CLK_LRCK_H, ES8311_LRCK_DIV_H);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_CLK_LRCK_L, ES8311_LRCK_DIV_L);
	if (ret < 0) {
		goto end;
	}

	/*
	 * The serial ports are NOT written here. They were muted above, and they stay muted
	 * until the very end of this function: see the commit block. Nothing between here
	 * and there is allowed to let a sample move in either direction.
	 */

	/*
	 * The bias, the mid-rail and the shared reference stay up for whichever
	 * converter runs; only the unused one's own references are dropped.
	 */
	ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_0D, analog);
	if (ret < 0) {
		goto end;
	}
	k_msleep(ES8311_PWR_UP_DELAY_MS);

	if (playback) {
		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_12, ES8311_DAC_PWR_ON);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_13, dcfg->out_reg);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_DAC_VOLUME, data->dac_volume_code);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_DAC_RAMP_EQ, ES8311_DAC_RAMP_EQ);
		if (ret < 0) {
			goto end;
		}
	} else {
		/* Power the DAC down rather than leaving a previous route's DAC live. */
		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_12, ES8311_DAC_PWR_DOWN);
		if (ret < 0) {
			goto end;
		}
	}

	if (capture) {
		/*
		 * Capture is always on once configured: the codec has no separate
		 * capture enable, and the application simply reads the I2S receive
		 * stream. This mirrors the in-tree wm8904 and da7212 codecs.
		 */
		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_0E, ES8311_ADC_PWR_ON);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_ADC_PGA, dcfg->pga_reg);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_ADC_RAMP, ES8311_ADC_RAMP_RATE);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_ADC_SCALE, ES8311_ADC_SCALE_24DB);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_ADC_VOLUME, data->adc_volume_code);
		if (ret < 0) {
			goto end;
		}

		/* The high-pass filter cancels the ADC's digital DC offset. */
		ret = es8311_reg_write(dev, ES8311_REG_ADC_HPF1, ES8311_ADC_HPF1_VAL);
		if (ret < 0) {
			goto end;
		}
		ret = es8311_reg_write(dev, ES8311_REG_ADC_HPF2, ES8311_ADC_HPF2_DCBLOCK);
		if (ret < 0) {
			goto end;
		}
	} else {
		/*
		 * Power the ADC down and take the microphone off the input mux. A
		 * muted serial port would still leave the PGA and the modulator
		 * running on a live microphone, which is not what a caller asking for
		 * a playback-only route is entitled to assume.
		 */
		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_0E, ES8311_ADC_PWR_DOWN);
		if (ret < 0) {
			goto end;
		}

		ret = es8311_reg_write(dev, ES8311_REG_ADC_PGA, ES8311_ADC_MIC_OFF);
		if (ret < 0) {
			goto end;
		}
	}

	/*
	 * 0x44 and 0x45 belong to BOTH routes, so they are normalised on every configure().
	 *
	 * 0x44 bit 7 is ADC2DAC_SEL, which routes the ADC digitally into the DAC: a PLAYBACK
	 * control living in a register whose other field is about the ADC. A chip that comes up
	 * with bit 7 set (a vendor bootloader, an aborted loopback) would play the ADC instead
	 * of the caller's audio -- silence out of the speaker with every register reading back
	 * as intended -- so it must be cleared even on a playback-only route.
	 *
	 * 0x45 is chip-wide (FORCECSM, PULLUP_SE) and has nothing to do with the route either.
	 */
	ret = es8311_reg_write(dev, ES8311_REG_GPIO, ES8311_GPIO_ADCDAT_ADC);
	if (ret < 0) {
		goto end;
	}

	ret = es8311_reg_write(dev, ES8311_REG_ADC_GP45, ES8311_ADC_GP45_DEFAULT);
	if (ret < 0) {
		goto end;
	}

	/*
	 * COMMIT: the last writes, the only ones that can move a converter.
	 *
	 * configure() leaves the output stopped, so the speaker stays muted here -- start_output()
	 * is what unmutes it. The microphone is the only converter a configure() opens, so its
	 * serial port is written LAST, with nothing failable after it: on a persistent bus failure
	 * the microphone is never left open behind a write that failed after it. 0x44 (ADC2DAC_SEL)
	 * is normalised well above this, so a chip handed over with the ADC wired into the DAC
	 * cannot play its microphone into the speaker in the meantime.
	 *
	 * These are I2C writes and can fail. If one does, the error path below runs a best-effort
	 * quiesce; on a bus that has stopped taking writes it may get no further than the write
	 * that failed. "Silent" is the intent and the best effort, not a guarantee.
	 */
	/*
	 * configure() leaves the output STOPPED (set at the top of this function), so the DAC stays
	 * muted: start_output() is what unmutes the speaker, never configure(). The ONLY converter
	 * a configure() opens is the microphone, so the microphone's serial port is the LAST write,
	 * with nothing failable after it, and the DAC mute and the DAC's own serial port precede
	 * it.
	 */
	ret = es8311_reg_write(dev, ES8311_REG_SDP_IN, sdp_in);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
	if (ret < 0) {
		goto end;
	}
	/*
	 * The microphone port last. sdp_out carries the UNMUTE bit only when the route captures and
	 * the caller has not muted the input; otherwise it is muted, and a muting write opens
	 * nothing. Either way it is the last thing this function does, so a persistent bus failure
	 * -- which fails the error-path quiesce too -- can never leave the microphone open behind a
	 * write that failed after it.
	 */
	ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT, sdp_out);
	if (ret < 0) {
		goto end;
	}

	data->playback = playback;
	data->capture = capture;

end:
	/*
	 * A FAILED configure() AIMS THE CODEC AT SILENT, BEST-EFFORT -- not merely undescribed.
	 *
	 * Clearing the route cache above is necessary -- the hardware is half reprogrammed and
	 * the old route no longer describes it -- but on its own it is a fail-OPEN. It makes
	 * the driver forget a converter, not stop one.
	 *
	 * Consider a live capture route and a configure() whose first write fails. The
	 * hardware is untouched: the ADC is powered, the PGA is live and MIC1 is still wired
	 * into it. The route cache now says there is no capture, so apply_properties() will
	 * not go near the ADC; stop_output() only mutes the DAC; and the audio_codec API has
	 * no stop_input() at all. The microphone would be left running, with no call in the
	 * API able to switch it off, and nothing in the driver's own state admitting it exists.
	 *
	 * So every error path quiesces, with the lock still held. It is best-effort by
	 * construction (the bus is already failing, or we would not be here) and its own
	 * errors are logged and dropped: the caller is owed the error that actually broke the
	 * configure, not whatever the cleanup tripped over afterwards.
	 *
	 * The old route is deliberately NOT restored. It is not trustworthy any more, and
	 * putting a converter back up from a description the hardware has already left behind
	 * is a worse answer than silence.
	 */
	if (ret < 0) {
		/*
		 * Attempted, not guaranteed, and the log says so. es8311_quiesce() returning 0
		 * means every write was ACKed, not that it landed: if this configure() failed on
		 * its 0xFA release, the part may still be holding its register file down, in which
		 * case every quiesce write is ACKed and silently discarded. Without a read-back --
		 * which on that same failing bus may not work either -- "quiesced" cannot be
		 * claimed, only "quiesce attempted".
		 */
		(void)es8311_quiesce(dev);
		LOG_ERR("configure() I2C error: %d. A best-effort quiesce was attempted.", ret);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/*
 * THE OUTPUT LIFECYCLE MODEL, stated once, because it is a design choice a reviewer should
 * be able to weigh rather than reverse-engineer.
 *
 * Two pieces of state, deliberately SEPARATE:
 *
 *   output_mute    -- the AUDIO_PROPERTY_OUTPUT_MUTE the caller last asked for.
 *   output_stopped -- the start_output()/stop_output() lifecycle.
 *
 * The DESIRED DAC state is unmuted exactly when a playback route is configured AND the caller has
 * not muted it AND the output has not been stopped -- playback && !output_mute && !output_stopped.
 * The HARDWARE is known to match that only after a call that programs the mute register succeeds:
 * configure(), start_output(), stop_output(), or apply_properties(). set_property() alone only
 * updates the cache; a bare set_property(OUTPUT_MUTE, true) leaves a live speaker live until the
 * next such call. This is the audio_codec contract (set_property caches, apply_properties() lands
 * the cache "as atomic as possible"), not a gap.
 * start_output() clears output_stopped; stop_output() sets it. Neither touches output_mute, and
 * OUTPUT_MUTE never touches output_stopped -- so a caller can mute, start and stop in any order
 * and each control keeps its own meaning. Folding them into one "is the DAC muted" flag would let
 * start_output() silently clear a pending OUTPUT_MUTE and stop_output() silently forge one.
 *
 * output_stopped DEFAULTS true, so configure() sets up and powers the path but leaves the DAC
 * muted; start_output() is the first unmute. configure() is not start_output(). That is what
 * samples/drivers/i2s/i2s_codec does -- audio_codec_configure(), then audio_codec_start_output().
 * stop_output() sets output_stopped again, and it is sticky: a later configure() will NOT quietly
 * re-open a stopped output; the application starts it again. So configure() describes the codec,
 * start_output() opens it, and the start/stop-driven caller gets a mute and a lifecycle that do
 * not overwrite each other.
 *
 * The other in-tree codecs bracket both ends: wm8904 and da7212 make start_output()/stop_output()
 * empty no-ops and unmute in configure(); tas2563 keeps a separate `is_started` flag, unmutes only
 * in start_output(), and refuses to unmute from set_property() until started. This driver follows
 * the tas2563 shape.
 *
 * The BCLK caveat: configure() leaves the DAC muted, so it opens no window; start_output() unmutes,
 * and since this codec's clock IS BCLK, calling it before i2s_trigger(START) briefly unmutes the
 * DAC with no clock. The binding's "Clock sequencing" note recommends running the transport (silent
 * frames) first; the acute form -- a DAC frozen on a real sample after a warm reboot -- is what
 * init() quiesces.
 *
 * The two ops themselves are NOT symmetric. Unmuting is dangerous and is gated: it puts a
 * speaker back on the output, so it only happens for a route that carries playback and that the
 * caller has not muted. Muting is a SAFETY operation and is gated by nothing -- writing the mute
 * bit of a powered-down DAC is harmless, and gating the off switch on a route that a failed
 * configure() cleared would leave a caller reaching for silence with nothing. The one call whose
 * entire job is to make the speaker stop must always try.
 */
static void es8311_start_output(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->playback) {
		/*
		 * Nothing to start -- and marking the lifecycle started here would pre-arm the
		 * NEXT configure() to open a speaker the caller never asked to run. Leave it.
		 */
		LOG_WRN("start_output: no playback route configured");
		goto out;
	}

	if (!data->output_stopped) {
		/*
		 * Already started: a duplicate start_output() must not re-issue a dangerous unmute
		 * that could glitch on the bus and stop a running stream. tas2563 guards the same
		 * way.
		 */
		goto out;
	}

	/*
	 * start_output() ESTABLISHES the hardware state it claims, it does not only flip the flag:
	 * a stop_output() whose mute write glitched may have left the DAC live, so the muted branch
	 * writes the mute rather than trusting the register. Either write marks the lifecycle
	 * started only if it SUCCEEDS -- like tas2563, which sets is_started after the hardware
	 * write -- and best-effort re-mutes (a monotonic mute) on an error that might have landed.
	 * A failed start leaves the output STOPPED.
	 */
	if (data->output_mute) {
		/* Started, but the caller's OUTPUT_MUTE is authoritative: establish MUTED. */
		ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
	} else {
		ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF);
	}
	if (ret < 0) {
		(void)es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
		data->output_stopped = true;
	} else {
		data->output_stopped = false;
	}

out:
	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		LOG_ERR("start_output: failed to establish the output state (%d)", ret);
	}
}

/*
 * The off switch. It has to work on a bus that is only half working.
 *
 * This WRITES the register rather than updating it, and that is the whole point. A
 * read-modify-write reads first and returns on a failed read WITHOUT attempting the write,
 * so a bus that can still push two bytes but can no longer do a write-then-read with a
 * repeated start would leave the DAC exactly as loud as it was, while the caller got an
 * error for an operation that was never attempted.
 *
 * A full write is safe here because the driver owns the whole register: 0x31 carries only
 * DSMMUTE and DEMMUTE, and init() and configure() already write it whole.
 */
static void es8311_stop_output(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->output_stopped = true;
	/* The lifecycle stops; the OUTPUT_MUTE property is left exactly as the caller set it. */
	ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		LOG_ERR("stop_output: failed to mute (%d)", ret);
	}
}

static int es8311_set_property(const struct device *dev, audio_property_t property,
			       audio_channel_t channel, audio_property_value_t val)
{
	struct es8311_data *data = dev->data;
	int ret = 0;

	if (channel != AUDIO_CHANNEL_ALL && channel != AUDIO_CHANNEL_FRONT_LEFT &&
	    channel != AUDIO_CHANNEL_FRONT_RIGHT) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		data->dac_volume_code = es8311_db_to_code(val.vol);
		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		data->output_mute = val.mute;
		break;
	case AUDIO_PROPERTY_INPUT_VOLUME:
		data->adc_volume_code = es8311_db_to_code(val.vol);
		break;
	case AUDIO_PROPERTY_INPUT_MUTE:
		data->adc_mute = val.mute;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/*
 * THE PROPERTY TRANSACTION.
 *
 * set_property(OUTPUT_MUTE, true) then apply_properties() is the OTHER off switch the audio_codec
 * API documents, and it must not disagree with the direct one, stop_output(). The rule, the same
 * one configure()'s commit follows:
 *
 *   A MUTE MAY NEVER BE CANCELLED, SKIPPED OR UNDONE BY A WRITE THAT IS NOT A MUTE.
 *   AN UNMUTE MAY ONLY HAPPEN ONCE EVERYTHING THAT CAN FAIL HAS ALREADY SUCCEEDED.
 *   A FAILURE MAY LEAVE THINGS UNCHANGED. IT MAY NOT MAKE THEM WORSE.
 *
 * So, in three phases: every requested mute first (none skipped because another failed); then the
 * volumes, but not on a direction whose mute just failed (a volume is a GAIN and could turn a
 * still-live path UP) -- phase 2 is gated on the WHOLE call; then the unmutes, only if the whole
 * call is clean, so a failed mute in one direction cannot be followed by an unmute in the other.
 * The first error is returned.
 *
 * The lock is held across all three phases, not merely across the read of the cached state: taking
 * a snapshot and releasing would let stop_output() run in between, mute the DAC in hardware, and
 * then have that write overwritten by the already-sampled "unmuted" value from phase 3.
 *
 * Only the routed directions are touched -- applying a cached state on a route that powered its
 * converter down would unmute a dead path, or clear a mute configure() set; the properties stay
 * cached and land at the next configure() that routes them.
 *
 * "As atomic as possible", per the API. An errored unmute might still have LANDED (a NAK on a
 * completed write is indistinguishable here from one that never left), so this best-effort
 * RE-MUTES rather than reading back: a mute is monotonic, so re-writing it can only leave the path
 * same-or-safer, and if the errored unmute was the speaker, the microphone THIS call opened is
 * re-muted with it. Best-effort, not a guarantee -- the re-mute can fail on the same dead bus,
 * exactly as configure()'s error-path quiesce can; the caller still has stop_output().
 *
 * What it does NOT prevent: a path the caller chose to keep LIVE and gave a new volume reaches that
 * volume even if another write in the call then fails -- that is the caller's own request, which a
 * successful apply() would land too. Muting around every volume write to prevent it would fight the
 * soft ramp (0x37 for the DAC, 0x15 for the ADC) that exists to keep level changes smooth.
 *
 * Finally, phase 1 re-asserts the DAC mute whenever the output is STOPPED, so a stop_output() whose
 * mute write glitched on the bus is healed by the next apply_properties(). stop_output() (void, so
 * it cannot report a transport failure) stays the immediate switch; apply() is the best-effort
 * re-assert behind it.
 */
static int es8311_apply_properties(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int first_err = 0;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	/*
	 * 1. Every mute the state calls for. That is the requested OUTPUT_MUTE, but ALSO a
	 *    re-assert whenever the output is stopped: the invariant is that a stopped DAC is a
	 *    muted DAC, and enforcing it here is what lets apply_properties() heal a stop_output()
	 *    whose own mute write glitched on the bus (stop_output() returns void, so the caller
	 *    never saw the failure -- but the next apply() re-mutes). None is skipped because
	 *    another failed.
	 */
	if (data->playback && (data->output_mute || data->output_stopped)) {
		ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
		if (ret < 0) {
			LOG_ERR("Failed to mute the DAC (%d)", ret);
			first_err = (first_err == 0) ? ret : first_err;
		}
	}

	/* The ADC mutes at its serial data port, not through its volume. */
	if (data->capture && data->adc_mute) {
		ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT,
				       ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
		if (ret < 0) {
			LOG_ERR("Failed to mute the microphone (%d)", ret);
			first_err = (first_err == 0) ? ret : first_err;
		}
	}

	/*
	 * 2. The volumes -- but NOT once ANY requested mute has failed, in EITHER direction.
	 *
	 * A volume register is a GAIN, and the rule for a failed safety write is DO NOT MAKE
	 * IT WORSE. If a mute the caller asked for did not land, the path it was meant to
	 * silence is still live -- and this call must not then turn any converter UP, not even
	 * the OTHER one. A caller muting the microphone and, in the same batch, raising the
	 * speaker must not come out of a failed mic-mute with a louder speaker. Gating the
	 * whole of phase 2 on `first_err == 0` is what makes that hold across directions, not
	 * just within one: a single failed mute stops every gain write in the call.
	 *
	 * That does mean a caller asking to be both quieter and muted gets neither if the mute
	 * fails. Leaving a direction as it was is a defensible failure; turning something up is
	 * not. And the driver does not attenuate as a consolation prize -- writing a level the
	 * caller never asked for is not this function's decision to make. The caller has
	 * stop_output(), ungated and always attempted.
	 */
	if (first_err == 0 && data->playback) {
		ret = es8311_reg_write(dev, ES8311_REG_DAC_VOLUME, data->dac_volume_code);
		if (ret < 0) {
			LOG_ERR("Failed to set DAC volume 0x%02x (%d)", data->dac_volume_code, ret);
			first_err = ret;
		}
	}

	if (first_err == 0 && data->capture) {
		ret = es8311_reg_write(dev, ES8311_REG_ADC_VOLUME, data->adc_volume_code);
		if (ret < 0) {
			LOG_ERR("Failed to set ADC volume 0x%02x (%d)", data->adc_volume_code, ret);
			first_err = ret;
		}
	}

	/*
	 * 3. The unmutes, last of all -- and each one only if EVERYTHING above it succeeded.
	 *
	 * Not merely "only if this direction's own volume landed". A half-applied transaction
	 * is not a licence to perform its dangerous half: if the microphone's mute failed and
	 * the speaker's unmute is still carried out, the call has left the microphone open AND
	 * opened the speaker, on a board where the two are centimetres apart, while returning
	 * an error that says none of it can be trusted. Failing to make something safe must
	 * not be followed by making something else live.
	 *
	 * The gate is `first_err == 0`, re-tested at EACH unmute -- NOT a snapshot taken once
	 * before the block. The microphone unmute below can itself fail, and if it does, its
	 * error has to stop the speaker unmute too. A snapshot taken before either ran would
	 * still say "clean" and let the speaker open behind a microphone unmute that just
	 * failed, which is the exact hazard this ordering exists to prevent. The
	 * less-dangerous unmute (the microphone) is deliberately first, so that the more
	 * dangerous one (the speaker) is gated on it.
	 *
	 * So a single error anywhere in this call disarms every unmute after it. The properties
	 * stay cached, the caller gets the error, and a retry that succeeds applies them all.
	 */
	bool this_apply_opened_mic = false;

	if (first_err == 0 && data->capture && !data->adc_mute) {
		ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT);
		if (ret < 0) {
			LOG_ERR("Failed to unmute the microphone (%d)", ret);
			first_err = ret;
			/*
			 * The unmute may have LANDED even though it errored -- the I2C layer cannot
			 * tell. Best-effort re-close: a mute is monotonic, it can only leave the
			 * microphone the same or safer, never more open. See the comment below.
			 */
			(void)es8311_reg_write(dev, ES8311_REG_SDP_OUT,
					       ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
		} else {
			this_apply_opened_mic = true;
		}
	}

	/*
	 * The DAC last of the two: it is the one with a speaker on it. It also carries the
	 * lifecycle gate -- a stopped output stays muted here, exactly as it does in the
	 * configure() commit, so apply_properties() never re-opens a speaker the application
	 * stopped. output_mute and output_stopped are independent; either one keeps it shut.
	 */
	if (first_err == 0 && data->playback && !data->output_mute && !data->output_stopped) {
		ret = es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF);
		if (ret < 0) {
			LOG_ERR("Failed to unmute the DAC (%d)", ret);
			first_err = ret;
			/*
			 * Best-effort re-close the speaker, same monotonic-mute argument. And back
			 * out the microphone THIS call opened: a failed apply() AIMS at muted
			 * rather than half-open -- best-effort, since the re-mute can fail on the
			 * same bus -- and a retry re-applies both cleanly.
			 */
			(void)es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
			if (this_apply_opened_mic) {
				(void)es8311_reg_write(dev, ES8311_REG_SDP_OUT,
						       ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
			}
		}
	}

	k_mutex_unlock(&data->lock);

	return first_err;
}

/*
 * route_input() and route_output() are deliberately not implemented: the ES8311
 * has one differential microphone input and one output, so there is nothing to
 * multiplex.
 */
static DEVICE_API(audio_codec, es8311_api) = {
	.configure = es8311_configure,
	.start_output = es8311_start_output,
	.stop_output = es8311_stop_output,
	.set_property = es8311_set_property,
	.apply_properties = es8311_apply_properties,
};

static int es8311_read_id(const struct device *dev, uint8_t *id1, uint8_t *id2)
{
	int ret;

	ret = es8311_reg_read(dev, ES8311_REG_CHIP_ID1, id1);
	if (ret < 0) {
		LOG_ERR("Failed to read chip id1 (%d)", ret);
		return ret;
	}

	ret = es8311_reg_read(dev, ES8311_REG_CHIP_ID2, id2);
	if (ret < 0) {
		LOG_ERR("Failed to read chip id2 (%d)", ret);
		return ret;
	}

	return 0;
}

/*
 * Establish that the part at this address really is an ES8311 -- and, if it is one that
 * cannot currently say so, put it in a state where it can.
 *
 * This function writes a register, which a function called "check" would not normally do.
 * It has to. A part whose INI_REG (0xFA bit 0) is asserted holds its register file at the
 * defaults: it answers 0x00 to EVERY read, chip-id registers included, and discards every
 * write except one -- 0xFA itself. The vendor Linux driver asserts that bit at shutdown,
 * deliberately, so this is not a hypothetical state: it is the state a chip is routinely
 * handed over in.
 *
 * A driver that reads 0x0000 there, concludes "not an ES8311" and returns -ENODEV never
 * issues the one write that would have recovered the part. The chip is then unrecoverable
 * by that driver, permanently, across every reboot. So 0x0000 is not treated as an
 * identity, it is treated as a symptom, and the only cure is attempted exactly once.
 *
 * The cost of being wrong is one write of 0x00 to register 0xFA of a device the devicetree
 * already declares to be an ES8311. Anything that still fails to identify itself after that
 * is rejected below, exactly as before -- including anything that answers 0x0000 twice.
 */
static int es8311_check_id(const struct device *dev)
{
	uint8_t id1 = 0U;
	uint8_t id2 = 0U;
	int ret;

	ret = es8311_read_id(dev, &id1, &id2);
	if (ret < 0) {
		return ret;
	}

	if (id1 == 0x00U && id2 == 0x00U) {
		LOG_WRN("chip id reads 0x0000: the register file may be held by INI_REG. "
			"Releasing 0xFA and re-reading.");

		ret = es8311_reg_write(dev, ES8311_REG_INI, ES8311_INI_RELEASE);
		if (ret < 0) {
			LOG_ERR("Failed to release the register file (%d)", ret);
			return ret;
		}

		ret = es8311_read_id(dev, &id1, &id2);
		if (ret < 0) {
			return ret;
		}
	}

	/*
	 * Fatal, not a warning. A chip id check that has no effect is not a check: it
	 * leaves the device ready, and every later register write goes to whatever part
	 * is actually at this address. Reading 0x8311 back is the only evidence the
	 * driver has that it is talking to an ES8311 at all.
	 */
	if (id1 != ES8311_CHIP_ID1 || id2 != ES8311_CHIP_ID2) {
		LOG_ERR("Not an ES8311: chip id 0x%02x%02x (expected 0x%02x%02x)", id1, id2,
			ES8311_CHIP_ID1, ES8311_CHIP_ID2);
		return -ENODEV;
	}

	return 0;
}

static int es8311_init(const struct device *dev)
{
	const struct es8311_config *cfg = dev->config;
	struct es8311_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C controller not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	/* Both directions default to 0 dB. */
	data->dac_volume_code = ES8311_VOL_0DB_CODE;
	data->adc_volume_code = ES8311_VOL_0DB_CODE;
	data->output_mute = false;
	data->adc_mute = false;
	/*
	 * STOPPED until start_output() opens the output. configure() sets up and powers the path
	 * but leaves the DAC muted; start_output() is the first unmute. That is the audio_codec
	 * lifecycle -- configure() is not start_output() -- and it is what samples/drivers/i2s/
	 * i2s_codec does: configure(), then start_output().
	 */
	data->output_stopped = true;
	/* Nothing is routed until configure() says so. */
	data->playback = false;
	data->capture = false;

	/*
	 * Identity first. If the part cannot be READ at all (a transport error, not a wrong
	 * answer), this returns and the driver writes NOTHING -- not the register-file release,
	 * not the quiesce. Quiescing would mean writing six ES8311 registers into whatever is
	 * really at an address the devicetree only claims is an ES8311; doing nothing to a part
	 * that cannot identify itself is the smaller failure. So the safety claims in this file
	 * are scoped: once the part has said it is an ES8311, init() leaves it as safe as the bus
	 * allows; before that, it does not touch it. (The one 0xFA write inside es8311_check_id()
	 * is bounded to a single register and argued for there.)
	 */
	ret = es8311_check_id(dev);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The ES8311 has no reset pin, so nothing a warm reboot does reaches it: west flash,
	 * sys_reboot(), a watchdog and a debugger reset the SoC and leave the codec as the previous
	 * firmware left it -- while the SoC's I2S peripheral resets, stopping the bit clock this
	 * codec derives its master clock from. A DAC left powered and unmuted is then frozen on its
	 * last sample (a DC level on the amplifier); a capture route leaves the microphone live at
	 * its last gain. This driver's own previous boot is enough to cause it. So: normalise 0xFA,
	 * then quiesce (see es8311_quiesce()), and leave the part there until configure() asks.
	 *
	 * The 0xFA write does NOT veto the quiesce. es8311_check_id() has just read 0x8311 back,
	 * and a held part answers 0x00 to every read, so INI_REG is proven clear here and this
	 * write only normalises the register's other (housekeeping) bits. It is attempted, its
	 * error is kept, and the quiesce runs regardless.
	 */
	int normalize_err = es8311_reg_write(dev, ES8311_REG_INI, ES8311_INI_RELEASE);
	int quiesce_err;

	if (normalize_err < 0) {
		LOG_ERR("Failed to normalise 0xFA (%d). Quiescing anyway: INI_REG is already "
			"known to be clear, because the chip id read back.",
			normalize_err);
	}

	/*
	 * Runs regardless of the 0xFA result, and its own errors are reported SEPARATELY. The
	 * two failures are not the same thing and must not be logged as if they were: a failed
	 * 0xFA normalisation with a fully successful quiesce is a safe part with one dirty
	 * housekeeping bit, and printing "failed to quiesce" over it would send whoever is
	 * debugging in exactly the wrong direction.
	 */
	quiesce_err = es8311_quiesce(dev);
	if (quiesce_err < 0) {
		LOG_ERR("Failed to fully quiesce the codec (%d). Every safety write was still "
			"attempted.",
			quiesce_err);
	}

	/*
	 * Either failure refuses the device: a part whose register file could not be normalised
	 * or fully quiesced is not one this driver can promise anything about. The quiesce error
	 * is the more informative of the two, so it wins when both fired.
	 */
	if (quiesce_err < 0) {
		return quiesce_err;
	}

	return normalize_err;
}

#define ES8311_INST(idx)                                                                           \
	static const struct es8311_config es8311_config_##idx = {                                  \
		.bus = I2C_DT_SPEC_INST_GET(idx),                                                  \
		.sdp_in_sel = (DT_INST_ENUM_IDX(idx, everest_mono_dac_source) == 1)                \
				      ? ES8311_SDP_IN_SEL_RIGHT                                    \
				      : 0U,                                                        \
		.pga_reg = ES8311_ADC_PGA_MIC1_0DB | (DT_INST_PROP(idx, everest_mic_pga_gain_db) / \
						      ES8311_ADC_PGA_GAIN_STEP_DB),                \
		.out_reg = (DT_INST_ENUM_IDX(idx, everest_output_mode) == 1)                       \
				   ? ES8311_OUT_LINEOUT                                            \
				   : ES8311_OUT_HEADPHONE,                                         \
	};                                                                                         \
	static struct es8311_data es8311_data_##idx;                                               \
	DEVICE_DT_INST_DEFINE(idx, es8311_init, NULL, &es8311_data_##idx, &es8311_config_##idx,    \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &es8311_api)

DT_INST_FOREACH_STATUS_OKAY(ES8311_INST)

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
 * The internal master clock comes either from the MCLK pin or, with register
 * 0x01 bit 7 set, from the I2S bit clock; audio_codec_cfg.mclk_freq picks
 * between them. Register programming follows the Everest ES8311 user guide
 * (rev 1.11). The playback and capture paths are validated on hardware.
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
#define ES8311_REG_SYSTEM_13    0x13U /* HPSW (bit 4): line-out or headphone drive */
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
 * 0x00: CSM_ON. RST_DIG is "reset digital EXCEPT control port block", and the
 * registers live in that block, so no value of 0x00 resets the register file. The
 * one bit that does is 0xFA bit 0, and this driver does not use it as a reset: it
 * programs the registers it depends on instead. Hence es8311_known_state[] below.
 */
#define ES8311_RESET_CSM_ON 0x80U

/*
 * The part is never reset, so every register it depends on is written. Two of
 * these override the volume registers the driver exposes: 0x18 ALC_EN ("When ALC
 * is on, ADC_VOLUME = MAXGAIN") and 0x34 DRC_EN, the same under 0x32. 0x0C powers
 * on at PWRUP_C = 32, outside its documented 0..31.
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
 * 0xFA bit 0, INI_REG: "reset registers to default except itself". An ES8311 driver
 * carried in Rockchip BSP kernels asserts it in its i2c shutdown handler and never
 * clears it, so a part can be handed over with its register file sitting at the
 * defaults. init() clears it once, after the identity check.
 */
#define ES8311_INI_RELEASE 0x00U

/*
 * 0x01: MCLK_SEL (bit 7) takes the internal master clock from BCLK instead of the
 * MCLK pin. MCLK_ON (5) and BCLK_ON (4) enable the clock inputs, and bits [3:0]
 * gate the digital and analog clocks of each converter: CLKADC_ON (3),
 * CLKDAC_ON (2), ANACLKADC_ON (1), ANACLKDAC_ON (0). The unused converter's
 * clocks are gated off, as the ASoC driver does.
 */
#define ES8311_CLK_SRC_BCLK          BIT(7) /* 0x01 bit 7: master clock from BCLK */
#define ES8311_CLK_MGR_BOTH_BASE     0x3FU  /* both converters clocked */
#define ES8311_CLK_MGR_PLAYBACK_BASE 0x35U  /* the ADC clocks gated off */
#define ES8311_CLK_MGR_CAPTURE_BASE  0x3AU  /* the DAC clocks gated off */

/*
 * Clock tree. MCLK is BCLK scaled by 0x02; a 16-bit stereo frame is 32 BCLK, so
 * DIV_PRE = 1 and MULT_PRE = x8 give 256 * Fs. The dividers are ratios of that,
 * so they hold at every rate a 16-bit word allows.
 *
 * The OSR registers are NOT dividers: 0x03 and 0x04 encode N as 4N * fs, so the
 * ratio argument does not cover them. ADC_OSR is still rate-independent, but
 * DAC_OSR is not, and the vendor moved 8k..16k to 128 * fs to fix noise there.
 * Bit 7 of both is undocumented and written 0, like every other bit this driver
 * depends on, because it never resets the part.
 */
#define ES8311_CLK_PRE_DIV1_MULT1 0x00U /* 0x02: DIV_PRE = 1, MULT_PRE = x1 */
#define ES8311_CLK_PRE_DIV1_MULT8 0x18U /* 0x02: DIV_PRE = 1, MULT_PRE = x8 */
#define ES8311_ADC_OSR_SINGLE_16  0x10U /* 0x03: single speed, ADC_OSR = 64 * fs */
#define ES8311_DAC_OSR_64FS       0x10U /* 0x04: DAC_OSR = 64 * fs */
#define ES8311_DAC_OSR_128FS      0x20U /* 0x04: DAC_OSR = 128 * fs, <= 16 kHz */
#define ES8311_CLK_DIV_ADC1_DAC1  0x00U /* 0x05: DIV_CLKADC = 1, DIV_CLKDAC = 1 */

/* Above this rate the vendor table uses 64 * fs; at or below it, 128 * fs. */
#define ES8311_DAC_OSR_128FS_MAX_RATE 16000U

/*
 * The master clock the dividers below are written for, and the bit clock a 16-bit
 * stereo frame carries.
 */
#define ES8311_MCLK_FS_RATIO 256U
#define ES8311_BCLK_FS_RATIO 32U

/*
 * Recommended operating conditions, note 2: when the internal clock source is
 * multiplied by 4 or 8, its frequency must be greater than 1 MHz for 3.3 V DVDD, or
 * 500 kHz for 1.8 V. The driver cannot see DVDD, so it applies the stricter figure,
 * and reads "its frequency" as the multiplier's input rather than its output, which
 * is the reading that constrains rather than excuses. Only the BCLK-derived mode
 * multiplies; feeding MCLK directly does not.
 */
#define ES8311_MULT_INPUT_MIN_HZ 1000000U

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
 * 0x09 and 0x0A, the serial data ports into the DAC and out of the ADC: word length
 * in [4:2] (0b011 is 16-bit), format in [1:0], mute at bit 6. Written whole.
 *
 * They differ at bit 7. 0x09 bit 7 is SDP_IN_SEL, which half of the stereo frame the
 * mono DAC plays, exposed as everest,mono-dac-source and written on every configure()
 * because a part that is never reset would otherwise keep a previous firmware's choice.
 * 0x0A has no bit 7: the datasheet's SDP_OUT table starts at bit 6.
 */
#define ES8311_SDP_I2S_16BIT    0x0CU
#define ES8311_SDP_MUTE         0x40U
#define ES8311_SDP_IN_SEL_RIGHT 0x80U /* 0x09 bit 7: mono DAC takes the RIGHT I2S slot */

/*
 * Power sequencing. In 0x0D, PDN_ANA (7), PDN_IBIASGEN (6), PDN_VREF (2) and
 * VMIDSEL [1:0] are shared and must stay up while either converter runs; only the
 * unused converter's own references are dropped.
 *
 * 0x0E powers the ADC down with PDN_PGA (6) and PDN_MOD (5). The vendor's full-idle
 * 0xFF is NOT used: it also sets LPVREFBUF, which puts the reference buffer the DAC
 * output shares into low power and would undermine a running playback stream.
 */
#define ES8311_ANALOG_BOTH     0x01U /* 0x0D */
#define ES8311_ANALOG_PLAYBACK 0x31U /* 0x0D: the ADC's bias and reference down */
#define ES8311_ANALOG_CAPTURE  0x09U /* 0x0D: the DAC's reference down */
#define ES8311_ADC_PWR_ON      0x02U /* 0x0E: PGA and modulator powered */
#define ES8311_ADC_PWR_DOWN    0x62U /* 0x0E: PDN_PGA | PDN_MOD, shared refs kept */
#define ES8311_DAC_PWR_ON      0x00U /* 0x12 */
#define ES8311_DAC_PWR_DOWN    0x02U /* 0x12: PDN_DAC */
/*
 * 0x13: HPSW=1, the headphone driver. Not the reset value -- 0x13 resets to 0x40, HPSW clear,
 * which is the line-out mode -- but what the reference drivers select and the only one this
 * driver has measured. Writing the whole byte also clears bit 6, which resets set and the
 * datasheet's bit table does not describe; the reference drivers write 0x10 too.
 */
#define ES8311_OUT_HEADPHONE 0x10U
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
 * These wait on the register writes only. Not on the audio clock, which is not
 * running during configure(), and not on the analog, which takes seconds from cold
 * and for which the datasheet specifies nothing.
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
 * The I2C bus and nothing else. There is no reset GPIO because the part has no reset
 * pin, and no power properties: supply-gpios and vin-supply come in from power.yaml
 * but neither is honoured, so a board whose codec sits behind a switch must bring
 * that rail up before this driver probes at POST_KERNEL. See the binding.
 */
struct es8311_config {
	struct i2c_dt_spec bus;
	uint8_t sdp_in_sel; /* 0x09 bit 7: mono DAC source slot (0 = left, 0x80 = right) */
	uint8_t pga_reg;    /* 0x14: MIC1 differential plus the configured PGA gain */
};

struct es8311_data {
	struct k_mutex lock;
	uint8_t dac_volume_code; /* cached 0x32 */
	uint8_t adc_volume_code; /* cached 0x17 */
	/*
	 * The caller's mute and our lifecycle are separate for each direction, so
	 * neither can forge the other: audible only when the route carries it AND
	 * the caller has not muted it AND it has not been stopped.
	 *
	 * Both default true. configure() powers a path but leaves it muted; start()
	 * is the first unmute in either direction.
	 */
	bool output_mute;
	bool adc_mute;
	bool output_stopped;
	bool input_stopped;
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
 * Leave both halves silent and unpowered. Used at the end of init() and on every
 * failing path out of configure(). Both directions, because muting SDP_OUT silences
 * only the digital output: a part that was capturing keeps its PGA powered and MIC1
 * wired in at whatever gain the last firmware chose.
 *
 * Every write is attempted and the first error returned, rather than stopping at the
 * first failure, because a bus that is already misbehaving is the worst moment to
 * walk away from a live speaker. The order is a priority: whatever lands is a prefix
 * of the list, so the speaker is muted first and the power-downs come last.
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
 * The dB clamp below is the only clamp needed, and these assertions are why: with
 * 0DB_CODE = 0xBF the endpoints land exactly on the register limits, 0xBF + 32*2 =
 * 0xFF and 0xBF - 95*2 = 0x01, so a second clamp on the code would be unreachable.
 * Retuning any of the three constants fails the build instead.
 *
 * The (int) casts are load-bearing: 0DB_CODE carries a U suffix, so without them the
 * expression promotes to unsigned and the lower-bound assertion becomes a tautology.
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
	uint8_t dac_osr;
	uint8_t sdp_in;
	uint8_t clk_pre;
	bool mclk_from_bclk;
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
	 * A 16-bit word in a two-slot frame is 32 bit clocks, which is what the
	 * BCLK-derived tree needs to land on 256fs and what the dividers below are
	 * written for in either clock mode.
	 */
	if (cfg->dai_cfg.i2s.word_size != AUDIO_PCM_WIDTH_16_BITS) {
		LOG_INF("Unsupported word size %u: this driver supports 16-bit I2S only",
			cfg->dai_cfg.i2s.word_size);
		return -ENOTSUP;
	}

	if (cfg->dai_cfg.i2s.channels != 2U) {
		LOG_INF("Unsupported channel count %u: the clock tree here assumes a two-slot "
			"frame",
			cfg->dai_cfg.i2s.channels);
		return -ENOTSUP;
	}

	rate = cfg->dai_cfg.i2s.frame_clk_freq;
	if (!es8311_rate_supported(rate)) {
		LOG_INF("Unsupported sample rate %u", rate);
		return -ENOTSUP;
	}

	/*
	 * A gated bit clock stops the serial port at a moment nothing tells this driver
	 * about, and in the BCLK-derived mode below it stops the whole clock tree, which
	 * leaves the DAC modulator frozen on its last sample as a DC level into the
	 * amplifier. Rejected rather than ignored, in both modes, because the mode is
	 * chosen further down and the hazard is worse in one of them. This covers only
	 * the configured state: underrun, DROP, PM and reset can remove BCLK without
	 * asking, and no codec driver sees those.
	 */
	if ((cfg->dai_cfg.i2s.options & I2S_OPT_BIT_CLK_GATED) != 0U) {
		LOG_INF("I2S_OPT_BIT_CLK_GATED is not supported: a gated bit clock stops the "
			"serial port, and stops the whole codec when the master clock is "
			"derived from BCLK");
		return -ENOTSUP;
	}

	/*
	 * This codec never drives BCLK or LRCK, so its role is TARGET on both. The
	 * I2S_OPT_*_CLK_* flags are endpoint-relative, so the host config carries
	 * CONTROLLER and the config handed here carries TARGET; i2s_codec builds exactly
	 * that pair. CONTROLLER on either clock would ask this part to drive a clock it
	 * cannot, leaving the link driven by nobody while configure() returned 0.
	 */
	if ((cfg->dai_cfg.i2s.options & (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) !=
	    (I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET)) {
		LOG_INF("this codec is always the clock target: the bit- and frame-clock roles "
			"must "
			"both be TARGET (the I2S controller drives BCLK and LRCK)");
		return -ENOTSUP;
	}

	/*
	 * Two clock sources, chosen by mclk_freq. Zero asks for the BCLK-derived tree,
	 * which runs the x8 multiplier off SCLK and so has to clear the input minimum
	 * above; any other value names the frequency present on the MCLK pin, which must
	 * be the 256fs the dividers below assume and which uses no multiplier at all.
	 */
	if (cfg->mclk_freq == 0U) {
		uint32_t mult_in = rate * ES8311_BCLK_FS_RATIO;

		if (mult_in <= ES8311_MULT_INPUT_MIN_HZ) {
			LOG_INF("a BCLK-derived clock at %u Hz gives a %u Hz multiplier input, "
				"under the published minimum; drive MCLK at %u Hz instead",
				rate, mult_in, rate * ES8311_MCLK_FS_RATIO);
			return -ENOTSUP;
		}
		mclk_from_bclk = true;
		clk_pre = ES8311_CLK_PRE_DIV1_MULT8;
	} else if (cfg->mclk_freq == rate * ES8311_MCLK_FS_RATIO) {
		mclk_from_bclk = false;
		clk_pre = ES8311_CLK_PRE_DIV1_MULT1;
	} else {
		LOG_INF("mclk_freq %u is neither 0 (derive from BCLK) nor %u (256fs on the "
			"MCLK pin)",
			cfg->mclk_freq, rate * ES8311_MCLK_FS_RATIO);
		return -ENOTSUP;
	}

	LOG_DBG("Configure: rate=%u", rate);

	k_mutex_lock(&data->lock, K_FOREVER);

	/*
	 * From here until the commit, this device has no route: a failure part-way
	 * through leaves the hardware half reprogrammed, and the old cached route would
	 * describe a chip that no longer exists. Necessary but not sufficient, since
	 * forgetting a converter is not stopping one; the error path below does that.
	 */
	data->playback = false;
	data->capture = false;
	/*
	 * And both directions go STOPPED. configure() is not start(): it sets up and powers the
	 * path but leaves it muted, whatever the lifecycle was before -- so "start() is the first
	 * unmute" holds on a RECONFIGURE too, not only a fresh init. It also stops a configure()
	 * that FAILED while a direction was started from letting the next successful configure()
	 * self-unmute before the caller has started it again.
	 */
	data->output_stopped = true;
	data->input_stopped = true;

	/*
	 * Route decisions are made under the lock: sampling the cached mute first would
	 * let another thread set an input mute this call then overwrites, leaving the
	 * cache muted and the microphone live.
	 *
	 * configure() never starts from a clean chip, so every route programs BOTH
	 * directions and powers down the one it does not use.
	 */
	if (playback && capture) {
		clk_mgr = ES8311_CLK_MGR_BOTH_BASE;
		analog = ES8311_ANALOG_BOTH;
	} else if (playback) {
		clk_mgr = ES8311_CLK_MGR_PLAYBACK_BASE;
		analog = ES8311_ANALOG_PLAYBACK;
	} else {
		clk_mgr = ES8311_CLK_MGR_CAPTURE_BASE;
		analog = ES8311_ANALOG_CAPTURE;
	}

	if (mclk_from_bclk) {
		clk_mgr |= ES8311_CLK_SRC_BCLK;
	}

	/*
	 * The only register in the clock tree that follows the sample rate rather than
	 * the 256fs ratio. See the DAC_OSR discussion above the register definitions:
	 * the vendor moved these four rates to 128 * fs to fix audible noise, in this
	 * same clock tree.
	 */
	dac_osr = (rate <= ES8311_DAC_OSR_128FS_MAX_RATE) ? ES8311_DAC_OSR_128FS
							  : ES8311_DAC_OSR_64FS;

	sdp_in = ES8311_SDP_I2S_16BIT | dcfg->sdp_in_sel | (playback ? 0U : ES8311_SDP_MUTE);

	/*
	 * Mute before reclocking. The clock manager below gates the clocks of whichever
	 * converter the new route drops, and gating the clock of a DAC that is still
	 * powered and unmuted freezes its modulator on the last sample, a DC step into
	 * the amplifier. So silence both ports and power the DAC down first, then
	 * reprogram the tree, then bring back only what the new route carries.
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
	 * The ADC is only muted here, not powered down: it drives no speaker so it cannot
	 * pop, and cycling its analog on every configure() would restart a settling time
	 * measured in seconds. The route logic below still powers it down when the new
	 * route drops capture, which is the case that matters.
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

	ret = es8311_reg_write(dev, ES8311_REG_CLK_PRE, clk_pre);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_ADC_OSR, ES8311_ADC_OSR_SINGLE_16);
	if (ret < 0) {
		goto end;
	}
	ret = es8311_reg_write(dev, ES8311_REG_DAC_OSR, dac_osr);
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

		ret = es8311_reg_write(dev, ES8311_REG_SYSTEM_13, ES8311_OUT_HEADPHONE);
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
	 * 0x44 and 0x45 belong to both routes, so both are normalised on every
	 * configure(). 0x44 bit 7 is ADC2DAC_SEL: a part that comes up with it set plays
	 * the ADC instead of the caller's audio, with every register reading back as
	 * intended, so it must be cleared even on a playback-only route. 0x45 is chip-wide.
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
	 * The commit: the only writes here that can move a converter. The output stays
	 * stopped, so the speaker is not unmuted by configure(). The microphone is the
	 * one converter configure() opens, so its serial port is written last with
	 * nothing failable after it, and 0x44 was normalised well above so an inherited
	 * ADC-into-DAC route cannot play the microphone into the speaker meanwhile.
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
	 * The microphone port last, and always muted: start(RX) is the only unmute. A muting write
	 * opens nothing, so a persistent bus failure -- which fails the error-path quiesce too --
	 * can never leave the microphone open behind a write that failed after it.
	 */
	ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
	if (ret < 0) {
		goto end;
	}

	data->playback = playback;
	data->capture = capture;

end:
	/*
	 * Clearing the route cache on failure is necessary but fails OPEN on its own:
	 * it makes the driver forget a converter, not stop one. A capture route whose
	 * first write failed still has its ADC powered and MIC1 wired in, and nothing
	 * left would reach it. So every error path also quiesces, under the same lock,
	 * best-effort: the caller is owed the error that broke the configure, not
	 * whatever the cleanup tripped over.
	 *
	 * The old route is deliberately not restored; it no longer describes the part.
	 */
	if (ret < 0) {
		/*
		 * Attempted, not guaranteed, and the log says so. es8311_quiesce() returning 0
		 * means every write was ACKed, not that it landed. Without a read-back -- which
		 * on that same failing bus may not work either -- "quiesced" cannot be claimed,
		 * only "quiesce attempted".
		 */
		(void)es8311_quiesce(dev);
		LOG_ERR("configure() I2C error: %d. A best-effort quiesce was attempted.", ret);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/*
 * Unmuted exactly when playback && !output_mute && !output_stopped, and the
 * hardware matches only after a call that programs 0x31; set_property() caches.
 * Unmuting is gated on the route and the caller's mute; muting is gated on nothing.
 * RX mutes at the ADC's serial port instead, the same 0x0A write.
 */
static int es8311_start_rx_locked(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	uint8_t sdp_out = ES8311_SDP_I2S_16BIT;
	int ret;

	if (!data->capture) {
		LOG_WRN("start: no capture route configured");
		return -EIO;
	}

	/* Already started: do not re-issue the unmute. The output side guards the same way. */
	if (!data->input_stopped) {
		return 0;
	}

	if (data->adc_mute) {
		sdp_out |= ES8311_SDP_MUTE;
	}

	ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT, sdp_out);
	if (ret < 0) {
		/* May have landed. A mute only ever leaves the path same-or-safer. */
		(void)es8311_reg_write(dev, ES8311_REG_SDP_OUT,
				       ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
		data->input_stopped = true;
		return ret;
	}

	data->input_stopped = false;

	return 0;
}

static int es8311_stop_rx_locked(const struct device *dev)
{
	struct es8311_data *data = dev->data;

	data->input_stopped = true;

	return es8311_reg_write(dev, ES8311_REG_SDP_OUT,
				ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
}

static int es8311_start_tx_locked(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int ret = 0;

	if (!data->playback) {
		/*
		 * Nothing to start -- and marking the lifecycle started here would pre-arm the
		 * NEXT configure() to open a speaker the caller never asked to run. Leave it.
		 */
		LOG_WRN("start: no playback route configured");
		return -EIO;
	}

	if (!data->output_stopped) {
		/*
		 * Already started: a duplicate start_output() must not re-issue a dangerous unmute
		 * that could glitch on the bus and stop a running stream. tas2563 guards the same
		 * way.
		 */
		return 0;
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

	return ret;
}

/*
 * The off switch, so it writes 0x31 rather than updating it: a read-modify-write
 * returns on a failed read without attempting the write, leaving the DAC as loud as
 * it was while reporting an error for something never tried. Safe to write whole
 * because 0x31 carries only DSMMUTE and DEMMUTE.
 */
static int es8311_stop_tx_locked(const struct device *dev)
{
	struct es8311_data *data = dev->data;

	data->output_stopped = true;

	/* The lifecycle stops; the OUTPUT_MUTE property is left exactly as the caller set it. */
	return es8311_reg_write(dev, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
}

/*
 * The checked, direction-aware lifecycle. It can report a failed mute, which matters
 * here because this part clocks off BCLK: a caller that stops the bit clock after one
 * leaves the output at whatever level the DAC froze at.
 *
 * Order is asymmetric on purpose. Start opens the microphone first and the speaker
 * last, so the dangerous opener is gated on the safe one. Stop closes the speaker
 * first, attempts every requested direction even after a failure, and returns the
 * first error.
 */
static int es8311_start(const struct device *dev, audio_dai_dir_t dir)
{
	struct es8311_data *data = dev->data;
	bool rx_was_stopped;
	int ret = 0;

	if (dir == 0U || (dir & ~(audio_dai_dir_t)AUDIO_DAI_DIR_TXRX) != 0U) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	rx_was_stopped = data->input_stopped;

	if ((dir & AUDIO_DAI_DIR_RX) != 0U) {
		ret = es8311_start_rx_locked(dev);
	}

	if (ret == 0 && (dir & AUDIO_DAI_DIR_TX) != 0U) {
		ret = es8311_start_tx_locked(dev);
		if (ret < 0 && (dir & AUDIO_DAI_DIR_RX) != 0U && rx_was_stopped) {
			/* Undo the opener this call made, not one it inherited. */
			(void)es8311_stop_rx_locked(dev);
		}
	}

	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		LOG_ERR("start(0x%x) failed (%d)", (unsigned int)dir, ret);
	}

	return ret;
}

static int es8311_stop(const struct device *dev, audio_dai_dir_t dir)
{
	struct es8311_data *data = dev->data;
	int first_err = 0;
	int ret;

	if (dir == 0U || (dir & ~(audio_dai_dir_t)AUDIO_DAI_DIR_TXRX) != 0U) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if ((dir & AUDIO_DAI_DIR_TX) != 0U) {
		ret = es8311_stop_tx_locked(dev);
		if (ret < 0) {
			first_err = ret;
		}
	}

	/*
	 * Ungated, like the TX half. configure() clears the route cache before it touches the
	 * chip, so a configure() that failed on its first write leaves data->capture false with
	 * the microphone still live -- and gating the off switch on that cache is exactly when it
	 * would be skipped. Muting a powered-down ADC costs one write and is always safe.
	 */
	if ((dir & AUDIO_DAI_DIR_RX) != 0U) {
		ret = es8311_stop_rx_locked(dev);
		if (ret < 0 && first_err == 0) {
			first_err = ret;
		}
	}

	k_mutex_unlock(&data->lock);

	if (first_err < 0) {
		LOG_ERR("stop(0x%x) failed (%d); the caller must not stop BCLK after this",
			(unsigned int)dir, first_err);
	}

	return first_err;
}

/*
 * The legacy void pair, kept because the API still marks them mandatory. They are
 * wrappers over the same locked helpers, so the two entry points cannot drift into
 * different state machines. What they cannot do is report the failure, which is the
 * whole reason the checked pair above exists.
 */
static void es8311_start_output(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = es8311_start_tx_locked(dev);
	k_mutex_unlock(&data->lock);

	if (ret < 0) {
		LOG_ERR("start_output: failed to establish the output state (%d)", ret);
	}
}

static void es8311_stop_output(const struct device *dev)
{
	struct es8311_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = es8311_stop_tx_locked(dev);
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
 * Three phases so a failure leaves things unchanged but never worse: every mute,
 * then the volumes only if no mute failed, then the unmutes only if the whole call
 * is clean. Returns the first error. The lock spans all three, or stop_output()
 * could mute the DAC between them and have phase 3 overwrite it.
 *
 * Full ordering rules: docs/15_ES8311_NO_RESET_DESIGN.md.
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
	if (data->capture && (data->adc_mute || data->input_stopped)) {
		ret = es8311_reg_write(dev, ES8311_REG_SDP_OUT,
				       ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
		if (ret < 0) {
			LOG_ERR("Failed to mute the microphone (%d)", ret);
			first_err = (first_err == 0) ? ret : first_err;
		}
	}

	/*
	 * 2. The volumes, but not once ANY requested mute has failed, in either
	 * direction. A volume is a gain: if a mute did not land, its path is still live
	 * and this call must not turn anything up, including the other direction. So
	 * the gate is the whole call, not the direction. A caller who asked to be both
	 * quieter and muted gets neither, which is a defensible failure where turning
	 * something up would not be.
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
	 * 3. The unmutes, only if everything above succeeded. The gate is first_err == 0
	 * re-tested at EACH unmute, not sampled once: the microphone unmute below can
	 * fail, and its error has to stop the speaker unmute too. The microphone goes
	 * first so the speaker is gated on it.
	 */
	bool this_apply_opened_mic = false;

	if (first_err == 0 && data->capture && !data->adc_mute && !data->input_stopped) {
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
	.start = es8311_start,
	.stop = es8311_stop,
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

/* Identify the part. Reads only: nothing is written to a device not yet known to be one. */
static int es8311_check_id(const struct device *dev)
{
	uint8_t id1 = 0U;
	uint8_t id2 = 0U;
	int ret;

	ret = es8311_read_id(dev, &id1, &id2);
	if (ret < 0) {
		return ret;
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
	 * Both STOPPED until start() opens them. configure() sets up and powers the path but
	 * leaves it muted; start() is the first unmute. That is the audio_codec lifecycle --
	 * configure() is not start() -- and it is what samples/drivers/i2s/i2s_codec does:
	 * configure(), then start_output().
	 */
	data->output_stopped = true;
	data->input_stopped = true;
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
	 * allows; before that, it does not touch it.
	 */
	ret = es8311_check_id(dev);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The part has no reset pin, so a warm reboot leaves a DAC frozen on its last
	 * sample and a microphone live. Recovery, not prevention: nothing runs between
	 * the SoC reset and here. INI_REG is released first, because the quiesce writes
	 * below are the ones that have to land.
	 */
	int normalize_err = es8311_reg_write(dev, ES8311_REG_INI, ES8311_INI_RELEASE);
	int quiesce_err;

	if (normalize_err < 0) {
		LOG_ERR("Failed to release INI_REG (%d). Quiescing anyway: the safety writes are "
			"worth attempting either way.",
			normalize_err);
	}

	/*
	 * Runs regardless of the INI_REG result, and its own errors are reported SEPARATELY. The
	 * two failures are not the same thing and must not be logged as if they were: a failed
	 * release with a fully successful quiesce is a safe part with one dirty housekeeping
	 * bit, and printing "failed to quiesce" over it would send whoever is debugging in
	 * exactly the wrong direction.
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
	};                                                                                         \
	static struct es8311_data es8311_data_##idx;                                               \
	DEVICE_DT_INST_DEFINE(idx, es8311_init, NULL, &es8311_data_##idx, &es8311_config_##idx,    \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &es8311_api)

DT_INST_FOREACH_STATUS_OKAY(ES8311_INST)

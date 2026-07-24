/*
 * Copyright (c) 2026 Hsiu-Chi Tsai
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>
#include <zephyr/sys/util.h>

#define CODEC_NODE DT_NODELABEL(codec)

static const struct device *const codec = DEVICE_DT_GET(CODEC_NODE);
static const struct i2c_dt_spec es = I2C_DT_SPEC_GET(CODEC_NODE);
static const struct emul *const emul = EMUL_DT_GET(CODEC_NODE);

/*
 * A second instance, marked zephyr,deferred-init, so its init() has NOT run yet. That is
 * the only way to probe a part that was already in a hostile state before the driver first
 * touched it: device_init() returns -EALREADY on an already-initialised device, so the
 * instance above cannot be re-probed.
 */
#define HELD_NODE DT_NODELABEL(codec_held)
static const struct device *const codec_held = DEVICE_DT_GET(HELD_NODE);
static const struct i2c_dt_spec es_held = I2C_DT_SPEC_GET(HELD_NODE);
static const struct emul *const emul_held = EMUL_DT_GET(HELD_NODE);

#define QUIESCE_NODE DT_NODELABEL(codec_quiesce)
static const struct device *const codec_quiesce = DEVICE_DT_GET(QUIESCE_NODE);
static const struct i2c_dt_spec es_quiesce = I2C_DT_SPEC_GET(QUIESCE_NODE);
static const struct emul *const emul_quiesce = EMUL_DT_GET(QUIESCE_NODE);

#define FOREIGN_NODE DT_NODELABEL(codec_foreign)
static const struct device *const codec_foreign = DEVICE_DT_GET(FOREIGN_NODE);
static const struct emul *const emul_foreign = EMUL_DT_GET(FOREIGN_NODE);

#define WARM_NODE DT_NODELABEL(codec_warm)
static const struct device *const codec_warm = DEVICE_DT_GET(WARM_NODE);
static const struct i2c_dt_spec es_warm = I2C_DT_SPEC_GET(WARM_NODE);

#define MUTE_NODE DT_NODELABEL(codec_mute)
static const struct device *const codec_mute = DEVICE_DT_GET(MUTE_NODE);
static const struct emul *const emul_mute = EMUL_DT_GET(MUTE_NODE);

#define INI_NODE DT_NODELABEL(codec_ini)
static const struct device *const codec_ini = DEVICE_DT_GET(INI_NODE);
static const struct i2c_dt_spec es_ini = I2C_DT_SPEC_GET(INI_NODE);
static const struct emul *const emul_ini = EMUL_DT_GET(INI_NODE);

/* A part with every board-policy devicetree property set to its non-default value. */
#define PROFILE_NODE DT_NODELABEL(codec_profile)
static const struct device *const codec_profile = DEVICE_DT_GET(PROFILE_NODE);
static const struct i2c_dt_spec es_profile = I2C_DT_SPEC_GET(PROFILE_NODE);

/* Emulator test backend (defined in drivers/audio/emul_es8311.c). */
#include <emul_es8311.h>

/*
 * ES8311 register map (the subset the driver touches), named after the fields
 * the Everest ES8311 user guide actually puts there: 0x03/0x04 are oversampling
 * rates and 0x05 holds the ADC and DAC clock dividers, not the other way round.
 */
#define ES8311_REG_RESET       0x00
#define ES8311_REG_CLK_MANAGER 0x01
#define ES8311_REG_CLK_PRE     0x02 /* DIV_PRE, MULT_PRE */
#define ES8311_REG_ADC_OSR     0x03 /* ADC_FSMODE, ADC_OSR */
#define ES8311_REG_DAC_OSR     0x04 /* DAC_OSR */
#define ES8311_REG_CLK_DIV     0x05 /* DIV_CLKADC, DIV_CLKDAC */
#define ES8311_REG_CLK_BCLK    0x06
#define ES8311_REG_CLK_LRCK_H  0x07
#define ES8311_REG_CLK_LRCK_L  0x08
#define ES8311_REG_SDP_IN      0x09
#define ES8311_REG_SYSTEM_0D   0x0D
#define ES8311_REG_SYSTEM_12   0x12
#define ES8311_REG_SYSTEM_13   0x13
#define ES8311_REG_DAC_MUTE    0x31
#define ES8311_REG_DAC_VOLUME  0x32
#define ES8311_REG_DAC_EQ      0x37
/* ADC / capture path registers. */
#define ES8311_REG_SDP_OUT     0x0A /* bit 6 is the ADC serial-port mute */
#define ES8311_REG_SYSTEM_0E   0x0E
#define ES8311_REG_ADC_PGA     0x14
#define ES8311_ADC_MIC_OFF     0x00U /* 0x14: microphone disconnected from the PGA mux */
#define ES8311_REG_ADC_RAMP    0x15  /* ADC_RAMPRATE, not an OSR */
#define ES8311_REG_ADC_SCALE   0x16  /* ADC polarity, ADC_SCALE */
#define ES8311_REG_ADC_VOLUME  0x17
#define ES8311_REG_ADC_HPF1    0x1B
#define ES8311_REG_ADC_HPF2    0x1C
#define ES8311_REG_ADC_MUX     0x44
#define ES8311_REG_ADC_GP45    0x45
#define ES8311_REG_CHIP_ID1    0xFD
#define ES8311_REG_CHIP_ID2    0xFE

/*
 * The registers the driver used to depend on and never write. Every one of them is a
 * state a previous firmware -- or this driver's own previous boot, on a part with no
 * reset pin that a warm reboot cannot reach -- can hand it.
 */
#define ES8311_REG_PWRUP_AB     0x0B
#define ES8311_REG_PWRUP_C      0x0C /* power-on default 0x20: PWRUP_C = 32 */
#define ES8311_REG_LOW_POWER    0x0F
#define ES8311_REG_ANALOG_10    0x10
#define ES8311_REG_ANALOG_11    0x11
#define ES8311_REG_ADC_ALC      0x18 /* ALC_EN bit 7 */
#define ES8311_REG_ADC_ALC_LVL  0x19
#define ES8311_REG_ADC_AUTOMUTE 0x1A
#define ES8311_REG_DAC_OFFSET   0x33
#define ES8311_REG_DAC_DRC      0x34 /* DRC_EN bit 7 */
#define ES8311_REG_DAC_DRC_LVL  0x35
#define ES8311_REG_INI          0xFA /* INI_REG bit 0 holds the register file down */

#define ES8311_ALC_EN      0x80 /* 0x18 bit 7: "when ALC is on, ADC_VOLUME = MAXGAIN" */
#define ES8311_DRC_EN      0x80 /* 0x34 bit 7: the same, for the DAC volume */
#define ES8311_ADC2DAC_SEL 0x80 /* 0x44 bit 7: routes the ADC into the DAC */
#define ES8311_RESET_BITS  0x1F /* 0x00 [4:0]: the digital block resets */

/* A register in the map that the driver never writes, for use as a witness. */
#define ES8311_REG_UNUSED 0x36

#define ES8311_SDP_MUTE      0x40 /* 0x09 / 0x0A bit 6 */
#define ES8311_SDP_I2S_16BIT 0x0CU
#define ES8311_DAC_MUTE_ON   0x60U
#define ES8311_DAC_MUTE_OFF  0x00U

/*
 * 0x09 bit 7, SDP_IN_SEL: the half of the stereo I2S frame the mono DAC takes. 0 is the left
 * slot and is the reset default; 1 is the right. It exists only on 0x09 -- the datasheet's
 * 0x0A table starts at bit 6. The driver always writes it clear, so the DAC always plays the
 * left slot, and on a part that is never reset that is a normalisation and not an accident.
 */
#define ES8311_SDP_IN_SEL_RIGHT 0x80U

/* Every rate the driver accepts, and the 256fs master clock each one implies. */
static const uint32_t supported_rates[] = {
	8000U, 11025U, 12000U, 16000U, 22050U, 24000U, 32000U, 44100U, 48000U,
};

static uint8_t reg_get(uint8_t r)
{
	uint8_t v = 0xa5U;

	zassert_ok(i2c_reg_read_byte_dt(&es, r, &v), "i2c read of 0x%02x failed", r);
	return v;
}

static void reg_put(uint8_t r, uint8_t v)
{
	zassert_ok(i2c_reg_write_byte_dt(&es, r, v), "i2c write of 0x%02x failed", r);
}

/* A configuration with the master clock derived from BCLK (mclk_freq == 0). */
static void make_cfg(struct audio_codec_cfg *cfg, uint32_t rate, audio_route_t route)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->mclk_freq = 0U;
	cfg->dai_type = AUDIO_DAI_TYPE_I2S;
	cfg->dai_route = route;
	cfg->dai_cfg.i2s.word_size = AUDIO_PCM_WIDTH_16_BITS;
	cfg->dai_cfg.i2s.channels = 2;
	cfg->dai_cfg.i2s.frame_clk_freq = rate;
	/*
	 * The codec-local clock role. This part is always the target (the I2S controller drives
	 * BCLK and LRCK), which is exactly what samples/drivers/i2s/i2s_codec passes to the codec
	 * when the SoC supplies the clock. options == 0 would be CONTROLLER|CONTROLLER -- the one
	 * role this codec cannot fill -- so every test must set it, not inherit the memset zero.
	 */
	cfg->dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
}

/* 16 kHz / 16-bit playback, the configuration the application uses. */
static void make_cfg_16k_16bit(struct audio_codec_cfg *cfg)
{
	make_cfg(cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
}

/*
 * The driver reads the chip-id registers (0xFD/0xFE) in init(). The emulator
 * seeds them to 0x83/0x11, so a readable identity proves the bus is wired and
 * init() ran.
 */
ZTEST(es8311, test_init_reads_chip_id)
{
	zassert_true(device_is_ready(codec), "codec device not ready");
	zassert_equal(reg_get(ES8311_REG_CHIP_ID1), 0x83U, "chip id1 should read 0x83");
	zassert_equal(reg_get(ES8311_REG_CHIP_ID2), 0x11U, "chip id2 should read 0x11");
}

/*
 * A part at this address that is not an ES8311 must not come up ready. The driver
 * used to log a warning and return success, which meant the identity check had no
 * effect at all: the device went ready and every later register write went to
 * whatever part was really there. Seed a wrong id, force init() to re-run, and
 * assert it now fails with -ENODEV and leaves the device NOT ready.
 */
ZTEST(es8311, test_init_wrong_chip_id_is_fatal)
{
	/*
	 * A foreign part at this address must be refused, and refused fatally: a chip-id
	 * check that leaves the device ready is not a check, because every later register
	 * write then goes to whatever is really there.
	 *
	 * It runs against its own never-initialised instance rather than faking a re-probe by
	 * writing Zephyr's internal device state. That state is not a test contract, and
	 * writing it left the shared instance un-ready for every test after this one whenever
	 * the restore was missed.
	 *
	 * 0x0000 is deliberately the identity used. It is also the signature of a part whose
	 * register file is held by INI_REG, so check_id() releases 0xFA and reads again --
	 * which makes this a test that the recovery path has not become a way of ACCEPTING a
	 * foreign device. It answers 0x0000 twice, and it is still rejected.
	 */
	zassert_false(device_is_ready(codec_foreign), "the deferred instance must not be up yet");

	emul_es8311_set_chip_id(emul_foreign, 0x00U, 0x00U);

	zassert_equal(device_init(codec_foreign), -ENODEV,
		      "init() must reject a part that cannot identify itself as an ES8311, "
		      "even after the INI_REG recovery attempt");
	zassert_false(device_is_ready(codec_foreign), "a foreign part must not be left ready");
}

/*
 * configure() for 16 kHz / 16-bit must emit the expected register sequence.
 * Seed every touched register to the opposite of the expected value first, so
 * each assertion requires a real write by the driver.
 */
ZTEST(es8311, test_configure_16k_16bit_sequence)
{
	struct audio_codec_cfg cfg;

	/* Poison the registers the sequence is expected to set. */
	reg_put(ES8311_REG_RESET, 0x00);
	reg_put(ES8311_REG_CLK_MANAGER, 0x00);
	reg_put(ES8311_REG_CLK_PRE, 0x00);
	reg_put(ES8311_REG_CLK_BCLK, 0xFF);
	reg_put(ES8311_REG_CLK_LRCK_L, 0x00);
	reg_put(ES8311_REG_SDP_IN, 0xFF);
	reg_put(ES8311_REG_SDP_OUT, 0x0C);
	reg_put(ES8311_REG_SYSTEM_0D, 0xFF);
	reg_put(ES8311_REG_SYSTEM_0E, 0x02);
	reg_put(ES8311_REG_SYSTEM_12, 0xFF);
	reg_put(ES8311_REG_SYSTEM_13, 0x00);
	reg_put(ES8311_REG_ADC_PGA, 0x1A);
	reg_put(ES8311_REG_DAC_EQ, 0x00);
	reg_put(ES8311_REG_DAC_MUTE, 0xFF);

	make_cfg_16k_16bit(&cfg);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	/* CSM on. */
	zassert_equal(reg_get(ES8311_REG_RESET), 0x80U, "0x00 should be 0x80");
	/* Master clock from BCLK, and the ADC's clocks gated off on a playback route. */
	zassert_equal(reg_get(ES8311_REG_CLK_MANAGER), 0xB5U, "0x01 should be 0xB5");
	/* DIV_PRE = 1, MULT_PRE = x8. */
	zassert_equal(reg_get(ES8311_REG_CLK_PRE), 0x18U, "0x02 should be 0x18");
	/* BCLK_CON clear, so the codec stays the I2S clock slave. */
	zassert_equal(reg_get(ES8311_REG_CLK_BCLK), 0x03U, "0x06 should be 0x03");
	/* DIV_LRCK low byte. */
	zassert_equal(reg_get(ES8311_REG_CLK_LRCK_L), 0xFFU, "0x08 should be 0xFF");
	/* The DAC serial port carries I2S, 16-bit, unmuted. */
	zassert_equal(reg_get(ES8311_REG_SDP_IN), 0x0CU, "0x09 should be 0x0C");
	/* The shared analog block is up, with the ADC's own references down. */
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0D), 0x31U, "0x0D should be 0x31");
	/* DAC power up. */
	zassert_equal(reg_get(ES8311_REG_SYSTEM_12), 0x00U, "0x12 should be 0x00");
	/* Headphone output path. */
	zassert_equal(reg_get(ES8311_REG_SYSTEM_13), 0x10U, "0x13 should be 0x10");
	/* EQ bypass. */
	zassert_equal(reg_get(ES8311_REG_DAC_EQ), 0x48U,
		      "0x37 should be 0x48: DAC volume soft-ramp (rate 4) + EQ bypass");
	/* The cached DAC volume is programmed. The fixture leaves it at 0 dB, which is 0xBF. */
	zassert_equal(reg_get(ES8311_REG_DAC_VOLUME), 0xBFU,
		      "0x32 should carry the cached volume (0 dB = 0xBF)");
	/* Muted at end of configure: configure() leaves the output stopped, start_output() unmutes.
	 */
	zassert_equal(
		reg_get(ES8311_REG_DAC_MUTE), 0x60U,
		"0x31 should be MUTED after configure (0x60): configure() is not start_output()");

	/*
	 * A playback-only route owes the caller a microphone that is off, not merely
	 * one whose data is being thrown away. The ADC is powered down, its serial
	 * port is muted, and the microphone is taken off the input mux.
	 */
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x62U,
		      "0x0E: the ADC must be powered down on a playback-only route");
	zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x00U,
		      "0x14: the microphone must be taken off the input mux");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "0x0A: the ADC serial port must be muted");
}

/*
 * Within configure(), reset (0x00) must precede the clock manager (0x01), and
 * the analog power-up (0x0D) must precede the DAC power-up (0x12).
 */
ZTEST(es8311, test_configure_write_order)
{
	struct audio_codec_cfg cfg;
	int n, reset_idx = -1, clk_idx = -1, ana_idx = -1, dac_up_idx = -1;

	emul_es8311_reset_log(emul);
	make_cfg_16k_16bit(&cfg);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	n = emul_es8311_write_count(emul);
	zassert_true(n >= 13, "configure should emit the full sequence (got %d)", n);

	for (int i = 0; i < n; i++) {
		int r = emul_es8311_write_at(emul, i);

		if (r == ES8311_REG_RESET && reset_idx < 0) {
			reset_idx = i;
		}
		if (r == ES8311_REG_CLK_MANAGER && clk_idx < 0) {
			clk_idx = i;
		}
		if (r == ES8311_REG_SYSTEM_0D && ana_idx < 0) {
			ana_idx = i;
		}
		/*
		 * The LAST 0x12, not the first. configure() now writes it twice: once in
		 * the quiesce block that powers the DAC down before the clock tree moves,
		 * and once at the end to power it back up if the route carries playback.
		 * The constraint is about the power-UP: the analog bias and references
		 * must be established before a converter comes up on them. Taking the
		 * first write would be measuring the power-DOWN, which is required to
		 * come earlier, not later.
		 */
		if (r == ES8311_REG_SYSTEM_12) {
			dac_up_idx = i;
		}
	}

	zassert_true(reset_idx >= 0 && reset_idx < clk_idx,
		     "reset (0x00) must be written before clk manager (0x01)");
	zassert_true(ana_idx >= 0 && ana_idx < dac_up_idx,
		     "analog references (0x0D) must precede the DAC power-up (0x12): "
		     "0x0D at %d, the last 0x12 at %d",
		     ana_idx, dac_up_idx);
}

/*
 * configure() with a capture route must additionally emit the ADC register
 * sequence. These values target a differential analog MIC1 at 16 kHz / 16-bit.
 * PLAYBACK_CAPTURE must still emit the DAC sequence too.
 */
ZTEST(es8311, test_configure_capture_sequence)
{
	struct audio_codec_cfg cfg;

	/* Poison the ADC registers the capture path is expected to set. */
	reg_put(ES8311_REG_SDP_OUT, 0xFF);
	reg_put(ES8311_REG_SYSTEM_0E, 0xFF);
	reg_put(ES8311_REG_ADC_PGA, 0x00);
	reg_put(ES8311_REG_ADC_RAMP, 0x00);
	reg_put(ES8311_REG_ADC_SCALE, 0x00);
	reg_put(ES8311_REG_ADC_VOLUME, 0x00);
	reg_put(ES8311_REG_ADC_HPF1, 0xFF);
	reg_put(ES8311_REG_ADC_HPF2, 0x00);
	reg_put(ES8311_REG_ADC_MUX, 0xFF);
	reg_put(ES8311_REG_ADC_GP45, 0xFF);

	make_cfg_16k_16bit(&cfg);
	cfg.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE;
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(PLAYBACK_CAPTURE) failed");

	/* ADC serial data port: standard I2S, 16-bit, not muted. */
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), 0x0CU, "0x0A should be 0x0C");
	/* ADC power up. */
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x02U, "0x0E should be 0x02");
	/* Differential MIC1 pair (LINSEL = 1) at the 30 dB PGA maximum. */
	zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x1AU, "0x14 should be 0x1A");
	/* ADC volume ramp rate. */
	zassert_equal(reg_get(ES8311_REG_ADC_RAMP), 0x40U, "0x15 should be 0x40");
	/* ADC digital scale. */
	zassert_equal(reg_get(ES8311_REG_ADC_SCALE), 0x24U, "0x16 should be 0x24");
	/* ADC digital volume, 0 dB by default. */
	zassert_equal(reg_get(ES8311_REG_ADC_VOLUME), 0xBFU, "0x17 should be 0xBF");
	/* ADC HPF + EQ bypass: cancels the digital DC offset. */
	zassert_equal(reg_get(ES8311_REG_ADC_HPF1), 0x0AU, "0x1B should be 0x0A");
	zassert_equal(reg_get(ES8311_REG_ADC_HPF2), 0x6AU, "0x1C should be 0x6A");
	/* 0x44 ADCDAT mux = plain ADC data on ASDOUT (no digital DAC feedback). */
	zassert_equal(reg_get(ES8311_REG_ADC_MUX), 0x08U, "0x44 should be 0x08");
	zassert_equal(reg_get(ES8311_REG_ADC_GP45), 0x00U, "0x45 should be 0x00");

	/* PLAYBACK_CAPTURE must also still emit the DAC path (spot-check). */
	zassert_equal(reg_get(ES8311_REG_SDP_IN), 0x0CU, "0x09 (DAC SDP) should be 0x0C");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_12), 0x00U, "0x12 (DAC power) should be 0x00");
}

/*
 * A capture-only route powers the ADC up and the DAC DOWN.
 *
 * The previous version of this test asserted that 0x12 was left untouched, which
 * did not describe correct behaviour: it described the bug. Register 0x00 does
 * not reset the register file, so a DAC a previous PLAYBACK_CAPTURE route powered
 * up stays powered, and the speaker path stays live behind a caller that asked
 * for capture only.
 */
ZTEST(es8311, test_configure_capture_only)
{
	struct audio_codec_cfg cfg;

	/* Come from a route that had the DAC up, which is the case that used to leak. */
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(PLAYBACK_CAPTURE) failed");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_12), 0x00U, "the DAC should be up here");

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(CAPTURE) failed");

	zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x02U, "0x0E: the ADC must be powered up");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_12), 0x02U,
		      "0x12: the DAC must be powered DOWN, not left as the previous route "
		      "left it");
	zassert_equal(reg_get(ES8311_REG_CLK_MANAGER), 0xBAU,
		      "0x01: the DAC clocks must be gated off");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0D), 0x09U,
		      "0x0D: the DAC's own reference must be dropped");
	zassert_equal(reg_get(ES8311_REG_SDP_IN) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "0x09: the DAC serial port must be muted");
}

/*
 * The other direction of the same defect: going from PLAYBACK_CAPTURE to
 * PLAYBACK must take the microphone down, not just stop reading it. A muted
 * serial port would leave the PGA and the modulator running on a live mic.
 */
ZTEST(es8311, test_route_transition_drops_the_microphone)
{
	struct audio_codec_cfg cfg;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(PLAYBACK_CAPTURE) failed");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x02U, "the ADC should be up here");
	zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x1AU, "the mic should be on the mux here");

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(PLAYBACK) failed");

	zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x62U,
		      "0x0E: the ADC must be powered down, not left running");
	zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x00U,
		      "0x14: the microphone must be taken off the input mux");
	zassert_equal(reg_get(ES8311_REG_CLK_MANAGER), 0xB5U,
		      "0x01: the ADC clocks must be gated off");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0D), 0x31U,
		      "0x0D: the ADC's own bias and reference must be dropped");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "0x0A: the ADC serial port must be muted");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_12), 0x00U, "the DAC must still be up");
}

/*
 * And the properties must respect the route too. On a playback-only route,
 * apply_properties() must not clear the microphone's serial-port mute that
 * configure() just set: the cached input state is not a licence to re-open a
 * path the route took down.
 */
ZTEST(es8311, test_apply_properties_respects_the_route)
{
	struct audio_codec_cfg cfg;
	audio_property_value_t unmute = {.mute = false};

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(PLAYBACK) failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "the ADC port should be muted after a playback-only configure");

	/* The default cached input state is "not muted". Applying it must not win. */
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute),
		   "set INPUT_MUTE(false) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");

	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "apply_properties() must not re-open the microphone on a route that "
		      "does not carry capture");
}

/*
 * The clock tree is ratiometric: the master clock is 8 * BCLK, and a 16-bit
 * stereo frame carries 32 bit clocks, so the master clock is 256 * Fs at every
 * supported rate. That means one register set must serve them all. Configure at
 * each supported rate and assert the clock registers are identical every time.
 */
ZTEST(es8311, test_configure_all_supported_rates)
{
	struct audio_codec_cfg cfg;

	for (size_t i = 0; i < ARRAY_SIZE(supported_rates); i++) {
		uint32_t rate = supported_rates[i];

		/* Poison the clock registers before each rate. */
		reg_put(ES8311_REG_CLK_MANAGER, 0x00);
		reg_put(ES8311_REG_CLK_PRE, 0x00);
		reg_put(ES8311_REG_ADC_OSR, 0x00);
		reg_put(ES8311_REG_DAC_OSR, 0x00);
		reg_put(ES8311_REG_CLK_DIV, 0xFF);
		reg_put(ES8311_REG_CLK_BCLK, 0x00);
		reg_put(ES8311_REG_CLK_LRCK_H, 0xFF);
		reg_put(ES8311_REG_CLK_LRCK_L, 0x00);

		make_cfg(&cfg, rate, AUDIO_ROUTE_PLAYBACK);
		zassert_ok(audio_codec_configure(codec, &cfg), "configure(%u Hz) failed", rate);

		/* A playback-only route: the ADC's clocks are gated off. */
		zassert_equal(reg_get(ES8311_REG_CLK_MANAGER), 0xB5U, "0x01 at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_CLK_PRE), 0x18U, "0x02 at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_ADC_OSR), 0x10U, "0x03 at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_DAC_OSR), 0x10U, "0x04 at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_CLK_DIV), 0x00U, "0x05 at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_CLK_BCLK), 0x03U, "0x06 at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_CLK_LRCK_H), 0x00U, "0x07 at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_CLK_LRCK_L), 0xFFU, "0x08 at %u Hz", rate);
	}
}

/* Rates outside the supported set must be rejected, not silently mis-clocked. */
ZTEST(es8311, test_configure_rejects_bad_rates)
{
	static const uint32_t bad[] = {0U, 7999U, 44099U, 96000U, 192000U};
	struct audio_codec_cfg cfg;

	for (size_t i = 0; i < ARRAY_SIZE(bad); i++) {
		make_cfg(&cfg, bad[i], AUDIO_ROUTE_PLAYBACK);
		zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
			      "rate %u must be rejected", bad[i]);
	}
}

/*
 * The master clock is derived from BCLK, so the frame has to carry 32 bit
 * clocks for the clock tree to land on 256fs. A 24-bit frame carries 48 and a
 * 32-bit frame 64, which would silently shift the pitch: both must be rejected.
 */
ZTEST(es8311, test_configure_rejects_non_16bit_word)
{
	struct audio_codec_cfg cfg;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	cfg.dai_cfg.i2s.word_size = AUDIO_PCM_WIDTH_24_BITS;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
		      "24-bit words must be rejected (BCLK-derived MCLK needs 16-bit)");

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	cfg.dai_cfg.i2s.word_size = AUDIO_PCM_WIDTH_32_BITS;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
		      "32-bit words must be rejected (BCLK-derived MCLK needs 16-bit)");
}

/*
 * audio_codec_cfg.mclk_freq describes the clock fed to the codec's MCLK *input*
 * pin. This driver never uses that pin: it derives the master clock from BCLK.
 * So zero is the only meaningful value, and anything else must be rejected
 * rather than silently ignored. In particular the internal 256fs figure is NOT
 * an MCLK input frequency, and accepting it would be describing a clock that is
 * not on any pin.
 */
ZTEST(es8311, test_configure_mclk_must_be_zero)
{
	struct audio_codec_cfg cfg;

	for (size_t i = 0; i < ARRAY_SIZE(supported_rates); i++) {
		uint32_t rate = supported_rates[i];

		make_cfg(&cfg, rate, AUDIO_ROUTE_PLAYBACK);
		cfg.mclk_freq = 0U;
		zassert_ok(audio_codec_configure(codec, &cfg),
			   "mclk 0 (BCLK-derived) must be accepted at %u Hz", rate);

		make_cfg(&cfg, rate, AUDIO_ROUTE_PLAYBACK);
		cfg.mclk_freq = rate * 256U;
		zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
			      "the internal 256fs clock is not an MCLK input and must be "
			      "rejected at %u Hz",
			      rate);

		make_cfg(&cfg, rate, AUDIO_ROUTE_PLAYBACK);
		cfg.mclk_freq = 12288000U;
		zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
			      "an external MCLK is not supported and must be rejected at %u Hz",
			      rate);
	}
}

/* configure() must reject a non-I2S DAI, an unsupported route and a bad format. */
ZTEST(es8311, test_configure_rejects_unsupported)
{
	struct audio_codec_cfg cfg;

	make_cfg_16k_16bit(&cfg);
	cfg.dai_type = AUDIO_DAI_TYPE_PCM;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP, "non-I2S DAI must be rejected");

	make_cfg_16k_16bit(&cfg);
	cfg.dai_route = AUDIO_ROUTE_BYPASS;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
		      "bypass route must be rejected (only playback/capture supported)");

	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
		      "left-justified data format must be rejected");

	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.format = I2S_FMT_DATA_ORDER_LSB;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
		      "LSB-first data ordering must be rejected");

	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.format = I2S_FMT_BIT_CLK_INV;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
		      "inverted bit clock must be rejected");
}

/*
 * OUTPUT_VOLUME set_property + apply_properties must write the DAC volume
 * register (0x32). 0 dB maps to code 0xBF.
 */
ZTEST(es8311, test_set_volume)
{
	audio_property_value_t val = {.vol = 0};

	reg_put(ES8311_REG_DAC_VOLUME, 0x00);

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val),
		   "set OUTPUT_VOLUME failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");

	zassert_equal(reg_get(ES8311_REG_DAC_VOLUME), 0xBFU, "0 dB should map to 0xBF");
}

/* The dB-to-code mapping must clamp at both ends of the register range. */
ZTEST(es8311, test_volume_clamps)
{
	audio_property_value_t val;

	val.vol = 1000;
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val),
		   "set OUTPUT_VOLUME(+1000 dB) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_DAC_VOLUME), 0xFFU,
		      "an absurdly high volume must clamp to the 0xFF maximum");

	val.vol = -1000;
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val),
		   "set OUTPUT_VOLUME(-1000 dB) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_DAC_VOLUME), 0x01U,
		      "an absurdly low volume must clamp to the -95 dB code");
}

/*
 * OUTPUT_MUTE set_property + apply_properties must write the DAC mute field
 * (0x31 bits [6:5]).
 */
ZTEST(es8311, test_set_mute)
{
	audio_property_value_t mute = {.mute = true};
	audio_property_value_t unmute = {.mute = false};

	reg_put(ES8311_REG_DAC_MUTE, 0x00);
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute),
		   "set OUTPUT_MUTE(true) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "mute field (bit6 DSMMUTE | bit5 DEMMUTE) should be set");

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute),
		   "set OUTPUT_MUTE(false) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U,
		      "mute field (bit6 DSMMUTE | bit5 DEMMUTE) should be clear");
}

/* start_output() and stop_output() drive the DAC mute across the stopped<->started transition. */
ZTEST(es8311, test_start_stop_output)
{
	/* The fixture leaves the output started; stop it first so start_output() has work to do. */
	audio_codec_stop_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "stop_output() must mute the DAC");

	audio_codec_start_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U,
		      "start_output() must unmute the DAC");

	audio_codec_stop_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U, "stop_output() must mute again");

	audio_codec_start_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U,
		      "start_output() must unmute again");
}

/*
 * A duplicate start_output() on an already-started output is a no-op: it must not re-issue the
 * unmute, so a transient error on that redundant write cannot glitch a running stream to a
 * best-effort stop. tas2563 guards the same way.
 */
ZTEST(es8311, test_duplicate_start_output_is_a_noop)
{
	/* The output is started (fixture). Re-start it with the DAC-mute write armed to fail. */
	emul_es8311_reset_log(emul);
	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	audio_codec_start_output(codec);
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(emul_es8311_write_count(emul), 0,
		      "a duplicate start_output() must issue no I2C writes");
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U,
		      "a duplicate start_output() must leave the running stream unmuted");
}

/*
 * A pending OUTPUT_MUTE + a stop_output() whose mute glitched + start_output() must NOT leave the
 * speaker live. start_output() ESTABLISHES the hardware state it claims: with a pending mute it
 * writes the mute, it does not merely flip the lifecycle flag. Without that, the invariant
 * "audible only when playback && !output_mute && !output_stopped" is violated -- output_mute is
 * set, yet the DAC is live.
 */
ZTEST(es8311, test_pending_mute_survives_a_glitched_stop_then_start)
{
	audio_property_value_t mute = {.mute = true};

	/* Speaker live (fixture), then cache a pending mute WITHOUT applying it. */
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U, "precondition: speaker live");
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute),
		   "set OUTPUT_MUTE(true) failed");

	/* stop_output() believes it stopped, but its mute write never lands. */
	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	audio_codec_stop_output(codec);
	emul_es8311_fail_write_to(emul, -1);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U,
		      "precondition: the glitched stop left the speaker live");

	/* start_output() must establish started-but-muted, not merely flip the flag. */
	audio_codec_start_output(codec);
	zassert_equal(
		reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		"start_output() with a pending mute must ESTABLISH the muted hardware state -- "
		"the pending mute was lost across a glitched stop/start");
}

/*
 * stop_output() marks the output STOPPED, and a later configure() has to respect that: a route
 * reconfigured after an explicit stop must come up muted, or reconfiguring a stopped stream would
 * silently start the speaker. This is the output_stopped half of the lifecycle split -- distinct
 * from OUTPUT_MUTE, which test_pending_output_mute_survives_start_stop covers.
 */
ZTEST(es8311, test_stop_output_state_survives_configure)
{
	struct audio_codec_cfg cfg;

	audio_codec_stop_output(codec);
	reg_put(ES8311_REG_DAC_MUTE, 0x00);

	make_cfg_16k_16bit(&cfg);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "configure() must re-apply the cached mute state, not unmute");
}

/*
 * configure() leaves the output STOPPED, whatever the lifecycle was before -- so a RECONFIGURE of
 * a started output comes up muted, the lifecycle is left stopped (a later apply_properties() must
 * NOT unmute -- only start_output() may), and a configure() that FAILED while started does not let
 * the retry self-unmute the DAC. That last case is the pre-split hazard: started -> failed
 * reconfigure -> retry -> speaker live with no start_output().
 */
ZTEST(es8311, test_configure_leaves_the_output_stopped)
{
	struct audio_codec_cfg cfg;
	audio_property_value_t vol = {.vol = 0};

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure failed");
	audio_codec_start_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U, "speaker should be live");

	/* Reconfigure a started output: it comes up MUTED, and the lifecycle is left STOPPED. */
	zassert_ok(audio_codec_configure(codec, &cfg), "reconfigure failed");
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "a reconfigure of a started output must come up MUTED");

	/* apply_properties() must not unmute after a configure: the output is stopped. */
	reg_put(ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF);
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol),
		   "set volume failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply failed");
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "apply_properties() must not unmute after a configure: start_output() is the "
		      "first unmute");

	/* And a FAILED configure leaves it stopped too: the retry must not self-unmute. */
	audio_codec_start_output(codec);
	emul_es8311_fail_at(emul, 5);
	zassert_true(audio_codec_configure(codec, &cfg) < 0, "the injected failure must surface");
	emul_es8311_fail_at(emul, -1);
	reg_put(ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF);
	zassert_ok(audio_codec_configure(codec, &cfg), "retry configure failed");
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "after a failed configure, the retry must come up MUTED, not self-unmute");
}

/*
 * start_output()'s error policy matches apply_properties() and tas2563: the lifecycle is marked
 * started only if the unmute SUCCEEDS. An unmute that fails before it lands leaves the output
 * STOPPED, so a later apply_properties() re-mutes rather than unmutes.
 */
ZTEST(es8311, test_start_output_that_fails_before_effect_stays_stopped)
{
	struct audio_codec_cfg cfg;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure failed");

	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	audio_codec_start_output(codec);
	emul_es8311_fail_write_to(emul, -1);

	reg_put(ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF);
	zassert_ok(audio_codec_apply_properties(codec), "apply failed");
	zassert_equal(
		reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		"start_output() whose unmute failed must leave the output STOPPED, so a later "
		"apply_properties() re-mutes rather than unmutes");
}

/*
 * And a start_output() unmute that LANDS then errors is best-effort re-muted, exactly as
 * apply_properties() does -- the DAC ends muted, not live.
 */
ZTEST(es8311, test_start_output_that_lands_then_errors_is_best_effort_remuted)
{
	struct audio_codec_cfg cfg;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure failed");

	emul_es8311_fail_write_landed(emul, ES8311_REG_DAC_MUTE);
	audio_codec_start_output(codec);
	emul_es8311_fail_write_landed(emul, -1);

	zassert_equal(
		reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		"a start_output() unmute that lands-then-errors must be best-effort re-muted");
}

/*
 * INPUT_VOLUME must reach the ADC digital volume register (0x17), using the same
 * linear mapping as the DAC: 0 dB is 0xBF and each dB is two codes.
 */
ZTEST(es8311, test_set_input_volume)
{
	audio_property_value_t val;

	reg_put(ES8311_REG_ADC_VOLUME, 0x00);

	val.vol = 0;
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val),
		   "set INPUT_VOLUME(0 dB) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_ADC_VOLUME), 0xBFU, "0 dB should map to 0xBF");

	val.vol = -6;
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val),
		   "set INPUT_VOLUME(-6 dB) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_ADC_VOLUME), 0xB3U, "-6 dB should map to 0xB3");
}

/*
 * INPUT_MUTE must use the ADC serial data port's own mute bit (0x0A bit 6), not
 * the volume: the ADC volume register bottoms out at -95.5 dB, which is quiet
 * but is not a mute, and collapsing the volume would also destroy the level the
 * caller set.
 */
ZTEST(es8311, test_set_input_mute)
{
	audio_property_value_t mute = {.mute = true};
	audio_property_value_t unmute = {.mute = false};
	audio_property_value_t vol;

	/* Park the input volume somewhere recognisable first. */
	vol.vol = 6;
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol),
		   "set INPUT_VOLUME(+6 dB) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_ADC_VOLUME), 0xCBU, "+6 dB should map to 0xCB");

	reg_put(ES8311_REG_SDP_OUT, 0x0C);
	zassert_ok(
		audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, mute),
		"set INPUT_MUTE(true) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "an input mute must set the ADC serial port mute bit (0x0A bit 6)");
	zassert_equal(reg_get(ES8311_REG_ADC_VOLUME), 0xCBU,
		      "muting the input must not disturb the input volume");

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute),
		   "set INPUT_MUTE(false) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, 0x00U,
		      "unmuting must clear the ADC serial port mute bit");
	zassert_equal(reg_get(ES8311_REG_ADC_VOLUME), 0xCBU,
		      "unmuting must leave the input volume where the caller put it");
}

/*
 * A cached input mute must survive into the ADC sequence configure() emits, the
 * same way the output mute does: reconfiguring a muted input must not silently
 * open the microphone.
 */
ZTEST(es8311, test_input_mute_survives_configure)
{
	struct audio_codec_cfg cfg;
	audio_property_value_t mute = {.mute = true};

	zassert_ok(
		audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, mute),
		"set INPUT_MUTE(true) failed");

	reg_put(ES8311_REG_SDP_OUT, 0x00);

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(CAPTURE) failed");

	zassert_equal(reg_get(ES8311_REG_SDP_OUT), 0x0CU | ES8311_SDP_MUTE,
		      "configure() must re-apply the cached input mute");
}

/*
 * A value set with INPUT_VOLUME before configure() must survive into the ADC
 * sequence that configure() emits, rather than being overwritten by the default.
 */
ZTEST(es8311, test_input_volume_survives_configure)
{
	struct audio_codec_cfg cfg;
	audio_property_value_t vol;

	vol.vol = -12;
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol),
		   "set INPUT_VOLUME(-12 dB) failed");

	reg_put(ES8311_REG_ADC_VOLUME, 0x00);

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(CAPTURE) failed");

	zassert_equal(reg_get(ES8311_REG_ADC_VOLUME), 0xA7U,
		      "configure() must program the cached input volume (-12 dB = 0xA7)");
}

/* A property the codec does not model must be rejected with -ENOTSUP. */
ZTEST(es8311, test_set_property_unsupported)
{
	audio_property_value_t val = {.vol = 0};

	zassert_equal(
		audio_codec_set_property(codec, (audio_property_t)0x7F, AUDIO_CHANNEL_ALL, val),
		-ENOTSUP, "an unknown property must be rejected");
}

/* A channel the mono codec cannot address must be rejected with -EINVAL. */
ZTEST(es8311, test_set_property_invalid_channel)
{
	audio_property_value_t val = {.vol = 0};

	zassert_equal(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       AUDIO_CHANNEL_REAR_LEFT, val),
		      -EINVAL, "a channel the codec has no output for must be rejected");
}

/* An I2C failure during configure() must propagate as an error, and recover. */
ZTEST(es8311, test_configure_propagates_i2c_error)
{
	struct audio_codec_cfg cfg;

	make_cfg_16k_16bit(&cfg);
	emul_es8311_set_fail(emul, 1); /* fail the next transfer */
	zassert_true(audio_codec_configure(codec, &cfg) < 0,
		     "configure() must return an error on I2C failure");

	emul_es8311_set_fail(emul, 0); /* clear injection */
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() should recover");
}

/* An I2C failure while applying properties must propagate too. */
ZTEST(es8311, test_apply_properties_propagates_i2c_error)
{
	emul_es8311_set_fail(emul, 1);
	zassert_true(audio_codec_apply_properties(codec) < 0,
		     "apply_properties() must return an error on I2C failure");

	emul_es8311_set_fail(emul, 0);
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties() should recover");
}

/*
 * The concurrency test. Everything above runs on one thread, so none of it can
 * see the defect this one is here for.
 *
 * apply_properties() must hold the codec lock across its register writes, not
 * merely across the read of the cached state. If it snapshots and unlocks,
 * stop_output() can slip into the gap, cache "muted" and mute the DAC in
 * hardware, and then have that write overwritten by the value apply_properties()
 * sampled before the gap. The driver's cache would say muted while the speaker
 * stayed live, which is a speaker-safety bug rather than a tidiness one.
 *
 * The emulator parks apply_properties() inside its first register write (the DAC
 * volume at 0x32, which stop_output() never touches), still holding whatever
 * lock the driver holds. A second thread then calls stop_output(). With the lock
 * held across the writes it blocks there and wins afterwards; with the lock
 * released early it runs immediately and loses.
 */
static K_THREAD_STACK_DEFINE(apply_stack, 2048);
static K_THREAD_STACK_DEFINE(stop_stack, 2048);
static struct k_thread apply_thread;
static struct k_thread stop_thread;

static void apply_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)audio_codec_apply_properties(codec);
}

/*
 * The stop thread announces itself on both sides of the call, so the test can assert the
 * thing it actually cares about -- that stop_output() is still BLOCKED while the apply thread
 * is parked inside its register write -- instead of inferring it from which write landed last.
 *
 * It runs at a COOPERATIVE priority and the test thread hands over to it with k_yield(), and
 * that pair is what makes the assertion sound rather than merely likely. This used to be a
 * preemptible thread and a k_msleep(50) "to let it reach the mutex", which rested on the
 * argument that the stop thread was the only runnable thread while the test thread slept.
 * True here, and still an argument rather than an oracle.
 *
 * Priority alone would not have fixed it: the ztest thread is ITSELF cooperative
 * (ZTEST_THREAD_PRIORITY defaults to -1), and a cooperative thread is not preempted by
 * anything, however high the other thread's priority. It has to give the CPU up. So the test
 * thread yields, the higher-priority coop stop thread runs, and -- being cooperative -- it
 * keeps running until it BLOCKS. The only reason the test thread gets control back at all is
 * that stop_output() blocked, and the only thing it can block on is the codec mutex.
 */
static K_SEM_DEFINE(stop_started, 0, 1);
static K_SEM_DEFINE(stop_finished, 0, 1);

static void stop_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_give(&stop_started);
	audio_codec_stop_output(codec);
	k_sem_give(&stop_finished);
}

ZTEST(es8311, test_apply_properties_holds_the_lock_across_its_writes)
{
	audio_property_value_t unmute = {.mute = false};
	k_tid_t a;
	k_tid_t b;

	/* Cache "unmuted", and leave the hardware muted so a stale write shows. */
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute),
		   "set OUTPUT_MUTE(false) failed");
	reg_put(ES8311_REG_DAC_MUTE, 0x60);

	emul_es8311_pause_before(emul, ES8311_REG_DAC_VOLUME);

	a = k_thread_create(&apply_thread, apply_stack, K_THREAD_STACK_SIZEOF(apply_stack),
			    apply_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
	zassert_ok(emul_es8311_wait_paused(emul, K_SECONDS(1)),
		   "apply_properties() never reached the DAC volume write");

	k_sem_reset(&stop_started);
	k_sem_reset(&stop_finished);

	b = k_thread_create(&stop_thread, stop_stack, K_THREAD_STACK_SIZEOF(stop_stack), stop_fn,
			    NULL, NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);

	/*
	 * Hand the CPU over. This thread is cooperative, so creating a higher-priority thread
	 * does not preempt it -- it has to yield. The stop thread is cooperative too, so once
	 * it starts it keeps the CPU until it BLOCKS. Therefore, when k_yield() returns here,
	 * stop_fn() has signalled stop_started, called stop_output(), and blocked; or it ran
	 * the whole way through, which is the bug.
	 */
	k_yield();

	zassert_ok(k_sem_take(&stop_started, K_NO_WAIT),
		   "the cooperative stop thread had not run after k_yield(), which it must have");

	/*
	 * THE assertion. The apply thread is parked inside its first register write, and the
	 * stop thread has already entered stop_output(). If the lock is held across the writes,
	 * stop_output() is blocked on it and cannot have finished. If it HAS finished, the lock
	 * was dropped after the snapshot and the two are racing -- which is the bug.
	 */
	zassert_not_equal(k_sem_take(&stop_finished, K_NO_WAIT), 0,
			  "stop_output() completed while apply_properties() was parked "
			  "mid-write: the lock is not held across the register writes");

	emul_es8311_release(emul);
	zassert_ok(k_thread_join(a, K_SECONDS(1)), "the apply_properties() thread hung");
	zassert_ok(k_thread_join(b, K_SECONDS(1)), "the stop_output() thread hung");

	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "stop_output() must win: the speaker must not be left live while the "
		      "driver's cached state says muted");
}

/*
 * configure() must quiesce the part before it moves the clock tree.
 *
 * The clock manager gates the clocks of whichever converter the new route does not
 * carry. Gating the clock of a DAC that is still powered and still unmuted does not
 * silence it: it freezes the modulator on the last sample, which is a DC step into
 * the amplifier. The mutes and the power-downs have to come BEFORE the first write
 * to the clock manager, and the register values at the end cannot show that -- only
 * the write ORDER can.
 */
ZTEST(es8311, test_configure_quiesces_before_it_moves_the_clocks)
{
	struct audio_codec_cfg cfg;
	int clk = -1;
	int sdp_in = -1;
	int sdp_out = -1;
	int dac_mute = -1;
	int dac_pwr = -1;
	int adc_pwr = -1;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	emul_es8311_reset_log(emul);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure must pass");

	for (int i = 0; i < emul_es8311_write_count(emul); i++) {
		int r = emul_es8311_write_at(emul, i);

		if (r == ES8311_REG_CLK_MANAGER && clk < 0) {
			clk = i;
		} else if (r == ES8311_REG_SDP_IN && sdp_in < 0) {
			sdp_in = i;
		} else if (r == ES8311_REG_SDP_OUT && sdp_out < 0) {
			sdp_out = i;
		} else if (r == ES8311_REG_DAC_MUTE && dac_mute < 0) {
			dac_mute = i;
		} else if (r == ES8311_REG_SYSTEM_12 && dac_pwr < 0) {
			dac_pwr = i;
		} else if (r == ES8311_REG_SYSTEM_0E && adc_pwr < 0) {
			adc_pwr = i;
		}
	}

	zassert_true(clk > 0, "the clock manager (0x01) must be written");

	zassert_true(sdp_in >= 0 && sdp_in < clk,
		     "the DAC serial port must be muted before the clocks move "
		     "(0x09 at %d, 0x01 at %d)",
		     sdp_in, clk);
	zassert_true(sdp_out >= 0 && sdp_out < clk,
		     "the ADC serial port must be muted before the clocks move "
		     "(0x0A at %d, 0x01 at %d)",
		     sdp_out, clk);
	zassert_true(dac_mute >= 0 && dac_mute < clk,
		     "the DAC must be muted before the clocks move (0x31 at %d, 0x01 at %d)",
		     dac_mute, clk);
	zassert_true(dac_pwr >= 0 && dac_pwr < clk,
		     "the DAC must be powered down before the clocks move "
		     "(0x12 at %d, 0x01 at %d)",
		     dac_pwr, clk);
	/*
	 * The ADC is NOT powered down in the quiesce and must not be: it drives no
	 * speaker, so it cannot pop, and power-cycling it on every configure leaves a DC
	 * offset that measurably raises the capture noise floor. Muting its serial port
	 * (0x0A, asserted above) is the whole of what it needs.
	 */
	ARG_UNUSED(adc_pwr);
}

/*
 * A configure() that dies part-way through must not leave the driver steering by a
 * route that no longer describes the chip.
 *
 * configure() writes a dozen registers. Break the I2C bus at each one in turn and
 * the hardware is left half reprogrammed: some clocks gated, a converter powered
 * down, an analog reference moved. If the driver still believed the OLD route at
 * that point, start_output(), stop_output() and apply_properties() would go on
 * writing to converters whose power and clocks the failed call had already changed.
 *
 * So after a failed configure() the driver must consider itself to have NO route,
 * and those three must touch nothing at all. That is the only honest state: the
 * hardware is undefined until somebody configures it again.
 *
 * The failure is walked across every transfer, not just the first. Breaking the
 * first one is the easy case, and it is the one a "fail the next transfer" hook can
 * reach; the interesting failures are in the middle of the sequence.
 */
ZTEST(es8311, test_failed_configure_disarms_start_but_never_stop)
{
	struct audio_codec_cfg cfg;
	int covered = 0;

	/* Bounded so a driver change that stops failing can never spin here. */
	for (int n = 0; n < 64; n++) {
		int ret;

		/*
		 * Start from a route that carries BOTH directions, so that a driver
		 * which kept its old route would have a DAC to go on writing to.
		 */
		make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
		zassert_ok(audio_codec_configure(codec, &cfg), "setup configure must pass");

		/* Break transfer n of a switch to capture-only. */
		make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_CAPTURE);
		emul_es8311_fail_at(emul, n);
		ret = audio_codec_configure(codec, &cfg);
		emul_es8311_fail_at(emul, -1);

		if (ret == 0) {
			/* The injection never fired: n is past the end of the sequence. */
			break;
		}

		covered++;

		/*
		 * UNMUTING is gated. The failed call may already have moved the DAC's
		 * power or its clocks, and there is no route to say otherwise, so
		 * start_output() must not put a speaker back on the output.
		 */
		emul_es8311_reset_log(emul);
		audio_codec_start_output(codec);
		zassert_equal(emul_es8311_write_count(emul), 0,
			      "start_output() must not unmute after a configure() that "
			      "failed at transfer %d (it wrote %d register(s))",
			      n, emul_es8311_write_count(emul));

		/*
		 * MUTING is not gated, and this is the case that matters. A configure()
		 * that died on its FIRST register write changed nothing: the old route's
		 * DAC is still powered, still unmuted and still driving the amplifier.
		 * The route cache has been cleared all the same. If stop_output() were
		 * gated on that cache it would do nothing, and the caller reaching for
		 * the off switch would find it disconnected -- a fail-open, and worse
		 * than the stale route it was meant to prevent.
		 *
		 * Seed the mute register to "not muted" so the assertion needs a real
		 * write by the driver.
		 */
		reg_put(ES8311_REG_DAC_MUTE, 0x00U);
		emul_es8311_reset_log(emul);
		audio_codec_stop_output(codec);

		zassert_true(emul_es8311_write_count(emul) > 0,
			     "stop_output() must attempt the mute even with no route "
			     "(configure() failed at transfer %d, and it wrote nothing)",
			     n);
		zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
			      "the DAC must end up muted after a configure() that failed "
			      "at transfer %d: it is the only way to silence a speaker "
			      "that a half-done configure left live",
			      n);
	}

	zassert_true(covered > 1,
		     "the fault injection must have reached more than the first transfer "
		     "(covered %d)",
		     covered);
}

/*
 * THE DEFECT CLASS A CLEAN-CHIP SUITE CANNOT SEE.
 *
 * Every test above starts from an emulator whose register file is all zeros -- which is to
 * say, from a chip in exactly the state the driver would like to find it in. The driver never
 * resets the part (a reset costs seconds of analog settling on the capture path), so on real
 * hardware it inherits its whole register file from whatever ran last: a vendor bootloader,
 * ESP-ADF, an earlier firmware, or its own previous boot. The ES8311 has no reset pin, so not
 * even a warm reboot clears it.
 *
 * A suite whose inputs never vary cannot find a defect that only appears when they do. The
 * tests below vary them.
 */
static void dirty_the_chip(void)
{
	/*
	 * Not arbitrary. Each of these is a state the part can really be handed, and the
	 * first three are the ones that quietly redefine a register the driver DOES write:
	 *
	 *   ALC on  -> the datasheet, under the ADC volume register: "When ALC is on,
	 *              ADC_VOLUME = MAXGAIN". 0x17 stops being a volume.
	 *   DRC on  -> the same claim under the DAC volume register. 0x32 stops being one.
	 *   0x44 b7 -> ADC2DAC_SEL. The DAC plays the ADC instead of the caller's audio.
	 */
	reg_put(ES8311_REG_ADC_ALC, ES8311_ALC_EN);
	reg_put(ES8311_REG_DAC_DRC, ES8311_DRC_EN);
	reg_put(ES8311_REG_ADC_MUX, ES8311_ADC2DAC_SEL);
	/*
	 * 0x09 bit 7 is SDP_IN_SEL: which half of the stereo frame the mono DAC plays. A
	 * previous firmware that picked the right slot leaves it set, and a driver that writes
	 * 0x09 read-modify-write, or writes only the low bits, keeps playing the right slot --
	 * silently, on an application that puts its samples in the left one.
	 */
	reg_put(ES8311_REG_SDP_IN, ES8311_SDP_IN_SEL_RIGHT);
	reg_put(ES8311_REG_LOW_POWER, 0xFF); /* every low-power mode, incl. LPDAC */
	reg_put(ES8311_REG_PWRUP_C, 0x20);   /* the power-on default, PWRUP_C = 32 */
	reg_put(ES8311_REG_PWRUP_AB, 0xFF);
	reg_put(ES8311_REG_ADC_ALC_LVL, 0xFF);
	reg_put(ES8311_REG_ADC_AUTOMUTE, 0xFF);
	reg_put(ES8311_REG_DAC_OFFSET, 0xFF); /* a DC offset into the amplifier */
	reg_put(ES8311_REG_DAC_DRC_LVL, 0xFF);
}

ZTEST(es8311, test_configure_normalizes_a_dirty_chip)
{
	struct audio_codec_cfg cfg;

	dirty_the_chip();

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	zassert_equal(reg_get(ES8311_REG_ADC_ALC), 0x00U,
		      "ALC left on: 0x17 is a servo ceiling, not the volume the driver set");
	zassert_equal(reg_get(ES8311_REG_DAC_DRC), 0x00U,
		      "DRC left on: 0x32 is a compressor ceiling, not the volume");
	zassert_equal(reg_get(ES8311_REG_ADC_MUX) & ES8311_ADC2DAC_SEL, 0x00U,
		      "ADC2DAC_SEL left set: the DAC plays the microphone, not the caller");
	zassert_equal(reg_get(ES8311_REG_SDP_IN) & ES8311_SDP_IN_SEL_RIGHT, 0x00U,
		      "SDP_IN_SEL left set: the mono DAC is still playing the RIGHT slot of the "
		      "frame, so an application that fills the left one hears silence");
	zassert_equal(reg_get(ES8311_REG_LOW_POWER), 0x00U, "low-power modes left on");
	zassert_equal(reg_get(ES8311_REG_PWRUP_C), 0x00U, "PWRUP_C left at its default");
	zassert_equal(reg_get(ES8311_REG_PWRUP_AB), 0x00U, "PWRUP_A/B left dirty");
	zassert_equal(reg_get(ES8311_REG_DAC_OFFSET), 0x00U, "a DC offset left on the DAC");
	zassert_equal(reg_get(ES8311_REG_ADC_ALC_LVL), 0x00U, "ALC levels left dirty");
	zassert_equal(reg_get(ES8311_REG_ADC_AUTOMUTE), 0x00U, "automute left dirty");
	zassert_equal(reg_get(ES8311_REG_DAC_DRC_LVL), 0x00U, "DRC levels left dirty");
}

ZTEST(es8311, test_playback_only_still_clears_adc2dac_sel)
{
	struct audio_codec_cfg cfg;

	/*
	 * 0x44 used to be written only on the capture path, on the reasonable-looking
	 * grounds that ADCDAT_SEL is about the ADC. But bit 7 of the same register is
	 * ADC2DAC_SEL, and that is a PLAYBACK control. So a playback-only configure() never
	 * touched it: a chip handed over with bit 7 set played its own microphone instead of
	 * the caller's audio, through a route that had just powered that microphone down --
	 * silently, with configure() returning 0 and every register the driver checked
	 * reading back exactly as intended, and with no way out on any later call unless
	 * capture happened to be routed once.
	 */
	reg_put(ES8311_REG_ADC_MUX, ES8311_ADC2DAC_SEL);

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	zassert_equal(reg_get(ES8311_REG_ADC_MUX) & ES8311_ADC2DAC_SEL, 0x00U,
		      "a playback-only route left ADC2DAC_SEL set: the speaker plays the mic");
}

ZTEST(es8311, test_ini_reg_is_released_before_anything_else)
{
	struct audio_codec_cfg cfg;

	/*
	 * 0xFA bit 0 is a LEVEL, not a pulse: while it is set the register file is held at
	 * its defaults, every write is silently discarded, and every read returns 0x00.
	 * The vendor Linux driver sets it at shutdown ON PURPOSE, to hold the file down
	 * across a reboot. A driver that does not clear it can be handed a chip it cannot
	 * recover -- and cannot even tell it has been, because configure() still returns 0.
	 *
	 * So it has to be the FIRST write, before the mutes: on a held chip every write
	 * after it would go nowhere.
	 */
	emul_es8311_reset_log(emul);

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	zassert_equal(emul_es8311_write_at(emul, 0), ES8311_REG_INI,
		      "0xFA must be the first write: on a held chip nothing after it lands");
	zassert_equal(reg_get(ES8311_REG_INI), 0x00U, "0xFA bit 0 left set");
}

ZTEST(es8311, test_the_driver_never_resets_the_register_file)
{
	struct audio_codec_cfg cfg;
	int writes;

	/*
	 * The one thing that must NOT come back.
	 *
	 * Both reference drivers open with 0x00 = 0x1F, and this driver deliberately does
	 * not: measured on hardware, the capture noise floor is then exactly zero for about
	 * six seconds, at every sample rate. (Why it costs six seconds is measured and not
	 * explained. A cold power-on, which charges the same analog reference capacitors
	 * from zero, is alive at the earliest moment it can be sampled -- so "the capacitors
	 * are recharging" is refuted, and the reset puts the part somewhere a cold start
	 * never goes.) Deviating from every reference implementation is exactly the kind of
	 * thing a future contributor helpfully undoes, and the part recovers a few seconds
	 * later, so the cost is invisible to any test that is not looking for it.
	 *
	 * This looks at what the driver WROTE. It deliberately does not look for a wiped
	 * register file: RST_DIG is "reset digital except control port block", so no value
	 * of 0x00 resets the register file on the real part, and an emulator that pretended
	 * otherwise would be testing a chip that does not exist.
	 */
	emul_es8311_reset_log(emul);

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	writes = emul_es8311_write_count(emul);
	zassert_true(writes > 0, "configure() wrote nothing");

	for (int i = 0; i < writes; i++) {
		if (emul_es8311_write_at(emul, i) != ES8311_REG_RESET) {
			continue;
		}

		zassert_equal(emul_es8311_wval_at(emul, i) & ES8311_RESET_BITS, 0x00U,
			      "write %d asserted reset bits in 0x00 (value 0x%02x). That costs "
			      "about six seconds of a deaf ADC, at every sample rate. This "
			      "driver reaches a known register state by writing it, not by "
			      "resetting. See the binding.",
			      i, emul_es8311_wval_at(emul, i));
	}

	zassert_equal(reg_get(ES8311_REG_RESET) & ES8311_RESET_BITS, 0x00U,
		      "0x00 must be left at CSM_ON with no reset bit set");
}

/*
 * THE FIXTURE HAS TO RESET BOTH SIDES, AND IT USED TO RESET ONE.
 *
 * emul_es8311_reset() puts the emulated CHIP back to a clean register file, but it cannot touch the
 * DRIVER: dac_volume_code, adc_volume_code, output_mute, adc_mute and output_stopped live in struct
 * es8311_data and persist across tests, so a test that left the output muted (or, since the split,
 * STOPPED) would hand that to every test after it. This fixture resets the four properties through
 * the PUBLIC API and re-starts the output with start_output() after the configure(), so every test
 * downstream sees the same configured, unmuted codec -- without reaching into driver private state.
 *
 * The precondition is ASSERTED, not attempted: a setup that silently fails would run against
 * whatever it inherited, and some inherited states make the next test pass for the wrong reason.
 */
static void es8311_before(void *fixture)
{
	static const struct {
		audio_property_t prop;
		audio_property_value_t val;
	} defaults[] = {
		{AUDIO_PROPERTY_OUTPUT_VOLUME, {.vol = 0}},
		{AUDIO_PROPERTY_INPUT_VOLUME, {.vol = 0}},
		{AUDIO_PROPERTY_OUTPUT_MUTE, {.mute = false}},
		{AUDIO_PROPERTY_INPUT_MUTE, {.mute = false}},
	};
	struct audio_codec_cfg cfg;

	ARG_UNUSED(fixture);

	emul_es8311_reset(emul);

	for (size_t i = 0; i < ARRAY_SIZE(defaults); i++) {
		zassert_ok(audio_codec_set_property(codec, defaults[i].prop, AUDIO_CHANNEL_ALL,
						    defaults[i].val),
			   "test precondition failed: could not reset property %d",
			   (int)defaults[i].prop);
	}

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg),
		   "test precondition failed: configure() did not succeed, so whatever this "
		   "test goes on to check, it is not checking it against a configured codec");

	/* Clear a stopped lifecycle left by a prior test: start_output() sets output_stopped
	 * false through the public API, restoring the started, unmuted state the fixture always
	 * handed downstream tests.
	 */
	audio_codec_start_output(codec);
}

/*
 * init() HAS TO PUT BOTH HALVES OF THE PART SOMEWHERE SAFE, OR NOT SAY THAT IT DOES.
 *
 * Muting SDP_OUT silences the DIGITAL output and does nothing whatever to the analog front
 * end. A part that was capturing when the reboot hit still has its PGA powered and MIC1
 * wired into it.
 */
ZTEST(es8311, test_init_quiesces_the_microphone_not_just_the_speaker)
{
	uint8_t reg = 0U;
	struct audio_codec_cfg cfg_warm;

	/*
	 * A warm reboot out of a capture route.
	 *
	 * The SoC resets and its I2S peripheral stops the bit clock; the codec does not
	 * reset, because it has no reset pin. So the part comes up with its PGA powered and
	 * MIC1 still selected into it, at whatever gain the last firmware chose. Muting the
	 * serial output silences the digital path and leaves all of that running.
	 *
	 * init() has to put BOTH halves of the part somewhere safe, or it should not claim
	 * to put the part somewhere safe.
	 */
	zassert_false(device_is_ready(codec_warm), "the deferred instance must not be up yet");

	zassert_ok(i2c_reg_write_byte_dt(&es_warm, ES8311_REG_SYSTEM_0E, 0x02U),
		   "seed: PGA and modulator powered");
	zassert_ok(i2c_reg_write_byte_dt(&es_warm, ES8311_REG_ADC_PGA, 0x1AU),
		   "seed: MIC1 selected, PGA at its 30 dB maximum");

	zassert_ok(device_init(codec_warm), "init failed");
	zassert_true(device_is_ready(codec_warm), "device must be ready");

	zassert_ok(i2c_reg_read_byte_dt(&es_warm, ES8311_REG_SYSTEM_0E, &reg), "read failed");
	zassert_equal(reg, 0x62U,
		      "init() left the ADC powered: a part that was capturing when the reboot "
		      "hit still has a live PGA on a live microphone");

	zassert_ok(i2c_reg_read_byte_dt(&es_warm, ES8311_REG_ADC_PGA, &reg), "read failed");
	zassert_equal(reg, ES8311_ADC_MIC_OFF, "init() left MIC1 selected into the PGA");

	/*
	 * And fresh from init() the output is STOPPED: a configure() with no start_output() leaves
	 * the DAC muted. output_stopped defaults true and configure() is not start_output(). This
	 * is a device the fixture's start_output() has never touched, so it pins the init DEFAULT
	 * itself, not just the stop_output() path.
	 */
	zassert_ok(i2c_reg_write_byte_dt(&es_warm, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF),
		   "seed the DAC unmuted so only a real mute can silence it");
	make_cfg(&cfg_warm, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec_warm, &cfg_warm), "configure(codec_warm) failed");
	zassert_ok(i2c_reg_read_byte_dt(&es_warm, ES8311_REG_DAC_MUTE, &reg), "read failed");
	zassert_equal(reg & 0x60U, 0x60U,
		      "fresh from init(), configure() must leave the DAC muted: output_stopped "
		      "defaults true and configure() is not start_output()");
}

ZTEST(es8311, test_configure_rejects_a_gated_bit_clock)
{
	struct audio_codec_cfg cfg;

	/*
	 * This codec's master clock IS the bit clock. A controller that gates BCLK when the
	 * TX queue is idle stops the codec's entire clock tree, freezing the DAC modulator
	 * on its last sample: a DC level on the amplifier, re-created on every underrun, at
	 * a moment the driver is never told about. It has to be refused, not ignored.
	 */
	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.options |= I2S_OPT_BIT_CLK_GATED;

	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
		      "a gated bit clock must be rejected: it stops the codec's master clock");

	/* And the continuous bit clock, which is the same bit cleared, must still work. */
	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.options |= I2S_OPT_BIT_CLK_CONT;
	zassert_ok(audio_codec_configure(codec, &cfg), "a continuous bit clock must be accepted");
}

/*
 * THE OFF SWITCH HAS TO WORK ON A BUS THAT IS ONLY HALF WORKING.
 *
 * A read on this bus is a write-then-read with a repeated start. A controller, a device, or
 * a bit of noise can break that while a plain two-byte register write still gets through.
 *
 * A driver that mutes with a read-modify-write cannot survive it. i2c_reg_update_byte_dt()
 * reads first and returns on a failed read WITHOUT attempting the write, so the DAC stays
 * exactly as loud as it was and the caller is handed an error for an operation that was
 * never made. That is the wrong failure mode for the one call whose entire job is to make
 * the speaker stop.
 */
ZTEST(es8311, test_stop_output_mutes_even_when_every_read_fails)
{
	struct audio_codec_cfg cfg;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	audio_codec_start_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_OFF,
		      "precondition: the DAC must be unmuted before we try to mute it");

	/* Reads die. Writes still work. */
	emul_es8311_fail_reads(emul, true);
	audio_codec_stop_output(codec);
	emul_es8311_fail_reads(emul, false);

	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_ON,
		      "stop_output() did not mute. It read the register first, the read failed, "
		      "and it never attempted the write. The DAC is still unmuted and the "
		      "speaker is still playing.");
}

/*
 * The same for the microphone. apply_properties() mutes the ADC at its serial port, and it
 * must not depend on being able to read that port back first either.
 */
ZTEST(es8311, test_input_mute_survives_a_bus_that_cannot_be_read)
{
	struct audio_codec_cfg cfg;
	audio_property_value_t val;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	val.mute = false;
	zassert_ok(
		audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, val),
		"unmute failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT,
		      "precondition: the microphone unmuted, and the port format intact");

	val.mute = true;
	zassert_ok(
		audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, val),
		"mute failed");

	emul_es8311_fail_reads(emul, true);
	(void)audio_codec_apply_properties(codec);
	emul_es8311_fail_reads(emul, false);

	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE,
		      "either the microphone was not muted, or the full write lost the port "
		      "format. A read-modify-write would have given up when the read failed.");
}

/*
 * The I2S_OPT_*_CLK role flags are ENDPOINT-RELATIVE (see the clock-role comment in configure()):
 * the codec-local role for this part is always TARGET on both clocks, because it never drives BCLK
 * or LRCK. samples/drivers/i2s/i2s_codec passes exactly TARGET|TARGET to audio_codec_configure()
 * when the SoC supplies the clock; CONTROLLER on either clock would ask this codec to drive a clock
 * it cannot, leaving the link driven by nobody while configure() returned 0. wm8904 reads the same
 * flag the same way. These four cases pin that contract.
 */
ZTEST(es8311, test_codec_clock_target_is_accepted)
{
	struct audio_codec_cfg cfg;

	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	zassert_ok(audio_codec_configure(codec, &cfg),
		   "TARGET|TARGET is the codec-local role the SoC-clocked sample passes");
}

ZTEST(es8311, test_codec_clock_controller_is_rejected)
{
	struct audio_codec_cfg cfg;

	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	zassert_equal(
		audio_codec_configure(codec, &cfg), -ENOTSUP,
		"CONTROLLER|CONTROLLER asks this codec to drive BCLK and LRCK, which it cannot");
}

ZTEST(es8311, test_mixed_bit_target_frame_controller_is_rejected)
{
	struct audio_codec_cfg cfg;

	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_CONTROLLER;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
		      "a frame-clock CONTROLLER role still asks this codec to drive LRCK");
}

ZTEST(es8311, test_mixed_bit_controller_frame_target_is_rejected)
{
	struct audio_codec_cfg cfg;

	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_TARGET;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
		      "a bit-clock CONTROLLER role still asks this codec to drive BCLK");
}

/*
 * A SAFETY QUIESCE THAT GIVES UP ON ITS FIRST FAILURE IS NOT A SAFETY QUIESCE.
 *
 * init() exists because a warm reboot does not reach this codec: it can come up with a
 * powered, unmuted DAC sitting on the amplifier and a live PGA on a live microphone. Every
 * write it makes is there to end one of those.
 *
 * So a single transient error on the FIRST of them must not skip the rest. Fail-fast here
 * would abandon the DAC mute, the DAC power-down, the ADC power-down and the microphone
 * disconnect -- and it would do it precisely when the bus is already misbehaving, which is
 * the worst possible moment to walk away from a live speaker.
 */
ZTEST(es8311, test_init_finishes_quiescing_even_when_a_safety_write_fails)
{
	uint8_t reg = 0U;

	zassert_false(device_is_ready(codec_quiesce), "the deferred instance must not be up yet");

	/* The part comes up as a previous firmware left it: playing, and listening. */
	zassert_ok(i2c_reg_write_byte_dt(&es_quiesce, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF),
		   "seed failed");
	zassert_ok(i2c_reg_write_byte_dt(&es_quiesce, ES8311_REG_SYSTEM_12, 0x00U), "seed failed");
	zassert_ok(i2c_reg_write_byte_dt(&es_quiesce, ES8311_REG_SYSTEM_0E, 0x02U), "seed failed");
	zassert_ok(i2c_reg_write_byte_dt(&es_quiesce, ES8311_REG_ADC_PGA, 0x1AU), "seed failed");

	/*
	 * And one of its safety writes will fail: the DAC serial-port mute, which the quiesce now
	 * emits THIRD (after the DAC mute and the microphone port -- the priority order the
	 * dedicated test_quiesce_mutes_the_speaker_first pins). This test is about continuation,
	 * not order: a failure part-way through must not abandon the writes after it.
	 */
	emul_es8311_fail_write_to(emul_quiesce, ES8311_REG_SDP_IN);

	zassert_not_equal(device_init(codec_quiesce), 0,
			  "init() must still report the failure it hit");

	emul_es8311_fail_write_to(emul_quiesce, -1);

	/*
	 * And every safety write AFTER the one that failed must have been attempted anyway.
	 * This is the whole point: the speaker is quiet and the microphone is disconnected,
	 * even though the bus hiccuped on the way there.
	 */
	zassert_ok(i2c_reg_read_byte_dt(&es_quiesce, ES8311_REG_DAC_MUTE, &reg), "read failed");
	zassert_equal(reg, ES8311_DAC_MUTE_ON,
		      "the DAC was left UNMUTED: init() gave up after the serial-port write "
		      "failed and never attempted the mute");

	zassert_ok(i2c_reg_read_byte_dt(&es_quiesce, ES8311_REG_SYSTEM_12, &reg), "read failed");
	zassert_equal(reg, 0x02U, "the DAC was left POWERED");

	zassert_ok(i2c_reg_read_byte_dt(&es_quiesce, ES8311_REG_SYSTEM_0E, &reg), "read failed");
	zassert_equal(reg, 0x62U, "the ADC was left POWERED, on a live microphone");

	zassert_ok(i2c_reg_read_byte_dt(&es_quiesce, ES8311_REG_ADC_PGA, &reg), "read failed");
	zassert_equal(reg, ES8311_ADC_MIC_OFF, "MIC1 was left connected to the PGA");
}

/*
 * THE ONE A CLEAN EMULATOR COULD NOT SEE.
 *
 * The vendor Linux driver asserts 0xFA bit 0 at shutdown, deliberately, to hold the register
 * file at its defaults across a reboot. A part handed over in that state answers 0x00 to
 * every read, chip-id registers included, and discards every write except one: a write to
 * 0xFA itself, which is the only way out.
 *
 * So a driver that reads the chip id BEFORE releasing 0xFA sees 0x0000, concludes it is not
 * talking to an ES8311, returns -ENODEV, and never issues the one write that would have
 * recovered the part. The chip is then unrecoverable by that driver, permanently, and no
 * amount of rebooting helps.
 *
 * This test is why the emulator models INI_REG as a level. Without that, the emulator cannot
 * express the state -- and a test cannot catch a bug in a state its model cannot express.
 */
ZTEST(es8311, test_init_recovers_a_chip_left_holding_its_register_file)
{
	uint8_t id1 = 0U;
	uint8_t id2 = 0U;
	int ret;

	zassert_false(device_is_ready(codec_held),
		      "the deferred instance must not have been initialised at boot");
	/* Exactly what a previous firmware leaves behind. */
	emul_es8311_set_ini_hold(emul_held, true);

	/* Prove the hold is real before asking the driver to survive it. */
	zassert_ok(i2c_reg_read_byte_dt(&es_held, ES8311_REG_CHIP_ID1, &id1), "i2c read failed");
	zassert_ok(i2c_reg_read_byte_dt(&es_held, ES8311_REG_CHIP_ID2, &id2), "i2c read failed");
	zassert_equal(id1, 0x00U, "a held part must answer 0x00, not 0x%02x", id1);
	zassert_equal(id2, 0x00U, "a held part must answer 0x00, not 0x%02x", id2);

	ret = device_init(codec_held);
	zassert_ok(ret,
		   "init() failed (%d) on a chip whose register file was held. The driver "
		   "read the chip id before releasing 0xFA, saw 0x0000, and gave up -- so "
		   "it never issued the one write that recovers the part.",
		   ret);
	zassert_true(device_is_ready(codec_held), "the device must be ready after recovery");

	/* And it must actually be released, not merely tolerated. */
	zassert_ok(i2c_reg_read_byte_dt(&es_held, ES8311_REG_CHIP_ID1, &id1), "i2c read failed");
	zassert_ok(i2c_reg_read_byte_dt(&es_held, ES8311_REG_CHIP_ID2, &id2), "i2c read failed");
	zassert_equal(id1, 0x83U, "0xFA is still asserted: the part is still held");
	zassert_equal(id2, 0x11U, "0xFA is still asserted: the part is still held");
}

/*
 * NOTHING THAT CAN FAIL MAY HAPPEN AFTER THE MICROPHONE IS OPENED.
 *
 * configure() leaves the DAC muted -- start_output() unmutes the speaker -- so the only converter
 * a configure() opens is the microphone. Its serial port must be the LAST write, and 0x44
 * (ADC2DAC_SEL, which wires the ADC into the DAC) must be normalised before it, or a chip handed
 * over with that bit set would route its microphone into the path in the window. Register values
 * at the end cannot show ordering; only the write ORDER can.
 */
ZTEST(es8311, test_configure_opens_the_microphone_last_of_all)
{
	struct audio_codec_cfg cfg;
	int n;
	int mux = -1;

	/* A chip handed over with the ADC wired into the DAC. */
	reg_put(ES8311_REG_ADC_MUX, ES8311_ADC2DAC_SEL);

	emul_es8311_reset_log(emul);
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	n = emul_es8311_write_count(emul);
	zassert_true(n >= 3, "configure() emitted only %d writes", n);

	for (int i = 0; i < n; i++) {
		if (emul_es8311_write_at(emul, i) == ES8311_REG_ADC_MUX) {
			mux = i;
		}
	}

	/* The last three writes: DAC serial port, DAC mute (held ON), microphone port (opened). */
	zassert_equal(emul_es8311_write_at(emul, n - 3), ES8311_REG_SDP_IN,
		      "write %d should be the DAC serial port (0x09), not 0x%02x", n - 3,
		      emul_es8311_write_at(emul, n - 3));
	zassert_equal(emul_es8311_write_at(emul, n - 2), ES8311_REG_DAC_MUTE,
		      "write %d should be the DAC mute (0x31), not 0x%02x", n - 2,
		      emul_es8311_write_at(emul, n - 2));
	zassert_equal(
		emul_es8311_write_at(emul, n - 1), ES8311_REG_SDP_OUT,
		"the LAST write of a configure() must be the microphone port (0x0A): it is the "
		"only converter configure() opens. It was 0x%02x",
		emul_es8311_write_at(emul, n - 1));

	/* The DAC is held muted; the microphone port is opened. */
	zassert_equal(emul_es8311_wval_at(emul, n - 2), ES8311_DAC_MUTE_ON,
		      "the DAC must be left MUTED by configure()");
	zassert_equal(emul_es8311_wval_at(emul, n - 1) & ES8311_SDP_MUTE, 0x00U,
		      "the microphone serial port was not opened");

	zassert_true(
		mux >= 0 && mux < n - 1,
		"0x44 was normalised at write %d but the microphone opened at %d: ADC2DAC_SEL is "
		"still set in that window",
		mux, n - 1);
}

/*
 * A FAILED configure() LEAVES THE CODEC SILENT, NOT MERELY UNDESCRIBED.
 *
 * configure() clears its route cache before it touches the chip, because a half-reprogrammed
 * part is not described by the old route any more. Necessary -- and, on its own, a fail-OPEN.
 * It makes the driver FORGET a converter, not STOP one.
 *
 * Take a live capture route and a configure() whose first write fails. The hardware is
 * untouched: the ADC is powered, the PGA is live, MIC1 is still wired into it. The route
 * cache now says there is no capture, so apply_properties() will not go near the ADC.
 * stop_output() only mutes the DAC. And the audio_codec API has no stop_input() at all. The
 * microphone is left running, with no call in the API able to switch it off.
 *
 * So this walks the failure across every transfer and checks the HARDWARE, before calling
 * anything else. That distinction is the whole test: the neighbouring
 * test_failed_configure_disarms_start_but_never_stop asks whether the CALLER can recover the
 * part afterwards, and answers yes -- for the speaker. This asks what configure() itself left
 * behind, which for the microphone is the only question there is.
 */
/*
 * The walk is bounded so that a driver change which stops failing can never spin here. The
 * bound is not the oracle, though: if configure() ever grows past it, the loop would end
 * quietly having covered only a prefix, and a test called "every transfer" would go on saying
 * so. So the walk must be seen to run PAST the end of the sequence -- that is what
 * reached_the_end is for.
 */
#define FAULT_WALK_BOUND 64

static void walk_every_failure_into(audio_route_t target, const char *name)
{
	struct audio_codec_cfg cfg;
	bool reached_the_end = false;
	int covered = 0;

	for (int n = 0; n < FAULT_WALK_BOUND; n++) {
		int ret;

		/* Come from a live full-duplex route: a playing speaker and an open mic.
		 * configure() leaves the DAC muted, so start_output() is what makes the speaker
		 * live.
		 */
		make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
		zassert_ok(audio_codec_configure(codec, &cfg), "setup configure must pass");
		audio_codec_start_output(codec);
		zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_OFF,
			      "setup: the DAC must be live before we break anything");
		zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x1AU,
			      "setup: MIC1 must be on the PGA mux before we break anything");

		/* Break transfer n of the switch to the target route. */
		make_cfg(&cfg, AUDIO_PCM_RATE_16K, target);
		emul_es8311_fail_at(emul, n);
		ret = audio_codec_configure(codec, &cfg);
		emul_es8311_fail_at(emul, -1);

		if (ret == 0) {
			/* The injection never fired: n is past the end of the sequence. */
			reached_the_end = true;
			break;
		}

		covered++;

		/*
		 * NO OTHER DRIVER CALL HAPPENS BEFORE THESE. The part must already be safe.
		 */
		zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_ON,
			      "-> %s: configure() failed at transfer %d and left the DAC UNMUTED",
			      name, n);
		zassert_equal(reg_get(ES8311_REG_SYSTEM_12), 0x02U,
			      "-> %s: configure() failed at transfer %d and left the DAC POWERED",
			      name, n);
		zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x62U,
			      "-> %s: configure() failed at transfer %d and left the ADC POWERED, "
			      "on a microphone no API call can now switch off",
			      name, n);
		zassert_equal(reg_get(ES8311_REG_ADC_PGA), ES8311_ADC_MIC_OFF,
			      "-> %s: configure() failed at transfer %d and left MIC1 wired into "
			      "the PGA",
			      name, n);
		zassert_equal(reg_get(ES8311_REG_SDP_IN) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
			      "-> %s: configure() failed at transfer %d and left the DAC port open",
			      name, n);
		zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
			      "-> %s: configure() failed at transfer %d and left the ADC port open",
			      name, n);
	}

	zassert_true(reached_the_end,
		     "-> %s: the walk hit its bound of %d transfers without ever getting past the "
		     "end of the sequence. It covered a PREFIX, not every transfer, and a test "
		     "that says otherwise is lying. Raise FAULT_WALK_BOUND.",
		     name, FAULT_WALK_BOUND);

	zassert_true(covered > 1,
		     "-> %s: the fault injection must have reached more than the first transfer "
		     "(covered %d)",
		     name, covered);
}

ZTEST(es8311, test_a_failed_configure_leaves_the_codec_silent)
{
	/*
	 * Every target route, not just one. Each programs a different branch (the DAC
	 * power-up, output mode, volume and EQ only exist on the playback path), so each has a
	 * different number of transfers to break and a different commit block at the end.
	 */
	walk_every_failure_into(AUDIO_ROUTE_CAPTURE, "CAPTURE");
	walk_every_failure_into(AUDIO_ROUTE_PLAYBACK, "PLAYBACK");
	walk_every_failure_into(AUDIO_ROUTE_PLAYBACK_CAPTURE, "PLAYBACK_CAPTURE");
}

/*
 * A MUTE MUST NOT BE CANCELLED BY A VOLUME.
 *
 * `set_property(OUTPUT_MUTE, true)` + `apply_properties()` is the API's way of silencing the
 * speaker -- the other one, next to stop_output(). It used to run DAC volume first and bail
 * out on its error, so a failed write to the DAC VOLUME register, which is not a safety
 * register and has nothing to do with the request, threw the mute away. The caller asked for
 * silence, got an error back, and the speaker went on playing.
 *
 * The fault is injected BY REGISTER, not by transfer index: "break the write to the DAC
 * volume" keeps meaning that when the order of the sequence changes, and "break transfer 3"
 * does not.
 */
ZTEST(es8311, test_output_mute_survives_a_failed_volume_write)
{
	audio_property_value_t mute = {.mute = true};

	/* A live speaker. */
	audio_codec_start_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_OFF,
		      "precondition: the DAC must be unmuted before we try to mute it");

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute),
		   "set OUTPUT_MUTE(true) failed");

	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_VOLUME);
	zassert_true(audio_codec_apply_properties(codec) < 0,
		     "apply_properties() must report the failed volume write");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_ON,
		      "the DAC is STILL PLAYING. apply_properties() gave up on the volume write "
		      "and never attempted the mute the caller actually asked for.");
}

/* The same for the microphone: an ADC volume failure must not cancel an input mute. */
ZTEST(es8311, test_input_mute_survives_a_failed_volume_write)
{
	audio_property_value_t mute = {.mute = true};

	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT,
		      "precondition: the microphone must be open before we try to mute it");

	zassert_ok(
		audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, mute),
		"set INPUT_MUTE(true) failed");

	emul_es8311_fail_write_to(emul, ES8311_REG_ADC_VOLUME);
	zassert_true(audio_codec_apply_properties(codec) < 0,
		     "apply_properties() must report the failed volume write");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE,
		      "the microphone is STILL OPEN: the ADC volume write failed and the mute "
		      "was never attempted");
}

/*
 * And the converse, which is the same rule read the other way round: an UNMUTE may not
 * outrun a failed volume write. Unmuting into a volume register whose write just failed is
 * unmuting into an unknown level -- on the DAC side, a speaker at whatever gain the last
 * firmware chose.
 */
ZTEST(es8311, test_output_unmute_does_not_outrun_a_failed_volume_write)
{
	audio_property_value_t unmute = {.mute = false};
	audio_property_value_t loud = {.vol = 32};

	audio_codec_stop_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_ON,
		      "precondition: the DAC must be muted");

	/* The caller asks to come back at full scale, and the volume write dies. */
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    loud),
		   "set OUTPUT_VOLUME failed");
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute),
		   "set OUTPUT_MUTE(false) failed");

	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_VOLUME);
	zassert_true(audio_codec_apply_properties(codec) < 0, "apply_properties() must report it");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_ON,
		      "the DAC was unmuted even though the volume it was supposed to come back "
		      "at never landed");
}

/*
 * A failure in one direction must not suppress the safety mute in the other. The playback
 * block used to come first and `goto end` on its error, so a DAC volume glitch left the
 * MICROPHONE open -- two directions, one shared exit, and the caller had asked for both.
 */
ZTEST(es8311, test_a_failure_on_one_direction_still_mutes_the_other)
{
	audio_property_value_t mute = {.mute = true};

	audio_codec_start_output(codec);
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT,
		      "precondition: full duplex, and the microphone is open");

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute),
		   "set OUTPUT_MUTE(true) failed");
	zassert_ok(
		audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, mute),
		"set INPUT_MUTE(true) failed");

	/* The DAC's own mute is the write that dies. The microphone's must still happen. */
	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	zassert_true(audio_codec_apply_properties(codec) < 0, "apply_properties() must report it");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE,
		      "the microphone was left OPEN because the speaker's mute failed. Two "
		      "safety writes, and one of them cancelled the other.");
}

/*
 * A FAILED MUTE IN ONE DIRECTION MUST NOT LET THE OTHER DIRECTION BE TURNED UP.
 *
 * The cross-direction version of "a failed mute must not be followed by more gain". The caller
 * mutes the microphone and, in the same batch, raises the speaker. The microphone mute fails.
 * The speaker's volume write must NOT happen -- a failed safety write anywhere stops every
 * gain write in the call, not just the one in its own direction. Otherwise a failed mic mute
 * comes out the far side as a louder speaker, which is precisely "a failure made it worse".
 */
ZTEST(es8311, test_a_failed_input_mute_does_not_turn_the_speaker_up)
{
	audio_property_value_t mute = {.mute = true};
	audio_property_value_t loud = {.vol = 32};

	audio_codec_start_output(codec);
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT,
		      "precondition: full duplex, microphone open");

	reg_put(ES8311_REG_DAC_VOLUME, 0xBFU); /* speaker at 0 dB in hardware */

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    loud),
		   "set OUTPUT_VOLUME(+32 dB) failed");
	zassert_ok(
		audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, mute),
		"set INPUT_MUTE(true) failed");

	/* The microphone mute (SDP_OUT) dies. The DAC volume must not be written after it. */
	emul_es8311_fail_write_to(emul, ES8311_REG_SDP_OUT);
	zassert_true(audio_codec_apply_properties(codec) < 0, "apply must report the failed mute");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_DAC_VOLUME), 0xBFU,
		      "the microphone's mute failed and the SPEAKER was turned up to 0x%02x "
		      "anyway -- a failure in one direction made the other one worse",
		      reg_get(ES8311_REG_DAC_VOLUME));
}

/* And the mirror: a failed output mute must not let the microphone gain be raised. */
ZTEST(es8311, test_a_failed_output_mute_does_not_turn_the_microphone_up)
{
	audio_property_value_t mute = {.mute = true};
	audio_property_value_t loud = {.vol = 32};

	audio_codec_start_output(codec);
	reg_put(ES8311_REG_ADC_VOLUME, 0xBFU); /* microphone at 0 dB in hardware */

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    loud),
		   "set INPUT_VOLUME(+32 dB) failed");
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute),
		   "set OUTPUT_MUTE(true) failed");

	/* The speaker mute (DAC_MUTE) dies. The ADC volume must not be written after it. */
	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	zassert_true(audio_codec_apply_properties(codec) < 0, "apply must report the failed mute");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_ADC_VOLUME), 0xBFU,
		      "the speaker's mute failed and the MICROPHONE gain was raised to 0x%02x "
		      "anyway",
		      reg_get(ES8311_REG_ADC_VOLUME));
}

/*
 * IDENTITY FIRST, AND IT IS A DECISION.
 *
 * A part that cannot be READ cannot be identified, and this driver then writes nothing to
 * it -- not the register-file release, not the quiesce. On a bus whose reads fail while its
 * writes still land (the failure this suite models elsewhere, and the reason stop_output()
 * does not read before it writes) that means a warm reboot out of playback leaves a DAC this
 * driver could have muted and did not.
 *
 * The alternative is writing six registers, chosen for their effect on an ES8311, into
 * whatever is really at an address the devicetree only CLAIMS is an ES8311. Doing nothing to
 * a device you cannot identify is the smaller failure, and this test pins that choice so it
 * cannot be quietly reversed by a future refactor.
 */
ZTEST(es8311, test_init_writes_nothing_to_a_part_it_cannot_identify)
{
	zassert_false(device_is_ready(codec_mute), "the deferred instance must not be up yet");

	/* Reads die. Writes would still land -- and must not be attempted. */
	emul_es8311_fail_reads(emul_mute, true);

	zassert_true(device_init(codec_mute) < 0,
		     "init() must refuse a part whose identity cannot be read");

	emul_es8311_fail_reads(emul_mute, false);

	zassert_equal(emul_es8311_write_count(emul_mute), 0,
		      "init() wrote %d register(s) to a device it could not identify. The "
		      "devicetree says it is an ES8311; it did not answer. Those writes were "
		      "chosen for their effect on an ES8311.",
		      emul_es8311_write_count(emul_mute));
	zassert_false(device_is_ready(codec_mute), "it must not be left ready");
}

/*
 * A FAILURE MAY LEAVE THINGS UNCHANGED. IT MAY NOT MAKE THEM WORSE.
 *
 * These three are the second-order version of the fail-open above, and they were introduced BY
 * the fix for it: putting the mutes first stopped a volume failure from cancelling a mute, and
 * left two ways for a FAILED mute to be followed by something that makes the danger bigger.
 */

/*
 * A volume register is a GAIN. If the mute write failed, the path is still live -- and writing
 * the cached volume to it can turn it UP. Ask for +32 dB and a mute in one call, lose the mute
 * write, and the speaker you asked to silence ends up at full scale.
 */
ZTEST(es8311, test_a_failed_output_mute_does_not_turn_the_speaker_up)
{
	audio_property_value_t mute = {.mute = true};
	audio_property_value_t loud = {.vol = 32};

	audio_codec_start_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_OFF,
		      "precondition: the speaker is live");

	/* The DAC volume register, as the hardware currently stands: 0 dB. */
	reg_put(ES8311_REG_DAC_VOLUME, 0xBFU);

	/* One call: turn it up, and mute it. */
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    loud),
		   "set OUTPUT_VOLUME(+32 dB) failed");
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute),
		   "set OUTPUT_MUTE(true) failed");

	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	zassert_true(audio_codec_apply_properties(codec) < 0, "apply must report the failed mute");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_DAC_VOLUME), 0xBFU,
		      "the mute FAILED, so the speaker is still playing -- and the volume was "
		      "written anyway, so it is now playing at 0x%02x instead of 0xBF. The one "
		      "thing a failed mute must never be followed by is more gain.",
		      reg_get(ES8311_REG_DAC_VOLUME));
}

/* The same for the microphone: a failed input mute must not be followed by more PGA gain. */
ZTEST(es8311, test_a_failed_input_mute_does_not_turn_the_microphone_up)
{
	audio_property_value_t mute = {.mute = true};
	audio_property_value_t loud = {.vol = 32};

	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT,
		      "precondition: the microphone is open");

	reg_put(ES8311_REG_ADC_VOLUME, 0xBFU);

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    loud),
		   "set INPUT_VOLUME(+32 dB) failed");
	zassert_ok(
		audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, mute),
		"set INPUT_MUTE(true) failed");

	/* The microphone's mute IS a write to 0x0A, so breaking that register breaks the mute. */
	emul_es8311_fail_write_to(emul, ES8311_REG_SDP_OUT);
	zassert_true(audio_codec_apply_properties(codec) < 0, "apply must report the failed mute");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_ADC_VOLUME), 0xBFU,
		      "the microphone mute FAILED, so it is still open -- and its gain was turned "
		      "up to 0x%02x anyway",
		      reg_get(ES8311_REG_ADC_VOLUME));
}

/*
 * And a failure in one direction must not be followed by OPENING the other.
 *
 * Lose the microphone's mute, carry out the speaker's unmute, and the call has left the
 * microphone open AND put a speaker live next to it -- on this board they are centimetres
 * apart -- while returning an error that says none of it can be trusted. Failing to make one
 * thing safe is not a licence to make another thing loud.
 */
ZTEST(es8311, test_a_failed_mute_blocks_the_unmute_in_the_other_direction)
{
	audio_property_value_t mute = {.mute = true};
	audio_property_value_t unmute = {.mute = false};

	/* Speaker muted, microphone open. */
	audio_codec_stop_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_ON,
		      "precondition: the speaker is muted");

	/* Now: silence the microphone, and open the speaker. */
	zassert_ok(
		audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, mute),
		"set INPUT_MUTE(true) failed");
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute),
		   "set OUTPUT_MUTE(false) failed");

	emul_es8311_fail_write_to(emul, ES8311_REG_SDP_OUT);
	zassert_true(audio_codec_apply_properties(codec) < 0, "apply must report the failed mute");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_ON,
		      "the microphone's mute failed and the speaker was opened anyway: a live mic "
		      "and a live speaker, from a call that returned an error");
}

/*
 * A FAILED UNMUTE MUST STOP THE NEXT UNMUTE -- AND THE GATE MUST BE LIVE, NOT A SNAPSHOT.
 *
 * apply_properties() does the microphone unmute before the speaker unmute, and the speaker
 * unmute is gated on nothing above it having failed. The bug this catches is subtle: an
 * earlier version took ONE snapshot of "is everything still clean" before phase 3 and reused
 * it for both unmutes. So a microphone unmute that FAILED still left the snapshot reading
 * "clean", and the speaker was opened anyway -- a live speaker next to a microphone whose
 * unmute just errored, from a call that returned failure. The gate has to be re-read at each
 * unmute, which is what makes the FIRST unmute's failure disarm the SECOND.
 *
 * The neighbour test above covers a PHASE-1 (requested-mute) failure blocking an unmute. This
 * is the one that covers the first unmute in PHASE 3 failing and taking the second down with
 * it.
 */
ZTEST(es8311, test_a_failed_unmute_stops_the_next_unmute)
{
	struct audio_codec_cfg cfg;

	/* Full duplex, both cached unmuted (the fixture leaves them so). */
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure failed");

	/* Force the hardware muted in both directions, so an unmute would be visible. */
	reg_put(ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
	reg_put(ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);

	/*
	 * The FIRST unmute apply_properties() performs is the microphone's (SDP_OUT). Break it.
	 * The speaker unmute (DAC_MUTE, 0x31) is a different register and would succeed if it
	 * were reached -- so if the DAC ends up unmuted, the first unmute's failure did NOT stop
	 * the second, which is the bug.
	 */
	emul_es8311_fail_write_to(emul, ES8311_REG_SDP_OUT);
	zassert_true(audio_codec_apply_properties(codec) < 0,
		     "apply must report the failed microphone unmute");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_ON,
		      "the microphone unmute failed and the speaker was opened anyway: the "
		      "clean-enough gate was a stale snapshot, not a live re-check");
}

/*
 * HOUSEKEEPING DOES NOT GET A VETO OVER SAFETY.
 *
 * init() writes 0xFA before it quiesces, but a failure of that write must NOT abandon the quiesce.
 * The 0xFA-is-a-precondition argument (nothing lands while INI_REG holds the file down) is true in
 * configure() but false here: es8311_check_id() has already read 0x8311 back, and a held part
 * answers 0x00 to every read, so INI_REG is proven clear and this write is housekeeping on its
 * other bits. Letting it abandon the DAC mute, the power-downs and the microphone disconnect is a
 * fail-open.
 */
ZTEST(es8311, test_init_quiesces_even_when_the_register_file_write_fails)
{
	uint8_t reg = 0U;

	zassert_false(device_is_ready(codec_ini), "the deferred instance must not be up yet");

	/* A warm reboot: the part comes up playing, and listening. */
	zassert_ok(i2c_reg_write_byte_dt(&es_ini, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF),
		   "seed failed");
	zassert_ok(i2c_reg_write_byte_dt(&es_ini, ES8311_REG_SYSTEM_12, 0x00U), "seed failed");
	zassert_ok(i2c_reg_write_byte_dt(&es_ini, ES8311_REG_SYSTEM_0E, 0x02U), "seed failed");
	zassert_ok(i2c_reg_write_byte_dt(&es_ini, ES8311_REG_ADC_PGA, 0x1AU), "seed failed");

	/* The chip id still reads fine. Only the 0xFA write dies. */
	emul_es8311_fail_write_to(emul_ini, ES8311_REG_INI);

	zassert_true(device_init(codec_ini) < 0, "init() must still report the failure it hit");

	emul_es8311_fail_write_to(emul_ini, -1);

	zassert_ok(i2c_reg_read_byte_dt(&es_ini, ES8311_REG_DAC_MUTE, &reg), "read failed");
	zassert_equal(reg, ES8311_DAC_MUTE_ON,
		      "the DAC was left UNMUTED: a failed housekeeping write to 0xFA threw away "
		      "the whole safety quiesce, on a part whose INI_REG was already known clear");

	zassert_ok(i2c_reg_read_byte_dt(&es_ini, ES8311_REG_SYSTEM_12, &reg), "read failed");
	zassert_equal(reg, 0x02U, "the DAC was left POWERED");

	zassert_ok(i2c_reg_read_byte_dt(&es_ini, ES8311_REG_SYSTEM_0E, &reg), "read failed");
	zassert_equal(reg, 0x62U, "the ADC was left POWERED, on a live microphone");

	zassert_ok(i2c_reg_read_byte_dt(&es_ini, ES8311_REG_ADC_PGA, &reg), "read failed");
	zassert_equal(reg, ES8311_ADC_MIC_OFF, "MIC1 was left connected to the PGA");
}

/*
 * THE LIFECYCLE SPLIT, from the mute's side: a pending OUTPUT_MUTE must survive a start/stop
 * cycle. Before output_mute and output_stopped were separated, start_output() cleared the mute
 * and stop_output() forged one, so "mute, then start" came out unmuted and the caller's request
 * was silently dropped. This is the test that would have caught that.
 */
ZTEST(es8311, test_pending_output_mute_survives_start_stop)
{
	audio_property_value_t mute = {.mute = true};

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute),
		   "set OUTPUT_MUTE(true) failed");

	/*
	 * stop_output() mutes the hardware; start_output() would normally unmute -- but it must
	 * respect the pending OUTPUT_MUTE and leave the speaker muted.
	 */
	audio_codec_stop_output(codec);
	audio_codec_start_output(codec);
	zassert_equal(
		reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		"start_output() unmuted a DAC the caller had muted: the pending OUTPUT_MUTE was "
		"destroyed by the lifecycle");

	/* And the property itself survived: apply_properties() still mutes, from a cleared reg. */
	reg_put(ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF);
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "apply_properties() did not mute: output_mute was lost across start/stop");
}

/*
 * The lifecycle split, from the FAILURE side. stop_output() returns void, so a caller cannot see
 * a mute write that glitched on the bus. The invariant "a stopped DAC is a muted DAC" is then
 * enforced by apply_properties(), which re-asserts the mute whenever the output is stopped -- so a
 * stop-then-apply heals the glitch. The old folded-flag model got this for free (stop_output()
 * forged a property mute); the split has to make it explicit, and this pins that it does.
 */
ZTEST(es8311, test_apply_re_mutes_a_stopped_output_whose_stop_glitched)
{
	/* Output running, DAC live. */
	audio_codec_start_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U, "precondition: DAC live");

	/* stop_output()'s mute write glitches on the bus; its void API hides the failure. */
	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	audio_codec_stop_output(codec);
	emul_es8311_fail_write_to(emul, -1);
	zassert_equal(
		reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U,
		"the glitched stop_output() left the speaker live -- this is the hazard apply "
		"has to heal");

	/* A later apply_properties() must re-mute the stopped output. */
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "apply_properties() must re-mute a stopped output whose stop_output() mute "
		      "glitched: a stopped DAC is a muted DAC");
}

/*
 * THE ROUTE-AWARE COMMIT, from the capture-only side. A playback route's last write is the DAC
 * mute (the speaker); a capture-only route has no speaker to open, so its last write is the
 * MICROPHONE port, with nothing that can fail after it. The earlier fixed
 * SDP_IN -> SDP_OUT -> DAC_MUTE order wrote a redundant DAC mute AFTER the microphone was open.
 */
ZTEST(es8311, test_capture_only_commit_opens_the_microphone_last)
{
	struct audio_codec_cfg cfg;
	int n;
	int dac_mute_idx = -1;

	emul_es8311_reset_log(emul);
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "capture configure failed");

	n = emul_es8311_write_count(emul);
	zassert_true(n >= 2, "capture configure emitted only %d writes", n);

	/* The LAST write opens the microphone, unmuted, and NOTHING failable follows it. */
	zassert_equal(
		emul_es8311_write_at(emul, n - 1), ES8311_REG_SDP_OUT,
		"the last write of a capture-only configure must be the ADC serial port (0x0A), "
		"not 0x%02x",
		emul_es8311_write_at(emul, n - 1));
	zassert_equal(emul_es8311_wval_at(emul, n - 1) & ES8311_SDP_MUTE, 0x00U,
		      "that last write must OPEN the microphone (unmuted)");

	/* The DAC mute is held ON and written in phase A, BEFORE the microphone opener -- never
	 * after it. That is what keeps the capture-only commit fail-closed under a persistent bus
	 * failure that also kills the quiesce.
	 */
	for (int i = 0; i < n; i++) {
		if (emul_es8311_write_at(emul, i) == ES8311_REG_DAC_MUTE) {
			dac_mute_idx = i;
		}
	}
	zassert_true(
		dac_mute_idx >= 0 && dac_mute_idx < n - 1,
		"the DAC mute must be written BEFORE the microphone opener (mute at %d, mic at %d)",
		dac_mute_idx, n - 1);
	zassert_equal(emul_es8311_wval_at(emul, dac_mute_idx), ES8311_DAC_MUTE_ON,
		      "the DAC must be held MUTED on a capture-only route, not unmuted");
}

/*
 * PERSISTENT FAIL-BEFORE-EFFECT. fail_from(idx) returns -EIO on transfer idx and every one after
 * it, BEFORE the write takes effect, so the error-path quiesce fails on the same dead bus. This is
 * what separates fail-closed by ORDERING from fail-closed-if-the-cleanup-runs: an opener is safe
 * only if it is genuinely the LAST write, so that a write failing after it cannot have opened it.
 *
 * configure() leaves the DAC muted, so the only converter it opens is the microphone, and that is
 * its last write. So under a persistent fail-before-effect BOTH converters stay muted on every
 * route: the speaker because configure() never unmutes it, the microphone because its opener is
 * the last write and never lands.
 *
 * This proves fail-closure for a fail-BEFORE-effect bus. A write that takes EFFECT and then reports
 * an error, on a bus that then dies forever, is a different fault the transport can leave
 * unrecoverable; the driver's contract there is best-effort, not a guarantee.
 */
static void walk_persistent_failure_into(audio_route_t target, const char *name)
{
	struct audio_codec_cfg cfg;
	bool reached_the_end = false;
	int covered = 0;

	for (int n = 0; n < FAULT_WALK_BOUND; n++) {
		int ret;

		/* Clean part, both openers seeded SHUT, so only a real write can open them. */
		emul_es8311_reset(emul);
		reg_put(ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
		reg_put(ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);

		make_cfg(&cfg, AUDIO_PCM_RATE_16K, target);
		emul_es8311_fail_from(emul, n);
		ret = audio_codec_configure(codec, &cfg);
		emul_es8311_fail_from(emul, -1);

		if (ret == 0) {
			/* n is past the end of the sequence: the configure fully succeeded. */
			reached_the_end = true;
			break;
		}
		covered++;

		zassert_equal(
			reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
			"-> %s: persistent failure from transfer %d left the SPEAKER live, with "
			"no working bus left to mute it",
			name, n);
		zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
			      "-> %s: persistent failure from transfer %d left the MICROPHONE open "
			      "-- its "
			      "serial port must be the LAST commit write",
			      name, n);
	}

	zassert_true(
		reached_the_end,
		"-> %s: the persistent walk hit its bound of %d without getting past the end of "
		"the sequence; it covered a PREFIX, not every transfer",
		name, FAULT_WALK_BOUND);
	zassert_true(covered > 1, "-> %s: must reach past the first transfer (covered %d)", name,
		     covered);
}

ZTEST(es8311, test_persistent_fail_before_effect_does_not_strand_any_opener)
{
	walk_persistent_failure_into(AUDIO_ROUTE_CAPTURE, "CAPTURE");
	walk_persistent_failure_into(AUDIO_ROUTE_PLAYBACK, "PLAYBACK");
	walk_persistent_failure_into(AUDIO_ROUTE_PLAYBACK_CAPTURE, "PLAYBACK_CAPTURE");
}

/*
 * The persistent walk again, but varying the mute/stopped STATE, not just the route. configure()
 * always leaves the DAC muted, so no state can make it the opener; the microphone is always the
 * only opener and always last. This checks that fail-closure is independent of OUTPUT_MUTE, the
 * stopped lifecycle and INPUT_MUTE -- a regression guard against a state-dependent unmute creeping
 * back into configure().
 */
ZTEST(es8311, test_persistent_fail_before_effect_is_fail_closed_across_state)
{
	const audio_property_value_t on = {.mute = true};
	const audio_property_value_t off = {.mute = false};

	audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL, on);
	walk_persistent_failure_into(AUDIO_ROUTE_PLAYBACK_CAPTURE, "PB_CAP+output_mute");
	audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL, off);

	audio_codec_stop_output(codec);
	walk_persistent_failure_into(AUDIO_ROUTE_PLAYBACK_CAPTURE, "PB_CAP+output_stopped");
	audio_codec_start_output(codec);

	audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, on);
	walk_persistent_failure_into(AUDIO_ROUTE_PLAYBACK_CAPTURE, "PB_CAP+input_mute");
	audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, off);

	audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL, on);
	audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, on);
	walk_persistent_failure_into(AUDIO_ROUTE_PLAYBACK_CAPTURE, "PB_CAP+both_muted");
	audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL, off);
	audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, off);
}

/*
 * Reconfiguring a LIVE route. The persistent walks above start from a part that is already shut;
 * this starts from a RUNNING codec -- full-duplex, DAC unmuted by start_output(), microphone open
 * -- which is the state a reconfigure actually meets in service.
 *
 * Part A: a successful reconfigure must silence the OLD route's live converters BEFORE it moves the
 * clock tree, so neither the DAC nor the microphone is still live when the dividers shift under it.
 *
 * Part B: a FAILED reconfigure must never be the thing that unmutes the DAC. Opening the speaker is
 * start_output()'s job alone, so configure() writes 0x31 only as MUTE, wherever the bus dies. (That
 * the quiesce may not COMPLETE on a dead bus -- leaving a live converter live from a live start --
 * is the best-effort limit the failed-configure and persistent-walk tests already pin; no software
 * beats a dead bus.)
 */
static void reconfigure_from_live_never_unmutes_the_dac(audio_route_t target, const char *name)
{
	struct audio_codec_cfg cfg;
	const audio_property_value_t off = {.mute = false};
	bool reached_the_end = false;
	int covered = 0;

	for (int n = 0; n < FAULT_WALK_BOUND; n++) {
		int ret;

		emul_es8311_reset(emul);
		make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
		zassert_ok(audio_codec_configure(codec, &cfg), "-> %s: setup configure must pass",
			   name);
		audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL, off);
		audio_codec_start_output(codec);
		zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U,
			      "-> %s: precondition -- the DAC must be LIVE before the reconfigure",
			      name);

		emul_es8311_reset_log(emul);
		make_cfg(&cfg, AUDIO_PCM_RATE_16K, target);
		emul_es8311_fail_from(emul, n);
		ret = audio_codec_configure(codec, &cfg);
		emul_es8311_fail_from(emul, -1);

		for (int i = 0; i < emul_es8311_write_count(emul); i++) {
			if (emul_es8311_write_at(emul, i) != ES8311_REG_DAC_MUTE) {
				continue;
			}
			zassert_equal(
				emul_es8311_wval_at(emul, i) & 0x60U, 0x60U,
				"-> %s: a reconfigure failing from transfer %d WROTE a DAC UNMUTE "
				"(0x31=0x%02x) -- only start_output() may open the speaker",
				name, n, emul_es8311_wval_at(emul, i));
		}

		if (ret == 0) {
			reached_the_end = true;
			break;
		}
		covered++;
	}

	zassert_true(
		reached_the_end,
		"-> %s: the live walk hit its bound of %d without reaching the end of the sequence",
		name, FAULT_WALK_BOUND);
	zassert_true(covered > 1, "-> %s: must reach past the first transfer (covered %d)", name,
		     covered);
}

ZTEST(es8311, test_reconfigure_from_a_live_route_quiesces_before_clocks_and_never_unmutes)
{
	struct audio_codec_cfg cfg;
	const audio_property_value_t off = {.mute = false};
	int clk = -1;
	int dac_mute = -1;
	int sdp_out = -1;

	/* Part A: a SUCCESSFUL reconfigure of a live full-duplex route down to playback-only. */
	emul_es8311_reset(emul);
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "setup configure must pass");
	audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL, off);
	audio_codec_start_output(codec);
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x00U,
		      "precondition -- the DAC must be LIVE");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, 0x00U,
		      "precondition -- the microphone must be OPEN");

	emul_es8311_reset_log(emul);
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "reconfigure must pass");

	for (int i = 0; i < emul_es8311_write_count(emul); i++) {
		int r = emul_es8311_write_at(emul, i);

		if (r == ES8311_REG_CLK_MANAGER && clk < 0) {
			clk = i;
		} else if (r == ES8311_REG_DAC_MUTE && dac_mute < 0) {
			dac_mute = i;
		} else if (r == ES8311_REG_SDP_OUT && sdp_out < 0) {
			sdp_out = i;
		}
	}

	zassert_true(clk > 0, "the clock manager (0x01) must be written");
	zassert_true(dac_mute >= 0 && dac_mute < clk,
		     "the live DAC must be muted before the clocks move (0x31 at %d, 0x01 at %d)",
		     dac_mute, clk);
	zassert_true(
		sdp_out >= 0 && sdp_out < clk,
		"the live microphone must be muted before the clocks move (0x0A at %d, 0x01 at %d)",
		sdp_out, clk);

	/* Part B: no FAILED reconfigure of a live route may write a DAC unmute, into any route. */
	reconfigure_from_live_never_unmutes_the_dac(AUDIO_ROUTE_CAPTURE, "live->CAPTURE");
	reconfigure_from_live_never_unmutes_the_dac(AUDIO_ROUTE_PLAYBACK, "live->PLAYBACK");
	reconfigure_from_live_never_unmutes_the_dac(AUDIO_ROUTE_PLAYBACK_CAPTURE, "live->PB_CAP");
}

/*
 * The three board-policy devicetree properties each reach the register they name. codec_profile
 * (native_sim.overlay) sets all of them to their non-default value -- right slot, 0 dB PGA,
 * line-out -- and the default node proves the defaults. This is what makes everest,es8311 a device
 * profile rather than one board's fixed tuning.
 */
ZTEST(es8311, test_board_policy_devicetree_properties)
{
	struct audio_codec_cfg cfg;
	uint8_t v;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);

	/* Defaults on the main node: left slot, 30 dB PGA (MIC1 differential), headphone. */
	emul_es8311_reset(emul);
	zassert_ok(audio_codec_configure(codec, &cfg), "default configure must pass");
	zassert_equal(reg_get(ES8311_REG_SDP_IN) & 0x80U, 0x00U,
		      "default everest,mono-dac-source is the LEFT slot (SDP_IN_SEL clear)");
	zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x1AU,
		      "default everest,mic-pga-gain-db is 30 dB (0x14 = MIC1 diff | code 0x0A)");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_13), 0x10U,
		      "default everest,output-mode is headphone (0x13 HPSW set)");

	/* The configured node: right slot, 0 dB PGA, line-out. */
	zassert_true(device_is_ready(codec_profile), "the profile codec must be ready");
	zassert_ok(audio_codec_configure(codec_profile, &cfg), "profile configure must pass");

	zassert_ok(i2c_reg_read_byte_dt(&es_profile, ES8311_REG_SDP_IN, &v), "read SDP_IN failed");
	zassert_equal(v & 0x80U, 0x80U, "mono-dac-source=right must set SDP_IN_SEL (0x09 bit 7)");
	zassert_ok(i2c_reg_read_byte_dt(&es_profile, ES8311_REG_ADC_PGA, &v),
		   "read ADC_PGA failed");
	zassert_equal(v, 0x10U, "mic-pga-gain-db=0 must be MIC1 differential | 0 dB (0x14 = 0x10)");
	zassert_ok(i2c_reg_read_byte_dt(&es_profile, ES8311_REG_SYSTEM_13, &v),
		   "read SYSTEM_13 failed");
	zassert_equal(v, 0x00U, "output-mode=lineout must clear HPSW (0x13 = 0x00)");
}

/*
 * apply_properties() BEST-EFFORT RE-MUTES a landed-then-errored unmute. fail_write_landed() is the
 * fault: a write LANDS in the register file and the transfer still returns -EIO. The DAC unmute
 * lands (register unmuted) and errors; apply() then best-effort re-mutes -- and because the re-mute
 * is another write to the same register, IT lands too (and also errors, harmlessly). So the DAC
 * ends MUTED even though the caller-visible unmute failed, and apply() still returns the error. A
 * best-effort mute is monotonic: it can only leave the path the same or safer, so re-muting is
 * strictly better than leaving a possibly-open path standing.
 */
ZTEST(es8311, test_an_unmute_that_lands_then_errors_is_best_effort_remuted)
{
	audio_property_value_t muted = {.mute = true};
	audio_property_value_t unmuted = {.mute = false};

	/* Get to a genuinely muted speaker first. */
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    muted),
		   "set OUTPUT_MUTE(true) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply mute failed");
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U, "should be muted");

	/* Ask to unmute; make every DAC-mute write LAND and still return an error. */
	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmuted),
		   "set OUTPUT_MUTE(false) failed");
	emul_es8311_fail_write_landed(emul, ES8311_REG_DAC_MUTE);
	zassert_true(audio_codec_apply_properties(codec) < 0,
		     "apply_properties() must still report the unmute's transport error");
	emul_es8311_fail_write_landed(emul, -1);

	/*
	 * The unmute landed, but the best-effort re-mute landed on top of it. The DAC ends MUTED,
	 * not left open, even though apply() failed -- a monotonic mute is strictly safer than
	 * leaving a path that might be open.
	 */
	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "a landed-then-errored unmute must be best-effort re-muted, not left open");
}

/*
 * The same best-effort re-mute on the MICROPHONE side. A landed-then-errored SDP_OUT unmute is
 * re-closed too, so an errored apply() aims BOTH converters back at muted, not just the speaker.
 */
ZTEST(es8311, test_a_mic_unmute_that_lands_then_errors_is_best_effort_remuted)
{
	/*
	 * The fixture configured PLAYBACK_CAPTURE with the microphone unmuted (adc_mute is false),
	 * so apply_properties() will unmute the microphone. Seed the port muted, then make every
	 * SDP_OUT write land-then-error.
	 */
	reg_put(ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
	emul_es8311_fail_write_landed(emul, ES8311_REG_SDP_OUT);
	zassert_true(audio_codec_apply_properties(codec) < 0,
		     "apply_properties() must report the microphone unmute's transport error");
	emul_es8311_fail_write_landed(emul, -1);

	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "a landed-then-errored microphone unmute must be best-effort re-muted");
}

/*
 * THE QUIESCE IS PRIORITY-ORDERED. On a bus that fails partway through the cleanup the writes that
 * land are a PREFIX, so the most dangerous thing to leave live -- the speaker -- must be muted
 * FIRST. This pins the whole order the error-path quiesce emits: speaker mute, microphone mute,
 * DAC data port, then the three power-downs -- so every mute precedes every power-down, and the
 * one write to land first if only one lands is the speaker mute.
 */
ZTEST(es8311, test_quiesce_mutes_the_speaker_first)
{
	static const uint8_t expect[] = {
		ES8311_REG_DAC_MUTE,  ES8311_REG_SDP_OUT,   ES8311_REG_SDP_IN,
		ES8311_REG_SYSTEM_12, ES8311_REG_SYSTEM_0E, ES8311_REG_ADC_PGA,
	};
	struct audio_codec_cfg cfg;
	int n;

	/*
	 * Fail a configure() mid-sequence so the error-path quiesce runs and is logged. fail_at
	 * fires once, so the quiesce itself completes and every one of its writes is recorded.
	 */
	emul_es8311_reset_log(emul);
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	emul_es8311_fail_at(emul, 8);
	zassert_true(audio_codec_configure(codec, &cfg) < 0, "configure() should have failed");
	emul_es8311_fail_at(emul, -1);

	n = emul_es8311_write_count(emul);
	zassert_true(n >= (int)ARRAY_SIZE(expect), "expected the six quiesce writes, got %d", n);

	/* The last six writes are the quiesce, in priority order. */
	for (size_t i = 0; i < ARRAY_SIZE(expect); i++) {
		zassert_equal(emul_es8311_write_at(emul, n - 6 + (int)i), expect[i],
			      "quiesce write %zu must be reg 0x%02x, was 0x%02x", i, expect[i],
			      emul_es8311_write_at(emul, n - 6 + (int)i));
	}
	zassert_equal(emul_es8311_wval_at(emul, n - 6), ES8311_DAC_MUTE_ON,
		      "the quiesce's first write must be a DAC MUTE, not an unmute");
}

ZTEST_SUITE(es8311, NULL, NULL, es8311_before, NULL, NULL);

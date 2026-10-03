/*
 * Copyright (c) 2026 Hsiu-Chi Tsai
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>
#include <zephyr/sys/util.h>

#define CODEC_NODE DT_NODELABEL(codec)

static const struct device *const codec = DEVICE_DT_GET(CODEC_NODE);
static const struct i2c_dt_spec es = I2C_DT_SPEC_GET(CODEC_NODE);
static const struct emul *const emul = EMUL_DT_GET(CODEC_NODE);

#define QUIESCE_NODE DT_NODELABEL(codec_quiesce)
#define FOREIGN_NODE DT_NODELABEL(codec_foreign)
#define WARM_NODE    DT_NODELABEL(codec_warm)
#define MUTE_NODE    DT_NODELABEL(codec_mute)
#define INI_NODE     DT_NODELABEL(codec_ini)
static const struct device *const codec_warm = DEVICE_DT_GET(WARM_NODE);
static const struct i2c_dt_spec es_warm = I2C_DT_SPEC_GET(WARM_NODE);

/* A part with every board-policy devicetree property set to its non-default value. */
#define PROFILE_NODE DT_NODELABEL(codec_profile)
static const struct device *const codec_profile = DEVICE_DT_GET(PROFILE_NODE);
static const struct i2c_dt_spec es_profile = I2C_DT_SPEC_GET(PROFILE_NODE);

/* Emulator test backend (defined in drivers/audio/emul_es8311.c). */
#include <emul_es8311.h>

/* Register fields used by the assertions. */
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

/* Seed inherited state that configure() must overwrite. */
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

/* SDP_IN bit 7 selects the right slot; the default node selects the left slot. */
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

/* External 256fs MCLK supports every rate in the test table. */
static void make_cfg(struct audio_codec_cfg *cfg, uint32_t rate, audio_route_t route)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->mclk_freq = rate * 256U;
	cfg->dai_type = AUDIO_DAI_TYPE_I2S;
	cfg->dai_route = route;
	cfg->dai_cfg.i2s.word_size = AUDIO_PCM_WIDTH_16_BITS;
	cfg->dai_cfg.i2s.channels = 2;
	cfg->dai_cfg.i2s.frame_clk_freq = rate;
	/* The clock-role flags describe the codec endpoint. */
	cfg->dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
}

/* The same, with the master clock derived from BCLK instead (mclk_freq == 0). */
static void make_cfg_bclk(struct audio_codec_cfg *cfg, uint32_t rate, audio_route_t route)
{
	make_cfg(cfg, rate, route);
	cfg->mclk_freq = 0U;
}

/* 16 kHz / 16-bit playback, the configuration the application uses. */
static void make_cfg_16k_16bit(struct audio_codec_cfg *cfg)
{
	make_cfg(cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
}

enum init_case {
	INIT_FOREIGN,
	INIT_WARM,
	INIT_QUIESCE_FAILURE,
	INIT_READ_FAILURE,
	INIT_RELEASE_FAILURE,
};

struct init_probe {
	const struct device *dev;
	const struct emul *emul;
	struct i2c_dt_spec bus;
	int fault_reg;
	bool read_fault;
	bool foreign_id;
	bool ready_before;
	bool ready_after;
	int seed_ret;
	int init_ret;
	int snapshot_ret;
	int writes;
	unsigned int calls;
	uint8_t regs[256];
};

#define INIT_PROBE(node, reg, read, foreign)                                                       \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node),                                                        \
		.emul = EMUL_DT_GET(node),                                                         \
		.bus = I2C_DT_SPEC_GET(node),                                                      \
		.fault_reg = (reg),                                                                \
		.read_fault = (read),                                                              \
		.foreign_id = (foreign),                                                           \
	}

static struct init_probe init_probes[] = {
	[INIT_FOREIGN] = INIT_PROBE(FOREIGN_NODE, -1, false, true),
	[INIT_WARM] = INIT_PROBE(WARM_NODE, -1, false, false),
	[INIT_QUIESCE_FAILURE] = INIT_PROBE(QUIESCE_NODE, ES8311_REG_SDP_IN, false, false),
	[INIT_READ_FAILURE] = INIT_PROBE(MUTE_NODE, -1, true, false),
	[INIT_RELEASE_FAILURE] = INIT_PROBE(INI_NODE, ES8311_REG_INI, false, false),
};

/* Probe once per boot; repeated tests inspect the same immutable init observations. */
static int capture_init_results(void)
{
	static const struct {
		uint8_t reg;
		uint8_t val;
	} warm_state[] = {
		{ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF},
		{ES8311_REG_SYSTEM_12, 0x00U},
		{ES8311_REG_SYSTEM_0E, 0x02U},
		{ES8311_REG_ADC_PGA, 0x1AU},
	};

	for (size_t i = 0; i < ARRAY_SIZE(init_probes); i++) {
		struct init_probe *probe = &init_probes[i];

		probe->ready_before = device_is_ready(probe->dev);
		for (size_t j = 0; j < ARRAY_SIZE(warm_state); j++) {
			probe->seed_ret = i2c_reg_write_byte_dt(&probe->bus, warm_state[j].reg,
								warm_state[j].val);
			if (probe->seed_ret < 0) {
				break;
			}
		}
		if (probe->seed_ret < 0) {
			continue;
		}

		if (probe->foreign_id) {
			emul_es8311_set_chip_id(probe->emul, 0x00U, 0x00U);
		}
		emul_es8311_reset_log(probe->emul);
		emul_es8311_fail_write_to(probe->emul, probe->fault_reg);
		emul_es8311_fail_reads(probe->emul, probe->read_fault);
		probe->calls++;
		probe->init_ret = device_init(probe->dev);
		probe->ready_after = device_is_ready(probe->dev);
		probe->writes = emul_es8311_write_count(probe->emul);
		emul_es8311_fail_write_to(probe->emul, -1);
		emul_es8311_fail_reads(probe->emul, false);
		probe->snapshot_ret =
			i2c_burst_read_dt(&probe->bus, 0, probe->regs, sizeof(probe->regs));
	}

	return 0;
}

SYS_INIT(capture_init_results, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static const struct init_probe *check_init(enum init_case which, int expected_ret)
{
	const struct init_probe *probe = &init_probes[which];

	zassert_ok(probe->seed_ret, "Failed to seed init case %d", which);
	zassert_equal(probe->calls, 1U, "Init case %d did not probe exactly once", which);
	zassert_false(probe->ready_before, "Init case %d was already initialized", which);
	zassert_equal(probe->init_ret, expected_ret, "Unexpected init result for case %d", which);
	zassert_equal(probe->ready_after, expected_ret == 0, "Unexpected readiness for case %d",
		      which);
	zassert_ok(probe->snapshot_ret, "Failed to capture init case %d", which);

	return probe;
}

ZTEST(es8311, test_init_reads_chip_id)
{
	zassert_true(device_is_ready(codec), "codec device not ready");
	zassert_equal(reg_get(ES8311_REG_CHIP_ID1), 0x83U, "chip id1 should read 0x83");
	zassert_equal(reg_get(ES8311_REG_CHIP_ID2), 0x11U, "chip id2 should read 0x11");
}

ZTEST(es8311, test_init_wrong_chip_id_is_fatal)
{
	const struct init_probe *probe = check_init(INIT_FOREIGN, -ENODEV);

	zassert_equal(probe->writes, 0, "An unidentified device received register writes");
}

/* Seed different values so each assertion requires a register write. */
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
	/* External MCLK with the unused ADC clocks gated off. */
	zassert_equal(reg_get(ES8311_REG_CLK_MANAGER), 0x35U, "0x01 should be 0x35");
	/* DIV_PRE = 1, MULT_PRE = x1: an external MCLK is already 256fs. */
	zassert_equal(reg_get(ES8311_REG_CLK_PRE), 0x00U, "0x02 should be 0x00");
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

	zassert_equal(
		reg_get(ES8311_REG_DAC_MUTE), 0x60U,
		"0x31 should be MUTED after configure (0x60): configure() is not start_output()");

	/* A playback-only route must power down and disconnect the capture input. */
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x62U,
		      "0x0E: the ADC must be powered down on a playback-only route");
	zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x00U,
		      "0x14: the microphone must be taken off the input mux");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "0x0A: the ADC serial port must be muted");
}

/* CSM must precede clock programming; shared analog power must precede DAC power. */
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
		/* Check the final DAC power-up, after the initial power-down for reclocking. */
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

/* Capture enables MIC1; full duplex must retain the playback configuration. */
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

	/* ADC serial data port: standard I2S, 16-bit, and MUTED. start(RX) is the first unmute. */
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), 0x0CU | ES8311_SDP_MUTE,
		      "0x0A should be 0x0C with the mute bit set");
	/* ADC power up. */
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x02U, "0x0E should be 0x02");
	/* Differential MIC1 pair (LINSEL = 1) at the 0 dB PGA default. */
	zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x10U, "0x14 should be 0x10");
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

/* A capture-only reconfiguration must power down a previously active DAC. */
ZTEST(es8311, test_configure_capture_only)
{
	struct audio_codec_cfg cfg;

	/* Come from a route that had the DAC up. */
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(PLAYBACK_CAPTURE) failed");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_12), 0x00U, "the DAC should be up here");

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(CAPTURE) failed");

	zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x02U, "0x0E: the ADC must be powered up");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_12), 0x02U,
		      "0x12: the DAC must be powered DOWN, not left as the previous route "
		      "left it");
	zassert_equal(reg_get(ES8311_REG_CLK_MANAGER), 0x3AU,
		      "0x01: the DAC clocks must be gated off");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0D), 0x09U,
		      "0x0D: the DAC's own reference must be dropped");
	zassert_equal(reg_get(ES8311_REG_SDP_IN) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "0x09: the DAC serial port must be muted");
}

/* Dropping capture must disconnect MIC1 and power down the ADC. */
ZTEST(es8311, test_route_transition_drops_the_microphone)
{
	struct audio_codec_cfg cfg;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(PLAYBACK_CAPTURE) failed");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x02U, "the ADC should be up here");
	zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x10U, "the mic should be on the mux here");

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(PLAYBACK) failed");

	zassert_equal(reg_get(ES8311_REG_SYSTEM_0E), 0x62U,
		      "0x0E: the ADC must be powered down, not left running");
	zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x00U,
		      "0x14: the microphone must be taken off the input mux");
	zassert_equal(reg_get(ES8311_REG_CLK_MANAGER), 0x35U,
		      "0x01: the ADC clocks must be gated off");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_0D), 0x31U,
		      "0x0D: the ADC's own bias and reference must be dropped");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "0x0A: the ADC serial port must be muted");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_12), 0x00U, "the DAC must still be up");
}

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

/* The 256fs dividers stay constant; DAC oversampling changes above 16 kHz. */
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
		zassert_equal(reg_get(ES8311_REG_CLK_MANAGER), 0x35U, "0x01 at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_CLK_PRE), 0x00U, "0x02 at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_ADC_OSR), 0x10U, "0x03 at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_DAC_OSR), rate <= 16000U ? 0x20U : 0x10U,
			      "0x04 at %u Hz", rate);
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

/* Only a two-slot, 16-bit frame matches the programmed clock ratios. */
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

ZTEST(es8311, test_external_mclk_must_be_256fs)
{
	struct audio_codec_cfg cfg;

	for (size_t i = 0; i < ARRAY_SIZE(supported_rates); i++) {
		uint32_t rate = supported_rates[i];

		make_cfg(&cfg, rate, AUDIO_ROUTE_PLAYBACK);
		zassert_ok(audio_codec_configure(codec, &cfg),
			   "256fs on the MCLK pin must be accepted at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_CLK_MANAGER) & 0x80U, 0x00U,
			      "0x01 bit 7 must select the MCLK pin at %u Hz", rate);
		zassert_equal(reg_get(ES8311_REG_CLK_PRE), 0x00U,
			      "0x02 must not multiply an external MCLK at %u Hz", rate);

		make_cfg(&cfg, rate, AUDIO_ROUTE_PLAYBACK);
		cfg.mclk_freq = rate * 128U;
		zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
			      "128fs on the MCLK pin must be rejected at %u Hz", rate);

		make_cfg(&cfg, rate, AUDIO_ROUTE_PLAYBACK);
		cfg.mclk_freq = 12288000U;
		if (rate != 48000U) {
			zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
				      "a fixed 12.288 MHz MCLK is not 256fs at %u Hz", rate);
		}
	}
}

/* BCLK multiplication requires an input above 1 MHz at the supported supply voltages. */
ZTEST(es8311, test_bclk_derived_clock_is_limited_to_compliant_rates)
{
	static const uint32_t compliant[] = {32000U, 44100U, 48000U};
	static const uint32_t below[] = {8000U, 11025U, 12000U, 16000U, 22050U, 24000U};
	struct audio_codec_cfg cfg;

	for (size_t i = 0; i < ARRAY_SIZE(compliant); i++) {
		make_cfg_bclk(&cfg, compliant[i], AUDIO_ROUTE_PLAYBACK);
		zassert_ok(audio_codec_configure(codec, &cfg), "BCLK-derived must work at %u Hz",
			   compliant[i]);
		zassert_equal(reg_get(ES8311_REG_CLK_MANAGER) & 0x80U, 0x80U,
			      "0x01 bit 7 must select BCLK at %u Hz", compliant[i]);
		zassert_equal(reg_get(ES8311_REG_CLK_PRE), 0x18U,
			      "0x02 must multiply BCLK by 8 at %u Hz", compliant[i]);
	}

	for (size_t i = 0; i < ARRAY_SIZE(below); i++) {
		int n;

		emul_es8311_reset_log(emul);
		make_cfg_bclk(&cfg, below[i], AUDIO_ROUTE_PLAYBACK);
		zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
			      "BCLK-derived must be refused at %u Hz", below[i]);

		/* Refused before the part is touched, not half way through reclocking it. */
		n = emul_es8311_write_count(emul);
		zassert_equal(n, 0, "a refused rate emitted %d writes at %u Hz", n, below[i]);
	}
}

/* The 32fs derivation assumes a two-slot frame, so anything else has to be refused. */
ZTEST(es8311, test_configure_rejects_a_non_stereo_frame)
{
	struct audio_codec_cfg cfg;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	cfg.dai_cfg.i2s.channels = 1U;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP, "mono must be refused");

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	cfg.dai_cfg.i2s.channels = 4U;
	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP, "4 slots must be refused");
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

/* A repeated start must not submit another unmute. */
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

/* Starting after a failed stop must still honor a pending mute property. */
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

/* A retry after failed reconfiguration must not restart playback implicitly. */
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

/* The ADC serial mute preserves the caller's volume setting. */
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

	/* The RX lifecycle must be running before a caller unmute can reach the port. */
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
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

	/* Start RX to distinguish the cached mute from the stopped-state mute. */
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), 0x0CU | ES8311_SDP_MUTE,
		      "start(RX) must leave a microphone the caller muted muted");

	/* And the path still opens, so an implementation stuck at muted does not pass either. */
	mute.mute = false;
	zassert_ok(
		audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL, mute),
		"set INPUT_MUTE(false) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties() failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), 0x0CU,
		      "clearing INPUT_MUTE must unmute a started microphone");
}

/* A playback-only property update must not access the ADC port. */
ZTEST(es8311, test_apply_on_a_playback_only_route_leaves_the_adc_port_alone)
{
	struct audio_codec_cfg cfg;
	int n;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure(PLAYBACK) failed");
	audio_codec_start_output(codec);

	emul_es8311_reset_log(emul);
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");

	n = emul_es8311_write_count(emul);
	zassert_true(n > 0, "apply_properties() wrote nothing, so this proves nothing");
	for (int i = 0; i < n; i++) {
		zassert_not_equal(emul_es8311_write_at(emul, i), ES8311_REG_SDP_OUT,
				  "apply() wrote the ADC serial port on a playback-only route");
	}
}

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

/* Pause apply_properties() mid-write and verify that stop_output() waits for its lock. */
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

/* The cooperative stop thread runs until it blocks after the test yields. */
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

	/* Yield so the cooperative stop thread reaches the codec lock. */
	k_yield();

	zassert_ok(k_sem_take(&stop_started, K_NO_WAIT),
		   "the cooperative stop thread had not run after k_yield(), which it must have");

	/* stop_output() must still be blocked while apply_properties() holds the transfer. */
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

/* Check write order, not just final values, before clocks are changed. */
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
	/* Keep ADC power on across capture reconfiguration to avoid restarting settling. */
	ARG_UNUSED(adc_pwr);
}

/* A failed configure must prevent starting the stale route, but must still allow stopping. */
ZTEST(es8311, test_failed_configure_disarms_start_but_never_stop)
{
	struct audio_codec_cfg cfg;
	int covered = 0;

	/* Bounded so a driver change that stops failing can never spin here. */
	for (int n = 0; n < 64; n++) {
		int ret;

		/* Start from full duplex so retaining either stale route flag is observable. */
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

		emul_es8311_reset_log(emul);
		audio_codec_start_output(codec);
		zassert_equal(emul_es8311_write_count(emul), 0,
			      "start_output() must not unmute after a configure() that "
			      "failed at transfer %d (it wrote %d register(s))",
			      n, emul_es8311_write_count(emul));

		/* Require a mute write even when the route is invalid. */
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

/* Test inherited register state without relying on reset defaults. */
static void dirty_the_chip(void)
{
	/* ALC, DRC and ADC2DAC_SEL change the meaning of the volume and data registers. */
	reg_put(ES8311_REG_ADC_ALC, ES8311_ALC_EN);
	reg_put(ES8311_REG_DAC_DRC, ES8311_DRC_EN);
	reg_put(ES8311_REG_ADC_MUX, ES8311_ADC2DAC_SEL);
	/* Seed the opposite DAC slot so a partial SDP_IN update cannot pass. */
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

	/* ADC2DAC_SEL affects playback even though it shares an ADC register. */
	reg_put(ES8311_REG_ADC_MUX, ES8311_ADC2DAC_SEL);

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	zassert_equal(reg_get(ES8311_REG_ADC_MUX) & ES8311_ADC2DAC_SEL, 0x00U,
		      "a playback-only route left ADC2DAC_SEL set: the speaker plays the mic");
}

/* Only init() releases INI_REG; reconfiguration must leave it unchanged. */
ZTEST(es8311, test_configure_never_touches_the_ini_register)
{
	struct audio_codec_cfg cfg;

	emul_es8311_reset_log(emul);

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");

	for (int i = 0; i < emul_es8311_write_count(emul); i++) {
		zassert_not_equal(emul_es8311_write_at(emul, i), ES8311_REG_INI,
				  "configure() wrote 0xFA at index %d", i);
	}
}

ZTEST(es8311, test_the_driver_never_resets_the_register_file)
{
	struct audio_codec_cfg cfg;
	int writes;

	/* Inspect writes directly; the emulator does not treat RST_DIG as a register-file reset. */
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

/* Reset emulator registers and restore driver properties through the public API. */
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

	audio_codec_start_output(codec);
}

/* Muting serial output alone does not power down the microphone front end. */
ZTEST(es8311, test_init_quiesces_the_microphone_not_just_the_speaker)
{
	const struct init_probe *probe = check_init(INIT_WARM, 0);
	struct audio_codec_cfg cfg_warm;
	uint8_t reg;

	zassert_equal(probe->regs[ES8311_REG_DAC_MUTE], ES8311_DAC_MUTE_ON,
		      "Init left the DAC unmuted");
	zassert_equal(probe->regs[ES8311_REG_SYSTEM_12], 0x02U, "Init left the DAC powered");
	zassert_equal(probe->regs[ES8311_REG_SYSTEM_0E], 0x62U, "Init left the ADC powered");
	zassert_equal(probe->regs[ES8311_REG_ADC_PGA], ES8311_ADC_MIC_OFF,
		      "Init left MIC1 connected");

	/* Configure must leave the output muted on a device untouched by the runtime fixture. */
	zassert_ok(i2c_reg_write_byte_dt(&es_warm, ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_OFF));
	make_cfg(&cfg_warm, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_configure(codec_warm, &cfg_warm));
	zassert_ok(i2c_reg_read_byte_dt(&es_warm, ES8311_REG_DAC_MUTE, &reg));
	zassert_equal(reg & ES8311_DAC_MUTE_ON, ES8311_DAC_MUTE_ON);
}

ZTEST(es8311, test_configure_rejects_a_gated_bit_clock)
{
	struct audio_codec_cfg cfg;

	/* BCLK gating can remove the codec clock before the caller mutes it. */
	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.options |= I2S_OPT_BIT_CLK_GATED;

	zassert_equal(audio_codec_configure(codec, &cfg), -ENOTSUP,
		      "a gated bit clock must be rejected: it stops the codec's master clock");

	/* And the continuous bit clock, which is the same bit cleared, must still work. */
	make_cfg_16k_16bit(&cfg);
	cfg.dai_cfg.i2s.options |= I2S_OPT_BIT_CLK_CONT;
	zassert_ok(audio_codec_configure(codec, &cfg), "a continuous bit clock must be accepted");
}

/* A failed register read must not prevent a direct mute write. */
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

ZTEST(es8311, test_input_mute_survives_a_bus_that_cannot_be_read)
{
	struct audio_codec_cfg cfg;
	audio_property_value_t val;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure() failed");
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");

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

/* An init error must not abandon the remaining quiesce writes. */
ZTEST(es8311, test_init_finishes_quiescing_even_when_a_safety_write_fails)
{
	const struct init_probe *probe = check_init(INIT_QUIESCE_FAILURE, -EIO);

	zassert_equal(probe->regs[ES8311_REG_DAC_MUTE], ES8311_DAC_MUTE_ON,
		      "Init left the DAC unmuted");
	zassert_equal(probe->regs[ES8311_REG_SYSTEM_12], 0x02U, "Init left the DAC powered");
	zassert_equal(probe->regs[ES8311_REG_SYSTEM_0E], 0x62U, "Init left the ADC powered");
	zassert_equal(probe->regs[ES8311_REG_ADC_PGA], ES8311_ADC_MIC_OFF,
		      "Init left MIC1 connected");
}

/* Configure the ADC-to-DAC mux before the final, muted capture-port write. */
ZTEST(es8311, test_configure_writes_muted_capture_port_last)
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

	/* Last three writes: DAC serial port, DAC mute (held ON), mic port (held OFF). */
	zassert_equal(emul_es8311_write_at(emul, n - 3), ES8311_REG_SDP_IN,
		      "write %d should be the DAC serial port (0x09), not 0x%02x", n - 3,
		      emul_es8311_write_at(emul, n - 3));
	zassert_equal(emul_es8311_write_at(emul, n - 2), ES8311_REG_DAC_MUTE,
		      "write %d should be the DAC mute (0x31), not 0x%02x", n - 2,
		      emul_es8311_write_at(emul, n - 2));
	zassert_equal(
		emul_es8311_write_at(emul, n - 1), ES8311_REG_SDP_OUT,
		"the LAST write of a configure() must be the microphone port (0x0A). It was 0x%02x",
		emul_es8311_write_at(emul, n - 1));

	/* Both converters are left silent: start() is the first unmute in either direction. */
	zassert_equal(emul_es8311_wval_at(emul, n - 2), ES8311_DAC_MUTE_ON,
		      "the DAC must be left MUTED by configure()");
	zassert_equal(emul_es8311_wval_at(emul, n - 1) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "the microphone serial port must be left MUTED by configure()");

	zassert_true(
		mux >= 0 && mux < n - 1,
		"0x44 was normalised at write %d but the microphone opened at %d: ADC2DAC_SEL is "
		"still set in that window",
		mux, n - 1);
}

/* Inspect cleanup immediately after each injected configure failure. */
/* Require a successful call beyond the last injected failure to prove complete coverage. */
#define FAULT_WALK_BOUND 64

static void walk_every_failure_into(audio_route_t target, const char *name)
{
	struct audio_codec_cfg cfg;
	bool reached_the_end = false;
	int covered = 0;

	for (int n = 0; n < FAULT_WALK_BOUND; n++) {
		int ret;

		/* Start both directions before injecting a reconfiguration failure. */
		make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
		zassert_ok(audio_codec_configure(codec, &cfg), "setup configure must pass");
		audio_codec_start_output(codec);
		zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_OFF,
			      "setup: the DAC must be live before we break anything");
		zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x10U,
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
		     "%s: fault walk ended before the sequence completed; increase bound %d", name,
		     FAULT_WALK_BOUND);

	zassert_true(covered > 1,
		     "-> %s: the fault injection must have reached more than the first transfer "
		     "(covered %d)",
		     name, covered);
}

ZTEST(es8311, test_a_failed_configure_leaves_the_codec_silent)
{

	walk_every_failure_into(AUDIO_ROUTE_CAPTURE, "CAPTURE");
	walk_every_failure_into(AUDIO_ROUTE_PLAYBACK, "PLAYBACK");
	walk_every_failure_into(AUDIO_ROUTE_PLAYBACK_CAPTURE, "PLAYBACK_CAPTURE");
}

/* Fail by register so adding earlier transfers cannot move the intended fault. */
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

	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
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

/* A failed volume write must prevent unmuting with an unknown level. */
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

ZTEST(es8311, test_a_failure_on_one_direction_still_mutes_the_other)
{
	audio_property_value_t mute = {.mute = true};

	audio_codec_start_output(codec);
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
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

ZTEST(es8311, test_a_failed_input_mute_does_not_turn_the_speaker_up)
{
	audio_property_value_t mute = {.mute = true};
	audio_property_value_t loud = {.vol = 32};

	audio_codec_start_output(codec);
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
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

/* Unreadable identity must prevent all writes, even when writes themselves would succeed. */
ZTEST(es8311, test_init_writes_nothing_to_a_part_it_cannot_identify)
{
	const struct init_probe *probe = check_init(INIT_READ_FAILURE, -EIO);

	zassert_equal(probe->writes, 0, "Init wrote registers after an identity read failure");
}

/* A failed mute must not be followed by a volume increase. */
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

	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
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

/* A failed RX unmute must block the subsequent TX unmute. */
ZTEST(es8311, test_a_failed_unmute_stops_the_next_unmute)
{
	struct audio_codec_cfg cfg;

	/* Full duplex, both cached unmuted (the fixture leaves them so). */
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure failed");

	/* Start both directions so the fault reaches an unmute, not a stopped-state mute. */
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_TXRX), "start(TXRX) failed");

	/* Force the hardware muted in both directions, so an unmute would be visible. */
	reg_put(ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
	reg_put(ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);

	emul_es8311_fail_write_to(emul, ES8311_REG_SDP_OUT);
	zassert_true(audio_codec_apply_properties(codec) < 0,
		     "apply must report the failed microphone unmute");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_DAC_MUTE), ES8311_DAC_MUTE_ON,
		      "the microphone unmute failed and the speaker was opened anyway: the "
		      "clean-enough gate was a stale snapshot, not a live re-check");
}

/* Attempt quiescing even when releasing INI_REG fails. */
ZTEST(es8311, test_init_quiesces_even_when_the_register_file_write_fails)
{
	const struct init_probe *probe = check_init(INIT_RELEASE_FAILURE, -EIO);

	zassert_equal(probe->regs[ES8311_REG_DAC_MUTE], ES8311_DAC_MUTE_ON,
		      "Init left the DAC unmuted");
	zassert_equal(probe->regs[ES8311_REG_SYSTEM_12], 0x02U, "Init left the DAC powered");
	zassert_equal(probe->regs[ES8311_REG_SYSTEM_0E], 0x62U, "Init left the ADC powered");
	zassert_equal(probe->regs[ES8311_REG_ADC_PGA], ES8311_ADC_MIC_OFF,
		      "Init left MIC1 connected");
}

ZTEST(es8311, test_pending_output_mute_survives_start_stop)
{
	audio_property_value_t mute = {.mute = true};

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    mute),
		   "set OUTPUT_MUTE(true) failed");

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

/* apply_properties() must retry the mute after a failed stop. */
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

/* Keep capture muted and make its serial-port write the final configure write. */
ZTEST(es8311, test_capture_only_configure_keeps_microphone_muted)
{
	struct audio_codec_cfg cfg;
	int n;
	int dac_mute_idx = -1;

	emul_es8311_reset_log(emul);
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "capture configure failed");

	n = emul_es8311_write_count(emul);
	zassert_true(n >= 2, "capture configure emitted only %d writes", n);

	/* No writes follow the muted capture-port write. */
	zassert_equal(
		emul_es8311_write_at(emul, n - 1), ES8311_REG_SDP_OUT,
		"the last write of a capture-only configure must be the ADC serial port (0x0A), "
		"not 0x%02x",
		emul_es8311_write_at(emul, n - 1));
	zassert_equal(emul_es8311_wval_at(emul, n - 1) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "that last write must leave the microphone MUTED");

	/* Assert the DAC mute before the final capture-port write. */
	for (int i = 0; i < n; i++) {
		if (emul_es8311_write_at(emul, i) == ES8311_REG_DAC_MUTE) {
			dac_mute_idx = i;
		}
	}
	zassert_true(dac_mute_idx >= 0 && dac_mute_idx < n - 1,
		     "DAC mute must precede the capture-port write (mute at %d, capture at %d)",
		     dac_mute_idx, n - 1);
	zassert_equal(emul_es8311_wval_at(emul, dac_mute_idx), ES8311_DAC_MUTE_ON,
		      "the DAC must be held MUTED on a capture-only route, not unmuted");
}

/* Persistent failures also reject cleanup; verify that configure never unmutes either path. */
static void walk_persistent_failure_into(audio_route_t target, const char *name)
{
	struct audio_codec_cfg cfg;
	bool reached_the_end = false;
	int covered = 0;

	for (int n = 0; n < FAULT_WALK_BOUND; n++) {
		int ret;

		/* Seed both directions muted to detect an unmute before a later failure. */
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

/* Repeat the persistent-failure walk across mute properties and lifecycle states. */
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

/* A live route must be muted before reclocking, including failed reconfigurations. */
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
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
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

/* Compare default properties with the right-slot, 30 dB PGA profile. */
ZTEST(es8311, test_board_policy_devicetree_properties)
{
	struct audio_codec_cfg cfg;
	uint8_t v;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);

	/* Defaults on the main node: left slot, 0 dB PGA (MIC1 differential). */
	emul_es8311_reset(emul);
	zassert_ok(audio_codec_configure(codec, &cfg), "default configure must pass");
	zassert_equal(reg_get(ES8311_REG_SDP_IN) & 0x80U, 0x00U,
		      "default everest,mono-dac-source is the LEFT slot (SDP_IN_SEL clear)");
	zassert_equal(reg_get(ES8311_REG_ADC_PGA), 0x10U,
		      "default everest,mic-pga-gain-db is 0 dB (0x14 = MIC1 differential only)");
	zassert_equal(reg_get(ES8311_REG_SYSTEM_13), 0x10U,
		      "the output mode is compiled in as headphone (0x13 HPSW set)");

	/* The configured node: right slot, 30 dB PGA. */
	zassert_true(device_is_ready(codec_profile), "the profile codec must be ready");
	zassert_ok(audio_codec_configure(codec_profile, &cfg), "profile configure must pass");

	zassert_ok(i2c_reg_read_byte_dt(&es_profile, ES8311_REG_SDP_IN, &v), "read SDP_IN failed");
	zassert_equal(v & 0x80U, 0x80U, "mono-dac-source=right must set SDP_IN_SEL (0x09 bit 7)");
	zassert_ok(i2c_reg_read_byte_dt(&es_profile, ES8311_REG_ADC_PGA, &v),
		   "read ADC_PGA failed");
	zassert_equal(v, 0x1AU,
		      "mic-pga-gain-db=30 must be MIC1 differential | 30 dB (0x14 = 0x1A)");
}

/* Model an unmute that takes effect but returns an error; the retry must mute it. */
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

	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "a landed-then-errored unmute must be best-effort re-muted, not left open");
}

ZTEST(es8311, test_a_mic_unmute_that_lands_then_errors_is_best_effort_remuted)
{
	int n;

	/* Start RX so fault injection reaches the unmute rather than the initial mute phase. */
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");

	reg_put(ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
	emul_es8311_reset_log(emul);
	emul_es8311_fail_write_landed(emul, ES8311_REG_SDP_OUT);
	zassert_true(audio_codec_apply_properties(codec) < 0,
		     "apply_properties() must report the microphone unmute's transport error");
	emul_es8311_fail_write_landed(emul, -1);

	/* The unmute went out and the re-mute followed it, rather than one mute standing in. */
	n = emul_es8311_write_count(emul);
	zassert_true(n >= 2, "expected the unmute and its re-mute, got %d writes", n);
	zassert_equal(emul_es8311_write_at(emul, n - 2), ES8311_REG_SDP_OUT,
		      "the second-to-last write must be the microphone unmute");
	zassert_equal(emul_es8311_wval_at(emul, n - 2), ES8311_SDP_I2S_16BIT,
		      "the microphone unmute must clear the mute bit");
	zassert_equal(emul_es8311_write_at(emul, n - 1), ES8311_REG_SDP_OUT,
		      "the last write must be the re-mute");
	zassert_equal(emul_es8311_wval_at(emul, n - 1), ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE,
		      "the re-mute must set the mute bit back");

	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "a landed-then-errored microphone unmute must be best-effort re-muted");
}

ZTEST(es8311, test_apply_re_mutes_a_stopped_mic_whose_stop_glitched)
{
	/* Capture running, microphone live. */
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, 0x00U,
		      "precondition: microphone live");

	/* stop(RX)'s mute write glitches on the bus. */
	emul_es8311_fail_write_to(emul, ES8311_REG_SDP_OUT);
	zassert_true(audio_codec_stop(codec, AUDIO_DAI_DIR_RX) < 0,
		     "stop(RX) must report the failed mute");
	emul_es8311_fail_write_to(emul, -1);
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, 0x00U,
		      "the glitched stop left the microphone live -- the hazard apply has to heal");

	/* A later apply_properties() must re-mute the stopped input. */
	zassert_ok(audio_codec_apply_properties(codec), "apply_properties failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "apply_properties() must re-mute a stopped microphone whose stop glitched: "
		      "a stopped ADC is a muted ADC");
}

/* A failed TX unmute must undo the RX unmute sent by the same apply call. */
ZTEST(es8311, test_a_failed_speaker_unmute_backs_out_the_microphone)
{
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");

	/* Start both directions, then seed muted hardware to observe both unmute writes. */
	reg_put(ES8311_REG_DAC_MUTE, ES8311_DAC_MUTE_ON);
	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	zassert_true(audio_codec_apply_properties(codec) < 0,
		     "apply_properties() must report the failed speaker unmute");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "a failed speaker unmute must back out the microphone unmute before it");
}

/* Check the attempted cleanup order even if later writes could fail. */
ZTEST(es8311, test_quiesce_mutes_the_speaker_first)
{
	static const uint8_t expect[] = {
		ES8311_REG_DAC_MUTE,  ES8311_REG_SDP_OUT,   ES8311_REG_SDP_IN,
		ES8311_REG_SYSTEM_12, ES8311_REG_SYSTEM_0E, ES8311_REG_ADC_PGA,
	};
	struct audio_codec_cfg cfg;
	int n;

	/* Use a one-shot fault so the cleanup writes can complete and be logged. */
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

ZTEST(es8311, test_configure_leaves_both_directions_stopped)
{
	struct audio_codec_cfg cfg;

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "configure failed");

	zassert_equal(reg_get(ES8311_REG_DAC_MUTE) & 0x60U, 0x60U,
		      "configure() must leave the DAC muted");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "configure() must leave the microphone muted");
}

ZTEST(es8311, test_start_rx_is_the_first_microphone_unmute)
{
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "precondition: the fixture's configure() leaves the microphone muted");

	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT,
		      "start(RX) must open the microphone with the port format intact");
}

ZTEST(es8311, test_start_rx_failure_before_write_keeps_input_stopped)
{
	emul_es8311_fail_at(emul, 0);
	zassert_equal(audio_codec_start(codec, AUDIO_DAI_DIR_RX), -EIO);
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);

	zassert_ok(audio_codec_apply_properties(codec));
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX));
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT);
}

ZTEST(es8311, test_start_rx_failure_after_write_remutes_input)
{
	/* The unmute takes effect even though the bus reports an error. */
	emul_es8311_fail_write_landed(emul, ES8311_REG_SDP_OUT);
	zassert_equal(audio_codec_start(codec, AUDIO_DAI_DIR_RX), -EIO);
	emul_es8311_fail_write_landed(emul, -1);
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);

	zassert_ok(audio_codec_apply_properties(codec));
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT | ES8311_SDP_MUTE);
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX));
	zassert_equal(reg_get(ES8311_REG_SDP_OUT), ES8311_SDP_I2S_16BIT);
}

ZTEST(es8311, test_stop_rx_survives_apply_properties)
{
	const audio_property_value_t unmuted = {.mute = false};

	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, 0x00U,
		      "precondition: the microphone is open");

	zassert_ok(audio_codec_stop(codec, AUDIO_DAI_DIR_RX), "stop(RX) failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "stop(RX) must mute the microphone");

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmuted),
		   "set INPUT_MUTE(false) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "apply_properties() re-opened a microphone that stop(RX) had stopped");
}

/* A reconfigure must not leave a started RX flag against a port configure() just muted. */
ZTEST(es8311, test_reconfigure_resets_rx_lifecycle_to_stopped)
{
	struct audio_codec_cfg cfg;
	const audio_property_value_t unmuted = {.mute = false};

	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");

	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_configure(codec, &cfg), "reconfigure failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "a reconfigure of a started capture must come up MUTED");

	zassert_ok(audio_codec_set_property(codec, AUDIO_PROPERTY_INPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmuted),
		   "set INPUT_MUTE(false) failed");
	zassert_ok(audio_codec_apply_properties(codec), "apply failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "the RX lifecycle flag did not follow the reconfigure");
}

ZTEST(es8311, test_duplicate_start_rx_is_idempotent)
{
	int n;

	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");

	emul_es8311_reset_log(emul);
	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "duplicate start(RX) failed");
	n = emul_es8311_write_count(emul);
	zassert_equal(n, 0, "a duplicate start(RX) emitted %d writes", n);
}

/* Rollback must not stop RX if it was already running before this call. */
ZTEST(es8311, test_txrx_start_failure_does_not_stop_preexisting_rx)
{
	/* The fixture leaves the output STARTED, and a started TX takes the idempotent path. */
	audio_codec_stop_output(codec);

	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, 0x00U,
		      "precondition: the microphone is open");

	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	zassert_true(audio_codec_start(codec, AUDIO_DAI_DIR_TXRX) < 0,
		     "the injected TX failure must surface");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, 0x00U,
		      "the rollback stopped an RX stream this call did not open");
}

ZTEST(es8311, test_txrx_start_failure_rolls_back_rx_opened_by_this_call)
{
	/* The fixture leaves the output STARTED, and a started TX takes the idempotent path. */
	audio_codec_stop_output(codec);

	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "precondition: the fixture leaves the microphone stopped");

	emul_es8311_fail_write_to(emul, ES8311_REG_DAC_MUTE);
	zassert_true(audio_codec_start(codec, AUDIO_DAI_DIR_TXRX) < 0,
		     "the injected TX failure must surface");
	emul_es8311_fail_write_to(emul, -1);

	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "a failed TXRX start left the microphone this call opened running");
}

ZTEST(es8311, test_stop_rx_still_mutes_after_a_failed_configure)
{
	struct audio_codec_cfg cfg;

	zassert_ok(audio_codec_start(codec, AUDIO_DAI_DIR_RX), "start(RX) failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, 0x00U,
		      "precondition: the microphone is open");

	/* Break the very first write of the reconfigure, so the route cache is already cleared. */
	make_cfg(&cfg, AUDIO_PCM_RATE_16K, AUDIO_ROUTE_PLAYBACK_CAPTURE);
	emul_es8311_fail_at(emul, 0);
	zassert_true(audio_codec_configure(codec, &cfg) < 0, "the injected failure must surface");
	emul_es8311_fail_at(emul, -1);

	/* Whatever the cache now says, the microphone must still be reachable. */
	reg_put(ES8311_REG_SDP_OUT, ES8311_SDP_I2S_16BIT);
	zassert_ok(audio_codec_stop(codec, AUDIO_DAI_DIR_RX), "stop(RX) failed");
	zassert_equal(reg_get(ES8311_REG_SDP_OUT) & ES8311_SDP_MUTE, ES8311_SDP_MUTE,
		      "stop(RX) skipped the mute because the route cache had been cleared");
}

ZTEST_SUITE(es8311, NULL, NULL, es8311_before, NULL, NULL);

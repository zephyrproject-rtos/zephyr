/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ADALM-LSMSPG NPN (AD5592R) and PNP (AD5593R) curve tracers.
 *
 * Port of no-OS projects/adalm-lsmspg curvetrace_example.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_backend.h>

#define MAX_CURVES 5
#define NUM_POINTS 50

#define BASE_SETTLE_MS  50
#define POINT_SETTLE_MS 10

#define GRAPH_HEIGHT 20
#define GRAPH_WIDTH  60

#define RESOLUTION_BITS 12

#define VBE_V 0.7f

#define USER_NODE DT_PATH(zephyr_user)

struct curve_tracer {
	const char *title;
	const char *chip;
	bool pnp;
	int num_curves;
	int vb_start_mv;
	int vb_step_mv;
	int vc_start_mv;
	int vc_step_mv;
	float rsense_ohm;
	float rbase_ohm;
	/* Emitter drive, PNP only */
	const struct dac_dt_spec *ve_drive;
	int ve_mv;
	const struct dac_dt_spec *vb_drive;
	const struct dac_dt_spec *vc_drive;
	const struct adc_dt_spec *vc_sense;
	const struct adc_dt_spec *vc_drive_meas;
	float mv_per_lsb;
	bool ready;
};

static const struct dac_dt_spec npn_vb_drive = DAC_DT_SPEC_GET_BY_NAME(USER_NODE, npn_vb_drive);
static const struct dac_dt_spec npn_vc_drive = DAC_DT_SPEC_GET_BY_NAME(USER_NODE, npn_vc_drive);
static const struct adc_dt_spec npn_vc_sense = ADC_DT_SPEC_GET_BY_NAME(USER_NODE, npn_vc_sense);
static const struct adc_dt_spec npn_vc_drive_meas =
	ADC_DT_SPEC_GET_BY_NAME(USER_NODE, npn_vc_drive_meas);

static const struct dac_dt_spec pnp_ve_drive = DAC_DT_SPEC_GET_BY_NAME(USER_NODE, pnp_ve_drive);
static const struct dac_dt_spec pnp_vb_drive = DAC_DT_SPEC_GET_BY_NAME(USER_NODE, pnp_vb_drive);
static const struct dac_dt_spec pnp_vc_drive = DAC_DT_SPEC_GET_BY_NAME(USER_NODE, pnp_vc_drive);
static const struct adc_dt_spec pnp_vc_sense = ADC_DT_SPEC_GET_BY_NAME(USER_NODE, pnp_vc_sense);
static const struct adc_dt_spec pnp_vc_drive_meas =
	ADC_DT_SPEC_GET_BY_NAME(USER_NODE, pnp_vc_drive_meas);

/* 2N3904 driven and sensed through the AD5592R, emitter to ground */
static struct curve_tracer npn = {
	.title = "NPN",
	.chip = "AD5592R (SPI)",
	.num_curves = 5,
	.vb_start_mv = 499,
	.vb_step_mv = 500,
	.vc_start_mv = 0,
	.vc_step_mv = 50,
	.rsense_ohm = CONFIG_SAMPLE_NPN_CURVE_TRACER_RSENSE_MOHM / 1000.0f,
	.rbase_ohm = CONFIG_SAMPLE_NPN_CURVE_TRACER_RBASE_OHM,
	.vb_drive = &npn_vb_drive,
	.vc_drive = &npn_vc_drive,
	.vc_sense = &npn_vc_sense,
	.vc_drive_meas = &npn_vc_drive_meas,
};

/* PNP driven and sensed through the AD5593R, emitter driven by a DAC */
static struct curve_tracer pnp = {
	.title = "PNP",
	.chip = "AD5593R (I2C)",
	.pnp = true,
	.num_curves = 4,
	.vb_start_mv = 2000,
	.vb_step_mv = -500,
	.vc_start_mv = CONFIG_SAMPLE_PNP_CURVE_TRACER_EMITTER_MV,
	.vc_step_mv = -50,
	.rsense_ohm = CONFIG_SAMPLE_PNP_CURVE_TRACER_RSENSE_MOHM / 1000.0f,
	.rbase_ohm = CONFIG_SAMPLE_PNP_CURVE_TRACER_RBASE_OHM,
	.ve_drive = &pnp_ve_drive,
	.ve_mv = CONFIG_SAMPLE_PNP_CURVE_TRACER_EMITTER_MV,
	.vb_drive = &pnp_vb_drive,
	.vc_drive = &pnp_vc_drive,
	.vc_sense = &pnp_vc_sense,
	.vc_drive_meas = &pnp_vc_drive_meas,
};

static float curve_vcs[MAX_CURVES][NUM_POINTS];
static float curve_ics[MAX_CURVES][NUM_POINTS];

static int read_adc(const struct adc_dt_spec *spec, uint16_t *raw)
{
	int16_t sample;
	struct adc_sequence seq = {
		.buffer = &sample,
		.buffer_size = sizeof(sample),
	};
	int ret;

	ret = adc_sequence_init_dt(spec, &seq);
	if (ret != 0) {
		return ret;
	}

	ret = adc_read_dt(spec, &seq);
	if (ret != 0) {
		return ret;
	}

	*raw = (uint16_t)sample & BIT_MASK(RESOLUTION_BITS);

	return 0;
}

static int write_dac_mv(const struct curve_tracer *ct, const struct dac_dt_spec *spec, int mv,
			uint16_t *raw_out)
{
	uint16_t raw = (uint16_t)CLAMP((float)mv / ct->mv_per_lsb, 0, BIT_MASK(RESOLUTION_BITS));

	if (raw_out != NULL) {
		*raw_out = raw;
	}

	return dac_write_value_dt(spec, raw);
}

static int setup_channels(const struct shell *sh, struct curve_tracer *ct)
{
	const struct dac_dt_spec *dacs[] = {ct->vb_drive, ct->vc_drive, ct->ve_drive};
	const struct adc_dt_spec *adcs[] = {ct->vc_sense, ct->vc_drive_meas};
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(dacs); i++) {
		if (dacs[i] == NULL) {
			continue;
		}
		if (!dac_is_ready_dt(dacs[i])) {
			shell_error(sh, "%s DAC channel %u not ready", ct->chip,
				    dacs[i]->channel_id);
			return -ENODEV;
		}
		ret = dac_channel_setup_dt(dacs[i]);
		if (ret != 0) {
			shell_error(sh, "%s DAC channel %u setup failed: %d", ct->chip,
				    dacs[i]->channel_id, ret);
			return ret;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(adcs); i++) {
		if (!adc_is_ready_dt(adcs[i])) {
			shell_error(sh, "%s ADC channel %u not ready", ct->chip,
				    adcs[i]->channel_id);
			return -ENODEV;
		}
		ret = adc_channel_setup_dt(adcs[i]);
		if (ret != 0) {
			shell_error(sh, "%s ADC channel %u setup failed: %d", ct->chip,
				    adcs[i]->channel_id, ret);
			return ret;
		}
	}

	/* Same reference/resolution on every channel of the device */
	ct->mv_per_lsb = (float)ct->vc_sense->vref_mv / (1 << RESOLUTION_BITS);
	ct->ready = true;

	return 0;
}

static void plot_ascii_graph(const struct shell *sh, const struct curve_tracer *ct)
{
	static char grid[GRAPH_HEIGHT + 2][GRAPH_WIDTH + 3];
	float max_v = 0.0f;
	float max_i = 0.0f;

	for (int y = 0; y < GRAPH_HEIGHT + 2; y++) {
		memset(grid[y], ' ', GRAPH_WIDTH + 2);
		grid[y][0] = '|';
		grid[y][GRAPH_WIDTH + 1] = '|';
		grid[y][GRAPH_WIDTH + 2] = '\0';
	}
	memset(grid[0], '-', GRAPH_WIDTH + 2);
	memset(grid[GRAPH_HEIGHT + 1], '-', GRAPH_WIDTH + 2);
	grid[0][0] = '+';
	grid[0][GRAPH_WIDTH + 1] = '+';
	grid[GRAPH_HEIGHT + 1][0] = '+';
	grid[GRAPH_HEIGHT + 1][GRAPH_WIDTH + 1] = '+';

	for (int c = 0; c < ct->num_curves; c++) {
		for (int p = 0; p < NUM_POINTS; p++) {
			max_v = MAX(max_v, curve_vcs[c][p]);
			max_i = MAX(max_i, curve_ics[c][p]);
		}
	}

	if (max_v < 0.01f) {
		max_v = 1.0f;
	}
	if (max_i < 0.001f) {
		max_i = 0.01f;
	}

	for (int c = 0; c < ct->num_curves; c++) {
		for (int p = 0; p < NUM_POINTS; p++) {
			int x = 1 + (int)((curve_vcs[c][p] / max_v) * (GRAPH_WIDTH - 1));
			int y = 1 + (int)((curve_ics[c][p] / max_i) * (GRAPH_HEIGHT - 1));

			if (!ct->pnp) {
				y = GRAPH_HEIGHT + 1 - y;
			}

			if (x >= 1 && x <= GRAPH_WIDTH && y >= 1 && y <= GRAPH_HEIGHT) {
				grid[y][x] = '*';
			}
		}
	}

	shell_print(sh, "\n\n=== %s - %s Curve Tracer (Ic vs Vc) ===", ct->chip, ct->title);
	shell_print(sh, "Y-axis: Ic (0 to %.2f mA)", (double)max_i);
	shell_print(sh, "X-axis: Vc (0 to %.2f V)\n", (double)max_v);

	for (int y = 0; y < GRAPH_HEIGHT + 2; y++) {
		shell_print(sh, "%s", grid[y]);
	}

	shell_fprintf(sh, SHELL_NORMAL, "0.0");
	for (int k = 1; k <= 5; k++) {
		shell_fprintf(sh, SHELL_NORMAL, "       %.2f", (double)(max_v / 5.0f * k));
	}
	shell_print(sh, " V\n\n===== %s %s Curve Trace Complete =====\n", ct->chip, ct->title);
}

static int curve_trace(const struct shell *sh, const struct curve_tracer *ct)
{
	uint16_t raw, vc_sense_raw, vc_meas_raw;
	int64_t start = k_uptime_get();
	float ve = 0.0f;
	int ret = 0;

	shell_print(sh, "\n========== %s %s Curve Tracer ==========", ct->chip, ct->title);
	shell_print(sh, "Vref: %u mV, Scale: %.4f mV/LSB, Rsense: %.1f ohm, Rbase: %.0f ohm",
		    ct->vc_sense->vref_mv, (double)ct->mv_per_lsb, (double)ct->rsense_ohm,
		    (double)ct->rbase_ohm);

	if (ct->pnp) {
		ret = write_dac_mv(ct, ct->ve_drive, ct->ve_mv, &raw);
		if (ret != 0) {
			shell_error(sh, "Failed to write emitter DAC: %d", ret);
			goto out;
		}
		ve = raw * ct->mv_per_lsb / 1000.0f;
		shell_print(sh, "Emitter Drive: %.4f V", (double)ve);
	}

	shell_print(sh, "\nStarting sweep...");

	for (int ci = 0; ci < ct->num_curves; ci++) {
		ret = write_dac_mv(ct, ct->vb_drive, ct->vb_start_mv + ci * ct->vb_step_mv, &raw);
		if (ret != 0) {
			shell_error(sh, "Failed to write base DAC: %d", ret);
			goto out;
		}

		k_msleep(BASE_SETTLE_MS);

		float vb = raw * ct->mv_per_lsb / 1000.0f;
		float ib = ((ct->pnp ? (ve - vb) : vb) - VBE_V) / ct->rbase_ohm;

		shell_print(sh, "Base Drive: %.4f V, %.3f uA", (double)vb, (double)(ib * 1e6f));

		for (int pi = 0; pi < NUM_POINTS; pi++) {
			ret = write_dac_mv(ct, ct->vc_drive, ct->vc_start_mv + pi * ct->vc_step_mv,
					   NULL);
			if (ret != 0) {
				shell_error(sh, "Failed to write collector DAC: %d", ret);
				goto out;
			}

			k_msleep(POINT_SETTLE_MS);

			ret = read_adc(ct->vc_sense, &vc_sense_raw);
			if (ret == 0) {
				ret = read_adc(ct->vc_drive_meas, &vc_meas_raw);
			}
			if (ret != 0) {
				shell_error(sh, "Failed to read ADC: %d", ret);
				goto out;
			}

			/* NPN: current flows from CH2 into the collector, PNP: the other way */
			int delta = ct->pnp ? (int)vc_sense_raw - (int)vc_meas_raw
					    : (int)vc_meas_raw - (int)vc_sense_raw;
			/* mV / ohm = mA */
			float ic = delta * ct->mv_per_lsb / ct->rsense_ohm;
			float vc = vc_sense_raw * ct->mv_per_lsb / 1000.0f;

			curve_vcs[ci][pi] = vc;
			curve_ics[ci][pi] = ic;

			shell_print(sh, "  coll voltage: %.4f V  coll current: %.4f mA", (double)vc,
				    (double)ic);
		}
	}

	plot_ascii_graph(sh, ct);

	shell_print(sh, "Sweep took %lld ms", k_uptime_get() - start);

out:
	/* Leave the transistor unbiased */
	if (ct->pnp) {
		write_dac_mv(ct, ct->ve_drive, 0, NULL);
	}
	write_dac_mv(ct, ct->vb_drive, 0, NULL);
	write_dac_mv(ct, ct->vc_drive, 0, NULL);

	return ret;
}

static int run_curve_trace(const struct shell *sh, struct curve_tracer *ct)
{
	int ret;

	/* Set the channels up on first use, so any error goes to this shell */
	if (!ct->ready) {
		ret = setup_channels(sh, ct);
		if (ret != 0) {
			return ret;
		}
	}

	return curve_trace(sh, ct);
}

static int cmd_npn_curvetrace(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return run_curve_trace(sh, &npn);
}

static int cmd_pnp_curvetrace(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return run_curve_trace(sh, &pnp);
}

SHELL_CMD_REGISTER(npn_curvetrace, NULL, "Run the AD5592R NPN curve trace", cmd_npn_curvetrace);
SHELL_CMD_REGISTER(pnp_curvetrace, NULL, "Run the AD5593R PNP curve trace", cmd_pnp_curvetrace);

int main(void)
{
	/*
	 * Print on every shell backend, through the shell, so the text shows up
	 * wherever the commands are typed and does not interleave with the prompt.
	 */
	for (int i = 0; i < shell_backend_count_get(); i++) {
		const struct shell *sh = shell_backend_get(i);

		/* The shell starts in its own thread, give it up to 1 s to come up */
		for (int t = 0; !shell_ready(sh) && t < 100; t++) {
			k_msleep(10);
		}

		if (!shell_ready(sh)) {
			continue;
		}

		shell_print(sh, "ADALM-LSMSPG NPN/PNP curve tracer");
		shell_print(sh, "Type 'npn_curvetrace' to run the NPN curve trace (AD5592R)");
		shell_print(sh, "Type 'pnp_curvetrace' to run the PNP curve trace (AD5593R)");
	}

	return 0;
}

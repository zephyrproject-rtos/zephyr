/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ADALM-LSMSPG AD5592R NPN curve tracer.
 *
 * Port of no-OS projects/adalm-lsmspg curvetrace_example (AD5592R part).
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

#define NUM_CURVES 5
#define NUM_POINTS 50

#define VB_START_MV 499
#define VB_STEP_MV  500
#define VC_START_MV 0
#define VC_STEP_MV  50

#define BASE_SETTLE_MS  50
#define POINT_SETTLE_MS 10

#define GRAPH_HEIGHT 20
#define GRAPH_WIDTH  60

#define RESOLUTION_BITS 12

#define RSENSE_OHM (CONFIG_CURVE_TRACER_RSENSE_MOHM / 1000.0f)
#define RBASE_OHM  ((float)CONFIG_CURVE_TRACER_RBASE_OHM)
#define VBE_V      0.7f

#define USER_NODE DT_PATH(zephyr_user)

static const struct dac_dt_spec vb_drive = DAC_DT_SPEC_GET_BY_NAME(USER_NODE, vb_drive);
static const struct dac_dt_spec vc_drive = DAC_DT_SPEC_GET_BY_NAME(USER_NODE, vc_drive);
static const struct adc_dt_spec vc_sense = ADC_DT_SPEC_GET_BY_NAME(USER_NODE, vc_sense);
static const struct adc_dt_spec vc_drive_meas = ADC_DT_SPEC_GET_BY_NAME(USER_NODE, vc_drive_meas);

static float curve_vcs[NUM_CURVES][NUM_POINTS];
static float curve_ics[NUM_CURVES][NUM_POINTS];

static float mv_per_lsb;

static int read_adc(const struct adc_dt_spec *spec, uint16_t *raw)
{
	int16_t sample;
	struct adc_sequence seq = {
		.buffer = &sample,
		.buffer_size = sizeof(sample),
	};
	int ret;

	ret = adc_sequence_init_dt(spec, &seq);
	if (ret) {
		return ret;
	}

	ret = adc_read_dt(spec, &seq);
	if (ret) {
		return ret;
	}

	*raw = (uint16_t)sample & BIT_MASK(RESOLUTION_BITS);

	return 0;
}

static int write_dac_mv(const struct dac_dt_spec *spec, int mv, uint16_t *raw_out)
{
	uint16_t raw = (uint16_t)CLAMP((float)mv / mv_per_lsb, 0, BIT_MASK(RESOLUTION_BITS));

	if (raw_out) {
		*raw_out = raw;
	}

	return dac_write_value_dt(spec, raw);
}

static int setup_channels(void)
{
	const struct dac_dt_spec *dacs[] = {&vb_drive, &vc_drive};
	const struct adc_dt_spec *adcs[] = {&vc_sense, &vc_drive_meas};
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(dacs); i++) {
		if (!dac_is_ready_dt(dacs[i])) {
			printk("DAC channel %u not ready\n", dacs[i]->channel_id);
			return -ENODEV;
		}
		ret = dac_channel_setup_dt(dacs[i]);
		if (ret) {
			printk("DAC channel %u setup failed: %d\n", dacs[i]->channel_id, ret);
			return ret;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(adcs); i++) {
		if (!adc_is_ready_dt(adcs[i])) {
			printk("ADC channel %u not ready\n", adcs[i]->channel_id);
			return -ENODEV;
		}
		ret = adc_channel_setup_dt(adcs[i]);
		if (ret) {
			printk("ADC channel %u setup failed: %d\n", adcs[i]->channel_id, ret);
			return ret;
		}
	}

	/* Same reference/resolution on every AD5592R channel */
	mv_per_lsb = (float)vc_sense.vref_mv / (1 << RESOLUTION_BITS);

	return 0;
}

static void plot_ascii_graph(void)
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
	grid[0][0] = grid[0][GRAPH_WIDTH + 1] = '+';
	grid[GRAPH_HEIGHT + 1][0] = grid[GRAPH_HEIGHT + 1][GRAPH_WIDTH + 1] = '+';

	for (int c = 0; c < NUM_CURVES; c++) {
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

	for (int c = 0; c < NUM_CURVES; c++) {
		for (int p = 0; p < NUM_POINTS; p++) {
			int x = 1 + (int)((curve_vcs[c][p] / max_v) * (GRAPH_WIDTH - 1));
			int y = 1 + (int)((curve_ics[c][p] / max_i) * (GRAPH_HEIGHT - 1));

			/* Highest current on the top row */
			y = GRAPH_HEIGHT + 1 - y;

			if (x >= 1 && x <= GRAPH_WIDTH && y >= 1 && y <= GRAPH_HEIGHT) {
				grid[y][x] = '*';
			}
		}
	}

	printk("\n\n=== AD5592R (SPI) - NPN Curve Tracer (Ic vs Vc) ===\n");
	printk("Y-axis: Ic (0 to %.2f mA)\n", (double)max_i);
	printk("X-axis: Vc (0 to %.2f V)\n\n", (double)max_v);

	for (int y = 0; y < GRAPH_HEIGHT + 2; y++) {
		printk("%s\n", grid[y]);
	}

	printk("0.0");
	for (int k = 1; k <= 5; k++) {
		printk("       %.2f", (double)(max_v / 5.0f * k));
	}
	printk(" V\n\n===== AD5592R Curve Trace Complete =====\n\n");
}

static int curve_trace(void)
{
	uint16_t vb_raw, vc_sense_raw, vc_meas_raw;
	int64_t start = k_uptime_get();
	int ret;

	printk("\n========== AD5592R (SPI) NPN Curve Tracer ==========\n");
	printk("Vref: %u mV, Scale: %.4f mV/LSB, Rsense: %.1f ohm, Rbase: %.0f ohm\n",
	       vc_sense.vref_mv, (double)mv_per_lsb, (double)RSENSE_OHM, (double)RBASE_OHM);

	ret = write_dac_mv(&vb_drive, 500, NULL);
	ret = ret ? ret : write_dac_mv(&vc_drive, 500, NULL);
	if (ret) {
		printk("Failed to initialize DACs: %d\n", ret);
		return ret;
	}

	printk("\nStarting sweep...\n");

	for (int ci = 0; ci < NUM_CURVES; ci++) {
		ret = write_dac_mv(&vb_drive, VB_START_MV + ci * VB_STEP_MV, &vb_raw);
		if (ret) {
			printk("Failed to write base DAC: %d\n", ret);
			goto out;
		}

		k_msleep(BASE_SETTLE_MS);

		float vb = vb_raw * mv_per_lsb / 1000.0f;
		float ib = (vb - VBE_V) / RBASE_OHM;

		printk("Base Drive: %.4f V, %.3f uA\n", (double)vb, (double)(ib * 1e6f));

		for (int pi = 0; pi < NUM_POINTS; pi++) {
			ret = write_dac_mv(&vc_drive, VC_START_MV + pi * VC_STEP_MV, NULL);
			if (ret) {
				printk("Failed to write collector DAC: %d\n", ret);
				goto out;
			}

			k_msleep(POINT_SETTLE_MS);

			ret = read_adc(&vc_sense, &vc_sense_raw);
			ret = ret ? ret : read_adc(&vc_drive_meas, &vc_meas_raw);
			if (ret) {
				printk("Failed to read ADC: %d\n", ret);
				goto out;
			}

			/* mV / ohm = mA */
			float ic = ((int)vc_meas_raw - (int)vc_sense_raw) * mv_per_lsb / RSENSE_OHM;
			float vc = vc_sense_raw * mv_per_lsb / 1000.0f;

			curve_vcs[ci][pi] = vc;
			curve_ics[ci][pi] = ic;

			printk("  coll voltage: %.4f V  coll current: %.4f mA\n", (double)vc,
			       (double)ic);
		}
	}

	plot_ascii_graph();

	printk("Sweep took %lld ms\n", k_uptime_get() - start);

out:
	/* Leave the transistor unbiased */
	write_dac_mv(&vb_drive, 0, NULL);
	write_dac_mv(&vc_drive, 0, NULL);

	return ret;
}

static int cmd_curvetrace(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return curve_trace();
}

SHELL_CMD_REGISTER(curvetrace, NULL, "Run the AD5592R NPN curve trace", cmd_curvetrace);

int main(void)
{
	int ret;

	printk("ADALM-LSMSPG AD5592R curve tracer on %s\n", CONFIG_BOARD_TARGET);

	ret = setup_channels();
	if (ret) {
		return ret;
	}

	printk("Type 'curvetrace' to run a trace\n");

	return 0;
}

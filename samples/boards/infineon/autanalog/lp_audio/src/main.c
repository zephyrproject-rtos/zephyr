/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Low-power autonomous analog audio sample.
 *
 * Uses the autonomous analog in low power mode to detect acoustic
 * activity and prints the detected audio frame to the console.
 *
 *   1. CTB0 OA0 amplifies the analog microphone signal.
 *   2. PTCOMP comparators perform Acoustic Activity Detection (AAD)
 *      by comparing the preamp output against two PRB reference
 *      voltages.
 *   3. The SAR ADC samples the preamp output in LP mode through an
 *      internal MUX channel.  Results are collected in a 512-word
 *      FIFO; a watermark interrupt fires every 160 samples (~10 ms).
 *   4. The AC state machine is started by ifx_autanalog_fw_trigger().
 *   5. Audio frame statistics are printed to the console using the
 *      ADC stream API (RTIO-based).
 */

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/opamp.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <infineon_autanalog.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

/* ---------- Device references ---------- */
static const struct device *autanalog_dev = DEVICE_DT_GET(DT_NODELABEL(autanalog));
static const struct device *comp0_dev = DEVICE_DT_GET(DT_NODELABEL(comp0));
static const struct device *comp1_dev = DEVICE_DT_GET(DT_NODELABEL(comp1));
static const struct device *opamp_dev = DEVICE_DT_GET(DT_NODELABEL(ctb0_opamp0));

/* ---------- ADC channel from DT ---------- */
#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

/* ---------- ADC Stream (RTIO) ---------- */
#define SAMPLE_ADC_TRIGGERS                                                                        \
	{ADC_TRIG_FIFO_WATERMARK, ADC_STREAM_DATA_INCLUDE},                                        \
	{                                                                                          \
		ADC_TRIG_FIFO_FULL, ADC_STREAM_DATA_INCLUDE                                        \
	}

ADC_DT_STREAM_IODEV(stream_iodev, DT_NODELABEL(adc0), adc_channels, SAMPLE_ADC_TRIGGERS);
RTIO_DEFINE_WITH_MEMPOOL(adc_ctx, 8, 8, 8, 1024, sizeof(void *));

/* ---------- Comparator (AAD) events ---------- */
static volatile uint32_t comp0_events;
static volatile uint32_t comp1_events;

/* ---------- Callbacks ---------- */

static void comp0_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);
	comp0_events++;
}

static void comp1_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);
	comp1_events++;
}

/* ---------- Peripheral setup ---------- */

static int setup_opamp(void)
{
	if (!device_is_ready(opamp_dev)) {
		printk("Opamp device %s not ready\n", opamp_dev->name);
		return -ENODEV;
	}
	printk("Opamp %s ready\n", opamp_dev->name);
	return 0;
}

static int setup_comparators(void)
{
	int err;

	if (!device_is_ready(comp0_dev) || !device_is_ready(comp1_dev)) {
		printk("Comparator device not ready\n");
		return -ENODEV;
	}

	err = comparator_set_trigger(comp0_dev, COMPARATOR_TRIGGER_RISING_EDGE);
	if (err) {
		printk("comp0 trigger setup failed (%d)\n", err);
		return err;
	}
	err = comparator_set_trigger_callback(comp0_dev, comp0_callback, NULL);
	if (err) {
		printk("comp0 callback setup failed (%d)\n", err);
		return err;
	}

	err = comparator_set_trigger(comp1_dev, COMPARATOR_TRIGGER_RISING_EDGE);
	if (err) {
		printk("comp1 trigger setup failed (%d)\n", err);
		return err;
	}
	err = comparator_set_trigger_callback(comp1_dev, comp1_callback, NULL);
	if (err) {
		printk("comp1 callback setup failed (%d)\n", err);
		return err;
	}

	printk("Comparators ready (AAD enabled)\n");
	return 0;
}

static int setup_adc(void)
{
	int err;
	static struct adc_sequence sequence;
	struct adc_read_config *read_cfg = stream_iodev.data;
	struct rtio_sqe *handle;

	if (!adc_is_ready_dt(&adc_channels[0])) {
		printk("ADC device %s not ready\n", adc_channels[0].dev->name);
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&adc_channels[0]);
	if (err) {
		printk("ADC channel setup failed (%d)\n", err);
		return err;
	}

	/* Configure the stream sequence */
	read_cfg->sequence = &sequence;
	err = adc_sequence_init_dt(&adc_channels[0], &sequence);
	if (err) {
		printk("ADC sequence init failed (%d)\n", err);
		return 0;
	}

	err = adc_stream(&stream_iodev, &adc_ctx, NULL, &handle);
	if (err) {
		printk("adc_stream failed (%d)\n", err);
		return 0;
	}

	printk("ADC ready (LP mode, MUX ch0 -> FIFO, stream)\n");
	return 0;
}

/* ---------- Main ---------- */

int main(void)
{
	int err;
	uint8_t *buf;
	uint32_t buf_len;
	uint32_t frame = 0;
	uint32_t prev_comp0 = 0;
	uint32_t prev_comp1 = 0;
	const struct adc_decoder_api *decoder;

	printk("AutAnalog LP audio sample\n");

	err = setup_opamp();
	if (err) {
		return 0;
	}

	err = setup_comparators();
	if (err) {
		return 0;
	}

	err = setup_adc();
	if (err) {
		return 0;
	}

	/* Wait for PRB references and opamp to stabilize */
	k_msleep(100);

	/*
	 * Send FW trigger 0 to the Autonomous Controller.
	 * The AC is already running (started by the MFD driver at boot).
	 * It is parked in state 1, waiting for TR_IN0.  This trigger
	 * advances the AC into the periodic SAR sampling loop
	 * (states 6->7->8->9).
	 */
	ifx_autanalog_fw_trigger(autanalog_dev, IFX_AUTANALOG_AC_FW_TRIGGER_0);
	printk("Waiting for audio data...\n\n");

	err = adc_get_decoder(adc_channels[0].dev, &decoder);
	if (err) {
		printk("adc_get_decoder failed (%d)\n", err);
		return 0;
	}

#ifndef CONFIG_COVERAGE
	while (1) {
#else
	for (int k = 0; k < 10; k++) {
#endif
		/* Block until the next FIFO watermark/overflow event */
		struct rtio_cqe *cqe = rtio_cqe_consume_block(&adc_ctx);

		if (cqe->result != 0) {
			printk("Stream error %d\n", cqe->result);
			rtio_cqe_release(&adc_ctx, cqe);
			break;
		}

		err = rtio_cqe_get_mempool_buffer(&adc_ctx, cqe, &buf, &buf_len);
		rtio_cqe_release(&adc_ctx, cqe);

		if (err) {
			printk("Failed to get mempool buffer (%d)\n", err);
			break;
		}

		/* Check whether any new comparator events fired */
		if (comp0_events != prev_comp0 || comp1_events != prev_comp1) {
			/* Decode frame count for channel 0 */
			uint16_t frame_count = 0;

			decoder->get_frame_count(buf, 0, &frame_count);

			/* Decode all samples and compute statistics */
			int32_t min_val = INT32_MAX;
			int32_t max_val = INT32_MIN;
			int64_t sum = 0;
			uint32_t fit = 0;

			for (uint16_t i = 0; i < frame_count; i++) {
				struct adc_data adc_data = {0};

				decoder->decode(buf, 0, &fit, 1, &adc_data);
				int32_t v = adc_data.readings[0].value;

				if (v < min_val) {
					min_val = v;
				}
				if (v > max_val) {
					max_val = v;
				}
				sum += v;
			}

			printk("Frame %u: %u samples, min=%d max=%d avg=%d\n", frame++, frame_count,
			       (int)min_val, (int)max_val,
			       frame_count ? (int)(sum / frame_count) : 0);
			printk("  AAD: comp0=%u comp1=%u\n", comp0_events, comp1_events);

			prev_comp0 = comp0_events;
			prev_comp1 = comp1_events;
		}

		rtio_release_buffer(&adc_ctx, buf, buf_len);

		/*
		 * After processing a FIFO frame, send FW trigger 1
		 * so the AC exits the SAR loop (state 9->10->3)
		 * and returns to AAD idle.  TR_IN1 is sticky but
		 * consumed the first time state 9 evaluates it,
		 * so we must re-send it each cycle.
		 */
		ifx_autanalog_fw_trigger(autanalog_dev, IFX_AUTANALOG_AC_FW_TRIGGER_1);
	}

	return 0;
}

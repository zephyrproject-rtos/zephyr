/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/adc/infineon-autanalog-sar.h>
#include <zephyr/drivers/adc/infineon_autanalog_sar.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

/* FIR filter 1: 4-tap moving average coefficients in Q1.15 fixed-point.
 * Each coefficient is 0.25 (0x2000).
 */
static const int16_t fir1_coefficients[] = {0x2000, 0x2000, 0x2000, 0x2000};

#define FIFO_BUF_RAW  0 /* FIFO buffer 0: GPIO channel 0 */
#define FIFO_BUF_MUX  1 /* FIFO buffer 1: MUX channel 0 */
#define FIFO_BUF_FIR0 2 /* FIFO buffer 2: FIR0 filtered */
#define FIFO_BUF_FIR1 3 /* FIFO buffer 3: FIR1 filtered */
#define FIFO_READ_MAX 8

static volatile uint32_t fifo_int_status;
static int32_t fifo_raw_data[FIFO_READ_MAX];
static int32_t fifo_mux_data[FIFO_READ_MAX];
static int32_t fifo_fir0_data[FIFO_READ_MAX];
static int32_t fifo_fir1_data[FIFO_READ_MAX];
static volatile int fifo_raw_count;
static volatile int fifo_mux_count;
static volatile int fifo_fir0_count;
static volatile int fifo_fir1_count;
static const char *const fifo_names[] = {"GPIO", "MUX ", "FIR0", "FIR1"};
static const int32_t *const fifo_bufs[] = {fifo_raw_data, fifo_mux_data, fifo_fir0_data,
					   fifo_fir1_data};
static volatile int *const fifo_counts[] = {&fifo_raw_count, &fifo_mux_count, &fifo_fir0_count,
					    &fifo_fir1_count};

static void fifo_callback(const struct device *dev, uint32_t status, void *user_data)
{
	ARG_UNUSED(user_data);

	fifo_int_status = status;

	fifo_raw_count =
		adc_ifx_autanalog_sar_fifo_read(dev, FIFO_BUF_RAW, fifo_raw_data, FIFO_READ_MAX);
	fifo_mux_count =
		adc_ifx_autanalog_sar_fifo_read(dev, FIFO_BUF_MUX, fifo_mux_data, FIFO_READ_MAX);
	fifo_fir0_count =
		adc_ifx_autanalog_sar_fifo_read(dev, FIFO_BUF_FIR0, fifo_fir0_data, FIFO_READ_MAX);
	fifo_fir1_count =
		adc_ifx_autanalog_sar_fifo_read(dev, FIFO_BUF_FIR1, fifo_fir1_data, FIFO_READ_MAX);
}

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

static int setup_adc(void)
{
	int err;

	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return -ENODEV;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return err;
		}
	}

	/* Load FIR filter 1 coefficients (FIR0 is loaded from DT) */
	err = adc_ifx_autanalog_sar_fir_load_coefficients(adc_channels[0].dev, 1, fir1_coefficients,
							  ARRAY_SIZE(fir1_coefficients));
	if (err < 0) {
		printk("Failed to load FIR1 coefficients (%d)\n", err);
		return err;
	}

	/* Register FIFO watermark interrupt callback */
	err = adc_ifx_autanalog_sar_fifo_set_callback(adc_channels[0].dev, fifo_callback, NULL);
	if (err < 0) {
		printk("Failed to set FIFO callback (%d)\n", err);
		return err;
	}

	return 0;
}

int main(void)
{
	int err;
	uint32_t count = 0;
	int32_t buf[4];
	struct adc_sequence sequence = {
		.buffer = buf,
		.buffer_size = sizeof(buf),
		.resolution = 12,
	};

	err = setup_adc();
	if (err < 0) {
		printk("Failed to setup ADC (%d)\n", err);
		return 0;
	}

	/* Build a combined channel mask for all configured channels */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		sequence.channels |= BIT(adc_channels[i].channel_id);
	}

	while (1) {
		err = adc_read(adc_channels[0].dev, &sequence);
		if (err < 0) {
			printk("ADC read error (%d)\n", err);
			k_sleep(K_MSEC(1000));
			continue;
		}

		printk("ADC reading[%u]:\n", count++);
		for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
			int32_t val_mv = buf[i];

			printk("- %s, channel %d: %" PRId32, adc_channels[i].dev->name,
			       adc_channels[i].channel_id, val_mv);

			err = adc_raw_to_millivolts_dt(&adc_channels[i], &val_mv);
			if (err < 0) {
				printk(" (value in mV not available)\n");
			} else {
				printk(" = %" PRId32 " mV\n", val_mv);
			}
		}

		if (fifo_int_status != 0) {
			printk("FIFO: watermark event (status=0x%08x)\n", fifo_int_status);
			for (int idx = 0; idx < (int)ARRAY_SIZE(fifo_names); idx++) {
				printk("FIFO[%d] %s: %d words -", idx, fifo_names[idx],
				       *fifo_counts[idx]);
				for (int j = 0; j < *fifo_counts[idx]; j++) {
					printk(" %d", (int)fifo_bufs[idx][j]);
				}
				printk("\n");
			}
			fifo_int_status = 0;
		}

		k_sleep(K_MSEC(1000));
		printk("\n");
	}
	return 0;
}

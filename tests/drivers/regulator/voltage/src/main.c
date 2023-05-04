/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define REG_INIT(node_id, prop, idx) \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define ADC_INIT(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct device *regs[] = {
	DT_FOREACH_PROP_ELEM(DT_NODELABEL(resources), regulators, REG_INIT)
};

static const struct adc_dt_spec adc_chs[] = {
	DT_FOREACH_PROP_ELEM(DT_NODELABEL(resources), io_channels, ADC_INIT)
};

static const int32_t tols[] =
	DT_PROP(DT_NODELABEL(resources), tolerance_microvolt);

static const unsigned int adc_avg_count = DT_PROP(DT_NODELABEL(resources),
						  adc_avg_count);
static const int32_t set_read_delay_ms = DT_PROP(DT_NODELABEL(resources),
						 set_read_delay_ms);

static const int32_t min_microvolt = DT_PROP(DT_NODELABEL(resources), min_microvolt);
static const int32_t max_microvolt = DT_PROP(DT_NODELABEL(resources), max_microvolt);

ZTEST(regulator_voltage, test_output_voltage)
{
	int16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
	};

	for (size_t i = 0U; i < ARRAY_SIZE(regs); i++) {
		int ret;
		unsigned int volt_cnt;
		int32_t volt_uv;

		ret = adc_sequence_init_dt(&adc_chs[i], &sequence);
		zassert_equal(ret, 0);

		volt_cnt = regulator_count_voltages(regs[i]);
		zassume_not_equal(volt_cnt, 0U);

		TC_PRINT("Testing %s, %u voltage/s (tolerance: %d uV)\n",
			 regs[i]->name, volt_cnt, tols[i]);

		ret = regulator_enable(regs[i]);
		zassert_equal(ret, 0);

		for (unsigned int j = 0U; j < volt_cnt; j++) {
			int32_t val_mv = 0;

			(void)regulator_list_voltage(regs[i], j, &volt_uv);
			/* Check if voltage is outside user constraints */
			if (!regulator_is_supported_voltage(regs[i],
				volt_uv, volt_uv)) {
				continue;
			}

			if ((volt_uv < min_microvolt) || (volt_uv > max_microvolt)) {
				TC_PRINT("Skip: %d uV\n", volt_uv);
				continue;
			}

			ret = regulator_set_voltage(regs[i], volt_uv, volt_uv);
			zassert_equal(ret, 0);

			if (set_read_delay_ms > 0) {
				k_msleep(set_read_delay_ms);
			}

			for (unsigned int k = 0U; k < adc_avg_count; k++) {
				ret = adc_read(adc_chs[i].dev, &sequence);
				zassert_equal(ret, 0);

				val_mv += buf;
			}

			val_mv /= (int32_t)adc_avg_count;

			ret = adc_raw_to_millivolts_dt(&adc_chs[i], &val_mv);
			zassert_equal(ret, 0);

			TC_PRINT("Set: %d, read: %d uV\n", volt_uv,
				 val_mv * 1000);

			zassert_between_inclusive(val_mv * 1000,
						  volt_uv - tols[i],
						  volt_uv + tols[i]);
		}

		ret = regulator_disable(regs[i]);
		zassert_equal(ret, 0);
	}
}

void *setup(void)
{
	zassert_equal(ARRAY_SIZE(regs), ARRAY_SIZE(adc_chs));
	zassert_equal(ARRAY_SIZE(regs), ARRAY_SIZE(tols));

	for (size_t i = 0U; i < ARRAY_SIZE(regs); i++) {
		zassert_true(device_is_ready(regs[i]));
		zassert_true(device_is_ready(adc_chs[i].dev));
		zassert_equal(adc_channel_setup_dt(&adc_chs[i]), 0);
	}

	return NULL;
}

ZTEST_SUITE(regulator_voltage, NULL, setup, NULL, NULL, NULL);

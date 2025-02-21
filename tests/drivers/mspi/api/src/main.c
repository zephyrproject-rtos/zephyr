/*
 * Copyright (c) 2024 Ambiq Micro Inc. <www.ambiq.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi_emul.h>
#include <zephyr/ztest.h>

#define TEST_MSPI_REINIT    1

/* add else if for other SoC platforms */
#if defined(CONFIG_SOC_POSIX)
typedef struct mspi_timing_cfg mspi_timing_cfg;
typedef enum mspi_timing_param mspi_timing_param;
#elif defined(CONFIG_SOC_FAMILY_AMBIQ)
#include "mspi_ambiq.h"
typedef struct mspi_ambiq_timing_cfg mspi_timing_cfg;
typedef enum mspi_ambiq_timing_param mspi_timing_param;
#endif

#define MSPI_BUS_NODE       DT_ALIAS(mspi0)

static const struct device *mspi_devices[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, DEVICE_DT_GET, (,))
};

static struct gpio_dt_spec ce_gpios[] = MSPI_CE_GPIOS_DT_SPEC_GET(MSPI_BUS_NODE);

#if TEST_MSPI_REINIT
struct mspi_cfg hardware_cfg = {
	.channel_num              = 0,
	.op_mode                  = DT_ENUM_IDX_OR(MSPI_BUS_NODE, op_mode, MSPI_OP_MODE_CONTROLLER),
	.duplex                   = DT_ENUM_IDX_OR(MSPI_BUS_NODE, duplex, MSPI_HALF_DUPLEX),
	.dqs_support              = DT_PROP_OR(MSPI_BUS_NODE, dqs_support, false),
	.ce_group                 = ce_gpios,
	.num_ce_gpios             = ARRAY_SIZE(ce_gpios),
	.num_periph               = DT_CHILD_NUM(MSPI_BUS_NODE),
	.max_freq                 = DT_PROP(MSPI_BUS_NODE, clock_frequency),
	.re_init                  = true,
};
#endif


static struct mspi_dev_id dev_id[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_DEVICE_ID_DT, (,))
};

static struct mspi_dev_cfg device_cfg[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_DEVICE_CONFIG_DT, (,))
};

#if CONFIG_MSPI_XIP
static struct mspi_xip_cfg xip_cfg[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_XIP_CONFIG_DT, (,))
};
#endif

#if CONFIG_MSPI_SCRAMBLE
static struct mspi_scramble_cfg scramble_cfg[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(MSPI_BUS_NODE, MSPI_SCRAMBLE_CONFIG_DT, (,))
};
#endif

ZTEST(mspi_api, test_mspi_api)
{
	int ret = 0;
	const struct device *mspi_bus = DEVICE_DT_GET(MSPI_BUS_NODE);

	zassert_true(device_is_ready(mspi_bus), "mspi_bus is not ready");

#if TEST_MSPI_REINIT
	const struct mspi_dt_spec spec = {
		.bus    = mspi_bus,
		.config = hardware_cfg,
	};

	ret = mspi_config(&spec);
	zassert_equal(ret, 0, "mspi_config failed.");
#endif

	for (int dev_idx = 0; dev_idx < ARRAY_SIZE(mspi_devices); ++dev_idx) {

		zassert_true(device_is_ready(mspi_devices[dev_idx]), "mspi_device is not ready");

		ret = mspi_dev_config(mspi_bus, &dev_id[dev_idx],
				      MSPI_DEVICE_CONFIG_ALL, &device_cfg[dev_idx]);
		zassert_equal(ret, 0, "mspi_dev_config failed.");

#if CONFIG_MSPI_XIP
		ret = mspi_xip_config(mspi_bus, &dev_id[dev_idx], &xip_cfg[dev_idx]);
		zassert_equal(ret, 0, "mspi_xip_config failed.");
#endif

#if CONFIG_MSPI_SCRAMBLE
		ret = mspi_scramble_config(mspi_bus, &dev_id[dev_idx], &scramble_cfg[dev_idx]);
		zassert_equal(ret, 0, "mspi_scramble_config failed.");
#endif

#if CONFIG_MSPI_TIMING
		mspi_timing_cfg timing_cfg;
		mspi_timing_param timing_cfg_mask = 0;

		ret = mspi_timing_config(mspi_bus, &dev_id[dev_idx], timing_cfg_mask, &timing_cfg);
		zassert_equal(ret, 0, "mspi_timing_config failed.");
#endif

		ret = mspi_register_callback(mspi_bus, &dev_id[dev_idx],
					     MSPI_BUS_XFER_COMPLETE, NULL, NULL);
		if (ret == -ENOTSUP) {
			printf("mspi_register_callback not supported.\n");
		} else {
			zassert_equal(ret, 0, "mspi_register_callback failed.");
		}

		ret = mspi_get_channel_status(mspi_bus, 0);
		zassert_equal(ret, 0, "mspi_get_channel_status failed.");

	}
}

ZTEST_SUITE(mspi_api, NULL, NULL, NULL, NULL, NULL);

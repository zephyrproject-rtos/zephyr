/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crc_example, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/crc.h>

/* The devicetree node identifier for the "crc" */
#define CRC_NODE DT_CHOSEN(zephyr_crc)

/* Pre-select CRC variant and constants via Kconfig */
#if defined(CONFIG_SAMPLE_CRC_VARIANT_CRC8)
#if !IS_ENABLED(CONFIG_CRC_DRIVER_HAS_CRC8)
#error "Selected CRC8 but driver/platform does not support it"
#endif
#define CRC_SAMPLE_TYPE     CRC8
#define CRC_SAMPLE_POLY     CRC8_POLY
#define CRC_SAMPLE_SEED     CRC8_INIT_VAL
#define CRC_SAMPLE_REVERSED (CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT)
#define CRC_SAMPLE_NAME     "CRC8"
#define CRC_SAMPLE_EXPECTED 0xB2
#elif defined(CONFIG_SAMPLE_CRC_VARIANT_CRC16_CCITT)
#if !IS_ENABLED(CONFIG_CRC_DRIVER_HAS_CRC16_CCITT) && !IS_ENABLED(CONFIG_CRC_DRIVER_HAS_CRC16)
#error "Selected CRC16-CCITT but platform does not support it"
#endif
#define CRC_SAMPLE_TYPE     CRC16_CCITT
#define CRC_SAMPLE_POLY     CRC16_CCITT_POLY
#define CRC_SAMPLE_SEED     CRC16_CCITT_INIT_VAL
#define CRC_SAMPLE_REVERSED (CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT)
#define CRC_SAMPLE_NAME     "CRC16-CCITT"
#define CRC_SAMPLE_EXPECTED 0x445c
#elif defined(CONFIG_SAMPLE_CRC_VARIANT_CRC32_IEEE)
#if !IS_ENABLED(CONFIG_CRC_DRIVER_HAS_CRC32_IEEE)
#error "Selected CRC32-IEEE but platform does not support it"
#endif
#define CRC_SAMPLE_TYPE     CRC32_IEEE
#define CRC_SAMPLE_POLY     CRC32_IEEE_POLY
#define CRC_SAMPLE_SEED     CRC32_IEEE_INIT_VAL
#define CRC_SAMPLE_REVERSED (CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT)
#define CRC_SAMPLE_NAME     "CRC32-IEEE"
#define CRC_SAMPLE_EXPECTED 0xCEA4A6C2
#elif defined(CONFIG_SAMPLE_CRC_VARIANT_CRC32_C)
#if !IS_ENABLED(CONFIG_CRC_DRIVER_HAS_CRC32_C)
#error "Selected CRC32-C but platform does not support it"
#endif
#define CRC_SAMPLE_TYPE     CRC32_C
#define CRC_SAMPLE_POLY     CRC32_C_POLY
#define CRC_SAMPLE_SEED     CRC32_C_INIT_VAL
#define CRC_SAMPLE_REVERSED (CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT)
#define CRC_SAMPLE_NAME     "CRC32-C"
#define CRC_SAMPLE_EXPECTED 0xBB19ECB2
#else
#error "No CRC sample variant selected"
#endif

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */

int main(void)
{
	static const struct device *const dev = DEVICE_DT_GET(CRC_NODE);
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};
	int ret;

	if (!device_is_ready(dev)) {
		LOG_ERR("Device is not ready");
		return -ENODEV;
	}

	struct crc_ctx ctx = {
		.type = CRC_SAMPLE_TYPE,
		.polynomial = CRC_SAMPLE_POLY,
		.seed = CRC_SAMPLE_SEED,
		.reversed = CRC_SAMPLE_REVERSED,
	};

	ret = crc_begin(dev, &ctx);
	if (ret != 0) {
		LOG_ERR("Failed to begin %s: %d", CRC_SAMPLE_NAME, ret);
		return ret;
	}

	ret = crc_update(dev, &ctx, data, sizeof(data));
	if (ret != 0) {
		LOG_ERR("Failed to update %s: %d", CRC_SAMPLE_NAME, ret);
		return ret;
	}

	ret = crc_finish(dev, &ctx);
	if (ret != 0) {
		LOG_ERR("Failed to finish %s: %d", CRC_SAMPLE_NAME, ret);
		return ret;
	}

	LOG_INF("%s result: 0x%08x", CRC_SAMPLE_NAME, (unsigned int)ctx.result);

#if defined(CRC_SAMPLE_EXPECTED) && (CRC_SAMPLE_EXPECTED != 0)
	ret = crc_verify(&ctx, CRC_SAMPLE_EXPECTED);
	if (ret != 0) {
		LOG_ERR("%s verification failed (expected 0x%08x): %d", CRC_SAMPLE_NAME,
			CRC_SAMPLE_EXPECTED, ret);
		return ret;
	}
	LOG_INF("%s verification succeeded (expected 0x%08x)", CRC_SAMPLE_NAME,
		CRC_SAMPLE_EXPECTED);
#endif

	return 0;
}

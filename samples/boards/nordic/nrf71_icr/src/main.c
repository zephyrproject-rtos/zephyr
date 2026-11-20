/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf71_icr.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(nrf71_icr_sample);

#define UICR_NODE                DT_NODELABEL(uicr)
#define EXPECTED_XTALCOMP_OFFSET DT_PROP(UICR_NODE, xtalcomp_offset)

struct sample_register {
	enum nrf71_icr_register reg;
	const char *name;
};

static const struct sample_register sample_registers[] = {
	{NRF71_ICR_MACADDR0_LOW, "MACADDR0.LOW"},
	{NRF71_ICR_MACADDR0_HIGH, "MACADDR0.HIGH"},
	{NRF71_ICR_MACADDR1_LOW, "MACADDR1.LOW"},
	{NRF71_ICR_MACADDR1_HIGH, "MACADDR1.HIGH"},
	{NRF71_ICR_XTALCOMP_OFFSET, "XTALCOMP.OFFSET"},
	{NRF71_ICR_XTALCOMP_LOWTEMPTHRESHOLD, "XTALCOMP.LOWTEMPTHRESHOLD"},
	{NRF71_ICR_XTALCOMP_LOWTEMPADJUST, "XTALCOMP.LOWTEMPADJUST"},
};

static const char *const source_names[] = {
	[NRF71_ICR_SOURCE_FICR] = "FICR",
	[NRF71_ICR_SOURCE_UICR] = "UICR",
};

BUILD_ASSERT(ARRAY_SIZE(sample_registers) == NRF71_ICR_REGISTER_COUNT);
BUILD_ASSERT(DT_NODE_HAS_PROP(UICR_NODE, xtalcomp_offset));

int main(void)
{
	bool xtalcomp_offset_verified = false;

	LOG_INF("nRF71 ICR values:");

	ARRAY_FOR_EACH(sample_registers, i) {
		uint32_t result = 0;
		int ret = nrf71_icr_read(sample_registers[i].reg, &result);

		if (ret == -EINVAL) {
			LOG_ERR("%-28s: error %d", sample_registers[i].name, ret);
			continue;
		} else if (ret == -ENODATA) {
			LOG_INF("%-28s: no xICR read, using default value", sample_registers[i].name);
			continue;
		} else {
			LOG_INF("%-28s: 0x%08x (%s)", sample_registers[i].name, result,
			source_names[ret]);
		}

		if (sample_registers[i].reg == NRF71_ICR_XTALCOMP_OFFSET) {
			xtalcomp_offset_verified = (ret == NRF71_ICR_SOURCE_UICR) &&
						   (result == EXPECTED_XTALCOMP_OFFSET);
		}
	}

	if (!xtalcomp_offset_verified) {
		LOG_ERR("XTALCOMP.OFFSET UICR verification failed");
		return 1;
	} else {
		LOG_INF("XTALCOMP.OFFSET UICR verification passed");
	}

	return 0;
}

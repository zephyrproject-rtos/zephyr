/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/sys/util_macro.h>

#define TRNG_NODE DT_INST(0, nxp_els_trng)
#define ELS_NODE DT_PARENT(TRNG_NODE)
#define CLOCK_DEV DT_CLOCKS_CTLR(ELS_NODE)
#define CLOCK_CELL DT_CLOCKS_CELL(ELS_NODE, name)
#define ELS_BASE DT_REG_ADDR(ELS_NODE)
#define ELS_STATUS (ELS_BASE + 0U)
#define ELS_STATUS_BUSY_MASK BIT(0)
#define ELS_STATUS_DRBG_ENT_LVL_MASK (BIT(9) | BIT(8))
#define ELS_CTRL (ELS_BASE + 4U)
#define ELS_CTRL_EN_MASK BIT(0)
#define ELS_PRNG_DATOUT (ELS_BASE + 0x5c)

static uint8_t els_entropy_level(volatile uint32_t *status)
{
	uint32_t val = *status;
	uint32_t level = (val & ELS_STATUS_DRBG_ENT_LVL_MASK) >> 8;

	return (uint8_t)level;
}

static int entropy_els_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	volatile uint32_t *status = (uint32_t *)ELS_STATUS;
	volatile uint32_t *prng_datout = (uint32_t *)ELS_PRNG_DATOUT;

	if (els_entropy_level(status) < 1) {
		return -EIO;
	}

	uint8_t byte_cnt = 0;
	uint32_t val = 0;
	uint8_t num = 0;

	while (len-- > 0) {
		if (byte_cnt == 0) {
			val = *prng_datout;
		}
		num = val >> (8 * byte_cnt);
		byte_cnt = (byte_cnt + 1) % 4;
		*buf = num;
		buf++;
	}

	return 0;
}

static int entropy_els_init(const struct device *dev)
{
	volatile uint32_t *status = (uint32_t *)ELS_STATUS;
	volatile uint32_t *ctrl = (uint32_t *)ELS_CTRL;
	int ret;

	ret = clock_control_on(DEVICE_DT_GET(CLOCK_DEV), (clock_control_subsys_t)CLOCK_CELL);
	if (ret) {
		return ret;
	}

	*ctrl |= ELS_CTRL_EN_MASK;

	while (*status & ELS_STATUS_BUSY_MASK) {
	}

	return 0;
}

static DEVICE_API(entropy, entropy_els_api_funcs) = {
	.get_entropy = entropy_els_get_entropy
};

DEVICE_DT_DEFINE(TRNG_NODE, entropy_els_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_ENTROPY_INIT_PRIORITY, &entropy_els_api_funcs);

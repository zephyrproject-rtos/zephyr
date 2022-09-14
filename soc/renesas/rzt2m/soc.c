/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <stdint.h>
#include <zephyr/drivers/syscon.h>
#include "soc.h"
#include <zephyr/sys/util_macro.h>

static const struct device *const prcrn_dev = DEVICE_DT_GET(DT_NODELABEL(prcrn));
static const struct device *const prcrs_dev = DEVICE_DT_GET(DT_NODELABEL(prcrs));

void rzt2m_unlock_prcrn(uint32_t mask)
{
	uint32_t prcrn;

	syscon_read_reg(prcrn_dev, 0, &prcrn);
	prcrn |= PRC_KEY_CODE | mask;

	syscon_write_reg(prcrn_dev, 0, prcrn);
}

void rzt2m_lock_prcrn(uint32_t mask)
{
	uint32_t prcrn;

	syscon_read_reg(prcrn_dev, 0, &prcrn);
	prcrn &= ~mask;
	prcrn |= PRC_KEY_CODE;

	syscon_write_reg(prcrn_dev, 0, prcrn);
}

void rzt2m_unlock_prcrs(uint32_t mask)
{
	uint32_t prcrs;

	syscon_read_reg(prcrs_dev, 0, &prcrs);
	prcrs |= PRC_KEY_CODE | mask;

	syscon_write_reg(prcrs_dev, 0, prcrs);
}

void rzt2m_lock_prcrs(uint32_t mask)
{
	uint32_t prcrs;

	syscon_read_reg(prcrs_dev, 0, &prcrs);
	prcrs &= ~mask;
	prcrs |= PRC_KEY_CODE;

	syscon_write_reg(prcrs_dev, 0, prcrs);
}

void rzt2m_enable_counters(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(gsc));

	syscon_write_reg(dev, 0, CNTCR_EN);
}

static int rzt2m_init(void)
{
	/* Unlock the Protect Registers
	 * so that device drivers can access configuration registers of peripherals.
	 */
	/* After the device drivers are done, lock the Protect Registers. */

	rzt2m_enable_counters();
	return 0;
}

SYS_INIT(rzt2m_init, PRE_KERNEL_1, 0);

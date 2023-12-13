/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pinctrl_xlnx_zynq, CONFIG_PINCTRL_LOG_LEVEL);

#define DT_DRV_COMPAT xlnx_pinctrl_zynq

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Unsupported number of instances");

/* Relative SLCR register offsets for use in asserts */
#define MIO_PIN_53_OFFSET    0x00d4
#define SD0_WP_CD_SEL_OFFSET 0x0130
#define SD1_WP_CD_SEL_OFFSET 0x0134

/* SLCR lock offsets and keys */
#define SLCR_LOCK_OFFSET   0x0004
#define SLCR_UNLOCK_OFFSET 0x0008
#define SLCR_STS_OFFSET    0x000C
#define SLCR_LOCK_KEY      0x0000767B
#define SLCR_UNLOCK_KEY    0x0000DF0D

static const struct device *const slcr = DEVICE_DT_GET(DT_INST_PHANDLE(0, syscon));
static mm_reg_t base = DT_INST_REG_ADDR(0);
K_SEM_DEFINE(pinctrl_lock, 1, 1);

static ALWAYS_INLINE int32_t slcr_unlock()
{
	return syscon_write_reg(slcr, SLCR_UNLOCK_OFFSET, SLCR_UNLOCK_KEY);
}
static ALWAYS_INLINE int32_t slcr_lock()
{
	return syscon_write_reg(slcr, SLCR_LOCK_OFFSET, SLCR_LOCK_KEY);
}
static ALWAYS_INLINE int32_t slcr_locked(int32_t *val)
{
	return syscon_read_reg(slcr, SLCR_STS_OFFSET, val);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	uint16_t addr;
	uint32_t val;
	uint8_t i;
	int err = 0;
	int slcr_locked_val = 0;

	ARG_UNUSED(reg);

	if (!device_is_ready(slcr)) {
		LOG_ERR("SLCR device not ready");
		return -ENODEV;
	}

	/* Guard the read-modify-write operations */
	k_sem_take(&pinctrl_lock, K_FOREVER);

	/* NOTE(pan): SLCR is already unlocked at zephyr/soc/arm/xilinx_zynq/xc_xx/soc.c  */

	/* read lock state */
	err = slcr_locked(&slcr_locked_val);
	if (err != 0) {
		LOG_ERR("failed to read SLCR lock (err %d)", err);
		return err;
	}

	/* unlock slcr, if locked */
	if (slcr_locked_val == 1) {
		slcr_unlock();
		err = slcr_unlock();
		if (err != 0) {
			LOG_ERR("failed to unlock SLCR (err %d)", err);
			return err;
		}
	}

	for (i = 0; i < pin_cnt; i++) {
		__ASSERT_NO_MSG(pins[i].offset <= MIO_PIN_53_OFFSET ||
				pins[i].offset == SD0_WP_CD_SEL_OFFSET ||
				pins[i].offset == SD1_WP_CD_SEL_OFFSET);

		addr = base + pins[i].offset;

		err = syscon_read_reg(slcr, addr, &val);
		if (err != 0) {
			LOG_ERR("failed to read SLCR addr 0x%04x (err %d)", addr, err);
			break;
		}

		LOG_DBG("0x%04x: mask 0x%08x, val 0x%08x", addr, pins[i].mask, pins[i].val);
		LOG_DBG("0x%04x r: 0x%08x", addr, val);

		val &= ~(pins[i].mask);
		val |= pins[i].val;

		LOG_DBG("0x%04x w: 0x%08x", addr, val);

		// write syscon shall unlock the slcr first
		err = syscon_write_reg(slcr, addr, val);
		if (err != 0) {
			LOG_ERR("failed to write SLCR addr 0x%04x (err %d)", addr, err);
			break;
		}
	}

	/* restore lock state */
	if (slcr_locked_val == 1) {
		err = slcr_lock();
		if (err != 0) {
			LOG_ERR("failed to lock SLCR (err %d)", err);
			return err;
		}
	}

	k_sem_give(&pinctrl_lock);

	return err;
}

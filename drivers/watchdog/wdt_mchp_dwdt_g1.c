/*
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_dwdt_g1_watchdog

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/*
 * Access the programmable-secure watchdog register bank.
 *
 * The Devicetree reg address must point to the PSWDT control-register
 * base for the selected SoC. Register field definitions and DWDT-block
 * offsets come from the active SoC's official component header.
 */

#include <component/dwdt.h>

/*
 * The official DWDT register offsets are relative to the DWDT block base.
 * Devicetree supplies the PSWDT register-bank base, so subtract the
 * official PSWDT CR offset to obtain offsets relative to that base.
 * This keeps the driver independent of the PSWDT bank's location within
 * the DWDT block on each SoC.
 */

#define PS_WDT_CR_OFFSET \
	(DWDT_PS_WDT_CR_REG_OFST - DWDT_PS_WDT_CR_REG_OFST)
#define PS_WDT_MR_OFFSET \
	(DWDT_PS_WDT_MR_REG_OFST - DWDT_PS_WDT_CR_REG_OFST)
#define PS_WDT_VR_OFFSET \
	(DWDT_PS_WDT_VR_REG_OFST - DWDT_PS_WDT_CR_REG_OFST)
#define PS_WDT_WL_OFFSET \
	(DWDT_PS_WDT_WL_REG_OFST - DWDT_PS_WDT_CR_REG_OFST)
#define PS_WDT_IL_OFFSET \
	(DWDT_PS_WDT_IL_REG_OFST - DWDT_PS_WDT_CR_REG_OFST)
#define PS_WDT_IER_OFFSET \
	(DWDT_PS_WDT_IER_REG_OFST - DWDT_PS_WDT_CR_REG_OFST)
#define PS_WDT_IDR_OFFSET \
	(DWDT_PS_WDT_IDR_REG_OFST - DWDT_PS_WDT_CR_REG_OFST)
#define PS_WDT_ISR_OFFSET \
	(DWDT_PS_WDT_ISR_REG_OFST - DWDT_PS_WDT_CR_REG_OFST)
#define PS_WDT_IMR_OFFSET \
	(DWDT_PS_WDT_IMR_REG_OFST - DWDT_PS_WDT_CR_REG_OFST)

/*
 * The watchdog clock is MD_SLCK / 128. This uses the typical MD_SLCK
 * frequency of 32.768 kHz, giving 256 watchdog ticks per second.
 */
#define PS_WDT_TICKS_PER_SEC 256U

#define PS_WDT_MAX_TICKS \
	(DWDT_PS_WDT_WL_PERIOD_Msk >> DWDT_PS_WDT_WL_PERIOD_Pos)

struct dwdt_config {
	uintptr_t base;
	void (*irq_config_func)(const struct device *dev);
};

struct dwdt_data {
	struct k_spinlock lock;
	wdt_callback_t callback;
	uint16_t period_ticks;
	uint16_t repeat_ticks;
	uint32_t callback_irq_mask;
	uint8_t flags;
	bool timeout_installed;
	bool running;
};

static inline uint32_t dwdt_read(const struct device *dev, uint32_t offset)
{
	const struct dwdt_config *cfg = dev->config;

	return sys_read32(cfg->base + offset);
}

static inline void dwdt_write(const struct device *dev, uint32_t offset,
			      uint32_t value)
{
	const struct dwdt_config *cfg = dev->config;

	sys_write32(value, cfg->base + offset);
}

static uint32_t dwdt_ms_to_ticks(uint32_t ms)
{
	/* Round up so the programmed watchdog period is not shorter than
	 * asked.
	 */
	return (uint32_t)(((uint64_t)ms * PS_WDT_TICKS_PER_SEC + 999U) /
		1000U);
}

static int dwdt_install_timeout(const struct device *dev,
				const struct wdt_timeout_cfg *cfg)
{
	struct dwdt_data *data = dev->data;
	uint32_t period;
	uint32_t repeat;
	k_spinlock_key_t key;

	if (cfg == NULL || cfg->window.max == 0U ||
	    cfg->window.min >= cfg->window.max) {
		return -EINVAL;
	}

	if (cfg->flags != WDT_FLAG_RESET_NONE &&
	    cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	period = dwdt_ms_to_ticks(cfg->window.max);
	repeat = dwdt_ms_to_ticks(cfg->window.min);

	if (period == 0U || period > PS_WDT_MAX_TICKS ||
	    repeat > PS_WDT_MAX_TICKS || repeat >= period) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (data->running) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->period_ticks = (uint16_t)period;
	data->repeat_ticks = (uint16_t)repeat;
	data->flags = cfg->flags;
	data->callback = cfg->callback;
	data->callback_irq_mask = 0U;
	data->timeout_installed = true;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int dwdt_setup(const struct device *dev, uint8_t options)
{
	struct dwdt_data *data = dev->data;
	const uint8_t supported_options = WDT_OPT_PAUSE_IN_SLEEP |
		WDT_OPT_PAUSE_HALTED_BY_DBG;
	uint32_t mode = 0U;
	uint32_t irq_mask = 0U;
	uint32_t level_threshold;
	uint32_t window_level;
	uint32_t readback;
	k_spinlock_key_t key;

	if ((options & ~supported_options) != 0U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (!data->timeout_installed) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	if (data->running) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0U) {
		mode |= DWDT_PS_WDT_MR_WDIDLEHLT_1;
	}

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0U) {
		mode |= DWDT_PS_WDT_MR_WDDBGHLT_1;
	}

	if (data->flags == WDT_FLAG_RESET_SOC) {
		mode |= DWDT_PS_WDT_MR_PERIODRST_1;

	/*
	 * RPTH == 0 disables repeat-threshold violation handling.
	 * Otherwise, a restart before RPTH requests a reset.
	 */
		if (data->repeat_ticks != 0U) {
			mode |= DWDT_PS_WDT_MR_RPTHRST_1;
		}
	}

	/*
	 * LVLTH specifies the interrupt threshold relative to PERIOD. The
	 * down-counter starts at PERIOD and the level interrupt is generated
	 * when the counter reaches PERIOD - LVLTH. Therefore, a smaller LVLTH
	 * delays the interrupt, while a larger LVLTH makes it occur earlier.
	 *
	 * Keep the existing threshold selection unchanged. The hardware
	 * requires RPTH <= LVLTH < PERIOD.
	 */
	level_threshold = MAX((uint32_t)data->repeat_ticks, 1U);

	if (level_threshold >= data->period_ticks) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	if (data->callback != NULL) {
		if (data->flags == WDT_FLAG_RESET_SOC) {
			irq_mask = DWDT_PS_WDT_IER_LVLINT_Msk;
		} else {
			irq_mask = DWDT_PS_WDT_IER_PERINT_Msk;
		}
	}

	window_level =
		DWDT_PS_WDT_WL_RPTH(data->repeat_ticks) |
		DWDT_PS_WDT_WL_PERIOD(data->period_ticks);

	/*
	 * Disable all PSWDT interrupt sources and clear pending status before
	 * changing the configuration. Reading ISR clears the latched status.
	 */
	dwdt_write(dev, PS_WDT_IDR_OFFSET, DWDT_PS_WDT_IDR_Msk);
		(void)dwdt_read(dev, PS_WDT_ISR_OFFSET);

	dwdt_write(dev, PS_WDT_WL_OFFSET, window_level);
	dwdt_write(dev, PS_WDT_IL_OFFSET,
		   DWDT_PS_WDT_IL_LVLTH(level_threshold));

	/*
	 * Writing MR reloads the watchdog. WDDIS is left clear to enable the
	 * watchdog, including after a previous wdt_disable(), unless LOCKMR
	 * has been issued by firmware.
	 */
	dwdt_write(dev, PS_WDT_MR_OFFSET, mode);

	/*
	 * Detect a configuration that did not take effect, for example because
	 * firmware previously locked the configuration registers.
	 */
	readback = dwdt_read(dev, PS_WDT_WL_OFFSET);
	if ((readback & DWDT_PS_WDT_WL_Msk) != window_level) {
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	readback = dwdt_read(dev, PS_WDT_IL_OFFSET);
	if ((readback & DWDT_PS_WDT_IL_Msk) !=
		DWDT_PS_WDT_IL_LVLTH(level_threshold)) {
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	readback = dwdt_read(dev, PS_WDT_MR_OFFSET);
	if ((readback & DWDT_PS_WDT_MR_Msk) != mode) {
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	/*
	 * Set the software mask before enabling the peripheral interrupt.
	 * The spinlock prevents the ISR from observing a partially configured
	 * driver state on this CPU.
	 */
	data->callback_irq_mask = irq_mask;

	if (irq_mask != 0U) {
		dwdt_write(dev, PS_WDT_IER_OFFSET, irq_mask);

		readback = dwdt_read(dev, PS_WDT_IMR_OFFSET);
		if ((readback & irq_mask) != irq_mask) {
			dwdt_write(dev, PS_WDT_IDR_OFFSET,
				   DWDT_PS_WDT_IDR_Msk);
			data->callback_irq_mask = 0U;
			k_spin_unlock(&data->lock, key);
			return -EIO;
		}
	}

	/*
	 * Restart after programming. MR is not written after this CR access,
	 * as required by the datasheet timing restriction.
	 */
	dwdt_write(dev, PS_WDT_CR_OFFSET,
		DWDT_PS_WDT_CR_KEY_PASSWD |
		DWDT_PS_WDT_CR_WDRSTT_1);

	data->running = true;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int dwdt_feed(const struct device *dev, int channel_id)
{
	struct dwdt_data *data = dev->data;
	k_spinlock_key_t key;

	if (channel_id != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (!data->running) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	dwdt_write(dev, PS_WDT_CR_OFFSET,
		   DWDT_PS_WDT_CR_KEY_PASSWD |
		   DWDT_PS_WDT_CR_WDRSTT_1);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int dwdt_disable(const struct device *dev)
{
	struct dwdt_data *data = dev->data;
	uint32_t mode;
	uint32_t readback;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	mode = dwdt_read(dev, PS_WDT_MR_OFFSET);
	mode &= DWDT_PS_WDT_MR_Msk;
	mode |= DWDT_PS_WDT_MR_WDDIS_1;
	dwdt_write(dev, PS_WDT_MR_OFFSET, mode);

	readback = dwdt_read(dev, PS_WDT_MR_OFFSET);
	if ((readback & DWDT_PS_WDT_MR_WDDIS_Msk) == 0U) {
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	dwdt_write(dev, PS_WDT_IDR_OFFSET, DWDT_PS_WDT_IDR_Msk);
		(void)dwdt_read(dev, PS_WDT_ISR_OFFSET);

	data->callback_irq_mask = 0U;
	data->callback = NULL;
	data->timeout_installed = false;
	data->running = false;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static volatile unsigned int isr_count;

static void dwdt_isr(const struct device *dev)
{
	struct dwdt_data *data = dev->data;
	wdt_callback_t callback;
	uint32_t callback_irq_mask;
	uint32_t status;
	k_spinlock_key_t key;

	isr_count++;
	/* Reading ISR returns and clears the latched interrupt status. */
	status = dwdt_read(dev, PS_WDT_ISR_OFFSET);

	key = k_spin_lock(&data->lock);
	callback = data->callback;
	callback_irq_mask = data->callback_irq_mask;
	/*
	 * LVLINT can remain asserted while the counter is beyond LVLTH.
	 * For RESET_SOC the watchdog will reset at PERIOD, so deliver the
	 * callback only once.
	 */
	if (callback != NULL &&
		data->flags == WDT_FLAG_RESET_SOC &&
		(status & callback_irq_mask) != 0U) {
		dwdt_write(dev, PS_WDT_IDR_OFFSET, callback_irq_mask);
		data->callback_irq_mask = 0U;
	}
	k_spin_unlock(&data->lock, key);

	if (callback != NULL && (status & callback_irq_mask) != 0U) {
		callback(dev, 0);
	}
}

static int dwdt_init(const struct device *dev)
{
	const struct dwdt_config *cfg = dev->config;

	cfg->irq_config_func(dev);

	return 0;
}

static DEVICE_API(wdt, dwdt_api) = {
	.setup = dwdt_setup,
	.disable = dwdt_disable,
	.install_timeout = dwdt_install_timeout,
	.feed = dwdt_feed,
};

#define DWDT_IRQ_CONFIG(inst)						\
	static void dwdt_irq_config_##inst(const struct device *dev)	\
	{								\
		ARG_UNUSED(dev);					\
		IRQ_CONNECT(DT_INST_IRQN(inst),				\
		DT_INST_IRQ(inst, priority),				\
		dwdt_isr, DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));				\
	}

#define DWDT_DEFINE(inst)						\
	DWDT_IRQ_CONFIG(inst);						\
	static struct dwdt_data dwdt_data_##inst;			\
	static const struct dwdt_config dwdt_config_##inst = {		\
		.base = DT_INST_REG_ADDR(inst),				\
		.irq_config_func = dwdt_irq_config_##inst,		\
	};								\
	DEVICE_DT_INST_DEFINE(inst, dwdt_init, NULL,			\
			&dwdt_data_##inst, &dwdt_config_##inst,		\
			POST_KERNEL,					\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			&dwdt_api);

DT_INST_FOREACH_STATUS_OKAY(DWDT_DEFINE)

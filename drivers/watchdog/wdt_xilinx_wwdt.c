/*
 * Copyright (c) 2025-2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_versal_wwdt

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(xilinx_wwdt, CONFIG_WDT_LOG_LEVEL);

/* interrupt-names entry used by the active mode: "wdt" (window), "gwdt" (generic). */
#if defined(CONFIG_XILINX_VERSAL_WDT_WINDOW_MODE)
#define XWWDT_IRQ_NAME wdt
#elif defined(CONFIG_XILINX_VERSAL_WDT_GENERIC_MODE)
#define XWWDT_IRQ_NAME gwdt
#endif

/* Register offsets for the WWDT device */
#define XWWDT_MWR_OFFSET	0x00
#define XWWDT_ESR_OFFSET	0x04
#define XWWDT_FCR_OFFSET	0x08
#define XWWDT_FWR_OFFSET	0x0c
#define XWWDT_SWR_OFFSET	0x10

/* Master Write Control Register Masks */
#define XWWDT_MWR_MASK	BIT(0)

/* Enable and Status Register Masks */
#define XWWDT_ESR_WINT_MASK	BIT(16)
#define XWWDT_ESR_WSW_MASK	BIT(8)
#define XWWDT_ESR_WEN_MASK	BIT(0)

/* Function Control Register Masks (second window interrupt assertion point) */
#define XWWDT_FCR_SBC_MASK GENMASK(15, 8)
#define XWWDT_FCR_BSS_MASK GENMASK(7, 6)

/* Byte Segment Selection: compare SBC against SW[31:24] (top byte) */
#define XWWDT_FCR_BSS_BYTE3 3U

/* Watchdog Second Window Shift */
#define XWWDT_ESR_WSW_SHIFT	8U

/* Maximum count value of each 32 bit window */
#define XWWDT_MAX_COUNT_WINDOW	GENMASK(31, 0)

/* Maximum count value of closed and open window combined */
#define XWWDT_MAX_COUNT_WINDOW_COMBINED	GENMASK64(32, 1)

/* Generic Watchdog (GWDT) register offsets (absolute from MMIO base) */
#define XGWDT_GWRR_OFFSET  0x1000 /* Refresh register */
#define XGWDT_GWCSR_OFFSET 0x2000 /* Control and status register */
#define XGWDT_GWOR_OFFSET  0x2008 /* Offset (first-window period) register */
#define XGWDT_GW_WR_OFFSET 0x2fd0 /* Warm reset register */

/* Generic Watchdog Control and Status Register masks */
#define XGWDT_GWCSR_GWEN_MASK BIT(0) /* Generic watchdog enable */
#define XGWDT_GWCSR_GWS1_MASK BIT(1) /* Stage-1 (interrupt) status */

#define XGWDT_GWRR_MASK  BIT(0) /* Generic watchdog refresh */
#define XGWDT_GW_WR_MASK BIT(0) /* Generic watchdog warm reset enable */

struct xilinx_wwdt_config {
	DEVICE_MMIO_ROM;
	uint32_t wdt_clock_freq;
	void (*irq_config)(void);
	unsigned int irq;
};

struct xilinx_wwdt_data {
	DEVICE_MMIO_RAM;
	struct k_spinlock lock;
	bool timeout_active;
	bool wdt_started;
	wdt_callback_t callback;
};

static int wdt_xilinx_wwdt_setup(const struct device *dev, uint8_t options)
{
	const struct xilinx_wwdt_config *config = dev->config;
	struct xilinx_wwdt_data *data = dev->data;
	mm_reg_t reg = DEVICE_MMIO_GET(dev);
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (!data->timeout_active) {
		ret = -EINVAL;
		goto out;
	}

	if (data->wdt_started) {
		ret = -EBUSY;
		goto out;
	}

	/*
	 * There is no control at driver level whether the WDT pauses in CPU sleep
	 * or when halted by debugger. Hence there is no check for the options.
	 */

	if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_WINDOW_MODE)) {
		uint32_t reg_value;

		/* Read enable status register and update WEN bit */
		reg_value = sys_read32(reg + XWWDT_ESR_OFFSET) | XWWDT_ESR_WEN_MASK;

		/* Write enable status register with updated WEN value */
		sys_write32(reg_value, reg + XWWDT_ESR_OFFSET);
	} else if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_GENERIC_MODE)) {
		/* Set the GWEN bit to enable the generic watchdog. */
		sys_write32(XGWDT_GWCSR_GWEN_MASK, reg + XGWDT_GWCSR_OFFSET);
	}
	data->wdt_started = true;

	/*
	 * The interrupt source is enabled by default in the IP (Interrupt_Mask
	 * reset state), so delivery is gated only at the interrupt controller:
	 * unmask it only when a warning callback is installed.
	 */
	if (data->callback != NULL) {
		irq_enable(config->irq);
	}
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_wwdt_install_timeout(const struct device *dev,
					   const struct wdt_timeout_cfg *cfg)
{
	const struct xilinx_wwdt_config *config = dev->config;
	struct xilinx_wwdt_data *data = dev->data;
	mm_reg_t reg = DEVICE_MMIO_GET(dev);
	uint64_t max_hw_timeout_ms;
	uint64_t timeout_ms_count;
	uint32_t timeout_ms;
	uint64_t ms_count;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->wdt_started) {
		ret = -EBUSY;
		goto out;
	}

	/* Reset action is owned by platform firmware (PLM/CDO); cfg->flags is only a hint. */
	if (cfg->flags != WDT_FLAG_RESET_NONE) {
		LOG_WRN("WDT_FLAG_RESET_* not honored; "
			"reset action is owned by firmware (PLM/CDO)");
	}

	/* A callback requires the mode's interrupt to be wired in DT. */
	if (cfg->callback != NULL && config->irq_config == NULL) {
		ret = -ENOTSUP;
		goto out;
	}

	/* Ticks for the requested timeout, shared by both modes. */
	ms_count = config->wdt_clock_freq / 1000;
	timeout_ms = cfg->window.max;

	/* A zero (open) window is invalid for both engines. */
	if (timeout_ms == 0) {
		ret = -EINVAL;
		goto out;
	}

	timeout_ms_count = timeout_ms * ms_count;

	if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_WINDOW_MODE)) {
		uint64_t closed_window_ms_count;
		uint64_t open_window_ms_count;

		max_hw_timeout_ms =
			(XWWDT_MAX_COUNT_WINDOW_COMBINED * 1000) / config->wdt_clock_freq;

		/* Timeout greater than the maximum hardware timeout is invalid. */
		if (timeout_ms > max_hw_timeout_ms) {
			ret = -EINVAL;
			goto out;
		}

		closed_window_ms_count = cfg->window.min * ms_count;
		if (closed_window_ms_count > XWWDT_MAX_COUNT_WINDOW) {
			LOG_ERR("The closed window timeout is invalid.");
			ret = -EINVAL;
			goto out;
		}

		open_window_ms_count = timeout_ms_count - closed_window_ms_count;
		if (open_window_ms_count > XWWDT_MAX_COUNT_WINDOW) {
			LOG_ERR("The open window timeout is invalid.");
			ret = -EINVAL;
			goto out;
		}

		sys_write32(XWWDT_MWR_MASK, reg + XWWDT_MWR_OFFSET);
		sys_write32(~(uint32_t)XWWDT_ESR_WEN_MASK, reg + XWWDT_ESR_OFFSET);
		sys_write32(closed_window_ms_count, reg + XWWDT_FWR_OFFSET);
		sys_write32(open_window_ms_count, reg + XWWDT_SWR_OFFSET);

		if (cfg->callback != NULL) {
			/*
			 * Assert WINT at the start of the open (second) window: with
			 * BSS=3 the SBC byte is compared against SW[31:24], which equals
			 * the top byte of the open window count at the instant the
			 * second window is entered. This fires exactly once per window
			 * because the down-counting SW never returns to that byte value.
			 */
			sys_write32(XWWDT_MWR_MASK, reg + XWWDT_MWR_OFFSET);
			sys_write32(FIELD_PREP(XWWDT_FCR_BSS_MASK, XWWDT_FCR_BSS_BYTE3) |
					    FIELD_PREP(XWWDT_FCR_SBC_MASK,
						       (open_window_ms_count >> 24) & 0xFF),
				    reg + XWWDT_FCR_OFFSET);
		}
	} else if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_GENERIC_MODE)) {
		/* The generic watchdog has a single window; a closed window is invalid. */
		if (cfg->window.min != 0) {
			ret = -EINVAL;
			goto out;
		}

		max_hw_timeout_ms =
			((uint64_t)XWWDT_MAX_COUNT_WINDOW * 1000) / config->wdt_clock_freq;
		if (timeout_ms > max_hw_timeout_ms) {
			ret = -EINVAL;
			goto out;
		}

		/* Program the first-window period (registers warm-reset at init). */
		sys_write32((uint32_t)timeout_ms_count, reg + XGWDT_GWOR_OFFSET);
	}

	data->callback = cfg->callback;
	data->timeout_active = true;
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_wwdt_feed(const struct device *dev, int channel_id)
{
	struct xilinx_wwdt_data *data = dev->data;
	mm_reg_t reg = DEVICE_MMIO_GET(dev);
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (channel_id != 0 || !data->timeout_active || !data->wdt_started) {
		ret = -EINVAL;
		goto out;
	}

	if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_WINDOW_MODE)) {
		uint32_t control_status_reg;
		uint32_t is_sec_window;

		/* Enable write access control bit for the WWDT. */
		sys_write32(XWWDT_MWR_MASK, reg + XWWDT_MWR_OFFSET);

		/* Trigger restart kick to WWDT. */
		control_status_reg = sys_read32(reg + XWWDT_ESR_OFFSET);

		/* Check if WWDT is in Second window. */
		is_sec_window =
			(control_status_reg & (uint32_t)XWWDT_ESR_WSW_MASK) >> XWWDT_ESR_WSW_SHIFT;

		if (is_sec_window != 1) {
			LOG_ERR("Feed in Closed window is not supported.");
			ret = -ENOTSUP;
			goto out;
		}

		control_status_reg |= (uint32_t)XWWDT_ESR_WSW_MASK;
		sys_write32(control_status_reg, reg + XWWDT_ESR_OFFSET);
	} else if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_GENERIC_MODE)) {
		const struct xilinx_wwdt_config *config = dev->config;

		/* Refresh the generic watchdog to restart the first window. */
		sys_write32(XGWDT_GWRR_MASK, reg + XGWDT_GWRR_OFFSET);

		/*
		 * The refresh clears GWS1, so re-arm the warning interrupt that the
		 * ISR masked to acknowledge the previous stage-1 event. The source
		 * is now deasserted, so unmasking the (level) line is race-free.
		 * This is a no-op on a normal feed where the interrupt is already
		 * enabled.
		 */
		if (data->callback != NULL) {
			irq_enable(config->irq);
		}
	}
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_wwdt_disable(const struct device *dev)
{
	const struct xilinx_wwdt_config *config = dev->config;
	struct xilinx_wwdt_data *data = dev->data;
	mm_reg_t reg = DEVICE_MMIO_GET(dev);
	uint32_t reg_value;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_WINDOW_MODE)) {
		uint32_t is_wwdt_enable;
		uint32_t is_sec_window;

		is_wwdt_enable = sys_read32(reg + XWWDT_ESR_OFFSET) & XWWDT_ESR_WEN_MASK;

		if (is_wwdt_enable == 0) {
			ret = -EFAULT;
			goto out;
		}

		/* Read enable status register and check if WWDT is in open window. */
		is_sec_window = (sys_read32(reg + XWWDT_ESR_OFFSET) & XWWDT_ESR_WSW_MASK) >>
				XWWDT_ESR_WSW_SHIFT;

		if (is_sec_window != 1) {
			LOG_ERR("Disabling WWDT in closed window is not allowed.");
			ret = -EPERM;
			goto out;
		}

		/* Read enable status register and update WEN bit. */
		reg_value = sys_read32(reg + XWWDT_ESR_OFFSET) & (~XWWDT_ESR_WEN_MASK);

		/* Write enable status register with updated WEN and WSW value. */
		sys_write32(reg_value, reg + XWWDT_ESR_OFFSET);
	} else if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_GENERIC_MODE)) {
		reg_value = sys_read32(reg + XGWDT_GWCSR_OFFSET);

		if ((reg_value & XGWDT_GWCSR_GWEN_MASK) == 0) {
			ret = -EFAULT;
			goto out;
		}

		/* Clear the GWEN bit. */
		reg_value &= ~XGWDT_GWCSR_GWEN_MASK;

		/* Write control status register to disable the generic watchdog. */
		sys_write32(reg_value, reg + XGWDT_GWCSR_OFFSET);
	}

	data->wdt_started = false;

	if (data->callback != NULL) {
		irq_disable(config->irq);
	}
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
__maybe_unused static void wdt_xilinx_wwdt_isr(const struct device *dev)
{
	struct xilinx_wwdt_data *data = dev->data;
	mm_reg_t reg = DEVICE_MMIO_GET(dev);
	wdt_callback_t callback;
	uint32_t reg_value;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_WINDOW_MODE)) {
		reg_value = sys_read32(reg + XWWDT_ESR_OFFSET);
		if ((reg_value & XWWDT_ESR_WINT_MASK) == 0U) {
			k_spin_unlock(&data->lock, key);
			return;
		}

		/*
		 * Clear the interrupt (WINT is write-1-to-clear) with WSW masked
		 * out of the written value so the clear does not issue a restart
		 * kick.
		 */
		reg_value |= XWWDT_ESR_WINT_MASK;
		reg_value &= ~XWWDT_ESR_WSW_MASK;
		sys_write32(XWWDT_MWR_MASK, reg + XWWDT_MWR_OFFSET);
		sys_write32(reg_value, reg + XWWDT_ESR_OFFSET);
	} else if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_GENERIC_MODE)) {
		const struct xilinx_wwdt_config *config = dev->config;

		reg_value = sys_read32(reg + XGWDT_GWCSR_OFFSET);
		if ((reg_value & XGWDT_GWCSR_GWS1_MASK) == 0U) {
			k_spin_unlock(&data->lock, key);
			return;
		}

		/*
		 * The stage-1 status (GWS1) is cleared only by a watchdog refresh,
		 * which would also restart the timer and prevent the stage-2 reset.
		 * To acknowledge the interrupt without restarting, mask it at the
		 * interrupt controller; the callback decides whether to feed (via
		 * wdt_feed(), which clears GWS1 and re-enables this interrupt) to
		 * continue, or let the stage-2 reset proceed.
		 */
		irq_disable(config->irq);
	}

	/*
	 * The lock is released before invoking the callback so a wdt_feed()
	 * from the callback can take it (the spinlock is not recursive).
	 */
	callback = data->callback;
	k_spin_unlock(&data->lock, key);

	if (callback != NULL) {
		callback(dev, 0);
	}
}
#endif

static int wdt_xilinx_wwdt_init(const struct device *dev)
{
	const struct xilinx_wwdt_config *config = dev->config;
	int ret = 0;

	if (config->wdt_clock_freq == 0) {
		return -EINVAL;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (IS_ENABLED(CONFIG_XILINX_VERSAL_WDT_GENERIC_MODE)) {
		/* Warm-reset the generic watchdog registers to a known state once. */
		sys_write32(XGWDT_GW_WR_MASK, DEVICE_MMIO_GET(dev) + XGWDT_GW_WR_OFFSET);
	}

	if (config->irq_config != NULL) {
		config->irq_config();
	}

	return ret;
}

static DEVICE_API(wdt, wdt_xilinx_wwdt_api) = {
	.setup = wdt_xilinx_wwdt_setup,
	.install_timeout = wdt_xilinx_wwdt_install_timeout,
	.feed = wdt_xilinx_wwdt_feed,
	.disable = wdt_xilinx_wwdt_disable,
};

#define WDT_XILINX_WWDT_IRQ_CONFIG_FUNC(inst)                                                      \
	static void wdt_xilinx_wwdt_irq_config_##inst(void)                                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, XWWDT_IRQ_NAME, irq),                        \
			    DT_INST_IRQ_BY_NAME(inst, XWWDT_IRQ_NAME, priority),                   \
			    wdt_xilinx_wwdt_isr, DEVICE_DT_INST_GET(inst), 0);                     \
	}

/*
 * The interrupts block is optional, but when present the active mode's
 * interrupt must be named so the driver can select it from the other
 * interrupt lines the node may expose ("wdt" for the window engine's WINT,
 * "gwdt" for the generic engine's stage-1 interrupt).
 */
#define WDT_XILINX_WWDT_CHECK_IRQ_NAME(inst)                                                       \
	BUILD_ASSERT(                                                                              \
		!DT_INST_IRQ_HAS_IDX(inst, 0) || DT_INST_IRQ_HAS_NAME(inst, XWWDT_IRQ_NAME),       \
		"versal-wwdt: interrupt-names must include \"" STRINGIFY(XWWDT_IRQ_NAME) "\"");

#define WDT_XILINX_WWDT_IRQ_CFG_GET(inst)                                                          \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, XWWDT_IRQ_NAME),					\
		    (wdt_xilinx_wwdt_irq_config_##inst), (NULL))

#define WDT_XILINX_WWDT_IRQ_GET(inst)                                                              \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, XWWDT_IRQ_NAME),					\
		    (DT_INST_IRQ_BY_NAME(inst, XWWDT_IRQ_NAME, irq)), (0))

#define WDT_XILINX_WWDT_IRQ_CONFIG_DEFINE(inst)                                                    \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, XWWDT_IRQ_NAME),					\
		    (WDT_XILINX_WWDT_IRQ_CONFIG_FUNC(inst)), ())

#define WDT_XILINX_WWDT_INIT(inst)                                                                 \
	WDT_XILINX_WWDT_CHECK_IRQ_NAME(inst)                                                       \
	WDT_XILINX_WWDT_IRQ_CONFIG_DEFINE(inst)                                                    \
                                                                                                   \
	static struct xilinx_wwdt_data wdt_xilinx_wwdt_##inst##_dev_data;                          \
                                                                                                   \
	static const struct xilinx_wwdt_config wdt_xilinx_wwdt_##inst##_cfg = {                    \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.wdt_clock_freq = DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency),          \
		.irq_config = WDT_XILINX_WWDT_IRQ_CFG_GET(inst),                                   \
		.irq = WDT_XILINX_WWDT_IRQ_GET(inst),                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &wdt_xilinx_wwdt_init, NULL,                                   \
			      &wdt_xilinx_wwdt_##inst##_dev_data, &wdt_xilinx_wwdt_##inst##_cfg,   \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                    \
			      &wdt_xilinx_wwdt_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_XILINX_WWDT_INIT)

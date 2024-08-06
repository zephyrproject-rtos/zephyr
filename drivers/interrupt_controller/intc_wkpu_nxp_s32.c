/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_wkpu

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/drivers/interrupt_controller/intc_wkpu_nxp_s32.h>

/* NMI Status Flag Register */
#define WKPU_NSR       0x0
/* NMI Configuration Register */
#define WKPU_NCR       0x8
/* Wakeup/Interrupt Status Flag Register */
#define WKPU_WISR(n)   (0x14 + 0x40 * (n))
/* Interrupt Request Enable Register */
#define WKPU_IRER(n)   (0x18 + 0x40 * (n))
/* Wakeup Request Enable Register */
#define WKPU_WRER(n)   (0x1c + 0x40 * (n))
/* Wakeup/Interrupt Rising-Edge Event Enable Register */
#define WKPU_WIREER(n) (0x28 + 0x40 * (n))
/* Wakeup/Interrupt Falling-Edge Event Enable Register */
#define WKPU_WIFEER(n) (0x2c + 0x40 * (n))
/* Wakeup/Interrupt Filter Enable Register */
#define WKPU_WIFER(n)  (0x30 + 0x40 * (n))

/* Handy accessors */
#define REG_READ(r)     sys_read32(config->base + (r))
#define REG_WRITE(r, v) sys_write32((v), config->base + (r))

struct wkpu_nxp_s32_config {
	mem_addr_t base;
	uint64_t filter_enable;
};

struct wkpu_nxp_s32_cb {
	wkpu_nxp_s32_callback_t cb;
	uint8_t pin;
	void *data;
};

struct wkpu_nxp_s32_data {
	struct wkpu_nxp_s32_cb *cb;
};

static void wkpu_nxp_s32_interrupt_handler(const struct device *dev)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	struct wkpu_nxp_s32_data *data = dev->data;
	uint64_t pending = wkpu_nxp_s32_get_pending(dev);
	uint64_t irq_mask;
	int irq;

	while (pending) {
		irq_mask = LSB_GET(pending);
		irq = u64_count_trailing_zeros(irq_mask);

		/* Clear status flag */
		REG_WRITE(WKPU_WISR(irq / 32U), REG_READ(WKPU_WISR(irq / 32U)) | irq_mask);

		if (data->cb[irq].cb != NULL) {
			data->cb[irq].cb(data->cb[irq].pin, data->cb[irq].data);
		}

		pending ^= irq_mask;
	}
}

int wkpu_nxp_s32_set_callback(const struct device *dev, uint8_t irq, uint8_t pin,
			      wkpu_nxp_s32_callback_t cb, void *arg)
{
	struct wkpu_nxp_s32_data *data = dev->data;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_S32_WKPU_SOURCES_MAX);

	if ((data->cb[irq].cb == cb) && (data->cb[irq].data == arg)) {
		return 0;
	}

	if (data->cb[irq].cb) {
		return -EBUSY;
	}

	data->cb[irq].cb = cb;
	data->cb[irq].pin = pin;
	data->cb[irq].data = arg;

	return 0;
}

void wkpu_nxp_s32_unset_callback(const struct device *dev, uint8_t irq)
{
	struct wkpu_nxp_s32_data *data = dev->data;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_S32_WKPU_SOURCES_MAX);

	data->cb[irq].cb = NULL;
	data->cb[irq].pin = 0;
	data->cb[irq].data = NULL;
}

void wkpu_nxp_s32_enable_interrupt(const struct device *dev, uint8_t irq,
				   enum wkpu_nxp_s32_trigger trigger)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	uint32_t mask = BIT(irq % 32U);
	uint8_t reg_idx = irq / 32U;
	uint32_t reg_val;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_S32_WKPU_SOURCES_MAX);

	/* Configure trigger */
	reg_val = REG_READ(WKPU_WIREER(reg_idx));
	if ((trigger == WKPU_NXP_S32_RISING_EDGE) || (trigger == WKPU_NXP_S32_BOTH_EDGES)) {
		reg_val |= mask;
	} else {
		reg_val &= ~mask;
	}
	REG_WRITE(WKPU_WIREER(reg_idx), reg_val);

	reg_val = REG_READ(WKPU_WIFEER(reg_idx));
	if ((trigger == WKPU_NXP_S32_FALLING_EDGE) || (trigger == WKPU_NXP_S32_BOTH_EDGES)) {
		reg_val |= mask;
	} else {
		reg_val &= ~mask;
	}
	REG_WRITE(WKPU_WIFEER(reg_idx), reg_val);

	/* Clear status flag and unmask interrupt */
	REG_WRITE(WKPU_WISR(reg_idx), REG_READ(WKPU_WISR(reg_idx)) | mask);
	REG_WRITE(WKPU_IRER(reg_idx), REG_READ(WKPU_IRER(reg_idx)) | mask);
}

void wkpu_nxp_s32_disable_interrupt(const struct device *dev, uint8_t irq)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	uint32_t mask = BIT(irq % 32U);
	uint8_t reg_idx = irq / 32U;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_S32_WKPU_SOURCES_MAX);

	/* Disable triggers */
	REG_WRITE(WKPU_WIREER(reg_idx), REG_READ(WKPU_WIREER(reg_idx)) & ~mask);
	REG_WRITE(WKPU_WIFEER(reg_idx), REG_READ(WKPU_WIFEER(reg_idx)) & ~mask);

	/* Clear status flag and mask interrupt */
	REG_WRITE(WKPU_WISR(reg_idx), REG_READ(WKPU_WISR(reg_idx)) | mask);
	REG_WRITE(WKPU_IRER(reg_idx), REG_READ(WKPU_IRER(reg_idx)) & ~mask);
}

uint64_t wkpu_nxp_s32_get_pending(const struct device *dev)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	uint64_t flags;

	flags = REG_READ(WKPU_WISR(0U)) & REG_READ(WKPU_IRER(0U));
	if (CONFIG_NXP_S32_WKPU_SOURCES_MAX > 32U) {
		flags |= ((uint64_t)(REG_READ(WKPU_WISR(1U)) & REG_READ(WKPU_IRER(1U)))) << 32U;
	}

	return flags;
}

static int wkpu_nxp_s32_init(const struct device *dev)
{
	const struct wkpu_nxp_s32_config *config = dev->config;

	/* Disable triggers, clear status flags and mask all interrupts */
	REG_WRITE(WKPU_WIREER(0U), 0U);
	REG_WRITE(WKPU_WIFEER(0U), 0U);
	REG_WRITE(WKPU_WISR(0U), 0xffffffff);
	REG_WRITE(WKPU_IRER(0U), 0U);

	/* Configure glitch filters */
	REG_WRITE(WKPU_WIFER(0U), (uint32_t)config->filter_enable);

	if (CONFIG_NXP_S32_WKPU_SOURCES_MAX > 32U) {
		REG_WRITE(WKPU_WIREER(1U), 0U);
		REG_WRITE(WKPU_WIFEER(1U), 0U);
		REG_WRITE(WKPU_WISR(1U), 0xffffffff);
		REG_WRITE(WKPU_IRER(1U), 0U);
		REG_WRITE(WKPU_WIFER(1U), (uint32_t)(config->filter_enable >> 32U));
	}

	return 0;
}

#define WKPU_NXP_S32_FILTER_CONFIG(idx, n)                                                         \
	COND_CODE_1(DT_PROP(DT_INST_CHILD(n, irq_##idx), filter_enable), (BIT(idx)), (0U))

#define WKPU_NXP_S32_INIT_DEVICE(n)                                                                \
	static const struct wkpu_nxp_s32_config wkpu_nxp_s32_conf_##n = {                          \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.filter_enable = LISTIFY(CONFIG_NXP_S32_WKPU_SOURCES_MAX,                          \
					 WKPU_NXP_S32_FILTER_CONFIG, (|), n),                      \
	};                                                                                         \
	static struct wkpu_nxp_s32_cb wkpu_nxp_s32_cb_##n[CONFIG_NXP_S32_WKPU_SOURCES_MAX];        \
	static struct wkpu_nxp_s32_data wkpu_nxp_s32_data_##n = {                                  \
		.cb = wkpu_nxp_s32_cb_##n,                                                         \
	};                                                                                         \
	static int wkpu_nxp_s32_init_##n(const struct device *dev)                                 \
	{                                                                                          \
		int err;                                                                           \
                                                                                                   \
		err = wkpu_nxp_s32_init(dev);                                                      \
		if (err) {                                                                         \
			return err;                                                                \
		}                                                                                  \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ(n, irq), DT_INST_IRQ(n, priority),                         \
			    wkpu_nxp_s32_interrupt_handler, DEVICE_DT_INST_GET(n),                 \
			    COND_CODE_1(CONFIG_GIC, (DT_INST_IRQ(n, flags)), (0U)));               \
		irq_enable(DT_INST_IRQ(n, irq));                                                   \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, wkpu_nxp_s32_init_##n, NULL, &wkpu_nxp_s32_data_##n,              \
			      &wkpu_nxp_s32_conf_##n, PRE_KERNEL_2, CONFIG_INTC_INIT_PRIORITY,     \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(WKPU_NXP_S32_INIT_DEVICE)

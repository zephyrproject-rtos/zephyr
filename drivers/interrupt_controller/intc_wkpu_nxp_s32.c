/*
 * Copyright 2023-2024, 2026 NXP
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
#include <zephyr/logging/log.h>

#include "nxp_s32_wkpu_shared.h"

LOG_MODULE_REGISTER(wkpu_nxp_s32, CONFIG_INTC_LOG_LEVEL);

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
	struct k_spinlock lock;
#ifdef CONFIG_WUC_NXP_S32_WKPU
	uint64_t triggered;
	uint64_t cleared;
#endif
};

/* The caller holds the controller lock. Never read-modify-write WISR. */
static void wkpu_nxp_s32_acknowledge(const struct device *dev, uint8_t bank, uint32_t mask)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
#ifdef CONFIG_WUC_NXP_S32_WKPU
	struct wkpu_nxp_s32_data *data = dev->data;
	uint64_t status = (uint64_t)(REG_READ(WKPU_WISR(bank)) & mask) << (bank * 32U);

	data->triggered |= status & ~data->cleared;
	data->cleared &= ~((uint64_t)mask << (bank * 32U));
#endif

	REG_WRITE(WKPU_WISR(bank), mask);
}

static void wkpu_nxp_s32_interrupt_handler(const struct device *dev)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	struct wkpu_nxp_s32_data *data = dev->data;
	uint64_t pending = wkpu_nxp_s32_get_pending(dev);
	uint64_t irq_mask;
	int irq;

	while (pending != 0U) {
		struct wkpu_nxp_s32_cb cb = {0};
		k_spinlock_key_t key;
		uint32_t mask;
		uint8_t bank;

		irq_mask = LSB_GET(pending);
		irq = u64_count_trailing_zeros(irq_mask);
		bank = irq / 32U;
		mask = BIT(irq % 32U);

		key = k_spin_lock(&data->lock);
		if ((REG_READ(WKPU_WISR(bank)) & REG_READ(WKPU_IRER(bank)) & mask) != 0U) {
			wkpu_nxp_s32_acknowledge(dev, bank, mask);
			cb = data->cb[irq];
		}
		k_spin_unlock(&data->lock, key);

		if (cb.cb != NULL) {
			cb.cb(cb.pin, cb.data);
		}

		pending ^= irq_mask;
	}
}

static int wkpu_nxp_s32_set_callback_locked(struct wkpu_nxp_s32_data *data, uint8_t irq,
					   uint8_t pin, wkpu_nxp_s32_callback_t cb, void *arg)
{
	int ret = 0;

	if ((data->cb[irq].cb != cb) || (data->cb[irq].data != arg)) {
		if (data->cb[irq].cb != NULL) {
			ret = -EBUSY;
		} else {
			data->cb[irq] = (struct wkpu_nxp_s32_cb){
				.cb = cb,
				.pin = pin,
				.data = arg,
			};
		}
	}
	return ret;
}

int wkpu_nxp_s32_set_callback(const struct device *dev, uint8_t irq, uint8_t pin,
			      wkpu_nxp_s32_callback_t cb, void *arg)
{
	struct wkpu_nxp_s32_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_S32_WKPU_SOURCES_MAX);
	key = k_spin_lock(&data->lock);
	ret = wkpu_nxp_s32_set_callback_locked(data, irq, pin, cb, arg);
	k_spin_unlock(&data->lock, key);

	return ret;
}

void wkpu_nxp_s32_unset_callback(const struct device *dev, uint8_t irq)
{
	struct wkpu_nxp_s32_data *data = dev->data;
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_S32_WKPU_SOURCES_MAX);

	key = k_spin_lock(&data->lock);
	data->cb[irq] = (struct wkpu_nxp_s32_cb){0};
	k_spin_unlock(&data->lock, key);
}

static uint32_t wkpu_nxp_s32_get_edges(const struct device *dev, uint8_t irq)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	uint32_t mask = BIT(irq % 32U);
	uint8_t bank = irq / 32U;

	return ((REG_READ(WKPU_WIREER(bank)) & mask) ? BIT(0) : 0U) |
	       ((REG_READ(WKPU_WIFEER(bank)) & mask) ? BIT(1) : 0U);
}

static void wkpu_nxp_s32_set_edges(const struct device *dev, uint8_t irq, uint32_t edges)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	uint32_t mask = BIT(irq % 32U);
	uint8_t reg_idx = irq / 32U;
	uint32_t reg_val;

	reg_val = REG_READ(WKPU_WIREER(reg_idx));
	if (edges & BIT(0)) {
		reg_val |= mask;
	} else {
		reg_val &= ~mask;
	}
	REG_WRITE(WKPU_WIREER(reg_idx), reg_val);

	reg_val = REG_READ(WKPU_WIFEER(reg_idx));
	if (edges & BIT(1)) {
		reg_val |= mask;
	} else {
		reg_val &= ~mask;
	}
	REG_WRITE(WKPU_WIFEER(reg_idx), reg_val);
}

static int wkpu_nxp_s32_validate_trigger(uint32_t irq, enum wkpu_nxp_s32_trigger trigger)
{
	if (irq >= CONFIG_NXP_S32_WKPU_SOURCES_MAX ||
	    trigger < WKPU_NXP_S32_RISING_EDGE || trigger > WKPU_NXP_S32_BOTH_EDGES ||
	    (irq < 4U && trigger != WKPU_NXP_S32_RISING_EDGE)) {
		return -EINVAL;
	}

	return 0;
}

static int wkpu_nxp_s32_enable_interrupt_locked(const struct device *dev, uint8_t irq,
					       enum wkpu_nxp_s32_trigger trigger)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	uint32_t mask = BIT(irq % 32U);
	uint8_t reg_idx = irq / 32U;
	uint32_t edges = trigger + 1U;
	bool clear_status;
#ifdef CONFIG_WUC_NXP_S32_WKPU
	struct wkpu_nxp_s32_data *data = dev->data;
#endif

	if ((REG_READ(WKPU_WRER(reg_idx)) & mask) != 0U &&
	    wkpu_nxp_s32_get_edges(dev, irq) != edges) {
		return -EBUSY;
	}
	wkpu_nxp_s32_set_edges(dev, irq, edges);
	/* An armed wake request must survive interrupt reconfiguration. */
	clear_status = (REG_READ(WKPU_WRER(reg_idx)) & mask) == 0U;
#ifdef CONFIG_WUC_NXP_S32_WKPU
	clear_status |= (data->cleared & BIT64(irq)) != 0U;
#endif
	if (clear_status) {
		wkpu_nxp_s32_acknowledge(dev, reg_idx, mask);
	}
	REG_WRITE(WKPU_IRER(reg_idx), REG_READ(WKPU_IRER(reg_idx)) | mask);

	return 0;
}

void wkpu_nxp_s32_enable_interrupt(const struct device *dev, uint8_t irq,
				   enum wkpu_nxp_s32_trigger trigger)
{
	struct wkpu_nxp_s32_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = wkpu_nxp_s32_validate_trigger(irq, trigger);

	if (ret == 0) {
		key = k_spin_lock(&data->lock);
		ret = wkpu_nxp_s32_enable_interrupt_locked(dev, irq, trigger);
		k_spin_unlock(&data->lock, key);
	}
	if (ret != 0) {
		LOG_ERR("Cannot enable WKPU interrupt %u: %d", irq, ret);
	}
}

int wkpu_nxp_s32_configure_interrupt(const struct device *dev, uint8_t irq, uint8_t pin,
				     enum wkpu_nxp_s32_trigger trigger,
				     wkpu_nxp_s32_callback_t cb, void *arg)
{
	struct wkpu_nxp_s32_data *data = dev->data;
	struct wkpu_nxp_s32_cb previous;
	k_spinlock_key_t key;
	int ret = wkpu_nxp_s32_validate_trigger(irq, trigger);

	if (ret != 0) {
		return ret;
	}
	key = k_spin_lock(&data->lock);
	previous = data->cb[irq];
	ret = wkpu_nxp_s32_set_callback_locked(data, irq, pin, cb, arg);
	if (ret == 0) {
		ret = wkpu_nxp_s32_enable_interrupt_locked(dev, irq, trigger);
		if (ret != 0) {
			data->cb[irq] = previous;
		}
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

void wkpu_nxp_s32_disable_interrupt(const struct device *dev, uint8_t irq)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	struct wkpu_nxp_s32_data *data = dev->data;
	uint32_t mask = BIT(irq % 32U);
	uint8_t reg_idx = irq / 32U;
	k_spinlock_key_t key;
	bool clear_status;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_S32_WKPU_SOURCES_MAX);

	key = k_spin_lock(&data->lock);
	REG_WRITE(WKPU_IRER(reg_idx), REG_READ(WKPU_IRER(reg_idx)) & ~mask);
	clear_status = (REG_READ(WKPU_WRER(reg_idx)) & mask) == 0U;
	if (clear_status) {
		REG_WRITE(WKPU_WIREER(reg_idx), REG_READ(WKPU_WIREER(reg_idx)) & ~mask);
		REG_WRITE(WKPU_WIFEER(reg_idx), REG_READ(WKPU_WIFEER(reg_idx)) & ~mask);
	}

#ifdef CONFIG_WUC_NXP_S32_WKPU
	clear_status |= (data->cleared & BIT64(irq)) != 0U;
#endif
	if (clear_status) {
		wkpu_nxp_s32_acknowledge(dev, reg_idx, mask);
	}
	k_spin_unlock(&data->lock, key);
}

uint64_t wkpu_nxp_s32_get_pending(const struct device *dev)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	struct wkpu_nxp_s32_data *data = dev->data;
	k_spinlock_key_t key;
	uint64_t flags;

	key = k_spin_lock(&data->lock);
	flags = REG_READ(WKPU_WISR(0U)) & REG_READ(WKPU_IRER(0U));
	if (CONFIG_NXP_S32_WKPU_SOURCES_MAX > 32U) {
		flags |= ((uint64_t)(REG_READ(WKPU_WISR(1U)) & REG_READ(WKPU_IRER(1U)))) << 32U;
	}
	k_spin_unlock(&data->lock, key);

	return flags;
}

static int wkpu_nxp_s32_init(const struct device *dev)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	struct wkpu_nxp_s32_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	for (uint8_t bank = 0U; bank < DIV_ROUND_UP(CONFIG_NXP_S32_WKPU_SOURCES_MAX, 32U);
	     bank++) {
		uint32_t status = REG_READ(WKPU_WISR(bank));
		uint32_t wake = REG_READ(WKPU_WRER(bank));
		uint32_t filter = (uint32_t)(config->filter_enable >> (bank * 32U));

		REG_WRITE(WKPU_IRER(bank), 0U);
		REG_WRITE(WKPU_WIREER(bank), REG_READ(WKPU_WIREER(bank)) & wake);
		REG_WRITE(WKPU_WIFEER(bank), REG_READ(WKPU_WIFEER(bank)) & wake);
		REG_WRITE(WKPU_WIFER(bank),
			  (REG_READ(WKPU_WIFER(bank)) & wake) | (filter & ~wake));
		wkpu_nxp_s32_acknowledge(dev, bank, status);
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

#ifdef CONFIG_WUC_NXP_S32_WKPU
void wkpu_nxp_s32_capture_events(const struct device *dev, uint64_t status)
{
	struct wkpu_nxp_s32_data *data = dev->data;

	/* Early boot only, after BSS initialization and before concurrent users. */
	data->triggered |= status & GENMASK64(CONFIG_NXP_S32_WKPU_SOURCES_MAX - 1U, 0U);
}

int wkpu_nxp_s32_set_wakeup(const struct device *dev, uint32_t source,
			    enum wkpu_nxp_s32_trigger trigger, bool enable)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	struct wkpu_nxp_s32_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t mask;
	uint8_t bank;
	uint32_t edges = trigger + 1U;
	int ret = wkpu_nxp_s32_validate_trigger(source, trigger);

	if (ret != 0) {
		return ret;
	}

	mask = BIT(source % 32U);
	bank = source / 32U;
	key = k_spin_lock(&data->lock);
	if (enable) {
		if (((REG_READ(WKPU_IRER(bank)) | REG_READ(WKPU_WRER(bank))) & mask) != 0U &&
		    wkpu_nxp_s32_get_edges(dev, source) != edges) {
			k_spin_unlock(&data->lock, key);
			return -EBUSY;
		}
		wkpu_nxp_s32_set_edges(dev, source, edges);
		REG_WRITE(WKPU_WRER(bank), REG_READ(WKPU_WRER(bank)) | mask);
	} else {
		if ((REG_READ(WKPU_WRER(bank)) & mask) != 0U &&
		    wkpu_nxp_s32_get_edges(dev, source) != edges) {
			k_spin_unlock(&data->lock, key);
			return -EBUSY;
		}
		REG_WRITE(WKPU_WRER(bank), REG_READ(WKPU_WRER(bank)) & ~mask);
		if ((REG_READ(WKPU_IRER(bank)) & mask) == 0U) {
			wkpu_nxp_s32_set_edges(dev, source, 0U);
		}
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

int wkpu_nxp_s32_get_event(const struct device *dev, uint32_t source)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	struct wkpu_nxp_s32_data *data = dev->data;
	k_spinlock_key_t key;
	uint64_t mask;
	uint64_t status;
	int ret;

	if (source >= CONFIG_NXP_S32_WKPU_SOURCES_MAX) {
		return -EINVAL;
	}

	mask = BIT64(source);
	key = k_spin_lock(&data->lock);
	status = (uint64_t)REG_READ(WKPU_WISR(source / 32U)) << ((source / 32U) * 32U);
	data->cleared &= status | ~mask;
	data->triggered |= status & mask & ~data->cleared;
	ret = (data->triggered & mask) != 0U;
	k_spin_unlock(&data->lock, key);

	return ret;
}

int wkpu_nxp_s32_clear_event(const struct device *dev, uint32_t source)
{
	const struct wkpu_nxp_s32_config *config = dev->config;
	struct wkpu_nxp_s32_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t mask;
	uint8_t bank;
	uint32_t status;

	if (source >= CONFIG_NXP_S32_WKPU_SOURCES_MAX) {
		return -EINVAL;
	}

	mask = BIT(source % 32U);
	bank = source / 32U;
	key = k_spin_lock(&data->lock);
	data->triggered &= ~BIT64(source);
	if ((REG_READ(WKPU_IRER(bank)) & mask) != 0U) {
		/* Never acknowledge an IRQ, including one arriving after this read. */
		status = REG_READ(WKPU_WISR(bank)) & mask;
		data->cleared &= ~BIT64(source);
		data->cleared |= (uint64_t)status << (bank * 32U);
	} else {
		REG_WRITE(WKPU_WISR(bank), mask);
		data->cleared &= ~BIT64(source);
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif

#define WKPU_NXP_S32_FILTER_CONFIG(idx, n)                                                         \
	COND_CODE_1(DT_PROP(DT_INST_CHILD(n, irq_##idx), filter_enable), (BIT64(idx)), (0U))

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

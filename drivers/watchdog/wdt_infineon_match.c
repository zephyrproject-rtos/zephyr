/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* Watchdog block: WDT_CTL or WDT_DISABLE_KEY at offset 0 */
#define IFX_MWDT_CTL         0x00U
#define IFX_MWDT_DISABLE_KEY 0x00U
#define IFX_MWDT_CNT         0x04U
#define IFX_MWDT_MATCH       0x08U
#define IFX_MWDT_MATCH2      0x0CU

/* System resources interrupt block */
#define IFX_MWDT_INTR      0x00U
#define IFX_MWDT_INTR_MASK 0x08U

#define IFX_MWDT_CTL_WDT_EN   BIT(0)
#define IFX_MWDT_CTL_WDT_LOCK GENMASK(31, 30)

/* Two different writes open the lock, one write closes it */
#define IFX_MWDT_LOCK_CLR0  1U
#define IFX_MWDT_LOCK_CLR1  2U
#define IFX_MWDT_LOCK_SET01 3U

/* Any other key value enables the watchdog reset */
#define IFX_MWDT_DISABLE_KEY_VALUE 0xACED8865U

#define IFX_MWDT_MATCH_MATCH         GENMASK(15, 0)
#define IFX_MWDT_MATCH_IGNORE_BITS   GENMASK(19, 16)
#define IFX_MWDT_MATCH2_IGNORE_ABOVE GENMASK(4, 0)

#define IFX_MWDT_INTR_WDT_MATCH BIT(0)

/* The four LSBs of the counter are always compared */
#define IFX_MWDT_MIN_BITS 4U

/* Minimum distance in ticks between the counter and a new match value */
#define IFX_MWDT_MIN_LEAD 4U

struct ifx_mwdt_config {
	mem_addr_t wdt;
	mem_addr_t intr;
	uint32_t rate;
	uint8_t width;
	/* Offset 0 is WDT_CTL with an enable bit and a lock, otherwise WDT_DISABLE_KEY */
	bool ctl;
	/* WDT_MATCH2 sets the compared bits, otherwise WDT_MATCH.IGNORE_BITS */
	bool match2;
	void (*irq_config)(void);
};

struct ifx_mwdt_data {
	struct k_spinlock lock;
	wdt_callback_t callback;
	/* Ticks from a service to the next match */
	uint32_t lead;
	/* Compared counter bits, a match comes every 2^bits ticks */
	uint8_t bits;
	/* The third unserviced match resets the device, otherwise the ISR services it */
	bool reset;
	bool installed;
	bool active;
};

static inline uint32_t ifx_mwdt_read(const struct device *dev, mem_addr_t reg)
{
	const struct ifx_mwdt_config *config = dev->config;

	return sys_read32(config->wdt + reg);
}

static inline void ifx_mwdt_write(const struct device *dev, mem_addr_t reg, uint32_t val)
{
	const struct ifx_mwdt_config *config = dev->config;

	sys_write32(val, config->wdt + reg);
}

static void ifx_mwdt_unlock(const struct device *dev)
{
	const struct ifx_mwdt_config *config = dev->config;
	uint32_t ctl;

	if (!config->ctl) {
		return;
	}

	ctl = ifx_mwdt_read(dev, IFX_MWDT_CTL) & ~IFX_MWDT_CTL_WDT_LOCK;
	ifx_mwdt_write(dev, IFX_MWDT_CTL,
		       ctl | FIELD_PREP(IFX_MWDT_CTL_WDT_LOCK, IFX_MWDT_LOCK_CLR0));
	ifx_mwdt_write(dev, IFX_MWDT_CTL,
		       ctl | FIELD_PREP(IFX_MWDT_CTL_WDT_LOCK, IFX_MWDT_LOCK_CLR1));
}

static void ifx_mwdt_lock(const struct device *dev)
{
	const struct ifx_mwdt_config *config = dev->config;
	uint32_t ctl;

	if (!config->ctl) {
		return;
	}

	ctl = ifx_mwdt_read(dev, IFX_MWDT_CTL) & ~IFX_MWDT_CTL_WDT_LOCK;
	ifx_mwdt_write(dev, IFX_MWDT_CTL,
		       ctl | FIELD_PREP(IFX_MWDT_CTL_WDT_LOCK, IFX_MWDT_LOCK_SET01));
}

static bool ifx_mwdt_is_enabled(const struct device *dev)
{
	const struct ifx_mwdt_config *config = dev->config;

	if (config->ctl) {
		return (ifx_mwdt_read(dev, IFX_MWDT_CTL) & IFX_MWDT_CTL_WDT_EN) != 0U;
	}

	return ifx_mwdt_read(dev, IFX_MWDT_DISABLE_KEY) != IFX_MWDT_DISABLE_KEY_VALUE;
}

/* Must be called with the lock open. Without WDT_CTL the counter keeps running. */
static void ifx_mwdt_set_enable(const struct device *dev, bool enable)
{
	const struct ifx_mwdt_config *config = dev->config;
	uint32_t ctl;

	if (!config->ctl) {
		ifx_mwdt_write(dev, IFX_MWDT_DISABLE_KEY, enable ? 0U : IFX_MWDT_DISABLE_KEY_VALUE);
		return;
	}

	ctl = ifx_mwdt_read(dev, IFX_MWDT_CTL) & ~(IFX_MWDT_CTL_WDT_LOCK | IFX_MWDT_CTL_WDT_EN);
	ifx_mwdt_write(dev, IFX_MWDT_CTL, enable ? (ctl | IFX_MWDT_CTL_WDT_EN) : ctl);
}

static void ifx_mwdt_set_irq_mask(const struct device *dev, bool unmask)
{
	const struct ifx_mwdt_config *config = dev->config;
	uint32_t mask = sys_read32(config->intr + IFX_MWDT_INTR_MASK);

	if (unmask) {
		mask |= IFX_MWDT_INTR_WDT_MATCH;
	} else {
		mask &= ~IFX_MWDT_INTR_WDT_MATCH;
	}

	sys_write32(mask, config->intr + IFX_MWDT_INTR_MASK);
}

/* Clearing the match flag services the watchdog, the read waits for the clear */
static void ifx_mwdt_clear_flag(const struct device *dev)
{
	const struct ifx_mwdt_config *config = dev->config;

	sys_write32(IFX_MWDT_INTR_WDT_MATCH, config->intr + IFX_MWDT_INTR);
	(void)sys_read32(config->intr + IFX_MWDT_INTR);
}

/* Must be called with the lock open. Puts the next match lead ticks ahead of the counter. */
static void ifx_mwdt_rearm(const struct device *dev)
{
	const struct ifx_mwdt_config *config = dev->config;
	struct ifx_mwdt_data *data = dev->data;
	uint32_t match = ifx_mwdt_read(dev, IFX_MWDT_CNT) + data->lead;

	if (config->match2) {
		ifx_mwdt_write(dev, IFX_MWDT_MATCH, match);
		return;
	}

	ifx_mwdt_write(dev, IFX_MWDT_MATCH,
		       FIELD_PREP(IFX_MWDT_MATCH_IGNORE_BITS, config->width - data->bits) |
			       FIELD_PREP(IFX_MWDT_MATCH_MATCH, match));
}

/*
 * The reset comes at the third unserviced match: lead + 2 * 2^bits ticks after a service.
 * Picks the shortest such time that is not shorter than the requested one.
 */
static int ifx_mwdt_reset_period(const struct ifx_mwdt_config *config, uint64_t ticks,
				 uint8_t *bits, uint32_t *lead)
{
	for (uint8_t k = IFX_MWDT_MIN_BITS; k <= config->width; k++) {
		uint64_t period = BIT64(k);

		if ((3U * period) - IFX_MWDT_MIN_LEAD >= ticks) {
			uint64_t rest = (ticks > 2U * period) ? (ticks - 2U * period) : 0U;

			*bits = k;
			*lead = (uint32_t)MAX(rest, IFX_MWDT_MIN_LEAD);

			return 0;
		}
	}

	return -EINVAL;
}

/* Without a reset the ISR services every match, the first one comes lead ticks after a service */
static int ifx_mwdt_service_period(const struct ifx_mwdt_config *config, uint64_t ticks,
				   uint8_t *bits, uint32_t *lead)
{
	for (uint8_t k = IFX_MWDT_MIN_BITS; k <= config->width; k++) {
		if (BIT64(k) - IFX_MWDT_MIN_LEAD >= ticks) {
			*bits = k;
			*lead = (uint32_t)MAX(ticks, IFX_MWDT_MIN_LEAD);

			return 0;
		}
	}

	return -EINVAL;
}

static bool ifx_mwdt_has_next_stage(const struct wdt_timeout_cfg *cfg)
{
#ifdef CONFIG_WDT_MULTISTAGE
	return cfg->next != NULL;
#else
	ARG_UNUSED(cfg);

	return false;
#endif
}

static int ifx_mwdt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct ifx_mwdt_config *config = dev->config;
	struct ifx_mwdt_data *data = dev->data;
	k_spinlock_key_t key;
	uint64_t ticks;
	uint32_t lead;
	uint8_t bits;
	bool reset;
	int ret;

	/* There is no lower limit */
	if ((cfg->window.max == 0U) || (cfg->window.min != 0U)) {
		return -EINVAL;
	}

	if (ifx_mwdt_has_next_stage(cfg)) {
		return -ENOTSUP;
	}

	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		/* The only interrupt comes at the first missed match, not before the reset */
		if (cfg->callback != NULL) {
			return -ENOTSUP;
		}
		reset = true;
		break;
	case WDT_FLAG_RESET_NONE:
		/* The ISR has to service every match to keep the reset away */
		if (config->irq_config == NULL) {
			return -ENOTSUP;
		}
		reset = false;
		break;
	default:
		return -ENOTSUP;
	}

	ticks = DIV_ROUND_UP((uint64_t)cfg->window.max * config->rate, MSEC_PER_SEC);

	if (reset) {
		ret = ifx_mwdt_reset_period(config, ticks, &bits, &lead);
	} else {
		ret = ifx_mwdt_service_period(config, ticks, &bits, &lead);
	}

	if (ret != 0) {
		return ret;
	}

	key = k_spin_lock(&data->lock);

	if (data->active) {
		ret = -EBUSY;
	} else if (data->installed) {
		ret = -ENOMEM;
	} else {
		data->callback = cfg->callback;
		data->lead = lead;
		data->bits = bits;
		data->reset = reset;
		data->installed = true;
	}

	k_spin_unlock(&data->lock, key);

	/* Channel 0 is the only channel */
	return ret;
}

static int ifx_mwdt_setup(const struct device *dev, uint8_t options)
{
	const struct ifx_mwdt_config *config = dev->config;
	struct ifx_mwdt_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	/* The counter has no pause control */
	if ((options & (WDT_OPT_PAUSE_IN_SLEEP | WDT_OPT_PAUSE_HALTED_BY_DBG)) != 0U) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	if (!data->installed) {
		ret = -EINVAL;
		goto out;
	}

	if (data->active) {
		ret = -EBUSY;
		goto out;
	}

	ifx_mwdt_unlock(dev);

	if (config->match2) {
		ifx_mwdt_write(dev, IFX_MWDT_MATCH2,
			       FIELD_PREP(IFX_MWDT_MATCH2_IGNORE_ABOVE, data->bits - 1U));
	}

	ifx_mwdt_rearm(dev);

	/* Drop a stale match, and only then let it reach the CPU */
	ifx_mwdt_clear_flag(dev);
	ifx_mwdt_set_irq_mask(dev, !data->reset);
	ifx_mwdt_set_enable(dev, true);

	ifx_mwdt_lock(dev);

	data->active = true;

out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ifx_mwdt_disable(const struct device *dev)
{
	struct ifx_mwdt_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	key = k_spin_lock(&data->lock);

	if (!ifx_mwdt_is_enabled(dev)) {
		ret = -EFAULT;
	} else {
		ifx_mwdt_unlock(dev);
		ifx_mwdt_set_enable(dev, false);
		ifx_mwdt_set_irq_mask(dev, false);
		ifx_mwdt_clear_flag(dev);
		ifx_mwdt_lock(dev);
	}

	data->installed = false;
	data->active = false;
	data->callback = NULL;

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ifx_mwdt_feed(const struct device *dev, int channel_id)
{
	struct ifx_mwdt_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	if (channel_id != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (!data->active) {
		ret = -EINVAL;
	} else {
		/* Move the match first, so a match at the old position is cleared too */
		ifx_mwdt_unlock(dev);
		ifx_mwdt_rearm(dev);
		ifx_mwdt_clear_flag(dev);
		ifx_mwdt_lock(dev);
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static void __maybe_unused ifx_mwdt_isr(const struct device *dev)
{
	const struct ifx_mwdt_config *config = dev->config;
	struct ifx_mwdt_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t pending;

	pending = sys_read32(config->intr + IFX_MWDT_INTR) &
		  sys_read32(config->intr + IFX_MWDT_INTR_MASK);

	/* The interrupt line is shared with other system resources sources */
	if ((pending & IFX_MWDT_INTR_WDT_MATCH) == 0U) {
		return;
	}

	/* Only a timeout without reset unmasks the match, so the ISR services it */
	key = k_spin_lock(&data->lock);
	ifx_mwdt_unlock(dev);
	ifx_mwdt_rearm(dev);
	ifx_mwdt_clear_flag(dev);
	ifx_mwdt_lock(dev);
	k_spin_unlock(&data->lock, key);

	if (data->callback != NULL) {
		data->callback(dev, 0);
	}
}

static int ifx_mwdt_init(const struct device *dev)
{
	const struct ifx_mwdt_config *config = dev->config;
	struct ifx_mwdt_data *data = dev->data;
	k_spinlock_key_t key;

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		key = k_spin_lock(&data->lock);
		ifx_mwdt_unlock(dev);
		ifx_mwdt_set_enable(dev, false);
		ifx_mwdt_set_irq_mask(dev, false);
		ifx_mwdt_clear_flag(dev);
		ifx_mwdt_lock(dev);
		k_spin_unlock(&data->lock, key);
	}

	if (config->irq_config != NULL) {
		config->irq_config();
	}

	return 0;
}

static DEVICE_API(wdt, ifx_mwdt_api) = {
	.setup = ifx_mwdt_setup,
	.disable = ifx_mwdt_disable,
	.install_timeout = ifx_mwdt_install_timeout,
	.feed = ifx_mwdt_feed,
};

#define IFX_MWDT_IRQ_CONFIG(node_id)                                                               \
	static void CONCAT(ifx_mwdt_irq_config_, DT_DEP_ORD(node_id))(void)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(node_id), DT_IRQ(node_id, priority), ifx_mwdt_isr,             \
			    DEVICE_DT_GET(node_id), 0);                                            \
		irq_enable(DT_IRQN(node_id));                                                      \
	}

#define IFX_MWDT_DEFINE(node_id, counter_width, has_ctl, has_match2)                               \
	BUILD_ASSERT(DT_PROP(DT_CLOCKS_CTLR(node_id), clock_frequency) > 0U,                       \
		     "Watchdog clock needs a clock-frequency");                                    \
                                                                                                   \
	IF_ENABLED(DT_IRQ_HAS_IDX(node_id, 0), (IFX_MWDT_IRQ_CONFIG(node_id)))                     \
                                                                                                   \
	static const struct ifx_mwdt_config CONCAT(ifx_mwdt_config_, DT_DEP_ORD(node_id)) = {      \
		.wdt = DT_REG_ADDR_BY_NAME(node_id, wdt),                                          \
		.intr = DT_REG_ADDR_BY_NAME(node_id, intr),                                        \
		.rate = DT_PROP(DT_CLOCKS_CTLR(node_id), clock_frequency),                         \
		.width = (counter_width),                                                          \
		.ctl = (has_ctl),                                                                  \
		.match2 = (has_match2),                                                            \
		.irq_config = COND_CODE_1(DT_IRQ_HAS_IDX(node_id, 0),                              \
				(CONCAT(ifx_mwdt_irq_config_, DT_DEP_ORD(node_id))), (NULL)),      \
	};                                                                                         \
                                                                                                   \
	static struct ifx_mwdt_data CONCAT(ifx_mwdt_data_, DT_DEP_ORD(node_id));                   \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ifx_mwdt_init, NULL,                                             \
			 &CONCAT(ifx_mwdt_data_, DT_DEP_ORD(node_id)),                             \
			 &CONCAT(ifx_mwdt_config_, DT_DEP_ORD(node_id)), PRE_KERNEL_1,             \
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ifx_mwdt_api);

/* PSOC 4: 16-bit counter that cannot be stopped, disable key instead of WDT_CTL */
#define IFX_MWDT_DEFINE_SRSSLT(node_id) IFX_MWDT_DEFINE(node_id, 16U, false, false)

/* PSOC 6: 16-bit counter, WDT_CTL, compared bits in WDT_MATCH */
#define IFX_MWDT_DEFINE_SRSS_V1(node_id) IFX_MWDT_DEFINE(node_id, 16U, true, false)

/* CAT1B and PSOC Edge: counter width from devicetree, compared bits in WDT_MATCH2 */
#define IFX_MWDT_DEFINE_MATCH2(node_id)                                                            \
	IFX_MWDT_DEFINE(node_id, DT_PROP(node_id, counter_bits), true, true)

DT_FOREACH_STATUS_OKAY(infineon_match_wdt_srsslt, IFX_MWDT_DEFINE_SRSSLT)
DT_FOREACH_STATUS_OKAY(infineon_match_wdt_srss_v1, IFX_MWDT_DEFINE_SRSS_V1)
DT_FOREACH_STATUS_OKAY(infineon_match_wdt, IFX_MWDT_DEFINE_MATCH2)

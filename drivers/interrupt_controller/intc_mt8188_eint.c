/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mediatek_mt8188_eint

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_mtk_eint.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

/*
 * Every register group holds one bit per line, so a line picks a word within
 * its group and a bit within that word.  Writing a bit to the SET or CLR alias
 * of a group changes that bit alone, which is what lets this driver touch a
 * single line without a read-modify-write and therefore without a lock.
 *
 * MASK is inverted with respect to the API: a set bit stops the line.
 * SENS selects level rather than edge detection, POL selects high rather than
 * low, and between them they give the four conditions in enum eint_mtk_trigger.
 *
 * DOM_EN routes a line to the application processors.  It is the one group
 * written here that has no SET or CLR alias, so it takes a read-modify-write,
 * under the lock.
 */
#define EINT_STA      0x000
#define EINT_ACK      0x040
#define EINT_MASK     0x080
#define EINT_MASK_SET 0x0c0
#define EINT_MASK_CLR 0x100
#define EINT_SENS     0x140
#define EINT_SENS_SET 0x180
#define EINT_SENS_CLR 0x1c0
#define EINT_POL      0x300
#define EINT_POL_SET  0x340
#define EINT_POL_CLR  0x380
#define EINT_DOM_EN   0x400

/* Lines held in one register of a group. */
#define EINT_LINES_PER_WORD 32U

/* The consumer interface addresses a line as a uint8_t. */
#define EINT_MT8188_MAX_LINES 256U

struct eint_mt8188_config {
	DEVICE_MMIO_ROM; /* Must be first. */
	void (*irq_config)(void);
	uint16_t num_lines;
};

struct eint_mt8188_data {
	DEVICE_MMIO_RAM; /* Must be first. */
	struct k_spinlock lock;
	sys_slist_t callbacks;
	/* Lines this driver has enabled, and so the only ones it services. */
	uint32_t enabled[EINT_MT8188_MAX_LINES / EINT_LINES_PER_WORD];
};

static inline uint32_t eint_mt8188_word(uint8_t line)
{
	return (line / EINT_LINES_PER_WORD) * sizeof(uint32_t);
}

static inline uint32_t eint_mt8188_bit(uint8_t line)
{
	return BIT(line % EINT_LINES_PER_WORD);
}

static void eint_mt8188_set(const struct device *dev, uint32_t group, uint8_t line)
{
	sys_write32(eint_mt8188_bit(line), DEVICE_MMIO_GET(dev) + group + eint_mt8188_word(line));
}

static uint32_t eint_mt8188_get(const struct device *dev, uint32_t group, uint8_t line)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + group + eint_mt8188_word(line));
}

static int eint_mt8188_add_callback(const struct device *dev, struct eint_mtk_callback *callback)
{
	const struct eint_mt8188_config *config = dev->config;
	struct eint_mt8188_data *data = dev->data;
	struct eint_mtk_callback *registered;
	k_spinlock_key_t key;
	int ret = 0;

	if ((callback == NULL) || (callback->cb_handler == NULL) || (callback->cb_dev == NULL) ||
	    (callback->num_lines == 0U)) {
		return -EINVAL;
	}

	if (((uint32_t)callback->first_line + callback->num_lines) > config->num_lines) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/*
	 * Reject an overlap rather than letting two consumers share a line: the
	 * second registration would never be reached, because dispatch stops at
	 * the first range that contains the line.
	 */
	SYS_SLIST_FOR_EACH_CONTAINER(&data->callbacks, registered, node) {
		if ((callback->first_line <
		     ((uint32_t)registered->first_line + registered->num_lines)) &&
		    (registered->first_line <
		     ((uint32_t)callback->first_line + callback->num_lines))) {
			ret = -EINVAL;
			break;
		}
	}

	if (ret == 0) {
		sys_slist_prepend(&data->callbacks, &callback->node);
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static void eint_mt8188_remove_callback(const struct device *dev,
					struct eint_mtk_callback *callback)
{
	struct eint_mt8188_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	(void)sys_slist_find_and_remove(&data->callbacks, &callback->node);

	k_spin_unlock(&data->lock, key);
}

static int eint_mt8188_enable(const struct device *dev, uint8_t line)
{
	const struct eint_mt8188_config *config = dev->config;
	struct eint_mt8188_data *data = dev->data;
	mm_reg_t dom_en;
	k_spinlock_key_t key;

	if (line >= config->num_lines) {
		return -EINVAL;
	}

	dom_en = DEVICE_MMIO_GET(dev) + EINT_DOM_EN + eint_mt8188_word(line);

	/* Claimed before it is unmasked, so a line that fires at once is serviced. */
	key = k_spin_lock(&data->lock);
	data->enabled[line / EINT_LINES_PER_WORD] |= eint_mt8188_bit(line);

	/*
	 * Whatever ran before this driver may already have routed every line to
	 * the application processors, but nothing guarantees it, and a line that
	 * is not routed is unmasked to no effect.
	 */
	sys_write32(sys_read32(dom_en) | eint_mt8188_bit(line), dom_en);
	k_spin_unlock(&data->lock, key);

	/* A set mask bit stops the line, so enabling clears it. */
	eint_mt8188_set(dev, EINT_MASK_CLR, line);

	return 0;
}

static int eint_mt8188_disable(const struct device *dev, uint8_t line)
{
	const struct eint_mt8188_config *config = dev->config;
	struct eint_mt8188_data *data = dev->data;
	k_spinlock_key_t key;

	if (line >= config->num_lines) {
		return -EINVAL;
	}

	eint_mt8188_set(dev, EINT_MASK_SET, line);

	/* Released after it is masked, so one that fires in between is still serviced. */
	key = k_spin_lock(&data->lock);
	data->enabled[line / EINT_LINES_PER_WORD] &= ~eint_mt8188_bit(line);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static bool eint_mt8188_is_enabled(const struct device *dev, uint8_t line)
{
	const struct eint_mt8188_config *config = dev->config;

	if (line >= config->num_lines) {
		return false;
	}

	return (eint_mt8188_get(dev, EINT_MASK, line) & eint_mt8188_bit(line)) == 0U;
}

static int eint_mt8188_set_trigger(const struct device *dev, uint8_t line,
				   enum eint_mtk_trigger trig)
{
	const struct eint_mt8188_config *config = dev->config;
	uint32_t sens;
	uint32_t pol;

	if (line >= config->num_lines) {
		return -EINVAL;
	}

	switch (trig) {
	case EINT_MTK_TRIG_EDGE_RISING:
		sens = EINT_SENS_CLR;
		pol = EINT_POL_SET;
		break;

	case EINT_MTK_TRIG_EDGE_FALLING:
		sens = EINT_SENS_CLR;
		pol = EINT_POL_CLR;
		break;

	case EINT_MTK_TRIG_LEVEL_HIGH:
		sens = EINT_SENS_SET;
		pol = EINT_POL_SET;
		break;

	case EINT_MTK_TRIG_LEVEL_LOW:
		sens = EINT_SENS_SET;
		pol = EINT_POL_CLR;
		break;

	default:
		return -EINVAL;
	}

	eint_mt8188_set(dev, sens, line);
	eint_mt8188_set(dev, pol, line);

	/*
	 * Changing the condition can itself latch a status bit - inverting the
	 * polarity of a line that is already at the new active level looks like
	 * an edge - so drop whatever is pending.  Otherwise a line enabled
	 * after this call fires once for a condition that predates it.
	 */
	eint_mt8188_set(dev, EINT_ACK, line);

	return 0;
}

static int eint_mt8188_set_polarity(const struct device *dev, uint8_t line, bool high)
{
	const struct eint_mt8188_config *config = dev->config;

	if (line >= config->num_lines) {
		return -EINVAL;
	}

	/*
	 * No acknowledge here, unlike eint_mt8188_set_trigger(): this is how a
	 * consumer turns a line around between events, and an edge that latches
	 * meanwhile is a real one that must still fire.
	 */
	eint_mt8188_set(dev, high ? EINT_POL_SET : EINT_POL_CLR, line);

	return 0;
}

/*
 * Find what services a line.  Returns false for a line this driver did not
 * enable; for one it did, *handler is NULL if no registered range covers it.
 */
static bool eint_mt8188_lookup(const struct device *dev, uint8_t line,
			       eint_mtk_cb_handler_t *handler, const struct device **cb_dev,
			       void **cb_arg)
{
	struct eint_mt8188_data *data = dev->data;
	struct eint_mtk_callback *callback;
	k_spinlock_key_t key;
	bool enabled;

	*handler = NULL;

	key = k_spin_lock(&data->lock);

	enabled = (data->enabled[line / EINT_LINES_PER_WORD] & eint_mt8188_bit(line)) != 0U;

	if (enabled) {
		SYS_SLIST_FOR_EACH_CONTAINER(&data->callbacks, callback, node) {
			if ((line >= callback->first_line) &&
			    (line < ((uint32_t)callback->first_line + callback->num_lines))) {
				*handler = callback->cb_handler;
				*cb_dev = callback->cb_dev;
				*cb_arg = callback->cb_arg;
				break;
			}
		}
	}

	k_spin_unlock(&data->lock, key);

	return enabled;
}

static void eint_mt8188_isr(const struct device *dev)
{
	const struct eint_mt8188_config *config = dev->config;
	uint16_t base;

	for (base = 0U; base < config->num_lines; base += EINT_LINES_PER_WORD) {
		uint16_t remaining = config->num_lines - base;
		uint32_t word = (base / EINT_LINES_PER_WORD) * sizeof(uint32_t);
		uint32_t status = sys_read32(DEVICE_MMIO_GET(dev) + EINT_STA + word);

		/*
		 * The last word is only partly populated, and the bits above the
		 * last line read back as whatever the block leaves there.
		 */
		if (remaining < EINT_LINES_PER_WORD) {
			status &= BIT_MASK(remaining);
		}

		while (status != 0U) {
			uint32_t bit = find_lsb_set(status) - 1U;
			uint8_t line = (uint8_t)(base + bit);
			eint_mtk_cb_handler_t handler;
			const struct device *cb_dev;
			void *cb_arg;

			status &= ~BIT(bit);

			if (!eint_mt8188_lookup(dev, line, &handler, &cb_dev, &cb_arg) ||
			    (handler == NULL)) {
				/*
				 * Not a line this driver enabled, or one no
				 * registration covers, so nothing here services it.
				 * Stop it before clearing it: a level that is still
				 * asserted would otherwise latch again at once and
				 * re-enter this handler indefinitely.
				 */
				sys_write32(BIT(bit), DEVICE_MMIO_GET(dev) + EINT_MASK_SET + word);
				sys_write32(BIT(bit), DEVICE_MMIO_GET(dev) + EINT_ACK + word);
				continue;
			}

			/*
			 * Acknowledge before the handler runs.  An edge that
			 * arrives while it is running then leaves the status bit
			 * set and the line fires again, rather than being
			 * cleared afterwards and lost.
			 */
			sys_write32(BIT(bit), DEVICE_MMIO_GET(dev) + EINT_ACK + word);

			/*
			 * Called with the lock dropped: a handler is free to
			 * reconfigure its lines, and one that registered a
			 * further range would otherwise deadlock on a lock this
			 * driver already holds.
			 */
			handler(cb_dev, line, cb_arg);
		}
	}
}

static DEVICE_API(eint_mtk, eint_mt8188_api) = {
	.add_callback = eint_mt8188_add_callback,
	.remove_callback = eint_mt8188_remove_callback,
	.enable = eint_mt8188_enable,
	.disable = eint_mt8188_disable,
	.is_enabled = eint_mt8188_is_enabled,
	.set_trigger = eint_mt8188_set_trigger,
	.set_polarity = eint_mt8188_set_polarity,
};

static int eint_mt8188_init(const struct device *dev)
{
	const struct eint_mt8188_config *config = dev->config;
	struct eint_mt8188_data *data = dev->data;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	sys_slist_init(&data->callbacks);

	/*
	 * Lines are left as they are found.  The block may be shared line by
	 * line with another operating system, which owns whatever it has
	 * enabled, so stopping every line here would take its interrupts away.
	 * A line this driver never enabled is stopped instead the first time it
	 * fires, by the interrupt handler.
	 */

	config->irq_config();

	return 0;
}

#define EINT_MT8188_INIT(n)                                                                        \
	BUILD_ASSERT(DT_INST_PROP(n, num_lines) <= EINT_MT8188_MAX_LINES,                          \
		     "num-lines exceeds what a uint8_t line number can address");                  \
                                                                                                   \
	static void eint_mt8188_irq_config_##n(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), eint_mt8188_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static struct eint_mt8188_data eint_mt8188_data_##n;                                       \
                                                                                                   \
	static const struct eint_mt8188_config eint_mt8188_config_##n = {                          \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.irq_config = eint_mt8188_irq_config_##n,                                          \
		.num_lines = DT_INST_PROP(n, num_lines),                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, eint_mt8188_init, NULL, &eint_mt8188_data_##n,                    \
			      &eint_mt8188_config_##n, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,    \
			      &eint_mt8188_api);

DT_INST_FOREACH_STATUS_OKAY(EINT_MT8188_INIT)

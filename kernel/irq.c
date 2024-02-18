/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/intc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/internal/irq.h>
#include <zephyr/sys/irq.h>
#include <zephyr/sys/irq_handler.h>
#include <zephyr/sys/slist.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

static inline const struct sys_irq_intl *get_intl(uint16_t irqn)
{
	__ASSERT(irqn < SYS_DT_IRQN_SIZE, "irqn is invalid");

	return &__sys_irq_intls[irqn];
}

int sys_irq_configure(uint16_t irqn, uint32_t flags)
{
	const struct sys_irq_intl *intl;

	intl = get_intl(irqn);

	return intc_configure_irq(intl->intc, intl->intln, flags);
}

#ifdef CONFIG_SYS_IRQ_DYNAMIC

static sys_slist_t sys_irq_intls[SYS_DT_IRQN_SIZE];
static struct k_spinlock lock;

static inline sys_slist_t *get_irq_list(uint16_t irqn)
{
	__ASSERT(irqn < SYS_DT_IRQN_SIZE, "irqn is invalid");

	return &sys_irq_intls[irqn];
}

int sys_irq_enable(uint16_t irqn)
{
	const struct sys_irq_intl *intl;
	int ret;

	intl = get_intl(irqn);

	K_SPINLOCK(&lock) {
		ret = intc_enable_irq(intl->intc, intl->intln);
	}

	return ret;
}

#else /* CONFIG_SYS_IRQ_DYNAMIC */

int sys_irq_enable(uint16_t irqn)
{
	const struct sys_irq_intl *intl;

	intl = get_intl(irqn);

	return intc_enable_irq(intl->intc, intl->intln);
}

#endif /* CONFIG_SYS_IRQ_DYNAMIC */

int sys_irq_disable(uint16_t irqn)
{
	const struct sys_irq_intl *intl;

	intl = get_intl(irqn);

	return intc_disable_irq(intl->intc, intl->intln);
}

int sys_irq_trigger(uint16_t irqn)
{
	const struct sys_irq_intl *intl;

	intl = get_intl(irqn);

	return intc_trigger_irq(intl->intc, intl->intln);
}

int sys_irq_clear(uint16_t irqn)
{
	const struct sys_irq_intl *intl;

	intl = get_intl(irqn);

	return intc_clear_irq(intl->intc, intl->intln);
}

#ifdef CONFIG_SYS_IRQ_DYNAMIC

int sys_irq_request(uint16_t irqn, struct sys_irq *irq, sys_irq_handler_t handler,
		    const void *data)
{
	const struct sys_irq_intl *intl;
	sys_slist_t *irq_list;
	int ret;

	intl = get_intl(irqn);
	irq_list = get_irq_list(irqn);

	irq->handler = handler;
	irq->data = data;

	K_SPINLOCK(&lock) {
		ret = intc_disable_irq(intl->intc, intl->intln);
		if (ret < 0) {
			K_SPINLOCK_BREAK;
		}

		sys_slist_append(irq_list, &irq->node);

		if (ret == 0) {
			K_SPINLOCK_BREAK;
		}

		ret = intc_enable_irq(intl->intc, intl->intln);
	}

	return ret;
}

int sys_irq_release(uint16_t irqn, struct sys_irq *irq)
{
	const struct sys_irq_intl *intl;
	sys_slist_t *irq_list;
	int ret;

	intl = get_intl(irqn);
	irq_list = get_irq_list(irqn);

	K_SPINLOCK(&lock) {
		ret = intc_disable_irq(intl->intc, intl->intln);
		if (ret < 0) {
			K_SPINLOCK_BREAK;
		}

		sys_slist_find_and_remove(irq_list, &irq->node);

		if (ret == 0) {
			K_SPINLOCK_BREAK;
		}

		ret = intc_enable_irq(intl->intc, intl->intln);
	}

	return ret;
}

#endif /* CONFIG_SYS_IRQ_DYNAMIC */

void sys_irq_log_spurious_intl(uint16_t intc_ord, uint16_t intln)
{
	LOG_ERR(">>> ZEPHYR SPURIOUS IRQ: INTC ord: %u, INT line: %u", intc_ord, intln);
}

void sys_irq_spurious_handler(void)
{
	z_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

#ifdef CONFIG_SYS_IRQ_DYNAMIC

int sys_irq_dynamic_handler(uint16_t irqn)
{
	sys_snode_t *node;
	sys_slist_t *irq_list;
	const struct sys_irq *irq;

	irq_list = get_irq_list(irqn);

	SYS_SLIST_FOR_EACH_NODE(irq_list, node) {
		irq = (const struct sys_irq *)node;
		if (irq->handler(irq->data)) {
			return SYS_IRQ_HANDLED;
		}
	}

	return SYS_IRQ_NONE;
}

#endif /* CONFIG_SYS_IRQ_DYNAMIC */

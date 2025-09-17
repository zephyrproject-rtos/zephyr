/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz GmbH
 * Copyright (c) 2025 Bang & Olufsen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <ti/driverlib/driverlib.h>

#if defined(CONFIG_SOC_TI_MSPM0_INT_GROUP_0) ||		\
	defined(CONFIG_SOC_TI_MSPM0_INT_GROUP_1)
#include <ti/driverlib/m0p/dl_interrupt.h>

#include <zephyr/irq.h>
#include <errno.h>

#include <soc.h>

struct mspm0_int_grp_iidx {
	void (*isr_handler)(const struct device *dev);
	const struct device *dev;
};
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_0 || CONFIG_SOC_TI_MSPM0_INT_GROUP_1 */

#ifdef CONFIG_SOC_TI_MSPM0_INT_GROUP_0
static struct mspm0_int_grp_iidx mspm0_int_grp0[8];
static uint32_t mspm0_int_grp0_mask;
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_0 */

#ifdef CONFIG_SOC_TI_MSPM0_INT_GROUP_1
static struct mspm0_int_grp_iidx mspm0_int_grp1[8];
static uint32_t mspm0_int_grp1_mask;
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_1 */

#if defined(CONFIG_SOC_TI_MSPM0_INT_GROUP_0) ||		\
	defined(CONFIG_SOC_TI_MSPM0_INT_GROUP_1)
static ALWAYS_INLINE void handle_group_isr(int group, uint32_t int_grp_mask,
					   struct mspm0_int_grp_iidx int_grp[8])
{
	uint32_t triggered = DL_Interrupt_getStatusGroup(group,
							 int_grp_mask) & 0xff;

	DL_Interrupt_clearGroup(group, triggered);

	/* Groups are priority-ordered: the lower the index,
	 * the higher the priority.
	 */
	while (triggered) {
		if (triggered & 1) {
			int_grp->isr_handler(int_grp->dev);
		}

		int_grp++;
		triggered >>= 1;
	}
}

static inline int register_group_isr(uint8_t int_idx,
				     uint32_t *int_grp_mask,
				     struct mspm0_int_grp_iidx int_grp[8],
				     void (*isr_handler)(const struct device *dev),
				     const struct device *dev)
{
	if (*int_grp_mask & BIT(int_idx)) {
		return -EALREADY;
	}

	int_grp[int_idx].isr_handler = isr_handler;
	int_grp[int_idx].dev = dev;

	*int_grp_mask |= BIT(int_idx);

	return 0;
}
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_0 || CONFIG_SOC_TI_MSPM0_INT_GROUP_1 */

#ifdef CONFIG_SOC_TI_MSPM0_INT_GROUP_0
static void mspm0_int_group_0_isr(const struct device *unused)
{
	ARG_UNUSED(unused);

	handle_group_isr(0, mspm0_int_grp0_mask, mspm0_int_grp0);
}
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_0 */

#ifdef CONFIG_SOC_TI_MSPM0_INT_GROUP_1
static void mspm0_int_group_1_isr(const struct device *unused)
{
	ARG_UNUSED(unused);

	handle_group_isr(1, mspm0_int_grp1_mask, mspm0_int_grp1);
}
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_1 */

#if defined(CONFIG_SOC_TI_MSPM0_INT_GROUP_0) ||		\
	defined(CONFIG_SOC_TI_MSPM0_INT_GROUP_1)
int mspm0_register_int_to_group(int group, uint8_t int_idx,
				void (*isr_handler)(const struct device *dev),
				const struct device *dev)
{
	int ret;

	if ((!IS_ENABLED(CONFIG_SOC_TI_MSPM0_INT_GROUP_0) && group == 0) ||
	    (!IS_ENABLED(CONFIG_SOC_TI_MSPM0_INT_GROUP_1) && group == 1)) {
		return -ENOTSUP;
	}

	if (int_idx > 7 || isr_handler == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_SOC_TI_MSPM0_INT_GROUP_0
	if (group == 0) {
		ret = register_group_isr(int_idx, &mspm0_int_grp0_mask,
					 mspm0_int_grp0, isr_handler, dev);
	} else
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_0 */
#ifdef CONFIG_SOC_TI_MSPM0_INT_GROUP_1
	if (group == 1) {
		ret = register_group_isr(int_idx, &mspm0_int_grp1_mask,
					 mspm0_int_grp1, isr_handler, dev);
	} else
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_1 */
	{
		return -EINVAL;
	}

	if (ret != 0) {
		return ret;
	}

	irq_enable(group);

	return 0;
}
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_0 || CONFIG_SOC_TI_MSPM0_INT_GROUP_1 */

void soc_early_init_hook(void)
{
	/* Low Power Mode is configured to be SLEEP0 */
	DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

#ifdef CONFIG_SOC_TI_MSPM0_INT_GROUP_0
	IRQ_CONNECT(0, 0, mspm0_int_group_0_isr, NULL, 0);
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_0 */
#ifdef CONFIG_SOC_TI_MSPM0_INT_GROUP_1
	IRQ_CONNECT(1, 0, mspm0_int_group_1_isr, NULL, 0);
#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_1 */
}

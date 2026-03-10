/*
 * Copyright 2023,2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for NXP's IRQ_STEER IP.
 *
 * Below you may find some useful information that will help you better understand how the
 * driver works. The ">" sign is used to mark ideas that are considered important and should
 * be taken note of.
 *
 * 1) What is the IRQ_STEER IP?
 *	- in Zephyr terminology, the IRQ_STEER can be considered an interrupt aggregator. As such,
 *	its main goal is to multiplex multiple interrupt lines into a single/multiple ones.
 *
 * 2) How does the IRQ_STEER IP work?
 *	- below you may find a diagram meant to give you an intuition regarding the IP's structure
 *	and how it works (all of the information below is applicable to i.MX8MP but it can be
 *	extended to any NXP SoC using the IRQ_STEER IP):
 *
 *                 SYSTEM_INTID[0:159]
 *                        |
 *      MASK[0:4]------   |
 *                    |   |
 *                  +------+
 *                  |      |
 *                  |32 AND|
 *                  |      |
 *                  +------+
 *                        |
 *      SET[0:4]------    |
 *                   |    |
 *                  +------+
 *                  |      |
 *                  |32 OR |
 *                  |      |
 *                  +------+
 *                     |__________ STATUS[0:4]
 *                     |
 *                  +------+
 *                  |GROUP |
 *                  |  BY  |
 *                  |  64  |
 *                  +------+
 *                   |  | |
 *      _____________|  | |________________
 *      |               |                 |
 * MASTER_IN[0]    MASTER_IN[1]      MASTER_IN[2]
 *      |               |                 |
 *      |               |                 |
 *      |_____________  |  _______________|
 *                   |  | |
 *                  +------+
 *                  |      |
 *                  | AND  | ---------- MINTDIS[0:2]
 *                  |      |
 *                  +------+
 *                   |  | |
 *      _____________|  | |________________
 *      |               |                 |
 * MASTER_OUT[0]  MASTER_OUT[1]      MASTER_OUT[2]
 *
 *	- initially, all SYSTEM_INTID are grouped by 32 => 5 groups.
 *
 *	> each of these groups is controlled by a MASK, SET and STATUS index as follows:
 *
 *		MASK/SET/STATUS[0] => SYSTEM_INTID[159:128]
 *		MASK/SET/STATUS[1] => SYSTEM_INTID[127:96]
 *		MASK/SET/STATUS[2] => SYSTEM_INTID[95:64]
 *		MASK/SET/STATUS[3] => SYSTEM_INTID[63:32]
 *		MASK/SET/STATUS[4] => SYSTEM_INTID[31:0]
 *
 *      > after that, all SYSTEM_INTID are grouped by 64 as follows:
 *
 *		SYSTEM_INTID[159:96] => MASTER_IN[2]
 *              SYSTEM_INTID[95:32] => MASTER_IN[1]
 *              SYSTEM_INTID[31:0] => MASTER_IN[0]
 *
 *      note: MASTER_IN[0] is only responsible for 32 interrupts
 *
 *      > the value of MASTER_IN[x] is obtained by OR'ing the input interrupt lines.
 *
 *      > the value of MASTER_OUT[x] is obtained by AND'ing MASTER_IN[x] with !MINTDIS[x].
 *
 *      - whenever a SYSTEM_INTID is asserted, its corresponding MASTER_OUT signal will also
 *	be asserted, thus signaling the target processor.
 *
 *	> please note the difference between an IRQ_STEER channel and an IRQ_STEER master output.
 *	An IRQ_STEER channel refers to an IRQ_STEER instance (e.g: the DSP uses IRQ_STEER channel
 *	0 a.k.a instance 0). An IRQ_STEER channel has multiple master outputs. For example, in
 *	the case of i.MX8MP each IRQ_STEER channel has 3 master outputs since an IRQ_STEER channel
 *	routes 160 interrupts (32 for first master output, 64 for second master output, and 64 for
 *	the third master output).
 *
 * 3) Using Zephyr's multi-level interrupt support
 *	- since Zephyr supports organizing interrupts on multiple levels, we can use this to
 *	separate the interrupts in 2 levels:
 *		1) LEVEL 1 INTERRUPTS
 *			- these are the interrupts that go directly to the processor (for example,
 *			on i.MX8MP the MU can directly assert the DSP's interrupt line 7)
 *
 *		2) LEVEL 2 INTERRUPTS
 *			- these interrupts go through IRQ_STEER and are signaled by a single
 *			processor interrupt line.
 *			- e.g: for i.MX8MP, INTID 34 (SDMA3) goes through IRQ_STEER and is signaled
 *			to the DSP by INTID 20 which is a direct interrupt (or LEVEL 1 interrupt).
 *
 *	- the following diagram (1) shows the interrupt organization on i.MX8MP:
 *	                                                        +------------+
 *								|            |
 *	      SYSTEM_INTID[31:0] ------ IRQ_STEER_MASTER_0 ----	| 19         |
 *								|            |
 *	      SYSTEM_INTID[95:32] ----- IRQ_STEER_MASTER_1 ----	| 20  DSP    |
 *								|            |
 *	      SYSTEM_INTID[159:96] ---- IRQ_STEER_MASTER_2 ----	| 21         |
 *								|            |
 *								+------------+
 *
 *	- as such, asserting a system interrupt will lead to asserting its corresponding DSP
 *	interrupt line (for example, if system interrupt 34 is asserted, that would lead to
 *	interrupt 20 being asserted)
 *
 *	- in the above diagram, SYSTEM_INTID[x] are LEVEL 2 interrupts, while 19, 20, and 21 are
 *	LEVEL 1 interrupts.
 *
 *	- INTID 19 is the parent of SYSTEM_INTID[31:0] and so on.
 *
 *	> before going into how the INTIDs are encoded, we need to distinguish between 3 types of
 *	INTIDs:
 *		1) System INTIDs
 *			- these are the values that can be found in NXP's TRMs for different
 *			SoCs (usually they have the same IDs as the GIC SPIs)
 *			- for example, INTID 34 is a system INTID for SDMA3 (i.MX8MP).
 *
 *		2) Zephyr INTIDs
 *			- these are the Zephyr-specific encodings of the system INTIDs.
 *			- these are used to encode multi-level interrupts (for more information
 *			please see [1])
 *			> if you need to register an interrupt dynamically, you need to use this
 *			encoding when specifying the interrupt.
 *
 *		3) DTS INTIDs
 *			- these are the encodings of the system INTIDs used in the DTS.
 *			- all of these INTIDs are relative to IRQ_STEER's MASTER_OUTs.
 *
 *	> encoding an INTID:
 *		1) SYSTEM INTID => ZEPHYR INTID
 *			- the following steps need to be performed:
 *
 *				a) Find out which IRQ_STEER MASTER
 *				is in charge of aggregating this interrupt.
 *					* for instance, SYSTEM_INTID 34 (SDMA3 on i.MX8MP) is
 *					aggregated by MASTER 1 as depicted in diagram (1).
 *
 *				b) After finding the MASTER aggregator, you need
 *				to find the corresponding parent interrupt.
 *					* for example, SYSTEM_INTID 34 (SDMA3 on i.MX8MP) is
 *					aggregated by MASTER 1, which has the parent INTID 20
 *					as depicted in diagram (1) => PARENT_INTID(34) = 20.
 *
 *				c) Find the INTID relative to the MASTER aggregator. This is done
 *				by subtracting the number of interrupts each of the previous
 *				master aggregators is in charge of. If the master aggregator is
 *				MASTER 0 then RELATIVE_INTID=SYSTEM_INTID.
 *					* for example, SYSTEM_ID 34 is aggregated by MASTER 1.
 *					As such, we need to subtract 32 from 34 (because the
 *					previous master - MASTER 0 - is in charge of aggregating
 *					32 interrupts) => RELATIVE_INTID(34) = 2.
 *
 *					* generally speaking, RELATIVE_INTID can be computed using
 *					the following formula (assuming SYSTEM_INTID belongs to
 *					MASTER y):
 *						RELATIVE_INTID(x) = x -
 *							\sum{i=0}^{y - 1} GET_MASTER_INT_NUM(i)
 *					where:
 *						1) GET_MASTER_INT_NUM(x) computes the number of
 *						interrupts master x aggregates
 *						2) x is the system interrupt
 *
 *					* to make sure your computation is correct use the
 *					following restriction:
 *						0 <= RELATIVE_INTID(x) < GET_MASTER_INT_NUM(y)
 *
 *				d) To the obtained RELATIVE_INTID you need to add the value of 1,
 *				left shift the result by the number of bits used to encode the
 *				level 1 interrupts (see [1] for details) and OR the parent ID.
 *					* for example, RELATIVE_INTID(34) = 2 (i.MX8MP),
 *					PARENT_INTID(34) = 20 => ZEPHYR_INTID = ((2 + 1) << 8) | 20
 *
 *					* generally speaking, ZEPHYR_INTID can be computed using
 *					the following formula:
 *						ZEPHYR_INTID(x) = ((RELATIVE_INTID(x) + 1) <<
 *						NUM_LVL1_BITS) | PARENT_INTID(x)
 *					where:
 *						1) RELATIVE_INTID(x) computes the relative INTID
 *						of system interrupt x (step c).
 *
 *						2) NUM_LVL1_BITS is the number of bits used to
 *						encode level 1 interrupts.
 *
 *						3) PARENT_INTID(x) computes the parent INTID of a
 *						system interrupt x (step b)
 *
 *			- all of these steps are performed by to_zephyr_irq().
 *			> for interrupts aggregated by MASTER 0 you may skip step c) as
 *			RELATIVE_INTID(x) = x.
 *
 *		2) SYSTEM INTID => DTS INTID
 *			- for this you just have to compute RELATIVE_INTID as described above in
 *			step c).
 *			- for example, if an IP uses INTID 34 you'd write its interrupts property
 *			as follows (i.MX8MP):
 *				interrupts = <&master1 2>;
 *
 * 4) Notes and comments
 *	> PLEASE DON'T MISTAKE THE ZEPHYR MULTI-LEVEL INTERRUPT ORGANIZATION WITH THE XTENSA ONE.
 *	THEY ARE DIFFERENT THINGS.
 *
 * [1]: https://docs.zephyrproject.org/latest/kernel/services/interrupts.html#multi-level-interrupt-handling
 */

#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/irq.h>
#include <zephyr/cache.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include "sw_isr_common.h"

LOG_MODULE_REGISTER(nxp_irqstr);

/* used for driver binding */
#define DT_DRV_COMPAT nxp_irqsteer_intc

/* IRQ_STEER register offsets */
#define CTRL_STRIDE_OFF(_t, _r)	(_t * 4 * _r)
#define CHAN_MASK(n, t)		(CTRL_STRIDE_OFF(t, 0) + 0x4 * (n) + 0x4)
#define CHAN_SET(n, t)		(CTRL_STRIDE_OFF(t, 1) + 0x4 * (n) + 0x4)
#define CHAN_STATUS(n, t)	(CTRL_STRIDE_OFF(t, 2) + 0x4 * (n) + 0x4)
#define CHAN_MINTDIS(t)		(CTRL_STRIDE_OFF(t, 3) + 0x4)
#define CHAN_MASTRSTAT(t)	(CTRL_STRIDE_OFF(t, 3) + 0x8)

/* IRQSTEER register bits width is 32 */
#define IRQSTEER_INT_SRC_REG_WIDTH 32
/* IRQSTEER AGGREGATED(fixed) interrupt number per group is 64 */
#define IRQSTEER_AGGRAGATED_INT_NUM_PER_GRP 64

/* macros used for DTS parsing */
#define _IRQSTEER_REGISTER_DISPATCHER(node_id)				\
	IRQ_CONNECT(DT_IRQN(node_id),					\
		    DT_IRQ(node_id, priority),				\
		    irqsteer_isr_dispatcher,				\
		    &dispatchers[DT_REG_ADDR(node_id)],			\
		    0)

#define _IRQSTEER_DECLARE_DISPATCHER(node_id)				\
{									\
	.dev = DEVICE_DT_GET(DT_PARENT(node_id)),			\
	.master_index = DT_REG_ADDR(node_id),				\
	.irq = DT_IRQN(node_id),					\
}

#define IRQSTEER_DECLARE_DISPATCHERS(parent_id)\
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(parent_id, _IRQSTEER_DECLARE_DISPATCHER, (,))

#define IRQSTEER_REGISTER_DISPATCHERS(parent_id)\
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(parent_id, _IRQSTEER_REGISTER_DISPATCHER, (;))

#if defined(CONFIG_XTENSA)
#define irqstr_l1_irq_enable_raw(irq)     xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq))
#define irqstr_l1_irq_disable_raw(irq)    xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq))
#define irqsteer_level1_irq_is_enabled(irq) xtensa_irq_is_enabled(XTENSA_IRQ_NUMBER(irq))
#elif defined(CONFIG_ARM)
#define irqstr_l1_irq_enable_raw(irq)     arm_irq_enable(irq)
#define irqstr_l1_irq_disable_raw(irq)    arm_irq_disable(irq)
#define irqsteer_level1_irq_is_enabled(irq) arm_irq_is_enabled(irq)
#else
#error ARCH not supported
#endif

struct irqsteer_config {
	uint32_t regmap_phys;
	uint32_t regmap_size;
	int irq_offset;
	int irqs_num;
	/* There is one output irq for each group of 64 inputs. */
	int num_masters;
	/* Totol register number of MASK/SET/STATUS */
	int reg_num;
	struct irqsteer_dispatcher *dispatchers;
};

struct irqsteer_dispatcher {
	const struct device *dev;
	/* which set of interrupts is the dispatcher in charge of? */
	uint32_t master_index;
	/* which interrupt line is the dispatcher tied to? */
	uint32_t irq;
	/* reference count for all IRQs aggregated by dispatcher */
	uint8_t irq_refcnt[CONFIG_MAX_IRQ_PER_AGGREGATOR];
};

static struct k_spinlock irqstr_lock;

static uint8_t l1_irq_refcnt[CONFIG_2ND_LVL_ISR_TBL_OFFSET];

static struct irqsteer_dispatcher dispatchers[] = {
	IRQSTEER_DECLARE_DISPATCHERS(DT_NODELABEL(irqsteer))
};

/**
 * @brief Initialize IRQ_STEER hardware
 */
static void irqsteer_hw_init(const struct irqsteer_config *cfg)
{
	int reg_num = cfg->reg_num;
	uint32_t base = cfg->regmap_phys;
	int reg_index = 0;

	/* Disable all master interrupts */
	sys_write32(0xFFFFFFFF, base + CHAN_MINTDIS(reg_num));

	/* Clear all channel masks */
	for (; reg_index < reg_num; reg_index++) {
		sys_write32(0, base + CHAN_MASK(reg_index, reg_num));
	}
}

static int irqsteer_get_reg_index(const struct irqsteer_config *cfg, uint32_t irq)
{
	return (cfg->reg_num - (irq - cfg->irq_offset) / IRQSTEER_INT_SRC_REG_WIDTH - 1);
}

/**
 * @brief Get number of interrupts for a specific master
 */
static int irqsteer_get_master_irq_count(const struct irqsteer_config *cfg, uint32_t master_index)
{
	int count = IRQSTEER_AGGRAGATED_INT_NUM_PER_GRP;

	/*
	 * Each interrupt group(one register represents 32 input interrupts)
	 * has 32 interrupt sources.
	 *
	 * There are two cases based on SOC integration:
	 *
	 * 1. The interrupt group count(number of CHn_MASKx registers) is
	 *    even number.
	 *    In this case, every two interrupt groups are connected to
	 *    one interrupt master. So each master has 64 interrupt
	 *    sources connected.
	 *
	 * 2. The interrupt group count(number of CHn_MASKx registers) is
	 *    odd number.
	 *    In this case, master 0 is connected to one interrupt group,
	 *    while other masters are all connected to two interrupt groups.
	 *    So master 0 has 32 interrupt sources connected, and
	 *    other masters have 64 interrupt sources connected.
	 */
	if ((cfg->reg_num % 2) != 0) {
		if (master_index == 0) {
			count = IRQSTEER_INT_SRC_REG_WIDTH;
		}
	}

	return count;
}

/**
 * @brief Enable or disable specific interrupt in IRQ_STEER
 */
static void irqsteer_enable_interrupt(const struct irqsteer_config *cfg, uint32_t irq,
				      bool enable)
{
	int reg_num = cfg->reg_num;
	uint32_t base = cfg->regmap_phys;
	int reg_index = irqsteer_get_reg_index(cfg, irq);
	uint32_t bit = (irq - cfg->irq_offset) % IRQSTEER_INT_SRC_REG_WIDTH;
	uint32_t mask;

	mask = sys_read32(base + CHAN_MASK(reg_index, reg_num));
	if (enable) {
		mask |= BIT(bit);
	} else {
		mask &= ~BIT(bit);
	}
	sys_write32(mask, base + CHAN_MASK(reg_index, reg_num));
	LOG_DBG("base = 0x%x, reg_index = %d, CHAN_MASK(%d, %d) = 0x%x, bit = %d, mask = 0x%x",
		base, reg_index, reg_index, reg_num, CHAN_MASK(reg_index, reg_num), bit, mask);
}

/**
 * @brief Enable or disable master interrupt output
 */
static void irqsteer_enable_master_interrupt(const struct irqsteer_config *cfg,
					     uint32_t master_index,
					     bool enable)
{
	uint32_t mintdis;
	int reg_num = cfg->reg_num;
	uint32_t base = cfg->regmap_phys;

	mintdis = sys_read32(base + CHAN_MINTDIS(reg_num));
	if (enable) {
		mintdis &= ~BIT(master_index);
	} else {
		mintdis |= BIT(master_index);
	}
	sys_write32(mintdis, base + CHAN_MINTDIS(reg_num));
}

/**
 * @brief Get master interrupt status
 */
static uint64_t irqsteer_get_master_interrupts_status(const struct irqsteer_config *cfg,
						      uint32_t master_index)
{
	uint64_t status = 0;
	uint32_t i;
	int reg_num = cfg->reg_num;
	uint32_t base = cfg->regmap_phys;
	int master_reg_index = (reg_num % 2 != 0 && master_index != 0) ?
			       (reg_num - (master_index * 2)) :
			       (reg_num - 1 - (master_index * 2));
	int master_reg_count = (reg_num % 2 != 0 && master_index == 0) ? 1 : 2;
	uint32_t ch_status = 0;
	uint32_t ch_mask = 0;

	/* Read status from relevant register index */
	for (i = 0; i < master_reg_count; i++) {
		ch_status = sys_read32(base + CHAN_STATUS(master_reg_index, reg_num));
		ch_mask = sys_read32(base + CHAN_MASK(master_reg_index, reg_num));

		/* Only consider masked (enabled) interrupts */
		ch_status &= ch_mask;
		status |= ((uint64_t)ch_status << (i * 32));
		master_reg_index--;
	}

	return status;
}

/* IRQ conversion functions */

/* used to Convert system INTID to Zephyr INTID */
static int to_zephyr_irq(const struct irqsteer_config *cfg, uint32_t irq,
			 struct irqsteer_dispatcher *dispatcher)
{
	int i, idx;

	idx = irq - cfg->irq_offset;

	/* Subtract interrupts handled by previous masters */
	for (i = dispatcher->master_index - 1; i >= 0; i--) {
		idx -= irqsteer_get_master_irq_count(cfg, i);
	}

	return irq_to_level_2(idx) | dispatcher->irq;
}

/**
 * @brief Convert master-relative INTID to system INTID
 */
static int to_system_irq(const struct irqsteer_config *cfg, uint32_t irq, uint32_t master_index)
{
	int i;

	/* Add interrupts handled by previous masters */
	for (i = master_index - 1; i >= 0; i--) {
		irq += irqsteer_get_master_irq_count(cfg, i);
	}

	return irq + cfg->irq_offset;
}

/**
 * @brief Convert level 2 INTID to system INTID
 */
static int from_level2_irq(const struct irqsteer_config *cfg,
			   uint32_t level2_irq, uint32_t master_index)
{
	int i, idx;

	idx = level2_irq;

	for (i = 0; i < master_index; i++) {
		idx += irqsteer_get_master_irq_count(cfg, i);
	}

	return idx + cfg->irq_offset;
}

/* note: if disp != NULL that means there's several level2 interrupts
 * multiplexed into this single level 1 line via irqsteer. In such cases,
 * the interrupt needs to also be enable/disabled at the irqsteer level.
 *
 * brief: disp == NULL => IRQ is not managed by IRQSTEER
 *        disp != NULL => IRQ is managed by IRQSTEER
 */
static void irqstr_l1_irq_enable_disable(uint32_t irq,
					 struct irqsteer_dispatcher *disp,
					 bool enable)
{
	const struct irqsteer_config *cfg;

	if (disp) {
		cfg = disp->dev->config;
	}

	if (enable) {
		irqstr_l1_irq_enable_raw(irq);

		if (disp) {
			irqsteer_enable_master_interrupt(cfg, disp->master_index, true);
		}
	} else {
		if (disp) {
			irqsteer_enable_master_interrupt(cfg, disp->master_index, false);
		}

		irqstr_l1_irq_disable_raw(irq);
	}
}

static void irqstr_request_l1_irq_unlocked(uint32_t irq,
					   struct irqsteer_dispatcher *disp)
{
	int ret;

#ifndef CONFIG_SHARED_INTERRUPTS
	if (l1_irq_refcnt[irq]) {
		LOG_WRN("L1 IRQ %d already requested", irq);
		return;
	}
#endif /* CONFIG_SHARED_INTERRUPTS */

	if (l1_irq_refcnt[irq] == UINT8_MAX) {
		LOG_WRN("L1 IRQ %d reference count reached limit", irq);
		return;
	}

	if (!l1_irq_refcnt[irq]) {
		if (disp) {
			ret = pm_device_runtime_get(disp->dev);
			if (ret < 0) {
				LOG_ERR("failed to enable PM resources: %d", ret);
				return;
			}
		}

		irqstr_l1_irq_enable_disable(irq, disp, true);
	}

	l1_irq_refcnt[irq]++;

	LOG_DBG("request for L1 IRQ %d results in refcnt: %d",
		irq, l1_irq_refcnt[irq]);
}

static void irqstr_release_l1_irq_unlocked(uint32_t irq,
					   struct irqsteer_dispatcher *disp)
{
	int ret;

	if (!l1_irq_refcnt[irq]) {
		LOG_WRN("L1 IRQ %d already released", irq);
		return;
	}

	l1_irq_refcnt[irq]--;

	if (!l1_irq_refcnt[irq]) {
		irqstr_l1_irq_enable_disable(irq, disp, false);

		if (disp) {
			ret = pm_device_runtime_put(disp->dev);
			if (ret < 0) {
				LOG_ERR("failed to disable PM resources: %d", ret);
				return;
			}
		}
	}

	LOG_DBG("release on L1 IRQ %d results in refcnt: %d",
		irq, l1_irq_refcnt[irq]);
}

static void _irqstr_enable_disable_irq(struct irqsteer_dispatcher *disp,
				       uint32_t system_irq, bool enable)
{
	const struct irqsteer_config *cfg = disp->dev->config;

	if (enable) {
		irqsteer_enable_interrupt(cfg, system_irq, true);
	} else {
		irqsteer_enable_interrupt(cfg, system_irq, false);
	}
}

static void irqstr_request_irq_unlocked(struct irqsteer_dispatcher *disp,
					uint32_t l2_irq)
{
	const struct irqsteer_config *cfg = disp->dev->config;
	uint32_t system_irq = from_level2_irq(cfg, l2_irq, disp->master_index);
	int irqsteer_master_irq_off = (system_irq - cfg->irq_offset) % IRQSTEER_INT_SRC_REG_WIDTH;

#ifndef CONFIG_SHARED_INTERRUPTS
	if (disp->irq_refcnt[irqsteer_master_irq_off]) {
		LOG_WRN("irq %d already requested", system_irq);
		return;
	}
#endif /* CONFIG_SHARED_INTERRUPTS */

	if (disp->irq_refcnt[irqsteer_master_irq_off] == UINT8_MAX) {
		LOG_WRN("irq %d reference count reached limit", system_irq);
		return;
	}

	if (!disp->irq_refcnt[irqsteer_master_irq_off]) {
		irqstr_request_l1_irq_unlocked(disp->irq, disp);
		_irqstr_enable_disable_irq(disp, system_irq, true);
	}

	disp->irq_refcnt[irqsteer_master_irq_off]++;
	LOG_DBG("requested irq %d has refcount %d, irqsteer_master_irq_off = %d",
		system_irq, disp->irq_refcnt[irqsteer_master_irq_off], irqsteer_master_irq_off);
}

static void irqstr_release_irq_unlocked(struct irqsteer_dispatcher *disp,
					uint32_t l2_irq)
{
	const struct irqsteer_config *cfg = disp->dev->config;
	uint32_t system_irq = from_level2_irq(cfg, l2_irq, disp->master_index);
	int irqsteer_master_irq_off = (system_irq - cfg->irq_offset) % IRQSTEER_INT_SRC_REG_WIDTH;

	if (!disp->irq_refcnt[irqsteer_master_irq_off]) {
		LOG_WRN("irq %d already released", system_irq);
		return;
	}

	disp->irq_refcnt[irqsteer_master_irq_off]--;
	if (!disp->irq_refcnt[irqsteer_master_irq_off]) {
		_irqstr_enable_disable_irq(disp, system_irq, false);
		irqstr_release_l1_irq_unlocked(disp->irq, disp);
	}

	LOG_DBG("released irq %d has refcount %d",
		system_irq, disp->irq_refcnt[irqsteer_master_irq_off]);
}

/* Zephyr SoC interrupt interface */
void z_soc_irq_enable_disable(uint32_t irq, bool enable)
{
	uint32_t parent_irq;
	int i, level2_irq;

	if (irq_get_level(irq) == 1) {
		/* LEVEL 1 interrupts are direct */
		K_SPINLOCK(&irqstr_lock) {
			if (enable) {
				irqstr_request_l1_irq_unlocked(irq, NULL);
			} else {
				irqstr_release_l1_irq_unlocked(irq, NULL);
			}
		}
		return;
	}

	parent_irq = irq_parent_level_2(irq);
	level2_irq = irq_from_level_2(irq);

	/* find dispatcher responsible for this interrupt */
	for (i = 0; i < ARRAY_SIZE(dispatchers); i++) {
		if (dispatchers[i].irq != parent_irq) {
			continue;
		}

		K_SPINLOCK(&irqstr_lock) {
			if (enable) {
				irqstr_request_irq_unlocked(&dispatchers[i], level2_irq);
			} else {
				irqstr_release_irq_unlocked(&dispatchers[i], level2_irq);
			}
		}

		return;
	}
}

void z_soc_irq_enable(uint32_t irq)
{
	z_soc_irq_enable_disable(irq, true);
}

void z_soc_irq_disable(uint32_t irq)
{
	z_soc_irq_enable_disable(irq, false);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	uint32_t parent_irq;
	int i;
	bool enabled = false;

	if (irq_get_level(irq) == 1) {
		K_SPINLOCK(&irqstr_lock) {
			enabled = l1_irq_refcnt[irq];
		}
		return enabled;
	}

	parent_irq = irq_parent_level_2(irq);

	/* Find dispatcher responsible for this interrupt */
	for (i = 0; i < ARRAY_SIZE(dispatchers); i++) {
		if (dispatchers[i].irq != parent_irq) {
			continue;
		}

		K_SPINLOCK(&irqstr_lock) {
			enabled = dispatchers[i].irq_refcnt[irq_from_level_2(irq)];
		}
		return enabled;
	}

	return false;
}

#if defined(CONFIG_ARM)
void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	uint32_t level1_irq = irq;

	if (irq_get_level(irq) != 1) {
		level1_irq = irq_parent_level_2(irq);
	}

	arm_irq_priority_set(level1_irq, prio, flags);
}
#endif

/* ISR dispatcher */
static void irqsteer_isr_dispatcher(const void *data)
{
	struct irqsteer_dispatcher *dispatcher;
	const struct irqsteer_config *cfg;
	uint32_t table_idx;
	int system_irq, zephyr_irq, i;
	uint64_t status;

	dispatcher = (struct irqsteer_dispatcher *)data;
	cfg = dispatcher->dev->config;

	/* Fetch master interrupts status */
	status = irqsteer_get_master_interrupts_status(cfg, dispatcher->master_index);

	for (i = 0; status; i++) {
		/* If bit 0 is set then that means relative INTID i is asserted */
		if (status & 1) {
			/* Convert master-relative INTID to a system INTID */
			system_irq = to_system_irq(cfg, i, dispatcher->master_index);

			/* Convert system INTID to a Zephyr INTID */
			zephyr_irq = to_zephyr_irq(cfg, system_irq, dispatcher);

			/* Compute index in the SW ISR table */
			table_idx = z_get_sw_isr_table_idx(zephyr_irq);

			/* Call child's ISR */
			_sw_isr_table[table_idx].isr(_sw_isr_table[table_idx].arg);
		}
		status >>= 1;
	}
}

__maybe_unused static int irqsteer_pm_action(const struct device *dev,
					     enum pm_device_action action)
{
	/* nothing to be done here */
	return 0;
}

#define NXP_IRQSTEER_CONFIG_DEFINE(n) \
	static struct irqsteer_dispatcher dispatchers_##n[] = { \
		IRQSTEER_DECLARE_DISPATCHERS(DT_DRV_INST(n)) \
	}; \
	static struct irqsteer_config irqsteer_config_##n = { \
		.regmap_phys = DT_INST_REG_ADDR(n), \
		.regmap_size = DT_INST_REG_SIZE(n), \
		.irq_offset = DT_INST_PROP_OR(n, nxp_irq_offset, 0), \
		.irqs_num = DT_INST_PROP(n, nxp_num_irqs), \
		.num_masters = DIV_ROUND_UP(DT_INST_PROP(n, nxp_num_irqs), 64), \
		.reg_num = DT_INST_PROP(n, nxp_num_irqs) / 32, \
		.dispatchers = dispatchers_##n, \
	}; \
	static int irqsteer_init_##n(const struct device *dev) \
	{ \
		const struct irqsteer_config *cfg = dev->config; \
		IRQSTEER_REGISTER_DISPATCHERS(DT_DRV_INST(n)); \
		irqsteer_hw_init(cfg); \
		return pm_device_runtime_enable(dev); \
	}

#define CONCAT4(a, b, c, d) UTIL_CAT(UTIL_CAT(UTIL_CAT(a, b), c), d)

#define NXP_IRQSTEER_MASTER_IRQ_ENTRY_DEF(node_id)                                                 \
	IRQ_PARENT_ENTRY_DEFINE(CONCAT4(nxp_irqsteer_,						   \
				DT_REG_ADDR(DT_PARENT(node_id)),				   \
				_master_,							   \
				DT_NODE_CHILD_IDX(node_id)),					   \
				NULL,								   \
				DT_IRQN(node_id), INTC_CHILD_ISR_TBL_OFFSET(node_id),              \
				DT_INTC_GET_AGGREGATOR_LEVEL(node_id));

#define NXP_IRQSTEER_IRQ_PARENT_ENTRIES_DEF(n) \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n,   \
	NXP_IRQSTEER_MASTER_IRQ_ENTRY_DEF);

#define NXP_IRQSTEER_DEVICE_DEFINE(n) \
	PM_DEVICE_DT_INST_DEFINE(n, irqsteer_pm_action); \
	DEVICE_DT_INST_DEFINE(n, \
			      irqsteer_init_##n, \
			      PM_DEVICE_DT_INST_GET(n), \
			      NULL, \
			      &irqsteer_config_##n, \
			      PRE_KERNEL_1, \
			      CONFIG_INTC_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(NXP_IRQSTEER_CONFIG_DEFINE);
DT_INST_FOREACH_STATUS_OKAY(NXP_IRQSTEER_DEVICE_DEFINE);
DT_INST_FOREACH_STATUS_OKAY(NXP_IRQSTEER_IRQ_PARENT_ENTRIES_DEF);

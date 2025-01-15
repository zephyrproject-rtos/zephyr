/*
 * Copyright 2023 NXP
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
#include <fsl_irqsteer.h>
#include <zephyr/cache.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device.h>

#include "sw_isr_common.h"

LOG_MODULE_REGISTER(nxp_irqstr);

/* used for driver binding */
#define DT_DRV_COMPAT nxp_irqsteer_intc

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

/* utility macros */
#define UINT_TO_IRQSTEER(x) ((IRQSTEER_Type *)(x))
#define DISPATCHER_REGMAP(disp) \
	(((const struct irqsteer_config *)disp->dev->config)->regmap_phys)

#if defined(CONFIG_XTENSA)
#define irqsteer_level1_irq_enable(irq)     xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq))
#define irqsteer_level1_irq_disable(irq)    xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq))
#define irqsteer_level1_irq_is_enabled(irq) xtensa_irq_is_enabled(XTENSA_IRQ_NUMBER(irq))
#elif defined(CONFIG_ARM)
#define irqsteer_level1_irq_enable(irq)     arm_irq_enable(irq)
#define irqsteer_level1_irq_disable(irq)    arm_irq_disable(irq)
#define irqsteer_level1_irq_is_enabled(irq) arm_irq_is_enabled(irq)
#else
#error ARCH not supported
#endif

struct irqsteer_config {
	uint32_t regmap_phys;
	uint32_t regmap_size;
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
	/* dispatcher lock */
	struct k_spinlock lock;
	/* reference count for dispatcher */
	uint8_t refcnt;
};

static struct irqsteer_dispatcher dispatchers[] = {
	IRQSTEER_DECLARE_DISPATCHERS(DT_NODELABEL(irqsteer))
};

/* used to convert system INTID to zephyr INTID */
static int to_zephyr_irq(uint32_t regmap, uint32_t irq,
			 struct irqsteer_dispatcher *dispatcher)
{
	int i, idx;

	idx = irq - FSL_FEATURE_IRQSTEER_IRQ_START_INDEX;

	for (i = dispatcher->master_index - 1; i >= 0; i--) {
		idx -= IRQSTEER_GetMasterIrqCount(UINT_TO_IRQSTEER(regmap), i);
	}

	return irq_to_level_2(idx) | dispatcher->irq;
}

/* used to convert master-relative INTID to system INTID */
static int to_system_irq(uint32_t regmap, int irq, int master_index)
{
	int i;

	for (i = master_index - 1; i >= 0; i--) {
		irq += IRQSTEER_GetMasterIrqCount(UINT_TO_IRQSTEER(regmap), i);
	}

	return irq + FSL_FEATURE_IRQSTEER_IRQ_START_INDEX;
}

/* used to convert zephyr INTID (level 2) to system INTID */
static int from_zephyr_irq(uint32_t regmap, uint32_t irq, uint32_t master_index)
{
	int i, idx;

	idx = irq;

	for (i = 0; i < master_index; i++) {
		idx += IRQSTEER_GetMasterIrqCount(UINT_TO_IRQSTEER(regmap), i);
	}

	return idx + FSL_FEATURE_IRQSTEER_IRQ_START_INDEX;
}

static void _irqstr_disp_enable_disable(struct irqsteer_dispatcher *disp,
					bool enable)
{
	uint32_t regmap = DISPATCHER_REGMAP(disp);

	if (enable) {
		irqsteer_level1_irq_enable(disp->irq);
		IRQSTEER_EnableMasterInterrupt(UINT_TO_IRQSTEER(regmap), disp->irq);
	} else {
		IRQSTEER_DisableMasterInterrupt(UINT_TO_IRQSTEER(regmap), disp->irq);
		irqsteer_level1_irq_disable(disp->irq);
	}
}

static void _irqstr_disp_get_unlocked(struct irqsteer_dispatcher *disp)
{
	int ret;

	if (disp->refcnt == UINT8_MAX) {
		LOG_WRN("disp for irq %d reference count reached limit", disp->irq);
		return;
	}

	if (!disp->refcnt) {
		ret = pm_device_runtime_get(disp->dev);
		if (ret < 0) {
			LOG_ERR("failed to enable PM resources: %d", ret);
			return;
		}

		_irqstr_disp_enable_disable(disp, true);
	}

	disp->refcnt++;

	LOG_DBG("get on disp for irq %d results in refcnt: %d",
		disp->irq, disp->refcnt);
}

static void _irqstr_disp_put_unlocked(struct irqsteer_dispatcher *disp)
{
	int ret;

	if (!disp->refcnt) {
		LOG_WRN("disp for irq %d already put", disp->irq);
		return;
	}

	disp->refcnt--;

	if (!disp->refcnt) {
		_irqstr_disp_enable_disable(disp, false);

		ret = pm_device_runtime_put(disp->dev);
		if (ret < 0) {
			LOG_ERR("failed to disable PM resources: %d", ret);
			return;
		}
	}

	LOG_DBG("put on disp for irq %d results in refcnt: %d",
		disp->irq, disp->refcnt);
}

static void _irqstr_enable_disable_irq(struct irqsteer_dispatcher *disp,
				       uint32_t system_irq, bool enable)
{
	uint32_t regmap = DISPATCHER_REGMAP(disp);

	if (enable) {
		IRQSTEER_EnableInterrupt(UINT_TO_IRQSTEER(regmap), system_irq);
	} else {
		IRQSTEER_DisableInterrupt(UINT_TO_IRQSTEER(regmap), system_irq);
	}
}

static void irqstr_request_irq_unlocked(struct irqsteer_dispatcher *disp,
					uint32_t zephyr_irq)
{
	uint32_t system_irq = from_zephyr_irq(DISPATCHER_REGMAP(disp),
					      zephyr_irq, disp->master_index);

#ifndef CONFIG_SHARED_INTERRUPTS
	if (disp->irq_refcnt[zephyr_irq]) {
		LOG_WRN("irq %d already requested", system_irq);
		return;
	}
#endif /* CONFIG_SHARED_INTERRUPTS */

	if (disp->irq_refcnt[zephyr_irq] == UINT8_MAX) {
		LOG_WRN("irq %d reference count reached limit", system_irq);
		return;
	}

	if (!disp->irq_refcnt[zephyr_irq]) {
		_irqstr_disp_get_unlocked(disp);
		_irqstr_enable_disable_irq(disp, system_irq, true);
	}

	disp->irq_refcnt[zephyr_irq]++;

	LOG_DBG("requested irq %d has refcount %d",
		system_irq, disp->irq_refcnt[zephyr_irq]);
}

static void irqstr_release_irq_unlocked(struct irqsteer_dispatcher *disp,
					uint32_t zephyr_irq)
{
	uint32_t system_irq = from_zephyr_irq(DISPATCHER_REGMAP(disp),
					      zephyr_irq, disp->master_index);

	if (!disp->irq_refcnt[zephyr_irq]) {
		LOG_WRN("irq %d already released", system_irq);
		return;
	}

	disp->irq_refcnt[zephyr_irq]--;

	if (!disp->irq_refcnt[zephyr_irq]) {
		_irqstr_enable_disable_irq(disp, system_irq, false);
		_irqstr_disp_put_unlocked(disp);
	}

	LOG_DBG("released irq %d has refcount %d",
		system_irq, disp->irq_refcnt[zephyr_irq]);
}

void z_soc_irq_enable_disable(uint32_t irq, bool enable)
{
	uint32_t parent_irq;
	int i, level2_irq;

	if (irq_get_level(irq) == 1) {
		/* LEVEL 1 interrupts are DSP direct */
		if (enable) {
			irqsteer_level1_irq_enable(irq);
		} else {
			irqsteer_level1_irq_disable(irq);
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

		K_SPINLOCK(&dispatchers[i].lock) {
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
	const struct irqsteer_config *cfg;
	bool enabled;

	if (irq_get_level(irq) == 1) {
		return irqsteer_level1_irq_is_enabled(irq);
	}

	parent_irq = irq_parent_level_2(irq);
	enabled = false;

	/* find dispatcher responsible for this interrupt */
	for (i = 0; i < ARRAY_SIZE(dispatchers); i++) {
		if (dispatchers[i].irq != parent_irq) {
			continue;
		}

		cfg = dispatchers[i].dev->config;

		K_SPINLOCK(&dispatchers[i].lock) {
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

static void irqsteer_isr_dispatcher(const void *data)
{
	struct irqsteer_dispatcher *dispatcher;
	const struct irqsteer_config *cfg;
	uint32_t table_idx;
	int system_irq, zephyr_irq, i;
	uint64_t status;

	dispatcher = (struct irqsteer_dispatcher *)data;
	cfg = dispatcher->dev->config;

	/* fetch master interrupts status */
	status = IRQSTEER_GetMasterInterruptsStatus(UINT_TO_IRQSTEER(cfg->regmap_phys),
						    dispatcher->master_index);

	for (i = 0; status; i++) {
		/* if bit 0 is set then that means relative INTID i is asserted */
		if (status & 1) {
			/* convert master-relative INTID to a system INTID */
			system_irq = to_system_irq(cfg->regmap_phys, i,
						   dispatcher->master_index);

			/* convert system INTID to a Zephyr INTID */
			zephyr_irq = to_zephyr_irq(cfg->regmap_phys, system_irq, dispatcher);

			/* compute index in the SW ISR table */
			table_idx = z_get_sw_isr_table_idx(zephyr_irq);

			/* call child's ISR */
			_sw_isr_table[table_idx].isr(_sw_isr_table[table_idx].arg);
		}

		status >>= 1;
	}
}

__maybe_unused static int irqstr_pm_action(const struct device *dev,
					   enum pm_device_action action)
{
	/* nothing to be done here */
	return 0;
}

static int irqsteer_init(const struct device *dev)
{
	IRQSTEER_REGISTER_DISPATCHERS(DT_NODELABEL(irqsteer));

	return pm_device_runtime_enable(dev);
}


/* TODO: do we need to add support for MMU-based SoCs? */
static struct irqsteer_config irqsteer_config = {
	.regmap_phys = DT_REG_ADDR(DT_NODELABEL(irqsteer)),
	.regmap_size = DT_REG_SIZE(DT_NODELABEL(irqsteer)),
	.dispatchers = dispatchers,
};

/* assumption: only 1 IRQ_STEER instance */
PM_DEVICE_DT_INST_DEFINE(0, irqstr_pm_action);
DEVICE_DT_INST_DEFINE(0,
		      &irqsteer_init,
		      PM_DEVICE_DT_INST_GET(0),
		      NULL, &irqsteer_config,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		      NULL);

#define NXP_IRQSTEER_MASTER_IRQ_ENTRY_DEF(node_id)                                                 \
	IRQ_PARENT_ENTRY_DEFINE(CONCAT(nxp_irqsteer_master_, DT_NODE_CHILD_IDX(node_id)), NULL,    \
				DT_IRQN(node_id), INTC_CHILD_ISR_TBL_OFFSET(node_id),              \
				DT_INTC_GET_AGGREGATOR_LEVEL(node_id));

DT_INST_FOREACH_CHILD_STATUS_OKAY(0, NXP_IRQSTEER_MASTER_IRQ_ENTRY_DEF);

/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt controller in Microchip XEC devices
 *
 * Driver is currently implemented to support MEC172x ECIA GIRQs
 */

#define DT_DRV_COMPAT microchip_xec_ecia

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>

/* defined at the SoC layer */
#define MCHP_FIRST_GIRQ			MCHP_FIRST_GIRQ_NOS
#define MCHP_LAST_GIRQ			MCHP_LAST_GIRQ_NOS
#define MCHP_XEC_DIRECT_CAPABLE		MCHP_ECIA_DIRECT_BITMAP

#define GIRQ_ID_TO_BITPOS(id) ((id) + 8)

/*
 * MEC SoC's have one and only one instance of ECIA. GIRQ8 register are located
 * at the beginning of the ECIA block.
 */
#define ECIA_XEC_REG_BASE						\
	((struct ecia_regs *)(DT_REG_ADDR(DT_NODELABEL(ecia))))

#define ECS_XEC_REG_BASE						\
	((struct ecs_regs *)(DT_REG_ADDR(DT_NODELABEL(ecs))))

#define PCR_XEC_REG_BASE						\
	((struct pcr_regs *)(DT_REG_ADDR(DT_NODELABEL(pcr))))

#define ECIA_XEC_PCR_REG_IDX	DT_INST_CLOCKS_CELL(0, regidx)
#define ECIA_XEC_PCR_BITPOS	DT_INST_CLOCKS_CELL(0, bitpos)

#define ECIA_XEC_PCR_INFO						\
	MCHP_XEC_PCR_SCR_ENCODE(DT_INST_CLOCKS_CELL(0, regidx),		\
				DT_INST_CLOCKS_CELL(0, bitpos))

struct xec_girq_config {
	uintptr_t base;
	uint8_t girq_id;
	uint8_t num_srcs;
	uint8_t sources[32];
};

struct xec_ecia_config {
	uintptr_t ecia_base;
	struct mchp_xec_pcr_clk_ctrl clk_ctrl;
	const struct device *girq_node_handles[32];
};

struct xec_girq_src_data {
	mchp_xec_ecia_callback_t cb;
	void *data;
};

#define DEV_ECIA_CFG(ecia_dev) \
	((const struct xec_ecia_config *const)(ecia_dev)->config)

#define DEV_GIRQ_CFG(girq_dev) \
	((const struct xec_girq_config *const)(girq_dev)->config)

#define DEV_GIRQ_DATA(girq_dev) \
	((struct xec_girq_src_data *const)(girq_dev)->data)

/*
 * Enable/disable specified GIRQ's aggregated output. Aggregated output is the
 * bit-wise or of all the GIRQ's result bits.
 */
void mchp_xec_ecia_girq_aggr_en(uint8_t girq_num, uint8_t enable)
{
	struct ecia_regs *regs = ECIA_XEC_REG_BASE;

	if (enable) {
		regs->BLK_EN_SET = BIT(girq_num);
	} else {
		regs->BLK_EN_CLR = BIT(girq_num);
	}
}

void mchp_xec_ecia_girq_src_clr(uint8_t girq_num, uint8_t src_bit_pos)
{
	if ((girq_num < MCHP_FIRST_GIRQ) || (girq_num > MCHP_LAST_GIRQ)) {
		return;
	}

	struct ecia_regs *regs = ECIA_XEC_REG_BASE;

	/* write 1 to clear */
	regs->GIRQ[girq_num - MCHP_FIRST_GIRQ].SRC = BIT(src_bit_pos);
}

void mchp_xec_ecia_girq_src_en(uint8_t girq_num, uint8_t src_bit_pos)
{
	if ((girq_num < MCHP_FIRST_GIRQ) || (girq_num > MCHP_LAST_GIRQ)) {
		return;
	}

	struct ecia_regs *regs = ECIA_XEC_REG_BASE;

	/* write 1 to set */
	regs->GIRQ[girq_num - MCHP_FIRST_GIRQ].EN_SET = BIT(src_bit_pos);
}

void mchp_xec_ecia_girq_src_dis(uint8_t girq_num, uint8_t src_bit_pos)
{
	if ((girq_num < MCHP_FIRST_GIRQ) || (girq_num > MCHP_LAST_GIRQ)) {
		return;
	}

	struct ecia_regs *regs = ECIA_XEC_REG_BASE;

	/* write 1 to clear */
	regs->GIRQ[girq_num - MCHP_FIRST_GIRQ].EN_CLR = BIT(src_bit_pos);
}

void mchp_xec_ecia_girq_src_clr_bitmap(uint8_t girq_num, uint32_t bitmap)
{
	if ((girq_num < MCHP_FIRST_GIRQ) || (girq_num > MCHP_LAST_GIRQ)) {
		return;
	}

	struct ecia_regs *regs = ECIA_XEC_REG_BASE;

	/* write 1 to clear */
	regs->GIRQ[girq_num - MCHP_FIRST_GIRQ].SRC = bitmap;
}

void mchp_xec_ecia_girq_src_en_bitmap(uint8_t girq_num, uint32_t bitmap)
{
	if ((girq_num < MCHP_FIRST_GIRQ) || (girq_num > MCHP_LAST_GIRQ)) {
		return;
	}

	struct ecia_regs *regs = ECIA_XEC_REG_BASE;

	/* write 1 to clear */
	regs->GIRQ[girq_num - MCHP_FIRST_GIRQ].EN_SET = bitmap;
}

void mchp_xec_ecia_girq_src_dis_bitmap(uint8_t girq_num, uint32_t bitmap)
{
	if ((girq_num < MCHP_FIRST_GIRQ) || (girq_num > MCHP_LAST_GIRQ)) {
		return;
	}

	struct ecia_regs *regs = ECIA_XEC_REG_BASE;

	/* write 1 to clear */
	regs->GIRQ[girq_num - MCHP_FIRST_GIRQ].EN_CLR = bitmap;
}

/*
 * Return read-only GIRQ result register. Result is bit-wise and of source
 * and enable registers.
 */
uint32_t mchp_xec_ecia_girq_result(uint8_t girq_num)
{
	if ((girq_num < MCHP_FIRST_GIRQ) || (girq_num > MCHP_LAST_GIRQ)) {
		return 0U;
	}

	struct ecia_regs *regs = ECIA_XEC_REG_BASE;

	return regs->GIRQ[girq_num - MCHP_FIRST_GIRQ].RESULT;
}

/* Clear NVIC pending given the external NVIC input number (zero based) */
void mchp_xec_ecia_nvic_clr_pend(uint32_t nvic_num)
{
	if (nvic_num >= ((SCnSCB->ICTR + 1) * 32)) {
		return;
	}

	NVIC_ClearPendingIRQ(nvic_num);
}

/* API taking input encoded with MCHP_XEC_ECIA(g, gb, na, nd) macro */

void mchp_xec_ecia_info_girq_aggr_en(int ecia_info, uint8_t enable)
{
	uint8_t girq_num = MCHP_XEC_ECIA_GIRQ(ecia_info);

	mchp_xec_ecia_girq_aggr_en(girq_num, enable);
}

void mchp_xec_ecia_info_girq_src_clr(int ecia_info)
{
	uint8_t girq_num = MCHP_XEC_ECIA_GIRQ(ecia_info);
	uint8_t bitpos = MCHP_XEC_ECIA_GIRQ_POS(ecia_info);

	mchp_xec_ecia_girq_src_clr(girq_num, bitpos);
}

void mchp_xec_ecia_info_girq_src_en(int ecia_info)
{
	uint8_t girq_num = MCHP_XEC_ECIA_GIRQ(ecia_info);
	uint8_t bitpos = MCHP_XEC_ECIA_GIRQ_POS(ecia_info);

	mchp_xec_ecia_girq_src_en(girq_num, bitpos);
}

void mchp_xec_ecia_info_girq_src_dis(int ecia_info)
{
	uint8_t girq_num = MCHP_XEC_ECIA_GIRQ(ecia_info);
	uint8_t bitpos = MCHP_XEC_ECIA_GIRQ_POS(ecia_info);

	mchp_xec_ecia_girq_src_dis(girq_num, bitpos);
}

uint32_t mchp_xec_ecia_info_girq_result(int ecia_info)
{
	uint8_t girq_num = MCHP_XEC_ECIA_GIRQ(ecia_info);

	return mchp_xec_ecia_girq_result(girq_num);
}

/*
 * Clear NVIC pending status given GIRQ source information encoded by macro
 * MCHP_XEC_ECIA. For aggregated only sources the encoding sets direct NVIC
 * number equal to aggregated NVIC number.
 */
void mchp_xec_ecia_info_nvic_clr_pend(int ecia_info)
{
	uint8_t nvic_num = MCHP_XEC_ECIA_NVIC_DIRECT(ecia_info);

	mchp_xec_ecia_nvic_clr_pend(nvic_num);
}

/**
 * @brief enable GIRQn interrupt for specific source
 *
 * @param girq is the GIRQ number (8 - 26)
 * @param src is the interrupt source in the GIRQ (0 - 31)
 */
int mchp_xec_ecia_enable(int girq, int src)
{
	if ((girq < MCHP_FIRST_GIRQ) || (girq > MCHP_LAST_GIRQ) ||
	    (src < 0) || (src > 31)) {
		return -EINVAL;
	}

	struct ecia_regs *regs = ECIA_XEC_REG_BASE;

	/* write 1 to set */
	regs->GIRQ[girq - MCHP_FIRST_GIRQ].EN_SET = BIT(src);

	return 0;
}

/**
 * @brief enable EXTI interrupt for specific line specified by parameter
 * encoded with MCHP_XEC_ECIA macro.
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 */
int mchp_xec_ecia_info_enable(int ecia_info)
{
	uint8_t girq = (uint8_t)MCHP_XEC_ECIA_GIRQ(ecia_info);
	uint8_t src = (uint8_t)MCHP_XEC_ECIA_GIRQ_POS(ecia_info);

	return mchp_xec_ecia_enable(girq, src);
}

/**
 * @brief disable EXTI interrupt for specific line
 *
 * @param girq is the GIRQ number (8 - 26)
 * @param src is the interrupt source in the GIRQ (0 - 31)
 */
int mchp_xec_ecia_disable(int girq, int src)
{
	if ((girq < MCHP_FIRST_GIRQ) || (girq > MCHP_LAST_GIRQ) ||
	    (src < 0) || (src > 31)) {
		return -EINVAL;
	}

	struct ecia_regs *regs = ECIA_XEC_REG_BASE;

	/* write 1 to clear */
	regs->GIRQ[girq - MCHP_FIRST_GIRQ].EN_CLR = BIT(src);

	return 0;
}

/**
 * @brief disable EXTI interrupt for specific line specified by parameter
 * encoded with MCHP_XEC_ECIA macro.
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 */
int mchp_xec_ecia_info_disable(int ecia_info)
{
	uint8_t girq = (uint8_t)MCHP_XEC_ECIA_GIRQ(ecia_info);
	uint8_t src = (uint8_t)MCHP_XEC_ECIA_GIRQ_POS(ecia_info);

	return mchp_xec_ecia_disable(girq, src);
}

/* forward reference */
static const struct device *get_girq_dev(int girq_num);

/**
 * @brief set GIRQn interrupt source callback
 *
 * @param dev_girq is the GIRQn device handle
 * @param src is the interrupt source in the GIRQ (0 - 31)
 * @param cb user callback
 * @param data user data
 */
int mchp_xec_ecia_set_callback_by_dev(const struct device *dev_girq, int src,
				      mchp_xec_ecia_callback_t cb, void *data)
{
	if ((dev_girq == NULL) || (src < 0) || (src > 31)) {
		return -EINVAL;
	}

	const struct xec_girq_config *const cfg = DEV_GIRQ_CFG(dev_girq);
	struct xec_girq_src_data *girq_data = DEV_GIRQ_DATA(dev_girq);

	/* source exists in this GIRQ? */
	if (!(cfg->sources[src] & BIT(7))) {
		return -EINVAL;
	}

	/* obtain the callback array index for the source */
	int idx = (int)(cfg->sources[src] & ~BIT(7));

	girq_data[idx].cb = cb;
	girq_data[idx].data = data;

	return 0;
}

/**
 * @brief set GIRQn interrupt source callback
 *
 * @param girq is the GIRQ number (8 - 26)
 * @param src is the interrupt source in the GIRQ (0 - 31)
 * @param cb user callback
 * @param data user data
 */
int mchp_xec_ecia_set_callback(int girq_num, int src,
			       mchp_xec_ecia_callback_t cb, void *data)
{
	const struct device *dev = get_girq_dev(girq_num);

	return mchp_xec_ecia_set_callback_by_dev(dev, src, cb, data);
}

/**
 * @brief set GIRQn interrupt source callback
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 * @param cb user callback
 * @param data user data
 */
int mchp_xec_ecia_info_set_callback(int ecia_info, mchp_xec_ecia_callback_t cb,
				    void *data)
{
	const struct device *dev = get_girq_dev(MCHP_XEC_ECIA_GIRQ(ecia_info));
	uint8_t src = MCHP_XEC_ECIA_GIRQ_POS(ecia_info);

	return mchp_xec_ecia_set_callback_by_dev(dev, src, cb, data);
}

/**
 * @brief unset GIRQn interrupt source callback by device handle
 *
 * @param dev_girq is the GIRQn device handle
 * @param src is the interrupt source in the GIRQ (0 - 31)
 */
int mchp_ecia_unset_callback_by_dev(const struct device *dev_girq, int src)
{
	if ((dev_girq == NULL) || (src < 0) || (src > 31)) {
		return -EINVAL;
	}

	const struct xec_girq_config *const cfg = DEV_GIRQ_CFG(dev_girq);
	struct xec_girq_src_data *girq_data = DEV_GIRQ_DATA(dev_girq);

	/* source exists in this GIRQ? */
	if (!(cfg->sources[src] & BIT(7))) {
		return -EINVAL;
	}

	/* obtain the callback array index for the source */
	int idx = (int)(cfg->sources[src] & ~BIT(7));

	girq_data[idx].cb = NULL;
	girq_data[idx].data = NULL;

	return 0;
}

/**
 * @brief unset GIRQn interrupt source callback
 *
 * @param girq is the GIRQ number (8 - 26)
 * @param src is the interrupt source in the GIRQ (0 - 31)
 */
int mchp_ecia_unset_callback(int girq_num, int src)
{
	const struct device *dev = get_girq_dev(girq_num);

	return mchp_ecia_unset_callback_by_dev(dev, src);
}

/**
 * @brief unset GIRQn interrupt source callback
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 */
int mchp_ecia_info_unset_callback(int ecia_info)
{
	const struct device *dev = get_girq_dev(MCHP_XEC_ECIA_GIRQ(ecia_info));
	uint8_t src = MCHP_XEC_ECIA_GIRQ_POS(ecia_info);

	return mchp_ecia_unset_callback_by_dev(dev, src);
}


/*
 * Create a build time flag to know if any aggregated GIRQ has been enabled.
 * We make use of DT FOREACH macro to check GIRQ node status.
 * Enabling a GIRQ node (status = "okay") implies you want it used in
 * aggregated mode. Note, GIRQ 8-12, 24-26 are aggregated only by HW design.
 * If a GIRQ node is disabled(status = "disabled") and is direct capable the
 * other driver/application may use IRQ_CONNECT, irq_enable, and the helper
 * functions in this driver to set/clear GIRQ enable bits and status.
 * Leaving a node disabled also allows another driver/application to take over
 * aggregation by managing the GIRQ itself.
 */
#define XEC_CHK_REQ_AGGR(n) DT_NODE_HAS_STATUS(n, okay) |

#define XEC_ECIA_REQUIRE_AGGR_ISR					\
	(								\
	DT_FOREACH_CHILD(DT_NODELABEL(ecia), XEC_CHK_REQ_AGGR) \
	0)

/* static const uint32_t xec_chk_req = (XEC_ECIA_REQUIRE_AGGR_ISR); */

#if XEC_ECIA_REQUIRE_AGGR_ISR
/*
 * Generic ISR for aggregated GIRQ's.
 * GIRQ source(status) bits are latched (R/W1C). The peripheral status
 * connected to the GIRQ source bit must be cleared first by the callback
 * and this routine will clear the GIRQ source bit. If a callback was not
 * registered for a source the enable will also be cleared to prevent
 * interrupt storms.
 * NOTE: dev_girq is a pointer to a GIRQ child device instance.
 */
static void xec_girq_isr(const struct device *dev_girq)
{
	const struct xec_girq_config *const cfg = DEV_GIRQ_CFG(dev_girq);
	struct xec_girq_src_data *data = DEV_GIRQ_DATA(dev_girq);
	struct girq_regs *girq = (struct girq_regs *)cfg->base;
	int girq_id = GIRQ_ID_TO_BITPOS(cfg->girq_id);
	uint32_t idx = 0;
	uint32_t result = girq->RESULT;

	for (int i = 0; result && i < 32; i++) {
		uint8_t bitpos = 31 - (__builtin_clz(result) & 0x1f);

		/* is it an implemented source? */
		if (cfg->sources[bitpos] & BIT(7)) {
			/* yes, get the index by removing bit[7] flag */
			idx = (uint32_t)cfg->sources[bitpos] & ~BIT(7);
			/* callback registered? */
			if (data[idx].cb) {
				data[idx].cb(girq_id, bitpos, data[idx].data);
			} else { /* no callback, clear the enable */
				girq->EN_CLR = BIT(bitpos);
			}
		} else { /* paranoia, we should not get here... */
			girq->EN_CLR = BIT(bitpos);
		}

		/* clear GIRQ latched status */
		girq->SRC = BIT(bitpos);
		result &= ~BIT(bitpos);
	}
}
#endif

/**
 * @brief initialize XEC ECIA driver
 * NOTE: GIRQ22 is special used for waking the PLL from deep sleep when a
 * peripheral receives data from an external entity (eSPI, I2C, etc). Once
 * the data transfer is complete the system re-enters deep sleep unless the
 * peripheral was configured to wake CPU after reception of data or event.
 * GIRQ22 aggregated output and sources are not connected to the NVIC.
 * We enable GIRQ22 aggregated output to ensure clock asynchronous wake
 * functionality is operational.
 */
static int xec_ecia_init(const struct device *dev)
{
	const struct xec_ecia_config *cfg =
		(const struct xec_ecia_config *const) (dev->config);
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcr));
	struct ecs_regs *const ecs = ECS_XEC_REG_BASE;
	struct ecia_regs *const ecia = (struct ecia_regs *)cfg->ecia_base;
	uint32_t n = 0, nr = 0;
	int ret;

	ret = clock_control_on(clk_dev,
			       (clock_control_subsys_t *)&cfg->clk_ctrl);
	if (ret < 0) {
		return ret;
	}

	/* Enable all direct NVIC connections */
	ecs->INTR_CTRL |= BIT(0);

	/* gate off all aggregated outputs */
	ecia->BLK_EN_CLR = UINT32_MAX;

	/* connect aggregated only GIRQs to NVIC */
	ecia->BLK_EN_SET = MCHP_ECIA_AGGR_BITMAP;

	/* Clear all GIRQn source enables */
	for (n = 0; n < MCHP_GIRQS; n++) {
		ecia->GIRQ[n].EN_CLR = UINT32_MAX;
	}

	/* Clear all external NVIC enables and pending status */
	nr = SCnSCB->ICTR;
	for (n = 0u; n <= nr; n++) {
		NVIC->ICER[n] = UINT32_MAX;
		NVIC->ICPR[n] = UINT32_MAX;
	}

	/* ecia->BLK_ACTIVE = xec_chk_req; */

	return 0;
}

/* xec_config_girq_xxx.sources[] entries from GIRQ node */
#define XEC_GIRQ_SOURCES2(node_id, prop, idx)				\
	.sources[DT_PROP_BY_IDX(node_id, prop, idx)] =			\
		((uint8_t)(idx) | BIT(7)),

/* Parameter n is a child node-id */
#define GIRQ_XEC_DEVICE(n)						\
	static int xec_girq_init_##n(const struct device *dev);		\
									\
	static struct xec_girq_src_data					\
		xec_data_girq_##n[DT_PROP_LEN(n, sources)];		\
									\
	static const struct xec_girq_config xec_config_girq_##n = {	\
		.base = DT_REG_ADDR(n),					\
		.girq_id = DT_PROP(n, girq_id),				\
		.num_srcs = DT_PROP_LEN(n, sources),			\
		DT_FOREACH_PROP_ELEM(n, sources, XEC_GIRQ_SOURCES2)	\
	};								\
									\
	DEVICE_DT_DEFINE(n, xec_girq_init_##n,				\
		 NULL, &xec_data_girq_##n, &xec_config_girq_##n,	\
		 PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,		\
		 NULL);							\
									\
	static int xec_girq_init_##n(const struct device *dev)		\
	{								\
		mchp_xec_ecia_girq_aggr_en(				\
			GIRQ_ID_TO_BITPOS(DT_PROP(n, girq_id)), 1);	\
									\
		IRQ_CONNECT(DT_IRQN(n),					\
			    DT_IRQ(n, priority),			\
			    xec_girq_isr,				\
			    DEVICE_DT_GET(n), 0);			\
									\
		irq_enable(DT_IRQN(n));					\
									\
		return 0;						\
	}

/*
 * iterate over each enabled child node of ECIA
 * Enable means property status = "okay"
 */
DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(ecia), GIRQ_XEC_DEVICE)

/* n = GIRQ node id */
#define XEC_GIRQ_HANDLE(n)					\
	.girq_node_handles[DT_PROP(n, girq_id)] = (DEVICE_DT_GET(n)),

static const struct xec_ecia_config xec_config_ecia = {
	.ecia_base = DT_REG_ADDR(DT_NODELABEL(ecia)),
	.clk_ctrl = {
		.pcr_info = ECIA_XEC_PCR_INFO,
	},
	DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(ecia), XEC_GIRQ_HANDLE)
};

DEVICE_DT_DEFINE(DT_NODELABEL(ecia), xec_ecia_init,
		 NULL, NULL, &xec_config_ecia,
		 PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		 NULL);

/* look up GIRQ node handle from ECIA configuration */
static const struct device *get_girq_dev(int girq_num)
{
	if ((girq_num < MCHP_FIRST_GIRQ) || (girq_num > MCHP_LAST_GIRQ)) {
		return NULL;
	}

	/* safe to convert to zero based index */
	girq_num -= MCHP_FIRST_GIRQ;

	return xec_config_ecia.girq_node_handles[girq_num];
}

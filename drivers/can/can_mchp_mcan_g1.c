/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Glue for the Microchip M_CAN G1 wrapper around the upstream Bosch M_CAN
 * core (can_mcan.c). The M_CAN core registers start at wrapper + 0x100;
 * below that offset are the wrapper's own CTRLA/CTRLB/SYNCBUSY/interrupt
 * registers, accessed directly rather than through can_mcan's
 * read_reg/write_reg.
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include "can_mcan.h"

#define DT_DRV_COMPAT microchip_mcan_g1

LOG_MODULE_REGISTER(can_mchp_mcan_g1, CONFIG_CAN_LOG_LEVEL);

/* Wrapper register offsets, hal_microchip component/can.h. */
#define CAN_MCHP_MCAN_G1_CTRLA    0x00U
#define CAN_MCHP_MCAN_G1_CTRLB    0x04U
#define CAN_MCHP_MCAN_G1_INTENSET 0x2CU
#define CAN_MCHP_MCAN_G1_INTFLAG  0x30U
#define CAN_MCHP_MCAN_G1_SYNCBUSY 0x5CU

#define CAN_MCHP_MCAN_G1_CTRLA_SWRST BIT(0)

/*
 * CTRLB.OFFSET is the message RAM window, bits 23:16 (hal_microchip
 * component/can.h: CAN_CTRLB_OFFSET_Pos = 16, CAN_CTRLB_OFFSET_Msk =
 * 0xFF << 16).
 */
#define CAN_MCHP_MCAN_G1_CTRLB_OFFSET_MASK GENMASK(23, 16)

/* INTFLAG and INTENSET carry two bits only. */
#define CAN_MCHP_MCAN_G1_INT_DBG  BIT(0)
#define CAN_MCHP_MCAN_G1_INT_BERR BIT(1)

/*
 * Only bits 15:2 of a Message RAM address are evaluated (data sheet
 * 33.6.1); the CAN core forms the rest as 0x20 << 24 | CTRLB.OFFSET << 16,
 * so only the 0x20000000 SRAM aperture is reachable.
 */
#define CAN_MCHP_MCAN_G1_MRAM_FIXED_MSB 0x20U

/* SWRST synchronises; nothing here should take long. */
#define CAN_MCHP_MCAN_G1_SYNC_TIMEOUT_US 1000

struct can_mchp_mcan_g1_config {
	mm_reg_t wrapper;
	mm_reg_t base;
	mem_addr_t mram;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
	const struct pinctrl_dev_config *pcfg;
	void (*config_irq)(void);
};

static int can_mchp_mcan_g1_sync(const struct can_mchp_mcan_g1_config *cfg)
{
	if (!WAIT_FOR(sys_read32(cfg->wrapper + CAN_MCHP_MCAN_G1_SYNCBUSY) == 0U,
		      CAN_MCHP_MCAN_G1_SYNC_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int can_mchp_mcan_g1_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mchp_mcan_g1_config *cfg = mcan_cfg->custom;

	return can_mcan_sys_read_reg(cfg->base, reg, val);
}

static int can_mchp_mcan_g1_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mchp_mcan_g1_config *cfg = mcan_cfg->custom;

	return can_mcan_sys_write_reg(cfg->base, reg, val);
}

static int can_mchp_mcan_g1_read_mram(const struct device *dev, uint16_t offset, void *dst,
				       size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mchp_mcan_g1_config *cfg = mcan_cfg->custom;

	return can_mcan_sys_read_mram(cfg->mram, offset, dst, len);
}

static int can_mchp_mcan_g1_write_mram(const struct device *dev, uint16_t offset, const void *src,
					size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mchp_mcan_g1_config *cfg = mcan_cfg->custom;

	return can_mcan_sys_write_mram(cfg->mram, offset, src, len);
}

static int can_mchp_mcan_g1_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mchp_mcan_g1_config *cfg = mcan_cfg->custom;

	return can_mcan_sys_clear_mram(cfg->mram, offset, len);
}

static int can_mchp_mcan_g1_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mchp_mcan_g1_config *cfg = mcan_cfg->custom;

	return clock_control_get_rate(cfg->clock_dev, cfg->gclk_sys, rate);
}

/*
 * The wrapper's own AHB bus error/debug interrupt, outside the Bosch
 * core. Only logged: no Zephyr CAN state describes a misplaced message
 * RAM window.
 */
static void can_mchp_mcan_g1_berr_isr(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mchp_mcan_g1_config *cfg = mcan_cfg->custom;
	uint32_t flags = sys_read32(cfg->wrapper + CAN_MCHP_MCAN_G1_INTFLAG);

	/* Write one to clear, both bits. */
	sys_write32(flags, cfg->wrapper + CAN_MCHP_MCAN_G1_INTFLAG);

	if ((flags & CAN_MCHP_MCAN_G1_INT_BERR) != 0U) {
		LOG_ERR("AHB bus error: the message RAM window is not where the "
			"controller is reading");
	}

	if ((flags & CAN_MCHP_MCAN_G1_INT_DBG) != 0U) {
		LOG_DBG("debug message received");
	}
}

/*
 * The message RAM must sit inside one 64 KB window, selected by
 * CTRLB.OFFSET (bits 23:16); the array is 4096-byte aligned and at most
 * 4096 bytes, so it always does.
 */
static int can_mchp_mcan_g1_configure_window(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mchp_mcan_g1_config *cfg = mcan_cfg->custom;
	uintptr_t mrba = (uintptr_t)cfg->mram & ~UINT32_C(0xFFFF);
	uint32_t offset = FIELD_PREP(CAN_MCHP_MCAN_G1_CTRLB_OFFSET_MASK, mrba >> 16);

	if ((mrba >> 24) != CAN_MCHP_MCAN_G1_MRAM_FIXED_MSB) {
		LOG_ERR("message RAM at 0x%lx is outside the addressable aperture",
			(unsigned long)cfg->mram);
		return -EINVAL;
	}

	sys_write32(offset, cfg->wrapper + CAN_MCHP_MCAN_G1_CTRLB);

	return can_mcan_configure_mram(dev, mrba, (uintptr_t)cfg->mram);
}

static int can_mchp_mcan_g1_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_mchp_mcan_g1_config *cfg = mcan_cfg->custom;
	int ret;

	/*
	 * -EALREADY means the clock is already on, which is not a failure.
	 */
	ret = clock_control_on(cfg->clock_dev, cfg->mclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("failed to enable the bus clock: %d", ret);
		return ret;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->gclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("failed to enable the generic clock: %d", ret);
		return ret;
	}

	sys_write32(CAN_MCHP_MCAN_G1_CTRLA_SWRST, cfg->wrapper + CAN_MCHP_MCAN_G1_CTRLA);

	ret = can_mchp_mcan_g1_sync(cfg);
	if (ret < 0) {
		LOG_ERR("reset did not complete");
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("failed to apply pinctrl: %d", ret);
		return ret;
	}

	ret = can_mchp_mcan_g1_configure_window(dev);
	if (ret < 0) {
		LOG_ERR("failed to configure the message RAM: %d", ret);
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret < 0) {
		LOG_ERR("core init failed: %d", ret);
		return ret;
	}

	cfg->config_irq();

	sys_write32(CAN_MCHP_MCAN_G1_INT_BERR, cfg->wrapper + CAN_MCHP_MCAN_G1_INTENSET);

	return 0;
}

static DEVICE_API(can, can_mchp_mcan_g1_driver_api) = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_mcan_start,
	.stop = can_mcan_stop,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
	.get_state = can_mcan_get_state,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_mcan_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.get_core_clock = can_mchp_mcan_g1_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static const struct can_mcan_ops can_mchp_mcan_g1_ops = {
	.read_reg = can_mchp_mcan_g1_read_reg,
	.write_reg = can_mchp_mcan_g1_write_reg,
	.read_mram = can_mchp_mcan_g1_read_mram,
	.write_mram = can_mchp_mcan_g1_write_mram,
	.clear_mram = can_mchp_mcan_g1_clear_mram,
};

/*
 * Not CAN_MCAN_DT_INST_MRAM_DEFINE: that macro aligns to 4, and the array
 * must stay inside one 64 KB CTRLB.OFFSET window (4096 divides 65536).
 */
#define CAN_MCHP_MCAN_G1_MRAM_DEFINE(inst, name)                                                 \
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_OFFSET(inst) == 0, "mram offset must be 0");           \
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_ELEMENTS_SIZE(inst) <= 4096,                           \
		     "message RAM must fit in one 4096-byte aligned block");                     \
	static char __nocache_noinit __aligned(4096) name[CAN_MCAN_DT_INST_MRAM_ELEMENTS_SIZE(inst)]

#define CAN_MCHP_MCAN_G1_IRQ_CFG(inst)                                                            \
	static void can_mchp_mcan_g1_config_irq_##inst(void)                                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, int0, irq),                                 \
			    DT_INST_IRQ_BY_NAME(inst, int0, priority), can_mcan_line_0_isr,       \
			    DEVICE_DT_INST_GET(inst), 0);                                         \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, int0, irq));                                 \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, int1, irq),                                 \
			    DT_INST_IRQ_BY_NAME(inst, int1, priority), can_mcan_line_1_isr,       \
			    DEVICE_DT_INST_GET(inst), 0);                                         \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, int1, irq));                                 \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, berr, irq),                                 \
			    DT_INST_IRQ_BY_NAME(inst, berr, priority),                            \
			    can_mchp_mcan_g1_berr_isr, DEVICE_DT_INST_GET(inst), 0);              \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, berr, irq));                                 \
	}

#define CAN_MCHP_MCAN_G1_INST(inst)                                                               \
	CAN_MCAN_DT_INST_BUILD_ASSERT_MRAM_CFG(inst);                                             \
	PINCTRL_DT_INST_DEFINE(inst);                                                             \
	CAN_MCAN_DT_INST_CALLBACKS_DEFINE(inst, can_mchp_mcan_g1_cbs_##inst);                     \
	CAN_MCHP_MCAN_G1_MRAM_DEFINE(inst, can_mchp_mcan_g1_mram_##inst);                         \
	CAN_MCHP_MCAN_G1_IRQ_CFG(inst)                                                            \
                                                                                                   \
	static const struct can_mchp_mcan_g1_config can_mchp_mcan_g1_cfg_##inst = {               \
		.wrapper = (mm_reg_t)DT_INST_REG_ADDR_BY_NAME(inst, wrapper),                     \
		.base = CAN_MCAN_DT_INST_MCAN_ADDR(inst),                                         \
		.mram = (mem_addr_t)POINTER_TO_UINT(&can_mchp_mcan_g1_mram_##inst),               \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                  \
		.mclk_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(inst, mclk, subsystem),           \
		.gclk_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(inst, gclk, subsystem),           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                     \
		.config_irq = can_mchp_mcan_g1_config_irq_##inst,                                 \
	};                                                                                         \
                                                                                                   \
	static const struct can_mcan_config can_mcan_cfg_##inst = CAN_MCAN_DT_CONFIG_INST_GET(    \
		inst, &can_mchp_mcan_g1_cfg_##inst, &can_mchp_mcan_g1_ops,                        \
		&can_mchp_mcan_g1_cbs_##inst);                                                    \
                                                                                                   \
	CAN_MCAN_DATA_DEFINE(can_mcan_data_##inst, NULL);                                         \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_mchp_mcan_g1_init, NULL, &can_mcan_data_##inst,       \
				  &can_mcan_cfg_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,    \
				  &can_mchp_mcan_g1_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_MCHP_MCAN_G1_INST)

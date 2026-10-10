/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Arkadiusz Grzelka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "udc_common.h"
#include "udc_musb.h"

LOG_MODULE_REGISTER(udc_mchp_usbhs, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_pic32cz_usbhs

#define USBHS_TIMEOUT_SYNCBUSY_US 1000U

#define USBHS_TIMEOUT_PHY_US 100000U

#define USBHS_AVREG_SETTLE_US 60U

/* Datasheet figure: RAMINFO is a bus width and the pack's EPINFO is wrong. */
#define USBHS_FIFO_RAM_BYTES (9U * 1024U)

#define USBHS_PHY_TRIM_ADDR 0x0A00718CUL

/* Absent from the pack header, whose PHY24 mask drops it. */
#define USBHS_PHY24_OTGPDN_Msk BIT(1)

#define USBHS_INSTANCE_BASE   0x4F010000UL
#define USBHS_INSTANCE_STRIDE 0x2000UL

#define USBHS_CORE_OFFSET 0x1000U

/* VREGCTRL and STATUS are shared by both instances. */
static struct k_spinlock usbhs_supc_lock;

struct usbhs_phy_trim {
	uint16_t reg_off;
	uint8_t src_lsb;
	uint8_t width;
	uint8_t dst_lsb;
};

static const struct usbhs_phy_trim usbhs_phy_trim_map[] = {
	{0x1504, 0, 3, 5},  /* PHY04[7:5] RxSQUELCH[2:0] */
	{0x1508, 3, 1, 0},  /* PHY08[0]   RxSQUELCH[3]   */
	{0x150C, 4, 3, 5},  /* PHY0C[7:5] TUNE[2:0]      */
	{0x1510, 7, 5, 0},  /* PHY10[4:0] TUNE[7:3]      */
	{0x1514, 12, 1, 7}, /* PHY14[7]   ODT[0]         */
	{0x1518, 13, 2, 0}, /* PHY18[1:0] ODT[2:1]       */
	{0x1520, 15, 2, 6}, /* PHY20[7:6] HSSLEW[1:0]    */
	{0x1524, 17, 1, 0}, /* PHY24[0]   HSSLEW[2]      */
	{0x1528, 18, 4, 1}, /* PHY28[4:1] DISCONDET[3:0] */
	{0x1528, 22, 3, 5}, /* PHY28[7:5] HSDRVCOMP[2:0] */
};

struct udc_usbhs_config {
	struct udc_musb_config musb;
	usbhs_registers_t *base;
	uint8_t avreg_idx;
	struct gpio_dt_spec drd;
	struct gpio_dt_spec vbus_enable;
	struct {
		const struct device *clock_dev;
		clock_control_subsys_t mclk_subsys;
	} clock;
};

enum usbhs_stage {
	USBHS_STAGE_NONE = 0,
	USBHS_STAGE_CONNECTOR,
	USBHS_STAGE_REGULATOR,
	USBHS_STAGE_CLOCK,
	USBHS_STAGE_ENABLED,
};

struct udc_usbhs_data {
	struct udc_musb_data musb;
	enum usbhs_stage stage;
};

static inline usbhs_endpoint0_registers_t *usbhs_wrapper(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;

	return &cfg->base->ENDPOINT0;
}

/* Errata 2.14.2: every PHY access needs a dummy STATUS read after it. */
static uint32_t usbhs_phy_read(const struct device *dev, uint16_t reg_off)
{
	const struct udc_usbhs_config *cfg = dev->config;
	volatile uint32_t *reg = (volatile uint32_t *)((uintptr_t)cfg->base + reg_off);
	uint32_t val = *reg;

	(void)cfg->base->ENDPOINT0.USBHS_STATUS;

	return val;
}

static void usbhs_phy_write(const struct device *dev, uint16_t reg_off, uint32_t val)
{
	const struct udc_usbhs_config *cfg = dev->config;
	volatile uint32_t *reg = (volatile uint32_t *)((uintptr_t)cfg->base + reg_off);

	*reg = val;

	(void)cfg->base->ENDPOINT0.USBHS_STATUS;
}

static void usbhs_load_phy_trim(const struct device *dev)
{
	uint32_t otp = sys_read32(USBHS_PHY_TRIM_ADDR);

	for (size_t i = 0; i < ARRAY_SIZE(usbhs_phy_trim_map); i++) {
		const struct usbhs_phy_trim *t = &usbhs_phy_trim_map[i];
		uint32_t field_msk = BIT_MASK(t->width);
		uint32_t val = (otp >> t->src_lsb) & field_msk;
		uint32_t reg = usbhs_phy_read(dev, t->reg_off);

		reg &= ~(field_msk << t->dst_lsb);
		reg |= val << t->dst_lsb;

		usbhs_phy_write(dev, t->reg_off, reg);
	}

	LOG_DBG("PHY trim word 0x%08x applied", otp);
}

static int usbhs_wait_syncbusy(const struct device *dev)
{
	usbhs_endpoint0_registers_t *const wrap = usbhs_wrapper(dev);

	if (!WAIT_FOR(wrap->USBHS_SYNCBUSY == 0U, USBHS_TIMEOUT_SYNCBUSY_US, NULL)) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int usbhs_connector_setup(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;
	int ret;

	if (cfg->vbus_enable.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->vbus_enable, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to deassert the VBUS switch: %d", ret);
			return ret;
		}
	}

	if (cfg->drd.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->drd, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to select the device role on the connector: %d", ret);
			return ret;
		}
	}

	return 0;
}

static void usbhs_connector_release(const struct device *dev)
{
	const struct udc_usbhs_config *cfg = dev->config;

	if (cfg->drd.port != NULL) {
		(void)gpio_pin_set_dt(&cfg->drd, 0);
	}

	if (cfg->vbus_enable.port != NULL) {
		(void)gpio_pin_set_dt(&cfg->vbus_enable, 0);
	}
}

static void usbhs_unwind(const struct device *dev, enum usbhs_stage stage)
{
	const struct udc_usbhs_config *cfg = dev->config;

	if (stage >= USBHS_STAGE_ENABLED) {
		usbhs_wrapper(dev)->USBHS_CTRLA = 0U;
		(void)usbhs_wait_syncbusy(dev);
	}

	if (stage >= USBHS_STAGE_CLOCK) {
		(void)clock_control_off(cfg->clock.clock_dev, cfg->clock.mclk_subsys);
	}

	if (stage >= USBHS_STAGE_REGULATOR) {
		K_SPINLOCK(&usbhs_supc_lock) {
			SUPC_REGS->SUPC_VREGCTRL &=
				~BIT(SUPC_VREGCTRL_AVREGEN_Pos + cfg->avreg_idx);
		}
	}

	if (stage >= USBHS_STAGE_CONNECTOR) {
		usbhs_connector_release(dev);
	}
}

static int usbhs_bringup(const struct device *dev, enum usbhs_stage *stage)
{
	const struct udc_usbhs_config *cfg = dev->config;
	usbhs_endpoint0_registers_t *const wrap = usbhs_wrapper(dev);
	uint32_t ctrla;
	int ret;

	*stage = USBHS_STAGE_NONE;

	ret = usbhs_connector_setup(dev);
	if (ret != 0) {
		return ret;
	}

	*stage = USBHS_STAGE_CONNECTOR;

	K_SPINLOCK(&usbhs_supc_lock) {
		SUPC_REGS->SUPC_VREGCTRL |= BIT(SUPC_VREGCTRL_AVREGEN_Pos + cfg->avreg_idx);
	}

	*stage = USBHS_STAGE_REGULATOR;

	if (!WAIT_FOR((SUPC_REGS->SUPC_STATUS & BIT(SUPC_STATUS_ADDVREGRDY_Pos + cfg->avreg_idx)) !=
			      0U,
		      USBHS_TIMEOUT_PHY_US, NULL)) {
		LOG_ERR("Additional voltage regulator %u did not come up", cfg->avreg_idx);
		return -EIO;
	}

	k_busy_wait(USBHS_AVREG_SETTLE_US);

	ret = clock_control_on(cfg->clock.clock_dev, cfg->clock.mclk_subsys);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the USBHS peripheral clock: %d", ret);
		return ret;
	}

	*stage = USBHS_STAGE_CLOCK;

	/* Enable-protected, so only the board devicetree can set it. */
	if ((OSCCTRL_REGS->OSCCTRL_XOSCCTRLA & OSCCTRL_XOSCCTRLA_USBHSDIV_Msk) ==
	    OSCCTRL_XOSCCTRLA_USBHSDIV_DIS) {
		LOG_ERR("The XOSC does not drive the USB PLL reference: the board devicetree "
			"needs xosc-usb-ref-clock-div on its XOSC node");
		return -EIO;
	}

	/* Errata 2.14.2: dummy read before the first access. */
	(void)wrap->USBHS_STATUS;

	wrap->USBHS_CTRLA = USBHS_CTRLA_SWRST_Msk;

	ret = usbhs_wait_syncbusy(dev);
	if (ret != 0) {
		LOG_ERR("Timeout on the software reset");
		return ret;
	}

	if (!WAIT_FOR((wrap->USBHS_STATUS & USBHS_STATUS_PHYRDY_Msk) == 0U, USBHS_TIMEOUT_PHY_US,
		      NULL)) {
		LOG_ERR("PHY still ready after the software reset");
		return -EIO;
	}

	usbhs_load_phy_trim(dev);

	/* Every other CTRLA bit is enable-protected: set them before ENABLE. */
	ctrla = USBHS_CTRLA_IDOVEN_Msk | USBHS_CTRLA_IDVAL_Msk;
	wrap->USBHS_CTRLA = ctrla;
	wrap->USBHS_CTRLA = ctrla | USBHS_CTRLA_ENABLE_Msk;
	*stage = USBHS_STAGE_ENABLED;

	ret = usbhs_wait_syncbusy(dev);
	if (ret != 0) {
		LOG_ERR("Timeout enabling the peripheral");
		return ret;
	}

	if (!WAIT_FOR((wrap->USBHS_STATUS & (USBHS_STATUS_PHYON_Msk | USBHS_STATUS_PHYRDY_Msk)) ==
			      (USBHS_STATUS_PHYON_Msk | USBHS_STATUS_PHYRDY_Msk),
		      USBHS_TIMEOUT_PHY_US, NULL)) {
		LOG_ERR("PHY did not become ready");
		return -EIO;
	}

	usbhs_phy_write(dev, 0x1524, usbhs_phy_read(dev, 0x1524) | USBHS_PHY24_OTGPDN_Msk);

	return 0;
}

static int usbhs_init(const struct device *dev)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	int ret;

	ret = usbhs_bringup(dev, &priv->stage);
	if (ret != 0) {
		usbhs_unwind(dev, priv->stage);
		priv->stage = USBHS_STAGE_NONE;
	}

	return ret;
}

static int usbhs_shutdown(const struct device *dev)
{
	struct udc_usbhs_data *const priv = udc_get_private(dev);
	int ret = 0;

	if (priv->stage >= USBHS_STAGE_ENABLED) {
		usbhs_wrapper(dev)->USBHS_CTRLA = 0U;

		ret = usbhs_wait_syncbusy(dev);
		if (ret != 0) {
			/* Carry on: returning would leave the regulator and clock on. */
			LOG_ERR("Timeout disabling the peripheral");
		}
	}

	/* CTRLA is already cleared above, so the unwind starts below it. */
	usbhs_unwind(dev, MIN(priv->stage, USBHS_STAGE_CLOCK));
	priv->stage = USBHS_STAGE_NONE;

	return ret;
}

static void usbhs_isr(const struct device *dev)
{
	/* Clear INTFLAG.USB first, or the interrupt line never deasserts. */
	usbhs_wrapper(dev)->USBHS_INTFLAG = USBHS_INTFLAG_USB_Msk;

	udc_musb_isr(dev);
}

#define UDC_USBHS_IRQ_ENABLE(i, n)                                                                 \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq), DT_INST_IRQ_BY_IDX(n, i, priority), usbhs_isr,  \
		    DEVICE_DT_INST_GET(n), 0);                                                     \
	irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));

#define UDC_USBHS_IRQ_DISABLE(i, n) irq_disable(DT_INST_IRQ_BY_IDX(n, i, irq));

/* The wrapper has its own mask in front of the core's. */
#define UDC_USBHS_IRQ_ENABLE_DEFINE(n)                                                             \
	static void udc_usbhs_irq_enable_func_##n(const struct device *dev)                        \
	{                                                                                          \
		usbhs_wrapper(dev)->USBHS_INTENSET = USBHS_INTENSET_USB_Msk;                       \
		LISTIFY(DT_INST_NUM_IRQS(n), UDC_USBHS_IRQ_ENABLE, (), n)                          \
	}

#define UDC_USBHS_IRQ_DISABLE_DEFINE(n)                                                            \
	static void udc_usbhs_irq_disable_func_##n(const struct device *dev)                       \
	{                                                                                          \
		usbhs_wrapper(dev)->USBHS_INTENCLR = USBHS_INTENCLR_USB_Msk;                       \
		LISTIFY(DT_INST_NUM_IRQS(n), UDC_USBHS_IRQ_DISABLE, (), n)                         \
	}

#define UDC_USBHS_THREAD_DEFINE(n)                                                                 \
	K_THREAD_STACK_DEFINE(udc_usbhs_stack_##n, CONFIG_UDC_MCHP_USBHS_STACK_SIZE);              \
                                                                                                   \
	static void udc_usbhs_make_thread_##n(const struct device *dev)                            \
	{                                                                                          \
		struct udc_musb_data *priv = udc_get_private(dev);                                 \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_usbhs_stack_##n,                           \
				K_THREAD_STACK_SIZEOF(udc_usbhs_stack_##n), udc_musb_thread,       \
				(void *)dev, NULL, NULL,                                           \
				K_PRIO_COOP(CONFIG_UDC_MCHP_USBHS_THREAD_PRIORITY), K_ESSENTIAL,   \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}

#define UDC_USBHS_AVREG_IDX(n)                                                                     \
	((uint8_t)((DT_INST_REG_ADDR(n) - USBHS_INSTANCE_BASE) / USBHS_INSTANCE_STRIDE))

/* maximum-speed enum: low, full, high, super; unset means full. */
#define UDC_USBHS_HIGH_SPEED(n) (DT_INST_ENUM_IDX_OR(n, maximum_speed, 1) >= 2)

#define UDC_USBHS_CONFIG_DEFINE(n)                                                                 \
	BUILD_ASSERT(UDC_USBHS_AVREG_IDX(n) < 3, "USBHS instance outside the known address map");  \
                                                                                                   \
	static struct udc_ep_config                                                                \
		udc_usbhs_ep_cfg_out_##n[DT_INST_PROP(n, num_bidir_endpoints)];                    \
	static struct udc_ep_config udc_usbhs_ep_cfg_in_##n[DT_INST_PROP(n, num_bidir_endpoints)]; \
	static struct udc_musb_fifo_block                                                          \
		udc_usbhs_fifo_##n[UDC_MUSB_FIFO_SLOTS(DT_INST_PROP(n, num_bidir_endpoints))];     \
                                                                                                   \
	static const struct udc_usbhs_config udc_usbhs_config_##n = {                              \
		.musb.mbase = DT_INST_REG_ADDR(n) + USBHS_CORE_OFFSET,                             \
		.musb.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),                           \
		.musb.ep_cfg_in = udc_usbhs_ep_cfg_in_##n,                                         \
		.musb.ep_cfg_out = udc_usbhs_ep_cfg_out_##n,                                       \
		.musb.fifo = udc_usbhs_fifo_##n,                                                   \
		.musb.fifo_ram_bytes = USBHS_FIFO_RAM_BYTES,                                       \
		.musb.high_speed = UDC_USBHS_HIGH_SPEED(n),                                        \
		.musb.init = usbhs_init,                                                           \
		.musb.shutdown = usbhs_shutdown,                                                   \
		.musb.irq_enable_func = udc_usbhs_irq_enable_func_##n,                             \
		.musb.irq_disable_func = udc_usbhs_irq_disable_func_##n,                           \
		.musb.make_thread = udc_usbhs_make_thread_##n,                                     \
		.base = (usbhs_registers_t *)DT_INST_REG_ADDR(n),                                  \
		.avreg_idx = UDC_USBHS_AVREG_IDX(n),                                               \
		.drd = GPIO_DT_SPEC_INST_GET_OR(n, drd_gpios, {0}),                                \
		.vbus_enable = GPIO_DT_SPEC_INST_GET_OR(n, vbus_enable_gpios, {0}),                \
		.clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                             \
		.clock.mclk_subsys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem),      \
	}

#define UDC_USBHS_DATA_DEFINE(n)                                                                   \
	static struct udc_usbhs_data udc_usbhs_priv_##n = {};                                      \
	static struct udc_data udc_usbhs_data_##n = {                                              \
		.mutex = Z_MUTEX_INITIALIZER(udc_usbhs_data_##n.mutex),                            \
		.priv = &udc_usbhs_priv_##n,                                                       \
	};

#define UDC_USBHS_DEVICE_DEFINE(n)                                                                 \
	UDC_USBHS_IRQ_ENABLE_DEFINE(n);                                                            \
	UDC_USBHS_IRQ_DISABLE_DEFINE(n);                                                           \
	UDC_USBHS_THREAD_DEFINE(n);                                                                \
	UDC_USBHS_DATA_DEFINE(n);                                                                  \
	UDC_USBHS_CONFIG_DEFINE(n);                                                                \
	DEVICE_DT_INST_DEFINE(n, udc_musb_preinit, NULL, &udc_usbhs_data_##n,                      \
			      &udc_usbhs_config_##n, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_musb_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_USBHS_DEVICE_DEFINE)

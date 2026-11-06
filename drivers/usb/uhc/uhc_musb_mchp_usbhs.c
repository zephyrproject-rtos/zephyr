/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_pic32cz_usbhs

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "uhc_common.h"
#include "uhc_musb.h"

LOG_MODULE_REGISTER(uhc_musb_mchp, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define USBHS_INSTANCE_BASE       0x4f010000UL
#define USBHS_INSTANCE_STRIDE     0x2000UL
#define USBHS_CORE_OFFSET         0x1000U
#define USBHS_TIMEOUT_SYNCBUSY_US 1000U
#define USBHS_TIMEOUT_PHY_US      100000U
#define USBHS_AVREG_SETTLE_US     60U
#define USBHS_PHY_TRIM_ADDR       0x0a00718cUL
#define USBHS_FIFO_RAM_BYTES      (9U * 1024U)
#define USBHS_PHY24_OTGPDN_Msk    BIT(1)

enum usbhs_stage {
	USBHS_STAGE_NONE,
	USBHS_STAGE_REGULATOR,
	USBHS_STAGE_CLOCK,
	USBHS_STAGE_ENABLED,
};

struct uhc_musb_mchp_config {
	struct uhc_musb_config musb;
	usbhs_registers_t *base;
	uint8_t avreg_idx;
	struct gpio_dt_spec drd;
	struct gpio_dt_spec vbus_enable;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_subsys;
};

struct uhc_musb_mchp_data {
	struct uhc_musb_data musb;
	enum usbhs_stage stage;
};

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

static struct k_spinlock usbhs_supc_lock;

#define DEV_CFG(dev)                                                                               \
	CONTAINER_OF((const struct uhc_musb_config *)(dev)->config,                                \
		     const struct uhc_musb_mchp_config, musb)
#define DEV_PRIV(dev)                                                                              \
	CONTAINER_OF((struct uhc_musb_data *)uhc_get_private(dev), struct uhc_musb_mchp_data, musb)

static int usbhs_wait_syncbusy(const struct device *dev)
{
	usbhs_endpoint0_registers_t *const wrap = &DEV_CFG(dev)->base->ENDPOINT0;

	if (!WAIT_FOR(wrap->USBHS_SYNCBUSY == 0U, USBHS_TIMEOUT_SYNCBUSY_US, NULL)) {
		return -ETIMEDOUT;
	}

	return 0;
}

static uint32_t usbhs_phy_read(const struct device *dev, uint16_t reg_off)
{
	usbhs_endpoint0_registers_t *const wrap = &DEV_CFG(dev)->base->ENDPOINT0;
	volatile uint32_t *reg = (volatile uint32_t *)((uintptr_t)DEV_CFG(dev)->base + reg_off);
	uint32_t val = *reg;

	(void)wrap->USBHS_STATUS;

	return val;
}

static void usbhs_phy_write(const struct device *dev, uint16_t reg_off, uint32_t val)
{
	usbhs_endpoint0_registers_t *const wrap = &DEV_CFG(dev)->base->ENDPOINT0;
	volatile uint32_t *reg = (volatile uint32_t *)((uintptr_t)DEV_CFG(dev)->base + reg_off);

	*reg = val;
	(void)wrap->USBHS_STATUS;
}

static void usbhs_load_phy_trim(const struct device *dev)
{
	uint32_t otp = sys_read32(USBHS_PHY_TRIM_ADDR);

	for (size_t idx = 0; idx < ARRAY_SIZE(usbhs_phy_trim_map); idx++) {
		const struct usbhs_phy_trim *trim = &usbhs_phy_trim_map[idx];
		uint32_t mask = BIT_MASK(trim->width);
		uint32_t val = (otp >> trim->src_lsb) & mask;
		uint32_t reg = usbhs_phy_read(dev, trim->reg_off);

		reg &= ~(mask << trim->dst_lsb);
		reg |= val << trim->dst_lsb;
		usbhs_phy_write(dev, trim->reg_off, reg);
	}
}

static int usbhs_bringup(const struct device *dev, enum usbhs_stage *stage)
{
	const struct uhc_musb_mchp_config *cfg = DEV_CFG(dev);
	usbhs_endpoint0_registers_t *const wrap = &cfg->base->ENDPOINT0;
	int ret;

	*stage = USBHS_STAGE_NONE;

	k_spinlock_key_t key = k_spin_lock(&usbhs_supc_lock);

	SUPC_REGS->SUPC_VREGCTRL |= BIT(SUPC_VREGCTRL_AVREGEN_Pos + cfg->avreg_idx);
	k_spin_unlock(&usbhs_supc_lock, key);
	*stage = USBHS_STAGE_REGULATOR;

	if (!WAIT_FOR((SUPC_REGS->SUPC_STATUS & BIT(SUPC_STATUS_ADDVREGRDY_Pos + cfg->avreg_idx)) !=
			      0U,
		      USBHS_TIMEOUT_PHY_US, NULL)) {
		LOG_ERR("USBHS regulator %u did not become ready", cfg->avreg_idx);
		return -EIO;
	}
	k_busy_wait(USBHS_AVREG_SETTLE_US);

	ret = clock_control_on(cfg->clock_dev, cfg->mclk_subsys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable USBHS clock: %d", ret);
		return ret;
	}
	*stage = USBHS_STAGE_CLOCK;

	if ((OSCCTRL_REGS->OSCCTRL_XOSCCTRLA & OSCCTRL_XOSCCTRLA_USBHSDIV_Msk) ==
	    OSCCTRL_XOSCCTRLA_USBHSDIV_DIS) {
		LOG_ERR("USBHS reference clock is disabled in XOSC configuration");
		return -EIO;
	}

	(void)wrap->USBHS_STATUS;
	wrap->USBHS_CTRLA = USBHS_CTRLA_SWRST_Msk;
	ret = usbhs_wait_syncbusy(dev);
	if (ret != 0) {
		LOG_ERR("Timeout resetting USBHS wrapper");
		return ret;
	}
	if (!WAIT_FOR((wrap->USBHS_STATUS & USBHS_STATUS_PHYRDY_Msk) == 0U, USBHS_TIMEOUT_PHY_US,
		      NULL)) {
		LOG_ERR("USBHS PHY remained ready after reset");
		return -EIO;
	}

	usbhs_load_phy_trim(dev);

	/* IDVAL is zero to select the A-device (host) role. */
	wrap->USBHS_CTRLA = USBHS_CTRLA_IDOVEN_Msk;
	wrap->USBHS_CTRLA = USBHS_CTRLA_IDOVEN_Msk | USBHS_CTRLA_ENABLE_Msk;
	*stage = USBHS_STAGE_ENABLED;

	ret = usbhs_wait_syncbusy(dev);
	if (ret != 0) {
		LOG_ERR("Timeout enabling USBHS wrapper");
		return ret;
	}
	if (!WAIT_FOR((wrap->USBHS_STATUS & (USBHS_STATUS_PHYON_Msk | USBHS_STATUS_PHYRDY_Msk)) ==
			      (USBHS_STATUS_PHYON_Msk | USBHS_STATUS_PHYRDY_Msk),
		      USBHS_TIMEOUT_PHY_US, NULL)) {
		LOG_ERR("USBHS PHY did not become ready");
		return -EIO;
	}

	/* Power down the OTG VBUS comparators, VBUS is switched by a GPIO */
	usbhs_phy_write(dev, 0x1524, usbhs_phy_read(dev, 0x1524) | USBHS_PHY24_OTGPDN_Msk);

	return 0;
}

static void usbhs_unwind(const struct device *dev, enum usbhs_stage stage)
{
	const struct uhc_musb_mchp_config *cfg = DEV_CFG(dev);

	if (stage >= USBHS_STAGE_ENABLED) {
		cfg->base->ENDPOINT0.USBHS_CTRLA = 0U;
		(void)usbhs_wait_syncbusy(dev);
	}
	if (stage >= USBHS_STAGE_CLOCK) {
		(void)clock_control_off(cfg->clock_dev, cfg->mclk_subsys);
	}
	if (stage >= USBHS_STAGE_REGULATOR) {
		k_spinlock_key_t key = k_spin_lock(&usbhs_supc_lock);

		SUPC_REGS->SUPC_VREGCTRL &= ~BIT(SUPC_VREGCTRL_AVREGEN_Pos + cfg->avreg_idx);
		k_spin_unlock(&usbhs_supc_lock, key);
	}
}

static int uhc_musb_mchp_init(const struct device *dev)
{
	const struct uhc_musb_mchp_config *cfg = DEV_CFG(dev);
	struct uhc_data *data = dev->data;
	struct uhc_musb_mchp_data *priv = DEV_PRIV(dev);
	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	if (cfg->vbus_enable.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->vbus_enable)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->vbus_enable, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to configure the VBUS switch: %d", ret);
			return ret;
		}
	}

	if (cfg->drd.port != NULL && !gpio_is_ready_dt(&cfg->drd)) {
		return -ENODEV;
	}

	k_mutex_init(&data->mutex);
	k_sem_init(&priv->musb.irq_sem, 0, 1);
	cfg->musb.make_thread(dev);

	return 0;
}

static int uhc_musb_mchp_vbus_on(const struct device *dev)
{
	const struct uhc_musb_mchp_config *cfg = DEV_CFG(dev);
	int ret;

	/* The inactive level of the role switch selects the host role */
	if (cfg->drd.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->drd, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to select the host role: %d", ret);
			return ret;
		}
	}

	if (cfg->vbus_enable.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->vbus_enable, 1);
		if (ret != 0) {
			LOG_ERR("Failed to switch on VBUS: %d", ret);
			return ret;
		}
	}

	return 0;
}

static int uhc_musb_mchp_vbus_off(const struct device *dev)
{
	const struct uhc_musb_mchp_config *cfg = DEV_CFG(dev);

	if (cfg->vbus_enable.port != NULL) {
		return gpio_pin_set_dt(&cfg->vbus_enable, 0);
	}

	return 0;
}

static int uhc_musb_mchp_shutdown(const struct device *dev)
{
	struct uhc_musb_mchp_data *priv = DEV_PRIV(dev);

	usbhs_unwind(dev, priv->stage);
	priv->stage = USBHS_STAGE_NONE;

	return 0;
}

static int uhc_musb_mchp_core_init(const struct device *dev)
{
	struct uhc_musb_mchp_data *priv = DEV_PRIV(dev);
	int ret = usbhs_bringup(dev, &priv->stage);

	if (ret != 0) {
		usbhs_unwind(dev, priv->stage);
		priv->stage = USBHS_STAGE_NONE;
	}

	return ret;
}

static void uhc_musb_mchp_irq(const struct device *dev)
{
	const struct uhc_musb_mchp_config *cfg = DEV_CFG(dev);

	cfg->base->ENDPOINT0.USBHS_INTFLAG = USBHS_INTFLAG_USB_Msk;
	uhc_musb_isr(dev);
}

#define UHC_MUSB_IRQ_ENABLE(n)                                                                     \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uhc_musb_mchp_irq,                  \
		    DEVICE_DT_INST_GET(n), 0);                                                     \
	irq_enable(DT_INST_IRQN(n))

#define UHC_MUSB_IRQ_DISABLE(n) irq_disable(DT_INST_IRQN(n))

#define UHC_MUSB_IRQ_ENABLE_DEFINE(n)                                                              \
	static void uhc_musb_irq_enable_##n(const struct device *dev)                              \
	{                                                                                          \
		DEV_CFG(dev)->base->ENDPOINT0.USBHS_INTENSET = USBHS_INTENSET_USB_Msk;             \
		UHC_MUSB_IRQ_ENABLE(n);                                                            \
	}

#define UHC_MUSB_IRQ_DISABLE_DEFINE(n)                                                             \
	static void uhc_musb_irq_disable_##n(const struct device *dev)                             \
	{                                                                                          \
		DEV_CFG(dev)->base->ENDPOINT0.USBHS_INTENCLR = USBHS_INTENCLR_USB_Msk;             \
		UHC_MUSB_IRQ_DISABLE(n);                                                           \
	}

#define UHC_MUSB_THREAD_DEFINE(n)                                                                  \
	K_THREAD_STACK_DEFINE(uhc_musb_stack_##n, CONFIG_UHC_MUSB_STACK_SIZE);                     \
	static void uhc_musb_make_thread_##n(const struct device *dev)                             \
	{                                                                                          \
		struct uhc_musb_data *priv = uhc_get_private(dev);                                 \
                                                                                                   \
		k_thread_create(&priv->thread, uhc_musb_stack_##n,                                 \
				K_THREAD_STACK_SIZEOF(uhc_musb_stack_##n), uhc_musb_thread,        \
				(void *)dev, NULL, NULL,                                           \
				K_PRIO_COOP(CONFIG_UHC_MUSB_THREAD_PRIORITY), K_ESSENTIAL,         \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread, dev->name);                                       \
	}

#define UHC_MUSB_CONFIG_DEFINE(n)                                                                  \
	static const struct uhc_musb_mchp_config uhc_musb_cfg_##n = {                              \
		.musb.mbase = DT_INST_REG_ADDR(n) + USBHS_CORE_OFFSET,                             \
		.musb.num_eps = DT_INST_PROP(n, num_bidir_endpoints),                              \
		.musb.fifo_size = USBHS_FIFO_RAM_BYTES,                                            \
		.musb.high_speed = DT_INST_ENUM_IDX_OR(n, maximum_speed, 1) == 2,                  \
		.musb.init = uhc_musb_mchp_core_init,                                              \
		.musb.shutdown = uhc_musb_mchp_shutdown,                                           \
		.musb.vbus_on = uhc_musb_mchp_vbus_on,                                             \
		.musb.vbus_off = uhc_musb_mchp_vbus_off,                                           \
		.musb.irq_enable_func = uhc_musb_irq_enable_##n,                                   \
		.musb.irq_disable_func = uhc_musb_irq_disable_##n,                                 \
		.musb.make_thread = uhc_musb_make_thread_##n,                                      \
		.base = (usbhs_registers_t *)DT_INST_REG_ADDR(n),                                  \
		.avreg_idx =                                                                       \
			((DT_INST_REG_ADDR(n) - USBHS_INSTANCE_BASE) / USBHS_INSTANCE_STRIDE),     \
		.drd = GPIO_DT_SPEC_INST_GET_OR(n, drd_gpios, {0}),                                \
		.vbus_enable = GPIO_DT_SPEC_INST_GET_OR(n, vbus_enable_gpios, {0}),                \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                   \
		.mclk_subsys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem),            \
	}

#define UHC_MUSB_DATA_DEFINE(n)                                                                    \
	static struct uhc_musb_mchp_data uhc_musb_priv_##n;                                        \
	static struct uhc_data uhc_musb_data_##n = {                                               \
		.priv = &uhc_musb_priv_##n.musb,                                                   \
	}

#define UHC_MUSB_DEVICE_DEFINE(n)                                                                  \
	BUILD_ASSERT(DT_INST_ENUM_IDX_OR(n, maximum_speed, 1) <= 2,                                \
		     "PIC32CZ USBHS does not support SuperSpeed");                                 \
	UHC_MUSB_IRQ_ENABLE_DEFINE(n);                                                             \
	UHC_MUSB_IRQ_DISABLE_DEFINE(n);                                                            \
	UHC_MUSB_THREAD_DEFINE(n);                                                                 \
	UHC_MUSB_CONFIG_DEFINE(n);                                                                 \
	UHC_MUSB_DATA_DEFINE(n);                                                                   \
	DEVICE_DT_INST_DEFINE(n, uhc_musb_mchp_init, NULL, &uhc_musb_data_##n,                     \
			      &uhc_musb_cfg_##n.musb, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uhc_musb_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_MUSB_DEVICE_DEFINE)

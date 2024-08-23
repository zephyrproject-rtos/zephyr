/**
 * clock_control_ra2_sysclk.c
 *
 * RA2 Virtual (base for ICLK, PCKLB etc) system clock driver implementation
 *
 * Copyright (c) 2022-2024 MUNIC Car Data
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/lpm/lpm_ra2.h>
#include <string.h>
#include <soc.h>
#include "clock_control_ra2_priv.h"

#define DT_DRV_COMPAT	renesas_ra2_sysclk
#define RA_SYSCLK_NODE	DT_DRV_INST(0)

struct ra_sysclk_cfg {
	uint8_t			valid_int_clocks;
};

struct ra_sysclk_data {
	const struct device	*clock_control;
	struct k_spinlock lock;
};

struct ra_internal_osc_cfg {
	/* Must be first */
	struct ra_common_osc_config common;
	uint8_t def_clock_div;
	uint8_t max_clock_div;
};

static int sysclk_driver_api_on_off(const struct device *dev,
		clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return 0;
}

static int sysclk_driver_api_get_rate(const struct device *dev,
				 clock_control_subsys_t sys,
				 uint32_t *rate)
{
	struct ra_sysclk_data *data = dev->data;
	const struct ra_sysclk_cfg *cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = clock_control_get_rate(data->clock_control, NULL, rate);

	if (!ret && sys) {
		const struct ra_common_osc_config *cmn_cfg = sys;
		unsigned int id = cmn_cfg->id;

		if (BIT(id) & (uint32_t)cfg->valid_int_clocks) {
			uint32_t divisor = sys_read32(CGC_SCKDIVCR) >> (8 * id);

			*rate >>= divisor & CGC_SCKDIVCR_Msk;
		}
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static enum clock_control_status sysclk_driver_api_get_status(const struct device *dev,
				clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);
	ARG_UNUSED(dev);

	return CLOCK_CONTROL_STATUS_ON;
}

static int sysclk_configure_internal_clk(const struct ra_internal_osc_cfg *icfg,
				  struct ra_sysclk_data *dat,
				  const struct ra_sysclk_cfg *cfg,
				  uint32_t clk_div)
{
	uint32_t val, cur;
	unsigned int id = icfg->common.id;

	if ((BIT(id) & (uint32_t)cfg->valid_int_clocks) == 0) {
		return -ENODEV;
	}

	if (!clk_div) {
		clk_div = icfg->def_clock_div;
	}

	if (!clk_div || clk_div > icfg->max_clock_div ||
			!is_power_of_two(clk_div)) {
		return -EINVAL;
	}

	K_SPINLOCK(&dat->lock) {
		val = sys_read32(CGC_SCKDIVCR);

		cur = val >> (8 * id);
		cur &= CGC_SCKDIVCR_Msk;

		if (BIT(cur) != clk_div) {
			uint16_t old_prcr;

			val &= ~(CGC_SCKDIVCR_Msk << (8 * id));
			val |= (find_lsb_set(clk_div) - 1) << (8 * id);

			old_prcr = get_register_protection();
			set_register_protection(old_prcr | SYSC_PRCR_CLK_PROT);

			sys_write32(val, CGC_SCKDIVCR);

			set_register_protection(old_prcr);
		}
	}

	return 0;
}

static int sysclk_driver_api_configure(const struct device *dev,
					  clock_control_subsys_t sys,
					  void *data)
{
	int ret = 0;
	struct ra_sysclk_data *dat = dev->data;
	const struct ra_sysclk_cfg *cfg = dev->config;
	const struct device *cctrl = (const struct device *)data;
	const struct ra_common_osc_config *cmn;

	ARG_UNUSED(sys);

	if (sys) {
		return sysclk_configure_internal_clk(
				(const struct ra_internal_osc_cfg *)sys,
				dat,
				cfg,
				POINTER_TO_UINT(data));
	}

	if (!cctrl) {
		cctrl = DEVICE_DT_GET(DT_CLOCKS_CTLR(RA_SYSCLK_NODE));
	}

	if (!cctrl) {
		return -EINVAL;
	}

	cmn = cctrl->config;
	if (!cmn || cmn->id > CGC_SCKSCR_CKSEL_MAX) {
		return -EINVAL;
	}

	K_SPINLOCK(&dat->lock) {
		if (dat->clock_control != cctrl) {
			ret = clock_control_on(cctrl, NULL);

			if (!ret) {
				unsigned int key = irq_lock();

				uint16_t old_prcr =
					get_register_protection();

				set_register_protection(old_prcr |
						SYSC_PRCR_CLK_PROT);
				sys_write8(cmn->id, CGC_SCKSCR);
				set_register_protection(old_prcr);

				irq_unlock(key);

				if (dat->clock_control) {
					clock_control_off(dat->clock_control,
								NULL);
				}

				dat->clock_control = cctrl;
			}
		}
	}

	return ret;
}

static int ra_internal_osc_driver_api_get_rate(const struct device *dev,
				 clock_control_subsys_t sys,
				 uint32_t *rate)
{
	ARG_UNUSED(sys);

	return sysclk_driver_api_get_rate(DEVICE_DT_GET(RA_SYSCLK_NODE),
			(clock_control_subsys_t)dev->config, rate);
}

static int ra_internal_osc_driver_configure(const struct device *dev,
					  clock_control_subsys_t sys,
					  void *data)
{
	const struct ra_internal_osc_cfg *icfg = dev->config;
	const struct device * const sysclk = DEVICE_DT_GET(RA_SYSCLK_NODE);

	ARG_UNUSED(sys);

	return sysclk_configure_internal_clk(icfg, sysclk->data, sysclk->config,
						POINTER_TO_UINT(data));
}

/* NOTE We boot in middle-speed mode. I'm passing to high-speed mode here,
 * but it should be done in the low-power module
 */
static int sysclk_init(const struct device *dev)
{
	int ret;
	uint32_t rate;

	const struct device * const iclk = DEVICE_DT_GET(DT_NODELABEL(iclk));
	const struct device * const pclkb = DEVICE_DT_GET(DT_NODELABEL(pclkb));
	const struct device * const pclkd = DEVICE_DT_GET(DT_NODELABEL(pclkd));

	set_memwait(true);

	/* Set iclk clock divisor to 16 */
	ret = clock_control_configure(iclk, NULL, UINT_TO_POINTER(16));
	if (ret) {
		return ret;
	}

	/* switch to the clock defined in DT, and enable it */
	ret = clock_control_configure(dev, NULL, NULL);
	if (ret) {
		/* stay with slow iclk and memwait ...*/
		return ret;
	}

	/* Go to high speed mode */
	ret = lpm_ra_set_op_mode(OM_HIGH_SPEED);
	if (ret) {
		/* Ditto ...*/
		return ret;
	}

	/* Set iclk clock to DTS defines divisor */
	ret = clock_control_configure(iclk, NULL, NULL);
	if (ret) {
		return ret;
	}

	/* Set iclk clock to DTS defines divisor */
	ret = clock_control_configure(pclkb, NULL, NULL);
	if (ret) {
		return ret;
	}

	/* Set iclk clock to DTS defines divisor */
	ret = clock_control_configure(pclkd, NULL, NULL);
	if (ret) {
		return ret;
	}

	ret = clock_control_get_rate(iclk, NULL, &rate);
	if (!ret && rate < MHZ(32)) {
		set_memwait(false);
	}

	disable_unused_root_osc();

	return 0;
}


static const struct clock_control_driver_api ra_internal_osc_driver_api = {
	.on = sysclk_driver_api_on_off,
	.off = sysclk_driver_api_on_off,
	.get_rate = ra_internal_osc_driver_api_get_rate,
	.get_status = sysclk_driver_api_get_status,
	.configure = ra_internal_osc_driver_configure,
};

#define RA_INTERNAL_OSC_INIT(inst)					       \
	static const struct ra_internal_osc_cfg ra_internal_osc_cfg##inst = {  \
		.common.id = DT_INST_REG_ADDR(inst),			       \
		.def_clock_div = DT_INST_PROP(inst, clock_div),		       \
		.max_clock_div = DT_INST_PROP(inst, max_clock_div),	       \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(inst,					       \
			NULL,						       \
			NULL, /* TODO */				       \
			NULL,						       \
			&ra_internal_osc_cfg##inst,			       \
			PRE_KERNEL_1,					       \
			CONFIG_CLOCK_CONTROL_INIT_PRIORITY,		       \
			&ra_internal_osc_driver_api);

#if defined(CONFIG_PLATFORM_SPECIFIC_INIT) && IS_ENABLED(EARLY_BOOT_HOCO_EN)
#define EARLY_NOINIT	__noinit
#else
#define EARLY_NOINIT
#endif

#define GET_CHILD_ID_MSK(node) BIT(DT_REG_ADDR(node))

static const struct ra_sysclk_cfg ra_sysclk_cfg = {
	.valid_int_clocks =
			DT_FOREACH_CHILD_STATUS_OKAY_SEP(RA_SYSCLK_NODE,
					GET_CHILD_ID_MSK, (|))
};

static EARLY_NOINIT struct ra_sysclk_data ra_data;

static const struct clock_control_driver_api sysclk_driver_api = {
	.on = sysclk_driver_api_on_off,
	.off = sysclk_driver_api_on_off,
	.get_rate = sysclk_driver_api_get_rate,
	.get_status = sysclk_driver_api_get_status,
	.configure = sysclk_driver_api_configure,
};

DEVICE_DT_DEFINE(RA_SYSCLK_NODE,
		&sysclk_init,
		NULL, /* TODO */
		&ra_data,
		&ra_sysclk_cfg,
		PRE_KERNEL_1,
		CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		&sysclk_driver_api);

#if defined(CONFIG_PLATFORM_SPECIFIC_INIT) && EARLY_BOOT_HOCO_EN
void early_boot_sysclk_setup(void)
{
	/* Don't try to change current source clock,
	 * it can be unstable/switched off.
	 * It's only safe to change internal clocks divisors here
	 */
	uint8_t id = sys_read8(CGC_SCKSCR);

	memset(&ra_data, 0, sizeof(ra_data));

	if (id != DT_REG_ADDR(DT_NODELABEL(loco)) &&
			id != DT_REG_ADDR(DT_NODELABEL(moco))) {
		return;
	}

	if (!sys_read8(CGC_HOCOCR)) {
		set_memwait(true);
		/* Try to switch to HOCO osc */
		if (clock_control_configure(
			DEVICE_DT_GET(RA_SYSCLK_NODE),
			NULL,
			(void *)DEVICE_DT_GET(DT_NODELABEL(hoco)))) {

			/* stay with slow iclk and memwait ...*/
			set_memwait(false);
			return;
		}
	}

	clock_control_configure(DEVICE_DT_GET(DT_NODELABEL(iclk)),
				NULL,
				UINT_TO_POINTER(1));

}
#endif

#undef  DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_ra2_internal_clk

DT_INST_FOREACH_STATUS_OKAY(RA_INTERNAL_OSC_INIT)

/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT hpmicro_hpm_clock

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/hpmicro_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <soc.h>
#include <hpm_common.h>
#include <hpm_clock_drv.h>
#include <hpm_pllctl_drv.h>

struct clock_control_hpmicro_config {
	PLLCTL_Type *base;
	uint32_t freq;
	uint32_t sys_core;
	uint32_t ram_up_time;
	uint32_t sysctl_present;
};

static int clock_control_hpmicro_init(const struct device *dev)
{
	const struct clock_control_hpmicro_config *config = dev->config;
	uint32_t cpu0_freq = clock_get_frequency(config->sys_core);
	uint32_t clock_group = 0;
	uint32_t cpu_id = 0;

	if (cpu0_freq == PLLCTL_SOC_PLL_REFCLK_FREQ) {
		/* Configure the External OSC ramp-up time*/
		pllctl_xtal_set_rampup_time(config->base, config->ram_up_time);

		/* Select clock setting preset1 */
		sysctl_clock_set_preset(HPM_SYSCTL, config->sysctl_present);
	}

	cpu_id = arch_proc_id();
	clock_group = cpu_id;

#if defined(CONFIG_SOC_SERIES_HPM67XX_64XX) || defined(CONFIG_SOC_SERIES_HPM62XX)
	clock_add_to_group((cpu_id == 0) ? clock_cpu0 : clock_cpu1, clock_group);
	clock_add_to_group((cpu_id == 0) ? clock_mchtmr0 : clock_mchtmr1, clock_group);
#else
	clock_add_to_group(clock_cpu0, clock_group);
	clock_add_to_group(clock_mchtmr0, clock_group);
#endif

#if defined(CONFIG_SOC_SERIES_HPM67XX_64XX)
	clock_add_to_group(clock_axi0,    clock_group);
	clock_add_to_group(clock_axi1,    clock_group);
	clock_add_to_group(clock_axi2,    clock_group);
#endif

#if defined(CONFIG_SOC_SERIES_HPM62XX) || defined(CONFIG_SOC_SERIES_HPM63XX)
	clock_add_to_group(clock_axi,    clock_group);
	clock_add_to_group(clock_axic,    clock_group);
	clock_add_to_group(clock_axis,    clock_group);
#endif

#if defined(CONFIG_SOC_SERIES_HPM68XX)
	clock_add_to_group(clock_axif,    clock_group);
	clock_add_to_group(clock_axis,    clock_group);
	clock_add_to_group(clock_axic,    clock_group);
	clock_add_to_group(clock_axiv,    clock_group);
	clock_add_to_group(clock_axig,    clock_group);
	clock_add_to_group(clock_axid,    clock_group);
#endif

	clock_add_to_group(clock_xpi0,    clock_group);
	clock_add_to_group(clock_gpio,    clock_group);

	clock_add_to_group(clock_ahb,     clock_group);
#if defined(CONFIG_SOC_SERIES_HPM62XX) || defined(CONFIG_SOC_SERIES_HPM63XX)
	clock_add_to_group(clock_ahbp,     clock_group);
#endif

	/* Connect Group to CPU */
	clock_connect_group_to_cpu(cpu_id, clock_group);

	if (status_success != pllctl_init_int_pll_with_freq(config->base, 0, config->freq)) {
		return -1;
	}

#if defined(CONFIG_SOC_SERIES_HPM67XX_64XX) || defined(CONFIG_SOC_SERIES_HPM62XX)
	clock_set_source_divider((cpu_id == 0) ? clock_cpu0 : clock_cpu1,
				 clk_src_pll0_clk0, 1);
	clock_set_source_divider((cpu_id == 0) ? clock_mchtmr0 : clock_mchtmr1,
				 clk_src_osc24m, 1);
#else
	clock_set_source_divider(clock_cpu0, clk_src_pll0_clk0, 1);
	clock_set_source_divider(clock_mchtmr0, clk_src_osc24m, 1);
#endif

	clock_set_source_divider(clock_ahb, clk_src_pll1_clk1, 2);
	clock_update_core_clock();

	/* Keep cpu clock on wfi, so that mchtmr irq can still work after wfi */
	sysctl_set_cpu_lp_mode(HPM_SYSCTL, HPM_CORE0, cpu_lp_mode_ungate_cpu_clock);

	return 0;
}

static int clock_control_hpmicro_on(const struct device *dev,
				 clock_control_subsys_t sys)
{
	struct hpm_clock_configure_data *cfg = sys;

	clock_enable(cfg->clock_name);

	return 0;
}

static int clock_control_hpmicro_off(const struct device *dev,
				  clock_control_subsys_t sys)
{
	struct hpm_clock_configure_data *cfg = sys;

	clock_disable(cfg->clock_name);

	return 0;
}

static int clock_control_hpmicro_get_rate(const struct device *dev,
					   clock_control_subsys_t sys,
					   uint32_t *rate)
{
	struct hpm_clock_configure_data *cfg = sys;

	*rate = clock_get_frequency(cfg->clock_name);

	return 0;
}

static int clock_control_hpmicro_configure(const struct device *dev,
					  clock_control_subsys_t sys,
					  void *data)
{
	uint32_t clock_group = arch_proc_id();
	struct hpm_clock_configure_data *cfg = sys;

	clock_set_source_divider(cfg->clock_name,
				 cfg->clock_src,
				 cfg->clock_div);

	clock_add_to_group(cfg->clock_name, clock_group);

	return 0;
}

static const struct clock_control_hpmicro_config config = {
	.base = (PLLCTL_Type *)DT_INST_REG_ADDR(0),
	.freq = DT_INST_PROP(0, clock_frequency),
	.sys_core = DT_INST_PROP(0, clock_sys_core),
	.ram_up_time = DT_INST_PROP(0, ram_up_time),
	.sysctl_present = DT_INST_PROP(0, sysctl_present),
};

static const struct clock_control_driver_api clock_control_hpmicro_api = {
	.on = clock_control_hpmicro_on,
	.off = clock_control_hpmicro_off,
	.get_rate = clock_control_hpmicro_get_rate,
	.configure = clock_control_hpmicro_configure,
};

DEVICE_DT_INST_DEFINE(0, clock_control_hpmicro_init, NULL, NULL, &config,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_hpmicro_api);

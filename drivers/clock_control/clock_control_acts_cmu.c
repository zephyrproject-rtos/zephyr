/*
 * Copyright (c) 2019 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT acts_cmu

#include <soc.h>
#include <drivers/clock_control.h>
#include <dt-bindings/clock/acts_cmu.h>

#define ACTS_CLOCK_32M_O 32000000
#define ACTS_CLOCK_32K_O 32000
#define ACTS_CLOCK_DIV_MASK 0x7
#define ACTS_CLOCK_SEL_MASK 0x8

#define CMU_DEVRST(base)        (base + 0x0010)
#define CMU_DEVCLKEN(base)      (base + 0x0014)
#define CMU_SPI0CLK(base)       (base + 0x0018)
#define CMU_SPI1CLK(base)       (base + 0x001c)
#define CMU_SPI2CLK(base)       (base + 0x0020)
#define CMU_PWM0CLK(base)       (base + 0x0024)
#define CMU_PWM1CLK(base)       (base + 0x0028)
#define CMU_PWM2CLK(base)       (base + 0x002c)
#define CMU_PWM3CLK(base)       (base + 0x0030)
#define CMU_PWM4CLK(base)       (base + 0x0034)
#define CMU_AUDIOCLK(base)      (base + 0x0038)
#define CMU_TIMER0CLK(base)     (base + 0x003c)
#define CMU_TIMER1CLK(base)     (base + 0x0040)
#define CMU_TIMER2CLK(base)     (base + 0x0044)
#define CMU_TIMER3CLK(base)     (base + 0x0048)

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control);

uint8_t clk_div_table[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

struct acts_cmu_config {
	uint32_t base;
};

#define DEV_CFG(dev)  ((struct acts_cmu_config *)(dev->config_info))
#define DEV_BASE(dev) (DEV_CFG(dev)->base)

static void acts_clock_peripheral_control(struct device *dev, int clock,
					  int enable)
{
	unsigned int key;
	uint32_t base = DEV_BASE(dev);

	if (clock > ACTS_CLOCK_MAX)
		return;

	key = irq_lock();

	if (enable)
		sys_write32(sys_read32(CMU_DEVCLKEN(base)) | (1 << clock),
			    CMU_DEVCLKEN(base));
	else
		sys_write32(sys_read32(CMU_DEVCLKEN(base)) & ~(1 << clock),
			    CMU_DEVCLKEN(base));

	irq_unlock(key);
}

static int acts_clock_on(struct device *dev, clock_control_subsys_t sub_system)
{
	acts_clock_peripheral_control(dev, (uint32_t)sub_system, 1);

	return 0;
}

static int acts_clock_off(struct device *dev, clock_control_subsys_t sub_system)
{
	acts_clock_peripheral_control(dev, (uint32_t)sub_system, 0);

	return 0;
}

static int acts_clock_get_rate(struct device *dev,
			       clock_control_subsys_t sub_system,
			       uint32_t *rate)
{
	uint32_t div, reg_val, clk_sel;
	uint32_t base = DEV_BASE(dev);

	switch ((uint32_t)sub_system) {
	case ACTS_CLOCK_SPI0:
		div = (sys_read32(CMU_SPI0CLK(base)) & ACTS_CLOCK_DIV_MASK);
		*rate = (ACTS_CLOCK_32M_O / div);
		break;
	case ACTS_CLOCK_SPI1:
		div = (sys_read32(CMU_SPI1CLK(base)) & ACTS_CLOCK_DIV_MASK);
		*rate = (ACTS_CLOCK_32M_O / div);
		break;
	case ACTS_CLOCK_SPI2:
		div = (sys_read32(CMU_SPI2CLK(base)) & ACTS_CLOCK_DIV_MASK);
		*rate = (ACTS_CLOCK_32M_O / div);
		break;
	/* UART clocks have a fixed rate of 16MHz */
	case ACTS_CLOCK_UART0:
		*rate = 16000000;
		break;
	case ACTS_CLOCK_UART1:
		*rate = 16000000;
		break;
	case ACTS_CLOCK_UART2:
		*rate = 16000000;
		break;
	case ACTS_CLOCK_PWM0:
		reg_val = sys_read32(CMU_PWM0CLK(base));
		clk_sel = (reg_val & ACTS_CLOCK_SEL_MASK) ? ACTS_CLOCK_32K_O :
			   ACTS_CLOCK_32M_O;
		div = reg_val & ACTS_CLOCK_DIV_MASK;
		*rate = clk_sel / clk_div_table[div];
		break;
	case ACTS_CLOCK_PWM1:
		reg_val = sys_read32(CMU_PWM1CLK(base));
		clk_sel = (reg_val & ACTS_CLOCK_SEL_MASK) ? ACTS_CLOCK_32K_O :
			   ACTS_CLOCK_32M_O;
		div = reg_val & ACTS_CLOCK_DIV_MASK;
		*rate = clk_sel / clk_div_table[div];
		break;
	case ACTS_CLOCK_PWM2:
		reg_val = sys_read32(CMU_PWM2CLK(base));
		clk_sel = (reg_val & ACTS_CLOCK_SEL_MASK) ? ACTS_CLOCK_32K_O :
			   ACTS_CLOCK_32M_O;
		div = reg_val & ACTS_CLOCK_DIV_MASK;
		*rate = clk_sel / clk_div_table[div];
		break;
	case ACTS_CLOCK_PWM3:
		reg_val = sys_read32(CMU_PWM3CLK(base));
		clk_sel = (reg_val & ACTS_CLOCK_SEL_MASK) ? ACTS_CLOCK_32K_O :
			   ACTS_CLOCK_32M_O;
		div = reg_val & ACTS_CLOCK_DIV_MASK;
		*rate = clk_sel / clk_div_table[div];
		break;
	case ACTS_CLOCK_PWM4:
		reg_val = sys_read32(CMU_PWM4CLK(base));
		clk_sel = (reg_val & ACTS_CLOCK_SEL_MASK) ? ACTS_CLOCK_32K_O :
			   ACTS_CLOCK_32M_O;
		div = reg_val & ACTS_CLOCK_DIV_MASK;
		*rate = clk_sel / clk_div_table[div];
		break;
	case ACTS_CLOCK_TIMER0:
		reg_val = sys_read32(CMU_TIMER0CLK(base));
		clk_sel = (reg_val & ACTS_CLOCK_SEL_MASK) ? ACTS_CLOCK_32K_O :
			   ACTS_CLOCK_32M_O;
		div = reg_val & ACTS_CLOCK_DIV_MASK;
		*rate = clk_sel / clk_div_table[div];
		break;
	case ACTS_CLOCK_TIMER1:
		reg_val = sys_read32(CMU_TIMER1CLK(base));
		clk_sel = (reg_val & ACTS_CLOCK_SEL_MASK) ? ACTS_CLOCK_32K_O :
			   ACTS_CLOCK_32M_O;
		div = reg_val & ACTS_CLOCK_DIV_MASK;
		*rate = clk_sel / clk_div_table[div];
		break;
	case ACTS_CLOCK_TIMER2:
		reg_val = sys_read32(CMU_TIMER2CLK(base));
		clk_sel = (reg_val & ACTS_CLOCK_SEL_MASK) ? ACTS_CLOCK_32K_O :
			   ACTS_CLOCK_32M_O;
		div = reg_val & ACTS_CLOCK_DIV_MASK;
		*rate = clk_sel / clk_div_table[div];
		break;
	case ACTS_CLOCK_TIMER3:
		reg_val = sys_read32(CMU_TIMER3CLK(base));
		clk_sel = (reg_val & ACTS_CLOCK_SEL_MASK) ? ACTS_CLOCK_32K_O :
			   ACTS_CLOCK_32M_O;
		div = reg_val & ACTS_CLOCK_DIV_MASK;
		*rate = clk_sel / clk_div_table[div];
		break;
	default:
		LOG_ERR("Clock not supported");
		break;
	}

	return 0;
}

static int acts_clock_init(struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api acts_clock_api = {
	.on = acts_clock_on,
	.off = acts_clock_off,
	.get_rate = acts_clock_get_rate,
};

static const struct acts_cmu_config acts_cmu_cfg = {
	.base = DT_INST_REG_ADDR(0)
};

DEVICE_AND_API_INIT(acts_clock0, DT_INST_LABEL(0),
		    &acts_clock_init,
		    NULL, &acts_cmu_cfg,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		    &acts_clock_api);

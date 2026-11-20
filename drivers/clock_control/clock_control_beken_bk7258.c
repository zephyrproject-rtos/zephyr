/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT beken_bk7258_cgu

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/beken_bk7258_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(clock_control_bk7258, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/*
 * The SYS block is a flat file of 32-bit registers. Offsets below are the
 * word index of the vendor documentation multiplied by four.
 */
#define SYS_CPU0_INT_HALT_CLK_OP 0x10
#define SYS_CLK_DIV_MODE1        0x20
#define SYS_DEVICE_CLK_ENABLE    0x30
#define SYS_DEVICE_CLK_ENABLE1   0x34

/* SYS_CPU0_INT_HALT_CLK_OP */
#define CPU0_SPEED BIT(4)

/* SYS_CLK_DIV_MODE1 */
#define CLKDIV_CORE  GENMASK(3, 0)
#define CKSEL_CORE   GENMASK(5, 4)
#define CLKDIV_UART0 GENMASK(9, 8)
#define CKSEL_UART0  BIT(10)
#define CLKDIV_UART1 GENMASK(12, 11)
#define CKSEL_UART1  BIT(13)
#define CLKDIV_UART2 GENMASK(15, 14)
#define CKSEL_UART2  BIT(16)

/* Highest clock that has a gate bit */
#define BK7258_CLK_GATED_MAX BK7258_CLK_CIS_AUXS

/* Gates from this one on live in the second enable register */
#define BK7258_CLK_ENABLE1_FIRST BK7258_CLK_H264

/*
 * Sources the core clock can be switched to, indexed by cksel_core: the
 * crystal, whose rate comes from devicetree, the digitally controlled
 * oscillator, and the two outputs of the DPLL.
 */
#define CKSEL_CORE_XTAL 0
#define CKSEL_CORE_DCO  1
#define CKSEL_CORE_320M 2

#define CORE_SRC_320M MHZ(320)
#define CORE_SRC_480M MHZ(480)

struct bk7258_cgu_config {
	uintptr_t base;
	uint32_t xtal_frequency;
};

static struct k_spinlock bk7258_cgu_lock;

static int bk7258_core_clock(const struct device *dev, uint32_t mode1, uint32_t *rate)
{
	const struct bk7258_cgu_config *config = dev->config;
	uint32_t source;

	switch (FIELD_GET(CKSEL_CORE, mode1)) {
	case CKSEL_CORE_XTAL:
		source = config->xtal_frequency;
		break;
	case CKSEL_CORE_DCO:
		/*
		 * 26 to 360 MHz; registers hold the calibration target,
		 * not the rate reached
		 */
		return -ENOTSUP;
	case CKSEL_CORE_320M:
		source = CORE_SRC_320M;
		break;
	default:
		source = CORE_SRC_480M;
		break;
	}

	*rate = source / (FIELD_GET(CLKDIV_CORE, mode1) + 1U);

	return 0;
}

/* CPU0 runs at the core clock, or at half of it when cpu0_speed is clear */
static int bk7258_cpu0_clock(const struct device *dev, uint32_t mode1, uint32_t *rate)
{
	const struct bk7258_cgu_config *config = dev->config;
	int ret;

	ret = bk7258_core_clock(dev, mode1, rate);
	if (ret < 0) {
		return ret;
	}

	if ((sys_read32(config->base + SYS_CPU0_INT_HALT_CLK_OP) & CPU0_SPEED) == 0U) {
		*rate /= 2U;
	}

	return 0;
}

/* Source select and divider fields of a UART clock, or 0 for any other clock */
static uint32_t bk7258_uart_clock_fields(uint32_t id)
{
	switch (id) {
	case BK7258_CLK_UART0:
		return CKSEL_UART0 | CLKDIV_UART0;
	case BK7258_CLK_UART1:
		return CKSEL_UART1 | CLKDIV_UART1;
	case BK7258_CLK_UART2:
		return CKSEL_UART2 | CLKDIV_UART2;
	default:
		return 0U;
	}
}

static int bk7258_cgu_gate(const struct device *dev, uint32_t id, bool enable)
{
	const struct bk7258_cgu_config *config = dev->config;
	uintptr_t reg = config->base + SYS_DEVICE_CLK_ENABLE;
	uint32_t uart_fields = bk7258_uart_clock_fields(id);
	k_spinlock_key_t key;

	/* Handles the UART clock gates */
	if (uart_fields == 0U) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&bk7258_cgu_lock);

	if (enable) {
		sys_clear_bits(config->base + SYS_CLK_DIV_MODE1, uart_fields);
		sys_set_bits(reg, BIT(id));
	} else {
		sys_clear_bits(reg, BIT(id));
	}

	k_spin_unlock(&bk7258_cgu_lock, key);

	return 0;
}

static int bk7258_cgu_on(const struct device *dev, clock_control_subsys_t sys)
{
	return bk7258_cgu_gate(dev, (uint32_t)(uintptr_t)sys, true);
}

static int bk7258_cgu_off(const struct device *dev, clock_control_subsys_t sys)
{
	return bk7258_cgu_gate(dev, (uint32_t)(uintptr_t)sys, false);
}

static enum clock_control_status bk7258_cgu_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	const struct bk7258_cgu_config *config = dev->config;
	uint32_t id = (uint32_t)(uintptr_t)sys;
	uintptr_t reg = config->base + SYS_DEVICE_CLK_ENABLE;

	if (id >= BK7258_CLK_XTAL && id <= BK7258_CLK_CPU0) {
		/* A clock with no gate is always running */
		return CLOCK_CONTROL_STATUS_ON;
	}

	if (id > BK7258_CLK_GATED_MAX) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (id >= BK7258_CLK_ENABLE1_FIRST) {
		reg = config->base + SYS_DEVICE_CLK_ENABLE1;
		id -= BK7258_CLK_ENABLE1_FIRST;
	}

	return (sys_read32(reg) & BIT(id)) ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
}

static int bk7258_uart_clock(const struct device *dev, uint32_t mode1, uint32_t id,
			     uint32_t *rate)
{
	const struct bk7258_cgu_config *config = dev->config;
	uint32_t divider;
	bool from_pll;

	switch (id) {
	case BK7258_CLK_UART0:
		from_pll = (mode1 & CKSEL_UART0) != 0U;
		divider = FIELD_GET(CLKDIV_UART0, mode1);
		break;
	case BK7258_CLK_UART1:
		from_pll = (mode1 & CKSEL_UART1) != 0U;
		divider = FIELD_GET(CLKDIV_UART1, mode1);
		break;
	default:
		from_pll = (mode1 & CKSEL_UART2) != 0U;
		divider = FIELD_GET(CLKDIV_UART2, mode1);
		break;
	}

	/*
	 * The audio PLL is the other source a UART can take. It starts at
	 * 98.304 MHz but audio sample rates change it, so its rate is not
	 * modelled here.
	 */
	if (from_pll) {
		return -ENOTSUP;
	}

	/* Unlike the core divider, this one divides by a power of two */
	*rate = config->xtal_frequency >> divider;

	return 0;
}

static int bk7258_cgu_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	const struct bk7258_cgu_config *config = dev->config;
	uint32_t id = (uint32_t)(uintptr_t)sys;
	uint32_t mode1 = sys_read32(config->base + SYS_CLK_DIV_MODE1);

	switch (id) {
	case BK7258_CLK_XTAL:
		*rate = config->xtal_frequency;
		return 0;
	case BK7258_CLK_CORE:
		return bk7258_core_clock(dev, mode1, rate);
	case BK7258_CLK_CPU0:
		return bk7258_cpu0_clock(dev, mode1, rate);
	case BK7258_CLK_UART0:
	case BK7258_CLK_UART1:
	case BK7258_CLK_UART2:
		return bk7258_uart_clock(dev, mode1, id, rate);
	default:
		return -ENOTSUP;
	}
}

static int bk7258_cgu_init(const struct device *dev)
{
	const struct bk7258_cgu_config *config = dev->config;
	uint32_t mode1 = sys_read32(config->base + SYS_CLK_DIV_MODE1);
	uint32_t core;
	uint32_t cpu0;

	/*
	 * Nothing is programmed here: the core runs at whatever the BootROM
	 * left behind. Report it, and say so when the kernel has been told
	 * something else, because SysTick counts this clock and every
	 * timeout in the system is scaled by the ratio between the two.
	 */
	if (bk7258_core_clock(dev, mode1, &core) < 0 ||
	    bk7258_cpu0_clock(dev, mode1, &cpu0) < 0) {
		LOG_WRN("core runs from the DCO, whose rate is unknown; "
			"timeouts may be wrong");
		return 0;
	}

	LOG_INF("core %u Hz, cpu0 %u Hz", core, cpu0);

	if (cpu0 != CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		LOG_WRN("cpu0 runs at %u Hz but the kernel was built for %u Hz; "
			"timeouts are wrong by a factor of %u/%u",
			cpu0, CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, cpu0);
	}

	return 0;
}

static DEVICE_API(clock_control, bk7258_cgu_driver_api) = {
	.on = bk7258_cgu_on,
	.off = bk7258_cgu_off,
	.get_rate = bk7258_cgu_get_rate,
	.get_status = bk7258_cgu_get_status,
};

#define BK7258_CGU_INIT(n)                                                        \
	static const struct bk7258_cgu_config bk7258_cgu_config_##n = {            \
		.base = DT_INST_REG_ADDR(n),                                       \
		.xtal_frequency = DT_INST_PROP(n, clock_frequency),                \
	};                                                                         \
                                                                                   \
	DEVICE_DT_INST_DEFINE(n, bk7258_cgu_init, NULL, NULL,                      \
			      &bk7258_cgu_config_##n, PRE_KERNEL_1,                \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                  \
			      &bk7258_cgu_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BK7258_CGU_INIT)

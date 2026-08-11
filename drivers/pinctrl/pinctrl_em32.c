/*
 * SPDX-FileCopyrightText: 2026 ELAN Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/dt-bindings/pinctrl/em32f967-pinctrl.h>
#include <soc.h>

LOG_MODULE_REGISTER(pinctrl_em32);

static const struct device *const em32_syscon = DEVICE_DT_GET(DT_NODELABEL(sysctrl));

#define PINMUX_IO1_VALID_BIT 8U
#define PINMUX_IO1_SHIFT_POS 9U
#define PINMUX_IO1_WIDTH_POS 14U
#define PINMUX_IO1_VAL_POS   16U
#define PINMUX_IO2_VALID_BIT 20U
#define PINMUX_IO2_SHIFT_POS 21U
#define PINMUX_IO2_WIDTH_POS 26U
#define PINMUX_IO2_VAL_POS   28U

/* Sysctrl-relative offsets (passed as the `reg` argument to syscon_update_bits,
 * which adds the syscon base from DTS: syscon@40030000)
 */
#define EM32_IOSHARE_OFFSET       0x23CU
#define EM32_IOMUXPACTRL_OFFSET   0x200U /* PA[7:0]  control */
#define EM32_IOMUXPACTRL2_OFFSET  0x204U /* PA[15:8] control */
#define EM32_IOMUXPBCTRL_OFFSET   0x208U /* PB[7:0]  control */
#define EM32_IOMUXPBCTRL2_OFFSET  0x20CU /* PB[15:8] control */
#define EM32_IOPUPACTRL_OFFSET    0x214U /* PA pull control    */
#define EM32_IOPUPBCTRL_OFFSET    0x218U /* PB pull control    */
#define EM32_IO_HD_PA_CTRL_OFFSET 0x21CU /* PA high drive      */
#define EM32_IO_HD_PB_CTRL_OFFSET 0x220U /* PB high drive      */
#define EM32_IOODEPACTRL_OFFSET   0x22CU /* PA open drain      */
#define EM32_IOODEPBCTRL_OFFSET   0x230U /* PB open drain      */

/* GPIO register offsets (only the two pinctrl needs) */
#define EM32_GPIO_ALTFUNCSET_OFFSET 0x18U
#define EM32_GPIO_ALTFUNCCLR_OFFSET 0x1CU
#define EM32_MAX_PORTS              2U
#define EM32_MAX_ALT_FUNC           7U
#define EM32_PORT_A                 0U
#define EM32_PORT_B                 1U

#define EM32_GPIO_BASE_ENTRY(node_id) [DT_PROP(node_id, port_id)] = DT_REG_ADDR(node_id),

static const uintptr_t em32_gpio_base[EM32_MAX_PORTS] = {
	DT_FOREACH_STATUS_OKAY(elan_em32_gpio, EM32_GPIO_BASE_ENTRY)};

/* Pin Configuration Macros */
#define EM32_DT_GET_PORT(pinmux) EM32F967_DT_PINMUX_PORT(pinmux)
#define EM32_DT_GET_PIN(pinmux)  EM32F967_DT_PINMUX_PIN(pinmux)
#define EM32_DT_GET_FUNC(pinmux) EM32F967_DT_PINMUX_MUX(pinmux)

/* Pincfg bit-field shifts (from pinctrl_soc.h) */
#define PINCFG_PUPDR_SHIFT  EM32_PUPDR_SHIFT
#define PINCFG_OTYPER_SHIFT EM32_OTYPER_SHIFT
#define PINCFG_DRIVE_SHIFT  EM32_DRIVE_SHIFT

struct em32_ioshare_config {
	uint8_t port;
	uint8_t pin_start;
	uint8_t pin_end;
	uint32_t alt_func;
	uint32_t bit_pos;
	uint32_t bit_mask;
	uint32_t bit_value;
	const char *peripheral;
};

static const struct em32_ioshare_config em32_ioshare_table[] = {
	/* UART configurations */
	{EM32_PORT_A, 1, 2, EM32F967_AF2, EM32_IP_SHARE_UART1, BIT(EM32_IP_SHARE_UART1),
	 BIT(EM32_IP_SHARE_UART1), "UART1"},
	{EM32_PORT_A, 4, 5, EM32F967_AF2, EM32_IP_SHARE_UART2, BIT(EM32_IP_SHARE_UART2),
	 BIT(EM32_IP_SHARE_UART2), "UART2"},
	{EM32_PORT_B, 8, 9, EM32F967_AF2, EM32_IP_SHARE_UART1, BIT(EM32_IP_SHARE_UART1), 0,
	 "UART1_ALT"},

	/* SPI configurations */
	{EM32_PORT_B, 0, 3, EM32F967_AF1, EM32_IP_SHARE_SPI1_SHIFT,
	 0x3U << EM32_IP_SHARE_SPI1_SHIFT, 0x0U << EM32_IP_SHARE_SPI1_SHIFT, "SPI1"},

	{EM32_PORT_B, 4, 7, EM32F967_AF2, EM32_IP_SHARE_SSP2_SHIFT,
	 0x3U << EM32_IP_SHARE_SSP2_SHIFT, 0x0U << EM32_IP_SHARE_SSP2_SHIFT, "SSP2"},

	/* I2C configurations */
	{EM32_PORT_A, 4, 5, EM32F967_AF4, EM32_IP_SHARE_I2C2, BIT(EM32_IP_SHARE_I2C2), 0, "I2C2"},
	{EM32_PORT_B, 0, 1, EM32F967_AF5, EM32_IP_SHARE_I2C1, BIT(EM32_IP_SHARE_I2C1),
	 BIT(EM32_IP_SHARE_I2C1), "I2C1"},

	/* PWM on Port A (PWM_S=0)
	 * PA3-PA5 with AF7 require IP_Share[1:0]=2 to release pins from SPI1 and
	 * IP_Share[18]=0 (PWM_S=0) for Port A PWM routing.
	 */
	{EM32_PORT_A, 3, 5, EM32F967_AF7, EM32_IP_SHARE_SPI1_SHIFT,
	 (0x3U << EM32_IP_SHARE_SPI1_SHIFT) | BIT(EM32_IP_SHARE_PWM),
	 (0x2U << EM32_IP_SHARE_SPI1_SHIFT) | 0, "PWM_PA"},

	/* PWM on Port B (PWM_S=1) */
	{EM32_PORT_B, 10, 15, EM32F967_AF1, EM32_IP_SHARE_PWM, BIT(EM32_IP_SHARE_PWM),
	 BIT(EM32_IP_SHARE_PWM), "PWM_PB"},

};

static int em32_apply_ioshare_from_pinmux(uint32_t pinmux, bool *applied)
{
	int ret;

	*applied = false;

	if (pinmux & BIT(PINMUX_IO1_VALID_BIT)) {
		uint32_t shift = (pinmux >> PINMUX_IO1_SHIFT_POS) & 0x1FU;
		uint32_t width = ((pinmux >> PINMUX_IO1_WIDTH_POS) & 0x3U) + 1U;
		uint32_t val = (pinmux >> PINMUX_IO1_VAL_POS) & 0xFU;
		uint32_t mask = ((1U << width) - 1U) << shift;

		ret = syscon_update_bits(em32_syscon, EM32_IOSHARE_OFFSET, mask,
					 (val << shift) & mask);
		if (ret < 0) {
			return ret;
		}
		LOG_DBG("IOShare op1: shift=%d width=%d val=%d", shift, width, val);
		*applied = true;
	}

	if (pinmux & BIT(PINMUX_IO2_VALID_BIT)) {
		uint32_t shift = (pinmux >> PINMUX_IO2_SHIFT_POS) & 0x1FU;
		uint32_t width = ((pinmux >> PINMUX_IO2_WIDTH_POS) & 0x3U) + 1U;
		uint32_t val = (pinmux >> PINMUX_IO2_VAL_POS) & 0xFU;
		uint32_t mask = ((1U << width) - 1U) << shift;

		ret = syscon_update_bits(em32_syscon, EM32_IOSHARE_OFFSET, mask,
					 (val << shift) & mask);
		if (ret < 0) {
			return ret;
		}
		LOG_DBG("IOShare op2: shift=%d width=%d val=%d", shift, width, val);
		*applied = true;
	}

	return 0;
}

static int em32_configure_ioshare(uint8_t port, uint8_t pin_num, uint32_t alt_func)
{
	bool config_found = false;
	int ret;

	if (port >= EM32_MAX_PORTS) {
		LOG_ERR("Invalid port: %d", port);
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(em32_ioshare_table); i++) {
		const struct em32_ioshare_config *cfg = &em32_ioshare_table[i];

		if (cfg->port == port && pin_num >= cfg->pin_start && pin_num <= cfg->pin_end &&
		    cfg->alt_func == alt_func) {

			ret = syscon_update_bits(em32_syscon, EM32_IOSHARE_OFFSET, cfg->bit_mask,
						 cfg->bit_value);
			if (ret < 0) {
				return ret;
			}

			LOG_DBG("Configured %s on P%c%d", cfg->peripheral, 'A' + port, pin_num);
			config_found = true;
			break;
		}
	}

	if (!config_found) {
		/*
		 * No table entry for this pin/function. That is normal for plain
		 * GPIO pins (and any AF that needs no IP_SHARE routing), so it is
		 * "nothing to do", not an error — return success.
		 */
		LOG_DBG("No IOShare config needed for P%c%d AF%d", 'A' + port, pin_num, alt_func);
		return 0;
	}

	return 0;
}

static int em32_pinctrl_set_mux(uint8_t port, uint8_t pin, uint8_t mux)
{
	uint32_t offset;
	uint32_t shift = (uint32_t)(pin % 8U) * 4U;
	uint32_t mask = 0xFU << shift;

	if (port == EM32_PORT_A) {
		offset = (pin < 8U) ? EM32_IOMUXPACTRL_OFFSET : EM32_IOMUXPACTRL2_OFFSET;
	} else {
		offset = (pin < 8U) ? EM32_IOMUXPBCTRL_OFFSET : EM32_IOMUXPBCTRL2_OFFSET;
	}

	return syscon_update_bits(em32_syscon, offset, mask, ((uint32_t)mux & 0xFU) << shift);
}

static void em32_pinctrl_set_altfunc(uint8_t port, uint8_t pin, uint8_t mux)
{
	uintptr_t base = em32_gpio_base[port];
	uint32_t pin_mask = BIT(pin);

	if (base == 0U) {
		return;
	}

	if (mux == 0U) {
		sys_write32(pin_mask, (uint32_t)(base + EM32_GPIO_ALTFUNCCLR_OFFSET));
	} else {
		sys_write32(pin_mask, (uint32_t)(base + EM32_GPIO_ALTFUNCSET_OFFSET));
	}
}

static int em32_pinctrl_set_pull(uint8_t port, uint8_t pin, uint32_t pupd)
{
	uint32_t offset = (port == EM32_PORT_A) ? EM32_IOPUPACTRL_OFFSET : EM32_IOPUPBCTRL_OFFSET;
	uint32_t shift = (uint32_t)pin * 2U;
	uint32_t mask = 0x3U << shift;

	return syscon_update_bits(em32_syscon, offset, mask, (pupd & 0x3U) << shift);
}

static int em32_pinctrl_set_open_drain(uint8_t port, uint8_t pin, bool od)
{
	uint32_t offset = (port == EM32_PORT_A) ? EM32_IOODEPACTRL_OFFSET : EM32_IOODEPBCTRL_OFFSET;

	return syscon_update_bits(em32_syscon, offset, BIT(pin), od ? BIT(pin) : 0U);
}

static int em32_pinctrl_set_drive(uint8_t port, uint8_t pin, bool high_drive)
{
	uint32_t offset =
		(port == EM32_PORT_A) ? EM32_IO_HD_PA_CTRL_OFFSET : EM32_IO_HD_PB_CTRL_OFFSET;

	return syscon_update_bits(em32_syscon, offset, BIT(pin), high_drive ? BIT(pin) : 0U);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	int ret;

	if (pins == NULL) {
		LOG_ERR("Pin configuration array unspecified");
		return -EINVAL;
	}

	if (pin_cnt == 0) {
		return 0;
	}

	if (!device_is_ready(em32_syscon)) {
		LOG_ERR("sysctrl syscon device not ready");
		return -ENODEV;
	}

	LOG_DBG("Configuring %d pins", pin_cnt);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		uint32_t pinmux = pins[i].pinmux;
		uint32_t pin_cfg = pins[i].pincfg;

		uint8_t port = EM32_DT_GET_PORT(pinmux);
		uint8_t pin_num = EM32_DT_GET_PIN(pinmux);
		uint32_t alt_func = EM32_DT_GET_FUNC(pinmux);

		if (port >= EM32_MAX_PORTS) {
			LOG_ERR("Pin %d: invalid port %d", i, port);
			return -EINVAL;
		}
		if (alt_func > EM32_MAX_ALT_FUNC) {
			LOG_ERR("Pin %d: invalid AF %d", i, alt_func);
			return -EINVAL;
		}

		LOG_DBG("P%c%d AF%d (pinmux=0x%08X, cfg=0x%08X)", 'A' + port, pin_num, alt_func,
			pinmux, pin_cfg);

		uint32_t pupd = (pin_cfg >> PINCFG_PUPDR_SHIFT) & 0x3U;
		bool od = ((pin_cfg >> PINCFG_OTYPER_SHIFT) & 0x1U) != 0U;
		bool hd = ((pin_cfg >> PINCFG_DRIVE_SHIFT) & 0x1U) != 0U;

		ret = em32_pinctrl_set_mux(port, pin_num, (uint8_t)alt_func);
		if (ret < 0) {
			LOG_ERR("Mux failed on P%c%d: %d", 'A' + port, pin_num, ret);
			return ret;
		}

		/* Alternate-function select writes a GPIO register directly (no syscon). */
		em32_pinctrl_set_altfunc(port, pin_num, (uint8_t)alt_func);

		ret = em32_pinctrl_set_pull(port, pin_num, pupd);
		if (ret < 0) {
			LOG_ERR("Pull failed on P%c%d: %d", 'A' + port, pin_num, ret);
			return ret;
		}

		ret = em32_pinctrl_set_open_drain(port, pin_num, od);
		if (ret < 0) {
			LOG_ERR("Open-drain failed on P%c%d: %d", 'A' + port, pin_num, ret);
			return ret;
		}

		ret = em32_pinctrl_set_drive(port, pin_num, hd);
		if (ret < 0) {
			LOG_ERR("Drive failed on P%c%d: %d", 'A' + port, pin_num, ret);
			return ret;
		}

		bool applied;

		ret = em32_apply_ioshare_from_pinmux(pinmux, &applied);
		if (ret < 0) {
			LOG_ERR("IOShare failed on P%c%d: %d", 'A' + port, pin_num, ret);
			return ret;
		}
		if (!applied) {
			ret = em32_configure_ioshare(port, pin_num, alt_func);
			if (ret < 0) {
				LOG_ERR("IOShare failed on P%c%d: %d", 'A' + port, pin_num, ret);
				return ret;
			}
		}
	}

	LOG_DBG("Configured %d pins OK", pin_cnt);
	return 0;
}

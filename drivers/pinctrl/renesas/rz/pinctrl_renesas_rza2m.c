/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rza2m_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rza2m.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>

#define RZA2M_PINCTRL_REG  DT_REG_ADDR(DT_NODELABEL(pinctrl))
#define RZA2M_PINCTRL_SIZE DT_REG_SIZE(DT_NODELABEL(pinctrl))

static struct rza2m_pinctrl_data {
	mm_reg_t base_addr;
	struct k_mutex lock;
} rza2m_pinctrl_data;

#define RZA2M_PDR(port)      (rza2m_pinctrl_data.base_addr + 0x0000 + (port) * 2)
#define RZA2M_PMR(port)      (rza2m_pinctrl_data.base_addr + 0x0080 + (port))
#define RZA2M_DSCR(port)     (rza2m_pinctrl_data.base_addr + 0x0140 + (port * 2))
#define RZA2M_PFS(port, pin) (rza2m_pinctrl_data.base_addr + 0x0200 + ((port) * 8) + (pin))
#define RZA2M_PPOC           (rza2m_pinctrl_data.base_addr + 0x0900)
#define RZA2M_PSDMMC0        (rza2m_pinctrl_data.base_addr + 0x0920)
#define RZA2M_PSDMMC1        (rza2m_pinctrl_data.base_addr + 0x0930)
#define RZA2M_PSDMMC2        (rza2m_pinctrl_data.base_addr + 0x0940)
#define RZA2M_PSPIBSC        (rza2m_pinctrl_data.base_addr + 0x0960)
#define RZA2M_PCKIO          (rza2m_pinctrl_data.base_addr + 0x09D0)
#define RZA2M_PWPR           (rza2m_pinctrl_data.base_addr + 0x02FF)

#define RZA2M_PWPR_PFSWE BIT(6) /* PFSWE Bit Enable */
#define RZA2M_PWPR_B0WI  BIT(7) /* PFSWE Bit Disable */

#define RZA2M_PDR_INPUT  (0x02)
#define RZA2M_PDR_OUTPUT (0x03)
#define RZA2M_PDR_MASK   (0x03)

#define RZA2M_DSCR_PIN_DRV_MASK (0x03)

#define RZA2M_PSDMMC0_MASK (0x3FFF)
#define RZA2M_PSDMMC1_MASK (0x7FF)
#define RZA2M_PSDMMC2_MASK (0x3FFF)

#define RZA2M_PIN_CURRENT_2mA  (0u)
#define RZA2M_PIN_CURRENT_8mA  (1u)
#define RZA2M_PIN_CURRENT_12mA (2u)

#define RZA2M_PPOC_POC0          (0x00000001u)
#define RZA2M_PPOC_POC0_SHIFT    (0u)
#define RZA2M_PPOC_POC2          (0x00000004u)
#define RZA2M_PPOC_POC2_SHIFT    (2u)
#define RZA2M_PPOC_POC3          (0x00000008u)
#define RZA2M_PPOC_POC3_SHIFT    (3u)
#define RZA2M_PPOC_POCSEL0       (0x00000100u)
#define RZA2M_PPOC_POCSEL0_SHIFT (8u)

/* Implemented pins */
static const uint8_t valid_gpio_support[] = {0x7F, 0x1F, 0x0F, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF,
					     0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0x7F, 0xFF,
					     0xFF, 0x7F, 0xFF, 0x3F, 0x1F, 0x03};

/* DSCR supported pins for 8mA */
static const uint8_t valid_gpio_dscr_8ma_support[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x7F, 0x00, 0x00, 0x00};

/* DSCR supported pins for 2mA */
static const uint8_t valid_gpio_dscr_2ma_support[] = {
	0x7F, 0x1F, 0x0F, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x3F, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF, 0x3F, 0x1F, 0x00};

static bool rza2m_pin_function_check(const uint8_t *check_array, uint8_t port, uint8_t pin)
{
	bool ret;

	ret = true;
	if ((check_array[port] & (1u << pin)) == 0) {
		ret = false;
	}

	return (ret);
}

static int rza2m_set_output_current_pin(const pinctrl_soc_pin_t pin)
{
	uint16_t reg_16;
	uint16_t dscr;

	dscr = 0;

	if (!rza2m_pin_function_check(valid_gpio_support, pin.port, pin.pin)) {
		return -EINVAL;
	}

	if (pin.drive_strength == RZA2M_PIN_CURRENT_2mA) {
		if (!rza2m_pin_function_check(valid_gpio_dscr_2ma_support, pin.port, pin.pin)) {
			return -EINVAL;
		}
		dscr = 1;
	} else if (pin.drive_strength == RZA2M_PIN_CURRENT_8mA) {
		if (!rza2m_pin_function_check(valid_gpio_dscr_8ma_support, pin.port, pin.pin)) {
			return -EINVAL;
		}
		dscr = 3;
	} else {
		return -EINVAL;
	}

	/* Write to DCSR */
	reg_16 = sys_read16(RZA2M_DSCR(pin.port));
	reg_16 &= ~(RZA2M_DSCR_PIN_DRV_MASK << (pin.pin * 2));
	reg_16 |= (dscr << pin.pin * 2);
	sys_write16(reg_16, RZA2M_DSCR(pin.port));

	return 0;
}

static int rza2m_set_output_current_ckio(uint8_t drive_strength)
{
	uint8_t ckio_drv;

	if (drive_strength == RZA2M_PIN_CURRENT_8mA) {
		ckio_drv = 1;
	} else if (drive_strength == RZA2M_PIN_CURRENT_12mA) {
		ckio_drv = 2;
	} else {
		return -EINVAL;
	}

	sys_write8(ckio_drv, RZA2M_PCKIO);

	return 0;
}

static void rza2m_update_pspibsc(void)
{
	uint32_t pocsel0_poc0;
	uint32_t reg_32;

	pocsel0_poc0 = sys_read32(RZA2M_PPOC);
	pocsel0_poc0 &= (RZA2M_PPOC_POCSEL0 | RZA2M_PPOC_POC0);

	if (pocsel0_poc0 == (RZA2M_PPOC_POCSEL0 | RZA2M_PPOC_POC0)) {
		/* 3.3v */
		reg_32 = 0x5555555;
	} else {
		/* 1.8v */
		reg_32 = 0xFFFFFFF;
	}

	sys_write32(reg_32, RZA2M_PSPIBSC);
}

static void rza2m_update_drv_sdmmc0(void)
{
	uint32_t reg_32;
	uint32_t poc2;
	uint32_t psdmmc0_val;
	uint32_t psdmmc1_val;

	poc2 = sys_read32(RZA2M_PPOC);
	poc2 &= RZA2M_PPOC_POC2;

	if (poc2 == RZA2M_PPOC_POC2) {
		/* 3.3 V */
		/* TDSEL = 0b11, other fields = 0b10 */
		psdmmc0_val = 0x3AAA;
		psdmmc1_val = 0x2AA;
	} else {
		/* 1.8 V */
		/* TDSEL = 0b01, other fields = 0b11 */
		psdmmc0_val = 0x1FFF;
		psdmmc1_val = 0x3FF;
	}

	/* Read and write PSDMMC0 */
	reg_32 = sys_read32(RZA2M_PSDMMC0);
	reg_32 &= ~(RZA2M_PSDMMC0_MASK);
	reg_32 |= psdmmc0_val;
	sys_write32(reg_32, RZA2M_PSDMMC0);

	/* Read and write PSDMMC1 */
	reg_32 = sys_read32(RZA2M_PSDMMC1);
	reg_32 &= ~(RZA2M_PSDMMC1_MASK);
	reg_32 |= psdmmc1_val;
	sys_write32(reg_32, RZA2M_PSDMMC1);
}

static void rza2m_update_drv_sdmmc1(void)
{
	uint32_t reg_32;
	uint32_t poc3;
	uint32_t psdmmc2_val;

	poc3 = sys_read32(RZA2M_PPOC);
	poc3 &= RZA2M_PPOC_POC3;

	if (poc3 == RZA2M_PPOC_POC3) {
		/* 3.3 V */
		/* TDSEL = 0b11, other fields = 0b10 */
		psdmmc2_val = 0x3AAA;
	} else {
		/* 1.8 V */
		/* TDSEL = 0b01, other fields = 0b11 */
		psdmmc2_val = 0x1FFF;
	}

	/* Read and write PSDMMC2 */
	reg_32 = sys_read32(RZA2M_PSDMMC2);
	reg_32 &= ~(RZA2M_PSDMMC2_MASK);
	reg_32 |= psdmmc2_val;
	sys_write32(reg_32, RZA2M_PSDMMC2);
}

static int rza2m_set_ppoc(const pinctrl_soc_pin_t pin)
{
	uint32_t reg_32;
	uint32_t ppoc_val;
	int ret;

	ret = 0;
	k_mutex_lock(&rza2m_pinctrl_data.lock, K_FOREVER);

	switch (pin.pin) {
	case PIN_POSEL:
		ppoc_val = ((pin.func & 0x1) << RZA2M_PPOC_POC0_SHIFT) |
			   (((pin.func & 0x2) >> 1) << RZA2M_PPOC_POCSEL0_SHIFT);

		/* Set POC0 and POCSEL0 */
		reg_32 = sys_read32(RZA2M_PPOC);
		reg_32 &= ~(RZA2M_PPOC_POC0 & RZA2M_PPOC_POCSEL0);
		reg_32 |= ppoc_val;
		sys_write32(reg_32, RZA2M_PPOC);

		rza2m_update_pspibsc();
		break;

	case PIN_POC2:
		ppoc_val = ((pin.func & 0x1) << RZA2M_PPOC_POC2_SHIFT);

		/* Set POC2 */
		reg_32 = sys_read32(RZA2M_PPOC);
		reg_32 &= ~(RZA2M_PPOC_POC2);
		reg_32 |= ppoc_val;
		sys_write32(reg_32, RZA2M_PPOC);

		rza2m_update_drv_sdmmc0();
		break;

	case PIN_POC3:
		ppoc_val = ((pin.func & 0x1) << RZA2M_PPOC_POC3_SHIFT);

		/* Set POC3 */
		reg_32 = sys_read32(RZA2M_PPOC);
		reg_32 &= ~(RZA2M_PPOC_POC3);
		reg_32 |= ppoc_val;
		sys_write32(reg_32, RZA2M_PPOC);

		rza2m_update_drv_sdmmc1();
		break;

	default:
		ret = -EINVAL;
	}

	k_mutex_unlock(&rza2m_pinctrl_data.lock);

	return ret;
}

/* PFS Register Write Protect : OFF */
static void rza2m_unprotect_pin_mux(void)
{
	uint8_t reg_8;

	/* Set B0WI to 0 */
	reg_8 = sys_read8(RZA2M_PWPR);
	reg_8 &= ~RZA2M_PWPR_B0WI;
	sys_write8(reg_8, RZA2M_PWPR);

	/* Set PFSWE to 1 */
	reg_8 = sys_read8(RZA2M_PWPR);
	reg_8 |= RZA2M_PWPR_PFSWE;
	sys_write8(reg_8, RZA2M_PWPR);
}

/* PFS Register Write Protect : ON */
static void rza2m_protect_pin_mux(void)
{
	uint8_t reg_8;

	/* Set PFSWE to 0 */
	reg_8 = sys_read8(RZA2M_PWPR);
	reg_8 &= ~RZA2M_PWPR_PFSWE;
	sys_write8(reg_8, RZA2M_PWPR);

	/* Set B0WI to 1 */
	reg_8 = sys_read8(RZA2M_PWPR);
	reg_8 |= RZA2M_PWPR_B0WI;
	sys_write8(reg_8, RZA2M_PWPR);
}

static int rza2m_set_pin_hiz(uint8_t port, uint8_t pin)
{
	uint16_t mask_16;
	uint16_t reg_16;
	uint8_t reg_8;

	if (!rza2m_pin_function_check(valid_gpio_support, port, pin)) {
		return -EINVAL;
	}

	k_mutex_lock(&rza2m_pinctrl_data.lock, K_FOREVER);

	/* Set pin to Hi-z input protection */
	reg_16 = sys_read16(RZA2M_PDR(port));
	mask_16 = RZA2M_PDR_MASK << (pin * 2);
	reg_16 &= ~mask_16;
	sys_write16(reg_16, RZA2M_PDR(port));

	rza2m_unprotect_pin_mux();

	/* Set Pin function to 0 */
	reg_8 = sys_read8(RZA2M_PFS(port, pin));
	reg_8 &= ~(RZA2M_MUX_FUNC_MAX);
	sys_write8(reg_8, RZA2M_PFS(port, pin));

	rza2m_protect_pin_mux();

	/* Switch to GPIO */
	reg_8 = sys_read8(RZA2M_PMR(port));
	reg_8 &= ~BIT(pin);
	sys_write8(reg_8, RZA2M_PMR(port));

	k_mutex_unlock(&rza2m_pinctrl_data.lock);

	return 0;
}

static int rza2m_pin_to_gpio(uint8_t port, uint8_t pin, uint8_t dir)
{
	uint16_t mask_16;
	uint16_t reg_16;
	uint8_t reg_8;

	if (!rza2m_pin_function_check(valid_gpio_support, port, pin)) {
		return -EINVAL;
	}

	k_mutex_lock(&rza2m_pinctrl_data.lock, K_FOREVER);

	/* Set pin to Hi-z input protection */
	reg_16 = sys_read16(RZA2M_PDR(port));
	mask_16 = RZA2M_PDR_MASK << (pin * 2);
	reg_16 &= ~mask_16;
	sys_write16(reg_16, RZA2M_PDR(port));

	/* Use the pin as a general I/O pin */
	reg_8 = sys_read8(RZA2M_PMR(port));
	reg_8 &= ~BIT(pin);
	sys_write8(reg_8, RZA2M_PMR(port));

	/* Set pin direction */
	reg_16 = sys_read16(RZA2M_PDR(port));
	mask_16 = RZA2M_PDR_MASK << (pin * 2);
	reg_16 &= ~mask_16;
	reg_16 |= dir << (pin * 2);
	sys_write16(reg_16, RZA2M_PDR(port));

	k_mutex_unlock(&rza2m_pinctrl_data.lock);

	return 0;
}

static int rza2m_set_pin_function(uint8_t port, uint8_t pin, uint8_t func)
{
	uint16_t mask_16;
	uint16_t reg_16;
	uint8_t reg_8;

	if (!rza2m_pin_function_check(valid_gpio_support, port, pin)) {
		return -EINVAL;
	}

	k_mutex_lock(&rza2m_pinctrl_data.lock, K_FOREVER);

	/* Set pin to Hi-z input protection */
	reg_16 = sys_read16(RZA2M_PDR(port));
	mask_16 = RZA2M_PDR_MASK << (pin * 2);
	reg_16 &= ~mask_16;
	sys_write16(reg_16, RZA2M_PDR(port));

	/* Temporarily switch to GPIO */
	reg_8 = sys_read8(RZA2M_PMR(port));
	reg_8 &= ~BIT(pin);
	sys_write8(reg_8, RZA2M_PMR(port));

	rza2m_unprotect_pin_mux();

	/* Set Pin function */
	reg_8 = sys_read8(RZA2M_PFS(port, pin));
	reg_8 |= (func & RZA2M_MUX_FUNC_MAX);
	sys_write8(reg_8, RZA2M_PFS(port, pin));

	rza2m_protect_pin_mux();

	/* Port Mode  : Peripheral module pin functions */
	reg_8 = sys_read8(RZA2M_PMR(port));
	reg_8 |= BIT(pin);
	sys_write8(reg_8, RZA2M_PMR(port));

	k_mutex_unlock(&rza2m_pinctrl_data.lock);

	return 0;
}

static int rza2m_set_gpio_int(uint8_t port, uint8_t pin, bool int_en)
{
	uint8_t reg_8;

	if (!rza2m_pin_function_check(valid_gpio_support, port, pin)) {
		return -EINVAL;
	}

	rza2m_unprotect_pin_mux();

	reg_8 = sys_read8(RZA2M_PFS(port, pin));
	if (int_en) {
		/* Enable interrupt, ISEL = 1 */
		reg_8 |= BIT(6);
	} else {
		/* Disable interrupt, ISEL = 0 */
		reg_8 &= ~BIT(6);
	}
	sys_write8(reg_8, RZA2M_PFS(port, pin));

	rza2m_protect_pin_mux();

	return 0;
}

static int pinctrl_configure_pin(const pinctrl_soc_pin_t pin)
{
	int ret;

	ret = 0;

	/* Some pins of PORT_G and PORT_J can be set to 8mA */
	/* Use PORT_CKIO to configure current for the CKIO pin */
	/* Use PORT_PPOC to configure voltage for SPI or SD/MMC interface, after that:
	 *	- If configure voltage for SPI, PSPIBSC need to be updated
	 *	- If configure voltage for SD/MMC, PSDMMC0, PSDMMC1 or PSDMMC2 need to be updated
	 */
	if (pin.port == PORT_G || pin.port == PORT_J) {
		ret = rza2m_set_output_current_pin(pin);
	} else if (pin.port == PORT_CKIO) {
		ret = rza2m_set_output_current_ckio(pin.drive_strength);
	} else if (pin.port == PORT_PPOC) {
		ret = rza2m_set_ppoc(pin);
	}

	if (ret) {
		return ret;
	}

	/* Configure pin to HiZ, input, output and peripheral */
	if (pin.func & RZA2M_FUNC_GPIO_HIZ) {
		ret = rza2m_set_pin_hiz(pin.port, pin.pin);
	} else if (pin.func & RZA2M_FUNC_GPIO_INPUT) {
		ret = rza2m_pin_to_gpio(pin.port, pin.pin, RZA2M_PDR_INPUT);
	} else if (pin.func & RZA2M_FUNC_GPIO_OUTPUT) {
		ret = rza2m_pin_to_gpio(pin.port, pin.pin, RZA2M_PDR_OUTPUT);
	} else {
		ret = rza2m_set_pin_function(pin.port, pin.pin, (pin.func & RZA2M_MUX_FUNC_MAX));
	}

	if (ret) {
		return ret;
	}

	/* Set TINT */
	if (pin.func & RZA2M_FUNC_GPIO_INT_EN) {
		ret = rza2m_set_gpio_int(pin.port, pin.pin, true);
	} else if (pin.func & RZA2M_FUNC_GPIO_INT_DIS) {
		ret = rza2m_set_gpio_int(pin.port, pin.pin, false);
	}

	return ret;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int ret;

	ret = 0;
	ARG_UNUSED(reg);
	while (pin_cnt-- > 0U) {
		ret = pinctrl_configure_pin(*pins++);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

__boot_func static int pinctrl_rza2m_driver_init(void)
{
	device_map(&rza2m_pinctrl_data.base_addr, RZA2M_PINCTRL_REG, RZA2M_PINCTRL_SIZE,
		   K_MEM_CACHE_NONE);

	k_mutex_init(&rza2m_pinctrl_data.lock);

	return 0;
}

SYS_INIT(pinctrl_rza2m_driver_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

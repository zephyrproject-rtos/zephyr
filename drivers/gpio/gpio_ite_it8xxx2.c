/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/ite-it8xxx2-gpio.h>
#include <zephyr/dt-bindings/interrupt-controller/ite-intc.h>
#include <zephyr/irq.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <chip_chipregs.h>
#include <soc_common.h>

LOG_MODULE_REGISTER(gpio_it8xxx2, LOG_LEVEL_ERR);

#define DT_DRV_COMPAT ite_it8xxx2_gpio

/*
 * Structure gpio_ite_cfg is about the setting of gpio
 * this config will be used at initial time
 */
struct gpio_ite_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* gpio port data register (bit mapping to pin) */
	uintptr_t reg_gpdr;
	/* gpio port control register (byte mapping to pin) */
	uintptr_t reg_gpcr;
	/* gpio port data mirror register (bit mapping to pin) */
	uintptr_t reg_gpdmr;
	/* gpio port output type register (bit mapping to pin) */
	uintptr_t reg_gpotr;
	/* Index in gpio_1p8v for voltage level control register element. */
	uint8_t index;
	/* gpio's irq */
	uint8_t gpio_irq[8];
};

/* Structure gpio_ite_data is about callback function */
struct gpio_ite_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

/* dev macros for GPIO */
#define DEV_GPIO_DATA(dev) \
	((struct gpio_ite_data *)(dev)->data)

#define DEV_GPIO_CFG(dev) \
	((const struct gpio_ite_cfg *)(dev)->config)

/**
 * Convert wake-up controller (WUC) group to the corresponding wake-up edge
 * sense register (WUESR). Return pointer to the register.
 *
 * @param grp  WUC group.
 *
 * @return Pointer to corresponding WUESR register.
 */
static volatile uint8_t *wuesr(uint8_t grp)
{
	/*
	 * From WUESR1-WUESR4, the address increases by ones. From WUESR5 on
	 * the address increases by fours.
	 */
	return (grp <= 4) ?
			(volatile uint8_t *)(IT8XXX2_WUC_WUESR1 + grp-1) :
			(volatile uint8_t *)(IT8XXX2_WUC_WUESR5 + 4*(grp-5));
}

/**
 * Convert wake-up controller (WUC) group to the corresponding wake-up edge
 * mode register (WUEMR). Return pointer to the register.
 *
 * @param grp  WUC group.
 *
 * @return Pointer to corresponding WUEMR register.
 */
static volatile uint8_t *wuemr(uint8_t grp)
{
	/*
	 * From WUEMR1-WUEMR4, the address increases by ones. From WUEMR5 on
	 * the address increases by fours.
	 */
	return (grp <= 4) ?
			(volatile uint8_t *)(IT8XXX2_WUC_WUEMR1 + grp-1) :
			(volatile uint8_t *)(IT8XXX2_WUC_WUEMR5 + 4*(grp-5));
}

/**
 * Convert wake-up controller (WUC) group to the corresponding wake-up both edge
 * mode register (WUBEMR). Return pointer to the register.
 *
 * @param grp  WUC group.
 *
 * @return Pointer to corresponding WUBEMR register.
 */
static volatile uint8_t *wubemr(uint8_t grp)
{
	/*
	 * From WUBEMR1-WUBEMR4, the address increases by ones. From WUBEMR5 on
	 * the address increases by fours.
	 */
	return (grp <= 4) ?
			(volatile uint8_t *)(IT8XXX2_WUC_WUBEMR1 + grp-1) :
			(volatile uint8_t *)(IT8XXX2_WUC_WUBEMR5 + 4*(grp-5));
}

/*
 * Array to store the corresponding GPIO WUC group and mask
 * for each WUC interrupt. This allows GPIO interrupts coming in through WUC
 * to easily identify which pin caused the interrupt.
 */
static const struct {
	uint8_t gpio_mask;
	uint8_t wuc_group;
	uint8_t wuc_mask;
} gpio_irqs[] = {
	/*     irq           gpio_mask, wuc_group, wuc_mask */
	[IT8XXX2_IRQ_WU20] = {BIT(0), 2, BIT(0)},
	[IT8XXX2_IRQ_WU21] = {BIT(1), 2, BIT(1)},
	[IT8XXX2_IRQ_WU22] = {BIT(4), 2, BIT(2)},
	[IT8XXX2_IRQ_WU23] = {BIT(6), 2, BIT(3)},
	[IT8XXX2_IRQ_WU24] = {BIT(2), 2, BIT(4)},
	[IT8XXX2_IRQ_WU40] = {BIT(5), 4, BIT(0)},
	[IT8XXX2_IRQ_WU45] = {BIT(6), 4, BIT(5)},
	[IT8XXX2_IRQ_WU46] = {BIT(7), 4, BIT(6)},
	[IT8XXX2_IRQ_WU50] = {BIT(0), 5, BIT(0)},
	[IT8XXX2_IRQ_WU51] = {BIT(1), 5, BIT(1)},
	[IT8XXX2_IRQ_WU52] = {BIT(2), 5, BIT(2)},
	[IT8XXX2_IRQ_WU53] = {BIT(3), 5, BIT(3)},
	[IT8XXX2_IRQ_WU54] = {BIT(4), 5, BIT(4)},
	[IT8XXX2_IRQ_WU55] = {BIT(5), 5, BIT(5)},
	[IT8XXX2_IRQ_WU56] = {BIT(6), 5, BIT(6)},
	[IT8XXX2_IRQ_WU57] = {BIT(7), 5, BIT(7)},
	[IT8XXX2_IRQ_WU60] = {BIT(0), 6, BIT(0)},
	[IT8XXX2_IRQ_WU61] = {BIT(1), 6, BIT(1)},
	[IT8XXX2_IRQ_WU62] = {BIT(2), 6, BIT(2)},
	[IT8XXX2_IRQ_WU63] = {BIT(3), 6, BIT(3)},
	[IT8XXX2_IRQ_WU64] = {BIT(4), 6, BIT(4)},
	[IT8XXX2_IRQ_WU65] = {BIT(5), 6, BIT(5)},
	[IT8XXX2_IRQ_WU65] = {BIT(6), 6, BIT(6)},
	[IT8XXX2_IRQ_WU67] = {BIT(7), 6, BIT(7)},
	[IT8XXX2_IRQ_WU70] = {BIT(0), 7, BIT(0)},
	[IT8XXX2_IRQ_WU71] = {BIT(1), 7, BIT(1)},
	[IT8XXX2_IRQ_WU72] = {BIT(2), 7, BIT(2)},
	[IT8XXX2_IRQ_WU73] = {BIT(3), 7, BIT(3)},
	[IT8XXX2_IRQ_WU74] = {BIT(4), 7, BIT(4)},
	[IT8XXX2_IRQ_WU75] = {BIT(5), 7, BIT(5)},
	[IT8XXX2_IRQ_WU76] = {BIT(6), 7, BIT(6)},
	[IT8XXX2_IRQ_WU77] = {BIT(7), 7, BIT(7)},
	[IT8XXX2_IRQ_WU80] = {BIT(3), 8, BIT(0)},
	[IT8XXX2_IRQ_WU81] = {BIT(4), 8, BIT(1)},
	[IT8XXX2_IRQ_WU82] = {BIT(5), 8, BIT(2)},
	[IT8XXX2_IRQ_WU83] = {BIT(6), 8, BIT(3)},
	[IT8XXX2_IRQ_WU84] = {BIT(2), 8, BIT(4)},
	[IT8XXX2_IRQ_WU85] = {BIT(0), 8, BIT(5)},
	[IT8XXX2_IRQ_WU86] = {BIT(7), 8, BIT(6)},
	[IT8XXX2_IRQ_WU87] = {BIT(7), 8, BIT(7)},
	[IT8XXX2_IRQ_WU88] = {BIT(4), 9, BIT(0)},
	[IT8XXX2_IRQ_WU89] = {BIT(5), 9, BIT(1)},
	[IT8XXX2_IRQ_WU90] = {BIT(6), 9, BIT(2)},
	[IT8XXX2_IRQ_WU91] = {BIT(0), 9, BIT(3)},
	[IT8XXX2_IRQ_WU92] = {BIT(1), 9, BIT(4)},
	[IT8XXX2_IRQ_WU93] = {BIT(2), 9, BIT(5)},
	[IT8XXX2_IRQ_WU94] = {BIT(4), 9, BIT(6)},
	[IT8XXX2_IRQ_WU95] = {BIT(2), 9, BIT(7)},
	[IT8XXX2_IRQ_WU96] = {BIT(0), 10, BIT(0)},
	[IT8XXX2_IRQ_WU97] = {BIT(1), 10, BIT(1)},
	[IT8XXX2_IRQ_WU98] = {BIT(2), 10, BIT(2)},
	[IT8XXX2_IRQ_WU99] = {BIT(3), 10, BIT(3)},
	[IT8XXX2_IRQ_WU100] = {BIT(7), 10, BIT(4)},
	[IT8XXX2_IRQ_WU101] = {BIT(0), 10, BIT(5)},
	[IT8XXX2_IRQ_WU102] = {BIT(1), 10, BIT(6)},
	[IT8XXX2_IRQ_WU103] = {BIT(3), 10, BIT(7)},
	[IT8XXX2_IRQ_WU104] = {BIT(5), 11, BIT(0)},
	[IT8XXX2_IRQ_WU105] = {BIT(6), 11, BIT(1)},
	[IT8XXX2_IRQ_WU106] = {BIT(7), 11, BIT(2)},
	[IT8XXX2_IRQ_WU107] = {BIT(1), 11, BIT(3)},
	[IT8XXX2_IRQ_WU108] = {BIT(3), 11, BIT(4)},
	[IT8XXX2_IRQ_WU109] = {BIT(5), 11, BIT(5)},
	[IT8XXX2_IRQ_WU110] = {BIT(3), 11, BIT(6)},
	[IT8XXX2_IRQ_WU111] = {BIT(4), 11, BIT(7)},
	[IT8XXX2_IRQ_WU112] = {BIT(5), 12, BIT(0)},
	[IT8XXX2_IRQ_WU113] = {BIT(6), 12, BIT(1)},
	[IT8XXX2_IRQ_WU114] = {BIT(4), 12, BIT(2)},
	[IT8XXX2_IRQ_WU115] = {BIT(0), 12, BIT(3)},
	[IT8XXX2_IRQ_WU116] = {BIT(1), 12, BIT(4)},
	[IT8XXX2_IRQ_WU117] = {BIT(2), 12, BIT(5)},
	[IT8XXX2_IRQ_WU118] = {BIT(6), 12, BIT(6)},
	[IT8XXX2_IRQ_WU119] = {BIT(0), 12, BIT(7)},
	[IT8XXX2_IRQ_WU120] = {BIT(1), 13, BIT(0)},
	[IT8XXX2_IRQ_WU121] = {BIT(2), 13, BIT(1)},
	[IT8XXX2_IRQ_WU122] = {BIT(3), 13, BIT(2)},
	[IT8XXX2_IRQ_WU123] = {BIT(3), 13, BIT(3)},
	[IT8XXX2_IRQ_WU124] = {BIT(4), 13, BIT(4)},
	[IT8XXX2_IRQ_WU125] = {BIT(5), 13, BIT(5)},
	[IT8XXX2_IRQ_WU126] = {BIT(7), 13, BIT(6)},
	[IT8XXX2_IRQ_WU128] = {BIT(0), 14, BIT(0)},
	[IT8XXX2_IRQ_WU129] = {BIT(1), 14, BIT(1)},
	[IT8XXX2_IRQ_WU130] = {BIT(2), 14, BIT(2)},
	[IT8XXX2_IRQ_WU131] = {BIT(3), 14, BIT(3)},
	[IT8XXX2_IRQ_WU132] = {BIT(4), 14, BIT(4)},
	[IT8XXX2_IRQ_WU133] = {BIT(5), 14, BIT(5)},
	[IT8XXX2_IRQ_WU134] = {BIT(6), 14, BIT(6)},
	[IT8XXX2_IRQ_WU135] = {BIT(7), 14, BIT(7)},
	[IT8XXX2_IRQ_WU136] = {BIT(0), 15, BIT(0)},
	[IT8XXX2_IRQ_WU137] = {BIT(1), 15, BIT(1)},
	[IT8XXX2_IRQ_WU138] = {BIT(2), 15, BIT(2)},
	[IT8XXX2_IRQ_WU139] = {BIT(3), 15, BIT(3)},
	[IT8XXX2_IRQ_WU140] = {BIT(4), 15, BIT(4)},
	[IT8XXX2_IRQ_WU141] = {BIT(5), 15, BIT(5)},
	[IT8XXX2_IRQ_WU142] = {BIT(6), 15, BIT(6)},
	[IT8XXX2_IRQ_WU143] = {BIT(7), 15, BIT(7)},
	[IT8XXX2_IRQ_WU144] = {BIT(0), 16, BIT(0)},
	[IT8XXX2_IRQ_WU145] = {BIT(1), 16, BIT(1)},
	[IT8XXX2_IRQ_WU146] = {BIT(2), 16, BIT(2)},
	[IT8XXX2_IRQ_WU147] = {BIT(3), 16, BIT(3)},
	[IT8XXX2_IRQ_WU148] = {BIT(4), 16, BIT(4)},
	[IT8XXX2_IRQ_WU149] = {BIT(5), 16, BIT(5)},
	[IT8XXX2_IRQ_WU150] = {BIT(6), 16, BIT(6)},
	[IT8XXX2_IRQ_COUNT] = {     0,  0,     0},
};
BUILD_ASSERT(ARRAY_SIZE(gpio_irqs) == IT8XXX2_IRQ_COUNT + 1);

/* 1.8v gpio group a, b, c, d, e, f, g, h, i, j, k, l, and m */
#define GPIO_GROUP_COUNT 13
#define GPIO_GROUP_INDEX(label) \
		(uint8_t)(DT_REG_ADDR(DT_NODELABEL(label)) - \
			  DT_REG_ADDR(DT_NODELABEL(gpioa)))

/* general control registers for selecting 1.8V/3.3V */
static const struct {
	uint8_t offset;
	uint8_t mask_1p8v;
} gpio_1p8v[GPIO_GROUP_COUNT][8] = {
	[GPIO_GROUP_INDEX(gpioa)] = {
		[4] = {IT8XXX2_GPIO_GCR24_OFFSET, BIT(0)},
		[5] = {IT8XXX2_GPIO_GCR24_OFFSET, BIT(1)},
		[6] = {IT8XXX2_GPIO_GCR24_OFFSET, BIT(5)},
		[7] = {IT8XXX2_GPIO_GCR24_OFFSET, BIT(6)} },
	[GPIO_GROUP_INDEX(gpiob)] = {
		[3] = {IT8XXX2_GPIO_GCR22_OFFSET, BIT(1)},
		[4] = {IT8XXX2_GPIO_GCR22_OFFSET, BIT(0)},
		[5] = {IT8XXX2_GPIO_GCR19_OFFSET, BIT(7)},
		[6] = {IT8XXX2_GPIO_GCR19_OFFSET, BIT(6)},
		[7] = {IT8XXX2_GPIO_GCR24_OFFSET, BIT(4)} },
	[GPIO_GROUP_INDEX(gpioc)] = {
		[0] = {IT8XXX2_GPIO_GCR22_OFFSET, BIT(7)},
		[1] = {IT8XXX2_GPIO_GCR19_OFFSET, BIT(5)},
		[2] = {IT8XXX2_GPIO_GCR19_OFFSET, BIT(4)},
		[4] = {IT8XXX2_GPIO_GCR24_OFFSET, BIT(2)},
		[6] = {IT8XXX2_GPIO_GCR24_OFFSET, BIT(3)},
		[7] = {IT8XXX2_GPIO_GCR19_OFFSET, BIT(3)} },
	[GPIO_GROUP_INDEX(gpiod)] = {
		[0] = {IT8XXX2_GPIO_GCR19_OFFSET, BIT(2)},
		[1] = {IT8XXX2_GPIO_GCR19_OFFSET, BIT(1)},
		[2] = {IT8XXX2_GPIO_GCR19_OFFSET, BIT(0)},
		[3] = {IT8XXX2_GPIO_GCR20_OFFSET, BIT(7)},
		[4] = {IT8XXX2_GPIO_GCR20_OFFSET, BIT(6)},
		[5] = {IT8XXX2_GPIO_GCR22_OFFSET, BIT(4)},
		[6] = {IT8XXX2_GPIO_GCR22_OFFSET, BIT(5)},
		[7] = {IT8XXX2_GPIO_GCR22_OFFSET, BIT(6)} },
	[GPIO_GROUP_INDEX(gpioe)] = {
		[0] = {IT8XXX2_GPIO_GCR20_OFFSET, BIT(5)},
		[1] = {IT8XXX2_GPIO_GCR28_OFFSET, BIT(6)},
		[2] = {IT8XXX2_GPIO_GCR28_OFFSET, BIT(7)},
		[4] = {IT8XXX2_GPIO_GCR22_OFFSET, BIT(2)},
		[5] = {IT8XXX2_GPIO_GCR22_OFFSET, BIT(3)},
		[6] = {IT8XXX2_GPIO_GCR20_OFFSET, BIT(4)},
		[7] = {IT8XXX2_GPIO_GCR20_OFFSET, BIT(3)} },
	[GPIO_GROUP_INDEX(gpiof)] = {
		[0] = {IT8XXX2_GPIO_GCR28_OFFSET, BIT(4)},
		[1] = {IT8XXX2_GPIO_GCR28_OFFSET, BIT(5)},
		[2] = {IT8XXX2_GPIO_GCR20_OFFSET, BIT(2)},
		[3] = {IT8XXX2_GPIO_GCR20_OFFSET, BIT(1)},
		[4] = {IT8XXX2_GPIO_GCR20_OFFSET, BIT(0)},
		[5] = {IT8XXX2_GPIO_GCR21_OFFSET, BIT(7)},
		[6] = {IT8XXX2_GPIO_GCR21_OFFSET, BIT(6)},
		[7] = {IT8XXX2_GPIO_GCR21_OFFSET, BIT(5)} },
	[GPIO_GROUP_INDEX(gpiog)] = {
		[0] = {IT8XXX2_GPIO_GCR28_OFFSET, BIT(2)},
		[1] = {IT8XXX2_GPIO_GCR21_OFFSET, BIT(4)},
		[2] = {IT8XXX2_GPIO_GCR28_OFFSET, BIT(3)},
		[6] = {IT8XXX2_GPIO_GCR21_OFFSET, BIT(3)} },
	[GPIO_GROUP_INDEX(gpioh)] = {
		[0] = {IT8XXX2_GPIO_GCR21_OFFSET, BIT(2)},
		[1] = {IT8XXX2_GPIO_GCR21_OFFSET, BIT(1)},
		[2] = {IT8XXX2_GPIO_GCR21_OFFSET, BIT(0)},
		[5] = {IT8XXX2_GPIO_GCR27_OFFSET, BIT(7)},
		[6] = {IT8XXX2_GPIO_GCR28_OFFSET, BIT(0)} },
	[GPIO_GROUP_INDEX(gpioi)] = {
		[0] = {IT8XXX2_GPIO_GCR27_OFFSET, BIT(3)},
		[1] = {IT8XXX2_GPIO_GCR23_OFFSET, BIT(4)},
		[2] = {IT8XXX2_GPIO_GCR23_OFFSET, BIT(5)},
		[3] = {IT8XXX2_GPIO_GCR23_OFFSET, BIT(6)},
		[4] = {IT8XXX2_GPIO_GCR23_OFFSET, BIT(7)},
		[5] = {IT8XXX2_GPIO_GCR27_OFFSET, BIT(4)},
		[6] = {IT8XXX2_GPIO_GCR27_OFFSET, BIT(5)},
		[7] = {IT8XXX2_GPIO_GCR27_OFFSET, BIT(6)} },
	[GPIO_GROUP_INDEX(gpioj)] = {
		[0] = {IT8XXX2_GPIO_GCR23_OFFSET, BIT(0)},
		[1] = {IT8XXX2_GPIO_GCR23_OFFSET, BIT(1)},
		[2] = {IT8XXX2_GPIO_GCR23_OFFSET, BIT(2)},
		[3] = {IT8XXX2_GPIO_GCR23_OFFSET, BIT(3)},
		[4] = {IT8XXX2_GPIO_GCR27_OFFSET, BIT(0)},
		[5] = {IT8XXX2_GPIO_GCR27_OFFSET, BIT(1)},
		[6] = {IT8XXX2_GPIO_GCR27_OFFSET, BIT(2)},
		[7] = {IT8XXX2_GPIO_GCR33_OFFSET, BIT(2)} },
	[GPIO_GROUP_INDEX(gpiok)] = {
		[0] = {IT8XXX2_GPIO_GCR26_OFFSET, BIT(0)},
		[1] = {IT8XXX2_GPIO_GCR26_OFFSET, BIT(1)},
		[2] = {IT8XXX2_GPIO_GCR26_OFFSET, BIT(2)},
		[3] = {IT8XXX2_GPIO_GCR26_OFFSET, BIT(3)},
		[4] = {IT8XXX2_GPIO_GCR26_OFFSET, BIT(4)},
		[5] = {IT8XXX2_GPIO_GCR26_OFFSET, BIT(5)},
		[6] = {IT8XXX2_GPIO_GCR26_OFFSET, BIT(6)},
		[7] = {IT8XXX2_GPIO_GCR26_OFFSET, BIT(7)} },
	[GPIO_GROUP_INDEX(gpiol)] = {
		[0] = {IT8XXX2_GPIO_GCR25_OFFSET, BIT(0)},
		[1] = {IT8XXX2_GPIO_GCR25_OFFSET, BIT(1)},
		[2] = {IT8XXX2_GPIO_GCR25_OFFSET, BIT(2)},
		[3] = {IT8XXX2_GPIO_GCR25_OFFSET, BIT(3)},
		[4] = {IT8XXX2_GPIO_GCR25_OFFSET, BIT(4)},
		[5] = {IT8XXX2_GPIO_GCR25_OFFSET, BIT(5)},
		[6] = {IT8XXX2_GPIO_GCR25_OFFSET, BIT(6)},
		[7] = {IT8XXX2_GPIO_GCR25_OFFSET, BIT(7)} },
	/*
	 * M group's voltage level is according to chip's VCC is connected
	 * to 1.8V or 3.3V.
	 */
	[GPIO_GROUP_INDEX(gpiom)] = {
		[0] = {IT8XXX2_GPIO_GCR30_OFFSET, BIT(4)},
		[1] = {IT8XXX2_GPIO_GCR30_OFFSET, BIT(4)},
		[2] = {IT8XXX2_GPIO_GCR30_OFFSET, BIT(4)},
		[3] = {IT8XXX2_GPIO_GCR30_OFFSET, BIT(4)},
		[4] = {IT8XXX2_GPIO_GCR30_OFFSET, BIT(4)},
		[5] = {IT8XXX2_GPIO_GCR30_OFFSET, BIT(4)},
		[6] = {IT8XXX2_GPIO_GCR30_OFFSET, BIT(4)} },
};

/**
 * Driver functions
 */
static int gpio_ite_configure(const struct device *dev,
				gpio_pin_t pin,
				gpio_flags_t flags)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);

	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;
	volatile uint8_t *reg_gpcr = (uint8_t *)(gpio_config->reg_gpcr + pin);
	volatile uint8_t *reg_gpotr = (uint8_t *)gpio_config->reg_gpotr;
	volatile uint8_t *reg_1p8v;
	volatile uint8_t mask_1p8v;
	uint8_t mask = BIT(pin);

	__ASSERT(gpio_config->index < GPIO_GROUP_COUNT,
		"Invalid GPIO group index");

	/* Don't support "open source" mode */
	if (((flags & GPIO_SINGLE_ENDED) != 0) &&
	    ((flags & GPIO_LINE_OPEN_DRAIN) == 0)) {
		return -ENOTSUP;
	}

	if (flags == GPIO_DISCONNECTED) {
		*reg_gpcr = GPCR_PORT_PIN_MODE_TRISTATE;
		/*
		 * Since not all GPIOs can be to configured as tri-state,
		 * prompt error if pin doesn't support the flag.
		 */
		if (*reg_gpcr != GPCR_PORT_PIN_MODE_TRISTATE) {
			/* Go back to default setting (input) */
			*reg_gpcr = GPCR_PORT_PIN_MODE_INPUT;
			LOG_ERR("Cannot config GPIO-%c%d as tri-state",
				(gpio_config->index + 'A'), pin);
			return -ENOTSUP;
		}
		/*
		 * The following configuration isn't necessary because the pin
		 * was configured as disconnected.
		 */
		return 0;
	}

	/*
	 * Select open drain first, so that we don't glitch the signal
	 * when changing the line to an output.
	 */
	if (flags & GPIO_OPEN_DRAIN) {
		*reg_gpotr |= mask;
	} else {
		*reg_gpotr &= ~mask;
	}

	/* 1.8V or 3.3V */
	reg_1p8v = &IT8XXX2_GPIO_GCRX(
			gpio_1p8v[gpio_config->index][pin].offset);
	mask_1p8v = gpio_1p8v[gpio_config->index][pin].mask_1p8v;
	if (reg_1p8v != &IT8XXX2_GPIO_GCRX(0)) {
		gpio_flags_t volt = flags & IT8XXX2_GPIO_VOLTAGE_MASK;

		if (volt == IT8XXX2_GPIO_VOLTAGE_1P8) {
			__ASSERT(!(flags & GPIO_PULL_UP),
			"Don't enable internal pullup if 1.8V voltage is used");
			*reg_1p8v |= mask_1p8v;
		} else if (volt == IT8XXX2_GPIO_VOLTAGE_3P3 ||
			   volt == IT8XXX2_GPIO_VOLTAGE_DEFAULT) {
			*reg_1p8v &= ~mask_1p8v;
		} else {
			return -EINVAL;
		}
	}

	/* If output, set level before changing type to an output. */
	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			*reg_gpdr |= mask;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			*reg_gpdr &= ~mask;
		}
	}

	/* Set input or output. */
	if (flags & GPIO_OUTPUT)
		*reg_gpcr = (*reg_gpcr | GPCR_PORT_PIN_MODE_OUTPUT) &
				~GPCR_PORT_PIN_MODE_INPUT;
	else
		*reg_gpcr = (*reg_gpcr | GPCR_PORT_PIN_MODE_INPUT) &
				~GPCR_PORT_PIN_MODE_OUTPUT;

	/* Handle pullup / pulldown */
	if (flags & GPIO_PULL_UP) {
		*reg_gpcr = (*reg_gpcr | GPCR_PORT_PIN_MODE_PULLUP) &
				~GPCR_PORT_PIN_MODE_PULLDOWN;
	} else if (flags & GPIO_PULL_DOWN) {
		*reg_gpcr = (*reg_gpcr | GPCR_PORT_PIN_MODE_PULLDOWN) &
				~GPCR_PORT_PIN_MODE_PULLUP;
	} else {
		/* No pull up/down */
		*reg_gpcr &= ~(GPCR_PORT_PIN_MODE_PULLUP |
				GPCR_PORT_PIN_MODE_PULLDOWN);
	}

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_ite_get_config(const struct device *dev,
			       gpio_pin_t pin,
			       gpio_flags_t *out_flags)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);

	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;
	volatile uint8_t *reg_gpcr = (uint8_t *)(gpio_config->reg_gpcr + pin);
	volatile uint8_t *reg_gpotr = (uint8_t *)gpio_config->reg_gpotr;
	volatile uint8_t *reg_1p8v;
	volatile uint8_t mask_1p8v;
	uint8_t mask = BIT(pin);
	gpio_flags_t flags = 0;

	__ASSERT(gpio_config->index < GPIO_GROUP_COUNT,
		"Invalid GPIO group index");

	/* push-pull or open-drain */
	if (*reg_gpotr & mask)
		flags |= GPIO_OPEN_DRAIN;

	/* 1.8V or 3.3V */
	reg_1p8v = &IT8XXX2_GPIO_GCRX(
			gpio_1p8v[gpio_config->index][pin].offset);
	/*
	 * Since not all GPIOs support voltage selection, voltage flag
	 * is only set if voltage selection register is present.
	 */
	if (reg_1p8v != &IT8XXX2_GPIO_GCRX(0)) {
		mask_1p8v = gpio_1p8v[gpio_config->index][pin].mask_1p8v;
		if (*reg_1p8v & mask_1p8v) {
			flags |= IT8XXX2_GPIO_VOLTAGE_1P8;
		} else {
			flags |= IT8XXX2_GPIO_VOLTAGE_3P3;
		}
	}

	/* set input or output. */
	if (*reg_gpcr & GPCR_PORT_PIN_MODE_OUTPUT) {
		flags |= GPIO_OUTPUT;

		/* set level */
		if (*reg_gpdr & mask) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
	}

	if (*reg_gpcr & GPCR_PORT_PIN_MODE_INPUT) {
		flags |= GPIO_INPUT;

		/* pullup / pulldown */
		if (*reg_gpcr & GPCR_PORT_PIN_MODE_PULLUP) {
			flags |= GPIO_PULL_UP;
		}

		if (*reg_gpcr & GPCR_PORT_PIN_MODE_PULLDOWN) {
			flags |= GPIO_PULL_DOWN;
		}
	}

	*out_flags = flags;

	return 0;
}
#endif

static int gpio_ite_port_get_raw(const struct device *dev,
					gpio_port_value_t *value)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	volatile uint8_t *reg_gpdmr = (uint8_t *)gpio_config->reg_gpdmr;

	/* Get raw bits of GPIO mirror register */
	*value = *reg_gpdmr;

	return 0;
}

static int gpio_ite_port_set_masked_raw(const struct device *dev,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;
	uint8_t out = *reg_gpdr;

	*reg_gpdr = ((out & ~mask) | (value & mask));

	return 0;
}

static int gpio_ite_port_set_bits_raw(const struct device *dev,
					gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;

	/* Set raw bits of GPIO data register */
	*reg_gpdr |= pins;

	return 0;
}

static int gpio_ite_port_clear_bits_raw(const struct device *dev,
						gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;

	/* Clear raw bits of GPIO data register */
	*reg_gpdr &= ~pins;

	return 0;
}

static int gpio_ite_port_toggle_bits(const struct device *dev,
					gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;

	/* Toggle raw bits of GPIO data register */
	*reg_gpdr ^= pins;

	return 0;
}

static int gpio_ite_manage_callback(const struct device *dev,
					struct gpio_callback *callback,
					bool set)
{
	struct gpio_ite_data *data = DEV_GPIO_DATA(dev);

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_ite_isr(const void *arg)
{
	uint8_t irq = ite_intc_get_irq_num();
	const struct device *dev = arg;
	struct gpio_ite_data *data = DEV_GPIO_DATA(dev);
	uint8_t gpio_mask = gpio_irqs[irq].gpio_mask;

	if (gpio_irqs[irq].wuc_group) {
		/* Clear the WUC status register. */
		*(wuesr(gpio_irqs[irq].wuc_group)) = gpio_irqs[irq].wuc_mask;
		gpio_fire_callbacks(&data->callbacks, dev, gpio_mask);
	}
}

static int gpio_ite_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint8_t gpio_irq = gpio_config->gpio_irq[pin];

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	if (mode == GPIO_INT_MODE_DISABLED || mode == GPIO_INT_MODE_DISABLE_ONLY) {
#else
	if (mode == GPIO_INT_MODE_DISABLED) {
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
		/* Disable GPIO interrupt */
		irq_disable(gpio_irq);
		return 0;
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	} else if (mode == GPIO_INT_MODE_ENABLE_ONLY) {
		/* Only enable GPIO interrupt */
		irq_enable(gpio_irq);
		return 0;
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		LOG_ERR("Level trigger mode not supported");
		return -ENOTSUP;
	}

	/* Disable irq before configuring it */
	irq_disable(gpio_irq);

	if (trig & GPIO_INT_TRIG_BOTH) {
		uint8_t wuc_group = gpio_irqs[gpio_irq].wuc_group;
		uint8_t wuc_mask = gpio_irqs[gpio_irq].wuc_mask;

		/* Set both edges interrupt. */
		if ((trig & GPIO_INT_TRIG_BOTH) == GPIO_INT_TRIG_BOTH) {
			*(wubemr(wuc_group)) |= wuc_mask;
		} else {
			*(wubemr(wuc_group)) &= ~wuc_mask;
		}

		if (trig & GPIO_INT_TRIG_LOW) {
			*(wuemr(wuc_group)) |= wuc_mask;
		} else {
			*(wuemr(wuc_group)) &= ~wuc_mask;
		}
		/*
		 * Always write 1 to clear the WUC status register after
		 * modifying edge mode selection register (WUBEMR and WUEMR).
		 */
		*(wuesr(wuc_group)) = wuc_mask;
	}

	/* Enable GPIO interrupt */
	irq_connect_dynamic(gpio_irq, 0, gpio_ite_isr, dev, 0);
	irq_enable(gpio_irq);

	return 0;
}

static const struct gpio_driver_api gpio_ite_driver_api = {
	.pin_configure = gpio_ite_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_ite_get_config,
#endif
	.port_get_raw = gpio_ite_port_get_raw,
	.port_set_masked_raw = gpio_ite_port_set_masked_raw,
	.port_set_bits_raw = gpio_ite_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ite_port_clear_bits_raw,
	.port_toggle_bits = gpio_ite_port_toggle_bits,
	.pin_interrupt_configure = gpio_ite_pin_interrupt_configure,
	.manage_callback = gpio_ite_manage_callback,
};

static int gpio_ite_init(const struct device *dev)
{
	return 0;
}

#define GPIO_ITE_DEV_CFG_DATA(inst)                                \
static struct gpio_ite_data gpio_ite_data_##inst;                  \
static const struct gpio_ite_cfg gpio_ite_cfg_##inst = {           \
	.common = {                                                \
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(   \
		DT_INST_PROP(inst, ngpios))                        \
	},                                                         \
	.reg_gpdr = DT_INST_REG_ADDR_BY_IDX(inst, 0),              \
	.reg_gpcr = DT_INST_REG_ADDR_BY_IDX(inst, 1),              \
	.reg_gpdmr = DT_INST_REG_ADDR_BY_IDX(inst, 2),             \
	.reg_gpotr = DT_INST_REG_ADDR_BY_IDX(inst, 3),             \
	.index = (uint8_t)(DT_INST_REG_ADDR(inst) -                \
			   DT_REG_ADDR(DT_NODELABEL(gpioa))),      \
	.gpio_irq[0] = DT_INST_IRQ_BY_IDX(inst, 0, irq),           \
	.gpio_irq[1] = DT_INST_IRQ_BY_IDX(inst, 1, irq),           \
	.gpio_irq[2] = DT_INST_IRQ_BY_IDX(inst, 2, irq),           \
	.gpio_irq[3] = DT_INST_IRQ_BY_IDX(inst, 3, irq),           \
	.gpio_irq[4] = DT_INST_IRQ_BY_IDX(inst, 4, irq),           \
	.gpio_irq[5] = DT_INST_IRQ_BY_IDX(inst, 5, irq),           \
	.gpio_irq[6] = DT_INST_IRQ_BY_IDX(inst, 6, irq),           \
	.gpio_irq[7] = DT_INST_IRQ_BY_IDX(inst, 7, irq),           \
	};                                                         \
DEVICE_DT_INST_DEFINE(inst,                                        \
		gpio_ite_init,                                     \
		NULL,                                              \
		&gpio_ite_data_##inst,                             \
		&gpio_ite_cfg_##inst,                              \
		PRE_KERNEL_1,                                       \
		CONFIG_GPIO_INIT_PRIORITY,                         \
		&gpio_ite_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ITE_DEV_CFG_DATA)

static int gpio_it8xxx2_init_set(void)
{

	if (IS_ENABLED(CONFIG_SOC_IT8XXX2_GPIO_GROUP_K_L_DEFAULT_PULL_DOWN)) {
		const struct device *const gpiok = DEVICE_DT_GET(DT_NODELABEL(gpiok));
		const struct device *const gpiol = DEVICE_DT_GET(DT_NODELABEL(gpiol));

		for (int i = 0; i < 8; i++) {
			gpio_pin_configure(gpiok, i, GPIO_INPUT | GPIO_PULL_DOWN);
			gpio_pin_configure(gpiol, i, GPIO_INPUT | GPIO_PULL_DOWN);
		}
	}

	if (IS_ENABLED(CONFIG_SOC_IT8XXX2_GPIO_H7_DEFAULT_OUTPUT_LOW)) {
		const struct device *const gpioh = DEVICE_DT_GET(DT_NODELABEL(gpioh));

		gpio_pin_configure(gpioh, 7, GPIO_OUTPUT_LOW);
	}

	return 0;
}
SYS_INIT(gpio_it8xxx2_init_set, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY);

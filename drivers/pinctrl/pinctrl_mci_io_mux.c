/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

static MCI_IO_MUX_Type *mci_iomux =
	(MCI_IO_MUX_Type *)DT_REG_ADDR(DT_NODELABEL(pinctrl));

static SOCCIU_Type *soc_ctrl =
	(SOCCIU_Type *)DT_REG_ADDR(DT_NODELABEL(soc_ctrl));
static AON_SOC_CIU_Type *aon_soc_ciu =
	(AON_SOC_CIU_Type *)DT_REG_ADDR(DT_NODELABEL(aon_soc_ctrl));

/*
 * GPIO mux option definitions. Stored as a static array, because
 * these mux options are needed to clear pin mux settings to
 * a known good state before selecting a new alternate function.
 */
static uint64_t gpio_muxes[] = {IOMUX_GPIO_OPS};

/*
 * Helper function to handle setting pin properties,
 * such as pin bias and slew rate
 */
static void configure_pin_props(uint32_t pin_mux, uint8_t gpio_idx)
{
	uint32_t mask, set;
	volatile uint32_t *pull_reg = &soc_ctrl->PAD_PU_PD_EN0;
	volatile uint32_t *slew_reg = &soc_ctrl->SR_CONFIG0;
	volatile uint32_t *sleep_force_en = &soc_ctrl->PAD_SLP_EN0;
	volatile uint32_t *sleep_force_val = &soc_ctrl->PAD_SLP_VAL0;

	/* GPIO 22-27 use always on configuration registers */
	if (gpio_idx > 21 && gpio_idx < 28) {
		pull_reg = (&aon_soc_ciu->PAD_PU_PD_EN1 - 1);
		slew_reg = (&aon_soc_ciu->SR_CONFIG1 - 1);
		sleep_force_en = &aon_soc_ciu->PAD_SLP_EN0;
		sleep_force_val = &aon_soc_ciu->PAD_SLP_VAL0;
	}
	/* Calculate register offset for pull and slew regs.
	 * Use bit shifting as opposed to division
	 */
	pull_reg += (gpio_idx >> 4);
	slew_reg += (gpio_idx >> 4);
	sleep_force_en += (gpio_idx >> 5);
	sleep_force_val += (gpio_idx >> 5);
	/* Set pull-up/pull-down */
	/* Use mask and bitshift here as opposed to modulo and multiplication.
	 * equivalent to ((gpio_idx % 16) * 2)
	 */
	mask = 0x3 << ((gpio_idx & 0xF) << 1);
	set = IOMUX_PAD_GET_PULL(pin_mux) << ((gpio_idx & 0xF) << 1);
	*pull_reg = (*pull_reg & ~mask) | set;

	/* Set slew rate */
	set = IOMUX_PAD_GET_SLEW(pin_mux) << ((gpio_idx & 0xF) << 1);
	*slew_reg = (*slew_reg & ~mask) | set;

	/* Set sleep force enable bit */
	mask = (0x1 << (gpio_idx & 0x1F));
	set = (IOMUX_PAD_GET_SLEEP_FORCE_EN(pin_mux) << (gpio_idx & 0x1F));
	*sleep_force_en = (*sleep_force_en & ~mask) | set;
	set = (IOMUX_PAD_GET_SLEEP_FORCE_VAL(pin_mux) << (gpio_idx & 0x1F));
	*sleep_force_val = (*sleep_force_val & ~mask) | set;
}

static void select_gpio_mode(uint8_t gpio_idx)
{
	uint64_t gpio_setting = gpio_muxes[gpio_idx];
	volatile uint32_t *flexcomm_reg = &mci_iomux->FC0;

	/* Clear flexcomm settings */
	flexcomm_reg += IOMUX_GET_FLEXCOMM_CLR_IDX(gpio_setting);
	*flexcomm_reg &= ~IOMUX_GET_FLEXCOMM_CLR_MASK(gpio_setting);
	/* Clear fsel settings */
	mci_iomux->FSEL &= ~IOMUX_GET_FSEL_CLR_MASK(gpio_setting);
	/* Clear CTimer in/out, if required */
	if (IOMUX_GET_SCTIMER_IN_CLR_ENABLE(gpio_setting)) {
		mci_iomux->C_TIMER_IN &=
			~(0x1 << IOMUX_GET_CTIMER_CLR_OFFSET(gpio_setting));
		mci_iomux->C_TIMER_OUT &=
			~(0x1 << IOMUX_GET_CTIMER_CLR_OFFSET(gpio_setting));
	}
	/* Clear SCTimer in/out, if required */
	if (IOMUX_GET_SCTIMER_IN_CLR_ENABLE(gpio_setting)) {
		mci_iomux->SC_TIMER &=
			~(0x1 << IOMUX_GET_SCTIMER_IN_CLR_OFFSET(gpio_setting));
	}
	if (IOMUX_GET_SCTIMER_OUT_CLR_ENABLE(gpio_setting)) {
		mci_iomux->SC_TIMER &=
			~(0x1 << (IOMUX_GET_SCTIMER_OUT_CLR_OFFSET(gpio_setting) + 16));
	}
	/* Clear security gpio enable */
	mci_iomux->S_GPIO &= ~(0x1 << (gpio_idx - 32));
}


int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	volatile uint32_t *flexcomm_reg;
	volatile uint32_t *iomux_en_reg;

	for (uint8_t i = 0; i < pin_cnt; i++) {
		flexcomm_reg = &mci_iomux->FC0;
		iomux_en_reg = &soc_ctrl->MCI_IOMUX_EN0;
		uint32_t pin_mux = pins[i];
		uint8_t gpio_idx = IOMUX_GET_GPIO_IDX(pin_mux);
		uint8_t type = IOMUX_GET_TYPE(pin_mux);
		/*
		 * Before selecting an alternate function, we must clear any
		 * conflicting pin configuration. We do this by resetting the
		 * pin to a gpio configuration, then selecting the alternate
		 * function.
		 */
		select_gpio_mode(gpio_idx);
		switch (type) {
		case IOMUX_FLEXCOMM:
			flexcomm_reg += IOMUX_GET_FLEXCOMM_IDX(pin_mux);
			*flexcomm_reg |=
				(0x1 << IOMUX_GET_FLEXCOMM_BIT(pin_mux));
			break;
		case IOMUX_FSEL:
			mci_iomux->FSEL |=
				(0x1 << IOMUX_GET_FSEL_BIT(pin_mux));
			break;
		case IOMUX_CTIMER_IN:
			mci_iomux->C_TIMER_IN |=
				(0x1 << IOMUX_GET_CTIMER_BIT(pin_mux));
			break;
		case IOMUX_CTIMER_OUT:
			mci_iomux->C_TIMER_OUT |=
				(0x1 << IOMUX_GET_CTIMER_BIT(pin_mux));
			break;
		case IOMUX_SCTIMER_IN:
			mci_iomux->SC_TIMER |=
				(0x1 << IOMUX_GET_SCTIMER_BIT(pin_mux));
			break;
		case IOMUX_SCTIMER_OUT:
			mci_iomux->SC_TIMER |=
				(0x1 << (IOMUX_GET_SCTIMER_BIT(pin_mux) + 16));
			break;
		case IOMUX_SGPIO:
			mci_iomux->S_GPIO |= (0x1 << (gpio_idx - 32));
			break;
		case IOMUX_GPIO:
			if (gpio_idx > 32) {
				mci_iomux->GPIO_GRP1 |= (0x1 << (gpio_idx - 32));
			} else {
				mci_iomux->GPIO_GRP0 |= (0x1 << gpio_idx);
			}
			break;
		case IOMUX_AON:
			/* No selection bits should be set */
			break;
		default:
			/* Unsupported type passed */
			return -ENOTSUP;
		}
		configure_pin_props(pin_mux, gpio_idx);
		/* Now, enable pin controller access to this pin */
		if (gpio_idx > 21 && gpio_idx < 28) {
			/* GPIO 22-27 use always on soc controller */
			iomux_en_reg = &aon_soc_ciu->MCI_IOMUX_EN0;
		}
		iomux_en_reg += (gpio_idx >> 5);
		*iomux_en_reg |= (0x1 << (gpio_idx & 0x1F));
	}
	return 0;
}

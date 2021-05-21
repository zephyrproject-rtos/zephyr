/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <soc.h>

/* More MEC1501 HAL bugs */
#ifdef CONFIG_SOC_SERIES_MEC1501X
#define MCHP_GPIO_MAX_ID	174U
#define MCHP_GPIO_BASE_ADDR	GPIO_CTRL_BASE
#endif

static void gpio_idet_set(uint32_t ctrl_addr, uint32_t cfg)
{
	uint32_t ctrl = REG32(ctrl_addr) & ~(MCHP_GPIO_CTRL_IDET_MASK);

	switch (cfg & MCHP_GPIO_CFG_IDET_MASK) {
	case MCHP_GPIO_CFG_IDET_LVL_LO:
		ctrl |= MCHP_GPIO_CTRL_IDET_LVL_LO;
		break;
	case MCHP_GPIO_CFG_IDET_LVL_HI:
		ctrl |= MCHP_GPIO_CTRL_IDET_LVL_HI;
		break;
	case MCHP_GPIO_CFG_IDET_EDGE_RISING:
		ctrl |= MCHP_GPIO_CTRL_IDET_REDGE;
		break;
	case MCHP_GPIO_CFG_IDET_EDGE_FALLING:
		ctrl |= MCHP_GPIO_CTRL_IDET_FEDGE;
		break;
	case MCHP_GPIO_CFG_IDET_EDGE_BOTH:
		ctrl |= MCHP_GPIO_CTRL_IDET_BEDGE;
		break;
	default:
		ctrl |= MCHP_GPIO_CTRL_IDET_DISABLE;
		break;
	}

	REG32(ctrl_addr) = ctrl;
}

static void gpio_pud_set(uint32_t ctrl_addr, uint32_t cfg)
{
	uint32_t ctrl = REG32(ctrl_addr) & ~(MCHP_GPIO_CTRL_PUD_MASK);

	switch (cfg & MCHP_GPIO_CFG_PULLS_MASK) {
	case MCHP_GPIO_CFG_PULLS_UP:
		ctrl |= MCHP_GPIO_CTRL_PUD_PU;
		break;
	case MCHP_GPIO_CFG_PULLS_DN:
		ctrl |= MCHP_GPIO_CTRL_PUD_PD;
		break;
	case MCHP_GPIO_CFG_PULLS_REPEAT:
		ctrl |= MCHP_GPIO_CTRL_PUD_RPT;
		break;
	default:
		ctrl |= MCHP_GPIO_CTRL_PUD_NONE;
	}

	REG32(ctrl_addr) = ctrl;
}

static void gpio_pwrgate_set(uint32_t ctrl_addr, uint32_t cfg)
{
	uint32_t ctrl = REG32(ctrl_addr) & ~(MCHP_GPIO_CTRL_PWRG_MASK);

	switch (cfg & MCHP_GPIO_CFG_PG_MASK) {
	case MCHP_GPIO_CFG_PG_VCC:
		ctrl |= MCHP_GPIO_CTRL_PWRG_VCC_IO;
		break;
	case MCHP_GPIO_CFG_PG_UNPWRD:
		ctrl |= MCHP_GPIO_CTRL_PWRG_OFF;
		break;
	default: /* PG VTR */
		ctrl |= MCHP_GPIO_CTRL_PWRG_VTR_IO;
		break;
	}

	REG32(ctrl_addr) = ctrl;
}

/* Set pin alternate functions and its polarity */
static void gpio_alt_func(uint32_t ctrl_addr, uint32_t cfg)
{
	uint32_t f = (cfg & MCHP_GPIO_CFG_FUNC_MASK) >> MCHP_GPIO_CFG_FUNC_POS;
	uint32_t ctrl = REG32(ctrl_addr) &
		~(MCHP_GPIO_CTRL_MUX_MASK | MCHP_GPIO_CTRL_POL_INVERT);

	ctrl |= ((f << MCHP_GPIO_CTRL_MUX_POS) & MCHP_GPIO_CTRL_MUX_MASK);
	if (cfg & MCHP_GPIO_CTRL_POL_INVERT) {
		ctrl |= MCHP_GPIO_CTRL_POL_INVERT;
	}
	REG32(ctrl_addr) = ctrl;
}

static void gpio_slew_set(uint32_t ctrl_addr, uint32_t cfg)
{
	uint32_t ctrl2_addr = ctrl_addr + MCHP_GPIO_CTRL2_OFS;

	if (cfg & MCHP_GPIO_CFG_SPD_FAST) {
		REG32(ctrl2_addr) |= MCHP_GPIO_CTRL2_SLEW_FAST;
	} else {
		REG32(ctrl2_addr) &= ~MCHP_GPIO_CTRL2_SLEW_FAST;
	}
}

static void gpio_drive_str_set(uint32_t ctrl_addr, uint32_t cfg)
{
	uint32_t ctrl2_addr = ctrl_addr + MCHP_GPIO_CTRL2_OFS;
	uint32_t ctrl2 = REG32(ctrl2_addr) & ~MCHP_GPIO_CTRL2_DRV_STR_MASK;

	switch (cfg & MCHP_GPIO_CFG_DRVS_MASK) {
	case MCHP_GPIO_CFG_DRVS_4MA:
		ctrl2 |= MCHP_GPIO_CTRL2_DRV_STR_4MA;
		break;
	case MCHP_GPIO_CFG_DRVS_8MA:
		ctrl2 |= MCHP_GPIO_CTRL2_DRV_STR_8MA;
		break;
	case MCHP_GPIO_CFG_DRVS_12MA:
		ctrl2 |= MCHP_GPIO_CTRL2_DRV_STR_12MA;
		break;
	default: /* MCHP_GPIO_CFG_DRVS_2MA */
		ctrl2 |= MCHP_GPIO_CTRL2_DRV_STR_2MA;
		break;
	}

	REG32(ctrl2_addr) = ctrl2;
}

static void gpio_output_buf_type(uint32_t ctrl_addr, uint32_t cfg)
{
	if (cfg & MCHP_GPIO_CFG_OPEN_DRAIN_EN) {
		REG32(ctrl_addr) |= MCHP_GPIO_CTRL_BUFT_OPENDRAIN;
	} else { /* push-pull */
		REG32(ctrl_addr) &= ~MCHP_GPIO_CTRL_BUFT_OPENDRAIN;
	}
}

uint32_t soc_gpio_ctrl_addr(uint32_t pin_id)
{
	if (pin_id >= MCHP_GPIO_MAX_ID) {
		return 0U;
	}

	return (uint32_t)MCHP_GPIO_CTRL_ADDR(pin_id);
}

/*
 * Read GPIO pin input pad state.
 * returns:
 * 0: success and pin_state updated with value: 0 or 1
 * -EINVAL: invalid pin_id or NULL pointer
 */
int soc_gpio_input(uint32_t pin_id, uint32_t *pin_state)
{
	if ((pin_id >= MCHP_GPIO_MAX_ID) || !pin_state) {
		return -EINVAL;
	}

	*pin_state = (REG32(MCHP_GPIO_CTRL_ADDR(pin_id))
			>> MCHP_GPIO_CTRL_INPAD_VAL_POS) & BIT(0);

	return 0;
}

/*
 * If parallel output enabled writing the parallel output bit to 1 is reflected
 * in the control register alternate output bit[16].
 * If parallel output is disabled writing the alternate output bit[16] in the
 * control register is not reflected in the parallel output register.
 */
int soc_gpio_output(uint32_t pin_id, uint32_t value)
{
	if (pin_id >= MCHP_GPIO_MAX_ID) {
		return -EINVAL;
	}

	uint32_t port = pin_id >> 5;
	uint32_t pos = pin_id & 0x1FU;
	uint32_t port_addr = MCHP_GPIO_PAROUT_ADDR(port);
	uint32_t ctrl_addr = MCHP_GPIO_CTRL_ADDR(pin_id);

	if (value) {
		REG32(port_addr) |= BIT(pos);
		REG32(ctrl_addr) |= MCHP_GPIO_CTRL_OUTV_HI;
	} else {
		REG32(port_addr) &= ~BIT(pos);
		REG32(ctrl_addr) &= ~MCHP_GPIO_CTRL_OUTV_HI;
	}

	return 0;
}

int soc_gpio_inport(uint32_t gpio_port, uint32_t *value)
{
	if ((gpio_port >= NUM_MCHP_GPIO_PORTS) || !value) {
		return -EINVAL;
	}

	*value = REG32(MCHP_GPIO_PARIN_ADDR(gpio_port));
	return 0;
}

int soc_gpio_outport(uint32_t gpio_port, uint32_t *value)
{
	if ((gpio_port >= NUM_MCHP_GPIO_PORTS) || !value) {
		return -EINVAL;
	}

	*value = REG32(MCHP_GPIO_PAROUT_ADDR(gpio_port));
	return 0;
}

int soc_gpio_outport_set(uint32_t gpio_port, uint32_t value)
{
	if ((gpio_port >= NUM_MCHP_GPIO_PORTS) || !value) {
		return -EINVAL;
	}

	REG32(MCHP_GPIO_PAROUT_ADDR(gpio_port)) = value;
	return 0;
}

/* Get GPIO pin function */
int soc_gpio_func(uint32_t pin_id, uint32_t *func)
{
	if ((pin_id >= MCHP_GPIO_MAX_ID) || !func) {
		return -EINVAL;
	}

	*func = (REG32(MCHP_GPIO_CTRL_ADDR(pin_id)) >> MCHP_GPIO_CTRL_MUX_POS)
		 & MCHP_GPIO_CTRL_MUX_MASK0;
	return 0;
}

int soc_gpio_func_set(uint32_t pin_id, uint32_t func)
{
	if (pin_id >= MCHP_GPIO_MAX_ID) {
		return -EINVAL;
	}

	REG32(MCHP_GPIO_CTRL_ADDR(pin_id)) =
		(REG32(MCHP_GPIO_CTRL_ADDR(pin_id)) & ~MCHP_GPIO_CTRL_MUX_MASK)
		| ((func << MCHP_GPIO_CTRL_MUX_POS) & MCHP_GPIO_CTRL_MUX_MASK);

	return 0;
}

int soc_gpio_output_en(uint32_t pin_id, uint32_t enable)
{
	if (pin_id >= MCHP_GPIO_MAX_ID) {
		return -EINVAL;
	}

	uint32_t ctrl_addr = MCHP_GPIO_CTRL_ADDR(pin_id);

	if (enable) {
		REG32(ctrl_addr) |= MCHP_GPIO_CTRL_DIR_OUTPUT;
	} else {
		REG32(ctrl_addr) &= ~MCHP_GPIO_CTRL_DIR_OUTPUT;
	}

	return 0;
}

int soc_gpio_config(uint32_t pin_id, uint32_t cfg)
{
	if (pin_id >= MCHP_GPIO_MAX_ID) {
		return -EINVAL;
	}

	uint32_t ctrl_addr = MCHP_GPIO_CTRL_ADDR(pin_id);

	/* disable interrupt detection before touching other fields */
	gpio_idet_set(ctrl_addr, MCHP_GPIO_IDET_DIS);
	gpio_drive_str_set(ctrl_addr, cfg);
	gpio_slew_set(ctrl_addr, cfg);
	gpio_pwrgate_set(ctrl_addr, cfg);

	uint32_t ctrl = REG32(ctrl_addr);

	if (cfg & MCHP_GPIO_CFG_INPAD_DIS) {
		REG32(ctrl_addr) |= MCHP_GPIO_CTRL_INPAD_DIS;
	} else {
		REG32(ctrl_addr) &= ~MCHP_GPIO_CTRL_INPAD_DIS;
	}

	/* is pin currently in output mode? */
	if (ctrl & MCHP_GPIO_CTRL_DIR_OUTPUT) {
		if (!(cfg & MCHP_GPIO_CFG_OUT)) { /* disable output ? */
			REG32(ctrl_addr) &= ~MCHP_GPIO_CTRL_DIR_OUTPUT;
		}

		/* Will enabling internal week pull cause a glitch if pin is
		 * in output mode (driven 0 or 1)?
		 */
		gpio_pud_set(ctrl_addr, cfg);
		gpio_output_buf_type(ctrl_addr, cfg);

		if (cfg & MCHP_GPIO_CFG_PAROUT_EN) {
			REG32(ctrl_addr) |= MCHP_GPIO_CTRL_AOD_DIS;
		} else {
			REG32(ctrl_addr) &= ~MCHP_GPIO_CTRL_AOD_DIS;
		}

		if (cfg & MCHP_GPIO_CFG_OUT_INIT_HI) {
			soc_gpio_output(pin_id, 1);
		} else {
			soc_gpio_output(pin_id, 0);
		}

	} else { /* No, input */
		if (cfg & MCHP_GPIO_CFG_PAROUT_EN) {
			REG32(ctrl_addr) |= MCHP_GPIO_CTRL_AOD_DIS;
		} else {
			REG32(ctrl_addr) &= ~MCHP_GPIO_CTRL_AOD_DIS;
		}

		/* pulls before output */
		gpio_pud_set(ctrl_addr, cfg);
		gpio_output_buf_type(ctrl_addr, cfg);

		/* set output state before enabling output */
		if (cfg & MCHP_GPIO_CFG_OUT_INIT_HI) {
			soc_gpio_output(pin_id, 1);
		} else {
			soc_gpio_output(pin_id, 0);
		}
		if (cfg & MCHP_GPIO_CFG_OUT) {
			REG32(ctrl_addr) |= MCHP_GPIO_CTRL_DIR_OUTPUT;
		}
	}

	gpio_alt_func(ctrl_addr, cfg);

	/* Last set requested interrupt detection */
	gpio_idet_set(ctrl_addr, cfg);

	return 0;
}

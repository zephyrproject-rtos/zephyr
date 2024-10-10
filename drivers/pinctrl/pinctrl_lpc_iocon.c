/*
 * Copyright 2022,2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>

#if !defined(CONFIG_SOC_SERIES_LPC11U6X)
#include <fsl_clock.h>
#endif

/* RT7xx requests to clear the reset bit in RESET module before a IOPCTL can be used. */
#if defined(CONFIG_SOC_SERIES_IMXRT7XX) && defined(CONFIG_SOC_FAMILY_NXP_IMXRT)
#include <fsl_iopctl.h>
#endif


#if defined(CONFIG_SOC_SERIES_IMXRT7XX) && defined(CONFIG_SOC_FAMILY_NXP_IMXRT)
/* For RT7xx , SDK driver need GPIO PORT NO and PIN NO for pin configuration purpose. */
#define IOPCTL_PORT_NO_MASK 0xFF000000
#define IOPCTL_PORT_NO_SHIFT 24
#define IOPCTL_PIN_NO_MASK 0xFF0000
#define IOPCTL_PIN_NO_SHIFT 16
#define IOPCTL_MUX_MASK 0xFFFFUL
#define IOPCTL_MUX_SHIFT 0

#define PORT_NO(mux) (((mux) & IOPCTL_PORT_NO_MASK) >> IOPCTL_PORT_NO_SHIFT)
#define PIN_NO(mux)  (((mux) & IOPCTL_PIN_NO_MASK) >> IOPCTL_PIN_NO_SHIFT)

#else
#define OFFSET(mux) (((mux) & 0xFFF00000) >> 20)
static volatile uint32_t *iocon = (volatile uint32_t *)DT_REG_ADDR(DT_NODELABEL(iocon));
#endif

#define TYPE(mux) (((mux) & 0xC0000) >> 18)

#define IOCON_TYPE_D 0x0
#define IOCON_TYPE_I 0x1
#define IOCON_TYPE_A 0x2

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	for (uint8_t i = 0; i < pin_cnt; i++) {
		uint32_t pin_mux = pins[i];
#if defined(CONFIG_SOC_SERIES_IMXRT7XX) && defined(CONFIG_SOC_FAMILY_NXP_IMXRT)
		uint8_t port_no = PORT_NO(pin_mux);
		uint8_t pin_no = PIN_NO(pin_mux);
#else
		uint32_t offset = OFFSET(pin_mux);
#endif

		/* Check if this is an analog or i2c type pin */
		switch (TYPE(pin_mux)) {
		case IOCON_TYPE_D:
			pin_mux &= Z_PINCTRL_IOCON_D_PIN_MASK;
			break;
		case IOCON_TYPE_I:
			pin_mux &= Z_PINCTRL_IOCON_I_PIN_MASK;
			break;
		case IOCON_TYPE_A:
			pin_mux &= Z_PINCTRL_IOCON_A_PIN_MASK;
			break;
		default:
			/* Should not occur */
			__ASSERT_NO_MSG(TYPE(pin_mux) <= IOCON_TYPE_A);
		}
		/* Set pinmux */

#if defined(CONFIG_SOC_SERIES_IMXRT7XX) && defined(CONFIG_SOC_FAMILY_NXP_IMXRT)
	pin_mux &= IOPCTL_MUX_MASK;
    /* PMIC_I2C_SDA/PMIC_I2C_SCL pin need to be configured separately */
	if ((port_no == 10) && (pin_no == 30)) {
		IOPCTL1->PMIC_I2C_SDA = pin_mux;
	} else if ((port_no == 10) && (pin_no == 31)) {
		IOPCTL1->PMIC_I2C_SCL = pin_mux;
	} else {
		IOPCTL_PinMuxSet(port_no, pin_no, pin_mux);
	}
#else
		*(iocon + offset) = pin_mux;
#endif
	}
	return 0;
}

#if defined(CONFIG_SOC_FAMILY_LPC) && !defined(CONFIG_SOC_SERIES_LPC11U6X)
/* LPC family (except 11u6x) needs iocon clock to be enabled */

static int pinctrl_clock_init(void)
{
	/* Enable IOCon clock */
	CLOCK_EnableClock(kCLOCK_Iocon);
	return 0;
}

SYS_INIT(pinctrl_clock_init, PRE_KERNEL_1, 0);

#endif /* CONFIG_SOC_FAMILY_LPC  && !CONFIG_SOC_SERIES_LPC11U6X */

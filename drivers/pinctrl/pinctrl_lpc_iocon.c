/*
 * Copyright 2022,2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/init.h>

#if !defined(CONFIG_SOC_SERIES_LPC11U6X)
#include <fsl_clock.h>
#endif

/* IOCON register addresses. */
static uint32_t volatile *iocon[] = {
#if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(iocon)))
	(uint32_t *)DT_REG_ADDR(DT_NODELABEL(iocon)),
#else
	NULL,
#endif
#if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(iocon1)))
	(uint32_t *)DT_REG_ADDR(DT_NODELABEL(iocon1)),
#else
	NULL,
#endif
#if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(iocon2)))
	(uint32_t *)DT_REG_ADDR(DT_NODELABEL(iocon2)),
#else
	NULL,
#endif
};

static const struct reset_dt_spec iocon_resets[] = {
	RESET_DT_SPEC_GET_OR(DT_NODELABEL(iocon), {0}),
	RESET_DT_SPEC_GET_OR(DT_NODELABEL(iocon1), {0}),
	RESET_DT_SPEC_GET_OR(DT_NODELABEL(iocon2), {0}),
};

#define OFFSET(mux) (((mux) & 0xFFF00000) >> 20)
#define TYPE(mux) (((mux) & 0xC0000) >> 18)
#define INDEX(mux)  (((mux) & 0x38000) >> 15)

#define IOCON_TYPE_D 0x0
#define IOCON_TYPE_I 0x1
#define IOCON_TYPE_A 0x2

#define PINCTRL_IOCON_ENABLE_CLOCK(node_id)								\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, clocks),							\
		({											\
			const struct device *clock_dev =						\
				DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(node_id, 0));			\
			int ret = 0;									\
													\
			if (!device_is_ready(clock_dev)) {						\
				return -ENODEV;								\
			}										\
													\
			ret = clock_control_on(clock_dev,						\
				(clock_control_subsys_t)DT_CLOCKS_CELL_BY_IDX(node_id, 0, name));	\
			if (ret != 0) {									\
				return ret;								\
			}										\
		}), ())


int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	for (uint8_t i = 0; i < pin_cnt; i++) {
		uint32_t pin_mux = pins[i];
		uint32_t offset = OFFSET(pin_mux);
		uint8_t index = INDEX(pin_mux);

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
		*(iocon[index] + offset) = pin_mux;

	}
	return 0;
}

#if defined(CONFIG_PINCTRL_NXP_IOCON)
static int pinctrl_iocon_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(iocon_resets); i++) {
		const struct reset_dt_spec *reset = &iocon_resets[i];

		if (reset->dev == NULL) {
			continue;
		}

		if (!device_is_ready(reset->dev)) {
			return -ENODEV;
		}

		int ret = reset_line_deassert_dt(reset);

		if (ret != 0) {
			return ret;
		}
	}

	/* LPC family (except 11u6x) needs iocon clock to be enabled */
#if defined(CONFIG_SOC_FAMILY_LPC) && !defined(CONFIG_SOC_SERIES_LPC11U6X)
	/* Enable IOCon clock */
	CLOCK_EnableClock(kCLOCK_Iocon);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(iocon))
	PINCTRL_IOCON_ENABLE_CLOCK(DT_NODELABEL(iocon));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(iocon1))
	PINCTRL_IOCON_ENABLE_CLOCK(DT_NODELABEL(iocon1));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(iocon2))
	PINCTRL_IOCON_ENABLE_CLOCK(DT_NODELABEL(iocon2));
#endif

	return 0;
}

SYS_INIT(pinctrl_iocon_init, PRE_KERNEL_1, 0);

#endif /* CONFIG_PINCTRL_NXP_IOCON */

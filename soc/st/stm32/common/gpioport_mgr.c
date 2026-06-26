/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_gpio

#include <cmsis_core.h>
#include <stm32_ll_gpio.h>
#include <stm32_ll_pwr.h>

#include <stm32_bitops.h>
#include <stm32_hsem.h>
#include <stm32_gpio_shared.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
#ifdef CONFIG_GPIO_LOG_LEVEL
#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#else
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#endif
LOG_MODULE_REGISTER(stm32_gpioport_mgr);

/*
 * Generic macrobatic utilities used later on
 */

/* `idx` if `mapper(elem)` is 1, else nothing */
#define ZIDX_IF_MAPPER_RET_1(idx, elem, mapper) COND_CODE_1(mapper(elem), (idx), ())

/* produces a comma-separated list of indices for elements equal to 1 */
#define ZFILTERED_INDICES(mapper, ...) \
	LIST_DROP_EMPTY(FOR_EACH_IDX_FIXED_ARG(ZIDX_IF_MAPPER_RET_1, (,), mapper, __VA_ARGS__))

#define LAST_LIST_ELEM_INDEX_INNER(_fi_list, _list_sz) \
	COND_CODE_0(_list_sz, (-1), (GET_ARG_N(_list_sz, _fi_list)))

/**
 * Returns the zero-based index of the last `1` in a list which
 * contains only zeroes and ones after per-element mapping.
 *
 * @param mapper Name of a macro `#define MAPPER(e)` that maps an
 * element `e` from the list to the value `0` or `1`
 * @param ...    List of elements
 * @return zero-based index of last `1` in list after mapping,
 * or `-1` if the list contains only `0`s after mapping
 */
#define LAST_LIST_ELEM_INDEX(mapper, ...)				\
	LAST_LIST_ELEM_INDEX_INNER(					\
		ZFILTERED_INDICES(mapper, __VA_ARGS__),			\
		NUM_VA_ARGS(ZFILTERED_INDICES(mapper, __VA_ARGS__)))

/*
 * End of the generic macrobatics
 */

/*
 * Check that the node is active AND has compatible handled by this driver.
 * It is possible for DT nodes to use a `gpioX` nodelabel despite not being
 * an in-SoC GPIO controller, in which case we would try to instantiate them
 * even though this driver does not know how to handle them.
 */
#define GPIOPORT_DEVICE_IS_ACTIVE(port)					\
	UTIL_OR(DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(gpio##port),	\
					  st_stm32_gpio, okay),		\
		DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(gpio##port),	\
					  st_stm32mp2_gpio, okay))

#define GET_GPIOPORT_DEVICE_OR_NULL(port)				\
	COND_CODE_1(GPIOPORT_DEVICE_IS_ACTIVE(port),			\
		    (DEVICE_DT_GET(DT_NODELABEL(gpio##port))), (NULL))

/* UTIL_INC() is needed because LAST_LIST_ELEM_INDEX() is zero-based */
#define LAST_ACTIVE_GPIO_PORT_IDX	\
	UTIL_INC(LAST_LIST_ELEM_INDEX(GPIOPORT_DEVICE_IS_ACTIVE, STM32_GPIO_PORTS_LIST_LWR))
/**
 * @brief Array containing pointers to each GPIO port.
 *
 * Entries will be NULL if the GPIO port is not enabled.
 * Entries past the last enabled GPIO port don't exist
 * (e.g., only 2 entries if GPIOB is last enabled).
 */
static const struct device *const gpio_ports[] = {
	FOR_EACH(GET_GPIOPORT_DEVICE_OR_NULL, (,),
		 GET_ARGS_FIRST_N(LAST_ACTIVE_GPIO_PORT_IDX, STM32_GPIO_PORTS_LIST_LWR))
};

static void ll_gpio_set_pin_pull(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Pull)
{
#if defined(CONFIG_SOC_SERIES_STM32WB0X)
	/* On STM32WB0, the PWRC PU/PD control registers should be used instead
	 * of the GPIO controller registers, so we cannot use LL_GPIO_SetPinPull.
	 */
	const uint32_t gpio = (GPIOx == GPIOA) ? LL_PWR_GPIO_A : LL_PWR_GPIO_B;

	if (Pull == LL_GPIO_PULL_UP) {
		LL_PWR_EnableGPIOPullUp(gpio, Pin);
		LL_PWR_DisableGPIOPullDown(gpio, Pin);
	} else if (Pull == LL_GPIO_PULL_DOWN) {
		LL_PWR_EnableGPIOPullDown(gpio, Pin);
		LL_PWR_DisableGPIOPullUp(gpio, Pin);
	} else if (Pull == LL_GPIO_PULL_NO) {
		LL_PWR_DisableGPIOPullUp(gpio, Pin);
		LL_PWR_DisableGPIOPullDown(gpio, Pin);
	}
#else
	LL_GPIO_SetPinPull(GPIOx, Pin, Pull);
#endif /* CONFIG_SOC_SERIES_STM32WB0X */
}

const struct device *stm32_gpioport_get(uint32_t port_index)
{
	const struct device *dev;

	if (port_index >= ARRAY_SIZE(gpio_ports)) {
		return NULL;
	}

	dev = gpio_ports[port_index];

	return device_is_ready(dev) ? dev : NULL;
}

int stm32_gpioport_configure_pin(const struct device *port,
				 gpio_pin_t pin,
				 pinctrl_soc_pin_t config,
				 bool apply_out_level)
{
	const struct gpio_stm32_config *dev_cfg = port->config;
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)dev_cfg->base;
	uint32_t pin_ll = stm32_gpiomgr_pinnum_to_ll_val(pin);

#ifdef CONFIG_SOC_SERIES_STM32F1X
	const uint32_t cnfmode = config & STM32_CNFMODE_Msk;

	if (cnfmode == STM32_CNFMODE_INPUT_PUPD) {
		/* Input with pull-up/down: configure the PU/PD resistor */
		const uint32_t pupd = config & STM32_PUPD_Msk;

		if (pupd == STM32_PUPD_PULL_UP) {
			ll_gpio_set_pin_pull(gpio, pin_ll, LL_GPIO_PULL_UP);
		} else {
			ll_gpio_set_pin_pull(gpio, pin_ll, LL_GPIO_PULL_DOWN);
		}
	} else {
		/*
		 * Requested configuration is not Input with pull-up/down.
		 * It must be Input floating, Analog, GP Output or AF Output.
		 *
		 * The only one which requires special handling is GP Output
		 * where we need to set the level if requested by caller.
		 *
		 * If MODE is not INPUT (i.e., it is OUT_MAX_SPEED_nMHZ),
		 * the requested configuration is AF Output / GP Output
		 * depending on whether the ALT_FUNCTION bit is set.
		 */
		const bool is_gp_output = ((cnfmode & STM32_MODE_Msk) != STM32_MODE_INPUT)
			&& ((cnfmode & STM32_CNF_OUT_ALT_FUNCTION) == 0);

		if (apply_out_level && is_gp_output) {
			const uint32_t odata = _FLD2VAL(STM32_ODR, config);

			/* Avoid LL overhead by writing directly to BSRR/BRR */
			if (odata == 1) {
				stm32_reg_write(&gpio->BSRR, BIT(pin));
			} else {
				stm32_reg_write(&gpio->BRR, BIT(pin));
			}
		}
	}

	/* Write CNFMODE value directly to avoid decoding and LL overhead */
	volatile uint32_t *const crlh = (pin <= 7) ? &gpio->CRL : &gpio->CRH;
	const uint32_t shift = GPIO_CRL_MODE1_Pos * (pin % 8u);

	stm32_reg_modify_bits(crlh,
		(GPIO_CRL_CNF0_Msk | GPIO_CRL_MODE0_Msk) << shift,
		_FLD2VAL(STM32_CNFMODE, config) << shift);
#else /* CONFIG_SOC_SERIES_STM32F1X */
	const uint32_t mode = config & STM32_MODER_Msk;

	z_stm32_hsem_lock(CFG_HW_GPIO_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	if (mode == STM32_MODER_ALT_MODE) {
		/* Alternate Function mode: configure AF mux */
		const uint32_t afnum = _FLD2VAL(STM32_AF, config);

		if (pin <= 7) {
			LL_GPIO_SetAFPin_0_7(gpio, pin_ll, afnum);
		} else {
			LL_GPIO_SetAFPin_8_15(gpio, pin_ll, afnum);
		}
		/* Configure output type - it also affects operation in AF mode */
		LL_GPIO_SetPinOutputType(gpio, pin_ll, _FLD2VAL(STM32_OTYPER, config));
	} else if (mode == STM32_MODER_OUTPUT_MODE) {
		/* Output mode: configure output type and set level if requested */
		LL_GPIO_SetPinOutputType(gpio, pin_ll, _FLD2VAL(STM32_OTYPER, config));

		if (apply_out_level) {
			const uint32_t odata = _FLD2VAL(STM32_ODR, config);

			if (odata == 1) {
				LL_GPIO_SetOutputPin(gpio, pin_ll);
			} else {
				LL_GPIO_ResetOutputPin(gpio, pin_ll);
			}
		}
	} else {
		/*
		 * Input/Analog mode: there's usually nothing to do...
		 */
#if defined(CONFIG_SOC_SERIES_STM32L4X) && defined(GPIO_ASCR_ASC0)
		/*
		 * ...except for STM32L47xx/48xx where register ASCR should be
		 * configured to connect analog switch of GPIO lines to the ADC.
		 */
		if (mode == STM32_MODER_ANALOG_MODE) {
			LL_GPIO_EnablePinAnalogControl(gpio, pin_ll);
		}
#endif /* CONFIG_SOC_SERIES_STM32L4X && defined(GPIO_ASCR_ASC0) */
	}

	/* Apply generic parameters (identical regardless of mode) */
	LL_GPIO_SetPinSpeed(gpio, pin_ll, _FLD2VAL(STM32_OSPEEDR, config));

	ll_gpio_set_pin_pull(gpio, pin_ll, _FLD2VAL(STM32_PUPDR, config));

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_pinctrl)
	uint32_t piocfgr = _FLD2VAL(STM32_ADVCFGR, config);
	uint32_t delayr = _FLD2VAL(STM32_DELAYR, config);

	if (pin <= 7) {
		LL_GPIO_SetDelayPin_0_7(gpio, pin_ll, delayr);
		LL_GPIO_SetPIOControlPin_0_7(gpio, pin_ll, piocfgr);
	} else {
		LL_GPIO_SetDelayPin_8_15(gpio, pin_ll, delayr);
		LL_GPIO_SetPIOControlPin_8_15(gpio, pin_ll, piocfgr);
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_pinctrl) */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h5_pinctrl)
	uint32_t hslv = _FLD2VAL(STM32_HSLVR, config);

	if (hslv) {
		LL_GPIO_EnableHighSPeedLowVoltage(gpio, pin_ll);
	} else {
		LL_GPIO_DisableHighSPeedLowVoltage(gpio, pin_ll);
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h5_pinctrl) */

	/*
	 * Configure pin mode last after all other parameters
	 * have been applied properly.
	 */
	LL_GPIO_SetPinMode(gpio, pin_ll, _FLD2VAL(STM32_MODER, config));

	z_stm32_hsem_unlock(CFG_HW_GPIO_SEMID);
#endif  /* CONFIG_SOC_SERIES_STM32F1X */

	return 0;
}

static int stm32_gpioport_pm_action(const struct device *dev,
				    enum pm_device_action action)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct gpio_stm32_config *cfg = dev->config;
	clock_control_subsys_t subsys = (void *)&cfg->pclken;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return clock_control_on(clk, subsys);
	case PM_DEVICE_ACTION_SUSPEND:
		return clock_control_off(clk, subsys);
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

__maybe_unused static int stm32_gpioport_init(const struct device *dev)
{
#if (defined(PWR_CR2_IOSV) || defined(PWR_SVMCR_IO2SV)) && \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpiog))
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	/* Port G[15:2] requires external power supply */
	/* Cf: L4/L5/U3 RM, Chapter "Independent I/O supply rail" */
#ifdef CONFIG_SOC_SERIES_STM32U3X
	LL_PWR_EnableVDDIO2();
#else
	LL_PWR_EnableVddIO2();
#endif
	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);
#endif

	return pm_device_driver_init(dev, stm32_gpioport_pm_action);
}

#if !defined(CONFIG_GPIO_STM32)
/*
 * The GPIO API is implemented entirely in the header file
 * so it can be *called* even when CONFIG_GPIO=n. A build
 * error would usually occur if one attempted to do that
 * since you also need a reference to the GPIO device to
 * use the API, and those devices are not instantiated when
 * their driver doesn't exist... but in our case, we always
 * instantiate the GPIO port nodes even when CONFIG_GPIO=n.
 * We originally set the `api` pointer to NULL in this case
 * but this is actually quite dangerous, since applications
 * that tried to use the GPIO API without enabling CONFIG_GPIO
 * would build fine but crash at runtime when the GPIO API
 * functions dereference the NULL `api` pointer.
 *
 * Provide a dummy implementation of the GPIO API where all
 * mandatory functions return -ENOSYS after printing a big
 * error message such that applications don't crash anymore.
 * Optional functions are not implemented to lower footprint.
 *
 * A broader rework would be necessary to allow not instancing
 * any of the GPIO port devices when CONFIG_GPIO_STM32=n, which
 * would restore the previous build-time-error behavior; for
 * now, this shall suffice.
 */
static int illegal_gpio_api_usage(void)
{
	LOG_ERR("Attempting to use GPIO API without enabling it!");
	return -ENOSYS;
}

static int dummy_gpio_pin_configure(const struct device *port, gpio_pin_t pin,
				    gpio_flags_t flags)
{
	ARG_UNUSED(port); ARG_UNUSED(pin); ARG_UNUSED(flags);
	return illegal_gpio_api_usage();
}

static int dummy_gpio_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	ARG_UNUSED(port); ARG_UNUSED(value);
	return illegal_gpio_api_usage();
}

static int dummy_gpio_port_set_masked_raw(const struct device *port,
					  gpio_port_pins_t mask, gpio_port_value_t value)
{
	ARG_UNUSED(port); ARG_UNUSED(mask); ARG_UNUSED(value);
	return illegal_gpio_api_usage();
}

static int dummy_gpio_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	ARG_UNUSED(port); ARG_UNUSED(pins);
	return illegal_gpio_api_usage();
}

static int dummy_gpio_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	ARG_UNUSED(port); ARG_UNUSED(pins);
	return illegal_gpio_api_usage();
}

static int dummy_gpio_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	ARG_UNUSED(port); ARG_UNUSED(pins);
	return illegal_gpio_api_usage();
}

static DEVICE_API(gpio, dummy_gpio_api) = {
	.pin_configure = dummy_gpio_pin_configure,
	.port_get_raw = dummy_gpio_port_get_raw,
	.port_set_masked_raw = dummy_gpio_port_set_masked_raw,
	.port_set_bits_raw = dummy_gpio_port_set_bits_raw,
	.port_clear_bits_raw = dummy_gpio_port_clear_bits_raw,
	.port_toggle_bits = dummy_gpio_port_toggle_bits,
};
#endif /* !CONFIG_GPIO_STM32 */

#if defined(CONFIG_GPIO)
#define GPIO_PORT_INIT_PRIORITY CONFIG_GPIO_INIT_PRIORITY
#else /* CONFIG_GPIO */
/*
 * Use the default value of CONFIG_GPIO_INIT_PRIORITY
 * as init priority if the GPIO subsystem is not enabled.
 */
#define GPIO_PORT_INIT_PRIORITY CONFIG_KERNEL_INIT_PRIORITY_DEFAULT
#endif /* CONFIG_GPIO */

#define GPIO_PORT_DEVICE_INIT(__node, __suffix, __base_addr, __port)		\
	static const struct gpio_stm32_config gpio_stm32_cfg_## __suffix = {	\
		.common = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(__node),	\
		},								\
		.base = (void *)__base_addr,					\
		.port = __port,							\
		IF_ENABLED(DT_NODE_HAS_PROP(__node, clocks),			\
			   (.pclken = STM32_CLOCK_INFO(0, __node),))		\
	};									\
										\
	static struct gpio_stm32_data gpio_stm32_data_## __suffix;		\
										\
	PM_DEVICE_DT_DEFINE(__node, stm32_gpioport_pm_action);			\
										\
	DEVICE_DT_DEFINE(__node,						\
			 COND_CODE_1(DT_NODE_HAS_PROP(__node, clocks),		\
				     (stm32_gpioport_init),			\
				     (NULL)),					\
			 PM_DEVICE_DT_GET(__node),				\
			 &gpio_stm32_data_## __suffix,				\
			 &gpio_stm32_cfg_## __suffix,				\
			 PRE_KERNEL_1,						\
			 GPIO_PORT_INIT_PRIORITY,				\
			 COND_CODE_1(IS_ENABLED(CONFIG_GPIO_STM32),		\
				(&gpio_stm32_driver), (&dummy_gpio_api)))

#define GPIO_PORT_DEVICE_INIT_STM32(__suffix, __SUFFIX)				\
	GPIO_PORT_DEVICE_INIT(DT_NODELABEL(gpio##__suffix),			\
			 __suffix,						\
			 DT_REG_ADDR(DT_NODELABEL(gpio##__suffix)),		\
			 STM32_PORT##__SUFFIX)

#define GPIO_PORT_DEVICE_INIT_STM32_IF_OKAY(__suffix, __SUFFIX)			\
	IF_ENABLED(GPIOPORT_DEVICE_IS_ACTIVE(__suffix),				\
		   (GPIO_PORT_DEVICE_INIT_STM32(__suffix, __SUFFIX)))

#define DEVICE_INIT_IF_OKAY(idx, __suffix)				\
	GPIO_PORT_DEVICE_INIT_STM32_IF_OKAY(__suffix,			\
		GET_ARG_N(UTIL_INC(idx), STM32_GPIO_PORTS_LIST_UPR))

FOR_EACH_IDX(DEVICE_INIT_IF_OKAY, (;), STM32_GPIO_PORTS_LIST_LWR);

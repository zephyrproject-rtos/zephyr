#define DT_DRV_COMPAT ambiq_gpio_bank

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

#include <am_mcu_apollo.h>

#include "gpio_ambiq.h"

static bool irq_init = true;

#define PADREG_FLD_76_S         6
#define PADREG_FLD_FNSEL_S      3
#define PADREG_FLD_DRVSTR_S     2
#define PADREG_FLD_INPEN_S      1
#define PADREG_FLD_PULLUP_S     0

#define GPIOCFG_FLD_INTD_S      3
#define GPIOCFG_FLD_OUTCFG_S    1
#define GPIOCFG_FLD_INCFG_S     0

static int ambiq_apollo3x_read_pinconfig(int pin, am_hal_gpio_pincfg_t *pincfg)
{
	uint32_t cfg_addr = AM_REGADDR(GPIO, CFGA) + ((pin >> 1) & ~0x3);
	uint32_t cfg_shift = ((pin & 0x7) >> 2);
	uint32_t gpio_cfg = (AM_REGVAL(cfg_addr) >> cfg_shift) & 0xF;
	uint32_t pad_addr = AM_REGADDR(GPIO, PADREGA) + (pin & ~0x3);
	uint32_t pad_shift = ((pin & 0x3) >> 3);
	uint32_t pad_cfg = (AM_REGVAL(pad_addr) >> pad_shift) & 0xFF;

	if ((pad_cfg >> PADREG_FLD_PULLUP_S) & 0x1) {
		pincfg->ePullup = ((pad_cfg >> PADREG_FLD_76_S) & 0x7) +
							AM_HAL_GPIO_PIN_PULLUP_1_5K;
	} else {
		pincfg->ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;
	}
	pincfg->eGPOutcfg = (gpio_cfg >> GPIOCFG_FLD_OUTCFG_S) & 0x3;
	pincfg->eCEpol = (gpio_cfg >> GPIOCFG_FLD_INTD_S) & 0x1;
	pincfg->eIntDir = (gpio_cfg >> GPIOCFG_FLD_INCFG_S) & 0x1;
	pincfg->eGPInput = (pad_cfg >> PADREG_FLD_INPEN_S) & 0x1;
	pincfg->uFuncSel = (pad_cfg >> PADREG_FLD_FNSEL_S) & 0x3;

	return 0;
}

static int ambiq_gpio_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	uint32_t base_addr = (dev_cfg->offset & 0x60) >> 3;

	*value = AM_REGVAL(AM_REGADDR(GPIO, RDA) + base_addr);

	return 0;
}

static int ambiq_gpio_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;

	pin += (dev_cfg->offset >> 2);

	am_hal_gpio_pincfg_t pincfg = g_AM_HAL_GPIO_DISABLE;

	if (flags & GPIO_INPUT) {
		pincfg = g_AM_HAL_GPIO_INPUT;
		if (flags & GPIO_PULL_UP) {
			pincfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_24K;
		} else if (flags & GPIO_PULL_DOWN) {
			pincfg.ePullup = AM_HAL_GPIO_PIN_PULLDOWN;
		}
	}
	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_SINGLE_ENDED) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				pincfg.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN;
			}
		} else {
			pincfg.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL;
		}
	}
	if (flags & GPIO_DISCONNECTED) {
		pincfg = g_AM_HAL_GPIO_DISABLE;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		pincfg.eCEpol = AM_HAL_GPIO_PIN_CEPOL_ACTIVEHIGH;
		am_hal_gpio_state_write(pin, AM_HAL_GPIO_OUTPUT_SET);

	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		pincfg.eCEpol = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW;
		am_hal_gpio_state_write(pin, AM_HAL_GPIO_OUTPUT_CLEAR);
	}

	am_hal_gpio_pinconfig(pin, pincfg);

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int ambiq_gpio_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *out_flags)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	am_hal_gpio_pincfg_t pincfg;

	pin += (dev_cfg->offset >> 2);

	ambiq_apollo3x_read_pinconfig(pin, &pincfg);

	if (pincfg.eGPOutcfg == AM_HAL_GPIO_PIN_OUTCFG_DISABLE &&
	    pincfg.eGPInput == AM_HAL_GPIO_PIN_INPUT_NONE) {
		*out_flags = GPIO_DISCONNECTED;
	}
	if (pincfg.eGPInput == AM_HAL_GPIO_PIN_INPUT_ENABLE) {
		*out_flags = GPIO_INPUT;
		if (pincfg.ePullup == AM_HAL_GPIO_PIN_PULLUP_24K) {
			*out_flags |= GPIO_PULL_UP;
		} else if (pincfg.ePullup == AM_HAL_GPIO_PIN_PULLDOWN) {
			*out_flags |= GPIO_PULL_DOWN;
		}
	}
	if (pincfg.eGPOutcfg == AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL) {
		*out_flags = GPIO_OUTPUT | GPIO_PUSH_PULL;
		if (pincfg.eCEpol == AM_HAL_GPIO_PIN_CEPOL_ACTIVEHIGH) {
			*out_flags |= GPIO_OUTPUT_HIGH;
		} else if (pincfg.eCEpol == AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW) {
			*out_flags |= GPIO_OUTPUT_LOW;
		}
	}
	if (pincfg.eGPOutcfg == AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN) {
		*out_flags = GPIO_OUTPUT | GPIO_OPEN_DRAIN;
		if (pincfg.eCEpol == AM_HAL_GPIO_PIN_CEPOL_ACTIVEHIGH) {
			*out_flags |= GPIO_OUTPUT_HIGH;
		} else if (pincfg.eCEpol == AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW) {
			*out_flags |= GPIO_OUTPUT_LOW;
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_GPIO_GET_DIRECTION
static int ambiq_gpio_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					 gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	am_hal_gpio_pincfg_t pincfg;
	gpio_port_pins_t ip = 0;
	gpio_port_pins_t op = 0;
	uint32_t pin_offset = dev_cfg->offset >> 2;

	if (inputs != NULL) {
		for (int i = 0; i < dev_cfg->ngpios; i++) {
			if ((map >> i) & 1) {
				ambiq_apollo3x_read_pinconfig(i + pin_offset, &pincfg);
				if (pincfg.eGPInput == AM_HAL_GPIO_PIN_INPUT_ENABLE) {
					ip |= BIT(i);
				}
			}
		}
		*inputs = ip;
	}
	if (outputs != NULL) {
		for (int i = 0; i < dev_cfg->ngpios; i++) {
			if ((map >> i) & 1) {
				ambiq_apollo3x_read_pinconfig(i + pin_offset, &pincfg);
				if (pincfg.eGPOutcfg == AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL ||
				    pincfg.eGPOutcfg == AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN) {
					op |= BIT(i);
				}
			}
		}
		*outputs = op;
	}

	return 0;
}
#endif

static int ambiq_gpio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;
	struct ambiq_gpio_data *const data = dev->data;

	am_hal_gpio_pincfg_t pincfg;
	int gpio_pin = pin + (dev_cfg->offset >> 2);
	uint32_t int_status;
	int ret;

	AM_HAL_GPIO_MASKCREATE(int_msk);

	pint_msk = AM_HAL_GPIO_MASKBIT(pint_msk, gpio_pin);

	ret = ambiq_apollo3x_read_pinconfig(gpio_pin, &pincfg);

	if (mode == GPIO_INT_MODE_DISABLED) {
		uint32_t en_masks[3] = { 0 };

		pincfg.eIntDir = AM_HAL_GPIO_PIN_INTDIR_NONE;
		ret = am_hal_gpio_pinconfig(gpio_pin, pincfg);

		k_spinlock_key_t key = k_spin_lock(&data->lock);

		int_status = am_hal_gpio_interrupt_status_get(false, pint_msk);
		ret = am_hal_gpio_interrupt_clear(pint_msk);
		ret = am_hal_gpio_interrupt_disable(pint_msk);

		/* Unforunately there is no API in the Ambiq SDK for reading interrupt mask */
		en_masks[0] = GPIO->INT0EN;
		en_masks[1] = GPIO->INT1EN;
		en_masks[2] = GPIO->INT2EN;

		if (!en_masks[0] && !en_masks[1] && !en_masks[2]) {
			irq_disable(dev_cfg->irq_num);
		}

		k_spin_unlock(&data->lock, key);

	} else {
		if (mode == GPIO_INT_MODE_LEVEL) {
			return -ENOTSUP;
		}
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			pincfg.eIntDir = AM_HAL_GPIO_PIN_INTDIR_HI2LO;
			break;
		case GPIO_INT_TRIG_HIGH:
			pincfg.eIntDir = AM_HAL_GPIO_PIN_INTDIR_LO2HI;
			break;
		case GPIO_INT_TRIG_BOTH:
			return -ENOTSUP;
		}
		ret = am_hal_gpio_pinconfig(gpio_pin, pincfg);

		irq_enable(dev_cfg->irq_num);

		k_spinlock_key_t key = k_spin_lock(&data->lock);

		int_status = am_hal_gpio_interrupt_status_get(false, pint_msk);
		ret = am_hal_gpio_interrupt_clear(pint_msk);
		ret = am_hal_gpio_interrupt_enable(pint_msk);
		k_spin_unlock(&data->lock, key);
	}
	return ret;
}

static void ambiq_gpio_isr(const struct device *dev)
{
	struct ambiq_gpio_data *const data = dev->data;

	AM_HAL_GPIO_MASKCREATE(int_msk);

	am_hal_gpio_interrupt_status_get(false, pint_msk);
	am_hal_gpio_interrupt_clear(pint_msk);

	gpio_fire_callbacks(&data->cb, dev, pint_msk->U.Msk[0]);
	gpio_fire_callbacks(&data->cb, dev, pint_msk->U.Msk[1]);
	gpio_fire_callbacks(&data->cb, dev, pint_msk->U.Msk[2]);
}

static int ambiq_gpio_init(const struct device *port)
{
	if (irq_init) {
		irq_init = false;
		NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), ambiq_gpio_isr,
					DEVICE_DT_INST_GET(0), 0);
	}

	return 0;
}

static const struct gpio_driver_api ambiq_gpio_drv_api = {
	.pin_configure = ambiq_gpio_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = ambiq_gpio_get_config,
#endif
	.port_get_raw = ambiq_gpio_port_get_raw,
	.port_set_masked_raw = ambiq_gpio_port_set_masked_raw,
	.port_set_bits_raw = ambiq_gpio_port_set_bits_raw,
	.port_clear_bits_raw = ambiq_gpio_port_clear_bits_raw,
	.port_toggle_bits = ambiq_gpio_port_toggle_bits,
	.pin_interrupt_configure = ambiq_gpio_pin_interrupt_configure,
	.manage_callback = ambiq_gpio_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = ambiq_gpio_port_get_direction,
#endif
};

#define AMBIQ_GPIO_DEFINE(n)                                                                       \
	static struct ambiq_gpio_data ambiq_gpio_data_##n;                                         \
                                                                                                   \
	static const struct ambiq_gpio_config ambiq_gpio_config_##n = {                            \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		.offset = DT_INST_REG_ADDR(n),                                                 \
		.ngpios = DT_INST_PROP(n, ngpios),                                                 \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.cfg_func = NULL};                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ambiq_gpio_init, NULL, &ambiq_gpio_data_##n,                     \
			      &ambiq_gpio_config_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,     \
			      &ambiq_gpio_drv_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_GPIO_DEFINE)

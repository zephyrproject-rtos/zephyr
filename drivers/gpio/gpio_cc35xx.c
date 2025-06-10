/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_gpio_port

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <GPIO.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_hostmcu_aon.h>
#include <inc/hw_soc_aon.h>
#include <inc/hw_ints.h>

/*
 * Below defines are copied from:
 * modules/hal/ti/simplelink_wifi/source/ti/drivers/gpio/GPIOWFF3.c
 */

/* Missing defines from hw_hostmcu_aon.h */
#define HOSTMCU_AON_CFGWICSNS_GPIO_AND_EN (1U << 1)
#define HOSTMCU_AON_CFGWICSNS_GPIO_OR_EN  (1U << 2)
#define HOSTMCU_AON_CFGWUTP_GPIO_AND_FAST (1U << 1)
#define HOSTMCU_AON_CFGWUTP_GPIO_OR_FAST  (1U << 2)

/* The size of each IO region in IOMUX id 4KB */
#define IOMUX_IO_REGION_SIZE (4096U)

#define IOMUX_CFG_ADDR(index)                                                                      \
	(IOMUX_BASE + IOMUX_O_SCLKICFG + ((index) * IOMUX_IO_REGION_SIZE) +                        \
	 (IOMUX_O_GPIO2CFG - IOMUX_O_GPIO2CFG))
#define IOMUX_PULLCTL_ADDR(index)                                                                  \
	(IOMUX_BASE + IOMUX_O_SCLKICFG + ((index) * IOMUX_IO_REGION_SIZE) +                        \
	 (IOMUX_O_GPIO2PCTL - IOMUX_O_GPIO2CFG))
#define IOMUX_CTL_ADDR(index)                                                                      \
	(IOMUX_BASE + IOMUX_O_SCLKICFG + ((index) * IOMUX_IO_REGION_SIZE) +                        \
	 (IOMUX_O_GPIO2CTL - IOMUX_O_GPIO2CFG))
#define IOMUX_EVTCTL_ADDR(index)                                                                   \
	(IOMUX_BASE + IOMUX_O_SCLKICFG + ((index) * IOMUX_IO_REGION_SIZE) +                        \
	 (IOMUX_O_GPIO2ECTL - IOMUX_O_GPIO2CFG))
#define IOMUX_PORTCFG_ADDR(index) (IOMUX_BASE + IOMUX_O_SCLKIPCFG + ((index) << 2))

struct cc35xx_gpio_port_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint8_t ngpios;
	uint32_t gpio_index_offset;
};

struct cc35xx_gpio_port_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

/*
 * The "cc35xx_ll_*" functions come from TI HAL and were copied almost
 * as-is, with a few exceptions to type changes and style to stay consistent
 * with the rest of the Zephyr code.
 *
 * The cc35xx HW is organized into a single GPIO bank with single index space
 * (0..44) and that's reflected in HAL functions - they accept the "global"
 * index value (i.e. 0-44).
 *
 * The Zephyr driver organizes GPIO pins into 2 banks with up to 32 pins
 * per bank. This is done to stay consistent with Zephyr GPIO API.
 * This means that Zephyr API functions accept GPIO index as per-bank value
 * (0..<ngpios>, where <ngpios> is the number of pins in given GPIO bank).
 *
 * The API functions also translate Zephyr GPIO flags to/from their
 * HAL counterparts.
 */

static uint8_t cc35xx_ll_gpio_read(uint8_t index)
{
	uint32_t ctlRegAddr = IOMUX_CTL_ADDR(index);

	return (sys_read32(ctlRegAddr) & IOMUX_GPIO2CTL_PADVAL_M) >> IOMUX_GPIO2CTL_PADVAL_S;
}

static void cc35xx_ll_gpio_write(uint8_t index, uint32_t value)
{
	uint32_t ctlRegAddr;
	uint32_t tmpctlReg;
	uintptr_t key;

	ctlRegAddr = IOMUX_CTL_ADDR(index);

	/* Protect read-modify-write operation to write to GPIO. */
	key = irq_lock();
	tmpctlReg = sys_read32(ctlRegAddr);
	tmpctlReg &= ~IOMUX_GPIO2CTL_OUT_M;
	tmpctlReg |= (value & 0x1) << IOMUX_GPIO2CTL_OUT_S;
	sys_write32(tmpctlReg, ctlRegAddr);
	irq_unlock(key);
}

static void cc35xx_ll_gpio_toggle(uint8_t index)
{
	uint32_t ctlRegAddr;
	uint32_t tmpctlReg;
	uintptr_t key;

	ctlRegAddr = IOMUX_CTL_ADDR(index);

	/* Protect read-modify-write operation to toggle GPIO. */
	key = irq_lock();
	tmpctlReg = sys_read32(ctlRegAddr);
	tmpctlReg ^= IOMUX_GPIO2CTL_OUT;
	sys_write32(tmpctlReg, ctlRegAddr);
	irq_unlock(key);
}

static void cc35xx_ll_gpio_disable_int(uint8_t index)
{
	uintptr_t key;
	uint32_t val;

	/* Clear bit in IMASK register to disable interrupts */
	key = irq_lock();
	if (index >= 32) {
		/* Disable interrupt by clearing functional mask bit */
		val = sys_read32(SOC_AON_BASE + SOC_AON_O_GPIOFNC1S);
		val &= ~(1 << (index - 32));
		sys_write32(val, (SOC_AON_BASE + SOC_AON_O_GPIOFNC1S));

		/*
		 * Disable wakeup event by setting the GPIO OR wakeup event mask bit.
		 * When the bit is set, the event is disabled.
		 */
		val = sys_read32(HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUOR1);
		val |= (1 << (index - 32));
		sys_write32(val, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUOR1));
	} else {
		/* Disable interrupt by clearing functional mask bit */
		val = sys_read32(SOC_AON_BASE + SOC_AON_O_GPIOFNC0S);
		val &= ~(1 << index);
		sys_write32(val, (SOC_AON_BASE + SOC_AON_O_GPIOFNC0S));

		/*
		 * Disable wakeup event by setting the GPIO OR wakeup event mask bit.
		 * When the bit is set, the event is disabled.
		 */
		val = sys_read32(HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUOR);
		val |= (1 << index);
		sys_write32(val, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUOR));
	}
	irq_unlock(key);
}

static void cc35xx_ll_gpio_enable_int(uint8_t index)
{
	uintptr_t key;
	uint32_t val;

	/* Set bit in IMASK register to enable interrupts */
	key = irq_lock();
	if (index >= 32) {
		/* Enable interrupt by setting functional mask bit */
		val = sys_read32(SOC_AON_BASE + SOC_AON_O_GPIOFNC1S);
		val |= (1 << (index - 32));
		sys_write32(val, (SOC_AON_BASE + SOC_AON_O_GPIOFNC1S));

		/*
		 * Enable wakeup event by clearing the GPIO OR wakeup event mask bit.
		 * When the bit is cleared, the event is enabled.
		 */
		val = sys_read32(HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUOR1);
		val &= ~(1 << (index - 32));
		sys_write32(val, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUOR1));
	} else {
		/* Enable interrupt by setting functional mask bit (IMASK equivalent) */
		val = sys_read32(SOC_AON_BASE + SOC_AON_O_GPIOFNC0S);
		val |= (1 << index);
		sys_write32(val, (SOC_AON_BASE + SOC_AON_O_GPIOFNC0S));

		/*
		 * Enable wakeup event by clearing the GPIO OR wakeup event mask bit.
		 * When the bit is cleared, the event is enabled.
		 */
		val = sys_read32(HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUOR);
		val &= ~(1 << index);
		sys_write32(val, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUOR));
	}
	irq_unlock(key);
}

static void cc35xx_ll_gpio_clear_int(uint8_t index)
{
	uintptr_t key;
	uint32_t registerAddr;
	uint32_t val;

	registerAddr = IOMUX_EVTCTL_ADDR(index);

	key = irq_lock();
	val = sys_read32(registerAddr);
	val |= IOMUX_GPIO2ECTL_CLR;
	sys_write32(val, registerAddr);
	irq_unlock(key);
}

static void cc35xx_ll_gpio_set_config_and_mux(uint8_t index, uint32_t config, uint32_t mux)
{
	uint32_t cfgRegAddr;
	uint32_t pullctlRegAddr;
	uint32_t ctlRegAddr;
	uint32_t evtctlRegAddr;
	uint32_t portcfgRegAddr;
	uint32_t tmpCfgReg;
	uint32_t tmpPullctlCfgReg;
	uint32_t tmpctlReg;
	uint32_t tmpEvtctlReg;
	uint32_t tmpPortcfgReg;

	cfgRegAddr = IOMUX_CFG_ADDR(index);
	pullctlRegAddr = IOMUX_PULLCTL_ADDR(index);
	ctlRegAddr = IOMUX_CTL_ADDR(index);
	evtctlRegAddr = IOMUX_EVTCTL_ADDR(index);
	portcfgRegAddr = IOMUX_PORTCFG_ADDR(index);

	/* Extract register-specific values from compressed pin config */
	tmpCfgReg = (config & GPIOWFF3_CFG_CFG_M) >> GPIOWFF3_CFG_CFG_S;
	tmpPullctlCfgReg = (config & GPIOWFF3_CFG_PULLCTL_M) >> GPIOWFF3_CFG_PULLCTL_S;
	tmpctlReg = (config & GPIOWFF3_CFG_CTL_M) >> GPIOWFF3_CFG_CTL_S;
	tmpEvtctlReg = (config & GPIOWFF3_CFG_EVTCTL_M) >> GPIOWFF3_CFG_EVTCTL_S;
	tmpPortcfgReg = (mux << IOMUX_GPIO2PCFG_IOSEL_S) & IOMUX_GPIO2PCFG_IOSEL_M;

	/*
	 * If the IO is muxed to the analog IP for that IO, then the analog switch
	 * must be controlled by the analog IP. If not, the analog switch must be
	 * controlled by the SW to keep the switch open.
	 * If tmpCfgReg is kept unmodified from above, the analog switch will be
	 * controlled by the analog IP. So the value of tmpCfgReg needs to be
	 * changed if the IO is not muxed to the analog IP.
	 */
	if (!(mux & GPIOWFF3_MUX_ANALOG_INTERNAL)) {
		/*
		 * Enable analog switch control override, and set the analog switch to
		 * be disabled (open)
		 */
		tmpCfgReg |= (IOMUX_GPIO2CFG_ANASWOVREN_ENABLE | IOMUX_GPIO2CFG_ANASW_DISABLE);
	}

	if (mux == GPIO_MUX_GPIO) {
		/*
		 * Mux to GPIO functionality.
		 * Change muxing after changing configuration to prevent glitching.
		 */
		sys_write32(tmpCfgReg, cfgRegAddr);
		sys_write32(tmpPullctlCfgReg, pullctlRegAddr);
		sys_write32(tmpctlReg, ctlRegAddr);
		sys_write32(tmpEvtctlReg, evtctlRegAddr);
		sys_write32(tmpPortcfgReg, portcfgRegAddr);
	} else {
		/*
		 * Change muxing before changing configuration
		 * This is to prevent glitching. If output was previously overridden, and
		 * if it will not be overridden by the new configuration, then the override
		 * will be disabled after the muxing has been changed.
		 */
		sys_write32(tmpPortcfgReg, portcfgRegAddr);
		sys_write32(tmpCfgReg, cfgRegAddr);
		sys_write32(tmpPullctlCfgReg, pullctlRegAddr);
		sys_write32(tmpctlReg, ctlRegAddr);
		sys_write32(tmpEvtctlReg, evtctlRegAddr);
	}

	if (config & GPIO_CFG_INT_ENABLE) {
		cc35xx_ll_gpio_enable_int(index);
	} else {
		cc35xx_ll_gpio_disable_int(index);
	}
}

#ifdef CONFIG_GPIO_GET_CONFIG
static void cc35xx_ll_gpio_get_config(uint8_t index, uint32_t *config)
{
	uint32_t cfgRegAddr;
	uint32_t pullctlRegAddr;
	uint32_t ctlRegAddr;
	uint32_t evtctlRegAddr;
	uint32_t tmpCfgReg;
	uint32_t tmpPullctlCfgReg;
	uint32_t tmpctlReg;
	uint32_t tmpEvtctlReg;
	uint32_t configValue;
	uint32_t tmpCfgRegBit;

	cfgRegAddr = IOMUX_CFG_ADDR(index);
	pullctlRegAddr = IOMUX_PULLCTL_ADDR(index);
	ctlRegAddr = IOMUX_CTL_ADDR(index);
	evtctlRegAddr = IOMUX_EVTCTL_ADDR(index);

	tmpCfgReg = sys_read32(cfgRegAddr);
	tmpPullctlCfgReg = sys_read32(pullctlRegAddr);
	tmpctlReg = sys_read32(ctlRegAddr);
	tmpEvtctlReg = sys_read32(evtctlRegAddr);

	configValue = ((tmpCfgReg << GPIOWFF3_CFG_CFG_S) & GPIOWFF3_CFG_CFG_M) |
		      ((tmpPullctlCfgReg << GPIOWFF3_CFG_PULLCTL_S) & GPIOWFF3_CFG_PULLCTL_M) |
		      ((tmpctlReg << GPIOWFF3_CFG_CTL_S) & GPIOWFF3_CFG_CTL_M) |
		      ((tmpEvtctlReg << GPIOWFF3_CFG_EVTCTL_S) & GPIOWFF3_CFG_EVTCTL_M);

	/* If IMASK bit is set, the interrupt is enabled. */
	if (index >= 32) {
		tmpCfgReg = sys_read32(SOC_AON_BASE + SOC_AON_O_GPIOFNC1S);
		tmpCfgRegBit = 1 << (index - 32);
	} else {
		tmpCfgReg = sys_read32(SOC_AON_BASE + SOC_AON_O_GPIOFNC0S);
		tmpCfgRegBit = 1 << index;
	}

	if ((tmpCfgReg & tmpCfgRegBit) == tmpCfgRegBit) {
		configValue |= GPIO_CFG_INT_ENABLE;
	}

	/* Report current configuration */
	*config = configValue;
}
#endif

static void cc35xx_ll_gpio_set_interrupt_config(uint8_t index, uint32_t config)
{
	uintptr_t key;
	uint32_t evtctlRegAddr;

	/* The EVTCTL contains all the interrupt configuration */
	evtctlRegAddr = IOMUX_EVTCTL_ADDR(index);

	/* Mask away all non-interrupt configuration (all non-EVTCTL configuration) */
	uint32_t maskedConfig = (config & GPIOWFF3_CFG_EVTCTL_M) >> GPIOWFF3_CFG_EVTCTL_S;

	/* Invert the EVTCTL mask, mask out current interrupt config and apply the new one */
	key = irq_lock();
	uint32_t currentRegisterConfig = sys_read32(evtctlRegAddr);

	currentRegisterConfig &= ~(GPIOWFF3_CFG_EVTCTL_M >> GPIOWFF3_CFG_EVTCTL_S);
	sys_write32((currentRegisterConfig | maskedConfig), evtctlRegAddr);
	irq_unlock(key);

	if (config & GPIO_CFG_INT_ENABLE) {
		cc35xx_ll_gpio_enable_int(index);
	} else {
		cc35xx_ll_gpio_disable_int(index);
	}
}

static int cc35xx_gpio_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct cc35xx_gpio_port_config *const port_cfg = dev->config;

	*value = 0;
	for (int i = 0; i < port_cfg->ngpios; i++) {
		*value = *value | (cc35xx_ll_gpio_read(port_cfg->gpio_index_offset + i) << i);
	}

	return 0;
}

static int cc35xx_gpio_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	const struct cc35xx_gpio_port_config *const port_cfg = dev->config;

	for (int i = 0; i < port_cfg->ngpios; i++) {
		if (mask & BIT(i)) {
			cc35xx_ll_gpio_write(port_cfg->gpio_index_offset + i,
					     (BIT(i) & value) >> i);
		}
	}
	return 0;
}

static int cc35xx_gpio_port_set_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct cc35xx_gpio_port_config *const port_cfg = dev->config;

	for (int i = 0; i < port_cfg->ngpios; i++) {
		if (mask & BIT(i)) {
			cc35xx_ll_gpio_write(port_cfg->gpio_index_offset + i, 0x01);
		}
	}
	return 0;
}

static int cc35xx_gpio_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct cc35xx_gpio_port_config *const port_cfg = dev->config;

	for (int i = 0; i < port_cfg->ngpios; i++) {
		if (mask & BIT(i)) {
			cc35xx_ll_gpio_write(port_cfg->gpio_index_offset + i, 0x00);
		}
	}
	return 0;
}

static int cc35xx_gpio_port_toggle_bits(const struct device *dev, gpio_port_pins_t mask)
{
	const struct cc35xx_gpio_port_config *const port_cfg = dev->config;

	for (int i = 0; i < port_cfg->ngpios; i++) {
		if (mask & BIT(i)) {
			cc35xx_ll_gpio_toggle(port_cfg->gpio_index_offset + i);
		}
	}

	return 0;
}

static int cc35xx_gpio_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct cc35xx_gpio_port_config *const port_cfg = dev->config;
	uint32_t config = 0;

	if (pin >= port_cfg->ngpios) {
		return -EINVAL;
	}

	if ((flags & GPIO_DIR_MASK) == GPIO_DIR_MASK) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) && (flags & GPIO_LINE_OPEN_DRAIN)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) && (flags & GPIO_LINE_OPEN_SOURCE)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT) {
		config |= GPIO_CFG_OUT_STD;
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			config |= GPIO_CFG_OUT_HIGH;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			config |= GPIO_CFG_OUT_LOW;
		}
	} else if (flags & GPIO_INPUT) {
		if (flags & GPIO_PULL_DOWN) {
			config |= GPIO_CFG_IN_PD;
		} else if (flags & GPIO_PULL_UP) {
			config |= GPIO_CFG_IN_PU;
		} else {
			config |= GPIO_CFG_IN_NOPULL;
		}
	} else {
		config |= GPIO_CFG_NO_DIR;
	}

	cc35xx_ll_gpio_set_config_and_mux(port_cfg->gpio_index_offset + pin, config, GPIO_MUX_GPIO);

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static inline bool cc35xx_config_is_output(uint32_t config)
{
	if ((config & ~0x1ff) == (GPIO_CFG_OUTPUT)) {
		return true;
	}
	return false;
}

static inline bool cc35xx_config_is_output_high(uint32_t config)
{
	if (!cc35xx_config_is_output(config)) {
		return false;
	}
	if ((config & 0x1ff) == (GPIO_CFG_OUT_HIGH)) {
		return true;
	}
	return false;
}

static inline bool cc35xx_config_is_output_low(uint32_t config)
{
	if (!cc35xx_config_is_output(config)) {
		return false;
	}
	if ((config & 0x1ff) == (GPIO_CFG_OUT_LOW)) {
		return true;
	}
	return false;
}

static inline bool cc35xx_config_is_input(uint32_t config)
{
	if ((config & ~0x1ff) == (GPIO_CFG_INPUT)) {
		return true;
	}
	return false;
}

static inline bool cc35xx_config_is_input_pull_up(uint32_t config)
{
	if (!cc35xx_config_is_input(config)) {
		return false;
	}
	/*
	 * The Pull up / Pull down definitions in HAL
	 * are reversed with regards to HW implementation
	 * so we adjust for that when reading pin config.
	 */
	if ((config & 0xff) == (IOMUX_GPIO2PCTL_CTL_DOWN)) {
		return true;
	}
	return false;
}

static inline bool cc35xx_config_is_input_pull_down(uint32_t config)
{
	if (!cc35xx_config_is_input(config)) {
		return false;
	}
	/*
	 * The Pull up / Pull down definitions in HAL
	 * are reversed with regards to HW implementation
	 * so we adjust for that when reading pin config.
	 */
	if ((config & 0xff) == (IOMUX_GPIO2PCTL_CTL_UP)) {
		return true;
	}
	return false;
}

static int cc35xx_gpio_pin_get_config(const struct device *dev, gpio_pin_t pin,
				      gpio_flags_t *out_flags)
{
	const struct cc35xx_gpio_port_config *const port_cfg = dev->config;
	uint32_t config = 0;

	if (pin >= port_cfg->ngpios) {
		return -EINVAL;
	}

	cc35xx_ll_gpio_get_config(port_cfg->gpio_index_offset + pin, &config);

	*out_flags = 0;

	if (cc35xx_config_is_output_high(config)) {
		*out_flags = GPIO_OUTPUT | GPIO_OUTPUT_HIGH;
	} else if (cc35xx_config_is_output_low(config)) {
		*out_flags = GPIO_OUTPUT | GPIO_OUTPUT_LOW;
	} else if (cc35xx_config_is_input_pull_down(config)) {
		*out_flags = GPIO_INPUT | GPIO_PULL_DOWN;
	} else if (cc35xx_config_is_input_pull_up(config)) {
		*out_flags = GPIO_INPUT | GPIO_PULL_UP;
	} else if (cc35xx_config_is_input(config)) {
		*out_flags = GPIO_INPUT;
	} else {
		return -EINVAL;
	}

	return 0;
}
#endif

#ifdef CONFIG_GPIO_GET_DIRECTION
static int cc35xx_gpio_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct cc35xx_gpio_port_config *const port_cfg = dev->config;
	uint32_t config;

	if (outputs != NULL) {
		*outputs = 0;
	}
	if (inputs != NULL) {
		*inputs = 0;
	}
	for (int i = 0; i < port_cfg->ngpios; i++) {
		if ((map >> i) & 1) {
			cc35xx_ll_gpio_get_config(port_cfg->gpio_index_offset + i, &config);
			if (outputs != NULL) {
				if (cc35xx_config_is_output(config)) {
					*outputs |= (1 << i);
					continue;
				}
			}
			if (inputs != NULL) {
				if (cc35xx_config_is_input(config)) {
					*inputs |= (1 << i);
					continue;
				}
			}
		}
	}
	return 0;
}
#endif

static int cc35xx_gpio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	uint32_t config = 0;
	const struct cc35xx_gpio_port_config *const port_cfg = dev->config;
	uint32_t gpio_index = pin + port_cfg->gpio_index_offset;

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	if (trig == GPIO_INT_TRIG_BOTH) {
		return -ENOTSUP;
	}

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		config |= GPIO_CFG_INT_DISABLE;
		break;
	case GPIO_INT_MODE_EDGE:
		config |= GPIO_CFG_INT_ENABLE;
		config |= (trig == GPIO_INT_TRIG_HIGH) ? GPIO_CFG_IN_INT_RISING
						       : GPIO_CFG_IN_INT_FALLING;
		break;
	default:
		return -ENOTSUP;
	}

	cc35xx_ll_gpio_set_interrupt_config(gpio_index, config);

	return 0;
}

static int cc35xx_gpio_manage_callback(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{
	struct cc35xx_gpio_port_data *port_data = dev->data;

	return gpio_manage_callback(&port_data->callbacks, callback, set);
}

static void cc35xx_gpio_port_isr(const struct device *dev, uint32_t event_mask)
{
	uint32_t flag_index;
	const struct cc35xx_gpio_port_config *port_cfg = dev->config;
	struct cc35xx_gpio_port_data *port_data = dev->data;

	gpio_fire_callbacks(&port_data->callbacks, dev, event_mask);

	while (event_mask) {
		/* MASK_TO_PIN returns the highest set bit */
		flag_index = GPIO_MASK_TO_PIN(event_mask);
		cc35xx_ll_gpio_clear_int(flag_index + port_cfg->gpio_index_offset);
		event_mask &= ~GPIO_PIN_TO_MASK(flag_index);
	}
}

static void cc35xx_gpio_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	uint32_t event_mask;

	/* Lower bank */
	event_mask = sys_read32(SOC_AON_BASE + SOC_AON_O_GPIOMIS0S);
	if (event_mask) {
		const struct device *port = DEVICE_DT_INST_GET(0);

		cc35xx_gpio_port_isr(port, event_mask);
	}

	/* Higher bank */
	event_mask = sys_read32(SOC_AON_BASE + SOC_AON_O_GPIOMIS1S);

	if (event_mask) {
		const struct device *port = DEVICE_DT_INST_GET(1);

		cc35xx_gpio_port_isr(port, event_mask);
	}
}

static int cc35xx_gpio_init(const struct device *dev)
{
	static bool gpio_init_done;
	uint32_t val;

	if (gpio_init_done) {
		return 0;
	}

	/*
	 * Make sure no GPIOs are wakeup sources. If the bit for a given GPIO is set
	 * in the mask, it means the event will not propagate to the wakeup
	 * interrupt. When interrupts are enabled/disabled for individual GPIOs, the
	 * corresponding bit in relevant mask will be cleared/set.
	 */
	sys_write32(0xFFFFFFFF, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUAND));
	sys_write32(0xFFFFFFFF, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUAND1));
	sys_write32(0xFFFFFFFF, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUOR));
	sys_write32(0xFFFFFFFF, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_GPWUOR1));

	/* Disallow all GPIO interrupt sources to pass from RIS to MIS */
	sys_write32(0, (SOC_AON_BASE + SOC_AON_O_GPIOFNC0S));
	sys_write32(0, (SOC_AON_BASE + SOC_AON_O_GPIOFNC1S));

	/* Configure the GPIO OR event as a sleep wakeup source */
	val = sys_read32(HOSTMCU_AON_BASE + HOSTMCU_AON_O_CFGWICSNS);
	val |= HOSTMCU_AON_CFGWICSNS_GPIO_OR_EN;
	sys_write32(val, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_CFGWICSNS));

	/* Disable GPIO AND event as a sleep wakeup source */
	val = sys_read32(HOSTMCU_AON_BASE + HOSTMCU_AON_O_CFGWICSNS);
	val &= ~(HOSTMCU_AON_CFGWICSNS_GPIO_AND_EN);
	sys_write32(val, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_CFGWICSNS));

	/* Configure the GPIO OR event to be a fast wakeup source */
	val = sys_read32(HOSTMCU_AON_BASE + HOSTMCU_AON_O_CFGWUTP);
	val |= HOSTMCU_AON_CFGWUTP_GPIO_OR_FAST;
	sys_write32(val, (HOSTMCU_AON_BASE + HOSTMCU_AON_O_CFGWUTP));

	IRQ_CONNECT(DT_IRQN(DT_DRV_INST(0)), DT_IRQ(DT_DRV_INST(0), priority), cc35xx_gpio_isr,
		    DEVICE_DT_GET(DT_INST(0, DT_DRV_COMPAT)), 0);

	irq_enable(DT_IRQN(DT_DRV_INST(0)));

	gpio_init_done = true;

	return 0;
}

static DEVICE_API(gpio, cc35xx_gpio_drv_api) = {
	.pin_configure = cc35xx_gpio_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = cc35xx_gpio_pin_get_config,
#endif
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = cc35xx_gpio_port_get_direction,
#endif
	.port_get_raw = cc35xx_gpio_port_get_raw,
	.port_set_masked_raw = cc35xx_gpio_port_set_masked_raw,
	.port_set_bits_raw = cc35xx_gpio_port_set_bits_raw,
	.port_clear_bits_raw = cc35xx_gpio_port_clear_bits_raw,
	.port_toggle_bits = cc35xx_gpio_port_toggle_bits,
	.pin_interrupt_configure = cc35xx_gpio_pin_interrupt_configure,
	.manage_callback = cc35xx_gpio_manage_callback,
};

#define CC35XX_GPIO_PORT_DEFINE(inst)                                                              \
	static struct cc35xx_gpio_port_data cc35xx_gpio_port_data_##inst;                          \
	static const struct cc35xx_gpio_port_config cc35xx_gpio_port_config_##inst = {             \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),            \
			},                                                                         \
		.ngpios = DT_PROP(DT_DRV_INST(inst), ngpios),                                      \
		.gpio_index_offset = DT_PROP(DT_DRV_INST(inst), gpio_index_offset),                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, cc35xx_gpio_init, NULL, &cc35xx_gpio_port_data_##inst,         \
			      &cc35xx_gpio_port_config_##inst, PRE_KERNEL_1,                       \
			      CONFIG_GPIO_INIT_PRIORITY, &cc35xx_gpio_drv_api);

DT_INST_FOREACH_STATUS_OKAY(CC35XX_GPIO_PORT_DEFINE)

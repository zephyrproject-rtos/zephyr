#define DT_DRV_COMPAT microchip_dspic_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Offsets from PORTx base */
#define TRIS_OFFSET   ((uintptr_t)0x08u)
#define LAT_OFFSET    ((uintptr_t)0x04u)
#define PORT_OFFSET   ((uintptr_t)0x00u)
#define CNSTAT_OFFSET ((uintptr_t)0x0Cu)

struct gpio_dspic_cfg {
	uintptr_t base;
};

static int dspic_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_dspic_cfg *cfg = dev->config;

	/* TRIS offset */
	volatile uint16_t *tris = (void *)(cfg->base + TRIS_OFFSET);

	/* LAT offset */
	volatile uint16_t *lat = (void *)(cfg->base + LAT_OFFSET);

	/* Set initial level before direction */
	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
		*lat |= BIT(pin);
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
		*lat &= ~BIT(pin);
	} else {

		/* Else statement added to clear MISRA warning */
	}

	/* Configure direction: TRIS bit = 0 for output, 1 for input */
	if ((flags & GPIO_OUTPUT) != 0) {
		*tris &= ~BIT(pin);
	} else {
		*tris |= BIT(pin);
	}

	return 0;
}

static int dspic_port_toggle_bits(const struct device *dev, gpio_port_pins_t pin)
{
	const struct gpio_dspic_cfg *cfg = dev->config;
	volatile uint16_t *lat = (void *)(cfg->base + LAT_OFFSET);

	/* Toggling the GPIO pin */
	*lat ^= pin;

	return 0;
}

static int dspic_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_dspic_cfg *cfg = dev->config;
	volatile const uint16_t *port = (void *)(cfg->base + PORT_OFFSET);

	/* Fetch all values in port */
	*value = *port;
	return 0;
}

static int dspic_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_dspic_cfg *cfg = dev->config;
	volatile uint16_t *lat = (void *)(cfg->base + LAT_OFFSET);

	/* Set bits in LAT register */
	*lat |= pins;
	return 0;
}

static int dspic_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_dspic_cfg *cfg = dev->config;
	volatile uint16_t *lat = (void *)(cfg->base + LAT_OFFSET);

	/* Clear bits in LAT register */
	*lat &= ~pins;
	return 0;
}

static uint32_t dspic_get_pending_int(const struct device *dev)
{
	const struct gpio_dspic_cfg *cfg = dev->config;
	volatile const uint16_t *cnstat = (void *)(cfg->base + CNSTAT_OFFSET);

	/* CNSTATx has the latched change status for each pin */
	return *cnstat;
}

static int dspic_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				gpio_port_value_t value)
{
	const struct gpio_dspic_cfg *cfg = dev->config;
	volatile uint16_t *lat = (void *)(cfg->base + LAT_OFFSET);
	uint16_t tmp = *lat;

	tmp = (tmp & ~mask) | (value & mask);
	*lat = tmp;

	return 0;
}

static const struct gpio_driver_api gpio_dspic_api = {
	.pin_configure = dspic_pin_configure,
	.port_toggle_bits = dspic_port_toggle_bits,
	.port_get_raw = dspic_port_get_raw,
	.port_set_bits_raw = dspic_port_set_bits_raw,
	.port_clear_bits_raw = dspic_port_clear_bits_raw,
	.get_pending_int = dspic_get_pending_int,
	.port_set_masked_raw = dspic_set_masked_raw,
};

/* Create instances from DT */
#define GPIO_DSPIC_INIT(inst)                                                                      \
	static const struct gpio_dspic_cfg gpio_dspic_cfg_##inst = {                               \
		.base = DT_INST_REG_ADDR(inst),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &gpio_dspic_cfg_##inst, POST_KERNEL,         \
			      CONFIG_GPIO_INIT_PRIORITY, &gpio_dspic_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_DSPIC_INIT)

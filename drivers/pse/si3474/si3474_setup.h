#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include "zephyr/drivers/pse.h"

void si3474_gpio_callback(void);
int si3474_init_interrupt(const struct device *dev);
int si3474_event_trigger_set(const struct device *dev, pse_event_trigger_handler_t handler);
int si3474_init_ports(const struct device *dev);

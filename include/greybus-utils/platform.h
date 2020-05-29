#ifndef ZEPHYR_INCLUDE_GREYBUS_UTILS_PLATFORM_H_
#define ZEPHYR_INCLUDE_GREYBUS_UTILS_PLATFORM_H_

#include <stdint.h>
#include <stddef.h>

struct gb_gpio_platform_driver {
	uint8_t (*line_count)(void);
	int (*activate)(uint8_t which);
	int (*deactivate)(uint8_t which);
	int (*get_direction)(uint8_t which);
	int (*direction_in)(uint8_t which);
	int (*direction_out)(uint8_t which, uint8_t val);
	int (*get_value)(uint8_t which);
	int (*set_value)(uint8_t which, uint8_t val);
	int (*set_debounce)(uint8_t which, uint16_t usec);
	int (*irq_mask)(uint8_t which);
	int (*irq_unmask)(uint8_t which);
	int (*irq_type)(uint8_t which, uint8_t type);
};
void gb_gpio_register_platform_driver(struct gb_gpio_platform_driver *drv);

#endif /* ZEPHYR_INCLUDE_GREYBUS_UTILS_PLATFORM_H_ */

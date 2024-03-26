#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include "zephyr/drivers/pse.h"

enum pse_event {
	POWER_EVENT = 0,
	CLASS_DETECT_EVENT,
	DISCONNECT_PCUT_FAULT,
	ILIM_START_FAULT,
	SUPPLY_EVENT,
	POWER_ON_FAULT,
};

enum pse_channel_state {
	SWITCH_ON,
	SWITCH_OFF,
};

struct si3474_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec gpio_int;
	struct gpio_dt_spec gpio_rst;
	struct gpio_dt_spec gpio_oss;
	uint32_t current_limit;
};

struct si3474_data {
	uint32_t current;
	bool overcurrent;

	const struct device *dev;
	struct gpio_callback gpio_cb;

	pse_event_trigger_handler_t event_ready_handler;
#ifdef CONFIG_SI3474_TRIGGER
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_SI3474_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
};

int si3474_init_ports(const struct device *dev);
int si3474_i2c_write_reg(const struct device *i2c, uint16_t dev_addr, uint8_t reg_ddr, uint8_t val);
int si3474_i2c_read_reg(const struct device *i2c, uint16_t dev_addr, uint8_t reg_ddr,
			uint8_t *buff);

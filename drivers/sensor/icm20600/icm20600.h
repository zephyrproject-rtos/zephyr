#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20600_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20600_H_

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <zephyr/types.h>

#define ICM20600_REG_CHIP_ID		0x75
#define ICM20600_CHIP_ID			0x11

#define ICM20600_REG_GYRO_CFG		0x1B
#define ICM20600_GYRO_FS_SHIFT		3

#define ICM20600_REG_ACCEL_CFG		0x1C
#define ICM20600_ACCEL_FS_SHIFT		3

#define ICM20600_REG_INT_EN		0x38
#define ICM20600_DRDY_EN			BIT(0)

#define ICM20600_REG_DATA_START		0x3B

#define ICM20600_REG_PWR_MGMT1		0x6B
#define ICM20600_SLEEP_EN		BIT(6)

/* measured in degrees/sec x10 to avoid floating point */
static const u16_t icm20600_gyro_sensitivity_x10[] = {
	1310, 655, 328, 164
};

struct icm20600_data {
	struct device *i2c;

	s16_t accel_x;
	s16_t accel_y;
	s16_t accel_z;
	u16_t accel_sensitivity_shift;

	s16_t temp;

	s16_t gyro_x;
	s16_t gyro_y;
	s16_t gyro_z;
	u16_t gyro_sensitivity_x10;

#ifdef CONFIG_ICM20600_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_ICM20600_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_ICM20600_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ICM20600_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_ICM20600_TRIGGER */
};

#ifdef CONFIG_ICM20600_TRIGGER
int icm20600_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int icm20600_init_interrupt(struct device *dev);
#endif

#endif /* __SENSOR_ICM20600__ */

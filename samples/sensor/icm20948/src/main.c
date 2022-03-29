#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <sys/util.h>

void main(void)
{

	const char *const label = DT_LABEL(DT_INST(0, invensense_icm20948));
	const struct device *icm20948 = device_get_binding(label);

	if (!icm20948) {
		printf("Failed to find sensor %s\n", label);
		return;
	}

	while (1) {
		struct sensor_value acc[3];

		sensor_sample_fetch(icm20948);
		sensor_channel_get(icm20948, SENSOR_CHAN_ACCEL_XYZ, &acc);

		printk("x: %d.%06d; y: %d.%06d; z: %d.%06d\n", acc[0].val1, acc[0].val2,
		       acc[1].val1, acc[1].val2, acc[2].val1, acc[2].val2);

		k_sleep(K_MSEC(500));
	}
}

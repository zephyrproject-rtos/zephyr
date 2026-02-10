#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(analog_ad74416h);

	if (dev == NULL) {
		printk("Error: No AD74416H device found.\n");
		return -ENODEV;
	}

	if (!device_is_ready(dev)) {
		printk("Error: Device %s is not ready.\n", dev->name);
		return -ENODEV;
	}

	printk("AD74416H Multi-Function Sample Started\n");

	/* 1. Configure GPIO (Channel 3 as Digital Input) */
	/* Note: The driver uses the same device pointer for all APIs */
	int ret = gpio_pin_configure(dev, 3, GPIO_INPUT);
	if (ret < 0) {
		printk("GPIO Config failed: %d\n", ret);
	}

	/* 2. Configure DAC (Channel 0 as Voltage Output) */
	struct dac_channel_cfg dac_cfg = {
		.channel_id = 0,
		.resolution = 16
	};
	dac_channel_setup(dev, &dac_cfg);

	/* Set CH0 to ~5.0V (0x6AAA in 0-12V range) */
	dac_write_value(dev, 0, 0x6AAA);

	while (1) {
		struct sensor_value voltage_in;
		struct sensor_value temp_in;
		int digital_in;

		/* 3. Fetch all Analog Samples (Trigger ADC Sequencer) */
		ret = sensor_sample_fetch(dev);
		if (ret < 0) {
			printk("Sensor fetch failed: %d\n", ret);
			continue;
		}

		/* 4. Read Voltage Input (Channel 1) */
		sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &voltage_in);

		/* 5. Read RTD Temperature (Channel 2) */
		/* The driver performs Callendar-Van Dusen math internally */
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_in);

		/* 6. Read Digital Input (Channel 3) */
		digital_in = gpio_pin_get(dev, 3);

		/* Output Results */
		printk("--- AD74416H Status ---\n");
		printk("CH0 (Out): 5.00 V\n");
		printk("CH1 (In):  %d.%06d V\n", voltage_in.val1, voltage_in.val2);
		printk("CH2 (RTD): %d.%02d C\n", temp_in.val1, temp_in.val2);
		printk("CH3 (DIN): %s\n", digital_in ? "HIGH" : "LOW");

		k_sleep(K_MSEC(2000));
	}
	return 0;
}

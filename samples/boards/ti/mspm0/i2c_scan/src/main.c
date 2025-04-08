#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>
#include <zephyr/drivers/i2c.h>

int main(void)
{
	uint8_t cnt = 0, first = 0x04, last = 0x77, attempt = 0;
	char *name = "i2c@400f0000";
	const struct device *dev = device_get_binding(name);
	if (!dev) {
		printk("I2C: Device driver %s not found.", name);
		return -ENODEV;
	}

	while (1) {
		printk("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
		for (uint8_t i = 0; i <= last; i += 16) {
			printk("%02x: ", i);
			for (uint8_t j = 0; j < 16; j++) {
				if (i + j < first || i + j > last) {
					printk("   ");
					continue;
				}

				struct i2c_msg msgs[1];
				uint8_t dst;

				/* Send the address to read from */
				msgs[0].buf = &dst;
				msgs[0].len = 0U;
				msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
				if (i2c_transfer(dev, &msgs[0], 1, i + j) == 0) {
					printk("%02x ", i + j);
					++cnt;
				} else {
					printk("-- ");
				}
			}
			printk("\n");
		}

		attempt++;
		printk("%u devices found on %s during attempt %u\n\n\n", cnt, name, attempt);
		cnt = 0;

		k_msleep(1000);
	}

	return 0;
}



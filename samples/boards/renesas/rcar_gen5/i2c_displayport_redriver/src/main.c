#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>

#define I2C_REG		0xf0
#define I2C_TX_BUF_SIZE	1
#define I2C_RX_BUF_SIZE	2

static const struct i2c_dt_spec target_dev =
	I2C_DT_SPEC_GET(DT_NODELABEL(dp_redriver));

int main(void)
{
	uint8_t buf_tx[I2C_TX_BUF_SIZE] = { I2C_REG };
	uint8_t buf_rx[I2C_RX_BUF_SIZE] = { 0x00 };
	int i;

	if (!device_is_ready(target_dev.bus)) {
		printk("I2C5 bus not ready\n");
		return 0;
	}

	i2c_write_read_dt(&target_dev, buf_tx, sizeof(buf_tx), buf_rx, sizeof(buf_rx));

	printk("data = ");
	for (i = 0 ; i < I2C_RX_BUF_SIZE ; i++)
		printk("0x%02x ", buf_rx[i]);
	printk("\n");

	return 0;
}

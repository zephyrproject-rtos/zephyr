#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/spi.h>


int main()
{
    struct spi_config config;
    printf("basic_write_9bit_words; ret: \n");

    const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
    uint8_t buff[3] = {0x34, 0x56, 0x78};
    // uint8_t buff[3];
    // buff[0] = 0xAB;
    // buff[1] = 0xCD;
    // buff[2] = 0xEF;
	int len = 2 * sizeof(buff[0]);

    struct spi_buf tx_buf = { .buf = buff, .len = len };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

    printf("basic_write_9bit_words; ret: \n");
    
    int ret = spi_write(dev, &config, &tx_bufs);

    printf("basic_write_9bit_words; ret: %d\n", ret);
	printf(" wrote %04x %04x %04x %04x %04x\n",
		buff[0], buff[1], buff[2], buff[3], buff[4]);

}

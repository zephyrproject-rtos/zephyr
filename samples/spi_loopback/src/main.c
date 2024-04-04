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
    const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(spi0));

    if(!device_is_ready(dev)){
        printk("device not ready.\n");
		return 0;
    }

    struct spi_config config;

    uint16_t tx_bufs[5] = { 0x0101, 0x00ff, 0x00a5, 0x0000, 0x0102};

	int ret = spi_write(dev, &config, &tx_bufs);
}
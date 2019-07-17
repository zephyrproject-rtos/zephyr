/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/spi.h>

#define GPIO_DEV "GPIO_0"

#if (CONFIG_SPI_0 - 0) && defined(DT_SPI_0_NAME)
#define SPI_DEV  DT_SPI_0_NAME
#elif (CONFIG_SPI_1 - 0) && defined(DT_SPI_1_NAME)
#define SPI_DEV  DT_SPI_1_NAME
#elif (CONFIG_SPI_2 - 0) && defined(DT_SPI_2_NAME)
#define SPI_DEV  DT_SPI_2_NAME
#elif (CONFIG_SPI_3 - 0) && defined(DT_SPI_3_NAME)
#define SPI_DEV  DT_SPI_3_NAME
#else
#error Unsupported SPI driver
#endif

/* Any GPIO pin that suits your design */
#define PIN_CS   18
#define SPI_WORDS 2

static struct device *spi_dev;
static struct spi_cs_control cs = {
	.gpio_pin = PIN_CS,
	.delay = 0,
};

static int device_init(void)
{
	cs.gpio_dev = device_get_binding(GPIO_DEV);
	if (!cs.gpio_dev) {
		printk("GPIO: Device driver not found.\n");
		return 0;
	}
	printk("GPIO device found\n");
	k_sleep(100);

	spi_dev = device_get_binding(SPI_DEV);
	if (!spi_dev) {
		printk("SPI: Device driver not found.\n");
		return 0;
	}
	printk("SPI device found\n");

	return 1;
}

static void max_send_command(u16_t cmd)
{
	u8_t ret, buffer_tx[SPI_WORDS];
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = SPI_WORDS,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	const struct spi_config spi_cfg = {
		/* 1 MHz, but MAX7219 supports up to 10 MHz */
		.frequency = 1000000U,
		.operation =
			SPI_TRANSFER_MSB | \
			SPI_WORD_SET(8) | \
			SPI_CS_ACTIVE_HIGH,
		.slave = 0,
		.cs = &cs,
	};

	buffer_tx[0] = (cmd >> 8);
	buffer_tx[1] = cmd & 0xff;
	ret = spi_write(spi_dev, &spi_cfg, &tx);
	if (ret != 0) {
		printk("SPI write failed %d\n", ret);
	}
}

static void send_patterns(int start)
{
	u8_t val = 0x00;
	int lineNum;

	for (lineNum = 1; lineNum <= 8; ++lineNum) {
		if (lineNum == start) {
			switch (start) {
			case 1:
			case 8:
				val = 0x81;
				break;
			case 2:
			case 7:
				val = 0x42;
				break;
			case 3:
			case 6:
				val = 0x24;
				break;
			case 4:
			case 5:
				val = 0x18;
				break;
			}
		} else {
			val = 0x00;
		}
		max_send_command((lineNum << 8) | val);
	}
	/*
	 * Send one No-Op command in order to provide the device with some
	 * more clock signals which are required to properly refresh
	 * the last display segment.
	 */
	max_send_command(0x00FF);
}

void main(void)
{
	int start = 1, cmdi, cycles = 5;
	bool dir = true;

	if (!device_init()) {
		/*
		 * Stop the app if we didn't manage
		 * to setup the devices correctly.
		 */
		return;
	}

	const u16_t cmds[] = {
		0x0F00, /* disable test display mode */
		0x0900, /* disable decode */
		0x0A01, /* reduce brightness */
		0x0B07, /* set scan limit */
		0x0C01, /* leave shutdown mode */

		/* Blank initial screen state */
		0x0100,
		0x0200,
		0x0300,
		0x0400,
		0x0500,
		0x0600,
		0x0700,
		0x0800,
	};

	for (cmdi = 0; cmdi < ARRAY_SIZE(cmds); ++cmdi) {
		max_send_command(cmds[cmdi]);
	}

	printk("Animating...\n");

	while (1) {
		send_patterns(start);
		if ((start == 1 && !dir) || (start == 8 && dir)) {
			dir = !dir;
			if (cycles > 0) {
				if (dir) {
					cycles--;
				}
			} else if (cycles == 0) {
				printk("Animation test complete.\n");
				cycles = -1;
			}
		}
		if (dir) {
			start++;
		} else {
			start--;
		}
		k_sleep(50);
	}
};

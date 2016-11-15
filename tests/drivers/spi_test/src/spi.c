/* spi.c - SPI test source file */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <string.h>
#include <spi.h>
#include <misc/printk.h>



#define SPI_DRV_NAME "SPI_0"

#ifdef CONFIG_SPI_INTEL
#include <spi/spi_intel.h>
#if defined(CONFIG_SPI_1)
#define SPI_DRV_NAME "SPI_1"
#endif
#define SPI_SLAVE 0
#elif defined(CONFIG_SPI_DW)
#define SPI_MAX_CLK_FREQ_250KHZ 128
#define SPI_SLAVE 2
#elif defined(CONFIG_SPI_QMSI)
#define SPI_MAX_CLK_FREQ_250KHZ 128
#define SPI_SLAVE 1
#endif

unsigned char wbuf[16] = "Hello";
unsigned char rbuf[16] = {};

static void print_buf_hex(unsigned char *b, uint32_t len)
{
	for (; len > 0; len--) {
		printk("0x%x ", *(b++));
	}

	printk("\n");
}

struct spi_config spi_conf = {
	.config = SPI_MODE_CPOL | SPI_MODE_CPHA | (8 << 4),
	.max_sys_freq = SPI_MAX_CLK_FREQ_250KHZ,
};

static void _spi_show(struct spi_config *spi_conf)
{
	printk("SPI Configuration:\n");
	printk("\tbits per word: %u\n", SPI_WORD_SIZE_GET(spi_conf->config));
	printk("\tMode: %u\n", SPI_MODE(spi_conf->config));
	printk("\tMax speed Hz: 0x%X\n", spi_conf->max_sys_freq);
}

void main(void)
{
	struct device *spi;

	printk("==== SPI Test Application ====\n");

	spi = device_get_binding(SPI_DRV_NAME);

	printk("Running...\n");

	spi_configure(spi, &spi_conf);
	spi_slave_select(spi, SPI_SLAVE);

	_spi_show(&spi_conf);

	printk("Writing...\n");
	spi_write(spi, (uint8_t *) wbuf, 6);

	printk("SPI sent: %s\n", wbuf);
	print_buf_hex(rbuf, 6);

	strcpy((char *)wbuf, "So what then?");
	spi_transceive(spi, wbuf, 14, rbuf, 16);

	printk("SPI transceived: %s\n", rbuf);
	print_buf_hex(rbuf, 6);
}

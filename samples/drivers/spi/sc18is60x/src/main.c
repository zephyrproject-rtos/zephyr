/*
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Exercise the SC18IS60x I2C-to-SPI bridge with real SPI hardware.
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#if !DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(spi_click))
#error "This sample requires the mikroe_i2c_to_spi_click shield (spi_click)"
#endif

#define SPI_CLICK_NODE DT_NODELABEL(spi_click)
#define MFD_NODE       DT_PARENT(SPI_CLICK_NODE)

/* Pattern the host sends through the bridge master. */
static const uint8_t host_tx[] = {0xA5, 0x5A, 0x00, 0xFF, 0x10, 0x20, 0x30, 0x40};
static uint8_t host_rx[sizeof(host_tx)];

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sc18is60x_target_spi))
#if DT_PROP(DT_NODELABEL(sc18is60x_target_spi), serial_target)
#define SC18IS60X_TARGET_SPI 1
#endif
#endif

#ifndef SC18IS60X_TARGET_SPI
#define SC18IS60X_TARGET_SPI 0
#endif

static void dump_hex(const char *label, const uint8_t *buf, size_t len)
{
	if (label[0] != '\0') {
		printk("%s: ", label);
	}
	for (size_t i = 0; i < len; i++) {
		printk("%02x ", buf[i]);
	}
	printk("\n");
}

#if SC18IS60X_TARGET_SPI
/* Pattern the MCU serial target sends back to the bridge master. */
static const uint8_t target_tx[] = {0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8};
static uint8_t target_rx[sizeof(host_tx)];
static int target_ret;
static K_SEM_DEFINE(target_done, 0, 1);

static void target_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *target_spi = DEVICE_DT_GET(DT_NODELABEL(sc18is60x_target_spi));
	const struct spi_config cfg = {
		.frequency = 1875000,
		.operation = SPI_OP_MODE_SLAVE | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
		.slave = 0,
	};
	struct spi_buf tx_buf = {
		.buf = (uint8_t *)target_tx,
		.len = sizeof(target_tx),
	};
	struct spi_buf rx_buf = {
		.buf = target_rx,
		.len = sizeof(target_rx),
	};
	const struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf_set rx_set = {
		.buffers = &rx_buf,
		.count = 1,
	};

	target_ret = spi_transceive(target_spi, &cfg, &tx_set, &rx_set);
	k_sem_give(&target_done);
}

K_THREAD_STACK_DEFINE(target_stack, 2048);
static struct k_thread target_tid;
#endif

int main(void)
{
	const struct device *mfd = DEVICE_DT_GET(MFD_NODE);
	const struct device *spi_click = DEVICE_DT_GET(SPI_CLICK_NODE);
	const struct spi_config mcfg = {
		.frequency = 1875000,
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
		.slave = 0,
	};
	struct spi_buf tx_buf = {
		.buf = (uint8_t *)host_tx,
		.len = sizeof(host_tx),
	};
	struct spi_buf rx_buf = {
		.buf = host_rx,
		.len = sizeof(host_rx),
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct spi_buf_set rx_set = {
		.buffers = &rx_buf,
		.count = 1,
	};
	int ret;

	if (!device_is_ready(mfd)) {
		printk("ERR: MFD %s not ready\n", mfd->name);
		return 0;
	}

	if (!device_is_ready(spi_click)) {
		printk("ERR: SPI %s not ready\n", spi_click->name);
		return 0;
	}

	printk("\n--- SC18IS60x sample ---\n");
	printk("mfd: %s\n", mfd->name);
	printk("spi: %s\n", spi_click->name);

	/* Click nRESET is released by the MFD driver; wait for POR. */
	k_msleep(10);

#if SC18IS60X_TARGET_SPI
	const struct device *target_spi = DEVICE_DT_GET(DT_NODELABEL(sc18is60x_target_spi));

	if (!device_is_ready(target_spi)) {
		printk("ERR: target SPI %s not ready\n", target_spi->name);
		return 0;
	}

	printk("target spi: %s\n", target_spi->name);
	printk("\n[full-duplex: bridge master <-> MCU serial target]\n");

	/* Arm the serial target first so it is ready when the bridge clocks it. */
	k_thread_create(&target_tid, target_stack, K_THREAD_STACK_SIZEOF(target_stack),
			target_thread, NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	k_msleep(2);

	ret = spi_transceive(spi_click, &mcfg, &tx_set, &rx_set);
	if (ret != 0) {
		printk("ERR: bridge transceive failed (%d)\n", ret);
		return 0;
	}

	ret = k_sem_take(&target_done, K_SECONDS(2));
	if (ret != 0) {
		printk("FAIL: target timeout (%d)\n", ret);
	} else if (target_ret < 0) {
		printk("FAIL: target transceive error %d\n", target_ret);
	} else {
		printk("Host TX:    ");
		dump_hex("", host_tx, sizeof(host_tx));
		printk("Target RX:  ");
		dump_hex("", target_rx, sizeof(target_rx));
		printk("Target TX:  ");
		dump_hex("", target_tx, sizeof(target_tx));
		printk("Host RX:    ");
		dump_hex("", host_rx, sizeof(host_rx));

		if (target_ret != (int)sizeof(host_tx) ||
			memcmp(target_rx, host_tx, sizeof(host_tx)) != 0 ||
			memcmp(host_rx, target_tx, sizeof(target_tx)) != 0) {
			printk("FAIL: full-duplex data mismatch (frames=%d)\n", target_ret);
		} else {
			printk("PASS: full-duplex data matched both directions\n");
		}
	}
#else
	printk("\n[bridge master -> extra header SPI slave]\n");

	ret = spi_transceive(spi_click, &mcfg, &tx_set, &rx_set);
	if (ret != 0) {
		printk("ERR: SPI transceive failed (%d)\n", ret);
		return 0;
	}

	printk("Host TX: ");
	dump_hex("", host_tx, sizeof(host_tx));
	printk("Host RX: ");
	dump_hex("", host_rx, sizeof(host_rx));
	if (memcmp(host_rx, host_tx, sizeof(host_tx)) == 0) {
		printk("PASS: RX matches TX (slave echoes or MOSI-MISO jumpered)\n");
	} else {
		printk("INFO: RX differs from TX (slave responded with its own data or MISO idle)\n");
	}
#endif

	printk("\n--- done ---\n");

	/* Keep output visible before any shell prompt takes over. */
	k_msleep(100);

	return 0;
}

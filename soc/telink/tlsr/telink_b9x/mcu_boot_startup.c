/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <bootutil/bootutil_log.h>
#include <zephyr/sys/crc.h>
BOOT_LOG_MODULE_REGISTER(telink_b9x_mcuboot);

static bool telink_b9x_mcu_boot_startup(void);
static __maybe_unused void printk_buf(const char *comment, void *buf, size_t buf_len);

void __wrap_main(void)
{
	if (telink_b9x_mcu_boot_startup()) {
		extern void __real_main(void);

		__real_main();
	}
}

/* Vendor specific code during MCUBoot startup */
static bool telink_b9x_mcu_boot_startup(void)
{
	bool result = true; /* run MCUBoot main */

	BOOT_LOG_INF("telink B9x MCUBoot on early boot");
#if CONFIG_SOC_RISCV_TELINK_B92
	bool show_chip_id = false;

	/* Check if Console UART RX line is at low level - shorted to ground */
	const struct device *const uart_con = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (device_is_ready(uart_con)) {
		/**********************************************************************
		 * Usually function
		 * uart_err_check(uart_con)
		 * should be called to detect low level at RX line - break condition.
		 * But on B92 platform this condition is not detected. Instead of it:
		 * - low level at RX line is treated as start bit
		 * - all data bits are received as zeros
		 * - parity bit (if exists) and stop bits are ignored
		 * - received zero byte becomes into UART FIFO and no future reception
		 * So lets check RX line low level by this way...
		 **********************************************************************/
		uint8_t ch;

		if (!uart_poll_in(uart_con, &ch)) {
			if (!ch) {
				if (uart_poll_in(uart_con, &ch) == -1) {
					show_chip_id = true;
				}
			}
		}
	} else {
		BOOT_LOG_ERR("uart console not ready");
	}

	if (show_chip_id) {
		extern unsigned char efuse_get_chip_id(unsigned char *chip_id_buff);
		uint8_t chip_id[18];

		if (efuse_get_chip_id(chip_id)) {
			uint16_t chip_id_crc = crc16_itu_t(0, chip_id, 16);

			chip_id[16] = chip_id_crc & 0x00ff;
			chip_id[17] = chip_id_crc >> 8;
			printk_buf("chip id", chip_id, sizeof(chip_id));
		} else {
			BOOT_LOG_ERR("chip id read error");
		}
	}
#endif /* CONFIG_SOC_RISCV_TELINK_B92 */
	return result;
}

static __maybe_unused void printk_buf(const char *comment, void *buf, size_t buf_len)
{
	printk("%s[%u]:", comment, buf_len);
	for (size_t i = 0; i < buf_len; ++i) {
		printk(" %02x", ((uint8_t *)buf)[i]);
	}
	printk("\n");
}

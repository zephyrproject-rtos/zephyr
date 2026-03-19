/*
 * Copyright (c) 2026 Vincent Jardin <vjardin@free.fr>, Free Mobile
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/retention/bootmode.h>

/*
 * Reboot into BOOTSEL via the Zephyr retention boot mode API.
 *
 * Sets BOOT_MODE_TYPE_BOOTLOADER in retained RAM, then cold-reboots.
 * On next boot, the SoC hook in rom_bootloader.c (PRE_KERNEL_2)
 * detects the flag and calls reset_usb_boot() to enter BOOTSEL.
 *
 * Requires: -S rp2-boot-mode-retention snippet (enables
 * CONFIG_RETENTION_BOOT_MODE and reserves 4 bytes at SRAM start).
 */
static void reboot_to_bootsel(void)
{
	bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
	sys_reboot(SYS_REBOOT_COLD);
}

static int cmd_reboot_bootloader(const struct shell *sh, size_t argc,
				 char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Rebooting into BOOTSEL...");
	k_sleep(K_MSEC(100));
	reboot_to_bootsel();

	return 0; /* unreachable */
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_reboot,
	SHELL_CMD(bootloader, NULL,
		  SHELL_HELP("Reboot into BOOTSEL mode (RP2040/RP2350).", NULL),
		  cmd_reboot_bootloader),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(reboot, &sub_reboot, "Reboot commands", NULL);

/*
 * Monitor baud rate changes on the CDC ACM UART.
 * When the host sets 1200 baud (the "magic baud rate"),
 * reboot into BOOTSEL mode. This allows triggering BOOTSEL
 * from the host via a USB SET_LINE_CODING control transfer
 * without opening a serial console (e.g. via bootsel.py).
 */
#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
static void bootsel_monitor(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	uint32_t baud = 0;

	while (1) {
		k_sleep(K_MSEC(100));
		if (uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE,
				       &baud) == 0) {
			if (baud == 1200) {
				k_sleep(K_MSEC(50));
				reboot_to_bootsel();
			}
		}
	}
}

K_THREAD_DEFINE(bootsel_tid, 512, bootsel_monitor, NULL, NULL, NULL, 7, 0, 0);
#endif

int main(void)
{
#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	const struct device *dev;
	uint32_t dtr = 0;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (!device_is_ready(dev)) {
		return 0;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
#endif
	return 0;
}

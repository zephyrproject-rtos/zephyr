/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * The BlueNRG-LP, BlueNRG-LPS UART bootloader protocol (AN5471)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ble_fw_upgrade, LOG_LEVEL_DBG);

#define BLE_FLASH_START_ADDRESS 0x10040000

/* Defined in ${ZEPHYR_HAL_ST_MODULE_DIR}/ble_firmware/ble_fw.c */
extern const uint8_t ble_fw_img[];
extern const int ble_fw_img_len;

const struct gpio_dt_spec green_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
const struct gpio_dt_spec red_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
const struct gpio_dt_spec blue_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);

typedef enum {
	BLE_FW_UPG_START = 0x0,
	BLE_FW_UPG_OK = 0x1,
	BLE_FW_UPG_ERR = 0x2,
} ble_fw_upg_status;

static void led_pattern_out(ble_fw_upg_status status)
{
	int i;

	gpio_pin_set_dt(&blue_gpio, 0);
	gpio_pin_set_dt(&green_gpio, 0);
	gpio_pin_set_dt(&red_gpio, 0);

	switch (status) {
	case BLE_FW_UPG_START:
		for (i = 0; i < 7; i++) {
			gpio_pin_toggle(blue_gpio.port, blue_gpio.pin);
			k_sleep(K_MSEC(200));
		}
		break;

	case BLE_FW_UPG_OK:
		for (i = 0; i < 7; i++) {
			gpio_pin_toggle(green_gpio.port, green_gpio.pin);
			k_sleep(K_MSEC(200));
		}
		break;

	case BLE_FW_UPG_ERR:
		for (i = 0; i < 7; i++) {
			gpio_pin_toggle(red_gpio.port, red_gpio.pin);
			k_sleep(K_MSEC(200));
		}
		break;
	}
}

static int led_init(void)
{
	/* led 0 */
	if (!gpio_is_ready_dt(&green_gpio)) {
		LOG_ERR("%s: device not ready.\n", green_gpio.port->name);
		return -1;
	}
	gpio_pin_configure_dt(&green_gpio, GPIO_OUTPUT_INACTIVE);

	/* led 1 */
	if (!gpio_is_ready_dt(&red_gpio)) {
		LOG_ERR("%s: device not ready.\n", red_gpio.port->name);
		return -1;
	}
	gpio_pin_configure_dt(&red_gpio, GPIO_OUTPUT_INACTIVE);

	/* led 3 */
	if (!gpio_is_ready_dt(&blue_gpio)) {
		LOG_ERR("%s: device not ready.\n", blue_gpio.port->name);
		return -1;
	}
	gpio_pin_configure_dt(&blue_gpio, GPIO_OUTPUT_INACTIVE);

	return 0;
}

#define BLE_UART_BOOTLOADER_ACK  0x79
#define BLE_UART_BOOTLOADER_NACK 0x1F

#define BLE_UART_BOOTLOADER_AUTO_BAUDRATE 0x7F
#define BLE_BL_CMD_GET_LIST               0x00
#define BLE_BL_CMD_GET_VERSION            0x01
#define BLE_BL_CMD_WRITE_MEMORY           0x31
#define BLE_BL_CMD_ERASE                  0x43

static uint8_t checksum(uint8_t *buffer, uint16_t len)
{
	uint8_t checksum = 0;
	uint16_t i;

	if (len > 1) {
		for (i = 0; i < len; i++) {
			checksum = checksum ^ buffer[i];
		}
	} else {
		checksum = ~buffer[0];
	}

	return checksum;
}

static int ble_uart_check_ack(const struct device *bl_uart)
{
	int ret;
	uint8_t ack;

	while ((ret = uart_poll_in(bl_uart, &ack)) == -1) {
	}

	if (ret != 0 || ack != BLE_UART_BOOTLOADER_ACK) {
		LOG_ERR("ble_uart_send_data: NAK (%02x)\n", ack);
		return -1;
	}

	return 0;
}

static int ble_uart_send_data(const struct device *bl_uart, uint8_t *data, uint16_t len)
{
	uint16_t i;

	for (i = 0; i < len; i++) {
		uart_poll_out(bl_uart, data[i]);
	}

	uart_poll_out(bl_uart, checksum(data, len));

	return ble_uart_check_ack(bl_uart);
}

static int ble_uart_send_cmd(const struct device *bl_uart, uint8_t cmd)
{
	return ble_uart_send_data(bl_uart, &cmd, 1);
}

static int ble_uart_bootloader_activate(const struct device *bl_uart)
{
	const struct gpio_dt_spec bt_rst = GPIO_DT_SPEC_GET(DT_NODELABEL(hci_spi), reset_gpios);
	const struct gpio_dt_spec bt_boot = GPIO_DT_SPEC_GET(DT_NODELABEL(hci_spi), irq_gpios);

	/* Configure RST pin and hold BLE in Reset */
	gpio_pin_configure_dt(&bt_rst, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&bt_boot, GPIO_OUTPUT_ACTIVE);

	k_sleep(K_MSEC(1));

	/* Take BLE out of reset */
	gpio_pin_set_dt(&bt_rst, 0);
	k_sleep(K_MSEC(1));

	/* Send bootloader activate command */
	uart_poll_out(bl_uart, BLE_UART_BOOTLOADER_AUTO_BAUDRATE);

	return ble_uart_check_ack(bl_uart);
}

static int ble_bl_cmd_get_version(const struct device *bl_uart, uint8_t *fw_ver)
{
	uint8_t version = 0, opt[2];

	if (ble_uart_send_cmd(bl_uart, BLE_BL_CMD_GET_VERSION) < 0) {
		return -1;
	}

	while (uart_poll_in(bl_uart, &version) == -1) {
	}

	while (uart_poll_in(bl_uart, &opt[0]) == -1) {
	}

	while (uart_poll_in(bl_uart, &opt[1]) == -1) {
	}

	*fw_ver = version;

	return ble_uart_check_ack(bl_uart);
}

static int ble_bl_cmd_mass_erase(const struct device *bl_uart)
{
	if (ble_uart_send_cmd(bl_uart, BLE_BL_CMD_ERASE) < 0) {
		return -1;
	}

	if (ble_uart_send_cmd(bl_uart, 0xFF) < 0) {
		return -1;
	}

	return 0;
}

static int ble_bl_cmd_write_memory(const struct device *bl_uart, uint32_t address,
				   const uint8_t *buf, uint16_t len)
{
	uint32_t address_be = sys_cpu_to_be32(address);
	uint8_t data[256 + 1]; /* buflen-1 (1 byte) + buf (max 256) */

	if (len == 0) {
		return 0;
	}

	if (ble_uart_send_cmd(bl_uart, BLE_BL_CMD_WRITE_MEMORY) < 0) {
		return -1;
	}

	/* send address */
	if (ble_uart_send_data(bl_uart, (uint8_t *)&address_be, 4) < 0) {
		LOG_ERR("send address fail\n");
		return -1;
	}

	/* send firmware data */
	data[0] = (uint8_t)len - 1;
	memcpy(&data[1], buf, len);
	if (ble_uart_send_data(bl_uart, data, len + 1) < 0) {
		LOG_ERR("send data fail\n");
		return -1;
	}

	printf(".");
	gpio_pin_toggle(blue_gpio.port, blue_gpio.pin);
	return 0;
}

static int ble_bl_fw_upgrade(const struct device *bl_uart, const uint8_t *buf, const uint32_t len)
{
	int i;
	uint32_t nb = len / 256;
	uint16_t r = len % 256;
	uint32_t address;
	const uint8_t *datap;

	for (i = 0; i < nb; i++) {
		address = BLE_FLASH_START_ADDRESS + i * 256;
		datap = buf + i * 256;
		if (ble_bl_cmd_write_memory(bl_uart, address, datap, 256) < 0) {
			return -1;
		}
	}

	address = BLE_FLASH_START_ADDRESS + nb * 256;
	datap = buf + nb * 256;
	if (ble_bl_cmd_write_memory(bl_uart, address, datap, r) < 0) {
		return -1;
	}
	printf("\n");

	return 0;
}

/*
 * BLE f/w upgrade would proceed if user gives his feedback either
 * pressing 'y' (or 'Y') on console or pressing button BT2 on the
 * board.
 */
static int check_user_feedback(void)
{
	int in, ret = 0;
	uint8_t go = 0;

	static const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	const struct gpio_dt_spec sw2 = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios);

	if (!device_is_ready(console)) {
		LOG_ERR("console not ready.\n");
		return -1;
	}

	if (!gpio_is_ready_dt(&sw2)) {
		LOG_ERR("%s: device not ready.\n", sw2.port->name);
		return -1;
	}
	gpio_pin_configure_dt(&sw2, GPIO_INPUT);

	while (1) {

		if (ret == 0) {
			printf("Start BLE f/w update (press Y to proceed): ");
		}

		ret = uart_poll_in(console, &go);
		if (ret == 0) {
			printf("%c\n", go);

			if (go == 'y' || go == 'Y') {
				break;
			}
		}

		in = gpio_pin_get_dt(&sw2);
		if (in == 1) {
			break;
		}
	}

	return 0;
}

int main(void)
{
	uint8_t fw_ver;

	printf("SensorTile.box Pro BLE f/w upgrade\n");

	led_init();

	/* signal that sample is started */
	if (led_init() < 0) {
		return -1;
	}

	led_pattern_out(BLE_FW_UPG_START);

	static const struct device *const bl_uart =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_bootloader_uart));

	if (!device_is_ready(bl_uart)) {
		LOG_ERR("bl_uart device not ready.\n");
		goto err;
	}

	/* check if user acknowledge the upgrade */
	if (check_user_feedback() < 0) {
		LOG_ERR("user feedback not working\n");
		goto err;
	}

	if (ble_uart_bootloader_activate(bl_uart) < 0) {
		LOG_ERR("activation failed\n");
		goto err;
	}
	printf("bootloader activated!\n");

	if (ble_bl_cmd_get_version(bl_uart, &fw_ver) < 0) {
		LOG_ERR("get_version failed\n");
		goto err;
	}
	printf("ble bootloader version is %d.0\n", fw_ver);

	if (ble_bl_cmd_mass_erase(bl_uart) < 0) {
		LOG_ERR("mass_erase failed\n");
		goto err;
	}
	printf("MASS ERASE ok\n");

	if (ble_bl_fw_upgrade(bl_uart, ble_fw_img, ble_fw_img_len) < 0) {
		LOG_ERR("write memory failed\n");
		goto err;
	}
	printf("BLE f/w upgrade ok\n");

	led_pattern_out(BLE_FW_UPG_OK);

	return 0;

err:
	led_pattern_out(BLE_FW_UPG_ERR);
	return -1;
}

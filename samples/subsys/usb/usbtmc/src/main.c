/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Sample application for the USBTMC device class. The sample makes the board
 * appear as a tiny laboratory instrument and implements a deliberately small
 * SCPI-like command parser on top of the USBTMC transport.
 */

#include <sample_usbd.h>

#include <ctype.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/class/usbd_usbtmc.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbtmc_sample, LOG_LEVEL_INF);

#define INDICATOR_PULSE_TIME		K_MSEC(750)

/* SCPI error codes used by the instrument, IEEE 488.2, 21.8.9 and 21.8.10 */
#define SCPI_ERR_NONE			0
#define SCPI_ERR_UNDEFINED_HEADER	-113
#define SCPI_ERR_INPUT_BUFFER_OVERRUN	-363

static const struct device *const usbtmc_dev =
	DEVICE_DT_GET(DT_NODELABEL(usbtmc0));

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

/*
 * The identification response is put together from the strings that are also
 * used for the USB device descriptors, a serial number obtained using the
 * HWINFO API when available, and the kernel version as the firmware level.
 */
static char idn_response[128];

static char cmd_buf[128];
static size_t cmd_len;
static bool cmd_discard;

/* Single entry SCPI-like error queue */
static int scpi_error;

static void scpi_error_set(const int error)
{
	if (scpi_error == SCPI_ERR_NONE) {
		scpi_error = error;
	}
}

static const char *scpi_error_text(const int error)
{
	switch (error) {
	case SCPI_ERR_NONE:
		return "No error";
	case SCPI_ERR_UNDEFINED_HEADER:
		return "Undefined header";
	case SCPI_ERR_INPUT_BUFFER_OVERRUN:
		return "Input buffer overrun";
	default:
		return "Device-specific error";
	}
}

static void scpi_reply(const char *const rsp, const size_t len)
{
	int ret;

	ret = usbd_usbtmc_msg_write(usbtmc_dev, (const uint8_t *)rsp, len, true);
	if (ret < 0 || (size_t)ret != len) {
		LOG_ERR("Failed to write response, error %d", ret);
	}
}

static void scpi_idn_handler(void)
{
	scpi_reply(idn_response, strlen(idn_response));
}

static void scpi_rst_handler(void)
{
	scpi_error = SCPI_ERR_NONE;
}

static void scpi_meas_temp_handler(void)
{
	char rsp[16];
	int len;

	/* There is no temperature sensor requirement for the sample,
	 * synthesize a value in celsius that slowly drifts.
	 */
	len = snprintk(rsp, sizeof(rsp), "%u.%02u\n",
		       (uint32_t)(20 + (k_uptime_seconds() / 60) % 10),
		       (uint32_t)(k_uptime_get() % 100));
	scpi_reply(rsp, len);
}

static void scpi_syst_upt_handler(void)
{
	char rsp[16];
	int len;

	len = snprintk(rsp, sizeof(rsp), "%u\n", k_uptime_seconds());
	scpi_reply(rsp, len);
}

static void scpi_syst_err_handler(void)
{
	char rsp[48];
	int len;

	len = snprintk(rsp, sizeof(rsp), "%d,\"%s\"\n",
		       scpi_error, scpi_error_text(scpi_error));
	scpi_error = SCPI_ERR_NONE;
	scpi_reply(rsp, len);
}

static const struct scpi_command {
	const char *name;
	void (*handler)(void);
} scpi_commands[] = {
	{"*IDN?", scpi_idn_handler},
	{"*RST", scpi_rst_handler},
	{"MEAS:TEMP?", scpi_meas_temp_handler},
	{"SYST:UPT?", scpi_syst_upt_handler},
	{"SYST:ERR?", scpi_syst_err_handler},
};

static void scpi_handle_command(const char *const cmd)
{
	LOG_INF("Command: %s", cmd);

	ARRAY_FOR_EACH_PTR(scpi_commands, command) {
		if (strcmp(cmd, command->name) == 0) {
			command->handler();
			return;
		}
	}

	LOG_WRN("Undefined command");
	scpi_error_set(SCPI_ERR_UNDEFINED_HEADER);
}

static void usbtmc_msg_out(const struct device *dev, const uint8_t *const data,
			   const size_t len, const bool begin, const bool eom)
{
	if (begin) {
		cmd_len = 0;
		cmd_discard = false;
	}

	if (cmd_len + len > sizeof(cmd_buf) - 1) {
		cmd_discard = true;
	} else {
		memcpy(&cmd_buf[cmd_len], data, len);
		cmd_len += len;
	}

	if (!eom) {
		return;
	}

	if (cmd_discard) {
		scpi_error_set(SCPI_ERR_INPUT_BUFFER_OVERRUN);
		return;
	}

	/* Strip message terminators and normalize to upper case */
	while (cmd_len > 0 && (cmd_buf[cmd_len - 1] == '\n' ||
			       cmd_buf[cmd_len - 1] == '\r' ||
			       cmd_buf[cmd_len - 1] == ' ')) {
		cmd_len--;
	}

	cmd_buf[cmd_len] = '\0';
	for (size_t i = 0; i < cmd_len; i++) {
		cmd_buf[i] = (char)toupper((unsigned char)cmd_buf[i]);
	}

	scpi_handle_command(cmd_buf);
}

static void usbtmc_ready(const struct device *dev, const bool ready)
{
	LOG_INF("Instrument %s", ready ? "ready" : "disconnected");
}

static void usbtmc_clear(const struct device *dev)
{
	cmd_len = 0;
	cmd_discard = false;
}

static void indicator_off_handler(struct k_work *work)
{
	if (led.port != NULL) {
		gpio_pin_set_dt(&led, 0);
	}
}

static K_WORK_DELAYABLE_DEFINE(indicator_off_work, indicator_off_handler);

static int usbtmc_indicator_pulse(const struct device *dev)
{
	LOG_INF("Indicator pulse");

	if (led.port != NULL) {
		gpio_pin_set_dt(&led, 1);
		k_work_schedule(&indicator_off_work, INDICATOR_PULSE_TIME);
	}

	return 0;
}

static const struct usbd_usbtmc_ops ops = {
	.ready = usbtmc_ready,
	.msg_out = usbtmc_msg_out,
	.clear = usbtmc_clear,
	.indicator_pulse = usbtmc_indicator_pulse,
};

static void instrument_init_idn(void)
{
	char sn[33] = "001337";

	if (IS_ENABLED(CONFIG_HWINFO)) {
		uint8_t hwid[16];
		ssize_t hwid_len;

		/* Use the same serial number as the USB device descriptor,
		 * USBTMC hosts use it to identify the instrument
		 */
		hwid_len = hwinfo_get_device_id(hwid, sizeof(hwid));
		for (ssize_t i = 0; i < hwid_len; i++) {
			snprintk(&sn[i * 2], 3, "%02X", hwid[i]);
		}
	}

	snprintk(idn_response, sizeof(idn_response), "%s,%s,%s,%s\n",
		 CONFIG_SAMPLE_USBD_MANUFACTURER, CONFIG_SAMPLE_USBD_PRODUCT,
		 sn, KERNEL_VERSION_STRING);
}

int main(void)
{
	struct usbd_context *sample_usbd;

	if (!device_is_ready(usbtmc_dev)) {
		LOG_ERR("USBTMC device not ready");
		return -1;
	}

	if (led.port != NULL) {
		if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Unable to setup LED, not using it");
			memset(&led, 0, sizeof(led));
		}
	}

	instrument_init_idn();

	if (usbd_usbtmc_register(usbtmc_dev, &ops)) {
		LOG_ERR("Failed to register USBTMC event handlers");
		return -1;
	}

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -1;
	}

	if (usbd_enable(sample_usbd)) {
		LOG_ERR("Failed to enable device support");
		return -1;
	}

	LOG_INF("USB device support enabled");

	return 0;
}

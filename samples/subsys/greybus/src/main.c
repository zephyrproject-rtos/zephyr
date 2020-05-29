/*
 * Copyright (c) 2109 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <console/console.h>
#include <console/tty.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <errno.h>
#include <greybus/debug.h>
#include <greybus/greybus.h>
#include <greybus-utils/manifest.h>
#include <greybus-utils/platform.h>
#include <posix/unistd.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#ifdef CONFIG_GREYBUS_STATIC_MANIFEST
#include <greybus/static-manifest.h>
#endif

enum {
	GB_TYPE_MANIFEST_SET = 0x42,
	GB_TYPE_ANY = (uint8_t)-1,
};

static struct tty_serial app_serial;
static u8_t app_serial_rxbuf[CONFIG_CONSOLE_GETCHAR_BUFSIZE];
static u8_t app_serial_txbuf[CONFIG_CONSOLE_PUTCHAR_BUFSIZE];

static int getMessage( struct gb_operation_hdr **msg, const u8_t expected_msg_type) {

	int r;
	void *tmp;
	size_t msg_size;
	size_t payload_size;
	size_t remaining;
	size_t offset;
	size_t recvd;

	if (NULL == msg) {
		gb_error("One or more arguments were NULL or invalid\n");
		r = -EINVAL;
		goto out;
	}

	tmp = realloc(*msg, sizeof(**msg));
	if (NULL == tmp) {
		gb_error("Failed to allocate memory\n");
		r = -ENOMEM;
		goto out;
	}
	*msg = tmp;

read_header:
	for( remaining = sizeof(**msg), offset = 0, recvd = 0; remaining; remaining -= recvd, offset += recvd, recvd = 0) {
		//gb_info("attempting to read %u bytes\n", remaining);
		r = tty_read(&app_serial, &((uint8_t *)*msg)[offset], remaining);
		//gb_info("tty_read(%p, &%p[%u], %u) => %d\n", &app_serial, *msg, offset, remaining, r);
		if (r < 0) {
			gb_error("tty_read failed (%d)\n", r);
			usleep(100);
			continue;
		}
		if (0 == r) {
			gb_error("tty_read returned 0 - EOF??\n");
		}
		recvd = r;
	}

	msg_size = sys_le16_to_cpu((*msg)->size);
	//gb_info("msg_size is %u\n", (unsigned)msg_size);

	if (msg_size < sizeof(struct gb_operation_hdr)) {
		gb_error("invalid message size %u\n", (unsigned)msg_size);
		goto read_header;
	}

	payload_size = msg_size - sizeof(**msg);
	//gb_info("payload_size is %u\n", (unsigned)payload_size);

	if (payload_size > GB_MAX_PAYLOAD_SIZE) {
		gb_error("invalid payload size %u\n", (unsigned)payload_size);
		goto read_header;
	}

	if (payload_size > 0) {
		tmp = realloc(*msg, msg_size);
		if (NULL == tmp) {
			gb_error("Failed to allocate memory\n");
			r = -ENOMEM;
			goto freemsg;
		}
		*msg = tmp;

		for( remaining = payload_size, offset = sizeof(**msg), recvd = 0; remaining; remaining -= recvd, offset += recvd, recvd = 0 ) {
			//gb_info("attempting to read %u bytes\n", remaining);
			r = tty_read(&app_serial, &((uint8_t *)*msg)[offset], remaining);
			//gb_info("tty_read(%p, &%p[%u], %u) => %d\n", &app_serial, *msg, offset, remaining, r);
			if (r < 0) {
				gb_error("tty_read failed (%d)\n", r);
				usleep(100);
				continue;
			}
			if (0 == r) {
				gb_error("tty_read returned 0 - EOF??\n");
			}
			recvd = r;
		}
	}

//	uint8_t *d = (uint8_t *)*msg;
//	printk("GB: I: %s() : ", __func__);
//	for( size_t i = 0; i < msg_size; i++ ) {
//		printk("%02x ", d[i]);
//	}
//	printk("\n");

	if (!(GB_TYPE_ANY == expected_msg_type || expected_msg_type == (*msg)->type)) {
		gb_error("Expected message type %d but received messsage type %d\n", expected_msg_type, (*msg)->type);
		r = -EPROTO;
		goto freemsg;
	}

	r = 0;
	goto out;

freemsg:
	free(*msg);
	*msg = NULL;

out:
	return r;
}

static int sendMessage(struct gb_operation_hdr *msg) {

	int r;
	size_t offset;
	size_t remaining;
	size_t written;

	if (NULL == msg) {
		gb_error("One or more arguments were NULL or invalid\n");
		r = -EINVAL;
		goto out;
	}

	for(remaining = sys_le16_to_cpu(msg->size), offset = 0, written = 0; remaining; remaining -= written, offset += written, written = 0) {
		r = tty_write(&app_serial, &((uint8_t *)msg)[offset], remaining);
		//gb_info("tty_write(%p, &%p[%u], %u) => %d\n", &app_serial, msg, offset, remaining, r);
		if (r < 0) {
			gb_error("tty_write failed (%d)\n", r);
			usleep(100);
			continue;
		}
		if (0 == r) {
			gb_error("tty_write returned 0 - EOF??\n");
		}
		written = r;
	}

//	uint8_t *d = (uint8_t *)msg;
//	printk("GB: I: %s(): ", __func__);
//	for( size_t i = 0; i < sys_le16_to_cpu(msg->size); i++ ) {
//		printk("%02x ", d[i]);
//	}
//	printk("\n");

	r = 0;

out:
	return r;
}

static void assert( bool val ) {
	for(;!val;);
}

unsigned int unipro_cport_count(void) {
	// limits us to 1 control port and 1 device
	return 2;
}

static void gb_xport_init(void) {}
static void gb_xport_exit(void) {}
static int gb_xport_listen(unsigned int cport) { return 0; }
static int gb_xport_stop_listening(unsigned int cport) { return 0; }
static int gb_xport_send(unsigned int cport, const void *buf, size_t len) {

	int r;

	struct gb_operation_hdr *msg;
	msg = (struct gb_operation_hdr *)buf;

	msg->pad[0] = cport & 0xff;
	msg->pad[1] = (cport >> 8) & 0xff;

	//gb_info("cport: %u, buf: %p, len: %u\n", cport, buf, len);

	r = sendMessage(msg);

	return r;
}
static void *gb_xport_alloc_buf(size_t size) {
	return malloc(size);
}
static void gb_xport_free_buf(void *ptr) {
	free(ptr);
}

static const struct gb_transport_backend gb_xport = {
    .init = gb_xport_init,
	.exit = gb_xport_exit,
	.listen = gb_xport_listen,
    .stop_listening = gb_xport_stop_listening,
    .send = gb_xport_send,
    .send_async = NULL,
    .alloc_buf = gb_xport_alloc_buf,
    .free_buf = gb_xport_free_buf,
};

const struct gb_gpio_platform_driver gb_gpio_sample;
static void register_gb_platform_drivers() {
	gb_gpio_register_platform_driver((struct gb_gpio_platform_driver *)&gb_gpio_sample);
}

static void blink(struct device *dev, unsigned pin, size_t count, size_t delay) {
	u32_t state = gpio_pin_get(dev, pin);
	for(size_t i = 0; i < count; i++) {
		gpio_pin_set(dev, pin, !state);
		usleep(delay);
		gpio_pin_set(dev, pin, state);
		usleep(delay);
	}
}

// TODO: First, we should upload the private key, then upload authorized keys, then the manifest, and then hand it off to Greybus
void main(void)
{
	int r;
	void *manifest;
	size_t manifest_size;
	struct gb_operation_hdr *msg = NULL;
	struct gb_operation_hdr resp;

	struct device *red_led_dev;
	const unsigned red_led_pin = DT_ALIAS_LED1_GPIOS_PIN;
	red_led_dev = device_get_binding(DT_ALIAS_LED1_GPIOS_CONTROLLER);
	assert(NULL != red_led_dev);
	gpio_pin_configure(red_led_dev, red_led_pin, GPIO_OUTPUT);

	struct device *green_led_dev;
	const unsigned green_led_pin = DT_ALIAS_LED0_GPIOS_PIN;
	green_led_dev = device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER);
	assert(NULL != green_led_dev);
	gpio_pin_configure(green_led_dev, green_led_pin, GPIO_INPUT);

	const char *app_serial_name = "UART_0";
	struct device *uart_dev;

	// The console uart is controlled by CONFIG_UART_CONSOLE_ON_DEV_NAME="UART_1"
	console_init();

	uart_dev = device_get_binding(app_serial_name);
	if (NULL == uart_dev) {
		gb_error("failed to get device for %s\n", app_serial_name);
		assert(0);
	}
	r = tty_init(&app_serial, uart_dev);
	if (r < 0) {
		gb_error("tty_init failed for %s (%d)\n", app_serial_name, r);
		assert(0);
	}
	r = tty_set_tx_buf(&app_serial, app_serial_txbuf, sizeof(app_serial_txbuf));
	if (r < 0) {
		gb_error("tty_set_tx_buf failed for %s (%d)\n", app_serial_name, r);
		assert(0);
	}
	r = tty_set_rx_buf(&app_serial, app_serial_rxbuf, sizeof(app_serial_rxbuf));
	if (r < 0) {
		gb_error("tty_set_rx_buf failed for %s (%d)\n", app_serial_name, r);
		assert(0);
	}

	gb_info("Registering platform drivers..\n");

	register_gb_platform_drivers();

	r = -EIO;

start_over:
#ifdef CONFIG_GREYBUS_STATIC_MANIFEST

	// blink an led twice to indicate that the manifest was received
	blink(red_led_dev, red_led_pin, 2, 100000);

	manifest = get_manifest_blob();
	manifest_size = (size_t)manifest_mnfb_len;
	r = manifest_parse(manifest, manifest_size);
	if (true != r) {
		gb_error("failed to parse manifest\n");
		goto start_over;
	}

#else

	gb_info("Awaiting manifest..\n");

	// blink an led once to indicate that the manifest is expected
	blink(red_led_dev, red_led_pin, 1, 100000);

	r = getMessage(&msg, GB_TYPE_MANIFEST_SET);
	if (r < 0) {
		goto start_over;
	}

	resp = *msg;
	resp.type |= GB_TYPE_RESPONSE_FLAG;
	resp.size = sys_cpu_to_le16(sizeof(resp));

	u16_t msg_size = sys_le16_to_cpu(msg->size);
	manifest_size = msg_size - sizeof(*msg);

	memmove(msg, (uint8_t *)msg + sizeof(*msg), manifest_size);
	manifest = (uint8_t *)msg;
	memset((uint8_t *)msg + manifest_size, 0, sizeof(*msg));
	msg = NULL; // give memory reference to manifest

	// blink an led twice to indicate that the manifest was received
	blink(red_led_dev, red_led_pin, 2, 100000);

	gb_info("Received manifest\n");

	gb_info("Parsing manifest\n");

	r = manifest_parse(manifest, manifest_size);
	if (true != r) {
		gb_error("failed to parse manifest\n");
		resp.result = gb_errno_to_op_result(-EINVAL);
		sendMessage(&resp);
		goto start_over;
	}

	gb_info("Parsed manifest\n");
#endif

	set_manifest_blob(manifest);

	gb_info("Calling gb_init()\n");

	r = gb_init((struct gb_transport_backend *)&gb_xport);
	if (0 != r) {
		gb_error("gb_init() failed (%d)\n", r);
		resp.result = gb_errno_to_op_result(r);
		sendMessage(&resp);
		goto start_over;
	}

	gb_info("Calling enable_cports()\n");
	enable_cports();

#ifndef CONFIG_GREYBUS_STATIC_MANIFEST
	gb_info("Sending back success message()\n");
	resp.result = GB_OP_SUCCESS;
	sendMessage(&resp);
#endif

	// Turn on the red LED to indicate the device is 'online'
	gpio_pin_set(red_led_dev, red_led_pin, 1);

	gb_info("Greybus is active.\n");

	msg = NULL;
	for( size_t i = 0;; i++) {

		//gb_info("Awaiting message..\n");

		// get messages and pass them to greybus
		r = getMessage(&msg, GB_TYPE_ANY);
		if (0 != r) {
			gb_error("failed to receive message (%d)\n", r);
			continue;
		}

		//gb_info("Received message\n");

		unsigned int cport = ((uint16_t)msg->pad[1] << 8) | msg->pad[0];
		//blink(red_led_dev, red_led_pin, 1, 10000);

		r = greybus_rx_handler(cport, msg, sys_le16_to_cpu(msg->size));
		if ( 0 == r ) {
			//gb_info("Handled message!\n");
		} else {
			gb_error("failed to handle message %u: size: %u, id: %u, type: %u\n", i, sys_le16_to_cpu(msg->size), sys_le16_to_cpu(msg->id), msg->type);
		}
	}
}

/*
 * Simple Greybus GPIO abstraction that only uses 1 gpio
 */

static const unsigned green_led_pin = DT_ALIAS_LED0_GPIOS_PIN;
static bool green_led_direction = 1; // make it an input by default

static uint8_t gb_gpio_sample_line_count(void) {
	return 1;
}
static int gb_gpio_sample_activate(uint8_t which) {
	return 0;
}
static int gb_gpio_sample_deactivate(uint8_t which) {
	return 0;
}
static int gb_gpio_sample_get_direction(uint8_t which) {

	if (0 != which) {
		return -ENODEV;
	}

	return green_led_direction;
}
static int gb_gpio_sample_direction_in(uint8_t which) {

	if (0 != which) {
		return -ENODEV;
	}

	struct device *green_led_dev;
	green_led_dev = device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER);
	gpio_pin_configure(green_led_dev, green_led_pin, GPIO_INPUT);

	return 0;
}
static int gb_gpio_sample_direction_out(uint8_t which, uint8_t val) {

	if (0 != which) {
		return -ENODEV;
	}

	struct device *green_led_dev;
	green_led_dev = device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER);
	gpio_pin_configure(green_led_dev, green_led_pin, GPIO_OUTPUT);
	gpio_pin_set(green_led_dev, green_led_pin, val);

	return 0;
}
static int gb_gpio_sample_get_value(uint8_t which) {

	if (0 != which) {
		return -ENODEV;
	}

	struct device *green_led_dev;
	green_led_dev = device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER);
	return gpio_pin_get(green_led_dev, green_led_pin);
}
static int gb_gpio_sample_set_value(uint8_t which, uint8_t val) {

	if (0 != which) {
		return -ENODEV;
	}

	struct device *green_led_dev;
	green_led_dev = device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER);
	gpio_pin_set(green_led_dev, green_led_pin, val);

	return 0;
}
static int gb_gpio_sample_set_debounce(uint8_t which, uint16_t usec) {
	return -ENOSYS;
}
static int gb_gpio_sample_irq_mask(uint8_t which) {
	return -ENOSYS;
}
static int gb_gpio_sample_irq_unmask(uint8_t which) {
	return -ENOSYS;
}
static int gb_gpio_sample_irq_type(uint8_t which, uint8_t type) {
	return -ENOSYS;
}

const struct gb_gpio_platform_driver gb_gpio_sample = {
	.line_count = gb_gpio_sample_line_count,
	.activate = gb_gpio_sample_activate,
	.deactivate = gb_gpio_sample_deactivate,
	.get_direction = gb_gpio_sample_get_direction,
	.direction_in = gb_gpio_sample_direction_in,
	.direction_out = gb_gpio_sample_direction_out,
	.get_value = gb_gpio_sample_get_value,
	.set_value = gb_gpio_sample_set_value,
	.set_debounce = gb_gpio_sample_set_debounce,
	.irq_mask = gb_gpio_sample_irq_mask,
	.irq_unmask = gb_gpio_sample_irq_unmask,
	.irq_type = gb_gpio_sample_irq_type,
};


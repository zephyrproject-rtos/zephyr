/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <zephyr/random/random.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/usb/class/usb_cdc.h>

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

#define SW0_NODE DT_ALIAS(sw0)

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
static const struct gpio_dt_spec sw0_gpio = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
#endif

#define SW1_NODE DT_ALIAS(sw1)

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
static const struct gpio_dt_spec sw1_gpio = GPIO_DT_SPEC_GET(SW1_NODE, gpios);
#endif

#define SW2_NODE DT_ALIAS(sw2)

#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
static const struct gpio_dt_spec sw2_gpio = GPIO_DT_SPEC_GET(SW2_NODE, gpios);
#endif

#define SW3_NODE DT_ALIAS(sw3)

#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
static const struct gpio_dt_spec sw3_gpio = GPIO_DT_SPEC_GET(SW3_NODE, gpios);
#endif

/* Event FIFO */

K_FIFO_DEFINE(evt_fifo);

enum evt_t {
	GPIO_BUTTON_0	= 0x00,
	GPIO_BUTTON_1	= 0x01,
	GPIO_BUTTON_2	= 0x02,
	GPIO_BUTTON_3	= 0x03,
	CDC_UP		= 0x04,
	CDC_DOWN	= 0x05,
	CDC_LEFT	= 0x06,
	CDC_RIGHT	= 0x07,
	CDC_UNKNOWN	= 0x08,
	CDC_STRING	= 0x09,
	HID_MOUSE_CLEAR	= 0x0A,
	HID_KBD_CLEAR	= 0x0B,
	HID_KBD_STRING	= 0x0C,
};

struct app_evt_t {
	sys_snode_t node;
	enum evt_t event_type;
};

#define FIFO_ELEM_MIN_SZ        sizeof(struct app_evt_t)
#define FIFO_ELEM_MAX_SZ        sizeof(struct app_evt_t)
#define FIFO_ELEM_COUNT         255
#define FIFO_ELEM_ALIGN         sizeof(unsigned int)

K_HEAP_DEFINE(event_elem_pool, FIFO_ELEM_MAX_SZ * FIFO_ELEM_COUNT + 256);

static inline void app_evt_free(struct app_evt_t *ev)
{
	k_heap_free(&event_elem_pool, ev);
}

static inline void app_evt_put(struct app_evt_t *ev)
{
	k_fifo_put(&evt_fifo, ev);
}

static inline struct app_evt_t *app_evt_get(void)
{
	return k_fifo_get(&evt_fifo, K_NO_WAIT);
}

static inline void app_evt_flush(void)
{
	struct app_evt_t *ev;

	do {
		ev = app_evt_get();
		if (ev) {
			app_evt_free(ev);
		}
	} while (ev != NULL);
}

static inline struct app_evt_t *app_evt_alloc(void)
{
	struct app_evt_t *ev;

	ev = k_heap_alloc(&event_elem_pool,
			  sizeof(struct app_evt_t),
			  K_NO_WAIT);
	if (ev == NULL) {
		LOG_ERR("APP event allocation failed!");
		app_evt_flush();

		ev = k_heap_alloc(&event_elem_pool,
				  sizeof(struct app_evt_t),
				  K_NO_WAIT);
		if (ev == NULL) {
			LOG_ERR("APP event memory corrupted.");
			__ASSERT_NO_MSG(0);
			return NULL;
		}
		return NULL;
	}

	return ev;
}

/* HID */

static const uint8_t hid_mouse_report_desc[] = HID_MOUSE_REPORT_DESC(2);
static const uint8_t hid_kbd_report_desc[] = HID_KEYBOARD_REPORT_DESC();

static K_SEM_DEFINE(evt_sem, 0, 1);	/* starts off "not available" */
static K_SEM_DEFINE(usb_sem, 1, 1);	/* starts off "available" */
static struct gpio_callback gpio_callbacks[4];

static char data_buf_mouse[64], data_buf_kbd[64];
static char string[64];
static uint8_t chr_ptr_mouse, chr_ptr_kbd, str_pointer;

#define MOUSE_BTN_REPORT_POS	0
#define MOUSE_X_REPORT_POS	1
#define MOUSE_Y_REPORT_POS	2

#define MOUSE_BTN_LEFT		BIT(0)
#define MOUSE_BTN_RIGHT		BIT(1)
#define MOUSE_BTN_MIDDLE	BIT(2)

static const char *welcome	=	"Welcome to ";
static const char *banner0	=	"\r\n"
					"Supported commands:\r\n"
					"up    - moves the mouse up\r\n"
					"down  - moves the mouse down\r\n"
					"right - moves the mouse to right\r\n"
					"left  - moves the mouse to left\r\n";
static const char *banner1	=	"\r\n"
					"Enter a string and terminate "
					"it with ENTER.\r\n"
					"It will be sent via HID "
					"when BUTTON 2 is pressed.\r\n"
					"You can modify it by sending "
					"a new one here.\r\n";
static const char *gpio0	=	"Button 0 pressed\r\n";
static const char *gpio1	=	"Button 1 pressed\r\n";
static const char *gpio2	=	"Button 2 pressed\r\n";
static const char *gpio3	=	"Button 3 pressed\r\n";
static const char *unknown	=	"Command not recognized.\r\n";
static const char *up		=	"Mouse up\r\n";
static const char *down		=	"Mouse down\r\n";
static const char *left		=	"Mouse left\r\n";
static const char *right	=	"Mouse right\r\n";
static const char *evt_fail	=	"Unknown event detected!\r\n";
static const char *set_str	=	"String set to: ";
static const char *endl		=	"\r\n";

static void in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_sem_give(&usb_sem);
}

static const struct hid_ops ops = {
	.int_in_ready = in_ready_cb,
};

static void clear_mouse_report(void)
{
	struct app_evt_t *new_evt = app_evt_alloc();

	new_evt->event_type = HID_MOUSE_CLEAR;
	app_evt_put(new_evt);
	k_sem_give(&evt_sem);
}

static void clear_kbd_report(void)
{
	struct app_evt_t *new_evt = app_evt_alloc();

	new_evt->event_type = HID_KBD_CLEAR;
	app_evt_put(new_evt);
	k_sem_give(&evt_sem);
}

static int ascii_to_hid(uint8_t ascii)
{
	if (ascii < 32) {
		/* Character not supported */
		return -1;
	} else if (ascii < 48) {
		/* Special characters */
		switch (ascii) {
		case 32:
			return HID_KEY_SPACE;
		case 33:
			return HID_KEY_1;
		case 34:
			return HID_KEY_APOSTROPHE;
		case 35:
			return HID_KEY_3;
		case 36:
			return HID_KEY_4;
		case 37:
			return HID_KEY_5;
		case 38:
			return HID_KEY_7;
		case 39:
			return HID_KEY_APOSTROPHE;
		case 40:
			return HID_KEY_9;
		case 41:
			return HID_KEY_0;
		case 42:
			return HID_KEY_8;
		case 43:
			return HID_KEY_EQUAL;
		case 44:
			return HID_KEY_COMMA;
		case 45:
			return HID_KEY_MINUS;
		case 46:
			return HID_KEY_DOT;
		case 47:
			return HID_KEY_SLASH;
		default:
			return -1;
		}
	} else if (ascii < 58) {
		/* Numbers */
		if (ascii == 48U) {
			return HID_KEY_0;
		} else {
			return ascii - 19;
		}
	} else if (ascii < 65) {
		/* Special characters #2 */
		switch (ascii) {
		case 58:
			return HID_KEY_SEMICOLON;
		case 59:
			return HID_KEY_SEMICOLON;
		case 60:
			return HID_KEY_COMMA;
		case 61:
			return HID_KEY_EQUAL;
		case 62:
			return HID_KEY_DOT;
		case 63:
			return HID_KEY_SLASH;
		case 64:
			return HID_KEY_2;
		default:
			return -1;
		}
	} else if (ascii < 91) {
		/* Uppercase characters */
		return ascii - 61U;
	} else if (ascii < 97) {
		/* Special characters #3 */
		switch (ascii) {
		case 91:
			return HID_KEY_LEFTBRACE;
		case 92:
			return HID_KEY_BACKSLASH;
		case 93:
			return HID_KEY_RIGHTBRACE;
		case 94:
			return HID_KEY_6;
		case 95:
			return HID_KEY_MINUS;
		case 96:
			return HID_KEY_GRAVE;
		default:
			return -1;
		}
	} else if (ascii < 123) {
		/* Lowercase letters */
		return ascii - 93;
	} else if (ascii < 128) {
		/* Special characters #4 */
		switch (ascii) {
		case 123:
			return HID_KEY_LEFTBRACE;
		case 124:
			return HID_KEY_BACKSLASH;
		case 125:
			return HID_KEY_RIGHTBRACE;
		case 126:
			return HID_KEY_GRAVE;
		case 127:
			return HID_KEY_DELETE;
		default:
			return -1;
		}
	}

	return -1;
}

static bool needs_shift(uint8_t ascii)
{
	if ((ascii < 33) || (ascii == 39U)) {
		return false;
	} else if ((ascii >= 33U) && (ascii < 44)) {
		return true;
	} else if ((ascii >= 44U) && (ascii < 58)) {
		return false;
	} else if ((ascii == 59U) || (ascii == 61U)) {
		return false;
	} else if ((ascii >= 58U) && (ascii < 91)) {
		return true;
	} else if ((ascii >= 91U) && (ascii < 94)) {
		return false;
	} else if ((ascii == 94U) || (ascii == 95U)) {
		return true;
	} else if ((ascii > 95) && (ascii < 123)) {
		return false;
	} else if ((ascii > 122) && (ascii < 127)) {
		return true;
	} else {
		return false;
	}
}

/* CDC ACM */

static volatile bool data_transmitted;
static volatile bool data_arrived;

static void flush_buffer_mouse(void)
{
	chr_ptr_mouse = 0U;
	memset(data_buf_mouse, 0, sizeof(data_buf_mouse));
}

static void flush_buffer_kbd(void)
{
	chr_ptr_kbd = 0U;
	memset(data_buf_kbd, 0, sizeof(data_buf_kbd));
}

static void write_data(const struct device *dev, const char *buf, int len)
{
	uart_irq_tx_enable(dev);

	while (len) {
		int written;

		data_transmitted = false;
		written = uart_fifo_fill(dev, (const uint8_t *)buf, len);
		while (data_transmitted == false) {
			k_yield();
		}

		len -= written;
		buf += written;
	}

	uart_irq_tx_disable(dev);
}

static void cdc_mouse_int_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}
	uint32_t bytes_read;

	while ((bytes_read = uart_fifo_read(dev,
		(uint8_t *)data_buf_mouse+chr_ptr_mouse,
		sizeof(data_buf_mouse)-chr_ptr_mouse))) {
		chr_ptr_mouse += bytes_read;
		if (data_buf_mouse[chr_ptr_mouse - 1] == '\r') {
			/* ENTER */
			struct app_evt_t *ev = app_evt_alloc();

			data_buf_mouse[chr_ptr_mouse - 1] = '\0';

			if (!strcmp(data_buf_mouse, "up")) {
				ev->event_type = CDC_UP;
			} else if (!strcmp(data_buf_mouse, "down")) {
				ev->event_type = CDC_DOWN;
			} else if (!strcmp(data_buf_mouse, "right")) {
				ev->event_type = CDC_RIGHT;
			} else if (!strcmp(data_buf_mouse, "left")) {
				ev->event_type = CDC_LEFT;
			} else {
				ev->event_type = CDC_UNKNOWN;
			}
			flush_buffer_mouse();
			app_evt_put(ev);
			k_sem_give(&evt_sem);
		}

		if (chr_ptr_mouse >= sizeof(data_buf_mouse)) {
			LOG_WRN("Buffer overflow");
			flush_buffer_mouse();
		}
	}
}

static void cdc_kbd_int_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}
	uint32_t bytes_read;

	while ((bytes_read = uart_fifo_read(dev,
		(uint8_t *)data_buf_kbd+chr_ptr_kbd,
		sizeof(data_buf_kbd)-chr_ptr_kbd))) {
		chr_ptr_kbd += bytes_read;
		if (data_buf_kbd[chr_ptr_kbd - 1] == '\r') {
			/* ENTER */
			struct app_evt_t *ev = app_evt_alloc();

			data_buf_kbd[chr_ptr_kbd - 1] = '\0';
			strcpy(string, data_buf_kbd);
			ev->event_type = CDC_STRING;
			flush_buffer_kbd();
			app_evt_put(ev);
			k_sem_give(&evt_sem);
		}
	}
}

/* Devices */

static void btn0(const struct device *gpio, struct gpio_callback *cb,
		 uint32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	ev->event_type = GPIO_BUTTON_0,
	app_evt_put(ev);
	k_sem_give(&evt_sem);
}

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
static void btn1(const struct device *gpio, struct gpio_callback *cb,
		 uint32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	ev->event_type = GPIO_BUTTON_1,
	app_evt_put(ev);
	k_sem_give(&evt_sem);
}
#endif

#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
static void btn2(const struct device *gpio, struct gpio_callback *cb,
		 uint32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	ev->event_type = GPIO_BUTTON_2,
	app_evt_put(ev);
	k_sem_give(&evt_sem);
}
#endif

#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
static void btn3(const struct device *gpio, struct gpio_callback *cb,
		 uint32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	ev->event_type = GPIO_BUTTON_3,
	app_evt_put(ev);
	k_sem_give(&evt_sem);
}
#endif

int callbacks_configure(const struct gpio_dt_spec *gpio,
			void (*handler)(const struct device *, struct gpio_callback*,
					uint32_t),
			struct gpio_callback *callback)
{
	if (!device_is_ready(gpio->port)) {
		LOG_ERR("%s: device not ready.", gpio->port->name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(gpio, GPIO_INPUT);

	gpio_init_callback(callback, handler, BIT(gpio->pin));
	gpio_add_callback(gpio->port, callback);
	gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	LOG_INF("Status %d", status);
}

#define DEVICE_AND_COMMA(node_id) DEVICE_DT_GET(node_id),

int main(void)
{
	const struct device *cdc_dev[] = {
		DT_FOREACH_STATUS_OKAY(zephyr_cdc_acm_uart, DEVICE_AND_COMMA)
	};
	BUILD_ASSERT(ARRAY_SIZE(cdc_dev) >= 2, "Not enough CDC ACM instances");
	const struct device *hid0_dev, *hid1_dev;
	struct app_evt_t *ev;
	uint32_t dtr = 0U;
	int ret;

	/* Configure devices */

	hid0_dev = device_get_binding("HID_0");
	if (hid0_dev == NULL) {
		LOG_ERR("Cannot get USB HID 0 Device");
		return 0;
	}

	hid1_dev = device_get_binding("HID_1");
	if (hid1_dev == NULL) {
		LOG_ERR("Cannot get USB HID 1 Device");
		return 0;
	}

	for (int idx = 0; idx < ARRAY_SIZE(cdc_dev); idx++) {
		if (!device_is_ready(cdc_dev[idx])) {
			LOG_ERR("CDC ACM device %s is not ready",
				cdc_dev[idx]->name);
			return 0;
		}
	}

	if (callbacks_configure(&sw0_gpio, &btn0, &gpio_callbacks[0])) {
		LOG_ERR("Failed configuring button 0 callback.");
		return 0;
	}

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
	if (callbacks_configure(&sw1_gpio, &btn1, &gpio_callbacks[1])) {
		LOG_ERR("Failed configuring button 1 callback.");
		return 0;
	}
#endif

#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
	if (callbacks_configure(&sw2_gpio, &btn2, &gpio_callbacks[2])) {
		LOG_ERR("Failed configuring button 2 callback.");
		return 0;
	}
#endif

#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
	if (callbacks_configure(&sw3_gpio, &btn3, &gpio_callbacks[3])) {
		LOG_ERR("Failed configuring button 3 callback.");
		return 0;
	}
#endif

	/* Initialize HID */

	usb_hid_register_device(hid0_dev, hid_mouse_report_desc,
				sizeof(hid_mouse_report_desc), &ops);

	usb_hid_register_device(hid1_dev, hid_kbd_report_desc,
				sizeof(hid_kbd_report_desc), &ops);

	usb_hid_init(hid0_dev);
	usb_hid_init(hid1_dev);

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	/* Initialize CDC ACM */
	for (int idx = 0; idx < ARRAY_SIZE(cdc_dev); idx++) {
		LOG_INF("Wait for DTR on %s", cdc_dev[idx]->name);
		while (1) {
			uart_line_ctrl_get(cdc_dev[idx],
					   UART_LINE_CTRL_DTR,
					   &dtr);
			if (dtr) {
				break;
			} else {
				/* Give CPU resources to low priority threads. */
				k_sleep(K_MSEC(100));
			}
		}

		LOG_INF("DTR on device %s", cdc_dev[idx]->name);
	}

	/* Wait 1 sec for the host to do all settings */
	k_busy_wait(USEC_PER_SEC);

	uart_irq_callback_set(cdc_dev[0], cdc_mouse_int_handler);
	uart_irq_callback_set(cdc_dev[1], cdc_kbd_int_handler);

	write_data(cdc_dev[0], welcome, strlen(welcome));
	write_data(cdc_dev[0], cdc_dev[0]->name, strlen(cdc_dev[0]->name));
	write_data(cdc_dev[0], banner0, strlen(banner0));
	write_data(cdc_dev[1], welcome, strlen(welcome));
	write_data(cdc_dev[1], cdc_dev[1]->name, strlen(cdc_dev[1]->name));
	write_data(cdc_dev[1], banner1, strlen(banner1));

	uart_irq_rx_enable(cdc_dev[0]);
	uart_irq_rx_enable(cdc_dev[1]);

	while (true) {
		k_sem_take(&evt_sem, K_FOREVER);

		while ((ev = app_evt_get()) != NULL) {
			switch (ev->event_type) {
			case GPIO_BUTTON_0:
			{
				/* Move the mouse in random direction */
				uint8_t rep[] = {0x00, sys_rand8_get(),
					      sys_rand8_get(), 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc_dev[0], gpio0, strlen(gpio0));
				clear_mouse_report();
				break;
			}
			case GPIO_BUTTON_1:
			{
				/* Press left mouse button */
				uint8_t rep[] = {0x00, 0x00, 0x00, 0x00};

				rep[MOUSE_BTN_REPORT_POS] |= MOUSE_BTN_LEFT;
				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc_dev[0], gpio1, strlen(gpio1));
				clear_mouse_report();
				break;
			}
			case GPIO_BUTTON_2:
			{
				/* Send string on HID keyboard */
				write_data(cdc_dev[1], gpio2, strlen(gpio2));
				if (strlen(string) > 0) {
					struct app_evt_t *ev2 = app_evt_alloc();

					ev2->event_type = HID_KBD_STRING,
					app_evt_put(ev2);
					str_pointer = 0U;
					k_sem_give(&evt_sem);
				}
				break;
			}
			case GPIO_BUTTON_3:
			{
				/* Toggle CAPS LOCK */
				uint8_t rep[] = {0x00, 0x00, 0x00, 0x00,
					      0x00, 0x00, 0x00,
					      HID_KEY_CAPSLOCK};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid1_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc_dev[1], gpio3, strlen(gpio3));
				clear_kbd_report();
				break;
			}
			case CDC_UP:
			{
				/* Mouse up */
				uint8_t rep[] = {0x00, 0x00, 0xE0, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc_dev[0], up, strlen(up));
				clear_mouse_report();
				break;
			}
			case CDC_DOWN:
			{
				/* Mouse down */
				uint8_t rep[] = {0x00, 0x00, 0x20, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc_dev[0], down, strlen(down));
				clear_mouse_report();
				break;
			}
			case CDC_RIGHT:
			{
				/* Mouse right */
				uint8_t rep[] = {0x00, 0x20, 0x00, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc_dev[0], right, strlen(right));
				clear_mouse_report();
				break;
			}
			case CDC_LEFT:
			{
				/* Mouse left */
				uint8_t rep[] = {0x00, 0xE0, 0x00, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc_dev[0], left, strlen(left));
				clear_mouse_report();
				break;
			}
			case CDC_UNKNOWN:
			{
				write_data(cdc_dev[0], unknown, strlen(unknown));
				write_data(cdc_dev[1], unknown, strlen(unknown));
				break;
			}
			case CDC_STRING:
			{
				write_data(cdc_dev[0], set_str, strlen(set_str));
				write_data(cdc_dev[0], string, strlen(string));
				write_data(cdc_dev[0], endl, strlen(endl));

				write_data(cdc_dev[1], set_str, strlen(set_str));
				write_data(cdc_dev[1], string, strlen(string));
				write_data(cdc_dev[1], endl, strlen(endl));
				break;
			}
			case HID_MOUSE_CLEAR:
			{
				/* Clear mouse report */
				uint8_t rep[] = {0x00, 0x00, 0x00, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				break;
			}
			case HID_KBD_CLEAR:
			{
				/* Clear kbd report */
				uint8_t rep[] = {0x00, 0x00, 0x00, 0x00,
					      0x00, 0x00, 0x00, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid1_dev, rep,
						 sizeof(rep), NULL);
				break;
			}
			case HID_KBD_STRING:
			{
				int ch = ascii_to_hid(string[str_pointer]);

				if (ch == -1) {
					LOG_WRN("Unsupported character: %d",
						string[str_pointer]);
				} else {
					uint8_t rep[] = {0x00, 0x00, 0x00, 0x00,
						      0x00, 0x00, 0x00, 0x00};
					if (needs_shift(string[str_pointer])) {
						rep[0] |=
						HID_KBD_MODIFIER_RIGHT_SHIFT;
					}
					rep[7] = ch;

					k_sem_take(&usb_sem, K_FOREVER);
					hid_int_ep_write(hid1_dev, rep,
							sizeof(rep), NULL);
				}

				str_pointer++;

				if (strlen(string) > str_pointer) {
					struct app_evt_t *ev2 = app_evt_alloc();

					ev2->event_type = HID_KBD_STRING,
					app_evt_put(ev2);
					k_sem_give(&evt_sem);
				} else if (strlen(string) == str_pointer) {
					clear_kbd_report();
				}

				break;
			}
			default:
			{
				LOG_ERR("Unknown event to execute");
				write_data(cdc_dev[0], evt_fail,
					   strlen(evt_fail));
				write_data(cdc_dev[1], evt_fail,
					   strlen(evt_fail));
				break;
			}
			break;
			}
			app_evt_free(ev);
		}
	}
}

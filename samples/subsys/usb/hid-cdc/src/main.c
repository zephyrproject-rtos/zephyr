/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <string.h>
#include <random/rand32.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>
#include <usb/class/usb_cdc.h>

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

#ifdef DT_ALIAS_SW0_GPIOS_CONTROLLER
#define PORT0 DT_ALIAS_SW0_GPIOS_CONTROLLER
#else
#error DT_ALIAS_SW0_GPIOS_CONTROLLER needs to be set
#endif

#ifdef DT_ALIAS_SW0_GPIOS_PIN
#define PIN0     DT_ALIAS_SW0_GPIOS_PIN
#else
#error DT_ALIAS_SW0_GPIOS_PIN needs to be set
#endif

#ifdef DT_ALIAS_SW0_GPIOS_FLAGS
#define PIN0_FLAGS DT_ALIAS_SW0_GPIOS_FLAGS
#else
#error DT_ALIAS_SW0_GPIOS_FLAGS needs to be set
#endif

#ifdef DT_ALIAS_SW1_GPIOS_PIN
#define PIN1	DT_ALIAS_SW1_GPIOS_PIN
#endif

#ifdef DT_ALIAS_SW1_GPIOS_CONTROLLER
#define PORT1	DT_ALIAS_SW1_GPIOS_CONTROLLER
#endif

#ifdef DT_ALIAS_SW1_GPIOS_FLAGS
#define PIN1_FLAGS DT_ALIAS_SW1_GPIOS_FLAGS
#endif

#ifdef DT_ALIAS_SW2_GPIOS_PIN
#define PIN2	DT_ALIAS_SW2_GPIOS_PIN
#endif

#ifdef DT_ALIAS_SW2_GPIOS_CONTROLLER
#define PORT2	DT_ALIAS_SW2_GPIOS_CONTROLLER
#endif

#ifdef DT_ALIAS_SW2_GPIOS_FLAGS
#define PIN2_FLAGS DT_ALIAS_SW2_GPIOS_FLAGS
#endif

#ifdef DT_ALIAS_SW3_GPIOS_PIN
#define PIN3	DT_ALIAS_SW3_GPIOS_PIN
#endif

#ifdef DT_ALIAS_SW3_GPIOS_CONTROLLER
#define PORT3	DT_ALIAS_SW3_GPIOS_CONTROLLER
#endif

#ifdef DT_ALIAS_SW3_GPIOS_FLAGS
#define PIN3_FLAGS DT_ALIAS_SW3_GPIOS_FLAGS
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
	struct k_mem_block block;
	enum evt_t event_type;
};

#define FIFO_ELEM_MIN_SZ        sizeof(struct app_evt_t)
#define FIFO_ELEM_MAX_SZ        sizeof(struct app_evt_t)
#define FIFO_ELEM_COUNT         255
#define FIFO_ELEM_ALIGN         sizeof(unsigned int)

K_MEM_POOL_DEFINE(event_elem_pool, FIFO_ELEM_MIN_SZ, FIFO_ELEM_MAX_SZ,
		  FIFO_ELEM_COUNT, FIFO_ELEM_ALIGN);

static inline void app_evt_free(struct app_evt_t *ev)
{
	k_mem_pool_free(&ev->block);
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
	int ret;
	struct app_evt_t *ev;
	struct k_mem_block block;

	ret = k_mem_pool_alloc(&event_elem_pool, &block,
			       sizeof(struct app_evt_t),
			       K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("APP event allocation failed!");
		app_evt_flush();

		ret = k_mem_pool_alloc(&event_elem_pool, &block,
				       sizeof(struct app_evt_t),
				       K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("APP event memory corrupted.");
			__ASSERT_NO_MSG(0);
			return NULL;
		}
		return NULL;
	}

	ev = (struct app_evt_t *)block.data;
	ev->block = block;

	return ev;
}

/* HID */

static const u8_t hid_mouse_report_desc[] = HID_MOUSE_REPORT_DESC(2);
static const u8_t hid_kbd_report_desc[] = HID_KEYBOARD_REPORT_DESC();

static K_SEM_DEFINE(evt_sem, 0, 1);	/* starts off "not available" */
static K_SEM_DEFINE(usb_sem, 1, 1);	/* starts off "available" */
static struct gpio_callback callback[4];

static char data_buf_mouse[64], data_buf_kbd[64];
static char string[64];
static u8_t chr_ptr_mouse, chr_ptr_kbd, str_pointer;

#define MOUSE_BTN_REPORT_POS	0
#define MOUSE_X_REPORT_POS	1
#define MOUSE_Y_REPORT_POS	2

#define MOUSE_BTN_LEFT		BIT(0)
#define MOUSE_BTN_RIGHT		BIT(1)
#define MOUSE_BTN_MIDDLE	BIT(2)

static const char *banner0	=	"Welcome to CDC ACM 0!\r\n"
					"Supported commands:\r\n"
					"up    - moves the mouse up\r\n"
					"down  - moves the mouse down\r\n"
					"right - moves the mouse to right\r\n"
					"left  - moves the mouse to left\r\n";
static const char *banner1	=	"Welcome to CDC ACM 1!\r\n"
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

static void in_ready_cb(void)
{
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

static int ascii_to_hid(u8_t ascii)
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

static bool needs_shift(u8_t ascii)
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

static void write_data(struct device *dev, const char *buf, int len)
{
	uart_irq_tx_enable(dev);

	while (len) {
		int written;

		data_transmitted = false;
		written = uart_fifo_fill(dev, (const u8_t *)buf, len);
		while (data_transmitted == false) {
			k_yield();
		}

		len -= written;
		buf += written;
	}

	uart_irq_tx_disable(dev);
}

static void cdc_mouse_int_handler(struct device *dev)
{
	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}
	u32_t bytes_read;

	while ((bytes_read = uart_fifo_read(dev,
		(u8_t *)data_buf_mouse+chr_ptr_mouse,
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

static void cdc_kbd_int_handler(struct device *dev)
{
	uart_irq_update(dev);

	if (uart_irq_tx_ready(dev)) {
		data_transmitted = true;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}
	u32_t bytes_read;

	while ((bytes_read = uart_fifo_read(dev,
		(u8_t *)data_buf_kbd+chr_ptr_kbd,
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

static void btn0(struct device *gpio, struct gpio_callback *cb, u32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	ev->event_type = GPIO_BUTTON_0,
	app_evt_put(ev);
	k_sem_give(&evt_sem);
}

#ifdef DT_ALIAS_SW1_GPIOS_PIN
static void btn1(struct device *gpio, struct gpio_callback *cb, u32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	ev->event_type = GPIO_BUTTON_1,
	app_evt_put(ev);
	k_sem_give(&evt_sem);
}
#endif

#ifdef DT_ALIAS_SW2_GPIOS_PIN
static void btn2(struct device *gpio, struct gpio_callback *cb, u32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	ev->event_type = GPIO_BUTTON_2,
	app_evt_put(ev);
	k_sem_give(&evt_sem);
}
#endif

#ifdef DT_ALIAS_SW3_GPIOS_PIN
static void btn3(struct device *gpio, struct gpio_callback *cb, u32_t pins)
{
	struct app_evt_t *ev = app_evt_alloc();

	ev->event_type = GPIO_BUTTON_3,
	app_evt_put(ev);
	k_sem_give(&evt_sem);
}
#endif

int callbacks_configure(struct device *gpio, u32_t pin, int flags,
			void (*handler)(struct device*, struct gpio_callback*,
			u32_t), struct gpio_callback *callback)
{
	if (!gpio) {
		LOG_ERR("Could not find PORT");
		return -ENXIO;
	}
	gpio_pin_configure(gpio, pin,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_DEBOUNCE |
			   GPIO_INT_EDGE | flags);
	gpio_init_callback(callback, handler, BIT(pin));
	gpio_add_callback(gpio, callback);
	gpio_pin_enable_callback(gpio, pin);
	return 0;
}

void main(void)
{
	struct device *hid0_dev, *hid1_dev, *cdc0_dev, *cdc1_dev;
	u32_t dtr = 0U;
	struct app_evt_t *ev;

	/* Configure devices */

	hid0_dev = device_get_binding("HID_0");
	if (hid0_dev == NULL) {
		LOG_ERR("Cannot get USB HID 0 Device");
		return;
	}

	hid1_dev = device_get_binding("HID_1");
	if (hid1_dev == NULL) {
		LOG_ERR("Cannot get USB HID 1 Device");
		return;
	}

	cdc0_dev = device_get_binding("CDC_ACM_0");
	if (cdc0_dev == NULL) {
		LOG_ERR("Cannot get USB CDC 0 Device");
		return;
	}

	cdc1_dev = device_get_binding("CDC_ACM_1");
	if (cdc1_dev == NULL) {
		LOG_ERR("Cannot get USB CDC 1 Device");
		return;
	}

	if (callbacks_configure(device_get_binding(PORT0), PIN0, PIN0_FLAGS,
				&btn0, &callback[0])) {
		LOG_ERR("Failed configuring button 0 callback.");
		return;
	}

#ifdef DT_ALIAS_SW1_GPIOS_PIN
	if (callbacks_configure(device_get_binding(PORT1), PIN1, PIN1_FLAGS,
				&btn1, &callback[1])) {
		LOG_ERR("Failed configuring button 1 callback.");
		return;
	}
#endif

#ifdef DT_ALIAS_SW2_GPIOS_PIN
	if (callbacks_configure(device_get_binding(PORT2), PIN2, PIN2_FLAGS,
				&btn2, &callback[2])) {
		LOG_ERR("Failed configuring button 2 callback.");
		return;
	}
#endif

#ifdef DT_ALIAS_SW3_GPIOS_PIN
	if (callbacks_configure(device_get_binding(PORT3), PIN3, PIN3_FLAGS,
				&btn3, &callback[3])) {
		LOG_ERR("Failed configuring button 3 callback.");
		return;
	}
#endif

	/* Initialize HID */

	usb_hid_register_device(hid0_dev, hid_mouse_report_desc,
				sizeof(hid_mouse_report_desc), &ops);

	usb_hid_register_device(hid1_dev, hid_kbd_report_desc,
				sizeof(hid_kbd_report_desc), &ops);
	usb_hid_init(hid0_dev);
	usb_hid_init(hid1_dev);

	/* Initialize CDC ACM */

	LOG_INF("Wait for DTR on CDC ACM 0");
	while (1) {
		uart_line_ctrl_get(cdc0_dev, LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}
	}
	LOG_INF("DTR on CDC ACM 0 set");

	LOG_INF("Wait for DTR on CDC ACM 1");
	while (1) {
		uart_line_ctrl_get(cdc1_dev, LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}
	}
	LOG_INF("DTR on CDC ACM 1 set");

	/* Wait 1 sec for the host to do all settings */
	k_busy_wait(K_SECONDS(1) * USEC_PER_MSEC);

	uart_irq_callback_set(cdc0_dev, cdc_mouse_int_handler);
	uart_irq_callback_set(cdc1_dev, cdc_kbd_int_handler);

	write_data(cdc0_dev, banner0, strlen(banner0));
	write_data(cdc1_dev, banner1, strlen(banner1));

	uart_irq_rx_enable(cdc0_dev);
	uart_irq_rx_enable(cdc1_dev);

	while (true) {
		k_sem_take(&evt_sem, K_FOREVER);

		while ((ev = app_evt_get()) != NULL) {
			switch (ev->event_type) {
			case GPIO_BUTTON_0:
			{
				/* Move the mouse in random direction */
				u8_t rep[] = {0x00, sys_rand32_get(),
					      sys_rand32_get(), 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc0_dev, gpio0, strlen(gpio0));
				clear_mouse_report();
				break;
			}
			case GPIO_BUTTON_1:
			{
				/* Press left mouse button */
				u8_t rep[] = {0x00, 0x00, 0x00, 0x00};

				rep[MOUSE_BTN_REPORT_POS] |= MOUSE_BTN_LEFT;
				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc0_dev, gpio1, strlen(gpio1));
				clear_mouse_report();
				break;
			}
			case GPIO_BUTTON_2:
			{
				/* Send string on HID keyboard */
				write_data(cdc1_dev, gpio2, strlen(gpio2));
				if (strlen(string) > 0) {
					struct app_evt_t *ev = app_evt_alloc();

					ev->event_type = HID_KBD_STRING,
					app_evt_put(ev);
					str_pointer = 0U;
					k_sem_give(&evt_sem);
				}
				break;
			}
			case GPIO_BUTTON_3:
			{
				/* Toggle CAPS LOCK */
				u8_t rep[] = {0x00, 0x00, 0x00, 0x00,
					      0x00, 0x00, 0x00,
					      HID_KEY_CAPSLOCK};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid1_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc1_dev, gpio3, strlen(gpio3));
				clear_kbd_report();
				break;
			}
			case CDC_UP:
			{
				/* Mouse up */
				u8_t rep[] = {0x00, 0x00, 0xE0, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc0_dev, up, strlen(up));
				clear_mouse_report();
				break;
			}
			case CDC_DOWN:
			{
				/* Mouse down */
				u8_t rep[] = {0x00, 0x00, 0x20, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc0_dev, down, strlen(down));
				clear_mouse_report();
				break;
			}
			case CDC_RIGHT:
			{
				/* Mouse right */
				u8_t rep[] = {0x00, 0x20, 0x00, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc0_dev, right, strlen(right));
				clear_mouse_report();
				break;
			}
			case CDC_LEFT:
			{
				/* Mouse left */
				u8_t rep[] = {0x00, 0xE0, 0x00, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				write_data(cdc0_dev, left, strlen(left));
				clear_mouse_report();
				break;
			}
			case CDC_UNKNOWN:
			{
				write_data(cdc0_dev, unknown, strlen(unknown));
				write_data(cdc1_dev, unknown, strlen(unknown));
				break;
			}
			case CDC_STRING:
			{
				write_data(cdc0_dev, set_str, strlen(set_str));
				write_data(cdc0_dev, string, strlen(string));
				write_data(cdc0_dev, endl, strlen(endl));

				write_data(cdc1_dev, set_str, strlen(set_str));
				write_data(cdc1_dev, string, strlen(string));
				write_data(cdc1_dev, endl, strlen(endl));
				break;
			}
			case HID_MOUSE_CLEAR:
			{
				/* Clear mouse report */
				u8_t rep[] = {0x00, 0x00, 0x00, 0x00};

				k_sem_take(&usb_sem, K_FOREVER);
				hid_int_ep_write(hid0_dev, rep,
						 sizeof(rep), NULL);
				break;
			}
			case HID_KBD_CLEAR:
			{
				/* Clear kbd report */
				u8_t rep[] = {0x00, 0x00, 0x00, 0x00,
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
					u8_t rep[] = {0x00, 0x00, 0x00, 0x00,
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
					struct app_evt_t *ev = app_evt_alloc();

					ev->event_type = HID_KBD_STRING,
					app_evt_put(ev);
					k_sem_give(&evt_sem);
				} else if (strlen(string) == str_pointer) {
					clear_kbd_report();
				}

				break;
			}
			default:
			{
				LOG_ERR("Unknown event to execute");
				write_data(cdc0_dev, evt_fail,
					   strlen(evt_fail));
				write_data(cdc1_dev, evt_fail,
					   strlen(evt_fail));
				break;
			}
			break;
			}
			app_evt_free(ev);
		}
	}
}

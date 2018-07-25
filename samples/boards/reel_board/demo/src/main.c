/*
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <sensor.h>
#include <board.h>
#include <display/cfb.h>
#include <misc/printk.h>
#include <misc/util.h>

#include <stdio.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#define EDGECON_TEST_ENABLED	0

K_SEM_DEFINE(test_sem, 0, 1);
struct k_delayed_work led_timer;

enum periph_device {
	DEV_IDX_LED0 = 0,
	DEV_IDX_LED1,
	DEV_IDX_LED2,
	DEV_IDX_LED3,
	DEV_IDX_BUTTON,
	DEV_IDX_HDC1010,
	DEV_IDX_MMA8652,
	DEV_IDX_APDS9960,
	DEV_IDX_EPD,
	DEV_IDX_NUMOF,
};

struct periph_device_info {
	struct device *dev;
	char *name;
};

/* change device names if you want to use different sensors */
static struct periph_device_info dev_info[] = {
	{NULL, LED0_GPIO_CONTROLLER},
	{NULL, LED1_GPIO_CONTROLLER},
	{NULL, LED2_GPIO_CONTROLLER},
	{NULL, LED3_GPIO_CONTROLLER},
	{NULL, SW0_GPIO_CONTROLLER},
	{NULL, CONFIG_HDC1008_NAME},
	{NULL, CONFIG_FXOS8700_NAME},
	{NULL, CONFIG_APDS9960_DRV_NAME},
	{NULL, CONFIG_SSD1673_DEV_NAME},
};

static char str_buf[256];
static u8_t font_height;

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/*
 * Set Advertisement data. Based on the Eddystone specification:
 * https://github.com/google/eddystone/blob/master/protocol-specification.md
 * https://github.com/google/eddystone/tree/master/eddystone-url
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
		      0xaa, 0xfe, /* Eddystone UUID */
		      0x10, /* Eddystone-URL frame type */
		      0x00, /* Calibrated Tx power at 0m */
		      0x00, /* URL Scheme Prefix http://www. */
		      'z', 'e', 'p', 'h', 'y', 'r',
		      'p', 'r', 'o', 'j', 'e', 'c', 't',
		      0x08) /* .org */
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Beacon started\n");
}

static void button_pressed(struct device *gpiob, struct gpio_callback *cb,
			   u32_t pins)
{
	printk("Button pressed at %d\n", k_cycle_get_32());
	k_sem_give(&test_sem);
}

static struct gpio_callback gpio_cb;

#define EXPCON_TPIN0		28 /* P0 */
#define EXPCON_TPIN1		4  /* P0 */
#define EXPCON_TPIN2		31 /* P0 */
#define EXPCON_TPIN3		30 /* P0 */
#define EXPCON_TPIN4		5  /* P0 */
#define EXPCON_TPIN5		5  /* P1 */
#define EXPCON_TPIN6		4  /* P1 */
#define EXPCON_TPIN7		3  /* P1 */
#define EXPCON_TPIN8		9  /* P0 */
#define EXPCON_TPIN9		10 /* P0 */
#define EXPCON_TPIN10		3  /* P0 */
#define EXPCON_TPIN11		6  /* P1 */
#define EXPCON_TPIN12		7  /* P1 */
#define EXPCON_TPIN13		8  /* P1 */
#define EXPCON_TPIN14		10 /* P1 */
#define EXPCON_TPIN15		11 /* P1 */
#define EXPCON_TPIN16		12 /* P1 */

#define EXPCON_TPIN19		26 /* P0 I2C */
#define EXPCON_TPIN20		27 /* P0 I2C */

#if EDGECON_TEST_ENABLED
static int test_exp_con(void)
{
	struct device *dev_port0;
	struct device *dev_port1;
	u32_t tmp1;
	u32_t tmp2;

	dev_port0 = device_get_binding("GPIO_0");
	if (dev_port0 == NULL) {
		printk("Failed to get GPIO_0 device\n");
		return -1;
	}

	dev_port1 = device_get_binding("GPIO_1");
	if (dev_port1 == NULL) {
		printk("Failed to get GPIO_1 device\n");
		return -1;
	}

	gpio_pin_configure(dev_port0, EXPCON_TPIN0, GPIO_DIR_OUT);
	gpio_pin_configure(dev_port0, EXPCON_TPIN1, GPIO_DIR_IN);

	gpio_pin_configure(dev_port0, EXPCON_TPIN2, GPIO_DIR_OUT);
	gpio_pin_configure(dev_port0, EXPCON_TPIN3, GPIO_DIR_IN);

	gpio_pin_configure(dev_port0, EXPCON_TPIN4, GPIO_DIR_OUT);
	gpio_pin_configure(dev_port1, EXPCON_TPIN5, GPIO_DIR_IN);

	gpio_pin_configure(dev_port1, EXPCON_TPIN6, GPIO_DIR_OUT);
	gpio_pin_configure(dev_port1, EXPCON_TPIN7, GPIO_DIR_IN);

	gpio_pin_configure(dev_port0, EXPCON_TPIN10, GPIO_DIR_OUT);
	gpio_pin_configure(dev_port1, EXPCON_TPIN11, GPIO_DIR_IN);

	gpio_pin_configure(dev_port1, EXPCON_TPIN12, GPIO_DIR_OUT);
	gpio_pin_configure(dev_port1, EXPCON_TPIN13, GPIO_DIR_IN);

	gpio_pin_configure(dev_port1, EXPCON_TPIN14, GPIO_DIR_OUT);
	gpio_pin_configure(dev_port1, EXPCON_TPIN15, GPIO_DIR_IN);
	gpio_pin_configure(dev_port1, EXPCON_TPIN16, GPIO_DIR_IN);

	for (int i = 0; i < 100; i++) {
		gpio_pin_write(dev_port0, EXPCON_TPIN0, 1);
		gpio_pin_read(dev_port0, EXPCON_TPIN1, &tmp1);
		gpio_pin_write(dev_port0, EXPCON_TPIN0, 0);
		gpio_pin_read(dev_port0, EXPCON_TPIN1, &tmp2);
		if (tmp1 != 1 || tmp2 != 0) {
			printk("Failed test GPIO EXCON_TPIN0\n");
			return -1;
		}

		gpio_pin_write(dev_port0, EXPCON_TPIN2, 1);
		gpio_pin_read(dev_port0, EXPCON_TPIN3, &tmp1);
		gpio_pin_write(dev_port0, EXPCON_TPIN2, 0);
		gpio_pin_read(dev_port0, EXPCON_TPIN3, &tmp2);
		if (tmp1 != 1 || tmp2 != 0) {
			printk("Failed test GPIO EXCON_TPIN2\n");
			return -1;
		}

		gpio_pin_write(dev_port0, EXPCON_TPIN4, 1);
		gpio_pin_read(dev_port1, EXPCON_TPIN5, &tmp1);
		gpio_pin_write(dev_port0, EXPCON_TPIN4, 0);
		gpio_pin_read(dev_port1, EXPCON_TPIN5, &tmp2);
		if (tmp1 != 1 || tmp2 != 0) {
			printk("Failed test GPIO EXCON_TPIN4\n");
			return -1;
		}

		gpio_pin_write(dev_port1, EXPCON_TPIN6, 1);
		gpio_pin_read(dev_port1, EXPCON_TPIN7, &tmp1);
		gpio_pin_write(dev_port1, EXPCON_TPIN6, 0);
		gpio_pin_read(dev_port1, EXPCON_TPIN7, &tmp2);
		if (tmp1 != 1 || tmp2 != 0) {
			printk("Failed test GPIO EXCON_TPIN6\n");
			return -1;
		}

		gpio_pin_write(dev_port0, EXPCON_TPIN10, 1);
		gpio_pin_read(dev_port1, EXPCON_TPIN11, &tmp1);
		gpio_pin_write(dev_port0, EXPCON_TPIN10, 0);
		gpio_pin_read(dev_port1, EXPCON_TPIN11, &tmp2);
		if (tmp1 != 1 || tmp2 != 0) {
			printk("Failed test GPIO EXCON_TPIN10\n");
			return -1;
		}

		gpio_pin_write(dev_port1, EXPCON_TPIN12, 1);
		gpio_pin_read(dev_port1, EXPCON_TPIN13, &tmp1);
		gpio_pin_write(dev_port1, EXPCON_TPIN12, 0);
		gpio_pin_read(dev_port1, EXPCON_TPIN13, &tmp2);
		if (tmp1 != 1 || tmp2 != 0) {
			printk("Failed test GPIO EXCON_TPIN12\n");
			return -1;
		}

		gpio_pin_write(dev_port1, EXPCON_TPIN14, 1);
		gpio_pin_read(dev_port1, EXPCON_TPIN15, &tmp1);
		gpio_pin_write(dev_port1, EXPCON_TPIN14, 0);
		gpio_pin_read(dev_port1, EXPCON_TPIN15, &tmp2);
		if (tmp1 != 1 || tmp2 != 0) {
			printk("Failed test GPIO EXCON_TPIN14\n");
			return -1;
		}
	}

	return 0;
}
#endif

static int get_hdc1010_val(struct sensor_value *val)
{
	if (sensor_sample_fetch(dev_info[DEV_IDX_HDC1010].dev)) {
		printk("Failed to fetch sample for device %s\n",
		       dev_info[DEV_IDX_HDC1010].name);
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_HDC1010].dev,
			       SENSOR_CHAN_AMBIENT_TEMP,
			       &val[0])) {
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_HDC1010].dev,
			       SENSOR_CHAN_HUMIDITY,
			       &val[1])) {
		return -1;
	}

	printk("Temperature: %d.%d C | RH: %d%%\n",
		val[0].val1, val[0].val2/100000, val[0].val1);

	snprintf(str_buf, sizeof(str_buf),
		 "Temperature:%d.%d C\n", val[0].val1, val[0].val2/100000);
	if (cfb_print(dev_info[DEV_IDX_EPD].dev, str_buf, 0, 0)) {
		return -1;
	}

	snprintf(str_buf, sizeof(str_buf), "Humidity:%d%%\n", val[1].val1);
	if (cfb_print(dev_info[DEV_IDX_EPD].dev, str_buf, 0, font_height * 1)) {
		return -1;
	}

	return 0;
}

static int get_mma8652_val(struct sensor_value *val)
{
	if (sensor_sample_fetch(dev_info[DEV_IDX_MMA8652].dev)) {
		printk("Failed to fetch sample for device %s\n",
		       dev_info[DEV_IDX_MMA8652].name);
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_MMA8652].dev,
			       SENSOR_CHAN_ACCEL_XYZ,
			       &val[0])) {
		return -1;
	}

	printf("AX=%10.6f AY=%10.6f AZ=%10.6f\n",
	       sensor_value_to_double(&val[0]),
	       sensor_value_to_double(&val[1]),
	       sensor_value_to_double(&val[2]));

	snprintf(str_buf, sizeof(str_buf), "AX :%10.3f\n",
		 sensor_value_to_double(&val[0]));
	if (cfb_print(dev_info[DEV_IDX_EPD].dev, str_buf, 0, font_height * 2)) {
		return -1;
	}

	snprintf(str_buf, sizeof(str_buf), "AY :%10.3f\n",
		 sensor_value_to_double(&val[1]));
	if (cfb_print(dev_info[DEV_IDX_EPD].dev, str_buf, 0, font_height * 3)) {
		return -1;
	}

	snprintf(str_buf, sizeof(str_buf), "AZ :%10.3f\n",
		 sensor_value_to_double(&val[2]));
	if (cfb_print(dev_info[DEV_IDX_EPD].dev, str_buf, 0, font_height * 4)) {
		return -1;
	}


	return 0;
}

static int get_apds9960_val(struct sensor_value *val)
{
	if (sensor_sample_fetch(dev_info[DEV_IDX_APDS9960].dev)) {
		printk("Failed to fetch sample for device %s\n",
		       dev_info[DEV_IDX_APDS9960].name);
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_APDS9960].dev,
			       SENSOR_CHAN_LIGHT,
			       &val[0])) {
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_APDS9960].dev,
			       SENSOR_CHAN_PROX,
			       &val[1])) {
		return -1;
	}

	printk("ambient light intensity: %d, proximity %d\n",
	       val[0].val1, val[1].val1);

	snprintf(str_buf, sizeof(str_buf),
		 "Light :%d, Proximity:%d\n", val[0].val1, val[1].val1);
	if (cfb_print(dev_info[DEV_IDX_EPD].dev, str_buf, 0, font_height * 5)) {
		return -1;
	}

	return 0;
}

static void led_timeout(struct k_work *work)
{
	static int led_cntr;
	u32_t led_interval = K_MSEC(100);

	gpio_pin_write(dev_info[DEV_IDX_LED0].dev, LED0_GPIO_PIN, 1);
	gpio_pin_write(dev_info[DEV_IDX_LED1].dev, LED1_GPIO_PIN, 1);
	gpio_pin_write(dev_info[DEV_IDX_LED2].dev, LED2_GPIO_PIN, 1);
	gpio_pin_write(dev_info[DEV_IDX_LED3].dev, LED3_GPIO_PIN, 1);
	if (led_cntr == 0) {
		gpio_pin_write(dev_info[DEV_IDX_LED0].dev,
			       LED0_GPIO_PIN, 0);
		led_cntr += 1;
	} else if (led_cntr == 1) {
		gpio_pin_write(dev_info[DEV_IDX_LED1].dev,
			       LED1_GPIO_PIN, 0);
		led_cntr += 1;
	} else if (led_cntr == 2) {
		gpio_pin_write(dev_info[DEV_IDX_LED2].dev,
			       LED2_GPIO_PIN, 0);
		led_cntr += 1;
	} else if (led_cntr == 3) {
		gpio_pin_write(dev_info[DEV_IDX_LED3].dev,
			       LED3_GPIO_PIN, 0);
		led_cntr += 1;
	} else if (led_cntr == 4) {
		led_cntr = 0;
		led_interval = K_MSEC(5000);
	}

	k_delayed_work_submit(&led_timer, led_interval);
}

void main(void)
{
	struct sensor_value val[3];
	unsigned int i;
	int err;

	for (i = 0; i < ARRAY_SIZE(dev_info); i++) {
		dev_info[i].dev = device_get_binding(dev_info[i].name);
		if (dev_info[i].dev == NULL) {
			printk("Failed to get %s device\n", dev_info[i].name);
			return;
		}
	}

	gpio_pin_configure(dev_info[DEV_IDX_LED0].dev, LED0_GPIO_PIN,
			   GPIO_DIR_OUT);
	gpio_pin_write(dev_info[DEV_IDX_LED0].dev, LED0_GPIO_PIN, 1);

	gpio_pin_configure(dev_info[DEV_IDX_LED1].dev, LED1_GPIO_PIN,
			   GPIO_DIR_OUT);
	gpio_pin_write(dev_info[DEV_IDX_LED1].dev, LED1_GPIO_PIN, 1);

	gpio_pin_configure(dev_info[DEV_IDX_LED2].dev, LED2_GPIO_PIN,
			   GPIO_DIR_OUT);
	gpio_pin_write(dev_info[DEV_IDX_LED2].dev, LED2_GPIO_PIN, 1);

	gpio_pin_configure(dev_info[DEV_IDX_LED3].dev, LED3_GPIO_PIN,
			   GPIO_DIR_OUT);
	gpio_pin_write(dev_info[DEV_IDX_LED3].dev, LED3_GPIO_PIN, 1);

	gpio_pin_configure(dev_info[DEV_IDX_BUTTON].dev, SW0_GPIO_PIN,
			   GPIO_DIR_IN | GPIO_INT |  SW0_GPIO_FLAGS |
			   GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW);

	gpio_init_callback(&gpio_cb, button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(dev_info[DEV_IDX_BUTTON].dev, &gpio_cb);
	gpio_pin_enable_callback(dev_info[DEV_IDX_BUTTON].dev, SW0_GPIO_PIN);

	if (cfb_framebuffer_init(dev_info[DEV_IDX_EPD].dev)) {
		printk("Framebuffer initialization failedn");
		return;
	}

	cfb_framebuffer_set_font(dev_info[DEV_IDX_EPD].dev, 1);
	cfb_framebuffer_clear(dev_info[DEV_IDX_EPD].dev, true);
	cfb_framebuffer_finalize(dev_info[DEV_IDX_EPD].dev);

	printk("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
		cfb_get_display_parameter(dev_info[DEV_IDX_EPD].dev,
					  CFB_DISPLAY_WIDTH),
		cfb_get_display_parameter(dev_info[DEV_IDX_EPD].dev,
					  CFB_DISPLAY_HEIGH),
		cfb_get_display_parameter(dev_info[DEV_IDX_EPD].dev,
					  CFB_DISPLAY_PPT),
		cfb_get_display_parameter(dev_info[DEV_IDX_EPD].dev,
					  CFB_DISPLAY_ROWS),
		cfb_get_display_parameter(dev_info[DEV_IDX_EPD].dev,
					  CFB_DISPLAY_COLS));

	cfb_framebuffer_clear(dev_info[DEV_IDX_EPD].dev, false);

#if EDGECON_TEST_ENABLED
	if (test_exp_con()) {
		cfb_print(dev_info[DEV_IDX_EPD].dev, "Test failed!", 0, 0);
		cfb_framebuffer_finalize(dev_info[DEV_IDX_EPD].dev);
		goto _error_get;
	}
#endif

	if (cfb_print(dev_info[DEV_IDX_EPD].dev,
		      "Press a Button", 0, 0)) {
		goto _error_get;
	}

	cfb_framebuffer_finalize(dev_info[DEV_IDX_EPD].dev);
	cfb_get_font_size(dev_info[DEV_IDX_EPD].dev, 2, NULL, &font_height);
	cfb_framebuffer_set_font(dev_info[DEV_IDX_EPD].dev, 2);

	printk("Starting Beacon Demo\n");
	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	k_sem_take(&test_sem, K_FOREVER);
	k_delayed_work_init(&led_timer, led_timeout);
	k_delayed_work_submit(&led_timer, K_MSEC(100));

	while (1) {
		cfb_framebuffer_clear(dev_info[DEV_IDX_EPD].dev, false);
		if (get_hdc1010_val(val)) {
			goto _error_get;
		}
		if (get_mma8652_val(val)) {
			goto _error_get;
		}
		if (get_apds9960_val(val)) {
			goto _error_get;
		}
		cfb_framebuffer_finalize(dev_info[DEV_IDX_EPD].dev);

		k_sleep(K_MSEC(1000));
	}

_error_get:
	printk("Failed to get sensor data or print a string\n");
}

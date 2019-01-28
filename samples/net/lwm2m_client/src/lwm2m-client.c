/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_client_app
#define LOG_LEVEL LOG_LEVEL_DBG

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr.h>
#include <gpio.h>
#include <net/lwm2m.h>

#define APP_BANNER "Run LWM2M client"

#if !defined(CONFIG_NET_CONFIG_PEER_IPV4_ADDR)
#define CONFIG_NET_CONFIG_PEER_IPV4_ADDR ""
#endif

#if !defined(CONFIG_NET_CONFIG_PEER_IPV6_ADDR)
#define CONFIG_NET_CONFIG_PEER_IPV6_ADDR ""
#endif

#if defined(CONFIG_NET_IPV6)
#define SERVER_ADDR CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#elif defined(CONFIG_NET_IPV4)
#define SERVER_ADDR CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#else
#error LwM2M requires either IPV6 or IPV4 support
#endif


#define WAIT_TIME	K_SECONDS(10)
#define CONNECT_TIME	K_SECONDS(10)

#define CLIENT_MANUFACTURER	"Zephyr"
#define CLIENT_MODEL_NUMBER	"OMA-LWM2M Sample Client"
#define CLIENT_SERIAL_NUMBER	"345000123"
#define CLIENT_FIRMWARE_VER	"1.0"
#define CLIENT_DEVICE_TYPE	"OMA-LWM2M Client"
#define CLIENT_HW_VER		"1.0.1"

#define ENDPOINT_LEN		32

#ifndef LED0_GPIO_CONTROLLER
#ifdef LED0_GPIO_PORT
#define LED0_GPIO_CONTROLLER 	LED0_GPIO_PORT
#else
#define LED0_GPIO_CONTROLLER "(fail)"
#define LED0_GPIO_PIN 0
#endif
#endif

#define LED_GPIO_PORT LED0_GPIO_CONTROLLER
#define LED_GPIO_PIN LED0_GPIO_PIN

static int pwrsrc_bat;
static int pwrsrc_usb;
static int battery_voltage = 3800;
static int battery_current = 125;
static int usb_voltage = 5000;
static int usb_current = 900;

static struct device *led_dev;
static u32_t led_state;

static struct lwm2m_ctx client;

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#define TLS_TAG			1

/* "000102030405060708090a0b0c0d0e0f" */
static unsigned char client_psk[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

static const char client_psk_id[] = "Client_identity";
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

static struct k_sem quit_lock;

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
static u8_t firmware_buf[64];
#endif

/* TODO: Move to a pre write hook that can handle ret codes once available */
static int led_on_off_cb(u16_t obj_inst_id, u8_t *data, u16_t data_len,
			 bool last_block, size_t total_size)
{
	int ret = 0;
	u32_t led_val;

	led_val = *(u8_t *) data;
	if (led_val != led_state) {
		ret = gpio_pin_write(led_dev, LED_GPIO_PIN, led_val);
		if (ret) {
			/*
			 * We need an extra hook in LWM2M to better handle
			 * failures before writing the data value and not in
			 * post_write_cb, as there is not much that can be
			 * done here.
			 */
			LOG_ERR("Fail to write to GPIO %d", LED_GPIO_PIN);
			return ret;
		}

		led_state = led_val;
		/* TODO: Move to be set by an internal post write function */
		lwm2m_engine_set_s32("3311/0/5852", 0);
	}

	return ret;
}

static int init_led_device(void)
{
	int ret;

	led_dev = device_get_binding(LED_GPIO_PORT);
	if (!led_dev) {
		return -ENODEV;
	}

	ret = gpio_pin_configure(led_dev, LED_GPIO_PIN, GPIO_DIR_OUT);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_write(led_dev, LED_GPIO_PIN, 0);
	if (ret) {
		return ret;
	}

	return 0;
}

static int device_reboot_cb(u16_t obj_inst_id)
{
	LOG_INF("DEVICE: REBOOT");
	/* Add an error for testing */
	lwm2m_device_add_err(LWM2M_DEVICE_ERROR_LOW_POWER);
	/* Change the battery voltage for testing */
	lwm2m_device_set_pwrsrc_voltage_mv(pwrsrc_bat, --battery_voltage);

	return 0;
}

static int device_factory_default_cb(u16_t obj_inst_id)
{
	LOG_INF("DEVICE: FACTORY DEFAULT");
	/* Add an error for testing */
	lwm2m_device_add_err(LWM2M_DEVICE_ERROR_GPS_FAILURE);
	/* Change the USB current for testing */
	lwm2m_device_set_pwrsrc_current_ma(pwrsrc_usb, --usb_current);

	return 0;
}

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT)
static int firmware_update_cb(u16_t obj_inst_id)
{
	LOG_DBG("UPDATE");

	/* TODO: kick off update process */

	/* If success, set the update result as RESULT_SUCCESS.
	 * In reality, it should be set at function lwm2m_setup()
	 */
	lwm2m_engine_set_u8("5/0/3", STATE_IDLE);
	lwm2m_engine_set_u8("5/0/5", RESULT_SUCCESS);
	return 0;
}
#endif

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
static void *firmware_get_buf(u16_t obj_inst_id, size_t *data_len)
{
	*data_len = sizeof(firmware_buf);
	return firmware_buf;
}

static int firmware_block_received_cb(u16_t obj_inst_id,
				      u8_t *data, u16_t data_len,
				      bool last_block, size_t total_size)
{
	LOG_INF("FIRMWARE: BLOCK RECEIVED: len:%u last_block:%d",
		data_len, last_block);
	return 0;
}
#endif

static int lwm2m_setup(void)
{
	struct float32_value float_value;
	int ret;
	char *server_url;
	u16_t server_url_len;
	u8_t server_url_flags;

	/* setup SECURITY object */

	/* Server URL */
	ret = lwm2m_engine_get_res_data("0/0/0",
					(void **)&server_url, &server_url_len,
					&server_url_flags);
	if (ret < 0) {
		return ret;
	}

	snprintk(server_url, server_url_len, "coap%s//%s%s%s",
		 IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) ? "s:" : ":",
		 strchr(SERVER_ADDR, ':') ? "[" : "", SERVER_ADDR,
		 strchr(SERVER_ADDR, ':') ? "]" : "");

	/* Security Mode */
	lwm2m_engine_set_u8("0/0/2",
			    IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) ? 0 : 3);
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	lwm2m_engine_set_string("0/0/3", (char *)client_psk_id);
	lwm2m_engine_set_opaque("0/0/5",
				(void *)client_psk, sizeof(client_psk));
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

	/* setup SERVER object */

	/* setup DEVICE object */

	lwm2m_engine_set_res_data("3/0/0", CLIENT_MANUFACTURER,
				  sizeof(CLIENT_MANUFACTURER),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/1", CLIENT_MODEL_NUMBER,
				  sizeof(CLIENT_MODEL_NUMBER),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/2", CLIENT_SERIAL_NUMBER,
				  sizeof(CLIENT_SERIAL_NUMBER),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/3", CLIENT_FIRMWARE_VER,
				  sizeof(CLIENT_FIRMWARE_VER),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_register_exec_callback("3/0/4", device_reboot_cb);
	lwm2m_engine_register_exec_callback("3/0/5", device_factory_default_cb);
	lwm2m_engine_set_u8("3/0/9", 95); /* battery level */
	lwm2m_engine_set_u32("3/0/10", 15); /* mem free */
	lwm2m_engine_set_res_data("3/0/17", CLIENT_DEVICE_TYPE,
				  sizeof(CLIENT_DEVICE_TYPE),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/18", CLIENT_HW_VER,
				  sizeof(CLIENT_HW_VER),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_u8("3/0/20", LWM2M_DEVICE_BATTERY_STATUS_CHARGING);
	lwm2m_engine_set_u32("3/0/21", 25); /* mem total */

	pwrsrc_bat = lwm2m_device_add_pwrsrc(LWM2M_DEVICE_PWR_SRC_TYPE_BAT_INT);
	if (pwrsrc_bat < 0) {
		LOG_ERR("LWM2M battery power source enable error (err:%d)",
			pwrsrc_bat);
		return pwrsrc_bat;
	}
	lwm2m_device_set_pwrsrc_voltage_mv(pwrsrc_bat, battery_voltage);
	lwm2m_device_set_pwrsrc_current_ma(pwrsrc_bat, battery_current);

	pwrsrc_usb = lwm2m_device_add_pwrsrc(LWM2M_DEVICE_PWR_SRC_TYPE_USB);
	if (pwrsrc_usb < 0) {
		LOG_ERR("LWM2M usb power source enable error (err:%d)",
			pwrsrc_usb);
		return pwrsrc_usb;
	}
	lwm2m_device_set_pwrsrc_voltage_mv(pwrsrc_usb, usb_voltage);
	lwm2m_device_set_pwrsrc_current_ma(pwrsrc_usb, usb_current);

	/* setup FIRMWARE object */

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
	/* setup data buffer for block-wise transfer */
	lwm2m_engine_register_pre_write_callback("5/0/0", firmware_get_buf);
	lwm2m_firmware_set_write_cb(firmware_block_received_cb);
#endif
#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT)
	lwm2m_firmware_set_update_cb(firmware_update_cb);
#endif

	/* setup TEMP SENSOR object */

	lwm2m_engine_create_obj_inst("3303/0");
	/* dummy temp data in C*/
	float_value.val1 = 25;
	float_value.val2 = 0;
	lwm2m_engine_set_float32("3303/0/5700", &float_value);

	/* IPSO: Light Control object */
	if (init_led_device() == 0) {
		lwm2m_engine_create_obj_inst("3311/0");
		lwm2m_engine_register_post_write_callback("3311/0/5850",
				led_on_off_cb);
	}

	return 0;
}

static void rd_client_event(struct lwm2m_ctx *client,
			    enum lwm2m_rd_client_event client_event)
{
	switch (client_event) {

	case LWM2M_RD_CLIENT_EVENT_NONE:
		/* do nothing */
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE:
		LOG_DBG("Bootstrap registration failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE:
		LOG_DBG("Bootstrap registration complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE:
		LOG_DBG("Bootstrap transfer complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
		LOG_DBG("Registration failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
		LOG_DBG("Registration complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE:
		LOG_DBG("Registration update failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
		LOG_DBG("Registration update complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		LOG_DBG("Deregister failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		LOG_DBG("Disconnected");
		break;

	}
}

void main(void)
{
	int ret;

	LOG_INF(APP_BANNER);

	k_sem_init(&quit_lock, 0, UINT_MAX);

	ret = lwm2m_setup();
	if (ret < 0) {
		LOG_ERR("Cannot setup LWM2M fields (%d)", ret);
		return;
	}

	(void)memset(&client, 0x0, sizeof(client));
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	client.tls_tag = TLS_TAG;
#endif

	/* client.sec_obj_inst is 0 as a starting point */
	lwm2m_rd_client_start(&client, CONFIG_BOARD, rd_client_event);
	k_sem_take(&quit_lock, K_FOREVER);
}

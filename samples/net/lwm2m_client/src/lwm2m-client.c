/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 Open Source Foundries Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "lwm2m-client"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <board.h>
#include <zephyr.h>
#include <gpio.h>
#include <net/lwm2m.h>

#define APP_BANNER "Run LWM2M client"

#if !defined(CONFIG_NET_APP_PEER_IPV4_ADDR)
#define CONFIG_NET_APP_PEER_IPV4_ADDR ""
#endif

#if !defined(CONFIG_NET_APP_PEER_IPV6_ADDR)
#define CONFIG_NET_APP_PEER_IPV6_ADDR ""
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

#if defined(LED0_GPIO_PORT)
#define LED_GPIO_PORT	LED0_GPIO_PORT
#define LED_GPIO_PIN	LED0_GPIO_PIN
#else
#define LED_GPIO_PORT	"(fail)"
#define LED_GPIO_PIN	0
#endif

static int pwrsrc_bat;
static int pwrsrc_usb;
static int battery_voltage = 3800;
static int battery_current = 125;
static int usb_voltage = 5000;
static int usb_current = 900;

static struct device *led_dev;
static u32_t led_state;

static struct lwm2m_ctx client;

#if defined(CONFIG_NET_APP_DTLS)
#if !defined(CONFIG_NET_APP_TLS_STACK_SIZE)
#define CONFIG_NET_APP_TLS_STACK_SIZE		30000
#endif /* CONFIG_NET_APP_TLS_STACK_SIZE */

#define HOSTNAME "localhost"   /* for cert verification if that is enabled */

/* The result buf size is set to large enough so that we can receive max size
 * buf back. Note that mbedtls needs also be configured to have equal size
 * value for its buffer size. See MBEDTLS_SSL_MAX_CONTENT_LEN option in DTLS
 * config file.
 */
#define RESULT_BUF_SIZE 1500

NET_APP_TLS_POOL_DEFINE(dtls_pool, 10);

/* "000102030405060708090a0b0c0d0e0f" */
static unsigned char client_psk[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

static const char client_psk_id[] = "Client_identity";

static u8_t dtls_result[RESULT_BUF_SIZE];
NET_STACK_DEFINE(NET_APP_DTLS, net_app_dtls_stack,
		 CONFIG_NET_APP_TLS_STACK_SIZE, CONFIG_NET_APP_TLS_STACK_SIZE);
#endif /* CONFIG_NET_APP_DTLS */

static struct k_sem quit_lock;

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
static u8_t firmware_buf[64];
#endif

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
NET_PKT_TX_SLAB_DEFINE(lwm2m_tx_udp, 5);
NET_PKT_DATA_POOL_DEFINE(lwm2m_data_udp, 20);

static struct k_mem_slab *tx_udp_slab(void)
{
	return &lwm2m_tx_udp;
}

static struct net_buf_pool *data_udp_pool(void)
{
	return &lwm2m_data_udp;
}
#else
#define tx_udp_slab NULL
#define data_udp_pool NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

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
			SYS_LOG_ERR("Fail to write to GPIO %d", LED_GPIO_PIN);
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
	SYS_LOG_INF("DEVICE: REBOOT");
	/* Add an error for testing */
	lwm2m_device_add_err(LWM2M_DEVICE_ERROR_LOW_POWER);
	/* Change the battery voltage for testing */
	lwm2m_device_set_pwrsrc_voltage_mv(pwrsrc_bat, --battery_voltage);

	return 0;
}

static int device_factory_default_cb(u16_t obj_inst_id)
{
	SYS_LOG_INF("DEVICE: FACTORY DEFAULT");
	/* Add an error for testing */
	lwm2m_device_add_err(LWM2M_DEVICE_ERROR_GPS_FAILURE);
	/* Change the USB current for testing */
	lwm2m_device_set_pwrsrc_current_ma(pwrsrc_usb, --usb_current);

	return 0;
}

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT)
static int firmware_update_cb(u16_t obj_inst_id)
{
	SYS_LOG_DBG("UPDATE");

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
	SYS_LOG_INF("FIRMWARE: BLOCK RECEIVED: len:%u last_block:%d",
		    data_len, last_block);
	return 0;
}
#endif

static int lwm2m_setup(void)
{
	struct float32_value float_value;

	/* setup SECURITY object */
	/* setup SERVER object */

	/* setup DEVICE object */

	lwm2m_engine_set_string("3/0/0", CLIENT_MANUFACTURER);
	lwm2m_engine_set_string("3/0/1", CLIENT_MODEL_NUMBER);
	lwm2m_engine_set_string("3/0/2", CLIENT_SERIAL_NUMBER);
	lwm2m_engine_set_string("3/0/3", CLIENT_FIRMWARE_VER);
	lwm2m_engine_register_exec_callback("3/0/4", device_reboot_cb);
	lwm2m_engine_register_exec_callback("3/0/5", device_factory_default_cb);
	lwm2m_engine_set_u8("3/0/9", 95); /* battery level */
	lwm2m_engine_set_u32("3/0/10", 15); /* mem free */
	lwm2m_engine_set_string("3/0/17", CLIENT_DEVICE_TYPE);
	lwm2m_engine_set_string("3/0/18", CLIENT_HW_VER);
	lwm2m_engine_set_u8("3/0/20", LWM2M_DEVICE_BATTERY_STATUS_CHARGING);
	lwm2m_engine_set_u32("3/0/21", 25); /* mem total */

	pwrsrc_bat = lwm2m_device_add_pwrsrc(LWM2M_DEVICE_PWR_SRC_TYPE_BAT_INT);
	if (pwrsrc_bat < 0) {
		SYS_LOG_ERR("LWM2M battery power source enable error (err:%d)",
			pwrsrc_bat);
		return pwrsrc_bat;
	}
	lwm2m_device_set_pwrsrc_voltage_mv(pwrsrc_bat, battery_voltage);
	lwm2m_device_set_pwrsrc_current_ma(pwrsrc_bat, battery_current);

	pwrsrc_usb = lwm2m_device_add_pwrsrc(LWM2M_DEVICE_PWR_SRC_TYPE_USB);
	if (pwrsrc_usb < 0) {
		SYS_LOG_ERR("LWM2M usb power source enable error (err:%d)",
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

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_FAILURE:
		SYS_LOG_DBG("Bootstrap failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_COMPLETE:
		SYS_LOG_DBG("Bootstrap complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
		SYS_LOG_DBG("Registration failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
		SYS_LOG_DBG("Registration complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE:
		SYS_LOG_DBG("Registration update failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
		SYS_LOG_DBG("Registration update complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		SYS_LOG_DBG("Deregister failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		SYS_LOG_DBG("Disconnected");
		break;

	}
}

void main(void)
{
	int ret;

	SYS_LOG_INF(APP_BANNER);

	k_sem_init(&quit_lock, 0, UINT_MAX);

	ret = lwm2m_setup();
	if (ret < 0) {
		SYS_LOG_ERR("Cannot setup LWM2M fields (%d)", ret);
		return;
	}

	memset(&client, 0x0, sizeof(client));
	client.net_init_timeout = WAIT_TIME;
	client.net_timeout = CONNECT_TIME;
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	client.tx_slab = tx_udp_slab;
	client.data_pool = data_udp_pool;
#endif

#if defined(CONFIG_NET_APP_DTLS)
	client.client_psk = client_psk;
	client.client_psk_len = 16;
	client.client_psk_id = (char *)client_psk_id;
	client.client_psk_id_len = strlen(client_psk_id);
	client.cert_host = HOSTNAME;
	client.dtls_pool = &dtls_pool;
	client.dtls_result_buf = dtls_result;
	client.dtls_result_buf_len = RESULT_BUF_SIZE;
	client.dtls_stack = net_app_dtls_stack;
	client.dtls_stack_len = K_THREAD_STACK_SIZEOF(net_app_dtls_stack);
#endif /* CONFIG_NET_APP_DTLS */

#if defined(CONFIG_NET_IPV6)
	ret = lwm2m_rd_client_start(&client, CONFIG_NET_APP_PEER_IPV6_ADDR,
				    CONFIG_LWM2M_PEER_PORT, CONFIG_BOARD,
				    rd_client_event);
#elif defined(CONFIG_NET_IPV4)
	ret = lwm2m_rd_client_start(&client, CONFIG_NET_APP_PEER_IPV4_ADDR,
				    CONFIG_LWM2M_PEER_PORT, CONFIG_BOARD,
				    rd_client_event);
#else
	SYS_LOG_ERR("LwM2M client requires IPv4 or IPv6.");
	ret = -EPROTONOSUPPORT;
#endif
	if (ret < 0) {
		SYS_LOG_ERR("LWM2M init LWM2M RD client error (%d)",
			ret);
		return;
	}

	k_sem_take(&quit_lock, K_FOREVER);
}

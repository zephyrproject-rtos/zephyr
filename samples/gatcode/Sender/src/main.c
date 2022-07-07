#include <zephyr.h>
#include <sys/printk.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <drivers/sensor.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>


#include <logging/log.h>

LOG_MODULE_REGISTER(logger);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static struct bt_conn *default_conn;

/* UUIDs - https://www.guidgenerator.com */
#define BT_UUID_PACKET_SERVICE BT_UUID_128_ENCODE(0x595d0001, 0xc3b2, 0x40a2, 0xab93, 0x28e30fa26298)
#define BT_UUID_CHARACTERISTIC_A BT_UUID_128_ENCODE(0x2fe883b0, 0x4305, 0x42f8, 0x8efe, 0x51bafee55253)
#define BT_UUID_CHARACTERISTIC_B BT_UUID_128_ENCODE(0x7a482d03, 0x80cd, 0x4db4, 0xbfa5, 0x103910d89b61)

/* Advertisement Data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1)
};

/* Scan Response Data */
static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_PACKET_SERVICE)
};

static void connected(struct bt_conn *conn, uint8_t err) {
	if (err) {
		LOG_ERR("Connection failed (err 0x%02x)", err);
	} else {
		default_conn = bt_conn_ref(conn);
		gpio_pin_set_dt(&led, 1);
		LOG_INF("Connected.");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
	LOG_INF("Disconnected (reason 0x%02x)", reason);

	if (default_conn) {
		gpio_pin_set_dt(&led, 0);
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(int err) {
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}
}

/* Services */
void A_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value);
void B_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value);

BT_GATT_SERVICE_DEFINE(remote_srv,
BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(BT_UUID_PACKET_SERVICE)),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(BT_UUID_CHARACTERISTIC_A),
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(A_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(BT_UUID_CHARACTERISTIC_B),
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(B_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void A_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Start/Stop notifications %s", notif_enabled? "enabled":"disabled");
}

void B_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Audio notifications %s", notif_enabled? "enabled":"disabled");
}

void on_sent(struct bt_conn *conn, void *user_data)
{
    ARG_UNUSED(user_data);
    LOG_INF("Notification sent on connection %p", (void *)conn);
}

int send_start_stop_notification(struct bt_conn *conn, bool on)
{
    int err = 0;
    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr *attr = &remote_srv.attrs[2];

	int state = on == true ? 1 : 0;

    params.attr = attr;
    params.data = &state;
    params.len = 1;
    params.func = on_sent;

    err = bt_gatt_notify_cb(conn, &params);

    return err;
}

int send_audio_notification(struct bt_conn *conn, uint8_t size, uint8_t *packet)
{
    int err = 0;
    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr *attr = &remote_srv.attrs[2];

    params.attr = attr;
    params.data = packet;
    params.len = size;
    params.func = on_sent;

    err = bt_gatt_notify_cb(conn, &params);

    return err;
}

static uint8_t last = 0;

void generateAudioPacket(uint8_t size, uint8_t *packet)
{
	for(uint8_t i = 0; i < size; i++) {
		packet[i] = i + last;
	}
	last++;
}

void my_timer_handler(struct k_timer *dummy)
{
	uint8_t packetSize = 10;
	uint8_t audioPacket[10];
	generateAudioPacket(packetSize, (uint8_t*)&audioPacket);
	send_audio_notification(default_conn, packetSize, (uint8_t*)&audioPacket);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

// static void button_handler(uint32_t button_states, uint32_t has_changed)
// {
// 	if (button_states && has_changed & DK_BTN1_MSK) {
// 		LOG_WRN("Send Start Notification");
// 		send_start_stop_notification(default_conn, true);

// 		k_timer_start(&my_timer, K_MSEC(200), K_MSEC(200));
// 	}

// 	if (button_states && has_changed & DK_BTN2_MSK) {
// 		LOG_WRN("Send Stop Notification");
// 		send_start_stop_notification(default_conn, false);

// 		k_timer_stop(&my_timer);
// 	}

// 	if (button_states && has_changed & DK_BTN3_MSK) {
// 		LOG_WRN("Send Audio Packet");

// 		uint8_t packetSize = 10;
// 		uint8_t audioPacket[10];
// 		generateAudioPacket(packetSize, (uint8_t*)&audioPacket);
// 		send_audio_notification(default_conn, packetSize, (uint8_t*)&audioPacket);
// 	}
// }

void main(void)
{
	int err;

	/* Initialize the LED */
	if (!device_is_ready(led.port)) {
 		printk("Error setting LED\n");
 	}

 	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
 	if (err < 0) {
 		printk("Error setting LED (err %d)\n", err);
 	}

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
	}

	/* Add Bluetooth Callbacks */
	bt_conn_cb_register(&conn_callbacks);

	k_timer_start(&my_timer, K_MSEC(1000), K_MSEC(1000));

	while (1) {
		k_sleep(K_SECONDS(1));
	}
}
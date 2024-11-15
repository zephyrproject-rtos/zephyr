/** @file
 *  @brief Bluetooth Object Transfer Client Sample
 *
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

#define OBJ_MAX_SIZE			      1024
/* Hardcoded here since definition is in internal header */
#define BT_GATT_OTS_OLCP_RES_OPERATION_FAILED 0x04
#define BT_GATT_OTS_OLCP_RES_OUT_OF_BONDS     0x05

static struct bt_ots_client otc;
static struct bt_ots_client_cb otc_cb;
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params *oacp_sub_params;
static struct bt_gatt_subscribe_params *olcp_sub_params;
static unsigned char obj_data_buf[OBJ_MAX_SIZE];
static uint32_t last_checksum;

static bool first_selected;
static void on_obj_selected(struct bt_ots_client *ots_inst, struct bt_conn *conn, int err);

static void on_obj_metadata_read(struct bt_ots_client *ots_inst, struct bt_conn *conn, int err,
				 uint8_t metadata_read);

static int on_obj_data_read(struct bt_ots_client *ots_inst, struct bt_conn *conn, uint32_t offset,
			    uint32_t len, uint8_t *data_p, bool is_complete);

static void start_scan(void);
static struct bt_uuid_16 discover_uuid = BT_UUID_INIT_16(0);
static struct bt_conn *default_conn;
static atomic_t discovery_state;

enum OTS_SERVICE_DISCOVERY_STATE_BIT {
	DISC_OTS_FEATURE,
	DISC_OTS_NAME,
	DISC_OTS_TYPE,
	DISC_OTS_SIZE,
	DISC_OTS_ID,
	DISC_OTS_PROPERTIES,
	DISC_OTS_ACTION_CP,
	DISC_OTS_LIST_CP,
};

static void print_hex_number(const uint8_t *num, size_t len)
{
	printk("0x");
	for (size_t i = 0; i < len; i++) {
		printk("%02x ", num[i]);
	}

	printk("\n");
}

/*
 * Get buttons configuration from the devicetree sw0~sw3 alias. This is mandatory.
 */
#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
#define SW2_NODE DT_ALIAS(sw2)
#define SW3_NODE DT_ALIAS(sw3)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE) || !DT_NODE_HAS_STATUS_OKAY(SW1_NODE) ||                    \
	!DT_NODE_HAS_STATUS_OKAY(SW2_NODE) || !DT_NODE_HAS_STATUS_OKAY(SW3_NODE)
#error "Unsupported board: This sample need 4 buttons to run"
#endif

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET_OR(SW1_NODE, gpios, {0});
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET_OR(SW2_NODE, gpios, {0});
static const struct gpio_dt_spec button3 = GPIO_DT_SPEC_GET_OR(SW3_NODE, gpios, {0});
#define BTN_COUNT 4

static const struct gpio_dt_spec btns[BTN_COUNT] = {button0, button1, button2, button3};
static struct gpio_callback button_cb_data;
struct otc_btn_work_info {
	struct k_work_delayable work;
	uint32_t pins;
} otc_btn_work;

struct otc_checksum_work_info {
	struct k_work_delayable work;
	off_t offset;
	size_t len;
} otc_checksum_work;

static void otc_btn_work_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct otc_btn_work_info *btn_work = CONTAINER_OF(dwork, struct otc_btn_work_info, work);
	int err;
	size_t size_to_write;

	if (btn_work->pins == BIT(button0.pin)) {
		if (!first_selected) {
			err = bt_ots_client_select_id(&otc, default_conn, BT_OTS_OBJ_ID_MIN);
			first_selected = true;
		} else {
			printk("select next\n");
			err = bt_ots_client_select_next(&otc, default_conn);
		}

		if (err != 0) {
			printk("Failed to select object (err %d)\n", err);
		}

		printk("Selecting object succeeded\n");
	} else if (btn_work->pins == BIT(button1.pin)) {
		printk("read OTS object meta\n");
		err = bt_ots_client_read_object_metadata(&otc, default_conn,
							 BT_OTS_METADATA_REQ_ALL);
		if (err != 0) {
			printk("Failed to read object metadata (err %d)\n", err);
		}

	} else if (btn_work->pins == BIT(button2.pin)) {
		if (BT_OTS_OBJ_GET_PROP_WRITE(otc.cur_object.props)) {
			size_to_write = MIN(OBJ_MAX_SIZE, otc.cur_object.size.alloc);
			(void)memset(obj_data_buf, 0, size_to_write);
			printk("Going to write OTS object len %d\n", size_to_write);
			for (uint32_t idx = 0; idx < size_to_write; idx++) {
				obj_data_buf[idx] = UINT8_MAX - (idx % UINT8_MAX);
			}

			last_checksum = bt_ots_client_calc_checksum(obj_data_buf, size_to_write);
			printk("Data sent checksum 0x%08x\n", last_checksum);
			err = bt_ots_client_write_object_data(&otc, default_conn, obj_data_buf,
							      size_to_write, 0,
							      BT_OTS_OACP_WRITE_OP_MODE_NONE);
			if (err != 0) {
				printk("Failed to write object (err %d)\n", err);
			}
		} else {
			printk("This OBJ does not support WRITE OP\n");
		}

	} else if (btn_work->pins == BIT(button3.pin)) {
		if (BT_OTS_OBJ_GET_PROP_READ(otc.cur_object.props)) {
			printk("read OTS object\n");
			err = bt_ots_client_read_object_data(&otc, default_conn);
			if (err != 0) {
				printk("Failed to read object %d\n", err);
			}
		} else {
			printk("This OBJ does not support READ OP\n");
		}
	}
}

static void otc_checksum_work_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct otc_checksum_work_info *checksum_work =
		CONTAINER_OF(dwork, struct otc_checksum_work_info, work);
	int err;

	err = bt_ots_client_get_object_checksum(&otc, default_conn, checksum_work->offset,
						checksum_work->len);
	if (err != 0) {
		printk("bt_ots_client_get_object_checksum failed (%d)\n", err);
	}
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	otc_btn_work.pins = pins;
	k_work_schedule(&otc_btn_work.work, K_MSEC(100));
}

static void configure_button_irq(const struct gpio_dt_spec btn)
{
	int ret;

	if (!gpio_is_ready_dt(&btn)) {
		printk("Error: button device %s is not ready\n", btn.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&btn, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, btn.port->name, btn.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&btn, GPIO_INT_EDGE_TO_ACTIVE);

	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
		       btn.port->name, btn.pin);
		return;
	}

	button_cb_data.pin_mask |= BIT(btn.pin);
	gpio_add_callback(btn.port, &button_cb_data);

	printk("Set up button at %s pin %d\n", btn.port->name, btn.pin);
}

static void configure_buttons(void)
{
	gpio_init_callback(&button_cb_data, button_pressed, 0);

	for (int idx = 0; idx < BTN_COUNT; idx++) {
		configure_button_irq(btns[idx]);
	}
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int i;

	switch (data->type) {
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(uint16_t) != 0U) {
			printk("AD malformed\n");
			return true;
		}

		for (i = 0; i < data->data_len; i += sizeof(uint16_t)) {
			struct bt_le_conn_param *param;
			const struct bt_uuid *uuid;
			uint16_t u16;
			int err;

			(void)memcpy(&u16, &data->data[i], sizeof(u16));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
			if (bt_uuid_cmp(uuid, BT_UUID_OTS) != 0) {
				continue;
			}

			err = bt_le_scan_stop();
			if (err != 0) {
				printk("Stop LE scan failed (err %d)\n", err);
				continue;
			}

			param = BT_LE_CONN_PARAM_DEFAULT;
			err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
			if (err != 0) {
				printk("Create conn failed (err %d)\n", err);
				start_scan();
			}

			return false;
		}
	}
	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));

	/* We're only interested in connectable events and scan response
	 * because service UUID is in sd of sample peripheral_ots.
	 */
	if (type == BT_GAP_ADV_TYPE_ADV_IND || type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND ||
	    type == BT_GAP_ADV_TYPE_SCAN_RSP) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static void start_scan(void)
{
	int err;

	/* Use active scanning and disable duplicate filtering to handle any
	 * devices that might update their advertising data at runtime.
	 */
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found);
	if (err != 0) {
		printk("Scanning OTS TAG failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static int subscribe_func(void)
{
	int ret;

	printk("Subscribe OACP and OLCP Indication\n");
	oacp_sub_params = &otc.oacp_sub_params;
	oacp_sub_params->disc_params = &otc.oacp_sub_disc_params;
	if (oacp_sub_params) {
		oacp_sub_params->ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
		oacp_sub_params->end_handle = otc.end_handle;
		oacp_sub_params->value = BT_GATT_CCC_INDICATE;
		oacp_sub_params->value_handle = otc.oacp_handle;
		oacp_sub_params->notify = bt_ots_client_indicate_handler;
		ret = bt_gatt_subscribe(default_conn, oacp_sub_params);

		if (ret != 0) {
			printk("Subscribe OACP failed %d\n", ret);
			return ret;
		}
	}

	olcp_sub_params = &otc.olcp_sub_params;
	olcp_sub_params->disc_params = &otc.olcp_sub_disc_params;
	if (olcp_sub_params) {
		olcp_sub_params->ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
		olcp_sub_params->end_handle = otc.end_handle;
		olcp_sub_params->value = BT_GATT_CCC_INDICATE;
		olcp_sub_params->value_handle = otc.olcp_handle;
		olcp_sub_params->notify = bt_ots_client_indicate_handler;
		ret = bt_gatt_subscribe(default_conn, olcp_sub_params);

		if (ret != 0) {
			printk("Subscribe OLCP failed %d\n", ret);
			return ret;
		}
	}

	return ret;
}

static bool is_discovery_complete(void)
{
	return (atomic_test_bit(&discovery_state, DISC_OTS_FEATURE) &&
		atomic_test_bit(&discovery_state, DISC_OTS_NAME) &&
		atomic_test_bit(&discovery_state, DISC_OTS_TYPE) &&
		atomic_test_bit(&discovery_state, DISC_OTS_SIZE) &&
		atomic_test_bit(&discovery_state, DISC_OTS_ID) &&
		atomic_test_bit(&discovery_state, DISC_OTS_PROPERTIES) &&
		atomic_test_bit(&discovery_state, DISC_OTS_ACTION_CP) &&
		atomic_test_bit(&discovery_state, DISC_OTS_LIST_CP));
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params, int err)
{
	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (bt_uuid_cmp(discover_params.uuid, BT_UUID_OTS) == 0) {
		(void)memcpy(&discover_uuid, BT_UUID_OTS_FEATURE, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		err = bt_gatt_discover(conn, &discover_params);

		if (err != 0) {
			printk("Discover failed (err %d)\n", err);
		}

	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_OTS_FEATURE) == 0) {
		atomic_set_bit(&discovery_state, DISC_OTS_FEATURE);
		otc.feature_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_OTS_NAME, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			printk("Discover failed (err %d)\n", err);
		}

	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_OTS_NAME) == 0) {
		atomic_set_bit(&discovery_state, DISC_OTS_NAME);
		otc.obj_name_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_OTS_TYPE, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			printk("Discover failed (err %d)\n", err);
		}

	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_OTS_TYPE) == 0) {
		atomic_set_bit(&discovery_state, DISC_OTS_TYPE);
		otc.obj_type_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_OTS_SIZE, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			printk("Discover failed (err %d)\n", err);
		}

	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_OTS_SIZE) == 0) {
		atomic_set_bit(&discovery_state, DISC_OTS_SIZE);
		otc.obj_size_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_OTS_ID, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			printk("Discover failed (err %d)\n", err);
		}

	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_OTS_ID) == 0) {
		atomic_set_bit(&discovery_state, DISC_OTS_ID);
		otc.obj_id_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_OTS_PROPERTIES, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			printk("Discover failed (err %d)\n", err);
		}

	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_OTS_PROPERTIES) == 0) {
		atomic_set_bit(&discovery_state, DISC_OTS_PROPERTIES);
		otc.obj_properties_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_OTS_ACTION_CP, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_OTS_ACTION_CP) == 0) {
		atomic_set_bit(&discovery_state, DISC_OTS_ACTION_CP);
		otc.oacp_handle = bt_gatt_attr_value_handle(attr);
		(void)memcpy(&discover_uuid, BT_UUID_OTS_LIST_CP, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err != 0) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_OTS_LIST_CP) == 0) {
		atomic_set_bit(&discovery_state, DISC_OTS_LIST_CP);
		otc.olcp_handle = bt_gatt_attr_value_handle(attr);
	} else {
		return BT_GATT_ITER_STOP;
	}

	if (is_discovery_complete()) {
		printk("Discovery complete for OTS Client\n");
		err = subscribe_func();

		if (err != 0) {
			return BT_GATT_ITER_STOP;
		}

		/* Read feature of OTS server*/
		err = bt_ots_client_read_feature(&otc, default_conn);
		if (err != 0) {
			printk("bt_ots_client_read_feature failed (err %d)", err);
		}
	}

	return BT_GATT_ITER_STOP;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	first_selected = false;
	if (err != 0) {
		printk("Failed to connect to %s %u %s\n", addr, err, bt_hci_err_to_str(err));

		bt_conn_unref(default_conn);
		default_conn = NULL;
		start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n", addr);

	if (conn == default_conn) {
		(void)memcpy(&discover_uuid, BT_UUID_OTS, sizeof(discover_uuid));
		discover_params.uuid = &discover_uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

		err = bt_gatt_discover(default_conn, &discover_params);
		if (err != 0) {
			printk("Discover failed(err %d)\n", err);
			return;
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	bt_conn_unref(default_conn);
	default_conn = NULL;
	discovery_state = ATOMIC_INIT(0);
	start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void on_obj_selected(struct bt_ots_client *ots_inst, struct bt_conn *conn, int err)
{
	printk("Current object selected cb OLCP result (%d)\n", err);

	if (err == BT_GATT_OTS_OLCP_RES_OPERATION_FAILED) {
		printk("BT_GATT_OTS_OLCP_RES_OPERATION_FAILED %d\n", err);
		first_selected = false;
	} else if (err == BT_GATT_OTS_OLCP_RES_OUT_OF_BONDS) {
		printk("BT_GATT_OTS_OLCP_RES_OUT_OF_BONDS %d. Select first valid instead\n", err);
		(void)bt_ots_client_select_id(&otc, default_conn, BT_OTS_OBJ_ID_MIN);
	}

	(void)memset(obj_data_buf, 0, OBJ_MAX_SIZE);
}

static int on_obj_data_read(struct bt_ots_client *ots_inst, struct bt_conn *conn, uint32_t offset,
			    uint32_t len, uint8_t *data_p, bool is_complete)
{
	printk("Received OTS Object content, %i bytes at offset %i\n", len, offset);

	print_hex_number(data_p, len);

	if ((offset + len) > OBJ_MAX_SIZE) {
		printk("Can not fit whole object, drop the rest of data\n");
	} else {
		(void)memcpy((obj_data_buf + offset), data_p, MIN((OBJ_MAX_SIZE - offset), len));
	}

	if (is_complete) {
		printk("Object total received %d\n", len + offset);
		print_hex_number(obj_data_buf, len + offset);
		(void)memset(obj_data_buf, 0, OBJ_MAX_SIZE);
		otc_checksum_work.offset = 0;
		otc_checksum_work.len = otc.cur_object.size.cur;
		k_work_schedule(&otc_checksum_work.work, K_NO_WAIT);
		return BT_OTS_STOP;
	}

	return BT_OTS_CONTINUE;
}

static void on_obj_metadata_read(struct bt_ots_client *ots_inst, struct bt_conn *conn, int err,
				 uint8_t metadata_read)
{
	printk("Object's meta data:\n");
	printk("\tCurrent size\t:%u", ots_inst->cur_object.size.cur);
	printk("\tAlloc size\t:%u\n", ots_inst->cur_object.size.alloc);

	if (ots_inst->cur_object.size.cur > OBJ_MAX_SIZE) {
		printk("Object larger than allocated buffer\n");
	}

	bt_ots_metadata_display(&ots_inst->cur_object, 1);
}
static void on_obj_data_written(struct bt_ots_client *ots_inst, struct bt_conn *conn, size_t len)
{
	int err;

	printk("Object been written %d\n", len);
	/* Update object size after write done*/
	err = bt_ots_client_read_object_metadata(&otc, default_conn,
						 BT_OTS_METADATA_REQ_ALL);
	if (err != 0) {
		printk("Failed to read object metadata (err %d)\n", err);
	}
}

void on_obj_checksum_calculated(struct bt_ots_client *ots_inst,
				struct bt_conn *conn, int err, uint32_t checksum)
{
	printk("Object Calculate checksum OACP result (%d)\nChecksum 0x%08x last sent 0x%08x %s\n",
	       err, checksum, last_checksum, (checksum == last_checksum) ? "match" : "not match");
}

static void bt_otc_init(void)
{
	otc_cb.obj_data_read = on_obj_data_read;
	otc_cb.obj_selected = on_obj_selected;
	otc_cb.obj_metadata_read = on_obj_metadata_read;
	otc_cb.obj_data_written = on_obj_data_written;
	otc_cb.obj_checksum_calculated = on_obj_checksum_calculated;
	otc.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	otc.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	printk("Current object selected callback: %p\n", otc_cb.obj_selected);
	printk("Content callback: %p\n", otc_cb.obj_data_read);
	printk("Metadata callback: %p\n", otc_cb.obj_metadata_read);
	otc.cb = &otc_cb;
	bt_ots_client_register(&otc);
}

int main(void)
{
	int err;

	first_selected = false;
	discovery_state = ATOMIC_INIT(0);
	k_work_init_delayable(&otc_btn_work.work, otc_btn_work_fn);
	k_work_init_delayable(&otc_checksum_work.work, otc_checksum_work_fn);

	configure_buttons();
	err = bt_enable(NULL);

	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_otc_init();
	printk("Bluetooth OTS client sample running\n");

	start_scan();
	return 0;
}

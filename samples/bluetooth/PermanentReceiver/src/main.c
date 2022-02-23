/*
	prj.conf file content:
		CONFIG_BT=y
		CONFIG_BT_OBSERVER=y
		CONFIG_BT_EXT_ADV=y
		CONFIG_BT_DEBUG_LOG=y
		CONFIG_BT_DEVICE_NAME="Extended Receiver"
		CONFIG_BT_BUF_EVT_RX_COUNT=180
*/

#include <bluetooth/bluetooth.h>

static uint64_t last_packet_id = 0;
static uint64_t failed_messages = 0;
static uint64_t received_messages = 0;

struct adv_payload {
	uint8_t id[8];
};

static bool data_cb(struct bt_data *data, void *user_data)
{
	switch (data->type) {
	case BT_DATA_MANUFACTURER_DATA:
		memcpy(user_data, data->data, sizeof(struct adv_payload));
		return false;
	default:
		return true;
	}
}

void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		    struct net_buf_simple *buf)
{
	if(adv_type == BT_GAP_ADV_TYPE_EXT_ADV) {
		struct adv_payload payload[sizeof(struct adv_payload)];
		bt_data_parse(buf, data_cb, payload);

		uint64_t sender_packet_id =
		(uint64_t)payload->id[0] << 0 * 8 |
		(uint64_t)payload->id[1] << 1 * 8 |
		(uint64_t)payload->id[2] << 2 * 8 |
		(uint64_t)payload->id[3] << 3 * 8 |
		(uint64_t)payload->id[4] << 4 * 8 |
		(uint64_t)payload->id[5] << 5 * 8 |
		(uint64_t)payload->id[6] << 6 * 8 |
		((uint64_t)payload->id[7] << 7 * 8);

		if(sender_packet_id != last_packet_id + 1) {
			failed_messages++;
		} else {
			received_messages++;	
		}

		printk("id: %llu - received: %lli - failed: %lli\n", sender_packet_id, received_messages, failed_messages);

		last_packet_id = sender_packet_id;
	}
}

void main(void)
{
	int err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	struct bt_le_scan_param param = {
        .type = BT_LE_SCAN_TYPE_PASSIVE,
        .options = BT_LE_SCAN_OPT_CODED | BT_LE_SCAN_OPT_NO_1M | BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval = 0x0030  /* 30 ms */,
        .window = 0x0030 /* 30ms */,
		.timeout = 0,
		.interval_coded = 0,
        .window_coded = 0,
    };

	err = bt_le_scan_start(&param, &scan_cb);
	if (err) {
		printk("Bluetooth scan start failed (err %d)\n", err);
		return;
	}
}
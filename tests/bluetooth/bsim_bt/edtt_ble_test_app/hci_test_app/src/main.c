/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief HCI interface application
 */

#include <zephyr.h>

#include <settings/settings.h>

#include <sys/byteorder.h>
#include <debug/stack.h>

#include <net/buf.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci_vs.h>
#include <bluetooth/hci_raw.h>

#include "edtt_driver.h"
#include "bs_tracing.h"
#include "commands.h"

#if IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_LEVEL LOG_LEVEL_DBG
#else
#define LOG_LEVEL CONFIG_BT_LOG_LEVEL
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(hci_test_app);

static uint16_t waiting_opcode;
static enum commands_t waiting_response;
static uint8_t m_events;

/**
 * @brief Clean out excess bytes from the input buffer
 */
static void read_excess_bytes(uint16_t size)
{
	if (size > 0) {
		uint8_t buffer[size];

		edtt_read((uint8_t *)buffer, size, EDTTT_BLOCK);
		LOG_ERR("command size wrong! (%u extra bytes removed)", size);
	}
}

/**
 * @brief Provide an error response when an HCI command send failed
 */
static void error_response(int error)
{
	uint16_t response = sys_cpu_to_le16(waiting_response);
	int   le_error = sys_cpu_to_le32(error);
	uint16_t size = sys_cpu_to_le16(sizeof(le_error));

	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
	edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
	edtt_write((uint8_t *)&le_error, sizeof(le_error), EDTTT_BLOCK);
	waiting_response = CMD_NOTHING;
	waiting_opcode = 0;
}

/**
 * @brief Allocate buffer for HCI command and fill in opCode for the command
 */
static struct net_buf *hci_cmd_create(uint16_t opcode, uint8_t param_len)
{
	struct bt_hci_cmd_hdr *hdr;
	struct net_buf *buf;

	buf = bt_buf_get_tx(BT_BUF_CMD, K_FOREVER, NULL, 0);
	__ASSERT_NO_MSG(buf);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->opcode = sys_cpu_to_le16(opcode);
	hdr->param_len = param_len;

	return buf;
}

/**
 * @brief Allocate buffer for ACL Data Package and fill in Header
 */
static struct net_buf *acl_data_create(struct bt_hci_acl_hdr *le_hdr)
{
	struct net_buf *buf;
	struct bt_hci_acl_hdr *hdr;

	buf = bt_buf_get_tx(BT_BUF_ACL_OUT, K_FOREVER, NULL, 0);
	__ASSERT_NO_MSG(buf);

	hdr = net_buf_add(buf, sizeof(*hdr));
	*hdr = *le_hdr;

	return buf;
}

/**
 * @brief Allocate buffer for HCI command, fill in parameters and send the
 * command...
 */
static int send_hci_command(uint16_t opcode, uint8_t param_len, uint16_t response)
{
	struct net_buf *buf;
	void *cp;
	int err = 0;

	waiting_response = response;
	buf = hci_cmd_create(waiting_opcode = opcode, param_len);
	if (buf) {
		if (param_len) {
			cp = net_buf_add(buf, param_len);
			edtt_read((uint8_t *)cp, param_len, EDTTT_BLOCK);
		}
		err = bt_send(buf);
		if (err) {
			LOG_ERR("Failed to send HCI command %d (err %d)",
				opcode, err);
			error_response(err);
		}
	} else {
		LOG_ERR("Failed to create buffer for HCI command 0x%04x",
			opcode);
		error_response(-1);
	}
	return err;
}

/**
 * @brief Echo function - echo input received...
 */
static void echo(uint16_t size)
{
	uint16_t response = sys_cpu_to_le16(CMD_ECHO_RSP);
	uint16_t le_size = sys_cpu_to_le16(size);

	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
	edtt_write((uint8_t *)&le_size, sizeof(le_size), EDTTT_BLOCK);

	if (size > 0) {
		uint8_t buff[size];

		edtt_read(buff, size, EDTTT_BLOCK);
		edtt_write(buff, size, EDTTT_BLOCK);
	}
}

NET_BUF_POOL_FIXED_DEFINE(event_pool, 32, BT_BUF_RX_SIZE + 4, NULL);
static K_FIFO_DEFINE(event_queue);
static K_FIFO_DEFINE(rx_queue);
NET_BUF_POOL_FIXED_DEFINE(data_pool, CONFIG_BT_CTLR_RX_BUFFERS + 14,
			  BT_BUF_ACL_SIZE + 4, NULL);
static K_FIFO_DEFINE(data_queue);

/**
 * @brief Handle Command Complete HCI event...
 */
static void command_complete(struct net_buf *buf)
{
	struct bt_hci_evt_cmd_complete *evt = (void *)buf->data;
	uint16_t opcode = sys_le16_to_cpu(evt->opcode);
	uint16_t response = sys_cpu_to_le16(waiting_response);
	struct net_buf_simple_state state;
	uint16_t size;

	net_buf_simple_save(&buf->b, &state);

	net_buf_pull(buf, sizeof(*evt));
	size = sys_cpu_to_le16(buf->len);

	if (opcode == waiting_opcode) {
		LOG_DBG("Command complete for 0x%04x", waiting_opcode);

		edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
		edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
		edtt_write((uint8_t *)buf->data, buf->len, EDTTT_BLOCK);
		waiting_opcode = 0;
	} else {
		LOG_WRN("Not waiting for 0x(%04x) command status,"
			" expected 0x(%04x)", opcode, waiting_opcode);
	}

	net_buf_simple_restore(&buf->b, &state);
}

/**
 * @brief Handle Command Status HCI event...
 */
static void command_status(struct net_buf *buf)
{
	struct bt_hci_evt_cmd_status *evt = (void *)buf->data;
	uint16_t opcode = sys_le16_to_cpu(evt->opcode);
	uint16_t response = sys_cpu_to_le16(waiting_response);
	struct net_buf_simple_state state;
	uint8_t status = evt->status;
	uint16_t size;

	net_buf_simple_save(&buf->b, &state);

	net_buf_pull(buf, sizeof(*evt));
	size = sys_cpu_to_le16(buf->len) + 1;

	if (opcode == waiting_opcode) {
		LOG_DBG("Command status for 0x%04x", waiting_opcode);

		edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
		edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
		edtt_write((uint8_t *)&status, sizeof(status), EDTTT_BLOCK);
		edtt_write((uint8_t *)buf->data, buf->len, EDTTT_BLOCK);
		waiting_opcode = 0;
	} else {
		LOG_WRN("Not waiting for 0x(%04x) command status,"
			" expected 0x(%04x)", opcode, waiting_opcode);
	}

	net_buf_simple_restore(&buf->b, &state);
}

/**
 * @brief Remove an event from the event queue
 */
static void discard_event(void)
{
	struct net_buf *buf = net_buf_get(&event_queue, K_FOREVER);

	net_buf_unref(buf);
	m_events--;
}

/**
 * @brief Allocate and store an event in the event queue
 */
static struct net_buf *queue_event(struct net_buf *buf)
{
	struct net_buf *evt;

	evt = net_buf_alloc(&event_pool, K_NO_WAIT);
	if (evt) {
		bt_buf_set_type(evt, BT_BUF_EVT);
		net_buf_add_le32(evt, sys_cpu_to_le32(k_uptime_get()));
		net_buf_add_mem(evt, buf->data, buf->len);
		net_buf_put(&event_queue, evt);
		m_events++;
	}
	return evt;
}

/**
 * @brief Thread to service events and ACL data packets from the HCI input queue
 */
static void service_events(void *p1, void *p2, void *p3)
{
	struct net_buf *buf, *evt;

	while (1) {
		buf = net_buf_get(&rx_queue, K_FOREVER);
		if (bt_buf_get_type(buf) == BT_BUF_EVT) {

			evt = queue_event(buf);
			if (!evt) {
				bs_trace_raw_time(4,
						  "Failed to allocated buffer "
						  "for event!\n");
				LOG_WRN("No event in queue");
			}

			struct bt_hci_evt_hdr *hdr = (void *)buf->data;

			net_buf_pull(buf, sizeof(*hdr));

			switch (hdr->evt) {
			case BT_HCI_EVT_CMD_COMPLETE:
				if (!evt) {
					discard_event();
					evt = queue_event(buf);
				}
				command_complete(buf);
				break;
			case BT_HCI_EVT_CMD_STATUS:
				if (!evt) {
					discard_event();
					evt = queue_event(buf);
				}
				command_status(buf);
				break;
			default:
				break;
			}
		} else if (bt_buf_get_type(buf) == BT_BUF_ACL_IN) {
			struct net_buf *data;

			data = net_buf_alloc(&data_pool, K_NO_WAIT);
			if (data) {
				bt_buf_set_type(data, BT_BUF_ACL_IN);
				net_buf_add_le32(data,
					sys_cpu_to_le32(k_uptime_get()));
				net_buf_add_mem(data, buf->data, buf->len);
				net_buf_put(&data_queue, data);
			}
		}
		net_buf_unref(buf);

		k_yield();
	}
}

/**
 * @brief Flush all HCI events from the input-copy queue
 */
static void flush_events(uint16_t size)
{
	uint16_t  response = sys_cpu_to_le16(CMD_FLUSH_EVENTS_RSP);
	struct net_buf *buf;

	while ((buf = net_buf_get(&event_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
		m_events--;
	}
	read_excess_bytes(size);
	size = 0;

	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
	edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
}

/**
 * @brief Get next available HCI event from the input-copy queue
 */
static void get_event(uint16_t size)
{
	uint16_t  response = sys_cpu_to_le16(CMD_GET_EVENT_RSP);
	struct net_buf *buf;

	read_excess_bytes(size);
	size = 0;

	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
	buf = net_buf_get(&event_queue, K_FOREVER);
	if (buf) {
		size = sys_cpu_to_le16(buf->len);
		edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
		edtt_write((uint8_t *)buf->data, buf->len, EDTTT_BLOCK);
		net_buf_unref(buf);
		m_events--;
	} else {
		edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
	}
}

/**
 * @brief Get next available HCI events from the input-copy queue
 */
static void get_events(uint16_t size)
{
	uint16_t response = sys_cpu_to_le16(CMD_GET_EVENT_RSP);
	struct net_buf *buf;
	uint8_t count = m_events;

	read_excess_bytes(size);
	size = 0;

	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
	edtt_write((uint8_t *)&count, sizeof(count), EDTTT_BLOCK);
	while (count--) {
		buf = net_buf_get(&event_queue, K_FOREVER);
		size = sys_cpu_to_le16(buf->len);
		edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
		edtt_write((uint8_t *)buf->data, buf->len, EDTTT_BLOCK);
		net_buf_unref(buf);
		m_events--;
	}
}

/**
 * @brief Check whether an HCI event is available in the input-copy queue
 */
static void has_event(uint16_t size)
{
	struct has_event_resp {
		uint16_t response;
		uint16_t size;
		uint8_t  count;
	} __packed;
	struct has_event_resp le_response = {
		.response = sys_cpu_to_le16(CMD_HAS_EVENT_RSP),
		.size = sys_cpu_to_le16(1),
		.count = m_events
	};

	if (size > 0) {
		read_excess_bytes(size);
	}
	edtt_write((uint8_t *)&le_response, sizeof(le_response), EDTTT_BLOCK);
}

/**
 * @brief Flush all ACL Data Packages from the input-copy queue
 */
static void le_flush_data(uint16_t size)
{
	uint16_t  response = sys_cpu_to_le16(CMD_LE_FLUSH_DATA_RSP);
	struct net_buf *buf;

	while ((buf = net_buf_get(&data_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}
	read_excess_bytes(size);
	size = 0;

	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
	edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
}

/**
 * @brief Check whether an ACL Data Package is available in the input-copy queue
 */
static void le_data_ready(uint16_t size)
{
	struct has_data_resp {
		uint16_t response;
		uint16_t size;
		uint8_t  empty;
	} __packed;
	struct has_data_resp le_response = {
		.response = sys_cpu_to_le16(CMD_LE_DATA_READY_RSP),
		.size = sys_cpu_to_le16(1),
		.empty = 0
	};

	if (size > 0) {
		read_excess_bytes(size);
	}
	if (k_fifo_is_empty(&data_queue)) {
		le_response.empty = 1;
	}
	edtt_write((uint8_t *)&le_response, sizeof(le_response), EDTTT_BLOCK);
}

/**
 * @brief Get next available HCI Data Package from the input-copy queue
 */
static void le_data_read(uint16_t size)
{
	uint16_t  response = sys_cpu_to_le16(CMD_LE_DATA_READ_RSP);
	struct net_buf *buf;

	read_excess_bytes(size);
	size = 0;

	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
	buf = net_buf_get(&data_queue, K_FOREVER);
	if (buf) {
		size = sys_cpu_to_le16(buf->len);
		edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
		edtt_write((uint8_t *)buf->data, buf->len, EDTTT_BLOCK);
		net_buf_unref(buf);
	} else {
		edtt_write((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
	}
}

/**
 * @brief Write ACL Data Package to the Controller...
 */
static void le_data_write(uint16_t size)
{
	struct data_write_resp {
		uint16_t code;
		uint16_t size;
		uint8_t  status;
	} __packed;
	struct data_write_resp response = {
		.code = sys_cpu_to_le16(CMD_LE_DATA_WRITE_RSP),
		.size = sys_cpu_to_le16(1),
		.status = 0
	};
	struct net_buf *buf;
	struct bt_hci_acl_hdr hdr;
	int err;

	if (size >= sizeof(hdr)) {
		edtt_read((uint8_t *)&hdr, sizeof(hdr), EDTTT_BLOCK);
		size -= sizeof(hdr);
		buf = acl_data_create(&hdr);
		if (buf) {
			uint16_t hdr_length = sys_le16_to_cpu(hdr.len);
			uint8_t *pdata = net_buf_add(buf, hdr_length);

			if (size >= hdr_length) {
				edtt_read(pdata, hdr_length, EDTTT_BLOCK);
				size -= hdr_length;
			}
			err = bt_send(buf);
			if (err) {
				LOG_ERR("Failed to send ACL Data (err %d)",
					err);
			}
		} else {
			err = -2; /* Failed to allocate data buffer */
			LOG_ERR("Failed to create buffer for ACL Data.");
		}
	} else {
		/* Size too small for header (handle and data length) */
		err = -3;
	}
	read_excess_bytes(size);

	response.status = sys_cpu_to_le32(err);
	edtt_write((uint8_t *)&response, sizeof(response), EDTTT_BLOCK);
}

static K_THREAD_STACK_DEFINE(service_events_stack,
			     CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread service_events_data;

/**
 * @brief Zephyr application main entry...
 */
void main(void)
{
	int err;
	uint16_t command;
	uint16_t size;
	uint16_t opcode;
	/**
	 * Initialize HCI command opcode and response variables...
	 */
	waiting_opcode = 0;
	waiting_response = CMD_NOTHING;
	m_events = 0;
	/**
	 * Initialize Bluetooth stack in raw mode...
	 */
	err = bt_enable_raw(&rx_queue);
	if (err) {
		LOG_ERR("Bluetooth initialization failed (err %d)", err);
		return;
	}
	/**
	 * Initialize and start EDTT system...
	 */
#if defined(CONFIG_ARCH_POSIX)
	enable_edtt_mode();
	set_edtt_autoshutdown(true);
#endif
	edtt_start();
	/**
	 * Initialize and start thread to service HCI events and ACL data...
	 */
	k_thread_create(&service_events_data, service_events_stack,
			K_THREAD_STACK_SIZEOF(service_events_stack),
			service_events, NULL, NULL, NULL, K_PRIO_COOP(7),
			0, K_NO_WAIT);

	while (1) {
		/**
		 * Wait for a command to arrive - then read and execute command
		 */
		edtt_read((uint8_t *)&command, sizeof(command), EDTTT_BLOCK);
		command = sys_le16_to_cpu(command);
		edtt_read((uint8_t *)&size, sizeof(size), EDTTT_BLOCK);
		size = sys_le16_to_cpu(size);
		bs_trace_raw_time(4, "command 0x%04X received (size %u) "
				  "events=%u\n",
				  command, size, m_events);

		switch (command) {
		case CMD_ECHO_REQ:
			echo(size);
			break;
		case CMD_FLUSH_EVENTS_REQ:
			flush_events(size);
			break;
		case CMD_HAS_EVENT_REQ:
			has_event(size);
			break;
		case CMD_GET_EVENT_REQ:
		{
			uint8_t multiple;

			edtt_read((uint8_t *)&multiple, sizeof(multiple),
				  EDTTT_BLOCK);
			if (multiple)
				get_events(--size);
			else
				get_event(--size);
		}
		break;
		case CMD_LE_FLUSH_DATA_REQ:
			le_flush_data(size);
			break;
		case CMD_LE_DATA_READY_REQ:
			le_data_ready(size);
			break;
		case CMD_LE_DATA_WRITE_REQ:
			le_data_write(size);
			break;
		case CMD_LE_DATA_READ_REQ:
			le_data_read(size);
			break;
		default:
			if (size >= 2) {
				edtt_read((uint8_t *)&opcode, sizeof(opcode),
					  EDTTT_BLOCK);
				send_hci_command(sys_le16_to_cpu(opcode),
						 size - 2, command + 1);
			}
		}
	}
}

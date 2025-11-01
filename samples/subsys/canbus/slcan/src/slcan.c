/*
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <string.h>

// LOG_MODULE_REGISTER(slcan, CONFIG_LOG_DEFAULT_LEVEL);
LOG_MODULE_REGISTER(slcan, LOG_LEVEL_DBG);

#define CAN_NODE    DT_CHOSEN(zephyr_canbus)
#define SERIAL_NODE DT_NODELABEL(uart1)

#define CAN_TX_QUEUE_SIZE 3

/* message with extended ID, 8 bytes payload and timestamp (last 4 bytes) */
#define SLCAN_MTU (sizeof("Tiiiiiiiilddddddddddddddddssss"))

#define CANBUS_RX_THREAD_STACK_SIZE 3000
#define SERIAL_RX_THREAD_STACK_SIZE 3000
#define SLCAN_THREAD_PRIORITY       3

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(serial_rx_msgq, SLCAN_MTU, 10, 4);

CAN_MSGQ_DEFINE(can_rx_msgq, 3);

K_THREAD_STACK_DEFINE(canbus_thread_stack, CANBUS_RX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(serial_thread_stack, SERIAL_RX_THREAD_STACK_SIZE);

static struct k_thread canbus_thread_data;
static struct k_thread serial_thread_data;

static const struct device *const can_dev = DEVICE_DT_GET(CAN_NODE);
// static const struct device *const hci_uart_dev =
// 	DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_c2h_uart));
static const struct device *const serial_dev = DEVICE_DT_GET(SERIAL_NODE);

static struct k_sem serial_tx_sem;

/* receive buffer used in serial ISR callback */
static char rx_buf[SLCAN_MTU];
static int rx_buf_pos;

static struct {
	bool can_initiated;
	bool can_open;
} slcan;

/*
 * Read characters from UART until line end (only CR for SLCAN) is detected. Afterwards push the
 * data to the message queue.
 */
static void serial_rx_callback(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(dev)) {
		return;
	}

	while (uart_irq_rx_ready(dev)) {

		uart_fifo_read(dev, &c, 1);

		if (c == '\r' || c == '\n') {
			if (rx_buf_pos > 0) {
				/* terminate string */
				rx_buf[rx_buf_pos] = '\0';

				/* copy the message to the queue (or drop if queue is full) */
				k_msgq_put(&serial_rx_msgq, &rx_buf, K_NO_WAIT);
			}
			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

static void slcan_serial_print(const char *buf, size_t len)
{
	k_sem_take(&serial_tx_sem, K_FOREVER);

	for (int i = 0; i < len; i++) {
		uart_poll_out(serial_dev, buf[i]);
	}

	k_sem_give(&serial_tx_sem);
}

static void slcan_can_tx_cb(const struct device *dev, int error, void *user_data)
{
	if (error != 0) {
		LOG_ERR("Sending CAN frame failed: %d", error);
	}
}

/*
 * Hex characters have to be upper-case, so we can't use Zephyr's bin2hex().
 */
static void buf_append_hex(char *dest, const uint8_t *src, size_t src_bytes)
{
	static const char nibble2char[16] = "0123456789ABCDEF";

	for (int i = 0; i < src_bytes; i++) {
		dest[i * 2] = nibble2char[(src[i] >> 4) & 0xF];
		dest[i * 2 + 1] = nibble2char[src[i] & 0xF];
	}
}

static int slcan_parse_send_cmd(char *buf, size_t size, struct can_frame *frame)
{
	uint8_t can_id[4];
	bool rtr = (buf[0] == 'R' || buf[0] == 'r');

	memset(frame, 0, sizeof(*frame));

	frame->flags = rtr ? CAN_FRAME_RTR : 0;

	if (buf[0] == 'R' || buf[0] == 'T') {
		if (size < 10) {
			LOG_ERR("Invalid SLCAN send command");
			return -EINVAL;
		}

		hex2bin(&buf[1], 8, can_id, 4);
		frame->id = sys_get_be32(can_id);
		frame->flags |= CAN_FRAME_IDE;

		frame->dlc = buf[9] - '0';

		if (rtr) {
			return 0;
		}

		if (size < 10 + frame->dlc * 2 || frame->dlc > 8) {
			LOG_ERR("Invalid DLC or frame too short");
			return -EINVAL;
		}

		hex2bin(&buf[10], frame->dlc * 2, frame->data, frame->dlc);
	} else {
		if (size < 5) {
			LOG_ERR("Invalid SLCAN send command");
			return -EINVAL;
		}

		buf[0] = '0'; /* add leading 0 to 3-digit ID for proper conversion */
		hex2bin(&buf[0], 4, can_id, 2);
		frame->id = sys_get_be16(can_id);

		frame->dlc = buf[4] - '0';

		if (rtr) {
			return 0;
		}

		if (size < 5 + frame->dlc * 2 || frame->dlc > 8) {
			LOG_ERR("Invalid DLC or frame too short");
			return -EINVAL;
		}

		hex2bin(&buf[5], frame->dlc * 2, frame->data, frame->dlc);
	}

	return 0;
}

static int slcan_serialize(struct can_frame *frame, char *buf)
{
	uint8_t can_id[4];
	int pos = 0;

	/* Only supports extended and remote frames */
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0 || frame->dlc > 8) {
		LOG_ERR("Unsupported CAN frame type received");
		return -EINVAL;
	}

	sys_put_be32(frame->id, can_id);

	/* CAN ID */
	if ((frame->flags & CAN_FRAME_IDE) == 0) {
		buf_append_hex(buf, can_id + 2, 2);
		/* overwrite first digit for correct 3-digit standard ID encoding */
		buf[0] = (frame->flags & CAN_FRAME_RTR) ? 'r' : 't';
		pos = 1 + 3;
	} else {
		buf[0] = (frame->flags & CAN_FRAME_RTR) ? 'R' : 'T';
		buf_append_hex(buf + 1, can_id, 4);
		pos = 1 + 8;
	}

	/* Data length */
	buf[pos++] = '0' + frame->dlc;

	/* Data bytes */
	buf_append_hex(buf + pos, frame->data, frame->dlc);
	pos += frame->dlc * 2;

	/* Message end */
	buf[pos++] = '\r';

	return pos;
}

static int slcan_setup_cmd(char *buf, size_t size)
{
	int err = -EINVAL;

	if (size < 2) {
		LOG_ERR("Invalid SLCAN setup command");
		return err;
	}

	switch (buf[1]) {
	case '0':
		err = can_set_bitrate(can_dev, KHZ(10));
		break;
	case '1':
		err = can_set_bitrate(can_dev, KHZ(20));
		break;
	case '2':
		err = can_set_bitrate(can_dev, KHZ(50));
		break;
	case '3':
		err = can_set_bitrate(can_dev, KHZ(100));
		break;
	case '4':
		err = can_set_bitrate(can_dev, KHZ(125));
		break;
	case '5':
		err = can_set_bitrate(can_dev, KHZ(250));
		break;
	case '6':
		err = can_set_bitrate(can_dev, KHZ(500));
		break;
	case '7':
		err = can_set_bitrate(can_dev, KHZ(800));
		break;
	case '8':
		err = can_set_bitrate(can_dev, MHZ(1));
		break;
	default:
		LOG_ERR("Unsupported CAN bitrate");
		break;
	}

	if (err == 0) {
		slcan.can_initiated = true;
		LOG_INF("CAN initialized");
	} else {
		LOG_ERR("Failed to initialze CAN: %d", err);
	}

	return err;
}

static int slcan_serial_process(char *buf, size_t size)
{
	int err;
	struct can_frame frame = {0};

	LOG_HEXDUMP_DBG(buf, size, "Serial message");

	if (size == 0) {
		return -EINVAL;
	}

	switch (buf[0]) {
	case 'S':
		/* Setup with standard CAN bit-rates */
		return slcan_setup_cmd(buf, size);
	case 's':
		/* Setup with custom CAN bit-rates (not implemented) */
		return -ENOSYS;
	case 'O':
		/* Open the CAN channel */
		if (slcan.can_initiated && !slcan.can_open) {
			err = can_start(can_dev);
			if (err == 0) {
				slcan.can_open = true;
				LOG_INF("CAN started");
			} else {
				LOG_ERR("Failed to start CAN: %d", err);
			}

			return err;
		} else if (slcan.can_initiated) {
			LOG_INF("CAN already started");
			return 0;
		}

		LOG_ERR("CAN bitrate must be set before opening");
		return -ENETDOWN;
	case 'C':
		/* Close the CAN channel */
		if (!slcan.can_open) {
			LOG_INF("CAN already stopped");
			return 0;
		}

		err = can_stop(can_dev);
		if (err == 0) {
			slcan.can_open = false;
			LOG_INF("CAN stopped");
		} else {
			LOG_ERR("Failed to stop CAN: %d", err);
		}

		return err;
	/* Transmitting frames or RTRs to the bus */
	case 't':
	case 'T':
	case 'r':
	case 'R':
		if (!slcan.can_open) {
			LOG_WRN("CAN not open, cannot send");
			return -ENETDOWN;
		}

		err = slcan_parse_send_cmd(buf, size, &frame);
		if (err != 0) {
			return err;
		}

		LOG_DBG("Sending CAN message with ID 0x%X, RTR %d, %d bytes: %.2X %.2X %.2X %.2X "
			"%.2X %.2X %.2X %.2X",
			frame.id, (frame.flags & CAN_FRAME_RTR) != 0, frame.dlc, frame.data[0],
			frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5],
			frame.data[6], frame.data[7]);

		return can_send(can_dev, &frame, K_MSEC(10), slcan_can_tx_cb, NULL);
	case 'F':
		/* Read Status Flags */
		if (!slcan.can_open) {
			return -ENETDOWN;
		}

		uint8_t resp[4] = {'F'};
		uint8_t slcan_status = 0;
		enum can_state state;

		can_get_state(can_dev, &state, NULL);
		switch (state) {
		case CAN_STATE_ERROR_WARNING:
			slcan_status |= BIT(2);
			break;
		case CAN_STATE_ERROR_PASSIVE:
			slcan_status |= BIT(5);
			break;
		case CAN_STATE_BUS_OFF:
			slcan_status |= BIT(7);
			break;
		default:
			slcan_status = 0;
		}
		if (bin2hex(&slcan_status, 1, resp + 1, 2) > 0) {
			resp[3] = '\r';
			slcan_serial_print(resp, 4);
		}

		return 0;
	case 'M':
	case 'm':
		/*
		 * Hardware filtering not implemented because of flawed protocol: You can not
		 * specify the filtering mode (standard or extended frame) using the
		 * SJA1000-compatible register. Leaving filtering to the host side.
		 */
		return -ENOSYS;
	case 'V':
		/* Get Version number of both CANUSB hardware and software (dummy response) */
		slcan_serial_print("V0000\r", 6);
		return 0;
	case 'N':
		/* Get Serial number of the CANUSB (dummy response) */
		slcan_serial_print("N0000\r", 6);
		return 0;
	default:
		LOG_WRN("Unsupported SLCAN command: %c", buf[0]);
		return -ENOSYS;
	}
}

static int slcan_canbus_process(struct can_frame *frame)
{
	char buf[SLCAN_MTU];
	int pos;

	LOG_DBG("CAN message with ID 0x%X, RTR %d, %d bytes: %.2X %.2X %.2X %.2X %.2X %.2X %.2X "
		"%.2X",
		frame->id, (frame->flags & CAN_FRAME_RTR) != 0, frame->dlc, frame->data[0],
		frame->data[1], frame->data[2], frame->data[3], frame->data[4], frame->data[5],
		frame->data[6], frame->data[7]);

	pos = slcan_serialize(frame, buf);
	if (pos < 0) {
		return pos;
	}

	slcan_serial_print(buf, pos);

	return 0;
}

static void slcan_serial_thread(void *arg1, void *arg2, void *arg3)
{
	char buf[SLCAN_MTU];
	int err;

	uart_irq_callback_set(serial_dev, serial_rx_callback);
	uart_irq_rx_enable(serial_dev);

	while (k_msgq_get(&serial_rx_msgq, &buf, K_FOREVER) == 0) {
		err = slcan_serial_process(buf, strlen(buf));

		if (err == 0) {
			/* respond with CR for OK */
			slcan_serial_print("\r", 1);
		} else {
			/* respond with BELL for error */
			slcan_serial_print("\a", 1);
			LOG_ERR("Serial processing error: %d", err);
		}
	}
}

static void slcan_canbus_thread(void *arg1, void *arg2, void *arg3)
{
	struct can_filter filter = {0};
	struct can_frame rx_frame;

	can_add_rx_filter_msgq(can_dev, &can_rx_msgq, &filter);

	filter.flags = CAN_FILTER_IDE;
	can_add_rx_filter_msgq(can_dev, &can_rx_msgq, &filter);

	while (k_msgq_get(&can_rx_msgq, &rx_frame, K_FOREVER) == 0) {
		slcan_canbus_process(&rx_frame);
	}
}

static int slcan_init(void)
{
	k_sem_init(&serial_tx_sem, 1, 1);

	if (!device_is_ready(can_dev)) {
		LOG_ERR("CAN device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(serial_dev)) {
		LOG_ERR("Serial device not ready");
		return -ENODEV;
	}

	k_thread_create(&canbus_thread_data, canbus_thread_stack,
			K_THREAD_STACK_SIZEOF(canbus_thread_stack), slcan_canbus_thread, NULL, NULL,
			NULL, SLCAN_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&serial_thread_data, serial_thread_stack,
			K_THREAD_STACK_SIZEOF(serial_thread_stack), slcan_serial_thread, NULL, NULL,
			NULL, SLCAN_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(slcan_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

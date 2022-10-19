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
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/byteorder.h>

#include <string.h>

LOG_MODULE_REGISTER(slcan, CONFIG_LOG_DEFAULT_LEVEL);

#define CAN_NODE	DT_CHOSEN(zephyr_canbus)
#define SERIAL_NODE	DT_NODELABEL(cdc_acm_uart0)

#define CAN_TX_QUEUE_SIZE 3

/* message with extended ID, 8 bytes payload and timestamp (last 4 bytes) */
#define SLCAN_MTU (sizeof("Tiiiiiiiilddddddddddddddddssss"))

#define CANBUS_RX_THREAD_STACK_SIZE 3000
#define SERIAL_RX_THREAD_STACK_SIZE 3000
#define SLCAN_THREAD_PRIORITY 3

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(serial_rx_msgq, SLCAN_MTU, 10, 4);

CAN_MSGQ_DEFINE(can_rx_msgq, 3);

K_THREAD_STACK_DEFINE(canbus_thread_stack, CANBUS_RX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(serial_thread_stack, SERIAL_RX_THREAD_STACK_SIZE);

static struct k_thread canbus_thread_data;
static struct k_thread serial_thread_data;

static const struct device *const can_dev = DEVICE_DT_GET(CAN_NODE);
static const struct device *const serial_dev = DEVICE_DT_GET(SERIAL_NODE);

static struct k_sem serial_tx_sem;

/* receive buffer used in serial ISR callback */
static char rx_buf[SLCAN_MTU];
static int rx_buf_pos;

static struct {
	bool can_initiated;
	bool can_open;
	bool timestamp_on;
} slcan;

/*
 * Read characters from UART until line end (only CR for SLCAN) is detected. Afterwards push the
 * data to the message queue.
 */
void serial_rx_callback(const struct device *dev, void *user_data)
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

void slcan_serial_print(const char *buf, size_t len)
{
	k_sem_take(&serial_tx_sem, K_FOREVER);

	for (int i = 0; i < len; i++) {
		uart_poll_out(serial_dev, buf[i]);
	}

	k_sem_give(&serial_tx_sem);
}

void slcan_can_tx_cb(const struct device *dev, int error, void *user_data)
{
	if (error != 0) {
		LOG_ERR("Sending CAN frame failed: %d", error);
	}
}

int slcan_serial_process(uint8_t *buf, size_t size)
{
	int err = -EINVAL;

	LOG_HEXDUMP_DBG(buf, size, "Serial message");

	switch (buf[0]) {
	case 'S':
		/* Setup with standard CAN bit-rates */
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
		}
		if (err == 0) {
			slcan.can_initiated = true;
			LOG_INF("CAN initialized");
		} else {
			LOG_ERR("Failed to initialze CAN");
		}
		break;
	case 's':
		/* Setup with custom CAN bit-rates (not implemented) */
		err = -ENOSYS;
		break;
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
		} else {
			err = 0;
			LOG_INF("CAN already started");
		}
		break;
	case 'C':
		/* Close the CAN channel */
		if (slcan.can_open) {
			err = can_stop(can_dev);
			if (err == 0) {
				slcan.can_open = false;
				LOG_INF("CAN stopped");
			} else {
				LOG_ERR("Failed to stop CAN: %d", err);
			}
		} else {
			err = 0;
			LOG_INF("CAN already stopped");
		}
		break;
	case 't':
		/* Transmit a standard (11bit) CAN frame */
		if (slcan.can_open) {
			uint8_t can_id[2];

			buf[0] = '0'; /* add leading 0 to 3-digit ID for proper conversion */
			hex2bin(&buf[0], 4, can_id, 2);

			struct can_frame frame = {
				.id = sys_get_be16(can_id),
				.flags = 0,
			};

			frame.dlc = buf[4] - '0';
			hex2bin(&buf[5], frame.dlc * 2, frame.data, frame.dlc);

			err = can_send(can_dev, &frame, K_MSEC(10), slcan_can_tx_cb, NULL);
		} else {
			err = -ENETDOWN;
		}
		break;
	case 'T':
		/* Transmit an extended (29bit) CAN frame */
		if (slcan.can_open) {
			uint8_t can_id[4];

			hex2bin(&buf[1], 8, can_id, 4);

			struct can_frame frame = {
				.id = sys_get_be32(can_id),
				.flags = CAN_FRAME_IDE,
			};

			frame.dlc = buf[9] - '0';
			hex2bin(&buf[10], frame.dlc * 2, frame.data, frame.dlc);

			err = can_send(can_dev, &frame, K_MSEC(10), slcan_can_tx_cb, NULL);
		} else {
			err = -ENETDOWN;
		}
		break;
	case 'r':
		/* Transmit a standard RTR (11bit) CAN frame */
	case 'R':
		/* Transmit an extended RTR (29bit) CAN frame */
		err = -ENOSYS;
		break;
	case 'F':
		/* Read Status Flags */
		if (slcan.can_open) {
			uint8_t resp[4] = {'F'};
			uint8_t slcan_status;
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
		} else {
			err = -ENETDOWN;
		}
		break;
	case 'M':
	case 'm':
		/*
		 * Hardware filtering not implemented because of flawed protocol: You can not
		 * specify the filtering mode (standard or extended frame) using the
		 * SJA1000-compatible register. Leaving filtering to the host side.
		 */
		err = -ENOSYS;
		break;
	case 'V':
		/* Get Version number of both CANUSB hardware and software (dummy response) */
		slcan_serial_print("V0000\r", 6);
		err = 0;
		break;
	case 'N':
		/* Get Serial number of the CANUSB (dummy response) */
		slcan_serial_print("N0000\r", 6);
		err = 0;
		break;
	case 'Z':
		/* Sets Time Stamp ON/OFF for received frames only */
		if (!slcan.can_open) {
			if (buf[1] == '0') {
				slcan.timestamp_on = false;
				err = 0;
			} else if (buf[1] == '1') {
				slcan.timestamp_on = true;
				err = 0;
			}
		} else {
			err = -EBUSY;
		}
		break;
	}

	if (err == 0) {
		/* respond with CR for OK */
		slcan_serial_print("\r", 1);
	} else {
		/* respond with BELL for error */
		slcan_serial_print("\a", 1);
		LOG_ERR("Serial processing error: %d", err);
	}

	return err;
}

/*
 * Hex characters have to be upper-case, so we can't use Zephyr's bin2hex().
 */
void buf_append_hex(char *dest, const uint8_t *src, size_t src_bytes)
{
	static const char nibble2char[16] = "0123456789ABCDEF";

	for (int i = 0; i < src_bytes; i++) {
		dest[i * 2] = nibble2char[(src[i] >> 4) & 0xF];
		dest[i * 2 + 1] = nibble2char[src[i] & 0xF];
	}
}

int slcan_canbus_process(struct can_frame *frame)
{
	char buf[SLCAN_MTU];
	uint8_t can_id[4];
	int pos = 0;

	LOG_DBG("CAN message with ID 0x%X, %d bytes: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X",
		frame->id, frame->dlc, frame->data[0], frame->data[1], frame->data[2],
		frame->data[3], frame->data[4], frame->data[5], frame->data[6], frame->data[7]);

	sys_put_be32(frame->id, can_id);

	/* CAN ID */
	if ((frame->flags & CAN_FRAME_IDE) == 0) {
		buf_append_hex(buf, can_id + 2, 2);
		buf[0] = 't'; /* overwrite first digit for correct 3-digit standard ID encoding */
		pos = 1 + 3;
	} else {
		buf[0] = 'T';
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

	slcan_serial_print(buf, pos);

	return 0;
}

void slcan_serial_thread(void *arg1, void *arg2, void *arg3)
{
	char buf[SLCAN_MTU];
	uint32_t dtr = 0;

	/* Poll if the DTR flag was set */
	while (!dtr) {
		uart_line_ctrl_get(serial_dev, UART_LINE_CTRL_DTR, &dtr);
		/* Give CPU resources to low priority threads. */
		k_sleep(K_MSEC(100));
	}

	uart_irq_callback_user_data_set(serial_dev, serial_rx_callback, NULL);
	uart_irq_rx_enable(serial_dev);

	while (k_msgq_get(&serial_rx_msgq, &buf, K_FOREVER) == 0) {
		slcan_serial_process(buf, strlen(buf));
	}
}

void slcan_canbus_thread(void *arg1, void *arg2, void *arg3)
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

	if (usb_enable(NULL)) {
		return -ENODEV;
	}

	k_thread_create(&canbus_thread_data, canbus_thread_stack,
			K_THREAD_STACK_SIZEOF(canbus_thread_stack),
			slcan_canbus_thread, NULL, NULL, NULL,
			SLCAN_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&serial_thread_data, serial_thread_stack,
			K_THREAD_STACK_SIZEOF(serial_thread_stack),
			slcan_serial_thread, NULL, NULL, NULL,
			SLCAN_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(slcan_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

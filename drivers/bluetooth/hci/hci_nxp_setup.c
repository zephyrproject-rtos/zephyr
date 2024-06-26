/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <string.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_nxp_ctlr);

#include "common/bt_str.h"

#include "bt_nxp_ctlr_fw.h"

#define DT_DRV_COMPAT nxp_bt_hci_uart

#define FW_UPLOAD_CHANGE_TIMEOUT_RETRY_COUNT 6

static const struct device *uart_dev = DEVICE_DT_GET(DT_INST_GPARENT(0));

#if DT_NODE_HAS_PROP(DT_DRV_INST(0), sdio_reset_gpios)
struct gpio_dt_spec sdio_reset = GPIO_DT_SPEC_GET(DT_DRV_INST(0), sdio_reset_gpios);
#endif /* DT_NODE_HAS_PROP(DT_DRV_INST(0), sdio_reset_gpios) */
#if DT_NODE_HAS_PROP(DT_DRV_INST(0), w_disable_gpios)
struct gpio_dt_spec w_disable = GPIO_DT_SPEC_GET(DT_DRV_INST(0), w_disable_gpios);
#endif /* DT_NODE_HAS_PROP(DT_DRV_INST(0), w_disable_gpios) */

struct nxp_ctlr_dev_data {
	uint32_t primary_speed;
	bool primary_flowcontrol;
	uint32_t secondary_speed;
	bool secondary_flowcontrol;
};

static struct nxp_ctlr_dev_data uart_dev_data;

#define DI         0x07U
#define POLYNOMIAL 0x04c11db7UL

#define CRC32_LEN 4

static unsigned long crc_table[256U];
static bool made_table;

static void fw_upload_gen_crc32_table(void)
{
	int i, j;
	unsigned long crc_accum;

	for (i = 0; i < 256; i++) {
		crc_accum = ((unsigned long)i << 24);
		for (j = 0; j < 8; j++) {
			if (crc_accum & 0x80000000L) {
				crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
			} else {
				crc_accum = (crc_accum << 1);
			}
		}
		crc_table[i] = crc_accum;
	}
}

static unsigned char fw_upload_crc8(unsigned char *array, unsigned char len)
{
	unsigned char crc_8 = 0xff;

	crc_8 = crc8(array, len, DI, crc_8, false);

	return crc_8;
}

static unsigned long fw_upload_update_crc32(unsigned long crc_accum, char *data_blk_ptr,
					 int data_blk_size)
{
	unsigned int i, j;

	for (j = 0; j < data_blk_size; j++) {
		i = ((unsigned int)(crc_accum >> 24) ^ *data_blk_ptr++) & 0xff;
		crc_accum = (crc_accum << 8) ^ crc_table[i];
	}
	return crc_accum;
}

#define CMD4 0x4U
#define CMD6 0x6U
#define CMD7 0x7U

#define V1_HEADER_DATA_REQ  0xa5U
#define V1_START_INDICATION 0xaaU
#define V1_REQUEST_ACK      0x5aU

#define V3_START_INDICATION 0xabU
#define V3_HEADER_DATA_REQ  0xa7U
#define V3_REQUEST_ACK      0x7aU
#define V3_TIMEOUT_ACK      0x7bU
#define V3_CRC_ERROR        0x7cU

#define REQ_HEADER_LEN    1U
#define A6REQ_PAYLOAD_LEN 8U
#define AbREQ_PAYLOAD_LEN 3U

#define CRC_ERR_BIT          BIT(0)
#define NAK_REC_BIT          BIT(1)
#define TIMEOUT_REC_ACK_BIT  BIT(2)
#define TIMEOUT_REC_HEAD_BIT BIT(3)
#define TIMEOUT_REC_DATA_BIT BIT(4)
#define INVALID_CMD_REC_BIT  BIT(5)
#define WIFI_MIC_FAIL_BIT    BIT(6)
#define BT_MIC_FAIL_BIT      BIT(7)

#define CMD_HDR_LEN 16

/* CMD5 Header to change bootloader baud rate */
static uint8_t cmd5_hdrData[CMD_HDR_LEN] = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					    0x2c, 0x00, 0x00, 0x00, 0x77, 0xdb, 0xfd, 0xe0};
/* CMD7 Header to change timeout of bootloader */
static uint8_t cmd7_hdrData[CMD_HDR_LEN] = {0x07, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
					    0x00, 0x00, 0x00, 0x00, 0x5b, 0x88, 0xf8, 0xba};

enum bt_nxp_ctlr_version {
	VER1 = 1,
	VER2,
	VER3,
};

struct change_speed_config {
	uint32_t clk_div_addr;
	uint32_t clk_div_val;
	uint32_t uart_clk_div_addr;
	uint32_t uart_clk_div_val;
	uint32_t mcr_addr;
	uint32_t mcr_val;
	uint32_t reinit_addr;
	uint32_t reinit_val;
	uint32_t icr_addr;
	uint32_t icr_val;
	uint32_t fcr_addr;
	uint32_t fcr_val;
};

#define SEND_BUFFER_MAX_LENGTH  0xFFFFU /* Maximum 2 byte value */
#define RECV_RING_BUFFER_LENGTH 1024

struct nxp_ctlr_fw_upload_state {
	uint8_t version;
	uint8_t hdr_sig;

	uint8_t buffer[A6REQ_PAYLOAD_LEN + REQ_HEADER_LEN + 1];

	uint8_t send_buffer[SEND_BUFFER_MAX_LENGTH + 1];

	struct {
		uint8_t buffer[RECV_RING_BUFFER_LENGTH];
		uint32_t head;
		uint32_t tail;
		struct k_sem sem;
	} rx;

	uint16_t length;
	uint32_t offset;
	uint16_t error;
	uint8_t crc8;

	uint32_t last_offset;
	uint8_t change_speed_buffer[sizeof(struct change_speed_config) + CRC32_LEN];

	uint32_t fw_length;
	uint32_t current_length;
	const uint8_t *fw;

	uint32_t cmd7_change_timeout_len;
	uint32_t change_speed_buffer_len;

	bool wait_hdr_sig;

	bool is_hdr_data;
	bool is_error_case;
	bool is_cmd7_req;
	bool is_entry_point_req;

	uint8_t last_5bytes_buffer[6];
};

static struct nxp_ctlr_fw_upload_state fw_upload;

static int fw_upload_read_data(uint8_t *buffer, uint32_t len)
{
	int err;

	while (len > 0) {
		err = k_sem_take(&fw_upload.rx.sem,
				 K_MSEC(CONFIG_BT_H4_NXP_CTLR_WAIT_HDR_SIG_TIMEOUT));
		if (err < 0) {
			LOG_ERR("Fail to read data");
			return err;
		}
		*buffer = fw_upload.rx.buffer[fw_upload.rx.tail];
		buffer++;
		fw_upload.rx.tail++;
		fw_upload.rx.tail = fw_upload.rx.tail % sizeof(fw_upload.rx.buffer);
		len--;
	}
	return 0;
}

static void fw_upload_read_to_clear(void)
{
	uint32_t key;

	key = irq_lock();
	k_sem_reset(&fw_upload.rx.sem);
	fw_upload.rx.head = 0;
	fw_upload.rx.tail = 0;
	irq_unlock(key);
}

static int fw_upload_wait_for_hdr_sig(void)
{
	int err;
	int64_t end;
	char c;

	end = k_uptime_get() + CONFIG_BT_H4_NXP_CTLR_WAIT_HDR_SIG_TIMEOUT;
	fw_upload.hdr_sig = 0xFF;

	while (k_uptime_get() < end) {
		err = fw_upload_read_data(&c, 1);
		if (err < 0) {
			k_msleep(1);
			continue;
		}
		if ((c == V1_HEADER_DATA_REQ) || (c == V1_START_INDICATION) ||
		    (c == V3_START_INDICATION) || (c == V3_HEADER_DATA_REQ)) {
			LOG_DBG("HDR SIG found 0x%02X", c);
			fw_upload.hdr_sig = c;
			if (fw_upload.version == 0) {
				if ((c == V3_START_INDICATION) || (c == V3_HEADER_DATA_REQ)) {
					fw_upload.version = VER3;
				} else {
					fw_upload.version = VER1;
				}
			}
			return 0;
		}
	}
	LOG_ERR("HDR SIG not found");
	return -EIO;
}

static void fw_upload_write_data(const uint8_t *buffer, uint32_t len)
{
	for (int i = 0; i < len; i++) {
		uart_poll_out(uart_dev, buffer[i]);
	}
}

static int fw_upload_request_check_crc(uint8_t *buffer, uint8_t request)
{
	uint8_t crc;

	if (request == V3_HEADER_DATA_REQ) {
		crc = fw_upload_crc8(buffer, A6REQ_PAYLOAD_LEN + REQ_HEADER_LEN);
		if (crc != buffer[A6REQ_PAYLOAD_LEN + REQ_HEADER_LEN]) {
			LOG_ERR("Request %d, CRC check failed", request);
			return -EINVAL;
		}
	} else if (request == V3_START_INDICATION) {
		crc = fw_upload_crc8(buffer, AbREQ_PAYLOAD_LEN + REQ_HEADER_LEN);
		if (crc != buffer[AbREQ_PAYLOAD_LEN + REQ_HEADER_LEN]) {
			LOG_ERR("Request %d, CRC check failed", request);
			return -EINVAL;
		}
	} else {
		LOG_ERR("Invalid request %d", request);
	}

	return 0;
}

static void fw_upload_send_ack(uint8_t ack)
{
	if ((ack == V3_REQUEST_ACK) || (ack == V3_CRC_ERROR)) {
		/* prepare crc for 0x7A or 0x7C */
		fw_upload.buffer[0] = ack;
		fw_upload.buffer[1] = fw_upload_crc8(fw_upload.buffer, 1);
		fw_upload_write_data(fw_upload.buffer, 2);
		LOG_DBG("ACK = %x, CRC = %x", ack, fw_upload.buffer[1]);
	} else if (ack == V3_TIMEOUT_ACK) {
		/* prepare crc for 0x7B */
		fw_upload.buffer[0] = ack;
		sys_put_le32(fw_upload.offset, &fw_upload.buffer[1]);
		fw_upload.buffer[5] = fw_upload_crc8(fw_upload.buffer, 5);
		fw_upload_write_data(fw_upload.buffer, 6);
		LOG_DBG("ACK = %x, CRC = %x", ack, fw_upload.buffer[5]);
	} else {
		LOG_ERR("Invalid ack");
	}
}

static int fw_upload_wait_req(bool secondary_speed)
{
	int err;
	uint32_t len;
	uint8_t buffer[10];

	buffer[0] = fw_upload.hdr_sig;
	if (fw_upload.hdr_sig == V3_HEADER_DATA_REQ) {
		/* CMD LINE: 0xA7 <len><offset><error><CRC8> */
		len = A6REQ_PAYLOAD_LEN + 1;
	} else if (fw_upload.hdr_sig == V3_START_INDICATION) {
		/* CMD LINE: 0xAB <CHIP ID><SW loader REV 1 byte><CRC8> */
		len = AbREQ_PAYLOAD_LEN + 1;
	} else {
		return -EINVAL;
	}

	err = fw_upload_read_data(&buffer[1], len);
	if (err < 0) {
		LOG_ERR("Fail to read req");
		return err;
	}

	err = fw_upload_request_check_crc(buffer, fw_upload.hdr_sig);
	if (err != 0) {
		LOG_ERR("Fail to check CRC");
		fw_upload_send_ack(V3_CRC_ERROR);
		return err;
	}

	if (fw_upload.hdr_sig == V3_HEADER_DATA_REQ) {
		fw_upload.length = sys_get_le16(&buffer[1]);
		fw_upload.offset = sys_get_le32(&buffer[3]);
		fw_upload.error = sys_get_le16(&buffer[7]);
		fw_upload.crc8 = buffer[9];
		LOG_DBG("Req: %hhd, %hd, %d, %hd, %hhd", fw_upload.hdr_sig, fw_upload.length,
			fw_upload.offset, fw_upload.error, fw_upload.crc8);
	} else if (fw_upload.hdr_sig == V3_START_INDICATION) {
		uint16_t chip_id;

		fw_upload_send_ack(V3_REQUEST_ACK);
		chip_id = sys_get_le16(&buffer[1]);
		LOG_DBG("Indicate: %hhd, %hd, %hhd, %hhd", fw_upload.hdr_sig, chip_id, buffer[3],
			buffer[4]);

		if (!secondary_speed) {
			return -EINVAL;
		}
	}

	return 0;
}

static int fw_upload_change_timeout(void)
{
	int err = 0;
	bool first = true;
	uint8_t retry = FW_UPLOAD_CHANGE_TIMEOUT_RETRY_COUNT;

	LOG_DBG("");

	fw_upload_gen_crc32_table();

	while (true) {
		err = fw_upload_wait_for_hdr_sig();
		if (err) {
			continue;
		}

		if (fw_upload.version == VER1) {
			return 0;
		} else if (fw_upload.version == VER3) {
			err = fw_upload_wait_req(true);
			if (err) {
				continue;
			}

			if (fw_upload.length == 0) {
				continue;
			}

			if (fw_upload.error == 0) {
				if (first || (fw_upload.last_offset == fw_upload.offset)) {
					fw_upload_send_ack(V3_REQUEST_ACK);
					fw_upload_write_data(cmd7_hdrData,
							     fw_upload.length > CMD_HDR_LEN
								     ? CMD_HDR_LEN
								     : fw_upload.length);
					fw_upload.last_offset = fw_upload.offset;
					first = false;
				} else {
					fw_upload.cmd7_change_timeout_len = CMD_HDR_LEN;
					fw_upload.wait_hdr_sig = false;
					return 0;
				}
			} else {
				if (retry > 0) {
					retry--;
					fw_upload_send_ack(V3_TIMEOUT_ACK);
				} else {
					LOG_ERR("Fail to change timeout with response err %d",
						fw_upload.error);
					return -ENOTSUP;
				}
			}
		} else {
			LOG_ERR("Unsupported version %d", fw_upload.version);
			return -ENOTSUP;
		}
	}

	return -EINVAL;
}

typedef struct {
	uint32_t uartBaudRate;
	uint32_t uartDivisio;
	uint32_t uartClkDivisor;
} uart_baudrate_clkDiv_map_t;

static const uart_baudrate_clkDiv_map_t clk_div_map[] = {
	{115200U, 16U, 0x0075F6FDU},
	{1000000U, 2U, 0x00800000U},
	{3000000U, 1U, 0x00C00000U},
};

static int fw_upload_change_speed_config(struct change_speed_config *config, uint32_t speed)
{
	config->clk_div_addr = 0x7f00008fU;
	config->uart_clk_div_addr = 0x7f000090U;
	config->mcr_addr = 0x7f000091U;
	config->reinit_addr = 0x7f000092U;
	config->icr_addr = 0x7f000093U;
	config->fcr_addr = 0x7f000094U;

	config->mcr_val = 0x00000022U;
	config->reinit_val = 0x00000001U;
	config->icr_val = 0x000000c7U;
	config->fcr_val = 0x000000c7U;

	for (int i = 0; i < ARRAY_SIZE(clk_div_map); i++) {
		if (speed == clk_div_map[i].uartBaudRate) {
			config->clk_div_val = clk_div_map[i].uartClkDivisor;
			config->uart_clk_div_val = clk_div_map[i].uartDivisio;
			return 0;
		}
	}
	return -ENOTSUP;
}

static uint16_t fw_upload_wait_length(uint8_t flag)
{
	uint8_t buffer[4];
	uint16_t len;
	uint16_t len_comp;
	int err;
	uint8_t ack;

	err = fw_upload_read_data(buffer, sizeof(buffer));
	if (err < 0) {
		return 0;
	}

	len = sys_get_le16(buffer);
	len_comp = sys_get_le16(buffer);

	if ((len ^ len_comp) == 0xFFFF) {
		LOG_DBG("remote asks for %d bytes", len);

		/* Successful. Send back the ack. */
		if ((fw_upload.hdr_sig == V1_HEADER_DATA_REQ) ||
		    (fw_upload.hdr_sig == V1_START_INDICATION)) {
			ack = V1_REQUEST_ACK;
			fw_upload_write_data(&ack, 1);
			if (fw_upload.hdr_sig == V1_START_INDICATION) {
				/* Eliminated longjmp(resync, 1); returning restart status */
				return (uint16_t)V1_START_INDICATION;
			}
		}
	} else {
		LOG_ERR("remote asks len %d bytes", len);
		LOG_ERR("remote asks len_comp %d bytes", len_comp);
		/* Failure due to mismatch. */
		ack = 0xbf;
		fw_upload_write_data(&ack, 1);
		/* Start all over again. */
		if (flag) {
			/* Eliminated longjmp(resync, 1); returning restart status */
			return (uint16_t)V1_START_INDICATION;
		}
		len = 0;
	}
	return len;
}

static uint32_t fw_upload_get_payload_length(uint8_t *cmd)
{
	uint32_t len;

	len = sys_get_le32(&cmd[8]);

	return len;
}

static void fw_upload_get_hdr_start(uint8_t *buffer)
{
	int err;
	bool done = false;
	uint32_t count = 0;

	while (!done) {
		err = fw_upload_read_data(&fw_upload.hdr_sig, 1);
		if (err >= 0) {
			if (fw_upload.hdr_sig == V1_HEADER_DATA_REQ) {
				buffer[count++] = fw_upload.hdr_sig;
				done = true;
				LOG_DBG("Found header %x", fw_upload.hdr_sig);
			}
		} else {
			LOG_ERR("Fail to read HDR sig %d", err);
			return;
		}
	}
	err = fw_upload_read_data(&buffer[count], 4);
	if (err < 0) {
		LOG_ERR("Fail to read HDR payload %d", err);
	}
}

static int fw_upload_len_valid(uint8_t *buffer, uint16_t *length)
{
	uint16_t len;
	uint16_t len_comp;

	len = sys_get_le16(&buffer[1]);
	len_comp = sys_get_le16(&buffer[3]);

	if ((len ^ len_comp) == 0xFFFFU) {
		*length = len;
		return 0;
	} else {
		return -EINVAL;
	}
}

static int fw_upload_get_last_5bytes(uint8_t *buffer)
{
	int err;
	uint32_t payload_len;
	uint16_t len = 0;

	memset(fw_upload.last_5bytes_buffer, 0, sizeof(fw_upload.last_5bytes_buffer));

	fw_upload_get_hdr_start(fw_upload.last_5bytes_buffer);
	err = fw_upload_len_valid(fw_upload.last_5bytes_buffer, &len);
	if (err >= 0) {
		LOG_DBG("Valid len %d", len);
	} else {
		LOG_ERR("Invalid HDR");
		return err;
	}

	payload_len = fw_upload_get_payload_length(buffer);

	if ((len == CMD_HDR_LEN) || ((uint32_t)len == payload_len)) {
		LOG_DBG("Len valid");
		fw_upload.is_error_case = false;
		return 0;
	}

	LOG_DBG("Len invalid");
	fw_upload.is_error_case = true;

	return -EINVAL;
}

static void fw_upload_update_result(uint32_t payload_len, uint16_t *sending_len,
				    bool *first_chunk_sent)
{
	if (fw_upload.is_cmd7_req || fw_upload.is_entry_point_req) {
		*sending_len = CMD_HDR_LEN;
		*first_chunk_sent = true;
	} else {
		*sending_len = payload_len;
		*first_chunk_sent = false;
		if (*sending_len == CMD_HDR_LEN) {
			fw_upload.is_hdr_data = true;
		}
	}
}

static int fw_upload_write_hdr_and_payload(uint16_t len_to_send, uint8_t *buffer, bool new_speed)
{
	int err;
	uint32_t payload_len;
	bool send_done = false;
	uint16_t sending_len = CMD_HDR_LEN;
	bool first_chunk_sent = false;

	LOG_DBG("");

	payload_len = fw_upload_get_payload_length(buffer);

	while (!send_done) {
		if (sending_len == len_to_send) {
			if ((sending_len == CMD_HDR_LEN) && (!fw_upload.is_hdr_data)) {
				if ((first_chunk_sent == false) ||
				    (first_chunk_sent && fw_upload.is_error_case)) {
					LOG_DBG("Send first chunk: len %d", sending_len);
					fw_upload_write_data(buffer, sending_len);
					fw_upload_update_result(payload_len, &sending_len,
								&first_chunk_sent);
				} else {
					send_done = true;
					break;
				}
			} else {
				LOG_DBG("Send data: len %d", sending_len);
				if (sending_len) {
					fw_upload_write_data(&buffer[CMD_HDR_LEN], sending_len);
					first_chunk_sent = true;
					sending_len = CMD_HDR_LEN;
					fw_upload.is_hdr_data = false;
					if (new_speed) {
						return 0;
					}
				} else {
					LOG_DBG("Download Complete");
					return 0;
				}
			}
		} else {
			if ((len_to_send & 0x01) == 0x01) {
				if (len_to_send == (CMD_HDR_LEN + 1)) {
					LOG_DBG("Resending first chunk...");
					fw_upload_write_data(buffer, len_to_send - 1);
					sending_len = payload_len;
					first_chunk_sent = false;
				} else if (len_to_send == (payload_len + 1)) {
					LOG_DBG("Resending second chunk...");
					fw_upload_write_data(&buffer[CMD_HDR_LEN], len_to_send - 1);
					sending_len = CMD_HDR_LEN;
					first_chunk_sent = true;
				}
			} else if (len_to_send == CMD_HDR_LEN) {
				LOG_DBG("Resending send buffer...");
				fw_upload_write_data(buffer, len_to_send);
				sending_len = payload_len;
				first_chunk_sent = false;
			} else if (len_to_send == payload_len) {
				LOG_DBG("Resending second chunk...");
				fw_upload_write_data(&buffer[CMD_HDR_LEN], len_to_send);
				sending_len = CMD_HDR_LEN;
				first_chunk_sent = true;
			}
		}

		err = fw_upload_get_last_5bytes(buffer);
		if (err < 0) {
			LOG_ERR("Fail to get response");
			return err;
		}

		if (fw_upload_len_valid(fw_upload.last_5bytes_buffer, &len_to_send) == 0) {
			fw_upload_send_ack(V1_REQUEST_ACK);
			LOG_DBG("BOOT_HEADER_ACK 0x5a sent");
		}
	}
	return len_to_send;
}

static int fw_upload_uart_reconfig(uint32_t speed, bool flow_control)
{
	struct uart_config config;
	int err;

	config.baudrate = speed;
	config.data_bits = UART_CFG_DATA_BITS_8;
	config.flow_ctrl = flow_control ? UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE;
	config.parity = UART_CFG_PARITY_NONE;
	config.stop_bits = UART_CFG_STOP_BITS_1;

	uart_irq_rx_disable(uart_dev);
	uart_irq_tx_disable(uart_dev);
	fw_upload_read_to_clear();
	err = uart_configure(uart_dev, &config);
	uart_irq_rx_enable(uart_dev);

	return err;
}

static int fw_upload_change_speed(uint8_t hdr)
{
	int err;
	uint32_t hdr_len;
	bool load_payload = false;
	bool recovery = false;
	uint16_t len_to_send;
	uint32_t crc;

	err = fw_upload_change_speed_config(
		(struct change_speed_config *)fw_upload.change_speed_buffer,
		uart_dev_data.secondary_speed);
	if (err) {
		return err;
	}

	hdr_len = sizeof(fw_upload.change_speed_buffer);

	fw_upload_gen_crc32_table();
	crc = sys_cpu_to_le32(hdr_len);
	memcpy(cmd5_hdrData + 8, &crc, 4);
	crc = fw_upload_update_crc32(0, (char *)cmd5_hdrData, 12);
	crc = sys_cpu_to_be32(crc);
	memcpy(cmd5_hdrData + 12, &crc, CRC32_LEN);
	crc = fw_upload_update_crc32(0, (char *)fw_upload.change_speed_buffer,
				  (int)sizeof(struct change_speed_config));
	crc = sys_cpu_to_be32(crc);
	memcpy(&fw_upload.change_speed_buffer[sizeof(struct change_speed_config)], &crc, CRC32_LEN);

	while (true) {
		err = fw_upload_wait_for_hdr_sig();

		if (hdr && (err == 0)) {
			if (load_payload) {
				if (fw_upload.version == VER3) {
					fw_upload.change_speed_buffer_len =
						CMD_HDR_LEN + fw_upload.length;
				}
				return 0;
			}
		} else {
			if (recovery) {
				return -ETIME;
			}

			if (load_payload) {
				LOG_ERR("HDR cannot be received by using second speed. receovery "
					"speed");

				err = fw_upload_uart_reconfig(uart_dev_data.primary_speed,
							      uart_dev_data.primary_flowcontrol);
				if (err) {
					return err;
				}

				load_payload = false;
				recovery = true;
				continue;
			}
		}

		if (fw_upload.version == VER1) {
			len_to_send = fw_upload_wait_length(0);
			if (len_to_send == V1_START_INDICATION) {
				return -EINVAL;
			} else if (len_to_send == 0) {
				continue;
			} else if (len_to_send == CMD_HDR_LEN) {
				memcpy(fw_upload.send_buffer, cmd5_hdrData, CMD_HDR_LEN);
				memcpy(&fw_upload.send_buffer[CMD_HDR_LEN],
				       fw_upload.change_speed_buffer, hdr_len);

				err = fw_upload_write_hdr_and_payload(len_to_send,
								   fw_upload.send_buffer, true);
				if (err < 0) {
					return err;
				}

				LOG_DBG("Change speed to %d", uart_dev_data.secondary_speed);

				err = fw_upload_uart_reconfig(uart_dev_data.secondary_speed,
							      uart_dev_data.secondary_flowcontrol);
				if (err) {
					return err;
				}
				load_payload = true;
			} else {
				fw_upload_write_data(fw_upload.change_speed_buffer, hdr_len);

				LOG_DBG("Change speed to %d", uart_dev_data.secondary_speed);

				err = fw_upload_uart_reconfig(uart_dev_data.secondary_speed,
							      uart_dev_data.secondary_flowcontrol);
				if (err) {
					return err;
				}
				load_payload = true;
			}
		} else if (fw_upload.version == VER3) {
			err = fw_upload_wait_req(true);
			if (!(!hdr || (err == 0))) {
				continue;
			}
			if (fw_upload.length && (fw_upload.hdr_sig == V3_HEADER_DATA_REQ)) {
				if (fw_upload.error != 0) {
					fw_upload_send_ack(V3_TIMEOUT_ACK);
					continue;
				}

				fw_upload_send_ack(V3_REQUEST_ACK);
				hdr = true;

				if (fw_upload.length == CMD_HDR_LEN) {
					LOG_DBG("Send CMD5");
					fw_upload_write_data(cmd5_hdrData, fw_upload.length);
					fw_upload.last_offset = fw_upload.offset;
				} else {
					LOG_DBG("Send UA RT config");
					fw_upload_write_data(fw_upload.change_speed_buffer,
							     fw_upload.length);

					LOG_DBG("Change speed to %d",
						uart_dev_data.secondary_speed);

					err = fw_upload_uart_reconfig(
						uart_dev_data.secondary_speed,
						uart_dev_data.secondary_flowcontrol);
					if (err) {
						return err;
					}
					load_payload = true;
				}
			}
		}
	}

	return 0;
}

static int fw_upload_v1_send_data(uint16_t len)
{
	uint32_t cmd;
	uint32_t data_len;
	int ret_len;

	memset(fw_upload.send_buffer, 0, sizeof(fw_upload.send_buffer));

	fw_upload.is_cmd7_req = false;
	fw_upload.is_entry_point_req = false;

	if ((fw_upload.fw_length - fw_upload.current_length) < len) {
		len = fw_upload.fw_length - fw_upload.current_length;
	}

	memcpy(fw_upload.send_buffer, fw_upload.fw + fw_upload.current_length, len);
	fw_upload.current_length += len;
	cmd = sys_get_le32(fw_upload.send_buffer);
	if (cmd == CMD7) {
		fw_upload.is_cmd7_req = true;
		data_len = 0;
	} else {
		data_len = fw_upload_get_payload_length(fw_upload.send_buffer);
		if ((data_len > (sizeof(fw_upload.send_buffer) - len)) ||
		    ((data_len + fw_upload.current_length) > fw_upload.fw_length)) {
			LOG_ERR("Invalid FW at %d/%d", fw_upload.current_length,
				fw_upload.fw_length);
			return -EINVAL;
		}
		memcpy(&fw_upload.send_buffer[len], fw_upload.fw + fw_upload.current_length,
		       data_len);
		fw_upload.current_length += data_len;
		if ((fw_upload.current_length < fw_upload.fw_length) &&
		    ((cmd == CMD6) || (cmd == CMD4))) {
			fw_upload.is_entry_point_req = true;
		}
	}

	ret_len = fw_upload_write_hdr_and_payload(len, fw_upload.send_buffer, false);
	LOG_DBG("FW upload %d/%d", fw_upload.current_length, fw_upload.fw_length);

	return ret_len;
}

static int fw_upload_v3_send_data(void)
{
	uint32_t start;

	LOG_DBG("Sending offset %d", fw_upload.offset);
	if (fw_upload.offset == fw_upload.last_offset) {
		LOG_WRN("Resending offset %d ...", fw_upload.offset);
		fw_upload_write_data(fw_upload.send_buffer, fw_upload.length);
		return fw_upload.length;
	}
	memset(fw_upload.send_buffer, 0, sizeof(fw_upload.send_buffer));
	start = fw_upload.offset - fw_upload.cmd7_change_timeout_len -
		fw_upload.change_speed_buffer_len;
	if (start >= fw_upload.fw_length) {
		LOG_ERR("Invalid fw offset");
		return -EINVAL;
	}

	if ((fw_upload.length + start) > fw_upload.fw_length) {
		fw_upload.length = fw_upload.fw_length - start;
	}
	memcpy(fw_upload.send_buffer, fw_upload.fw + start, fw_upload.length);
	fw_upload.current_length = start + fw_upload.length;

	fw_upload_write_data(fw_upload.send_buffer, fw_upload.length);
	fw_upload.last_offset = fw_upload.offset;

	return fw_upload.length;
}

static int fw_uploading(const uint8_t *fw, uint32_t fw_length)
{
	int err;
	bool secondary_speed = false;
	uint16_t len_to_send;

	fw_upload.wait_hdr_sig = true;
	fw_upload.is_hdr_data = false;
	fw_upload.is_error_case = false;
	fw_upload.is_cmd7_req = false;
	fw_upload.is_entry_point_req = false;
	fw_upload.last_offset = 0xFFFFU;

	err = fw_upload_change_timeout();
	LOG_DBG("Change timeout hdr flag %d (err %d)", fw_upload.wait_hdr_sig, err);
	if (err) {
		return err;
	}

	fw_upload_read_to_clear();

	if (uart_dev_data.secondary_speed &&
	    (uart_dev_data.secondary_speed != uart_dev_data.primary_speed)) {
		LOG_DBG("Change speed to %d", uart_dev_data.secondary_speed);
		err = fw_upload_change_speed(fw_upload.wait_hdr_sig);
		if (err != 0) {
			LOG_ERR("Fail to change speed");
			return err;
		}
		secondary_speed = true;
	}

	fw_upload.fw_length = fw_length;
	fw_upload.current_length = 0;
	fw_upload.fw = fw;

	while (true) {
		err = fw_upload_wait_for_hdr_sig();
		if (secondary_speed && (err != 0)) {
			return -ETIME;
		}

		secondary_speed = false;

		if (fw_upload.version == VER1) {
			len_to_send = fw_upload_wait_length(true);

			if (len_to_send == V1_START_INDICATION) {
				continue;
			}
			while (len_to_send > 0) {
				len_to_send = fw_upload_v1_send_data(len_to_send);
			}
			if (fw_upload.current_length >= fw_upload.fw_length) {
				LOG_DBG("FW download done");
				return 0;
			}
			LOG_ERR("FW download failed");
			return len_to_send;
		} else if (fw_upload.version == VER3) {
			if (fw_upload.hdr_sig == V3_START_INDICATION) {
				fw_upload_wait_req(false);
				continue;
			}
			err = fw_upload_wait_req(false);
			if (err) {
				LOG_ERR("Fail to wait req");
				return err;
			}
			if (fw_upload.length) {
				if (fw_upload.error == 0) {
					fw_upload_send_ack(V3_REQUEST_ACK);
					err = fw_upload_v3_send_data();
					if (err < 0) {
						LOG_ERR("FW download failed");
						return err;
					}
				} else {
					LOG_ERR("Error occurs %d", fw_upload.error);
					fw_upload_send_ack(V3_TIMEOUT_ACK);
					if (fw_upload.error & BT_MIC_FAIL_BIT) {
						fw_upload.change_speed_buffer_len = 0;
						fw_upload.current_length = 0;
						fw_upload.last_offset = 0;
					}
				}
			} else {
				if (fw_upload.error == 0) {
					fw_upload_send_ack(V3_REQUEST_ACK);
					LOG_DBG("FW download done");
					return 0;
				}
				LOG_ERR("Error occurs %d", fw_upload.error);
				fw_upload_send_ack(V3_TIMEOUT_ACK);
				if (fw_upload.error & BT_MIC_FAIL_BIT) {
					fw_upload.change_speed_buffer_len = 0;
					fw_upload.current_length = 0;
					fw_upload.last_offset = 0;
				}
			}
		} else {
			return -ENOTSUP;
		}
	}
	return -EINVAL;
}

static void bt_nxp_ctlr_uart_isr(const struct device *unused, void *user_data)
{
	int err = 0;
	int count = 0;

	ARG_UNUSED(unused);
	ARG_UNUSED(user_data);

	while (uart_irq_update(uart_dev) && uart_irq_is_pending(uart_dev)) {
		err = uart_poll_in(uart_dev, &fw_upload.rx.buffer[fw_upload.rx.head]);
		if (err >= 0) {
			fw_upload.rx.head++;
			fw_upload.rx.head = fw_upload.rx.head % sizeof(fw_upload.rx.buffer);
			count++;
		}
	}

	while (count > 0) {
		k_sem_give(&fw_upload.rx.sem);
		count--;
	}
}

static int bt_nxp_ctlr_init(void)
{
	int err;
	uint32_t speed;
	bool flowcontrol_of_hci;

	if (!device_is_ready(uart_dev)) {
		return -ENODEV;
	}

	speed = DT_PROP(DT_INST_GPARENT(0), current_speed);
	speed = DT_PROP_OR(DT_DRV_INST(0), hci_operation_speed, speed);
	uart_dev_data.primary_speed = DT_PROP_OR(DT_DRV_INST(0), fw_download_primary_speed, speed);
	uart_dev_data.secondary_speed =
		DT_PROP_OR(DT_DRV_INST(0), fw_download_secondary_speed, speed);

	flowcontrol_of_hci = (bool)DT_PROP_OR(DT_DRV_INST(0), hw_flow_control, false);
	uart_dev_data.primary_flowcontrol =
		(bool)DT_PROP_OR(DT_DRV_INST(0), fw_download_primary_flowcontrol, false);
	uart_dev_data.secondary_flowcontrol =
		(bool)DT_PROP_OR(DT_DRV_INST(0), fw_download_secondary_flowcontrol, false);

#if DT_NODE_HAS_PROP(DT_DRV_INST(0), sdio_reset_gpios) ||                                          \
	DT_NODE_HAS_PROP(DT_DRV_INST(0), w_disable_gpios)
#if DT_NODE_HAS_PROP(DT_DRV_INST(0), sdio_reset_gpios)
	/* Check BT REG_ON gpio instance */
	if (!gpio_is_ready_dt(&sdio_reset)) {
		LOG_ERR("Error: failed to configure sdio_reset %s pin %d", sdio_reset.port->name,
			sdio_reset.pin);
		return -EIO;
	}

	/* Configure sdio_reset as output  */
	err = gpio_pin_configure_dt(&sdio_reset, GPIO_OUTPUT);
	if (err) {
		LOG_ERR("Error %d: failed to configure sdio_reset %s pin %d", err,
			sdio_reset.port->name, sdio_reset.pin);
		return err;
	}
	err = gpio_pin_set_dt(&sdio_reset, 0);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(DT_DRV_INST(0), sdio_reset_gpios) */

#if DT_NODE_HAS_PROP(DT_DRV_INST(0), w_disable_gpios)
	/* Check BT REG_ON gpio instance */
	if (!gpio_is_ready_dt(&w_disable)) {
		LOG_ERR("Error: failed to configure w_disable %s pin %d", w_disable.port->name,
			w_disable.pin);
		return -EIO;
	}

	/* Configure w_disable as output  */
	err = gpio_pin_configure_dt(&w_disable, GPIO_OUTPUT);
	if (err) {
		LOG_ERR("Error %d: failed to configure w_disable %s pin %d", err,
			w_disable.port->name, w_disable.pin);
		return err;
	}
	err = gpio_pin_set_dt(&w_disable, 0);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(DT_DRV_INST(0), w_disable_gpios) */

	/* wait for reset done */
	k_sleep(K_MSEC(100));

#if DT_NODE_HAS_PROP(DT_DRV_INST(0), sdio_reset_gpios)
	err = gpio_pin_set_dt(&sdio_reset, 1);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(DT_DRV_INST(0), sdio_reset_gpios) */

#if DT_NODE_HAS_PROP(DT_DRV_INST(0), w_disable_gpios)
	err = gpio_pin_set_dt(&w_disable, 1);
	if (err) {
		return err;
	}
#endif /* DT_NODE_HAS_PROP(DT_DRV_INST(0), w_disable_gpios) */
#endif

	uart_irq_rx_disable(uart_dev);
	uart_irq_tx_disable(uart_dev);

	fw_upload.rx.head = 0;
	fw_upload.rx.tail = 0;

	k_sem_init(&fw_upload.rx.sem, 0, sizeof(fw_upload.rx.buffer));

	uart_irq_callback_set(uart_dev, bt_nxp_ctlr_uart_isr);

	made_table = false;

	err = fw_upload_uart_reconfig(uart_dev_data.primary_speed,
				      uart_dev_data.primary_flowcontrol);
	if (err) {
		LOG_ERR("Fail to config uart");
		return err;
	}

	uart_irq_rx_enable(uart_dev);

	err = fw_uploading(bt_fw_bin, bt_fw_bin_len);

	if (err) {
		LOG_ERR("Fail to upload firmware");
		return err;
	}

	(void)fw_upload_uart_reconfig(speed, flowcontrol_of_hci);

	uart_irq_rx_disable(uart_dev);
	uart_irq_tx_disable(uart_dev);

	k_sleep(K_MSEC(CONFIG_BT_H4_NXP_CTLR_WAIT_TIME_AFTER_UPLOAD));

	return 0;
}

int bt_hci_transport_setup(const struct device *dev)
{
	if (dev != uart_dev) {
		return -EINVAL;
	}

	return bt_nxp_ctlr_init();
}

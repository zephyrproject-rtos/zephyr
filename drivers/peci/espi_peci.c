/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_espi_peci

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/peci.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(espi_peci, CONFIG_PECI_LOG_LEVEL);

/** eSPI OOB command code used for PECI encapsulation. */
#define PECI_OOB_CMD_CODE            0x01U
/**
 * @brief Compute OOB destination address field.
 *
 * OOB address encoding uses the 7-bit address shifted left and a R/W bit in bit0.
 *
 * @param addr 7-bit address
 * @return Encoded destination address byte.
 */
#define ESPI_PECI_OOB_DST_ADDR(addr) ((addr << 1) | 0)

/**
 * @brief Compute OOB source address field.
 *
 * OOB address encoding uses the 7-bit address shifted left and a R/W bit in bit0.
 *
 * @param addr 7-bit address
 * @return Encoded source address byte.
 */
#define ESPI_PECI_OOB_SRC_ADDR(addr) ((addr << 1) | 1)

/** Size of the eSPI OOB header in bytes. */
#define OOB_PACKET_HEADER_SIZE 4U

/** Size of the PECI command header carried over OOB (excluding OOB header). */
#define OOB_PECI_CMD_HDR_SIZE 4U

/**
 * Minimum valid PECI-over-OOB response size.
 *
 * Includes:
 * - PECI command header size (as carried over OOB)
 * - 1 byte response completion/status code
 */
#define OOB_PECI_MIN_RESP_SIZE (OOB_PECI_CMD_HDR_SIZE + 1)

/** Maximum PECI payload bytes supported by this transport. */
#define ESPI_PECI_DATA_BUF_LEN 32

/**
 * @brief Size (in bytes) of the temporary OOB RX buffer used for PECI responses.
 *
 * The receive buffer must be large enough to hold:
 * - the fixed PECI-over-OOB response overhead (@ref OOB_PECI_MIN_RESP_SIZE), and
 * - up to @ref ESPI_PECI_DATA_BUF_LEN bytes of response payload data.
 *
 * This macro defines that total buffer size and is used to dimension the local
 * buffer passed to @ref espi_receive_oob().
 */
#define ESPI_PECI_OOB_RX_BUF_LEN (ESPI_PECI_DATA_BUF_LEN + OOB_PECI_MIN_RESP_SIZE)

/** Maximum number of receive retries for a PECI-over-eSPI transaction. */
#define ESPI_PECI_RECV_RETRY_COUNT 3

/**
 * @brief eSPI OOB header for PECI messages.
 *
 * This header is placed at the beginning of OOB packets carrying PECI traffic.
 */
struct espi_oob_peci_hdr {
	uint8_t dest_addr; /**< Encoded OOB destination address. */
	uint8_t cmd_code;  /**< OOB command code (PECI_OOB_CMD_CODE). */
	uint8_t byte_cnt;  /**< Byte count as expected by the receiver. */
	uint8_t src_addr;  /**< Encoded OOB source address. */
} __packed;

/**
 * @brief PECI request encapsulated in an OOB packet.
 */
struct espi_oob_peci_req {
	struct espi_oob_peci_hdr oob_hdr;     /**< OOB header. */
	uint8_t addr;                         /**< PECI target address. */
	uint8_t wr_len;                       /**< Number of bytes written (including PECI cmd). */
	uint8_t rd_len;                       /**< Expected number of bytes to read back. */
	uint8_t cmd_code;                     /**< PECI command code. */
	uint8_t data[ESPI_PECI_DATA_BUF_LEN]; /**< PECI payload. */
} __packed;

/**
 * @brief PECI response encapsulated in an OOB packet.
 */
struct espi_oob_peci_resp {
	struct espi_oob_peci_hdr oob_hdr;     /**< OOB header. */
	uint8_t resp_code;                    /**< PECI completion/response code. */
	uint8_t data[ESPI_PECI_DATA_BUF_LEN]; /**< Response payload. */
} __packed;

/**
 * @brief Runtime data for the eSPI PECI device instance.
 */
struct espi_peci_data {
	bool enabled; /**< Whether transfers are enabled. */
};

/**
 * @brief Build-time configuration for the eSPI PECI device instance.
 */
struct espi_peci_config {
	const struct device *espi_dev; /**< Parent eSPI controller device. */
	uint8_t oob_addr;              /**< eSPI source address for OOB header. */
};

/**
 * @brief CRC table used for AWFCS calculation.
 *
 * As per the PECI specification, the originator computes AWFCS (Assured Write
 * Frame Check Sequence) using a CRC-8 lookup table.
 */
static const uint8_t peci_crc_table[] = {
	0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, /* Offset 0-7 */
	0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d, /* Offset 8-15 */
	0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, /* Offset 16-23 */
	0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D, /* Offset 24-31 */
	0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, /* Offset 32-39 */
	0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD, /* Offset 40-47 */
	0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, /* Offset 48-55 */
	0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD, /* Offset 56-63 */
	0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, /* Offset 64-71 */
	0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA, /* Offset 72-79 */
	0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, /* Offset 80-87 */
	0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A, /* Offset 88-95 */
	0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, /* Offset 96-103 */
	0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A, /* Offset 104-111 */
	0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, /* Offset 112-119 */
	0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A, /* Offset 120-127 */
	0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, /* Offset 128-135 */
	0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4, /* Offset 136-143 */
	0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, /* Offset 144-151 */
	0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4, /* Offset 152-159 */
	0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, /* Offset 160-167 */
	0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44, /* Offset 168-175 */
	0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, /* Offset 176-183 */
	0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34, /* Offset 184-191 */
	0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, /* Offset 192-199 */
	0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63, /* Offset 200-207 */
	0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, /* Offset 208-215 */
	0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13, /* Offset 216-223 */
	0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, /* Offset 224-231 */
	0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83, /* Offset 232-239 */
	0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, /* Offset 240-247 */
	0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3  /* Offset 248-255 */
};

/**
 * @brief Calculate PECI AWFCS (Assured Write Frame Check Sequence).
 *
 * AWFCS is used by some PECI write commands to provide integrity protection.
 * The AWFCS is computed over the provided buffer using the PECI CRC table and
 * then bit 7 is inverted.
 *
 * @param peci_buffer Pointer to the buffer to checksum.
 * @param awfcs_len Number of bytes from @p peci_buffer to include.
 *
 * @return The computed AWFCS value.
 *
 * @note The current implementation logs and returns @c -ENOMEM cast to uint8_t
 *       if @p peci_buffer is NULL (this is likely a bug in the original code,
 *       but documented here as-is).
 */
static uint8_t peci_calc_awfcs(uint8_t *peci_buffer, uint8_t awfcs_len)
{
	uint8_t peci_awfcs = 0x0;

	/* Parameter verification */
	if (peci_buffer == NULL) {
		LOG_ERR("%s(): No memory available for peci buf", __func__);
		return -ENOMEM;
	}

	/* Compute AWFCS code */
	for (uint8_t count = 0; count < awfcs_len; count++) {
		peci_awfcs = peci_awfcs ^ peci_buffer[count];
		peci_awfcs = peci_crc_table[peci_awfcs];
	}

	/* Invert upper bit to prevent mathematical failure that would
	 * cause the Write FCS to always be 0x0.
	 */
	peci_awfcs ^= BIT(7);

	return peci_awfcs;
}

/**
 * @brief Determine whether a PECI message requires an AWFCS byte.
 *
 * Some PECI write commands require the originator to append an AWFCS (Assured
 * Write Frame Check Sequence) byte to the transmitted frame to provide
 * integrity checking per the PECI specification. This helper inspects the
 * PECI command in @p msg and returns whether AWFCS must be generated and
 * appended by the transport.
 *
 * @param msg PECI message descriptor to inspect.
 *
 * @retval true  If the command in @p msg requires AWFCS (e.g. WR_PKG_CFG0/1).
 * @retval false If the command does not require AWFCS.
 */
static bool espi_peci_msg_needs_awfcs(struct peci_msg *msg)
{
	switch (msg->cmd_code) {
	case PECI_CMD_WR_PKG_CFG0:
	case PECI_CMD_WR_PKG_CFG1:
		return true;
	default:
		break;
	}

	return false;
}

/**
 * @brief Enable PECI-over-eSPI transfers for this device instance.
 *
 * @param dev PECI device instance.
 * @return 0 on success.
 */
int espi_peci_enable(const struct device *dev)
{
	const struct espi_peci_config *config = dev->config;
	struct espi_peci_data *data = dev->data;

	if (!device_is_ready(config->espi_dev) ||
	    !espi_get_channel_status(config->espi_dev, ESPI_CHANNEL_OOB)) {
		return -ENODEV;
	}

	data->enabled = true;

	return 0;
}

/**
 * @brief Disable PECI-over-eSPI transfers for this device instance.
 *
 * @param dev PECI device instance.
 * @return 0 on success.
 */
int espi_peci_disable(const struct device *dev)
{
	struct espi_peci_data *data = dev->data;

	data->enabled = false;

	return 0;
}

/**
 * @brief Send a PECI request over the eSPI OOB channel.
 *
 * Builds an OOB packet containing the PECI transaction and writes it using
 * @ref espi_send_oob.
 *
 * @param dev PECI device instance.
 * @param msg PECI message to send.
 *
 * @retval 0 on success.
 * @retval -ENOMEM if the message is too large for the fixed buffer.
 * @return a negative errno code from the eSPI layer on failure.
 */
static int espi_peci_send(const struct device *dev, struct peci_msg *msg)
{
	const struct espi_peci_config *config = dev->config;
	struct espi_oob_peci_req oob_peci_req;
	struct espi_oob_peci_hdr *oob_hdr = &oob_peci_req.oob_hdr;
	struct espi_oob_packet oob_pckt;

	if (msg->tx_buffer.len > ESPI_PECI_DATA_BUF_LEN - 1) {
		return -ENOMEM;
	}

	oob_hdr->dest_addr = ESPI_PECI_OOB_DST_ADDR(msg->oob_addr);
	oob_hdr->src_addr = ESPI_PECI_OOB_SRC_ADDR(config->oob_addr);
	oob_hdr->cmd_code = PECI_OOB_CMD_CODE;

	if (msg->cmd_code == PECI_CMD_PING) {
		/* PING command requires these two parameters to be set to 0 */
		oob_peci_req.wr_len = 0;
		oob_peci_req.rd_len = 0;
	} else {
		oob_peci_req.rd_len = msg->rx_buffer.len;
		/* Payload length plus commmand byte */
		oob_peci_req.wr_len = msg->tx_buffer.len + 1;
	}
	oob_hdr->byte_cnt = OOB_PACKET_HEADER_SIZE + oob_peci_req.wr_len;

	oob_peci_req.cmd_code = msg->cmd_code;
	oob_peci_req.addr = msg->oob_addr;
	/* Tx length includes peci cmd code. So copy len-1 byte as data */
	if (msg->tx_buffer.len > 1) {
		memcpy(oob_peci_req.data, msg->tx_buffer.buf, msg->tx_buffer.len);
	}

	oob_pckt.buf = (uint8_t *)&oob_peci_req;
	oob_pckt.len = OOB_PACKET_HEADER_SIZE + oob_hdr->byte_cnt - 1;

	if (espi_peci_msg_needs_awfcs(msg)) {
		/* Add AWFCS byte to all length fields */
		oob_peci_req.wr_len++;
		oob_hdr->byte_cnt++;
		oob_pckt.buf[oob_pckt.len] = peci_calc_awfcs(oob_pckt.buf, oob_pckt.len);
		oob_pckt.len++;
	}

	LOG_HEXDUMP_DBG(oob_pckt.buf, oob_pckt.len, "Tx Data:");

	return espi_send_oob(config->espi_dev, &oob_pckt);
}

/**
 * @brief Receive a PECI response over the eSPI OOB channel.
 *
 * Receives an OOB packet using @ref espi_receive_oob, checks for minimum size,
 * validates the PECI completion code, and copies response bytes into the PECI
 * message RX buffer.
 *
 * @param dev PECI device instance.
 * @param msg PECI message whose rx_buffer will be filled.
 *
 * @retval 0 on success.
 * @retval -EIO if the packet is invalid or an unknown completion code is seen.
 * @retval -ETIME for PECI timeout completion codes.
 * @retval -EINVAL for illegal request completion code.
 * @return a negative errno code from the eSPI layer on receive failure.
 */
static int espi_peci_receive(const struct device *dev, struct peci_msg *msg)
{
	const struct espi_peci_config *config = dev->config;
	struct espi_oob_peci_resp *oob_peci_req;
	struct espi_oob_packet oob_pckt;
	uint8_t oob_rx_buf[ESPI_PECI_OOB_RX_BUF_LEN];
	int ret;

	oob_pckt.len = ESPI_PECI_OOB_RX_BUF_LEN;
	oob_pckt.buf = oob_rx_buf;

	ret = espi_receive_oob(config->espi_dev, &oob_pckt);
	if (ret) {
		LOG_ERR("PECI OOB Rxn failed %d", ret);
		return ret;
	}

	if (oob_pckt.len < OOB_PECI_MIN_RESP_SIZE) {
		LOG_ERR("Received packet too small %d", oob_pckt.len);
		return -EIO;
	}

	LOG_HEXDUMP_DBG(oob_pckt.buf, oob_pckt.len, "Rx Data:");
	oob_peci_req = (struct espi_oob_peci_resp *)oob_pckt.buf;
	switch (oob_peci_req->resp_code) {
	case PECI_CC_RSP_SUCCESS:
		/* Successful response */
		break;
	case PECI_CC_RSP_TIMEOUT:
	case PECI_CC_OUT_OF_RESOURCES_TIMEOUT:
		return -ETIME;
	case PECI_CC_ILLEGAL_REQUEST:
		return -EINVAL;
	default:
		LOG_ERR("Unknown return code %d", oob_peci_req->resp_code);
		return -EIO;
	}

	/* Copy command response data only */
	if (oob_pckt.len > OOB_PECI_MIN_RESP_SIZE && msg->rx_buffer.buf) {
		memcpy(msg->rx_buffer.buf, oob_rx_buf + OOB_PECI_MIN_RESP_SIZE,
		       oob_pckt.len - OOB_PECI_MIN_RESP_SIZE);
	}

	msg->rx_buffer.len = oob_pckt.len - OOB_PECI_MIN_RESP_SIZE;

	return 0;
}

/**
 * @brief Perform a full PECI transfer (send request and receive response).
 *
 * @param dev PECI device instance.
 * @param msg PECI transaction descriptor.
 *
 * @retval 0 on success.
 * @retval -ENODEV if the device/transport is not enabled.
 * @return negative errno code on failure.
 *
 * @note If the device is not enabled, this currently returns 0 without doing
 *       anything.
 */
int espi_peci_transfer(const struct device *dev, struct peci_msg *msg)
{
	struct espi_peci_data *data = dev->data;
	int ret;
	int rcv_retries;

	if (!data->enabled) {
		return -ENODEV;
	}

	ret = espi_peci_send(dev, msg);
	if (ret) {
		LOG_ERR("PECI OOB Txn failed %d", ret);
		return ret;
	}

	rcv_retries = 0;
	while (rcv_retries++ < ESPI_PECI_RECV_RETRY_COUNT) {
		ret = espi_peci_receive(dev, msg);
		if (ret != -ETIME) {
			break;
		}
		/* Timeout error, try again */
	}

	return ret;
}

/** PECI driver API implementation for the eSPI transport. */
static DEVICE_API(peci, espi_peci_api) = {
	.disable = espi_peci_disable,
	.enable = espi_peci_enable,
	.transfer = espi_peci_transfer,
};

static int espi_peci_init(const struct device *dev)
{
	const struct espi_peci_config *config = dev->config;

	if (!device_is_ready(config->espi_dev)) {
		return -ENODEV;
	}

	return 0;
}

#define ESPI_PECI_DEFINE(inst)                                                                     \
	static struct espi_peci_data espi_peci_data_##inst;                                        \
	static const struct espi_peci_config espi_peci_config_##inst = {                           \
		.espi_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),                                      \
		.oob_addr = DT_INST_PROP(inst, peci_addr),                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, espi_peci_init, NULL, &espi_peci_data_##inst,                  \
			      &espi_peci_config_##inst, POST_KERNEL, CONFIG_ESPI_INIT_PRIORITY,    \
			      &espi_peci_api);

DT_INST_FOREACH_STATUS_OKAY(ESPI_PECI_DEFINE)

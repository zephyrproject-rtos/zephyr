/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_DISC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(sdhc);

#include <disk_access.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <spi.h>
#include <crc.h>

#define SDHC_SECTOR_SIZE 512
#define SDHC_CMD_SIZE 6
#define SDHC_CMD_BODY_SIZE (SDHC_CMD_SIZE - 1)
#define SDHC_CRC16_SIZE 2

/* Command IDs */
#define SDHC_GO_IDLE_STATE 0
#define SDHC_SEND_IF_COND 8
#define SDHC_SEND_CSD 9
#define SDHC_SEND_CID 10
#define SDHC_STOP_TRANSMISSION 12
#define SDHC_SEND_STATUS 13
#define SDHC_READ_SINGLE_BLOCK 17
#define SDHC_READ_MULTIPLE_BLOCK 18
#define SDHC_WRITE_BLOCK 24
#define SDHC_WRITE_MULTIPLE_BLOCK 25
#define SDHC_APP_CMD 55
#define SDHC_READ_OCR 58
#define SDHC_CRC_ON_OFF 59
#define SDHC_SEND_OP_COND 41

/* Command flags */
#define SDHC_START 0x80
#define SDHC_TX 0x40

/* Fields in various card registers */
#define SDHC_HCS (1 << 30)
#define SDHC_CCS (1 << 30)
#define SDHC_VHS_MASK (0x0F << 8)
#define SDHC_VHS_3V3 (1 << 8)
#define SDHC_CHECK 0xAA
#define SDHC_CSD_SIZE 16
#define SDHC_CSD_V2 1

/* R1 response status */
#define SDHC_R1_IDLE 0x01
#define SDHC_R1_ERASE_RESET 0x02
#define SDHC_R1_ILLEGAL_COMMAND 0x04
#define SDHC_R1_COM_CRC 0x08
#define SDHC_R1_ERASE_SEQ 0x10
#define SDHC_R1_ADDRESS 0x20
#define SDHC_R1_PARAMETER 0x40

/* Data block tokens */
#define SDHC_TOKEN_SINGLE 0xFE
#define SDHC_TOKEN_MULTI_WRITE 0xFC
#define SDHC_TOKEN_STOP_TRAN 0xFD

/* Data block responses */
#define SDHC_RESPONSE_ACCEPTED 0x05
#define SDHC_RESPONSE_CRC_ERR 0x0B
#define SDHC_RESPONSE_WRITE_ERR 0x0E

/* Clock speed used during initialisation */
#define SDHC_INITIAL_SPEED 400000
/* Clock speed used after initialisation */
#define SDHC_SPEED 4000000

#define SDHC_MIN_TRIES 20
#define SDHC_RETRY_DELAY K_MSEC(20)
/* Time to wait for the card to initialise */
#define SDHC_INIT_TIMEOUT K_MSEC(5000)
/* Time to wait for the card to respond or come ready */
#define SDHC_READY_TIMEOUT K_MSEC(500)

struct sdhc_data {
	struct device *spi;
	struct spi_config cfg;
	struct device *cs;
	u32_t pin;

	u32_t sector_count;
	u8_t status;
	int trace_dir;
};

struct sdhc_retry {
	u32_t end;
	s16_t tries;
	u16_t sleep;
};

struct sdhc_flag_map {
	u8_t mask;
	u8_t err;
};

DEVICE_DECLARE(sdhc_0);

/* The SD protocol requires sending ones while reading but Zephyr
 * defaults to writing zeros.
 */
static const u8_t sdhc_ones[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

BUILD_ASSERT(sizeof(sdhc_ones) % SDHC_CSD_SIZE == 0);
BUILD_ASSERT(SDHC_SECTOR_SIZE % sizeof(sdhc_ones) == 0);

/* Maps R1 response flags to error codes */
static const struct sdhc_flag_map sdhc_r1_flags[] = {
	{SDHC_R1_PARAMETER, EFAULT},	   {SDHC_R1_ADDRESS, EFAULT},
	{SDHC_R1_ILLEGAL_COMMAND, EINVAL}, {SDHC_R1_COM_CRC, EILSEQ},
	{SDHC_R1_ERASE_SEQ, EIO},	  {SDHC_R1_ERASE_RESET, EIO},
	{SDHC_R1_IDLE, ECONNRESET},	{0, 0},
};

/* Maps disk status flags to error codes */
static const struct sdhc_flag_map sdhc_disk_status_flags[] = {
	{DISK_STATUS_UNINIT, ENODEV},
	{DISK_STATUS_NOMEDIA, ENOENT},
	{DISK_STATUS_WR_PROTECT, EROFS},
	{0, 0},
};

/* Maps data block flags to error codes */
static const struct sdhc_flag_map sdhc_data_response_flags[] = {
	{SDHC_RESPONSE_WRITE_ERR, EIO},
	{SDHC_RESPONSE_CRC_ERR, EILSEQ},
	{SDHC_RESPONSE_ACCEPTED, 0},
	/* Unrecognised value */
	{0, EPROTO},
};

/* Traces card traffic for LOG_LEVEL_DBG */
static int sdhc_trace(struct sdhc_data *data, int dir, int err,
		      const u8_t *buf, int len)
{
#if LOG_LEVEL >= LOG_LEVEL_DBG
	if (err != 0) {
		printk("(err=%d)", err);
		data->trace_dir = 0;
	}

	if (dir != data->trace_dir) {
		data->trace_dir = dir;

		printk("\n");

		if (dir == 1) {
			printk(">>");
		} else if (dir == -1) {
			printk("<<");
		}
	}

	for (; len != 0; len--) {
		printk(" %x", *buf++);
	}
#endif
	return err;
}

/* Returns true if an error code is retryable at the disk layer */
static bool sdhc_is_retryable(int err)
{
	switch (err) {
	case 0:
		return false;
	case -EILSEQ:
	case -EIO:
	case -ETIMEDOUT:
		return true;
	default:
		return false;
	}
}

/* Maps a flag based error code into a Zephyr errno */
static int sdhc_map_flags(const struct sdhc_flag_map *map, int flags)
{
	if (flags < 0) {
		return flags;
	}

	for (; map->mask != 0; map++) {
		if ((flags & map->mask) == map->mask) {
			return -map->err;
		}
	}

	return -map->err;
}

/* Converts disk status into an error code */
static int sdhc_map_disk_status(int status)
{
	return sdhc_map_flags(sdhc_disk_status_flags, status);
}

/* Converts the R1 response flags into an error code */
static int sdhc_map_r1_status(int status)
{
	return sdhc_map_flags(sdhc_r1_flags, status);
}

/* Converts an eary stage idle mode R1 code into an error code */
static int sdhc_map_r1_idle_status(int status)
{
	if (status < 0) {
		return status;
	}

	if (status == SDHC_R1_IDLE) {
		return 0;
	}

	return sdhc_map_r1_status(status);
}

/* Converts the data block response flags into an error code */
static int sdhc_map_data_status(int status)
{
	return sdhc_map_flags(sdhc_data_response_flags, status);
}

/* Initialises a retry helper */
static void sdhc_retry_init(struct sdhc_retry *retry, u32_t timeout,
			    u16_t sleep)
{
	retry->end = k_uptime_get_32() + timeout;
	retry->tries = 0;
	retry->sleep = sleep;
}

/* Called at the end of a retry loop.  Returns if the minimum try
 * count and timeout has passed.  Delays/yields on retry.
 */
static bool sdhc_retry_ok(struct sdhc_retry *retry)
{
	s32_t remain = retry->end - k_uptime_get_32();

	if (retry->tries < SDHC_MIN_TRIES) {
		retry->tries++;
		if (retry->sleep != 0) {
			k_sleep(retry->sleep);
		}

		return true;
	}

	if (remain >= 0) {
		if (retry->sleep > 0) {
			k_sleep(retry->sleep);
		} else {
			k_yield();
		}

		return true;
	}

	return false;
}

/* Asserts or deasserts chip select */
static void sdhc_set_cs(struct sdhc_data *data, int value)
{
	gpio_pin_write(data->cs, data->pin, value);
}

/* Receives a fixed number of bytes */
static int sdhc_rx_bytes(struct sdhc_data *data, u8_t *buf, int len)
{
	struct spi_buf tx_bufs[] = {
		{
			.buf = (u8_t *)sdhc_ones,
			.len = len
		}
	};

	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = 1,
	};

	struct spi_buf rx_bufs[] = {
		{
			.buf = buf,
			.len = len
		}
	};

	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = 1,
	};

	return sdhc_trace(data, -1,
			  spi_transceive(data->spi, &data->cfg, &tx, &rx),
			  buf, len);
}

/* Receives and returns a single byte */
static int sdhc_rx_u8(struct sdhc_data *data)
{
	u8_t buf[1];
	int err = sdhc_rx_bytes(data, buf, sizeof(buf));

	if (err != 0) {
		return err;
	}

	return buf[0];
}

/* Transmits a block of bytes */
static int sdhc_tx(struct sdhc_data *data, const u8_t *buf, int len)
{
	struct spi_buf spi_bufs[] = {
		{
			.buf = (u8_t *)buf,
			.len = len
		}
	};

	const struct spi_buf_set tx = {
		.buffers = spi_bufs,
		.count = 1
	};

	return sdhc_trace(data, 1, spi_write(data->spi, &data->cfg, &tx), buf,
			  len);
}

/* Transmits the command and payload */
static int sdhc_tx_cmd(struct sdhc_data *data, u8_t cmd, u32_t payload)
{
	u8_t buf[SDHC_CMD_SIZE];

	LOG_DBG("cmd%d payload=%u", cmd, payload);
	sdhc_trace(data, 0, 0, NULL, 0);

	/* Encode the command */
	buf[0] = SDHC_TX | (cmd & ~SDHC_START);
	sys_put_be32(payload, &buf[1]);
	buf[SDHC_CMD_BODY_SIZE] = crc7_be(0, buf, SDHC_CMD_BODY_SIZE);

	return sdhc_tx(data, buf, sizeof(buf));
}

/* Reads until anything but `discard` is received */
static int sdhc_skip(struct sdhc_data *data, int discard)
{
	int err;
	struct sdhc_retry retry;

	sdhc_retry_init(&retry, SDHC_READY_TIMEOUT, 0);

	do {
		err = sdhc_rx_u8(data);
		if (err != discard) {
			return err;
		}
	} while (sdhc_retry_ok(&retry));

	LOG_WRN("Timeout while waiting for !%d", discard);
	return -ETIMEDOUT;
}

/* Reads until the first byte in a response is received */
static int sdhc_skip_until_start(struct sdhc_data *data)
{
	struct sdhc_retry retry;
	int status;

	sdhc_retry_init(&retry, SDHC_READY_TIMEOUT, 0);

	do {
		status = sdhc_rx_u8(data);
		if (status < 0) {
			return status;
		}

		if ((status & SDHC_START) == 0) {
			return status;
		}
	} while (sdhc_retry_ok(&retry));

	return -ETIMEDOUT;
}

/* Reads until the bus goes high */
static int sdhc_skip_until_ready(struct sdhc_data *data)
{
	struct sdhc_retry retry;
	int status;

	sdhc_retry_init(&retry, SDHC_READY_TIMEOUT, 0);

	do {
		status = sdhc_rx_u8(data);
		if (status < 0) {
			return status;
		}

		if (status == 0) {
			/* Card is still busy */
			continue;
		}

		if (status == 0xFF) {
			return 0;
		}

		/* Got something else.	Some cards release MISO part
		 * way through the transfer.  Read another and see if
		 * MISO went high.
		 */
		status = sdhc_rx_u8(data);
		if (status < 0) {
			return status;
		}

		if (status == 0xFF) {
			return 0;
		}

		return -EPROTO;
	} while (sdhc_retry_ok(&retry));

	return -ETIMEDOUT;
}

/* Sends a command and returns the received R1 status code */
static int sdhc_cmd_r1_raw(struct sdhc_data *data, u8_t cmd, u32_t payload)
{
	int err;

	err = sdhc_tx_cmd(data, cmd, payload);
	if (err != 0) {
		return err;
	}

	err = sdhc_skip_until_start(data);

	/* Ensure there's a idle byte between commands */
	sdhc_rx_u8(data);

	return err;
}

/* Sends a command and returns the mapped error code */
static int sdhc_cmd_r1(struct sdhc_data *data, u8_t cmd, uint32_t payload)
{
	return sdhc_map_r1_status(sdhc_cmd_r1_raw(data, cmd, payload));
}

/* Sends a command in idle mode returns the mapped error code */
static int sdhc_cmd_r1_idle(struct sdhc_data *data, u8_t cmd,
			    uint32_t payload)
{
	return sdhc_map_r1_idle_status(sdhc_cmd_r1_raw(data, cmd, payload));
}

/* Sends a command and returns the received multi-byte R2 status code */
static int sdhc_cmd_r2(struct sdhc_data *data, u8_t cmd, uint32_t payload)
{
	int err;
	int r1;
	int r2;

	err = sdhc_tx_cmd(data, cmd, payload);
	if (err != 0) {
		return err;
	}

	r1 = sdhc_map_r1_status(sdhc_skip_until_start(data));
	/* Always read the rest of the reply */
	r2 = sdhc_rx_u8(data);

	/* Ensure there's a idle byte between commands */
	sdhc_rx_u8(data);

	if (r1 < 0) {
		return r1;
	}

	return r2;
}

/* Sends a command and returns the received multi-byte status code */
static int sdhc_cmd_r37_raw(struct sdhc_data *data, u8_t cmd, u32_t payload,
			    u32_t *reply)
{
	int err;
	int status;
	u8_t buf[sizeof(*reply)];

	err = sdhc_tx_cmd(data, cmd, payload);
	if (err != 0) {
		return err;
	}

	status = sdhc_skip_until_start(data);

	/* Always read the rest of the reply */
	err = sdhc_rx_bytes(data, buf, sizeof(buf));
	*reply = sys_get_be32(buf);

	/* Ensure there's a idle byte between commands */
	sdhc_rx_u8(data);

	if (err != 0) {
		return err;
	}

	return status;
}

/* Sends a command in idle mode returns the mapped error code */
static int sdhc_cmd_r7_idle(struct sdhc_data *data, u8_t cmd, u32_t payload,
			    u32_t *reply)
{
	return sdhc_map_r1_idle_status(
		sdhc_cmd_r37_raw(data, cmd, payload, reply));
}

/* Sends a command and returns the received multi-byte R3 error code */
static int sdhc_cmd_r3(struct sdhc_data *data, u8_t cmd, uint32_t payload,
		       u32_t *reply)
{
	return sdhc_map_r1_status(
		sdhc_cmd_r37_raw(data, cmd, payload, reply));
}

/* Receives a SDHC data block */
static int sdhc_rx_block(struct sdhc_data *data, u8_t *buf, int len)
{
	int err;
	int token;
	int i;
	/* Note the one extra byte to ensure there's an idle byte
	 * between commands.
	 */
	u8_t crc[SDHC_CRC16_SIZE + 1];

	token = sdhc_skip(data, 0xFF);
	if (token < 0) {
		return token;
	}

	if (token != SDHC_TOKEN_SINGLE) {
		/* No start token */
		return -EIO;
	}

	/* Read the data in batches */
	for (i = 0; i < len; i += sizeof(sdhc_ones)) {
		int remain = min(sizeof(sdhc_ones), len - i);

		struct spi_buf tx_bufs[] = {
			{
				.buf = (u8_t *)sdhc_ones,
				.len = remain
			}
		};

		const struct spi_buf_set tx = {
			.buffers = tx_bufs,
			.count = 1,
		};

		struct spi_buf rx_bufs[] = {
			{
				.buf = &buf[i],
				.len = remain
			}
		};

		const struct spi_buf_set rx = {
			.buffers = rx_bufs,
			.count = 1,
		};

		err = sdhc_trace(data, -1, spi_transceive(data->spi, &data->cfg,
							  &tx, &rx),
				 &buf[i], remain);
		if (err != 0) {
			return err;
		}
	}

	err = sdhc_rx_bytes(data, crc, sizeof(crc));
	if (err != 0) {
		return err;
	}

	if (sys_get_be16(crc) != crc16_itu_t(0, buf, len)) {
		/* Bad CRC */
		return -EILSEQ;
	}

	return 0;
}

/* Transmits a SDHC data block */
static int sdhc_tx_block(struct sdhc_data *data, u8_t *send, int len)
{
	u8_t buf[SDHC_CRC16_SIZE];
	int err;

	/* Start the block */
	buf[0] = SDHC_TOKEN_SINGLE;
	err = sdhc_tx(data, buf, 1);
	if (err != 0) {
		return err;
	}

	/* Write the payload */
	err = sdhc_tx(data, send, len);
	if (err != 0) {
		return err;
	}

	/* Build and write the trailing CRC */
	sys_put_be16(crc16_itu_t(0, send, len), buf);

	err = sdhc_tx(data, buf, sizeof(buf));
	if (err != 0) {
		return err;
	}

	return sdhc_map_data_status(sdhc_rx_u8(data));
}

static int sdhc_recover(struct sdhc_data *data)
{
	/* TODO(nzmichaelh): implement */
	return sdhc_cmd_r1(data, SDHC_SEND_STATUS, 0);
}

/* Attempts to return the card to idle mode */
static int sdhc_go_idle(struct sdhc_data *data)
{
	sdhc_set_cs(data, 1);

	/* Write the initial >= 74 clocks */
	sdhc_tx(data, sdhc_ones, 10);

	sdhc_set_cs(data, 0);

	return sdhc_cmd_r1_idle(data, SDHC_GO_IDLE_STATE, 0);
}

/* Checks the supported host volatage and basic protocol */
static int sdhc_check_card(struct sdhc_data *data)
{
	u32_t cond;
	int err;

	/* Check that the current voltage is supported */
	err = sdhc_cmd_r7_idle(data, SDHC_SEND_IF_COND,
			       SDHC_VHS_3V3 | SDHC_CHECK, &cond);
	if (err != 0) {
		return err;
	}

	if ((cond & 0xFF) != SDHC_CHECK) {
		/* Card returned a different check pattern */
		return -ENOENT;
	}

	if ((cond & SDHC_VHS_MASK) != SDHC_VHS_3V3) {
		/* Card doesn't support this voltage */
		return -ENOTSUP;
	}

	return 0;
}

/* Detect and initialise the card */
static int sdhc_detect(struct sdhc_data *data)
{
	int err;
	u32_t ocr;
	struct sdhc_retry retry;
	u8_t structure;
	u32_t csize;
	u8_t buf[SDHC_CSD_SIZE];

	data->cfg.frequency = SDHC_INITIAL_SPEED;
	data->status = DISK_STATUS_UNINIT;

	sdhc_retry_init(&retry, SDHC_INIT_TIMEOUT, SDHC_RETRY_DELAY);

	/* Synchronise with the card by sending it to idle */
	do {
		err = sdhc_go_idle(data);
		if (err == 0) {
			err = sdhc_check_card(data);
			if (err == 0) {
				break;
			}
		}

		if (!sdhc_retry_ok(&retry)) {
			return -ENOENT;
		}
	} while (true);

	/* Enable CRC mode */
	err = sdhc_cmd_r1_idle(data, SDHC_CRC_ON_OFF, 1);
	if (err != 0) {
		return err;
	}

	/* Wait for the card to leave idle state */
	do {
		sdhc_cmd_r1_raw(data, SDHC_APP_CMD, 0);

		err = sdhc_cmd_r1(data, SDHC_SEND_OP_COND, SDHC_HCS);
		if (err == 0) {
			break;
		}
	} while (sdhc_retry_ok(&retry));

	if (err != 0) {
		/* Card never exited idle */
		return -ETIMEDOUT;
	}

	/* Read OCR and confirm this is a SDHC card */
	err = sdhc_cmd_r3(data, SDHC_READ_OCR, 0, &ocr);
	if (err != 0) {
		return err;
	}

	if ((ocr & SDHC_CCS) == 0) {
		/* A 'SDSC' card */
		return -ENOTSUP;
	}

	/* Read the CSD */
	err = sdhc_cmd_r1(data, SDHC_SEND_CSD, 0);
	if (err != 0) {
		return err;
	}

	err = sdhc_rx_block(data, buf, sizeof(buf));
	if (err != 0) {
		return err;
	}

	/* Bits 126..127 are the structure version */
	structure = (buf[0] >> 6);
	if (structure != SDHC_CSD_V2) {
		/* Unsupported CSD format */
		return -ENOTSUP;
	}

	/* Bits 48..69 are the capacity of the card in 512 KiB units, minus 1.
	 */
	csize = sys_get_be32(&buf[6]) & ((1 << 22) - 1);
	if (csize < 4112) {
		/* Invalid capacity according to section 5.3.3 */
		return -ENOTSUP;
	}

	data->sector_count = (csize + 1) * (512 * 1024 / SDHC_SECTOR_SIZE);

	LOG_INF("Found a ~%u MiB SDHC card.",
		data->sector_count / (1024 * 1024 / SDHC_SECTOR_SIZE));

	/* Read the CID */
	err = sdhc_cmd_r1(data, SDHC_SEND_CID, 0);
	if (err != 0) {
		return err;
	}

	err = sdhc_rx_block(data, buf, sizeof(buf));
	if (err != 0) {
		return err;
	}

	LOG_INF("Manufacturer ID=%d OEM='%c%c' Name='%c%c%c%c%c' "
		"Revision=0x%x Serial=0x%x",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6],
		buf[7], buf[8], sys_get_be32(&buf[9]));

	/* Initilisation complete */
	data->cfg.frequency = SDHC_SPEED;
	data->status = DISK_STATUS_OK;

	return 0;
}

static int sdhc_read(struct sdhc_data *data, u8_t *buf, u32_t sector,
		     u32_t count)
{
	int err;

	err = sdhc_map_disk_status(data->status);
	if (err != 0) {
		return err;
	}

	sdhc_set_cs(data, 0);

	/* Send the start read command */
	err = sdhc_cmd_r1(data, SDHC_READ_MULTIPLE_BLOCK, sector);
	if (err != 0) {
		goto error;
	}

	/* Read the sectors */
	for (; count != 0; count--) {
		err = sdhc_rx_block(data, buf, SDHC_SECTOR_SIZE);
		if (err != 0) {
			goto error;
		}

		buf += SDHC_SECTOR_SIZE;
	}

	/* Ignore the error as STOP_TRANSMISSION always returns 0x7F */
	sdhc_cmd_r1(data, SDHC_STOP_TRANSMISSION, 0);

	/* Wait until the card becomes ready */
	err = sdhc_skip_until_ready(data);

error:
	sdhc_set_cs(data, 1);

	return err;
}

static int sdhc_write(struct sdhc_data *data, const u8_t *buf, u32_t sector,
		      u32_t count)
{
	int err;

	err = sdhc_map_disk_status(data->status);
	if (err != 0) {
		return err;
	}

	sdhc_set_cs(data, 0);

	/* Write the blocks one-by-one */
	for (; count != 0; count--) {
		err = sdhc_cmd_r1(data, SDHC_WRITE_BLOCK, sector);
		if (err < 0) {
			goto error;
		}

		err = sdhc_tx_block(data, (u8_t *)buf, SDHC_SECTOR_SIZE);
		if (err != 0) {
			goto error;
		}

		/* Wait for the card to finish programming */
		err = sdhc_skip_until_ready(data);
		if (err != 0) {
			goto error;
		}

		err = sdhc_cmd_r2(data, SDHC_SEND_STATUS, 0);
		if (err != 0) {
			goto error;
		}

		buf += SDHC_SECTOR_SIZE;
		sector++;
	}

	err = 0;
error:
	sdhc_set_cs(data, 1);

	return err;
}

static int disk_sdhc_init(struct device *dev);

static int sdhc_init(struct device *dev)
{
	struct sdhc_data *data = dev->driver_data;

	data->spi = device_get_binding(DT_ZEPHYR_MMC_SPI_SLOT_0_BUS_NAME);

	data->cfg.frequency = SDHC_INITIAL_SPEED;
	data->cfg.operation = SPI_WORD_SET(8) | SPI_HOLD_ON_CS;
	data->cfg.slave = DT_ZEPHYR_MMC_SPI_SLOT_0_BASE_ADDRESS;
	data->cs = device_get_binding(DT_ZEPHYR_MMC_SPI_SLOT_0_CS_GPIO_CONTROLLER);
	__ASSERT_NO_MSG(data->cs != NULL);

	data->pin = DT_ZEPHYR_MMC_SPI_SLOT_0_CS_GPIO_PIN;

	disk_sdhc_init(dev);

	return gpio_pin_configure(data->cs, data->pin, GPIO_DIR_OUT);
}

static struct device *sdhc_get_device(void) { return DEVICE_GET(sdhc_0); }

static int disk_sdhc_access_status(struct disk_info *disk)
{
	struct device *dev = sdhc_get_device();
	struct sdhc_data *data = dev->driver_data;

	return data->status;
}

static int disk_sdhc_access_read(struct disk_info *disk, u8_t *buf,
				 u32_t sector, u32_t count)
{
	struct device *dev = sdhc_get_device();
	struct sdhc_data *data = dev->driver_data;
	int err;

	LOG_DBG("sector=%u count=%u", sector, count);

	err = sdhc_read(data, buf, sector, count);
	if (err != 0 && sdhc_is_retryable(err)) {
		sdhc_recover(data);
		err = sdhc_read(data, buf, sector, count);
	}

	return err;
}

static int disk_sdhc_access_write(struct disk_info *disk, const u8_t *buf,
				  u32_t sector, u32_t count)
{
	struct device *dev = sdhc_get_device();
	struct sdhc_data *data = dev->driver_data;
	int err;

	LOG_DBG("sector=%u count=%u", sector, count);

	err = sdhc_write(data, buf, sector, count);
	if (err != 0 && sdhc_is_retryable(err)) {
		sdhc_recover(data);
		err = sdhc_write(data, buf, sector, count);
	}

	return err;
}

static int disk_sdhc_access_ioctl(struct disk_info *disk, u8_t cmd, void *buf)
{
	struct device *dev = sdhc_get_device();
	struct sdhc_data *data = dev->driver_data;
	int err;

	err = sdhc_map_disk_status(data->status);
	if (err != 0) {
		return err;
	}

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(u32_t *)buf = data->sector_count;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(u32_t *)buf = SDHC_SECTOR_SIZE;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(u32_t *)buf = SDHC_SECTOR_SIZE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int disk_sdhc_access_init(struct disk_info *disk)
{
	struct device *dev = sdhc_get_device();
	struct sdhc_data *data = dev->driver_data;
	int err;

	if (data->status == DISK_STATUS_OK) {
		/* Called twice, don't re-init. */
		return 0;
	}

	err = sdhc_detect(data);
	sdhc_set_cs(data, 1);

	return err;
}

static const struct disk_operations sdhc_disk_ops = {
	.init = disk_sdhc_access_init,
	.status = disk_sdhc_access_status,
	.read = disk_sdhc_access_read,
	.write = disk_sdhc_access_write,
	.ioctl = disk_sdhc_access_ioctl,
};

static struct disk_info sdhc_disk = {
	.name = CONFIG_DISK_SDHC_VOLUME_NAME,
	.ops = &sdhc_disk_ops,
};

static int disk_sdhc_init(struct device *dev)
{
	struct sdhc_data *data = dev->driver_data;

	data->status = DISK_STATUS_UNINIT;

	return disk_access_register(&sdhc_disk);
}

static struct sdhc_data sdhc_data_0;

DEVICE_AND_API_INIT(sdhc_0, "sdhc_0", sdhc_init, &sdhc_data_0, NULL,
		    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

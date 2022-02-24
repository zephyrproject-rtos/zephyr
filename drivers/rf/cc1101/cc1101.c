/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc1101

#define LOG_MODULE_NAME cc1101
#define LOG_LEVEL CONFIG_RF_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <kernel.h>
#include <device.h>
#include <debug/stack.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <sys/util.h>

#include <drivers/rf.h>
#include "cc1101.h"

BUILD_ASSERT(CONFIG_CC1101_MAX_PACKET_SIZE <= 253,
		"CC1101 packet size must be <= 253");
BUILD_ASSERT(CONFIG_CC1101_MAX_PACKET_SIZE <= CONFIG_RF_MAX_PACKET_SIZE,
		"CC1101 packet size must be <= RF MAX PACKET SIZE");

/*
 * Content is split as follows:
 *
 * - SPI communication
 * - Helper functions
 * - Debug related functions
 * - RF device API functions
 * - RX Thread
 * - GPIO related functions
 * - Initialization
 */


/*
 * SPI communication
 */

int cc1101_reg_read(const struct device *dev,
		uint8_t reg,
		uint8_t *buf)
{
	int ret;
	uint8_t addr;
	const struct cc1101_config *cfg = dev->config;
	struct cc1101_data *data = dev->data;

	const struct spi_buf tx_buf = {
		.buf = &addr,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	struct spi_buf rx_buf[2];
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf)
	};

	addr = reg | CC1101_SPI_READ;

	rx_buf[0].buf = &data->status.reg;
	rx_buf[0].len = 1;

	rx_buf[1].buf = buf;
	rx_buf[1].len = 1;

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret) {
		LOG_ERR("SPI read failed: %d\n", ret);
		return ret;
	}

	return ret;
}

int cc1101_reg_write(const struct device *dev,
		uint8_t addr,
		uint8_t val)
{
	int ret;
	const struct cc1101_config *cfg = dev->config;
	uint8_t cmd[] = { (addr | CC1101_SPI_WRITE), val };
	const struct spi_buf tx_buf = {
		.buf = cmd,
		.len = sizeof(cmd)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct cc1101_data *data = dev->data;
	const struct spi_buf rx_buf = {
		.buf = &data->status.reg,
		.len = 1
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret) {
		LOG_ERR("SPI write failed: %d\n", ret);
		return ret;
	}
	return ret;
}


int cc1101_burst_read(const struct device *dev,
		uint8_t start,
		uint8_t *buf,
		int size)
{
	int ret;
	uint8_t addr;
	const struct cc1101_config *cfg = dev->config;
	struct cc1101_data *data = dev->data;
	const struct spi_buf tx_buf = {
		.buf = &addr,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf[2];
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf)
	};

	addr = start | CC1101_SPI_BURST_READ;

	rx_buf[0].buf = &data->status.reg;
	rx_buf[0].len = 1;

	rx_buf[1].buf = buf;
	rx_buf[1].len = size;

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret) {
		LOG_ERR("SPI burst read failed: %d\n", ret);
		return ret;
	}

	return ret;
}

int cc1101_burst_write(const struct device *dev,
		uint8_t start,
		uint8_t *buf,
		int size)
{
	int ret;
	uint8_t addr;
	const struct cc1101_config *cfg = dev->config;
	struct spi_buf tx_buf[2];
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf)
	};
	struct cc1101_data *data = dev->data;
	const struct spi_buf rx_buf = {
		.buf = &data->status.reg,
		.len = 1
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	addr = start | CC1101_SPI_BURST_WRITE;

	tx_buf[0].buf = &addr;
	tx_buf[0].len = 1;

	tx_buf[1].buf = buf;
	tx_buf[1].len = size;

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret) {
		LOG_ERR("SPI burst write failed: %d\n", ret);
		return ret;
	}
	return ret;
}

int cc1101_strobe(const struct device *dev, uint8_t addr)
{
	int ret;
	const struct cc1101_config *cfg = dev->config;
	uint8_t strobe[] = { (addr | CC1101_SPI_WRITE) };
	const struct spi_buf tx_buf = {
		.buf = strobe,
		.len = sizeof(strobe)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct cc1101_data *data = dev->data;
	const struct spi_buf rx_buf = {
		.buf = &data->status.reg,
		.len = 1
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret) {
		LOG_ERR("SPI strobe failed: %d\n", ret);
		return ret;
	}
	return ret;
}


/*
 * Helper functions
 */

static inline int cc1101_status_bytes(const struct device *dev)
{
	const struct cc1101_data *data = dev->data;

	if (data->config.pktctrl1.APPEND_STATUS) {
		return 2;
	} else {
		return 0;
	}
}

static inline void cc1101_wait_ready(const struct device *dev)
{
	const struct cc1101_data *data = dev->data;
	while(gpio_pin_get_dt(&data->gdo2)) {
		k_usleep(CONFIG_CC1101_WAIT_INTERVAL_US);
	};
}

static inline int cc1101_wait_state(const struct device *dev, enum cc1101_state state)
{
	struct cc1101_data *data = dev->data;
	uint8_t retry = CONFIG_CC1101_WAIT_RETRIES;
	int ret = 0;
	while((retry > 0) && (data->status.STATE != state)) {
		k_usleep(CONFIG_CC1101_WAIT_INTERVAL_US);
		ret = cc1101_strobe(dev, CC1101_SNOP_ADDR);
		if (ret) {
			return ret;
		};
		retry--;
	}
	/*
	 * TODO: negative error code when waiting fails
	 * -> check return value where this function is used and
	 *  act accordingly
	 */
	//printk("state: %s\n", cc1101_status2str(data->status.STATE));

	return ret;
}

static inline int cc1101_rx_data(const struct device *dev)
{
	int ret;
	struct cc1101_data *data = dev->data;

	ret = cc1101_burst_read(dev, CC1101_RXBYTES_ADDR, &data->rxbytes.reg, 1);
	if (ret) {
		return ret;
	};

	return ret;
}


/*
 * Debug functions
 */

#if LOG_LEVEL == LOG_LEVEL_DBG
static int cc1101_print_status(const struct device *dev)
{
	int ret;
	struct cc1101_data *data = dev->data;
	union CC1101_TXBYTES tx;
	union CC1101_RXBYTES rx;
	struct k_msgq_attrs rx_attrs;

	k_msgq_get_attrs(&data->rxq, &rx_attrs);

	ret = cc1101_burst_read(dev, CC1101_TXBYTES_ADDR, &tx.reg, 1);
	if (ret) {
		return ret;
	};

	ret = cc1101_burst_read(dev, CC1101_RXBYTES_ADDR, &rx.reg, 1);
	if (ret) {
		return ret;
	};

	ret = cc1101_strobe(dev, CC1101_SNOP_ADDR);
	if (ret) {
		return ret;
	};

	log_stack_usage(&data->rx_thread);

	LOG_DBG("ready ? %s -- state: %s -- fifo(tx): %d -- fifo(rx): %d",
			data->status.CHIP_RDY ? "false" : "true",
			cc1101_status2str(data->status.STATE),
			tx.NUM_TXBYTES, rx.NUM_RXBYTES);
	LOG_DBG("RXQUEUE: size: %d -- waiting: %d -- free: %d",
			rx_attrs.max_msgs,
			rx_attrs.used_msgs,
			(rx_attrs.max_msgs - rx_attrs.used_msgs));

	return ret;
}

static inline void cc1101_pkt_dump(const char *str,
			       const uint8_t *packet, size_t length)
{
	if (!length) {
		LOG_DBG("%s zero-length packet", str);
		return;
	}

	LOG_HEXDUMP_DBG(packet, length, str);
}
#else
#define cc1101_print_status(...)
#define cc1101_pkt_dump(...)
#endif /* LOG_LEVEL == LOG_LEVEL_DBG */


/*
 * RF API functions
 */

static int cc1101_set_config(const struct device *dev, uint8_t *cfg)
{
	struct cc1101_data *data = dev->data;
	int ret;
	uint8_t max_pkt_bytes;

	if ((uint8_t *)&data->config != cfg) {
		memcpy(&data->config, cfg, CC1101_NUM_CFG_REG);
	}

	max_pkt_bytes = data->config.pktlen + cc1101_status_bytes(dev);

	if (data->config.pktlen > CONFIG_CC1101_MAX_PACKET_SIZE) {
		LOG_ERR("PKTLEN (%d) larger than max allowed size (%d).",
				data->config.pktlen, CONFIG_CC1101_MAX_PACKET_SIZE);
		return -ENOTSUP;
	};

	if (data->config.pktctrl1.CRC_AUTOFLUSH && max_pkt_bytes > CC1101_FIFO_SIZE) {
		LOG_ERR("CRC_AUTOFLUSH impossible with packet size  %d+%d > %d.",
				data->config.pktlen, cc1101_status_bytes(dev), CC1101_FIFO_SIZE);
		return -ENOTSUP;
	};

	if (data->config.pktctrl0.PKT_FORMAT != CC1101_ALLOWED_PKT_FORMAT) {
		LOG_ERR("Only FIFO interface is supported.");
		return -ENOTSUP;
	};

	if (data->config.pktctrl0.LENGTH_CONFIG != CC1101_ALLOWED_PKT_LEN_MODE) {
		LOG_ERR("Only variable packet length mode supported.");
		return -ENOTSUP;
	};

	ret = cc1101_burst_write(dev, CC1101_IOCFG2_ADDR, (uint8_t *) &data->config, CC1101_NUM_CFG_REG);

	return ret;
}

static int cc1101_set_mode(const struct device *dev, enum rf_op_mode mode)
{
	int ret;

	switch(mode)
	{
	case RF_MODE_CALIBRATE:
		ret = cc1101_reg_write(dev, CC1101_IOCFG2_ADDR, CC1101_IOCFG_CHIP_RDY);
		if (ret) {
			return ret;
		};
		ret = cc1101_strobe(dev, CC1101_SIDLE_ADDR);
		if (ret) {
			return ret;
		};
		ret = cc1101_wait_state(dev, CC1101_STATE_IDLE);
		if (ret) {
			return ret;
		};
		ret = cc1101_strobe(dev, CC1101_SCAL_ADDR);
		if (ret) {
			return ret;
		};
		ret = cc1101_wait_state(dev, CC1101_STATE_IDLE);
		if (ret) {
			return ret;
		};
		break;
	case RF_MODE_POWER_OFF:
		ret = -ENOSYS;
		break;
	case RF_MODE_SLEEP:
		ret = -ENOSYS;
		break;
	case RF_MODE_IDLE:
		ret = cc1101_reg_write(dev, CC1101_IOCFG2_ADDR, CC1101_IOCFG_CHIP_RDY);
		if (ret) {
			return ret;
		};
		ret = cc1101_strobe(dev, CC1101_SIDLE_ADDR);
		if (ret) {
			return ret;
		};
		ret = cc1101_wait_state(dev, CC1101_STATE_IDLE);
		if (ret) {
			return ret;
		};
		break;
	case RF_MODE_RX_WAKE_ON_EVENT:
		ret = -ENOSYS;
		break;
	case RF_MODE_RX_WAKE_PERIODIC:
		ret = -ENOSYS;
		break;
	case RF_MODE_RX:
		ret = cc1101_strobe(dev, CC1101_SRX_ADDR);
		if (ret) {
			return ret;
		};
		ret = cc1101_wait_state(dev, CC1101_STATE_RX);
		if (ret) {
			return ret;
		};
		ret = cc1101_reg_write(dev, CC1101_IOCFG2_ADDR, CC1101_IOCFG_RX);
		if (ret) {
			return ret;
		};
		break;
	case RF_MODE_TX:
		ret = cc1101_strobe(dev, CC1101_STX_ADDR);
		if (ret) {
			return ret;
		};
		ret = cc1101_wait_state(dev, CC1101_STATE_TX);
		if (ret) {
			return ret;
		};
		ret = cc1101_reg_write(dev, CC1101_IOCFG2_ADDR, CC1101_IOCFG_TX);
		if (ret) {
			return ret;
		};
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int cc1101_device_set(const struct device *dev,
				enum rf_device_arg arg,
				void *val)
{
	int ret;
	struct cc1101_data *data = dev->data;

	switch(arg)
	{
	case RF_DEVICE_FREQUENCY:
		/*
		 * Datasheet: 21 Frequency Programming
		 */
		ret = -ENOSYS;
		//ret = cc1101_set_frequency(dev, *(rf_freq_t *)val);
		break;
	case RF_DEVICE_CHANNEL:
		/**
		 * TODO
		 * No sanity checking is done..
		 * the argument will/should be of type rf_chan_t aka. uint16_t
		 */
		ret = cc1101_reg_write(dev, CC1101_CHANNR_ADDR, *(uint8_t *)val);
		//ret = cc1101_set_channel(dev, *(rf_chan_t *)val);
		break;
	case RF_DEVICE_MODULATION_FORMAT:
		/**
		 * Datasheet: 16 Modulation Formats
		 */
		ret = -ENOSYS;
		//ret = cc1101_set_mod_format(dev, *(enum rf_mod_format *)val);
		break;
	case RF_DEVICE_BAUDRATE:
		/*
		 * TODO: do it!
		 * Datasheet: 12 Data Rate Programming
		 */
		ret = -ENOSYS;
		//ret = cc1101_set_baudrate(dev, *(rf_baud_t *)val);
		break;
	case RF_DEVICE_OUTPUT_POWER:
		/*
		 * TODO: do it!
		 * Datasheet: 24 Output Power Programming
		 */
		ret = -ENOSYS;
		//ret = cc1101_set_output_power(dev, *(uint16_t *)val);
		break;
	case RF_DEVICE_OPERATING_MODE:
		ret = cc1101_set_mode(dev, *(enum rf_op_mode *)val);
		break;
	case RF_DEVICE_SETTINGS:
		ret = cc1101_set_config(dev, (uint8_t *)val);
		break;
	case RF_DEVICE_CALIBRATION_SETTINGS:
		ret = -ENOSYS;
		break;
	case RF_DEVICE_POWER_TABLE:
		if ((uint8_t *)&data->patable != (uint8_t *)val) {
			memcpy(&data->patable, (uint8_t *)val, CC1101_NUM_CFG_REG);
		}

		ret = cc1101_burst_write(dev,
				CC1101_PATABLE_ADDR,
				(uint8_t *)&data->patable,
				CC1101_NUM_PATABLE);
		break;
	case RF_DEVICE_SET_EVENT_CB:
		data->event_cb = (rf_event_cb_t)val;
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int cc1101_send(const struct device *dev, union rf_packet *pkt)
{
	int ret;
	struct cc1101_data *data = dev->data;
	int bytes_remaining = pkt->length + 1; /* +1 for length byte */
	union CC1101_TXBYTES tx;
	uint8_t * pp = (uint8_t *)pkt;

	if ((pkt->length == 0) || (pkt->length > data->config.pktlen)) {
		LOG_ERR("Illegal packet length!");
		return -EINVAL;
	}

	if (k_mutex_lock(&data->xfer_lock, K_MSEC(10)) != 0) {
		LOG_DBG("Busy: RX in progress.");
		return -EBUSY;
	}

	cc1101_pkt_dump("CC1101 sending packet", (uint8_t *)pkt, bytes_remaining);

	if (cc1101_reg_write(dev, CC1101_IOCFG2_ADDR, CC1101_IOCFG_TX) ||
			cc1101_strobe(dev, CC1101_SFSTXON_ADDR) ||
			cc1101_wait_state(dev, CC1101_STATE_FSTXON)) {
		goto error;
	}

	if (bytes_remaining <= CC1101_FIFO_SIZE) {
		ret = cc1101_burst_write(dev, CC1101_TX_FIFO_ADDR,
					pp, bytes_remaining);
		if (ret) {
			goto error;
		} else {
			cc1101_set_mode(dev, RF_MODE_TX);
			goto out;
		}
	} else {
		ret = cc1101_burst_write(dev, CC1101_TX_FIFO_ADDR,
					pp, CC1101_FIFO_SIZE);
		if (ret) {
			goto error;
		}

		bytes_remaining -= CC1101_FIFO_SIZE;
		pp += CC1101_FIFO_SIZE;
		cc1101_set_mode(dev, RF_MODE_TX);

		while (bytes_remaining > 0) {
			k_sem_take(&data->fifo_cont, K_FOREVER);

			if (bytes_remaining < CC1101_FIFO_HWM) {
				ret = cc1101_burst_write(dev, CC1101_TX_FIFO_ADDR,
							pp, bytes_remaining);
				if (ret) {
					goto error;
				}
				goto out;
			} else {
				ret = cc1101_burst_write(dev, CC1101_TX_FIFO_ADDR,
							pp, CC1101_FIFO_HWM);
				if (ret) {
					goto error;
				}
			}
			bytes_remaining -= CC1101_FIFO_HWM;
			pp += CC1101_FIFO_HWM;
		}
	}

	goto out;
error:
	/*
	 * TODO: better error checking/handling?
	 * -> move error tag to bottom -> do everything to avoid stuck receiver
	 * -> return -EIO
	 */
	LOG_ERR("Transmission failed");
out:
	k_sem_take(&data->tx_done, K_MSEC(1000));

	ret = cc1101_burst_read(dev, CC1101_TXBYTES_ADDR, &tx.reg, 1);
	if (ret) {
		return ret;
	};

	if (tx.TXFIFO_UNDERFLOW) {
		LOG_WRN("TX FIFO underflow -> flushing.");
		ret = cc1101_strobe(dev, CC1101_SFTX_ADDR);
		if (ret) {
			return ret;
		};
		ret = cc1101_set_mode(dev, RF_MODE_RX);
		if (ret) {
			return ret;
		};
	}

	ret = cc1101_reg_write(dev, CC1101_IOCFG2_ADDR, CC1101_IOCFG_CHIP_RDY);
	if (ret) {
		return ret;
	}

	ret = cc1101_set_mode(dev, data->initial_mode);
	if (ret) {
		LOG_ERR("Could not resume to initial mode: %s.", cc1101_status2str(data->initial_mode));
		return ret;
	}

	k_mutex_unlock(&data->xfer_lock);

	if (data->event_cb) {
		data->event_cb(dev, RF_EVENT_SEND_DONE, NULL);
	}

	return ret;
}

static int cc1101_recv(const struct device *dev, union rf_packet *pkt)
{
	struct cc1101_data *data = dev->data;
	int ret;

	ret = k_msgq_get(&data->rxq, (union cc1101_data_item *)pkt, K_NO_WAIT);
	if(ret < 0) {
		return ret;
	}

	return pkt->length;
}


/*
 * RX thread
 */

static void cc1101_rx_thread(void *device, void *p2, void *p3)
{
	int ret;
	const struct device *dev = (const struct device *)device;
	struct cc1101_data *data = dev->data;
	union cc1101_data_item pkt;
	uint8_t status_bytes;
	int bytes_remaining;
	uint16_t retries;

	while(1) {
		if (data->event_cb) {
			data->event_cb(dev, RF_EVENT_RECV_READY, NULL);
		}
		k_sem_take(&data->rx_data_available, K_FOREVER);
		k_mutex_lock(&data->xfer_lock, K_FOREVER);
		uint8_t *pp = (uint8_t *)&pkt;

		status_bytes = cc1101_status_bytes(dev);

		/**
		 * CC1101 Silicon Errata (SWRZ020E):
		 * One should never empty the RX FIFO
		 * before the last byte of the packet is reached.
		 *
		 * Wait for up to 30 ms.
		 * With 1.2 kBaud GFSK it takes ~14ms for two bytes to arrive.
		 * With 500 kBaud MSK it takes ~130us for two bytes to arrive.
		 */
		retries = 300;
		do {
			cc1101_rx_data(dev);
			if (data->rxbytes.RXFIFO_OVERFLOW) {
				LOG_DBG("Datarate of the transmitter might be too high.");
				goto flush;
			}
			retries--;
			if (retries == 0) {
				if (data->rxbytes.NUM_RXBYTES == 0) {
					LOG_DBG("Packed dropped by hardware filter.");
					goto out;
				}
				LOG_DBG("Did not receive any data after waiting for 30ms.");
				goto flush;
			}
			k_busy_wait(100); /* TODO sleep vs busy_wait ?? */
		} while(data->rxbytes.NUM_RXBYTES < 2);

		ret = cc1101_reg_read(dev, CC1101_RX_FIFO_ADDR, pp);
		if (ret) {
			goto flush;
		};

		/* TODO pkt.length > CONFIG_CC1101_MAX_PACKET_SIZE  might be redundant (is it *always* HW filtered?) */
		if ((pkt.length == 0) || (pkt.length > CONFIG_CC1101_MAX_PACKET_SIZE)) {
			LOG_DBG("Received packet with illegal length: %d", pkt.length);
			goto flush;
		}

		bytes_remaining = pkt.length + status_bytes;
		pp++;

		while (bytes_remaining > 0) {
			k_sem_take(&data->fifo_cont, K_FOREVER);

			if (bytes_remaining < CC1101_FIFO_HWM) {
				ret = cc1101_burst_read(dev, CC1101_RX_FIFO_ADDR,
							pp, bytes_remaining);
				if (ret) {
					goto flush;
				}
				break;
			} else if (bytes_remaining <= CC1101_FIFO_SIZE) {
				k_sem_take(&data->fifo_cont, K_FOREVER);

				ret = cc1101_burst_read(dev, CC1101_RX_FIFO_ADDR,
							pp, bytes_remaining);
				if (ret) {
					goto flush;
				}
				break;
			} else {
				ret = cc1101_burst_read(dev, CC1101_RX_FIFO_ADDR,
							pp, CC1101_FIFO_HWM);
				if (ret) {
					goto flush;
				}
				bytes_remaining -= CC1101_FIFO_HWM;
				pp += CC1101_FIFO_HWM;
			}

		}

		cc1101_pkt_dump("received:", (uint8_t *)&pkt, (pkt.length + status_bytes + 1));

		while (k_msgq_put(&data->rxq, &pkt, K_NO_WAIT) != 0) {
			k_msgq_purge(&data->rxq);
			LOG_INF("RX Queue full dropping data!");
		}

		if (data->event_cb) {
			data->event_cb(dev, RF_EVENT_RECV_DONE, NULL);
		}

out:
		k_sem_take(&data->fifo_cont, K_NO_WAIT); /* TODO: comment in which case the sem has to be taken! */
		k_mutex_unlock(&data->xfer_lock);

		continue;
flush:
		LOG_DBG("Flushing RX FIFO.");

		k_sem_take(&data->fifo_cont, K_NO_WAIT);

		ret = cc1101_set_mode(dev, RF_MODE_IDLE);
		if (ret) {
			LOG_ERR("Could not enter IDLE mode. Receiver might be stuck.");
		};

		ret = cc1101_strobe(dev, CC1101_SFRX_ADDR);
		if (ret) {
			LOG_ERR("Could not flush RX FIFO.");
		};

		ret = cc1101_set_mode(dev, data->initial_mode);
		if (ret) {
			LOG_ERR("Could not resume to: %s.", cc1101_status2str(data->initial_mode));
		};

		k_mutex_unlock(&data->xfer_lock);

		if (data->event_cb) {
			data->event_cb(dev, RF_EVENT_SEND_READY, NULL);
		}

		cc1101_print_status(dev);
	}
}


/*
 * GPIO functions
 */

static inline void gdo0_int_handler(const struct device *port,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct cc1101_data *data =
		CONTAINER_OF(cb, struct cc1101_data, gdo0_cb);

	int level = gpio_pin_get_dt(&data->gdo0);

	if (data->status.STATE == CC1101_STATE_TX ||
			data->status.STATE == CC1101_STATE_FSTXON ||
			data->status.STATE == CC1101_STATE_TXFIFO_UNDERFLOW) {

		if (level == 0) {
			k_sem_give(&data->tx_done);
			k_sem_take(&data->fifo_cont, K_NO_WAIT); /* TODO comment why */
		}
	}
	if (data->status.STATE == CC1101_STATE_RX ||
			data->status.STATE == CC1101_STATE_RXFIFO_OVERFLOW) {
		if (level == 1) {
			k_sem_give(&data->rx_data_available);
		}
		if (level == 0) {
			k_sem_give(&data->fifo_cont);
		}
	}
}

static inline void gdo2_int_handler(const struct device *port,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct cc1101_data *data =
		CONTAINER_OF(cb, struct cc1101_data, gdo2_cb);

	int level = gpio_pin_get_dt(&data->gdo2);

	if (data->status.STATE == CC1101_STATE_TX ||
			data->status.STATE == CC1101_STATE_FSTXON ||
			data->status.STATE == CC1101_STATE_TXFIFO_UNDERFLOW) {
		if (level == 0) {
			k_sem_give(&data->fifo_cont);
		}
	}
	if (data->status.STATE == CC1101_STATE_RX ||
			data->status.STATE == CC1101_STATE_RXFIFO_OVERFLOW) {
		if (level == 1) {
			k_sem_give(&data->fifo_cont);
		}
	}
}

static void cc1101_gdo0_interrupt(const struct device *dev,
				 bool enable)
{
	const struct cc1101_data *data = dev->data;
	gpio_flags_t mode = enable ? GPIO_INT_EDGE_BOTH : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&data->gdo0, mode);
}

static void cc1101_gdo2_interrupt(const struct device *dev,
				 bool enable)
{
	const struct cc1101_data *data = dev->data;
	gpio_flags_t mode = enable ? GPIO_INT_EDGE_BOTH : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&data->gdo2, mode);
}


/*
 * Initialization functions
 */

int cc1101_init_gpio(const struct device *dev)
{
	int ret;
	struct cc1101_data *data = dev->data;

	if (!device_is_ready(data->gdo0.port)) {
		return -ENODEV;
	}

	if (!device_is_ready(data->gdo2.port)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&data->gdo0, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&data->gdo2, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gdo0_cb, gdo0_int_handler, BIT(data->gdo0.pin));

	ret = gpio_add_callback(data->gdo0.port, &data->gdo0_cb);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gdo2_cb, gdo2_int_handler, BIT(data->gdo2.pin));

	ret = gpio_add_callback(data->gdo2.port, &data->gdo2_cb);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int cc1101_init(const struct device *dev)
{
	int ret;
	const struct cc1101_config *cfg = dev->config;
	struct cc1101_data *data = dev->data;
	uint8_t tmp_buf;
	k_tid_t rx_tid;

	k_mutex_init(&data->xfer_lock);
	k_sem_init(&data->rx_data_available, 0, 1);
	k_sem_init(&data->tx_done, 0, 1);
	k_sem_init(&data->fifo_cont, 0, 1);

	LOG_DBG("Initializing %s", dev->name);

	if (!spi_is_ready(&cfg->spi)) {
		LOG_ERR("SPI device not ready: %s", cfg->spi.bus->name);
		return -ENODEV;
	}

	cc1101_wait_ready(dev);

	ret = cc1101_strobe(dev, CC1101_SRES_ADDR);
	if (ret) {
		LOG_ERR("Failed resetting device.");
		return ret;
	};

	cc1101_wait_ready(dev);

	ret = cc1101_burst_read(dev, CC1101_VERSION_ADDR, &tmp_buf, 1);
	if (ret) {
		return ret;
	};

	LOG_DBG("Version: 0x%02X", tmp_buf);

	ret = cc1101_init_gpio(dev);
	if (ret){
		LOG_ERR("Failed setting up GPIOs.");
		return ret;
	}

	data->config.iocfg0.GDO0_CFG = CC1101_IOCFG_XFER;

	//ret = cc1101_set_config(dev, (uint8_t *)&data->config);
	ret = cc1101_device_set(dev, RF_DEVICE_SETTINGS, (void *)&data->config);
	if (ret) {
		return ret;
	};

	//ret = cc1101_burst_write(dev, CC1101_PATABLE_ADDR, (uint8_t *) &data->patable, CC1101_NUM_PATABLE);
	ret = cc1101_device_set(dev, RF_DEVICE_POWER_TABLE, (void *)&data->patable);
	if (ret) {
		return ret;
	};

#ifdef CONFIG_CC1101_INITIAL_CALIBRATION
	ret = cc1101_set_mode(dev, RF_MODE_CALIBRATE);
	if (ret) {
		return ret;
	};
	LOG_DBG("Initial calibration done.");
#endif /* CONFIG_CC1101_INITIAL_CALIBRATION */

	data->initial_mode = CONFIG_CC1101_INITIAL_RF_MODE;

	ret = cc1101_set_mode(dev, data->initial_mode);
	if (ret) {
		return ret;
	};

	k_msgq_init(&data->rxq, rxq_buffer, sizeof(union cc1101_data_item), CONFIG_CC1101_RX_MSG_QUEUE_DEPTH);

	rx_tid = k_thread_create(&data->rx_thread,
			data->rx_stack,
			K_KERNEL_STACK_SIZEOF(data->rx_stack),
                        cc1101_rx_thread,
                        (void *)dev, NULL, NULL,
                        K_PRIO_COOP(CONFIG_CC1101_THREAD_PRIORITY),
			0,
			K_NO_WAIT);
	k_thread_name_set(rx_tid, "cc1101_rx_thread");

	cc1101_gdo0_interrupt(dev, true);
	cc1101_gdo2_interrupt(dev, true);

	LOG_DBG("Initialized in Mode: %s\n", cc1101_status2str(data->status.STATE));

	return ret;
}

static const struct rf_driver_api cc1101_api = {
	.device_set = cc1101_device_set,
	.device_get = NULL,
	.send = cc1101_send,
	.recv = cc1101_recv,
};

#define CC1101_INIT(inst)							\
	static struct cc1101_data cc1101_data_##inst = {			\
		.config.all = DT_INST_PROP(inst, initial_config),		\
		.patable = DT_INST_PROP(inst, initial_patable),			\
		.gdo0 = GPIO_DT_SPEC_INST_GET(inst, gdo0_gpios),		\
		.gdo2 = GPIO_DT_SPEC_INST_GET(inst, gdo2_gpios),		\
	};									\
	static const struct cc1101_config cc1101_config_##inst = {		\
		.spi = SPI_DT_SPEC_INST_GET(inst,				\
			SPI_OP_MODE_MASTER					\
			| SPI_WORD_SET(8)					\
			| SPI_LINES_SINGLE, 0),					\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst,						\
			 cc1101_init,						\
			 NULL,							\
			 &cc1101_data_##inst,					\
			 &cc1101_config_##inst,					\
			 POST_KERNEL,						\
			 CONFIG_RF_INIT_PRIORITY,				\
			 &cc1101_api);

DT_INST_FOREACH_STATUS_OKAY(CC1101_INIT)

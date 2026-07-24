/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(sd_dev, LOG_LEVEL_INF);

#include <zephyr/sd_dev/sd_dev_io.h>
#include <zephyr/sd_dev/sd_dev.h>
#include <zephyr/sd_dev/sdio_dev.h>
#include <zephyr/drivers/sd_dev.h>
#include <zephyr/sd_dev/sd_dev_pkt.h>

/**
 * @brief Dispatch received packet to SDIO function
 *
 * @param dev Pointer to the device structure
 * @param pkt Pointer to the packet to dispatch
 */
void sd_dev_rx_dispatch(const struct device *dev, sd_dev_pkt_t *pkt)
{
	struct k_fifo *rx_fifo = 0;
#ifdef CONFIG_SDIO_DEV
	struct sd_dev_card *card = sd_dev_get_card(dev);

	struct sdio_dev *sdio_dev = card->sdio;
	int fn = pkt->fn;
	struct sdio_dev_func *func = 0;

	if (fn < 1 || fn > 7) {
		LOG_INF("%s, wrong fn %d", __func__, fn);
		return;
	}
	func = sdio_dev->funcs[fn];

	if (sd_dev_get_state(dev) != SDEV_DEVICE_READY) {
		LOG_INF("%s, card is not ready, and can't receive data", __func__);
		return;
	}
	rx_fifo = &func->rx_fifo;
#endif
	sd_dev_pkt_get(pkt);
	if (!rx_fifo) {
		LOG_INF("%s, rx fifo is null", __func__);
		return;
	}
	k_fifo_put(rx_fifo, pkt);
}

#ifdef CONFIG_SDIO_DEV
/**
 * @brief Read data from SDIO function
 *
 * @param func Pointer to SDIO function structure
 * @param data Pointer to buffer to store read data
 *
 * @retval Positive number of bytes read on success
 * @retval -ENODEV if device is not ready
 */
int sdio_read(struct sdio_dev_func *func, uint8_t *data)
{
	sd_dev_pkt_t *rx_pkt = 0;

	if (sd_dev_get_state(func->sdio->card->dev) != SDEV_DEVICE_READY) {
		LOG_INF("%s, card is not ready, and can't write data", __func__);
		return -ENODEV;
	}

	rx_pkt = k_fifo_get(&func->rx_fifo, K_FOREVER);
	memcpy(data, rx_pkt->data, rx_pkt->len);
	sd_dev_pkt_free(rx_pkt);

	return rx_pkt->len;
}

/**
 * @brief Read packet from SDIO function
 *
 * @param func Pointer to SDIO function structure
 *
 * @return Pointer to received packet
 */
sd_dev_pkt_t *sdio_read_pkt(struct sdio_dev_func *func)
{
	return (sd_dev_pkt_t *)k_fifo_get(&func->rx_fifo, K_FOREVER);
}

/**
 * @brief Write packet to SDIO function
 *
 * @param func Pointer to SDIO function structure
 * @param pkt Pointer to packet to write
 *
 * @retval 0 on success
 * @retval -ENODEV if device is not ready
 */
int sdio_write_pkt(struct sdio_dev_func *func, sd_dev_pkt_t *pkt)
{
	int ret = 0;

	if (sd_dev_get_state(func->sdio->card->dev) != SDEV_DEVICE_READY) {
		LOG_INF("%s, card is not ready, and can't write data", __func__);
		return -ENODEV;
	}

	ret = sdio_send_data(func->sdio->card->dev, pkt);
	if (ret < 0) {
		LOG_INF("%s, sdio send data fail", __func__);
	}

	return ret;
}

/**
 * @brief Write data to SDIO function
 *
 * @param func Pointer to SDIO function structure
 * @param data Pointer to data buffer to write
 * @param len Length of data to write
 *
 * @retval 0 on success
 * @retval -ENODEV if device is not ready
 */
int sdio_write(struct sdio_dev_func *func, uint8_t *data, int len)
{
	int ret = 0;
	sd_dev_pkt_t *pkt;

	if (sd_dev_get_state(func->sdio->card->dev) != SDEV_DEVICE_READY) {
		LOG_INF("%s, card is not ready, and can't write data", __func__);
		return -ENODEV;
	}

	pkt = sd_dev_pkt_alloc(SDEV_PKT_TX);
	if (!pkt) {
		LOG_ERR("%s, sd_dev alloc pkt fail", __func__);
		return -ENOMEM;
	}
	pkt->len = len;
	pkt->data = data;
	pkt->fn = func->fn;

	ret = sdio_send_data(func->sdio->card->dev, pkt);
	if (ret < 0) {
		LOG_ERR("%s, sdio send data fail", __func__);
	}

	sd_dev_pkt_free(pkt);

	return ret;
}

/**
 * @brief Poll SDIO device for events
 *
 * @param func Pointer to SDIO function structure
 * @param events Events to poll for
 * @param revents Pointer to store returned events
 * @param timeout Timeout for polling
 *
 * @retval 0 on success
 * @retval Negative error code on failure
 */
int sdio_dev_poll(struct sdio_dev_func *func, uint32_t events, uint32_t *revents,
		  k_timeout_t timeout)
{
	struct k_poll_event poll_events[3];
	int n = 0;
	int ret;
	int idx = 0;

	*revents = 0;

	if (events & SDEV_POLLIN) {
		poll_events[n++] = (struct k_poll_event)K_POLL_EVENT_INITIALIZER(
			K_POLL_TYPE_FIFO_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &func->rx_fifo);
	}

	ret = k_poll(poll_events, n, timeout);
	if (ret < 0) {
		return ret;
	}

	if (events & SDEV_POLLIN) {
		if (poll_events[idx].state == K_POLL_STATE_FIFO_DATA_AVAILABLE) {
			*revents |= SDEV_POLLIN;
		}
		poll_events[idx++].state = K_POLL_STATE_NOT_READY;
	}

	return 0;
}
#endif

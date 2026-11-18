#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>

#include "scl3300_spi.h"

LOG_MODULE_REGISTER(scl3300_spi, CONFIG_SENSOR_LOG_LEVEL);

#define SCL3300_SPI_FRAME_SIZE 4U

static int scl3300_spi_transceive_sync(const struct device *dev,
                                       uint32_t command,
                                       uint8_t *rx_buf,
                                       size_t rx_buf_size)
{
    const struct scl3300_config *dev_cfg = dev->config;
    struct scl3300_data *data = dev->data;
    struct rtio_sqe *sqe;
    struct rtio_cqe *cqe;
    int ret;

    if (rx_buf == NULL || rx_buf_size < SCL3300_SPI_FRAME_SIZE) {
        return -EINVAL;
    }
    
    LOG_INF("Submitting SPI command: 0x%08x", command);  

    data->tx_buf[0] = (uint8_t)(command >> 24);
    data->tx_buf[1] = (uint8_t)(command >> 16);
    data->tx_buf[2] = (uint8_t)(command >> 8);
    data->tx_buf[3] = (uint8_t)command;

    memset(rx_buf, 0, SCL3300_SPI_FRAME_SIZE);

    sqe = rtio_sqe_acquire(data->rtio);
    if (sqe == NULL) {
        LOG_INF("Failed to acquire RTIO SQE");
        return -ENOMEM;
    }

    rtio_sqe_prep_transceive(sqe, dev_cfg->spi_iodev,
                             RTIO_PRIO_NORM,
                             data->tx_buf, rx_buf,
                             SCL3300_SPI_FRAME_SIZE, data);

    ret = rtio_submit(data->rtio, 0);
    if (ret < 0) {
        LOG_INF("RTIO submission failed: %d", ret);
        return ret;
    }

    cqe = rtio_cqe_consume_block(data->rtio);
    if (cqe == NULL) {
        LOG_INF("Failed to consume RTIO CQE");
        return -EIO;
    }

    ret = cqe->result;
    rtio_cqe_release(data->rtio, cqe);

    if (ret < 0) {
        LOG_INF("SPI transaction failed: %d", ret);
        return ret;
    }

    printk("SPI RX data: "); // Note: Moving further, RX data will be displayed in the decoder.
    for (size_t i = 0; i < SCL3300_SPI_FRAME_SIZE; i++) {
        printk(" 0x%02x", rx_buf[i]);
    }
    printk("\n");

    return 0;
}

int scl3300_spi_write(const struct device *dev, uint32_t command)
{
    struct scl3300_data *data = dev->data;

    return scl3300_spi_transceive_sync(dev, command,
                                       data->rx_buf,
                                       sizeof(data->rx_buf));
}

int scl3300_spi_read(const struct device *dev,
                     struct rtio_iodev_sqe *iodev_sqe,
                     uint32_t command)
{
    uint8_t *rx_buf;
    uint32_t rx_buf_len;
    int ret;

    ret = rtio_sqe_rx_buf(iodev_sqe,
                          SCL3300_SPI_FRAME_SIZE,
                          SCL3300_SPI_FRAME_SIZE,
                          &rx_buf,
                          &rx_buf_len);
    if (ret < 0) {
        LOG_INF("Failed to allocate RTIO RX buffer: %d", ret);
        return ret;
    }

    if (rx_buf_len < SCL3300_SPI_FRAME_SIZE) {
        LOG_INF("RTIO RX buffer is too small: %u", rx_buf_len);
        return -ENOMEM;
    }

    return scl3300_spi_transceive_sync(dev, command,
                                       rx_buf,
                                       rx_buf_len);
}

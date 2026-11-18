#define DT_DRV_COMPAT murata_scl3300

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/iodev.h>
#include <zephyr/rtio/sqe.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "scl3300.h"

LOG_MODULE_REGISTER(scl3300, CONFIG_SENSOR_LOG_LEVEL);

SPI_DT_IODEV_DEFINE(
    scl3300_spi_iodev,
    DT_DRV_INST(0),
    SPI_WORD_SET(8) | SPI_TRANSFER_MSB
);

static const struct scl3300_config scl3300_config = {
    .spi_iodev = &scl3300_spi_iodev,
    .init = DT_INST_PROP_OR(0, init, 0),
};

RTIO_DEFINE_WITH_MEMPOOL(
    scl3300_rtio,
    2,                  /* Submission queue depth */
    2,                  /* Completion queue depth */
    2,                  /* Number of memory blocks */
    256,                /* Memory block size */
    sizeof(void *)      /* Memory block alignment */
);

static struct scl3300_data scl3300_data = {
    .rtio = &scl3300_rtio,
};

static int scl3300_init_device(const struct device *dev);

static void scl3300_submit(const struct device *dev,
                           struct rtio_iodev_sqe *iodev_sqe)
{
    int ret;

    ret = scl3300_spi_read(dev,
                           iodev_sqe,
                           SCL3300_CMD_READ_ANG_X);

    if (ret < 0) {
        LOG_ERR("SPI read failed: %d", ret);
        rtio_iodev_sqe_err(iodev_sqe, ret);
        return;
    }

    rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static int scl3300_init_device(const struct device *dev)
{
    const struct scl3300_config *cfg = dev->config;
    int ret;

    if (!spi_is_ready_iodev(cfg->spi_iodev)) {
        LOG_ERR("SCL3300 SPI bus is not ready");
        return -ENODEV;
    }

    ret = scl3300_safely_configure(dev, cfg);
    if (ret < 0) {
        LOG_ERR("Failed to configure SCL3300: %d", ret);
        return ret;
    }

    return 0;
}

int scl3300_safely_configure(const struct device *dev,
                             const struct scl3300_config *cfg)
{
    if ((dev == NULL) || (cfg == NULL) || (dev->config == NULL)) {
        return -EINVAL;
    }

    /* TODO: Perform validation and backup. */

    return scl3300_configure(dev, cfg);
}

int scl3300_configure(const struct device *dev,
                      const struct scl3300_config *cfg)
{
    int ret;

    ret = scl3300_spi_write(dev, SCL3300_CMD_WHOAMI);
    if (ret < 0) {
        LOG_ERR("Failed to send WHOAMI command: %d", ret);
        return ret;
    }

    if (cfg->init) {
        ret = scl3300_spi_write(dev, SCL3300_CMD_WAKEUP);
        if (ret < 0) {
            LOG_ERR("Failed to send WAKEUP command: %d", ret);
            return ret;
        }

        ret = scl3300_spi_write(dev, SCL3300_CMD_SW_RESET);
        if (ret < 0) {
            LOG_ERR("Failed to send SW_RESET command: %d", ret);
            return ret;
        }

        k_msleep(2);

        ret = scl3300_spi_write(dev, SCL3300_CMD_ANG_CTRL);
        if (ret < 0) {
            LOG_ERR("Failed to send ANG_CTRL command: %d", ret);
            return ret;
        }

        k_msleep(25);

        ret = scl3300_spi_write(dev, SCL3300_CMD_STATUS);
        if (ret < 0) {
            LOG_ERR("Failed to send STATUS command: %d", ret);
            return ret;
        }

        ret = scl3300_spi_write(dev, SCL3300_CMD_STATUS);
        if (ret < 0) {
            LOG_ERR("Failed to send STATUS command: %d", ret);
            return ret;
        }

        ret = scl3300_spi_write(dev, SCL3300_CMD_STATUS);
        if (ret < 0) {
            LOG_ERR("Failed to send STATUS command: %d", ret);
            return ret;
        }
    }

    /* TODO: Runtime measurement mode configuration. */

    return 0;
}

static DEVICE_API(sensor, scl3300_driver_api) = {
    .submit = scl3300_submit,
};

DEVICE_DT_INST_DEFINE(
    0,
    scl3300_init_device,
    NULL,
    &scl3300_data,
    &scl3300_config,
    POST_KERNEL,
    CONFIG_SENSOR_INIT_PRIORITY,
    &scl3300_driver_api
);
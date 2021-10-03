//
// Created by jonasotto on 9/30/21.
//

#define DT_DRV_COMPAT nxp_flexio_spi

#include <errno.h>
#include <device.h>
#include <drivers/spi.h>
#include <soc.h>
#include "spi_context.h"


#include <fsl_flexio_spi.h>


#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL

#include <logging/log.h>
#include <drivers/clock_control.h>

LOG_MODULE_REGISTER(spi_flexio);

#define CHIP_SELECT_COUNT    4 // TODO: ?


/* Device constant configuration parameters */
struct spi_flexio_config {
    const struct device *clock_dev;
    clock_control_subsys_t clock_subsys;
    FLEXIO_Type *flexioBase;
};

/* Device run time data */
struct spi_flexio_data {
    const struct device *dev;
    flexio_spi_master_handle_t handle;
    FLEXIO_SPI_Type spiDev;
    struct spi_context ctx;
    size_t transfer_len;
};

static void spi_mcux_flexio_transfer_next_packet(const struct device *dev) {
    // TODO:
}


static void spi_mcux_flexio_master_transfer_callback(FLEXIO_SPI_Type *base,
                                                     flexio_spi_master_handle_t *handle, status_t status,
                                                     void *userData) {
    struct spi_flexio_data *data = userData;

    spi_context_update_tx(&data->ctx, 1, data->transfer_len);
    spi_context_update_rx(&data->ctx, 1, data->transfer_len);

    spi_mcux_flexio_transfer_next_packet(data->dev);
}


static int spi_flexio_configure(const struct device *dev,
                                const struct spi_config *spi_cfg) {
    const struct spi_flexio_config *config = dev->config;
    struct spi_flexio_data *data = dev->data;
    FLEXIO_SPI_Type *spiDev = &data->spiDev;

    if (spi_context_configured(&data->ctx, spi_cfg)) {
        // This configuration is already in use
        return 0;
    }

    flexio_spi_master_config_t master_config;
    FLEXIO_SPI_MasterGetDefaultConfig(&master_config);


    if (spi_cfg->slave > CHIP_SELECT_COUNT) {
        LOG_ERR("Slave %d is greater than %d",
                spi_cfg->slave,
                CHIP_SELECT_COUNT);
        return -EINVAL;
    }

    uint32_t word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);

    if (word_size == 8) {
        master_config.dataMode = kFLEXIO_SPI_8BitMode;
    } else if (word_size == 16) {
        master_config.dataMode = kFLEXIO_SPI_16BitMode;
    } else {
        LOG_ERR("Word size %lu is not supported, only 8 or 16 bit are supported.", word_size);
        return -EINVAL;
    }

    if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
        LOG_ERR("FlexIO SPI master only supports CPOL = 0.");
        return -EINVAL;
    }

    master_config.phase =
            (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
            ? kFLEXIO_SPI_ClockPhaseSecondEdge
            : kFLEXIO_SPI_ClockPhaseFirstEdge;

    // TODO: Direction (lsb/msb first) is set in flexio_spi_master_handle_t, perhaps for every transfer?

    master_config.baudRate_Bps = spi_cfg->frequency;

    uint32_t clock_freq;
    if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
        return -EINVAL;
    }

    spiDev->SDOPinIndex = 0;
    spiDev->SCKPinIndex = 1;
    spiDev->SDIPinIndex = 2;
    spiDev->CSnPinIndex = 3;
    spiDev->shifterIndex[0] = 0;
    spiDev->shifterIndex[1] = 1;
    spiDev->timerIndex[0] = 0;
    spiDev->timerIndex[1] = 1;

    FLEXIO_SPI_MasterInit(&(config->spiDev), &master_config, clock_freq);

    FLEXIO_SPI_MasterTransferCreateHandle(&(config->spiDev), &data->handle, spi_mcux_flexio_master_transfer_callback,
                                          data);


    data->ctx.config = spi_cfg;
    spi_context_cs_configure(&data->ctx);

    return 0;
}


static int transceive(const struct device *dev,
                      const struct spi_config *spi_cfg,
                      const struct spi_buf_set *tx_bufs,
                      const struct spi_buf_set *rx_bufs,
                      bool asynchronous,
                      struct k_poll_signal *signal) {
    struct spi_flexio_data *data = dev->data;
    int ret;

    spi_context_lock(&data->ctx, asynchronous, signal, spi_cfg);

    ret = spi_flexio_configure(dev, spi_cfg);
    if (ret) {
        goto out;
    }

    spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

    spi_context_cs_control(&data->ctx, true);

    spi_mcux_flexio_transfer_next_packet(dev);

    ret = spi_context_wait_for_completion(&data->ctx);
    out:
    spi_context_release(&data->ctx, ret);

    return ret;
}


static int spi_flexio_transceive(const struct device *dev,
                                 const struct spi_config *config,
                                 const struct spi_buf_set *tx_bufs,
                                 const struct spi_buf_set *rx_bufs) {
    const struct spi_flexio_config *cfg = dev->config;
    struct spi_flexio_data *data = dev->data;

    // TODO

    flexio_spi_transfer_t xfer = {0};
    //xfer.flags
    //xfer.rxData
    //xfer.txData
    //xfer.dataSize




    return 0;
}

static int spi_flexio_transceive_sync(const struct device *dev,
                                      const struct spi_config *config,
                                      const struct spi_buf_set *tx_bufs,
                                      const struct spi_buf_set *rx_bufs) {
    return spi_flexio_transceive(dev, config, tx_bufs, rx_bufs);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_flexio_transceive_async(const struct device *dev,
                     const struct spi_config *config,
                     const struct spi_buf_set *tx_bufs,
                     const struct spi_buf_set *rx_bufs,
                     struct k_poll_signal *async)
{
    /* TODO: implement asyc transceive */
    return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_flexio_release(const struct device *dev,
                              const struct spi_config *config) {
    //struct spi_flexio_data *data = dev->data;

    //spi_context_unlock_unconditionally(&data->ctx);

    return 0;
}

static int spi_flexio_init(const struct device *dev) {
    const struct spi_flexio_config *cfg = dev->config;
    struct spi_flexio_data *data = dev->data;


    return 0;
}

static const struct spi_driver_api spi_flexio_driver_api = {
        .transceive = spi_flexio_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
        .transceive_async = spi_flexio_transceive_async,
#endif
        .release = spi_flexio_release,
};

#define SPI_FLEXIO_DEFINE_CONFIG(n) \
    static const struct spi_flexio_config spi_flexio_config_##n = { \
        .clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
        .clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name), \
        .flexioBase= (FLEXIO_Type *)DT_INST_REG_ADDR(n), \
    }

#define SPI_FLEXIO_DEVICE_INIT(n)                        \
    SPI_FLEXIO_DEFINE_CONFIG(n);                    \
    static struct spi_flexio_data spi_flexio_dev_data_##n = {        \
        SPI_CONTEXT_INIT_LOCK(spi_flexio_dev_data_##n, ctx),    \
        SPI_CONTEXT_INIT_SYNC(spi_flexio_dev_data_##n, ctx),    \
    };                                \
    DEVICE_DT_INST_DEFINE(n, &spi_flexio_init, NULL,            \
                &spi_flexio_dev_data_##n,            \
                &spi_flexio_config_##n, POST_KERNEL,        \
                CONFIG_SPI_INIT_PRIORITY, &spi_flexio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_FLEXIO_DEVICE_INIT)
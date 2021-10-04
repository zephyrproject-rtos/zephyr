//
// Created by jonasotto on 9/30/21.
//

#define DT_DRV_COMPAT nxp_flexio_spi

#include <errno.h>
#include <device.h>
#include <drivers/spi.h>
#include <soc.h>
#include "spi_context.h"
#include <drivers/clock_control.h>
#include <fsl_flexio_spi.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(spi_flexio, CONFIG_SPI_LOG_LEVEL);

/* Device constant configuration parameters */
struct spi_flexio_config {
    FLEXIO_Type *flexioBase;
    const struct device *clock_dev;
    clock_control_subsys_t clock_subsys;

    /**
     * IRQ handling: The ISRs for every FlexIO instance are all configured to call the same callback (spi_flexio_isr),
     * with the proper device* parameter. This function then retrieves the SPI configuration from the device* and calls
     * the library "handleIRQ" function for the appropriate SPI instance.
     *
     * This irq_config_func hooks the device up to spi_flexio_isr.
     */
    void (*irq_config_func)(const struct device *dev);
};

/* Device run time data */
struct spi_flexio_data {
    const struct device *dev;
    flexio_spi_master_handle_t handle;
    FLEXIO_SPI_Type spiDev;
    struct spi_context ctx;
    size_t transfer_len;
};

/**
 * This ISR is called for every FlexIO SPI instance
 *
 * @param dev The FlexIO SPI device the interrupt originated from
 */
static void spi_flexio_isr(const struct device *dev) {
    struct spi_flexio_data *data = dev->data;
    FLEXIO_SPI_Type *spiDev = &data->spiDev;

    FLEXIO_SPI_MasterTransferHandleIRQ(spiDev, &data->handle);
}


static void spi_mcux_flexio_transfer_next_packet(const struct device *dev) {
    // TODO


    const struct spi_flexio_config *config = dev->config;
    struct spi_flexio_data *data = dev->data;
    FLEXIO_SPI_Type *base = &data->spiDev;
    struct spi_context *ctx = &data->ctx;

    if ((ctx->tx_len == 0) && (ctx->rx_len == 0)) {
        /* nothing left to rx or tx, we're done! */
        spi_context_cs_control(&data->ctx, false);
        spi_context_complete(&data->ctx, 0);
        return;
    }

    flexio_spi_transfer_t transfer;
    transfer.flags = kFLEXIO_SPI_8bitMsb; // TODO: Support 16bit and LSB first modes

    if (ctx->tx_len == 0) {
        /* rx only, nothing to tx */
        transfer.txData = NULL;
        transfer.rxData = ctx->rx_buf;
        transfer.dataSize = ctx->rx_len;
    } else if (ctx->rx_len == 0) {
        /* tx only, nothing to rx */
        transfer.txData = (uint8_t *) ctx->tx_buf;
        transfer.rxData = NULL;
        transfer.dataSize = ctx->tx_len;
    } else if (ctx->tx_len == ctx->rx_len) {
        /* rx and tx are the same length */
        transfer.txData = (uint8_t *) ctx->tx_buf;
        transfer.rxData = ctx->rx_buf;
        transfer.dataSize = ctx->tx_len;
    } else if (ctx->tx_len > ctx->rx_len) {
        /* Break up the tx into multiple transfers so we don't have to
         * rx into a longer intermediate buffer. Leave chip select
         * active between transfers.
         */
        transfer.txData = (uint8_t *) ctx->tx_buf;
        transfer.rxData = ctx->rx_buf;
        transfer.dataSize = ctx->rx_len;
    } else {
        /* Break up the rx into multiple transfers so we don't have to
         * tx from a longer intermediate buffer. Leave chip select
         * active between transfers.
         */
        transfer.txData = (uint8_t *) ctx->tx_buf;
        transfer.rxData = ctx->rx_buf;
        transfer.dataSize = ctx->tx_len;
    }

    if (!(ctx->tx_count <= 1 && ctx->rx_count <= 1)) {
        // TODO: ?
        //transfer.configFlags |= kLPSPI_MasterPcsContinuous;
    }

    data->transfer_len = transfer.dataSize;

    status_t status = FLEXIO_SPI_MasterTransferNonBlocking(base, &data->handle,
                                                           &transfer);
    if (status != kStatus_Success) {
        LOG_ERR("Transfer could not start");
    }
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

    /*
    if (spi_cfg->slave > CHIP_SELECT_COUNT) {
        LOG_ERR("Slave %d is greater than %d",
                spi_cfg->slave,
                CHIP_SELECT_COUNT);
        return -EINVAL;
    }
    */

    uint32_t word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);

    if (word_size == 8) {
        master_config.dataMode = kFLEXIO_SPI_8BitMode;
    } else if (word_size == 16) {
        master_config.dataMode = kFLEXIO_SPI_16BitMode;
    } else {
        LOG_ERR("Word size %u is not supported, only 8 or 16 bit are supported.", word_size);
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

    FLEXIO_SPI_MasterInit(&(data->spiDev), &master_config, clock_freq);

    FLEXIO_SPI_MasterTransferCreateHandle(&(data->spiDev), &data->handle, spi_mcux_flexio_master_transfer_callback,
                                          data);


    data->ctx.config = spi_cfg;
    spi_context_cs_configure(&data->ctx);

    return 0;
}


static int transceive(const struct device *dev,
                      const struct spi_config *spi_cfg,
                      const struct spi_buf_set *tx_bufs,
                      const struct spi_buf_set *rx_bufs) {
    struct spi_flexio_data *data = dev->data;
    int ret;

    spi_context_lock(&data->ctx, false, NULL, spi_cfg);

    ret = spi_flexio_configure(dev, spi_cfg);
    if (ret) {
        goto out;
    }

    spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

    spi_context_cs_control(&data->ctx,
                           true); // TODO: Chip select? This supports GPIO, how is this configured, and how does this use HW CS pins?

    spi_mcux_flexio_transfer_next_packet(dev);

    ret = spi_context_wait_for_completion(&data->ctx);
    out:
    spi_context_release(&data->ctx, ret);

    return ret;
}

static int spi_flexio_transceive_sync(const struct device *dev,
                                      const struct spi_config *spi_cfg,
                                      const struct spi_buf_set *tx_bufs,
                                      const struct spi_buf_set *rx_bufs) {
    return transceive(dev, spi_cfg, tx_bufs, rx_bufs);
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
    struct spi_flexio_data *data = dev->data;

    spi_context_unlock_unconditionally(&data->ctx);

    return 0;
}

static int spi_flexio_init(const struct device *dev) {
    const struct spi_flexio_config *config = dev->config;
    struct spi_flexio_data *data = dev->data;

    //config->irq_config_func(dev);

    spi_context_unlock_unconditionally(&data->ctx);

    data->dev = dev;

    return 0;
}

/**
 * Define an implementation of the spi_driver_api
 */
static const struct spi_driver_api spi_flexio_driver_api = {
        .transceive = spi_flexio_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
        .transceive_async = spi_flexio_transceive_async,
#endif
        .release = spi_flexio_release,
};

#define SPI_FLEXIO_DEVICE_INIT(n) \
    static void spi_flexio_config_func_##n(const struct device *dev); \
    static const struct spi_flexio_config spi_flexio_config_##n = { \
        .flexioBase= (FLEXIO_Type *)DT_INST_REG_ADDR(n), \
        .clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
        .clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name), \
        .irq_config_func = spi_flexio_config_func_##n, \
    }; \
    static struct spi_flexio_data spi_flexio_dev_data_##n = { \
        SPI_CONTEXT_INIT_LOCK(spi_flexio_dev_data_##n, ctx), \
        SPI_CONTEXT_INIT_SYNC(spi_flexio_dev_data_##n, ctx), \
    }; \
    DEVICE_DT_INST_DEFINE(n, &spi_flexio_init, NULL, \
                &spi_flexio_dev_data_##n, \
                &spi_flexio_config_##n, POST_KERNEL, \
                CONFIG_SPI_INIT_PRIORITY, &spi_flexio_driver_api); \
    static void spi_flexio_config_func_##n(const struct device *dev) { \
            /* Connect the specific IRQ to spi_mcux_isr with the current device pointer as argument */ \
            IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_flexio_isr, DEVICE_DT_INST_GET(n), 0); \
            irq_enable(DT_INST_IRQN(n)); \
    };
DT_INST_FOREACH_STATUS_OKAY(SPI_FLEXIO_DEVICE_INIT)
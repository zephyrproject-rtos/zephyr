/*
 * Copyright (c) 2018 Foundries.io
 * Modified by Christoph Schramm, Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define SYS_LOG_DOMAIN "generic_uart"
#define SYS_LOG_LEVEL 4 // TODO

#include <drivers/generic_uart/generic_uart_drv.h>

#include <kernel.h>
#include <init.h>
#include <uart.h>

#include <logging/sys_log.h>

#define MAX_READ_SIZE	128


static struct uart_drv_context *contexts[MAX_UART_DRV_CTX];


struct uart_drv_context *uart_drv_context_from_id(int id)
{
    if (id >= 0 && id < MAX_UART_DRV_CTX) {
        return contexts[id];
    } else {
        return NULL;
    }
}

static struct uart_drv_context *context_from_dev(struct device *dev)
{
    int i;

    for (i = 0; i < MAX_UART_DRV_CTX; i++) {
        if (contexts[i] && contexts[i]->uart_dev == dev) {
            return contexts[i];
        }
    }

    return NULL;
}

static int uart_drv_get(struct uart_drv_context *ctx)
{
    int i;

    /* find a free modem_context */
    for (i = 0; i < MAX_UART_DRV_CTX; i++) {
        if (!contexts[i]) {
            contexts[i] = ctx;
            return 0;
        }
    }

    return -ENOMEM;
}

static void uart_drv_flush(struct uart_drv_context *ctx)
{
    u8_t c;

    if (!ctx) {
        return;
    }

    /* Drain the fifo */
    while (uart_fifo_read(ctx->uart_dev, &c, 1) > 0) {
        continue;
    }

    /* clear the UART pipe */
    k_pipe_init(&ctx->uart_pipe, ctx->uart_pipe_buf, ctx->uart_pipe_size);
}

static void uart_drv_isr(struct device *uart_dev)
{
    struct uart_drv_context *ctx;
    int ret;
    size_t rx;
    size_t bytes_written;
    static u8_t read_buf[MAX_READ_SIZE];

    /* lookup the device */
    ctx = context_from_dev(uart_dev);
    if (!ctx) {
        return;
    }

    /* get all of the data off UART as fast as we can */
    while (uart_irq_update(ctx->uart_dev) &&
           uart_irq_rx_ready(ctx->uart_dev)) {
        rx = uart_fifo_read(ctx->uart_dev, read_buf, sizeof(read_buf));
        if (rx > 0) {
            ret = k_pipe_put(&ctx->uart_pipe, read_buf, rx,
                             &bytes_written, rx, K_NO_WAIT);
            if (ret < 0) {
                printk("UART buffer write error (%d)! "
                            "Flushing UART!", ret);
                uart_drv_flush(ctx);
                return;
            }

            k_sem_give(&ctx->rx_sem);
        }
    }
}

int uart_drv_recv(struct uart_drv_context *ctx,
                      u8_t *buf, size_t size, size_t *bytes_read)
{
    if (!ctx) {
        return -EINVAL;
    }

    return k_pipe_get(&ctx->uart_pipe, buf, size, bytes_read, 1, K_NO_WAIT);
}

int uart_drv_send(struct uart_drv_context *ctx,
                      const u8_t *buf, size_t size)
{
    if (!ctx) {
        return -EINVAL;
    }

    printk("OUT: %s", buf);

    while (size)  {
        int written;

        written = uart_fifo_fill(ctx->uart_dev,
                                 (const u8_t *)buf, size);
        if (written < 0) {
            /* error */
            uart_irq_tx_disable(ctx->uart_dev);
            return written;
        } else if (written < size) {
            k_yield();
        }

        size -= written;
        buf += written;
    }

    return 0;
}

static void uart_drv_setup(struct uart_drv_context *ctx)
{
    if (!ctx) {
        return;
    }

    uart_irq_rx_disable(ctx->uart_dev);
    uart_irq_tx_disable(ctx->uart_dev);
    uart_drv_flush(ctx);
    uart_irq_callback_set(ctx->uart_dev, uart_drv_isr);
    uart_irq_rx_enable(ctx->uart_dev);
}

int uart_drv_register(struct uart_drv_context *ctx,
                          const char *uart_dev_name,
                          u8_t *buf, size_t size)
{
    int ret;

    if (!ctx) {
        return -EINVAL;
    }

    ctx->uart_dev = device_get_binding(uart_dev_name);
    if (!ctx->uart_dev) {
        return -ENOENT;
    }

    /* k_pipe is setup later in uart_drv_flush() */
    ctx->uart_pipe_buf = buf;
    ctx->uart_pipe_size = size;
    k_sem_init(&ctx->rx_sem, 0, 1);

    ret = uart_drv_get(ctx);
    if (ret < 0) {
        return ret;
    }

    uart_drv_setup(ctx);
    return 0;
}

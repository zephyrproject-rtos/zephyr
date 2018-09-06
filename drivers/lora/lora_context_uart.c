#include <kernel.h>
#include <init.h>
#include <uart.h>
#include <logging/sys_log.h>
#include <drivers/lora/lora_context.h>
#include <string.h>

/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#if (defined(CONFIG_LORA_UART0) && CONFIG_LORA_UART0 == 1) || (defined(CONFIG_LORA_UARTE0) && CONFIG_LORA_UARTE0 == 1)
#define ASSIGNED_UART_PERIPHERAL CONFIG_UART_0_NAME
#elif defined(CONFIG_LORA_UARTE1) && CONFIG_LORA_UARTE1 == 1
#define ASSIGNED_UART_PERIPHERAL CONFIG_UART_1_NAME
#endif

#define BUF_MAXSIZE	256

static struct device *uart_dev;
u8_t *uart_pipe_buf;
size_t uart_pipe_size;
struct k_pipe uart_pipe;

static u8_t rx_buf[BUF_MAXSIZE];
static u8_t tx_buf[BUF_MAXSIZE];

static void msg_dump(const char *s, u8_t *data, unsigned len)
{
    unsigned i;

    printk("%s: ", s);
    for (i = 0; i < len; i++) {
        printk("%02x ", data[i]);
    }
    printk("(%u bytes)\n", len);
}

static void lora_uart_isr(struct device *x)
{

    int len = uart_fifo_read(uart_dev, rx_buf, BUF_MAXSIZE);

    //gpio_pin_configure()
    ARG_UNUSED(x);
    msg_dump(__func__, rx_buf, len);

}


/* Send an AT command with a series of response handlers */
static int send_at_cmd(
                       const u8_t *buf, size_t size)
{
    printk("sending: %s\n", buf);
    while (size)  {
        int written;

        written = uart_fifo_fill(uart_dev,
                                 (const u8_t *)buf, size);
        if (written < 0) {
            /* error */
            uart_irq_tx_disable(uart_dev);
            return written;
        } else if (written < size) {
            k_yield();
        }

        size -= written;
        buf += written;
    }
    printk("sent: %s\n", buf);
    return 0;
}

void uart_clear(void)
{
    u8_t c;

    /* Drain the fifo */
    while (uart_fifo_read(uart_dev, &c, 1) > 0) {
        continue;
    }
    /* clear the UART pipe */
    k_pipe_init(&uart_pipe, uart_pipe_buf, uart_pipe_size);
}

void lora_context_uart_init(void)
{
    uart_dev = device_get_binding(ASSIGNED_UART_PERIPHERAL);
    printk("LoRa device assigned to %s\n", uart_dev->config->name);

    uart_clear();

    uart_irq_callback_set(uart_dev, lora_uart_isr);
    uart_irq_rx_enable(uart_dev);

    lora_device_init();

    const char* getver = "sys get ver\r\n";
    send_at_cmd(getver, strlen(getver));
}
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

#ifndef CMD_HANDLER
#define CMD_HANDLER(cmd_, cb_) { \
	.cmd = cmd_, \
	.cmd_len = (u16_t)sizeof(cmd_)-1, \
	.func = on_cmd_ ## cb_ \
}
#endif

#define BUF_MAXSIZE	256
#define MAX_READ_SIZE	128
#define LORA_MAX_DATA_LENGTH		1500
#define MDM_RECV_MAX_BUF		30
#define BUF_ALLOC_TIMEOUT K_SECONDS(1)




K_PIPE_DEFINE(uart_rx_pipe, 256, 4);


/* RX thread structures */
K_THREAD_STACK_DEFINE(lora_device_rx_stack,
                      LORA_RX_STACK_SIZE);
struct k_thread lora_device_rx_thread;

/* RX thread work queue */
K_THREAD_STACK_DEFINE(lora_device_workq_stack,
                      LORA_RX_WORKQ_STACK_SIZE);
static struct k_work_q lora_device_workq;


static struct device *uart_dev;
u8_t *uart_pipe_buf;
size_t uart_pipe_size;

static u8_t rx_buf[BUF_MAXSIZE];
static u8_t tx_buf[BUF_MAXSIZE];

const static char* newLineConstant;
static size_t newLineLen;

struct cmd_handler {
    const char *cmd;
    u16_t cmd_len;
    void (*func)(struct net_buf **buf, u16_t len);
};

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

static bool is_crlf(u8_t c)
{
    if (c == '\n' || c == '\r') {
        return true;
    } else {
        return false;
    }
}



static inline void _hexdump(const u8_t *packet, size_t length)
{
    char output[sizeof("xxxxyyyy xxxxyyyy")];
    int n = 0, k = 0;
    u8_t byte;

    while (length--) {
        if (n % 16 == 0) {
            printk(" %08X ", n);
        }

        byte = *packet++;

        printk("%02X ", byte);

        if (byte < 0x20 || byte > 0x7f) {
            output[k++] = '.';
        } else {
            output[k++] = byte;
        }

        n++;
        if (n % 8 == 0) {
            if (n % 16 == 0) {
                output[k] = '\0';
                printk(" [%s]\n", output);
                k = 0;
            } else {
                printk(" ");
            }
        }
    }

    if (n % 16) {
        int i;

        output[k] = '\0';

        for (i = 0; i < (16 - (n % 16)); i++) {
            printk("   ");
        }

        if ((n % 16) < 8) {
            printk(" "); /* one extra delimiter after 8 chars */
        }

        printk(" [%s]\n", output);
    }
}

static void lora_uart_flush(void)
{
    u8_t c;

    /* Drain the fifo */
    while (uart_fifo_read(uart_dev, &c, 1) > 0) {
        continue;
    }

    /* clear the UART pipe */
// TODO
}

static void lora_uart_reset(struct device *x)
{
    u8_t c;

    /* Drain the fifo */
    while (uart_fifo_read(x, &c, 1) > 0) {
        continue;
    }

    /* clear the UART pipe */
    // TODO
}

static u8_t read_buf[MAX_READ_SIZE];
static void lora_uart_isr(struct device *x)
{
    int ret;
    size_t bytes_read_from_uart, bytes_written_to_pipe;
    while (uart_irq_update(uart_dev) &&
           uart_irq_rx_ready(uart_dev)) {
        u8_t byte;
        bytes_read_from_uart = uart_fifo_read(uart_dev, &byte, 1);
        if (bytes_read_from_uart > 0) {
            ret = k_pipe_put(&uart_rx_pipe, &byte, bytes_read_from_uart, &bytes_written_to_pipe, bytes_read_from_uart, K_NO_WAIT);

            if (ret < 0) {
                SYS_LOG_ERR("UART buffer write error (%d)! "
                            "Flushing UART!", ret);
                lora_uart_reset(x);
                return;
            }


        }
    }
    //k_sem_give(&lora_uart_rx_sem);
}

static void on_cmd_atcmdinfo_manufacturer(struct net_buf **buf, u16_t len)
{

    SYS_LOG_INF("Manufacturer: %s", *buf);
}


static uint8_t device_read_rx_buffer(void)
{
    size_t bytes_read = 0;
    static u8_t uart_buffer[1024];
    static u8_t temp_buffer[256];

    static size_t line_len = 0;
    static size_t test_line_len = 0;

    while (true) {
        k_pipe_get(&uart_rx_pipe, &uart_buffer, 1024, &bytes_read, 1, K_NO_WAIT);


        if(bytes_read)
        {
            memcpy(&temp_buffer[line_len], &uart_buffer, bytes_read);

            line_len += bytes_read;

            test_line_len = line_len-(newLineLen);

            /* check if there is a newline */
            if(strcmp(&temp_buffer[test_line_len],newLineConstant) == 0)
            {
                u8_t line[256];
                memcpy(line, temp_buffer, test_line_len);
                line[test_line_len] = '\0';
                printk("IN: [%s]\n", line);
                line_len = 0;
                return 0;
            }


        }
    }

}

/* RX thread */
void lora_device_uart_rx(void)
{



    static const struct cmd_handler handlers[] = {

            /* MODEM Information */
            CMD_HANDLER("Manufacturer: ", atcmdinfo_manufacturer),
    };


    uint8_t ret = 0;
    while (ret == 0) {
        ret = device_read_rx_buffer();
    }
}


/* Send an AT command with a series of response handlers */
static int send_at_cmd(
                       const u8_t *bufIn)
{
    size_t buffLen = strlen(bufIn);
    size_t size = buffLen + newLineLen;
    u8_t buf[size];
    strcpy(buf, bufIn);
    memcpy(&buf[buffLen], newLineConstant, newLineLen);

    printk("OUT: [%s]\n", bufIn);
    int written = 0;

    while (size)  {
        written = uart_fifo_fill(uart_dev,
                                 (const u8_t *)&buf[written], size-written);
        if (written < 0) {
            /* error */
            uart_irq_tx_disable(uart_dev);
            return written;
        } else if (written < size) {
            k_yield();
        }

        size -= written;
    }

    printk("sent\n");
    return 0;
}


 void lora_device_uart_init(const char* newLine)
{
    int ret = 0;
    uart_dev = device_get_binding(ASSIGNED_UART_PERIPHERAL);
    printk("LoRa device assigned to %s\n", uart_dev->config->name);
    printk("Heap size is %u\n", CONFIG_HEAP_MEM_POOL_SIZE);


    uart_irq_rx_disable(uart_dev);
    uart_irq_tx_disable(uart_dev);
    lora_uart_flush();
    uart_irq_callback_set(uart_dev, lora_uart_isr);
    uart_irq_rx_enable(uart_dev);

    newLineConstant = newLine;
    newLineLen = strlen(newLine);




    const char* getver = "sys get ver";
    send_at_cmd(getver);
    //k_sleep(K_SECONDS(2));
}

K_THREAD_DEFINE(lora_device_uart_rx_id, 1024, lora_device_uart_rx, NULL, NULL, NULL,
                PRIORITY, 0, K_NO_WAIT);
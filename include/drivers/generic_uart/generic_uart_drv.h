/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GENERIC_UART_DRV_H
#define GENERIC_UART_DRV_H

#include <stdlib.h>
#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UART_DRV_MAX_DATA_LENGTH 1500 // TODO
#define CONFIG_MODEM_uart_dev_RX_WORKQ_STACK_SIZE 2048 // TODO
#define CONFIG_MODEM_uart_dev_RX_STACK_SIZE 1028// TODO

struct uart_drv_context {
    struct device *uart_dev;

    /* rx data */
    u8_t *uart_pipe_buf;
    size_t uart_pipe_size;
    struct k_pipe uart_pipe;
    struct k_sem rx_sem;
};

struct uart_dev_ctx {
    int last_error;

    u8_t recv_buf[UART_DRV_MAX_DATA_LENGTH];
    const char* linebreak_constant;
    u16_t linebreak_len;

    struct k_work_q workq;
    struct _k_thread_stack_element* workq_stack;

    struct k_thread rx_thread;
    struct _k_thread_stack_element* rx_thread_stack;

    struct uart_drv_context drv_ctx;
    const struct cmd_handler* command_handlers;
    int (*cmd_resp_handler)(uint8_t* buf, u16_t len);
    int (*generic_resp_handler)(uint8_t* buf, u16_t len);
    uint8_t command_handler_cnt;

    /* semaphores */
    struct k_sem response_sem;
};

struct cmd_handler {
    const char *cmd;
    u16_t cmd_len;
    int (*func)(uint8_t* buf, u16_t len);
};


struct uart_drv_context *uart_drv_context_from_id(int id);
struct uart_drv_context *context_from_dev(struct device *dev);

int uart_drv_recv(struct uart_drv_context *ctx,
                      u8_t *buf, size_t size, size_t *bytes_read);
int uart_drv_send(struct uart_drv_context *ctx,
                       u8_t *buf, size_t size);
int uart_drv_register(struct uart_drv_context *ctx,
                          const char *uart_dev_name,
                          u8_t *buf, size_t size);

int uart_dev_init(struct uart_dev_ctx *dev_ctx, struct device *uart_dev);

int uart_dev_send_cmd(/*struct uart_dev_socket *sock,*/
        struct uart_dev_ctx *dev_ctx,
        const u8_t *data, int timeout, int (*response_handler)(uint8_t* buf, u16_t len));

#define MAX_UART_DRV_CTX 3	// TODO CONFIG_MODEM_RECEIVER_MAX_CONTEXTS

#define CMD_HANDLER(cmd_, cb_) { \
	.cmd = cmd_, \
	.cmd_len = (u16_t)sizeof(cmd_)-1, \
	.func = on_cmd_ ## cb_ \
}

#ifdef __cplusplus
}
#endif
#endif

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

struct uart_drv_context {
    struct device *uart_dev;

    /* rx data */
    u8_t *uart_pipe_buf;
    size_t uart_pipe_size;
    struct k_pipe uart_pipe;
    struct k_sem rx_sem;
};

struct cmd_handler {
    const char *cmd;
    u16_t cmd_len;
    int (*func)(uint8_t* buf, u16_t len);
};


struct uart_drv_context *uart_drv_context_from_id(int id);

int uart_drv_recv(struct uart_drv_context *ctx,
                      u8_t *buf, size_t size, size_t *bytes_read);
int uart_drv_send(struct uart_drv_context *ctx,
                      const u8_t *buf, size_t size);
int uart_drv_register(struct uart_drv_context *ctx,
                          const char *uart_dev_name,
                          u8_t *buf, size_t size);

int uart_dev_init(struct device *uart_dev, const struct cmd_handler command_handlers[], uint8_t cmd_handler_cnt, int (*resp_handler)(uint8_t* buf, u16_t len));
int uart_dev_send_cmd(/*struct uart_dev_socket *sock,*/
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

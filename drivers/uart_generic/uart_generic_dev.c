/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr.h>
#include <gpio.h>
#include <device.h>
#include <init.h>
#include <logging/sys_log.h>
#include <drivers/generic_uart/generic_uart_drv.h>
#include <string.h>
#include <net/buf.h>

//#undef CONFIG_NET_BUF




#define UART_DRV_RECV_BUF_SIZE		128 // TODO









// TODO
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


int uart_dev_send_cmd(/*struct uart_dev_socket *sock,*/
        struct uart_dev_ctx *dev_ctx,
        const u8_t *cmd, int timeout, int (*response_handler)(uint8_t* buf, u16_t len))
{
    int ret;

    dev_ctx->last_error = 0;

    size_t buff_len = strlen(cmd);
    size_t size = buff_len + dev_ctx->linebreak_len;
    u8_t* data = k_malloc(size);

    strcpy(data, cmd);
    memcpy(&data[buff_len], dev_ctx->linebreak_constant,  dev_ctx->linebreak_len);


    SYS_LOG_DBG("OUT: [%s]", cmd);

    uart_drv_send(&dev_ctx->drv_ctx, data, strlen(data));

    k_free(data);

    if (timeout == K_NO_WAIT) {
        return 0;
    }

    if(dev_ctx->cmd_resp_handler)
    {
        SYS_LOG_WRN("Trying to assign new response handler while previous handler is still waiting");
        ret = -EAGAIN;
    } else {
        dev_ctx->cmd_resp_handler = response_handler;
    }

    //if (!sock) {
        k_sem_reset(&dev_ctx->response_sem);
        ret = k_sem_take(&dev_ctx->response_sem, timeout);
    /*   }  TODO: implement sock and transform to CHANNEL or so
 else {
         k_sem_reset(&sock->sock_send_sem);
         ret = k_sem_take(&sock->sock_send_sem, timeout);
     }
 */
    if (ret == 0) {
        ret = dev_ctx->last_error;
    } else if (ret == -EAGAIN) {
        ret = -ETIMEDOUT;
    }

    return ret;
}



//#define CONFIG_NET_BUF 1
#undef CONFIG_NET_BUF
#ifdef CONFIG_NET_BUF

#define UART_DEV_MAX_BUF_CNT 30
#define NETBUF_ALLOC_TIMEOUT K_SECONDS(1)

NET_BUF_POOL_DEFINE(uart_drv_recv_pool, UART_DEV_MAX_BUF_CNT, UART_DRV_RECV_BUF_SIZE, 0, NULL);

static inline struct net_buf *read_rx_allocator(s32_t timeout, void *user_data)
{
    return net_buf_alloc((struct net_buf_pool *)user_data, timeout);
}


static bool is_crlf(u8_t c)
{
    if (c == '\n' || c == '\r') {
        return true;
    } else {
        return false;
    }
}

static void net_buf_skipcrlf(struct net_buf **buf)
{
    /* chop off any /n or /r */
    while (*buf && is_crlf(*(*buf)->data)) {
        net_buf_pull_u8(*buf);
        if (!(*buf)->len) {
            *buf = net_buf_frag_del(NULL, *buf);
        }
    }
}

static u16_t net_buf_findcrlf(struct net_buf *buf, struct net_buf **frag,
                              u16_t *offset)
{
    u16_t len = 0, pos = 0;

    while (buf && !is_crlf(*(buf->data + pos))) {
        if (pos + 1 >= buf->len) {
            len += buf->len;
            buf = buf->frags;
            pos = 0;
        } else {
            pos++;
        }
    }

    if (buf && is_crlf(*(buf->data + pos))) {
        len += pos;
        *offset = pos;
        *frag = buf;
        return len;
    }

    return 0;
}

static int net_buf_ncmp(struct net_buf *buf, const u8_t *s2, size_t n)
{
    struct net_buf *frag = buf;
    u16_t offset = 0;

    while ((n > 0) && (*(frag->data + offset) == *s2) && (*s2 != '\0')) {
        if (offset == frag->len) {
            if (!frag->frags) {
                break;
            }
            frag = frag->frags;
            offset = 0;
        } else {
            offset++;
        }

        s2++;
        n--;
    }

    return (n == 0) ? 0 : (*(frag->data + offset) - *s2);
}

static void uart_dev_read_rx(struct net_buf **buf)
{
    u8_t uart_buffer[UART_DRV_RECV_BUF_SIZE];
	size_t bytes_read = 0;
	int ret;
	u16_t rx_len;

	while (true) {
		ret = uart_drv_recv(&ictx.drv_ctx,
					uart_buffer,
					sizeof(uart_buffer),
					&bytes_read);
		if (ret < 0) {
			/*  buffer is empty */
			break;
		}

		_hexdump(uart_buffer, bytes_read);

		/* make sure we have storage */
		if (!*buf) {
			*buf = net_buf_alloc(&uart_drv_recv_pool, NETBUF_ALLOC_TIMEOUT);
			if (!*buf) {
				SYS_LOG_ERR("Can't allocate RX data! "
					    "Skipping data!");
				break;
			}
		}

		rx_len = net_buf_append_bytes(*buf, bytes_read, uart_buffer,
					      NETBUF_ALLOC_TIMEOUT,
					      read_rx_allocator,
					      &uart_drv_recv_pool);
		if (rx_len < bytes_read) {
			SYS_LOG_ERR("Data was lost! read %u of %u!",
				    rx_len, bytes_read);
		}
	}
}


/* RX thread */
static void uart_dev_rx(void)
{
	struct net_buf *rx_buf = NULL;
	struct net_buf *frag = NULL;
	int i;
	u16_t offset, len;


	while (true) {
		/* wait for incoming data */
		k_sem_take(&ictx.drv_ctx.rx_sem, K_FOREVER);

		uart_dev_read_rx(&rx_buf);

		while (rx_buf) {
			net_buf_skipcrlf(&rx_buf);
			if (!rx_buf) {
				break;
			}

			frag = NULL;
			len = net_buf_findcrlf(rx_buf, &frag, &offset);
			if (!frag) {
				break;
			}

			/* look for matching data handlers */
			i = -1;
			for (i = 0; i < ARRAY_SIZE(handlers); i++) {
				if (net_buf_ncmp(rx_buf, handlers[i].cmd,
						 *handlers[i].cmd_len) == 0) {
					/* found a matching handler */
					SYS_LOG_DBG("MATCH %s (len:%u)",
						    handlers[i].cmd, len);

					/* skip cmd_len */
					rx_buf = net_buf_skip(rx_buf,
							handlers[i].cmd_len);

					/* locate next cr/lf */
					frag = NULL;
					len = net_buf_findcrlf(rx_buf,
							       &frag, &offset);
					if (!frag) {
						break;
					}

					/* call handler */
					if (handlers[i].func) {
						handlers[i].func(&rx_buf, len);
					}

					frag = NULL;
					/* make sure buf still has data */
					if (!rx_buf) {
						break;
					}

					/* locate next cr/lf */
					len = net_buf_findcrlf(rx_buf,
							       &frag, &offset);
					if (!frag) {
						break;
					}

					break;
				}
			}

			if (frag && rx_buf) {
				/* clear out processed line (buffers) */
				while (frag && rx_buf != frag) {
					rx_buf = net_buf_frag_del(NULL, rx_buf);
				}

				net_buf_pull(rx_buf, offset);
			}
		}

		/* give up time if we have a solid stream of data */
		k_yield();
	}
}

#else


static size_t uart_dev_read_rx(struct uart_dev_ctx *ictx, u8_t* uart_buffer)
{
    int ret = 0;
    size_t bytes_read = 0;
    while (true) {
        ret = uart_drv_recv(&ictx->drv_ctx,
                            uart_buffer,
                            sizeof(uart_buffer),
                            &bytes_read);
        if (ret < 0) {
            /* buffer is empty */
            return 0;
        }

        return bytes_read;
    }
}

static int uart_dev_process_line(struct uart_dev_ctx *ictx, char line[], u16_t line_len)
{

    SYS_LOG_DBG("[%s] IN (%u b): [%s]", ictx->drv_ctx.uart_dev->config->name, strlen(line), line);


    int ret_handled = 1;

    if(ret_handled > 0)
    {
        if(ictx->cmd_resp_handler)
        {
            ret_handled = ictx->cmd_resp_handler(line, line_len);
            SYS_LOG_DBG("cmd_resp_handler returned %u", ret_handled);
            if(ret_handled == 0)
            {
                ictx->cmd_resp_handler = NULL;
            }
        }

    }

    for (int i = 0; i < ictx->command_handler_cnt; ++i) {
        struct cmd_handler handler = ictx->command_handlers[i];

        if (strncmp(line, handler.cmd,
                    handler.cmd_len) == 0) {

            char cmd[256];
            u16_t cmd_len = (u16_t) line_len-handler.cmd_len;
            memcpy(cmd, &line[handler.cmd_len+1], cmd_len);
            ret_handled = handler.func(cmd, cmd_len);
            break;
        }
    }

    if(ret_handled > 0)
    {
        if(ictx->generic_resp_handler)
        {
            ret_handled = ictx->generic_resp_handler(line, line_len);
        }

    }

    return ret_handled;
}
/* RX thread */
static void uart_dev_rx(struct uart_dev_ctx *ictx)
{
    int ret = 0;

    size_t bytes_read = 0;
    u8_t uart_buffer[1024];
    u8_t temp_buffer[256];

    u16_t line_len = 0;
    u16_t test_line_len = 0;

    while (ret == 0) {
        k_sem_take(&ictx->drv_ctx.rx_sem, K_FOREVER);
SYS_LOG_DBG("RX SEM taken");
        bytes_read = uart_dev_read_rx(ictx, uart_buffer);


        while (bytes_read) {
            memcpy(&temp_buffer[line_len], &uart_buffer, bytes_read);

            line_len += bytes_read;

            test_line_len = line_len-(ictx->linebreak_len);

            /* check if there is a newline at end of input */
            if(memcmp(&temp_buffer[test_line_len],ictx->linebreak_constant, ictx->linebreak_len) == 0)
            {
                u8_t line[256];
                memcpy(line, temp_buffer, test_line_len);
                line[test_line_len] = '\0';


                u16_t chunk_start_idx = 0;
                /* check if there are skipped new lines */
                for(u16_t char_idx = 0; char_idx < ARRAY_SIZE(line) - ictx->linebreak_len; char_idx++)
                {

                    if(memcmp(&temp_buffer[char_idx],ictx->linebreak_constant, ictx->linebreak_len) == 0)
                    {
                        u16_t chunk_len = char_idx-chunk_start_idx;
                        if(chunk_len > 0)
                        {
                            char* chunk = k_malloc(chunk_len);
                            chunk[chunk_len] = '\0';
                            memcpy(chunk, &temp_buffer[chunk_start_idx], chunk_len);
                            int ret_handled = uart_dev_process_line(ictx, chunk, chunk_len);

                            if(ret_handled == 0)
                            {
                                SYS_LOG_WRN("UART handled");
                                // TODO if (!sock) {
                                k_sem_give(&ictx->response_sem);
                                SYS_LOG_DBG("%s Sem count (%u)", ictx->drv_ctx.uart_dev->config->name,
                                        k_sem_count_get(&ictx->response_sem));
                                //} else {
                                //    k_sem_give(&sock->sock_send_sem);
                                //}
                            } else {
                                SYS_LOG_WRN("UART not handled");
                            }
                            k_free(chunk);
                        }

                        chunk_start_idx = char_idx + ictx->linebreak_len;
                    }
                }


                memset(temp_buffer, 0, sizeof(temp_buffer));
                line_len = 0;
                break;
            }


            bytes_read = uart_dev_read_rx(ictx, uart_buffer);
        }

        k_yield();
    }

}

#endif


int uart_dev_init(struct uart_dev_ctx *dev_ctx, struct device *uart_dev)
{


    int i, ret = 0;

    //memset(&ictx, 0, sizeof(ictx));
    k_sem_init(&dev_ctx->response_sem, 0, 1);

    // TODO
    /*
    dev_ctx->command_handlers = command_handlers;
    dev_ctx->command_handler_cnt = cmd_handler_cnt;
    dev_ctx->generic_resp_handler = resp_handler;
*/

    /* initialize the work queue */
    k_work_q_start(&dev_ctx->workq,
                   dev_ctx->workq_stack,
                   K_THREAD_STACK_SIZEOF(dev_ctx->workq_stack),
                   K_PRIO_COOP(7));

    // TODO:
    dev_ctx->linebreak_constant = "\r\n";
    dev_ctx->linebreak_len = 2;

/* setup port devices and pin directions */
    // TODO
    /*
    ictx.last_socket_id = 0;

    for (i = 0; i < MAX_MDM_CONTROL_PINS; i++) {
        ictx.gpio_port_dev[i] =
                device_get_binding(pinconfig[i].dev_name);
        if (!ictx.gpio_port_dev[i]) {
            SYS_LOG_ERR("gpio port (%s) not found!",
                        pinconfig[i].dev_name);
            return -ENODEV;
        }

        gpio_pin_configure(ictx.gpio_port_dev[i], pinconfig[i].pin,
                           GPIO_DIR_OUT);
    }
    */

    ret = uart_drv_register(&dev_ctx->drv_ctx, uart_dev->config->name,
                                dev_ctx->recv_buf, sizeof(dev_ctx->recv_buf));
    if (ret < 0) {
        SYS_LOG_ERR("Error registering modem receiver (%d)!", ret);
        goto error;
    }
    /* start RX thread */
    k_thread_create(&dev_ctx->rx_thread, dev_ctx->rx_thread_stack,
                    K_THREAD_STACK_SIZEOF(dev_ctx->rx_thread_stack),
                    (k_thread_entry_t) uart_dev_rx,
                    dev_ctx, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

    error:
    return ret;
}
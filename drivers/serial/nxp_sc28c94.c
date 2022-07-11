/*
 * Copyright (c) 2022 Trackunit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/************************************************************************ 
 * Dependencies
 ************************************************************************/
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include "zephyr/drivers/spi.h"
#include "zephyr/drivers/uart.h"

/************************************************************************ 
 * Definitions
 ************************************************************************/

/************************************************************************ 
 * Register driver to log module
 ************************************************************************/
LOG_MODULE_REGISTER(nxp_sc28c94, LOG_LEVEL_INF);

/************************************************************************ 
 * Configuration structures
 ************************************************************************/
struct nxp_sc28c94_cfg {
    const struct device *uart0;
    const struct device *uart1;
    const struct device *uart2;
    const struct device *uart3;
    const struct device *bus;
};

struct nxp_sc28c94_uart_cfg {
    const struct device *parent;
    uint32_t current_speed;
    bool hw_flow_control;
};

/************************************************************************ 
 * UART device API
 ************************************************************************/
int nxp_sc28c94_poll_in(const struct device *parent,
        const struct device *child, unsigned char *p_char)
{
    /* ... */
    /* ... */
    /* ... */
    return 0;
}

void nxp_sc28c94_poll_out(const struct device *parent,
        const struct device *child, , unsigned char out_char)
{
    /* ... */
    /* ... */
    /* ... */
    return 0;
}

/************************************************************************ 
 * UART child device API
 ************************************************************************/
int nxp_sc28c94_uart_poll_in(const struct device *dev, unsigned char *p_char)
{
    nxp_sc28c94_uart_cfg cfg = (nxp_sc28c94_uart_cfg *)dev->cfg;
    return nxp_sc28c94_poll_in(cfg->parent, dev, p_char);
}

void nxp_sc28c94_uart_poll_out(const struct device *parent, unsigned char out_char)
{
    nxp_sc28c94_uart_cfg cfg = (nxp_sc28c94_uart_cfg *)dev->cfg;
    return nxp_sc28c94_poll_out(cfg->parent, dev, out_char);}

struct uart_driver_api nxp_sc28c94_uart_api {
    .nxp_sc28c94_uart_poll_in,
    .nxp_sc28c94_uart_poll_out,
};

/************************************************************************ 
 * Initialization
 ************************************************************************/
static int nxp_sc28c94_init(const struct device *dev)
{
    return 0;
}

/************************************************************************ 
 * Instanciation macro for child device
 ************************************************************************/
#define NXP_SC29C94_UART_DEVICE(id)                                                             \
    static struct nxp_sc28c94_uart_cfg nxp_sc28c94_uart_cfg_##id {                              \
        parent = DT_PARENT(id, current_speed);                                                  \
        current_speed = DT_PROP(id, current_speed);                                             \
        hw_flow_control = DT_PROP(id, hw_flow_control);                                         \
    };                                                                                          \
                                                                                                \
    DEVICE_DT_DEFINE(id, NULL, NULL, NULL, nxp_sc28c94_uart_cfg_##id, POST_KERNEL, 70,          \
            nxp_sc28c94_uart_api);

/************************************************************************ 
 * Instanciation macro for parent device
 ************************************************************************/
#define NXP_SC29C94_DEVICE(id)                                                                  \
    static struct nxp_sc28c94_cfg nxp_sc28c94_cfg_##id = {	                                    \
        .uart0 = DEVICE_CHILD_DT_GET_OR_NULL(id, uart0),                                        \
        .uart1 = DEVICE_CHILD_DT_GET_OR_NULL(id, uart1),                                        \
        .uart2 = DEVICE_CHILD_DT_GET_OR_NULL(id, uart2),                                        \
        .uart3 = DEVICE_CHILD_DT_GET_OR_NULL(id, uart3),                                        \
        .bus = DEVICE_DT_GET(DT_BUS(id))                                                        \
    };                                                                                          \
                                                                                                \
    DEVICE_DT_DEFINE(id, nxp_sc28c94_init, NULL, NULL,                                          \
        quectel_bg95_modem_##id##_cfg, POST_KERNEL, 10, NULL);

/************************************************************************ 
 * Instanciation
 ************************************************************************/
DT_FOREACH_STATUS_OKAY(nxp_sc28c94_uart, NXP_SC29C94_UART_DEVICE)
DT_FOREACH_STATUS_OKAY(nxp_sc28c94, NXP_SC29C94_DEVICE)

/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal APIs for UART drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_UART_INTERNAL_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_UART_INTERNAL_H_

#include <errno.h>
#include <stddef.h>

#include <zephyr/device.h>

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief For configuring IRQ on each individual UART device.
 *
 * @param dev UART device instance.
 */
typedef void (*uart_irq_config_func_t)(const struct device *dev);

/** @brief Driver API structure. */
__subsystem struct uart_driver_api {

#ifdef CONFIG_UART_ASYNC_API

	int (*callback_set)(const struct device *dev, uart_callback_t callback, void *user_data);

	int (*tx)(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout);
	int (*tx_abort)(const struct device *dev);

	int (*rx_enable)(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout);
	int (*rx_buf_rsp)(const struct device *dev, uint8_t *buf, size_t len);
	int (*rx_disable)(const struct device *dev);

#ifdef CONFIG_UART_WIDE_DATA
	int (*tx_u16)(const struct device *dev, const uint16_t *buf, size_t len, int32_t timeout);
	int (*rx_enable_u16)(const struct device *dev, uint16_t *buf, size_t len, int32_t timeout);
	int (*rx_buf_rsp_u16)(const struct device *dev, uint16_t *buf, size_t len);
#endif

#endif

	/** Console I/O function */
	int (*poll_in)(const struct device *dev, unsigned char *p_char);
	void (*poll_out)(const struct device *dev, unsigned char out_char);

#ifdef CONFIG_UART_WIDE_DATA
	int (*poll_in_u16)(const struct device *dev, uint16_t *p_u16);
	void (*poll_out_u16)(const struct device *dev, uint16_t out_u16);
#endif

	/** Console I/O function */
	int (*err_check)(const struct device *dev);

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	/** UART configuration functions */
	int (*configure)(const struct device *dev, const struct uart_config *cfg);
	int (*config_get)(const struct device *dev, struct uart_config *cfg);
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	/** Interrupt driven FIFO fill function */
	int (*fifo_fill)(const struct device *dev, const uint8_t *tx_data, int len);

#ifdef CONFIG_UART_WIDE_DATA
	int (*fifo_fill_u16)(const struct device *dev, const uint16_t *tx_data, int len);
#endif

	/** Interrupt driven FIFO read function */
	int (*fifo_read)(const struct device *dev, uint8_t *rx_data, const int size);

#ifdef CONFIG_UART_WIDE_DATA
	int (*fifo_read_u16)(const struct device *dev, uint16_t *rx_data, const int size);
#endif

	/** Interrupt driven transfer enabling function */
	void (*irq_tx_enable)(const struct device *dev);

	/** Interrupt driven transfer disabling function */
	void (*irq_tx_disable)(const struct device *dev);

	/** Interrupt driven transfer ready function */
	int (*irq_tx_ready)(const struct device *dev);

	/** Interrupt driven receiver enabling function */
	void (*irq_rx_enable)(const struct device *dev);

	/** Interrupt driven receiver disabling function */
	void (*irq_rx_disable)(const struct device *dev);

	/** Interrupt driven transfer complete function */
	int (*irq_tx_complete)(const struct device *dev);

	/** Interrupt driven receiver ready function */
	int (*irq_rx_ready)(const struct device *dev);

	/** Interrupt driven error enabling function */
	void (*irq_err_enable)(const struct device *dev);

	/** Interrupt driven error disabling function */
	void (*irq_err_disable)(const struct device *dev);

	/** Interrupt driven pending status function */
	int (*irq_is_pending)(const struct device *dev);

	/** Interrupt driven interrupt update function */
	int (*irq_update)(const struct device *dev);

	/** Set the irq callback function */
	void (*irq_callback_set)(const struct device *dev, uart_irq_callback_user_data_t cb,
				 void *user_data);

#endif

#ifdef CONFIG_UART_LINE_CTRL
	int (*line_ctrl_set)(const struct device *dev, uint32_t ctrl, uint32_t val);
	int (*line_ctrl_get)(const struct device *dev, uint32_t ctrl, uint32_t *val);
#endif

#ifdef CONFIG_UART_DRV_CMD
	int (*drv_cmd)(const struct device *dev, uint32_t cmd, uint32_t p);
#endif
};

static inline int z_impl_uart_err_check(const struct device *dev)
{
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->err_check == NULL) {
		return -ENOSYS;
	}

	return api->err_check(dev);
}

static inline int z_impl_uart_poll_in(const struct device *dev, unsigned char *p_char)
{
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->poll_in == NULL) {
		return -ENOSYS;
	}

	return api->poll_in(dev, p_char);
}

static inline int z_impl_uart_poll_in_u16(const struct device *dev, uint16_t *p_u16)
{
#ifdef CONFIG_UART_WIDE_DATA
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->poll_in_u16 == NULL) {
		return -ENOSYS;
	}

	return api->poll_in_u16(dev, p_u16);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(p_u16);
	return -ENOTSUP;
#endif
}

static inline void z_impl_uart_poll_out(const struct device *dev, unsigned char out_char)
{
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	api->poll_out(dev, out_char);
}

static inline void z_impl_uart_poll_out_u16(const struct device *dev, uint16_t out_u16)
{
#ifdef CONFIG_UART_WIDE_DATA
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	api->poll_out_u16(dev, out_u16);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(out_u16);
#endif
}

static inline int z_impl_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->configure == NULL) {
		return -ENOSYS;
	}
	return api->configure(dev, cfg);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->config_get == NULL) {
		return -ENOSYS;
	}

	return api->config_get(dev, cfg);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
#endif
}

static inline int uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->fifo_fill == NULL) {
		return -ENOSYS;
	}

	return api->fifo_fill(dev, tx_data, size);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(tx_data);
	ARG_UNUSED(size);
	return -ENOTSUP;
#endif
}

static inline int uart_fifo_fill_u16(const struct device *dev, const uint16_t *tx_data, int size)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_UART_WIDE_DATA)
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->fifo_fill_u16 == NULL) {
		return -ENOSYS;
	}

	return api->fifo_fill_u16(dev, tx_data, size);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(tx_data);
	ARG_UNUSED(size);
	return -ENOTSUP;
#endif
}

static inline int uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->fifo_read == NULL) {
		return -ENOSYS;
	}

	return api->fifo_read(dev, rx_data, size);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_data);
	ARG_UNUSED(size);
	return -ENOTSUP;
#endif
}

static inline int uart_fifo_read_u16(const struct device *dev, uint16_t *rx_data, const int size)
{
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_UART_WIDE_DATA)
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->fifo_read_u16 == NULL) {
		return -ENOSYS;
	}

	return api->fifo_read_u16(dev, rx_data, size);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_data);
	ARG_UNUSED(size);
	return -ENOTSUP;
#endif
}

static inline void z_impl_uart_irq_tx_enable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_tx_enable != NULL) {
		api->irq_tx_enable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static inline void z_impl_uart_irq_tx_disable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_tx_disable != NULL) {
		api->irq_tx_disable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static inline int uart_irq_tx_ready(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_tx_ready == NULL) {
		return -ENOSYS;
	}

	return api->irq_tx_ready(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

static inline void z_impl_uart_irq_rx_enable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_rx_enable != NULL) {
		api->irq_rx_enable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static inline void z_impl_uart_irq_rx_disable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_rx_disable != NULL) {
		api->irq_rx_disable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static inline int uart_irq_tx_complete(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_tx_complete == NULL) {
		return -ENOSYS;
	}
	return api->irq_tx_complete(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

static inline int uart_irq_rx_ready(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_rx_ready == NULL) {
		return -ENOSYS;
	}
	return api->irq_rx_ready(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

static inline void z_impl_uart_irq_err_enable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_err_enable) {
		api->irq_err_enable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static inline void z_impl_uart_irq_err_disable(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_err_disable) {
		api->irq_err_disable(dev);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static inline int z_impl_uart_irq_is_pending(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_is_pending == NULL) {
		return -ENOSYS;
	}
	return api->irq_is_pending(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_irq_update(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->irq_update == NULL) {
		return -ENOSYS;
	}
	return api->irq_update(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

static inline int uart_irq_callback_user_data_set(const struct device *dev,
						  uart_irq_callback_user_data_t cb, void *user_data)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if ((api != NULL) && (api->irq_callback_set != NULL)) {
		api->irq_callback_set(dev, cb, user_data);
		return 0;
	} else {
		return -ENOSYS;
	}
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
	return -ENOTSUP;
#endif
}

static inline int uart_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb)
{
	return uart_irq_callback_user_data_set(dev, cb, NULL);
}

static inline int uart_callback_set(const struct device *dev, uart_callback_t callback,
				    void *user_data)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->callback_set == NULL) {
		return -ENOSYS;
	}

	return api->callback_set(dev, callback, user_data);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_tx(const struct device *dev, const uint8_t *buf, size_t len,
				 int32_t timeout)

{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	return api->tx(dev, buf, len, timeout);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_tx_u16(const struct device *dev, const uint16_t *buf, size_t len,
				     int32_t timeout)

{
#if defined(CONFIG_UART_ASYNC_API) && defined(CONFIG_UART_WIDE_DATA)
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	return api->tx_u16(dev, buf, len, timeout);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_tx_abort(const struct device *dev)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	return api->tx_abort(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
					int32_t timeout)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	return api->rx_enable(dev, buf, len, timeout);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_rx_enable_u16(const struct device *dev, uint16_t *buf, size_t len,
					    int32_t timeout)
{
#if defined(CONFIG_UART_ASYNC_API) && defined(CONFIG_UART_WIDE_DATA)
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	return api->rx_enable_u16(dev, buf, len, timeout);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
#endif
}

static inline int uart_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	return api->rx_buf_rsp(dev, buf, len);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -ENOTSUP;
#endif
}

static inline int uart_rx_buf_rsp_u16(const struct device *dev, uint16_t *buf, size_t len)
{
#if defined(CONFIG_UART_ASYNC_API) && defined(CONFIG_UART_WIDE_DATA)
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	return api->rx_buf_rsp_u16(dev, buf, len);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_rx_disable(const struct device *dev)
{
#ifdef CONFIG_UART_ASYNC_API
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	return api->rx_disable(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_line_ctrl_set(const struct device *dev, uint32_t ctrl, uint32_t val)
{
#ifdef CONFIG_UART_LINE_CTRL
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->line_ctrl_set == NULL) {
		return -ENOSYS;
	}
	return api->line_ctrl_set(dev, ctrl, val);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(ctrl);
	ARG_UNUSED(val);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_line_ctrl_get(const struct device *dev, uint32_t ctrl, uint32_t *val)
{
#ifdef CONFIG_UART_LINE_CTRL
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->line_ctrl_get == NULL) {
		return -ENOSYS;
	}
	return api->line_ctrl_get(dev, ctrl, val);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(ctrl);
	ARG_UNUSED(val);
	return -ENOTSUP;
#endif
}

static inline int z_impl_uart_drv_cmd(const struct device *dev, uint32_t cmd, uint32_t p)
{
#ifdef CONFIG_UART_DRV_CMD
	const struct uart_driver_api *api = (const struct uart_driver_api *)dev->api;

	if (api->drv_cmd == NULL) {
		return -ENOSYS;
	}
	return api->drv_cmd(dev, cmd, p);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cmd);
	ARG_UNUSED(p);
	return -ENOTSUP;
#endif
}

#ifdef __cplusplus
}
#endif

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_UART_INTERNAL_H_ */

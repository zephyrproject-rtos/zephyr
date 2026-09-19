/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SDIO device/slave-role API
 *
 * Device-role model layered on top of an SDIO device controller
 * (@ref sdio_dc_interface). It lets an application expose SDIO functions to a
 * remote host.
 */

#ifndef ZEPHYR_INCLUDE_SDIO_SDIO_DEVICE_H_
#define ZEPHYR_INCLUDE_SDIO_SDIO_DEVICE_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sd/sd_spec.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SDIO device/slave-role abstraction
 * @defgroup sdio_device SDIO device/slave-role abstraction
 * @ingroup io_interfaces
 * @{
 */

struct sdio_device_function;

/**
 * @brief Function-0 / card identity configuration.
 *
 * Supplied by the vendor. The subsystem uses it to serve the function-0 CCCR,
 * the FBR of each function and the CIS tuple chains when a host reads them.
 * Pass to @ref sdio_device_init, or NULL if the controller/hardware serves the
 * function-0 register file itself.
 */
struct sdio_device_config {
	uint8_t cccr_revision; /**< CCCR/SDIO revision (@c SDIO_CCCR_CCCR_REV_*) */
	uint8_t sd_spec; /**< SD physical spec version code */
	uint16_t manf_id; /**< CIS manufacturer ID (MANFID tuple) */
	uint16_t manf_code; /**< CIS manufacturer code (MANFID tuple) */
	uint8_t func0_id; /**< CIS function ID of function 0 (FUNCID tuple) */
	uint16_t max_blk_size; /**< Function-0 maximum block size (FUNCE tuple) */
	uint8_t max_speed; /**< Function-0 maximum transfer rate code (FUNCE) */
	uint8_t caps; /**< CCCR capability bits (@c SDIO_CCCR_CAPS_*) */
};

/**
 * @brief FIFO / data-port handler.
 *
 * Invoked when the host performs a fixed-address (FIFO) access to the
 * function's configured data-port register. For a read the handler fills
 * @p data with @p len bytes; for a write it consumes them.
 *
 * @param func function being accessed
 * @param dir  access direction
 * @param data data buffer to fill (read) or consume (write)
 * @param len  number of bytes
 * @param user user data registered with the function
 * @retval 0 on success, negative errno to report an I/O error to the host
 */
typedef int (*sdio_device_fifo_cb_t)(struct sdio_device_function *func,
				     enum sdio_io_dir dir, uint8_t *data,
				     uint32_t len, void *user);

/**
 * @brief Device-side SDIO function.
 *
 * Configure @ref num and at least one of a register window (@ref regs /
 * @ref regs_size) or a FIFO handler (@ref fifo_reg / @ref fifo_cb), then
 * register it with @ref sdio_device_register_function.
 */
struct sdio_device_function {
	/** Function number exposed to the host */
	enum sdio_func_num num;
	/** Backing store for incrementing-address accesses (may be NULL) */
	uint8_t *regs;
	/** Size of @ref regs in bytes */
	size_t regs_size;
	/** Register offset mapped to the FIFO/data port (when @ref fifo_cb set) */
	uint32_t fifo_reg;
	/** Handler for fixed-address FIFO/data-port accesses (may be NULL) */
	sdio_device_fifo_cb_t fifo_cb;
	/** User data passed to @ref fifo_cb */
	void *user;

	/** Standard SDIO function interface code (FBR / CIS FUNCID) */
	uint8_t func_code;
	/** Maximum block size advertised for this function (CIS FUNCE) */
	uint16_t max_blk_size;
	/** I/O-ready timeout advertised in the CIS in 10ms units (FUNCE) */
	uint16_t rdy_timeout;

	/**
	 * Zero-copy RX completion: a buffer posted with
	 * @ref sdio_device_rx_post now holds @p len bytes (may be NULL).
	 */
	void (*rx_done)(struct sdio_device_function *func, uint8_t *buf,
			uint32_t len);
	/**
	 * Zero-copy TX completion: a buffer submitted with
	 * @ref sdio_device_tx_submit has been consumed (may be NULL).
	 */
	void (*tx_done)(struct sdio_device_function *func, uint8_t *buf);

	/** @cond INTERNAL_HIDDEN */
	uint16_t block_size; /* host-programmed block size (FBR) */
	sys_snode_t node;
	struct sdio_device *parent;
	/** @endcond */
};

/**
 * @brief Device-role SDIO endpoint.
 *
 * Binds a set of @ref sdio_device_function instances to an SDIO device
 * controller.
 */
struct sdio_device {
	/** SDIO device controller backing this endpoint */
	const struct device *controller;
	/** Function-0 configuration served by the subsystem (may be NULL) */
	const struct sdio_device_config *config;
	/** @cond INTERNAL_HIDDEN */
	sys_slist_t functions;
	struct k_mutex lock;
	uint8_t io_enable; /* CCCR I/O enable bitmap */
	uint8_t io_ready; /* CCCR I/O ready bitmap */
	uint8_t int_enable; /* CCCR interrupt enable bitmap */
	uint8_t int_pending; /* CCCR interrupt pending bitmap */
	uint8_t bus_width; /* CCCR bus interface width setting */
	uint8_t speed_sel; /* CCCR bus speed selection */
	/** @endcond */
};

/**
 * @brief Initialize a device-role SDIO endpoint.
 *
 * When @p config is non-NULL the subsystem serves the function-0 register file
 * (CCCR/FBR/CIS) and tracks enable/interrupt state on the host's behalf. Pass
 * NULL if the controller or hardware serves function 0 itself.
 *
 * @param dev        endpoint to initialize
 * @param controller SDIO device controller device
 * @param config     function-0 configuration, or NULL
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 */
int sdio_device_init(struct sdio_device *dev, const struct device *controller,
		     const struct sdio_device_config *config);

/**
 * @brief Expose a function to the remote host.
 *
 * @param dev  device endpoint
 * @param func function description (must outlive the registration)
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 * @retval -EALREADY a function with the same number is already registered
 */
int sdio_device_register_function(struct sdio_device *dev,
				  struct sdio_device_function *func);

/**
 * @brief Start answering host accesses.
 *
 * @param dev device endpoint
 * @retval 0 on success, negative errno otherwise
 */
int sdio_device_enable(struct sdio_device *dev);

/**
 * @brief Stop answering host accesses.
 *
 * @param dev device endpoint
 * @retval 0 on success, negative errno otherwise
 */
int sdio_device_disable(struct sdio_device *dev);

/**
 * @brief Assert the SDIO interrupt towards the host on behalf of a function.
 *
 * @param func function asserting the interrupt
 * @retval 0 on success
 * @retval -ENOSYS controller cannot assert interrupts
 * @retval -EINVAL invalid argument
 */
int sdio_device_raise_interrupt(struct sdio_device_function *func);

/**
 * @brief Clear a pending SDIO interrupt for a function.
 *
 * Clears the function's bit in the CCCR interrupt-pending register. Call once
 * the condition that raised the interrupt has been serviced.
 *
 * @param func function whose interrupt to clear
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 */
int sdio_device_clear_interrupt(struct sdio_device_function *func);

/**
 * @brief Whether the backing controller offers a zero-copy data path.
 *
 * @param dev device endpoint
 * @retval true controller supports @ref sdio_device_rx_post /
 *         @ref sdio_device_tx_submit
 * @retval false only the synchronous FIFO handler path is available
 */
bool sdio_device_is_zero_copy(struct sdio_device *dev);

/**
 * @brief Post an empty buffer to receive an inbound frame (zero-copy).
 *
 * The controller takes ownership until it fills the buffer, then calls the
 * function's @ref sdio_device_function.rx_done.
 *
 * @param func function to receive on
 * @param buf  buffer the controller may write into
 * @param cap  capacity of @p buf in bytes
 * @retval 0 on success, negative errno otherwise
 */
int sdio_device_rx_post(struct sdio_device_function *func, uint8_t *buf,
			uint32_t cap);

/**
 * @brief Submit a filled buffer for the host to read (zero-copy).
 *
 * The controller takes ownership until the host reads the data, then calls the
 * function's @ref sdio_device_function.tx_done.
 *
 * @param func function to send from
 * @param buf  buffer holding the data
 * @param len  number of bytes in @p buf
 * @retval 0 on success, negative errno otherwise
 */
int sdio_device_tx_submit(struct sdio_device_function *func, uint8_t *buf,
			  uint32_t len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SDIO_SDIO_DEVICE_H_ */

/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Role-neutral SDIO core abstraction
 *
 * This header defines a standalone SDIO abstraction.
 * It models SDIO functions, function I/O space, register access, FIFO/data-port access,
 * fixed-address vs incrementing-address transfers and host/device role separation.
 *
 * The core is transport-neutral: an @ref sdio_dev is bound to a transport
 * backend (host controller, device controller or an in-process loopback) that
 * implements the low level CMD52/CMD53 primitives. The function I/O helpers in
 * this header are shared by every role and backend.
 */

#ifndef ZEPHYR_INCLUDE_SDIO_SDIO_CORE_H_
#define ZEPHYR_INCLUDE_SDIO_SDIO_CORE_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sd/sd_spec.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SDIO core abstraction
 * @defgroup sdio_core SDIO core abstraction
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief SDIO role
 *
 * Role played by an SDIO endpoint on the bus. SDIO is an inherently
 * asymmetric bus: exactly one endpoint drives the clock and issues commands
 * (the host/master), and the other responds (the device/slave).
 */
enum sdio_role {
	/** Endpoint acts as the SDIO host/master (drives clock, issues CMD52/53) */
	SDIO_ROLE_HOST = 0,
	/** Endpoint acts as the SDIO device/slave (responds to the host) */
	SDIO_ROLE_DEVICE = 1,
};

/**
 * @brief SDIO core capability flags
 *
 * Capabilities of the bus as seen through an @ref sdio_dev. On the host role
 * these reflect the negotiated capabilities of the remote device; on the
 * device role they reflect what the local controller exposes.
 */
enum sdio_caps {
	/** High speed (SDR25) timing supported */
	SDIO_CAP_HS = BIT(0),
	/** Multi-block (CMD53 block mode) transfers supported */
	SDIO_CAP_MULTIBLOCK = BIT(1),
	/** 4-bit bus width supported */
	SDIO_CAP_4BIT_BUS = BIT(2),
	/** SPI command/response framing in use */
	SDIO_CAP_SPI = BIT(3),
};

struct sdio_dev;

/**
 * @brief Extended (CMD53) transfer descriptor
 *
 * Groups the parameters of an extended transfer so the transport layer and the
 * function I/O helpers can pass them as a single object.
 */
struct sdio_xfer {
	/** Function number to address */
	enum sdio_func_num func;
	/** Register offset within the function I/O space */
	uint32_t reg;
	/** true: incrementing-address; false: fixed-address (FIFO/data port) */
	bool increment;
	/** Data buffer */
	uint8_t *buf;
	/** Number of blocks (0 selects byte mode) */
	uint32_t blocks;
	/** Block/byte size */
	uint32_t block_size;
};

/**
 * @brief SDIO transport backend API
 *
 * Low level primitives that move bytes/blocks across the SDIO bus, expressed
 * in terms of the two SDIO I/O commands. A transport backend is role and
 * hardware specific (host controller over @ref sdhc_interface, device controller over
 * @ref sdio_dc_interface, or an in-process loopback); the role-neutral function I/O
 * helpers are layered on top of this vtable.
 *
 * Both callbacks are mandatory.
 */
struct sdio_transport_api {
	/**
	 * @brief Direct (CMD52) single byte register access.
	 *
	 * @param dev      SDIO device
	 * @param dir      transfer direction
	 * @param func     function number to address
	 * @param reg      register offset within the function I/O space
	 * @param data_in  byte to write (write/RAW direction)
	 * @param data_out if non-NULL, filled with the byte read back
	 * @retval 0 on success, negative errno otherwise
	 */
	int (*rw_direct)(struct sdio_dev *dev, enum sdio_io_dir dir,
			 enum sdio_func_num func, uint32_t reg,
			 uint8_t data_in, uint8_t *data_out);
	/**
	 * @brief Extended (CMD53) multi byte/block access.
	 *
	 * @param dev  SDIO device
	 * @param dir  transfer direction
	 * @param xfer extended transfer descriptor
	 * @retval 0 on success, negative errno otherwise
	 */
	int (*rw_extended)(struct sdio_dev *dev, enum sdio_io_dir dir,
			   const struct sdio_xfer *xfer);
};

/**
 * @brief Role-neutral SDIO device
 *
 * Represents one endpoint of an SDIO bus.
 * This structure is usable for host transports, device/slave transports and
 * non-card companion transports.
 *
 * Applications normally obtain a fully initialized instance from a role
 * specific helper (e.g. @ref sdio_host_init) rather than populating it by
 * hand.
 */
struct sdio_dev {
	/** Role played by this endpoint */
	enum sdio_role role;
	/** Transport backend vtable */
	const struct sdio_transport_api *transport;
	/** Backend private context (e.g. const struct device *sdhc) */
	const void *transport_ctx;
	/** Bus capability flags, see @ref sdio_caps */
	uint32_t caps;
	/** Serializes access to the bus */
	struct k_mutex lock;
	/** Maximum single byte-mode transfer length (CIS func0 max block size) */
	uint16_t max_blk_size;
};

/**
 * @brief Role-neutral SDIO function
 *
 * Models a single SDIO function (0-7) and its I/O space. The function refers
 * to the owning @ref sdio_dev.
 */
struct sdio_function {
	/** Owning SDIO endpoint */
	struct sdio_dev *dev;
	/** Function number */
	enum sdio_func_num num;
	/** CIS tuple data for this function (host role) */
	struct sdio_cis cis;
	/** Current block size used for block-mode transfers */
	uint16_t block_size;
};

/**
 * @brief Low level initialization of an SDIO endpoint.
 *
 * Installs a transport backend on @p dev and initializes its lock. Role
 * specific helpers such as @ref sdio_host_init are thin wrappers around this
 * function; transports other than the built-in host/device backends (for
 * example an in-process loopback used for testing) bind through it directly.
 *
 * @param dev       endpoint to initialize
 * @param role      role played by this endpoint
 * @param transport transport backend vtable (must outlive @p dev)
 * @param ctx       backend private context
 * @param caps      bus capability flags (see @ref sdio_caps)
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 */
int sdio_dev_init(struct sdio_dev *dev, enum sdio_role role,
		  const struct sdio_transport_api *transport, const void *ctx,
		  uint32_t caps);

/**
 * @brief Bind a function structure to an SDIO endpoint.
 *
 * Associates @p func with @p dev and function number @p num. This does not
 * perform any bus traffic; on the host role @ref sdio_func_bind may be used to
 * additionally read the function CIS.
 *
 * @param dev  SDIO endpoint
 * @param func function structure to initialize
 * @param num  function number
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 */
int sdio_func_bind(struct sdio_dev *dev, struct sdio_function *func,
		   enum sdio_func_num num);

/**
 * @brief Set the block size used for block-mode transfers on a function.
 *
 * @param func  function to configure
 * @param bsize block size in bytes
 * @retval 0 on success
 * @retval -EINVAL unsupported block size
 * @retval -EIO I/O error
 */
int sdio_func_set_block_size(struct sdio_function *func, uint16_t bsize);

/**
 * @brief Read a single byte from a function register (CMD52).
 *
 * @param func function to read from
 * @param reg  register offset
 * @param val  filled with the byte read
 * @retval 0 on success, negative errno otherwise
 */
int sdio_func_read_reg(struct sdio_function *func, uint32_t reg, uint8_t *val);

/**
 * @brief Write a single byte to a function register (CMD52).
 *
 * @param func function to write to
 * @param reg  register offset
 * @param val  byte to write
 * @retval 0 on success, negative errno otherwise
 */
int sdio_func_write_reg(struct sdio_function *func, uint32_t reg, uint8_t val);

/**
 * @brief Write then read back a function register (CMD52 RAW).
 *
 * @param func     function to access
 * @param reg      register offset
 * @param write_val byte to write
 * @param read_val  filled with the byte read back
 * @retval 0 on success, negative errno otherwise
 */
int sdio_func_rw_reg(struct sdio_function *func, uint32_t reg,
		     uint8_t write_val, uint8_t *read_val);

/**
 * @brief Read bytes from a fixed-address data port / FIFO (CMD53).
 *
 * All reads target the same address (no auto-increment).
 *
 * @param func function to read from
 * @param reg  data-port register offset
 * @param data destination buffer
 * @param len  number of bytes to read
 * @retval 0 on success, negative errno otherwise
 */
int sdio_func_read_fifo(struct sdio_function *func, uint32_t reg, uint8_t *data,
			uint32_t len);

/**
 * @brief Write bytes to a fixed-address data port / FIFO (CMD53).
 *
 * @param func function to write to
 * @param reg  data-port register offset
 * @param data source buffer
 * @param len  number of bytes to write
 * @retval 0 on success, negative errno otherwise
 */
int sdio_func_write_fifo(struct sdio_function *func, uint32_t reg,
			 uint8_t *data, uint32_t len);

/**
 * @brief Read whole blocks from a fixed-address data port / FIFO (CMD53).
 *
 * @param func   function to read from
 * @param reg    data-port register offset
 * @param data   destination buffer
 * @param blocks number of blocks to read
 * @retval 0 on success, negative errno otherwise
 */
int sdio_func_read_blocks_fifo(struct sdio_function *func, uint32_t reg,
			       uint8_t *data, uint32_t blocks);

/**
 * @brief Write whole blocks to a fixed-address data port / FIFO (CMD53).
 *
 * @param func   function to write to
 * @param reg    data-port register offset
 * @param data   source buffer
 * @param blocks number of blocks to write
 * @retval 0 on success, negative errno otherwise
 */
int sdio_func_write_blocks_fifo(struct sdio_function *func, uint32_t reg,
				uint8_t *data, uint32_t blocks);

/**
 * @brief Read bytes from an incrementing-address window (CMD53).
 *
 * @param func function to read from
 * @param reg  starting register offset
 * @param data destination buffer
 * @param len  number of bytes to read
 * @retval 0 on success, negative errno otherwise
 */
int sdio_func_read_addr(struct sdio_function *func, uint32_t reg, uint8_t *data,
			uint32_t len);

/**
 * @brief Write bytes to an incrementing-address window (CMD53).
 *
 * @param func function to write to
 * @param reg  starting register offset
 * @param data source buffer
 * @param len  number of bytes to write
 * @retval 0 on success, negative errno otherwise
 */
int sdio_func_write_addr(struct sdio_function *func, uint32_t reg,
			 uint8_t *data, uint32_t len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SDIO_SDIO_CORE_H_ */
